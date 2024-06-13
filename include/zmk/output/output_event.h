/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zmk/event_manager.h>

enum output_source {
    OUTPUT_SOURCE__MIN__ = 0,
    OUTPUT_SOURCE_LAYER_STATE_CHANGE,
    OUTPUT_SOURCE_POSITION_STATE_CHANGE,
    OUTPUT_SOURCE_KEYCODE_STATE_CHANGE,
    OUTPUT_SOURCE_MOUSE_BUTTON_STATE_CHANGE,
    OUTPUT_SOURCE_MOUSE_WHEEL_STATE_CHANGE,
    OUTPUT_SOURCE_TRANSPORT,
    OUTPUT_SOURCE__MAX__,
};

struct zmk_output_event {
    enum output_source source;
    uint8_t layer;
    uint32_t position;
    bool state;
    uint8_t force;
    uint8_t duration;
    uint64_t timestamp;
};

ZMK_EVENT_DECLARE(zmk_output_event);
