/**
 * @file ll_spi.h
 * @author salalei (1028609078@qq.com)
 * @brief spi驱动框架
 * @version 0.1
 * @date 2021-12-05
 *
 * @copyright Copyright (c) 2021
 *
 */
#ifndef __LL_SPI_H__
#define __LL_SPI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ll_drv.h"

#include "FreeRTOS.h"

#define __LL_SPI_DIR_SEND 0
#define __LL_SPI_DIR_RECV 1

#define __LL_SPI_PROTO_STD  0
#define __LL_SPI_PROTO_DUAL 1
#define __LL_SPI_PROTO_QUAD 2

#define __LL_SPI_FRAME_8BIT  0
#define __LL_SPI_FRAME_16BIT 1
#define __LL_SPI_FRAME_24BIT 2
#define __LL_SPI_FRAME_32BIT 3

#define __LL_SPI_LSB 0
#define __LL_SPI_MSB 1

#define __LL_SPI_SOFT_CS 0
#define __LL_SPI_HARD_CS 1
#define __LL_SPI_NO_CS   2

struct ll_spi_dev;
struct ll_spi_bus;
typedef struct QueueDefinition *QueueHandle_t;
typedef QueueHandle_t SemaphoreHandle_t;
struct tskTaskControlBlock;
typedef struct tskTaskControlBlock *TaskHandle_t;
struct ll_pin;

struct ll_spi_trans
{
    void *buf;
    size_t size;
    int dir : 1; //传输方向
};

struct ll_spi_msg
{
    struct ll_list_node node;
    struct ll_spi_trans *trans;
    size_t size;
    struct ll_spi_dev *dev;
    void (*complete)(int res);
    TaskHandle_t thread;
    int result;
};

struct ll_spi_conf
{
    uint32_t max_speed_hz;     //最大传输速率
    int frame_bits : 2;        //每帧传输的数据位
    int cpha : 1;              //时钟相位
    int cpol : 1;              //时钟极性
    int endian : 1;            //数据传输大小端
    int cs_mode : 2;           //片选模式
    int proto : 2;             //传输协议
    int send_addr_not_inc : 1; //发送数据地址不要自增，即重复发送同一个数据
};

struct ll_spi_dev
{
    struct ll_drv parent;
    struct ll_spi_bus *spi;
    struct ll_spi_conf conf;
    struct ll_pin *cs_pin; //软件片选控制的引脚
    uint32_t cs_index;     //硬件片选控制的索引
};

struct ll_spi_ops
{
    ssize_t (*master_send)(struct ll_spi_dev *dev, const void *buf, size_t size);
    ssize_t (*matser_recv)(struct ll_spi_dev *dev, void *buf, size_t size);
    void (*hard_cs_ctrl)(struct ll_spi_dev *dev, bool state);
    int (*config)(struct ll_spi_dev *dev);
};

struct ll_spi_bus
{
    struct ll_drv parent;
    const struct ll_spi_ops *ops;
    struct ll_spi_dev *dev;
    struct ll_list_node msg_head;
    struct ll_list_node dev_head;
    uint16_t trans_index;
    uint16_t send_busy : 1;
    uint16_t cs_state : 1;
    uint16_t cs_hard_max_numb;
    uint16_t cs_numb;

    SemaphoreHandle_t lock;
};

#define LL_SPI_MSG_INIT(t, len, cb) \
    { \
        .trans = t, \
        .size = len, \
        .dev = NULL, \
        .complete = cb, \
        .thread = NULL, \
        .result = 0, \
    }

void __ll_spi_irq_handler(struct ll_spi_dev *dev);
int __ll_spi_bus_register(struct ll_spi_bus *bus,
                          const char *name,
                          void *priv,
                          int drv_mode);

int ll_spi_sync(struct ll_spi_dev *dev, struct ll_spi_msg *msg);
int ll_spi_async(struct ll_spi_dev *dev, struct ll_spi_msg *msg);
int ll_spi_config(struct ll_spi_dev *dev, struct ll_spi_conf *conf);
void ll_spi_msg_init(struct ll_spi_msg *msg,
                     struct ll_spi_trans *trans,
                     size_t size,
                     void (*complete)(int res));
int ll_spi_bus_init(struct ll_spi_bus *bus);
int ll_spi_bus_deinit(struct ll_spi_bus *bus);
struct ll_spi_dev *ll_spi_dev_find_by_name(struct ll_spi_bus *bus, const char *name);
int ll_spi_dev_register(struct ll_spi_dev *dev,
                        const char *name,
                        const char *spi_bus,
                        void *priv,
                        int drv_mode);
int ll_spi_dev_unregister(struct ll_spi_dev *dev);

#ifdef __cplusplus
}
#endif

#endif