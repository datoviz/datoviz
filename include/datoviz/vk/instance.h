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
#include "datoviz/vk/vulkan.h"

#include "datoviz/common/macros.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

// Maximum number of requested layers/extensions.
#define DVZ_MAX_REQ_LAYERS     32
#define DVZ_MAX_GPUS           8
#define DVZ_MAX_REQ_EXTENSIONS 256



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzInstance DvzInstance;
typedef struct DvzInstanceConfig DvzInstanceConfig;

typedef struct VkInstance_T* VkInstance;



struct DvzInstanceConfig
{
    uint32_t struct_size;
    uint32_t flags;
    uint32_t vk_version;
    const char* app_name;
    uint32_t app_version;
    bool portability;
    uint32_t layer_count;
    const char* layers[DVZ_MAX_REQ_LAYERS];
    uint32_t extension_count;
    const char* extensions[DVZ_MAX_REQ_EXTENSIONS];
};



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Instance flags.
typedef enum
{
    DVZ_INSTANCE_VALIDATION_FLAGS = 0x01,
} DvzInstanceFlags;



EXTERN_C_ON

/*************************************************************************************************/
/*  Instance                                                                                     */
/*************************************************************************************************/

/**
 * Return default configuration values for creating an instance.
 *
 * @return the default instance configuration
 */
DVZ_EXPORT DvzInstanceConfig dvz_instance_config(void);



/**
 * Add an instance layer request to a configuration.
 *
 * @param cfg the instance configuration
 * @param layer the layer name
 * @return whether the layer was added to the request list
 */
DVZ_EXPORT bool dvz_instance_config_request_layer(DvzInstanceConfig* cfg, const char* layer);



/**
 * Add an instance extension request to a configuration.
 *
 * @param cfg the instance configuration
 * @param extension the extension name
 * @return whether the extension was added to the request list
 */
DVZ_EXPORT bool
dvz_instance_config_request_extension(DvzInstanceConfig* cfg, const char* extension);



/**
 * Create and initialize a heap-allocated instance from a configuration.
 *
 * @param cfg the instance configuration
 * @return a created instance on success, `NULL` on failure
 */
DVZ_EXPORT DvzInstance* dvz_instance_create(const DvzInstanceConfig* cfg);



/**
 * Return the native VkInstance for a DvzInstance.
 *
 * @param instance the Datoviz instance
 * @return the Vulkan instance
 */
DVZ_EXPORT VkInstance dvz_instance_handle(DvzInstance* instance);



/**
 * Destroy the instance.
 *
 * @param instance the instance
 */
DVZ_EXPORT void dvz_instance_destroy(DvzInstance* instance);



/**
 * Return the validation error counter accumulated by an instance.
 *
 * @param instance the instance
 * @return the number of validation errors reported through the debug callback
 */
DVZ_EXPORT uint32_t dvz_instance_error_count(DvzInstance* instance);



/*************************************************************************************************/
/*  Layers                                                                                       */
/*************************************************************************************************/

/**
 * Probe instance layers.
 *
 * @param instance the instance
 */
DVZ_EXPORT void dvz_instance_probe_layers(DvzInstance* instance);



/**
 * Get the supported layers before creating an instance.
 *
 * The returned string array is borrowed instance-owned storage. It remains valid until the next
 * layer probe on the same instance or instance destruction. Callers must not mutate or free the
 * array or strings.
 *
 * @param instance the instance
 * @param[out] count the number of supported layers
 * @return borrowed string array owned by `instance`, valid until layers are reprobed or the
 * instance is destroyed
 */
DVZ_EXPORT char** dvz_instance_supported_layers(DvzInstance* instance, uint32_t* count);



/**
 * Returns whether an instance layer is supported on the system?
 *
 * @param instance the instance
 * @param layer the layer name
 * @return a boolean indicating whether this layer is supported
 */
DVZ_EXPORT bool dvz_instance_has_layer(DvzInstance* instance, const char* layer);



/*************************************************************************************************/
/*  Extensions                                                                                   */
/*************************************************************************************************/

/**
 * Probe instance extensions.
 *
 * @param instance the instance
 */
DVZ_EXPORT void dvz_instance_probe_extensions(DvzInstance* instance);



/**
 * Get the supported extensions before creating an instance.
 *
 * The returned string array is borrowed instance-owned storage. It remains valid until the next
 * extension probe on the same instance or instance destruction. Callers must not mutate or free the
 * array or strings.
 *
 * @param instance the instance
 * @param[out] count the number of supported extensions
 * @return borrowed string array owned by `instance`, valid until extensions are reprobed or the
 * instance is destroyed
 */
DVZ_EXPORT char** dvz_instance_supported_extensions(DvzInstance* instance, uint32_t* count);



/**
 * Returns whether an instance extension is supported on the system?
 *
 * @param instance the instance
 * @param extension the extension name
 * @return a boolean indicating whether this extension is supported
 */
DVZ_EXPORT bool dvz_instance_has_extension(DvzInstance* instance, const char* extension);



EXTERN_C_OFF
