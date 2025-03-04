/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Host                                                                                         */
/*************************************************************************************************/

// NOTE: glfw is required because it exposes the Vulkan extensions used by the instance
#include "glfw_utils.h"

#include "common.h"
#include "host.h"
#include "vklite.h"
#include "vkutils.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static const char** backend_extensions(DvzBackend backend, uint32_t* required_extension_count)
{
    ASSERT(backend != DVZ_BACKEND_NONE);

    const char** required_extensions = NULL;

    // Backend initialization and required extensions.
    switch (backend)
    {
    case DVZ_BACKEND_GLFW:
#if HAS_GLFW
        // ASSERT(glfwVulkanSupported() != 0);
        required_extensions = glfwGetRequiredInstanceExtensions(required_extension_count);
        log_trace("%d extension(s) required by backend GLFW", *required_extension_count);
#endif
        break;
    case DVZ_BACKEND_QT:
        *required_extension_count = 1;
        required_extensions = (const char**)calloc(*required_extension_count, sizeof(char*));
        required_extensions[0] = VK_KHR_SURFACE_EXTENSION_NAME;
        break;
    default:
        break;
    }

    return required_extensions;
}



/*************************************************************************************************/
/*  Host                                                                                         */
/*************************************************************************************************/

DvzHost* dvz_host(DvzBackend backend)
{
    log_set_level_env();
    log_debug("create the host with backend %d", backend);

    DvzHost* host = calloc(1, sizeof(DvzHost));
    ANN(host);
    dvz_obj_init(&host->obj);
    host->obj.type = DVZ_OBJECT_TYPE_HOST;

#if SWIFTSHADER
    if (backend != DVZ_BACKEND_OFFSCREEN)
    {
        log_warn("when the library is compiled for switshader, offscreen rendering is mandatory");
        backend = DVZ_BACKEND_OFFSCREEN;
    }
#endif

    // Backend-specific initialization code.
    host->backend = backend;
    backend_init(backend);

    // Initialize the global clock.
    host->clock = dvz_clock();

    host->gpus = dvz_container(DVZ_CONTAINER_DEFAULT_COUNT, sizeof(DvzGpu), DVZ_OBJECT_TYPE_GPU);

    return host;
}



void dvz_host_extension(DvzHost* host, const char* extension_name)
{
    ANN(host);
    ANN(extension_name);
    host->extensions[host->extension_count++] = extension_name;
}



void dvz_host_create(DvzHost* host)
{
    ANN(host);

    // TODO
    // // Which extensions are required? Depends on the backend.
    // uint32_t required_extension_count = 0;
    // const char** required_extensions =
    //     backend_extensions(host->backend, &required_extension_count);

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

    log_debug("destroy the host with backend %d", host->backend);
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

    // Backend-specific termination code.
    backend_terminate(host->backend);

    // Free the App memory.
    int res = (int)host->n_errors;
    FREE(host);
    log_trace("host destroyed");

    return res;
}
