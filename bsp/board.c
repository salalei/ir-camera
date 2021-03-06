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
#include "ll_0_96_lcd.h"
#include "ll_init.h"

static int board_init(void)
{
    SystemInit();
    nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);
    return 0;
}
LL_EARLY_INITCALL(board_init);

static int lcd_init(void)
{
    static struct lcd_0_96_drv lcd;
    struct ll_pin *lcd_cs = (struct ll_pin *)ll_drv_find_by_name("lcd cs");
    struct ll_spi_bus *spi = (struct ll_spi_bus *)ll_drv_find_by_name("spi0");

    lcd.dc_pin = (struct ll_pin *)ll_drv_find_by_name("lcd dc");
    lcd.res_pin = (struct ll_pin *)ll_drv_find_by_name("lcd res");
    return ll_0_96_lcd_init(&lcd, "lcd", lcd_cs, spi);
}
LL_LATE_INITCALL(lcd_init);