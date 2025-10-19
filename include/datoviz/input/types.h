/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Common types                                                                                 */
/*************************************************************************************************/

#pragma once


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#include "datoviz_enums.h"
#include "datoviz_keycodes.h"
#include "datoviz_math.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzKeyboardEvent DvzKeyboardEvent;
typedef struct DvzMouseEvent DvzMouseEvent;
typedef union DvzMouseEventUnion DvzMouseEventUnion;
typedef struct DvzMouseWheelEvent DvzMouseWheelEvent;
typedef struct DvzMouseDragEvent DvzMouseDragEvent;

typedef struct DvzWindowEvent DvzWindowEvent;
typedef struct DvzFrameEvent DvzFrameEvent;
typedef struct DvzGuiEvent DvzGuiEvent;
typedef struct DvzTimerEvent DvzTimerEvent;

typedef struct DvzApp DvzApp;
typedef struct DvzGuiWindow DvzGuiWindow;
typedef struct DvzTimerItem DvzTimerItem;


// Callback types.
typedef void (*DvzAppGuiCallback)(DvzApp* app, DvzId canvas_id, DvzGuiEvent* ev);
typedef void (*DvzAppMouseCallback)(DvzApp* app, DvzId window_id, DvzMouseEvent* ev);
typedef void (*DvzAppKeyboardCallback)(DvzApp* app, DvzId window_id, DvzKeyboardEvent* ev);
typedef void (*DvzAppFrameCallback)(DvzApp* app, DvzId window_id, DvzFrameEvent* ev);
typedef void (*DvzAppTimerCallback)(DvzApp* app, DvzId window_id, DvzTimerEvent* ev);
typedef void (*DvzAppResizeCallback)(DvzApp* app, DvzId window_id, DvzWindowEvent* ev);


/*************************************************************************************************/
/*  Events                                                                                       */
/*************************************************************************************************/

struct DvzKeyboardEvent
{
    DvzKeyboardEventType type;
    DvzKeyCode key;
    int mods;
    void* user_data;
};



struct DvzMouseWheelEvent
{
    vec2 dir;
};

struct DvzMouseDragEvent
{
    vec2 press_pos;
    vec2 last_pos;
    vec2 shift;          // difference between current position and press position
    bool is_press_valid; // whether the press event was valid
};

union DvzMouseEventUnion
{
    DvzMouseWheelEvent w;
    DvzMouseDragEvent d;
};

struct DvzMouseEvent
{
    DvzMouseEventType type;
    DvzMouseEventUnion content;
    vec2 pos; // current position
    DvzMouseButton button;
    int mods;
    float content_scale;
    void* user_data;
};



struct DvzWindowEvent
{
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint32_t screen_width;
    uint32_t screen_height;
    int flags;
    void* user_data;
};

struct DvzFrameEvent
{
    uint64_t frame_idx;
    double time;
    double interval;
    void* user_data;
};

struct DvzGuiEvent
{
    DvzGuiWindow* gui_window;
    void* user_data;
};

struct DvzTimerEvent
{
    uint32_t timer_idx;
    DvzTimerItem* timer_item;
    uint64_t step_idx;
    double time;
    void* user_data;
};
