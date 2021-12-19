/**
 * @file object.c
 * @author salalei (1028609078@qq.com)
 * @brief 基础句柄
 * @version 0.1
 * @date 2021-12-05
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "object.h"

#include <string.h>

/**
 * @brief 按指定名字查找
 *
 * @param head 指向链头的指针
 * @param name 想要查找的名字
 * @return struct object* 若查找成功返回地址，失败则返回NULL
 */
struct object *object_find_by_name(struct list_node *head, const char *name)
{
    struct list_node *node;
    struct list_node *temp;
    struct object *p;
    ASSERT(name);

    FOR_EACH_LIST_NODE_SAFE(head, node, temp)
    {
        p = (struct object *)node;
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
 * @return struct object* 若查找成功返回地址，失败则返回NULL
 */
struct object *object_find_by_index(struct list_node *head, int index)
{
    struct list_node *node;
    struct list_node *temp;

    FOR_EACH_LIST_NODE_SAFE(head, node, temp)
    {
        if (!index)
            return (struct object *)node;
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
bool object_is_exist(struct list_node *head, struct list_node *obj)
{
    struct list_node *node;
    struct list_node *temp;

    FOR_EACH_LIST_NODE_SAFE(head, node, temp)
    {
        if (node == obj)
            return true;
    }
    return false;
}