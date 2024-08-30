/*
* Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
* Licensed under the MIT license. See LICENSE file in the project root for details.
* SPDX-License-Identifier: MIT
*/

/*************************************************************************************************/
/*  Keyboard                                                                                     */
/*************************************************************************************************/

#include "keyboard.h"
#include "common.h"
#include "datoviz_types.h"



/*************************************************************************************************/
/*  Keyboard util functions                                                                      */
/*************************************************************************************************/

static bool _is_key_modifier(DvzKeyCode key)
{
    return (
        key == DVZ_KEY_LEFT_SHIFT || key == DVZ_KEY_RIGHT_SHIFT || key == DVZ_KEY_LEFT_CONTROL ||
        key == DVZ_KEY_RIGHT_CONTROL || key == DVZ_KEY_LEFT_ALT || key == DVZ_KEY_RIGHT_ALT ||
        key == DVZ_KEY_LEFT_SUPER || key == DVZ_KEY_RIGHT_SUPER);
}



static int _key_modifiers(int key_code)
{
    int mods = 0;
    if (key_code == DVZ_KEY_LEFT_CONTROL || key_code == DVZ_KEY_RIGHT_CONTROL)
        mods |= DVZ_KEY_MODIFIER_CONTROL;
    if (key_code == DVZ_KEY_LEFT_SHIFT || key_code == DVZ_KEY_RIGHT_SHIFT)
        mods |= DVZ_KEY_MODIFIER_SHIFT;
    if (key_code == DVZ_KEY_LEFT_ALT || key_code == DVZ_KEY_RIGHT_ALT)
        mods |= DVZ_KEY_MODIFIER_ALT;
    if (key_code == DVZ_KEY_LEFT_SUPER || key_code == DVZ_KEY_RIGHT_SUPER)
        mods |= DVZ_KEY_MODIFIER_SUPER;
    return mods;
}



static void _callbacks(DvzKeyboard* keyboard, DvzKeyboardEvent event)
{
    ANN(keyboard);
    ANN(keyboard->callbacks);

    DvzKeyboardPayload* payload = NULL;
    uint32_t n = dvz_list_count(keyboard->callbacks);
    for (uint32_t i = 0; i < n; i++)
    {
        payload = (DvzKeyboardPayload*)dvz_list_get(keyboard->callbacks, i).p;
        event.user_data = payload->user_data;
        if (payload->type == event.type)
        {
            payload->callback(keyboard, event);
        }
    }
}



/*************************************************************************************************/
/*  Keyboard functions                                                                           */
/*************************************************************************************************/

DvzKeyboard* dvz_keyboard(void)
{
    DvzKeyboard* keyboard = calloc(1, sizeof(DvzKeyboard));
    keyboard->keys = dvz_list();
    keyboard->callbacks = dvz_list();
    keyboard->mods = 0;
    return keyboard;
}



void dvz_keyboard_press(DvzKeyboard* keyboard, DvzKeyCode key)
{
    ANN(keyboard);
    ANN(keyboard->keys);

    if (_is_key_modifier(key))
    {
        keyboard->mods |= _key_modifiers(key);
    }
    else
    {
        dvz_list_append(keyboard->keys, (DvzListItem){.i = (int)key});
    }

    // Create the PRESS event struct.
    DvzKeyboardEvent ev = {.type = DVZ_KEYBOARD_EVENT_PRESS, .mods = keyboard->mods, .key = key};
    // Call the registered callbacks.
    _callbacks(keyboard, ev);
}



void dvz_keyboard_repeat(DvzKeyboard* keyboard, DvzKeyCode key)
{
    ANN(keyboard);
    ANN(keyboard->keys);

    if (_is_key_modifier(key))
    {
        keyboard->mods |= _key_modifiers(key);
    }
    else
    {
        dvz_list_append(keyboard->keys, (DvzListItem){.i = (int)key});
    }

    // Create the PRESS event struct.
    DvzKeyboardEvent ev = {.type = DVZ_KEYBOARD_EVENT_REPEAT, .mods = keyboard->mods, .key = key};
    // Call the registered callbacks.
    _callbacks(keyboard, ev);
}



void dvz_keyboard_release(DvzKeyboard* keyboard, DvzKeyCode key)
{
    ANN(keyboard);
    ANN(keyboard->keys);

    if (_is_key_modifier(key))
    {
        keyboard->mods &= ~_key_modifiers(key);
    }
    else
    {
        uint64_t idx = dvz_list_index(keyboard->keys, (int)key);
        if (idx != UINT64_MAX)
            dvz_list_remove(keyboard->keys, idx);
    }

    // Create the PRESS event struct.
    DvzKeyboardEvent ev = {.type = DVZ_KEYBOARD_EVENT_RELEASE, .mods = keyboard->mods, .key = key};
    // Call the registered callbacks.
    _callbacks(keyboard, ev);
}



DvzKeyCode dvz_keyboard_get(DvzKeyboard* keyboard, uint32_t key_idx)
{
    ANN(keyboard);
    ANN(keyboard->keys);

    if (key_idx < keyboard->keys->count)
        return (DvzKeyCode)dvz_list_get(keyboard->keys, key_idx).i;
    else
        return DVZ_KEY_NONE;
}



bool dvz_keyboard_is_pressed(DvzKeyboard* keyboard, DvzKeyCode key, int mods)
{
    ANN(keyboard);
    ANN(keyboard->keys);

    return ((keyboard->mods & mods) == mods) && dvz_list_has(keyboard->keys, key);
}



int dvz_keyboard_mods(DvzKeyboard* keyboard)
{
    ANN(keyboard);
    return keyboard->mods;
}



void dvz_keyboard_callback(
    DvzKeyboard* keyboard, DvzKeyboardEventType type, DvzKeyboardCallback callback,
    void* user_data)
{
    ANN(keyboard);
    ANN(keyboard->callbacks);

    DvzKeyboardPayload* payload = (DvzKeyboardPayload*)calloc(1, sizeof(DvzKeyboardPayload));
    payload->type = type;
    payload->callback = callback;
    payload->user_data = user_data;
    dvz_list_append(keyboard->callbacks, (DvzListItem){.p = (void*)payload});
}



void dvz_keyboard_destroy(DvzKeyboard* keyboard)
{
    ANN(keyboard);
    ANN(keyboard->callbacks);

    // Free the callback payloads.
    DvzKeyboardPayload* payload = NULL;
    for (uint32_t i = 0; i < keyboard->callbacks->count; i++)
    {
        payload = (DvzKeyboardPayload*)(dvz_list_get(keyboard->callbacks, i).p);
        ANN(payload);
        FREE(payload);
    }

    dvz_list_destroy(keyboard->callbacks);
    dvz_list_destroy(keyboard->keys);
    FREE(keyboard);
}
