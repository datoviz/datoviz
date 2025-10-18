/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing keyboard                                                                             */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_keyboard.h"
#include "keyboard.h"
#include "test.h"
#include "testing.h"



/*************************************************************************************************/
/*  Keyboard tests                                                                               */
/*************************************************************************************************/

int test_keyboard_1(TstSuite* suite, TstItem* tstitem)
{
    DvzKeyboard* keyboard = dvz_keyboard();

    AT(dvz_keyboard_get(keyboard, 0) == DVZ_KEY_NONE);


    // Press and release A.
    dvz_keyboard_press(keyboard, DVZ_KEY_A);
    AT(dvz_keyboard_get(keyboard, 0) == DVZ_KEY_A);
    AT(dvz_keyboard_get(keyboard, 1) == DVZ_KEY_NONE);

    dvz_keyboard_release(keyboard, DVZ_KEY_A);
    AT(dvz_keyboard_get(keyboard, 0) == DVZ_KEY_NONE);
    AT(dvz_keyboard_get(keyboard, 1) == DVZ_KEY_NONE);


    // Press and release CTRL A.
    dvz_keyboard_press(keyboard, DVZ_KEY_LEFT_CONTROL);
    AT(dvz_keyboard_get(keyboard, 0) == DVZ_KEY_NONE);
    AT(dvz_keyboard_mods(keyboard) == DVZ_KEY_MODIFIER_CONTROL);

    dvz_keyboard_press(keyboard, DVZ_KEY_A);
    AT(dvz_keyboard_get(keyboard, 0) == DVZ_KEY_A);
    AT(dvz_keyboard_mods(keyboard) == DVZ_KEY_MODIFIER_CONTROL);

    dvz_keyboard_release(keyboard, DVZ_KEY_A);
    dvz_keyboard_release(keyboard, DVZ_KEY_LEFT_CONTROL);


    // Destroy the resources.
    dvz_keyboard_destroy(keyboard);
    return 0;
}



static void _on_key_press(DvzKeyboard* keyboard, DvzKeyboardEvent* ev)
{
    ANN(keyboard);
    log_debug("key press %d, modifiers %d", ev->key, ev->mods);
    DvzList* keys = keyboard->keys;
    uint32_t n = dvz_list_count(keys);
    DvzKeyCode k[4] = {0};
    for (uint32_t i = 0; i < MIN(4, n); i++)
        k[i] = (DvzKeyCode)dvz_list_get(keys, i).i;
    log_debug("%d key(s) pressed: %d %d %d %d", n, k[0], k[1], k[2], k[3]);
    if (ev->user_data != NULL)
    {
        *((int*)ev->user_data) = (int)ev->key;
    }
}

static void _on_key_release(DvzKeyboard* keyboard, DvzKeyboardEvent* ev)
{
    ANN(keyboard);
    log_debug("key release %d", ev->key);
    if (ev->user_data != NULL)
    {
        *((int*)ev->user_data) = (int)DVZ_KEY_NONE;
    }
}

int test_keyboard_2(TstSuite* suite, TstItem* tstitem)
{
    DvzKeyboard* keyboard = dvz_keyboard();


    // Register keyboard callbacks.
    int res = 0;
    dvz_keyboard_callback(keyboard, DVZ_KEYBOARD_EVENT_PRESS, _on_key_press, &res);
    dvz_keyboard_callback(keyboard, DVZ_KEYBOARD_EVENT_RELEASE, _on_key_release, &res);
    AT(dvz_keyboard_get(keyboard, 0) == DVZ_KEY_NONE);


    // Press and release A.
    AT(res == 0);
    dvz_keyboard_press(keyboard, DVZ_KEY_A);
    AT(res == (int)DVZ_KEY_A);
    AT(dvz_keyboard_get(keyboard, 0) == DVZ_KEY_A);
    AT(dvz_keyboard_get(keyboard, 1) == DVZ_KEY_NONE);

    dvz_keyboard_release(keyboard, DVZ_KEY_A);
    AT(dvz_keyboard_get(keyboard, 0) == DVZ_KEY_NONE);
    AT(dvz_keyboard_get(keyboard, 1) == DVZ_KEY_NONE);


    // Press and release CTRL A.
    dvz_keyboard_press(keyboard, DVZ_KEY_LEFT_CONTROL);
    AT(res == (int)DVZ_KEY_LEFT_CONTROL);
    AT(dvz_keyboard_get(keyboard, 0) == DVZ_KEY_NONE);
    AT(dvz_keyboard_mods(keyboard) == DVZ_KEY_MODIFIER_CONTROL);

    dvz_keyboard_press(keyboard, DVZ_KEY_A);
    AT(res == (int)DVZ_KEY_A);
    AT(dvz_keyboard_get(keyboard, 0) == DVZ_KEY_A);
    AT(dvz_keyboard_mods(keyboard) == DVZ_KEY_MODIFIER_CONTROL);

    dvz_keyboard_release(keyboard, DVZ_KEY_A);
    dvz_keyboard_release(keyboard, DVZ_KEY_LEFT_CONTROL);


    // Destroy the resources.
    dvz_keyboard_destroy(keyboard);
    return 0;
}
