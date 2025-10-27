/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  GPU                                                                                          */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include <volk.h>

#include "_alloc.h"
#include "_compat.h"
#include "datoviz/vk/gpu.h"
#include "datoviz/vk/instance.h"
#include "macros.h"
#include "types.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

// Consistency check.
#define MAX_COUNT 1024



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzGpu* dvz_instance_gpus(DvzInstance* instance, uint32_t* count)
{
    ANN(instance);
    ANN(count);

    // Count the number of GPUs.
    VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance->vk_instance, &instance->gpu_count, NULL));
    log_trace("found %d GPU(s)", instance->gpu_count);

    if (instance->gpu_count == 0)
        return NULL;
    ASSERT(instance->gpu_count > 0);

    // Hard-coded limitation.
    if (instance->gpu_count >= DVZ_MAX_GPUS)
    {
        log_warn(
            "only the first %d GPUs out of %d are available", DVZ_MAX_GPUS, instance->gpu_count);
        instance->gpu_count = DVZ_MAX_GPUS;
    }

    // Get the GPUs.
    VkPhysicalDevice* pdevices =
        (VkPhysicalDevice*)dvz_calloc(instance->gpu_count, sizeof(VkPhysicalDevice));
    ANN(pdevices);
    VK_CHECK_RESULT(
        vkEnumeratePhysicalDevices(instance->vk_instance, &instance->gpu_count, pdevices));

    // Populate the DvzGpu structures, only with pdevice field for now.
    for (uint32_t i = 0; i < instance->gpu_count; i++)
    {
        instance->gpus[i].instance = instance;
        instance->gpus[i].pdevice = pdevices[i];
    }

    dvz_free(pdevices);

    return instance->gpus;
}



// // TODO: move to device.c
// int dvz_gpu_device(DvzGpu* gpu, DvzDevice* device)
// {
//     ANN(gpu);
//     // TODO: create the logical device

//     // NOTE: recreate the underlying vulkan logical device if called multiple times (destroying
//     the
//     // old one first), which allows to change the queues before

//     VK_CHECK_RESULT(vkCreateDevice(gpu->pdevice, &device_info, NULL, &device->vk_device));

//     return 0;
// }



/*************************************************************************************************/
/*  GPU properties                                                                               */
/*************************************************************************************************/

void dvz_gpu_probe_properties(DvzGpu* gpu)
{
    ANN(gpu);

    gpu->props11 = (VkPhysicalDeviceVulkan11Properties){
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES,
    };
    gpu->props12 = (VkPhysicalDeviceVulkan12Properties){
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES,
        .pNext = &gpu->props11,
    };
    gpu->props13 = (VkPhysicalDeviceVulkan13Properties){
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES,
        .pNext = &gpu->props12,
    };

    gpu->props = (VkPhysicalDeviceProperties2){
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        .pNext = &gpu->props13,
    };

    vkGetPhysicalDeviceProperties2(gpu->pdevice, &gpu->props);
}



VkPhysicalDeviceProperties* dvz_gpu_properties10(DvzGpu* gpu)
{
    ANN(gpu);
    return &gpu->props.properties;
}



VkPhysicalDeviceVulkan11Properties* dvz_gpu_properties11(DvzGpu* gpu)
{
    ANN(gpu);
    return &gpu->props11;
}



VkPhysicalDeviceVulkan12Properties* dvz_gpu_properties12(DvzGpu* gpu)
{
    ANN(gpu);
    return &gpu->props12;
}



VkPhysicalDeviceVulkan13Properties* dvz_gpu_properties13(DvzGpu* gpu)
{
    ANN(gpu);
    return &gpu->props13;
}



/*************************************************************************************************/
/*  GPU memory properties                                                                        */
/*************************************************************************************************/

void dvz_gpu_probe_memprops(DvzGpu* gpu)
{
    ANN(gpu);

    gpu->memprops = (VkPhysicalDeviceMemoryProperties2){
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2,
        .pNext = NULL,
    };

    vkGetPhysicalDeviceMemoryProperties2(gpu->pdevice, &gpu->memprops);
}



VkPhysicalDeviceMemoryProperties* dvz_gpu_memprops(DvzGpu* gpu)
{
    ANN(gpu);
    if (gpu->memprops.sType == 0)
    {
        dvz_gpu_probe_memprops(gpu);
    }
    return &gpu->memprops.memoryProperties;
}



DvzSize dvz_gpu_vram(DvzGpu* gpu)
{
    ANN(gpu);
    DvzSize vram = 0;

    VkMemoryHeap* heap = NULL;
    for (uint32_t j = 0; j < gpu->memprops.memoryProperties.memoryHeapCount; j++)
    {
        heap = &gpu->memprops.memoryProperties.memoryHeaps[j];
        if (heap->size > 0 && ((heap->flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0))
        {
            ASSERT(heap->size > 0);
            vram += heap->size;
        }
    }

    return vram;
}



/*************************************************************************************************/
/*  Device features                                                                              */
/*************************************************************************************************/

void dvz_gpu_probe_features(DvzGpu* gpu)
{
    ANN(gpu);

    gpu->features11 = (VkPhysicalDeviceVulkan11Features){
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
    };
    gpu->features12 = (VkPhysicalDeviceVulkan12Features){
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .pNext = &gpu->features11,
    };
    gpu->features13 = (VkPhysicalDeviceVulkan13Features){
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = &gpu->features12,
    };

    gpu->features = (VkPhysicalDeviceFeatures2){
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &gpu->features13,
    };

    vkGetPhysicalDeviceFeatures2(gpu->pdevice, &gpu->features);
}



VkPhysicalDeviceFeatures* dvz_gpu_features10(DvzGpu* gpu)
{
    ANN(gpu);
    return &gpu->features.features;
}



VkPhysicalDeviceVulkan11Features* dvz_gpu_features11(DvzGpu* gpu)
{
    ANN(gpu);
    return &gpu->features11;
}



VkPhysicalDeviceVulkan12Features* dvz_gpu_features12(DvzGpu* gpu)
{
    ANN(gpu);
    return &gpu->features12;
}



VkPhysicalDeviceVulkan13Features* dvz_gpu_features13(DvzGpu* gpu)
{
    ANN(gpu);
    return &gpu->features13;
}



/*************************************************************************************************/
/*  Device extensions                                                                            */
/*************************************************************************************************/

void dvz_gpu_probe_extensions(DvzGpu* gpu)
{
    ANN(gpu);

    if (gpu->extension_count > 0)
        return;

    // Get the number of gpu extensions.
    VkResult res =
        vkEnumerateDeviceExtensionProperties(gpu->pdevice, NULL, &gpu->extension_count, NULL);
    if (res != VK_SUCCESS || gpu->extension_count == 0)
        return;

    // Get the names of the gpu extensions.
    ASSERT(gpu->extension_count > 0);
    ASSERT(gpu->extension_count < MAX_COUNT); // consistency check
    VkExtensionProperties* props = (VkExtensionProperties*)dvz_calloc(
        (size_t)gpu->extension_count, sizeof(VkExtensionProperties));
    if (!props)
        return;

    res = vkEnumerateDeviceExtensionProperties(gpu->pdevice, NULL, &gpu->extension_count, props);
    if (res != VK_SUCCESS)
    {
        dvz_free(props);
        return;
    }
    ASSERT(gpu->extension_count > 0);

    // Allocate the array of strings.
    gpu->extensions = (char**)dvz_calloc((size_t)gpu->extension_count, sizeof(char*));
    for (uint32_t i = 0; i < gpu->extension_count; i++)
    {
        // Allocate the string.
        gpu->extensions[i] = (char*)dvz_calloc(VK_MAX_EXTENSION_NAME_SIZE, sizeof(char));
        ANN(gpu->extensions[i]);
        ANN(props[i].extensionName);
        ASSERT(
            strnlen(props[i].extensionName, VK_MAX_EXTENSION_NAME_SIZE) <
            VK_MAX_EXTENSION_NAME_SIZE - 1);

        // Fill in the string.
        (void)dvz_snprintf(
            gpu->extensions[i], VK_MAX_EXTENSION_NAME_SIZE, "%s", props[i].extensionName);
    }

    dvz_free(props);
}



char** dvz_gpu_supported_extensions(DvzGpu* gpu, uint32_t* count)
{
    ANN(gpu);
    ANN(count);
    if (gpu->extension_count == 0)
    {
        dvz_gpu_probe_extensions(gpu);
    }
    *count = gpu->extension_count;
    return gpu->extensions;
}



bool dvz_gpu_has_extension(DvzGpu* gpu, const char* extension)
{
    ANN(gpu);
    ANN(extension);
    if (gpu->extension_count == 0)
    {
        dvz_gpu_probe_extensions(gpu);
    }
    return dvz_strings_contains(gpu->extension_count, gpu->extensions, extension);
}
