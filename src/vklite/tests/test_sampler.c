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

#include "test_vk.h"
#include "_assertions.h"
#include "datoviz/vk/gpu_ctx.h"
#include "datoviz/vklite/sampler.h"
#include "test_vklite.h"
#include "testing.h"
#include "vulkan_core.h"



/*************************************************************************************************/
/*  vklite tests                                                                                 */
/*************************************************************************************************/

int test_vklite_sampler_1(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    // Bootstrap.
    DvzGpuCtxConfig cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceFeatures features10 = {0};
    features10.samplerAnisotropy = true;
    dvz_gpu_ctx_config_features10(&cfg, &features10);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&cfg);
    ANN(ctx);

    DvzSampler* sampler = dvz_sampler_create_wrapper();
    ANN(sampler);
    dvz_sampler(dvz_gpu_ctx_device(ctx), sampler);
    dvz_sampler_min_filter(sampler, VK_FILTER_LINEAR);
    dvz_sampler_mag_filter(sampler, VK_FILTER_LINEAR);
    dvz_sampler_address_mode(sampler, DVZ_SAMPLER_AXIS_U, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    dvz_sampler_anisotropy(sampler, 8);
    AT(dvz_sampler_create(sampler) == 0);

    // Cleanup.
    dvz_sampler_destroy(sampler);
    dvz_sampler_free(sampler);
    uint32_t err_count = dvz_gpu_ctx_error_count(ctx);
    dvz_gpu_ctx_destroy(ctx);

    return err_count > 0;
}
