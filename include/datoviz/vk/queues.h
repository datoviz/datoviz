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

#include <stdint.h>

#include <vulkan/vulkan.h>

#include "datoviz/common/macros.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_MAX_QUEUE_FAMILIES 8
// #define DVZ_MAX_QUEUES         8



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
typedef struct DvzGpu DvzGpu;
typedef struct DvzDevice DvzDevice;
typedef struct DvzQueueCaps DvzQueueCaps;
typedef struct DvzQueue DvzQueue;
typedef struct DvzQueues DvzQueues;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzQueueCaps
{
    uint32_t family_count;
    VkQueueFlags flags[DVZ_MAX_QUEUE_FAMILIES];
    uint32_t queue_count[DVZ_MAX_QUEUE_FAMILIES];
};



struct DvzQueue
{
    uint32_t family_idx;
    uint32_t queue_idx;
    VkQueue handle;
    VkQueueFlags flags;
    bool is_main; // whether this queue is the main one
    bool is_set;  // whether this queue exists
};



struct DvzQueues
{
    uint32_t queue_count;
    DvzQueue queues[DVZ_QUEUE_COUNT]; // for each role, a dedicated queue, or none.
};



/*************************************************************************************************/
/*  Queues                                                                                       */
/*************************************************************************************************/

/**
 * Get the queue capabilities of a GPU.
 *
 * @param gpu
 * @returns the queue capabilities
 */
DVZ_EXPORT DvzQueueCaps* dvz_gpu_queue_caps(DvzGpu* gpu);



/**
 * Choose the requested queues for the logical device depending on the GPU queues capabilities.
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
 * Get a queue from its role, either a dedicated queue, or the main queue if it supports the role.
 *
 * @param queues the queues
 * @param role the role
 * @returns the queue
 */
DVZ_EXPORT DvzQueue* dvz_queue_from_role(DvzQueues* queues, DvzQueueRole role);



/**
 * Returns whether a queue supports a given role.
 *
 * @param queue a queue
 * @param role a queue role
 */
DVZ_EXPORT bool dvz_queue_supports(DvzQueue* queue, DvzQueueRole role);



// TODO: put in device.c: void dvz_queues_create(DvzQueues* queues, DvzDevice* device);
