/**
 * @file ll_mlx90640.h
 * @author salalei (1028609078@qq.com)
 * @brief
 * @version 0.1
 * @date 2022-02-07
 *
 * @copyright Copyright (c) 2022
 *
 */
#ifndef __LL_MLX90640_H__
#define __LL_MLX90640_H__

#include "ll_i2c.h"

enum ll_mlx90640_rate
{
    LL_MLX90640_RATE_0_5 = 0,
    LL_MLX90640_RATE_1,
    LL_MLX90640_RATE_2,
    LL_MLX90640_RATE_4,
    LL_MLX90640_RATE_8,
    LL_MLX90640_RATE_16,
    LL_MLX90640_RATE_32,
    LL_MLX90640_RATE_64,
    LL_MLX90640_RATE_LIMIT,
};

struct ll_mlx90640_raw_data
{
    uint16_t data[768];
    uint16_t params[64];
};

struct ll_mlx90640_fixed_params
{
    int16_t k_vdd;
    int16_t vdd25;

    int16_t v_ptat25;
    float k_vptat;
    float k_tptat;
    float v_ptat_art;
    float alpha_ptat;

    int16_t pix_os_ref[768];
    int16_t alpha[768];
    int8_t kv[768];
    int8_t kta[768];
};

struct ll_mlx90640
{
    float ta;
    struct ll_i2c_dev dev;
};

int ll_mlx90640_init(struct ll_mlx90640 *handle,
                     struct ll_i2c_bus *i2c_bus,
                     enum ll_mlx90640_rate rate);
int ll_mlx90640_config(struct ll_mlx90640 *handle, enum ll_mlx90640_rate rate);
int ll_mlx90640_read_raw_data(struct ll_mlx90640 *handle, struct ll_mlx90640_raw_data *data);

#endif