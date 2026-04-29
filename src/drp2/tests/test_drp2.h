/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing DRP2                                                                                 */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "testing.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_drp2_stream_empty(TstSuite* suite, TstItem* item);

int test_drp2_stream_append(TstSuite* suite, TstItem* item);

int test_drp2_stream_json(TstSuite* suite, TstItem* item);



int test_drp2(TstSuite* suite);
