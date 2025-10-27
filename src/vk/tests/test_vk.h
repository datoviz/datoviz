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

int test_gpu_memprops(TstSuite* suite, TstItem* tstitem);

int test_gpu_features(TstSuite* suite, TstItem* tstitem);

int test_gpu_extensions(TstSuite* suite, TstItem* tstitem);



int test_queues_caps(TstSuite* suite, TstItem* tstitem);

int test_queue_supports(TstSuite* suite, TstItem* tstitem);

int test_queue_from_role(TstSuite* suite, TstItem* tstitem);

int test_queues_basic(TstSuite* suite, TstItem* tstitem);

int test_queues_multiple(TstSuite* suite, TstItem* tstitem);

int test_queues_tie_break(TstSuite* suite, TstItem* tstitem);

int test_queues_no_optional(TstSuite* suite, TstItem* tstitem);

int test_queues_video_roles(TstSuite* suite, TstItem* tstitem);



int test_device_1(TstSuite* suite, TstItem* tstitem);

int test_device_2(TstSuite* suite, TstItem* tstitem);



int test_memory_1(TstSuite* suite, TstItem* tstitem);

int test_memory_cuda_1(TstSuite* suite, TstItem* tstitem);

int test_memory_cuda_2(TstSuite* suite, TstItem* tstitem);

int test_vk(TstSuite* suite);
