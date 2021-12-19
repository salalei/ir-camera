/**
 * @file assert.c
 * @author salalei (1028609078@qq.com)
 * @brief 断言
 * @version 0.1
 * @date 2021-12-04
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "log.h"

void assert_failed(const char *file, const char *function, int line, const char *detail)
{
    kprintf("ASSERT\r\n");
    kprintf("file:%s\r\n", file);
    kprintf("function:%s\r\n", function);
    kprintf("line:%d\r\n", line);
    kprintf("detail:%s\r\n", detail);
    while (1)
    {
    }
}