/**
 * @file drv_0_96_lcd.h
 * @author salalei (1028609078@qq.com)
 * @brief 0.96寸lcd驱动
 * @version 0.1
 * @date 2021-12-04
 *
 * @copyright Copyright (c) 2021
 *
 */
#ifndef __DRV_0_96_LCD_H__
#define __DRV_0_96_LCD_H__

#include "../drivers/disp.h"
#include "../drivers/spi.h"

int drv_0_96_lcd_init(const char *name,
                      const char *lcd_res,
                      const char *lcd_dc,
                      const char *lcd_cs,
                      const char *spi);

#endif