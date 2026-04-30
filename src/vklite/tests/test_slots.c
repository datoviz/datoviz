/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing slots                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <volk.h>

#include "test_vk.h"
#include "_assertions.h"
#include "datoviz/vk/device.h"
#include "datoviz/vk/gpu_ctx.h"
#include "datoviz/vklite/slots.h"
#include "test_vklite.h"
#include "testing.h"
#include "vulkan_core.h"



/*************************************************************************************************/
/*  Slots tests                                                                                  */
/*************************************************************************************************/

int test_vklite_slots_1(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    // Bootstrap.
    DvzGpuCtxConfig cfg = dvz_gpu_ctx_config();
    DvzGpuCtx* ctx = dvz_gpu_ctx(&cfg);
    ANN(ctx);

    // Create slots.
    DvzSlots* slots = dvz_slots_create_wrapper();
    ANN(slots);
    dvz_slots(dvz_gpu_ctx_device(ctx), slots);

    // Bindings.
    dvz_slots_binding(slots, 0, 0, 1, VK_SHADER_STAGE_ALL, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    dvz_slots_binding(slots, 0, 1, 1, VK_SHADER_STAGE_ALL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
    dvz_slots_binding(slots, 1, 0, 1, VK_SHADER_STAGE_ALL, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    AT(dvz_slots_set_count(slots) == 2);
    AT(dvz_slots_binding_count(slots, 0) == 2);
    AT(dvz_slots_binding_count(slots, 1) == 1);
    AT(dvz_slots_descriptor_type(slots, 1, 0) == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    // Push constants.
    dvz_slots_push(slots, VK_SHADER_STAGE_COMPUTE_BIT, 0, 64);
    AT(dvz_slots_push_count(slots) == 1);

    // Create the slots.
    int res = dvz_slots_create(slots);
    AT(res == 0);

    // Retrieve the pipeline layout handle.
    VkPipelineLayout handle = dvz_slots_handle(slots);
    AT(handle != VK_NULL_HANDLE);

    // Cleanup.
    dvz_slots_destroy(slots);
    dvz_slots_destroy(slots);
    AT(dvz_slots_handle(slots) == VK_NULL_HANDLE);
    AT(dvz_slots_set_layout(slots, 0) == VK_NULL_HANDLE);
    dvz_slots_free(slots);
    uint32_t err_count = dvz_gpu_ctx_error_count(ctx);
    dvz_gpu_ctx_destroy(ctx);

    return err_count > 0;
}



int test_vklite_slots_create_failure_unwinds_layouts(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzGpuCtxConfig cfg = dvz_gpu_ctx_config();
    DvzGpuCtx* ctx = dvz_gpu_ctx(&cfg);
    ANN(ctx);

    DvzDevice* device = dvz_gpu_ctx_device(ctx);
    ANN(device);

    VkPhysicalDeviceProperties props = {0};
    vkGetPhysicalDeviceProperties(dvz_device_physical_device(device), &props);

    DvzSlots* slots = dvz_slots_create_wrapper();
    ANN(slots);
    dvz_slots(device, slots);
    dvz_slots_binding(slots, 0, 0, 1, VK_SHADER_STAGE_ALL, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    dvz_slots_push(
        slots, VK_SHADER_STAGE_COMPUTE_BIT, 0, (DvzSize)props.limits.maxPushConstantsSize + 4);

    AT_EXPECTED_ERROR_STRICT(suite, dvz_slots_create(slots) != 0);
    AT(dvz_slots_handle(slots) == VK_NULL_HANDLE);
    AT(dvz_slots_set_layout(slots, 0) == VK_NULL_HANDLE);

    dvz_slots(device, slots);
    dvz_slots_binding(slots, 0, 0, 1, VK_SHADER_STAGE_ALL, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    AT(dvz_slots_create(slots) == 0);
    AT(dvz_slots_handle(slots) != VK_NULL_HANDLE);
    dvz_slots_destroy(slots);
    dvz_slots_free(slots);

    uint32_t err_count = dvz_gpu_ctx_error_count(ctx);
    dvz_gpu_ctx_destroy(ctx);

    return err_count > 0;
}
