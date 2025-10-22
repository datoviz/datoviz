/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Device: instance, GPU...                                                                     */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "datoviz/common/macros.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzInstance DvzInstance;
typedef struct DvzGpu DvzGpu;

typedef struct VkInstance_T* VkInstance;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

/**
 * Get the supported layers before creating an instance.
 *
 * !!! warning
 *     This function assumes layers is already allocated with enough memory. *
 *
 * @param[out] layers a pointer to an array of strings
 * @returns the number of supported layers
 */
DVZ_EXPORT uint32_t dvz_instance_supported_layers(char** layers);



/**
 * Get the supported extensions before creating an instance.
 *
 * @param[out] extensions a pointer to an array of strings
 * @returns the number of supported extensions
 */
DVZ_EXPORT uint32_t dvz_instance_supported_extensions(char** layers);



/**
 * Set the requested layers before instance creation.
 *
 * @param instance the instance
 * @param count number of requested layers
 * @param layers array of layer names
 */
DVZ_EXPORT void dvz_instance_layers(DvzInstance* instance, uint32_t count, const char** layers);



/**
 * Set the requested extensions before instance creation.
 *
 * @param instance the instance
 * @param count number of requested extensions
 * @param extensions array of extension names
 */
DVZ_EXPORT void
dvz_instance_extensions(DvzInstance* instance, uint32_t count, const char** extensions);



/**
 * Set the instance application name and version.
 *
 * @param instance the instance
 * @param name the application name
 * @param version the application version
 */
DVZ_EXPORT void dvz_instance_info(DvzInstance* instance, const char* name, uint32_t version);



/**
 * Create the instance.
 *
 * @param instance the instance
 * @param vk_version the Vulkan API version
 * @returns the instance creation result
 */
DVZ_EXPORT int dvz_instance_create(DvzInstance* instance, uint32_t vk_version);



/**
 * Get the detected physical GPUs.
 *
 * @param instance the instance
 * @param gpus the array of detected GPUs
 * @returns the number of returned GPUs.
 */
DVZ_EXPORT uint32_t dvz_instance_gpus(DvzInstance* instance, DvzGpu* gpus);



/**
 * Return the native VkInstance for a DvzInstance.
 *
 * @param instance the Datoviz instance
 * @returns the Vulkan instance
 */
DVZ_EXPORT VkInstance dvz_instance_handle(DvzInstance* instance);



/**
 * Destroy the instance.
 *
 * @param the instance
 */
DVZ_EXPORT int dvz_instance_destroy(DvzInstance* instance);
