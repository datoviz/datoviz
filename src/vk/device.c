/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Device                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include <vulkan/vulkan.h>

#include "_alloc.h"
#include "_compat.h"
#include "datoviz/common/obj.h"
#include "datoviz/vk/device.h"
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

static void _fill_queue_requests(
    DvzQueues* queues, VkDeviceQueueCreateInfo* infos, uint32_t* qfc, float* priority)
{
    ANN(queues);
    ANN(infos);
    ANN(qfc);
    ANN(priority);

    uint32_t qf = 0;
    VkDeviceQueueCreateInfo* info = NULL;
    DvzQueue* queue = NULL;

    uint32_t qf2infoidx[DVZ_MAX_QUEUE_FAMILIES] = {0};
    // Initialize to a large value so it is properly initialized.
    for (uint32_t i = 0; i < DVZ_MAX_QUEUE_FAMILIES; i++)
    {
        qf2infoidx[i] = 1024;
    }
    uint32_t infoidx = 0;
    uint32_t nidx = 0;

    // Go through all queues, and count the number of requested queues per queue family index,
    // keeping only the queue family indices for which there is at least 1 requested queue.
    for (uint32_t i = 0; i < queues->queue_count; i++)
    {
        queue = &queues->queues[i];
        ANN(queue);

        qf = queue->family_idx;
        ASSERT(qf < DVZ_MAX_QUEUE_FAMILIES);

        // Append a new info struct if there isn't already one with that family index, or reuse it
        // if it already exists.
        infoidx = qf2infoidx[qf];
        if (infoidx >= DVZ_MAX_QUEUE_FAMILIES)
        {
            infoidx = nidx++;
            qf2infoidx[qf] = infoidx;
        }
        ASSERT(infoidx < DVZ_MAX_QUEUE_FAMILIES);
        log_info("i=%d, qf=%d, infoidx=%d, nidx=%d", i, qf, infoidx, nidx);

        info = &infos[infoidx];
        ANN(info);
        info->sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        info->pQueuePriorities = priority;
        info->queueCount++;
        info->queueFamilyIndex = qf;
    }

    *qfc = nidx;
}



static void _fill_features(DvzDevice* device)
{
    // Set up the chain of features to enable for the device creation.
    device->features11 = (VkPhysicalDeviceVulkan11Features){
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
    };
    device->features12 = (VkPhysicalDeviceVulkan12Features){
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .pNext = &device->features11,
    };
    device->features13 = (VkPhysicalDeviceVulkan13Features){
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = &device->features12,
    };
    device->features = (VkPhysicalDeviceFeatures2){
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &device->features13,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

void dvz_gpu_device(DvzGpu* gpu, DvzDevice* device)
{
    ANN(gpu);
    ANN(device);

    device->gpu = gpu;

    dvz_obj_init(&device->obj);
}



void dvz_device_request_queue(DvzDevice* device, uint32_t family, uint32_t count)
{
    ANN(device);
    if (family >= DVZ_MAX_QUEUE_FAMILIES)
    {
        log_warn("invalid requested queue family: %d", family);
        return;
    }
    ASSERT(family < DVZ_MAX_QUEUE_FAMILIES);

    device->req_queues_per_family[family] += count;
}



void dvz_device_request_queues(DvzDevice* device, DvzQueues* queues)
{
    ANN(device);
    ANN(queues);
    for (uint32_t i = 0; i < queues->queue_count; i++)
    {
        dvz_device_request_queue(device, queues->queues[i].family_idx, 1);
    }
}



void dvz_device_request_extension(DvzDevice* device, const char* extension)
{
    ANN(device);
    ANN(extension);

    ANN(device->req_extensions);
    ASSERT(device->req_extension_count < DVZ_MAX_REQ_EXTENSIONS - 1);

    if (!dvz_strings_contains(device->req_extension_count, device->req_extensions, extension))
    {
        device->req_extensions[device->req_extension_count++] = dvz_strdup(extension);
    }
}



VkPhysicalDeviceFeatures* dvz_device_request_features10(DvzDevice* device)
{
    ANN(device);
    return &device->features.features;
}



VkPhysicalDeviceVulkan11Features* dvz_device_request_features11(DvzDevice* device)
{
    ANN(device);
    return &device->features11;
}



VkPhysicalDeviceVulkan12Features* dvz_device_request_features12(DvzDevice* device)
{
    ANN(device);
    return &device->features12;
}



VkPhysicalDeviceVulkan13Features* dvz_device_request_features13(DvzDevice* device)
{
    ANN(device);
    return &device->features13;
}



int dvz_device_create(DvzDevice* device)
{
    ANN(device);

    /*
    TODO
    recreate the underlying vulkan logical device if called multiple times (destroying the old one
    first), which allows to change the queues before

    portability : if gpu supported extensions
    include VK_KHR_portability_subset, must be included in the requested extensions
    */

    // Device creation info structure.
    VkDeviceCreateInfo device_info = {0};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    // Queue family info.
    VkDeviceQueueCreateInfo queue_families_info[DVZ_MAX_QUEUE_FAMILIES] = {0};
    uint32_t qfs = 0;
    float priority = 1.0f;
    _fill_queue_requests(&device->queues, queue_families_info, &qfs, &priority);
    ASSERT(qfs > 0);
    device_info.queueCreateInfoCount = qfs;
    device_info.pQueueCreateInfos = queue_families_info;

    // Extensions.
    device_info.enabledExtensionCount = device->req_extension_count;
    device_info.ppEnabledExtensionNames = (const char* const*)device->req_extensions;

    // Features using v2 API with features structs chain.
    _fill_features(device);
    device_info.pNext = &device->features;

    // Create the device.
    VkResult res = vkCreateDevice(device->gpu->pdevice, &device_info, NULL, &device->vk_device);
    int out = check_result(res);

    dvz_obj_created(&device->obj);
    return out;
}



DvzQueue* dvz_device_queue(DvzDevice* device, DvzQueueRole role)
{
    ANN(device);
    ANN(&device->queues);
    if (device->queues.queue_count == 0)
    {
        log_warn("there are no queues in DvzDevice.queues (DvzQueues struct)");
        return NULL;
    }
    return dvz_queue_from_role(&device->queues, role);
}



void dvz_device_destroy(DvzDevice* device)
{
    ANN(device);

    dvz_free_strings(device->req_extension_count, device->req_extensions);

    dvz_obj_destroyed(&device->obj);
}
