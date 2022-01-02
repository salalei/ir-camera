/**
 * @file FreeRTOSConfig.h
 * @author salalei (1028609078@qq.com)
 * @brief
 * @version 0.1
 * @date 2021-12-02
 *
 * @copyright Copyright (c) 2021
 *
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#define configUSE_PREEMPTION                  1
#define configCPU_CLOCK_HZ                    72000000
#define configTICK_RATE_HZ                    1000
#define configMAX_PRIORITIES                  5
#define configMINIMAL_STACK_SIZE              128
#define configMAX_TASK_NAME_LEN               8
#define configUSE_16_BIT_TICKS                0
#define configIDLE_SHOULD_YIELD               1
#define configUSE_TASK_NOTIFICATIONS          1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES 2
#define configUSE_MUTEXES                     1

#define configSUPPORT_STATIC_ALLOCATION  0
#define configSUPPORT_DYNAMIC_ALLOCATION 1
#define configTOTAL_HEAP_SIZE            (14 * 1024)

#define configUSE_IDLE_HOOK                0
#define configUSE_TICK_HOOK                0
#define configCHECK_FOR_STACK_OVERFLOW     0
#define configUSE_MALLOC_FAILED_HOOK       0
#define configUSE_DAEMON_TASK_STARTUP_HOOK 0

#define configGENERATE_RUN_TIME_STATS        0
#define configUSE_TRACE_FACILITY             0
#define configUSE_STATS_FORMATTING_FUNCTIONS 0

#define configUSE_CO_ROUTINES           0
#define configMAX_CO_ROUTINE_PRIORITIES 0

#define configUSE_TIMERS             1
#define configTIMER_TASK_PRIORITY    1
#define configTIMER_QUEUE_LENGTH     5
#define configTIMER_TASK_STACK_DEPTH configMINIMAL_STACK_SIZE

#ifdef __NVIC_PRIO_BITS
#define configPRIO_BITS __NVIC_PRIO_BITS
#else
#define configPRIO_BITS 4
#endif
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY      0xf
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5

#define configKERNEL_INTERRUPT_PRIORITY      (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

#include "ll_assert.h"
#ifdef LL_USING_ASSERT
#define configASSERT(x) LL_ASSERT(x)
#endif

#define INCLUDE_vTaskPrioritySet            0
#define INCLUDE_uxTaskPriorityGet           0
#define INCLUDE_vTaskDelete                 1
#define INCLUDE_vTaskSuspend                1
#define INCLUDE_xResumeFromISR              0
#define INCLUDE_vTaskDelayUntil             0
#define INCLUDE_vTaskDelay                  1
#define INCLUDE_xTaskGetSchedulerState      0
#define INCLUDE_xTaskGetCurrentTaskHandle   0
#define INCLUDE_uxTaskGetStackHighWaterMark 0
#define INCLUDE_xTaskGetIdleTaskHandle      0
#define INCLUDE_eTaskGetState               0
#define INCLUDE_xEventGroupSetBitFromISR    0
#define INCLUDE_xTimerPendFunctionCall      0
#define INCLUDE_xTaskAbortDelay             0
#define INCLUDE_xTaskGetHandle              0
#define INCLUDE_xTaskResumeFromISR          0
#define INCLUDE_xQueueGetMutexHolder        1

#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

#endif
