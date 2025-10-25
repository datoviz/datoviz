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

#include <stdint.h>
#include <vulkan/vulkan.h>

#include "datoviz/common/macros.h"
#include "queues.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_MAX_DESCRIPTOR_SETS 1024



/*************************************************************************************************/
/*  Device                                                                                       */
/*************************************************************************************************/

/**
 * Prepare creating a (logical) device from a GPU (physical device).
 *
 * @param gpu the GPU
 * @param[out] device the device
 */
DVZ_EXPORT void dvz_gpu_device(DvzGpu* gpu, DvzDevice* device);



/**
 * Manually request a number of queues from a queue family.
 *
 * @param device the device
 * @param family the queue family index
 * @param count the number of queues requested
 */
DVZ_EXPORT void dvz_device_request_queues(DvzDevice* device, uint32_t family, uint32_t count);



/**
 * Create the logical device after requesting queues.
 *
 * @param device the device
 * @returns the result code
 */
DVZ_EXPORT int dvz_device_create(DvzDevice* device);



/**
 * Retrieve a queue from a role.
 *
 * @param device the device
 * @param role the role
 * @returns the queue
 */
DVZ_EXPORT DvzQueue* dvz_device_queue(DvzDevice* device, DvzQueueRole role);



/**
 * Destroy a device.
 *
 * @param device
 */
DVZ_EXPORT void dvz_device_destroy(DvzDevice* device);



/*************************************************************************************************/
/*  Extensions & features                                                                        */
/*************************************************************************************************/

/**
 * Request device extension.
 *
 * @param device the device
 * @param extension the requested extension
 */
DVZ_EXPORT void dvz_device_request_extension(DvzDevice* device, const char* extension);


/**
 * Request features for Vulkan 1.0.
 *
 * @param device the device
 * @returns the features struct to use to enable individual features
 */
DVZ_EXPORT VkPhysicalDeviceFeatures* dvz_device_request_features10(DvzDevice* device);



/**
 * Request features for Vulkan 1.1.
 *
 * @param device the device
 * @returns the features struct to use to enable individual features
 */
DVZ_EXPORT VkPhysicalDeviceVulkan11Features* dvz_device_request_features11(DvzDevice* device);



/**
 * Request features for Vulkan 1.2.
 *
 * @param device the device
 * @returns the features struct to use to enable individual features
 */
DVZ_EXPORT VkPhysicalDeviceVulkan12Features* dvz_device_request_features12(DvzDevice* device);



/**
 * Request features for Vulkan 1.3.
 *
 * @param device the device
 * @returns the features struct to use to enable individual features
 */
DVZ_EXPORT VkPhysicalDeviceVulkan13Features* dvz_device_request_features13(DvzDevice* device);
