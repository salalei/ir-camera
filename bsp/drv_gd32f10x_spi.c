/**
 * @file drv_gd32f10x_spi.c
 * @author salalei (1028609078@qq.com)
 * @brief gd32f10x的spi驱动程序
 * @version 0.1
 * @date 2021-12-13
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "board.h"
#ifdef bool
#undef bool
#endif
#include "ll_init.h"
#include "ll_log.h"
#include "ll_spi.h"

struct gd32f10x_spi_handle
{
    struct ll_spi_bus parent;
    uint32_t spi;
    uint32_t send_dma;
    dma_channel_enum send_dma_ch;
    struct ll_spi_conf conf;
    spi_parameter_struct param;
    uint32_t max_speed_hz;
};

static ssize_t master_send(struct ll_spi_dev *dev, const void *buf, size_t size)
{
    struct gd32f10x_spi_handle *handle = (struct gd32f10x_spi_handle *)dev->spi;

    dma_memory_address_config(handle->send_dma, handle->send_dma_ch, (uint32_t)buf);
    dma_transfer_number_config(handle->send_dma,
                               handle->send_dma_ch,
                               handle->conf.frame_bits == __LL_SPI_FRAME_8BIT ? size : size / 2);
    dma_channel_enable(handle->send_dma, handle->send_dma_ch);
    return size;
}

static void hard_cs_ctrl(struct ll_spi_dev *dev, bool state)
{
    struct gd32f10x_spi_handle *handle = (struct gd32f10x_spi_handle *)dev->spi;

    if (state)
        spi_enable(handle->spi);
    else
        spi_disable(handle->spi);
}

static inline int get_prescale(struct ll_spi_dev *dev)
{
    int prescale;
    int max_speed_hz = ((struct gd32f10x_spi_handle *)dev->spi)->max_speed_hz;
    uint32_t spi = ((struct gd32f10x_spi_handle *)dev->spi)->spi;

    if (spi == SPI0)
        prescale = 1;
    else
        prescale = 0;
    while (dev->conf.max_speed_hz < max_speed_hz)
    {
        if (++prescale > 7)
            return -EIO;
        max_speed_hz >>= 1;
    }
    return prescale;
}

static int config(struct ll_spi_dev *dev)
{
    bool dirty = false;
    struct gd32f10x_spi_handle *handle = (struct gd32f10x_spi_handle *)dev->spi;
    if (handle->conf.max_speed_hz != dev->conf.max_speed_hz)
    {
        int prescale = get_prescale(dev);
        if (prescale < 0)
        {
            LL_ERROR("not support spi speed hz %d", dev->conf.max_speed_hz);
            return -EIO;
        }
        handle->param.prescale = CTL0_PSC(prescale);
        handle->conf.max_speed_hz = dev->conf.max_speed_hz;
        dirty = true;
    }
    if (handle->conf.frame_bits != dev->conf.frame_bits)
    {
        if (dev->conf.frame_bits == __LL_SPI_FRAME_8BIT)
        {
            dma_memory_width_config(handle->send_dma, handle->send_dma_ch, DMA_MEMORY_WIDTH_8BIT);
            dma_periph_width_config(handle->send_dma, handle->send_dma_ch, DMA_PERIPHERAL_WIDTH_8BIT);
            handle->param.frame_size = SPI_FRAMESIZE_8BIT;
        }
        else if (dev->conf.frame_bits == __LL_SPI_FRAME_16BIT)
        {
            dma_memory_width_config(handle->send_dma, handle->send_dma_ch, DMA_MEMORY_WIDTH_16BIT);
            dma_periph_width_config(handle->send_dma, handle->send_dma_ch, DMA_PERIPHERAL_WIDTH_16BIT);
            handle->param.frame_size = SPI_FRAMESIZE_16BIT;
        }
        else
        {
            LL_ERROR("not support spi frame %d bits", dev->conf.frame_bits * 8);
            return -EIO;
        }
        handle->conf.frame_bits = dev->conf.frame_bits;
        dirty = true;
    }
    if (handle->conf.cpha != dev->conf.cpha || handle->conf.cpol != dev->conf.cpol)
    {
        if (!dev->conf.cpha && !dev->conf.cpol)
            handle->param.clock_polarity_phase = SPI_CK_PL_LOW_PH_1EDGE;
        else if (dev->conf.cpha && !dev->conf.cpol)
            handle->param.clock_polarity_phase = SPI_CK_PL_LOW_PH_2EDGE;
        else if (!dev->conf.cpha && dev->conf.cpol)
            handle->param.clock_polarity_phase = SPI_CK_PL_HIGH_PH_1EDGE;
        else
            handle->param.clock_polarity_phase = SPI_CK_PL_HIGH_PH_2EDGE;
        handle->conf.cpha = dev->conf.cpha;
        handle->conf.cpol = dev->conf.cpol;
        dirty = true;
    }
    if (handle->conf.endian != dev->conf.endian)
    {
        if (dev->conf.endian)
            handle->param.endian = SPI_ENDIAN_MSB;
        else
            handle->param.endian = SPI_ENDIAN_LSB;
        handle->conf.endian = dev->conf.endian;
        dirty = true;
    }
    if (handle->conf.cs_mode != dev->conf.cs_mode)
    {
        if (dev->conf.cs_mode == __LL_SPI_HARD_CS)
            handle->param.nss = SPI_NSS_HARD;
        else
            handle->param.nss = SPI_NSS_SOFT;
        handle->conf.cs_mode = dev->conf.cs_mode;
        dirty = true;
    }
    if (dev->conf.proto != __LL_SPI_PROTO_STD)
    {
        LL_ERROR("not support spi proto %d", dev->conf.proto);
        return -EIO;
    }
    if (handle->conf.send_addr_not_inc != dev->conf.send_addr_not_inc)
    {
        handle->conf.send_addr_not_inc = dev->conf.send_addr_not_inc;
        if (handle->conf.send_addr_not_inc)
            dma_memory_increase_disable(handle->send_dma, handle->send_dma_ch);
        else
            dma_memory_increase_enable(handle->send_dma, handle->send_dma_ch);
    }
    if (dirty)
    {
        spi_disable(handle->spi);
        spi_init(handle->spi, &handle->param);
        spi_enable(handle->spi);
    }

    return 0;
}

const static struct ll_spi_ops ops = {
    .master_send = master_send,
    .hard_cs_ctrl = hard_cs_ctrl,
    .config = config,
};

static struct gd32f10x_spi_handle spi0;

static void spi_irq_handler(struct gd32f10x_spi_handle *handle)
{
    if (dma_interrupt_flag_get(handle->send_dma, handle->send_dma_ch, DMA_INT_FLAG_FTF))
    {
        dma_interrupt_flag_clear(handle->send_dma, handle->send_dma_ch, DMA_INT_FLAG_FTF);
        dma_channel_disable(handle->send_dma, handle->send_dma_ch);
        __ll_spi_irq_handler(handle->parent.dev);
    }
}

static void spi0_init(void)
{
    dma_parameter_struct dma_init_struct;

    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_AF);
    rcu_periph_clock_enable(RCU_SPI0);
    rcu_periph_clock_enable(RCU_DMA0);

    gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_5 | GPIO_PIN_7);

    spi_i2s_deinit(SPI0);
    spi0.param.trans_mode = SPI_TRANSMODE_FULLDUPLEX;
    spi0.param.device_mode = SPI_MASTER;
    spi0.param.frame_size = SPI_FRAMESIZE_8BIT;
    spi0.param.clock_polarity_phase = SPI_CK_PL_HIGH_PH_2EDGE;
    spi0.param.nss = SPI_NSS_SOFT;
    spi0.param.prescale = SPI_PSC_4;
    spi0.param.endian = SPI_ENDIAN_MSB;
    spi_init(SPI0, &spi0.param);
    spi_dma_enable(SPI0, SPI_DMA_TRANSMIT);
    spi_enable(SPI0);

    dma_deinit(DMA0, DMA_CH2);
    dma_init_struct.periph_addr = (uint32_t)&SPI_DATA(SPI0);
    dma_init_struct.memory_addr = 0;
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
    dma_init_struct.priority = DMA_PRIORITY_LOW;
    dma_init_struct.number = 0;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init(DMA0, DMA_CH2, &dma_init_struct);
    dma_circulation_disable(DMA0, DMA_CH2);
    dma_memory_to_memory_disable(DMA0, DMA_CH2);
    dma_interrupt_enable(DMA0, DMA_CH2, DMA_INT_FTF);

    nvic_irq_enable(DMA0_Channel2_IRQn, 0xf - 4, 0);
}

void DMA0_Channel2_IRQHandler(void)
{
    spi_irq_handler(&spi0);
}

static int bsp_spi_init(void)
{
    spi0_init();
    spi0.spi = SPI0;
    spi0.send_dma = DMA0;
    spi0.send_dma_ch = DMA_CH2;
    spi0.conf.max_speed_hz = SystemCoreClock / 4;
    spi0.conf.frame_bits = __LL_SPI_FRAME_8BIT;
    spi0.conf.cpha = 1;
    spi0.conf.cpol = 1;
    spi0.conf.endian = __LL_SPI_MSB;
    spi0.conf.cs_mode = __LL_SPI_SOFT_CS;
    spi0.conf.proto = __LL_SPI_PROTO_STD;
    spi0.max_speed_hz = spi0.conf.max_speed_hz;
    spi0.parent.cs_hard_max_numb = 0;
    spi0.parent.ops = &ops;
    return __ll_spi_bus_register(&spi0.parent, "spi0", NULL, __LL_DRV_MODE_ASYNC_WRITE);
}
LL_BOARD_INITCALL(bsp_spi_init);