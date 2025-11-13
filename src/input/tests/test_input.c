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

#include "test_input.h"
#include "_assertions.h"
#include "datoviz/input.h"
#include "testing.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct
{
    DvzMouseEventType last_type;
    uint32_t count;
    DvzMouseEventType history[8];
} EventRecorder;



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
_make_event(DvzMouseEventType type, float x, float y, DvzMouseButton button, uint64_t timestamp)
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
        _make_event(DVZ_MOUSE_EVENT_PRESS, 10.0f, 5.0f, DVZ_MOUSE_BUTTON_LEFT, 1);
    dvz_input_emit_pointer(router, &event);
    AT(state == 2);
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
        _make_event(DVZ_MOUSE_EVENT_PRESS, 10.0f, 10.0f, DVZ_MOUSE_BUTTON_LEFT, now);
    dvz_input_emit_pointer(router, &press);
    DvzPointerEvent release = press;
    release.type = DVZ_MOUSE_EVENT_RELEASE;
    release.timestamp_ns = now + 50000000;
    dvz_input_emit_pointer(router, &release);
    AT(recorder.last_type == DVZ_MOUSE_EVENT_CLICK);

    recorder.count = 0;
    recorder.last_type = DVZ_MOUSE_EVENT_NONE;

    DvzPointerEvent press2 = press;
    press2.timestamp_ns = release.timestamp_ns + 10000000;
    dvz_input_emit_pointer(router, &press2);
    DvzPointerEvent release2 = press2;
    release2.type = DVZ_MOUSE_EVENT_RELEASE;
    release2.timestamp_ns = press2.timestamp_ns + 40000000;
    dvz_input_emit_pointer(router, &release2);
    AT(recorder.last_type == DVZ_MOUSE_EVENT_DOUBLE_CLICK);

    recorder.count = 0;
    recorder.last_type = DVZ_MOUSE_EVENT_NONE;

    DvzPointerEvent drag_press = press;
    drag_press.timestamp_ns = release2.timestamp_ns + 100000000;
    dvz_input_emit_pointer(router, &drag_press);
    DvzPointerEvent drag_move = drag_press;
    drag_move.type = DVZ_MOUSE_EVENT_MOVE;
    drag_move.pos[0] += 30.0f;
    drag_move.pos[1] += 0.0f;
    drag_move.timestamp_ns = drag_press.timestamp_ns + 20000000;
    dvz_input_emit_pointer(router, &drag_move);
    DvzPointerEvent drag_release = drag_move;
    drag_release.type = DVZ_MOUSE_EVENT_RELEASE;
    drag_release.timestamp_ns = drag_move.timestamp_ns + 10000000;
    dvz_input_emit_pointer(router, &drag_release);
    AT(recorder.last_type == DVZ_MOUSE_EVENT_DRAG_STOP);

    dvz_pointer_gesture_handler_destroy(gestures);
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
    TEST_SIMPLE(test_keyboard_modifiers);
    TEST_SIMPLE(test_pointer_gestures);
    return 0;
}
