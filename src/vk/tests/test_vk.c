/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing Vulkan                                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_assertions.h"

#include "test_vk.h"
#include "testing.h"



/*************************************************************************************************/
/*  Sanitizer detection                                                                          */
/*************************************************************************************************/

#ifndef DVZ_HAS_FEATURE
#if defined(__has_feature)
#define DVZ_HAS_FEATURE(x) __has_feature(x)
#else
#define DVZ_HAS_FEATURE(x) 0
#endif
#endif

#ifndef DVZ_USING_MSAN
#if DVZ_HAS_FEATURE(memory_sanitizer) || defined(__SANITIZE_MEMORY__)
#define DVZ_USING_MSAN 1
#else
#define DVZ_USING_MSAN 0
#endif
#endif

#ifndef DVZ_USING_TSAN
#if DVZ_HAS_FEATURE(thread_sanitizer) || defined(__SANITIZE_THREAD__)
#define DVZ_USING_TSAN 1
#else
#define DVZ_USING_TSAN 0
#endif
#endif



/*************************************************************************************************/
/*  Entry-point                                                                                  */
/*************************************************************************************************/

int test_vk(TstSuite* suite)
{
    ANN(suite);

    const char* tags = "vk";

    TEST_SIMPLE(test_instance_layers);
    TEST_SIMPLE(test_instance_extensions);
    TEST_SIMPLE(test_instance_creation);

    TEST_SIMPLE(test_gpu_props);
    TEST_SIMPLE(test_gpu_memprops);
    TEST_SIMPLE(test_gpu_features);
    TEST_SIMPLE(test_gpu_extensions);

    TEST_SIMPLE(test_queues_caps);
    TEST_SIMPLE(test_queues_basic);
    TEST_SIMPLE(test_queues_single_family);
    TEST_SIMPLE(test_queues_multiple);
    TEST_SIMPLE(test_queues_tie_break);
    TEST_SIMPLE(test_queues_no_optional);
    TEST_SIMPLE(test_queues_video_roles);
    TEST_SIMPLE(test_queues_queue_limits);
    TEST_SIMPLE(test_queue_from_role);
    TEST_SIMPLE(test_queue_supports);

    TEST_SIMPLE(test_device_1);
    TEST_SIMPLE(test_device_2);



    TEST_SIMPLE(test_memory_1);

    // #if HAS_CUDA && !DVZ_ENABLE_ASAN_IN_DEBUG && !DVZ_USING_MSAN && !DVZ_USING_TSAN
    // Skip CUDA interop test when sanitizers that conflict with CUDA are active.
    // #endif
    // TEST_SIMPLE(test_memory_cuda_1);
    // TEST_SIMPLE(test_memory_cuda_2);


    return 0;
}
