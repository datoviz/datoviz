/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Keyboard events                                                                             */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "datoviz/common/macros.h"
#include "datoviz/common/types.h"
#include "datoviz/input/enums.h"
#include "datoviz/input/keycodes.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzInputRouter DvzInputRouter;
typedef struct DvzKeyboardModifierState DvzKeyboardModifierState;
typedef struct DvzKeyboardEvent DvzKeyboardEvent;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzKeyboardEvent
{
    DvzKeyboardEventType type;
    DvzKeyCode key;
    int mods;
    void* user_data;
};



struct DvzKeyboardModifierState
{
    int mods;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return the modifier bit mask for a key.
 *
 * @param key key code to classify
 * @return the corresponding `DvzKeyboardModifiers` bit, or zero for a non-modifier key
 */
DVZ_EXPORT int dvz_keyboard_modifier_bit(DvzKeyCode key);



/**
 * Create a modifier tracker.
 *
 * @return a new owned zero-initialized tracker, or NULL on allocation failure; destroy it with
 * `dvz_keyboard_modifier_state_destroy()`
 */
DVZ_EXPORT DvzKeyboardModifierState* dvz_keyboard_modifier_state(void);



/**
 * Destroy a modifier tracker.
 *
 * @param state owned tracker to destroy; may be NULL
 */
DVZ_EXPORT void dvz_keyboard_modifier_state_destroy(DvzKeyboardModifierState* state);



/**
 * Update the modifier tracker with a keyboard event.
 *
 * @param state tracker to update; must not be NULL
 * @param type press, repeat, or release event type
 * @param key modifier key to update
 * @return DVZ_OK on success, DVZ_ERROR on validation error
 */
DVZ_EXPORT DvzResult dvz_keyboard_modifier_state_update(
    DvzKeyboardModifierState* state, DvzKeyboardEventType type, DvzKeyCode key);



/**
 * Return the current modifier mask.
 *
 * @param state tracker to query; must not be NULL
 * @return bitwise combination of `DvzKeyboardModifiers` values
 */
DVZ_EXPORT int dvz_keyboard_modifier_state_mods(const DvzKeyboardModifierState* state);



/**
 * Emit a keyboard event on the router.
 *
 * The constructed event borrows @p user_data; callbacks run synchronously before this function
 * returns.
 *
 * @param router target router; must not be NULL
 * @param type keyboard event type
 * @param key backend-normalized key code
 * @param mods bitwise combination of `DvzKeyboardModifiers` values
 * @param user_data opaque pointer stored in the event; may be NULL
 */
DVZ_EXPORT void dvz_keyboard_emit(
    DvzInputRouter* router, DvzKeyboardEventType type, DvzKeyCode key, int mods, void* user_data);



EXTERN_C_OFF
