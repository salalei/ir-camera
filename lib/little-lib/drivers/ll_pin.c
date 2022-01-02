/**
 * @file ll_pin.c
 * @author salalei (1028609078@qq.com)
 * @brief 引脚驱动框架
 * @version 0.1
 * @date 2021-12-04
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "ll_pin.h"

/**
 * @brief 注册一个引脚(供底层驱动使用)
 *
 * @param pin 指向pin的指针
 * @param name 引脚的名字，可以为NULL，但无法使用pin_find_by_name来获取引脚
 * @param priv 指向驱动的私人数据
 * @return int 返回0
 */
int __ll_pin_register(struct ll_pin *pin,
                      const char *name,
                      void *priv)
{
    LL_ASSERT(pin && pin->ops && pin->ops->pin_high && pin->ops->pin_low && pin->ops->ll_pin_toggle && pin->ops->ll_pin_toggle);
    __ll_drv_init(&pin->parent, name, priv, 0);

    __ll_drv_register(&pin->parent);
    ll_pin_deactive(pin);
    return 0;
}

/**
 * @brief 设置指定引脚的外部中断回调
 *
 * @param pin 指向pin的指针
 * @param cb 用户需要设置的回调，NULL代表断开中断回调
 * @param priv 指向用户的私人数据
 */
void ll_pin_set_irq_cb(struct ll_pin *pin, void (*cb)(void *), void *priv)
{
    LL_ASSERT(pin);
    pin->ops->set_irq_cb(pin, cb, priv);
}

/**
 * @brief 使能指定引脚的外部中断回调
 *
 * @param pin 指向pin的指针
 * @param enabled 是否使能中断
 */
void ll_pin_enable_irq(struct ll_pin *pin, bool enabled)
{
    LL_ASSERT(pin);
    pin->ops->enable_irq(pin, enabled);
}
