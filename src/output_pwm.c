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

static int output_pwm_enable(const struct device *dev, uint8_t force) {
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
    uint32_t pulse = period / 256.0 * force;
    LOG_DBG("enable: force %d period %d pulse %d", force, period, pulse);
    pwm_set_dt(&pwm, period, pulse);

exit:
    data->busy = false;
    return rc;
}

static int output_pwm_disable(const struct device *dev) {
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

    LOG_DBG("disable");
    uint32_t period = pwm.period;
    pwm_set_dt(&pwm, period, 0);

exit:
    data->busy = false;
    return rc;
}

static int output_pwm_get(const struct device *dev) {
    struct output_pwm_data *data = dev->data;
    return !data->busy;
}

static int output_pwm_init(const struct device *dev) {
    struct output_pwm_data *data = dev->data;
    const struct output_pwm_config *config = dev->config;
    data->dev = dev;

    struct pwm_dt_spec pwm = config->pwm;
    if (!pwm_is_ready_dt(&pwm)) {
        LOG_WRN("PWM is not ready");
        return -EIO;
    }

    LOG_DBG("init");
    uint32_t period = pwm.period;
    pwm_set_dt(&pwm, period, 0);

    return 0;
}

static const struct output_generic_api api = {
    .enable = output_pwm_enable,
    .disable = output_pwm_disable,
    .get = output_pwm_get,
};

#define ZMK_OUTPUT_INIT_PRIORITY 91

#define OPWM_INST(n)                                                             \
    static struct output_pwm_data data_##n = { .busy = false, };                 \
    static const struct output_pwm_config config_##n = {                         \
        .pwm = PWM_DT_SPEC_GET(DT_DRV_INST(n)),                                  \
    };                                                                           \
    DEVICE_DT_INST_DEFINE(0, output_pwm_init, DEVICE_DT_INST_GET(n),             \
                          &data_##n, &config_##n,                                \
                          POST_KERNEL, ZMK_OUTPUT_INIT_PRIORITY, &api);

DT_INST_FOREACH_STATUS_OKAY(OPWM_INST)

// #endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
