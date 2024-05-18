/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>

#include <zmk/output/output_generic_api.h>

#ifdef __cplusplus
extern "C" {
#endif

struct output_pwm_config {
    struct pwm_dt_spec pwm;
};

struct output_pwm_data {
    const struct device *dev;
    bool busy;
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */
