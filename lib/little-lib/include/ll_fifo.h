/**
 * @file ll_fifo.h
 * @author salalei (1028609078@qq.com)
 * @brief ll_fifo(32bit系统可以实现无锁读写)
 * @version 0.1
 * @date 2021-12-04
 *
 * @copyright Copyright (c) 2021
 *
 */

#ifndef __LL_FIFO_H__
#define __LL_FIFO_H__

#include "ll_types.h"

#define LL_FIFO_ROUND_BIT         0x80000000
#define LL_FIFO_READ_ROUND_BIT(x) ((x)&LL_FIFO_ROUND_BIT)
#define LL_FIFO_POS_MASK          0x7fffffff
#define LL_FIFO_READ_POS(x)       ((size_t)((x)&LL_FIFO_POS_MASK))

/**
 * @brief fifo句柄
 */
struct ll_fifo
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
#define LL_FIFO_INIT_INS(set_buf, buf_size) \
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
#define LL_FIFO_STATIC_DEF(name, buf_size) \
    static uint8_t name##_buf[(buf_size)]; \
    struct ll_fifo name = LL_FIFO_INIT_INS(name##_buf, buf_size)

void ll_fifo_init(struct ll_fifo *handle, const uint8_t *buf, size_t size);
void ll_fifo_deinit(struct ll_fifo *handle);
size_t ll_fifo_data_size(struct ll_fifo *handle);
size_t ll_fifo_free_size(struct ll_fifo *handle);
size_t ll_fifo_push(struct ll_fifo *handle, const uint8_t *buf, size_t size);
bool ll_fifo_push_force(struct ll_fifo *handle, const uint8_t *buf, size_t size);
size_t ll_fifo_peek(struct ll_fifo *handle, uint8_t *buf, size_t offset, size_t size);
size_t ll_fifo_pop(struct ll_fifo *handle, uint8_t *buf, size_t size);
void ll_fifo_clear(struct ll_fifo *handle, size_t size);
void ll_fifo_clear_all(struct ll_fifo *handle);
bool ll_fifo_is_full(struct ll_fifo *handle);
bool ll_fifo_is_empty(struct ll_fifo *handle);

#endif
