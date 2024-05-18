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
#include <zephyr/drivers/gpio.h>

#include <zmk/output/output_generic_api.h>

#ifdef __cplusplus
extern "C" {
#endif

struct output_generic_config {
    bool has_control;
    const struct gpio_dt_spec control;
};

struct output_generic_data {
    const struct device *dev;
    bool busy;
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */
