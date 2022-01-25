/**
 * @file ll_log.h
 * @author salalei (1028609078@qq.com)
 * @brief 日志模块
 * @version 0.1
 * @date 2021-12-02
 *
 * @copyright Copyright (c) 2021
 *
 */
#ifndef __LL_LOG_H__
#define __LL_LOG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ll_types.h"

#define __DO_NOTHING() \
    do \
    { \
    } \
    while (0)

#ifndef LL_LOG_BUF_SIZE
#define LL_LOG_BUF_SIZE 128
#endif

#ifndef LL_LOG_FIFO_SIZE
#define LL_LOG_FIFO_SIZE 256
#endif

#define LL_LOG_LEVEL_DEBUG 0
#define LL_LOG_LEVEL_INFO  1
#define LL_LOG_LEVEL_WARN  2
#define LL_LOG_LEVEL_ERROR 3

#if !defined LL_LOG_LEVEL && defined LL_DEFAULT_LOG_LEVEL
#define LL_LOG_LEVEL LL_DEFAULT_LOG_LEVEL
#endif

#ifdef LL_USING_LOG_TIMESTAMP
#define __TIMESTAMP "[%9lu]"
extern uint32_t xTaskGetTickCount(void);
#define __GET_TIMESTAMP , xTaskGetTickCount()
#else
#define __TIMESTAMP
#define __GET_TIMESTAMP
#endif

#ifdef USING_LOG_LABEL
#define __LOG(label, fmt, ...) ll_printf(__TIMESTAMP #label ": " fmt "\r\n" __GET_TIMESTAMP, ##__VA_ARGS__)
#else
#define __LOG(label, fmt, ...) ll_printf(__TIMESTAMP ": " fmt "\r\n" __GET_TIMESTAMP, ##__VA_ARGS__)
#endif

#if defined LL_USING_LOG && defined LL_LOG_LEVEL && LL_LOG_LEVEL >= LL_LOG_LEVEL_DEBUG
#define LL_DEBUG(fmt, ...) __LOG(DEBUG, fmt, ##__VA_ARGS__)
#else
#define LL_DEBUG(fmt, ...) __DO_NOTHING()
#endif

#if defined LL_USING_LOG && defined LL_LOG_LEVEL && LL_LOG_LEVEL >= LL_LOG_LEVEL_INFO
#define LL_INFO(fmt, ...) __LOG(INFO, fmt, ##__VA_ARGS__)
#else
#define LL_INFO(fmt, ...) __DO_NOTHING()
#endif

#if defined LL_USING_LOG && defined LL_LOG_LEVEL && LL_LOG_LEVEL >= LL_LOG_LEVEL_WARN
#define LL_WARN(fmt, ...) __LOG(WARN, fmt, ##__VA_ARGS__)
#else
#define LL_WARN(fmt, ...) __DO_NOTHING()
#endif

#if defined LL_USING_LOG && defined LL_LOG_LEVEL && LL_LOG_LEVEL >= LL_LOG_LEVEL_ERROR
#define LL_ERROR(fmt, ...) __LOG(ERROR, fmt, ##__VA_ARGS__)
#else
#define LL_ERROR(fmt, ...) __DO_NOTHING()
#endif

int ll_printf(char *fmt, ...);
int ll_log_init(void);

#ifdef __cplusplus
}
#endif

#endif