/**
 * @file serial.h
 * @author salalei (1028609078@qq.com)
 * @brief 串口驱动框架
 * @version 0.1
 * @date 2021-12-03
 *
 * @copyright Copyright (c) 2021
 *
 */
#ifndef __SERIAL_H__
#define __SERIAL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "../include/types.h"
#include "../modules/fifo.h"

#include "FreeRTOS.h"

/**
 * @brief 停止位
 */
#define SERIAL_STOP_1BIT   0
#define SERIAL_STOP_1_5BIT 1
#define SERIAL_STOP_2BIT   2

/**
 * @brief 奇偶位
 */
#define SERIAL_PARITY_NONE 0
#define SERIAL_PARITY_ODD  1 //奇校验
#define SERIAL_PARITY_EVEN 2 //偶校验

/**
 * @brief 流控制
 */
#define SERIAL_FLOW_CTRL_NONE     0
#define SERIAL_FLOW_CTRL_HARDWARE 1 //硬件流控制
#define SERIAL_FLOW_CTRL_SOFTWARE 2 //软件流控制

struct tskTaskControlBlock;
typedef struct tskTaskControlBlock *TaskHandle_t;
struct serial;

struct serial_conf
{
    uint32_t baud;          //波特率
    uint32_t stop_bit : 2;  //停止位，0 1位，1 1.5位，2 2位
    uint32_t parity : 2;    //校验位，0无，1奇校验，2偶校验
    uint32_t data_bit : 4;  //数据位
    uint32_t flow_ctrl : 2; //流控制
};

struct serial_ops
{
    int (*poll_put_char)(struct serial *serial, int ch);                      //轮询发送一个字节
    int (*poll_get_char)(struct serial *serial);                              //轮询接受一个字节
    ssize_t (*irq_send)(struct serial *serial, const void *buf, size_t size); //通过中断发送数据(可以是dma或者硬件fifo)
    void (*stop_send)(struct serial *serial);                                 //停止发送
    void (*recv_ctrl)(struct serial *serial, bool enabled);                   //控制接受是否使能
    int (*config)(struct serial *serial, struct serial_conf *param);          //配置串口数据
};

/**
 * @brief 串口
 */
struct serial
{
    struct driver parent;

    const struct serial_ops *ops;

    uint32_t send_busy : 1;
    uint32_t recv_busy : 1;
    uint32_t send_fifo_empty : 1;

    struct serial_conf conf;

    uint32_t recv_byte_timeout;

    uint32_t last_send_size;
    struct fifo_handle send_fifo;
    struct fifo_handle recv_fifo;

    TaskHandle_t send_thread;
    TaskHandle_t recv_thread;

    void (*user_cb)(void *priv, int type, void *arg);
    void *priv;
};

void __serial_send_irq_handler(struct serial *serial);
void __serial_recv_push(struct serial *serial, const uint8_t *buf, size_t size);
int __serial_register(struct serial *serial,
                      const char *name,
                      void *priv,
                      int drv_mode);

ssize_t serial_write(struct serial *serial, const void *buf, size_t size);
ssize_t serial_read(struct serial *serial, void *buf, size_t size);
int serial_config(struct serial *serial, struct serial_conf *param);
int serial_get_config(struct serial *serial, struct serial_conf *param);
int serial_init(struct serial *serial,
                int mode,
                uint8_t *write_buf, size_t write_buf_size,
                uint8_t *read_buf, size_t read_buf_size);
int serial_deinit(struct serial *serial);
void serial_set_recv_timeout(struct serial *serial, uint32_t timeout);
void serial_set_cb(struct serial *serial, void (*cb)(void *, int, void *), void *priv);
struct serial *serial_find_by_name(const char *name);

#ifdef __cplusplus
}
#endif

#endif