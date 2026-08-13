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

int test_router_remove_later_callback(TstContext* suite, const TstCase* item);

int test_router_growth_failure(TstContext* suite, const TstCase* item);

int test_keyboard_modifiers(TstContext* suite, const TstCase* item);

int test_text_router_delivery(TstContext* suite, const TstCase* item);

int test_text_router_rejects_malformed_utf8(TstContext* suite, const TstCase* item);

int test_text_router_separates_keyboard_actions(TstContext* suite, const TstCase* item);

int test_text_router_subscription_mutation(TstContext* suite, const TstCase* item);

int test_pointer_gestures(TstContext* suite, const TstCase* item);

int test_pointer_gesture_invalid_sequences(TstContext* suite, const TstCase* item);

int test_pointer_wheel(TstContext* suite, const TstCase* item);

int test_pointer_emit_window_size(TstContext* suite, const TstCase* item);

int test_resize_scale_events(TstContext* suite, const TstCase* item);



int test_input(TstSuite* suite);
