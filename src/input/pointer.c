/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

#include <math.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_time_utils.h"
#include "datoviz/input/pointer.h"
#include "datoviz/input/router.h"

#define NULL_EVENT (DvzPointerEvent){0}

struct DvzPointerGestureHandler
{
    DvzInputRouter* router;
    DvzMouseState state;
    DvzMouseButton button;
    vec2 press_pos;
    vec2 last_pos;
    vec2 cur_pos;
    double time;
    double last_press;
    double last_click;
};

/**
 * Return the Euclidean distance between two vec2 values.
 *
 * @param a first vector
 * @param b second vector
 * @return distance between the vectors
 */
static float _vec2_distance(const vec2 a, const vec2 b)
{
    float dx = a[0] - b[0];
    float dy = a[1] - b[1];
    return (float)sqrtf(dx * dx + dy * dy);
}

/**
 * Copy the contents of one vec2 into another.
 *
 * @param src source vector
 * @param dst destination vector
 */
static void _vec2_copy(const vec2 src, vec2 dst)
{
    dst[0] = src[0];
    dst[1] = src[1];
}

/**
 * Convert a nanosecond timestamp into seconds with a fallback.
 *
 * @param fallback fallback value when timestamp is zero
 * @param timestamp nanoseconds timestamp
 * @return seconds equivalent of the timestamp
 */
static double _resolve_time(double fallback, uint64_t timestamp)
{
    if (timestamp != 0)
        return (double)timestamp / 1000000000.0;
    return fallback;
}

/**
 * Query the wall-clock time in nanoseconds.
 *
 * @return current time in nanoseconds
 */
static uint64_t _now_ns(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000000ULL + (uint64_t)tv.tv_usec * 1000ULL;
}

/**
 * Translate a GLFW-style button index to the Datoviz enum.
 */
DVZ_EXPORT DvzMouseButton dvz_pointer_button_from_glfw(int button)
{
    if (button == 0)
        return DVZ_MOUSE_BUTTON_LEFT;
    if (button == 1)
        return DVZ_MOUSE_BUTTON_RIGHT;
    if (button == 2)
        return DVZ_MOUSE_BUTTON_MIDDLE;
    return DVZ_MOUSE_BUTTON_NONE;
}


/**
 * Return a monotonic timestamp in nanoseconds.
 */
DVZ_EXPORT uint64_t dvz_input_timestamp_ns(void)
{
    return _now_ns();
}


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
    void* user_data)
{
    ANN(router);
    if (timestamp_ns == 0)
        timestamp_ns = dvz_input_timestamp_ns();
    DvzPointerEvent event = {0};
    event.type = type;
    event.pos[0] = raw_x - window_x;
    event.pos[1] = raw_y - window_y;
    event.button = button;
    event.mods = mods;
    event.content_scale = content_scale;
    event.user_data = user_data;
    event.timestamp_ns = timestamp_ns;
    dvz_input_emit_pointer(router, &event);
}


/**
 * Forward a pointer event to the union emitter.
 *
 * @param handler gesture handler that owns the router
 * @param event pointer event to re-emit
 */
static void _emit_gesture_event(DvzPointerGestureHandler* handler, const DvzPointerEvent* event)
{
    if (event->type == DVZ_MOUSE_EVENT_NONE)
        return;
    DvzInputEvent input_event = {0};
    input_event.type = DVZ_INPUT_EVENT_POINTER;
    input_event.content.pointer = *event;
    dvz_input_emit_event(handler->router, &input_event);
}


/**
 * Update the pointer state for a wheel event.
 *
 * @param handler gesture handler owning the state
 * @param event incoming wheel event
 * @return event to forward
 */
static DvzPointerEvent _after_wheel(
    DvzPointerGestureHandler* handler, const DvzPointerEvent* event)
{
    ANN(handler);
    ANN(event);
    handler->time = _resolve_time(handler->time, event->timestamp_ns);
    DvzPointerEvent ev = *event;
    if (handler->state == DVZ_MOUSE_STATE_DOUBLE_CLICK)
        handler->state = DVZ_MOUSE_STATE_RELEASE;
    return ev;
}


/**
 * Update the pointer state for a move event and report drags.
 *
 * @param handler gesture handler owning the state
 * @param event incoming move event
 * @return event to forward (drag start/move or original)
 */
static DvzPointerEvent _after_move(
    DvzPointerGestureHandler* handler, const DvzPointerEvent* event)
{
    ANN(handler);
    ANN(event);

    DvzPointerEvent ev = *event;
    handler->time = _resolve_time(handler->time, event->timestamp_ns);
    _vec2_copy(handler->cur_pos, handler->last_pos);
    _vec2_copy(event->pos, handler->cur_pos);

    float delta = _vec2_distance(handler->press_pos, handler->cur_pos);

    switch (handler->state)
    {
    case DVZ_MOUSE_STATE_RELEASE:
        break;

    case DVZ_MOUSE_STATE_PRESS:
    case DVZ_MOUSE_STATE_CLICK_PRESS:
        if (delta > DVZ_MOUSE_CLICK_MAX_SHIFT)
        {
            handler->state = DVZ_MOUSE_STATE_DRAGGING;
            ev.type = DVZ_MOUSE_EVENT_DRAG_START;
            _vec2_copy(handler->press_pos, ev.content.d.press_pos);
            ev.content.d.is_press_valid = true;
        }
        break;

    case DVZ_MOUSE_STATE_CLICK:
        if (delta > DVZ_MOUSE_CLICK_MAX_SHIFT)
        {
            handler->state = DVZ_MOUSE_STATE_RELEASE;
        }
        break;

    case DVZ_MOUSE_STATE_DOUBLE_CLICK:
        handler->state = DVZ_MOUSE_STATE_RELEASE;
        break;

    case DVZ_MOUSE_STATE_DRAGGING:
        ev.type = DVZ_MOUSE_EVENT_DRAG;
        _vec2_copy(handler->press_pos, ev.content.d.press_pos);
        _vec2_copy(handler->last_pos, ev.content.d.last_pos);
        ev.content.d.shift[0] = handler->cur_pos[0] - handler->press_pos[0];
        ev.content.d.shift[1] = handler->cur_pos[1] - handler->press_pos[1];
        ev.content.d.is_press_valid = true;
        break;

    default:
        break;
    }

    return ev;
}


/**
 * Update the pointer state for a press event, handling double-click latency.
 *
 * @param handler gesture handler owning the state
 * @param event incoming press event
 * @return event to forward or NULL_EVENT to drop
 */
static DvzPointerEvent _after_press(
    DvzPointerGestureHandler* handler, const DvzPointerEvent* event)
{
    ANN(handler);
    ANN(event);

    DvzPointerEvent ev = *event;
    handler->time = _resolve_time(handler->time, event->timestamp_ns);
    double delay = handler->time - handler->last_press;

    if (handler->state == DVZ_MOUSE_STATE_RELEASE)
        _vec2_copy(event->pos, handler->press_pos);

    handler->button = event->button;
    handler->cur_pos[0] = event->pos[0];
    handler->cur_pos[1] = event->pos[1];

    handler->last_press = handler->time;

    switch (handler->state)
    {
    case DVZ_MOUSE_STATE_PRESS:
    case DVZ_MOUSE_STATE_DRAGGING:
        return NULL_EVENT;

    case DVZ_MOUSE_STATE_RELEASE:
    case DVZ_MOUSE_STATE_DOUBLE_CLICK:
        handler->state = DVZ_MOUSE_STATE_PRESS;
        break;

    case DVZ_MOUSE_STATE_CLICK:
        handler->state =
            (delay <= DVZ_MOUSE_DOUBLE_CLICK_MAX_DELAY) ? DVZ_MOUSE_STATE_CLICK_PRESS
                                                        : DVZ_MOUSE_STATE_PRESS;
        break;

    case DVZ_MOUSE_STATE_CLICK_PRESS:
    default:
        break;
    }

    return ev;
}


/**
 * Update the pointer state for a release event and detect clicks/drags.
 *
 * @param handler gesture handler owning the state
 * @param event incoming release event
 * @return event to forward or NULL_EVENT when suppressed
 */
static DvzPointerEvent _after_release(
    DvzPointerGestureHandler* handler, const DvzPointerEvent* event)
{
    ANN(handler);
    ANN(event);

    DvzPointerEvent ev = *event;
    handler->time = _resolve_time(handler->time, event->timestamp_ns);
    double delay = handler->time - handler->last_press;
    DvzMouseState prev = handler->state;
    handler->button = DVZ_MOUSE_BUTTON_NONE;
    handler->cur_pos[0] = event->pos[0];
    handler->cur_pos[1] = event->pos[1];

    switch (prev)
    {
    case DVZ_MOUSE_STATE_RELEASE:
    case DVZ_MOUSE_STATE_CLICK:
        return NULL_EVENT;

    case DVZ_MOUSE_STATE_DOUBLE_CLICK:
        handler->state = DVZ_MOUSE_STATE_RELEASE;
        break;

    case DVZ_MOUSE_STATE_DRAGGING:
        handler->state = DVZ_MOUSE_STATE_RELEASE;
        ev.type = DVZ_MOUSE_EVENT_DRAG_STOP;
        _vec2_copy(handler->press_pos, ev.content.d.press_pos);
        _vec2_copy(handler->last_pos, ev.content.d.last_pos);
        ev.content.d.is_press_valid = true;
        break;

    case DVZ_MOUSE_STATE_PRESS:
    case DVZ_MOUSE_STATE_CLICK_PRESS:
        if (delay <= DVZ_MOUSE_CLICK_MAX_DELAY)
        {
            handler->state = DVZ_MOUSE_STATE_CLICK;
            ev.type = (prev == DVZ_MOUSE_STATE_CLICK_PRESS)
                          ? DVZ_MOUSE_EVENT_DOUBLE_CLICK
                          : DVZ_MOUSE_EVENT_CLICK;
            handler->last_click = handler->time;
        }
        else
        {
            handler->state = DVZ_MOUSE_STATE_RELEASE;
        }
        break;

    default:
        break;
    }

    return ev;
}


/**
 * Router callback that drives the gesture interpreter.
 *
 * @param router input router instance
 * @param event pointer event from the backend
 * @param user_data gesture handler instance
 */
static void _pointer_router_callback(
    DvzInputRouter* router, const DvzPointerEvent* event, void* user_data)
{
    ANN(router);
    ANN(event);
    ANN(user_data);

    DvzPointerGestureHandler* handler = (DvzPointerGestureHandler*)user_data;
    DvzPointerEvent derived = NULL_EVENT;

    switch (event->type)
    {
    case DVZ_MOUSE_EVENT_PRESS:
        derived = _after_press(handler, event);
        break;

    case DVZ_MOUSE_EVENT_MOVE:
        derived = _after_move(handler, event);
        break;

    case DVZ_MOUSE_EVENT_RELEASE:
        derived = _after_release(handler, event);
        break;

    case DVZ_MOUSE_EVENT_WHEEL:
        derived = _after_wheel(handler, event);
        break;

    default:
        derived = *event;
        break;
    }

    _emit_gesture_event(handler, &derived);
}


/**
 * Attach the legacy gesture interpreter to the router.
 */
DVZ_EXPORT DvzPointerGestureHandler* dvz_pointer_gesture_handler(DvzInputRouter* router)
{
    ANN(router);
    DvzPointerGestureHandler* handler =
        (DvzPointerGestureHandler*)dvz_calloc(1, sizeof(DvzPointerGestureHandler));
    handler->router = router;
    handler->state = DVZ_MOUSE_STATE_RELEASE;
    dvz_input_subscribe_pointer(router, _pointer_router_callback, handler);
    return handler;
}


/**
 * Destroy the gesture interpreter.
 */
DVZ_EXPORT void dvz_pointer_gesture_handler_destroy(DvzPointerGestureHandler* handler)
{
    if (handler == NULL)
        return;
    if (handler->router != NULL)
        dvz_input_unsubscribe_pointer(handler->router, _pointer_router_callback, handler);
    dvz_free(handler);
}
