/**
 * @file serial.c
 * @author salalei (1028609078@qq.com)
 * @brief 串口驱动框架
 * @version 0.1
 * @date 2021-12-03
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "serial.h"
#include "../modules/list.h"
#include "../modules/object.h"
#include "dbg.h"

#include "semphr.h"
#include "task.h"

#include <string.h>

static struct list_node head = LIST_HEAD_INIT_INS(head);

static void exec_user_cb(struct serial *serial, int type, void *arg)
{
    void (*cb)(void *priv, int type, void *arg);
    void *priv;

    do
    {
        cb = serial->user_cb;
        priv = serial->priv;
    }
    while (cb != serial->user_cb || priv != serial->priv);
    if (cb)
        cb(priv, type, arg);
}

static ssize_t fifo_irq_send(struct serial *serial)
{
    size_t size;
    uint32_t temp;

    temp = taskENTER_CRITICAL_FROM_ISR();
    fifo_clear(&serial->send_fifo, serial->last_send_size);
    size = fifo_data_size(&serial->send_fifo);
    if (!size)
    {
        serial->last_send_size = 0;
        serial->send_fifo_empty = 1;
        taskEXIT_CRITICAL_FROM_ISR(temp);
        return 0;
    }
    else
    {
        size_t pos;
        ssize_t res;
        taskEXIT_CRITICAL_FROM_ISR(temp);
        pos = FIFO_READ_POS(serial->send_fifo.read_pos);
        if (pos + size >= serial->send_fifo.size)
            size = serial->send_fifo.size - pos;
        serial->last_send_size = size;
        res = serial->ops->irq_send(serial, &serial->send_fifo.buf[pos], size);
        if (res < 0)
            return res;
        return (ssize_t)size;
    }
}

/**
 * @brief 由底层驱动在数据发送完成中断调用
 *
 * @param serial 指向串口的指针
 */
void __serial_send_irq_handler(struct serial *serial)
{
    uint32_t temp;

    ASSERT(serial && serial->parent.drv_mode & __DRIVER_MODE_ASYNC_WRITE);
    if (fifo_irq_send(serial))
        return;
    temp = taskENTER_CRITICAL_FROM_ISR();
    if (serial->send_thread)
    {
        BaseType_t woken;
        vTaskNotifyGiveIndexedFromISR(serial->send_thread, 1, &woken);
        portYIELD_FROM_ISR(woken);
        taskEXIT_CRITICAL_FROM_ISR(temp);
    }
    else
    {
        taskEXIT_CRITICAL_FROM_ISR(temp);
        exec_user_cb(serial, USER_CB_TYPE_SEND_COMPLETE, NULL);
    }
}

/**
 * @brief 底层驱动接受到数据后，调用该接口将数据push到内部
 *
 * @param serial 指向串口的指针
 * @param buf 指向驱动接受的数据的指针
 * @param size 驱动接受到的数据大小
 */
void __serial_recv_push(struct serial *serial, const uint8_t *buf, size_t size)
{
    uint32_t temp;

    ASSERT(serial && buf && serial->parent.drv_mode & __DRIVER_MODE_ASYNC_READ);

    if (!size)
        return;
    fifo_push(&serial->recv_fifo, buf, size);
    temp = taskENTER_CRITICAL_FROM_ISR();
    if (serial->recv_thread)
    {
        BaseType_t woken;
        vTaskGenericNotifyGiveFromISR(serial->recv_thread, 1, &woken);
        portYIELD_FROM_ISR(woken);
        taskEXIT_CRITICAL_FROM_ISR(temp);
    }
    else
    {
        taskEXIT_CRITICAL_FROM_ISR(temp);
        exec_user_cb(serial, USER_CB_TYPE_RECV_COMPLETE, NULL);
    }
}

/**
 * @brief 向串口驱动框架注册一个串口
 *
 * @param serial 指向串口的指针
 * @param name 串口的名字
 * @param priv 指向驱动的私有数据
 * @param drv_mode 底层驱动给定的工作模式
 * @return int 返回0
 */
int __serial_register(struct serial *serial,
                      const char *name,
                      void *priv,
                      int drv_mode)
{
    ASSERT(serial && name && serial->ops && serial->ops->config && serial->ops->poll_get_char && serial->ops->poll_put_char);
    if (drv_mode & __DRIVER_MODE_ASYNC_WRITE)
    {
        ASSERT(serial->ops->irq_send && serial->ops->stop_send);
        drv_mode |= __DRIVER_MODE_WRITE;
    }
    if (drv_mode & __DRIVER_MODE_ASYNC_READ)
    {
        ASSERT(serial->ops->recv_ctrl);
        drv_mode |= __DRIVER_MODE_READ;
    }

    __driver_init(&serial->parent, name, priv, drv_mode);
    serial->send_busy = 0;
    serial->recv_busy = 0;
    serial->send_fifo_empty = 1;
    serial->conf.baud = 115200;
    serial->conf.data_bit = 8;
    serial->conf.parity = SERIAL_PARITY_NONE;
    serial->conf.stop_bit = SERIAL_STOP_1BIT;
    serial->conf.flow_ctrl = SERIAL_FLOW_CTRL_NONE;
    serial->recv_byte_timeout = 0;
    serial->last_send_size = 0;
    fifo_deinit(&serial->send_fifo);
    fifo_deinit(&serial->recv_fifo);
    serial->send_thread = NULL;
    serial->recv_thread = NULL;
    serial->user_cb = NULL;
    serial->priv = NULL;

    list_add_tail(&head, &serial->parent.parent.node);

    return 0;
}

static inline ssize_t poll_send(struct serial *serial, const void *buf, size_t size)
{
    size_t index = 0;
    const uint8_t *pbuf = (const uint8_t *)buf;

    while (index < size)
    {
        int res;
        if (serial->conf.data_bit > 8)
            res = serial->ops->poll_put_char(serial, *pbuf);
        else
            res = serial->ops->poll_put_char(serial, *((const uint16_t *)pbuf));
        if (res < 0)
            return res;
        else
        {
            if (serial->conf.data_bit > 8)
            {
                index += 2;
                pbuf += 2;
            }
            else
            {
                index++;
                pbuf++;
            }
        }
    }
    return (ssize_t)index;
}

static inline ssize_t fifo_send_nonblock(struct serial *serial, const void *buf, size_t size)
{
    uint32_t temp;

    size = fifo_push(&serial->send_fifo, buf, size);
    temp = taskENTER_CRITICAL_FROM_ISR();
    if (serial->send_fifo_empty && size)
    {
        serial->send_fifo_empty = 0;
        taskEXIT_CRITICAL_FROM_ISR(temp);
        fifo_irq_send(serial);
    }
    else
        taskEXIT_CRITICAL_FROM_ISR(temp);
    return (ssize_t)size;
}

static inline ssize_t fifo_send_block(struct serial *serial, const void *buf, size_t size)
{
    uint32_t temp;
    size_t _size = size;
    size_t len;

    while (size)
    {
        len = fifo_push(&serial->send_fifo, buf, size);
        temp = taskENTER_CRITICAL_FROM_ISR();
        if (len != size && !serial->send_fifo_empty)
            serial->send_thread = xTaskGetCurrentTaskHandle();
        if (serial->send_fifo_empty && len)
        {
            serial->send_fifo_empty = 0;
            taskEXIT_CRITICAL_FROM_ISR(temp);
            fifo_irq_send(serial);
        }
        else
            taskEXIT_CRITICAL_FROM_ISR(temp);
        if (serial->send_thread)
        {
            ulTaskNotifyTakeIndexed(1, pdTRUE, portMAX_DELAY);
            serial->send_thread = NULL;
        }
        size -= len;
    }

    return (ssize_t)_size;
}

/**
 * @brief 向串口写入指定长度的数据
 *
 * @param serial 指向串口的指针
 * @param buf 用户需要写入的数据
 * @param size 用户写入的数据大小
 * @return ssize_t 成功则返回写入的数据大小，失败则返回
 */
ssize_t serial_write(struct serial *serial, const void *buf, size_t size)
{
    uint32_t temp;
    ssize_t res = 0;

    ASSERT(serial && buf && serial->parent.init && serial->parent.drv_mode & __DRIVER_MODE_WRITE);
    if (!size)
        return 0;
    temp = taskENTER_CRITICAL_FROM_ISR();
    if (serial->send_busy)
    {
        taskEXIT_CRITICAL_FROM_ISR(temp);
        return -EAGAIN;
    }
    else
    {
        serial->send_busy = 1;
        taskEXIT_CRITICAL_FROM_ISR(temp);
    }
    if (serial->parent.init_mode & DEVICE_MODE_NONBLOCK_WRITE)
        res = fifo_send_nonblock(serial, buf, size);
    else if (serial->parent.drv_mode & __DRIVER_MODE_ASYNC_WRITE)
        res = fifo_send_block(serial, buf, size);
    else
        res = poll_send(serial, buf, size);
    serial->send_busy = 0;
    return res;
}

static inline ssize_t poll_recv(struct serial *serial, void *buf, size_t size)
{
    size_t index = 0;
    uint32_t tick;
    uint8_t *pbuf = (uint8_t *)buf;
    uint32_t timeout = serial->recv_byte_timeout;

    while (index < size)
    {
        int ch;
        tick = xTaskGetTickCount();
        while ((ch = serial->ops->poll_get_char(serial)) < 0)
        {
            if ((int32_t)xTaskGetTickCount() - (int32_t)(tick + timeout) >= 0)
                goto out;
        }
        if (serial->conf.data_bit > 8)
        {
            *((uint16_t *)pbuf) = (uint16_t)ch;
            pbuf += 2;
            index += 2;
        }
        else
        {
            *pbuf = (uint8_t)ch;
            pbuf++;
            index++;
        }
    }
out:
    if (!index)
        return -ETIMEDOUT;
    else
        return (ssize_t)index;
}

static inline ssize_t fifo_recv_nonblock(struct serial *serial, void *buf, size_t size)
{
    ssize_t res;

    res = (ssize_t)fifo_pop(&serial->recv_fifo, buf, size);
    return res;
}

static ssize_t fifo_recv_block(struct serial *serial, void *buf, size_t size)
{
    uint32_t temp;
    size_t br = size;
    uint32_t begin_ticks = xTaskGetTickCount();
    uint32_t timeout = serial->recv_byte_timeout;

    while (1)
    {
        size_t len = fifo_pop(&serial->recv_fifo, buf, br);
        uint32_t cur_ticks = xTaskGetTickCount();

        br -= len;
        if (!br || cur_ticks - begin_ticks >= timeout)
            break;
        buf += len;

        temp = taskENTER_CRITICAL_FROM_ISR();
        if (fifo_data_size(&serial->recv_fifo) < br && !fifo_is_full(&serial->recv_fifo))
            serial->recv_thread = xTaskGetCurrentTaskHandle();
        taskEXIT_CRITICAL_FROM_ISR(temp);
        if (serial->recv_thread)
        {
            if (ulTaskNotifyTakeIndexed(1, pdTRUE, timeout - (cur_ticks - begin_ticks)) != 1)
                break;
        }
    }
    if (size == br)
        return -ETIMEDOUT;
    else
        return (ssize_t)(size - br);
}

/**
 * @brief 从串口读取指定长度的数据
 *
 * @param serial 指向串口的指针
 * @param buf 用户用于接受数据的缓存区
 * @param size 用户想要读取的数据大小
 * @return ssize_t 读取成功返回实际读取的数据大小，若失败则返回一个负数
 */
ssize_t serial_read(struct serial *serial, void *buf, size_t size)
{
    uint32_t temp;
    ssize_t res = 0;

    ASSERT(serial && buf && serial->parent.init && serial->parent.drv_mode & __DRIVER_MODE_READ);
    if (!size)
        return 0;
    temp = taskENTER_CRITICAL_FROM_ISR();
    if (serial->recv_busy)
    {
        taskEXIT_CRITICAL_FROM_ISR(temp);
        return -EAGAIN;
    }
    else
    {
        serial->recv_busy = 1;
        taskEXIT_CRITICAL_FROM_ISR(temp);
    }
    if (serial->parent.init_mode & DEVICE_MODE_NONBLOCK_READ)
        res = fifo_recv_nonblock(serial, buf, size);
    else if (serial->parent.drv_mode & __DRIVER_MODE_ASYNC_READ)
        res = fifo_recv_block(serial, buf, size);
    else
        res = poll_recv(serial, buf, size);
    serial->recv_busy = 0;
    return res;
}

/**
 * @brief 设置指定串口的参数
 *
 * @param serial 指向串口的指针
 * @param conf 指向用户提供的串口参数
 * @return int 成功返回0，失败返回一个负数
 */
int serial_config(struct serial *serial, struct serial_conf *conf)
{
    uint32_t temp;
    int res;

    ASSERT(serial && conf && serial->parent.init &&
           conf->baud &&
           conf->data_bit >= 5 && conf->data_bit <= 9 &&
           conf->stop_bit <= SERIAL_STOP_2BIT &&
           conf->parity <= SERIAL_PARITY_EVEN &&
           conf->flow_ctrl <= SERIAL_FLOW_CTRL_SOFTWARE);

    temp = taskENTER_CRITICAL_FROM_ISR();
    if (serial->send_busy || serial->recv_busy)
    {
        taskEXIT_CRITICAL_FROM_ISR(temp);
        return -EAGAIN;
    }
    taskEXIT_CRITICAL_FROM_ISR(temp);
    serial->conf = *conf;
    res = serial->ops->config(serial, &serial->conf);
    if (res)
        ERROR("failed to config serial");
    return res;
}

/**
 * @brief 获取串口当前的参数
 *
 * @param serial 指向串口的指针
 * @param conf 指向用户用于保存当前串口参数的数据
 * @return int 成功返回0，失败返回一个负数
 */
int serial_get_config(struct serial *serial, struct serial_conf *conf)
{
    ASSERT(serial && conf && serial->parent.init);
    memcpy(conf, &serial->conf, sizeof(struct serial_conf));
    return 0;
}

/**
 * @brief 初始化指定的串口
 *
 * @param serial 指向串口的指针
 * @param mode 串口的工作模式
 * @param write_buf 用户提供的写缓存区，当使用非阻塞模式时，必须提供缓存区
 * @param write_buf_size 写缓存区大小
 * @param read_buf 用户提供的读缓存区，当使用非阻塞模式时，必须提供缓存区
 * @param read_buf_size 读缓存区大小
 * @return int 成功返回0，失败返回一个负数
 */
int serial_init(struct serial *serial,
                int mode,
                uint8_t *write_buf, size_t write_buf_size,
                uint8_t *read_buf, size_t read_buf_size)
{
    int res = 0;

    ASSERT(serial);
    if (serial->parent.init)
    {
        DEBUG("serial has been initialized");
        return -EBUSY;
    }

    if (mode & DEVICE_MODE_NONBLOCK_WRITE)
        mode |= DEVICE_MODE_WRITE;
    if (mode & DEVICE_MODE_NONBLOCK_READ)
        mode |= DEVICE_MODE_READ;
    if ((mode & DEVICE_MODE_WRITE && !(serial->parent.drv_mode & __DRIVER_MODE_WRITE)) ||
        (mode & DEVICE_MODE_READ && !(serial->parent.drv_mode & __DRIVER_MODE_READ)) ||
        (mode & DEVICE_MODE_NONBLOCK_WRITE && !(serial->parent.drv_mode & __DRIVER_MODE_ASYNC_WRITE)) ||
        (mode & DEVICE_MODE_NONBLOCK_READ && !(serial->parent.drv_mode & __DRIVER_MODE_ASYNC_READ)))
        return -EIO;

    serial->parent.init_mode = (uint16_t)mode;
    serial->send_busy = 0;
    serial->recv_busy = 0;
    serial->send_fifo_empty = 1;
    serial->conf.baud = 115200;
    serial->conf.data_bit = 8;
    serial->conf.parity = SERIAL_PARITY_NONE;
    serial->conf.stop_bit = SERIAL_STOP_1BIT;
    serial->conf.flow_ctrl = SERIAL_FLOW_CTRL_NONE;
    serial->recv_byte_timeout = 0;
    serial->last_send_size = 0;
    fifo_init(&serial->send_fifo, write_buf, write_buf_size);
    fifo_init(&serial->recv_fifo, read_buf, read_buf_size);
    serial->user_cb = NULL;
    serial->priv = NULL;

    serial->send_thread = NULL;
    serial->recv_thread = NULL;

    res = serial->ops->config(serial, &serial->conf);
    if (res)
    {
        ERROR("failed to config serial");
        return res;
    }

    serial->parent.init = 1;
    if (serial->parent.drv_mode & __DRIVER_MODE_ASYNC_READ)
        serial->ops->recv_ctrl(serial, true);

    return 0;
}

/**
 * @brief 注销指定的串口
 *
 * @param serial 指向串口的指针
 * @return int 注销成功返回0，失败返回一个负数
 */
int serial_deinit(struct serial *serial)
{
    if (!serial->parent.init)
    {
        WARN("serial has not been initialized");
        return -EINVAL;
    }
    else if (serial->send_busy || serial->recv_busy)
    {
        WARN("serial is busy");
        return -EBUSY;
    }
    if (serial->parent.drv_mode & __DRIVER_MODE_ASYNC_WRITE)
        serial->ops->stop_send(serial);
    if (serial->parent.drv_mode & __DRIVER_MODE_ASYNC_READ)
        serial->ops->recv_ctrl(serial, false);
    serial->send_thread = NULL;
    serial->recv_thread = NULL;
    serial->parent.init = 0;
    serial->user_cb = NULL;
    serial->priv = NULL;

    return 0;
}

/**
 * @brief 设置串口的接受超时时间
 *
 * @param serial 指向串口的指针
 * @param timeout 设置的接受超时时间
 */
void serial_set_recv_timeout(struct serial *serial, uint32_t timeout)
{
    ASSERT(serial && serial->parent.init);
    serial->recv_byte_timeout = timeout;
}

/**
 * @brief 设置串口的通知回调函数
 *
 * @param serial 指向串口的指针
 * @param cb 用户提供的回调函数
 * @param priv 用户的私有数据
 */
void serial_set_cb(struct serial *serial, void (*cb)(void *, int, void *), void *priv)
{
    uint32_t temp;

    ASSERT(serial);
    temp = taskENTER_CRITICAL_FROM_ISR();
    serial->user_cb = cb;
    serial->priv = priv;
    taskEXIT_CRITICAL_FROM_ISR(temp);
}

/**
 * @brief 通过名字来获取串口
 *
 * @param name 想要查找的串口名字
 * @return struct serial* 成功返回串口的指针，失败返回NULL
 */
struct serial *serial_find_by_name(const char *name)
{
    ASSERT(name);
    return (struct serial *)object_find_by_name(&head, name);
}