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
#include "_log.h"
#include "datoviz/vk/instance.h"

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

/**
 * Probe whether the current runtime can create a Vulkan instance for vk test coverage.
 *
 * @returns true when the runtime can create a Vulkan instance, false otherwise
 */
static bool _vk_runtime_available(void)
{
    DvzInstanceConfig cfg = dvz_instance_default_config();
    cfg.flags = 0;
    DvzInstance* instance = dvz_instance_create(&cfg);
    if (instance == NULL)
    {
        log_warn("vk tests skipped because Vulkan instance creation failed");
        return false;
    }
    dvz_instance_destroy(instance);
    return true;
}



static int test_vk_runtime_unavailable(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    log_warn("vk tests skipped because Vulkan runtime is unavailable");
    return 0;
}



int test_vk(TstSuite* suite)
{
    ANN(suite);

    const char* tags = "vk";
    TST_MODULE(suite, tags);
    if (!_vk_runtime_available())
    {
        TST_CASE(test_vk_runtime_unavailable);
        return 0;
    }

    TST_CASE(test_instance_layers);
    TST_CASE(test_instance_extensions);
    TST_CASE(test_instance_creation);
    TST_CASE(test_instance_invalid_layer);

    TST_CASE(test_gpu_props);
    TST_CASE(test_gpu_memprops);
    TST_CASE(test_gpu_features);
    TST_CASE(test_gpu_extensions);

    TST_CASE(test_queues_caps);
    TST_CASE(test_queues_basic);
    TST_CASE(test_queues_single_family);
    TST_CASE(test_queues_multiple);
    TST_CASE(test_queues_tie_break);
    TST_CASE(test_queues_no_optional);
    TST_CASE(test_queues_video_roles);
    TST_CASE(test_queues_queue_limits);
    TST_CASE(test_queue_from_role);
    TST_CASE(test_queue_supports);

    TST_CASE(test_device_1);
    TST_CASE(test_device_2);
    TST_CASE(test_device_3);
    TST_CASE(test_device_4);
    TST_CASE(test_device_destroy_rebuild);
    TST_CASE(test_device_build_requires_destroy);



    TST_CASE(test_memory_1);
    TST_CASE(test_memory_interop_buffer_export);

#if DVZ_HAS_CUDA && !DVZ_ENABLE_ASAN_IN_DEBUG && !DVZ_USING_MSAN && !DVZ_USING_TSAN
    // Skip CUDA interop tests when sanitizers that conflict with CUDA are active.
    TST_CASE(test_memory_cuda_1);
#endif


    return 0;
}
