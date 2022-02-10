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
#define LL_LOG_LEVEL LL_LOG_LEVEL_DEBUG
#include "ll_disp.h"
#include "ll_i2c.h"
#include "ll_log.h"
#include "ll_mlx90640.h"
#include "ll_pin.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

static TimerHandle_t timer0;
static struct ll_pin *led;
static struct ll_disp_drv *lcd;
static struct ll_i2c_bus *i2c;
static struct ll_mlx90640 mlx90640;
struct ll_mlx90640_fixed_params *params;

void timer_cb(TimerHandle_t timer)
{
    ll_pin_toggle(led);
}

int main(void)
{
    led = (struct ll_pin *)ll_drv_find_by_name("led");
    if (led)
    {
        timer0 = xTimerCreate("timer0", 500, pdTRUE, NULL, timer_cb);
        xTimerStart(timer0, portMAX_DELAY);
        LL_DEBUG("start timer");
    }
    lcd = (struct ll_disp_drv *)ll_drv_find_by_name("lcd");
    if (lcd)
    {
        LL_DEBUG("lcd init OK");
        ll_disp_init(lcd, LL_DRV_MODE_NONBLOCK_WRITE);
        ll_disp_set_backlight(lcd, LL_DISP_BACKLIGHT_MAX);
        ll_disp_fill_color(lcd,
                           &(struct ll_disp_rect){0, 0, ll_disp_get_width(lcd) - 1, ll_disp_get_hight(lcd) - 1},
                           &(uint16_t){0xffff});
        ll_disp_on_off(lcd, true);
    }
    i2c = (struct ll_i2c_bus *)ll_drv_find_by_name("i2c0");
    if (i2c)
    {
        if (ll_mlx90640_init(&mlx90640, i2c, LL_MLX90640_RATE_2))
            LL_ERROR("mlx90640 init failed");
        else
        {
            struct ll_mlx90640_ee_buf *ee_buf = pvPortMalloc(sizeof(struct ll_mlx90640_ee_buf));
            params = pvPortMalloc(sizeof(struct ll_mlx90640_fixed_params));
            if (ee_buf && params)
            {
                if (!ll_mlx90640_get_params(&mlx90640, ee_buf, params))
                {
                    struct ll_mlx90640_ram_buf *ram_buf = pvPortMalloc(sizeof(struct ll_mlx90640_ram_buf));
                    ll_mlx90640_calculate_temp(&mlx90640, params, ram_buf);
                    LL_DEBUG("get params OK");
                }
            }
        }
    }
    while (1)
    {
        vTaskDelay(20);
    }
}