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

enum output_motorized_fader_driver {
    OUTPUT_MOTORIZED_FADER_DRIVER_TB6612FNG,
    OUTPUT_MOTORIZED_FADER_DRIVER_DRV883X,
};

struct output_motorized_fader_config {
    const struct device *sensor_dev;
    uint32_t sensor_channel;
    const struct device *motor_dev;
    enum output_motorized_fader_driver motor_drv;
    uint8_t motor_channel;
    int16_t proportional;
    int16_t integral;
    int16_t derivative;
};

struct output_motorized_fader_data {
    const struct device *dev;
    int16_t value;
    bool busy;
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */
