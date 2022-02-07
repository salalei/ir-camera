/**
 * @file ll_assert.h
 * @author salalei (1028609078@qq.com)
 * @brief
 * @version 0.1
 * @date 2022-01-02
 *
 * @copyright Copyright (c) 2022
 *
 */
#ifndef __LL_ASSERT_H__
#define __LL_ASSERT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ll_config.h"

#ifdef LL_USING_ASSERT
//需要外部实现该函数
extern void ll_assert_failed(const char *file, const char *function, int line, const char *detail);
#define LL_ASSERT(x) \
    do \
    { \
        if (!(x)) \
        { \
            ll_assert_failed(__FILE__, __func__, __LINE__, #x); \
        } \
    } \
    while (0)
#else
#define LL_ASSERT(x)
#endif

#ifdef __cplusplus
}
#endif

#endif