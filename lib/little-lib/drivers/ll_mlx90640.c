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
 * @param buf 指向用与缓存ram数据的缓存区
 * @return int
 */
int ll_mlx90640_read_raw_data(struct ll_mlx90640 *handle, struct ll_mlx90640_ram_buf *buf)
{
    int res;
    uint16_t regdata;

    LL_ASSERT(handle && buf);
    READ_16BITS(handle, STATUS_REG, &regdata, 1);
    if (!(regdata & DATA_READY_IN_RAM_BIT))
        return -EAGAIN;

    WRITE_16BIT(handle, STATUS_REG, 0);
    READ_16BITS(handle, RAM_ADDR, (uint16_t *)buf, sizeof(struct ll_mlx90640_ram_buf));

    return 0;
}

static inline void restoring_vdd_param(const struct ll_mlx90640_ee_buf *buf,
                                       struct ll_mlx90640_fixed_params *params)
{
    params->kvdd = (int8_t)(buf->data[0x33] >> 8);
    params->kvdd *= (1 << 5);

    params->vdd25 = (int16_t)(buf->data[0x33] & 0x00ff);
    params->vdd25 = (params->vdd25 - 256) * (1 << 5) - (1 << 13);
}

static inline void restoring_ta_param(const struct ll_mlx90640_ee_buf *buf,
                                      struct ll_mlx90640_fixed_params *params)
{
    int16_t temp;

    temp = (int16_t)(buf->data[0x32] >> 10);
    if (temp > 31)
        temp -= 64;
    params->k_vptat = (float)temp / (1 << 12);

    temp = (int16_t)(buf->data[0x32] & 0x03ff);
    if (temp > 511)
        temp -= 1024;
    params->k_tptat = (float)temp / (1 << 3);

    params->v_ptat25 = (int16_t)buf->data[0x31];

    temp = (int16_t)(buf->data[0x10] >> 12);
    params->alpha_ptat = (float)temp / (1 << 2) + 8;
}

static inline void restoring_offset(const struct ll_mlx90640_ee_buf *buf,
                                    struct ll_mlx90640_fixed_params *params)
{
    int i, j;
    int16_t offset_avg = (int16_t)buf->data[0x11];
    uint8_t occ_scale_row = (uint8_t)((buf->data[0x10] & 0x0f00) >> 8);
    uint8_t occ_scale_col = (uint8_t)((buf->data[0x10] & 0x00f0) >> 4);
    uint8_t occ_scale_remnant = (uint8_t)(buf->data[0x10] & 0x000f);
    int8_t occ_row[24];
    int8_t occ_col[32];
    const uint16_t *p_offset = &buf->data[0x40];
    int16_t *p_pix_os_ref = params->pix_os_ref;

    for (i = 0; i < 6; i++)
    {
        for (j = 0; j < 4; j++)
        {
            int8_t temp = (int8_t)((buf->data[0x12 + i] >> (j << 2)) & 0x000f);
            if (temp > 7)
                temp -= 16;
            occ_row[j + i * 4] = temp;
        }
    }
    for (i = 0; i < 8; i++)
    {
        for (j = 0; j < 4; j++)
        {
            int8_t temp = (int8_t)((buf->data[0x18 + i] >> (j << 2)) & 0x000f);
            if (temp > 7)
                temp -= 16;
            occ_col[j + i * 4] = temp;
        }
    }

    for (i = 0; i < 24; i++)
    {
        for (j = 0; j < 32; j++)
        {
            int8_t offset = (int8_t)(*p_offset++ >> 10);
            if (offset > 31)
                offset -= 64;
            *p_pix_os_ref++ = offset_avg +
                              occ_row[i] * (1 << occ_scale_row) +
                              occ_col[j] * (1 << occ_scale_col) +
                              offset * (1 << occ_scale_remnant);
        }
    }
}

static inline void restoring_sensitivity_alpha(const struct ll_mlx90640_ee_buf *buf,
                                               struct ll_mlx90640_fixed_params *params)
{
    int i, j;
    int16_t a_reference = (int16_t)buf->data[0x21];
    uint8_t acc_scale_row = (buf->data[0x20] & 0x0f00) >> 8;
    uint8_t acc_scale_col = (buf->data[0x20] & 0x00f0) >> 4;
    uint8_t acc_scale_remnant = buf->data[0x20] & 0x000f;
    int8_t acc_row[24];
    int8_t acc_col[32];
    const uint16_t *p_a_pixel = &buf->data[0x40];
    int16_t *p_alpha = params->alpha;

    params->alpha_scale = (buf->data[0x20] >> 12) + 30;

    for (i = 0; i < 6; i++)
    {
        for (j = 0; j < 4; j++)
        {
            int8_t temp = (int8_t)((buf->data[0x22 + i] >> (j << 2)) & 0x000f);
            if (temp > 7)
                temp -= 16;
            acc_row[j + i * 4] = temp;
        }
    }
    for (i = 0; i < 8; i++)
    {
        for (j = 0; j < 4; j++)
        {
            int8_t temp = (int8_t)((buf->data[0x28 + i] >> (j << 2)) & 0x000f);
            if (temp > 7)
                temp -= 16;
            acc_col[j + i * 4] = temp;
        }
    }

    for (i = 0; i < 24; i++)
    {
        for (j = 0; j < 32; j++)
        {
            int8_t a_pixel = (int8_t)((*p_a_pixel++ & 0x03f0) >> 4);
            if (a_pixel > 31)
                a_pixel -= 64;
            *p_alpha++ = a_reference +
                         acc_row[i] * (1 << acc_scale_row) +
                         acc_col[j] * (1 << acc_scale_col) +
                         a_pixel * (1 << acc_scale_remnant);
        }
    }
}

static inline float read_alpha(struct ll_mlx90640_fixed_params *params, int i)
{
    return (float)params->alpha[i] / (1 << params->alpha_scale);
}

static inline void restoring_kv(const struct ll_mlx90640_ee_buf *buf,
                                struct ll_mlx90640_fixed_params *params)
{
    int i;
    uint8_t kv_scale = (uint8_t)((buf->data[0x38] & 0x0f00) >> 8);
    int8_t kv[2][2];
    int8_t *p_kv = (int8_t *)kv;
    float *p_float_kv = (float *)params->kv;

    kv[0][0] = (int8_t)((buf->data[0x34] >> 12) & 0x000f);
    kv[1][0] = (int8_t)((buf->data[0x34] >> 8) & 0x000f);
    kv[0][1] = (int8_t)((buf->data[0x34] >> 4) & 0x000f);
    kv[1][1] = (int8_t)(buf->data[0x34] & 0x000f);

    for (i = 0; i < 4; i++)
    {
        if (*p_kv > 7)
            *p_kv -= 16;
        *p_float_kv++ = (float)*p_kv++ / (1 << kv_scale);
    }
}

static inline float read_kv(struct ll_mlx90640_fixed_params *params,
                            uint8_t x,
                            uint8_t y)
{
    return params->kv[y % 2][x % 2];
}

static inline void restoring_kta(const struct ll_mlx90640_ee_buf *buf,
                                 struct ll_mlx90640_fixed_params *params)
{
    int i, j;
    int8_t kta_rc_ee[2][2];
    uint8_t kta_scale_2 = (uint8_t)(buf->data[0x38] & 0x000f);
    int16_t *p_kta = params->kta;

    kta_rc_ee[0][0] = (int8_t)((buf->data[0x36] & 0xff00) >> 8);
    kta_rc_ee[1][0] = (int8_t)(buf->data[0x36] & 0x00ff);
    kta_rc_ee[0][1] = (int8_t)((buf->data[0x37] & 0xff00) >> 8);
    kta_rc_ee[1][1] = (int8_t)(buf->data[0x37] & 0x00ff);

    params->kta_scale_1 = (uint8_t)(((buf->data[0x38] & 0x00f0) >> 4) + 8);

    for (i = 0; i < 24; i++)
    {
        for (j = 0; j < 32; j++)
        {
            int8_t kta_ee = (int8_t)((buf->data[0x40] & 0x000e) >> 1);
            if (kta_ee > 3)
                kta_ee -= 8;
            *p_kta++ = kta_rc_ee[i % 2][j % 2] + kta_ee * (1 << kta_scale_2);
        }
    }
}

static inline void restoring_gain(const struct ll_mlx90640_ee_buf *buf,
                                  struct ll_mlx90640_fixed_params *params)
{
    params->gain = (int16_t)buf->data[0x30];
}

static inline void restoring_ks_ta(const struct ll_mlx90640_ee_buf *buf,
                                   struct ll_mlx90640_fixed_params *params)
{
    int8_t ks_ta_ee = (int8_t)((buf->data[0x3c] & 0xff00) >> 8);
    params->ks_ta = ks_ta_ee / (1 << 13);
}

static inline void restoring_corner_temp(const struct ll_mlx90640_ee_buf *buf,
                                         struct ll_mlx90640_fixed_params *params)
{
    int8_t step = ((buf->data[0x3f] & 0x3000) >> 12) * 10;
    params->ct[0] = -40;
    params->ct[1] = 0;
    params->ct[2] = ((buf->data[0x3f] & 0x00f0) >> 4) * step;
    params->ct[3] = ((buf->data[0x3f] & 0x0f00) >> 8) * step + params->ct[2];
}

static inline void restoring_ks_to(const struct ll_mlx90640_ee_buf *buf,
                                   struct ll_mlx90640_fixed_params *params)
{
    uint8_t ks_to_scale = (uint8_t)(buf->data[0x3f] & 0x000f) + 8;
    params->ks_to[0] = (float)((int8_t)(buf->data[0x3d] & 0x00ff)) / (1 << ks_to_scale);
    params->ks_to[1] = (float)((int8_t)((buf->data[0x3d] & 0xff00) >> 8)) / (1 << ks_to_scale);
    params->ks_to[2] = (float)((int8_t)(buf->data[0x3e] & 0x00ff)) / (1 << ks_to_scale);
    params->ks_to[3] = (float)((int8_t)((buf->data[0x3e] & 0xff00) >> 8)) / (1 << ks_to_scale);
}

static inline void restoring_alpha_corr_range(const struct ll_mlx90640_ee_buf *buf,
                                              struct ll_mlx90640_fixed_params *params)
{
    params->alpha_corr_range[0] = 1 / (float)(1 + params->ks_to[0] * 40);
    params->alpha_corr_range[1] = 1;
    params->alpha_corr_range[2] = 1 + params->ks_to[1] * params->ct[2];
    params->alpha_corr_range[3] = (1 + params->ks_to[1] * params->ct[2]) *
                                  (1 + params->ks_to[2] * (params->ct[3] - params->ct[2]));
}

static inline void restoring_sensitivity_a_cp(const struct ll_mlx90640_ee_buf *buf,
                                              struct ll_mlx90640_fixed_params *params)
{
    uint8_t a_scale_cp = (uint8_t)((buf->data[0x20] & 0xf000) >> 12) + 27;
    int8_t cp_p1_p0_ratio;
    cp_p1_p0_ratio = (int8_t)((buf->data[0x39] & 0xfc00) >> 10);
    if (cp_p1_p0_ratio > 31)
        cp_p1_p0_ratio -= 64;
    params->a_cp_subpage[0] = (float)((int16_t)(buf->data[0x39] & 0x03ff)) / (1 << a_scale_cp);
    params->a_cp_subpage[1] = params->a_cp_subpage[0] * (1 + (float)cp_p1_p0_ratio / (1 << 7));
}

static inline void restoring_off_cp(const struct ll_mlx90640_ee_buf *buf,
                                    struct ll_mlx90640_fixed_params *params)
{
    int8_t off_cp_subpage_1_delta;
    params->off_cp_subpage[0] = (int16_t)(buf->data[0x3a] & 0x3ff);
    if (params->off_cp_subpage[0] > 511)
        params->off_cp_subpage[0] -= 1024;
    off_cp_subpage_1_delta = (int8_t)((buf->data[0x3a] & 0xfc00) >> 10);
    if (off_cp_subpage_1_delta > 31)
        off_cp_subpage_1_delta -= 64;
    params->off_cp_subpage[1] = params->off_cp_subpage[0] + off_cp_subpage_1_delta;
}

static inline void restoring_kv_cp(const struct ll_mlx90640_ee_buf *buf,
                                   struct ll_mlx90640_fixed_params *params)
{
    uint8_t kv_scale = (uint8_t)((buf->data[0x38] & 0x0f00) >> 8);
    int8_t kv_cp_ee = (int8_t)((buf->data[0x3b] & 0xff00) >> 8);
    params->kv_cp = (float)kv_cp_ee / (1 << kv_scale);
}

static inline void restoring_kta_cp(const struct ll_mlx90640_ee_buf *buf,
                                    struct ll_mlx90640_fixed_params *params)
{
    int8_t kta_cp_ee = (int8_t)(buf->data[0x3b] & 0x00ff);
    params->kta_cp = (float)kta_cp_ee / (1 << params->kta_scale_1);
}

static inline void restoring_tgc(const struct ll_mlx90640_ee_buf *buf,
                                 struct ll_mlx90640_fixed_params *params)
{
    params->tgc = (float)((int8_t)(buf->data[0x3c] & 0x00ff)) / (1 << 5);
}

static inline void restoring_resolution(const struct ll_mlx90640_ee_buf *buf,
                                        struct ll_mlx90640_fixed_params *params)
{
    params->resolution_ee = (uint8_t)((buf->data[0x38] & 0x3000) >> 12);
}

int ll_mlx90640_get_params(struct ll_mlx90640 *handle,
                           struct ll_mlx90640_ee_buf *buf,
                           struct ll_mlx90640_fixed_params *params)
{
    int res;

    READ_16BITS(handle, 0x2400, buf->data, sizeof(struct ll_mlx90640_ee_buf) >> 1);
    restoring_vdd_param(buf, params);
    restoring_ta_param(buf, params);
    restoring_offset(buf, params);
    restoring_sensitivity_alpha(buf, params);
    restoring_kv(buf, params);
    restoring_kta(buf, params);
    restoring_gain(buf, params);
    restoring_ks_ta(buf, params);
    restoring_corner_temp(buf, params);
    restoring_ks_to(buf, params);
    restoring_alpha_corr_range(buf, params);
    restoring_sensitivity_a_cp(buf, params);
    restoring_off_cp(buf, params);
    restoring_kv_cp(buf, params);
    restoring_kta_cp(buf, params);
    restoring_tgc(buf, params);
    restoring_resolution(buf, params);
    return 0;
}

static inline float vdd_diff_calculate(struct ll_mlx90640_fixed_params *params,
                                       struct ll_mlx90640_ram_buf *buf)
{
    int16_t temp;
    float resolution_reg;

    resolution_reg = (float)(1 << params->resolution_ee) / (1 << 2);
    temp = (int16_t)(buf->params[0x2a]);
    return (resolution_reg * temp - params->vdd25) / params->kvdd;
}

static inline float ta_diff_calculate(struct ll_mlx90640_fixed_params *params,
                                      struct ll_mlx90640_ram_buf *data,
                                      float v_diff)
{
    float ta_diff;
    int16_t v_ptat = (int16_t)data->params[0x0020];
    int16_t v_be = (int16_t)data->params[0x0000];
    float v_ptat_art;

    v_ptat_art = (float)v_ptat * (1 << 18) / (v_ptat * params->alpha_ptat + v_be);

    ta_diff = v_ptat_art / (1 + params->k_vptat * v_diff);
    ta_diff = (ta_diff - (float)params->v_ptat25) / params->k_tptat;
    return ta_diff;
}

static inline float kgain_calculate(struct ll_mlx90640_fixed_params *params,
                                    struct ll_mlx90640_ram_buf *buf)
{
    int16_t temp = (int16_t)buf->params[0x0a];
    return (float)params->gain / temp;
}

static inline ir_data_compensation(struct ll_mlx90640_fixed_params *params,
                                   struct ll_mlx90640_ram_buf *buf,
                                   struct ll_mlx90640_ir_data *data,
                                   float kgain,
                                   float ta_diff,
                                   float v_diff)
{
    int i, j;
    int16_t *src = (int16_t *)buf->data;
    int16_t *p_off = params->pix_os_ref;
    int16_t *p_kta = params->kta;
    float *dst = data->temp;

    for (i = 0; i < 32; i++)
    {
        for (j = 0; j < 24; j++)
        {
            float compen = (float)(*p_kta++) / (1 << params->kta_scale_1);
            compen = *p_off++ * (1 + compen * ta_diff);
            compen *= (1 + read_kv(params, i, j) * v_diff);
            *dst = *src++ * kgain - compen;
        }
    }
}

int ll_mlx90640_calculate_temp(struct ll_mlx90640 *handle,
                               struct ll_mlx90640_fixed_params *params,
                               struct ll_mlx90640_ram_buf *buf,
                               struct ll_mlx90640_ir_data *data)
{
    float v_diff;
    float ta_diff;
    float kgain;

    while (ll_mlx90640_read_raw_data(handle, buf))
    {
    }

    v_diff = vdd_diff_calculate(params, buf);
    LL_INFO("vdd_diff %.3f", v_diff);
    ta_diff = ta_diff_calculate(params, buf, v_diff);
    LL_INFO("ta_diff %.3f", ta_diff);
    kgain = kgain_calculate(params, buf);
    LL_INFO("kgain %f", kgain);
    ir_data_compensation(params, buf, data, kgain, ta_diff, v_diff);

    return 0;
}