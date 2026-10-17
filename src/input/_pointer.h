/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "datoviz/input/pointer.h"



/** Canonical gesture history owned by one input router. */
typedef struct DvzPointerGestureState
{
    DvzPointerState state;
    DvzPointerButton button;
    vec2 press_pos;
    vec2 last_pos;
    vec2 cur_pos;
    double time;
    double last_press;
} DvzPointerGestureState;



/**
 * Reset canonical gesture history to the released state.
 *
 * @param state router-owned gesture history
 */
void _dvz_pointer_gesture_reset(DvzPointerGestureState* state);



/**
 * Process one raw event without invoking callbacks.
 *
 * @param state router-owned gesture history
 * @param event original pointer event
 * @param[out] derived new gesture transition, or a NONE event
 */
void _dvz_pointer_gesture_process(
    DvzPointerGestureState* state, const DvzPointerEvent* event, DvzPointerEvent* derived);
