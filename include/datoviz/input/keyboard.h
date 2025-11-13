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
 */
DVZ_EXPORT int dvz_keyboard_modifier_bit(DvzKeyCode key);



/**
 * Create a modifier tracker.
 */
DVZ_EXPORT DvzKeyboardModifierState* dvz_keyboard_modifier_state(void);



/**
 * Destroy a modifier tracker.
 */
DVZ_EXPORT void dvz_keyboard_modifier_state_destroy(DvzKeyboardModifierState* state);



/**
 * Update the modifier tracker with a keyboard event.
 */
DVZ_EXPORT void dvz_keyboard_modifier_state_update(
    DvzKeyboardModifierState* state, DvzKeyboardEventType type, DvzKeyCode key);



/**
 * Return the current modifier mask.
 */
DVZ_EXPORT int dvz_keyboard_modifier_state_mods(const DvzKeyboardModifierState* state);



/**
 * Emit a keyboard event on the router.
 */
DVZ_EXPORT void dvz_keyboard_emit(
    DvzInputRouter* router, DvzKeyboardEventType type, DvzKeyCode key, int mods, void* user_data);



EXTERN_C_OFF
