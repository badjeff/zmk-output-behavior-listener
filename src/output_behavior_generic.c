/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_output_behavior_generic

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <drivers/behavior.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/keymap.h>
#include <zmk/behavior.h>

#include <zmk/output/output_generic.h>

// #if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct output_behavior_geenric_config {
    const struct device *output_dev;
    uint32_t delay;
    uint32_t time_to_live_ms;
    bool toggle;
    int16_t force;
};

struct output_behavior_geenric_data {
    bool active;
    struct k_work_delayable activate_work;
    struct k_work_delayable deactivate_work;
    const struct device *dev;
};

static void ob_geenric_set_output_enable(struct output_behavior_geenric_data *data) {
    const struct output_behavior_geenric_config *cfg = data->dev->config;

    const struct device *output_dev = cfg->output_dev;
    if (!output_dev) {
        LOG_WRN("No output device assigned");
        return;
    }

    const struct output_generic_api *api = (const struct output_generic_api *)output_dev->api;
    if (api->enable == NULL) {
        LOG_WRN("No enable() api assigned on device %s", cfg->output_dev->name);
        return;
    }

    if (data->active) {
        api->enable(output_dev, cfg->force);
    } else {
        api->disable(output_dev);
    }
}

static void ob_generic_deactivate_cb(struct k_work *work) {
    struct k_work_delayable *work_delayable = (struct k_work_delayable *)work;
    struct output_behavior_geenric_data *data = CONTAINER_OF(work_delayable, 
                                                             struct output_behavior_geenric_data,
                                                             deactivate_work);
    // const struct output_behavior_geenric_config *cfg = data->dev->config;
    if (!data->active) {
        return;
    }
    LOG_DBG("deactivate");
    data->active = false;
    ob_geenric_set_output_enable(data);
}

static void ob_generic_activate_cb(struct k_work *work) {
    struct k_work_delayable *work_delayable = (struct k_work_delayable *)work;
    struct output_behavior_geenric_data *data = CONTAINER_OF(work_delayable, 
                                                             struct output_behavior_geenric_data,
                                                             activate_work);
    const struct output_behavior_geenric_config *cfg = data->dev->config;
    if (data->active) {
        return;
    }
    if (!data->active) {
        LOG_DBG("actvate");
        data->active = true;
        ob_geenric_set_output_enable(data);
    }
    if (cfg->toggle) {
        return;
    }
    if (cfg->time_to_live_ms) {
        LOG_DBG("sche ttl work %d", cfg->time_to_live_ms);
        k_work_schedule(&data->deactivate_work, K_MSEC(cfg->time_to_live_ms));
    }
}

static int ob_generic_binding_pressed(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    struct output_behavior_geenric_data *data = (struct output_behavior_geenric_data *)dev->data;
    const struct output_behavior_geenric_config *cfg = dev->config;
    if (cfg->toggle) {
        LOG_WRN("toggle");
        struct k_work_delayable *work = data->active ? &data->deactivate_work : &data->activate_work;
        k_work_schedule(work, K_MSEC(cfg->delay));
    } else {
        k_work_schedule(&data->activate_work, K_MSEC(cfg->delay));
    }
    return ZMK_BEHAVIOR_TRANSPARENT;
}

static int output_behavior_to_init(const struct device *dev) {
    struct output_behavior_geenric_data *data = dev->data;

    data->dev = dev;
    k_work_init_delayable(&data->activate_work, ob_generic_activate_cb);
    k_work_init_delayable(&data->deactivate_work, ob_generic_deactivate_cb);

    return 0;
};

static const struct behavior_driver_api output_behavior_geenric_driver_api = {
    .binding_pressed = ob_generic_binding_pressed,
};

#define ZMK_OUTPUT_INIT_PRIORITY 81

#define KP_INST(n)                                                                                 \
    static struct output_behavior_geenric_data output_behavior_geenric_data_##n = {};              \
    static struct output_behavior_geenric_config output_behavior_geenric_config_##n = {            \
        .output_dev = DEVICE_DT_GET(DT_INST_PHANDLE(n, device)),                                   \
        .delay = DT_INST_PROP(n, delay),                                                           \
        .time_to_live_ms = DT_INST_PROP(n, time_to_live_ms),                                       \
        .toggle = DT_INST_PROP(n, toggle),                                                         \
        .force = DT_INST_PROP(n, force),                                                           \
    };                                                                                             \
    BEHAVIOR_DT_INST_DEFINE(n, output_behavior_to_init, NULL,                                      \
                            &output_behavior_geenric_data_##n,                                     \
                            &output_behavior_geenric_config_##n,                                   \
                            POST_KERNEL, ZMK_OUTPUT_INIT_PRIORITY,                                 \
                            &output_behavior_geenric_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KP_INST)

// #endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
