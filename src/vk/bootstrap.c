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

#include "_assertions.h"
#include "_gpu.h"
#include "datoviz/vk/bootstrap.h"
#include "datoviz/vk/device.h"
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
    bootstrap->gpu_index = UINT32_MAX;
    bootstrap->owns_instance = false;
    bootstrap->owns_device = false;

    if (bootstrap->instance == NULL)
    {
        if ((flags & DVZ_BOOTSTRAP_MANUAL_CREATE_INSTANCE) != 0)
            return;

        DvzInstanceConfig icfg = dvz_instance_default_config();
        icfg.flags = DVZ_INSTANCE_VALIDATION_FLAGS;
        bootstrap->instance = dvz_instance_create(&icfg);
        bootstrap->owns_instance = bootstrap->instance != NULL;
    }
    DvzInstance* instance = bootstrap->instance;
    if (instance == NULL)
        return;

    if ((flags & DVZ_BOOTSTRAP_MANUAL_CREATE_INSTANCE) != 0)
        return;

    // Obtain the first GPU for simplicity.
    uint32_t count = 0;
    DvzGpu* gpus = dvz_instance_gpus(instance, &count);
    if (gpus == NULL || count == 0)
        return;
    bootstrap->gpu_index = 0;

    if ((flags & DVZ_BOOTSTRAP_MANUAL_CREATE_DEVICE) != 0)
        return;

    if (bootstrap->device != NULL)
        bootstrap->owns_device = false;

    if (bootstrap->device == NULL)
    {
        DvzQueueCaps qc = {0};
        if (!dvz_instance_gpu_queue_caps(instance, bootstrap->gpu_index, &qc))
        {
            return;
        }
        DvzQueues queues = {0};
        dvz_queues(&qc, &queues);

        DvzDeviceConfig dcfg = dvz_device_default_config(instance);
        for (uint32_t i = 0; i < queues.queue_count; i++)
        {
            DvzQueue* queue = &queues.queues[i];
            dvz_device_config_request_queue(&dcfg, queue->family_idx, 1);
        }
        bootstrap->device = dvz_device_create(&dcfg);
        bootstrap->owns_device = bootstrap->device != NULL;
    }
    DvzDevice* device = bootstrap->device;
    if (device == NULL)
        return;

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



/**
 * Return the bootstrap selected GPU index.
 *
 * @param bootstrap the bootstrap
 * @return selected GPU index in the instance, or UINT32_MAX if unavailable
 */
uint32_t dvz_bootstrap_gpu_index(DvzBootstrap* bootstrap)
{
    ANN(bootstrap);
    return bootstrap->gpu_index;
}



/**
 * Return the bootstrap selected GPU descriptor.
 *
 * @param bootstrap the bootstrap
 * @param[out] out_info destination GPU descriptor
 * @return whether the descriptor could be retrieved
 */
bool dvz_bootstrap_gpu_info(DvzBootstrap* bootstrap, DvzGpuInfo* out_info)
{
    ANN(bootstrap);
    ANN(out_info);
    if (bootstrap->instance == NULL)
    {
        return false;
    }
    return (
        bootstrap->gpu_index != UINT32_MAX &&
        dvz_instance_gpu_info(bootstrap->instance, bootstrap->gpu_index, out_info));
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
    if (bootstrap->device != NULL && bootstrap->owns_device)
    {
        dvz_device_destroy(bootstrap->device);
    }
    if (bootstrap->instance != NULL && bootstrap->owns_instance)
    {
        dvz_instance_destroy(bootstrap->instance);
    }
    bootstrap->device = NULL;
    bootstrap->instance = NULL;
    bootstrap->gpu_index = UINT32_MAX;
    bootstrap->owns_device = false;
    bootstrap->owns_instance = false;
}
