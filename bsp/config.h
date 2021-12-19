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

#define USING_LOG
#define USING_LOG_TIMESTAMP
#define USING_LOG_LABEL
#define USING_ASYNC_LOG
#define DEFAULT_LOG_LEVEL    LOG_LEVEL_DEBUG
#define LOG_WRITE_FIFO_SIZE  128
#define LOG_READ_FIFO_SIZE   128
#define LOG_SERIAL_NAME      "uart0"
#define LOG_SERIAL_BAUD      256000UL
#define LOG_SERIAL_STOP_BIT  SERIAL_STOP_2BIT
#define LOG_SERIAL_DATA_BIT  8UL
#define LOG_SERIAL_PARITY    SERIAL_PARITY_NONE
#define LOG_SERIAL_FLOW_CTRL SERIAL_FLOW_CTRL_NONE

// #define USING_ASSERT

#endif