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
} DvzInputEventType;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzInputEvent DvzInputEvent;
typedef struct DvzInputRouter DvzInputRouter;

typedef void (*DvzPointerCallback)(DvzInputRouter*, const DvzPointerEvent*, void*);
typedef void (*DvzKeyboardCallback)(DvzInputRouter*, const DvzKeyboardEvent*, void*);
typedef void (*DvzInputCallback)(DvzInputRouter*, const DvzInputEvent*, void*);



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzInputEvent
{
    DvzInputEventType type;
    union
    {
        DvzPointerEvent pointer;
        DvzKeyboardEvent keyboard;
    } content;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create a new router instance.
 */
DVZ_EXPORT DvzInputRouter* dvz_input_router(void);



/**
 * Destroy a router.
 */
DVZ_EXPORT void dvz_input_router_destroy(DvzInputRouter* router);



/**
 * Subscribe to pointer events.
 */
DVZ_EXPORT void
dvz_input_subscribe_pointer(DvzInputRouter* router, DvzPointerCallback callback, void* user_data);



/**
 * Unsubscribe from pointer events.
 */
DVZ_EXPORT void dvz_input_unsubscribe_pointer(
    DvzInputRouter* router, DvzPointerCallback callback, void* user_data);



/**
 * Emit a pointer event.
 */
DVZ_EXPORT void dvz_input_emit_pointer(DvzInputRouter* router, const DvzPointerEvent* event);



/**
 * Subscribe to keyboard events.
 */
DVZ_EXPORT void dvz_input_subscribe_keyboard(
    DvzInputRouter* router, DvzKeyboardCallback callback, void* user_data);



/**
 * Unsubscribe from keyboard events.
 */
DVZ_EXPORT void dvz_input_unsubscribe_keyboard(
    DvzInputRouter* router, DvzKeyboardCallback callback, void* user_data);



/**
 * Emit a keyboard event.
 */
DVZ_EXPORT void dvz_input_emit_keyboard(DvzInputRouter* router, const DvzKeyboardEvent* event);



/**
 * Subscribe to union-style input events.
 */
DVZ_EXPORT void
dvz_input_subscribe_event(DvzInputRouter* router, DvzInputCallback callback, void* user_data);



/**
 * Unsubscribe from union-style input events.
 */
DVZ_EXPORT void
dvz_input_unsubscribe_event(DvzInputRouter* router, DvzInputCallback callback, void* user_data);



/**
 * Emit a union input event.
 */
DVZ_EXPORT void dvz_input_emit_event(DvzInputRouter* router, const DvzInputEvent* event);



EXTERN_C_OFF
