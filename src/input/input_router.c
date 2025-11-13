/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

#include <stddef.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "datoviz/input/router.h"

typedef struct DvzPointerSubscription
{
    DvzPointerCallback callback;
    void* user_data;
} DvzPointerSubscription;

typedef struct DvzKeyboardSubscription
{
    DvzKeyboardCallback callback;
    void* user_data;
} DvzKeyboardSubscription;

typedef struct DvzEventSubscription
{
    DvzInputCallback callback;
    void* user_data;
} DvzEventSubscription;

struct DvzInputRouter
{
    DvzPointerSubscription* pointer_subs;
    uint32_t pointer_count;
    uint32_t pointer_capacity;

    DvzKeyboardSubscription* keyboard_subs;
    uint32_t keyboard_count;
    uint32_t keyboard_capacity;

    DvzEventSubscription* event_subs;
    uint32_t event_count;
    uint32_t event_capacity;
};

static void _ensure_capacity(
    uint32_t item_size, void** data, uint32_t* capacity, uint32_t min_capacity)
{
    if (*capacity >= min_capacity)
        return;
    uint32_t new_capacity = *capacity ? (*capacity * 2) : 4;
    while (new_capacity < min_capacity)
        new_capacity *= 2;
    *data = dvz_realloc(*data, item_size * new_capacity);
    *capacity = new_capacity;
}

/**
 * Create a new router instance.
 */
DVZ_EXPORT DvzInputRouter* dvz_input_router(void)
{
    DvzInputRouter* router = (DvzInputRouter*)dvz_calloc(1, sizeof(DvzInputRouter));
    ANN(router);
    return router;
}


/**
 * Destroy a router.
 */
DVZ_EXPORT void dvz_input_router_destroy(DvzInputRouter* router)
{
    ANN(router);
    if (router->pointer_subs != NULL)
        dvz_free(router->pointer_subs);
    if (router->keyboard_subs != NULL)
        dvz_free(router->keyboard_subs);
    if (router->event_subs != NULL)
        dvz_free(router->event_subs);
    dvz_free(router);
}


/**
 * Subscribe to pointer events.
 */
DVZ_EXPORT void dvz_input_subscribe_pointer(
    DvzInputRouter* router, DvzPointerCallback callback, void* user_data)
{
    ANN(router);
    ANN(callback);
    _ensure_capacity(
        sizeof(DvzPointerSubscription), (void**)&router->pointer_subs, &router->pointer_capacity,
        router->pointer_count + 1);
    router->pointer_subs[router->pointer_count++] = (DvzPointerSubscription){callback, user_data};
}


/**
 * Unsubscribe from pointer events.
 */
DVZ_EXPORT void dvz_input_unsubscribe_pointer(
    DvzInputRouter* router, DvzPointerCallback callback, void* user_data)
{
    ANN(router);
    for (uint32_t i = 0; i < router->pointer_count; i++)
    {
        DvzPointerSubscription* sub = &router->pointer_subs[i];
        if (sub->callback == callback && sub->user_data == user_data)
        {
            router->pointer_count--;
            if (i != router->pointer_count)
            {
                router->pointer_subs[i] = router->pointer_subs[router->pointer_count];
            }
            break;
        }
    }
}


/**
 * Emit a pointer event.
 */
DVZ_EXPORT void dvz_input_emit_pointer(
    DvzInputRouter* router, const DvzPointerEvent* event)
{
    ANN(router);
    ANN(event);
    for (uint32_t i = 0; i < router->pointer_count; i++)
    {
        DvzPointerSubscription* sub = &router->pointer_subs[i];
        sub->callback(router, event, sub->user_data);
    }
}


/**
 * Subscribe to keyboard events.
 */
DVZ_EXPORT void dvz_input_subscribe_keyboard(
    DvzInputRouter* router, DvzKeyboardCallback callback, void* user_data)
{
    ANN(router);
    ANN(callback);
    _ensure_capacity(
        sizeof(DvzKeyboardSubscription), (void**)&router->keyboard_subs,
        &router->keyboard_capacity, router->keyboard_count + 1);
    router->keyboard_subs[router->keyboard_count++] = (DvzKeyboardSubscription){callback, user_data};
}


/**
 * Unsubscribe from keyboard events.
 */
DVZ_EXPORT void dvz_input_unsubscribe_keyboard(
    DvzInputRouter* router, DvzKeyboardCallback callback, void* user_data)
{
    ANN(router);
    for (uint32_t i = 0; i < router->keyboard_count; i++)
    {
        DvzKeyboardSubscription* sub = &router->keyboard_subs[i];
        if (sub->callback == callback && sub->user_data == user_data)
        {
            router->keyboard_count--;
            if (i != router->keyboard_count)
            {
                router->keyboard_subs[i] = router->keyboard_subs[router->keyboard_count];
            }
            break;
        }
    }
}


/**
 * Emit a keyboard event.
 */
DVZ_EXPORT void dvz_input_emit_keyboard(
    DvzInputRouter* router, const DvzKeyboardEvent* event)
{
    ANN(router);
    ANN(event);
    for (uint32_t i = 0; i < router->keyboard_count; i++)
    {
        DvzKeyboardSubscription* sub = &router->keyboard_subs[i];
        sub->callback(router, event, sub->user_data);
    }
}


/**
 * Subscribe to union-style input events.
 */
DVZ_EXPORT void dvz_input_subscribe_event(
    DvzInputRouter* router, DvzInputCallback callback, void* user_data)
{
    ANN(router);
    ANN(callback);
    _ensure_capacity(
        sizeof(DvzEventSubscription), (void**)&router->event_subs, &router->event_capacity,
        router->event_count + 1);
    router->event_subs[router->event_count++] = (DvzEventSubscription){callback, user_data};
}


/**
 * Unsubscribe from union-style input events.
 */
DVZ_EXPORT void dvz_input_unsubscribe_event(
    DvzInputRouter* router, DvzInputCallback callback, void* user_data)
{
    ANN(router);
    for (uint32_t i = 0; i < router->event_count; i++)
    {
        DvzEventSubscription* sub = &router->event_subs[i];
        if (sub->callback == callback && sub->user_data == user_data)
        {
            router->event_count--;
            if (i != router->event_count)
            {
                router->event_subs[i] = router->event_subs[router->event_count];
            }
            break;
        }
    }
}


/**
 * Emit a union input event.
 */
DVZ_EXPORT void dvz_input_emit_event(
    DvzInputRouter* router, const DvzInputEvent* event)
{
    ANN(router);
    ANN(event);
    for (uint32_t i = 0; i < router->event_count; i++)
    {
        DvzEventSubscription* sub = &router->event_subs[i];
        sub->callback(router, event, sub->user_data);
    }
}
