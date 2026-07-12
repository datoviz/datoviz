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

#include "datoviz/vk/instance.h"
#include "datoviz/vk/gpu_ctx.h"
#include "testing.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_instance_layers(TstContext* suite, const TstCase* tstitem);

int test_instance_extensions(TstContext* suite, const TstCase* tstitem);

int test_instance_creation(TstContext* suite, const TstCase* tstitem);

int test_instance_validation_features(TstContext* suite, const TstCase* tstitem);

int test_instance_invalid_layer(TstContext* suite, const TstCase* tstitem);



int test_gpu_props(TstContext* suite, const TstCase* tstitem);

int test_gpu_memprops(TstContext* suite, const TstCase* tstitem);

int test_gpu_features(TstContext* suite, const TstCase* tstitem);

int test_gpu_extensions(TstContext* suite, const TstCase* tstitem);



int test_queues_caps(TstContext* suite, const TstCase* tstitem);

int test_queue_supports(TstContext* suite, const TstCase* tstitem);

int test_queue_from_role(TstContext* suite, const TstCase* tstitem);

int test_queues_basic(TstContext* suite, const TstCase* tstitem);

int test_queues_single_family(TstContext* suite, const TstCase* tstitem);

int test_queues_multiple(TstContext* suite, const TstCase* tstitem);

int test_queues_tie_break(TstContext* suite, const TstCase* tstitem);

int test_queues_no_optional(TstContext* suite, const TstCase* tstitem);

int test_queues_video_roles(TstContext* suite, const TstCase* tstitem);

int test_queues_queue_limits(TstContext* suite, const TstCase* tstitem);



int test_device_1(TstContext* suite, const TstCase* tstitem);

int test_device_2(TstContext* suite, const TstCase* tstitem);

int test_device_3(TstContext* suite, const TstCase* tstitem);

int test_device_4(TstContext* suite, const TstCase* tstitem);

int test_device_destroy_rebuild(TstContext* suite, const TstCase* tstitem);

int test_device_build_requires_destroy(TstContext* suite, const TstCase* tstitem);



int test_memory_1(TstContext* suite, const TstCase* tstitem);

int test_memory_interop_buffer_export(TstContext* suite, const TstCase* tstitem);

int test_memory_cuda_1(TstContext* suite, const TstCase* tstitem);

int test_memory_cuda_2(TstContext* suite, const TstCase* tstitem);

int test_vk(TstSuite* suite);
