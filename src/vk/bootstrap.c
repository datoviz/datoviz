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

#include "_assertions.h"
#include "datoviz/common/macros.h"
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
    // TODO: flags

    DvzInstance* instance = &bootstrap->instance;
    ANN(instance);

    DvzDevice* device = &bootstrap->device;
    ANN(device);

    dvz_instance(instance, DVZ_INSTANCE_VALIDATION_FLAGS);

    if ((flags & DVZ_BOOTSTRAP_MANUAL_CREATE_INSTANCE) != 0)
        return;

    dvz_instance_create(instance, VK_API_VERSION_1_3);

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
    dvz_device_create(device);

    if ((flags & DVZ_BOOTSTRAP_MANUAL_CREATE_ALLOCATOR) != 0)
        return;

    // Create the memory allocator.
    dvz_device_allocator(device, 0, &bootstrap->allocator);
}



DvzInstance* dvz_bootstrap_instance(DvzBootstrap* bootstrap)
{
    ANN(bootstrap);
    return &bootstrap->instance;
}



DvzGpu* dvz_bootstrap_gpu(DvzBootstrap* bootstrap)
{
    ANN(bootstrap);
    return bootstrap->gpu;
}



DvzDevice* dvz_bootstrap_device(DvzBootstrap* bootstrap)
{
    ANN(bootstrap);
    return &bootstrap->device;
}



DvzVma* dvz_bootstrap_allocator(DvzBootstrap* bootstrap)
{
    ANN(bootstrap);
    return &bootstrap->allocator;
}



void dvz_bootstrap_destroy(DvzBootstrap* bootstrap)
{
    ANN(bootstrap);

    dvz_allocator_destroy(&bootstrap->allocator);
    dvz_device_destroy(&bootstrap->device);
    dvz_instance_destroy(&bootstrap->instance);
}
