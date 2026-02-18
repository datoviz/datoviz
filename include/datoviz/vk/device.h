/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Device                                                                                       */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <vulkan/vulkan_core.h>

#include "datoviz/common/macros.h"
#include "datoviz/vk/instance.h"
#include "queues.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_MAX_DESCRIPTOR_SETS 1024



typedef struct DvzDevice DvzDevice;
typedef struct DvzDeviceQueueRequest DvzDeviceQueueRequest;
typedef struct DvzDeviceConfig DvzDeviceConfig;

struct DvzDeviceQueueRequest
{
    uint32_t family;
    uint32_t count;
};



struct DvzDeviceConfig
{
    DvzInstance* instance;
    uint32_t gpu_index;
    bool enable_canvas_extensions;
    uint32_t queue_request_count;
    DvzDeviceQueueRequest queue_requests[DVZ_MAX_QUEUE_FAMILIES];
    uint32_t extension_count;
    const char* extensions[DVZ_MAX_REQ_EXTENSIONS];
    bool has_features10;
    bool has_features11;
    bool has_features12;
    bool has_features13;
    VkPhysicalDeviceFeatures features10;
    VkPhysicalDeviceVulkan11Features features11;
    VkPhysicalDeviceVulkan12Features features12;
    VkPhysicalDeviceVulkan13Features features13;
};



/*************************************************************************************************/
/*  Device                                                                                       */
/*************************************************************************************************/

/**
 * Return default configuration values for creating a device.
 *
 * @param instance the source instance
 * @returns the default device configuration
 */
DVZ_EXPORT DvzDeviceConfig dvz_device_default_config(DvzInstance* instance);



/**
 * Select the GPU index used for device creation.
 *
 * @param cfg the device configuration
 * @param gpu_index the selected GPU index in dvz_instance_gpus()
 * @returns whether the index was stored successfully
 */
DVZ_EXPORT bool dvz_device_config_set_gpu_index(DvzDeviceConfig* cfg, uint32_t gpu_index);



/**
 * Add a queue request to a device configuration.
 *
 * @param cfg the device configuration
 * @param family the queue family index
 * @param count the number of queues requested
 * @returns whether the request was added
 */
DVZ_EXPORT bool
dvz_device_config_request_queue(DvzDeviceConfig* cfg, uint32_t family, uint32_t count);



/**
 * Add an extension request to a device configuration.
 *
 * @param cfg the device configuration
 * @param extension the extension name
 * @returns whether the extension was added
 */
DVZ_EXPORT bool
dvz_device_config_request_extension(DvzDeviceConfig* cfg, const char* extension);



/**
 * Toggle canvas extension requests on a device configuration.
 *
 * @param cfg the device configuration
 * @param enabled whether canvas extensions should be requested
 */
DVZ_EXPORT void dvz_device_config_enable_canvas_extensions(DvzDeviceConfig* cfg, bool enabled);



/**
 * Copy Vulkan 1.0 features into a device configuration.
 *
 * @param cfg the device configuration
 * @param features the Vulkan 1.0 feature struct
 */
DVZ_EXPORT void
dvz_device_config_set_features10(DvzDeviceConfig* cfg, const VkPhysicalDeviceFeatures* features);



/**
 * Copy Vulkan 1.1 features into a device configuration.
 *
 * @param cfg the device configuration
 * @param features the Vulkan 1.1 feature struct
 */
DVZ_EXPORT void dvz_device_config_set_features11(
    DvzDeviceConfig* cfg, const VkPhysicalDeviceVulkan11Features* features);



/**
 * Copy Vulkan 1.2 features into a device configuration.
 *
 * @param cfg the device configuration
 * @param features the Vulkan 1.2 feature struct
 */
DVZ_EXPORT void dvz_device_config_set_features12(
    DvzDeviceConfig* cfg, const VkPhysicalDeviceVulkan12Features* features);



/**
 * Copy Vulkan 1.3 features into a device configuration.
 *
 * @param cfg the device configuration
 * @param features the Vulkan 1.3 feature struct
 */
DVZ_EXPORT void dvz_device_config_set_features13(
    DvzDeviceConfig* cfg, const VkPhysicalDeviceVulkan13Features* features);



/**
 * Create and initialize a heap-allocated device from a configuration.
 *
 * @param cfg the device configuration
 * @returns a created device on success, `NULL` on failure
 */
DVZ_EXPORT DvzDevice* dvz_device_create(const DvzDeviceConfig* cfg);



/**
 * Get the Vulkan VkDevice handle of a device.
 *
 * @param device the device
 * @returns the Vulkan VkDevice handle
 */
DVZ_EXPORT VkDevice dvz_device_handle(DvzDevice* device);



/**
 * Return the Vulkan 1.0 feature set enabled on this device.
 *
 * @param device the device
 * @returns immutable pointer to enabled Vulkan 1.0 features
 */
DVZ_EXPORT const VkPhysicalDeviceFeatures* dvz_device_features10(DvzDevice* device);



/**
 * Retrieve a queue from a role.
 *
 * @param device the device
 * @param role the role
 * @returns the queue
 */
DVZ_EXPORT DvzQueue* dvz_device_queue(DvzDevice* device, DvzQueueRole role);



/**
 * Return the command pool associated to a queue family index.
 *
 * @param device the device
 * @param queue_family the queue family index
 * @returns the Vulkan command pool
 */
DVZ_EXPORT VkCommandPool dvz_device_command_pool(DvzDevice* device, uint32_t queue_family);



/**
 * Wait until the device is ready. Inefficient.
 *
 * @param device the device
 */
DVZ_EXPORT void dvz_device_wait(DvzDevice* device);



/**
 * Destroy a device.
 *
 * @param device
 */
DVZ_EXPORT void dvz_device_destroy(DvzDevice* device);



/**
 * Return whether a device was created with support for a given extension or not.
 *
 * @param device the device
 * @param extension the extension name
 * @returns whether the device has support for the extension
 */
DVZ_EXPORT bool dvz_device_has_extension(DvzDevice* device, const char* extension);
