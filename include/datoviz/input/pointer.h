/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Pointer events                                                                               */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/common/macros.h"
#include "datoviz/input/enums.h"
#include "datoviz/math/types.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

// Gesture heuristics.
#define DVZ_POINTER_CLICK_MAX_DELAY        0.25
#define DVZ_POINTER_CLICK_MAX_SHIFT        5.0f
#define DVZ_POINTER_DOUBLE_CLICK_MAX_DELAY 0.2



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzInputRouter DvzInputRouter;

typedef struct DvzPointerWheelEvent DvzPointerWheelEvent;
typedef struct DvzPointerDragEvent DvzPointerDragEvent;
typedef union DvzPointerEventUnion DvzPointerEventUnion;
typedef struct DvzPointerEvent DvzPointerEvent;
typedef struct DvzPointerGestureHandler DvzPointerGestureHandler;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzPointerWheelEvent
{
    vec2 dir;
};



struct DvzPointerDragEvent
{
    vec2 press_pos;
    vec2 last_pos;
    vec2 shift;
    bool is_press_valid;
};



union DvzPointerEventUnion
{
    DvzPointerWheelEvent w;
    DvzPointerDragEvent d;
};



struct DvzPointerEvent
{
    DvzPointerEventType type;
    DvzPointerEventUnion content;
    vec2 pos;
    DvzPointerButton button;
    int mods;
    float content_scale;
    uint64_t timestamp_ns;
    void* user_data;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Translate a backend mouse button identifier into the Datoviz pointer button enum.
 */
DVZ_EXPORT DvzPointerButton dvz_pointer_button_from_glfw(int button);



/**
 * Return a monotonic timestamp in nanoseconds.
 */
DVZ_EXPORT uint64_t dvz_input_timestamp_ns(void);



/**
 * Emit a normalized pointer event on the router.
 */
DVZ_EXPORT void dvz_pointer_emit_position(
    DvzInputRouter* router, DvzPointerEventType type, float raw_x, float raw_y, float window_x,
    float window_y, DvzPointerButton button, int mods, float content_scale, uint64_t timestamp_ns,
    void* user_data);



/**
 * Emit a wheel event with pixel deltas.
 */
DVZ_EXPORT void dvz_pointer_emit_wheel(
    DvzInputRouter* router, float raw_x, float raw_y, float window_x, float window_y, float dir_x,
    float dir_y, int mods, float content_scale, uint64_t timestamp_ns, void* user_data);



/**
 * Attach the legacy gesture interpreter to the router.
 */
DVZ_EXPORT DvzPointerGestureHandler* dvz_pointer_gesture_handler(DvzInputRouter* router);



/**
 * Destroy the gesture interpreter.
 */
DVZ_EXPORT void dvz_pointer_gesture_handler_destroy(DvzPointerGestureHandler* handler);



EXTERN_C_OFF
