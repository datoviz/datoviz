/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing object                                                                               */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "testing.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_obj_1(TstContext* suite, const TstCase* tstitem);
int test_alloc_basic(TstContext* suite, const TstCase* tstitem);
int test_alloc_aligned(TstContext* suite, const TstCase* tstitem);
int test_time_monotonic_ns(TstContext* suite, const TstCase* tstitem);



int test_common(TstSuite* suite);
