/**
 * @file log.h
 * @author salalei (1028609078@qq.com)
 * @brief 日志模块
 * @version 0.1
 * @date 2021-12-02
 *
 * @copyright Copyright (c) 2021
 *
 */
#ifndef __LOG_H__
#define __LOG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "../include/types.h"

#define DO_NOTHING() \
    do \
    { \
    } \
    while (0)

#ifndef LOG_BUF_SIZE
#define LOG_BUF_SIZE 128
#endif

#ifndef LOG_FIFO_SIZE
#define LOG_FIFO_SIZE 256
#endif

#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO  1
#define LOG_LEVEL_WARN  2
#define LOG_LEVEL_ERROR 3

#if !defined LOG_LEVEL && defined DEFAULT_LOG_LEVEL
#define LOG_LEVEL DEFAULT_LOG_LEVEL
#endif

#ifdef USING_LOG_TIMESTAMP
#define __TIMESTAMP "[%9lu]"
extern uint32_t xTaskGetTickCount(void);
#define GET_TIMESTAMP , xTaskGetTickCount()
#else
#define __TIMESTAMP
#define GET_TIMESTAMP
#endif

#ifdef USING_LOG_LABEL
#define __LOG(label, fmt, ...) kprintf(__TIMESTAMP #label ": " fmt "\r\n" GET_TIMESTAMP, ##__VA_ARGS__)
#else
#define __LOG(label, fmt, ...) kprintf(__TIMESTAMP ": " fmt "\r\n" GET_TIMESTAMP, ##__VA_ARGS__)
#endif

#if defined USING_LOG && defined LOG_LEVEL && LOG_LEVEL >= LOG_LEVEL_DEBUG
#define DEBUG(fmt, ...) __LOG(DEBUG, fmt, ##__VA_ARGS__)
#else
#define DEBUG(fmt, ...) DO_NOTHING()
#endif

#if defined USING_LOG && defined LOG_LEVEL && LOG_LEVEL >= LOG_LEVEL_INFO
#define INFO(fmt, ...) __LOG(INFO, fmt, ##__VA_ARGS__)
#else
#define INFO(fmt, ...) DO_NOTHING()
#endif

#if defined USING_LOG && defined LOG_LEVEL && LOG_LEVEL >= LOG_LEVEL_WARN
#define WARN(fmt, ...) __LOG(WARN, fmt, ##__VA_ARGS__)
#else
#define WARN(fmt, ...) DO_NOTHING()
#endif

#if defined USING_LOG && defined LOG_LEVEL && LOG_LEVEL >= LOG_LEVEL_ERROR
#define ERROR(fmt, ...) __LOG(ERROR, fmt, ##__VA_ARGS__)
#else
#define ERROR(fmt, ...) DO_NOTHING()
#endif

int kprintf(char *fmt, ...);
int log_init(void);

#ifdef __cplusplus
}
#endif

#endif