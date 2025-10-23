/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing GPU                                                                                  */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <float.h>
#include <inttypes.h>
#include <stdbool.h>

#include <vulkan/vulkan_core.h>

#include "../types.h"
#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "datoviz/math/types.h"
#include "datoviz/vk/gpu.h"
#include "datoviz/vk/instance.h"
#include "test_vk.h"
#include "testing.h"



/*************************************************************************************************/
/*  GPU tests                                                                                    */
/*************************************************************************************************/

int test_gpu_props(TstSuite* suite, TstItem* tstitem)
{

    ANN(suite);
    ANN(tstitem);

    // Create an instance.
    DvzInstance instance = {0};
    dvz_instance(&instance, DVZ_INSTANCE_VALIDATION_FLAGS);
    dvz_instance_create(&instance, VK_API_VERSION_1_3);

    uint32_t count = 0;
    DvzGpu* gpus = dvz_instance_gpus(&instance, &count);
    DvzGpu* gpu = &gpus[0];

    dvz_gpu_probe_properties(gpu);

    VkPhysicalDeviceProperties* props = dvz_gpu_properties10(gpu);
    log_info("device ID: %u", props->deviceID);
    log_info("device name: %s", props->deviceName);
    log_info("device type: %u", props->deviceType);
    log_info("API version: %u", props->apiVersion);
    log_info("driver version: %u", props->driverVersion);
    log_info("vendor ID: %u", props->vendorID);
    log_info("max image dim 2D: %u", props->limits.maxImageDimension2D);

    VkPhysicalDeviceVulkan11Properties* props11 = dvz_gpu_properties11(gpu);
    log_info("max memory allocation size: %s", dvz_pretty_size(props11->maxMemoryAllocationSize));

    VkPhysicalDeviceVulkan12Properties* props12 = dvz_gpu_properties12(gpu);
    log_info(
        "max descriptor set update after bind samplers: %u",
        props12->maxDescriptorSetUpdateAfterBindSamplers);

    VkPhysicalDeviceVulkan13Properties* props13 = dvz_gpu_properties13(gpu);
    log_info("max buffer size: %s", dvz_pretty_size(props13->maxBufferSize));

    dvz_instance_destroy(&instance);
    return 0;
}



int test_gpu_memprops(TstSuite* suite, TstItem* tstitem)
{

    ANN(suite);
    ANN(tstitem);

    // Create an instance.
    DvzInstance instance = {0};
    dvz_instance(&instance, DVZ_INSTANCE_VALIDATION_FLAGS);
    dvz_instance_create(&instance, VK_API_VERSION_1_3);

    uint32_t count = 0;
    DvzGpu* gpus = dvz_instance_gpus(&instance, &count);
    DvzGpu* gpu = &gpus[0];

    dvz_gpu_probe_memprops(gpu);
    VkPhysicalDeviceMemoryProperties* memprops = dvz_gpu_memprops(gpu);

    log_info("========== Memory Heaps ==========");
    for (uint32_t i = 0; i < memprops->memoryHeapCount; i++)
    {
        VkMemoryHeap* h = &memprops->memoryHeaps[i];
        log_info(
            "Heap %u: size=%s %s", i, dvz_pretty_size(h->size),
            (h->flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) ? "DEVICE_LOCAL" : "");
    }

    log_info("========== Memory Types ==========");
    for (uint32_t i = 0; i < memprops->memoryTypeCount; i++)
    {
        VkMemoryType* t = &memprops->memoryTypes[i];
        VkMemoryPropertyFlags f = t->propertyFlags;
        char s[128] = "";
        if (f & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
            strcat(s, "DEVICE_LOCAL ");
        if (f & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
            strcat(s, "HOST_VISIBLE ");
        if (f & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
            strcat(s, "HOST_COHERENT ");
        if (f & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
            strcat(s, "HOST_CACHED ");
        if (f & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
            strcat(s, "LAZILY_ALLOCATED ");
        log_info("Type %2u: heap=%u  %s", i, t->heapIndex, s);
    }

    dvz_instance_destroy(&instance);
    return 0;
}
