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

#include "_alloc.h"
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
    DvzPointerButton drag_start_button;
    uint32_t count;
    DvzPointerEventType history[16];
} EventRecorder;



typedef struct
{
    DvzCallbackId unsubscribe_id;
    uint32_t unsubscribe_calls;
    bool unsubscribe_ok;
    uint32_t follower_calls;
} DispatchRecorder;



typedef struct
{
    DvzCallbackId removed_id;
    uint32_t remover_calls;
    uint32_t removed_calls;
    bool remove_ok;
} RemovalRecorder;



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


typedef struct
{
    DvzPointerEvent event;
    uint32_t count;
} PointerRecorder;



typedef struct
{
    const char* utf8;
    uint32_t byte_size;
    int mods;
    void* event_user_data;
    char copied[32];
    uint32_t count;
} TextRecorder;



typedef struct
{
    DvzCallbackId self_id;
    DvzCallbackId removed_id;
    uint32_t self_calls;
    uint32_t remover_calls;
    uint32_t removed_calls;
    uint32_t follower_calls;
    uint32_t added_calls;
    DvzCallbackId added_id;
    bool self_unsubscribe_ok;
    bool remove_ok;
    bool added;
} TextMutationRecorder;



typedef struct
{
    uint32_t keyboard_count;
    uint32_t text_count;
} TextKeyboardRecorder;



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
    if (recorder->last_type == DVZ_POINTER_EVENT_DRAG_START)
        recorder->drag_start_button = event->content.pointer.button;
    if (recorder->count < sizeof(recorder->history) / sizeof(recorder->history[0]))
        recorder->history[recorder->count++] = recorder->last_type;
}



/**
 * Reset an event recorder.
 */
static void _recorder_reset(EventRecorder* recorder)
{
    ANN(recorder);
    dvz_memset(recorder, sizeof(*recorder), 0, sizeof(*recorder));
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
    recorder->unsubscribe_ok = dvz_input_unsubscribe(router, recorder->unsubscribe_id);
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
 * Remove a later callback before its turn in the current dispatch.
 */
static void
_remove_later_pointer(DvzInputRouter* router, const DvzPointerEvent* event, void* user_data)
{
    ANN(router);
    ANN(event);
    ANN(user_data);
    RemovalRecorder* recorder = user_data;
    recorder->remover_calls++;
    recorder->remove_ok = dvz_input_unsubscribe(router, recorder->removed_id);
}



/**
 * Record an invocation that should be suppressed after unsubscription.
 */
static void
_removed_pointer(DvzInputRouter* router, const DvzPointerEvent* event, void* user_data)
{
    ANN(router);
    ANN(event);
    ANN(user_data);
    RemovalRecorder* recorder = user_data;
    recorder->removed_calls++;
}



static void _record_text(DvzInputRouter* router, const DvzInputTextEvent* event, void* user_data)
{
    ANN(router);
    ANN(event);
    ANN(user_data);
    TextRecorder* recorder = user_data;
    recorder->utf8 = event->utf8;
    recorder->byte_size = event->byte_size;
    recorder->mods = event->mods;
    recorder->event_user_data = event->user_data;
    if (event->byte_size >= sizeof(recorder->copied))
        return;
    memcpy(recorder->copied, event->utf8, event->byte_size);
    recorder->copied[event->byte_size] = '\0';
    recorder->count++;
}



static void _record_text_union(
    DvzInputRouter* router, const DvzInputEvent* event, void* user_data)
{
    ANN(router);
    ANN(event);
    if (event->type != DVZ_INPUT_EVENT_TEXT)
        return;
    _record_text(router, &event->content.text, user_data);
}



static void _unsubscribe_text(DvzInputRouter* router, const DvzInputTextEvent* event, void* user_data)
{
    ANN(router);
    ANN(event);
    ANN(user_data);
    TextMutationRecorder* recorder = user_data;
    recorder->self_calls++;
    recorder->self_unsubscribe_ok = dvz_input_unsubscribe(router, recorder->self_id);
}



static void _remove_later_text(
    DvzInputRouter* router, const DvzInputTextEvent* event, void* user_data)
{
    ANN(router);
    ANN(event);
    ANN(user_data);
    TextMutationRecorder* recorder = user_data;
    recorder->remover_calls++;
    recorder->remove_ok = dvz_input_unsubscribe(router, recorder->removed_id);
}



static void _removed_text(DvzInputRouter* router, const DvzInputTextEvent* event, void* user_data)
{
    ANN(router);
    ANN(event);
    ANN(user_data);
    TextMutationRecorder* recorder = user_data;
    recorder->removed_calls++;
}



static void _added_text(DvzInputRouter* router, const DvzInputTextEvent* event, void* user_data)
{
    ANN(router);
    ANN(event);
    ANN(user_data);
    TextMutationRecorder* recorder = user_data;
    recorder->added_calls++;
}



static void _add_text_during_dispatch(
    DvzInputRouter* router, const DvzInputTextEvent* event, void* user_data)
{
    ANN(router);
    ANN(event);
    ANN(user_data);
    TextMutationRecorder* recorder = user_data;
    recorder->follower_calls++;
    if (!recorder->added)
    {
        recorder->added_id = dvz_input_subscribe_text(router, _added_text, recorder);
        recorder->added = true;
    }
}



static void _record_keyboard_or_text(
    DvzInputRouter* router, const DvzInputEvent* event, void* user_data)
{
    ANN(router);
    ANN(event);
    ANN(user_data);
    TextKeyboardRecorder* recorder = user_data;
    if (event->type == DVZ_INPUT_EVENT_KEYBOARD)
        recorder->keyboard_count++;
    else if (event->type == DVZ_INPUT_EVENT_TEXT)
        recorder->text_count++;
}



/**
 * Reject subscription-array growth for allocation-failure coverage.
 */
static void* _reject_realloc(void* pointer, DvzSize size)
{
    (void)pointer;
    (void)size;
    return NULL;
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
 * Capture the last pointer event.
 */
static void _record_pointer(DvzInputRouter* router, const DvzPointerEvent* event, void* user_data)
{
    ANN(router);
    ANN(event);
    ANN(user_data);
    PointerRecorder* recorder = user_data;
    recorder->event = *event;
    recorder->count++;
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
int test_router_callbacks(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    DvzInputRouter* router = dvz_input_router();
    int state = 0;
    DvzCallbackId id0 = dvz_input_subscribe_pointer(router, _router_callback_one, &state);
    DvzCallbackId id1 = dvz_input_subscribe_pointer(router, _router_callback_two, &state);
    AT(id0 != DVZ_CALLBACK_ID_NONE);
    AT(id1 != DVZ_CALLBACK_ID_NONE);
    AT(id0 != id1);
    DvzPointerEvent event =
        _make_event(DVZ_POINTER_EVENT_PRESS, 10.0f, 5.0f, DVZ_POINTER_BUTTON_LEFT, 1);
    dvz_input_emit_pointer(router, &event);
    AT(state == 2);
    AT(dvz_input_unsubscribe(router, id0));
    AT(!dvz_input_unsubscribe(router, id0));
    dvz_input_router_destroy(router);
    return 0;
}



/**
 * Ensure unsubscribing from inside a callback does not stop dispatch.
 */
int test_router_unsubscribe(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    DvzInputRouter* router = dvz_input_router();
    DispatchRecorder recorder = {0};
    recorder.unsubscribe_id = dvz_input_subscribe_pointer(router, _unsubscribe_pointer, &recorder);
    DvzCallbackId follower_id = dvz_input_subscribe_pointer(router, _follower_pointer, &recorder);
    AT(recorder.unsubscribe_id != DVZ_CALLBACK_ID_NONE);
    AT(follower_id != DVZ_CALLBACK_ID_NONE);
    DvzPointerEvent event =
        _make_event(DVZ_POINTER_EVENT_PRESS, 0.0f, 0.0f, DVZ_POINTER_BUTTON_LEFT, 1);
    dvz_input_emit_pointer(router, &event);
    AT(recorder.unsubscribe_calls == 1);
    AT(recorder.unsubscribe_ok);
    AT(recorder.follower_calls == 1);
    AT(!dvz_input_unsubscribe(router, recorder.unsubscribe_id));
    AT(dvz_input_unsubscribe(router, follower_id));
    dvz_input_router_destroy(router);
    return 0;
}



/**
 * Ensure removing a later callback prevents it from running with stale user data.
 */
int test_router_remove_later_callback(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    DvzInputRouter* router = dvz_input_router();
    RemovalRecorder recorder = {0};
    DvzCallbackId remover_id =
        dvz_input_subscribe_pointer(router, _remove_later_pointer, &recorder);
    recorder.removed_id = dvz_input_subscribe_pointer(router, _removed_pointer, &recorder);
    AT(remover_id != DVZ_CALLBACK_ID_NONE);
    AT(recorder.removed_id != DVZ_CALLBACK_ID_NONE);
    DvzPointerEvent event =
        _make_event(DVZ_POINTER_EVENT_MOVE, 0.0f, 0.0f, DVZ_POINTER_BUTTON_NONE, 1);
    dvz_input_emit_pointer(router, &event);
    AT(recorder.remover_calls == 1);
    AT(recorder.remove_ok);
    AT(recorder.removed_calls == 0);
    dvz_input_router_destroy(router);
    return 0;
}



/**
 * Preserve existing subscriptions when growing the callback array fails.
 */
int test_router_growth_failure(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    DvzInputRouter* router = dvz_input_router();
    DispatchRecorder recorder = {0};
    for (uint32_t i = 0; i < 4; i++)
        AT(dvz_input_subscribe_pointer(router, _follower_pointer, &recorder) !=
           DVZ_CALLBACK_ID_NONE);

    const DvzAllocator* previous_allocator = dvz_get_allocator();
    DvzAllocator failing_allocator = {.realloc_fn = _reject_realloc};
    dvz_set_allocator(&failing_allocator);
    DvzCallbackId failed_id =
        dvz_input_subscribe_pointer(router, _follower_pointer, &recorder);
    dvz_set_allocator(previous_allocator);

    AT(failed_id == DVZ_CALLBACK_ID_NONE);
    DvzPointerEvent event =
        _make_event(DVZ_POINTER_EVENT_MOVE, 0.0f, 0.0f, DVZ_POINTER_BUTTON_NONE, 1);
    dvz_input_emit_pointer(router, &event);
    AT(recorder.follower_calls == 4);
    dvz_input_router_destroy(router);
    return 0;
}



/**
 * Verify modifier bit tracking works for shift.
 */
int test_keyboard_modifiers(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    DvzKeyboardModifierState* state = dvz_keyboard_modifier_state();
    AT(dvz_keyboard_modifier_state_update(state, DVZ_KEYBOARD_EVENT_PRESS, DVZ_KEY_LEFT_SHIFT) ==
       DVZ_OK);
    AT(dvz_keyboard_modifier_state_mods(state) == DVZ_KEY_MODIFIER_SHIFT);
    AT(dvz_keyboard_modifier_state_update(state, DVZ_KEYBOARD_EVENT_RELEASE, DVZ_KEY_LEFT_SHIFT) ==
       DVZ_OK);
    AT(dvz_keyboard_modifier_state_mods(state) == 0);
    AT(dvz_keyboard_modifier_state_update(state, DVZ_KEYBOARD_EVENT_PRESS, DVZ_KEY_A) ==
       DVZ_ERROR);
    dvz_keyboard_modifier_state_destroy(state);
    return 0;
}



/**
 * Deliver UTF-8 commits through direct and union subscriptions without copying their source span.
 */
int test_text_router_delivery(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzInputRouter* router = dvz_input_router();
    ANN(router);
    TextRecorder direct = {0};
    TextRecorder union_recorder = {0};
    int marker = 42;
    AT(dvz_input_subscribe_text(router, _record_text, &direct) != DVZ_CALLBACK_ID_NONE);
    AT(dvz_input_subscribe_event(router, _record_text_union, &union_recorder) !=
       DVZ_CALLBACK_ID_NONE);

    const char* commits[] = {"a", "\xC3\xA9", "\xC3\xA9\xF0\x9F\x99\x82"};
    const uint32_t sizes[] = {1, 2, 6};
    for (uint32_t i = 0; i < 3; i++)
    {
        DvzInputTextEvent event = {
            .utf8 = commits[i],
            .byte_size = sizes[i],
            .mods = DVZ_KEY_MODIFIER_SHIFT | DVZ_KEY_MODIFIER_ALT,
            .user_data = &marker,
        };
        AT(dvz_input_emit_text(router, &event) == DVZ_OK);
        AT(direct.count == i + 1);
        AT(union_recorder.count == i + 1);
        AT(direct.utf8 == commits[i]);
        AT(union_recorder.utf8 == commits[i]);
        AT(direct.byte_size == sizes[i]);
        AT(union_recorder.byte_size == sizes[i]);
        AT(direct.mods == event.mods);
        AT(union_recorder.mods == event.mods);
        AT(direct.event_user_data == &marker);
        AT(union_recorder.event_user_data == &marker);
        AT(memcmp(direct.copied, commits[i], sizes[i]) == 0);
        AT(memcmp(union_recorder.copied, commits[i], sizes[i]) == 0);
    }

    dvz_input_router_destroy(router);
    return 0;
}



/**
 * Reject invalid and empty UTF-8 commits without notifying subscribers.
 */
int test_text_router_rejects_malformed_utf8(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzInputRouter* router = dvz_input_router();
    ANN(router);
    TextRecorder recorder = {0};
    AT(dvz_input_subscribe_text(router, _record_text, &recorder) != DVZ_CALLBACK_ID_NONE);

    const char invalid_lead[] = "\xC3\x28";
    const char invalid_overlong[] = "\xC0\xAF";
    const DvzInputTextEvent invalid_events[] = {
        {.utf8 = invalid_lead, .byte_size = sizeof(invalid_lead) - 1},
        {.utf8 = invalid_overlong, .byte_size = sizeof(invalid_overlong) - 1},
        {.utf8 = "", .byte_size = 0},
        {.utf8 = NULL, .byte_size = 1},
    };
    for (uint32_t i = 0; i < 4; i++)
    {
        AT(dvz_input_emit_text(router, &invalid_events[i]) == DVZ_ERROR);
        AT(recorder.count == 0);
    }

    dvz_input_router_destroy(router);
    return 0;
}



/**
 * Keep committed text independent from physical press, repeat, and release events.
 */
int test_text_router_separates_keyboard_actions(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzInputRouter* router = dvz_input_router();
    ANN(router);
    TextKeyboardRecorder recorder = {0};
    AT(dvz_input_subscribe_event(router, _record_keyboard_or_text, &recorder) !=
       DVZ_CALLBACK_ID_NONE);

    DvzKeyboardEvent key = {.key = DVZ_KEY_W, .mods = DVZ_KEY_MODIFIER_CONTROL};
    key.type = DVZ_KEYBOARD_EVENT_PRESS;
    dvz_input_emit_keyboard(router, &key);
    AT(dvz_input_emit_text(
           router, &(DvzInputTextEvent){.utf8 = "w", .byte_size = 1, .mods = key.mods}) ==
       DVZ_OK);
    key.type = DVZ_KEYBOARD_EVENT_REPEAT;
    dvz_input_emit_keyboard(router, &key);
    AT(dvz_input_emit_text(
           router, &(DvzInputTextEvent){.utf8 = "w", .byte_size = 1, .mods = key.mods}) ==
       DVZ_OK);
    key.type = DVZ_KEYBOARD_EVENT_RELEASE;
    dvz_input_emit_keyboard(router, &key);

    AT(recorder.keyboard_count == 3);
    AT(recorder.text_count == 2);
    dvz_input_router_destroy(router);
    return 0;
}



/**
 * Preserve text-dispatch semantics when subscriptions mutate during a callback.
 */
int test_text_router_subscription_mutation(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzInputRouter* router = dvz_input_router();
    ANN(router);
    TextMutationRecorder recorder = {0};
    recorder.self_id = dvz_input_subscribe_text(router, _unsubscribe_text, &recorder);
    AT(recorder.self_id != DVZ_CALLBACK_ID_NONE);
    AT(dvz_input_subscribe_text(router, _add_text_during_dispatch, &recorder) !=
       DVZ_CALLBACK_ID_NONE);
    AT(dvz_input_subscribe_text(router, _remove_later_text, &recorder) != DVZ_CALLBACK_ID_NONE);
    recorder.removed_id = dvz_input_subscribe_text(router, _removed_text, &recorder);
    AT(recorder.removed_id != DVZ_CALLBACK_ID_NONE);

    DvzInputTextEvent event = {.utf8 = "x", .byte_size = 1};
    AT(dvz_input_emit_text(router, &event) == DVZ_OK);
    AT(recorder.self_calls == 1);
    AT(recorder.self_unsubscribe_ok);
    AT(recorder.follower_calls == 1);
    AT(recorder.remover_calls == 1);
    AT(recorder.remove_ok);
    AT(recorder.removed_calls == 0);
    AT(recorder.added_calls == 0);
    AT(recorder.added_id != DVZ_CALLBACK_ID_NONE);

    AT(dvz_input_emit_text(router, &event) == DVZ_OK);
    AT(recorder.self_calls == 1);
    AT(recorder.follower_calls == 2);
    AT(recorder.remover_calls == 2);
    AT(recorder.added_calls == 1);

    dvz_input_router_destroy(router);
    return 0;
}



/**
 * Confirm gesture detection emits clicks, double-clicks, and drags.
 */
int test_pointer_gestures(TstContext* suite, const TstCase* item)
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
 * Reject corrupting duplicate presses and backwards event timestamps.
 */
int test_pointer_gesture_invalid_sequences(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    DvzInputRouter* router = dvz_input_router();
    DvzPointerGestureHandler* gestures = dvz_pointer_gesture_handler(router);
    EventRecorder recorder = {0};
    dvz_input_subscribe_event(router, _record_event, &recorder);

    DvzPointerEvent press =
        _make_event(DVZ_POINTER_EVENT_PRESS, 10.0f, 10.0f, DVZ_POINTER_BUTTON_LEFT, 1000000000);
    dvz_input_emit_pointer(router, &press);
    DvzPointerEvent duplicate = press;
    duplicate.button = DVZ_POINTER_BUTTON_RIGHT;
    duplicate.timestamp_ns += 10000000;
    dvz_input_emit_pointer(router, &duplicate);
    DvzPointerEvent move = press;
    move.type = DVZ_POINTER_EVENT_MOVE;
    move.pos[0] += 30.0f;
    move.timestamp_ns += 20000000;
    dvz_input_emit_pointer(router, &move);
    AT(recorder.drag_start_button == DVZ_POINTER_BUTTON_LEFT);
    DvzPointerEvent drag_release = move;
    drag_release.type = DVZ_POINTER_EVENT_RELEASE;
    drag_release.timestamp_ns += 10000000;
    dvz_input_emit_pointer(router, &drag_release);

    _recorder_reset(&recorder);
    DvzPointerEvent late_press =
        _make_event(DVZ_POINTER_EVENT_PRESS, 0.0f, 0.0f, DVZ_POINTER_BUTTON_LEFT, 2000000000);
    dvz_input_emit_pointer(router, &late_press);
    DvzPointerEvent early_release = late_press;
    early_release.type = DVZ_POINTER_EVENT_RELEASE;
    early_release.timestamp_ns--;
    dvz_input_emit_pointer(router, &early_release);
    AT(!_recorder_contains(&recorder, DVZ_POINTER_EVENT_CLICK));

    dvz_pointer_gesture_handler_destroy(gestures);
    dvz_input_router_destroy(router);
    return 0;
}



/**
 * Ensure wheel helpers propagate deltas.
 */
int test_pointer_wheel(TstContext* suite, const TstCase* item)
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
 * Ensure pointer emitters keep per-event logical window size.
 */
int test_pointer_emit_window_size(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzInputRouter* router = dvz_input_router();
    PointerRecorder recorder = {0};
    dvz_input_subscribe_pointer(router, _record_pointer, &recorder);

    dvz_pointer_emit_position(
        router, DVZ_POINTER_EVENT_MOVE, 100.0f, 50.0f, 640.0f, 360.0f,
        DVZ_POINTER_BUTTON_NONE, 0, 2.0f, 0, NULL);
    AT(recorder.count == 1);
    AT(recorder.event.window_size[0] == 640.0f);
    AT(recorder.event.window_size[1] == 360.0f);

    dvz_pointer_emit_wheel(
        router, 100.0f, 50.0f, 800.0f, 400.0f, 0.0f, 1.0f, 0, 2.0f, 0, NULL);
    AT(recorder.count == 2);
    AT(recorder.event.window_size[0] == 800.0f);
    AT(recorder.event.window_size[1] == 400.0f);

    dvz_input_router_destroy(router);
    return 0;
}



/**
 * Validate resize/scale routing and union forwarding.
 */
int test_resize_scale_events(TstContext* suite, const TstCase* item)
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
    TST_MODULE(suite, tags);
    TST_CASE(test_router_callbacks);
    TST_CASE(test_router_unsubscribe);
    TST_CASE(test_router_remove_later_callback);
    TST_CASE(test_router_growth_failure);
    TST_CASE(test_keyboard_modifiers);
    TST_CASE(test_text_router_delivery);
    TST_CASE(test_text_router_rejects_malformed_utf8);
    TST_CASE(test_text_router_separates_keyboard_actions);
    TST_CASE(test_text_router_subscription_mutation);
    TST_CASE(test_pointer_gestures);
    TST_CASE(test_pointer_gesture_invalid_sequences);
    TST_CASE(test_pointer_wheel);
    TST_CASE(test_pointer_emit_window_size);
    TST_CASE(test_resize_scale_events);
    return 0;
}
