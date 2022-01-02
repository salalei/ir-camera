/**
 * @file fifo.c
 * @author salalei (1028609078@qq.com)
 * @brief fifo
 * @version 0.1
 * @date 2021-12-04
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "ll_fifo.h"
#include "ll_assert.h"

#include <string.h>

/**
 * @brief 初始化fifo
 *
 * @param fifo 指向fifo的句柄指针
 * @param buf 指向用户提供的缓存区
 * @param size 缓存区大小
 */
void ll_fifo_init(struct ll_fifo *fifo, const uint8_t *buf, size_t size)
{
    LL_ASSERT(fifo && buf && size > 0 && size < 0x80000000);

    fifo->buf = (uint8_t *)buf;
    fifo->size = size;
    fifo->read_pos = 0;
    fifo->write_pos = 0;
}

/**
 * @brief 注销fifo
 *
 * @param fifo 指向fifo的句柄指针
 */
void ll_fifo_deinit(struct ll_fifo *fifo)
{
    memset(fifo, 0, sizeof(struct ll_fifo));
}

/**
 * @brief 读取fifo中的已存入数据长度
 *
 * @param fifo 指向fifo的句柄指针
 * @return size_t 数据长度
 */
size_t ll_fifo_data_size(struct ll_fifo *fifo)
{
    int read_pos = fifo->read_pos;
    int write_pos = fifo->write_pos;

    LL_ASSERT(fifo);

    return LL_FIFO_READ_ROUND_BIT(read_pos) == LL_FIFO_READ_ROUND_BIT(write_pos) ? LL_FIFO_READ_POS(write_pos) - LL_FIFO_READ_POS(read_pos)
                                                                                 : LL_FIFO_READ_POS(write_pos) + fifo->size - LL_FIFO_READ_POS(read_pos);
}

/**
 * @brief 读取fifo中的空余区域长度
 *
 * @param fifo 指向fifo的句柄指针
 * @return size_t 空闲区域长度
 */
size_t ll_fifo_free_size(struct ll_fifo *fifo)
{
    int read_pos = fifo->read_pos;
    int write_pos = fifo->write_pos;

    LL_ASSERT(fifo);

    return LL_FIFO_READ_ROUND_BIT(read_pos) == LL_FIFO_READ_ROUND_BIT(write_pos) ? fifo->size - LL_FIFO_READ_POS(write_pos) + LL_FIFO_READ_POS(read_pos)
                                                                                 : LL_FIFO_READ_POS(read_pos) - LL_FIFO_READ_POS(write_pos);
}

/**
 * @brief 往fifo写入指定长度数据
 *
 * @param fifo 指向fifo的句柄指针
 * @param buf 指向用户的数据的指针
 * @param size 用户需要写入的长度
 * @return size_t 实际写入的长度
 */
size_t ll_fifo_push(struct ll_fifo *fifo, const uint8_t *buf, size_t size)
{
    size_t free_len;
    int write_pos;
    size_t pos;

    LL_ASSERT(fifo && buf);
    free_len = ll_fifo_free_size(fifo);
    if (free_len == 0 || size == 0)
        return 0;

    size = free_len < size ? free_len : size;

    write_pos = fifo->write_pos;
    pos = LL_FIFO_READ_POS(write_pos);
    if (pos + size >= fifo->size)
    {
        memcpy(&fifo->buf[pos], buf, fifo->size - pos);
        memcpy(fifo->buf, &buf[fifo->size - pos], size - fifo->size + pos);
        fifo->write_pos = (LL_FIFO_READ_ROUND_BIT(write_pos) + LL_FIFO_ROUND_BIT) | ((pos + size) % fifo->size);
    }
    else
    {
        memcpy(&fifo->buf[pos], buf, size);
        fifo->write_pos = write_pos + size;
    }

    return size;
}

/**
 * @brief 强制往fifo写入数据，若长度超过可写范围，将会覆盖之前的数据
 *
 * @param fifo 指向fifo的句柄指针
 * @param buf 指向用户的数据的指针
 * @param size 用户需要写入的长度
 * @return bool 是否写入成功
 */
bool ll_fifo_push_force(struct ll_fifo *fifo, const uint8_t *buf, size_t size)
{
    size_t free_len;

    LL_ASSERT(fifo && buf);
    if (size == 0 || size > fifo->size)
        return false;

    free_len = ll_fifo_free_size(fifo);

    if (size > free_len)
    {
        int read_pos;
        size_t pos;
        size_t read_diff = size - free_len;

        read_pos = fifo->read_pos;
        pos = LL_FIFO_READ_POS(read_pos);
        if (pos + read_diff >= fifo->size)
            fifo->read_pos = (LL_FIFO_READ_ROUND_BIT(read_pos) + LL_FIFO_ROUND_BIT) | ((pos + read_diff) % fifo->size);
        else
            fifo->read_pos = read_pos + read_diff;
    }
    ll_fifo_push(fifo, buf, size);

    return true;
}

/**
 * @brief 从fifo取出但不清除数据
 *
 * @param fifo 指向fifo的句柄指针
 * @param buf 指向用户用来接收缓存区数据的指针
 * @param offset 读取的偏移量
 * @param size 用户需要读取的数据大小
 * @return size_t 实际读取的数据大小
 */
size_t ll_fifo_peek(struct ll_fifo *fifo, uint8_t *buf, size_t offset, size_t size)
{
    size_t data_len;
    size_t pos;

    LL_ASSERT(fifo && buf);
    data_len = ll_fifo_data_size(fifo);
    if (size == 0 || data_len == 0 || data_len <= offset)
        return 0;

    size = data_len - offset < size ? data_len - offset : size;

    pos = LL_FIFO_READ_POS(fifo->read_pos);
    if (pos + offset >= fifo->size)
        memcpy(&buf[0], &fifo->buf[pos + offset - fifo->size], size);
    else
    {
        size_t _cur_read_pos = pos + offset;

        if (_cur_read_pos + size >= fifo->size)
        {
            memcpy(&buf[0], &fifo->buf[_cur_read_pos], fifo->size - _cur_read_pos);
            memcpy(&buf[fifo->size - _cur_read_pos], &fifo->buf[0], size - fifo->size + _cur_read_pos);
        }
        else
            memcpy(&buf[0], &fifo->buf[_cur_read_pos], size);
    }

    return size;
}

/**
 * @brief 从fifo取出数据
 *
 * @param fifo 指向fifo的句柄指针
 * @param buf 指向用户用来接收缓存区数据的指针
 * @param size 用户需要读取的数据大小
 * @return size_t 实际读取的数据大小
 */
size_t ll_fifo_pop(struct ll_fifo *fifo, uint8_t *buf, size_t size)
{
    size_t data_len;
    int read_pos;
    size_t pos;

    LL_ASSERT(fifo && buf);
    data_len = ll_fifo_data_size(fifo);
    if (size == 0 || data_len == 0)
        return 0;

    size = data_len < size ? data_len : size;

    read_pos = fifo->read_pos;
    pos = LL_FIFO_READ_POS(read_pos);
    if (pos + size >= fifo->size)
    {
        memcpy(&buf[0], &fifo->buf[pos], fifo->size - pos);
        memcpy(&buf[fifo->size - pos], &fifo->buf[0], size - fifo->size + pos);
        fifo->read_pos = (LL_FIFO_READ_ROUND_BIT(read_pos) + LL_FIFO_ROUND_BIT) | ((pos + size) % fifo->size);
    }
    else
    {
        memcpy(&buf[0], &fifo->buf[pos], size);
        fifo->read_pos = read_pos + size;
    }

    return size;
}

/**
 * @brief 清除缓存区中指定长度的数据
 *
 * @param fifo 指向fifo的句柄指针
 * @param size 指定长度
 */
void ll_fifo_clear(struct ll_fifo *fifo, size_t size)
{
    size_t data_len;
    int read_pos;
    size_t pos;

    LL_ASSERT(fifo);
    data_len = ll_fifo_data_size(fifo);
    if (size == 0 || data_len == 0)
        return;

    size = size < data_len ? size : data_len;
    read_pos = fifo->read_pos;
    pos = LL_FIFO_READ_POS(read_pos);
    if (pos + size >= fifo->size)
        fifo->read_pos = (LL_FIFO_READ_ROUND_BIT(read_pos) + LL_FIFO_ROUND_BIT) | ((pos + size) % fifo->size);
    else
        fifo->read_pos = read_pos + size;
}

/**
 * @brief 清除缓存区中所有数据
 *
 * @param fifo 指向fifo的句柄指针
 */
void ll_fifo_clear_all(struct ll_fifo *fifo)
{
    LL_ASSERT(fifo);
    fifo->read_pos = fifo->write_pos;
}

/**
 * @brief 判断fifo是否数据已满
 *
 * @param fifo 指向FIFO的指针
 * @return true
 * @return false
 */
bool ll_fifo_is_full(struct ll_fifo *fifo)
{
    LL_ASSERT(fifo);
    return (ll_fifo_free_size(fifo) == 0);
}

/**
 * @brief 判断fifo是否数据为空
 *
 * @param fifo 指向FIFO的指针
 * @return true
 * @return false
 */
bool ll_fifo_is_empty(struct ll_fifo *fifo)
{
    LL_ASSERT(fifo);
    return (ll_fifo_data_size(fifo) == 0);
}