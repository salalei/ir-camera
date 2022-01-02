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

int ll_0_96_lcd_init(const char *name,
                     const char *lcd_res,
                     const char *lcd_dc,
                     const char *lcd_cs,
                     const char *spi);

#endif