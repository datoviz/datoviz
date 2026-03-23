/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing fixture GPU                                                                          */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/common/macros.h"
#include "datoviz/vk/device.h"
#include "datoviz/vk/gpu_ctx.h"
#include "datoviz/vk/memory.h"
#include "datoviz/vk/queues.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzFixtureGpu DvzFixtureGpu;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

EXTERN_C_ON

/**
 * Create a GPU test fixture with GPU context, device, allocator, and main queue access.
 *
 * @return allocated GPU fixture, or NULL on allocation failure
 */
DVZ_EXPORT DvzFixtureGpu* dvz_fixture_gpu(void);



/**
 * Destroy a GPU test fixture.
 *
 * @param fixture the GPU fixture
 */
DVZ_EXPORT void dvz_fixture_gpu_destroy(DvzFixtureGpu* fixture);



/**
 * Get the GPU context owned by the fixture.
 *
 * @param fixture the GPU fixture
 * @return borrowed GPU context
 */
DVZ_EXPORT DvzGpuCtx* dvz_fixture_gpu_ctx(DvzFixtureGpu* fixture);



/**
 * Get the device owned by the fixture GPU context.
 *
 * @param fixture the GPU fixture
 * @return borrowed device
 */
DVZ_EXPORT DvzDevice* dvz_fixture_gpu_device(DvzFixtureGpu* fixture);



/**
 * Get the allocator owned by the fixture GPU context.
 *
 * @param fixture the GPU fixture
 * @return borrowed allocator
 */
DVZ_EXPORT DvzVma* dvz_fixture_gpu_alloc(DvzFixtureGpu* fixture);



/**
 * Get the main queue owned by the fixture device.
 *
 * @param fixture the GPU fixture
 * @return borrowed queue
 */
DVZ_EXPORT DvzQueue* dvz_fixture_gpu_queue(DvzFixtureGpu* fixture);

EXTERN_C_OFF
