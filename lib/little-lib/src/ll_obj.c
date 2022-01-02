/**
 * @file ll_obj.c
 * @author salalei (1028609078@qq.com)
 * @brief obj
 * @version 0.1
 * @date 2021-12-05
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "ll_obj.h"
#include "ll_assert.h"
#include <string.h>

/**
 * @brief 按指定名字查找
 *
 * @param head 指向链头的指针
 * @param name 想要查找的名字
 * @return struct ll_obj* 若查找成功返回地址，失败则返回NULL
 */
struct ll_obj *ll_obj_find_by_name(struct ll_list_node *head, const char *name)
{
    struct ll_list_node *node;
    struct ll_list_node *temp;
    struct ll_obj *p;
    LL_ASSERT(name);

    LL_FOR_EACH_LIST_NODE_SAFE(head, node, temp)
    {
        p = (struct ll_obj *)node;
        if (p->name && !strcmp(p->name, name))
            return p;
    }
    return NULL;
}

/**
 * @brief 按索引查找
 *
 * @param head 指向链头的指针
 * @param index 想要查找的索引
 * @return struct ll_obj* 若查找成功返回地址，失败则返回NULL
 */
struct ll_obj *ll_obj_find_by_index(struct ll_list_node *head, int index)
{
    struct ll_list_node *node;
    struct ll_list_node *temp;

    LL_FOR_EACH_LIST_NODE_SAFE(head, node, temp)
    {
        if (!index)
            return (struct ll_obj *)node;
        else
            index--;
    }
    return NULL;
}

/**
 * @brief 检查某个obj是否已存在链表中
 *
 * @param head 指向链头的指针
 * @param obj 要检查的obj
 * @return true
 * @return false
 */
bool ll_obj_is_exist(struct ll_list_node *head, struct ll_list_node *obj)
{
    struct ll_list_node *node;
    struct ll_list_node *temp;

    LL_FOR_EACH_LIST_NODE_SAFE(head, node, temp)
    {
        if (node == obj)
            return true;
    }
    return false;
}