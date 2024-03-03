/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_output_generic

#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

// #if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/output/output_generic.h>

static int output_generic_enable(const struct device *dev, uint8_t force) {
    struct output_generic_data *data = dev->data;
    const struct output_generic_config *config = dev->config;

    if (data->status) {
        LOG_WRN("output device is busy");
        return -EBUSY;
    }

    if (gpio_pin_set_dt(&config->control, 1)) {
        LOG_WRN("Failed to set output control pin");
        return -EIO;
    }
    data->status = true;

    return 0;
}

static int output_generic_disable(const struct device *dev) {
    struct output_generic_data *data = dev->data;
    const struct output_generic_config *config = dev->config;

    if (!data->status) {
        LOG_WRN("output device is busy");
        return -EBUSY;
    }

    if (gpio_pin_set_dt(&config->control, 0)) {
        LOG_WRN("Failed to set output control pin");
        LOG_WRN("Failed to clear output control pin");
        return -EIO;
    }
    data->status = false;
    return 0;
}

static int output_generic_get(const struct device *dev) {
    struct output_generic_data *data = dev->data;
    return data->status;
}

static int output_generic_init(const struct device *dev) {
    struct output_generic_data *data = dev->data;
    const struct output_generic_config *config = dev->config;

    data->dev = dev;

    if (gpio_pin_configure_dt(&config->control, GPIO_OUTPUT_INACTIVE)) {
        LOG_ERR("Failed to configure output control pin");
        return -EIO;
    }

    return 0;
}

static const struct output_generic_api api = {
    .enable = output_generic_enable,
    .disable = output_generic_disable,
    .get = output_generic_get,
};

static struct output_generic_data data = {
    .status = false,
};

static const struct output_generic_config config = {
    .control = GPIO_DT_SPEC_INST_GET(0, control_gpios),
};

#define ZMK_OUTPUT_INIT_PRIORITY 81

DEVICE_DT_INST_DEFINE(0, output_generic_init, DEVICE_DT_INST_GET(0), &data, &config,
                      POST_KERNEL, ZMK_OUTPUT_INIT_PRIORITY, &api);

// #endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
