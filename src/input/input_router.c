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
            if (i != router->pointer_count)
                router->pointer_subs[i] = router->pointer_subs[router->pointer_count];
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
            if (i != router->keyboard_count)
                router->keyboard_subs[i] = router->keyboard_subs[router->keyboard_count];
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
            if (i != router->text_count)
                router->text_subs[i] = router->text_subs[router->text_count];
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
            if (i != router->event_count)
                router->event_subs[i] = router->event_subs[router->event_count];
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
            if (i != router->resize_count)
                router->resize_subs[i] = router->resize_subs[router->resize_count];
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
            if (i != router->scale_count)
                router->scale_subs[i] = router->scale_subs[router->scale_count];
            return true;
        }
    }
    return false;
}



static void _dispatch_pointer_subs(DvzInputRouter* router, const DvzPointerEvent* event)
{
    ANN(router);
    ANN(event);
    if (router->pointer_count == 0)
        return;
    uint32_t count = router->pointer_count;
    DvzPointerSubscription* subs =
        (DvzPointerSubscription*)dvz_calloc(count, sizeof(DvzPointerSubscription));
    if (subs == NULL)
        return;
    size_t bytes = sizeof(DvzPointerSubscription) * count;
    dvz_memcpy(subs, bytes, router->pointer_subs, bytes);
    for (uint32_t i = 0; i < count; i++)
    {
        DvzPointerSubscription* sub = &subs[i];
        if (sub->callback != NULL && _subscription_active(router, sub->id))
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
        (DvzKeyboardSubscription*)dvz_calloc(count, sizeof(DvzKeyboardSubscription));
    if (subs == NULL)
        return;
    size_t bytes = sizeof(DvzKeyboardSubscription) * count;
    dvz_memcpy(subs, bytes, router->keyboard_subs, bytes);
    for (uint32_t i = 0; i < count; i++)
    {
        DvzKeyboardSubscription* sub = &subs[i];
        if (sub->callback != NULL && _subscription_active(router, sub->id))
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
        (DvzResizeSubscription*)dvz_calloc(count, sizeof(DvzResizeSubscription));
    if (subs == NULL)
        return;
    size_t bytes = sizeof(DvzResizeSubscription) * count;
    dvz_memcpy(subs, bytes, router->resize_subs, bytes);
    for (uint32_t i = 0; i < count; i++)
    {
        DvzResizeSubscription* sub = &subs[i];
        if (sub->callback != NULL && _subscription_active(router, sub->id))
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
        (DvzScaleSubscription*)dvz_calloc(count, sizeof(DvzScaleSubscription));
    if (subs == NULL)
        return;
    size_t bytes = sizeof(DvzScaleSubscription) * count;
    dvz_memcpy(subs, bytes, router->scale_subs, bytes);
    for (uint32_t i = 0; i < count; i++)
    {
        DvzScaleSubscription* sub = &subs[i];
        if (sub->callback != NULL && _subscription_active(router, sub->id))
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
        (DvzEventSubscription*)dvz_calloc(count, sizeof(DvzEventSubscription));
    if (subs == NULL)
        return;
    size_t bytes = sizeof(DvzEventSubscription) * count;
    dvz_memcpy(subs, bytes, router->event_subs, bytes);
    for (uint32_t i = 0; i < count; i++)
    {
        DvzEventSubscription* sub = &subs[i];
        if (sub->callback != NULL && _subscription_active(router, sub->id))
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
    return router;
}



void dvz_input_router_destroy(DvzInputRouter* router)
{
    ANN(router);
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
    _dispatch_pointer_subs(router, event);
    _emit_union_pointer(router, event);
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
    _dispatch_keyboard_subs(router, event);
    _emit_union_keyboard(router, event);
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

    uint32_t text_count = router->text_count;
    uint32_t event_count = router->event_count;
    DvzTextSubscription* text_subs = NULL;
    DvzEventSubscription* event_subs = NULL;
    if (text_count > 0)
    {
        text_subs = (DvzTextSubscription*)dvz_calloc(text_count, sizeof(DvzTextSubscription));
        if (text_subs == NULL)
            return DVZ_ERROR;
        size_t bytes = sizeof(DvzTextSubscription) * text_count;
        dvz_memcpy(text_subs, bytes, router->text_subs, bytes);
    }
    if (event_count > 0)
    {
        event_subs = (DvzEventSubscription*)dvz_calloc(event_count, sizeof(DvzEventSubscription));
        if (event_subs == NULL)
        {
            dvz_free(text_subs);
            return DVZ_ERROR;
        }
        size_t bytes = sizeof(DvzEventSubscription) * event_count;
        dvz_memcpy(event_subs, bytes, router->event_subs, bytes);
    }

    for (uint32_t i = 0; i < text_count; i++)
    {
        DvzTextSubscription* sub = &text_subs[i];
        if (sub->callback != NULL && _subscription_active(router, sub->id))
            sub->callback(router, event, sub->user_data);
    }
    DvzInputEvent union_event = {.type = DVZ_INPUT_EVENT_TEXT};
    union_event.content.text = *event;
    for (uint32_t i = 0; i < event_count; i++)
    {
        DvzEventSubscription* sub = &event_subs[i];
        if (sub->callback != NULL && _subscription_active(router, sub->id))
            sub->callback(router, &union_event, sub->user_data);
    }
    dvz_free(text_subs);
    dvz_free(event_subs);
    return DVZ_OK;
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
    _dispatch_event_subs(router, event);
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
    router->last_resize = *event;
    router->has_last_resize = true;
    _dispatch_resize_subs(router, event);
    _emit_union_resize(router, event);
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
    _dispatch_scale_subs(router, event);
    _emit_union_scale(router, event);
}
