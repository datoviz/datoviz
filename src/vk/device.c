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

#include <volk.h>

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
/*  Utils                                                                                        */
/*************************************************************************************************/

static inline int32_t array_index_of(uint32_t n, uint32_t* array, uint32_t x)
{
    ANN(array);
    for (uint32_t i = 0; i < n; i++)
        if (array[i] == x)
            return (int32_t)i;
    return -1;
}



static void queues_per_family(DvzQueues* queues, uint32_t* qfn, uint32_t* qfi, uint32_t* qfc)
{
    ANN(queues);
    ANN(qfn); // the number of different families
    ANN(qfi); // the queue family index of each kept family
    ANN(qfc); // the number of queues in each of the kept family

    // Initialize to a large value so it is properly initialized.
    for (uint32_t i = 0; i < DVZ_MAX_QUEUE_FAMILIES; i++)
        qfi[i] = 1024;

    // Go through all queues, and count the number of requested queues per queue family index,
    // keeping only the queue family indices for which there is at least 1 requested queue.
    for (uint32_t i = 0; i < queues->queue_count; i++)
    {
        ASSERT(i < DVZ_QUEUE_COUNT);
        DvzQueue* queue = &queues->queues[i];
        ANN(queue);

        uint32_t qf = queue->family_idx;
        ASSERT(qf < DVZ_MAX_QUEUE_FAMILIES);

        int32_t idx = array_index_of(*qfn, qfi, qf);
        // The queue family doesn't already exist in the list, append it.
        if (idx < 0)
        {
            idx = (int32_t)*qfn;
            qfi[(*qfn)++] = qf;
        }
        ASSERT(idx < DVZ_MAX_QUEUE_FAMILIES);
        // Register one more queue for that queue family.
        qfc[idx]++;
    }

    // Consistency check.
    uint32_t n = 0;
    for (uint32_t i = 0; i < *qfn; i++)
    {
        n += qfc[i];
    }
    ASSERT(n == queues->queue_count);
}



static void fill_queue_requests(
    uint32_t qfn, uint32_t* qfi, uint32_t* qfc, VkDeviceQueueCreateInfo* infos, float* priority)
{
    ASSERT(qfn > 0);
    ASSERT(qfn < DVZ_MAX_QUEUE_FAMILIES);

    ANN(qfi);
    ANN(qfc);
    ANN(infos);
    ANN(priority);

    VkDeviceQueueCreateInfo* info = NULL;

    // Go through all kept families, and create one VkDeviceQueueCreateInfo for each.
    for (uint32_t i = 0; i < qfn; i++)
    {
        uint32_t qf = qfi[i];
        uint32_t qc = qfc[i];

        info = &infos[i];
        ANN(info);
        info->sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        info->queueFamilyIndex = qf;
        info->queueCount = qc;
        info->pQueuePriorities = priority;
    }
}



static void fill_features(DvzDevice* device)
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



static void create_queues(DvzDevice* device)
{
    ANN(device);
    ANNVK(device->vk_device);

    DvzQueues* queues = &device->queues;
    ANN(queues);

    for (uint32_t i = 0; i < queues->queue_count; i++)
    {
        ASSERT(i < DVZ_QUEUE_COUNT);
        vkGetDeviceQueue(
            device->vk_device, queues->queues[i].family_idx, queues->queues[i].queue_idx,
            &queues->queues[i].handle);
    }
}



static void create_cpools(DvzDevice* device, uint32_t qfn, uint32_t* qfi, uint32_t* qfc)
{
    ANN(device);
    ANNVK(device->vk_device);

    VkCommandPoolCreateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    uint32_t qf = 0;
    for (uint32_t i = 0; i < qfn; i++)
    {
        qf = qfi[i];
        ASSERT(qf < DVZ_MAX_QUEUE_FAMILIES);

        info.queueFamilyIndex = qf;
        VK_CHECK_RESULT(vkCreateCommandPool(device->vk_device, &info, NULL, &device->cpools[qf]));
    }
}



static void destroy_cpools(DvzDevice* device)
{
    ANN(device);
    log_trace("destroy command pool(s)");
    for (uint32_t i = 0; i < DVZ_MAX_QUEUE_FAMILIES; i++)
    {
        if (device->cpools[i] != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(device->vk_device, device->cpools[i], NULL);
            device->cpools[i] = VK_NULL_HANDLE;
        }
    }
}



static void create_dpool(DvzDevice* device)
{
    ANN(device);

    VkDescriptorPoolSize poolSizes[] = {
        {VK_DESCRIPTOR_TYPE_SAMPLER, DVZ_MAX_DESCRIPTOR_SETS},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, DVZ_MAX_DESCRIPTOR_SETS},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, DVZ_MAX_DESCRIPTOR_SETS},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, DVZ_MAX_DESCRIPTOR_SETS},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, DVZ_MAX_DESCRIPTOR_SETS},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, DVZ_MAX_DESCRIPTOR_SETS},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, DVZ_MAX_DESCRIPTOR_SETS},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, DVZ_MAX_DESCRIPTOR_SETS},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, DVZ_MAX_DESCRIPTOR_SETS},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, DVZ_MAX_DESCRIPTOR_SETS},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, DVZ_MAX_DESCRIPTOR_SETS}};

    VkDescriptorPoolCreateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    info.poolSizeCount = 11;
    info.pPoolSizes = poolSizes;
    info.maxSets = DVZ_MAX_DESCRIPTOR_SETS * info.poolSizeCount;

    // Create descriptor pool.
    log_trace("create descriptor pool");
    ANNVK(device->vk_device);
    VK_CHECK_RESULT(vkCreateDescriptorPool(device->vk_device, &info, NULL, &device->dpool));
}



static void destroy_dpool(DvzDevice* device)
{
    ANN(device);
    ANNVK(device->vk_device);
    if (device->dpool != VK_NULL_HANDLE)
    {
        log_trace("destroy descriptor pool");
        vkDestroyDescriptorPool(device->vk_device, device->dpool, NULL);
        device->dpool = VK_NULL_HANDLE;
    }
}



/*************************************************************************************************/
/*  Device                                                                                       */
/*************************************************************************************************/

void dvz_gpu_device(DvzGpu* gpu, DvzDevice* device)
{
    ANN(gpu);
    ANN(device);

    device->gpu = gpu;

    dvz_obj_init(&device->obj);
}



void dvz_device_request_queues(DvzDevice* device, uint32_t family, uint32_t count)
{
    ANN(device);
    if (family >= DVZ_MAX_QUEUE_FAMILIES)
    {
        log_warn("invalid requested queue family: %d", family);
        return;
    }
    ASSERT(family < DVZ_MAX_QUEUE_FAMILIES);

    uint32_t max = device->gpu->queue_caps.queue_count[family];
    if (count > max)
    {
        log_error(
            "cannot request %d queues on family %d which only supports %d queues", //
            count, family, max);
        return;
    }

    uint32_t queue_idx = 0; // local to each family
    for (uint32_t i = 0; i < count; i++)
    {
        ASSERT(device->queues.queue_count < DVZ_QUEUE_COUNT);
        DvzQueue* queue = &device->queues.queues[device->queues.queue_count++];
        queue->family_idx = family;
        queue->queue_idx = queue_idx++;
    }
}



int dvz_device_create(DvzDevice* device)
{
    ANN(device);

    // TODO: recreate the underlying vulkan logical device if called multiple times (destroying the
    // old one first), which allows to change the queues before

    // NOTE: if the GPU supports extension VK_KHR_portability_subset, it must be requested
    if (dvz_gpu_has_extension(device->gpu, "VK_KHR_portability_subset"))
    {
        dvz_device_request_extension(device, "VK_KHR_portability_subset");
    }

    // From the requested queues, get the number of queues per queue family.
    uint32_t qfn = 0;
    uint32_t qfi[DVZ_MAX_QUEUE_FAMILIES] = {0};
    uint32_t qfc[DVZ_MAX_QUEUE_FAMILIES] = {0};
    queues_per_family(&device->queues, &qfn, qfi, qfc);
    ASSERT(qfn > 0);

    // Device creation info structure.
    VkDeviceCreateInfo device_info = {0};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    // Queue family info.
    VkDeviceQueueCreateInfo queue_families_info[DVZ_MAX_QUEUE_FAMILIES] = {0};
    float priority = 1.0f;
    fill_queue_requests(qfn, qfi, qfc, queue_families_info, &priority);
    if (qfn == 0)
    {
        log_error("at least one queue must be requested to create the device");
        return 1;
    }
    ASSERT(qfn > 0);
    device_info.queueCreateInfoCount = qfn;
    device_info.pQueueCreateInfos = queue_families_info;

    // Extensions.
    device_info.enabledExtensionCount = device->req_extension_count;
    device_info.ppEnabledExtensionNames = (const char* const*)device->req_extensions;

    // Features using v2 API with features structs chain.
    fill_features(device);
    device_info.pNext = &device->features;

    // Create the device.
    log_trace("creating the Vulkan device");
    VkResult res = vkCreateDevice(device->gpu->pdevice, &device_info, NULL, &device->vk_device);
    int out = check_result(res);
    if (res != VK_SUCCESS)
    {
        return out;
    }
    volkLoadDevice(device->vk_device);
    log_trace("Vulkan device created");

    // Create the Vulkan queues.
    create_queues(device);

    // Create the command pools for each queue family.
    create_cpools(device, qfn, qfi, qfc);

    // Create the descriptor pool.
    create_dpool(device);

    dvz_obj_created(&device->obj);
    log_trace("DvzDevice created");
    return out;
}



VkDevice dvz_device_handle(DvzDevice* device)
{
    ANN(device);
    return device->vk_device;
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



VkCommandPool dvz_device_command_pool(DvzDevice* device, uint32_t queue_family)
{
    ANN(device);
    ASSERT(queue_family < DVZ_MAX_QUEUE_FAMILIES);
    return device->cpools[queue_family];
}



void dvz_device_wait(DvzDevice* device)
{
    ANN(device);
    vkDeviceWaitIdle(device->vk_device);
}



void dvz_device_destroy(DvzDevice* device)
{
    ANN(device);

    // Destroy pools.
    destroy_dpool(device);
    destroy_cpools(device);

    // Destroy device.
    if (device->vk_device != VK_NULL_HANDLE)
    {
        log_trace("destroying the device");
        vkDestroyDevice(device->vk_device, NULL);
        log_trace("device destroyed");
    }

    dvz_free_strings(device->req_extension_count, device->req_extensions);
    dvz_obj_destroyed(&device->obj);
}



/*************************************************************************************************/
/*  Extensions & features                                                                        */
/*************************************************************************************************/

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



bool dvz_device_has_extension(DvzDevice* device, const char* extension)
{
    ANN(device);
    ANN(extension);

    ANN(device->req_extensions);
    ASSERT(device->req_extension_count < DVZ_MAX_REQ_EXTENSIONS - 1);

    return dvz_strings_contains(device->req_extension_count, device->req_extensions, extension);
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
