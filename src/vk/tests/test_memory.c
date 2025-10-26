/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing memory                                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <float.h>
#include <inttypes.h>
#include <stdbool.h>

#include "../types.h"
#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "datoviz/vk/device.h"
#include "datoviz/vk/gpu.h"
#include "datoviz/vk/instance.h"
#include "datoviz/vk/queues.h"
#include "test_vk.h"
#include "testing.h"
#include "vulkan/vulkan_core.h"



/*************************************************************************************************/
/*  Memory tests                                                                                 */
/*************************************************************************************************/

int test_memory_1(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    // Create an instance.
    DvzInstance instance = {0};
    dvz_instance(&instance, DVZ_INSTANCE_VALIDATION_FLAGS);
    dvz_instance_create(&instance, VK_API_VERSION_1_3);

    // Obtain a GPU.
    uint32_t count = 0;
    DvzGpu* gpus = dvz_instance_gpus(&instance, &count);
    DvzGpu* gpu = &gpus[0];

    // Query the queues.
    DvzQueueCaps* qc = dvz_gpu_queue_caps(gpu);

    // Initialize a device.
    DvzDevice device = {0};
    dvz_gpu_device(gpu, &device);

    // Find an adequate set of queues to request.
    dvz_queues(qc, &device.queues);

    // Create the device.
    dvz_device_create(&device);

    // Cleanup.
    dvz_device_destroy(&device);
    dvz_instance_destroy(&instance);
    return 0;
}
