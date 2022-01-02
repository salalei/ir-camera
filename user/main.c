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
#include "ll_log.h"
#include "ll_pin.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

static TimerHandle_t timer0;
static struct ll_pin *led;

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
    }

    struct ll_disp_drv *lcd = (struct ll_disp_drv *)ll_drv_find_by_name("lcd");

    LL_ASSERT(lcd);
    ll_disp_init(lcd, LL_DRV_MODE_NONBLOCK_WRITE);
    ll_disp_set_backlight(lcd, LL_DISP_BACKLIGHT_MAX);
    ll_disp_on_off(lcd, true);
    ll_disp_fill_color(lcd,
                       &(struct ll_disp_rect){0, 0, ll_disp_get_width(lcd) - 1, ll_disp_get_hight(lcd) - 1},
                       &(uint16_t){0xffff});
    while (1)
    {
        vTaskDelay(1000);
    }
}