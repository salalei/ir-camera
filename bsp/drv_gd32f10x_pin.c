/**
 * @file drv_gd32f10x_pin.c
 * @author salalei (1028609078@qq.com)
 * @brief gd32f10x的引脚驱动程序
 * @version 0.1
 * @date 2021-12-04
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "board.h"
#ifdef bool
#undef bool
#endif
#include "ll_init.h"
#include "ll_pin.h"

#define PORT (((struct gd32f10x_pin_handle *)(handle))->port)
#define PIN  (((struct gd32f10x_pin_handle *)(handle))->pin)

struct gd32f10x_pin_handle
{
    struct ll_pin parent;
    uint32_t port;
    uint32_t pin;
};

static void _pin_high(struct ll_pin *handle)
{
    GPIO_BOP(PORT) = PIN;
}

static void _pin_low(struct ll_pin *handle)
{
    GPIO_BC(PORT) = PIN;
}

static void _pin_toggle(struct ll_pin *handle)
{
    GPIO_OCTL(PORT) ^= PIN;
}

static enum ll_pin_state _pin_read(struct ll_pin *handle)
{
    return (GPIO_ISTAT(PORT) & (PIN)) ? LL_PIN_HIGH : LL_PIN_LOW;
}

const static struct ll_pin_ops ops = {
    .pin_high = _pin_high,
    .pin_low = _pin_low,
    .ll_pin_toggle = _pin_toggle,
    .pin_read = _pin_read,
    .set_irq_cb = NULL,
    .enable_irq = NULL,
};

/**
 * @brief 注册一个引脚
 *
 * @param handle 指向引脚句柄的指针
 * @param port 引脚的端口
 * @param pin 引脚
 * @param active 有效状态
 * @param name 引脚名字
 */
static void drv_gd32f10x_register_pin(struct gd32f10x_pin_handle *handle,
                                      uint32_t port,
                                      uint32_t pin,
                                      enum ll_pin_active active,
                                      const char *name)
{
    handle->port = port;
    handle->pin = pin;
    handle->parent.ops = &ops;
    handle->parent.active = active;
    __ll_pin_register(&handle->parent, name, NULL);
}

#define GD32F10X_REGISTER_PIN(port, pin, active, name) \
    do \
    { \
        static struct gd32f10x_pin_handle handle; \
        drv_gd32f10x_register_pin(&handle, port, pin, active, name); \
    } \
    while (0)

static int bsp_pin_init(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOC);

    gpio_init(GPIOC, GPIO_MODE_OUT_PP, GPIO_OSPEED_2MHZ, GPIO_PIN_13);
    GD32F10X_REGISTER_PIN(GPIOC, GPIO_PIN_13, LL_LOW_ACTIVE, "led");
    gpio_init(GPIOA, GPIO_MODE_OUT_PP, GPIO_OSPEED_2MHZ, GPIO_PIN_2);
    GD32F10X_REGISTER_PIN(GPIOA, GPIO_PIN_2, LL_HIGH_ACTIVE, "lcd dc");
    gpio_init(GPIOA, GPIO_MODE_OUT_PP, GPIO_OSPEED_2MHZ, GPIO_PIN_3);
    GD32F10X_REGISTER_PIN(GPIOA, GPIO_PIN_3, LL_LOW_ACTIVE, "lcd res");
    gpio_init(GPIOA, GPIO_MODE_OUT_PP, GPIO_OSPEED_2MHZ, GPIO_PIN_4);
    GD32F10X_REGISTER_PIN(GPIOA, GPIO_PIN_4, LL_LOW_ACTIVE, "lcd cs");
    return 0;
}
LL_BOARD_INITCALL(bsp_pin_init);