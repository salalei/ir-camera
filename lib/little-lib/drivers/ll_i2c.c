/**
 * @file ll_i2c.c
 * @author salalei (1028609078@qq.com)
 * @brief i2c驱动框架
 * @version 0.1
 * @date 2022-01-03
 *
 * @copyright Copyright (c) 2022
 *
 */
#include "ll_i2c.h"
#include "ll_log.h"

#include "FreeRTOS.h"
#include "semphr.h"

/**
 * @brief 向i2c驱动框架注册一个i2c
 *
 * @param i2c 指向i2c总线的指针
 * @param name i2c的名字
 * @param priv 指向驱动的私有数据
 * @param drv_mode 底层驱动给定的工作模式
 * @return int 返回0
 */
int __ll_i2c_bus_register(struct ll_i2c_bus *i2c,
                          const char *name,
                          void *priv,
                          int drv_mode)
{
    LL_ASSERT(i2c && i2c->ops && i2c->ops->master_xfer);
    __ll_drv_init(&i2c->parent, name, priv, drv_mode);
    ll_list_head_init(&i2c->dev_head);
    i2c->lock = NULL;
    __ll_drv_register(&i2c->parent);
    return 0;
}

/**
 * @brief 进行一次i2c传输
 *
 * @param dev 指向i2c设备的指针
 * @param msgs 指向用户的消息队列
 * @param numb 用户要传输的消息数量
 * @return ssize_t 实际传输的消息数量
 */
ssize_t ll_i2c_trans(struct ll_i2c_dev *dev, struct ll_i2c_msg *msgs, size_t numb)
{
    ssize_t res;

    LL_ASSERT(dev && msgs);
    if (!numb)
        return 0;
    xSemaphoreTake(dev->i2c->lock, portMAX_DELAY);
    res = dev->i2c->ops->master_xfer(dev, msgs, numb);
    xSemaphoreGive(dev->i2c->lock);
    return res;
}

/**
 * @brief 初始化指定的i2c总线
 *
 * @param bus 指向i2c总线的指针
 * @return int 成功返回0，失败返回一个负数
 */
int ll_i2c_bus_init(struct ll_i2c_bus *bus)
{
    LL_ASSERT(bus);
    if (bus->parent.init_count)
        return 0;
    ll_list_head_init(&bus->dev_head);
    bus->lock = xSemaphoreCreateMutex();
    if (!bus->lock)
        return -EAGAIN;
    bus->parent.init_count++;
    bus->parent.init = 1;

    return 0;
}

/**
 * @brief 注销指定的i2c
 *
 * @param bus 指向i2c总线的指针
 * @return int 成功返回0，失败返回一个负数
 */
int ll_i2c_bus_deinit(struct ll_i2c_bus *bus)
{
    LL_ASSERT(bus);
    if (bus->parent.init_count == 0)
    {
        LL_DEBUG("i2c has not been initialized");
        return -EINVAL;
    }
    if (bus->parent.init_count > 1)
    {
        bus->parent.init_count--;
        return 0;
    }
    if (xSemaphoreGetMutexHolder(bus->lock))
        return -EAGAIN;
    bus->parent.init_count--;
    vSemaphoreDelete(bus->lock);
    return 0;
}

/**
 * @brief 通过名字来获取i2c设备
 *
 * @param bus 想要查找的i2c总线
 * @param name 想要查找的设备名字
 * @return struct ll_i2c_dev* 成功返回i2c设备的指针，失败返回NULL
 */
struct ll_i2c_dev *ll_i2c_dev_find_by_name(struct ll_i2c_bus *bus, const char *name)
{
    LL_ASSERT(bus && name);
    return (struct ll_i2c_dev *)ll_obj_find_by_name(&bus->dev_head, name);
}

/**
 * @brief 注册一个i2c设备
 *
 * @param dev 指向i2c设备的指针
 * @param name 设备的名字
 * @param priv 用户的私有参数
 * @param drv_mode i2c设备的驱动模式
 * @return int 成功返回0，失败返回一个负数
 */
int ll_i2c_dev_register(struct ll_i2c_dev *dev,
                        const char *name,
                        void *priv,
                        int drv_mode)
{
    int res;

    LL_ASSERT(dev && name);
    __ll_drv_init(&dev->parent, name, priv, drv_mode);
    res = ll_i2c_bus_init(dev->i2c);
    if (res)
        return res;
    ll_list_add_tail(&dev->i2c->dev_head, &dev->parent.parent.node);
    dev->parent.init = 1;

    return 0;
}

/**
 * @brief 注销一个i2c设备
 *
 * @param dev 指向i2c设备的指针
 * @return int 成功返回0，失败返回一个负数
 */
int ll_i2c_dev_unregister(struct ll_i2c_dev *dev)
{
    LL_ASSERT(dev && dev->i2c);
    if (ll_i2c_bus_deinit(dev->i2c))
        return -EAGAIN;
    dev->parent.init = 0;
    ll_list_delete(&dev->parent.parent.node);
    dev->i2c = NULL;

    return 0;
}