/**
 * @file ll_disp.c
 * @author salalei (1028609078@qq.com)
 * @brief disp驱动框架
 * @version 0.1
 * @date 2021-12-16
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "ll_disp.h"
#include "ll_log.h"

#include "FreeRTOS.h"
#include "task.h"

/**
 * @brief 底层驱动在数据填充完成后调用该函数，以通知上层填充完成
 *
 * @param disp 指向显示设备的指针
 */
void __ll_disp_fill_complete(struct ll_disp_drv *disp)
{
    void (*cb)(void *);
    void *priv;

    LL_ASSERT(disp && disp->parent.drv_mode & __LL_DRV_MODE_ASYNC_WRITE);
    do
    {
        cb = disp->fill_cb;
        priv = disp->priv;
    }
    while (cb != disp->fill_cb || priv != disp->priv);
    if (cb)
        cb(priv);
}

/**
 * @brief 向显示驱动框架注册一个显示驱动
 *
 * @param disp 指向显示设备的指针
 * @param name 显示设备的名字
 * @param priv 驱动的私有数据
 * @param drv_mode 驱动模式
 * @return int 返回0
 */
int __ll_disp_register(struct ll_disp_drv *disp,
                       const char *name,
                       void *priv,
                       int drv_mode)
{
    LL_ASSERT(disp && name && disp->color < LL_DISP_COLOR_LIMIT && disp->width && disp->height);
    if (!disp->framebuf)
        LL_ASSERT(disp->ops && disp->ops->fill && disp->ops->color_fill);
    __ll_drv_init(&disp->parent, name, priv, drv_mode);
    disp->dir = LL_DISP_DIR_HORIZONTAL;
    disp->fill_cb = NULL;
    disp->priv = NULL;

    __ll_drv_register(&disp->parent);

    return 0;
}

/**
 * @brief 初始化一个显示设备
 *
 * @param disp 指向显示设备的指针
 * @param mode 初始化的模式
 * @return int 成功返回0，失败返回负数
 */
int ll_disp_init(struct ll_disp_drv *disp, int mode)
{
    int res = 0;

    LL_ASSERT(disp);
    if (mode & LL_DRV_MODE_NONBLOCK_WRITE && !(disp->parent.drv_mode & __LL_DRV_MODE_ASYNC_WRITE))
        return -EIO;
    if (disp->parent.init)
    {
        LL_DEBUG("display device has been initialized");
        return -EBUSY;
    }
    if (disp->ops->init)
    {
        res = disp->ops->init(disp);
        if (res)
        {
            LL_ERROR("failed to init display");
            return res;
        }
    }
    disp->parent.init = 1;

    return 0;
}

/**
 * @brief 复位一个显示设备
 *
 * @param disp 指向显示设备的指针
 * @return int 成功返回0，失败返回负数
 */
int ll_disp_deinit(struct ll_disp_drv *disp)
{
    LL_ASSERT(disp);
    if (!disp->parent.init)
    {
        LL_WARN("display device has not been initialized");
        return -EINVAL;
    }
    disp->parent.init = 0;
    disp->fill_cb = NULL;
    disp->priv = NULL;
    if (disp->ops->deinit)
        disp->ops->deinit(disp);

    return 0;
}

/**
 * @brief 填充指定区域的画面
 *
 * @param disp 指向显示设备的指针
 * @param rect 指向填充区域的指针
 * @param color 指向填充颜色的指针
 * @return int 成功返回0，失败返回负数
 */
int ll_disp_fill(struct ll_disp_drv *disp, const struct ll_disp_rect *rect, void *color)
{
    LL_ASSERT(disp &&
              rect &&
              rect->x1 <= rect->x2 && rect->y1 <= rect->y2 &&
              rect->x2 < disp->width && rect->y2 < disp->height &&
              color && !disp->framebuf);
    return disp->ops->fill(disp, rect, color);
}

/**
 * @brief 将指定区域填充成单色
 *
 * @param disp 指向显示设备的指针
 * @param rect 指向填充区域的指针
 * @param color 指向填充颜色的指针
 * @return int 成功返回0，失败返回负数
 */
int ll_disp_fill_color(struct ll_disp_drv *disp, const struct ll_disp_rect *rect, void *color)
{
    LL_ASSERT(disp &&
              rect &&
              rect->x1 <= rect->x2 && rect->y1 <= rect->y2 &&
              rect->x2 < disp->width && rect->y2 < disp->height &&
              color && !disp->framebuf);
    return disp->ops->color_fill(disp, rect, color);
}

/**
 * @brief 画一个点
 *
 * @param disp 指向显示设备的指针
 * @param x 点的x坐标
 * @param y 点的y坐标
 * @param color 指向填充颜色的指针
 * @return int 成功返回0，失败返回负数
 */
int ll_disp_draw_point(struct ll_disp_drv *disp, uint16_t x, uint16_t y, void *color)
{
    return ll_disp_fill_color(disp, &(struct ll_disp_rect){x, y, x, y}, color);
}

/**
 * @brief 画一条横线
 *
 * @param disp 指向显示设备的指针
 * @param x1 横线的x起始坐标
 * @param y 横线的y坐标
 * @param x2 横线的x结束坐标
 * @param color 指向填充颜色的指针
 * @return int 成功返回0，失败返回负数
 */
int ll_disp_draw_hline(struct ll_disp_drv *disp, uint16_t x1, uint16_t y, uint16_t x2, void *color)
{
    return ll_disp_fill_color(disp, &(struct ll_disp_rect){x1, y, x2, y}, color);
}

/**
 * @brief 画一条竖线
 *
 * @param disp 指向显示设备的指针
 * @param x 竖线的x坐标
 * @param y1 竖线的y起始坐标
 * @param y2 竖线的y结束坐标
 * @param color 指向填充颜色的指针
 * @return int 成功返回0，失败返回负数
 */
int ll_disp_draw_vline(struct ll_disp_drv *disp, uint16_t x, uint16_t y1, uint16_t y2, void *color)
{
    return ll_disp_fill_color(disp, &(struct ll_disp_rect){x, y1, x, y2}, color);
}

/**
 * @brief 关闭或开启显示设备
 *
 * @param disp 指向显示设备的指针
 * @param state 控制的状态，开启or关闭
 * @return int 成功返回0，失败返回负数
 */
int ll_disp_on_off(struct ll_disp_drv *disp, bool state)
{
    LL_ASSERT(disp);
    if (disp->ops->on_off)
        return disp->ops->on_off(disp, state);
    else
        return -ENOSYS;
}

/**
 * @brief 设置显示设备的显示方向
 *
 * @param disp 指向显示设备的指针
 * @param dir 设置的显示方向
 * @return int 成功返回0，失败返回负数
 */
int ll_disp_set_dir(struct ll_disp_drv *disp, enum ll_disp_dir dir)
{
    LL_ASSERT(disp && dir < LL_DISP_DIR_LIMIT);
    if (disp->ops->set_dir)
    {
        int res = disp->ops->set_dir(disp, dir);
        if (!res)
            disp->dir = dir;
        return res;
    }
    else
        return -ENOSYS;
}

/**
 * @brief 设置显示设备的背光
 *
 * @param disp 指向显示设备的指针
 * @param duty 设置的背光，不能超过99
 * @return int 成功返回0，失败返回负数
 */
int ll_disp_set_backlight(struct ll_disp_drv *disp, uint8_t duty)
{
    LL_ASSERT(disp && duty <= LL_DISP_BACKLIGHT_MAX);
    if (disp->ops->backlight)
        return disp->ops->backlight(disp, duty);
    else
        return -ENOSYS;
}

/**
 * @brief 设置显示设备填充数据完成的通知回调
 *
 * @param disp 指向显示设备的指针
 * @param cb 用户的回调函数
 * @param priv 回调函数的入参
 */
void ll_disp_set_cb(struct ll_disp_drv *disp, void (*cb)(void *), void *priv)
{
    uint32_t temp;

    LL_ASSERT(disp);
    temp = taskENTER_CRITICAL_FROM_ISR();
    disp->fill_cb = cb;
    disp->priv = priv;
    taskEXIT_CRITICAL_FROM_ISR(temp);
}
