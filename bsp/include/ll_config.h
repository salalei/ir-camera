/**
 * @file config.h
 * @author salalei (1028609078@qq.com)
 * @brief 配置文件
 * @version 0.1
 * @date 2021-12-02
 *
 * @copyright Copyright (c) 2021
 *
 */
#ifndef __CONFIG_H__
#define __CONFIG_H__

// #define LL_USING_LOG
#define LL_USING_LOG_TIMESTAMP
#define USING_LOG_LABEL
// #define USING_ASYNC_LOG
#define LL_LOG_BUF_SIZE      128
#define LL_LOG_FIFO_SIZE     256
#define LL_DEFAULT_LOG_LEVEL LL_LOG_LEVEL_DEBUG
#define LOG_WRITE_FIFO_SIZE  128
#define LOG_READ_FIFO_SIZE   128
#define LOG_SERIAL_NAME      "uart0"
#define LOG_SERIAL_BAUD      256000UL
#define LOG_SERIAL_STOP_BIT  LL_SERIAL_STOP_1BIT
#define LOG_SERIAL_DATA_BIT  8UL
#define LOG_SERIAL_PARITY    LL_SERIAL_PARITY_NONE
#define LOG_SERIAL_FLOW_CTRL LL_SERIAL_FLOW_CTRL_NONE

// #define LL_USING_ASSERT

#endif