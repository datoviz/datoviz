/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Queues                                                                                       */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include "datoviz/vk/vulkan.h"

#include "datoviz/common/macros.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_MAX_QUEUE_FAMILIES 8



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Queue role.
typedef enum
{
    DVZ_QUEUE_MAIN,         // guaranteed: graphics + compute (+ transfer implicitly)
    DVZ_QUEUE_COMPUTE,      // optional async compute
    DVZ_QUEUE_TRANSFER,     // optional async transfer
    DVZ_QUEUE_VIDEO_ENCODE, // optional
    DVZ_QUEUE_VIDEO_DECODE, // optional
    DVZ_QUEUE_COUNT,
} DvzQueueRole;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzInstance DvzInstance;
typedef struct DvzDevice DvzDevice;
typedef struct DvzQueueCaps DvzQueueCaps;
typedef struct DvzQueue DvzQueue;
typedef struct DvzQueues DvzQueues;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

/**
 * Snapshot of the queue families exposed by one GPU.
 *
 * This is an intentional public low-level planning type. It is used to choose queue families
 * before logical-device creation, not to expose mutable device internals after startup.
 */
struct DvzQueueCaps
{
    uint32_t family_count;
    VkQueueFlags flags[DVZ_MAX_QUEUE_FAMILIES];
    uint32_t queue_count[DVZ_MAX_QUEUE_FAMILIES];
};



/**
 * Snapshot of one selected queue role/family/index assignment.
 *
 * This is an intentional public low-level queue-selection type. It is part of the planning and
 * lookup surface used by device setup, command submission helpers, and tests.
 */
struct DvzQueue
{
    uint32_t family_idx;
    uint32_t queue_idx;
    VkQueue vk_queue;
    VkQueueFlags flags;
    bool is_main; // whether this queue is the main one
    bool is_set;  // whether this queue exists
};



/**
 * Queue-selection plan for a logical device.
 *
 * This is an intentional public low-level planning type. It represents the queue-role assignment
 * chosen from a GPU capability snapshot and then consumed by device-configuration code.
 */
struct DvzQueues
{
    uint32_t queue_count;
    DvzQueue queues[DVZ_QUEUE_COUNT]; // for each role, a dedicated queue, or none.
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Queues                                                                                       */
/*************************************************************************************************/

/**
 * Query a GPU queue-family capability snapshot from an instance.
 *
 * Together with DvzQueues, this forms the public low-level queue-planning surface used before
 * device creation. Callers should treat the result as a capability snapshot, not as owned runtime
 * state.
 *
 * @param instance source instance
 * @param gpu_index selected GPU index in the instance
 * @param[out] out_caps destination queue capabilities snapshot
 * @return whether queue capabilities were retrieved
 */
DVZ_EXPORT bool
dvz_instance_gpu_queue_caps(DvzInstance* instance, uint32_t gpu_index, DvzQueueCaps* out_caps);



/**
 * Choose a logical-device queue plan from a capability snapshot.
 *
 * This function is part of the intentional public low-level queue-planning API. It does not create
 * Vulkan queues by itself; it selects family/role assignments that higher-level device setup then
 * requests explicitly.
 *
 * @param qc the queue caps
 * @param[out] queues the queues specification
 */
DVZ_EXPORT void dvz_queues(DvzQueueCaps* qc, DvzQueues* queues);



/**
 * Show the queues.
 *
 * @param queues the queues
 */
DVZ_EXPORT void dvz_queues_show(DvzQueues* queues);



/**
 * Get a queue for a role from a queue-selection plan or device-owned queue table.
 *
 * The returned queue may be a dedicated queue for the role, or the main queue when that queue
 * intentionally satisfies the requested role.
 *
 * @param queues the queues
 * @param role the role
 * @return the queue
 */
DVZ_EXPORT DvzQueue* dvz_queue_from_role(DvzQueues* queues, DvzQueueRole role);



/**
 * Return the queue index of a queue.
 *
 * @param queue the queue
 * @return the queue index
 */
DVZ_EXPORT uint32_t dvz_queue_index(DvzQueue* queue);



/**
 * Return the queue family of a queue.
 *
 * @param queue the queue
 * @return the queue family index
 */
DVZ_EXPORT uint32_t dvz_queue_family(DvzQueue* queue);



/**
 * Return the Vulkan handle of a queue.
 *
 * @param queue the queue
 * @return borrowed Vulkan queue handle, or `VK_NULL_HANDLE` when unavailable
 */
DVZ_EXPORT VkQueue dvz_queue_handle(DvzQueue* queue);



/**
 * Wait for a queue to be idle. Inefficient.
 *
 * @param queue the queue
 */
DVZ_EXPORT void dvz_queue_wait(DvzQueue* queue);



/**
 * Returns whether a queue supports a given role.
 *
 * @param queue a queue
 * @param role a queue role
 * @return true if the queue advertises the requested role
 */
DVZ_EXPORT bool dvz_queue_supports(DvzQueue* queue, DvzQueueRole role);



// TODO: put in device.c: void dvz_queues_create(DvzQueues* queues, DvzDevice* device);



EXTERN_C_OFF
