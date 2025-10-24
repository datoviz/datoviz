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
/*  Utils                                                                                        */
/*************************************************************************************************/

static bool queue_flags_supports(VkQueueFlags f, DvzQueueRole role)
{
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



static uint32_t queue_flags_count(VkQueueFlags f)
{
    //  return the number of different queue flag bits the flag has.
    uint32_t count = 0;
    while (f)
    {
        count += f & 1;
        f >>= 1;
    }
    return count;
}



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



void dvz_queues(DvzQueueCaps* qc, DvzQueues* queues)
{
    ANN(qc);
    ANN(queues);

    // First, we find the main queue, let's define it as the first queue that supports
    // graphics+compute (and also transfer implicitly as per the Vulkan spec).

    // Then, for each non-main role, we go through all non-main queue families, and we find those
    // that support the role.
    // If there is none, we do nothing.
    // If there is one, we add that queue.
    // If there are 2+, we only keep those that have the minimum number of queue_flags_count(f)
    // and we take the first one among them.


    // ------------------------------------------------------------------------------------------
    // 1. Find the main queue: first family that supports GRAPHICS + COMPUTE.
    // ------------------------------------------------------------------------------------------
    int main_family = -1;
    for (uint32_t i = 0; i < qc->family_count; i++)
    {
        VkQueueFlags f = qc->flags[i];
        if ((f & VK_QUEUE_GRAPHICS_BIT) && (f & VK_QUEUE_COMPUTE_BIT))
        {
            main_family = (int)i;
            break;
        }
    }

    if (main_family < 0)
    {
        log_error("No queue family supports both graphics and compute!");
        return;
    }
    ASSERT(main_family >= 0);

    queues->queues[DVZ_QUEUE_MAIN] = (DvzQueue){
        .family_idx = (uint32_t)main_family,
        .queue_idx = 0,
        .flags = qc->flags[main_family],
        .is_main = true,
        .is_set = true,
    };
    queues->queue_count = 1;

    // ------------------------------------------------------------------
    // 2. For each other role, find the best queue family.
    // ------------------------------------------------------------------
    for (uint32_t role = DVZ_QUEUE_COMPUTE; role < DVZ_QUEUE_COUNT; role++)
    {
        int best_idx = -1;
        uint32_t best_count = UINT32_MAX;

        // Goes through all non-main roles.
        for (uint32_t i = 0; i < qc->family_count; i++)
        {
            if ((int)i == main_family)
                continue;

            VkQueueFlags f = qc->flags[i];
            if (!queue_flags_supports(f, (DvzQueueRole)role))
                continue;

            // Here, this queue family is not main and supports the current role.
            uint32_t c = queue_flags_count(f);

            if (c < best_count)
            {
                best_count = c;
                best_idx = (int)i;
            }
        }

        if (best_idx >= 0)
        {
            queues->queues[role] = (DvzQueue){
                .family_idx = (uint32_t)best_idx,
                .queue_idx = 0,
                .flags = qc->flags[best_idx],
                .is_set = true,
            };
            queues->queue_count++;
        }
    }
}



void dvz_queues_show(DvzQueues* queues)
{
    ANN(queues);
    log_info("Selected %u queue(s):", queues->queue_count);
    for (uint32_t role = 0; role < DVZ_QUEUE_COUNT; role++)
    {
        DvzQueue* q = &queues->queues[role];
        if (!q->is_set)
            continue;
        log_info(
            "  role %u: family=%u  flags=0x%x%s", role, q->family_idx, q->flags,
            q->is_main ? " (main)" : "");
    }
}



bool dvz_queue_supports(DvzQueue* queue, DvzQueueRole role)
{
    ANN(queue);
    return queue_flags_supports(queue->flags, role);
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
