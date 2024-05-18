/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_output_haptic_feedback

#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#if IS_ENABLED(CONFIG_DRV2605)
#include <zmk/drivers/drv2605.h>
#endif

// #if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/output/output_haptic_feedback.h>

static int output_haptic_feedback_set_value(const struct device *dev, uint8_t value) {
    struct output_haptic_feedback_data *data = dev->data;
    const struct output_haptic_feedback_config *config = dev->config;

    const struct device *fb_dev = config->fb_dev;
    if (!fb_dev) {
        LOG_WRN("No haptic feedback device assigned");
        return -EIO;
    }

    if (data->busy) {
        LOG_WRN("output device is busy");
        return -EBUSY;
    }
    data->busy = true;
    
    int rc = 0;

#if IS_ENABLED(CONFIG_DRV2605)
    if (config->fb_drv == OUTPUT_HAPTIC_FEEDBACK_DRIVER_DRV2605) {
        int err = 0;
    	struct sensor_value val = { .val1 = 0, .val2 = 0 };

        if (value) {
            val.val1 = 0; // slot 0
            val.val2 = value; // waveformm
            err = sensor_attr_set(fb_dev, SENSOR_CHAN_ALL, 
                                (enum sensor_attribute) DRV2605_ATTR_WAVEFORM, &val);
            if (err) {
                LOG_WRN("Fail to sensor_attr_set DRV2605_ATTR_WAVEFORM");
                rc = -EIO;
                goto exit;
            }

            val.val1 = 1; // slot 1
            val.val2 = 0; // waveformm - empty
            err = sensor_attr_set(fb_dev, SENSOR_CHAN_ALL,
                                (enum sensor_attribute) DRV2605_ATTR_WAVEFORM, &val);
            if (err) {
                LOG_WRN("Fail to sensor_attr_set DRV2605_ATTR_WAVEFORM");
                rc = -EIO;
                goto exit;
            }

            err = sensor_attr_set(fb_dev, SENSOR_CHAN_ALL,
                                (enum sensor_attribute) DRV2605_ATTR_GO, &val);
            if (err) {
                LOG_WRN("Fail to sensor_attr_set DRV2605_ATTR_GO");
                rc = -EIO;
                goto exit;
            }
        }
        else {
            err = sensor_attr_set(fb_dev, SENSOR_CHAN_ALL, 
                                (enum sensor_attribute) DRV2605_ATTR_WAVEFORM, &val);
            if (err) {
                LOG_WRN("Fail to sensor_attr_set DRV2605_ATTR_WAVEFORM");
                rc = -EIO;
                goto exit;
            }
        }
    }
#endif /* IS_ENABLED(CONFIG_DRV2605) */

exit:
    data->busy = false;
    return rc;
}

static int output_haptic_feedback_get_ready(const struct device *dev) {
    struct output_haptic_feedback_data *data = dev->data;
    return !data->busy;
}

static int output_haptic_feedback_init(const struct device *dev) {
    struct output_haptic_feedback_data *data = dev->data;
    // const struct output_haptic_feedback_config *config = dev->config;
    data->dev = dev;
    return 0;
}

static const struct output_generic_api api = {
    .set_value = output_haptic_feedback_set_value,
    .get_ready = output_haptic_feedback_get_ready,
};

#define ZMK_OUTPUT_INIT_PRIORITY 91

#define OHFB_INST(n)                                                             \
    static struct output_haptic_feedback_data data_##n = { .busy = false, };     \
    static const struct output_haptic_feedback_config config_##n = {             \
        .fb_drv = DT_INST_ENUM_IDX_OR(0, driver,                                 \
                                      OUTPUT_HAPTIC_FEEDBACK_DRIVER_DRV2605),    \
        .fb_dev = COND_CODE_1(                                                   \
                    DT_INST_NODE_HAS_PROP(n, device),                            \
                    (DEVICE_DT_GET(DT_INST_PHANDLE(n, device))),                 \
                    (NULL)),                                                     \
    };                                                                           \
    DEVICE_DT_INST_DEFINE(0, output_haptic_feedback_init, DEVICE_DT_INST_GET(n), \
                          &data_##n, &config_##n,                                \
                          POST_KERNEL, ZMK_OUTPUT_INIT_PRIORITY, &api);

DT_INST_FOREACH_STATUS_OKAY(OHFB_INST)

// #endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
