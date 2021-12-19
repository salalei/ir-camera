/**
 * @file startup.S
 * @author salalei (1028609078@qq.com)
 * @brief 启动文件
 * @version 0.1
 * @date 2021-11-30
 * 
 * @copyright Copyright (c) 2021
 * 
 */
.syntax unified
.arch armv7-m

.section .stack
.align 3
.equ Stack_Size, __STACK_SIZE
.global __StackTop
.global __StackLimit
__StackLimit:
    .space Stack_Size
    .size __StackLimit, . - __StackLimit
__StackTop:
    .size __StackTop, . - __StackTop

.section .heap
.align 3
.equ Heap_Size, __HEAP_SIZE
.global __HeapBase
.global __HeapLimit
__HeapBase:
    .if Heap_Size
    .space Heap_Size
    .endif
    .size __HeapBase, . - __HeapBase
__HeapLimit:
    .size __HeapLimit, . - __HeapLimit

.section .isr_vector
.align 2
.global __isr_vector
__isr_vector:
    .long	__StackTop            /* Top of Stack */
	.long	Reset_Handler         /* Reset Handler */
	.long	NMI_Handler           /* NMI Handler */
	.long	HardFault_Handler     /* Hard Fault Handler */
	.long	MemManage_Handler     /* MPU Fault Handler */
	.long	BusFault_Handler      /* Bus Fault Handler */
	.long	UsageFault_Handler    /* Usage Fault Handler */
	.long	0                     /* Reserved */
	.long	0                     /* Reserved */
	.long	0                     /* Reserved */
	.long	0                     /* Reserved */
	.long	SVC_Handler           /* SVCall Handler */
	.long	DebugMon_Handler      /* Debug Monitor Handler */
	.long	0                     /* Reserved */
	.long	PendSV_Handler        /* PendSV Handler */
	.long	SysTick_Handler       /* SysTick Handler */

    /* External interrupts */
    .long   WWDGT_IRQHandler
    .long   LVD_IRQHandler
    .long   TAMPER_IRQHandler
    .long   RTC_IRQHandler
    .long   FMC_IRQHandler
    .long   RCU_IRQHandler
    .long   EXTI0_IRQHandler
    .long   EXTI1_IRQHandler
    .long   EXTI2_IRQHandler
    .long   EXTI3_IRQHandler
    .long   EXTI4_IRQHandler
    .long   DMA0_Channel0_IRQHandler
    .long   DMA0_Channel1_IRQHandler
    .long   DMA0_Channel2_IRQHandler
    .long   DMA0_Channel3_IRQHandler
    .long   DMA0_Channel4_IRQHandler
    .long   DMA0_Channel5_IRQHandler
    .long   DMA0_Channel6_IRQHandler
    .long   ADC0_1_IRQHandler
    .long   CAN0_TX_IRQHandler
    .long   CAN0_RX0_IRQHandler
    .long   CAN0_RX1_IRQHandler
    .long   CAN0_EWMC_IRQHandler
    .long   EXTI5_9_IRQHandler
    .long   TIMER0_BRK_IRQHandler
    .long   TIMER0_UP_IRQHandler
    .long   TIMER0_TRG_CMT_IRQHandler
    .long   TIMER0_Channel_IRQHandler
    .long   TIMER1_IRQHandler
    .long   TIMER2_IRQHandler
    .long   TIMER3_IRQHandler
    .long   I2C0_EV_IRQHandler
    .long   I2C0_ER_IRQHandler
    .long   I2C1_EV_IRQHandler
    .long   I2C1_ER_IRQHandler
    .long   SPI0_IRQHandler
    .long   SPI1_IRQHandler
    .long   USART0_IRQHandler
    .long   USART1_IRQHandler
    .long   USART2_IRQHandler
    .long   EXTI10_15_IRQHandler
    .long   RTC_Alarm_IRQHandler
    .long   USBFS_WKUP_IRQHandler
    .long   TIMER7_BRK_IRQHandler
    .long   TIMER7_UP_IRQHandler
    .long   TIMER7_TRG_CMT_IRQHandler
    .long   TIMER7_Channel_IRQHandler
    .long   0
    .long   EXMC_IRQHandler
    .long   0
    .long   TIMER4_IRQHandler
    .long   SPI2_IRQHandler
    .long   UART3_IRQHandler
    .long   UART4_IRQHandler
    .long   TIMER5_IRQHandler
    .long   TIMER6_IRQHandler
    .long   DMA1_Channel0_IRQHandler
    .long   DMA1_Channel1_IRQHandler
    .long   DMA1_Channel2_IRQHandler
    .long   DMA1_Channel3_IRQHandler
    .long   DMA1_Channel4_IRQHandler
    .long   ENET_IRQHandler
    .long   ENET_WKUP_IRQHandler
    .long   CAN1_TX_IRQHandler
    .long   CAN1_RX0_IRQHandler
    .long   CAN1_RX1_IRQHandler
    .long   CAN1_EWMC_IRQHandler
    .long   USBFS_IRQHandler

    .size __isr_vector, . - __isr_vector

.text
.thumb
.thumb_func
.align 2
.global Reset_Handler
.type Reset_Handler, %function
Reset_Handler:
    ldr r1, = __etext
    ldr r2, = __data_start__
    ldr r3, = __data_end__

.L_loop1:
    cmp r2, r3
    ittt lt
    ldrlt r0, [r1], #4
    strlt r0, [r2], #4
    blt .L_loop1

    ldr r1, = __bss_start__
    ldr r2, = __bss_end__
    movs r0, 0
.L_loop2:
    cmp r1, r2
    itt lt
    strlt r0, [r1], #4
    blt .L_loop2

    bl __entry

.pool
.size Reset_Handler, . - Reset_Handler

.align 1
.thumb_func
.weak Default_Handler
.type Default_Handler, %function
Default_Handler:
    b .
    .size Default_Handler, . - Default_Handler

.macro def_irq_handler handler_name
.weak \handler_name
.set \handler_name, Default_Handler
.endm

def_irq_handler NMI_Handler
def_irq_handler HardFault_Handler
def_irq_handler MemManage_Handler
def_irq_handler BusFault_Handler
def_irq_handler UsageFault_Handler
def_irq_handler SVC_Handler
def_irq_handler DebugMon_Handler
def_irq_handler PendSV_Handler
def_irq_handler SysTick_Handler
def_irq_handler WWDGT_IRQHandler
def_irq_handler LVD_IRQHandler
def_irq_handler TAMPER_IRQHandler
def_irq_handler RTC_IRQHandler
def_irq_handler FMC_IRQHandler
def_irq_handler RCU_IRQHandler
def_irq_handler EXTI0_IRQHandler
def_irq_handler EXTI1_IRQHandler
def_irq_handler EXTI2_IRQHandler
def_irq_handler EXTI3_IRQHandler
def_irq_handler EXTI4_IRQHandler
def_irq_handler DMA0_Channel0_IRQHandler
def_irq_handler DMA0_Channel1_IRQHandler
def_irq_handler DMA0_Channel2_IRQHandler
def_irq_handler DMA0_Channel3_IRQHandler
def_irq_handler DMA0_Channel4_IRQHandler
def_irq_handler DMA0_Channel5_IRQHandler
def_irq_handler DMA0_Channel6_IRQHandler
def_irq_handler ADC0_1_IRQHandler
def_irq_handler CAN0_TX_IRQHandler
def_irq_handler CAN0_RX0_IRQHandler
def_irq_handler CAN0_RX1_IRQHandler
def_irq_handler CAN0_EWMC_IRQHandler
def_irq_handler EXTI5_9_IRQHandler
def_irq_handler TIMER0_BRK_IRQHandler
def_irq_handler TIMER0_UP_IRQHandler
def_irq_handler TIMER0_TRG_CMT_IRQHandler
def_irq_handler TIMER0_Channel_IRQHandler
def_irq_handler TIMER1_IRQHandler
def_irq_handler TIMER2_IRQHandler
def_irq_handler TIMER3_IRQHandler
def_irq_handler I2C0_EV_IRQHandler
def_irq_handler I2C0_ER_IRQHandler
def_irq_handler I2C1_EV_IRQHandler
def_irq_handler I2C1_ER_IRQHandler
def_irq_handler SPI0_IRQHandler
def_irq_handler SPI1_IRQHandler
def_irq_handler USART0_IRQHandler
def_irq_handler USART1_IRQHandler
def_irq_handler USART2_IRQHandler
def_irq_handler EXTI10_15_IRQHandler
def_irq_handler RTC_Alarm_IRQHandler
def_irq_handler USBFS_WKUP_IRQHandler
def_irq_handler TIMER7_BRK_IRQHandler
def_irq_handler TIMER7_UP_IRQHandler
def_irq_handler TIMER7_TRG_CMT_IRQHandler
def_irq_handler TIMER7_Channel_IRQHandler
def_irq_handler EXMC_IRQHandler
def_irq_handler TIMER4_IRQHandler
def_irq_handler SPI2_IRQHandler
def_irq_handler UART3_IRQHandler
def_irq_handler UART4_IRQHandler
def_irq_handler TIMER5_IRQHandler
def_irq_handler TIMER6_IRQHandler
def_irq_handler DMA1_Channel0_IRQHandler
def_irq_handler DMA1_Channel1_IRQHandler
def_irq_handler DMA1_Channel2_IRQHandler
def_irq_handler DMA1_Channel3_IRQHandler
def_irq_handler DMA1_Channel4_IRQHandler
def_irq_handler ENET_IRQHandler
def_irq_handler ENET_WKUP_IRQHandler
def_irq_handler CAN1_TX_IRQHandler
def_irq_handler CAN1_RX0_IRQHandler
def_irq_handler CAN1_RX1_IRQHandler
def_irq_handler CAN1_EWMC_IRQHandler
def_irq_handler USBFS_IRQHandler