/**
 * @file ll_drv.c
 * @author salalei (1028609078@qq.com)
 * @brief
 * @version 0.1
 * @date 2021-12-30
 *
 * @copyright Copyright (c) 2021
 *
 */

#include "ll_drv.h"
#include "ll_list.h"
#include "ll_obj.h"

static struct ll_list_node head = LL_LIST_HEAD_INIT_INS(head);

void __ll_drv_init(struct ll_drv *p, const char *name, void *priv, int drv_mode)
{
    LL_ASSERT(p && name);
    p->parent.name = name;
    p->priv = priv;
    if (drv_mode & __LL_DRV_MODE_ASYNC_WRITE)
        drv_mode |= __LL_DRV_MODE_WRITE;
    if (drv_mode & __LL_DRV_MODE_ASYNC_READ)
        drv_mode |= __LL_DRV_MODE_READ;
    p->drv_mode = drv_mode;
    p->init_mode = 0;
    p->init_count = 0;
    p->init = 0;
}

void __ll_drv_register(struct ll_drv *p)
{
    LL_ASSERT(p);
    ll_list_add_tail(&head, &p->parent.node);
}

struct ll_drv *ll_drv_find_by_name(const char *name)
{
    LL_ASSERT(name);
    return (struct ll_drv *)ll_obj_find_by_name(&head, name);
}