/**
 * @file init.h
 * @author salalei (1028609078@qq.com)
 * @brief 自动初始化
 * @version 0.1
 * @date 2021-12-18
 *
 * @copyright Copyright (c) 2021
 *
 */
#ifndef __INIT_H__
#define __INIT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"

typedef int (*__initcall_t)(void);

#define __INITCALL(fn, level) \
    const static __initcall_t __initcall_##fn##level __USED \
        __attribute__((__section__(".initcall" #level))) = fn

#define EARLY_INITCALL(fn)  __INITCALL(fn, 0)
#define BOARD_INITCALL(fn)  __INITCALL(fn, 1)
#define DEVICE_INITCALL(fn) __INITCALL(fn, 2)
#define LATE_INITCALL(fn)   __INITCALL(fn, 3)
#define APP_INITCALL(fn)    __INITCALL(fn, 4)

#ifdef __cplusplus
}
#endif

#endif