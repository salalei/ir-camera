/**
 * @file ll_0_96_lcd.h
 * @author salalei (1028609078@qq.com)
 * @brief 0.96寸lcd驱动
 * @version 0.1
 * @date 2021-12-04
 *
 * @copyright Copyright (c) 2021
 *
 */
#ifndef __LL_0_96_LCD_H__
#define __LL_0_96_LCD_H__

#include "ll_disp.h"
#include "ll_pin.h"
#include "ll_spi.h"

struct lcd_0_96_drv
{
    struct ll_disp_drv parent;
    struct ll_spi_dev dev;
    struct ll_pin *res_pin;
    struct ll_pin *dc_pin;
};

int ll_0_96_lcd_init(struct lcd_0_96_drv *lcd_drv,
                     const char *name,
                     struct ll_pin *lcd_cs_pin,
                     struct ll_spi_bus *lcd_spi);

#endif