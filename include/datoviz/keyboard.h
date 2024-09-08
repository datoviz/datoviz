/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Keyboard                                                                                     */
/*************************************************************************************************/

#ifndef DVZ_HEADER_KEYBOARD
#define DVZ_HEADER_KEYBOARD



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_list.h"
#include "datoviz_types.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

// Maximum number of simultaneously pressed keys.
#define DVZ_KEYBOARD_MAX_KEYS 8



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzKeyboard DvzKeyboard;
typedef struct DvzKeyboardPayload DvzKeyboardPayload;

typedef void (*DvzKeyboardCallback)(DvzKeyboard* keyboard, DvzKeyboardEvent ev);



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzKeyboardPayload
{
    DvzKeyboardEventType type;
    DvzKeyboardCallback callback;
    void* user_data;
};



struct DvzKeyboard
{
    DvzList* keys;
    int mods;

    DvzList* callbacks;

    // double press_time;
    bool is_active;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Keyboard functions                                                                           */
/*************************************************************************************************/

DvzKeyboard* dvz_keyboard(void);



void dvz_keyboard_press(DvzKeyboard* keyboard, DvzKeyCode key);



void dvz_keyboard_repeat(DvzKeyboard* keyboard, DvzKeyCode key);



void dvz_keyboard_release(DvzKeyboard* keyboard, DvzKeyCode key);



DvzKeyCode dvz_keyboard_get(DvzKeyboard* keyboard, uint32_t key_idx);



bool dvz_keyboard_is_pressed(DvzKeyboard* keyboard, DvzKeyCode key, int mods);



int dvz_keyboard_mods(DvzKeyboard* keyboard);



void dvz_keyboard_callback(
    DvzKeyboard* keyboard, DvzKeyboardEventType type, DvzKeyboardCallback callback,
    void* user_data);



void dvz_keyboard_destroy(DvzKeyboard* keyboard);



EXTERN_C_OFF

#endif
