/**
 * @file types.h
 * @author salalei (1028609078@qq.com)
 * @brief 常用定义
 * @version 0.1
 * @date 2021-12-04
 *
 * @copyright Copyright (c) 2021
 *
 */
#ifndef __TYPES_H__
#define __TYPES_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/**
 * @brief 通用回调类型
 */
#define USER_CB_TYPE_SEND_COMPLETE (1 << 0)
#define USER_CB_TYPE_RECV_COMPLETE (1 << 1)

/**
 * @brief 求最小值
 *
 * @param x 需要比较的两数之一
 * @param y 需要比较的两数之一
 * @return 返回最小的那个数
 */
#define MIN(x, y) ((x) < (y) ? (x) : (y))
/**
 * @brief 求最大值
 *
 * @param x 需要比较的两数之一
 * @param y 需要比较的两数之一
 * @return 返回最大的那个数
 */
#define MAX(x, y) ((x) > (y) ? (x) : (y))
/**
 * @brief 对指定的数进行限幅
 *
 * @param x 需要限幅的数据
 * @param min 限定的最小值
 * @param max 限定的最大值
 * @return 限幅后的值
 */
#define LIMIT(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))
/**
 * @brief 对指定的数进行绝对值计算
 *
 * @param x 需要进行绝对值计算的数据
 * @return 计算后的数据
 */
#define ABS(x) ((x) < 0 ? -(x) : (x))
/**
 * @brief 求某数组的元素个数
 *
 * @param x 数组名
 * @return 求得的元素个数
 */
#define ARR_NUMB(x) ((size_t)(sizeof(x) / sizeof(x[0])))
/**
 * @brief 求某结构体成员在该结构体的地址偏移量
 *
 * @param type 结构体类型
 * @param mem 结构体成员名称
 * @return 地址偏移量
 */
#define MEMBER_OFFSET(type, mem) ((size_t) & (((type *)0)->mem))
/**
 * @brief 根据某结构体变量中某成员的地址求得其结构体的首地址
 *
 * @param p 结构体变量某成员的地址
 * @param type 结构体类型
 * @param mem 结构体成员名称
 * @return 返回的结构体首地址
 */
#define CONTAINER_OF(p, type, mem) ((type *)((size_t)(p)-MEMBER_OFFSET(type, mem)))

#define __UNUSED __attribute__((unused))
#define __USED __attribute__((used))

struct list_node
{
    struct list_node *prev;
    struct list_node *next;
};

struct object
{
    struct list_node node;
    const char *name;
};

/**
 * @brief 底层注册驱动时可以使用的标志
 */
#define __DRIVER_MODE_WRITE       (1 << 0) //可写入
#define __DRIVER_MODE_READ        (1 << 1) //可读取
#define __DRIVER_MODE_ASYNC_WRITE (1 << 2) //可异步写入
#define __DRIVER_MODE_ASYNC_READ  (1 << 3) //可异步读取

#define DEVICE_MODE_WRITE          (1 << 0) //写入模式
#define DEVICE_MODE_READ           (1 << 1) //读取模式
#define DEVICE_MODE_NONBLOCK_WRITE (1 << 2) //无阻塞写入
#define DEVICE_MODE_NONBLOCK_READ  (1 << 3) //无阻塞读取

struct driver
{
    struct object parent;
    void *priv;
    uint32_t drv_mode : 8;
    uint32_t init_mode : 8;
    uint32_t init_count : 8;
    uint32_t init : 1;
};

static inline void __driver_init(struct driver *p, const char *name, void *priv, int drv_mode)
{
    p->parent.name = name;
    p->priv = priv;
    p->drv_mode = drv_mode;
    p->init_mode = 0;
    p->init_count = 0;
    p->init = 0;
}

#ifdef USING_ASSERT
//需要外部实现该函数
extern void assert_failed(const char *file, const char *function, int line, const char *detail);
#define ASSERT(x) \
    do \
    { \
        if (!(x)) \
        { \
            assert_failed(__FILE__, __func__, __LINE__, #x); \
        } \
    } \
    while (0)
#else
#define ASSERT(x)
#endif

#ifdef __cplusplus
}
#endif

#endif