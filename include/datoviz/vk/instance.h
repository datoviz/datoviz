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
#include <vulkan/vulkan_core.h>

#include "datoviz/common/macros.h"
#include "datoviz/common/obj.h"
#include "datoviz/vk/gpu.h"

MUTE_ON
#define VMA_EXTERNAL_MEMORY 1
#include "vk_mem_alloc.h"
MUTE_OFF



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
typedef struct DvzGpu DvzGpu;
typedef struct DvzInstanceConfig DvzInstanceConfig;

typedef struct VkInstance_T* VkInstance;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzInstance
{
    DvzObject obj;
    bool is_heap_allocated;

    VkInstance vk_instance;
    uint32_t vk_version;

    // Instance creation structures.
    bool flags;
    bool portability;
    VkInstanceCreateInfo info_inst;
    VkDebugUtilsMessengerCreateInfoEXT info_debug;
    VkValidationFeaturesEXT validation_features;

    // Requested layers and extensions.
    uint32_t req_layer_count;
    uint32_t req_extension_count;
    char* req_layers[DVZ_MAX_REQ_LAYERS];
    char* req_extensions[DVZ_MAX_REQ_EXTENSIONS];

    // All supported instance layers and extensions.
    uint32_t layer_count;
    uint32_t extension_count;
    char** layers;
    char** extensions;

    // Application info.
    char* name;
    uint32_t version;

    // Validation.
    VkDebugUtilsMessengerEXT debug_messenger;
    uint32_t n_errors;

    // GPUs
    uint32_t gpu_count;
    DvzGpu gpus[DVZ_MAX_GPUS];
};



struct DvzInstanceConfig
{
    int flags;
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



/*************************************************************************************************/
/*  Instance                                                                                     */
/*************************************************************************************************/

/**
 * Return default configuration values for creating an instance.
 *
 * @returns the default instance configuration
 */
DVZ_EXPORT DvzInstanceConfig dvz_instance_default_config(void);



/**
 * Add an instance layer request to a configuration.
 *
 * @param cfg the instance configuration
 * @param layer the layer name
 * @returns whether the layer was added to the request list
 */
DVZ_EXPORT bool dvz_instance_config_request_layer(DvzInstanceConfig* cfg, const char* layer);



/**
 * Add an instance extension request to a configuration.
 *
 * @param cfg the instance configuration
 * @param extension the extension name
 * @returns whether the extension was added to the request list
 */
DVZ_EXPORT bool
dvz_instance_config_request_extension(DvzInstanceConfig* cfg, const char* extension);



/**
 * Create and initialize a heap-allocated instance from a configuration.
 *
 * @param cfg the instance configuration
 * @returns a created instance on success, `NULL` on failure
 */
DVZ_EXPORT DvzInstance* dvz_instance_create_from_config(const DvzInstanceConfig* cfg);



/**
 * Initialize an instance.
 *
 * @param instance the instance
 * @param flags the instance flags
 */
DVZ_EXPORT void dvz_instance(DvzInstance* instance, int flags);



/**
 * Set the instance application name and version.
 *
 * @param instance the instance
 * @param name the application name
 * @param version the application version
 */
DVZ_EXPORT void dvz_instance_info(DvzInstance* instance, const char* name, uint32_t version);



/**
 * Set the instance portability enumeration extension/creation flag if the system supports it.
 *
 * @param instance the instance
 */
DVZ_EXPORT void dvz_instance_portability(DvzInstance* instance);



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
 * @param instance the instance
 * @param[out] count the number of supported layers
 * @returns a pointer to an array of strings
 */
DVZ_EXPORT char** dvz_instance_supported_layers(DvzInstance* instance, uint32_t* count);



/**
 * Returns whether an instance layer is supported on the system?
 *
 * @param instance the instance
 * @param layer the layer name
 * @returns a boolean indicating whether this layer is supported
 */
DVZ_EXPORT bool dvz_instance_has_layer(DvzInstance* instance, const char* layer);



/**
 * Add an instance layer.
 *
 * @param instance the instance
 * @param layer the layer name
 */
DVZ_EXPORT void dvz_instance_request_layer(DvzInstance* instance, const char* layer);



/**
 * Set the requested layers before instance creation.
 *
 * @param instance the instance
 * @param count number of requested layers
 * @param layers array of layer names
 */
// DVZ_EXPORT void dvz_instance_layers(DvzInstance* instance, uint32_t count, const char** layers);



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
 * @param instance the instance
 * @param[out] count the number of supported extensions
 * @returns a pointer to an array of strings
 */
DVZ_EXPORT char** dvz_instance_supported_extensions(DvzInstance* instance, uint32_t* count);



/**
 * Returns whether an instance extension is supported on the system?
 *
 * @param instance the instance
 * @param extension the extension name
 * @returns a boolean indicating whether this extension is supported
 */
DVZ_EXPORT bool dvz_instance_has_extension(DvzInstance* instance, const char* extension);



/**
 * Add an instance extension.
 *
 * @param instance the instance
 * @param extension the extension name.
 */
DVZ_EXPORT void dvz_instance_request_extension(DvzInstance* instance, const char* extension);



/**
 * Set the requested extensions before instance creation.
 *
 * @param instance the instance
 * @param count number of requested extensions
 * @param extensions array of extension names
 */
// DVZ_EXPORT void
// dvz_instance_extensions(DvzInstance* instance, uint32_t count, const char** extensions);
