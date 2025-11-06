/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Proto                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "datoviz/common/macros.h"
#include "datoviz/common/obj.h"
#include "datoviz/fileio/fileio.h"
#include "datoviz/vk/device.h"
#include "datoviz/vklite/graphics.h"
#include "datoviz/vklite/images.h"
#include "datoviz/vklite/proto.h"
#include "datoviz/vklite/rendering.h"
#include "datoviz/vklite/sampler.h"
#include "vulkan/vulkan_core.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

void dvz_proto(DvzProto* proto)
{
    ANN(proto);

    // Bootstrap.
    DvzBootstrap* bootstrap = &proto->bootstrap;
    ANN(bootstrap);

    dvz_bootstrap(bootstrap, DVZ_BOOTSTRAP_MANUAL_CREATE_DEVICE);

    DvzDevice* device = &bootstrap->device;
    ANN(device);

    DvzQueue* queue = dvz_device_queue(device, DVZ_QUEUE_MAIN);
    ANN(queue);

    // Create a device with support for dynamic rendering.
    VkPhysicalDeviceFeatures* fet10 = dvz_device_request_features10(device);
    fet10->samplerAnisotropy = true;
    VkPhysicalDeviceVulkan13Features* fet13 = dvz_device_request_features13(device);
    fet13->dynamicRendering = true;
    fet13->synchronization2 = true;
    dvz_device_create(device);
    dvz_device_allocator(device, 0, &bootstrap->allocator);



    // Rendering.
    dvz_rendering(&proto->rendering);
    dvz_rendering_area(&proto->rendering, 0, 0, DVZ_PROTO_WIDTH, DVZ_PROTO_HEIGHT);

    // Image to render to.
    DvzImages* img = &proto->img;
    ANN(img);
    dvz_images(device, &proto->bootstrap.allocator, VK_IMAGE_TYPE_2D, 1, img);
    dvz_images_format(img, VK_FORMAT_R8G8B8A8_UNORM);
    dvz_images_size(img, DVZ_PROTO_WIDTH, DVZ_PROTO_HEIGHT, 1);
    // NOTE: need transfer src for screenshot
    dvz_images_usage(img, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    dvz_images_create(img);

    // Image views.
    DvzImageViews* view = &proto->view;
    ANN(view);
    dvz_image_views(img, view);
    dvz_image_views_create(view);

    // Attachments.
    DvzRendering* rendering = &proto->rendering;
    ANN(rendering);
    DvzAttachment* catt = dvz_rendering_color(rendering, 0);
    dvz_attachment_image(
        catt, dvz_image_views_handle(view, 0), VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL);
    dvz_attachment_ops(catt, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);
    dvz_attachment_clear(catt, (VkClearValue){.color.float32 = DVZ_PROTO_CLEAR_COLOR});



    // Image to render to.
    DvzImages* dimg = &proto->dimg;
    ANN(dimg);
    dvz_images(device, &proto->bootstrap.allocator, VK_IMAGE_TYPE_2D, 1, dimg);
    dvz_images_format(dimg, VK_FORMAT_D32_SFLOAT_S8_UINT);
    dvz_images_size(dimg, DVZ_PROTO_WIDTH, DVZ_PROTO_HEIGHT, 1);
    dvz_images_usage(dimg, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    dvz_images_create(dimg);

    // Image views.
    DvzImageViews* dview = &proto->dview;
    ANN(dview);
    dvz_image_views(dimg, dview);
    dvz_image_views_aspect(dview, VK_IMAGE_ASPECT_DEPTH_BIT);
    dvz_image_views_create(dview);

    DvzAttachment* datt = dvz_rendering_depth(rendering);
    dvz_attachment_image(
        datt, dvz_image_views_handle(dview, 0), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    dvz_attachment_ops(datt, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);
    dvz_attachment_clear(datt, (VkClearValue){.depthStencil = {1.0f, 0}});

    // Image barrier.
    DvzBarriers* barriers = &proto->barriers;
    ANN(barriers);
    dvz_barriers(barriers);

    // Image transition.
    DvzBarrierImage* bimg = dvz_barriers_image(barriers, dvz_image_handle(img, 0));
    ANN(bimg);
    dvz_barrier_image_stage(
        bimg, VK_PIPELINE_STAGE_2_NONE, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
    dvz_barrier_image_access(bimg, 0, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
    dvz_barrier_image_layout(
        bimg, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    // Command buffers.
    dvz_commands(device, queue, 1, &proto->cmds);
}



DvzSlots* dvz_proto_slots(DvzProto* proto)
{
    ANN(proto);
    return &proto->slots;
}



DvzGraphics* dvz_proto_graphics(
    DvzProto* proto, DvzSize vs_size, uint32_t* vs_spv, DvzSize fs_size, uint32_t* fs_spv)
{
    ANN(proto);

    DvzGraphics* graphics = &proto->graphics;
    ANN(graphics);

    DvzDevice* device = &proto->bootstrap.device;
    ANN(device);

    DvzQueue* queue = dvz_device_queue(device, DVZ_QUEUE_MAIN);
    ANN(queue);

    // Initialize the graphics pipeline.
    dvz_graphics(device, graphics);

    // Shaders.
    dvz_shader(device, vs_size, vs_spv, &proto->vs);
    dvz_shader(device, fs_size, fs_spv, &proto->fs);
    dvz_graphics_shader(graphics, VK_SHADER_STAGE_VERTEX_BIT, dvz_shader_handle(&proto->vs));
    dvz_graphics_shader(graphics, VK_SHADER_STAGE_FRAGMENT_BIT, dvz_shader_handle(&proto->fs));

    // Slots.
    dvz_slots(device, &proto->slots);

    // Color attachment.
    dvz_graphics_attachment_color(graphics, 0, VK_FORMAT_R8G8B8A8_UNORM);
    dvz_graphics_blend_color(
        graphics, 0, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        VK_BLEND_OP_ADD, 0xF);
    dvz_graphics_blend_alpha(
        graphics, 0, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD);

    // Depth-stencil attachment.
    dvz_graphics_attachment_depth(graphics, VK_FORMAT_D32_SFLOAT_S8_UINT);
    dvz_graphics_attachment_stencil(graphics, VK_FORMAT_D32_SFLOAT_S8_UINT);

    // Fixed state.
    dvz_graphics_primitive(
        graphics, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, DVZ_GRAPHICS_FLAGS_FIXED);

    // Dynamic state.
    dvz_graphics_viewport(
        graphics, 0, 0, DVZ_PROTO_WIDTH, DVZ_PROTO_HEIGHT, 0, 1, DVZ_GRAPHICS_FLAGS_DYNAMIC);
    dvz_graphics_scissor(
        graphics, 0, 0, DVZ_PROTO_WIDTH, DVZ_PROTO_HEIGHT, DVZ_GRAPHICS_FLAGS_DYNAMIC);

    // NOTE: we do NOT create the graphics pipeline, we leave it to the caller.

    return graphics;
}



DvzCommands* dvz_proto_commands(DvzProto* proto)
{
    ANN(proto);
    return &proto->cmds;
}



void dvz_proto_transition(
    DvzProto* proto, DvzImages* img, VkAccessFlags2 access, VkImageLayout layout)
{
    ANN(proto);
    ANN(img);

    DvzCommands* cmds = &proto->cmds;
    ANN(cmds);

    DvzBarriers* barriers = &proto->barriers;
    ANN(barriers);

    // Screenshot.
    dvz_cmd_reset(cmds);
    dvz_cmd_begin(cmds);

    // Image transition.
    DvzBarrierImage* bimg = dvz_barriers_image(barriers, dvz_image_handle(img, 0));
    ANN(bimg);
    dvz_barrier_image_stage(bimg, VK_PIPELINE_STAGE_2_NONE, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT);
    dvz_barrier_image_access(bimg, 0, access);
    dvz_barrier_image_layout(bimg, 0, layout);

    dvz_cmd_barriers(cmds, 0, barriers);

    // End the command buffer.
    dvz_cmd_end(cmds);

    // Submit the command buffer.
    dvz_cmd_submit(cmds);
}



void dvz_proto_screenshot(DvzProto* proto, const char* filename)
{
    ANN(proto);

    // Staging buffer for screenshot.
    DvzBuffer* staging = &proto->staging;
    ANN(staging);

    DvzSize screenshot_size = DVZ_PROTO_WIDTH * DVZ_PROTO_HEIGHT * 4;
    ASSERT(screenshot_size > 0);

    dvz_buffer(&proto->bootstrap.device, &proto->bootstrap.allocator, staging);
    dvz_buffer_size(staging, screenshot_size);
    dvz_buffer_flags(
        staging, VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
    dvz_buffer_usage(staging, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    dvz_buffer_create(staging);

    // Screenshot.
    DvzCommands* cmds = &proto->cmds;
    ANN(cmds);
    dvz_cmd_reset(cmds);
    dvz_cmd_begin(cmds);

    // Image barrier.
    DvzBarriers* barriers = &proto->barriers;
    ANN(barriers);
    DvzBarrierImage* bimg = dvz_barriers_image(barriers, dvz_image_handle(&proto->img, 0));
    ANN(bimg);

    // Layout transition.
    dvz_barrier_image_stage(
        bimg, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT);
    dvz_barrier_image_access(
        bimg, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_2_TRANSFER_READ_BIT);
    dvz_barrier_image_layout(
        bimg, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    dvz_cmd_barriers(cmds, 0, barriers);

    // Copy image to buffer.
    DvzImageRegion region = {0};
    dvz_image_region(&region);
    dvz_image_region_extent(&region, DVZ_PROTO_WIDTH, DVZ_PROTO_HEIGHT, 1);
    dvz_cmd_copy_image_to_buffer(
        cmds, 0, dvz_image_handle(&proto->img, 0), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, &region,
        dvz_buffer_handle(staging), 0);

    // End the command buffer.
    dvz_cmd_end(cmds);

    // Submit the command buffer.
    dvz_cmd_submit(cmds);

    // Recover the screenshot.
    uint8_t* screenshot = (uint8_t*)dvz_calloc(DVZ_PROTO_WIDTH * DVZ_PROTO_HEIGHT, 4);
    dvz_buffer_download(staging, 0, screenshot_size, screenshot);
    dvz_write_png(filename, DVZ_PROTO_WIDTH, DVZ_PROTO_HEIGHT, screenshot);
    dvz_free(screenshot);
}



void dvz_proto_destroy(DvzProto* proto)
{
    ANN(proto);
    dvz_image_views_destroy(&proto->view);
    dvz_images_destroy(&proto->img);
    dvz_image_views_destroy(&proto->dview);
    dvz_images_destroy(&proto->dimg);
    dvz_buffer_destroy(&proto->staging);
    dvz_shader_destroy(&proto->vs);
    dvz_shader_destroy(&proto->fs);
    dvz_slots_destroy(&proto->slots);
    dvz_graphics_destroy(&proto->graphics);
    dvz_bootstrap_destroy(&proto->bootstrap);
}
