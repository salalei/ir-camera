/**
 * @file ll_i2c.h
 * @author salalei (1028609078@qq.com)
 * @brief i2c驱动框架
 * @version 0.1
 * @date 2022-01-02
 *
 * @copyright Copyright (c) 2022
 *
 */
#ifndef __LL_I2C_H__
#define __LL_I2C_H__

#include "ll_drv.h"

#define __LL_I2C_DIR_SEND 0
#define __LL_I2C_DIR_RECV 1

struct ll_i2c_bus;
typedef struct QueueDefinition *QueueHandle_t;
typedef QueueHandle_t SemaphoreHandle_t;

struct ll_i2c_msg
{
    void *buf;
    size_t size;
    uint16_t dir : 1;
    uint16_t ten_addr : 1;
    uint16_t no_start : 1;
    uint16_t ignore_ack : 1;
    uint16_t no_stop : 1;
};

struct ll_i2c_dev
{
    struct ll_drv parent;
    struct ll_i2c_bus *i2c;
    uint16_t addr;
};

struct ll_i2c_ops
{
    ssize_t (*master_xfer)(struct ll_i2c_dev *dev, struct ll_i2c_msg *msgs, size_t numb);
};

struct ll_i2c_bus
{
    struct ll_drv parent;
    const struct ll_i2c_ops *ops;
    struct ll_list_node dev_head;

    SemaphoreHandle_t lock;
};

int __ll_i2c_bus_register(struct ll_i2c_bus *i2c,
                          const char *name,
                          void *priv,
                          int drv_mode);

ssize_t ll_i2c_trans(struct ll_i2c_dev *dev, struct ll_i2c_msg *msgs, size_t numb);
int ll_i2c_bus_init(struct ll_i2c_bus *bus);
int ll_i2c_bus_deinit(struct ll_i2c_bus *bus);
struct ll_i2c_dev *ll_i2c_dev_find_by_name(struct ll_i2c_bus *bus, const char *name);
int ll_i2c_dev_register(struct ll_i2c_dev *dev,
                        const char *name,
                        const char *i2c_bus,
                        void *priv,
                        int drv_mode);
int ll_i2c_dev_unregister(struct ll_i2c_dev *dev);

#endif