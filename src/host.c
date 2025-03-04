/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Host                                                                                         */
/*************************************************************************************************/

#include "host.h"
#include "backend.h"
#include "common.h"
#include "datoviz_macros.h"
#include "vklite.h"
#include "vklite_utils.h"
#include "vkutils.h"



/*************************************************************************************************/
/*  Host                                                                                         */
/*************************************************************************************************/

DvzHost* dvz_host(void)
{
    log_set_level_env();
    log_debug("create the host");

    DvzHost* host = calloc(1, sizeof(DvzHost));
    ANN(host);

    host->obj.type = DVZ_OBJECT_TYPE_HOST;

    // Initialize the global clock.
    host->clock = dvz_clock();

    host->gpus = dvz_container(DVZ_CONTAINER_DEFAULT_COUNT, sizeof(DvzGpu), DVZ_OBJECT_TYPE_GPU);

    dvz_obj_init(&host->obj);
    return host;
}



void dvz_host_extension(DvzHost* host, const char* extension_name)
{
    ANN(host);
    ANN(extension_name);
    host->extensions[host->extension_count++] = extension_name;
}



void dvz_host_backend(DvzHost* host, DvzBackend backend)
{
    // Add required extensions from a backend.
    uint32_t n = 0;
    const char** required_extensions = dvz_backend_required_extensions(backend, &n);
    for (uint32_t i = 0; i < n; i++)
    {
        dvz_host_extension(host, required_extensions[i]);
    }
}



void dvz_host_create(DvzHost* host)
{
    ANN(host);

    // Create the instance.
    // NOTE: with Qt, we use Qt to create the Vulkan instance
    if (host->instance == VK_NULL_HANDLE)
    {
        create_instance(
            host->extension_count, host->extensions, //
            &host->instance, &host->debug_messenger, &host->n_errors);
    }

    if (host->instance == VK_NULL_HANDLE)
    {
        log_error("unable to create Vulkan instance");
        exit(1);
    }
    // debug_messenger != VK_NULL_HANDLE means validation enabled
    dvz_obj_created(&host->obj);

    // Count the number of devices.
    uint32_t gpu_count = 0;
    VK_CHECK_RESULT(vkEnumeratePhysicalDevices(host->instance, &gpu_count, NULL));
    log_trace("found %d GPU(s)", gpu_count);
    if (gpu_count == 0)
    {
        log_error("no compatible device found! aborting");
        exit(1);
    }

    // Discover the available GPUs.
    // ----------------------------
    {
        // Initialize the GPU(s).
        VkPhysicalDevice* physical_devices = calloc(gpu_count, sizeof(VkPhysicalDevice));
        VK_CHECK_RESULT(vkEnumeratePhysicalDevices(host->instance, &gpu_count, physical_devices));
        ASSERT(gpu_count <= DVZ_CONTAINER_DEFAULT_COUNT);
        DvzGpu* gpu = NULL;
        for (uint32_t i = 0; i < gpu_count; i++)
        {
            gpu = (DvzGpu*)dvz_container_alloc(&host->gpus);
            dvz_obj_init(&gpu->obj);
            gpu->host = host;
            gpu->idx = i;
            discover_gpu(physical_devices[i], gpu);
            log_debug("found device #%d: %s", gpu->idx, gpu->name);
        }

        FREE(physical_devices);
    }
}



void dvz_host_wait(DvzHost* host)
{
    ANN(host);
    log_trace("wait for all GPUs to be idle");
    DvzGpu* gpu = NULL;
    DvzContainerIterator iter = dvz_container_iterator(&host->gpus);
    while (iter.item != NULL)
    {
        gpu = iter.item;
        dvz_gpu_wait(gpu);
        dvz_container_iter(&iter);
    }
}



int dvz_host_destroy(DvzHost* host)
{
    ANN(host);

    log_debug("destroy the host");
    dvz_host_wait(host);

    // Destroy the GPUs.
    CONTAINER_DESTROY_ITEMS(DvzGpu, host->gpus, dvz_gpu_destroy)
    dvz_container_destroy(&host->gpus);

    // Destroy the debug messenger.
    if (host->debug_messenger)
    {
        destroy_debug_utils_messenger_EXT(host->instance, host->debug_messenger, NULL);
        host->debug_messenger = NULL;
    }

    // Destroy the instance.
    log_trace("destroy Vulkan instance");
    if (host->instance != VK_NULL_HANDLE && !host->no_instance_destroy)
    {
        vkDestroyInstance(host->instance, NULL);
        host->instance = 0;
    }

    // Free the App memory.
    int res = (int)host->n_errors;
    FREE(host);
    log_trace("host destroyed");

    return res;
}



/*************************************************************************************************/
/*  GPU                                                                                          */
/*************************************************************************************************/

DvzGpu* dvz_host_gpu(DvzHost* host, int32_t idx)
{
    if (idx < 0)
    {
        log_trace("requesting GPU #%d which is negative, so returning the 'best' GPU", idx);
        return dvz_gpu_best(host);
    }

    if (idx >= (int32_t)host->gpus.count)
    {
        log_error("GPU index %d higher than number of GPUs %d", idx, host->gpus.count);
        idx = 0;
    }

    DvzGpu* gpu = host->gpus.items[idx];
    return gpu;
}



DvzGpu* dvz_gpu_best(DvzHost* host)
{
    ANN(host);
    log_trace("start looking for the best GPU on the system among %d GPU(s)", host->gpus.count);

    DvzGpu* gpu = NULL;
    DvzGpu* best_gpu = NULL;
    DvzGpu* best_gpu_discrete = NULL;
    VkDeviceSize best_vram = 0;
    VkDeviceSize best_vram_discrete = 0;

    ASSERT(host->gpus.count > 0);
    for (int32_t i = 0; i < (int32_t)host->gpus.count; i++)
    {
        gpu = dvz_host_gpu(host, i);
        ANN(gpu);
        ASSERT(gpu->vram > 0);

        // Find the discrete GPU with the most VRAM.
        if (gpu->device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            if (gpu->vram > best_vram_discrete)
            {
                log_trace(
                    "best discrete GPU so far: %s with %s VRAM", gpu->name,
                    pretty_size(gpu->vram));
                best_vram_discrete = gpu->vram;
                best_gpu_discrete = gpu;
            }
        }

        // Find the GPU with the most VRAM.
        if (gpu->vram > best_vram)
        {
            log_trace("best GPU so far: %s with %s VRAM", gpu->name, pretty_size(gpu->vram));
            best_vram = gpu->vram;
            best_gpu = gpu;
        }
    }

    best_gpu = best_gpu_discrete != NULL ? best_gpu_discrete : best_gpu;
    ANN(best_gpu);
    log_trace("best GPU: %s with %s VRAM", best_gpu->name, pretty_size(best_gpu->vram));
    return best_gpu;
}



DvzRenderpass dvz_gpu_renderpass(DvzGpu* gpu, cvec4 clear_color, VkImageLayout layout)
{
    ANN(gpu);
    DvzRenderpass renderpass = {0};
    make_renderpass(gpu, &renderpass, DVZ_DEFAULT_FORMAT, layout, get_clear_color(clear_color));
    return renderpass;
}



void dvz_gpu_request_features(DvzGpu* gpu, VkPhysicalDeviceFeatures requested_features)
{
    ANN(gpu);
    gpu->requested_features = requested_features;
}



void dvz_gpu_extension(DvzGpu* gpu, const char* extension_name)
{
    ANN(gpu);
    ANN(extension_name);

    // Check for duplicates
    for (uint32_t i = 0; i < gpu->extension_count; i++)
    {
        if (strcmp(gpu->extensions[i], extension_name) == 0)
        {
            // Extension already exists, do nothing
            return;
        }
    }

    // Check for overflow
    if (gpu->extension_count >= DVZ_MAX_DEVICE_EXTENSIONS)
    {
        log_error("Maximum number of device extensions reached.");
        return;
    }

    gpu->extensions[gpu->extension_count++] = extension_name;
}



void dvz_gpu_external(DvzGpu* gpu, VkExternalMemoryHandleTypeFlagsKHR flags)
{
    ANN(gpu);
    gpu->external_memory_handle_type = flags;
}



void dvz_gpu_queue(DvzGpu* gpu, uint32_t idx, DvzQueueType type)
{
    ANN(gpu);
    DvzQueues* q = &gpu->queues;
    ANN(q);
    ASSERT(idx < DVZ_MAX_QUEUES);
    q->queue_types[idx] = type;
    ASSERT(idx == q->queue_count);
    q->queue_count++;
}



void dvz_gpu_create(DvzGpu* gpu, VkSurfaceKHR surface)
{
    ANN(gpu);
    ANN(gpu->host);

    if (gpu->queues.queue_count == 0)
    {
        log_error(
            "you must request at least one queue with dvz_gpu_queue() before creating the GPU");
        exit(1);
    }
    log_trace(
        "starting creation of GPU #%d WITH%s surface...", gpu->idx,
        surface != VK_NULL_HANDLE ? "" : "OUT");
    create_device(gpu, surface);

    DvzQueues* q = &gpu->queues;

    // Create queues.
    uint32_t qf = 0;
    for (uint32_t i = 0; i < q->queue_count; i++)
    {
        qf = q->queue_families[i];
        vkGetDeviceQueue(gpu->device, qf, q->queue_indices[i], &q->queues[i]);

        // Create command pool only for the queue families that appear in the requested queues.
        if (q->cmd_pools[qf] == VK_NULL_HANDLE)
            create_command_pool(gpu->device, qf, &q->cmd_pools[qf]);
    }

    // Create descriptor pool.
    create_descriptor_pool(gpu->device, &gpu->dset_pool);

    // Create allocator.
    VmaAllocatorCreateInfo alloc_info = {0};
    alloc_info.vulkanApiVersion = DVZ_VULKAN_API;
    alloc_info.physicalDevice = gpu->physical_device;
    alloc_info.device = gpu->device;
    alloc_info.instance = gpu->host->instance;

    // HACK: get the number of memory types, and create an array of
    // VkExternalMemoryHandleTypeFlagsKHR to copy gpu->external_memory_handle_type to each memory
    // type.
    VkExternalMemoryHandleTypeFlagsKHR* handle_types = NULL;
    if (gpu->external_memory_handle_type != 0)
    {
        log_trace("setting external memory handle types to %d", gpu->external_memory_handle_type);
        VkPhysicalDeviceMemoryProperties memoryProperties = {0};
        vkGetPhysicalDeviceMemoryProperties(gpu->physical_device, &memoryProperties);
        handle_types = (VkExternalMemoryHandleTypeFlagsKHR*)calloc(
            memoryProperties.memoryTypeCount, sizeof(VkExternalMemoryHandleTypeFlagsKHR));
        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
        {
            handle_types[i] = gpu->external_memory_handle_type;
        }
        alloc_info.pTypeExternalMemoryHandleTypes = handle_types;
    }

    vmaCreateAllocator(&alloc_info, &gpu->allocator);
    ASSERT(gpu->allocator != VK_NULL_HANDLE);

    // Free handle_types.
    if (handle_types != NULL)
    {
        FREE(handle_types);
    }

    dvz_obj_created(&gpu->obj);

    // HACK: use queue 0 for transfers (convention).
    gpu->cmd = dvz_commands(gpu, 0, 1);

    log_trace("GPU #%d created", gpu->idx);
}



void dvz_queue_wait(DvzGpu* gpu, uint32_t queue_idx)
{
    ANN(gpu);
    ASSERT(queue_idx < gpu->queues.queue_count);
    // log_trace("waiting for queue #%d", queue_idx);
    vkQueueWaitIdle(gpu->queues.queues[queue_idx]);
}



void dvz_gpu_wait(DvzGpu* gpu)
{
    ANN(gpu);
    log_trace("waiting for device");
    if (gpu->device != VK_NULL_HANDLE)
        vkDeviceWaitIdle(gpu->device);
}



void dvz_gpu_destroy(DvzGpu* gpu)
{
    ANN(gpu);
    log_trace("starting destruction of GPU #%d...", gpu->idx);
    if (!dvz_obj_is_created(&gpu->obj))
    {

        log_trace("skip destruction of GPU as it was not properly created");
        ASSERT(gpu->device == 0);
        return;
    }
    VkDevice device = gpu->device;
    ASSERT(device != VK_NULL_HANDLE);

    // Destroy the command pools.
    log_trace("GPU destroy %d command pool(s)", gpu->queues.queue_family_count);
    for (uint32_t i = 0; i < gpu->queues.queue_family_count; i++)
    {
        if (gpu->queues.cmd_pools[i] != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(device, gpu->queues.cmd_pools[i], NULL);
            gpu->queues.cmd_pools[i] = VK_NULL_HANDLE;
        }
    }

    // Destroy the descriptor pools.
    if (gpu->dset_pool != VK_NULL_HANDLE)
    {
        log_trace("destroy descriptor pool");
        vkDestroyDescriptorPool(gpu->device, gpu->dset_pool, NULL);
        gpu->dset_pool = VK_NULL_HANDLE;
    }

    // Destroy the allocator.
    ASSERT(gpu->allocator != VK_NULL_HANDLE);
    vmaDestroyAllocator(gpu->allocator);

    // Destroy the device.
    log_trace("destroy device");
    if (gpu->device != VK_NULL_HANDLE)
    {
        vkDestroyDevice(gpu->device, NULL);
        gpu->device = VK_NULL_HANDLE;
    }

    dvz_commands_destroy(&gpu->cmd);

    // dvz_obj_destroyed(&gpu->obj);
    dvz_obj_init(&gpu->obj);
    gpu->queues.queue_count = 0;
    log_trace("GPU #%d destroyed", gpu->idx);
}
