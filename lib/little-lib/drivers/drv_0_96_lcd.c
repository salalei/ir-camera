/**
 * @file drv_0_96_lcd.c
 * @author salalei (1028609078@qq.com)
 * @brief 0.96寸lcd驱动
 * @version 0.1
 * @date 2021-12-04
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "drv_0_96_lcd.h"
#include "pin.h"

#include "FreeRTOS.h"
#include "task.h"

#define LCD_Y_OFFSET 32
#define LCD_WIDTH    128
#define LCD_HEIGHT   64

struct lcd_0_96_drv
{
    struct disp_drv parent;
    struct spi_dev dev;
    struct pin *res_pin;
    struct pin *dc_pin;
};

static int lcd_write_cmd(struct lcd_0_96_drv *lcd, uint8_t cmd, const uint8_t *buf, size_t size)
{
    int res;
    struct spi_trans t = {
        .buf = &cmd,
        .size = 1,
        .dir = __SPI_DIR_SEND,
    };
    struct spi_msg msg = SPI_MSG_INIT(&t, 1, NULL);

    pin_low(lcd->dc_pin);
    res = spi_sync(&lcd->dev, &msg);
    pin_high(lcd->dc_pin);
    if (res || !buf)
        return res;
    t.buf = (uint8_t *)buf;
    t.size = size;
    return spi_sync(&lcd->dev, &msg);
}

static int lcd_write_color(struct lcd_0_96_drv *lcd, const struct disp_rect *rect, const void *color)
{
    struct spi_trans t = {
        .buf = (uint8_t *)color,
        .size = (rect->x2 - rect->x1 + 1) * (rect->y2 - rect->y1 + 1) * 2,
        .dir = __SPI_DIR_SEND,
    };
    struct spi_msg msg = SPI_MSG_INIT(&t, 1, NULL);

    return spi_sync(&lcd->dev, &msg);
}

int init(struct disp_drv *disp)
{
    uint8_t buf[3];
    struct lcd_0_96_drv *lcd = (struct lcd_0_96_drv *)disp;

    pin_high(lcd->res_pin);
    vTaskDelay(40); //>20ms
    pin_low(lcd->res_pin);
    vTaskDelay(1); //>3us
    pin_high(lcd->res_pin);
    vTaskDelay(300); //>300ms

    if (lcd_write_cmd(lcd, 0xfd, &(uint8_t){0x12}, 1))
        return -EIO;
    if (lcd_write_cmd(lcd, 0xae, NULL, 0))
        return -EIO;
    buf[0] = 0x71;
    buf[1] = 0x00;
    if (lcd_write_cmd(lcd, 0xa0, buf, 2))
        return -EIO;
    if (lcd_write_cmd(lcd, 0xa1, &(uint8_t){0x00}, 1))
        return -EIO;
    if (lcd_write_cmd(lcd, 0xa2, &(uint8_t){0x00}, 1))
        return -EIO;
    if (lcd_write_cmd(lcd, 0xb1, &(uint8_t){0x84}, 1))
        return -EIO;
    if (lcd_write_cmd(lcd, 0xb3, &(uint8_t){0x20}, 1))
        return -EIO;
    if (lcd_write_cmd(lcd, 0xb6, &(uint8_t){0x01}, 1))
        return -EIO;
    if (lcd_write_cmd(lcd, 0xbb, &(uint8_t){0x00}, 1))
        return -EIO;
    if (lcd_write_cmd(lcd, 0xbe, &(uint8_t){0x07}, 1))
        return -EIO;
    if (lcd_write_cmd(lcd, 0xc7, &(uint8_t){0x0f}, 1))
        return -EIO;
    buf[0] = 0x32;
    buf[1] = 0x29;
    buf[2] = 0x53;
    if (lcd_write_cmd(lcd, 0xc1, buf, 3))
        return -EIO;
    if (lcd_write_cmd(lcd, 0xca, &(uint8_t){0x7f}, 1))
        return -EIO;

    return 0;
}

static int lcd_set_addr(struct lcd_0_96_drv *lcd, const struct disp_rect *rect)
{
    uint8_t buf[2];

    if (lcd->parent.dir <= DISP_DIR_VERTICAL)
    {
        buf[0] = rect->y1 + LCD_Y_OFFSET;
        buf[1] = rect->y2 + LCD_Y_OFFSET;
    }
    else
    {
        buf[0] = rect->x1 + LCD_Y_OFFSET;
        buf[1] = rect->x2 + LCD_Y_OFFSET;
    }
    if (lcd_write_cmd(lcd, 0x15, buf, 2))
        return -EIO;

    if (lcd->parent.dir <= DISP_DIR_VERTICAL)
    {
        buf[0] = rect->x1;
        buf[1] = rect->x2;
    }
    else
    {
        buf[0] = rect->y1;
        buf[1] = rect->y2;
    }
    if (lcd_write_cmd(lcd, 0x75, buf, 2))
        return -EIO;

    return 0;
}

static int fill(struct disp_drv *disp, const struct disp_rect *rect, const void *color)
{
    struct lcd_0_96_drv *lcd = (struct lcd_0_96_drv *)disp;
    size_t size = (rect->x2 - rect->x1 + 1) * (rect->y2 - rect->y1 + 1) * 2;

    lcd_set_addr(lcd, rect);
    lcd_write_cmd(lcd, 0x5c, (const uint8_t *)color, size);
    return 0;
}

static int color_fill(struct disp_drv *disp, const struct disp_rect *rect, const void *color)
{
    int res;
    struct lcd_0_96_drv *lcd = (struct lcd_0_96_drv *)disp;
    struct spi_conf conf = lcd->dev.conf;

    if (lcd_set_addr(lcd, rect))
        return -EIO;
    if (lcd_write_cmd(lcd, 0x5c, NULL, 0))
        return -EIO;
    conf.send_addr_not_inc = 1;
    conf.frame_bits = __SPI_FRAME_16BIT;
    res = spi_config(&lcd->dev, &conf);
    if (!res)
        res = lcd_write_color(lcd, rect, color);
    conf.send_addr_not_inc = 0;
    conf.frame_bits = __SPI_FRAME_8BIT;
    if (!res)
        res = spi_config(&lcd->dev, &conf);
    else
        spi_config(&lcd->dev, &conf);

    return res;
}

int on_off(struct disp_drv *disp, bool state)
{
    struct lcd_0_96_drv *lcd = (struct lcd_0_96_drv *)disp;

    if (state)
        return lcd_write_cmd(lcd, 0xaf, NULL, 0);
    else
        return lcd_write_cmd(lcd, 0xae, NULL, 0);
}

int set_dir(struct disp_drv *disp, enum disp_dir dir)
{
    struct lcd_0_96_drv *lcd = (struct lcd_0_96_drv *)disp;
    static const uint8_t value[4] = {0x71, 0x63, 0x60, 0x72};
    uint8_t buf[2] = {value[dir], 0};

    return lcd_write_cmd(lcd, 0xa0, buf, 2);
}

int backlight(struct disp_drv *disp, uint8_t duty)
{
    struct lcd_0_96_drv *lcd = (struct lcd_0_96_drv *)disp;

    return lcd_write_cmd(lcd, 0xc7, &(uint8_t){duty * 15 / DISP_BACKLIGHT_MAX}, 1);
}

const static struct disp_ops ops = {
    .init = init,
    .fill = fill,
    .color_fill = color_fill,
    .on_off = on_off,
    .set_dir = set_dir,
    .backlight = backlight,
};

int drv_0_96_lcd_init(const char *name,
                      const char *lcd_res,
                      const char *lcd_dc,
                      const char *lcd_cs,
                      const char *spi)
{
    int res;

    struct lcd_0_96_drv *lcd_drv = pvPortMalloc(sizeof(struct lcd_0_96_drv));

    if (!lcd_drv)
    {
        res = -ENOSYS;
        goto error;
    }
    lcd_drv->res_pin = pin_find_by_name(lcd_res);
    if (!lcd_drv->res_pin)
    {
        res = -ENOSYS;
        goto error;
    }
    lcd_drv->dc_pin = pin_find_by_name(lcd_dc);
    if (!lcd_drv->dc_pin)
    {
        res = -ENOSYS;
        goto error;
    }
    lcd_drv->dev.cs_pin = pin_find_by_name(lcd_cs);
    if (!lcd_drv->dev.cs_pin)
    {
        res = -ENOSYS;
        goto error;
    }
    lcd_drv->dev.conf.cpha = 1;
    lcd_drv->dev.conf.cpol = 1;
    lcd_drv->dev.conf.cs_mode = __SPI_SOFT_CS;
    lcd_drv->dev.conf.endian = __SPI_MSB;
    lcd_drv->dev.conf.frame_bits = __SPI_FRAME_8BIT;
    lcd_drv->dev.conf.max_speed_hz = 10000000;
    lcd_drv->dev.conf.proto = __SPI_PROTO_STD;
    lcd_drv->dev.conf.send_addr_not_inc = 0;
    res = spi_dev_register(&lcd_drv->dev, name, spi, NULL, __DRIVER_MODE_WRITE);
    if (res)
        goto error;
    lcd_drv->parent.framebuf = NULL;
    lcd_drv->parent.ops = &ops;
    lcd_drv->parent.dir = DISP_DIR_HORIZONTAL;
    lcd_drv->parent.color = DISP_COLOR_16_RGB565;
    lcd_drv->parent.width = LCD_WIDTH;
    lcd_drv->parent.height = LCD_HEIGHT;
    return __register_disp(&lcd_drv->parent, name, NULL, __DRIVER_MODE_ASYNC_WRITE);

error:
    vPortFree(lcd_drv);
    return -ENOSYS;
}