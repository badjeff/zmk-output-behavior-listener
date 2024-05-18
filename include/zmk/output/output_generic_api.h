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

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*output_set_value_t)(const struct device *dev, uint8_t value);
typedef int (*output_get_ready_t)(const struct device *dev);

struct output_generic_api {
    output_set_value_t set_value;
    output_get_ready_t get_ready;
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */
