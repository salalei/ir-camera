/**
 * @file ll_types.h
 * @author salalei (1028609078@qq.com)
 * @brief 常用定义
 * @version 0.1
 * @date 2021-12-04
 *
 * @copyright Copyright (c) 2021
 *
 */
#ifndef __LL_TYPES_H__
#define __LL_TYPES_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ll_config.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/**
 * @brief 通用回调类型
 */
#define LL_CB_SEND_COMPLETE (1 << 0)
#define LL_CB_RECV_COMPLETE (1 << 1)

/**
 * @brief 求最小值
 *
 * @param x 需要比较的两数之一
 * @param y 需要比较的两数之一
 * @return 返回最小的那个数
 */
#define LL_MIN(x, y) ((x) < (y) ? (x) : (y))
/**
 * @brief 求最大值
 *
 * @param x 需要比较的两数之一
 * @param y 需要比较的两数之一
 * @return 返回最大的那个数
 */
#define LL_MAX(x, y) ((x) > (y) ? (x) : (y))
/**
 * @brief 对指定的数进行限幅
 *
 * @param x 需要限幅的数据
 * @param min 限定的最小值
 * @param max 限定的最大值
 * @return 限幅后的值
 */
#define LL_LIMIT(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))
/**
 * @brief 对指定的数进行绝对值计算
 *
 * @param x 需要进行绝对值计算的数据
 * @return 计算后的数据
 */
#define LL_ABS(x) ((x) < 0 ? -(x) : (x))
/**
 * @brief 求某数组的元素个数
 *
 * @param x 数组名
 * @return 求得的元素个数
 */
#define LL_ARR_NUMB(x) ((size_t)(sizeof(x) / sizeof(x[0])))
/**
 * @brief 求某结构体成员在该结构体的地址偏移量
 *
 * @param type 结构体类型
 * @param mem 结构体成员名称
 * @return 地址偏移量
 */
#define LL_MEMBER_OFFSET(type, mem) ((size_t) & (((type *)0)->mem))
/**
 * @brief 根据某结构体变量中某成员的地址求得其结构体的首地址
 *
 * @param p 结构体变量某成员的地址
 * @param type 结构体类型
 * @param mem 结构体成员名称
 * @return 返回的结构体首地址
 */
#define LL_CONTAINER_OF(p, type, mem) ((type *)((size_t)(p)-LL_MEMBER_OFFSET(type, mem)))

#define __LL_UNUSED __attribute__((unused))
#define __LL_USED   __attribute__((used))

#ifdef __cplusplus
}
#endif

#endif