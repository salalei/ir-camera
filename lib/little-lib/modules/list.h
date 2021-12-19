/**
 * @file list.h
 * @author salalei (1028609078@qq.com)
 * @brief 双向循环链表(参考linux)
 * @version 0.1
 * @date 2021-12-04
 *
 * @copyright Copyright (c) 2021
 *
 */
#ifndef __LIST_H__
#define __LIST_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "../include/types.h"

/**
 * @brief 创建链表头的实体
 *
 * @param node 需要创建的链表头
 */
#define LIST_HEAD_INIT_INS(node) \
    { \
        .prev = &(node), \
        .next = &(node) \
    }

/**
 * @brief 正向遍历所有节点
 *
 * @param head 指向要遍历的链表头指针
 * @param node 链表节点指针，由用户提供
 */
#define FOR_EACH_LIST_NODE(head, node) \
    for ((node) = (head)->next; (node) != (head); (node) = (node)->next)

/**
 * @brief 正向遍历所有节点，可以在遍历的时候移除节点
 *
 * @param head 指向要遍历的链表头指针
 * @param node 链表节点指针，由用户提供
 * @param temp 储存当前节点的下一个节点的地址
 */
#define FOR_EACH_LIST_NODE_SAFE(head, node, temp) \
    for ((node) = (head)->next, (temp) = (node)->next; (node) != (head); (node) = (temp), (temp) = (node)->next)

/**
 * @brief 逆向遍历所有节点
 *
 * @param head 指向要遍历的链表头指针
 * @param node 链表节点指针，由用户提供
 */
#define FOR_EACH_LIST_NODE_REVERSE(head, node) \
    for ((node) = (head)->prev; (node) != (head); (node) = (node)->prev)

/**
 * @brief 逆向遍历所有节点，可以在遍历的时候移除节点
 *
 * @param head 指向要遍历的链表头指针
 * @param node 链表节点指针，由用户提供
 * @param temp 储存当前节点的上一个节点的地址
 */
#define FOR_EACH_LIST_NODE_REVERSE_SAFE(head, node, temp) \
    for ((node) = (head)->prev, (temp) = (node)->prev; (node) != (head); (node) = (temp), (temp) = (node)->prev)

/**
 * @brief 正向遍历某结构体中的节点
 *
 * @param head 指向要遍历的链表头指针
 * @param p 指向某结构体的指针，由用户提供
 * @param mem 节点在该结构体中的成员名
 */
#define FOR_EACH_LIST_ENTRY(head, p, mem) \
    for ((p) = CONTAINER_OF((head)->next, typeof(*p), mem); \
         &(p)->mem != (head); \
         (p) = CONTAINER_OF((p)->mem.next, typeof(*p), mem))

/**
 * @brief 正向遍历某结构体中的节点，可以在遍历的时候移除节点
 *
 * @param head 指向要遍历的链表头指针
 * @param p 指向某结构体的指针，由用户提供
 * @param mem 节点在该结构体中的成员名
 * @param temp 储存当前节点的下一个节点的地址
 */
#define FOR_EACH_LIST_ENTRY_SAFE(head, p, mem, temp) \
    for ((p) = CONTAINER_OF((head)->next, typeof(*p), mem), (temp) = (p)->mem.next; \
         &(p)->mem != (head); \
         (p) = CONTAINER_OF((temp), typeof(*p), mem), (temp) = (p)->mem.next)

/**
 * @brief 逆向遍历某结构体中的节点
 *
 * @param head 指向要遍历的链表头指针
 * @param p 指向某结构体的指针，由用户提供
 * @param mem 节点在该结构体中的成员名
 */
#define FOR_EACH_LIST_ENTRY_REVERSE(head, p, mem) \
    for ((p) = CONTAINER_OF((head)->prev, typeof(*p), mem); \
         &(p)->mem != (head); \
         (p) = CONTAINER_OF((p)->mem.prev, typeof(*p), mem))

/**
 * @brief 逆向遍历某结构体中的节点，可以在遍历的时候移除节点
 *
 * @param head 指向要遍历的链表头指针
 * @param p 指向某结构体的指针，由用户提供
 * @param mem 节点在该结构体中的成员名
 * @param temp 储存当前节点的上一个节点的地址
 */
#define FOR_EACH_LIST_ENTRY_REVERSE_SAFE(head, p, mem, temp) \
    for ((p) = CONTAINER_OF((head)->prev, typeof(*p), mem), (temp) = (p)->mem.prev; \
         &(p)->mem != (head); \
         (p) = CONTAINER_OF((temp), typeof(*p), mem), (temp) = (p)->mem.prev)

/**
 * @brief 初始化链表头
 *
 * @param head 指向链表头的指针
 */
static inline void list_head_init(struct list_node *head)
{
    head->prev = head;
    head->next = head;
}

/**
 * @brief 在指定的节点位置前插入一个新节点
 *
 * @param node 指向需要被插入的节点指针
 * @param new_node 指向要插入的新节点指针
 */
static inline void list_insert_before(struct list_node *node, struct list_node *new_node)
{
    new_node->next = node;
    new_node->prev = node->prev;
    node->prev->next = new_node;
    node->prev = new_node;
}

/**
 * @brief 在指定的节点位置后插入一个新节点
 *
 * @param node 指向需要被插入的节点指针
 * @param new_node 指向要插入的新节点指针
 */
static inline void list_insert_after(struct list_node *node, struct list_node *new_node)
{
    new_node->next = node->next;
    new_node->prev = node;
    node->next->prev = new_node;
    node->next = new_node;
}

/**
 * @brief 添加一个节点到指定的链表上
 *
 * @param head 指向链表头的指针
 * @param new_node 指向要添加的新节点指针
 */
static inline void list_add(struct list_node *head, struct list_node *new_node)
{
    list_insert_after(head, new_node);
}

/**
 * @brief 添加一个节点到指定的链表上，该节点将会添加在链表的尾部
 *
 * @param head 指向链表头的指针
 * @param new_node 指向要添加的新节点指针
 */
static inline void list_add_tail(struct list_node *head, struct list_node *new_node)
{
    list_insert_before(head, new_node);
}

/**
 * @brief 删除指定的节点
 *
 * @param node 指向需要删除的节点指针
 */
static inline void list_delete(struct list_node *node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->prev = node;
    node->next = node;
}

/**
 * @brief 判断链表是否为空
 *
 * @param head 指向链头的指针
 * @return true
 * @return false
 */
static inline bool list_is_empty(struct list_node *head)
{
    return head->next == head;
}

/**
 * @brief 获取链表的首个元素
 *
 * @param head 指向链头的指针
 * @return struct list_node* 链表中首个元素的地址
 */
static inline struct list_node *list_first(struct list_node *head)
{
    return head->next;
}

/**
 * @brief 获取链表的末尾元素
 *
 * @param head 指向链头的指针
 * @return struct list_node* 链表中最后元素的地址
 */
static inline struct list_node *list_last(struct list_node *head)
{
    return head->prev;
}

#ifdef __cplusplus
}
#endif

#endif