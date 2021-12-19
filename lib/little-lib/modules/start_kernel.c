/**
 * @file start_kernel.c
 * @author salalei (1028609078@qq.com)
 * @brief 内核启动部分
 * @version 0.1
 * @date 2021-12-18
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "FreeRTOS.h"
#include "task.h"

#include "init.h"

int do_one_initcall(int fn)
{
    return ((__initcall_t)fn)();
}

int do_initcall(int *start, int *end)
{
    for (int *fn = start; fn < end; fn++)
    {
        do_one_initcall(*fn);
    }
    return 0;
}

static inline void early_initcall(void)
{
    extern int __initcall0_start;
    extern int __initcall1_start;
    do_initcall(&__initcall0_start, &__initcall1_start);
}

static inline void board_initcall(void)
{
    extern int __initcall1_start;
    extern int __initcall2_start;
    do_initcall(&__initcall1_start, &__initcall2_start);
}

static inline void device_initcall(void)
{
    extern int __initcall2_start;
    extern int __initcall3_start;
    do_initcall(&__initcall2_start, &__initcall3_start);
}

static inline void late_initcall(void)
{
    extern int __initcall3_start;
    extern int __initcall4_start;
    do_initcall(&__initcall3_start, &__initcall4_start);
}

static inline void app_initcall(void)
{
    extern int __initcall4_start;
    extern int __initcall_end;
    do_initcall(&__initcall4_start, &__initcall_end);
}

static void first_thread(void *param)
{
    device_initcall();
    late_initcall();
    app_initcall();
    extern int main(void);
    main();
    vTaskDelete(NULL);
}

void __entry(void)
{
    taskDISABLE_INTERRUPTS();
    early_initcall();
    taskENABLE_INTERRUPTS();
    board_initcall();
    xTaskCreate(first_thread, "main", 512, NULL, 1, NULL);
    vTaskStartScheduler();
}