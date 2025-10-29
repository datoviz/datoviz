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

#include <float.h>
#include <inttypes.h>
#include <stdbool.h>

#include "../../vk/types.h"
#include "../types.h"
#include "_assertions.h"
#include "_log.h"
#include "datoviz/common/macros.h"
#include "datoviz/vk/bootstrap.h"
#include "datoviz/vklite/slots.h"
#include "test_vklite.h"
#include "testing.h"
#include "vulkan_core.h"



/*************************************************************************************************/
/*  Shader tests                                                                                */
/*************************************************************************************************/

int test_vklite_slots_1(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    // Bootstrap.
    DvzBootstrap bootstrap = {0};
    dvz_bootstrap(&bootstrap, 0);

    // Create slots.
    DvzSlots slots = {0};
    dvz_slots(&bootstrap.device, &slots);

    // Bindings.
    dvz_slots_binding(&slots, 0, 0, VK_SHADER_STAGE_ALL, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1);
    dvz_slots_binding(&slots, 0, 1, VK_SHADER_STAGE_ALL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1);
    dvz_slots_binding(&slots, 1, 0, VK_SHADER_STAGE_ALL, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1);

    // Push constants.
    dvz_slots_push(&slots, VK_SHADER_STAGE_COMPUTE_BIT, 0, 64);

    // Create the slots.
    int res = dvz_slots_create(&slots);
    AT(res == 0);

    // Retrieve the pipeline layout handle.
    VkPipelineLayout handle = dvz_slots_handle(&slots);
    AT(handle != VK_NULL_HANDLE);

    // Cleanup.
    dvz_slots_destroy(&slots);
    dvz_bootstrap_destroy(&bootstrap);
    return 0;
}
