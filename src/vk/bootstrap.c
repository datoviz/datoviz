/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  bootstrap */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>
#include <vulkan/vulkan_core.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_device.h"
#include "_instance.h"
#include "datoviz/vk/bootstrap.h"
#include "datoviz/vk/device.h"
#include "datoviz/vk/gpu.h"
#include "datoviz/vk/instance.h"
#include "datoviz/vk/memory.h"
#include "datoviz/vk/queues.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

void dvz_bootstrap(DvzBootstrap* bootstrap, int flags)
{
    ANN(bootstrap);
    bootstrap->flags = flags;

    if (bootstrap->instance == NULL)
    {
        bootstrap->instance = (DvzInstance*)dvz_calloc(1, sizeof(DvzInstance));
    }
    DvzInstance* instance = bootstrap->instance;
    ANN(instance);

    if (bootstrap->device == NULL)
    {
        bootstrap->device = (DvzDevice*)dvz_calloc(1, sizeof(DvzDevice));
    }
    DvzDevice* device = bootstrap->device;
    ANN(device);

    dvz_instance(instance, DVZ_INSTANCE_VALIDATION_FLAGS);

    if ((flags & DVZ_BOOTSTRAP_MANUAL_CREATE_INSTANCE) != 0)
        return;

    dvz_instance_build(instance, VK_API_VERSION_1_3);

    // Obtain the first GPU for simplicity.
    uint32_t count = 0;
    DvzGpu* gpus = dvz_instance_gpus(instance, &count);
    bootstrap->gpu = &gpus[0];

    // Query the queues.
    DvzQueueCaps* qc = dvz_gpu_queue_caps(bootstrap->gpu);

    // Initialize a device.
    dvz_gpu_device(bootstrap->gpu, device);

    // Find an adequate set of queues to request.
    dvz_queues(qc, &device->queues);

    if ((flags & DVZ_BOOTSTRAP_MANUAL_CREATE_DEVICE) != 0)
        return;

    // Create the device.
    dvz_device_build(device);

    if ((flags & DVZ_BOOTSTRAP_MANUAL_CREATE_ALLOCATOR) != 0)
        return;

    // Create the memory allocator.
    dvz_device_allocator(device, 0, &bootstrap->allocator);
}



DvzInstance* dvz_bootstrap_instance(DvzBootstrap* bootstrap)
{
    ANN(bootstrap);
    return bootstrap->instance;
}



DvzGpu* dvz_bootstrap_gpu(DvzBootstrap* bootstrap)
{
    ANN(bootstrap);
    return bootstrap->gpu;
}



DvzDevice* dvz_bootstrap_device(DvzBootstrap* bootstrap)
{
    ANN(bootstrap);
    return bootstrap->device;
}



DvzVma* dvz_bootstrap_allocator(DvzBootstrap* bootstrap)
{
    ANN(bootstrap);
    return &bootstrap->allocator;
}



/**
 * Return the current bootstrap validation error count.
 *
 * @param bootstrap the bootstrap
 * @return the number of validation errors
 */
uint32_t dvz_bootstrap_error_count(DvzBootstrap* bootstrap)
{
    ANN(bootstrap);
    if (bootstrap->instance != NULL)
    {
        return dvz_instance_error_count(bootstrap->instance);
    }
    return bootstrap->validation_error_count;
}



void dvz_bootstrap_destroy(DvzBootstrap* bootstrap)
{
    ANN(bootstrap);

    bootstrap->validation_error_count = dvz_bootstrap_error_count(bootstrap);
    dvz_allocator_destroy(&bootstrap->allocator);
    if (bootstrap->device != NULL)
    {
        bootstrap->device->is_heap_allocated = true;
        dvz_device_destroy(bootstrap->device);
        bootstrap->device = NULL;
    }
    if (bootstrap->instance != NULL)
    {
        bootstrap->instance->is_heap_allocated = true;
        dvz_instance_destroy(bootstrap->instance);
        bootstrap->instance = NULL;
    }
    bootstrap->gpu = NULL;
}
