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

#include "_assertions.h"
#include "datoviz/vk/gpu.h"
#include "datoviz/vk/instance.h"
#include "datoviz/vk/queues.h"
#include "macros.h"
#include "types.h"
#include "vulkan/vulkan_core.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzQueueCaps* dvz_gpu_queue_caps(DvzGpu* gpu)
{
    ANN(gpu);

    VkPhysicalDevice pdevice = gpu->pdevice;
    ANNVK(pdevice);

    DvzQueueCaps* qc = &gpu->queue_caps;
    ANN(qc);

    if (qc->family_count > 0)
    {
        return qc;
    }

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



void dvz_queues(DvzQueueCaps* qc, DvzQueues* queues) { ANN(qc); }



bool dvz_queue_supports(DvzQueue* queue, DvzQueueRole role)
{
    ANN(queue);
    VkQueueFlags f = queue->flags;

    switch (role)
    {
    case DVZ_QUEUE_MAIN:
        return (f & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) ==
               (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);
    case DVZ_QUEUE_COMPUTE:
        return (f & VK_QUEUE_COMPUTE_BIT) != 0;
    case DVZ_QUEUE_TRANSFER:
        return ((f & VK_QUEUE_TRANSFER_BIT) != 0) ||
               (f & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) ==
                   (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);
    case DVZ_QUEUE_VIDEO_ENCODE:
        return (f & VK_QUEUE_VIDEO_ENCODE_BIT_KHR) != 0;
    case DVZ_QUEUE_VIDEO_DECODE:
        return (f & VK_QUEUE_VIDEO_DECODE_BIT_KHR) != 0;
    default:
        return false;
    }
    return false;
}



DvzQueue* dvz_queue_from_role(DvzQueues* queues, DvzQueueRole role)
{
    ANN(queues);
    if ((int)role < 0 || (int)role >= DVZ_QUEUE_COUNT)
        return NULL;

    // If requesting the main queue, return it.
    DvzQueue* main = &queues->queues[DVZ_QUEUE_MAIN];
    ANN(main);

    if (role == DVZ_QUEUE_MAIN)
        return main;

    // Otherwise, try to find a queue beyond main that supports the requested role.
    DvzQueue* queue = NULL;
    for (uint32_t i = 0; i < queues->queue_count; i++)
    {
        queue = &queues->queues[i];
        ANN(queue);
        if (queue == main)
            continue;
        if (dvz_queue_supports(queue, role))
        {
            log_trace("find queue #%d supporting role %d", i, (int)role);
            return queue;
        }
    }

    // Otherwise, return main if it supports the role, or NULL otherwise.
    if (dvz_queue_supports(main, role))
    {
        log_trace("return main queue supporting role %d", (int)role);
        return main;
    }

    log_error("could not find a queue supporting role %d", role);

    return NULL;
}
