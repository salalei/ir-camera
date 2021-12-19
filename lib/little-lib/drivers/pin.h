/**
 * @file pin.h
 * @author salalei (1028609078@qq.com)
 * @brief 引脚驱动框架
 * @version 0.1
 * @date 2021-12-04
 *
 * @copyright Copyright (c) 2021
 *
 */

#include "../include/types.h"

#ifndef __PIN_H__
#define __PIN_H__

#ifdef __cplusplus
extern "C" {
#endif

struct pin;

/**
 * @brief 电平
 */
enum pin_state
{
    PIN_LOW = 0, //低电平
    PIN_HIGH     //高电平
};

/**
 * @brief 极性
 */
enum pin_active
{
    LOW_ACTIVE = 0, //低有效
    HIGH_ACTIVE     //高有效
};

/**
 * @brief 有效状态
 */
enum pin_active_state
{
    PIN_DEACTIVE = 0, //无效
    PIN_ACTIVE        //有效
};

/**
 * @brief 引脚驱动接口
 */
struct pin_ops
{
    void (*pin_high)(struct pin *pin);
    void (*pin_low)(struct pin *pin);
    void (*pin_toggle)(struct pin *pin);
    enum pin_state (*pin_read)(struct pin *pin);
    void (*set_irq_cb)(struct pin *pin, void (*cb)(void *priv), void *priv);
    void (*enable_irq)(struct pin *pin, bool enabled);
};

/**
 * @brief pin
 */
struct pin
{
    struct driver parent;
    const struct pin_ops *ops; //理论上所有的引脚应该共享一套操控接口，但不排除有些通过外部芯片扩展的引脚，因此每个句柄需要都自带操作接口
    enum pin_active active;
};

int __pin_register(struct pin *pin,
                   const char *name,
                   void *priv);

/**
 * @brief 置高指定引脚
 *
 * @param pin 指向pin的指针
 */
static inline void pin_high(struct pin *pin)
{
    ASSERT(pin);
    pin->ops->pin_high(pin);
}
/**
 * @brief 置低指定引脚
 *
 * @param pin 指向pin的指针
 */
static inline void pin_low(struct pin *pin)
{
    ASSERT(pin);
    pin->ops->pin_low(pin);
}
/**
 * @brief 翻转指定引脚
 *
 * @param pin 指向pin的指针
 */
static inline void pin_toggle(struct pin *pin)
{
    ASSERT(pin);
    pin->ops->pin_toggle(pin);
}
/**
 * @brief 将指定引脚置成指定电平
 *
 * @param pin 指向pin的指针
 * @param state 要设置的电平状态
 */
static inline void pin_write(struct pin *pin, enum pin_state state)
{
    ASSERT(pin);
    state ? (pin->ops->pin_high(pin)) : (pin->ops->pin_low(pin));
}
/**
 * @brief 读取指定引脚的电平
 *
 * @param pin 指向pin的指针
 * @return enum pin_state 当前的电平
 */
static inline enum pin_state pin_read(struct pin *pin)
{
    ASSERT(pin);
    return pin->ops->pin_read(pin);
}
/**
 * @brief 使指定引脚有效
 *
 * @param pin 指向pin的指针
 */
static inline void pin_active(struct pin *pin)
{
    ASSERT(pin);
    pin->active ? (pin->ops->pin_high(pin)) : (pin->ops->pin_low(pin));
}
/**
 * @brief 使指定引脚失效
 *
 * @param pin 指向pin的指针
 */
static inline void pin_deactive(struct pin *pin)
{
    ASSERT(pin);
    pin->active ? (pin->ops->pin_low(pin)) : (pin->ops->pin_high(pin));
}
/**
 * @brief 控制指定引脚的状态
 *
 * @param pin 指向pin的指针
 * @param state 指定的状态，有效或无效
 */
static inline void pin_write_state(struct pin *pin, enum pin_active_state state)
{
    ASSERT(pin);
    ((enum pin_active)state == pin->active) ? (pin->ops->pin_high(pin)) : (pin->ops->pin_low(pin));
}
/**
 * @brief 获取引脚的状态
 *
 * @param pin 指向pin的指针
 * @return enum sk_active_state 当前引脚的状态，有效或无效
 */
static inline enum pin_active_state pin_read_state(struct pin *pin)
{
    ASSERT(pin);
    return ((enum pin_active)pin_read(pin) == pin->active) ? PIN_ACTIVE : PIN_DEACTIVE;
}
void pin_set_irq_cb(struct pin *pin, void (*cb)(void *), void *priv);
void pin_enable_irq(struct pin *pin, bool enabled);
struct pin *pin_find_by_name(const char *name);
struct pin *pin_find_by_index(uint32_t index);

#ifdef __cplusplus
}
#endif

#endif