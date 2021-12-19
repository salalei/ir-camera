/**
 * @file drv_uart.c
 * @author salalei (1028609078@qq.com)
 * @brief 串口驱动
 * @version 0.1
 * @date 2021-12-03
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "board.h"
#ifdef bool
#undef bool
#endif
#include "../lib/little-lib/drivers/serial.h"
#include "dbg.h"
#include "init.h"

struct gd32f10x_uart_handle
{
    struct serial parent;
    uint32_t uart;
    uint32_t send_dma;
    dma_channel_enum dma_channel;
};

static int poll_getc(struct serial *handle)
{
    struct gd32f10x_uart_handle *p = (struct gd32f10x_uart_handle *)handle;

    if (usart_flag_get(p->uart, USART_FLAG_RBNE))
        return (int)usart_data_receive(p->uart);
    else
        return -EINVAL;
}

static int poll_putc(struct serial *handle, int ch)
{
    struct gd32f10x_uart_handle *p = (struct gd32f10x_uart_handle *)handle;

    usart_data_transmit(p->uart, (uint16_t)ch);
    while (!usart_flag_get(p->uart, USART_FLAG_TBE))
    {
    }
    return 0;
}

static ssize_t irq_send(struct serial *handle, const void *buf, size_t size)
{
    struct gd32f10x_uart_handle *p = (struct gd32f10x_uart_handle *)handle;

    dma_memory_address_config(p->send_dma, p->dma_channel, (uint32_t)buf);
    dma_transfer_number_config(p->send_dma, p->dma_channel, size);
    dma_channel_enable(p->send_dma, p->dma_channel);
    return (ssize_t)size;
}

static int config(struct serial *handle, struct serial_conf *conf)
{
    struct gd32f10x_uart_handle *p = (struct gd32f10x_uart_handle *)handle;

    usart_disable(p->uart);
    usart_baudrate_set(p->uart, conf->baud);
    if (conf->data_bit == 8)
        usart_word_length_set(p->uart, USART_WL_8BIT);
    else if (conf->data_bit == 9)
        usart_word_length_set(p->uart, USART_WL_9BIT);
    else
    {
        WARN("cannot set uart data bit to %d", conf->data_bit);
        return -EINVAL;
    }

    if (conf->stop_bit == SERIAL_STOP_1BIT)
        usart_stop_bit_set(p->uart, USART_STB_1BIT);
    else if (conf->stop_bit == SERIAL_STOP_1_5BIT)
        usart_stop_bit_set(p->uart, USART_STB_1_5BIT);
    else if (conf->stop_bit == SERIAL_STOP_2BIT)
        usart_stop_bit_set(p->uart, USART_STB_2BIT);

    if (conf->parity == SERIAL_PARITY_NONE)
        usart_parity_config(p->uart, USART_PM_NONE);
    else if (conf->parity == SERIAL_PARITY_ODD)
        usart_parity_config(p->uart, USART_PM_ODD);
    else if (conf->parity == SERIAL_PARITY_EVEN)
        usart_parity_config(p->uart, USART_PM_EVEN);

    if (conf->flow_ctrl == SERIAL_FLOW_CTRL_NONE)
    {
        usart_hardware_flow_cts_config(p->uart, USART_CTS_DISABLE);
        usart_hardware_flow_rts_config(p->uart, USART_RTS_DISABLE);
    }
    else if (conf->flow_ctrl == SERIAL_FLOW_CTRL_SOFTWARE)
    {
        WARN("cannot set uart software control");
        return -EINVAL;
    }
    else if (conf->flow_ctrl == SERIAL_FLOW_CTRL_HARDWARE)
    {
        usart_hardware_flow_cts_config(p->uart, USART_CTS_ENABLE);
        usart_hardware_flow_rts_config(p->uart, USART_RTS_ENABLE);
    }
    usart_enable(p->uart);

    return 0;
}

static void stop_send(struct serial *handle)
{
    struct gd32f10x_uart_handle *p = (struct gd32f10x_uart_handle *)handle;

    dma_channel_disable(p->send_dma, p->dma_channel);
    dma_interrupt_flag_clear(p->send_dma, p->dma_channel, DMA_INT_FLAG_G);
}

static void recv_ctrl(struct serial *handle, bool enabled)
{
    struct gd32f10x_uart_handle *p = (struct gd32f10x_uart_handle *)handle;

    if (enabled)
    {
        usart_receive_config(p->uart, USART_RECEIVE_ENABLE);
        usart_interrupt_enable(p->uart, USART_INT_RBNE);
    }
    else
    {
        usart_receive_config(p->uart, USART_RECEIVE_DISABLE);
        usart_interrupt_disable(p->uart, USART_INT_RBNE);
        usart_interrupt_flag_clear(p->uart, USART_INT_FLAG_RBNE);
    }
}

const static struct serial_ops ops = {
    .poll_get_char = poll_getc,
    .poll_put_char = poll_putc,
    .irq_send = irq_send,
    .stop_send = stop_send,
    .recv_ctrl = recv_ctrl,
    .config = config,
};

static void uart_handler(struct gd32f10x_uart_handle *handle)
{
    if (usart_interrupt_flag_get(handle->uart, USART_INT_FLAG_RBNE))
    {
        usart_interrupt_flag_clear(handle->uart, USART_INT_FLAG_RBNE);
        __serial_recv_push(&handle->parent, &(uint8_t){usart_data_receive(handle->uart)}, 1);
    }
}

static void send_dma_handler(struct gd32f10x_uart_handle *handle)
{
    if (dma_interrupt_flag_get(handle->send_dma, handle->dma_channel, DMA_INT_FLAG_FTF))
    {
        dma_interrupt_flag_clear(handle->send_dma, handle->dma_channel, DMA_INT_FLAG_G);
        dma_channel_disable(handle->send_dma, handle->dma_channel);
        __serial_send_irq_handler(&handle->parent);
    }
}

static struct gd32f10x_uart_handle uart0;

static void uart0_init(void)
{
    dma_parameter_struct dma_init_struct;

    rcu_periph_clock_enable(RCU_USART0);
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_DMA0);

    gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_9);
    gpio_init(GPIOA, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_10);

    usart_deinit(USART0);
    usart_baudrate_set(USART0, 115200);
    usart_word_length_set(USART0, USART_WL_8BIT);
    usart_stop_bit_set(USART0, USART_STB_1BIT);
    usart_parity_config(USART0, USART_PM_NONE);
    usart_hardware_flow_cts_config(USART0, USART_CTS_DISABLE);
    usart_hardware_flow_rts_config(USART0, USART_RTS_DISABLE);
    usart_receive_config(USART0, USART_RECEIVE_DISABLE);
    usart_transmit_config(USART0, USART_TRANSMIT_ENABLE);
    usart_enable(USART0);

    dma_deinit(DMA0, DMA_CH3);
    dma_struct_para_init(&dma_init_struct);

    dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
    dma_init_struct.memory_addr = (uint32_t)0;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
    dma_init_struct.number = 0;
    dma_init_struct.periph_addr = (uint32_t)&USART_DATA(USART0);
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
    dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;
    dma_init(DMA0, DMA_CH3, &dma_init_struct);

    usart_dma_transmit_config(USART0, USART_DENT_ENABLE);
    dma_interrupt_enable(DMA0, DMA_CH3, DMA_INT_FTF);

    nvic_irq_enable(USART0_IRQn, 0xf - 4, 0);
    nvic_irq_enable(DMA0_Channel3_IRQn, 0xf - 4, 0);
}

void DMA0_Channel3_IRQHandler(void)
{
    send_dma_handler(&uart0);
}

void USART0_IRQHandler(void)
{
    uart_handler(&uart0);
}

static int bsp_uart_init(void)
{
    uart0_init();
    uart0.uart = USART0;
    uart0.send_dma = DMA0;
    uart0.dma_channel = DMA_CH3;
    uart0.parent.ops = &ops;
    return __serial_register(&uart0.parent, "uart0", NULL, __DRIVER_MODE_ASYNC_WRITE | __DRIVER_MODE_ASYNC_READ);
}
BOARD_INITCALL(bsp_uart_init);