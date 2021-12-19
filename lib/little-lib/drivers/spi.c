/**
 * @file spi.c
 * @author salalei (1028609078@qq.com)
 * @brief spi驱动框架
 * @version 0.1
 * @date 2021-12-05
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "spi.h"
#include "dbg.h"
#include "pin.h"

#include "../modules/list.h"
#include "../modules/object.h"

#include "semphr.h"
#include "task.h"

#include <string.h>

static struct list_node head = LIST_HEAD_INIT_INS(head);

static int take_bus(struct spi_dev *dev)
{
    if (dev->spi->dev != dev)
    {
        int res;
        res = dev->spi->ops->config(dev);
        if (res)
            return res;
        dev->spi->dev = dev;
    }
    if (dev->conf.cs_mode == __SPI_SOFT_CS)
        pin_active(dev->cs_pin);
    else if (dev->conf.cs_mode == __SPI_HARD_CS)
        dev->spi->ops->hard_cs_ctrl(dev, true);

    return 0;
}

static void release_bus(struct spi_dev *dev)
{
    if (dev->conf.cs_mode == __SPI_SOFT_CS)
        pin_deactive(dev->cs_pin);
    else if (dev->conf.cs_mode == __SPI_HARD_CS)
        dev->spi->ops->hard_cs_ctrl(dev, false);
}

static int spi_trans(struct spi_bus *bus, struct spi_trans *trans)
{
    size_t trans_size;

    if (trans->dir == __SPI_DIR_SEND)
    {
        ASSERT(bus->dev->parent.drv_mode & __DRIVER_MODE_WRITE);
        trans_size = bus->ops->master_send(bus->dev, trans->buf, trans->size);
    }
    else
    {
        ASSERT(bus->dev->parent.drv_mode & __DRIVER_MODE_WRITE);
        trans_size = bus->ops->matser_recv(bus->dev, trans->buf, trans->size);
    }
    if (trans_size != trans->size)
    {
        ERROR("failed to spi transfer");
        return -EIO;
    }
    return 0;
}

static void noticy_or_exec_cb(struct spi_msg *msg, BaseType_t *woken)
{
    if (msg->thread)
    {
        if (*woken)
        {
            vTaskNotifyGiveIndexedFromISR(msg->thread, 1, NULL);
        }
        else
        {
            vTaskNotifyGiveIndexedFromISR(msg->thread, 1, woken);
        }
    }
    else
    {
        if (msg->complete)
            msg->complete(msg->result);
    }
}

void __spi_irq_handler(struct spi_dev *dev)
{
    struct spi_msg *msg;
    struct spi_bus *bus = dev->spi;
    BaseType_t woken = 0;
    uint32_t temp;

    ASSERT(dev && dev->spi->parent.drv_mode & __DRIVER_MODE_ASYNC_WRITE);
    msg = (struct spi_msg *)list_first(&bus->msg_head);
    if (bus->trans_index < msg->size)
    {
        msg->result = spi_trans(bus, &msg->trans[bus->trans_index]);
        if (!msg->result)
        {
            bus->trans_index++;
            return;
        }
    }
    else
        msg->result = 0;
    release_bus(msg->dev);
    noticy_or_exec_cb(msg, &woken);
    temp = taskENTER_CRITICAL_FROM_ISR();
    list_delete(&msg->node);
    while (1)
    {
        if (list_is_empty(&bus->msg_head))
        {
            bus->send_busy = 0;
            break;
        }
        else
        {
            msg = (struct spi_msg *)list_first(&bus->msg_head);
            msg->result = take_bus(bus->dev);
            if (!msg->result)
            {
                bus->trans_index = 1;
                msg->result = spi_trans(bus, &msg->trans[0]);
            }
            if (msg->result)
            {
                release_bus(bus->dev);
                list_delete(&msg->node);
                taskEXIT_CRITICAL_FROM_ISR(temp);
                noticy_or_exec_cb(msg, &woken);
                temp = taskENTER_CRITICAL_FROM_ISR();
            }
            else
                break;
        }
    }
    taskEXIT_CRITICAL_FROM_ISR(temp);
    portYIELD_FROM_ISR(woken);
}

/**
 * @brief 向spi驱动框架注册一个spi
 *
 * @param bus 指向spi总线的指针
 * @param name spi的名字
 * @param priv 指向驱动的私有数据
 * @param drv_mode 底层驱动给定的工作模式
 * @return int 返回0
 */
int __spi_bus_register(struct spi_bus *bus,
                       const char *name,
                       void *priv,
                       int drv_mode)
{
    ASSERT(bus && name && bus->ops && bus->ops->config);
    if (drv_mode & __DRIVER_MODE_ASYNC_WRITE)
    {
        ASSERT(bus->ops->master_send);
        drv_mode |= __DRIVER_MODE_WRITE;
    }
    if (drv_mode & __DRIVER_MODE_ASYNC_READ)
    {
        ASSERT(bus->ops->matser_recv);
        drv_mode |= __DRIVER_MODE_READ;
    }

    __driver_init(&bus->parent, name, priv, drv_mode);
    bus->dev = NULL;
    list_head_init(&bus->msg_head);
    list_head_init(&bus->dev_head);
    bus->trans_index = 0;
    bus->send_busy = 0;
    bus->cs_hard_max_numb = 0;
    bus->lock = NULL;

    list_add_tail(&head, &bus->parent.parent.node);

    return 0;
}

static inline int spi_poll_xfer(struct spi_msg *msg)
{
    int res = 0;
    struct spi_bus *bus = msg->dev->spi;
    size_t index = 0;

    xSemaphoreTake(bus->lock, portMAX_DELAY);
    bus->send_busy = 1;
    res = take_bus(msg->dev);
    if (res)
    {
        xSemaphoreGive(bus->lock);
        return res;
    }
    while (index < msg->size)
    {
        if (spi_trans(msg->dev->spi, &msg->trans[index]))
        {
            res = -EIO;
            break;
        }
        index++;
    }
    release_bus(msg->dev);
    bus->send_busy = 0;
    if (xSemaphoreGive(bus->lock) != pdTRUE && !res)
        res = -EINVAL;

    return res;
}

static inline int spi_int_trans(struct spi_msg *msg)
{
    int res = 0;
    uint32_t temp;
    struct spi_bus *bus = msg->dev->spi;

    temp = taskENTER_CRITICAL_FROM_ISR();
    list_add_tail(&bus->msg_head, &msg->node);
    if (!bus->send_busy)
    {
        res = take_bus(msg->dev);
        if (!res)
        {
            bus->trans_index = 1;
            res = spi_trans(bus, &msg->trans[0]);
        }
        if (res)
        {
            release_bus(msg->dev);
            list_delete(&msg->node);
        }
        else
            bus->send_busy = 1;
    }
    taskEXIT_CRITICAL_FROM_ISR(temp);

    return res;
}

int spi_sync(struct spi_dev *dev, struct spi_msg *msg)
{
    struct spi_bus *bus = dev->spi;

    ASSERT(dev && dev->parent.init && msg && bus->parent.init);
    if (!msg->size)
        return 0;
    msg->dev = dev;
    if (bus->parent.drv_mode & (__DRIVER_MODE_ASYNC_WRITE | __DRIVER_MODE_ASYNC_READ))
    {
        int res;
        msg->thread = xTaskGetCurrentTaskHandle();
        res = spi_int_trans(msg);
        if (!res)
        {
            if (ulTaskNotifyTakeIndexed(1, pdTRUE, portMAX_DELAY) != 1)
                res = -EINVAL;
        }
        return res;
    }
    else
        return spi_poll_xfer(msg);
}

int spi_async(struct spi_dev *dev, struct spi_msg *msg)
{
    ASSERT(dev && dev->parent.init && msg && dev->spi->parent.init &&
           dev->spi->parent.drv_mode & (__DRIVER_MODE_ASYNC_WRITE | __DRIVER_MODE_ASYNC_READ));
    if (!msg->size)
        return 0;
    msg->dev = dev;
    msg->thread = NULL;
    return spi_int_trans(msg);
}

int spi_config(struct spi_dev *dev, struct spi_conf *conf)
{
    uint32_t temp;
    int res;

    ASSERT(dev && conf);
    temp = taskENTER_CRITICAL_FROM_ISR();
    if (!dev->spi->send_busy)
    {
        memcpy(&dev->conf, conf, sizeof(struct spi_conf));
        res = dev->spi->ops->config(dev);
    }
    else
        res = -EAGAIN;
    taskEXIT_CRITICAL_FROM_ISR(temp);
    return res;
}

/**
 * @brief 对spi的消息队列初始化
 *
 * @param msg 指向spi_msg的指针
 * @param trans 指向spi_trans的指针
 * @param size 需要传输spi_trans的数量
 * @param complete 用来通知传输完成的回调函数
 */
void spi_msg_init(struct spi_msg *msg,
                  struct spi_trans *trans,
                  size_t size,
                  void (*complete)(int res))
{
    ASSERT(msg && trans);
    if (!size)
        return;
    msg->trans = trans;
    msg->size = size;
    msg->dev = NULL;
    msg->complete = complete;
    msg->thread = NULL;
    msg->result = 0;
}

/**
 * @brief 初始化指定的spi总线
 *
 * @param bus 指向spi总线的指针
 * @return int 成功返回0，失败返回一个负数
 */
int spi_bus_init(struct spi_bus *bus)
{
    ASSERT(bus);
    bus->parent.init_count++;
    if (bus->parent.init_count > 1)
        return 0;
    bus->parent.init = 1;
    bus->dev = NULL;
    list_head_init(&bus->msg_head);
    list_head_init(&bus->dev_head);
    bus->trans_index = 0;
    bus->send_busy = 0;
    bus->cs_numb = 0;
    if (bus->parent.drv_mode & (__DRIVER_MODE_ASYNC_READ | __DRIVER_MODE_ASYNC_WRITE))
        bus->lock = NULL;
    else
    {
        bus->lock = xSemaphoreCreateMutex();
        if (!bus->lock)
            return -EAGAIN;
    }

    return 0;
}

/**
 * @brief 注销指定的spi
 *
 * @param bus 指向spi总线的指针
 * @return int 成功返回0，失败返回一个负数
 */
int spi_bus_deinit(struct spi_bus *bus)
{
    ASSERT(bus);
    if (bus->parent.init_count == 0)
    {
        DEBUG("spi has not been initialized");
        return -EINVAL;
    }
    if (--bus->parent.init_count)
        return 0;
    if (bus->send_busy)
        return -EAGAIN;
    bus->dev = NULL;
    bus->trans_index = 0;
    bus->cs_numb = 0;
    if (!(bus->parent.drv_mode & (__DRIVER_MODE_ASYNC_READ | __DRIVER_MODE_ASYNC_WRITE)))
        vSemaphoreDelete(bus->lock);
    return 0;
}

/**
 * @brief 通过名字来获取spi
 *
 * @param name 想要查找的spi名字
 * @return struct spi_bus* 成功返回spi总线的指针，失败返回NULL
 */
struct spi_bus *spi_bus_find_by_name(const char *name)
{
    ASSERT(name);
    return (struct spi_bus *)object_find_by_name(&head, name);
}

/**
 * @brief 通过名字来获取spi设备
 *
 * @param bus 想要查找的spi总线
 * @param name 想要查找的设备名字
 * @return struct spi_dev* 成功返回spi设备的指针，失败返回NULL
 */
struct spi_dev *spi_dev_find_by_name(struct spi_bus *bus, const char *name)
{
    ASSERT(bus && name);
    return (struct spi_dev *)object_find_by_name(&bus->dev_head, name);
}

static inline bool spi_cs_is_used(struct spi_dev *dev)
{
    struct list_node *node;
    struct list_node *temp;

    FOR_EACH_LIST_NODE_SAFE(&dev->spi->dev_head, node, temp)
    {
        struct spi_dev *dev_node = (struct spi_dev *)node;
        if ((dev->conf.cs_mode == __SPI_HARD_CS && dev_node->conf.cs_mode == __SPI_HARD_CS &&
             dev->cs_index == dev_node->cs_index) ||
            (dev->conf.cs_mode == __SPI_SOFT_CS && dev_node->conf.cs_mode == __SPI_SOFT_CS &&
             dev->cs_pin == dev_node->cs_pin))
            return true;
    }
    return false;
}

/**
 * @brief 注册一个spi设备
 *
 * @param dev 指向spi设备的指针
 * @param name 设备的名字
 * @param spi_bus 设备所挂载的总线的名字
 * @param priv 用户的私有参数
 * @param drv_mode spi设备的驱动模式
 * @return int 成功返回0，失败返回一个负数
 */
int spi_dev_register(struct spi_dev *dev,
                     const char *name,
                     const char *spi_bus,
                     void *priv,
                     int drv_mode)
{
    int res;

    ASSERT(dev && name && spi_bus);
    __driver_init(&dev->parent, name, priv, drv_mode);
    dev->spi = spi_bus_find_by_name(spi_bus);
    if (!dev->spi)
    {
        ERROR("cannot find %s", spi_bus);
        return -EIO;
    }
    //检查片选设置是否正确
    if (dev->conf.cs_mode == __SPI_HARD_CS)
    {
        if (dev->cs_index >= dev->spi->cs_hard_max_numb || dev->spi->cs_numb >= dev->spi->cs_hard_max_numb)
        {
            ERROR("spi hard cs error");
            return -EIO;
        }
    }
    else if (dev->conf.cs_mode == __SPI_SOFT_CS)
        ASSERT(dev->cs_pin);
    //检查片选是否占用或重复注册
    if (spi_cs_is_used(dev))
    {
        ERROR("the cs has been used");
        return -EEXIST;
    }
    res = spi_bus_init(dev->spi);
    if (res)
        return res;
    list_add_tail(&dev->spi->dev_head, &dev->parent.parent.node);
    dev->spi->cs_numb++;
    dev->parent.init = 1;

    return 0;
}

/**
 * @brief 注销一个spi设备
 *
 * @param dev 指向spi设备的指针
 * @return int 成功返回0，失败返回一个负数
 */
int spi_dev_unregister(struct spi_dev *dev)
{
    ASSERT(dev && dev->spi);
    if (dev->spi->dev == dev && dev->spi->send_busy)
        return -EAGAIN;
    if (spi_bus_deinit(dev->spi))
        return -EAGAIN;
    dev->parent.init = 0;
    list_delete(&dev->parent.parent.node);
    dev->spi->cs_numb--;
    dev->spi = NULL;

    return 0;
}