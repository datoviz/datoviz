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
#include "datoviz/math/types.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzInstance DvzInstance;
typedef struct DvzGpu DvzGpu;
typedef struct DvzDevice DvzDevice;



/*************************************************************************************************/
/*  GPU                                                                                          */
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
 * Create a logical device from a physical GPU device.
 *
 * @param gpu the GPU
 * @param[out] device the created logical device
 * @returns the creation result
 */
// DVZ_EXPORT int dvz_gpu_device(DvzGpu* gpu, DvzDevice* device);



/*************************************************************************************************/
/*  GPU properties                                                                               */
/*************************************************************************************************/

/**
 * Probe GPU properties.
 *
 * @param gpu the GPU
 */
DVZ_EXPORT void dvz_gpu_probe_properties(DvzGpu* gpu);


/**
 * Return physical device properties for Vulkan 1.0.
 *
 * @param gpu the GPU
 * @returns properties for Vulkan 1.0
 */
DVZ_EXPORT VkPhysicalDeviceProperties* dvz_gpu_properties10(DvzGpu* gpu);



/**
 * Return physical device properties for Vulkan 1.1.
 *
 * @param gpu the GPU
 * @returns properties for Vulkan 1.1
 */
DVZ_EXPORT VkPhysicalDeviceVulkan11Properties* dvz_gpu_properties11(DvzGpu* gpu);



/**
 * Return physical device properties for Vulkan 1.2.
 *
 * @param gpu the GPU
 * @returns properties for Vulkan 1.2
 */
DVZ_EXPORT VkPhysicalDeviceVulkan12Properties* dvz_gpu_properties12(DvzGpu* gpu);



/**
 * Return physical device properties for Vulkan 1.3.
 *
 * @param gpu the GPU
 * @returns properties for Vulkan 1.3
 */
DVZ_EXPORT VkPhysicalDeviceVulkan13Properties* dvz_gpu_properties13(DvzGpu* gpu);



/*************************************************************************************************/
/*  GPU memory properties                                                                        */
/*************************************************************************************************/

/**
 * Probe GPU memory properties.
 *
 * @param gpu the GPU
 */
DVZ_EXPORT void dvz_gpu_probe_memprops(DvzGpu* gpu);



/**
 * Return physical device memory properties.
 *
 * @param gpu the GPU
 * @returns memory properties
 */
DVZ_EXPORT VkPhysicalDeviceMemoryProperties* dvz_gpu_memprops(DvzGpu* gpu);



/**
 * Return the total GPU VRAM available.
 *
 * @param gpu the GPU
 * @returns the available VRAM
 */
DVZ_EXPORT DvzSize dvz_gpu_vram(DvzGpu* gpu);



/*************************************************************************************************/
/*  Device features                                                                              */
/*************************************************************************************************/

/**
 * Probe GPU features.
 *
 * @param gpu the GPU
 */
DVZ_EXPORT void dvz_gpu_probe_features(DvzGpu* gpu);



/**
 * Return physical device features for Vulkan 1.0.
 *
 * @param gpu the GPU
 * @returns features for Vulkan 1.0
 */
DVZ_EXPORT VkPhysicalDeviceFeatures* dvz_gpu_features10(DvzGpu* gpu);



/**
 * Return physical device features for Vulkan 1.1.
 *
 * @param gpu the GPU
 * @returns features for Vulkan 1.1
 */
DVZ_EXPORT VkPhysicalDeviceVulkan11Features* dvz_gpu_features11(DvzGpu* gpu);



/**
 * Return physical device features for Vulkan 1.2.
 *
 * @param gpu the GPU
 * @returns features for Vulkan 1.2
 */
DVZ_EXPORT VkPhysicalDeviceVulkan12Features* dvz_gpu_features12(DvzGpu* gpu);



/**
 * Return physical device features for Vulkan 1.3.
 *
 * @param gpu the GPU
 * @returns features for Vulkan 1.3
 */
DVZ_EXPORT VkPhysicalDeviceVulkan13Features* dvz_gpu_features13(DvzGpu* gpu);



/*************************************************************************************************/
/*  Device extensions                                                                            */
/*************************************************************************************************/

/**
 * Get the supported extensions before creating a GPU.
 *
 * !!! warning
 *     The caller needs to free the returned array AS WELL AS every string in it.
 *
 * @param gpu the GPU
 * @param[out] count the number of supported extensions
 * @returns a pointer to an array of strings
 */
DVZ_EXPORT char** dvz_gpu_supported_extensions(DvzGpu* gpu, uint32_t* count);



/**
 * Returns whether an GPU extension is supported?
 *
 * @param gpu the GPU
 * @param extension the extension name
 * @returns a boolean indicating whether this extension is supported by the GPU
 */
DVZ_EXPORT bool dvz_gpu_has_extension(DvzGpu* gpu, const char* extension);
