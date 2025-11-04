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
#include "datoviz/vk/queues.h"
#include "datoviz/vklite/commands.h"
#include "datoviz/vklite/graphics.h"
#include "datoviz/vklite/images.h"
#include "datoviz/vklite/rendering.h"
#include "datoviz/vklite/shader.h"
#include "datoviz/vklite/slots.h"
#include "test_vklite.h"
#include "testing.h"
#include "vulkan_core.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;



/*************************************************************************************************/
/*  Graphics tests                                                                               */
/*************************************************************************************************/

int test_vklite_graphics_1(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    // Bootstrap.
    DvzBootstrap bootstrap = {0};
    dvz_bootstrap(&bootstrap, DVZ_BOOTSTRAP_MANUAL_CREATE_DEVICE);

    DvzDevice* device = &bootstrap.device;
    ANN(device);

    DvzQueue* queue = dvz_device_queue(device, DVZ_QUEUE_MAIN);
    ANN(queue);

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
    dvz_graphics_shader(&graphics, VK_SHADER_STAGE_VERTEX_BIT, dvz_shader_handle(&vs));
    dvz_graphics_shader(&graphics, VK_SHADER_STAGE_FRAGMENT_BIT, dvz_shader_handle(&fs));

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
    dvz_graphics_viewport(&graphics, 0, 0, WIDTH, HEIGHT, 0, 1, DVZ_GRAPHICS_FLAGS_DYNAMIC);

    // Graphics creation.
    dvz_graphics_create(&graphics);

    // Rendering.
    DvzRendering rendering = {0};
    dvz_rendering(&rendering);
    dvz_rendering_area(&rendering, 0, 0, WIDTH, HEIGHT);

    // Image to render to.
    VkImageLayout img_layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;

    DvzImages images = {0};
    dvz_images(&bootstrap.device, &bootstrap.allocator, VK_IMAGE_TYPE_2D, 1, &images);
    dvz_images_format(&images, VK_FORMAT_R8G8B8A8_UNORM);
    dvz_images_size(&images, WIDTH, HEIGHT, 1);
    dvz_images_usage(&images, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    dvz_images_layout(&images, img_layout);
    dvz_images_create(&images);

    // Image views.
    DvzImageViews view = {0};
    dvz_image_views(&images, &view);
    dvz_image_views_create(&view);

    // Attachments.
    DvzAttachment* attachment = dvz_rendering_color(&rendering, 0);
    dvz_attachment_image(attachment, dvz_image_views_handle(&view, 0), img_layout);
    dvz_attachment_ops(attachment, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);

    // Command buffer.
    DvzCommands cmds = {0};
    dvz_commands(device, queue, 1, &cmds);
    dvz_cmd_begin(&cmds, 0);
    dvz_cmd_rendering_begin(&cmds, 0, &rendering);
    dvz_cmd_bind_graphics(&cmds, 0, &graphics);
    dvz_cmd_rendering_end(&cmds, 0);
    dvz_cmd_end(&cmds, 0);

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
