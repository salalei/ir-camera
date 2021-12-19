/**
 * @file spi.h
 * @author salalei (1028609078@qq.com)
 * @brief spi驱动框架
 * @version 0.1
 * @date 2021-12-05
 *
 * @copyright Copyright (c) 2021
 *
 */
#ifndef __SPI_H__
#define __SPI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "../include/types.h"

#include "FreeRTOS.h"

#define __SPI_DIR_SEND 0
#define __SPI_DIR_RECV 1

#define __SPI_PROTO_STD  0
#define __SPI_PROTO_DUAL 1
#define __SPI_PROTO_QUAD 2

#define __SPI_FRAME_8BIT  0
#define __SPI_FRAME_16BIT 1
#define __SPI_FRAME_24BIT 2
#define __SPI_FRAME_32BIT 3

#define __SPI_LSB 0
#define __SPI_MSB 1

#define __SPI_SOFT_CS 0
#define __SPI_HARD_CS 1
#define __SPI_NO_CS   2

struct spi_dev;
struct spi_bus;
typedef struct QueueDefinition *QueueHandle_t;
typedef QueueHandle_t SemaphoreHandle_t;
struct tskTaskControlBlock;
typedef struct tskTaskControlBlock *TaskHandle_t;
struct pin;

struct spi_trans
{
    void *buf;
    size_t size;
    int dir : 1; //传输方向
};

struct spi_msg
{
    struct list_node node;
    struct spi_trans *trans;
    size_t size;
    struct spi_dev *dev;
    void (*complete)(int res);
    TaskHandle_t thread;
    int result;
};

struct spi_conf
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

struct spi_dev
{
    struct driver parent;
    struct spi_bus *spi;
    struct spi_conf conf;
    struct pin *cs_pin; //软件片选控制的引脚
    uint32_t cs_index;  //硬件片选控制的索引
};

struct spi_ops
{
    ssize_t (*master_send)(struct spi_dev *dev, const void *buf, size_t size);
    ssize_t (*matser_recv)(struct spi_dev *dev, void *buf, size_t size);
    void (*hard_cs_ctrl)(struct spi_dev *dev, bool state);
    int (*config)(struct spi_dev *dev);
};

struct spi_bus
{
    struct driver parent;
    const struct spi_ops *ops;
    struct spi_dev *dev;
    struct list_node msg_head;
    struct list_node dev_head;
    uint16_t trans_index;
    uint16_t send_busy : 1;
    uint16_t cs_state : 1;
    uint16_t cs_hard_max_numb;
    uint16_t cs_numb;

    SemaphoreHandle_t lock;
};

#define SPI_MSG_INIT(t, len, cb) \
    { \
        .trans = t, \
        .size = len, \
        .dev = NULL, \
        .complete = cb, \
        .thread = NULL, \
        .result = 0, \
    }

void __spi_irq_handler(struct spi_dev *dev);
int __spi_bus_register(struct spi_bus *bus,
                       const char *name,
                       void *priv,
                       int drv_mode);

int spi_sync(struct spi_dev *dev, struct spi_msg *msg);
int spi_async(struct spi_dev *dev, struct spi_msg *msg);
int spi_config(struct spi_dev *dev, struct spi_conf *conf);
void spi_msg_init(struct spi_msg *msg,
                  struct spi_trans *trans,
                  size_t size,
                  void (*complete)(int res));
int spi_bus_init(struct spi_bus *bus);
int spi_bus_deinit(struct spi_bus *bus);
struct spi_bus *spi_bus_find_by_name(const char *name);
struct spi_dev *spi_dev_find_by_name(struct spi_bus *bus, const char *name);
int spi_dev_register(struct spi_dev *dev,
                     const char *name,
                     const char *spi_bus,
                     void *priv,
                     int drv_mode);
int spi_dev_unregister(struct spi_dev *dev);

#ifdef __cplusplus
}
#endif

#endif