/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing input                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <string.h>

#include "_assertions.h"
#include "datoviz/input.h"
#include "test_input.h"
#include "testing.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct
{
    DvzPointerEventType last_type;
    uint32_t count;
    DvzPointerEventType history[16];
} EventRecorder;



typedef struct
{
    uint32_t unsubscribe_calls;
    uint32_t follower_calls;
} DispatchRecorder;



typedef struct
{
    float last_dir[2];
    uint32_t wheel_count;
} WheelRecorder;



typedef struct
{
    DvzInputResizeEvent resize;
    DvzInputScaleEvent scale;
    uint32_t resize_calls;
    uint32_t scale_calls;
    uint32_t union_resize_calls;
    uint32_t union_scale_calls;
} WindowEventRecorder;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Create a pointer event template.
 *
 * @param type event type to set
 * @param x x position
 * @param y y position
 * @param button mouse button
 * @param timestamp timestamp value
 * @return constructed pointer event
 */
static DvzPointerEvent
_make_event(DvzPointerEventType type, float x, float y, DvzPointerButton button, uint64_t timestamp)
{
    DvzPointerEvent event = {0};
    event.type = type;
    event.pos[0] = x;
    event.pos[1] = y;
    event.button = button;
    event.content_scale = 1.0f;
    event.timestamp_ns = timestamp;
    return event;
}



/**
 * Validate that the first pointer callback runs first.
 */
static void
_router_callback_one(DvzInputRouter* router, const DvzPointerEvent* event, void* user_data)
{
    ANN(router);
    ANN(event);
    int* state = user_data;
    if (*state != 0)
    {
        *state = -1;
        return;
    }
    *state = 1;
}



/**
 * Validate that the second pointer callback sees the first callback run.
 */
static void
_router_callback_two(DvzInputRouter* router, const DvzPointerEvent* event, void* user_data)
{
    ANN(router);
    ANN(event);
    int* state = user_data;
    if (*state != 1)
    {
        *state = -1;
        return;
    }
    *state = 2;
}



/**
 * Record pointer events emitted through the union callbacks.
 */
static void _record_event(DvzInputRouter* router, const DvzInputEvent* event, void* user_data)
{
    ANN(router);
    ANN(event);
    EventRecorder* recorder = user_data;
    if (event->type != DVZ_INPUT_EVENT_POINTER)
        return;
    recorder->last_type = event->content.pointer.type;
    if (recorder->count < sizeof(recorder->history) / sizeof(recorder->history[0]))
        recorder->history[recorder->count++] = recorder->last_type;
}



/**
 * Reset an event recorder.
 */
static void _recorder_reset(EventRecorder* recorder)
{
    ANN(recorder);
    memset(recorder, 0, sizeof(*recorder));
    recorder->last_type = DVZ_POINTER_EVENT_NONE;
}



/**
 * Check whether an event recorder captured a given type.
 */
static bool _recorder_contains(const EventRecorder* recorder, DvzPointerEventType type)
{
    ANN(recorder);
    for (uint32_t i = 0; i < recorder->count; i++)
    {
        if (recorder->history[i] == type)
            return true;
    }
    return false;
}



/**
 * Pointer callback from which we unsubscribe immediately.
 */
static void
_unsubscribe_pointer(DvzInputRouter* router, const DvzPointerEvent* event, void* user_data)
{
    ANN(router);
    ANN(event);
    ANN(user_data);
    DispatchRecorder* recorder = user_data;
    recorder->unsubscribe_calls++;
    dvz_input_unsubscribe_pointer(router, _unsubscribe_pointer, user_data);
}



/**
 * Pointer callback used to ensure we keep dispatching after an unsubscribe.
 */
static void
_follower_pointer(DvzInputRouter* router, const DvzPointerEvent* event, void* user_data)
{
    ANN(router);
    ANN(event);
    ANN(user_data);
    DispatchRecorder* recorder = user_data;
    recorder->follower_calls++;
}



/**
 * Capture wheel payloads.
 */
static void _record_wheel(DvzInputRouter* router, const DvzPointerEvent* event, void* user_data)
{
    ANN(router);
    ANN(event);
    ANN(user_data);
    WheelRecorder* recorder = user_data;
    if (event->type != DVZ_POINTER_EVENT_WHEEL)
        return;
    recorder->last_dir[0] = event->content.w.dir[0];
    recorder->last_dir[1] = event->content.w.dir[1];
    recorder->wheel_count++;
}



/**
 * Capture resize events.
 */
static void
_record_resize(DvzInputRouter* router, const DvzInputResizeEvent* event, void* user_data)
{
    ANN(router);
    ANN(event);
    ANN(user_data);
    WindowEventRecorder* recorder = user_data;
    recorder->resize_calls++;
    recorder->resize = *event;
}



/**
 * Capture scale events.
 */
static void _record_scale(DvzInputRouter* router, const DvzInputScaleEvent* event, void* user_data)
{
    ANN(router);
    ANN(event);
    ANN(user_data);
    WindowEventRecorder* recorder = user_data;
    recorder->scale_calls++;
    recorder->scale = *event;
}



/**
 * Record window events via the union API.
 */
static void
_record_window_union(DvzInputRouter* router, const DvzInputEvent* event, void* user_data)
{
    ANN(router);
    ANN(event);
    ANN(user_data);
    WindowEventRecorder* recorder = user_data;
    if (event->type == DVZ_INPUT_EVENT_RESIZE)
    {
        recorder->union_resize_calls++;
        recorder->resize = event->content.resize;
    }
    else if (event->type == DVZ_INPUT_EVENT_SCALE)
    {
        recorder->union_scale_calls++;
        recorder->scale = event->content.scale;
    }
}



/*************************************************************************************************/
/*  Test functions                                                                               */
/*************************************************************************************************/

/**
 * Ensure pointer subscriptions respect insertion order.
 */
int test_router_callbacks(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    DvzInputRouter* router = dvz_input_router();
    int state = 0;
    dvz_input_subscribe_pointer(router, _router_callback_one, &state);
    dvz_input_subscribe_pointer(router, _router_callback_two, &state);
    DvzPointerEvent event =
        _make_event(DVZ_POINTER_EVENT_PRESS, 10.0f, 5.0f, DVZ_POINTER_BUTTON_LEFT, 1);
    dvz_input_emit_pointer(router, &event);
    AT(state == 2);
    dvz_input_router_destroy(router);
    return 0;
}



/**
 * Ensure unsubscribing from inside a callback does not stop dispatch.
 */
int test_router_unsubscribe(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    DvzInputRouter* router = dvz_input_router();
    DispatchRecorder recorder = {0};
    dvz_input_subscribe_pointer(router, _unsubscribe_pointer, &recorder);
    dvz_input_subscribe_pointer(router, _follower_pointer, &recorder);
    DvzPointerEvent event =
        _make_event(DVZ_POINTER_EVENT_PRESS, 0.0f, 0.0f, DVZ_POINTER_BUTTON_LEFT, 1);
    dvz_input_emit_pointer(router, &event);
    AT(recorder.unsubscribe_calls == 1);
    AT(recorder.follower_calls == 1);
    dvz_input_router_destroy(router);
    return 0;
}



/**
 * Verify modifier bit tracking works for shift.
 */
int test_keyboard_modifiers(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    DvzKeyboardModifierState* state = dvz_keyboard_modifier_state();
    dvz_keyboard_modifier_state_update(state, DVZ_KEYBOARD_EVENT_PRESS, DVZ_KEY_LEFT_SHIFT);
    AT(dvz_keyboard_modifier_state_mods(state) == DVZ_KEY_MODIFIER_SHIFT);
    dvz_keyboard_modifier_state_update(state, DVZ_KEYBOARD_EVENT_RELEASE, DVZ_KEY_LEFT_SHIFT);
    AT(dvz_keyboard_modifier_state_mods(state) == 0);
    dvz_keyboard_modifier_state_destroy(state);
    return 0;
}



/**
 * Confirm gesture detection emits clicks, double-clicks, and drags.
 */
int test_pointer_gestures(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    DvzInputRouter* router = dvz_input_router();
    DvzPointerGestureHandler* gestures = dvz_pointer_gesture_handler(router);
    EventRecorder recorder = {0};
    dvz_input_subscribe_event(router, _record_event, &recorder);

    uint64_t now = dvz_input_timestamp_ns();
    DvzPointerEvent press =
        _make_event(DVZ_POINTER_EVENT_PRESS, 10.0f, 10.0f, DVZ_POINTER_BUTTON_LEFT, now);
    _recorder_reset(&recorder);
    dvz_input_emit_pointer(router, &press);
    DvzPointerEvent release = press;
    release.type = DVZ_POINTER_EVENT_RELEASE;
    release.timestamp_ns = now + 50000000;
    dvz_input_emit_pointer(router, &release);
    AT(_recorder_contains(&recorder, DVZ_POINTER_EVENT_CLICK));

    DvzPointerEvent press2 = press;
    press2.timestamp_ns = release.timestamp_ns + 10000000;
    dvz_input_emit_pointer(router, &press2);
    DvzPointerEvent release2 = press2;
    release2.type = DVZ_POINTER_EVENT_RELEASE;
    release2.timestamp_ns = press2.timestamp_ns + 40000000;
    dvz_input_emit_pointer(router, &release2);
    AT(_recorder_contains(&recorder, DVZ_POINTER_EVENT_DOUBLE_CLICK));

    DvzPointerEvent drag_press = press;
    drag_press.timestamp_ns = release2.timestamp_ns + 100000000;
    _recorder_reset(&recorder);
    dvz_input_emit_pointer(router, &drag_press);
    DvzPointerEvent drag_move = drag_press;
    drag_move.type = DVZ_POINTER_EVENT_MOVE;
    drag_move.pos[0] += 30.0f;
    drag_move.pos[1] += 0.0f;
    drag_move.timestamp_ns = drag_press.timestamp_ns + 20000000;
    dvz_input_emit_pointer(router, &drag_move);
    DvzPointerEvent drag_release = drag_move;
    drag_release.type = DVZ_POINTER_EVENT_RELEASE;
    drag_release.timestamp_ns = drag_move.timestamp_ns + 10000000;
    dvz_input_emit_pointer(router, &drag_release);
    AT(_recorder_contains(&recorder, DVZ_POINTER_EVENT_DRAG_STOP));

    dvz_pointer_gesture_handler_destroy(gestures);
    dvz_input_router_destroy(router);
    return 0;
}



/**
 * Ensure wheel helpers propagate deltas.
 */
int test_pointer_wheel(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    DvzInputRouter* router = dvz_input_router();
    WheelRecorder recorder = {0};
    dvz_input_subscribe_pointer(router, _record_wheel, &recorder);
    dvz_pointer_emit_wheel(
        router, 100.0f, 50.0f, 0.0f, 0.0f, 0.5f, -1.5f, DVZ_KEY_MODIFIER_SHIFT, 1.0f, 0, NULL);
    AT(recorder.wheel_count == 1);
    AT(recorder.last_dir[0] == 0.5f);
    AT(recorder.last_dir[1] == -1.5f);
    dvz_input_router_destroy(router);
    return 0;
}



/**
 * Validate resize/scale routing and union forwarding.
 */
int test_resize_scale_events(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    DvzInputRouter* router = dvz_input_router();
    WindowEventRecorder recorder = {0};
    dvz_input_subscribe_resize(router, _record_resize, &recorder);
    dvz_input_subscribe_scale(router, _record_scale, &recorder);
    dvz_input_subscribe_event(router, _record_window_union, &recorder);

    DvzInputResizeEvent resize = {800, 600, 400, 300, 2.0f, 2.0f};
    dvz_input_emit_resize(router, &resize);
    AT(recorder.resize_calls == 1);
    AT(recorder.union_resize_calls == 1);
    AT(recorder.resize.framebuffer_width == 800);
    AT(recorder.resize.content_scale_x == 2.0f);

    DvzInputScaleEvent scale = {1.5f, 1.5f};
    dvz_input_emit_scale(router, &scale);
    AT(recorder.scale_calls == 1);
    AT(recorder.union_scale_calls == 1);
    AT(recorder.scale.content_scale_x == 1.5f);

    dvz_input_router_destroy(router);
    return 0;
}



/**
 * Register the input module tests.
 */
int test_input(TstSuite* suite)
{
    ANN(suite);
    const char* tags = "input";
    TEST_SIMPLE(test_router_callbacks);
    TEST_SIMPLE(test_router_unsubscribe);
    TEST_SIMPLE(test_keyboard_modifiers);
    TEST_SIMPLE(test_pointer_gestures);
    TEST_SIMPLE(test_pointer_wheel);
    TEST_SIMPLE(test_resize_scale_events);
    return 0;
}
