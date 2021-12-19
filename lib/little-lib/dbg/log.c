/**
 * @file log.c
 * @author salalei (1028609078@qq.com)
 * @brief 日志模块
 * @version 0.1
 * @date 2021-12-02
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "../drivers/serial.h"
#include "../modules/fifo.h"
#include "init.h"
#include "log.h"

#include <stdarg.h>

static struct serial *serial = NULL;
#ifdef USING_ASYNC_LOG
FIFO_STATIC_DEF(log_fifo, LOG_FIFO_SIZE);
#endif

static inline int output(const char *buf, size_t size)
{
    size_t len = size;

    while (len)
    {
        uint32_t temp;
        ssize_t res;
        temp = taskENTER_CRITICAL_FROM_ISR();
#ifdef USING_ASYNC_LOG
        res = fifo_push(&log_fifo, (const uint8_t *)buf, len);
#else
        res = serial_write(serial, buf, size);
        if (res < 0)
        {
            taskEXIT_CRITICAL_FROM_ISR(temp);
            return res;
        }
#endif
        taskEXIT_CRITICAL_FROM_ISR(temp);
        len -= res;
        buf += res;
    }
    return 0;
}

int kprintf(char *fmt, ...)
{
    char buf[LOG_BUF_SIZE];
    va_list args;
    int size;

    if (serial)
    {
        va_start(args, fmt);
        size = vsnprintf(buf, LOG_BUF_SIZE, fmt, args);
        if (size < 0)
            return size;
        va_end(args);
        return output(buf, size);
    }
    else
        return -EIO;
}

#ifdef USING_ASYNC_LOG
void async_output(void *param)
{
    static uint8_t buf[64];
    size_t size;

    while (1)
    {
        size = fifo_pop(&log_fifo, buf, sizeof(buf));
        serial_write(serial, buf, size);
        vTaskDelay(2);
    }
}
#endif

int log_init(void)
{
    int res;
    struct serial_conf conf = {
        .baud = LOG_SERIAL_BAUD,
        .stop_bit = LOG_SERIAL_STOP_BIT,
        .parity = LOG_SERIAL_PARITY,
        .data_bit = LOG_SERIAL_DATA_BIT,
        .flow_ctrl = LOG_SERIAL_FLOW_CTRL,
    };
    static uint8_t write_buf[LOG_WRITE_FIFO_SIZE];
    static uint8_t read_buf[LOG_READ_FIFO_SIZE];

    serial = serial_find_by_name(LOG_SERIAL_NAME);
    if (!serial)
        return -ENODEV;
    res = serial_init(serial,
                      DEVICE_MODE_WRITE | DEVICE_MODE_NONBLOCK_READ,
                      write_buf, LOG_WRITE_FIFO_SIZE,
                      read_buf, LOG_READ_FIFO_SIZE);
    if (res)
        return res;
    res = serial_config(serial, &conf);
    if (res)
        return res;

#ifdef USING_ASYNC_LOG
    if (xTaskCreate(async_output, "log", 512, NULL, 1, NULL) != pdPASS)
        return -EINVAL;
#endif
    return 0;
}

LATE_INITCALL(log_init);