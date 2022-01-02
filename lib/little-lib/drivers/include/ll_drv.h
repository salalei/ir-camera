/**
 * @file ll_drv.h
 * @author salalei (1028609078@qq.com)
 * @brief
 * @version 0.1
 * @date 2021-12-30
 *
 * @copyright Copyright (c) 2021
 *
 */
#ifndef __LL_DRV_H__
#define __LL_DRV_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ll_assert.h"
#include "ll_obj.h"

#define LL_DRV_MODE_WRITE          (1 << 0) //写入模式
#define LL_DRV_MODE_READ           (1 << 1) //读取模式
#define LL_DRV_MODE_NONBLOCK_WRITE (1 << 2) //无阻塞写入
#define LL_DRV_MODE_NONBLOCK_READ  (1 << 3) //无阻塞读取

struct ll_drv
{
    struct ll_obj parent;
    void *priv;
    uint32_t drv_mode : 8;
    uint32_t init_mode : 8;
    uint32_t init_count : 8;
    uint32_t init : 1;
};

/**
 * @brief 底层注册驱动时可以使用的标志
 */
#define __LL_DRV_MODE_WRITE       (1 << 0) //可写入
#define __LL_DRV_MODE_READ        (1 << 1) //可读取
#define __LL_DRV_MODE_ASYNC_WRITE (1 << 2) //可异步写入
#define __LL_DRV_MODE_ASYNC_READ  (1 << 3) //可异步读取

void __ll_drv_init(struct ll_drv *p, const char *name, void *priv, int drv_mode);
void __ll_drv_register(struct ll_drv *p);
struct ll_drv *ll_drv_find_by_name(const char *name);

#ifdef __cplusplus
}
#endif

#endif