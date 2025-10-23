/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Instance                                                                                     */
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

// Instance flags.
typedef enum
{
    DVZ_INSTANCE_VALIDATION_FLAGS = 0x01,
} DvzInstanceFlags;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Initialize an instance.
 *
 * @param instance the instance
 * @param flags the instance flags
 */
DVZ_EXPORT void dvz_instance(DvzInstance* instance, int flags);



/**
 * Get the supported layers before creating an instance.
 *
 * !!! warning
 *     The caller needs to free the returned array AS WELL AS every string in it.
 *
 * @param[out] count the number of supported layers
 * @returns a pointer to an array of strings
 */
DVZ_EXPORT char** dvz_instance_supported_layers(uint32_t* count);



/**
 * Returns whether an instance layer is supported on the system?
 *
 * @param layer the layer name.
 * @returns a boolean indicating whether this layer is supported.
 */
DVZ_EXPORT bool dvz_instance_has_layer(const char* layer);



/**
 * Get the supported extensions before creating an instance.
 *
 * !!! warning
 *     The caller needs to free the returned array AS WELL AS every string in it.
 *
 * @param[out] count the number of supported extensions
 * @returns a pointer to an array of strings
 */
DVZ_EXPORT char** dvz_instance_supported_extensions(uint32_t* count);



/**
 * Returns whether an instance extension is supported on the system?
 *
 * @param extension the extension name.
 * @returns a boolean indicating whether this extension is supported.
 */
DVZ_EXPORT bool dvz_instance_has_extension(const char* extension);



/**
 * Set the instance portability enumeration extension/creation flag if the system supports it.
 *
 * @param instance the instance
 */
DVZ_EXPORT void dvz_instance_portability(DvzInstance* instance);



/**
 * Add an instance layer.
 *
 * @param instance the instance
 * @param layer the layer name
 */
DVZ_EXPORT void dvz_instance_layer(DvzInstance* instance, const char* layer);



/**
 * Set the requested layers before instance creation.
 *
 * @param instance the instance
 * @param count number of requested layers
 * @param layers array of layer names
 */
DVZ_EXPORT void dvz_instance_layers(DvzInstance* instance, uint32_t count, const char** layers);



/**
 * Add an instance extension.
 *
 * @param instance the instance
 * @param extension the extension name.
 */
DVZ_EXPORT void dvz_instance_extension(DvzInstance* instance, const char* extension);



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
 * Set up validation before instance creation.
 *
 * @param instance the instance.
 */
DVZ_EXPORT void dvz_instance_validation_pre(DvzInstance* instance);



/**
 * Set up validation after instance creation.
 *
 * @param instance the instance.
 */
DVZ_EXPORT void dvz_instance_validation_post(DvzInstance* instance);



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
 * @param instance the instance
 */
DVZ_EXPORT void dvz_instance_destroy(DvzInstance* instance);
