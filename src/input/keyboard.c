/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */
#include <stdbool.h>

#include "_alloc.h"
#include "_assertions.h"
#include "datoviz/input/keyboard.h"
#include "datoviz/input/router.h"

/**
 * Check whether a key code maps to a modifier.
 *
 * @param key key code to test
 * @return true when the key represents a modifier
 */
static bool _is_key_modifier(DvzKeyCode key)
{
    return (
        key == DVZ_KEY_LEFT_SHIFT || key == DVZ_KEY_RIGHT_SHIFT || key == DVZ_KEY_LEFT_CONTROL ||
        key == DVZ_KEY_RIGHT_CONTROL || key == DVZ_KEY_LEFT_ALT || key == DVZ_KEY_RIGHT_ALT ||
        key == DVZ_KEY_LEFT_SUPER || key == DVZ_KEY_RIGHT_SUPER);
}


/**
 * Return the modifier bit mask for a key.
 */
DVZ_EXPORT int dvz_keyboard_modifier_bit(DvzKeyCode key)
{
    int mods = 0;
    if (key == DVZ_KEY_LEFT_CONTROL || key == DVZ_KEY_RIGHT_CONTROL)
        mods |= DVZ_KEY_MODIFIER_CONTROL;
    if (key == DVZ_KEY_LEFT_SHIFT || key == DVZ_KEY_RIGHT_SHIFT)
        mods |= DVZ_KEY_MODIFIER_SHIFT;
    if (key == DVZ_KEY_LEFT_ALT || key == DVZ_KEY_RIGHT_ALT)
        mods |= DVZ_KEY_MODIFIER_ALT;
    if (key == DVZ_KEY_LEFT_SUPER || key == DVZ_KEY_RIGHT_SUPER)
        mods |= DVZ_KEY_MODIFIER_SUPER;
    return mods;
}


/**
 * Create a modifier tracker.
 */
DVZ_EXPORT DvzKeyboardModifierState* dvz_keyboard_modifier_state(void)
{
    DvzKeyboardModifierState* state =
        (DvzKeyboardModifierState*)dvz_calloc(1, sizeof(DvzKeyboardModifierState));
    ANN(state);
    return state;
}


/**
 * Destroy a modifier tracker.
 */
DVZ_EXPORT void dvz_keyboard_modifier_state_destroy(DvzKeyboardModifierState* state)
{
    if (state == NULL)
        return;
    dvz_free(state);
}


/**
 * Update the modifier tracker with a keyboard event.
 */
DVZ_EXPORT void dvz_keyboard_modifier_state_update(
    DvzKeyboardModifierState* state, DvzKeyboardEventType type, DvzKeyCode key)
{
    ANN(state);
    if (!_is_key_modifier(key))
        return;
    int mask = dvz_keyboard_modifier_bit(key);
    if (type == DVZ_KEYBOARD_EVENT_PRESS || type == DVZ_KEYBOARD_EVENT_REPEAT)
        state->mods |= mask;
    else if (type == DVZ_KEYBOARD_EVENT_RELEASE)
        state->mods &= ~mask;
}


/**
 * Return the current modifier mask.
 */
DVZ_EXPORT int dvz_keyboard_modifier_state_mods(const DvzKeyboardModifierState* state)
{
    ANN(state);
    return state->mods;
}


/**
 * Emit a keyboard event on the router.
 */
DVZ_EXPORT void dvz_keyboard_emit(
    DvzInputRouter* router,
    DvzKeyboardEventType type,
    DvzKeyCode key,
    int mods,
    void* user_data)
{
    ANN(router);
    DvzKeyboardEvent event = {0};
    event.type = type;
    event.key = key;
    event.mods = mods;
    event.user_data = user_data;
    dvz_input_emit_keyboard(router, &event);
}
