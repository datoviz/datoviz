/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Input router                                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_input_router.h"
#include "_pointer.h"
#include "datoviz/input/router.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DvzPointerSubscription
{
    DvzCallbackId id;
    DvzPointerCallback callback;
    void* user_data;
} DvzPointerSubscription;



typedef struct DvzKeyboardSubscription
{
    DvzCallbackId id;
    DvzKeyboardCallback callback;
    void* user_data;
} DvzKeyboardSubscription;



typedef struct DvzTextSubscription
{
    DvzCallbackId id;
    DvzInputTextCallback callback;
    void* user_data;
} DvzTextSubscription;



typedef struct DvzEventSubscription
{
    DvzCallbackId id;
    DvzInputCallback callback;
    void* user_data;
} DvzEventSubscription;



typedef struct DvzResizeSubscription
{
    DvzCallbackId id;
    DvzResizeCallback callback;
    void* user_data;
} DvzResizeSubscription;



typedef struct DvzScaleSubscription
{
    DvzCallbackId id;
    DvzScaleCallback callback;
    void* user_data;
} DvzScaleSubscription;



/** Audiences frozen before any callback or state advancement for one emission. */
typedef struct DvzInputDispatch
{
    void* typed_subs;
    uint32_t typed_count;
    DvzEventSubscription* event_subs;
    uint32_t event_count;
} DvzInputDispatch;



struct DvzInputRouter
{
    DvzPointerSubscription* pointer_subs;
    uint32_t pointer_count;
    uint32_t pointer_capacity;

    DvzKeyboardSubscription* keyboard_subs;
    uint32_t keyboard_count;
    uint32_t keyboard_capacity;

    DvzTextSubscription* text_subs;
    uint32_t text_count;
    uint32_t text_capacity;

    DvzEventSubscription* event_subs;
    uint32_t event_count;
    uint32_t event_capacity;

    DvzResizeSubscription* resize_subs;
    uint32_t resize_count;
    uint32_t resize_capacity;

    DvzScaleSubscription* scale_subs;
    uint32_t scale_count;
    uint32_t scale_capacity;

    DvzCallbackId next_callback_id;
    uint32_t dispatch_depth;
    uint32_t gesture_activations;
    DvzPointerGestureState gesture_state;

    /* Last seen resize event — cached so subscribers joining after the initial resize
     * can still query the current window/framebuffer dimensions. */
    DvzInputResizeEvent last_resize;
    bool has_last_resize;
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static bool
_ensure_capacity(size_t item_size, void** data, uint32_t* capacity, uint32_t min_capacity)
{
    if (*capacity >= min_capacity)
        return true;
    uint32_t new_capacity = *capacity ? *capacity : 4;
    while (new_capacity < min_capacity)
    {
        if (new_capacity > UINT32_MAX / 2)
        {
            new_capacity = min_capacity;
            break;
        }
        new_capacity *= 2;
    }
    uint64_t bytes = 0;
    if (_dvz_mul_u64_overflows(item_size, new_capacity, &bytes) || bytes > SIZE_MAX)
        return false;
    void* resized = dvz_realloc(*data, bytes);
    if (resized == NULL)
        return false;
    *data = resized;
    *capacity = new_capacity;
    return true;
}



static bool _subscription_active(const DvzInputRouter* router, DvzCallbackId id)
{
    ANN(router);
    if (id == DVZ_CALLBACK_ID_NONE)
        return false;
    for (uint32_t i = 0; i < router->pointer_count; i++)
        if (router->pointer_subs[i].id == id)
            return true;
    for (uint32_t i = 0; i < router->keyboard_count; i++)
        if (router->keyboard_subs[i].id == id)
            return true;
    for (uint32_t i = 0; i < router->text_count; i++)
        if (router->text_subs[i].id == id)
            return true;
    for (uint32_t i = 0; i < router->event_count; i++)
        if (router->event_subs[i].id == id)
            return true;
    for (uint32_t i = 0; i < router->resize_count; i++)
        if (router->resize_subs[i].id == id)
            return true;
    for (uint32_t i = 0; i < router->scale_count; i++)
        if (router->scale_subs[i].id == id)
            return true;
    return false;
}



static DvzCallbackId _next_callback_id(DvzInputRouter* router)
{
    ANN(router);
    do
    {
        router->next_callback_id++;
    } while (
        router->next_callback_id == DVZ_CALLBACK_ID_NONE ||
        _subscription_active(router, router->next_callback_id));
    return router->next_callback_id;
}



static bool
_remove_pointer_sub(DvzInputRouter* router, DvzCallbackId id)
{
    ANN(router);
    for (uint32_t i = 0; i < router->pointer_count; i++)
    {
        if (router->pointer_subs[i].id == id)
        {
            router->pointer_count--;
            for (uint32_t j = i; j < router->pointer_count; j++)
                router->pointer_subs[j] = router->pointer_subs[j + 1];
            return true;
        }
    }
    return false;
}



static bool
_remove_keyboard_sub(DvzInputRouter* router, DvzCallbackId id)
{
    ANN(router);
    for (uint32_t i = 0; i < router->keyboard_count; i++)
    {
        if (router->keyboard_subs[i].id == id)
        {
            router->keyboard_count--;
            for (uint32_t j = i; j < router->keyboard_count; j++)
                router->keyboard_subs[j] = router->keyboard_subs[j + 1];
            return true;
        }
    }
    return false;
}



static bool _remove_text_sub(DvzInputRouter* router, DvzCallbackId id)
{
    ANN(router);
    for (uint32_t i = 0; i < router->text_count; i++)
    {
        if (router->text_subs[i].id == id)
        {
            router->text_count--;
            for (uint32_t j = i; j < router->text_count; j++)
                router->text_subs[j] = router->text_subs[j + 1];
            return true;
        }
    }
    return false;
}



static bool
_remove_event_sub(DvzInputRouter* router, DvzCallbackId id)
{
    ANN(router);
    for (uint32_t i = 0; i < router->event_count; i++)
    {
        if (router->event_subs[i].id == id)
        {
            router->event_count--;
            for (uint32_t j = i; j < router->event_count; j++)
                router->event_subs[j] = router->event_subs[j + 1];
            return true;
        }
    }
    return false;
}



static bool
_remove_resize_sub(DvzInputRouter* router, DvzCallbackId id)
{
    ANN(router);
    for (uint32_t i = 0; i < router->resize_count; i++)
    {
        if (router->resize_subs[i].id == id)
        {
            router->resize_count--;
            for (uint32_t j = i; j < router->resize_count; j++)
                router->resize_subs[j] = router->resize_subs[j + 1];
            return true;
        }
    }
    return false;
}



static bool
_remove_scale_sub(DvzInputRouter* router, DvzCallbackId id)
{
    ANN(router);
    for (uint32_t i = 0; i < router->scale_count; i++)
    {
        if (router->scale_subs[i].id == id)
        {
            router->scale_count--;
            for (uint32_t j = i; j < router->scale_count; j++)
                router->scale_subs[j] = router->scale_subs[j + 1];
            return true;
        }
    }
    return false;
}



/**
 * Copy one subscription audience, checking the allocation size before copying.
 *
 * @param source current subscription array
 * @param count number of subscriptions
 * @param item_size size of one subscription
 * @return owned copy, or NULL for an empty audience or allocation failure
 */
static void* _copy_subscriptions(const void* source, uint32_t count, size_t item_size)
{
    if (count == 0)
        return NULL;
    uint64_t bytes = 0;
    if (_dvz_mul_u64_overflows(item_size, count, &bytes) || bytes > SIZE_MAX)
        return NULL;
    void* copy = dvz_calloc(count, item_size);
    if (copy != NULL)
        dvz_memcpy(copy, (size_t)bytes, source, (size_t)bytes);
    return copy;
}



/**
 * Release any allocated audiences, including partially prepared snapshots.
 *
 * @param dispatch owned emission snapshots
 */
static void _release_dispatch(DvzInputDispatch* dispatch)
{
    if (dispatch->typed_subs != NULL)
        dvz_free(dispatch->typed_subs);
    if (dispatch->event_subs != NULL)
        dvz_free(dispatch->event_subs);
    *dispatch = (DvzInputDispatch){0};
}



/**
 * Freeze typed and union audiences before either stream can invoke a callback.
 *
 * @param router input router
 * @param type event family, or NONE for union-only dispatch
 * @param[out] dispatch owned snapshots, left empty on failure
 * @return true when both snapshots were prepared
 */
static bool
_snapshot_dispatch(DvzInputRouter* router, DvzInputEventType type, DvzInputDispatch* dispatch)
{
    ANN(router);
    ANN(dispatch);
    *dispatch = (DvzInputDispatch){0};
    const void* typed_subs = NULL;
    size_t item_size = 0;
    switch (type)
    {
    case DVZ_INPUT_EVENT_POINTER:
        typed_subs = router->pointer_subs;
        dispatch->typed_count = router->pointer_count;
        item_size = sizeof(DvzPointerSubscription);
        break;
    case DVZ_INPUT_EVENT_KEYBOARD:
        typed_subs = router->keyboard_subs;
        dispatch->typed_count = router->keyboard_count;
        item_size = sizeof(DvzKeyboardSubscription);
        break;
    case DVZ_INPUT_EVENT_TEXT:
        typed_subs = router->text_subs;
        dispatch->typed_count = router->text_count;
        item_size = sizeof(DvzTextSubscription);
        break;
    case DVZ_INPUT_EVENT_RESIZE:
        typed_subs = router->resize_subs;
        dispatch->typed_count = router->resize_count;
        item_size = sizeof(DvzResizeSubscription);
        break;
    case DVZ_INPUT_EVENT_SCALE:
        typed_subs = router->scale_subs;
        dispatch->typed_count = router->scale_count;
        item_size = sizeof(DvzScaleSubscription);
        break;
    default:
        break;
    }
    dispatch->typed_subs = _copy_subscriptions(typed_subs, dispatch->typed_count, item_size);
    if (dispatch->typed_count > 0 && dispatch->typed_subs == NULL)
        goto fail;
    dispatch->event_count = router->event_count;
    dispatch->event_subs = _copy_subscriptions(
        router->event_subs, dispatch->event_count, sizeof(DvzEventSubscription));
    if (dispatch->event_count > 0 && dispatch->event_subs == NULL)
        goto fail;
    return true;

fail:
    _release_dispatch(dispatch);
    return false;
}



/**
 * Invoke still-active typed callbacks from the frozen emission audience.
 *
 * @param router input router
 * @param event copied original input event
 * @param dispatch frozen typed audience
 */
static void _dispatch_typed_subs(
    DvzInputRouter* router, const DvzInputEvent* event, const DvzInputDispatch* dispatch)
{
    for (uint32_t i = 0; i < dispatch->typed_count; i++)
    {
        switch (event->type)
        {
        case DVZ_INPUT_EVENT_POINTER:
        {
            DvzPointerSubscription* sub = &((DvzPointerSubscription*)dispatch->typed_subs)[i];
            if (sub->callback != NULL && _subscription_active(router, sub->id))
                sub->callback(router, &event->content.pointer, sub->user_data);
            break;
        }
        case DVZ_INPUT_EVENT_KEYBOARD:
        {
            DvzKeyboardSubscription* sub = &((DvzKeyboardSubscription*)dispatch->typed_subs)[i];
            if (sub->callback != NULL && _subscription_active(router, sub->id))
                sub->callback(router, &event->content.keyboard, sub->user_data);
            break;
        }
        case DVZ_INPUT_EVENT_TEXT:
        {
            DvzTextSubscription* sub = &((DvzTextSubscription*)dispatch->typed_subs)[i];
            if (sub->callback != NULL && _subscription_active(router, sub->id))
                sub->callback(router, &event->content.text, sub->user_data);
            break;
        }
        case DVZ_INPUT_EVENT_RESIZE:
        {
            DvzResizeSubscription* sub = &((DvzResizeSubscription*)dispatch->typed_subs)[i];
            if (sub->callback != NULL && _subscription_active(router, sub->id))
                sub->callback(router, &event->content.resize, sub->user_data);
            break;
        }
        case DVZ_INPUT_EVENT_SCALE:
        {
            DvzScaleSubscription* sub = &((DvzScaleSubscription*)dispatch->typed_subs)[i];
            if (sub->callback != NULL && _subscription_active(router, sub->id))
                sub->callback(router, &event->content.scale, sub->user_data);
            break;
        }
        default:
            break;
        }
    }
}



/**
 * Invoke still-active union callbacks from the frozen emission audience.
 *
 * @param router input router
 * @param event original or derived input event
 * @param dispatch frozen union audience
 */
static void _dispatch_event_subs(
    DvzInputRouter* router, const DvzInputEvent* event, const DvzInputDispatch* dispatch)
{
    for (uint32_t i = 0; i < dispatch->event_count; i++)
    {
        DvzEventSubscription* sub = &dispatch->event_subs[i];
        if (sub->callback != NULL && _subscription_active(router, sub->id))
            sub->callback(router, event, sub->user_data);
    }
}



/**
 * Dispatch one copied input event with one audience and optional canonical recognition.
 *
 * @param router input router
 * @param event copied original event, borrowed only for this emission
 * @param typed whether to dispatch typed callbacks and process canonical state
 * @return DVZ_OK on dispatch, DVZ_ERROR if audience allocation fails
 */
static DvzResult _emit_input(DvzInputRouter* router, const DvzInputEvent* event, bool typed)
{
    DvzInputDispatch dispatch = {0};
    if (!_snapshot_dispatch(router, typed ? event->type : DVZ_INPUT_EVENT_NONE, &dispatch))
    {
        log_error("input audience snapshot allocation failed; dropping event");
        return DVZ_ERROR;
    }

    ASSERT(router->dispatch_depth < UINT32_MAX);
    router->dispatch_depth++;
    DvzInputEvent derived = {0};
    if (typed && event->type == DVZ_INPUT_EVENT_POINTER && router->gesture_activations > 0)
    {
        derived.type = DVZ_INPUT_EVENT_POINTER;
        _dvz_pointer_gesture_process(
            &router->gesture_state, &event->content.pointer, &derived.content.pointer);
        if (derived.content.pointer.type != DVZ_POINTER_EVENT_NONE)
            _dispatch_event_subs(router, &derived, &dispatch);
    }
    else if (typed && event->type == DVZ_INPUT_EVENT_RESIZE)
    {
        router->last_resize = event->content.resize;
        router->has_last_resize = true;
    }

    if (typed)
        _dispatch_typed_subs(router, event, &dispatch);
    _dispatch_event_subs(router, event, &dispatch);
    _release_dispatch(&dispatch);
    router->dispatch_depth--;
    return DVZ_OK;
}



static bool _utf8_valid(const char* utf8, uint32_t byte_size)
{
    if (utf8 == NULL || byte_size == 0)
        return false;
    uint32_t i = 0;
    while (i < byte_size)
    {
        uint8_t b0 = (uint8_t)utf8[i++];
        if (b0 <= 0x7f)
            continue;

        uint32_t codepoint = 0;
        uint32_t continuation_count = 0;
        if (b0 >= 0xc2 && b0 <= 0xdf)
        {
            codepoint = b0 & 0x1fu;
            continuation_count = 1;
        }
        else if (b0 >= 0xe0 && b0 <= 0xef)
        {
            codepoint = b0 & 0x0fu;
            continuation_count = 2;
        }
        else if (b0 >= 0xf0 && b0 <= 0xf4)
        {
            codepoint = b0 & 0x07u;
            continuation_count = 3;
        }
        else
            return false;

        if (continuation_count > byte_size - i)
            return false;
        for (uint32_t j = 0; j < continuation_count; j++)
        {
            uint8_t continuation = (uint8_t)utf8[i++];
            if ((continuation & 0xc0u) != 0x80u)
                return false;
            codepoint = (codepoint << 6) | (continuation & 0x3fu);
        }
        if ((continuation_count == 2 && codepoint < 0x800u) ||
            (continuation_count == 3 && codepoint < 0x10000u) || codepoint > 0x10ffffu ||
            (codepoint >= 0xd800u && codepoint <= 0xdfffu))
            return false;
    }
    return true;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Acquire one activation without replacing an existing gesture history.
 *
 * @param router borrowed input router
 * @return true on success, false if the activation count would overflow
 */
bool _dvz_input_router_gestures_acquire(DvzInputRouter* router)
{
    ANN(router);
    if (router->gesture_activations == UINT32_MAX)
    {
        log_error("pointer gesture activation count overflow");
        return false;
    }
    if (router->gesture_activations == 0)
        _dvz_pointer_gesture_reset(&router->gesture_state);
    router->gesture_activations++;
    return true;
}



/**
 * Release an activation and discard history only when recognition becomes inactive.
 *
 * @param router borrowed input router with an active recognition handle
 */
void _dvz_input_router_gestures_release(DvzInputRouter* router)
{
    ANN(router);
    ASSERT(router->gesture_activations > 0);
    router->gesture_activations--;
    if (router->gesture_activations == 0)
        _dvz_pointer_gesture_reset(&router->gesture_state);
}



DvzInputRouter* dvz_input_router(void)
{
    DvzInputRouter* router = (DvzInputRouter*)dvz_calloc(1, sizeof(DvzInputRouter));
    return router;
}



void dvz_input_router_destroy(DvzInputRouter* router)
{
    ANN(router);
    ASSERT(router->dispatch_depth == 0);
    ASSERT(router->gesture_activations == 0);
    if (router->pointer_subs != NULL)
        dvz_free(router->pointer_subs);
    if (router->keyboard_subs != NULL)
        dvz_free(router->keyboard_subs);
    if (router->text_subs != NULL)
        dvz_free(router->text_subs);
    if (router->event_subs != NULL)
        dvz_free(router->event_subs);
    if (router->resize_subs != NULL)
        dvz_free(router->resize_subs);
    if (router->scale_subs != NULL)
        dvz_free(router->scale_subs);
    dvz_free(router);
}



DvzCallbackId dvz_input_subscribe_pointer(
    DvzInputRouter* router, DvzPointerCallback callback, void* user_data)
{
    if (router == NULL || callback == NULL)
        return DVZ_CALLBACK_ID_NONE;
    if (router->pointer_count == UINT32_MAX ||
        !_ensure_capacity(
            sizeof(DvzPointerSubscription), (void**)&router->pointer_subs,
            &router->pointer_capacity, router->pointer_count + 1))
        return DVZ_CALLBACK_ID_NONE;
    DvzCallbackId id = _next_callback_id(router);
    router->pointer_subs[router->pointer_count++] =
        (DvzPointerSubscription){id, callback, user_data};
    return id;
}



bool dvz_input_unsubscribe(DvzInputRouter* router, DvzCallbackId id)
{
    if (router == NULL || id == DVZ_CALLBACK_ID_NONE)
        return false;
    if (_remove_pointer_sub(router, id))
        return true;
    if (_remove_keyboard_sub(router, id))
        return true;
    if (_remove_text_sub(router, id))
        return true;
    if (_remove_event_sub(router, id))
        return true;
    if (_remove_resize_sub(router, id))
        return true;
    if (_remove_scale_sub(router, id))
        return true;
    return false;
}



void dvz_input_emit_pointer(DvzInputRouter* router, const DvzPointerEvent* event)
{
    ANN(router);
    ANN(event);
    DvzInputEvent input = {.type = DVZ_INPUT_EVENT_POINTER};
    input.content.pointer = *event;
    (void)_emit_input(router, &input, true);
}



DvzCallbackId dvz_input_subscribe_keyboard(
    DvzInputRouter* router, DvzKeyboardCallback callback, void* user_data)
{
    if (router == NULL || callback == NULL)
        return DVZ_CALLBACK_ID_NONE;
    if (router->keyboard_count == UINT32_MAX ||
        !_ensure_capacity(
            sizeof(DvzKeyboardSubscription), (void**)&router->keyboard_subs,
            &router->keyboard_capacity, router->keyboard_count + 1))
        return DVZ_CALLBACK_ID_NONE;
    DvzCallbackId id = _next_callback_id(router);
    router->keyboard_subs[router->keyboard_count++] =
        (DvzKeyboardSubscription){id, callback, user_data};
    return id;
}



void dvz_input_emit_keyboard(DvzInputRouter* router, const DvzKeyboardEvent* event)
{
    ANN(router);
    ANN(event);
    DvzInputEvent input = {.type = DVZ_INPUT_EVENT_KEYBOARD};
    input.content.keyboard = *event;
    (void)_emit_input(router, &input, true);
}



DvzCallbackId dvz_input_subscribe_text(
    DvzInputRouter* router, DvzInputTextCallback callback, void* user_data)
{
    if (router == NULL || callback == NULL)
        return DVZ_CALLBACK_ID_NONE;
    if (router->text_count == UINT32_MAX ||
        !_ensure_capacity(
            sizeof(DvzTextSubscription), (void**)&router->text_subs, &router->text_capacity,
            router->text_count + 1))
        return DVZ_CALLBACK_ID_NONE;
    DvzCallbackId id = _next_callback_id(router);
    router->text_subs[router->text_count++] = (DvzTextSubscription){id, callback, user_data};
    return id;
}



DvzResult dvz_input_emit_text(DvzInputRouter* router, const DvzInputTextEvent* event)
{
    if (router == NULL || event == NULL || !_utf8_valid(event->utf8, event->byte_size))
        return DVZ_ERROR;

    DvzInputEvent input = {.type = DVZ_INPUT_EVENT_TEXT};
    input.content.text = *event;
    return _emit_input(router, &input, true);
}



DvzCallbackId
dvz_input_subscribe_event(DvzInputRouter* router, DvzInputCallback callback, void* user_data)
{
    if (router == NULL || callback == NULL)
        return DVZ_CALLBACK_ID_NONE;
    if (router->event_count == UINT32_MAX ||
        !_ensure_capacity(
            sizeof(DvzEventSubscription), (void**)&router->event_subs, &router->event_capacity,
            router->event_count + 1))
        return DVZ_CALLBACK_ID_NONE;
    DvzCallbackId id = _next_callback_id(router);
    router->event_subs[router->event_count++] = (DvzEventSubscription){id, callback, user_data};
    return id;
}



void dvz_input_emit_event(DvzInputRouter* router, const DvzInputEvent* event)
{
    ANN(router);
    ANN(event);
    DvzInputEvent input = *event;
    (void)_emit_input(router, &input, false);
}



DvzCallbackId dvz_input_subscribe_resize(
    DvzInputRouter* router, DvzResizeCallback callback, void* user_data)
{
    if (router == NULL || callback == NULL)
        return DVZ_CALLBACK_ID_NONE;
    if (router->resize_count == UINT32_MAX ||
        !_ensure_capacity(
            sizeof(DvzResizeSubscription), (void**)&router->resize_subs,
            &router->resize_capacity, router->resize_count + 1))
        return DVZ_CALLBACK_ID_NONE;
    DvzCallbackId id = _next_callback_id(router);
    router->resize_subs[router->resize_count++] = (DvzResizeSubscription){id, callback, user_data};
    return id;
}



void dvz_input_emit_resize(DvzInputRouter* router, const DvzInputResizeEvent* event)
{
    ANN(router);
    ANN(event);
    DvzInputEvent input = {.type = DVZ_INPUT_EVENT_RESIZE};
    input.content.resize = *event;
    (void)_emit_input(router, &input, true);
}



bool dvz_input_router_last_resize(const DvzInputRouter* router, DvzInputResizeEvent* out)
{
    ANN(router);
    ANN(out);
    if (!router->has_last_resize)
        return false;
    *out = router->last_resize;
    return true;
}



DvzCallbackId
dvz_input_subscribe_scale(DvzInputRouter* router, DvzScaleCallback callback, void* user_data)
{
    if (router == NULL || callback == NULL)
        return DVZ_CALLBACK_ID_NONE;
    if (router->scale_count == UINT32_MAX ||
        !_ensure_capacity(
            sizeof(DvzScaleSubscription), (void**)&router->scale_subs, &router->scale_capacity,
            router->scale_count + 1))
        return DVZ_CALLBACK_ID_NONE;
    DvzCallbackId id = _next_callback_id(router);
    router->scale_subs[router->scale_count++] = (DvzScaleSubscription){id, callback, user_data};
    return id;
}



void dvz_input_emit_scale(DvzInputRouter* router, const DvzInputScaleEvent* event)
{
    ANN(router);
    ANN(event);
    DvzInputEvent input = {.type = DVZ_INPUT_EVENT_SCALE};
    input.content.scale = *event;
    (void)_emit_input(router, &input, true);
}
