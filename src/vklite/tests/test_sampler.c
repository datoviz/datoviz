/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing sampler                                                                              */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>

#include "../../vk/tests/test_vk.h"
#include "_assertions.h"
#include "datoviz/vk/bootstrap.h"
#include "datoviz/vklite/sampler.h"
#include "test_vklite.h"
#include "testing.h"
#include "vulkan_core.h"



/*************************************************************************************************/
/*  vklite tests                                                                                 */
/*************************************************************************************************/

int test_vklite_sampler_1(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    // Bootstrap.
    DvzBootstrap bootstrap = {0};
    dvz_bootstrap(&bootstrap, DVZ_BOOTSTRAP_MANUAL_CREATE_DEVICE);
    ANN(dvz_bootstrap_instance(&bootstrap));
    uint32_t gpu_index = dvz_bootstrap_gpu_index(&bootstrap);
    AT(gpu_index != UINT32_MAX);
    DvzQueueCaps qc = {0};
    AT(dvz_instance_gpu_queue_caps(dvz_bootstrap_instance(&bootstrap), gpu_index, &qc));
    DvzQueues queues = {0};
    dvz_queues(&qc, &queues);
    DvzDeviceConfig dcfg = dvz_device_default_config(dvz_bootstrap_instance(&bootstrap));
    dvz_device_config_set_gpu_index(&dcfg, gpu_index);
    for (uint32_t i = 0; i < queues.queue_count; i++)
    {
        DvzQueue* req = &queues.queues[i];
        dvz_device_config_request_queue(&dcfg, dvz_queue_family(req), 1);
    }
    VkPhysicalDeviceFeatures features10 = {0};
    features10.samplerAnisotropy = true;
    dvz_device_config_set_features10(&dcfg, &features10);
    DvzDevice* created_device = dvz_device_create(&dcfg);
    AT(dvz_bootstrap_set_device(&bootstrap, created_device, created_device != NULL));
    AT(dvz_bootstrap_device(&bootstrap) != NULL);

    DvzSampler sampler = {0};
    dvz_sampler(dvz_bootstrap_device(&bootstrap), &sampler);
    dvz_sampler_min_filter(&sampler, VK_FILTER_LINEAR);
    dvz_sampler_mag_filter(&sampler, VK_FILTER_LINEAR);
    dvz_sampler_address_mode(&sampler, DVZ_SAMPLER_AXIS_U, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    dvz_sampler_anisotropy(&sampler, 8);
    AT(dvz_sampler_create(&sampler) == 0);

    // Cleanup.
    dvz_sampler_destroy(&sampler);
    dvz_bootstrap_destroy(&bootstrap);

    RETURN_VALIDATION
}
