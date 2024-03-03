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

#ifdef __cplusplus
extern "C" {
#endif

struct output_generic_config {
    const struct gpio_dt_spec control;
};

struct output_generic_data {
    const struct device *dev;
    bool status;
};

typedef int (*output_enable_t)(const struct device *dev, uint8_t force);
typedef int (*output_disable_t)(const struct device *dev);
typedef int (*output_get_t)(const struct device *dev);

struct output_generic_api {
    output_enable_t enable;
    output_disable_t disable;
    output_get_t get;
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */
