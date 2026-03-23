/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing fixture GPU                                                                          */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "datoviz/vk/bootstrap.h"
#include "datoviz/vk/device.h"
#include "datoviz/vk/instance.h"
#include "datoviz/vk/queues.h"
#include "fixture_gpu.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzFixtureGpu
{
    DvzBootstrap bootstrap;
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create a GPU test fixture with bootstrap, device, allocator, and main queue access.
 *
 * @return allocated GPU fixture, or NULL on allocation failure
 */
DvzFixtureGpu* dvz_fixture_gpu(void)
{
    DvzFixtureGpu* fixture = (DvzFixtureGpu*)dvz_calloc(1, sizeof(DvzFixtureGpu));
    ANN(fixture);

    DvzBootstrap* bootstrap = &fixture->bootstrap;
    ANN(bootstrap);

    dvz_bootstrap(bootstrap, DVZ_BOOTSTRAP_MANUAL_CREATE_DEVICE);
    DvzInstance* instance = dvz_bootstrap_instance(bootstrap);
    ANN(instance);

    uint32_t gpu_index = dvz_bootstrap_gpu_index(bootstrap);
    ASSERT(gpu_index != UINT32_MAX);

    DvzQueueCaps qc = {0};
    ASSERT(dvz_instance_gpu_queue_caps(instance, gpu_index, &qc));

    DvzQueues queues = {0};
    dvz_queues(&qc, &queues);

    DvzDeviceConfig dcfg = dvz_device_default_config(instance);
    dvz_device_config_set_gpu_index(&dcfg, gpu_index);
    for (uint32_t i = 0; i < queues.queue_count; i++)
    {
        DvzQueue* req = &queues.queues[i];
        dvz_device_config_request_queue(&dcfg, dvz_queue_family(req), 1);
    }

    VkPhysicalDeviceFeatures fet10 = {0};
    fet10.samplerAnisotropy = true;
    fet10.sampleRateShading = true;
    dvz_device_config_set_features10(&dcfg, &fet10);

    VkPhysicalDeviceVulkan13Features fet13 = {0};
    fet13.dynamicRendering = true;
    fet13.synchronization2 = true;
    dvz_device_config_set_features13(&dcfg, &fet13);

    DvzDevice* created_device = dvz_device_create(&dcfg);
    ASSERT(dvz_bootstrap_set_device(bootstrap, created_device, created_device != NULL));
    ASSERT(dvz_bootstrap_create_allocator(bootstrap, 0) == 0);

    return fixture;
}



/**
 * Destroy a GPU test fixture.
 *
 * @param fixture the GPU fixture
 */
void dvz_fixture_gpu_destroy(DvzFixtureGpu* fixture)
{
    if (fixture == NULL)
    {
        return;
    }

    dvz_bootstrap_destroy(&fixture->bootstrap);
    dvz_free(fixture);
}



/**
 * Get the bootstrap wrapper owned by the fixture.
 *
 * @param fixture the GPU fixture
 * @return borrowed bootstrap wrapper
 */
DvzBootstrap* dvz_fixture_gpu_bootstrap(DvzFixtureGpu* fixture)
{
    ANN(fixture);
    return &fixture->bootstrap;
}



/**
 * Get the device owned by the fixture bootstrap.
 *
 * @param fixture the GPU fixture
 * @return borrowed device
 */
DvzDevice* dvz_fixture_gpu_device(DvzFixtureGpu* fixture)
{
    ANN(fixture);
    return dvz_bootstrap_device(&fixture->bootstrap);
}



/**
 * Get the allocator owned by the fixture bootstrap.
 *
 * @param fixture the GPU fixture
 * @return borrowed allocator
 */
DvzVma* dvz_fixture_gpu_alloc(DvzFixtureGpu* fixture)
{
    ANN(fixture);
    return dvz_bootstrap_allocator(&fixture->bootstrap);
}



/**
 * Get the main queue owned by the fixture device.
 *
 * @param fixture the GPU fixture
 * @return borrowed queue
 */
DvzQueue* dvz_fixture_gpu_queue(DvzFixtureGpu* fixture)
{
    ANN(fixture);
    return dvz_device_queue(dvz_fixture_gpu_device(fixture), DVZ_QUEUE_MAIN);
}
