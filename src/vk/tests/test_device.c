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

#include "../_device.h"
#include "../_gpu.h"
#include "_alloc.h"
#include "_assertions.h"
#include "datoviz/vk/instance.h"
#include "datoviz/vk/queues.h"
#include "datoviz_testing.h"
#include "test_vk.h"
#include "testing.h"
#include "vulkan_core.h"



/*************************************************************************************************/
/*  Queue tests                                                                                  */
/*************************************************************************************************/

int test_device_1(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    // Create an instance.
    DvzInstanceConfig icfg = dvz_instance_config();
    icfg.flags = DVZ_INSTANCE_VALIDATION_FLAGS;
    DvzInstance* instance = dvz_instance_create(&icfg);
    AT(instance != NULL);

    // Query the queues.
    DvzQueueCaps qc = {0};
    const uint32_t gpu_index = dvz_testing_gpu_index(suite);
    AT(dvz_instance_gpu_queue_caps(instance, gpu_index, &qc));

    // Initialize a device.
    DvzQueues queues = {0};
    dvz_queues(&qc, &queues);
    DvzDeviceConfig dcfg = dvz_device_config(instance);
    dvz_device_config_set_gpu_index(&dcfg, gpu_index);
    for (uint32_t i = 0; i < queues.queue_count; i++)
    {
        DvzQueue* queue = &queues.queues[i];
        dvz_device_config_request_queue(&dcfg, queue->family_idx, 1);
    }
    DvzDevice* device = dvz_device_create(&dcfg);
    AT(device != NULL);
    VkPhysicalDevice expected = VK_NULL_HANDLE;
    AT(dvz_instance_gpu_handle(instance, gpu_index, &expected));
    AT(device->gpu->pdevice == expected);

    // Cleanup.
    dvz_device_destroy(device);
    dvz_instance_destroy(instance);
    return 0;
}



int test_device_2(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    // Create an instance.
    DvzInstanceConfig icfg = dvz_instance_config();
    icfg.flags = DVZ_INSTANCE_VALIDATION_FLAGS;
    DvzInstance* instance = dvz_instance_create(&icfg);
    AT(instance != NULL);

    // Obtain a GPU.
    uint32_t count = 0;
    DvzGpu* gpus = dvz_instance_gpus(instance, &count);
    const uint32_t gpu_index = dvz_testing_gpu_index(suite);
    AT(gpu_index < count);
    DvzGpu* gpu = &gpus[gpu_index];

    // Initialize a device.
    DvzDeviceConfig dcfg = dvz_device_config(instance);

    // Device extensions.
    dvz_gpu_probe_extensions(gpu);
    uint32_t extension_count = 0;
    char** extensions = dvz_gpu_supported_extensions(gpu, &extension_count);
    AT(extensions != NULL);
    AT(extension_count > 0);
    dvz_device_config_request_extension(&dcfg, "VK_KHR_dynamic_rendering");

    // Queue requests.
    DvzQueueCaps qc = {0};
    AT(dvz_instance_gpu_queue_caps(instance, gpu_index, &qc));
    DvzQueues queues = {0};
    dvz_queues(&qc, &queues);
    AT(queues.queue_count > 0);
    const DvzQueue* main_queue = &queues.queues[DVZ_QUEUE_MAIN];
    AT(main_queue->is_set);
    dvz_device_config_set_gpu_index(&dcfg, gpu_index);
    dvz_device_config_request_queue(&dcfg, main_queue->family_idx, 1);

    // Features.
    VkPhysicalDeviceFeatures features10 = {0};
    VkPhysicalDeviceVulkan11Features features11 = {0};
    VkPhysicalDeviceVulkan12Features features12 = {0};
    VkPhysicalDeviceVulkan13Features features13 = {0};
    features10.depthClamp = true;
    features11.multiview = true;
    features12.bufferDeviceAddress = true;
    features13.dynamicRendering = true;
    dvz_device_config_set_features10(&dcfg, &features10);
    dvz_device_config_set_features11(&dcfg, &features11);
    dvz_device_config_set_features12(&dcfg, &features12);
    dvz_device_config_set_features13(&dcfg, &features13);

    // Create the device.
    DvzDevice* device = dvz_device_create(&dcfg);
    AT(device != NULL);
    AT(device->vk_device != VK_NULL_HANDLE);

    // Get queue.
    DvzQueue* queue = dvz_device_queue(device, DVZ_QUEUE_MAIN);
    AT(queue != NULL);

    // Cleanup.
    dvz_device_destroy(device);
    dvz_instance_destroy(instance);
    return 0;
}



int test_device_3(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzInstanceConfig icfg = dvz_instance_config();
    icfg.flags = DVZ_INSTANCE_VALIDATION_FLAGS;
    DvzInstance* instance = dvz_instance_create(&icfg);
    AT(instance != NULL);

    uint32_t count = 0;
    DvzGpu* gpus = dvz_instance_gpus(instance, &count);
    AT(gpus != NULL);
    AT(count > 0);

    DvzDeviceConfig dcfg = dvz_device_config(instance);
    dvz_device_config_set_gpu_index(&dcfg, count);
    DvzDevice* device = NULL;
    AT_EXPECTED_ERROR_STRICT(suite, (device = dvz_device_create(&dcfg)) == NULL);

    dvz_instance_destroy(instance);
    return 0;
}



int test_device_4(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzInstanceConfig icfg = dvz_instance_config();
    icfg.flags = DVZ_INSTANCE_VALIDATION_FLAGS;
    DvzInstance* instance = dvz_instance_create(&icfg);
    AT(instance != NULL);

    dvz_instance_destroy(instance);
    return 0;
}



int test_device_destroy_rebuild(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzInstanceConfig icfg = dvz_instance_config();
    icfg.flags = DVZ_INSTANCE_VALIDATION_FLAGS;
    DvzInstance* instance = dvz_instance_create(&icfg);
    AT(instance != NULL);

    uint32_t gpu_count = 0;
    DvzGpu* gpus = dvz_instance_gpus(instance, &gpu_count);
    AT(gpus != NULL);
    AT(gpu_count > 0);

    DvzQueueCaps qc = {0};
    const uint32_t gpu_index = dvz_testing_gpu_index(suite);
    AT(gpu_index < gpu_count);
    AT(dvz_instance_gpu_queue_caps(instance, gpu_index, &qc));

    DvzQueues queues = {0};
    dvz_queues(&qc, &queues);
    AT(queues.queue_count > 0);

    DvzDevice device = {0};
    dvz_gpu_device(&gpus[gpu_index], &device);
    for (uint32_t i = 0; i < queues.queue_count; i++)
    {
        DvzQueue* queue = &queues.queues[i];
        dvz_device_request_queues(&device, queue->family_idx, 1);
    }

    AT(dvz_device_build(&device) == 0);
    AT(device.vk_device != VK_NULL_HANDLE);
    AT(device.dpool != VK_NULL_HANDLE);

    dvz_device_destroy(&device);
    AT(device.vk_device == VK_NULL_HANDLE);
    AT(device.dpool == VK_NULL_HANDLE);

    dvz_device_destroy(&device);
    AT(device.vk_device == VK_NULL_HANDLE);
    AT(device.dpool == VK_NULL_HANDLE);

    AT(dvz_device_build(&device) == 0);
    AT(device.vk_device != VK_NULL_HANDLE);
    AT(device.dpool != VK_NULL_HANDLE);

    dvz_device_destroy(&device);
    dvz_instance_destroy(instance);
    return 0;
}



int test_device_build_requires_destroy(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzInstanceConfig icfg = dvz_instance_config();
    icfg.flags = DVZ_INSTANCE_VALIDATION_FLAGS;
    DvzInstance* instance = dvz_instance_create(&icfg);
    AT(instance != NULL);

    uint32_t gpu_count = 0;
    DvzGpu* gpus = dvz_instance_gpus(instance, &gpu_count);
    AT(gpus != NULL);
    AT(gpu_count > 0);

    DvzQueueCaps qc = {0};
    const uint32_t gpu_index = dvz_testing_gpu_index(suite);
    AT(gpu_index < gpu_count);
    AT(dvz_instance_gpu_queue_caps(instance, gpu_index, &qc));

    DvzQueues queues = {0};
    dvz_queues(&qc, &queues);
    AT(queues.queue_count > 0);

    DvzDevice device = {0};
    dvz_gpu_device(&gpus[gpu_index], &device);
    for (uint32_t i = 0; i < queues.queue_count; i++)
    {
        DvzQueue* queue = &queues.queues[i];
        dvz_device_request_queues(&device, queue->family_idx, 1);
    }

    AT(dvz_device_build(&device) == 0);
    AT(device.vk_device != VK_NULL_HANDLE);

    AT_EXPECTED_ERROR_STRICT(suite, dvz_device_build(&device) != 0);
    AT(device.vk_device != VK_NULL_HANDLE);

    dvz_device_destroy(&device);
    AT(device.vk_device == VK_NULL_HANDLE);

    AT(dvz_device_build(&device) == 0);
    AT(device.vk_device != VK_NULL_HANDLE);

    dvz_device_destroy(&device);
    dvz_instance_destroy(instance);
    return 0;
}
