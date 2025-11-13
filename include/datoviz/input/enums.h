/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Input enums                                                                                  */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Keyboard mods
// NOTE: must match GLFW values! no mapping is done as of now
typedef enum
{
    DVZ_KEY_MODIFIER_NONE = 0x00000000,
    DVZ_KEY_MODIFIER_SHIFT = 0x00000001,
    DVZ_KEY_MODIFIER_CONTROL = 0x00000002,
    DVZ_KEY_MODIFIER_ALT = 0x00000004,
    DVZ_KEY_MODIFIER_SUPER = 0x00000008,
} DvzKeyboardModifiers;



// Keyboard event type (press or release)
typedef enum
{
    DVZ_KEYBOARD_EVENT_NONE,
    DVZ_KEYBOARD_EVENT_PRESS,
    DVZ_KEYBOARD_EVENT_REPEAT,
    DVZ_KEYBOARD_EVENT_RELEASE,
} DvzKeyboardEventType;



// Pointer buttons
// WARNING: these don't match to GLFW_MOUSE_BUTTON because the left button is 0 which
// is inconvenient. Use _from_glfw_button() from input.c if needed.
typedef enum
{
    DVZ_POINTER_BUTTON_NONE = 0,
    DVZ_POINTER_BUTTON_LEFT = 1,
    DVZ_POINTER_BUTTON_MIDDLE = 2,
    DVZ_POINTER_BUTTON_RIGHT = 3,
} DvzPointerButton;



// Pointer states.
typedef enum
{
    DVZ_POINTER_STATE_RELEASE = 0,
    DVZ_POINTER_STATE_PRESS = 1,
    DVZ_POINTER_STATE_CLICK = 3,
    DVZ_POINTER_STATE_CLICK_PRESS = 4,
    DVZ_POINTER_STATE_DOUBLE_CLICK = 5,
    DVZ_POINTER_STATE_DRAGGING = 11,
} DvzPointerState;



// Pointer events.
typedef enum
{
    DVZ_POINTER_EVENT_NONE = -1,
    DVZ_POINTER_EVENT_RELEASE = 0,
    DVZ_POINTER_EVENT_PRESS = 1,
    DVZ_POINTER_EVENT_MOVE = 2,
    DVZ_POINTER_EVENT_CLICK = 3,
    DVZ_POINTER_EVENT_DOUBLE_CLICK = 5,
    DVZ_POINTER_EVENT_DRAG_START = 10,
    DVZ_POINTER_EVENT_DRAG = 11,
    DVZ_POINTER_EVENT_DRAG_STOP = 12,
    DVZ_POINTER_EVENT_WHEEL = 20,
    DVZ_POINTER_EVENT_ALL = 255,
} DvzPointerEventType;
