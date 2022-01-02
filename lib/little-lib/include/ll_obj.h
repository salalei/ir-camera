/**
 * @file ll_obj.h
 * @author salalei (1028609078@qq.com)
 * @brief
 * @version 0.1
 * @date 2021-12-05
 *
 * @copyright Copyright (c) 2021
 *
 */
#ifndef __LL_OBJ_H__
#define __LL_OBJ_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ll_list.h"

struct ll_obj
{
    struct ll_list_node node;
    const char *name;
};

struct ll_obj *ll_obj_find_by_name(struct ll_list_node *head, const char *name);
struct ll_obj *ll_obj_find_by_index(struct ll_list_node *head, int index);
bool ll_obj_is_exist(struct ll_list_node *head, struct ll_list_node *obj);

#ifdef __cplusplus
}
#endif

#endif