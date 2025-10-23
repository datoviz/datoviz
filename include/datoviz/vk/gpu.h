/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  GPU                                                                                          */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include <vulkan/vulkan.h>

#include "datoviz/common/macros.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzInstance DvzInstance;
typedef struct DvzGpu DvzGpu;
typedef struct DvzDevice DvzDevice;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Get the detected physical GPUs.
 *
 * @param instance the instance
 * @param[out] count a pointer the number of returned GPUs
 * @returns the array of detected GPUs
 */
DVZ_EXPORT DvzGpu* dvz_instance_gpus(DvzInstance* instance, uint32_t* count);



/**
 * Return physical device properties for Vulkan 1.0.
 *
 * @param gpu
 * @returns properties for Vulkan 1.0
 */
DVZ_EXPORT VkPhysicalDeviceProperties dvz_gpu_properties10(DvzGpu* gpu);



/**
 * Return physical device properties for Vulkan 1.1.
 *
 * @param gpu
 * @returns properties for Vulkan 1.1
 */
DVZ_EXPORT VkPhysicalDeviceVulkan11Properties dvz_gpu_properties11(DvzGpu* gpu);



/**
 * Return physical device properties for Vulkan 1.2.
 *
 * @param gpu
 * @returns properties for Vulkan 1.2
 */
DVZ_EXPORT VkPhysicalDeviceVulkan12Properties dvz_gpu_properties12(DvzGpu* gpu);



/**
 * Return physical device properties for Vulkan 1.3.
 *
 * @param gpu
 * @returns properties for Vulkan 1.3
 */
DVZ_EXPORT VkPhysicalDeviceVulkan13Properties dvz_gpu_properties13(DvzGpu* gpu);



/**
 * Return physical device memory properties.
 *
 * @param gpu
 * @returns memory properties
 */
DVZ_EXPORT VkPhysicalDeviceMemoryProperties dvz_gpu_memprops(DvzGpu* gpu);



DVZ_EXPORT char** dvz_gpu_supported_extensions(DvzGpu* gpu, uint32_t* count);



DVZ_EXPORT bool dvz_gpu_has_extension(DvzGpu* gpu, const char* extension);



DVZ_EXPORT void dvz_gpu_extensions(DvzGpu* gpu, uint32_t count, const char** extensions);



DVZ_EXPORT void dvz_gpu_extension(DvzGpu* gpu, const char* extension);



DVZ_EXPORT char** dvz_gpu_supported_features(DvzGpu* gpu, uint32_t* count);



DVZ_EXPORT bool dvz_gpu_has_feature(DvzGpu* gpu, const char* feature);



DVZ_EXPORT void dvz_gpu_features(DvzGpu* gpu, uint32_t count, const char** features);



DVZ_EXPORT void dvz_gpu_feature(DvzGpu* gpu, const char* feature);



DVZ_EXPORT int dvz_gpu_device(DvzGpu* gpu, DvzDevice* device);
