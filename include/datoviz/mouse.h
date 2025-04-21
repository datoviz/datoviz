/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Mouse                                                                                        */
/*************************************************************************************************/

#ifndef DVZ_HEADER_MOUSE
#define DVZ_HEADER_MOUSE



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_list.h"
#include "datoviz_types.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_MOUSE_CLICK_MAX_DELAY        0.25
#define DVZ_MOUSE_CLICK_MAX_SHIFT        5
#define DVZ_MOUSE_DOUBLE_CLICK_MAX_DELAY 0.2
#define DVZ_MOUSE_MOVE_MAX_PENDING       16
#define DVZ_MOUSE_MOVE_MIN_DELAY         0.01



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzMouse DvzMouse;
typedef struct DvzMousePayload DvzMousePayload;

typedef void (*DvzMouseCallback)(DvzMouse* mouse, DvzMouseEvent ev);



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzMousePayload
{
    DvzMouseEventType type;
    DvzMouseCallback callback;
    void* user_data;
};



struct DvzMouse
{
    DvzList* callbacks;
    bool is_active;

    DvzMouseState state;
    DvzMouseButton button;
    vec2 press_pos, last_pos, cur_pos, wheel_delta;
    double time, last_press, last_click;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Mouse functions                                                                              */
/*************************************************************************************************/

void dvz_mouse_tick(DvzMouse* mouse, double time);



void dvz_mouse_callback(
    DvzMouse* mouse, DvzMouseEventType type, DvzMouseCallback callback, void* user_data);



EXTERN_C_OFF

#endif
