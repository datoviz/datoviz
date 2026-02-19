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

#include <inttypes.h>
#include <stdbool.h>

#include "../../vk/tests/test_vk.h"
#include "_alloc.h"
#include "_assertions.h"
#include "datoviz/fileio/fileio.h"
#include "datoviz/math/types.h"
#include "datoviz/vk/bootstrap.h"
#include "datoviz/vk/instance.h"
#include "datoviz/vk/memory.h"
#include "datoviz/vk/queues.h"
#include "datoviz/vklite/buffers.h"
#include "datoviz/vklite/commands.h"
#include "datoviz/vklite/graphics.h"
#include "datoviz/vklite/images.h"
#include "../_proto.h"
#include "datoviz/vklite/rendering.h"
#include "datoviz/vklite/shader.h"
#include "datoviz/vklite/slots.h"
#include "datoviz/vklite/sync.h"
#include "test_vklite.h"
#include "testing.h"
#include "vulkan_core.h"



/*************************************************************************************************/
/*  Graphics tests                                                                               */
/*************************************************************************************************/

int test_vklite_graphics_1(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    const uint32_t WIDTH = DVZ_PROTO_WIDTH;
    const uint32_t HEIGHT = DVZ_PROTO_HEIGHT;

    // Bootstrap.
    DvzBootstrap bootstrap = {0};
    dvz_bootstrap(&bootstrap, DVZ_BOOTSTRAP_MANUAL_CREATE_DEVICE);
    DvzInstance* instance = dvz_bootstrap_instance(&bootstrap);
    ANN(instance);
    uint32_t gpu_index = dvz_bootstrap_gpu_index(&bootstrap);
    AT(gpu_index != UINT32_MAX);
    DvzQueueCaps qc = {0};
    AT(dvz_instance_gpu_queue_caps(instance, gpu_index, &qc));
    DvzQueues queues = {0};
    dvz_queues(&qc, &queues);

    DvzDeviceConfig dcfg = dvz_device_default_config(instance);
    dvz_device_config_set_gpu_index(&dcfg, gpu_index);
    for (uint32_t i = 0; i < queues.queue_count; i++)
    {
        DvzQueue* req = &queues.queues[i];
        dvz_device_config_request_queue(&dcfg, dvz_queue_family(req), 1);
    }
    VkPhysicalDeviceVulkan13Features features13 = {0};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_device_config_set_features13(&dcfg, &features13);
    DvzDevice* created_device = dvz_device_create(&dcfg);
    AT(dvz_bootstrap_set_device(&bootstrap, created_device, created_device != NULL));
    DvzDevice* device = dvz_bootstrap_device(&bootstrap);
    AT(device != NULL);
    AT(dvz_bootstrap_create_allocator(&bootstrap, 0) == 0);

    DvzQueue* queue = dvz_device_queue(device, DVZ_QUEUE_MAIN);
    ANN(queue);

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
    dvz_slots(dvz_bootstrap_device(&bootstrap), &slots);
    AT(dvz_slots_create(&slots) == 0);
    dvz_graphics_layout(&graphics, dvz_slots_handle(&slots));

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
    dvz_graphics_scissor(&graphics, 0, 0, WIDTH, HEIGHT, DVZ_GRAPHICS_FLAGS_DYNAMIC);

    // Graphics pipeline creation.
    AT(dvz_graphics_create(&graphics) == 0);

    // Rendering.
    DvzRendering rendering = {0};
    dvz_rendering(&rendering);
    dvz_rendering_area(&rendering, 0, 0, WIDTH, HEIGHT);

    // Image to render to.
    DvzImages img = {0};
    dvz_images(dvz_bootstrap_device(&bootstrap), dvz_bootstrap_allocator(&bootstrap), VK_IMAGE_TYPE_2D, 1, &img);
    dvz_images_format(&img, VK_FORMAT_R8G8B8A8_UNORM);
    dvz_images_size(&img, WIDTH, HEIGHT, 1);
    dvz_images_usage(&img, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    dvz_images_create(&img);

    // Image views.
    DvzImageViews view = {0};
    dvz_image_views(&img, &view);
    dvz_image_views_create(&view);

    // Attachments.
    DvzAttachment* attachment = dvz_rendering_color(&rendering, 0);
    dvz_attachment_image(
        attachment, dvz_image_views_handle(&view, 0), VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL);
    dvz_attachment_ops(attachment, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);
    dvz_attachment_clear(attachment, (VkClearValue){.color.float32 = {.1, .2, .3, 1}});

    // Image barrier.
    DvzBarriers barriers = {0};
    dvz_barriers(&barriers);

    // Image transition.
    DvzBarrierImage* bimg = dvz_barriers_image(&barriers, dvz_image_handle(&img, 0));
    ANN(bimg);
    dvz_barrier_image_stage(
        bimg, VK_PIPELINE_STAGE_2_NONE, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
    dvz_barrier_image_access(bimg, 0, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
    dvz_barrier_image_layout(
        bimg, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    // Command buffer.
    DvzCommands cmds = {0};
    dvz_commands(device, queue, 1, &cmds);
    dvz_cmd_begin(&cmds);
    dvz_cmd_barriers(&cmds, &barriers);
    dvz_cmd_rendering_begin(&cmds, &rendering);
    dvz_cmd_bind_graphics(&cmds, &graphics);
    dvz_cmd_draw(&cmds, 0, 3, 0, 1);
    dvz_cmd_rendering_end(&cmds);
    dvz_cmd_end(&cmds);

    // Submit the command buffer.
    dvz_cmd_submit(&cmds);

    // Staging buffer for screenshot.
    DvzBuffer staging = {0};
    DvzSize screenshot_size = WIDTH * HEIGHT * 4;
    dvz_buffer(dvz_bootstrap_device(&bootstrap), dvz_bootstrap_allocator(&bootstrap), &staging);
    dvz_buffer_size(&staging, screenshot_size);
    dvz_buffer_flags(
        &staging, VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
    dvz_buffer_usage(&staging, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    dvz_buffer_create(&staging);

    // Screenshot.
    dvz_cmd_reset(&cmds);
    dvz_cmd_begin(&cmds);

    // Layout transition.
    dvz_barrier_image_stage(
        bimg, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT);
    dvz_barrier_image_access(
        bimg, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_2_TRANSFER_READ_BIT);
    dvz_barrier_image_layout(
        bimg, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    dvz_cmd_barriers(&cmds, &barriers);

    // Copy image to buffer.
    DvzImageRegion region = {0};
    dvz_image_region(&region);
    dvz_image_region_extent(&region, WIDTH, HEIGHT, 1);
    dvz_cmd_copy_image_to_buffer(
        &cmds, dvz_image_handle(&img, 0), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, &region,
        dvz_buffer_handle(&staging), 0);

    // End the command buffer.
    dvz_cmd_end(&cmds);

    // Submit the command buffer.
    dvz_cmd_submit(&cmds);

    // Recover the screenshot.
    uint8_t* screenshot = (uint8_t*)dvz_calloc(WIDTH * HEIGHT, 4);
    dvz_buffer_download(&staging, 0, screenshot_size, screenshot);
    dvz_write_png("build/screenshot.png", WIDTH, HEIGHT, screenshot);

    // Cleanup.
    dvz_image_views_destroy(&view);
    dvz_images_destroy(&img);
    dvz_buffer_destroy(&staging);
    dvz_shader_destroy(&vs);
    dvz_shader_destroy(&fs);
    dvz_slots_destroy(&slots);
    dvz_graphics_destroy(&graphics);
    dvz_bootstrap_destroy(&bootstrap);
    dvz_free(vs_spv);
    dvz_free(fs_spv);
    dvz_free(screenshot);

    RETURN_VALIDATION
}



int test_vklite_proto_screenshot_repeat(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzProto proto = {0};
    dvz_proto(&proto);

    DvzCommands* cmds = dvz_proto_commands(&proto);
    ANN(cmds);
    if (cmds->count == 0 || dvz_commands_handle(cmds) == VK_NULL_HANDLE)
    {
        dvz_proto_destroy(&proto);
        return 0;
    }

    const char* screenshot0 = "build/proto_screenshot_repeat_0.png";
    const char* screenshot1 = "build/proto_screenshot_repeat_1.png";
    if (cmds->count == 0 || dvz_commands_handle(cmds) == VK_NULL_HANDLE)
    {
        dvz_proto_destroy(&proto);
        return 0;
    }
    dvz_barriers(&proto.barriers);
    dvz_proto_screenshot(&proto, screenshot0);
    if (cmds->count == 0 || dvz_commands_handle(cmds) == VK_NULL_HANDLE)
    {
        dvz_proto_destroy(&proto);
        return 0;
    }
    dvz_barriers(&proto.barriers);
    dvz_proto_screenshot(&proto, screenshot1);

    DvzSize size0 = 0;
    DvzSize size1 = 0;
    void* data0 = dvz_read_file(screenshot0, &size0);
    void* data1 = dvz_read_file(screenshot1, &size1);
    AT(data0 != NULL);
    AT(data1 != NULL);
    AT(size0 > 0);
    AT(size1 > 0);

    dvz_free(data0);
    dvz_free(data1);

    uint32_t err_count = dvz_bootstrap_error_count(&proto.bootstrap);
    dvz_proto_destroy(&proto);

    return err_count > 0 ? 1 : 0;
}
