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
#define LOG_LEVEL LOG_LEVEL_DEBUG
#include "dbg.h"
#include "driver.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

static TimerHandle_t timer0;
static struct pin *led;

void timer_cb(TimerHandle_t timer)
{
    pin_toggle(led);
}

int main(void)
{
    led = pin_find_by_name("led");
    if (led)
    {
        timer0 = xTimerCreate("timer0", 500, pdTRUE, NULL, timer_cb);
        xTimerStart(timer0, portMAX_DELAY);
    }

    struct disp_drv *lcd = disp_find_by_name("lcd");

    ASSERT(lcd);
    disp_init(lcd, DEVICE_MODE_NONBLOCK_WRITE);
    disp_set_backlight(lcd, 99);
    disp_on_off(lcd, true);
    disp_fill_color(lcd,
                    &(struct disp_rect){0, 0, disp_get_width(lcd) - 1, disp_get_hight(lcd) - 1},
                    &(uint16_t){0XFFFF});
    while (1)
    {
        vTaskDelay(1000);
    }
}