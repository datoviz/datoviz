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

#include <inttypes.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "../_gpu.h"
#include "datoviz/math/types.h"
#include "datoviz/vk/instance.h"
#include "datoviz_testing.h"
#include "test_vk.h"
#include "testing.h"
#include "vulkan_core.h"



/*************************************************************************************************/
/*  GPU tests                                                                                    */
/*************************************************************************************************/

int test_gpu_ctx_config_validation_default(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzGpuCtxConfig cfg = dvz_gpu_ctx_config();
#if ENABLE_VALIDATION_LAYERS
    AT(cfg.enable_validation);
#else
    AT(!cfg.enable_validation);
#endif

    dvz_gpu_ctx_config_validation(&cfg, true);
    AT(cfg.enable_validation);

    return 0;
}



int test_gpu_props(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    // Create an instance.
    DvzInstanceConfig cfg = dvz_instance_config();
    cfg.flags = DVZ_INSTANCE_VALIDATION_FLAGS;
    DvzInstance* instance = dvz_instance_create(&cfg);
    AT(instance != NULL);

    uint32_t count = 0;
    DvzGpu* gpus = dvz_instance_gpus(instance, &count);
    const uint32_t gpu_index = dvz_testing_gpu_index(suite);
    AT(gpu_index < count);
    DvzGpu* gpu = &gpus[gpu_index];

    dvz_gpu_probe_properties(gpu);

    VkPhysicalDeviceProperties* props = dvz_gpu_properties10(gpu);
    DvzGpuInfo selected = {0};
    AT(dvz_testing_gpu_info(suite, &selected));
    AT(selected.index == gpu_index);
    AT(selected.vendor_id == props->vendorID);
    AT(selected.device_id == props->deviceID);
    AT(strcmp(selected.name, props->deviceName) == 0);
    log_debug("device ID: %u", props->deviceID);
    log_debug("device name: %s", props->deviceName);
    log_debug("device type: %u", props->deviceType);
    log_debug("API version: %u", props->apiVersion);
    log_debug("driver version: %u", props->driverVersion);
    log_debug("vendor ID: %u", props->vendorID);
    log_debug("max image dim 2D: %u", props->limits.maxImageDimension2D);

    VkPhysicalDeviceVulkan11Properties* props11 = dvz_gpu_properties11(gpu);
    char allocation_size_str[64] = {0};
    log_debug(
        "max memory allocation size: %s",
        dvz_pretty_size(
            props11->maxMemoryAllocationSize, allocation_size_str, sizeof(allocation_size_str)));

    VkPhysicalDeviceVulkan12Properties* props12 = dvz_gpu_properties12(gpu);
    log_debug(
        "max descriptor set update after bind samplers: %u",
        props12->maxDescriptorSetUpdateAfterBindSamplers);

    VkPhysicalDeviceVulkan13Properties* props13 = dvz_gpu_properties13(gpu);
    char buffer_size_str[64] = {0};
    log_debug(
        "max buffer size: %s",
        dvz_pretty_size(props13->maxBufferSize, buffer_size_str, sizeof(buffer_size_str)));

    dvz_instance_destroy(instance);

    DvzGpuCtxConfig gpu_config = dvz_testing_gpu_ctx_config(suite);
    DvzGpuCtx* gpu_ctx = dvz_gpu_ctx(&gpu_config);
    AT(gpu_ctx != NULL);
    DvzGpuInfo context_info = {0};
    AT(dvz_gpu_ctx_gpu_info(gpu_ctx, &context_info));
    AT(context_info.index == selected.index);
    AT(context_info.vendor_id == selected.vendor_id);
    AT(context_info.device_id == selected.device_id);
    AT(strcmp(context_info.name, selected.name) == 0);
    dvz_gpu_ctx_destroy(gpu_ctx);
    return 0;
}



int test_gpu_memprops(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    // Create an instance.
    DvzInstanceConfig cfg = dvz_instance_config();
    cfg.flags = DVZ_INSTANCE_VALIDATION_FLAGS;
    DvzInstance* instance = dvz_instance_create(&cfg);
    AT(instance != NULL);

    uint32_t count = 0;
    DvzGpu* gpus = dvz_instance_gpus(instance, &count);
    const uint32_t gpu_index = dvz_testing_gpu_index(suite);
    AT(gpu_index < count);
    DvzGpu* gpu = &gpus[gpu_index];

    dvz_gpu_probe_memprops(gpu);
    VkPhysicalDeviceMemoryProperties* memprops = dvz_gpu_memprops(gpu);

    log_debug("========== Memory Heaps ==========");
    for (uint32_t i = 0; i < memprops->memoryHeapCount; i++)
    {
        VkMemoryHeap* h = &memprops->memoryHeaps[i];
        char heap_size_str[64] = {0};
        log_debug(
            "Heap %u: size=%s %s", i,
            dvz_pretty_size(h->size, heap_size_str, sizeof(heap_size_str)),
            (h->flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) ? "DEVICE_LOCAL" : "");
    }

    log_debug("========== Memory Types ==========");
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
        log_debug("Type %2u: heap=%u  %s", i, t->heapIndex, s);
    }

    char vram_size_str[64] = {0};
    log_debug(
        "total VRAM: %s",
        dvz_pretty_size(dvz_gpu_vram(gpu), vram_size_str, sizeof(vram_size_str)));

    dvz_instance_destroy(instance);
    return 0;
}



int test_gpu_features(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    // Create an instance.
    DvzInstanceConfig cfg = dvz_instance_config();
    cfg.flags = DVZ_INSTANCE_VALIDATION_FLAGS;
    DvzInstance* instance = dvz_instance_create(&cfg);
    AT(instance != NULL);

    uint32_t count = 0;
    DvzGpu* gpus = dvz_instance_gpus(instance, &count);
    const uint32_t gpu_index = dvz_testing_gpu_index(suite);
    AT(gpu_index < count);
    DvzGpu* gpu = &gpus[gpu_index];

    dvz_gpu_probe_features(gpu);

    VkPhysicalDeviceFeatures* features = dvz_gpu_features10(gpu);
    log_debug("geometry shader: %d", features->geometryShader);

    VkPhysicalDeviceVulkan11Features* features11 = dvz_gpu_features11(gpu);
    log_debug("sampler Ycbcr conversion: %d", features11->samplerYcbcrConversion);

    VkPhysicalDeviceVulkan12Features* features12 = dvz_gpu_features12(gpu);
    log_debug("draw indirect count: %d", features12->drawIndirectCount);

    VkPhysicalDeviceVulkan13Features* features13 = dvz_gpu_features13(gpu);
    log_debug("dynamic rendering: %d", features13->dynamicRendering);

    dvz_instance_destroy(instance);
    return 0;
}



int test_gpu_extensions(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    // Create an instance.
    DvzInstanceConfig cfg = dvz_instance_config();
    cfg.flags = DVZ_INSTANCE_VALIDATION_FLAGS;
    DvzInstance* instance = dvz_instance_create(&cfg);
    AT(instance != NULL);

    uint32_t count = 0;
    DvzGpu* gpus = dvz_instance_gpus(instance, &count);

    // Probe GPU extensions.
    const uint32_t gpu_index = dvz_testing_gpu_index(suite);
    AT(gpu_index < count);
    DvzGpu* gpu = &gpus[gpu_index];
    dvz_gpu_probe_extensions(gpu);

    // Call the function under test.
    uint32_t ext_count = 0;
    char** extensions = dvz_gpu_supported_extensions(gpu, &ext_count);
    log_debug("Found %u supported GPU instance extensions:", ext_count);

    for (uint32_t i = 0; i < ext_count; i++)
    {
        log_debug("  [%02u] %s", i, extensions[i]);
    }

    // NOTE: `extensions` is owned by `gpu` and released in dvz_instance_destroy().

    dvz_instance_destroy(instance);
    return 0;
}
