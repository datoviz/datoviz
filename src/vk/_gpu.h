/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Internal GPU                                                                                */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/vk/gpu.h"
#include "datoviz/vk/instance.h"
#include "datoviz/math/types.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzGpu DvzGpu;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzGpu
{
    DvzInstance* instance;

    VkPhysicalDevice pdevice;

    VkPhysicalDeviceProperties2 props;
    VkPhysicalDeviceVulkan11Properties props11;
    VkPhysicalDeviceVulkan12Properties props12;
    VkPhysicalDeviceVulkan13Properties props13;

    VkPhysicalDeviceMemoryProperties2 memprops;

    VkPhysicalDeviceFeatures2 features;
    VkPhysicalDeviceVulkan11Features features11;
    VkPhysicalDeviceVulkan12Features features12;
    VkPhysicalDeviceVulkan13Features features13;

    uint32_t extension_count;
    char** extensions;

    DvzQueueCaps queue_caps;
};



/*************************************************************************************************/
/*  Internal GPU API                                                                            */
/*************************************************************************************************/

DvzGpu* dvz_instance_gpus(DvzInstance* instance, uint32_t* count);
void dvz_gpu_probe_properties(DvzGpu* gpu);
VkPhysicalDeviceProperties* dvz_gpu_properties10(DvzGpu* gpu);
VkPhysicalDeviceVulkan11Properties* dvz_gpu_properties11(DvzGpu* gpu);
VkPhysicalDeviceVulkan12Properties* dvz_gpu_properties12(DvzGpu* gpu);
VkPhysicalDeviceVulkan13Properties* dvz_gpu_properties13(DvzGpu* gpu);
void dvz_gpu_probe_memprops(DvzGpu* gpu);
VkPhysicalDeviceMemoryProperties* dvz_gpu_memprops(DvzGpu* gpu);
DvzSize dvz_gpu_vram(DvzGpu* gpu);
void dvz_gpu_probe_features(DvzGpu* gpu);
VkPhysicalDeviceFeatures* dvz_gpu_features10(DvzGpu* gpu);
VkPhysicalDeviceVulkan11Features* dvz_gpu_features11(DvzGpu* gpu);
VkPhysicalDeviceVulkan12Features* dvz_gpu_features12(DvzGpu* gpu);
VkPhysicalDeviceVulkan13Features* dvz_gpu_features13(DvzGpu* gpu);
void dvz_gpu_probe_extensions(DvzGpu* gpu);
char** dvz_gpu_supported_extensions(DvzGpu* gpu, uint32_t* count);
bool dvz_gpu_has_extension(DvzGpu* gpu, const char* extension);
