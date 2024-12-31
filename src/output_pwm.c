/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_output_pwm

#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>

// #if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/output/output_pwm.h>

static int output_pwm_set_value(const struct device *dev, uint8_t value) {
    struct output_pwm_data *data = dev->data;
    const struct output_pwm_config *config = dev->config;

    struct pwm_dt_spec pwm = config->pwm;
    if (!pwm_is_ready_dt(&pwm)) {
        LOG_WRN("PWM is not ready");
        return -EIO;
    }

    if (data->busy) {
        LOG_WRN("output device is busy");
        return -EBUSY;
    }
    data->busy = true;

    int rc = 0;

    uint32_t period = pwm.period;
    if (value) {
        uint32_t pulse = period / 256.0 * value;
        LOG_DBG("enable: force %d period %d pulse %d", value, period, pulse);
        pwm_set_dt(&pwm, period, pulse);
    } else {
        pwm_set_dt(&pwm, period, 0);
    }

exit:
    data->busy = false;
    return rc;
}

static int output_pwm_get_ready(const struct device *dev) {
    struct output_pwm_data *data = dev->data;
    return !data->busy;
}

static int output_pwm_init(const struct device *dev) {
    struct output_pwm_data *data = dev->data;
    const struct output_pwm_config *config = dev->config;
    data->dev = dev;
    return output_pwm_set_value(dev, 0);
}

static const struct output_generic_api api = {
    .set_value = output_pwm_set_value,
    .get_ready = output_pwm_get_ready,
};

#define ZMK_OUTPUT_INIT_PRIORITY 91

#define OPWM_INST(n)                                                                               \
    static struct output_pwm_data data_##n = {                                                     \
        .busy = false,                                                                             \
    };                                                                                             \
    static const struct output_pwm_config config_##n = {                                           \
        .pwm = PWM_DT_SPEC_GET(DT_DRV_INST(n)),                                                    \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(0, output_pwm_init, DEVICE_DT_INST_GET(n), &data_##n, &config_##n,       \
                          POST_KERNEL, ZMK_OUTPUT_INIT_PRIORITY, &api);

DT_INST_FOREACH_STATUS_OKAY(OPWM_INST)

// #endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
