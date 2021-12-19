/**
 * @file object.h
 * @author salalei (1028609078@qq.com)
 * @brief 基类
 * @version 0.1
 * @date 2021-12-05
 *
 * @copyright Copyright (c) 2021
 *
 */
#ifndef __OBJECT_H__
#define __OBJECT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "list.h"

struct object *object_find_by_name(struct list_node *head, const char *name);
struct object *object_find_by_index(struct list_node *head, int index);
bool object_is_exist(struct list_node *head, struct list_node *obj);

#ifdef __cplusplus
}
#endif

#endif