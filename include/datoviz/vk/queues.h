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
#define DVZ_MAX_QUEUES         8



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Queue role.
typedef enum
{
    DVZ_QUEUE_MAIN = 0,     // guaranteed
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
typedef struct DvzQueue DvzQueue;
typedef struct DvzQueueCaps DvzQueueCaps;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzQueueCaps
{
    uint32_t family_count;
    VkQueueFlags flags[DVZ_MAX_QUEUE_FAMILIES];
    uint32_t queue_count[DVZ_MAX_QUEUE_FAMILIES];

    // Filled by automatic queue strategy, finding one queue per role depending on the queue caps.
    uint32_t role_family[DVZ_MAX_QUEUES]; // for each role, the family idx
    uint32_t role_idx[DVZ_MAX_QUEUES];    // for each role, the queue idx within its family
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
DVZ_EXPORT DvzQueueCaps* dvz_gpu_queues(DvzGpu* gpu);



/**
 * Probe the queues of a GPU.
 *
 * @param gpu the GPU
 * @returns the queue capabilities
 */
DVZ_EXPORT DvzQueueCaps* dvz_gpu_probe_queues(DvzGpu* gpu);



/**
 * Choose the requested queues for the logical device depending on the GPU queues capabilities.
 * Fills in the gpu->queue_caps structure.
 *
 * @param gpu the GPU
 */
DVZ_EXPORT void dvz_gpu_choose_queues(DvzGpu* gpu);
