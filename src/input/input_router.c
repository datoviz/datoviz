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
#include "datoviz/input/router.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

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



typedef struct DvzResizeSubscription
{
    DvzResizeCallback callback;
    void* user_data;
} DvzResizeSubscription;



typedef struct DvzScaleSubscription
{
    DvzScaleCallback callback;
    void* user_data;
} DvzScaleSubscription;



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

    DvzResizeSubscription* resize_subs;
    uint32_t resize_count;
    uint32_t resize_capacity;

    DvzScaleSubscription* scale_subs;
    uint32_t scale_count;
    uint32_t scale_capacity;
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static void
_ensure_capacity(uint32_t item_size, void** data, uint32_t* capacity, uint32_t min_capacity)
{
    if (*capacity >= min_capacity)
        return;
    uint32_t new_capacity = *capacity ? (*capacity * 2) : 4;
    while (new_capacity < min_capacity)
        new_capacity *= 2;
    *data = dvz_realloc(*data, item_size * new_capacity);
    *capacity = new_capacity;
}



static void _dispatch_pointer_subs(DvzInputRouter* router, const DvzPointerEvent* event)
{
    ANN(router);
    ANN(event);
    if (router->pointer_count == 0)
        return;
    uint32_t count = router->pointer_count;
    DvzPointerSubscription* subs =
        (DvzPointerSubscription*)dvz_malloc(sizeof(DvzPointerSubscription) * count);
    ANN(subs);
    memcpy(subs, router->pointer_subs, sizeof(DvzPointerSubscription) * count);
    for (uint32_t i = 0; i < count; i++)
    {
        DvzPointerSubscription* sub = &subs[i];
        if (sub->callback != NULL)
            sub->callback(router, event, sub->user_data);
    }
    dvz_free(subs);
}



static void _dispatch_keyboard_subs(DvzInputRouter* router, const DvzKeyboardEvent* event)
{
    ANN(router);
    ANN(event);
    if (router->keyboard_count == 0)
        return;
    uint32_t count = router->keyboard_count;
    DvzKeyboardSubscription* subs =
        (DvzKeyboardSubscription*)dvz_malloc(sizeof(DvzKeyboardSubscription) * count);
    ANN(subs);
    memcpy(subs, router->keyboard_subs, sizeof(DvzKeyboardSubscription) * count);
    for (uint32_t i = 0; i < count; i++)
    {
        DvzKeyboardSubscription* sub = &subs[i];
        if (sub->callback != NULL)
            sub->callback(router, event, sub->user_data);
    }
    dvz_free(subs);
}



static void _dispatch_resize_subs(DvzInputRouter* router, const DvzInputResizeEvent* event)
{
    ANN(router);
    ANN(event);
    if (router->resize_count == 0)
        return;
    uint32_t count = router->resize_count;
    DvzResizeSubscription* subs =
        (DvzResizeSubscription*)dvz_malloc(sizeof(DvzResizeSubscription) * count);
    ANN(subs);
    memcpy(subs, router->resize_subs, sizeof(DvzResizeSubscription) * count);
    for (uint32_t i = 0; i < count; i++)
    {
        DvzResizeSubscription* sub = &subs[i];
        if (sub->callback != NULL)
            sub->callback(router, event, sub->user_data);
    }
    dvz_free(subs);
}



static void _dispatch_scale_subs(DvzInputRouter* router, const DvzInputScaleEvent* event)
{
    ANN(router);
    ANN(event);
    if (router->scale_count == 0)
        return;
    uint32_t count = router->scale_count;
    DvzScaleSubscription* subs =
        (DvzScaleSubscription*)dvz_malloc(sizeof(DvzScaleSubscription) * count);
    ANN(subs);
    memcpy(subs, router->scale_subs, sizeof(DvzScaleSubscription) * count);
    for (uint32_t i = 0; i < count; i++)
    {
        DvzScaleSubscription* sub = &subs[i];
        if (sub->callback != NULL)
            sub->callback(router, event, sub->user_data);
    }
    dvz_free(subs);
}



static void _dispatch_event_subs(DvzInputRouter* router, const DvzInputEvent* event)
{
    ANN(router);
    ANN(event);
    if (router->event_count == 0)
        return;
    uint32_t count = router->event_count;
    DvzEventSubscription* subs =
        (DvzEventSubscription*)dvz_malloc(sizeof(DvzEventSubscription) * count);
    ANN(subs);
    memcpy(subs, router->event_subs, sizeof(DvzEventSubscription) * count);
    for (uint32_t i = 0; i < count; i++)
    {
        DvzEventSubscription* sub = &subs[i];
        if (sub->callback != NULL)
            sub->callback(router, event, sub->user_data);
    }
    dvz_free(subs);
}



static void _emit_union_pointer(DvzInputRouter* router, const DvzPointerEvent* event)
{
    DvzInputEvent ev = {0};
    ev.type = DVZ_INPUT_EVENT_POINTER;
    ev.content.pointer = *event;
    _dispatch_event_subs(router, &ev);
}



static void _emit_union_keyboard(DvzInputRouter* router, const DvzKeyboardEvent* event)
{
    DvzInputEvent ev = {0};
    ev.type = DVZ_INPUT_EVENT_KEYBOARD;
    ev.content.keyboard = *event;
    _dispatch_event_subs(router, &ev);
}



static void _emit_union_resize(DvzInputRouter* router, const DvzInputResizeEvent* event)
{
    DvzInputEvent ev = {0};
    ev.type = DVZ_INPUT_EVENT_RESIZE;
    ev.content.resize = *event;
    _dispatch_event_subs(router, &ev);
}



static void _emit_union_scale(DvzInputRouter* router, const DvzInputScaleEvent* event)
{
    DvzInputEvent ev = {0};
    ev.type = DVZ_INPUT_EVENT_SCALE;
    ev.content.scale = *event;
    _dispatch_event_subs(router, &ev);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzInputRouter* dvz_input_router(void)
{
    DvzInputRouter* router = (DvzInputRouter*)dvz_calloc(1, sizeof(DvzInputRouter));
    ANN(router);
    return router;
}



void dvz_input_router_destroy(DvzInputRouter* router)
{
    ANN(router);
    if (router->pointer_subs != NULL)
        dvz_free(router->pointer_subs);
    if (router->keyboard_subs != NULL)
        dvz_free(router->keyboard_subs);
    if (router->event_subs != NULL)
        dvz_free(router->event_subs);
    if (router->resize_subs != NULL)
        dvz_free(router->resize_subs);
    if (router->scale_subs != NULL)
        dvz_free(router->scale_subs);
    dvz_free(router);
}



void dvz_input_subscribe_pointer(
    DvzInputRouter* router, DvzPointerCallback callback, void* user_data)
{
    ANN(router);
    ANN(callback);
    _ensure_capacity(
        sizeof(DvzPointerSubscription), (void**)&router->pointer_subs, &router->pointer_capacity,
        router->pointer_count + 1);
    router->pointer_subs[router->pointer_count++] = (DvzPointerSubscription){callback, user_data};
}



void dvz_input_unsubscribe_pointer(
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



void dvz_input_emit_pointer(DvzInputRouter* router, const DvzPointerEvent* event)
{
    ANN(router);
    ANN(event);
    _dispatch_pointer_subs(router, event);
    _emit_union_pointer(router, event);
}



void dvz_input_subscribe_keyboard(
    DvzInputRouter* router, DvzKeyboardCallback callback, void* user_data)
{
    ANN(router);
    ANN(callback);
    _ensure_capacity(
        sizeof(DvzKeyboardSubscription), (void**)&router->keyboard_subs,
        &router->keyboard_capacity, router->keyboard_count + 1);
    router->keyboard_subs[router->keyboard_count++] =
        (DvzKeyboardSubscription){callback, user_data};
}



void dvz_input_unsubscribe_keyboard(
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



void dvz_input_emit_keyboard(DvzInputRouter* router, const DvzKeyboardEvent* event)
{
    ANN(router);
    ANN(event);
    _dispatch_keyboard_subs(router, event);
    _emit_union_keyboard(router, event);
}



void dvz_input_subscribe_event(DvzInputRouter* router, DvzInputCallback callback, void* user_data)
{
    ANN(router);
    ANN(callback);
    _ensure_capacity(
        sizeof(DvzEventSubscription), (void**)&router->event_subs, &router->event_capacity,
        router->event_count + 1);
    router->event_subs[router->event_count++] = (DvzEventSubscription){callback, user_data};
}



void dvz_input_unsubscribe_event(
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



void dvz_input_emit_event(DvzInputRouter* router, const DvzInputEvent* event)
{
    ANN(router);
    ANN(event);
    _dispatch_event_subs(router, event);
}
void dvz_input_subscribe_resize(
    DvzInputRouter* router, DvzResizeCallback callback, void* user_data)
{
    ANN(router);
    ANN(callback);
    _ensure_capacity(
        sizeof(DvzResizeSubscription), (void**)&router->resize_subs, &router->resize_capacity,
        router->resize_count + 1);
    router->resize_subs[router->resize_count++] = (DvzResizeSubscription){callback, user_data};
}



void dvz_input_unsubscribe_resize(
    DvzInputRouter* router, DvzResizeCallback callback, void* user_data)
{
    ANN(router);
    for (uint32_t i = 0; i < router->resize_count; i++)
    {
        DvzResizeSubscription* sub = &router->resize_subs[i];
        if (sub->callback == callback && sub->user_data == user_data)
        {
            router->resize_count--;
            if (i != router->resize_count)
            {
                router->resize_subs[i] = router->resize_subs[router->resize_count];
            }
            break;
        }
    }
}



void dvz_input_emit_resize(DvzInputRouter* router, const DvzInputResizeEvent* event)
{
    ANN(router);
    ANN(event);
    _dispatch_resize_subs(router, event);
    _emit_union_resize(router, event);
}



void dvz_input_subscribe_scale(DvzInputRouter* router, DvzScaleCallback callback, void* user_data)
{
    ANN(router);
    ANN(callback);
    _ensure_capacity(
        sizeof(DvzScaleSubscription), (void**)&router->scale_subs, &router->scale_capacity,
        router->scale_count + 1);
    router->scale_subs[router->scale_count++] = (DvzScaleSubscription){callback, user_data};
}



void dvz_input_unsubscribe_scale(
    DvzInputRouter* router, DvzScaleCallback callback, void* user_data)
{
    ANN(router);
    for (uint32_t i = 0; i < router->scale_count; i++)
    {
        DvzScaleSubscription* sub = &router->scale_subs[i];
        if (sub->callback == callback && sub->user_data == user_data)
        {
            router->scale_count--;
            if (i != router->scale_count)
            {
                router->scale_subs[i] = router->scale_subs[router->scale_count];
            }
            break;
        }
    }
}



void dvz_input_emit_scale(DvzInputRouter* router, const DvzInputScaleEvent* event)
{
    ANN(router);
    ANN(event);
    _dispatch_scale_subs(router, event);
    _emit_union_scale(router, event);
}
