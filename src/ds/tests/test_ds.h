/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing data structures                                                                      */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "testing.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_map_1(TstSuite* suite, TstItem* tstitem);

int test_map_2(TstSuite* suite, TstItem* tstitem);

int test_list_1(TstSuite* suite, TstItem* tstitem);

int test_list_remove_pointer(TstSuite* suite, TstItem* tstitem);



int test_ds(TstSuite* suite);
