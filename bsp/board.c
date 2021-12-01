/**
 * @file board.c
 * @author salalei (1028609078@qq.com)
 * @brief 
 * @version 0.1
 * @date 2021-12-02
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "board.h"

int board_init(void)
{
    rcu_periph_clock_enable(RCU_GPIOC);
    gpio_bit_set(GPIOC, GPIO_PIN_13);
    gpio_init(GPIOC, GPIO_MODE_OUT_PP, GPIO_OSPEED_2MHZ, GPIO_PIN_13);

    return 0;
}