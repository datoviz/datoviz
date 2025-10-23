/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing GPU                                                                                  */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <float.h>
#include <inttypes.h>
#include <stdbool.h>
#include <vulkan/vulkan_core.h>

#include "../types.h"
#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "datoviz/vk/gpu.h"
#include "datoviz/vk/instance.h"
#include "test_vk.h"
#include "testing.h"



/*************************************************************************************************/
/*  GPU tests                                                                                    */
/*************************************************************************************************/

int test_gpu_1(TstSuite* suite, TstItem* tstitem)
{

    ANN(suite);
    ANN(tstitem);

    // Initialize the instance structure.
    DvzInstance instance = {0};
    dvz_instance(&instance, DVZ_INSTANCE_VALIDATION_FLAGS);
    dvz_instance_create(&instance, VK_API_VERSION_1_3);

    // DvzGpu* dvz_instance_gpus(DvzInstance* instance, uint32_t* count)
    // void dvz_gpu_probe_properties(DvzGpu* gpu)
    // VkPhysicalDeviceProperties* dvz_gpu_properties10(DvzGpu* gpu)
    // VkPhysicalDeviceVulkan11Properties* dvz_gpu_properties11(DvzGpu* gpu)

    dvz_instance_destroy(&instance);
    return 0;
}
