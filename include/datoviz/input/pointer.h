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
    vec2 window_size;
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
 *
 * @param button GLFW mouse-button value
 * @return the matching pointer button, or `DVZ_POINTER_BUTTON_NONE` if unsupported
 */
DVZ_EXPORT DvzPointerButton dvz_pointer_button_from_glfw(int button);



/**
 * Return the current wall-clock timestamp in nanoseconds.
 *
 * @return Unix wall-clock timestamp in nanoseconds
 */
DVZ_EXPORT uint64_t dvz_input_timestamp_ns(void);



/**
 * Emit a normalized pointer event on the router.
 *
 * Raw coordinates, window dimensions, and @p content_scale are stored unchanged. The constructed
 * event borrows @p user_data and is dispatched synchronously.
 *
 * @param router target router; must not be NULL
 * @param type pointer event type
 * @param raw_x horizontal pointer position in backend window coordinates
 * @param raw_y vertical pointer position in backend window coordinates
 * @param window_width window width in the same units as @p raw_x
 * @param window_height window height in the same units as @p raw_y
 * @param button button associated with the event, or `DVZ_POINTER_BUTTON_NONE`
 * @param mods bitwise combination of keyboard modifier flags
 * @param content_scale backend content scale associated with the event
 * @param timestamp_ns event timestamp in nanoseconds, or zero if unavailable
 * @param user_data opaque pointer stored in the event; may be NULL
 */
DVZ_EXPORT void dvz_pointer_emit_position(
    DvzInputRouter* router, DvzPointerEventType type, float raw_x, float raw_y, float window_width,
    float window_height, DvzPointerButton button, int mods, float content_scale,
    uint64_t timestamp_ns, void* user_data);



/**
 * Emit a wheel event with pixel deltas.
 *
 * Pointer coordinates, window dimensions, content scale, and wheel deltas are copied unchanged.
 * Dispatch is synchronous.
 *
 * @param router target router; must not be NULL
 * @param raw_x horizontal pointer position in backend window coordinates
 * @param raw_y vertical pointer position in backend window coordinates
 * @param window_width window width in the same units as @p raw_x
 * @param window_height window height in the same units as @p raw_y
 * @param dir_x horizontal wheel delta in pixels
 * @param dir_y vertical wheel delta in pixels
 * @param mods bitwise combination of keyboard modifier flags
 * @param content_scale backend content scale associated with the event
 * @param timestamp_ns event timestamp in nanoseconds, or zero if unavailable
 * @param user_data opaque pointer stored in the event; may be NULL
 */
DVZ_EXPORT void dvz_pointer_emit_wheel(
    DvzInputRouter* router, float raw_x, float raw_y, float window_width, float window_height,
    float dir_x, float dir_y, int mods, float content_scale, uint64_t timestamp_ns,
    void* user_data);



/**
 * Attach the pointer gesture interpreter to the router.
 *
 * The interpreter listens to raw pointer events and emits gesture-derived pointer events on the
 * router's union input stream. Subscribe with `dvz_input_subscribe_event()` to receive click,
 * double-click, drag-start, drag, and drag-stop events.
 *
 * @param router router to observe; borrowed and must outlive the handler
 * @return a new owned handler, or NULL on allocation or subscription failure; destroy it before
 * destroying @p router
 */
DVZ_EXPORT DvzPointerGestureHandler* dvz_pointer_gesture_handler(DvzInputRouter* router);



/**
 * Destroy the gesture interpreter.
 *
 * This also removes its raw-pointer subscription from the borrowed router.
 *
 * @param handler owned handler to destroy; may be NULL
 */
DVZ_EXPORT void dvz_pointer_gesture_handler_destroy(DvzPointerGestureHandler* handler);



EXTERN_C_OFF
