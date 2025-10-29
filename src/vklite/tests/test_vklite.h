/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing vklite                                                                               */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "testing.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_vklite_commands_1(TstSuite* suite, TstItem* tstitem);

int test_vklite_sampler_1(TstSuite* suite, TstItem* tstitem);

int test_vklite_shader_1(TstSuite* suite, TstItem* tstitem);

int test_vklite_slots_1(TstSuite* suite, TstItem* tstitem);

int test_vklite_compute_1(TstSuite* suite, TstItem* tstitem);

int test_vklite_buffers_1(TstSuite* suite, TstItem* tstitem);



int test_vklite(TstSuite* suite);
