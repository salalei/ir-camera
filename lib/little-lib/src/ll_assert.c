/**
 * @file ll_assert.c
 * @author salalei (1028609078@qq.com)
 * @brief 断言
 * @version 0.1
 * @date 2021-12-04
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "ll_log.h"

void ll_assert_failed(const char *file, const char *function, int line, const char *detail)
{
    ll_printf("LL_ASSERT\r\n");
    ll_printf("file:%s\r\n", file);
    ll_printf("function:%s\r\n", function);
    ll_printf("line:%d\r\n", line);
    ll_printf("detail:%s\r\n", detail);
    while (1)
    {
    }
}