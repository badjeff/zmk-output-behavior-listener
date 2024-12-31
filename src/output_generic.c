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

static int output_generic_set(const struct device *dev, uint8_t value) {
    struct output_generic_data *data = dev->data;
    const struct output_generic_config *config = dev->config;

    if (data->busy) {
        LOG_WRN("output device is busy");
        return -EBUSY;
    }
    data->busy = true;

    int rc = 0;

    if (config->has_control) {
        if (gpio_pin_set_dt(&config->control, value ? 1 : 0)) {
            LOG_WRN("Failed to set output control pin");
            rc = -EIO;
            goto exit;
        }
    }

exit:
    data->busy = false;
    return rc;
}

static int output_generic_get(const struct device *dev) {
    struct output_generic_data *data = dev->data;
    return !data->busy;
}

static int output_generic_init(const struct device *dev) {
    struct output_generic_data *data = dev->data;
    const struct output_generic_config *config = dev->config;

    data->dev = dev;

    if (!config->has_control) {
        LOG_ERR("Missing configure output control pin");
        return -EIO;
    }

    if (gpio_pin_configure_dt(&config->control, GPIO_OUTPUT_INACTIVE)) {
        LOG_ERR("Failed to configure output control pin");
        return -EIO;
    }

    return 0;
}

static const struct output_generic_api api = {
    .set_value = output_generic_set,
    .get_ready = output_generic_get,
};

#define ZMK_OUTPUT_INIT_PRIORITY 91

#define OG_INST(n)                                                                                 \
    static struct output_generic_data data_##n = {                                                 \
        .busy = false,                                                                             \
    };                                                                                             \
    static const struct output_generic_config config_##n = {                                       \
        .has_control = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, control_gpios), (true), (false)),      \
        .control = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, control_gpios),                            \
                               (GPIO_DT_SPEC_INST_GET(n, control_gpios)), (NULL)),                 \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(0, output_generic_init, DEVICE_DT_INST_GET(n), &data_##n, &config_##n,   \
                          POST_KERNEL, ZMK_OUTPUT_INIT_PRIORITY, &api);

DT_INST_FOREACH_STATUS_OKAY(OG_INST)

// #endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
