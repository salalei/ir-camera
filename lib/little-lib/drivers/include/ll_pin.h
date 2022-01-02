/**
 * @file ll_pin.h
 * @author salalei (1028609078@qq.com)
 * @brief 引脚驱动框架
 * @version 0.1
 * @date 2021-12-04
 *
 * @copyright Copyright (c) 2021
 *
 */

#include "ll_drv.h"

#ifndef __LL_PIN_H__
#define __LL_PIN_H__

#ifdef __cplusplus
extern "C" {
#endif

struct ll_pin;

/**
 * @brief 电平
 */
enum ll_pin_state
{
    LL_PIN_LOW = 0, //低电平
    LL_PIN_HIGH     //高电平
};

/**
 * @brief 极性
 */
enum ll_pin_active
{
    LL_LOW_ACTIVE = 0, //低有效
    LL_HIGH_ACTIVE     //高有效
};

/**
 * @brief 有效状态
 */
enum ll_pin_active_state
{
    LL_PIN_DEACTIVE = 0, //无效
    LL_PIN_ACTIVE        //有效
};

/**
 * @brief 引脚驱动接口
 */
struct ll_pin_ops
{
    void (*pin_high)(struct ll_pin *pin);
    void (*pin_low)(struct ll_pin *pin);
    void (*ll_pin_toggle)(struct ll_pin *pin);
    enum ll_pin_state (*pin_read)(struct ll_pin *pin);
    void (*set_irq_cb)(struct ll_pin *pin, void (*cb)(void *priv), void *priv);
    void (*enable_irq)(struct ll_pin *pin, bool enabled);
};

/**
 * @brief pin
 */
struct ll_pin
{
    struct ll_drv parent;
    const struct ll_pin_ops *ops; //理论上所有的引脚应该共享一套操控接口，但不排除有些通过外部芯片扩展的引脚，因此每个句柄需要都自带操作接口
    enum ll_pin_active active;
};

int __ll_pin_register(struct ll_pin *pin,
                      const char *name,
                      void *priv);

/**
 * @brief 置高指定引脚
 *
 * @param pin 指向pin的指针
 */
static inline void ll_pin_high(struct ll_pin *pin)
{
    LL_ASSERT(pin);
    pin->ops->pin_high(pin);
}
/**
 * @brief 置低指定引脚
 *
 * @param pin 指向pin的指针
 */
static inline void ll_pin_low(struct ll_pin *pin)
{
    LL_ASSERT(pin);
    pin->ops->pin_low(pin);
}
/**
 * @brief 翻转指定引脚
 *
 * @param pin 指向pin的指针
 */
static inline void ll_pin_toggle(struct ll_pin *pin)
{
    LL_ASSERT(pin);
    pin->ops->ll_pin_toggle(pin);
}
/**
 * @brief 将指定引脚置成指定电平
 *
 * @param pin 指向pin的指针
 * @param state 要设置的电平状态
 */
static inline void ll_pin_write(struct ll_pin *pin, enum ll_pin_state state)
{
    LL_ASSERT(pin);
    state ? (pin->ops->pin_high(pin)) : (pin->ops->pin_low(pin));
}
/**
 * @brief 读取指定引脚的电平
 *
 * @param pin 指向pin的指针
 * @return enum ll_pin_state 当前的电平
 */
static inline enum ll_pin_state ll_pin_read(struct ll_pin *pin)
{
    LL_ASSERT(pin);
    return pin->ops->pin_read(pin);
}
/**
 * @brief 使指定引脚有效
 *
 * @param pin 指向pin的指针
 */
static inline void ll_pin_active(struct ll_pin *pin)
{
    LL_ASSERT(pin);
    pin->active ? (pin->ops->pin_high(pin)) : (pin->ops->pin_low(pin));
}
/**
 * @brief 使指定引脚失效
 *
 * @param pin 指向pin的指针
 */
static inline void ll_pin_deactive(struct ll_pin *pin)
{
    LL_ASSERT(pin);
    pin->active ? (pin->ops->pin_low(pin)) : (pin->ops->pin_high(pin));
}
/**
 * @brief 控制指定引脚的状态
 *
 * @param pin 指向pin的指针
 * @param state 指定的状态，有效或无效
 */
static inline void ll_pin_write_state(struct ll_pin *pin, enum ll_pin_active_state state)
{
    LL_ASSERT(pin);
    ((enum ll_pin_active)state == pin->active) ? (pin->ops->pin_high(pin)) : (pin->ops->pin_low(pin));
}
/**
 * @brief 获取引脚的状态
 *
 * @param pin 指向pin的指针
 * @return enum sk_active_state 当前引脚的状态，有效或无效
 */
static inline enum ll_pin_active_state ll_pin_read_state(struct ll_pin *pin)
{
    LL_ASSERT(pin);
    return ((enum ll_pin_active)ll_pin_read(pin) == pin->active) ? LL_PIN_ACTIVE : LL_PIN_DEACTIVE;
}
void ll_pin_set_irq_cb(struct ll_pin *pin, void (*cb)(void *), void *priv);
void ll_pin_enable_irq(struct ll_pin *pin, bool enabled);

#ifdef __cplusplus
}
#endif

#endif