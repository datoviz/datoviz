/*
 * Copyright (c) 2024 Contributors. All rights reserved.
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

int test_router_callbacks(TstSuite* suite, TstItem* item);

int test_router_unsubscribe(TstSuite* suite, TstItem* item);

int test_keyboard_modifiers(TstSuite* suite, TstItem* item);

int test_pointer_gestures(TstSuite* suite, TstItem* item);

int test_pointer_wheel(TstSuite* suite, TstItem* item);

int test_resize_scale_events(TstSuite* suite, TstItem* item);



int test_input(TstSuite* suite);
