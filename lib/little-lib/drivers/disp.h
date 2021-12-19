/**
 * @file disp.h
 * @author salalei (1028609078@qq.com)
 * @brief disp驱动框架
 * @version 0.1
 * @date 2021-12-16
 *
 * @copyright Copyright (c) 2021
 *
 */
#ifndef __DISP_H__
#define __DISP_H__

#include "../include/types.h"

#define DISP_BACKLIGHT_MAX 99

enum disp_color
{
    DISP_COLOR_1 = 0,
    DISP_COLOR_8_RGB233,
    DISP_COLOR_16_RGB565,
    DISP_COLOR_16_BGR565,
    DISP_COLOR_16_ARGB1555,
    DISP_COLOR_24_RGB888,
    DISP_COLOR_24_BGR888,
    DISP_COLOR_32_ARGB8888,
    DISP_COLOR_LIMIT
};

enum disp_dir
{
    DISP_DIR_HORIZONTAL = 0,
    DISP_DIR_MIRROR_HORIZONTAL,
    DISP_DIR_MIRROR_VERTICAL,
    DISP_DIR_VERTICAL,
    DISP_DIR_LIMIT
};

struct disp_drv;

struct disp_rect
{
    uint16_t x1;
    uint16_t y1;
    uint16_t x2;
    uint16_t y2;
};

struct disp_ops
{
    int (*init)(struct disp_drv *disp);
    void (*deinit)(struct disp_drv *disp);
    int (*fill)(struct disp_drv *disp, const struct disp_rect *rect, const void *color);
    int (*color_fill)(struct disp_drv *disp, const struct disp_rect *rect, const void *color);
    int (*on_off)(struct disp_drv *disp, bool state);
    int (*set_dir)(struct disp_drv *disp, enum disp_dir dir);
    int (*backlight)(struct disp_drv *disp, uint8_t duty);
};

struct disp_drv
{
    struct driver parent;
    const struct disp_ops *ops;
    void *framebuf;
    enum disp_dir dir;
    enum disp_color color;
    uint16_t width;
    uint16_t height;
    void (*fill_cb)(void *priv);
    void *priv;
};

/**
 * @brief 获取当前显示设备的方向
 *
 * @param disp 指向显示设备的指针
 * @return enum disp_color 当前的显示方向
 */
static inline enum disp_color disp_get_dir(struct disp_drv *disp)
{
    ASSERT(disp);
    return disp->dir;
}
/**
 * @brief 获取显示设备的颜色格式
 *
 * @param disp 指向显示设备的指针
 * @return enum disp_color 颜色格式
 */
static inline enum disp_color disp_get_color_format(struct disp_drv *disp)
{
    ASSERT(disp);
    return disp->color;
}
/**
 * @brief 获取显示设备的宽度
 *
 * @param disp 指向显示设备的指针
 * @return uint16_t 宽度
 */
static inline uint16_t disp_get_width(struct disp_drv *disp)
{
    ASSERT(disp);
    return disp->width;
}
/**
 * @brief 获取显示设备的高度
 *
 * @param disp 指向显示设备的指针
 * @return uint16_t 高度
 */
static inline uint16_t disp_get_hight(struct disp_drv *disp)
{
    ASSERT(disp);
    return disp->height;
}

void __disp_fill_complete(struct disp_drv *disp);
int __register_disp(struct disp_drv *disp,
                    const char *name,
                    void *priv,
                    int drv_mode);

int disp_init(struct disp_drv *disp, int mode);
int disp_deinit(struct disp_drv *disp);
int disp_fill(struct disp_drv *disp, const struct disp_rect *rect, void *color);
int disp_fill_color(struct disp_drv *disp, const struct disp_rect *rect, void *color);
int disp_draw_point(struct disp_drv *disp, uint16_t x, uint16_t y, void *color);
int disp_draw_hline(struct disp_drv *disp, uint16_t x1, uint16_t y, uint16_t x2, void *color);
int disp_draw_vline(struct disp_drv *disp, uint16_t x, uint16_t y1, uint16_t y2, void *color);
int disp_on_off(struct disp_drv *disp, bool state);
int disp_set_dir(struct disp_drv *disp, enum disp_dir dir);
int disp_set_backlight(struct disp_drv *disp, uint8_t duty);
void disp_set_cb(struct disp_drv *disp, void (*cb)(void *), void *priv);
struct disp_drv *disp_find_by_name(const char *name);

#endif