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
            strlcat(s, "DEVICE_LOCAL ", 64);
        if (f & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
            strlcat(s, "HOST_VISIBLE ", 64);
        if (f & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
            strlcat(s, "HOST_COHERENT ", 64);
        if (f & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
            strlcat(s, "HOST_CACHED ", 64);
        if (f & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
            strlcat(s, "LAZILY_ALLOCATED ", 64);
        log_info("Type %2u: heap=%u  %s", i, t->heapIndex, s);
    }

    log_info("total VRAM: %s", dvz_pretty_size(dvz_gpu_vram(gpu)));

    dvz_instance_destroy(&instance);
    return 0;
}



int test_gpu_features(TstSuite* suite, TstItem* tstitem)
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

    dvz_gpu_probe_features(gpu);

    VkPhysicalDeviceFeatures* features = dvz_gpu_features10(gpu);
    log_info("geometry shader: %b", features->geometryShader);

    VkPhysicalDeviceVulkan11Features* features11 = dvz_gpu_features11(gpu);
    log_info("sampler Ycbcr conversion: %b", features11->samplerYcbcrConversion);

    VkPhysicalDeviceVulkan12Features* features12 = dvz_gpu_features12(gpu);
    log_info("draw indirect count: %b", features12->drawIndirectCount);

    VkPhysicalDeviceVulkan13Features* features13 = dvz_gpu_features13(gpu);
    log_info("dynamic rendering: %b", features13->dynamicRendering);

    dvz_instance_destroy(&instance);
    return 0;
}



int test_gpu_extensions(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    // Create an instance.
    DvzInstance instance = {0};
    dvz_instance(&instance, DVZ_INSTANCE_VALIDATION_FLAGS);
    dvz_instance_create(&instance, VK_API_VERSION_1_3);

    uint32_t count = 0;
    DvzGpu* gpus = dvz_instance_gpus(&instance, &count);

    // Probe GPU extensions.
    DvzGpu* gpu = &gpus[0];
    dvz_gpu_probe_extensions(gpu);

    // Call the function under test.
    uint32_t ext_count = 0;
    char** extensions = dvz_gpu_supported_extensions(gpu, &ext_count);
    log_info("Found %u supported GPU instance extensions:", ext_count);

    for (uint32_t i = 0; i < ext_count; i++)
    {
        log_info("  [%02u] %s", i, extensions[i]);
    }

    // Free the array of strings.
    dvz_free_strings(count, extensions);

    dvz_instance_destroy(&instance);
    return 0;
}
