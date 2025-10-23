/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing Vulkan                                                                               */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "testing.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_instance_layers(TstSuite* suite, TstItem* tstitem);

int test_instance_extensions(TstSuite* suite, TstItem* tstitem);

int test_instance_creation(TstSuite* suite, TstItem* tstitem);



int test_gpu_props(TstSuite* suite, TstItem* tstitem);



int test_vk(TstSuite* suite);
