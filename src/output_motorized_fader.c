/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_output_motorized_fader

#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>

#include <zmk/drivers/analog_input.h>

#if IS_ENABLED(CONFIG_TB6612FNG)
#include <zmk/drivers/tb6612fng.h>
#endif

#if IS_ENABLED(CONFIG_DRV883X)
#include <zmk/drivers/drv883x.h>
#endif

// #if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/output/output_motorized_fader.h>

static int output_motorized_fader_set_value(const struct device *dev, uint8_t value) {
    struct output_motorized_fader_data *data = dev->data;
    const struct output_motorized_fader_config *config = dev->config;

    const struct device *sensor_dev = config->sensor_dev;
    if (!sensor_dev) {
        LOG_WRN("No sensor device assigned");
        return -EIO;
    }

    const struct device *motor_dev = config->motor_dev;
    if (!motor_dev) {
        LOG_WRN("No motor driver device assigned");
        return -EIO;
    }

    if (data->busy) {
        LOG_WRN("output device is busy");
        return -EBUSY;
    }
    data->busy = true;

    LOG_WRN("fader set point: %d", value);

    int rc = 0;
    int err = 0;

    // err = sensor_sample_fetch(sensor_dev);
    // if (err) {
    //     LOG_WRN("Failed to fetch sample from device %d", err);
    //     rc = -EIO;
    //     goto exit;
    // }

    struct sensor_value val = {.val1 = 0, .val2 = 0};
    err = sensor_channel_get(sensor_dev, SENSOR_CHAN_ALL, &val);
    if (err) {
        LOG_WRN("Failed to get channel from device %d", err);
        rc = -EIO;
        goto exit;
    }
    uint32_t sen_val = val.val1;
    if (config->sensor_channel == 1) {
        sen_val = val.val2;
    }
    LOG_WRN("fader sen_val: %d", sen_val);

    val.val1 = 0;
    err = sensor_attr_set(sensor_dev, SENSOR_CHAN_ALL,
                          (enum sensor_attribute)ANALOG_INPUT_ATTR_ACTIVE, &val);
    if (err) {
        LOG_WRN("Fail to sensor_attr_set ANALOG_INPUT_ATTR_ACTIVE");
        rc = -EIO;
        goto exit;
    }

#if IS_ENABLED(CONFIG_TB6612FNG)
    if (config->motor_drv == OUTPUT_MOTORIZED_FADER_DRIVER_TB6612FNG) {
        struct sensor_value val = {.val1 = 0, .val2 = 0};

        const struct device *tb6612fng_dev = motor_dev;

        // int16_t p = config->proportional;
        // int16_t i = config->integral;
        // int16_t d = config->derivative;
        uint8_t vec = 96;

        uint32_t chan = config->motor_channel;
        int16_t vel = 0;

        if (value > sen_val) {
            // vel = -(value - sen_val);
            vel = -vec;
        } else if (value < sen_val) {
            // vel = (sen_val - value);
            vel = vec;
        }
        // LOG_WRN("fader vel: %d", vel);

        if (vel) {
            // enable enable pin, disable iif vel == 0
            val.val1 = (!vel) ? 0 : 1;
            val.val2 = 0;
            err = sensor_attr_set(tb6612fng_dev, SENSOR_CHAN_ALL,
                                  (enum sensor_attribute)TB6612FNG_ATTR_ENABLE, &val);
            if (err) {
                LOG_WRN("Fail to sensor_attr_set TB6612FNG_ATTR_ENABLE");
            }

            // set velocity and inversr flag
            val.val1 = abs(vel);
            val.val2 = vel < 0;
            err = sensor_attr_set(tb6612fng_dev, SENSOR_CHAN_ALL,
                                  (enum sensor_attribute)TB6612FNG_ATTR_VELOCITY, &val);
            if (err) {
                LOG_WRN("Fail to sensor_attr_set TB6612FNG_ATTR_VELOCITY");
            }

            // call sync to latch velocity setting to driver
            val.val1 = chan;
            val.val2 = 0;
            err = sensor_attr_set(tb6612fng_dev, SENSOR_CHAN_ALL,
                                  (enum sensor_attribute)TB6612FNG_ATTR_SYNC, &val);
            if (err) {
                LOG_WRN("Fail to sensor_attr_set TB6612FNG_ATTR_SYNC");
            }
        }

        while (true) {
            k_msleep(3);

            // err = sensor_sample_fetch(sensor_dev);
            // if (err) {
            //     LOG_WRN("Failed to fetch sample from device %d", err);
            //     rc = -EIO;
            //     goto exit;
            // }

            err = sensor_channel_get(sensor_dev, SENSOR_CHAN_ALL, &val);
            if (err) {
                LOG_WRN("Failed to get channel from device %d", err);
                // rc = -EIO;
                // goto exit;
            }
            sen_val = val.val1;
            if (config->sensor_channel == 1) {
                sen_val = val.val2;
            }
            // LOG_WRN("fader sen_val: %d", sen_val);

            if (sen_val == value) {
                break;
            }

            int16_t vel2 = 0;
            if (value > sen_val) {
                // vel2 = -(value - sen_val);
                vel2 = -abs(vel);
            } else if (value < sen_val) {
                // vel2 = (sen_val - value);
                vel2 = abs(vel);
            }
            // LOG_WRN("fader vel: %d", vel);

            if (vel2 && vel2 != vel) {
                vel = vel2 / 2;

                // enable enable pin, disable iif vel == 0
                val.val1 = (!vel) ? 0 : 1;
                val.val2 = 0;
                err = sensor_attr_set(tb6612fng_dev, SENSOR_CHAN_ALL,
                                      (enum sensor_attribute)TB6612FNG_ATTR_ENABLE, &val);
                if (err) {
                    LOG_WRN("Fail to sensor_attr_set TB6612FNG_ATTR_ENABLE");
                }

                // set velocity and inversr flag
                val.val1 = abs(vel);
                val.val2 = vel < 0;
                err = sensor_attr_set(tb6612fng_dev, SENSOR_CHAN_ALL,
                                      (enum sensor_attribute)TB6612FNG_ATTR_VELOCITY, &val);
                if (err) {
                    LOG_WRN("Fail to sensor_attr_set TB6612FNG_ATTR_VELOCITY");
                }

                // call sync to latch velocity setting to driver
                val.val1 = chan;
                val.val2 = 0;
                err = sensor_attr_set(tb6612fng_dev, SENSOR_CHAN_ALL,
                                      (enum sensor_attribute)TB6612FNG_ATTR_SYNC, &val);
                if (err) {
                    LOG_WRN("Fail to sensor_attr_set TB6612FNG_ATTR_SYNC");
                }
            }
        }

        // disable
        val.val1 = 0;
        val.val2 = 0;
        err = sensor_attr_set(tb6612fng_dev, SENSOR_CHAN_ALL,
                              (enum sensor_attribute)TB6612FNG_ATTR_ENABLE, &val);
        if (err) {
            LOG_WRN("Fail to sensor_attr_set TB6612FNG_ATTR_ENABLE");
            rc = -EIO;
            goto exit;
        }
    }
#endif /* IS_ENABLED(CONFIG_TB6612FNG) */

#if IS_ENABLED(CONFIG_DRV883X)
    if (config->motor_drv == OUTPUT_MOTORIZED_FADER_DRIVER_DRV883X) {
        struct sensor_value val = {.val1 = 0, .val2 = 0};

        const struct device *tb6612fng_dev = motor_dev;

        // int16_t p = config->proportional;
        // int16_t i = config->integral;
        // int16_t d = config->derivative;
        uint8_t vec = 96;

        uint32_t chan = config->motor_channel;
        int16_t vel = 0;

        if (value > sen_val) {
            // vel = -(value - sen_val);
            vel = -vec;
        } else if (value < sen_val) {
            // vel = (sen_val - value);
            vel = vec;
        }
        // LOG_WRN("fader vel: %d", vel);

        if (vel) {
            // enable enable pin, disable iif vel == 0
            val.val1 = (!vel) ? 0 : 1;
            val.val2 = 0;
            err = sensor_attr_set(tb6612fng_dev, SENSOR_CHAN_ALL,
                                  (enum sensor_attribute)DRV883X_ATTR_ENABLE, &val);
            if (err) {
                LOG_WRN("Fail to sensor_attr_set DRV883X_ATTR_ENABLE");
            }

            // set velocity and inversr flag
            val.val1 = abs(vel);
            val.val2 = vel < 0;
            err = sensor_attr_set(tb6612fng_dev, SENSOR_CHAN_ALL,
                                  (enum sensor_attribute)DRV883X_ATTR_VELOCITY, &val);
            if (err) {
                LOG_WRN("Fail to sensor_attr_set DRV883X_ATTR_VELOCITY");
            }

            // call sync to latch velocity setting to driver
            val.val1 = chan;
            val.val2 = 0;
            err = sensor_attr_set(tb6612fng_dev, SENSOR_CHAN_ALL,
                                  (enum sensor_attribute)DRV883X_ATTR_SYNC, &val);
            if (err) {
                LOG_WRN("Fail to sensor_attr_set DRV883X_ATTR_SYNC");
            }
        }

        while (true) {
            k_msleep(3);

            // err = sensor_sample_fetch(sensor_dev);
            // if (err) {
            //     LOG_WRN("Failed to fetch sample from device %d", err);
            //     rc = -EIO;
            //     goto exit;
            // }

            err = sensor_channel_get(sensor_dev, SENSOR_CHAN_ALL, &val);
            if (err) {
                LOG_WRN("Failed to get channel from device %d", err);
                // rc = -EIO;
                // goto exit;
            }
            sen_val = val.val1;
            if (config->sensor_channel == 1) {
                sen_val = val.val2;
            }
            // LOG_WRN("fader sen_val: %d", sen_val);

            if (sen_val == value) {
                break;
            }

            int16_t vel2 = 0;
            if (value > sen_val) {
                // vel2 = -(value - sen_val);
                vel2 = -abs(vel);
            } else if (value < sen_val) {
                // vel2 = (sen_val - value);
                vel2 = abs(vel);
            }
            // LOG_WRN("fader vel: %d", vel);

            if (vel2 && vel2 != vel) {
                vel = vel2 / 2;

                // enable enable pin, disable iif vel == 0
                val.val1 = (!vel) ? 0 : 1;
                val.val2 = 0;
                err = sensor_attr_set(tb6612fng_dev, SENSOR_CHAN_ALL,
                                      (enum sensor_attribute)DRV883X_ATTR_ENABLE, &val);
                if (err) {
                    LOG_WRN("Fail to sensor_attr_set DRV883X_ATTR_ENABLE");
                }

                // set velocity and inversr flag
                val.val1 = abs(vel);
                val.val2 = vel < 0;
                err = sensor_attr_set(tb6612fng_dev, SENSOR_CHAN_ALL,
                                      (enum sensor_attribute)DRV883X_ATTR_VELOCITY, &val);
                if (err) {
                    LOG_WRN("Fail to sensor_attr_set DRV883X_ATTR_VELOCITY");
                }

                // call sync to latch velocity setting to driver
                val.val1 = chan;
                val.val2 = 0;
                err = sensor_attr_set(tb6612fng_dev, SENSOR_CHAN_ALL,
                                      (enum sensor_attribute)DRV883X_ATTR_SYNC, &val);
                if (err) {
                    LOG_WRN("Fail to sensor_attr_set DRV883X_ATTR_SYNC");
                }
            }
        }

        // disable
        val.val1 = 0;
        val.val2 = 0;
        err = sensor_attr_set(tb6612fng_dev, SENSOR_CHAN_ALL,
                              (enum sensor_attribute)DRV883X_ATTR_ENABLE, &val);
        if (err) {
            LOG_WRN("Fail to sensor_attr_set DRV883X_ATTR_ENABLE");
            rc = -EIO;
            goto exit;
        }
    }
#endif /* IS_ENABLED(CONFIG_DRV883X) */

    val.val1 = 1;
    err = sensor_attr_set(sensor_dev, SENSOR_CHAN_ALL,
                          (enum sensor_attribute)ANALOG_INPUT_ATTR_ACTIVE, &val);
    if (err) {
        LOG_WRN("Fail to sensor_attr_set ANALOG_INPUT_ATTR_ACTIVE");
        rc = -EIO;
        goto exit;
    }

exit:
    data->busy = false;
    return rc;
}

static int output_motorized_fader_get_ready(const struct device *dev) {
    struct output_motorized_fader_data *data = dev->data;
    return !data->busy;
}

static int output_motorized_fader_init(const struct device *dev) {
    struct output_motorized_fader_data *data = dev->data;
    // const struct output_motorized_fader_config *config = dev->config;
    data->dev = dev;
    return 0;
}

static const struct output_generic_api api = {
    .set_value = output_motorized_fader_set_value,
    .get_ready = output_motorized_fader_get_ready,
};

#define ZMK_OUTPUT_INIT_PRIORITY 91

#define OHFB_INST(n)                                                                               \
    static struct output_motorized_fader_data data_##n = {                                         \
        .busy = false,                                                                             \
    };                                                                                             \
    static const struct output_motorized_fader_config config_##n = {                               \
        .sensor_dev = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, sensor_device),                         \
                                  (DEVICE_DT_GET(DT_INST_PHANDLE(n, sensor_device))), (NULL)),     \
        .sensor_channel = DT_INST_PROP(n, sensor_channel),                                         \
        .motor_dev =                                                                               \
            COND_CODE_1(DT_INST_NODE_HAS_PROP(n, motor_driver_device),                             \
                        (DEVICE_DT_GET(DT_INST_PHANDLE(n, motor_driver_device))), (NULL)),         \
        .motor_drv =                                                                               \
            DT_INST_ENUM_IDX_OR(0, motor_driver, OUTPUT_MOTORIZED_FADER_DRIVER_TB6612FNG),         \
        .motor_channel = DT_INST_PROP(n, motor_channel),                                           \
        .proportional = DT_INST_PROP(n, proportional),                                             \
        .integral = DT_INST_PROP(n, integral),                                                     \
        .derivative = DT_INST_PROP(n, derivative),                                                 \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(0, output_motorized_fader_init, DEVICE_DT_INST_GET(n), &data_##n,        \
                          &config_##n, POST_KERNEL, ZMK_OUTPUT_INIT_PRIORITY, &api);

DT_INST_FOREACH_STATUS_OKAY(OHFB_INST)

// #endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
