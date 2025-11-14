/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing window module                                                                        */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "testing.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_window_headless(TstSuite* suite, TstItem* item);

int test_window_resize_events(TstSuite* suite, TstItem* item);

int test_window_frame_requests(TstSuite* suite, TstItem* item);

int test_window_fallback(TstSuite* suite, TstItem* item);



int test_window(TstSuite* suite);
