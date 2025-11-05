/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing queues                                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "datoviz/vk/device.h"
#include "datoviz/vk/gpu.h"
#include "datoviz/vk/instance.h"
#include "datoviz/vk/queues.h"
#include "test_vk.h"
#include "testing.h"
#include "vulkan_core.h"



/*************************************************************************************************/
/*  Queue tests                                                                                  */
/*************************************************************************************************/

int test_device_1(TstSuite* suite, TstItem* tstitem)
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



int test_device_2(TstSuite* suite, TstItem* tstitem)
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

    // Initialize a device.
    DvzDevice device = {0};
    dvz_gpu_device(gpu, &device);

    // Device extensions.
    dvz_gpu_probe_extensions(gpu);
    uint32_t extension_count = 0;
    char** extensions = dvz_gpu_supported_extensions(gpu, &extension_count);
    dvz_strings_show(extension_count, extensions);
    dvz_device_request_extension(&device, "VK_KHR_dynamic_rendering");

    // Queue requests.
    DvzQueueCaps* qc = dvz_gpu_queue_caps(gpu);
    AT(qc);
    dvz_device_request_queues(&device, 0, 1);
    dvz_device_request_queues(&device, 1, 1);

    // Features.
    VkPhysicalDeviceFeatures* features10 = dvz_device_request_features10(&device);
    VkPhysicalDeviceVulkan11Features* features11 = dvz_device_request_features11(&device);
    VkPhysicalDeviceVulkan12Features* features12 = dvz_device_request_features12(&device);
    VkPhysicalDeviceVulkan13Features* features13 = dvz_device_request_features13(&device);

    features10->depthClamp = true;
    features11->multiview = true;
    features12->bufferDeviceAddress = true;
    features13->dynamicRendering = true;

    // Create the device.
    dvz_device_create(&device);
    AT(device.vk_device != VK_NULL_HANDLE);

    // Get queue.
    DvzQueue* queue = dvz_device_queue(&device, DVZ_QUEUE_MAIN);
    AT(queue != NULL);

    // Cleanup.
    dvz_device_destroy(&device);
    dvz_instance_destroy(&instance);
    return 0;
}
