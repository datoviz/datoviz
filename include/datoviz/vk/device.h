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

#include "datoviz/common/macros.h"
#include "datoviz/common/obj.h"

#include <vulkan/vulkan.h>

MUTE_ON
#define VMA_EXTERNAL_MEMORY 1
#include "vk_mem_alloc.h"
MUTE_OFF

#include "queues.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzInstance DvzInstance;
typedef struct DvzGpu DvzGpu;
typedef struct DvzDevice DvzDevice;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzDevice
{
    DvzObject obj;
    DvzQueues queues;
    VkDescriptorPool dset_pool;
    VkDevice device;
    VmaAllocator allocator;
};



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
DVZ_EXPORT void dvz_device_request_queue(DvzDevice* device, uint32_t family, uint32_t count);



/**
 * Request queues corresponding from a DvzQueues structure.
 *
 * @param device the device
 * @param queues the requested queues
 */
DVZ_EXPORT void dvz_device_request_queues(DvzDevice* device, DvzQueues* queues);



/**
 * Create the logical device after requesting queues.
 *
 * @param device the device
 * @returns the result code
 */
DVZ_EXPORT int dvz_device_create(DvzDevice* device);



/**
 * Retrieve a queue after device creation, from its queue family index and queue index within the
 * family.
 *
 * @param device the device
 * @param family the queue family index
 * @param idx the queue index within its family
 * @returns the queue
 */
DVZ_EXPORT DvzQueue* dvz_device_queue_from_idx(DvzDevice* device, uint32_t family, uint32_t idx);



/**
 * Retrieve a queue from a role.
 *
 * @param device the device
 * @param role the role
 * @returns the queue
 */
DVZ_EXPORT DvzQueue* dvz_device_queue_for_role(DvzDevice* device, DvzQueueRole role);



/**
 * Destroy a device.
 *
 * @param device
 */
DVZ_EXPORT void dvz_device_destroy(DvzDevice* device);
