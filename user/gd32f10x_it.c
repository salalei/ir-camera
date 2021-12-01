/**
 * @file gd32f10x_it.c
 * @author salalei (1028609078@qq.com)
 * @brief 中断文件
 * @version 0.1
 * @date 2021-11-29
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "gd32f10x_it.h"

void NMI_Handler(void)
{
}

void HardFault_Handler(void)
{
    while(1){
    }
}

void MemManage_Handler(void)
{
    while(1){
    }
}

void BusFault_Handler(void)
{
    while(1){
    }
}

void UsageFault_Handler(void)
{
    while(1){
    }
}

void SVC_Handler(void)
{
}

void DebugMon_Handler(void)
{
}

void PendSV_Handler(void)
{
}

void SysTick_Handler(void)
{
}
