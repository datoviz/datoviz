/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Pointer events                                                                              */
/*************************************************************************************************/

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/common/macros.h"
#include "datoviz/math/types.h"
#include "datoviz/input/enums.h"

typedef struct DvzInputRouter DvzInputRouter;

typedef struct DvzMouseWheelEvent DvzMouseWheelEvent;
typedef struct DvzMouseDragEvent DvzMouseDragEvent;
typedef union DvzPointerEventUnion DvzPointerEventUnion;
typedef struct DvzPointerEvent DvzPointerEvent;
typedef struct DvzPointerGestureHandler DvzPointerGestureHandler;

struct DvzMouseWheelEvent
{
    vec2 dir;
};

struct DvzMouseDragEvent
{
    vec2 press_pos;
    vec2 last_pos;
    vec2 shift;
    bool is_press_valid;
};

// Gesture heuristics.
#define DVZ_MOUSE_CLICK_MAX_DELAY        0.25
#define DVZ_MOUSE_CLICK_MAX_SHIFT        5.0f
#define DVZ_MOUSE_DOUBLE_CLICK_MAX_DELAY 0.2

union DvzPointerEventUnion
{
    DvzMouseWheelEvent w;
    DvzMouseDragEvent d;
};

struct DvzPointerEvent
{
    DvzMouseEventType type;
    DvzPointerEventUnion content;
    vec2 pos;
    DvzMouseButton button;
    int mods;
    float content_scale;
    uint64_t timestamp_ns;
    void* user_data;
};

/**
 * Translate a GLFW mouse button into the Datoviz button enum.
 */
DVZ_EXPORT DvzMouseButton dvz_pointer_button_from_glfw(int button);

/**
 * Return a monotonic timestamp in nanoseconds.
 */
DVZ_EXPORT uint64_t dvz_input_timestamp_ns(void);

/**
 * Emit a normalized pointer event on the router.
 */
DVZ_EXPORT void dvz_pointer_emit_position(
    DvzInputRouter* router,
    DvzMouseEventType type,
    float raw_x,
    float raw_y,
    float window_x,
    float window_y,
    DvzMouseButton button,
    int mods,
    float content_scale,
    uint64_t timestamp_ns,
    void* user_data);

/**
 * Attach the legacy gesture interpreter to the router.
 */
DVZ_EXPORT DvzPointerGestureHandler* dvz_pointer_gesture_handler(DvzInputRouter* router);

/**
 * Destroy the gesture interpreter.
 */
DVZ_EXPORT void dvz_pointer_gesture_handler_destroy(DvzPointerGestureHandler* handler);
