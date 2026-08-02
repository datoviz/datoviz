/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Input router                                                                               */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "datoviz/common/macros.h"
#include "datoviz/input/keyboard.h"
#include "datoviz/input/pointer.h"



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum DvzInputEventType
{
    DVZ_INPUT_EVENT_NONE = 0,
    DVZ_INPUT_EVENT_POINTER,
    DVZ_INPUT_EVENT_KEYBOARD,
    DVZ_INPUT_EVENT_RESIZE,
    DVZ_INPUT_EVENT_SCALE,
} DvzInputEventType;



typedef enum DvzCallbackIdSpecial
{
    DVZ_CALLBACK_ID_NONE = 0,
} DvzCallbackIdSpecial;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzInputResizeEvent DvzInputResizeEvent;
typedef struct DvzInputScaleEvent DvzInputScaleEvent;
typedef struct DvzInputEvent DvzInputEvent;
typedef struct DvzInputRouter DvzInputRouter;

typedef uint64_t DvzCallbackId;

typedef void (*DvzPointerCallback)(DvzInputRouter*, const DvzPointerEvent*, void*);
typedef void (*DvzKeyboardCallback)(DvzInputRouter*, const DvzKeyboardEvent*, void*);
typedef void (*DvzResizeCallback)(DvzInputRouter*, const DvzInputResizeEvent*, void*);
typedef void (*DvzScaleCallback)(DvzInputRouter*, const DvzInputScaleEvent*, void*);
typedef void (*DvzInputCallback)(DvzInputRouter*, const DvzInputEvent*, void*);



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzInputResizeEvent
{
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint32_t window_width;
    uint32_t window_height;
    float content_scale_x;
    float content_scale_y;
};



struct DvzInputScaleEvent
{
    float content_scale_x;
    float content_scale_y;
};



struct DvzInputEvent
{
    DvzInputEventType type;
    union
    {
        DvzPointerEvent pointer;
        DvzKeyboardEvent keyboard;
        DvzInputResizeEvent resize;
        DvzInputScaleEvent scale;
    } content;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create an input event router.
 *
 * @return a new owned router, or NULL on allocation failure; destroy it with
 * `dvz_input_router_destroy()`
 */
DVZ_EXPORT DvzInputRouter* dvz_input_router(void);



/**
 * Destroy a router.
 *
 * @param router router returned by dvz_input_router(); must not be NULL
 */
DVZ_EXPORT void dvz_input_router_destroy(DvzInputRouter* router);



/**
 * Subscribe to raw pointer events.
 *
 * These callbacks receive backend-normalized pointer position, button, and wheel events emitted
 * directly through `dvz_input_emit_pointer()`. Gesture-derived events such as click, double-click,
 * drag-start, drag, and drag-stop are emitted on the union input stream; use
 * `dvz_input_subscribe_event()` when those higher-level pointer events are needed.
 *
 * The router borrows @p user_data and passes it unchanged to @p callback. The callback and user
 * data must remain valid until the subscription is removed or the router is destroyed.
 *
 * @param router target router; must not be NULL
 * @param callback callback invoked synchronously for each raw pointer event; must not be NULL
 * @param user_data borrowed opaque pointer passed to the callback; may be NULL
 * @return subscription id, or `DVZ_CALLBACK_ID_NONE` on failure
 */
DVZ_EXPORT DvzCallbackId
dvz_input_subscribe_pointer(DvzInputRouter* router, DvzPointerCallback callback, void* user_data);



/**
 * Unsubscribe from any router callback by subscription id.
 *
 * Returns true when a callback was removed and false when @p id is `DVZ_CALLBACK_ID_NONE` or is not
 * currently registered on this router.
 * Subscriptions added during dispatch begin with the next event. A subscription removed before its
 * turn in the current dispatch is not invoked.
 *
 * @param router target router; must not be NULL
 * @param id subscription identifier returned by a subscribe function
 * @return true if the subscription was removed, false if it was not registered
 */
DVZ_EXPORT bool dvz_input_unsubscribe(DvzInputRouter* router, DvzCallbackId id);



/**
 * Emit a raw pointer event. Callbacks run synchronously on the emitting thread.
 *
 * @param router target router; must not be NULL
 * @param event event borrowed for the duration of the call; must not be NULL
 */
DVZ_EXPORT void dvz_input_emit_pointer(DvzInputRouter* router, const DvzPointerEvent* event);



/**
 * Subscribe to keyboard events.
 *
 * @param router target router; must not be NULL
 * @param callback callback invoked synchronously for each keyboard event; must not be NULL
 * @param user_data borrowed opaque pointer passed to the callback; may be NULL and must remain
 * valid until unsubscription or router destruction
 * @return subscription id, or `DVZ_CALLBACK_ID_NONE` on failure
 */
DVZ_EXPORT DvzCallbackId dvz_input_subscribe_keyboard(
    DvzInputRouter* router, DvzKeyboardCallback callback, void* user_data);



/**
 * Emit a keyboard event. Callbacks run synchronously on the emitting thread.
 *
 * @param router target router; must not be NULL
 * @param event event borrowed for the duration of the call; must not be NULL
 */
DVZ_EXPORT void dvz_input_emit_keyboard(DvzInputRouter* router, const DvzKeyboardEvent* event);



/**
 * Subscribe to resize events.
 *
 * @param router target router; must not be NULL
 * @param callback callback invoked synchronously for each resize event; must not be NULL
 * @param user_data borrowed opaque pointer passed to the callback; may be NULL and must remain
 * valid until unsubscription or router destruction
 * @return subscription id, or `DVZ_CALLBACK_ID_NONE` on failure
 */
DVZ_EXPORT DvzCallbackId
dvz_input_subscribe_resize(DvzInputRouter* router, DvzResizeCallback callback, void* user_data);



/**
 * Emit a resize event. Callbacks run synchronously on the emitting thread.
 *
 * The router copies the event and caches it for `dvz_input_router_last_resize()`.
 *
 * @param router target router; must not be NULL
 * @param event event borrowed for the duration of the call; dimensions are in pixels; must not be
 * NULL
 */
DVZ_EXPORT void
dvz_input_emit_resize(DvzInputRouter* router, const DvzInputResizeEvent* event);



/**
 * Read the most recent resize event seen by the router.
 *
 * Returns true and fills @p out when a resize has been emitted at least once,
 * false otherwise (e.g. for routers that have never had a backend resize).
 * Useful for late subscribers (e.g. controllers connected after window creation)
 * to learn the current window/framebuffer dimensions without waiting for the
 * next resize.
 *
 * @param router router to query; must not be NULL
 * @param[out] out destination receiving a copy of the cached event; must not be NULL
 * @return true if @p out was filled, false if no resize event has been emitted
 */
DVZ_EXPORT bool
dvz_input_router_last_resize(const DvzInputRouter* router, DvzInputResizeEvent* out);



/**
 * Subscribe to content scale events.
 *
 * @param router target router; must not be NULL
 * @param callback callback invoked synchronously for each scale event; must not be NULL
 * @param user_data borrowed opaque pointer passed to the callback; may be NULL and must remain
 * valid until unsubscription or router destruction
 * @return subscription id, or `DVZ_CALLBACK_ID_NONE` on failure
 */
DVZ_EXPORT DvzCallbackId
dvz_input_subscribe_scale(DvzInputRouter* router, DvzScaleCallback callback, void* user_data);



/**
 * Emit a scale event. Callbacks run synchronously on the emitting thread.
 *
 * @param router target router; must not be NULL
 * @param event event borrowed for the duration of the call; must not be NULL
 */
DVZ_EXPORT void dvz_input_emit_scale(DvzInputRouter* router, const DvzInputScaleEvent* event);



/**
 * Subscribe to routed union-style input events.
 *
 * This stream carries pointer, keyboard, resize, and scale events in one callback. When a
 * `DvzPointerGestureHandler` is attached to the router, this stream also receives
 * gesture-derived pointer events such as click, double-click, drag-start, drag, and drag-stop.
 *
 * @param router target router; must not be NULL
 * @param callback callback invoked synchronously for each union event; must not be NULL
 * @param user_data borrowed opaque pointer passed to the callback; may be NULL and must remain
 * valid until unsubscription or router destruction
 * @return subscription id, or `DVZ_CALLBACK_ID_NONE` on failure
 */
DVZ_EXPORT DvzCallbackId
dvz_input_subscribe_event(DvzInputRouter* router, DvzInputCallback callback, void* user_data);



/**
 * Emit a union input event. Callbacks run synchronously on the emitting thread.
 *
 * @param router target router; must not be NULL
 * @param event event borrowed for the duration of the call; must not be NULL
 */
DVZ_EXPORT void dvz_input_emit_event(DvzInputRouter* router, const DvzInputEvent* event);



EXTERN_C_OFF
