/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing input                                                                                */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "testing.h"



/*************************************************************************************************/
/*  Tests input                                                                                  */
/*************************************************************************************************/

int test_router_callbacks(TstContext* suite, const TstCase* item);

int test_router_unsubscribe(TstContext* suite, const TstCase* item);

int test_keyboard_modifiers(TstContext* suite, const TstCase* item);

int test_pointer_gestures(TstContext* suite, const TstCase* item);

int test_pointer_wheel(TstContext* suite, const TstCase* item);

int test_resize_scale_events(TstContext* suite, const TstCase* item);



int test_input(TstSuite* suite);
