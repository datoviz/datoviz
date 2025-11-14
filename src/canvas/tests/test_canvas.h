/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing canvas module                                                                        */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "testing.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_canvas_defaults(TstSuite* suite, TstItem* item);

int test_canvas_frame_pool(TstSuite* suite, TstItem* item);

int test_canvas_timings(TstSuite* suite, TstItem* item);



int test_canvas(TstSuite* suite);
