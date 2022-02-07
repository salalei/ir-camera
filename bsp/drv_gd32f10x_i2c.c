/**
 * @file drv_gd32f10x_i2c.c
 * @author salalei (1028609078@qq.com)
 * @brief
 * @version 0.1
 * @date 2022-02-01
 *
 * @copyright Copyright (c) 2022
 *
 */
#include "board.h"
#ifdef bool
#undef bool
#endif
#include "ll_i2c.h"
#include "ll_init.h"
#define LL_LOG_LEVEL LL_LOG_LEVEL_DEBUG
#include "ll_log.h"

#include "FreeRTOS.h"
#include "task.h"

#define WAIT_TIME_COUNT 400

struct gd32f10x_i2c_handle
{
    struct ll_i2c_bus parent;
    uint32_t i2c;
    uint32_t scl_gpio;
    uint32_t scl_pin;
    uint32_t sda_gpio;
    uint32_t sda_pin;
    uint32_t recv_dma;
    dma_channel_enum recv_dma_ch;
    uint32_t send_dma;
    dma_channel_enum send_dma_ch;
    TaskHandle_t thread;
};

static int i2c_wait(struct gd32f10x_i2c_handle *handle, i2c_flag_enum flag, FlagStatus status)
{
    int i = WAIT_TIME_COUNT;
    while (i2c_flag_get(handle->i2c, flag) != status)
    {
        if (i-- == 0)
        {
            switch (flag)
            {
            case I2C_FLAG_SBSEND:
                LL_ERROR("i2c send start failed");
                break;
            case I2C_FLAG_ADDSEND:
                LL_ERROR("i2c send address failed");
                break;
            case I2C_FLAG_TBE:
                LL_ERROR("wait i2c data empty failed");
                break;
            case I2C_FLAG_BTC:
                LL_ERROR("wait i2c data send finished failed");
                break;
            case I2C_FLAG_RBNE:
                LL_ERROR("wait i2c data recv finished failed");
                break;
            default:
                break;
            }
            return -ETIMEDOUT;
        }
    }
    return 0;
}

static int i2c_wait_stop(struct gd32f10x_i2c_handle *handle)
{
    int i = WAIT_TIME_COUNT;
    while (I2C_CTL0(I2C0) & 0x0200)
    {
        if (i-- == 0)
        {
            LL_ERROR("i2c stop failed");
            return -ETIMEDOUT;
        }
    }
    return 0;
}

#define I2C_WAIT(handle, flag, status) \
    do \
    { \
        res = i2c_wait(handle, flag, status); \
        if (res) \
            return res; \
    } \
    while (0)

#define I2C_WAIT_STOP(handle) \
    do \
    { \
        res = i2c_wait_stop(handle); \
        if (res) \
            return res; \
    } \
    while (0)

static int i2c_block_begin(struct gd32f10x_i2c_handle *handle, uint16_t addr, struct ll_i2c_msg *msg, size_t count)
{
    int res;

    if (!count)
    {
        i2c_disable(handle->i2c);
        i2c_enable(handle->i2c);
        i2c_ack_config(handle->i2c, I2C_ACK_ENABLE);
        I2C_WAIT(handle, I2C_FLAG_I2CBSY, RESET);
    }
    i2c_start_on_bus(handle->i2c);
    I2C_WAIT(handle, I2C_FLAG_SBSEND, SET);
    if (msg->ten_addr)
    {
        LL_ERROR("not support 10bit address");
        return -EIO;
    }
    else
    {
        if (msg->dir == __LL_I2C_DIR_SEND)
            i2c_master_addressing(handle->i2c, addr << 1, I2C_TRANSMITTER);
        else
            i2c_master_addressing(handle->i2c, addr << 1, I2C_RECEIVER);
        I2C_WAIT(handle, I2C_FLAG_ADDSEND, SET);
        i2c_flag_clear(handle->i2c, I2C_FLAG_ADDSEND);
    }
    return 0;
}

static int dma_trans(struct gd32f10x_i2c_handle *handle, uint32_t dma, uint32_t dma_ch, struct ll_i2c_msg *msg)
{
    handle->thread = xTaskGetCurrentTaskHandle();
    dma_memory_address_config(dma, dma_ch, (uint32_t)msg->buf);
    dma_transfer_number_config(dma, dma_ch, msg->size);
    dma_interrupt_flag_clear(dma, dma_ch, DMA_FLAG_G);
    dma_interrupt_enable(dma, dma_ch, DMA_INT_FTF);
    dma_channel_enable(dma, dma_ch);

    if (ulTaskNotifyTakeIndexed(1, pdTRUE, (msg->size + 80) / 40) == 1)
        return 0;
    else
    {
        dma_interrupt_disable(dma, dma_ch, DMA_INT_FTF);
        dma_channel_disable(dma, dma_ch);
        dma_interrupt_flag_clear(dma, dma_ch, DMA_FLAG_G);
        if (ulTaskNotifyTakeIndexed(1, pdTRUE, 0) == 1)
            return 0;
        else
        {
            LL_ERROR("i2c wait dma trans finished failed");
            return -ETIMEDOUT;
        }
    }
}

static int i2c_block_read(struct gd32f10x_i2c_handle *handle, struct ll_i2c_msg *msg)
{
    int res;

    if (msg->size < 2)
    {
        uint8_t *p = (uint8_t *)msg->buf;
        if (!msg->no_stop)
        {
            i2c_ack_config(handle->i2c, I2C_ACK_DISABLE);
            i2c_flag_get(handle->i2c, I2C_FLAG_ADDSEND);
            i2c_stop_on_bus(handle->i2c);
        }
        I2C_WAIT(handle, I2C_FLAG_RBNE, SET);
        *p = i2c_data_receive(handle->i2c);
    }
    else
    {
        if (!msg->no_stop)
            i2c_dma_last_transfer_config(handle->i2c, I2C_DMALST_ON);
        else
            i2c_dma_last_transfer_config(handle->i2c, I2C_DMALST_OFF);
        res = dma_trans(handle, handle->recv_dma, handle->recv_dma_ch, msg);
        if(res)
            return res;
    }
    return 0;
}

static int i2c_block_write(struct gd32f10x_i2c_handle *handle, struct ll_i2c_msg *msg)
{
    int res;
    uint8_t *p = msg->buf;

    I2C_WAIT(handle, I2C_FLAG_TBE, SET);
    if (msg->size < 2)
        i2c_data_transmit(handle->i2c, *p);
    else
    {
        res = dma_trans(handle, handle->send_dma, handle->send_dma_ch, msg);
        if (res)
            return res;
    }
    I2C_WAIT(handle, I2C_FLAG_BTC, SET);
    return 0;
}

static void i2c_bus_reset(struct gd32f10x_i2c_handle *handle)
{
    i2c_deinit(handle->i2c);
    gpio_bit_reset(handle->scl_gpio, handle->scl_pin);
    gpio_bit_reset(handle->sda_gpio, handle->sda_pin);
    gpio_init(handle->scl_gpio, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, handle->scl_pin);
    gpio_init(handle->sda_gpio, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, handle->sda_pin);
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    gpio_bit_set(handle->scl_gpio, handle->scl_pin);
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    gpio_bit_set(handle->sda_gpio, handle->sda_pin);
    gpio_init(handle->scl_gpio, GPIO_MODE_AF_OD, GPIO_OSPEED_50MHZ, handle->scl_pin);
    gpio_init(handle->sda_gpio, GPIO_MODE_AF_OD, GPIO_OSPEED_50MHZ, handle->sda_pin);
    i2c_clock_config(handle->i2c, 400000, I2C_DTCY_2);
    i2c_mode_addr_config(handle->i2c, I2C_I2CMODE_ENABLE, I2C_ADDFORMAT_7BITS, 0x00);
    i2c_enable(handle->i2c);
    i2c_ack_config(handle->i2c, I2C_ACK_ENABLE);
}

static ssize_t master_block_xfer(struct ll_i2c_dev *dev, struct ll_i2c_msg *msgs, size_t numb)
{
    int res;
    struct ll_i2c_msg *msg = &msgs[0];
    struct gd32f10x_i2c_handle *handle = (struct gd32f10x_i2c_handle *)(dev->i2c);
    size_t i;

    for (i = 0; i < numb; i++)
    {
        msg = &msgs[i];

        if (!msg->no_start)
        {
            if (i2c_block_begin(handle, dev->addr, msg, i))
            {
                LL_ERROR("i2c begin error");
                goto error;
            }
        }
        if (msg->dir == __LL_I2C_DIR_SEND)
        {
            if (i2c_block_write(handle, msg))
            {
                LL_ERROR("i2c write failed");
                goto error;
            }
        }
        else
        {
            if (i2c_block_read(handle, msg))
            {
                LL_ERROR("i2c read failed");
                goto error;
            }
        }
    }

    if (!msg->no_stop)
    {
        i2c_stop_on_bus(handle->i2c);
        I2C_WAIT_STOP(handle);
    }

    return i;

error:
    i2c_bus_reset(handle);
    return -EIO;
}

const static struct ll_i2c_ops ops = {
    .master_xfer = master_block_xfer,
};

struct gd32f10x_i2c_handle i2c0;

static void __i2c_init(struct gd32f10x_i2c_handle *handle)
{
    dma_parameter_struct dma_init_struct;

    gpio_init(handle->scl_gpio, GPIO_MODE_AF_OD, GPIO_OSPEED_50MHZ, handle->scl_pin);
    gpio_init(handle->sda_gpio, GPIO_MODE_AF_OD, GPIO_OSPEED_50MHZ, handle->sda_pin);
    i2c_clock_config(handle->i2c, 400000, I2C_DTCY_2);
    i2c_mode_addr_config(handle->i2c, I2C_I2CMODE_ENABLE, I2C_ADDFORMAT_7BITS, 0x00);
    i2c_enable(handle->i2c);
    i2c_ack_config(handle->i2c, I2C_ACK_ENABLE);
    i2c_dma_enable(handle->i2c, I2C_DMA_ON);

    dma_deinit(handle->recv_dma, handle->recv_dma_ch);
    dma_init_struct.direction = DMA_PERIPHERAL_TO_MEMORY;
    dma_init_struct.memory_addr = 0;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
    dma_init_struct.number = 0;
    dma_init_struct.periph_addr = (uint32_t)&I2C_DATA(handle->i2c);
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
    dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;
    dma_init(handle->recv_dma, handle->recv_dma_ch, &dma_init_struct);

    dma_deinit(handle->send_dma, handle->send_dma_ch);
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
    dma_init(handle->send_dma, handle->send_dma_ch, &dma_init_struct);
}

static void __dma_handler(struct gd32f10x_i2c_handle *handle, uint32_t dma, uint32_t dma_ch)
{
    if (dma_interrupt_flag_get(dma, dma_ch, DMA_INT_FLAG_FTF))
    {
        BaseType_t woken;
        dma_interrupt_disable(dma, dma_ch, DMA_INT_FTF);
        dma_channel_disable(dma, dma_ch);
        vTaskNotifyGiveIndexedFromISR(handle->thread, 1, &woken);
        portYIELD_FROM_ISR(woken);
    }
    else
        dma_interrupt_flag_clear(dma, dma_ch, DMA_INT_FLAG_G);
}

static inline void send_dma_handler(struct gd32f10x_i2c_handle *handle)
{
    __dma_handler(handle, handle->send_dma, handle->send_dma_ch);
}

static inline void recv_dma_handler(struct gd32f10x_i2c_handle *handle)
{
    __dma_handler(handle, handle->recv_dma, handle->recv_dma_ch);
}

void DMA0_Channel5_IRQHandler(void)
{
    send_dma_handler(&i2c0);
}

void DMA0_Channel6_IRQHandler(void)
{
    recv_dma_handler(&i2c0);
}

static int bsp_i2c_init(void)
{
    rcu_periph_clock_enable(RCU_AF);

    i2c0.i2c = I2C0;
    i2c0.scl_gpio = GPIOB;
    i2c0.scl_pin = GPIO_PIN_6;
    i2c0.sda_gpio = GPIOB;
    i2c0.sda_pin = GPIO_PIN_7;
    i2c0.recv_dma = DMA0;
    i2c0.recv_dma_ch = DMA_CH6;
    i2c0.send_dma = DMA0;
    i2c0.send_dma_ch = DMA_CH5;
    i2c0.parent.ops = &ops;
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_I2C0);
    rcu_periph_clock_enable(RCU_DMA0);
    __i2c_init(&i2c0);
    nvic_irq_enable(DMA0_Channel5_IRQn, 0xf - 4, 0);
    nvic_irq_enable(DMA0_Channel6_IRQn, 0xf - 4, 0);
    return __ll_i2c_bus_register(&i2c0.parent, "i2c0", NULL, __LL_DRV_MODE_READ | __LL_DRV_MODE_WRITE);
}
LL_BOARD_INITCALL(bsp_i2c_init);