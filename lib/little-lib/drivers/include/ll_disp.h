/**
 * @file ll_disp.h
 * @author salalei (1028609078@qq.com)
 * @brief disp驱动框架
 * @version 0.1
 * @date 2021-12-16
 *
 * @copyright Copyright (c) 2021
 *
 */
#ifndef __LL_DISP_H__
#define __LL_DISP_H__

#include "ll_drv.h"

#define LL_DISP_BACKLIGHT_MAX 99

enum ll_disp_color
{
    LL_DISP_COLOR_1 = 0,
    LL_DISP_COLOR_8_RGB233,
    LL_DISP_COLOR_16_RGB565,
    LL_DISP_COLOR_16_BGR565,
    LL_DISP_COLOR_16_ARGB1555,
    LL_DISP_COLOR_24_RGB888,
    LL_DISP_COLOR_24_BGR888,
    LL_DISP_COLOR_32_ARGB8888,
    LL_DISP_COLOR_LIMIT
};

enum ll_disp_dir
{
    LL_DISP_DIR_HORIZONTAL = 0,
    LL_DISP_DIR_MIRROR_HORIZONTAL,
    LL_DISP_DIR_MIRROR_VERTICAL,
    LL_DISP_DIR_VERTICAL,
    LL_DISP_DIR_LIMIT
};

struct ll_disp_drv;

struct ll_disp_rect
{
    uint16_t x1;
    uint16_t y1;
    uint16_t x2;
    uint16_t y2;
};

struct ll_disp_ops
{
    int (*init)(struct ll_disp_drv *disp);
    void (*deinit)(struct ll_disp_drv *disp);
    int (*fill)(struct ll_disp_drv *disp, const struct ll_disp_rect *rect, const void *color);
    int (*color_fill)(struct ll_disp_drv *disp, const struct ll_disp_rect *rect, const void *color);
    int (*on_off)(struct ll_disp_drv *disp, bool state);
    int (*set_dir)(struct ll_disp_drv *disp, enum ll_disp_dir dir);
    int (*backlight)(struct ll_disp_drv *disp, uint8_t duty);
};

struct ll_disp_drv
{
    struct ll_drv parent;
    const struct ll_disp_ops *ops;
    void *framebuf;
    enum ll_disp_dir dir;
    enum ll_disp_color color;
    uint16_t width;
    uint16_t height;
    void (*fill_cb)(void *priv);
    void *priv;
};

/**
 * @brief 获取当前显示设备的方向
 *
 * @param disp 指向显示设备的指针
 * @return enum ll_disp_color 当前的显示方向
 */
static inline enum ll_disp_dir ll_disp_get_dir(struct ll_disp_drv *disp)
{
    LL_ASSERT(disp);
    return disp->dir;
}
/**
 * @brief 获取显示设备的颜色格式
 *
 * @param disp 指向显示设备的指针
 * @return enum ll_disp_color 颜色格式
 */
static inline enum ll_disp_color ll_disp_get_color_format(struct ll_disp_drv *disp)
{
    LL_ASSERT(disp);
    return disp->color;
}
/**
 * @brief 获取显示设备的宽度
 *
 * @param disp 指向显示设备的指针
 * @return uint16_t 宽度
 */
static inline uint16_t ll_disp_get_width(struct ll_disp_drv *disp)
{
    LL_ASSERT(disp);
    return disp->width;
}
/**
 * @brief 获取显示设备的高度
 *
 * @param disp 指向显示设备的指针
 * @return uint16_t 高度
 */
static inline uint16_t ll_disp_get_hight(struct ll_disp_drv *disp)
{
    LL_ASSERT(disp);
    return disp->height;
}

void __ll_ll_disp_fill_complete(struct ll_disp_drv *disp);
int __ll_disp_register(struct ll_disp_drv *disp,
                       const char *name,
                       void *priv,
                       int drv_mode);

int ll_disp_init(struct ll_disp_drv *disp, int mode);
int ll_disp_deinit(struct ll_disp_drv *disp);
int ll_disp_fill(struct ll_disp_drv *disp, const struct ll_disp_rect *rect, void *color);
int ll_disp_fill_color(struct ll_disp_drv *disp, const struct ll_disp_rect *rect, void *color);
int ll_disp_draw_point(struct ll_disp_drv *disp, uint16_t x, uint16_t y, void *color);
int ll_disp_draw_hline(struct ll_disp_drv *disp, uint16_t x1, uint16_t y, uint16_t x2, void *color);
int ll_disp_draw_vline(struct ll_disp_drv *disp, uint16_t x, uint16_t y1, uint16_t y2, void *color);
int ll_disp_on_off(struct ll_disp_drv *disp, bool state);
int ll_disp_set_dir(struct ll_disp_drv *disp, enum ll_disp_dir dir);
int ll_disp_set_backlight(struct ll_disp_drv *disp, uint8_t duty);
void ll_disp_set_cb(struct ll_disp_drv *disp, void (*cb)(void *), void *priv);

#endif