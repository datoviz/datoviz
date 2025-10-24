/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Queues                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include <vulkan/vulkan.h>

#include "_alloc.h"
#include "_compat.h"
#include "datoviz/vk/gpu.h"
#include "datoviz/vk/instance.h"
#include "datoviz/vk/queues.h"
#include "macros.h"
#include "types.h"
#include "vulkan/vulkan_core.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzQueue DvzQueue;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzQueue
{
    uint32_t family_index;
    uint32_t queue_index;
    VkQueue handle;
    VkQueueFlags flags;
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzQueueCaps* dvz_gpu_queues(DvzGpu* gpu)
{
    ANN(gpu);
    return &gpu->queue_caps;
}



DvzQueueCaps* dvz_gpu_probe_queues(DvzGpu* gpu)
{
    ANN(gpu);

    VkPhysicalDevice pdevice = gpu->pdevice;
    ANNVK(pdevice);

    DvzQueueCaps* qc = &gpu->queue_caps;

    // Get the queue family properties.
    vkGetPhysicalDeviceQueueFamilyProperties(pdevice, &qc->family_count, NULL);
    log_trace("found %d queue families", qc->family_count);
    ASSERT(qc->family_count > 0);
    ASSERT(qc->family_count <= DVZ_MAX_QUEUE_FAMILIES);

    VkQueueFamilyProperties qfp[DVZ_MAX_QUEUE_FAMILIES] = {0};
    vkGetPhysicalDeviceQueueFamilyProperties(pdevice, &qc->family_count, qfp);

    for (uint32_t qf = 0; qf < qc->family_count; qf++)
    {
        qc->queue_count[qf] = qfp[qf].queueCount;
        qc->flags[qf] = qfp[qf].queueFlags;
    }

    return qc;
}
