/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_output_split_output_relay

#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#if IS_ENABLED(CONFIG_ZMK_SPLT_PERIPHERAL_OUTPUT_RELAY)
#include <zmk/split/output-relay/event.h>
#endif

// #if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/output/output_split_output_relay.h>

static int output_split_output_relay_enable(const struct device *dev, uint8_t force) {
    struct output_split_output_relay_data *data = dev->data;
    const struct output_split_output_relay_config *config = dev->config;

    if (data->busy) {
        LOG_WRN("split output relay is busy");
        return -EBUSY;
    }
    data->busy = true;

    int rc = 0;

#if IS_ENABLED(CONFIG_ZMK_SPLT_PERIPHERAL_OUTPUT_RELAY)
    struct zmk_split_bt_output_relay_event ev = { .state = true, .force = force };
    zmk_split_bt_invoke_output(dev, ev);
#endif /* IS_ENABLED(CONFIG_ZMK_SPLT_PERIPHERAL_OUTPUT_RELAY) */

exit:
    data->busy = false;
    return rc;
}

static int output_split_output_relay_disable(const struct device *dev) {
    struct output_split_output_relay_data *data = dev->data;
    const struct output_split_output_relay_config *config = dev->config;

    if (!data->busy) {
        LOG_WRN("split output relay is busy");
        return -EBUSY;
    }
    data->busy = true;
    
    int rc = 0;

#if IS_ENABLED(CONFIG_ZMK_SPLT_PERIPHERAL_OUTPUT_RELAY)
    struct zmk_split_bt_output_relay_event ev = { .state = false, .force = 0 };
    zmk_split_bt_invoke_output(dev, ev);
#endif /* IS_ENABLED(CONFIG_ZMK_SPLT_PERIPHERAL_OUTPUT_RELAY) */

exit:
    data->busy = false;
    return rc;
}

static int output_split_output_relay_get(const struct device *dev) {
    struct output_split_output_relay_data *data = dev->data;
    return !data->busy;
}

static int output_split_output_relay_init(const struct device *dev) {
    struct output_split_output_relay_data *data = dev->data;
    // const struct output_split_output_relay_config *config = dev->config;
    data->dev = dev;
    return 0;
}

static const struct output_generic_api api = {
    .enable = output_split_output_relay_enable,
    .disable = output_split_output_relay_disable,
    .get = output_split_output_relay_get,
};

#define ZMK_OUTPUT_INIT_PRIORITY 91

#define OSOR_INST(n)                                                                        \
    static struct output_split_output_relay_data data_##n = { .busy = false, };             \
    static const struct output_split_output_relay_config config_##n = {                     \
    };                                                                                      \
    DEVICE_DT_INST_DEFINE(0, output_split_output_relay_init, DEVICE_DT_INST_GET(n),         \
                          &data_##n, &config_##n,                                           \
                          POST_KERNEL, ZMK_OUTPUT_INIT_PRIORITY, &api);

DT_INST_FOREACH_STATUS_OKAY(OSOR_INST)

// #endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
