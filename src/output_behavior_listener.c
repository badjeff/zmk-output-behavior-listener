/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_output_behavior_listener

#include <drivers/behavior.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/input/input.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// #if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#include <zmk/endpoints.h>
#include <zmk/hid.h>
#include <zmk/keymap.h>
#include <zmk/behavior.h>
#include <zmk/event_manager.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/events/mouse_button_state_changed.h>

#include <zmk/output/output_event.h>
#include <zmk/output/output_haptic_feedback.h>

struct output_behavior_listener_config {
    uint8_t sources_count;
    enum output_source sources[OUTPUT_SOURCE__MAX__];
    bool has_position;
    uint32_t position;
    bool invert_state;
    bool all_state;
    uint32_t tap_ms;
    uint8_t layers_count;
    uint8_t layers[ZMK_KEYMAP_LAYERS_LEN];
    uint8_t bindings_count;
    struct zmk_behavior_binding bindings[];
};

struct output_behavior_listener_data {
    struct output_behavior_listener_config *cfg;
    struct k_work_delayable tap_release_work;
    uint8_t tap_release_layer;
    uint8_t tap_release_binding_idx;
    struct zmk_output_event *tap_release_output_event;
};

#define OBL_EXTRACT_BINDING(idx, drv_inst)                                                         \
    {                                                                                              \
        .behavior_dev = DEVICE_DT_NAME(DT_INST_PHANDLE_BY_IDX(drv_inst, bindings, idx)),           \
        .param1 = COND_CODE_0(DT_INST_PHA_HAS_CELL_AT_IDX(drv_inst, bindings, idx, param1), (0),   \
                              (DT_INST_PHA_BY_IDX(drv_inst, bindings, idx, param1))),              \
        .param2 = COND_CODE_0(DT_INST_PHA_HAS_CELL_AT_IDX(drv_inst, bindings, idx, param2), (0),   \
                              (DT_INST_PHA_BY_IDX(drv_inst, bindings, idx, param2))),              \
    }

#define OBL_INST(n)                                                                                \
    static struct output_behavior_listener_data data_##n = {};                                     \
    static const struct output_behavior_listener_config config_##n = {                             \
        .sources_count = DT_INST_PROP_LEN(n, sources),                                             \
        .sources = DT_INST_PROP(n, sources),                                                       \
        .has_position = DT_INST_NODE_HAS_PROP(n, position),                                        \
        .position = COND_CODE_1(                                                                   \
            DT_INST_NODE_HAS_PROP(n, position),                                                    \
            (DT_INST_PROP(n, position)), (0)),                                                     \
        .invert_state = DT_INST_PROP(n, invert_state),                                             \
        .all_state = DT_INST_PROP(n, all_state),                                                   \
        .tap_ms = DT_INST_PROP(n, tap_ms),                                                         \
        .layers_count = DT_INST_PROP_LEN(n, layers),                                               \
        .layers = DT_INST_PROP(n, layers),                                                         \
        .bindings_count = COND_CODE_1(                                                             \
            DT_INST_NODE_HAS_PROP(n, bindings),                                                    \
            (DT_INST_PROP_LEN(n, bindings)), (0)),                                                 \
        .bindings = COND_CODE_1(                                                                   \
            DT_INST_NODE_HAS_PROP(n, bindings),                                                    \
            ({LISTIFY(DT_INST_PROP_LEN(n, bindings), OBL_EXTRACT_BINDING, (, ), n)}),              \
            ({})),                                                                                 \
    };

DT_INST_FOREACH_STATUS_OKAY(OBL_INST)

static void ob_behavior_listener_tap_cb(struct k_work *work) {
    struct k_work_delayable *work_delayable = (struct k_work_delayable *)work;
    struct output_behavior_listener_data *data = CONTAINER_OF(work_delayable, 
                                                              struct output_behavior_listener_data,
                                                              tap_release_work);
    LOG_DBG("");
    const struct output_behavior_listener_config *cfg = data->cfg;
    struct zmk_behavior_binding binding = cfg->bindings[data->tap_release_binding_idx];
    const struct device *behavior = zmk_behavior_get_binding(binding.behavior_dev);
    if (!behavior) {
        LOG_WRN("No output behavior assigned");
        return;
    }

    const struct behavior_driver_api *api = (const struct behavior_driver_api *)behavior->api;
    if (!api->binding_released) {
        LOG_WRN("Binding behavior does not has binding_released");
        return;
    }

    uint8_t layer = data->tap_release_layer;
    struct zmk_output_event *evt = data->tap_release_output_event;
    struct zmk_behavior_binding_event event = {
        .layer = layer, .timestamp = k_uptime_get(),
        .position = (struct zmk_output_event *)evt, // util uint32_t to pass event ptr :)
    };
    LOG_DBG("call binding_released");
    api->binding_released(&binding, event);

    // clear occupying output event
    data->tap_release_output_event = NULL;
}

static bool intercept_with_output_config(const struct output_behavior_listener_config *cfg,
                                         struct output_behavior_listener_data *data,
                                         struct zmk_output_event *evt) {

    uint8_t source = evt->source;
    bool active_source_check = false;
    for (uint8_t i = 0; i < cfg->sources_count && !active_source_check; i++) {
        active_source_check |= source == cfg->sources[i];
    }
    if (!active_source_check) {
        return false;
    }

    uint8_t layer = zmk_keymap_highest_layer_active();
    bool active_layer_check = false;
    for (uint8_t i = 0; i < cfg->layers_count && !active_layer_check; i++) {
        active_layer_check |= layer == cfg->layers[i];
    }
    if (!active_layer_check) {
        return false;
    }

    bool cfg_state = cfg->invert_state ? false : true;
    bool state_is_matched_cfg = evt->state == cfg_state;

    if (evt->source == OUTPUT_SOURCE_LAYER_STATE_CHANGE
    ||  evt->source == OUTPUT_SOURCE_POSITION_STATE_CHANGE
    ||  evt->source == OUTPUT_SOURCE_KEYCODE_STATE_CHANGE
    ||  evt->source == OUTPUT_SOURCE_MOUSE_BUTTON_STATE_CHANGE
    ) {
        if (evt->state != cfg_state && !cfg->all_state) {
            return false;
        }
        if (cfg->has_position && evt->position != cfg->position) {
            return false;
        }
    }

    bool intercepted = false;

    for (uint8_t b = 0; b < cfg->bindings_count; b++) {
        struct zmk_behavior_binding binding = cfg->bindings[b];

        LOG_DBG("layer: %d binding name: %s", layer, binding.behavior_dev);

        const struct device *behavior = zmk_behavior_get_binding(binding.behavior_dev);
        if (!behavior) {
            LOG_WRN("No output behavior assigned on layer %d", layer);
            continue;
        }

        const struct behavior_driver_api *api = (const struct behavior_driver_api *)behavior->api;
        int ret = ZMK_BEHAVIOR_TRANSPARENT;

        if (api->binding_pressed || api->binding_released) {

            struct zmk_behavior_binding_event event = {
                .layer = layer, .timestamp = k_uptime_get(),
                .position = (struct zmk_output_event *)evt, // util uint32_t to pass event ptr :)
            };

            if (api->binding_pressed && state_is_matched_cfg) {
                ret = api->binding_pressed(&binding, event);
                if (api->binding_released && cfg->tap_ms >= 0) {
                    if (!data->cfg) {
                        data->cfg = cfg;
                        k_work_init_delayable(&data->tap_release_work, ob_behavior_listener_tap_cb);
                    }
                    // bypassing tap-ms while some tap-release is ongoing to be happened
                    if (!data->tap_release_output_event) {
                        data->tap_release_output_event = evt;
                        // LOG_DBG("sche tap-ms work %d", cfg->tap_ms);
                        data->tap_release_layer = layer;
                        data->tap_release_binding_idx = b;
                        k_work_schedule(&data->tap_release_work, K_MSEC(cfg->tap_ms));
                    }
                }
            }
            else if (api->binding_released && !evt->state) {
                ret = api->binding_released(&binding, event);
            }

        }
        else if (api->sensor_binding_process) {

            struct zmk_behavior_binding_event event = {
                .layer = layer, .timestamp = k_uptime_get(),
                .position = 0,
            };
            if (api->sensor_binding_accept_data) {
                const struct zmk_sensor_config *sensor_config = 
                    (const struct zmk_sensor_config *)cfg;
                const struct zmk_sensor_channel_data val[] = {
                    { .value = { .val1 = (struct zmk_output_event *)evt },
                    .channel = SENSOR_CHAN_ALL, },
                };
                int ret = behavior_sensor_keymap_binding_accept_data(
                    &binding, event, sensor_config, sizeof(val), val);
                if (ret < 0) {
                    LOG_WRN("behavior data accept for behavior %s returned an error (%d). "
                            "Processing to continue to next layer",  binding.behavior_dev, ret);
                }
            }
            enum behavior_sensor_binding_process_mode mode =
                    BEHAVIOR_SENSOR_BINDING_PROCESS_MODE_TRIGGER;
            ret = behavior_sensor_keymap_binding_process(&binding, event, mode);

        }

        if (ret == ZMK_BEHAVIOR_OPAQUE) {
            // LOG_DBG("output event processing complete, behavior response was opaque");
            intercepted = true;
            break;
        } else if (ret < 0) {
            // LOG_DBG("onput behavior returned error: %d", ret);
            return ret;
        }
    }

    return intercepted;
}

static int zmk_output_event_triggered(struct zmk_output_event *ev) {
    bool intercepted = false;

    #define EXEC__OUTPUT_BEHAVIOR_LISTENER(n)                                            \
        if (!intercepted) {                                                              \
            intercepted = intercept_with_output_config(&config_##n, &data_##n, ev);      \
        }

    DT_INST_FOREACH_STATUS_OKAY(EXEC__OUTPUT_BEHAVIOR_LISTENER)

    return 0;
}

static int output_event_listener(const zmk_event_t *ev) {
    struct zmk_output_event *out_ev;
    if ((out_ev = as_zmk_output_event(ev)) != NULL) {
        int ret = zmk_output_event_triggered(out_ev);
        return ret;
    }

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)

    struct zmk_output_event e = (struct zmk_output_event){
        .layer = zmk_keymap_highest_layer_active(),
        .force = CONFIG_ZMK_OUTPUT_DEFAULT_FORCE,
        .value = CONFIG_ZMK_OUTPUT_DEFAULT_DURATION,
        .timestamp = k_uptime_get()
    };

    const struct zmk_layer_state_changed *lay_ev;
    if ((lay_ev = as_zmk_layer_state_changed(ev)) != NULL) {
        e.source = OUTPUT_SOURCE_LAYER_STATE_CHANGE;
        e.position = lay_ev->layer;
        e.state = lay_ev->state;
        zmk_output_event_triggered(&e); // raise_zmk_output_event(e);
    }

    const struct zmk_position_state_changed *pos_ev;
    if ((pos_ev = as_zmk_position_state_changed(ev)) != NULL) {
        e.source = OUTPUT_SOURCE_POSITION_STATE_CHANGE;
        e.position = pos_ev->position;
        e.state = pos_ev->state;
        zmk_output_event_triggered(&e); // raise_zmk_output_event(e);
    }

    const struct zmk_keycode_state_changed *kc_ev;
    if ((kc_ev = as_zmk_keycode_state_changed(ev)) != NULL) {
        e.source = OUTPUT_SOURCE_KEYCODE_STATE_CHANGE;
        e.position = kc_ev->keycode;
        e.state = kc_ev->state;
        zmk_output_event_triggered(&e); // raise_zmk_output_event(e);
    }
#endif /* IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL) */

    return 0;
}

ZMK_LISTENER(output_event_listener, output_event_listener);
ZMK_SUBSCRIPTION(output_event_listener, zmk_output_event);

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)

ZMK_SUBSCRIPTION(output_event_listener, zmk_layer_state_changed);
ZMK_SUBSCRIPTION(output_event_listener, zmk_position_state_changed);
ZMK_SUBSCRIPTION(output_event_listener, zmk_keycode_state_changed);

#if IS_ENABLED(CONFIG_INPUT)

void ouput_input_handler(struct input_event *evt) {
    switch (evt->type) {
    case INPUT_EV_KEY:
        switch (evt->code) {
        case INPUT_BTN_0:
        case INPUT_BTN_1:
        case INPUT_BTN_2:
        case INPUT_BTN_3:
        case INPUT_BTN_4:
            struct zmk_output_event e = (struct zmk_output_event){
                .layer = zmk_keymap_highest_layer_active(),
                .force = CONFIG_ZMK_OUTPUT_DEFAULT_FORCE,
                .value = CONFIG_ZMK_OUTPUT_DEFAULT_DURATION,
                .timestamp = k_uptime_get()
            };
            e.source = OUTPUT_SOURCE_MOUSE_BUTTON_STATE_CHANGE;
            e.position = 1 + evt->code - INPUT_BTN_0;
            e.state = evt->value > 0;
            // LOG_WRN("mouse button: %d state: %d", e.position, e.state ? 1 : 0);
            zmk_output_event_triggered(&e);
            break;
        default:
            break;
        }
        break;
    case INPUT_EV_REL:
        switch (evt->code) {
        case INPUT_REL_WHEEL:
            struct zmk_output_event e = (struct zmk_output_event){
                .layer = zmk_keymap_highest_layer_active(),
                .force = CONFIG_ZMK_OUTPUT_DEFAULT_FORCE,
                .value = CONFIG_ZMK_OUTPUT_DEFAULT_DURATION,
                .timestamp = k_uptime_get()
            };
            e.source = OUTPUT_SOURCE_MOUSE_WHEEL_STATE_CHANGE;
            e.position = evt->value;
            e.state = evt->value != 0;
            // LOG_WRN("mouse wheel: %d state: %d", e.position, e.state ? 1 : 0);
            zmk_output_event_triggered(&e);
            break;
        default:
            break;
        }
        break;
    }
}

INPUT_CALLBACK_DEFINE(NULL, ouput_input_handler);

#endif /* IS_ENABLED(CONFIG_INPUT) */

#endif /* IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL) */

// #endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
