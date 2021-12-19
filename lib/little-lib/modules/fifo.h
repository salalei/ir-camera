/**
 * @file fifo.h
 * @author salalei (1028609078@qq.com)
 * @brief fifo(32bit系统可以实现无锁读写)
 * @version 0.1
 * @date 2021-12-04
 *
 * @copyright Copyright (c) 2021
 *
 */

#ifndef __FIFO_H__
#define __FIFO_H__

#include "../include/types.h"

#define FIFO_ROUND_BIT         0x80000000
#define FIFO_READ_ROUND_BIT(x) ((x)&FIFO_ROUND_BIT)
#define FIFO_POS_MASK          0x7fffffff
#define FIFO_READ_POS(x)       ((size_t)((x)&FIFO_POS_MASK))

/**
 * @brief fifo句柄
 */
struct fifo_handle
{
    uint8_t *buf;
    size_t size;
    volatile size_t write_pos;
    volatile size_t read_pos;
};

/**
 * @brief 初始化一个fifo的实体
 *
 * @param set_buf 指定的缓存区
 * @param buf_size 指定的缓存大小
 *
 */
#define FIFO_INIT_INS(set_buf, buf_size) \
    { \
        .buf = (set_buf), \
        .size = (buf_size), \
        .write_pos = 0, \
        .read_pos = 0, \
    }

/**
 * @brief 静态定义一个fifo
 *
 * @param name 缓存区名字
 * @param buf_size 指定的缓存大小
 *
 */
#define FIFO_STATIC_DEF(name, buf_size) \
    static uint8_t name##_buf[(buf_size)]; \
    struct fifo_handle name = FIFO_INIT_INS(name##_buf, buf_size)

void fifo_init(struct fifo_handle *handle, const uint8_t *buf, size_t size);
void fifo_deinit(struct fifo_handle *handle);
size_t fifo_data_size(struct fifo_handle *handle);
size_t fifo_free_size(struct fifo_handle *handle);
size_t fifo_push(struct fifo_handle *handle, const uint8_t *buf, size_t size);
bool fifo_push_force(struct fifo_handle *handle, const uint8_t *buf, size_t size);
size_t fifo_peek(struct fifo_handle *handle, uint8_t *buf, size_t offset, size_t size);
size_t fifo_pop(struct fifo_handle *handle, uint8_t *buf, size_t size);
void fifo_clear(struct fifo_handle *handle, size_t size);
void fifo_clear_all(struct fifo_handle *handle);
bool fifo_is_full(struct fifo_handle *handle);
bool fifo_is_empty(struct fifo_handle *handle);

#endif
