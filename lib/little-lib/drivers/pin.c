/**
 * @file pin.c
 * @author salalei (1028609078@qq.com)
 * @brief 引脚驱动框架
 * @version 0.1
 * @date 2021-12-04
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "pin.h"
#include "../modules/list.h"
#include "../modules/object.h"

static struct list_node head = LIST_HEAD_INIT_INS(head);

/**
 * @brief 注册一个引脚(供底层驱动使用)
 *
 * @param pin 指向pin的指针
 * @param name 引脚的名字，可以为NULL，但无法使用pin_find_by_name来获取引脚
 * @param priv 指向驱动的私人数据
 * @return int 返回0
 */
int __pin_register(struct pin *pin,
                   const char *name,
                   void *priv)
{
    ASSERT(pin && pin->ops && pin->ops->pin_high && pin->ops->pin_low && pin->ops->pin_toggle && pin->ops->pin_toggle);
    __driver_init(&pin->parent, name, priv, 0);

    list_add_tail(&head, &pin->parent.parent.node);
    pin_deactive(pin);
    return 0;
}

/**
 * @brief 设置指定引脚的外部中断回调
 *
 * @param pin 指向pin的指针
 * @param cb 用户需要设置的回调，NULL代表断开中断回调
 * @param priv 指向用户的私人数据
 */
void pin_set_irq_cb(struct pin *pin, void (*cb)(void *), void *priv)
{
    ASSERT(pin);
    pin->ops->set_irq_cb(pin, cb, priv);
}

/**
 * @brief 使能指定引脚的外部中断回调
 *
 * @param pin 指向pin的指针
 * @param enabled 是否使能中断
 */
void pin_enable_irq(struct pin *pin, bool enabled)
{
    ASSERT(pin);
    pin->ops->enable_irq(pin, enabled);
}

/**
 * @brief 通过名字来获取引脚
 *
 * @param name 想要查找的引脚名字
 * @return struct pin* 成功返回pin的指针，失败返回NULL
 */
struct pin *pin_find_by_name(const char *name)
{
    ASSERT(name);
    return (struct pin *)object_find_by_name(&head, name);
}

/**
 * @brief 通过引脚序号来获取引脚，序号是在引脚注册的时候决定，第一个注册的引脚序号为，后面依次递增
 *
 * @param index 想要查找的引脚序号
 * @return struct pin*成功返回pin的指针，失败返回NULL
 */
struct pin *pin_find_by_index(uint32_t index)
{
    return (struct pin *)object_find_by_index(&head, index);
}