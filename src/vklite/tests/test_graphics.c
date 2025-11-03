/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing graphics                                                                             */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <float.h>
#include <inttypes.h>
#include <stdbool.h>

#include "../../vk/macros.h"
#include "../../vk/tests/test_vk.h"
#include "../../vk/types.h"
#include "../types.h"
#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "datoviz/common/macros.h"
#include "datoviz/vk/bootstrap.h"
#include "datoviz/vk/device.h"
#include "datoviz/vklite/graphics.h"
#include "datoviz/vklite/images.h"
#include "datoviz/vklite/shader.h"
#include "datoviz/vklite/slots.h"
#include "test_vklite.h"
#include "testing.h"
#include "vulkan_core.h"



/*************************************************************************************************/
/*  Graphics tests                                                                            */
/*************************************************************************************************/

int test_vklite_graphics_1(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    // Bootstrap.
    DvzBootstrap bootstrap = {0};
    dvz_bootstrap(&bootstrap, DVZ_BOOTSTRAP_MANUAL_CREATE_DEVICE);

    DvzDevice* device = &bootstrap.device;

    // Create a device with support for dynamic rendering.
    VkPhysicalDeviceVulkan13Features* features = dvz_device_request_features13(device);
    features->dynamicRendering = true;
    dvz_device_create(device);

    // Graphics setup.
    DvzGraphics graphics = {0};
    dvz_graphics(device, &graphics);

    // Shaders.
    DvzShader vs = {0};
    DvzShader fs = {0};
    DvzSize vs_size = 0;
    DvzSize fs_size = 0;
    uint32_t* vs_spv = dvz_test_shader_load("hello_triangle.vert.spv", &vs_size);
    uint32_t* fs_spv = dvz_test_shader_load("hello_triangle.frag.spv", &fs_size);
    ANN(vs_spv);
    ANN(fs_spv);
    dvz_shader(device, vs_size, vs_spv, &vs);
    dvz_shader(device, fs_size, fs_spv, &fs);
    dvz_graphics_shader(&graphics, VK_SHADER_STAGE_VERTEX_BIT, vs.vk_shader);
    dvz_graphics_shader(&graphics, VK_SHADER_STAGE_FRAGMENT_BIT, fs.vk_shader);

    // Slots.
    DvzSlots slots = {0};
    dvz_slots(&bootstrap.device, &slots);
    VK_CHECK_RESULT(dvz_slots_create(&slots));
    VkPipelineLayout layout = dvz_slots_handle(&slots);
    dvz_graphics_layout(&graphics, layout);

    // Attachments.
    dvz_graphics_attachment_color(&graphics, 0, VK_FORMAT_R8G8B8A8_UNORM);
    dvz_graphics_blend_color(
        &graphics, 0, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        VK_BLEND_OP_ADD, 0xF);
    dvz_graphics_blend_alpha(
        &graphics, 0, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD);

    // Fixed state.
    dvz_graphics_primitive(
        &graphics, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, DVZ_GRAPHICS_FLAGS_FIXED);

    // Dynamic state.
    dvz_graphics_viewport(&graphics, 0, 0, 800, 600, 0, 1, DVZ_GRAPHICS_FLAGS_DYNAMIC);

    // Graphics creation.
    dvz_graphics_create(&graphics);

    // Cleanup.
    dvz_shader_destroy(&vs);
    dvz_shader_destroy(&fs);
    dvz_slots_destroy(&slots);
    dvz_graphics_destroy(&graphics);
    dvz_bootstrap_destroy(&bootstrap);

    dvz_free(vs_spv);
    dvz_free(fs_spv);

    RETURN_VALIDATION
}
