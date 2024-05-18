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

#include <zmk/output/output_generic.h> // struct output_generic_api

#ifdef __cplusplus
extern "C" {
#endif

enum output_haptic_feedback_driver {
    OUTPUT_HAPTIC_FEEDBACK_DRIVER_DRV2605,
};

struct output_haptic_feedback_config {
    enum output_haptic_feedback_driver fb_drv;
    const struct device *fb_dev;
};

struct output_haptic_feedback_data {
    const struct device *dev;
    bool busy;
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */
