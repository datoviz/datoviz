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
#include "datoviz/vk/device.h"
#include "datoviz/vk/gpu_ctx.h"
#include "datoviz/vk/queues.h"
#include "fixture_gpu.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzFixtureGpu
{
    DvzGpuCtx* ctx;
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create a GPU test fixture with GPU context, device, allocator, and main queue access.
 *
 * @return allocated GPU fixture, or NULL on allocation failure
 */
DvzFixtureGpu* dvz_fixture_gpu(void)
{
    DvzFixtureGpu* fixture = (DvzFixtureGpu*)dvz_calloc(1, sizeof(DvzFixtureGpu));
    ANN(fixture);

    DvzGpuCtxConfig cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceFeatures features10 = {0};
    features10.samplerAnisotropy = true;
    features10.sampleRateShading = true;
    dvz_gpu_ctx_config_features10(&cfg, &features10);

    VkPhysicalDeviceVulkan13Features features13 = {0};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&cfg, &features13);
    fixture->ctx = dvz_gpu_ctx(&cfg);
    ASSERT(fixture->ctx != NULL);

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

    dvz_gpu_ctx_destroy(fixture->ctx);
    dvz_free(fixture);
}



/**
 * Get the GPU context owned by the fixture.
 *
 * @param fixture the GPU fixture
 * @return borrowed GPU context
 */
DvzGpuCtx* dvz_fixture_gpu_ctx(DvzFixtureGpu* fixture)
{
    ANN(fixture);
    return fixture->ctx;
}



/**
 * Get the device owned by the fixture GPU context.
 *
 * @param fixture the GPU fixture
 * @return borrowed device
 */
DvzDevice* dvz_fixture_gpu_device(DvzFixtureGpu* fixture)
{
    ANN(fixture);
    return dvz_gpu_ctx_device(fixture->ctx);
}



/**
 * Get the allocator owned by the fixture GPU context.
 *
 * @param fixture the GPU fixture
 * @return borrowed allocator
 */
DvzVma* dvz_fixture_gpu_alloc(DvzFixtureGpu* fixture)
{
    ANN(fixture);
    return dvz_gpu_ctx_alloc(fixture->ctx);
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
