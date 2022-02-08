/**
 * @file ll_mlx90640.c
 * @author salalei (1028609078@qq.com)
 * @brief
 * @version 0.1
 * @date 2022-02-07
 *
 * @copyright Copyright (c) 2022
 *
 */
#include "ll_mlx90640.h"
#include "ll_assert.h"
#include "ll_log.h"

#include "FreeRTOS.h"
#include "task.h"

#define DEVICE_ADDRESS 0x33

#define STATUS_REG                 0x8000
#define EN_OW_BIT                  (1 << 4) // 1 使能，0 失能
#define DATA_READY_IN_RAM_BIT      (1 << 3) // 1 ready
#define LAST_MEASURED_SUBPAGE_POS  0
#define LAST_MEASURED_SUBPAGE_MASK (0x7 << LAST_MEASURED_SUBPAGE_POS) // 1 子界面1，0 子界面0

#define CTRL_REG              0x800d
#define READING_PATTERN_BIT   (1 << 12) // 1 棋盘模式，0为隔行模式
#define RESOLUTION_CTRL_POS   10
#define RESOLUTION_CTRL_MASK  (0x3 << RESOLUTION_CTRL_POS) // 0 16bit，1 17bit，2 18bit，3 19bit
#define RATE_CTRL_POS         7
#define RATE_CTRL_MASK        (0x7 << RATE_CTRL_POS)
#define SELECT_SUBPAGE_POS    4
#define SELECT_SUBPAGE_MASK   (0x7 << SELECT_SUBPAGE_POS)
#define EN_SUBPAGE_REPEAT_BIT (1 << 3)
#define EN_DATA_HOLD_BIT      (1 << 2)
#define EN_SUBPAGE_MODE_BIT   (1 << 0)

#define I2C_CONFIG_REG                0x800f
#define SDA_DRV_CUR_LIMIT_CTRL_BIT    (1 << 2)
#define I2C_THRESHOLD_HOLD_LEVELS_BIT (1 << 1)
#define FM_DIS_BIT                    (1 << 0)

#define RAM_ADDR 0x0400

static int read_16bits(struct ll_mlx90640 *handle, uint16_t regaddr, uint16_t *buf, size_t size)
{
    struct ll_i2c_msg msgs[2] = {
        {
            .buf = (uint8_t *)&regaddr,
            .size = 2,
            .dir = __LL_I2C_DIR_SEND,
        },
        {
            .buf = (uint8_t *)buf,
            .size = 2 * size,
            .dir = __LL_I2C_DIR_RECV,
        },
    };
    int i;
    uint16_t data;

    regaddr = (regaddr >> 8) | (regaddr << 8);
    if (ll_i2c_trans(&handle->dev, msgs, 2) != 2)
        return -EIO;

    for (i = 0; i < size; i++)
    {
        data = *buf;
        *buf++ = (data >> 8) | (data << 8);
    }

    return 0;
}

#define READ_16BITS(handle, regaddr, regdata, size) \
    do \
    { \
        res = read_16bits((handle), (regaddr), (regdata), (size)); \
        if (res) \
            return res; \
    } \
    while (0);

static int write_16bit(struct ll_mlx90640 *handle, uint16_t regaddr, uint16_t regdata)
{
    uint16_t data[2];
    struct ll_i2c_msg msgs[1] = {
        {
            .buf = (uint8_t *)&data[0],
            .size = 4,
            .dir = __LL_I2C_DIR_SEND,
        },
    };
    data[0] = (regaddr >> 8) | (regaddr << 8);
    data[1] = (regdata >> 8) | (regdata << 8);
    if (ll_i2c_trans(&handle->dev, msgs, 1) != 1)
        return -EIO;
    return 0;
}

#define WRITE_16BIT(handle, regaddr, regdata) \
    do \
    { \
        res = write_16bit((handle), (regaddr), (regdata)); \
        if (res) \
            return res; \
    } \
    while (0);

int ll_mlx90640_init(struct ll_mlx90640 *handle,
                     struct ll_i2c_bus *i2c_bus,
                     enum ll_mlx90640_rate rate)
{
    int res;
    uint16_t regdata;
    TickType_t pov_tick = 0;

    LL_ASSERT(handle && i2c_bus && rate < LL_MLX90640_RATE_LIMIT);
    handle->dev.addr = DEVICE_ADDRESS;
    handle->dev.i2c = i2c_bus;
    res = ll_i2c_dev_register(&handle->dev, "mlx90640", NULL, __LL_DRV_MODE_READ | __LL_DRV_MODE_WRITE);
    if (res)
        return res;

    //上电40ms开始操作
    vTaskDelayUntil(&pov_tick, 40);
    regdata = READING_PATTERN_BIT | (0x2 << RESOLUTION_CTRL_POS) | (rate << RATE_CTRL_POS) | EN_SUBPAGE_MODE_BIT;
    WRITE_16BIT(handle, CTRL_REG, regdata);
    WRITE_16BIT(handle, STATUS_REG, 0);

    return 0;
}

int ll_mlx90640_config(struct ll_mlx90640 *handle, enum ll_mlx90640_rate rate)
{
    int res;
    uint16_t regdata = READING_PATTERN_BIT | (0x2 << RESOLUTION_CTRL_POS) | (rate << RATE_CTRL_POS) | EN_SUBPAGE_MODE_BIT;
    WRITE_16BIT(handle, CTRL_REG, regdata);
    return 0;
}

/**
 * @brief 获取原始数据
 *
 * @param handle 指向ll_mlx90640
 * @param buf 用户提供的缓存区，必须>=768
 * @return int
 */
int ll_mlx90640_read_raw_data(struct ll_mlx90640 *handle, struct ll_mlx90640_raw_data *data)
{
    int res;
    uint16_t regdata;

    LL_ASSERT(handle && data);
    READ_16BITS(handle, STATUS_REG, &regdata, 1);
    if (!(regdata & DATA_READY_IN_RAM_BIT))
        return -EAGAIN;

    WRITE_16BIT(handle, STATUS_REG, 0);
    READ_16BITS(handle, RAM_ADDR, (uint16_t *)data, sizeof(struct ll_mlx90640_raw_data));

    return 0;
}

static inline int restoring_vdd_param(struct ll_mlx90640 *handle, struct ll_mlx90640_fixed_params *params)
{
    int res;
    uint16_t regdata;

    READ_16BITS(handle, 0x2433, &regdata, 1);
    params->k_vdd = (int8_t)(regdata >> 8);
    params->k_vdd <<= 5;

    params->vdd25 = regdata & 0xff;
    params->vdd25 = ((params->vdd25 - 256) << 5) - (1 << 13);

    return 0;
}

static inline int restoring_ta_param(struct ll_mlx90640 *handle, struct ll_mlx90640_fixed_params *params)
{
    int res;
    uint16_t regdata;
    int16_t temp;
    int16_t alpha_ptat_ee;

    READ_16BITS(handle, 0x2432, &regdata, 1);
    temp = (int16_t)(regdata >> 10);
    if (temp > 31)
        temp -= 64;
    params->k_vptat = (float)temp / (1 << 12);

    temp = (int16_t)(regdata & 0x03ff);
    if (temp > 511)
        temp -= 1024;
    params->k_tptat = (float)temp / (1 << 3);

    READ_16BITS(handle, 0x2431, &regdata, 1);
    params->v_ptat25 = (int16_t)regdata;

    READ_16BITS(handle, 0x2410, &regdata, 1);
    temp = (int16_t)(regdata >> 12);
    params->alpha_ptat = (float)temp / (1 << 2) + 8;

    return 0;
}

static inline int restoring_offset(struct ll_mlx90640 *handle,
                                   struct ll_mlx90640_raw_data *data,
                                   struct ll_mlx90640_fixed_params *params)
{
    int res;
    int i, j;
    uint16_t regdata;
    int16_t offset_avg;
    uint8_t occ_scale_row;
    uint8_t occ_scale_col;
    uint8_t occ_scale_remnant;
    int8_t occ_row[24];
    int8_t occ_col[32];
    uint16_t *p_offset;
    int16_t *p_pix_os_ref;

    READ_16BITS(handle, 0x2411, (uint16_t *)&offset_avg, 1);
    READ_16BITS(handle, 0x2410, &regdata, 1);
    occ_scale_row = (uint8_t)((regdata & 0x0f00) >> 8);
    occ_scale_col = (uint8_t)((regdata & 0x00f0) >> 4);
    occ_scale_remnant = (uint8_t)(regdata & 0x000f);
    READ_16BITS(handle, 0x2412, data->params, 6);
    for (i = 0; i < 6; i++)
    {
        for (j = 0; j < 4; j++)
        {
            int8_t temp = (int8_t)((data->params[i] >> (j << 2)) & 0x000f);
            if (temp > 7)
                temp -= 16;
            occ_row[j + i * 4] = temp;
        }
    }
    READ_16BITS(handle, 0x2418, data->params, 8);
    for (i = 0; i < 8; i++)
    {
        for (j = 0; j < 4; j++)
        {
            int8_t temp = (uint8_t)((data->params[i] >> (j << 2)) & 0x000f);
            if (temp > 7)
                temp -= 16;
            occ_col[j + i * 4] = temp;
        }
    }
    READ_16BITS(handle, 0x2440, data->data, 768);
    p_offset = data->data;
    p_pix_os_ref = params->pix_os_ref;
    for (i = 0; i < 24; i++)
    {
        for (j = 0; j < 32; j++)
        {
            int8_t offset = (int8_t)(*p_offset++ >> 10);
            if (offset > 31)
                offset -= 64;
            *p_pix_os_ref++ = offset_avg +
                              (occ_row[i] << occ_scale_row) +
                              (occ_col[j] << occ_scale_col) +
                              (offset << occ_scale_remnant);
        }
    }
    return 0;
}

static inline int restoring_sensitivity_alpha(struct ll_mlx90640 *handle,
                                              struct ll_mlx90640_raw_data *data,
                                              struct ll_mlx90640_fixed_params *params)
{
    int res;
    int i, j;
    uint16_t regdata;
    int16_t a_reference;
    uint8_t a_scale;
    uint8_t acc_scale_row;
    uint8_t acc_scale_col;
    uint8_t acc_scale_remnant;
    int8_t acc_row[24];
    int8_t acc_col[32];
    uint16_t *p_a_pixel;
    int16_t *p_alpha;

    READ_16BITS(handle, 0x2421, (uint16_t *)&a_reference, 1);
    READ_16BITS(handle, 0x2420, &regdata, 1);
    a_scale = (regdata >> 12) + 30;
    acc_scale_row = (regdata & 0x0f00) >> 8;
    acc_scale_col = (regdata & 0x00f0) >> 4;
    acc_scale_remnant = regdata & 0x000f;

    READ_16BITS(handle, 0x2422, data->params, 6);
    for (i = 0; i < 6; i++)
    {
        for (j = 0; j < 4; j++)
        {
            int8_t temp = (int8_t)((data->params[i] >> (j << 2)) & 0x000f);
            if (temp > 7)
                temp -= 16;
            acc_row[j + i * 4] = temp;
        }
    }
    READ_16BITS(handle, 0x2428, data->params, 8);
    for (i = 0; i < 8; i++)
    {
        for (j = 0; j < 4; j++)
        {
            int8_t temp = (int8_t)((data->params[i] >> (j << 2)) & 0x000f);
            if (temp > 7)
                temp -= 16;
            acc_col[j + i * 4] = temp;
        }
    }
    // READ_16BITS(handle, 0x2440, data->data, 768);//获取offset时读过一次 不需要再读
    p_a_pixel = data->data;
    p_alpha = params->alpha;
    for (i = 0; i < 24; i++)
    {
        for (j = 0; j < 32; j++)
        {
            int8_t a_pixel = (int8_t)((*p_a_pixel++ & 0x03f0) >> 4);
            if (a_pixel > 31)
                a_pixel = a_pixel - 64;
            *p_alpha = a_reference +
                       (acc_row[i] << acc_scale_row) +
                       (acc_col[j] << acc_scale_col) +
                       (a_pixel << acc_scale_remnant);
            *p_alpha++ /= (1 << a_scale);
        }
    }
    return 0;
}

static inline int restoring_kv(struct ll_mlx90640 *handle, struct ll_mlx90640_fixed_params *params)
{
    int res;
    int i, j;
    uint16_t regdata;
    uint8_t kv_scale;
    int8_t kv[2][2];
    int8_t *p_kv;

    READ_16BITS(handle, 0x2434, &regdata, 1);
    kv[0][0] = (int8_t)((regdata >> 12) & 0x000f);
    kv[1][0] = (int8_t)((regdata >> 8) & 0x000f);
    kv[0][1] = (int8_t)((regdata >> 4) & 0x000f);
    kv[1][1] = (int8_t)(regdata & 0x000f);

    p_kv = (uint8_t *)kv;
    for (i = 0; i < 4; i++)
    {
        if (*p_kv > 7)
            *p_kv -= 16;
        p_kv++;
    }
    READ_16BITS(handle, 0x2438, &regdata, 1);
    kv_scale = (uint8_t)((regdata & 0x0f00) >> 8);

    p_kv = params->kv;
    for (i = 0; i < 24; i++)
    {
        for (j = 0; j < 32; j++)
        {
            *p_kv++ = kv[i % 2][j % 2] / (1 << kv_scale);
        }
    }
    return 0;
}

static inline int restoring_kta(struct ll_mlx90640 *handle, struct ll_mlx90640_fixed_params *params)
{
    int res;
    int i, j;
    uint16_t regdata;



    return 0;
}
int ll_mlx90640_get_params(struct ll_mlx90640 *handle, struct ll_mlx90640_fixed_params *params)
{
}

static int calculate_ta(struct ll_mlx90640 *handle, struct ll_mlx90640_fixed_params *params, struct ll_mlx90640_raw_data *data)
{
    int16_t v_ptat = data->params[0x0020];
    int16_t v_be = data->params[0x0000];
    float v_ptat_art;
    float v_diff;

    v_ptat_art = (float)v_ptat / (v_ptat * params->alpha_ptat + v_be);
    v_ptat_art *= (2 << 18);

    v_diff = (float)(data->params[0x002a] - params->vdd25) / params->k_vdd;
    handle->ta = params->v_ptat_art / (1 + params->k_vptat * v_diff);
    handle->ta = (handle->ta - (float)params->v_ptat25) / params->k_tptat + 25;
}