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
