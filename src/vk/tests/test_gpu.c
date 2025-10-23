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
#include "datoviz/math/types.h"
#include "datoviz/vk/gpu.h"
#include "datoviz/vk/instance.h"
#include "test_vk.h"
#include "testing.h"



/*************************************************************************************************/
/*  GPU tests                                                                                    */
/*************************************************************************************************/

int test_gpu_props(TstSuite* suite, TstItem* tstitem)
{

    ANN(suite);
    ANN(tstitem);

    // Create an instance.
    DvzInstance instance = {0};
    dvz_instance(&instance, DVZ_INSTANCE_VALIDATION_FLAGS);
    dvz_instance_create(&instance, VK_API_VERSION_1_3);

    uint32_t count = 0;
    DvzGpu* gpus = dvz_instance_gpus(&instance, &count);
    DvzGpu* gpu = &gpus[0];
    dvz_gpu_probe_properties(gpu);

    VkPhysicalDeviceProperties* props = dvz_gpu_properties10(gpu);
    log_info("device ID: %u", props->deviceID);
    log_info("device name: %s", props->deviceName);
    log_info("device type: %u", props->deviceType);
    log_info("API version: %u", props->apiVersion);
    log_info("driver version: %u", props->driverVersion);
    log_info("vendor ID: %u", props->vendorID);
    log_info("max image dim 2D: %u", props->limits.maxImageDimension2D);

    VkPhysicalDeviceVulkan11Properties* props11 = dvz_gpu_properties11(gpu);
    log_info("max memory allocation size: %s", dvz_pretty_size(props11->maxMemoryAllocationSize));

    VkPhysicalDeviceVulkan12Properties* props12 = dvz_gpu_properties12(gpu);
    log_info(
        "max descriptor set update after bind samplers: %u",
        props12->maxDescriptorSetUpdateAfterBindSamplers);

    VkPhysicalDeviceVulkan13Properties* props13 = dvz_gpu_properties13(gpu);
    log_info("max buffer size: %s", dvz_pretty_size(props13->maxBufferSize));

    dvz_instance_destroy(&instance);
    return 0;
}
