/**
 * @file main.c
 * @author salalei (1028609078@qq.com)
 * @brief
 * @version 0.1
 * @date 2021-11-30
 *
 * @copyright Copyright (c) 2021
 *
 */

#include "main.h"

int main(void)
{
    int i;

    board_init();

    while (1)
    {
        gpio_bit_set(GPIOC, GPIO_PIN_13);
        i = 0xffffff;
        while (i--)
        {
        }
        gpio_bit_reset(GPIOC, GPIO_PIN_13);
        i = 0xffffff;
        while (i--)
        {
        }
    }
}