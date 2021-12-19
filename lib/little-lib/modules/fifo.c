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

#include "fifo.h"

#include <string.h>

/**
 * @brief 初始化fifo
 *
 * @param handle 指向fifo的句柄指针
 * @param buf 指向用户提供的缓存区
 * @param size 缓存区大小
 */
void fifo_init(struct fifo_handle *handle, const uint8_t *buf, size_t size)
{
    ASSERT(handle && buf && size > 0 && size < 0x80000000);

    handle->buf = (uint8_t *)buf;
    handle->size = size;
    handle->read_pos = 0;
    handle->write_pos = 0;
}

/**
 * @brief 注销fifo
 *
 * @param handle 指向fifo的句柄指针
 */
void fifo_deinit(struct fifo_handle *handle)
{
    memset(handle, 0, sizeof(struct fifo_handle));
}

/**
 * @brief 读取fifo中的已存入数据长度
 *
 * @param handle 指向fifo的句柄指针
 * @return size_t 数据长度
 */
size_t fifo_data_size(struct fifo_handle *handle)
{
    int read_pos = handle->read_pos;
    int write_pos = handle->write_pos;

    ASSERT(handle);

    return FIFO_READ_ROUND_BIT(read_pos) == FIFO_READ_ROUND_BIT(write_pos) ? FIFO_READ_POS(write_pos) - FIFO_READ_POS(read_pos)
                                                                           : FIFO_READ_POS(write_pos) + handle->size - FIFO_READ_POS(read_pos);
}

/**
 * @brief 读取fifo中的空余区域长度
 *
 * @param handle 指向fifo的句柄指针
 * @return size_t 空闲区域长度
 */
size_t fifo_free_size(struct fifo_handle *handle)
{
    int read_pos = handle->read_pos;
    int write_pos = handle->write_pos;

    ASSERT(handle);

    return FIFO_READ_ROUND_BIT(read_pos) == FIFO_READ_ROUND_BIT(write_pos) ? handle->size - FIFO_READ_POS(write_pos) + FIFO_READ_POS(read_pos)
                                                                           : FIFO_READ_POS(read_pos) - FIFO_READ_POS(write_pos);
}

/**
 * @brief 往fifo写入指定长度数据
 *
 * @param handle 指向fifo的句柄指针
 * @param buf 指向用户的数据的指针
 * @param size 用户需要写入的长度
 * @return size_t 实际写入的长度
 */
size_t fifo_push(struct fifo_handle *handle, const uint8_t *buf, size_t size)
{
    size_t free_len;
    int write_pos;
    size_t pos;

    ASSERT(handle && buf);
    free_len = fifo_free_size(handle);
    if (free_len == 0 || size == 0)
        return 0;

    size = free_len < size ? free_len : size;

    write_pos = handle->write_pos;
    pos = FIFO_READ_POS(write_pos);
    if (pos + size >= handle->size)
    {
        memcpy(&handle->buf[pos], buf, handle->size - pos);
        memcpy(handle->buf, &buf[handle->size - pos], size - handle->size + pos);
        handle->write_pos = (FIFO_READ_ROUND_BIT(write_pos) + FIFO_ROUND_BIT) | ((pos + size) % handle->size);
    }
    else
    {
        memcpy(&handle->buf[pos], buf, size);
        handle->write_pos = write_pos + size;
    }

    return size;
}

/**
 * @brief 强制往fifo写入数据，若长度超过可写范围，将会覆盖之前的数据
 *
 * @param handle 指向fifo的句柄指针
 * @param buf 指向用户的数据的指针
 * @param size 用户需要写入的长度
 * @return bool 是否写入成功
 */
bool fifo_push_force(struct fifo_handle *handle, const uint8_t *buf, size_t size)
{
    size_t free_len;

    ASSERT(handle && buf);
    if (size == 0 || size > handle->size)
        return false;

    free_len = fifo_free_size(handle);

    if (size > free_len)
    {
        int read_pos;
        size_t pos;
        size_t read_diff = size - free_len;

        read_pos = handle->read_pos;
        pos = FIFO_READ_POS(read_pos);
        if (pos + read_diff >= handle->size)
            handle->read_pos = (FIFO_READ_ROUND_BIT(read_pos) + FIFO_ROUND_BIT) | ((pos + read_diff) % handle->size);
        else
            handle->read_pos = read_pos + read_diff;
    }
    fifo_push(handle, buf, size);

    return true;
}

/**
 * @brief 从fifo取出但不清除数据
 *
 * @param handle 指向fifo的句柄指针
 * @param buf 指向用户用来接收缓存区数据的指针
 * @param offset 读取的偏移量
 * @param size 用户需要读取的数据大小
 * @return size_t 实际读取的数据大小
 */
size_t fifo_peek(struct fifo_handle *handle, uint8_t *buf, size_t offset, size_t size)
{
    size_t data_len;
    size_t pos;

    ASSERT(handle && buf);
    data_len = fifo_data_size(handle);
    if (size == 0 || data_len == 0 || data_len <= offset)
        return 0;

    size = data_len - offset < size ? data_len - offset : size;

    pos = FIFO_READ_POS(handle->read_pos);
    if (pos + offset >= handle->size)
        memcpy(&buf[0], &handle->buf[pos + offset - handle->size], size);
    else
    {
        size_t _cur_read_pos = pos + offset;

        if (_cur_read_pos + size >= handle->size)
        {
            memcpy(&buf[0], &handle->buf[_cur_read_pos], handle->size - _cur_read_pos);
            memcpy(&buf[handle->size - _cur_read_pos], &handle->buf[0], size - handle->size + _cur_read_pos);
        }
        else
            memcpy(&buf[0], &handle->buf[_cur_read_pos], size);
    }

    return size;
}

/**
 * @brief 从fifo取出数据
 *
 * @param handle 指向fifo的句柄指针
 * @param buf 指向用户用来接收缓存区数据的指针
 * @param size 用户需要读取的数据大小
 * @return size_t 实际读取的数据大小
 */
size_t fifo_pop(struct fifo_handle *handle, uint8_t *buf, size_t size)
{
    size_t data_len;
    int read_pos;
    size_t pos;

    ASSERT(handle && buf);
    data_len = fifo_data_size(handle);
    if (size == 0 || data_len == 0)
        return 0;

    size = data_len < size ? data_len : size;

    read_pos = handle->read_pos;
    pos = FIFO_READ_POS(read_pos);
    if (pos + size >= handle->size)
    {
        memcpy(&buf[0], &handle->buf[pos], handle->size - pos);
        memcpy(&buf[handle->size - pos], &handle->buf[0], size - handle->size + pos);
        handle->read_pos = (FIFO_READ_ROUND_BIT(read_pos) + FIFO_ROUND_BIT) | ((pos + size) % handle->size);
    }
    else
    {
        memcpy(&buf[0], &handle->buf[pos], size);
        handle->read_pos = read_pos + size;
    }

    return size;
}

/**
 * @brief 清除缓存区中指定长度的数据
 *
 * @param handle 指向fifo的句柄指针
 * @param size 指定长度
 */
void fifo_clear(struct fifo_handle *handle, size_t size)
{
    size_t data_len;
    int read_pos;
    size_t pos;

    ASSERT(handle);
    data_len = fifo_data_size(handle);
    if (size == 0 || data_len == 0)
        return;

    size = size < data_len ? size : data_len;
    read_pos = handle->read_pos;
    pos = FIFO_READ_POS(read_pos);
    if (pos + size >= handle->size)
        handle->read_pos = (FIFO_READ_ROUND_BIT(read_pos) + FIFO_ROUND_BIT) | ((pos + size) % handle->size);
    else
        handle->read_pos = read_pos + size;
}

/**
 * @brief 清除缓存区中所有数据
 *
 * @param handle 指向fifo的句柄指针
 */
void fifo_clear_all(struct fifo_handle *handle)
{
    ASSERT(handle);
    handle->read_pos = handle->write_pos;
}

/**
 * @brief 判断fifo是否数据已满
 * 
 * @param handle 指向FIFO的指针
 * @return true 
 * @return false 
 */
bool fifo_is_full(struct fifo_handle *handle)
{
    ASSERT(handle);
    return (fifo_free_size(handle) == 0);
}

/**
 * @brief 判断fifo是否数据为空
 * 
 * @param handle 指向FIFO的指针
 * @return true 
 * @return false 
 */
bool fifo_is_empty(struct fifo_handle *handle)
{
    ASSERT(handle);
    return (fifo_data_size(handle) == 0);
}