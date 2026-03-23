/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing fixture offscreen                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_buffers.h"
#include "_images.h"
#include "_log.h"
#include "_shader.h"
#include "datoviz/fileio/fileio.h"
#include "datoviz/vk/device.h"
#include "datoviz/vklite/graphics.h"
#include "datoviz/vklite/images.h"
#include "datoviz/vklite/rendering.h"
#include "datoviz/vklite/sync.h"
#include "fixture_offscreen.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzFixtureOffscreen
{
    DvzFixtureGpu* gpu;
    uint32_t width;
    uint32_t height;
    DvzShader vs;
    DvzShader fs;
    DvzGraphics graphics;
    DvzSlots slots;
    DvzDescriptors* desc;
    DvzRendering* rendering;
    DvzImages color;
    DvzImageViews color_view;
    DvzImages depth;
    DvzImageViews depth_view;
    DvzBarriers barriers;
    DvzCommands cmds;
    DvzBuffer staging;
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create an offscreen rendering fixture borrowing a GPU fixture.
 *
 * @param gpu the parent GPU fixture
 * @param width width of the offscreen target
 * @param height height of the offscreen target
 * @return allocated offscreen fixture, or NULL on allocation failure
 */
DvzFixtureOffscreen*
dvz_fixture_offscreen(DvzFixtureGpu* gpu, uint32_t width, uint32_t height)
{
    ANN(gpu);
    ASSERT(width > 0);
    ASSERT(height > 0);

    DvzFixtureOffscreen* fixture =
        (DvzFixtureOffscreen*)dvz_calloc(1, sizeof(DvzFixtureOffscreen));
    ANN(fixture);

    fixture->gpu = gpu;
    fixture->width = width;
    fixture->height = height;
    fixture->desc = dvz_descriptors_create();
    ANN(fixture->desc);
    fixture->rendering = dvz_rendering_create();
    ANN(fixture->rendering);

    DvzDevice* device = dvz_fixture_gpu_device(gpu);
    DvzVma* allocator = dvz_fixture_gpu_alloc(gpu);
    DvzQueue* queue = dvz_fixture_gpu_queue(gpu);
    ANN(device);
    ANN(allocator);
    ANN(queue);

    dvz_rendering(fixture->rendering);
    dvz_rendering_area(fixture->rendering, 0, 0, width, height);

    dvz_images(device, allocator, VK_IMAGE_TYPE_2D, 1, &fixture->color);
    dvz_images_format(&fixture->color, VK_FORMAT_R8G8B8A8_UNORM);
    dvz_images_size(&fixture->color, width, height, 1);
    dvz_images_usage(
        &fixture->color, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    dvz_images_create(&fixture->color);

    dvz_image_views(&fixture->color, &fixture->color_view);
    dvz_image_views_create(&fixture->color_view);

    DvzAttachment* catt = dvz_rendering_color(fixture->rendering, 0);
    dvz_attachment_image(
        catt, dvz_image_views_handle(&fixture->color_view, 0), VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL);
    dvz_attachment_ops(catt, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);
    dvz_attachment_clear(catt, (VkClearValue){.color.float32 = DVZ_FIXTURE_CLEAR_COLOR});

    dvz_images(device, allocator, VK_IMAGE_TYPE_2D, 1, &fixture->depth);
    dvz_images_format(&fixture->depth, VK_FORMAT_D32_SFLOAT_S8_UINT);
    dvz_images_size(&fixture->depth, width, height, 1);
    dvz_images_usage(&fixture->depth, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    dvz_images_create(&fixture->depth);

    dvz_image_views(&fixture->depth, &fixture->depth_view);
    dvz_image_views_aspect(&fixture->depth_view, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
    dvz_image_views_create(&fixture->depth_view);

    DvzAttachment* datt = dvz_rendering_depth(fixture->rendering);
    dvz_attachment_image(
        datt, dvz_image_views_handle(&fixture->depth_view, 0), VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL);
    dvz_attachment_ops(datt, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);
    dvz_attachment_clear(datt, (VkClearValue){.depthStencil = {1.0f, 0}});

    DvzAttachment* satt = dvz_rendering_stencil(fixture->rendering);
    dvz_attachment_image(
        satt, dvz_image_views_handle(&fixture->depth_view, 0), VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL);
    dvz_attachment_ops(satt, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);
    dvz_attachment_clear(satt, (VkClearValue){.depthStencil = {1.0f, 0}});

    dvz_barriers(&fixture->barriers);

    DvzBarrierImage* bcolor =
        dvz_barriers_image(&fixture->barriers, dvz_image_handle(&fixture->color, 0));
    ANN(bcolor);
    dvz_barrier_image_stage(
        bcolor, VK_PIPELINE_STAGE_2_NONE, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
    dvz_barrier_image_access(bcolor, 0, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
    dvz_barrier_image_layout(
        bcolor, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    DvzBarrierImage* bdepth =
        dvz_barriers_image(&fixture->barriers, dvz_image_handle(&fixture->depth, 0));
    ANN(bdepth);
    dvz_barrier_image_stage(
        bdepth, VK_PIPELINE_STAGE_2_NONE,
        VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT);
    dvz_barrier_image_access(bdepth, 0, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
    dvz_barrier_image_layout(
        bdepth, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL);
    dvz_barrier_image_aspect(bdepth, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);

    dvz_commands(device, queue, 1, &fixture->cmds);

    return fixture;
}



/**
 * Destroy an offscreen rendering fixture.
 *
 * @param fixture the offscreen fixture
 */
void dvz_fixture_offscreen_destroy(DvzFixtureOffscreen* fixture)
{
    if (fixture == NULL)
    {
        return;
    }

    dvz_commands_destroy(&fixture->cmds);
    dvz_image_views_destroy(&fixture->color_view);
    dvz_images_destroy(&fixture->color);
    dvz_image_views_destroy(&fixture->depth_view);
    dvz_images_destroy(&fixture->depth);
    dvz_buffer_destroy(&fixture->staging);
    dvz_shader_destroy(&fixture->vs);
    dvz_shader_destroy(&fixture->fs);
    dvz_slots_destroy(&fixture->slots);
    dvz_descriptors_free(fixture->desc);
    fixture->desc = NULL;
    dvz_rendering_free(fixture->rendering);
    fixture->rendering = NULL;
    dvz_graphics_destroy(&fixture->graphics);
    dvz_free(fixture);
}



/**
 * Get the parent GPU fixture.
 *
 * @param fixture the offscreen fixture
 * @return borrowed GPU fixture
 */
DvzFixtureGpu* dvz_fixture_offscreen_gpu(DvzFixtureOffscreen* fixture)
{
    ANN(fixture);
    return fixture->gpu;
}



/**
 * Get the slots helper owned by the fixture.
 *
 * @param fixture the offscreen fixture
 * @return borrowed slots wrapper
 */
DvzSlots* dvz_fixture_offscreen_slots(DvzFixtureOffscreen* fixture)
{
    ANN(fixture);
    dvz_slots(dvz_fixture_gpu_device(fixture->gpu), &fixture->slots);
    return &fixture->slots;
}



/**
 * Get the graphics helper owned by the fixture.
 *
 * @param fixture the offscreen fixture
 * @param vs_size size in bytes of the vertex shader SPIR-V buffer
 * @param vs_spv vertex shader SPIR-V buffer
 * @param fs_size size in bytes of the fragment shader SPIR-V buffer
 * @param fs_spv fragment shader SPIR-V buffer
 * @return borrowed graphics wrapper
 */
DvzGraphics* dvz_fixture_offscreen_graphics(
    DvzFixtureOffscreen* fixture, DvzSize vs_size, uint32_t* vs_spv, DvzSize fs_size,
    uint32_t* fs_spv)
{
    ANN(fixture);

    DvzDevice* device = dvz_fixture_gpu_device(fixture->gpu);
    ANN(device);

    dvz_graphics(device, &fixture->graphics);

    dvz_shader(device, vs_size, vs_spv, &fixture->vs);
    dvz_shader(device, fs_size, fs_spv, &fixture->fs);
    dvz_graphics_shader(
        &fixture->graphics, VK_SHADER_STAGE_VERTEX_BIT, dvz_shader_handle(&fixture->vs));
    dvz_graphics_shader(
        &fixture->graphics, VK_SHADER_STAGE_FRAGMENT_BIT, dvz_shader_handle(&fixture->fs));

    dvz_graphics_attachment_color(&fixture->graphics, 0, VK_FORMAT_R8G8B8A8_UNORM);
    dvz_graphics_blend_color(
        &fixture->graphics, 0, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        VK_BLEND_OP_ADD, 0xF);
    dvz_graphics_blend_alpha(
        &fixture->graphics, 0, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD);
    dvz_graphics_attachment_depth(&fixture->graphics, VK_FORMAT_D32_SFLOAT_S8_UINT);
    dvz_graphics_attachment_stencil(&fixture->graphics, VK_FORMAT_D32_SFLOAT_S8_UINT);
    dvz_graphics_primitive(
        &fixture->graphics, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, DVZ_GRAPHICS_FLAGS_FIXED);
    dvz_graphics_viewport(
        &fixture->graphics, 0, 0, fixture->width, fixture->height, 0, 1,
        DVZ_GRAPHICS_FLAGS_DYNAMIC);
    dvz_graphics_scissor(
        &fixture->graphics, 0, 0, fixture->width, fixture->height, DVZ_GRAPHICS_FLAGS_DYNAMIC);

    return &fixture->graphics;
}



/**
 * Get the descriptor wrapper owned by the fixture.
 *
 * @param fixture the offscreen fixture
 * @return borrowed descriptor wrapper
 */
DvzDescriptors* dvz_fixture_offscreen_desc(DvzFixtureOffscreen* fixture)
{
    ANN(fixture);
    return fixture->desc;
}



/**
 * Get the command wrapper owned by the fixture.
 *
 * @param fixture the offscreen fixture
 * @return borrowed command wrapper
 */
DvzCommands* dvz_fixture_offscreen_cmds(DvzFixtureOffscreen* fixture)
{
    ANN(fixture);
    return &fixture->cmds;
}



/**
 * Get the rendering wrapper owned by the fixture.
 *
 * @param fixture the offscreen fixture
 * @return borrowed rendering wrapper
 */
DvzRendering* dvz_fixture_offscreen_rendering(DvzFixtureOffscreen* fixture)
{
    ANN(fixture);
    return fixture->rendering;
}



/**
 * Get the barrier wrapper owned by the fixture.
 *
 * @param fixture the offscreen fixture
 * @return borrowed barrier wrapper
 */
DvzBarriers* dvz_fixture_offscreen_barriers(DvzFixtureOffscreen* fixture)
{
    ANN(fixture);
    return &fixture->barriers;
}



/**
 * Get the main color image owned by the fixture.
 *
 * @param fixture the offscreen fixture
 * @return borrowed color image wrapper
 */
DvzImages* dvz_fixture_offscreen_color(DvzFixtureOffscreen* fixture)
{
    ANN(fixture);
    return &fixture->color;
}



/**
 * Get the main color image view owned by the fixture.
 *
 * @param fixture the offscreen fixture
 * @return borrowed color image-view wrapper
 */
DvzImageViews* dvz_fixture_offscreen_color_view(DvzFixtureOffscreen* fixture)
{
    ANN(fixture);
    return &fixture->color_view;
}



/**
 * Get the main depth-stencil image owned by the fixture.
 *
 * @param fixture the offscreen fixture
 * @return borrowed depth image wrapper
 */
DvzImages* dvz_fixture_offscreen_depth(DvzFixtureOffscreen* fixture)
{
    ANN(fixture);
    return &fixture->depth;
}



/**
 * Get the main depth-stencil image view owned by the fixture.
 *
 * @param fixture the offscreen fixture
 * @return borrowed depth image-view wrapper
 */
DvzImageViews* dvz_fixture_offscreen_depth_view(DvzFixtureOffscreen* fixture)
{
    ANN(fixture);
    return &fixture->depth_view;
}



/**
 * Transition an image with a one-shot command recorded by the fixture.
 *
 * @param fixture the offscreen fixture
 * @param img the image to transition
 * @param access destination access flags
 * @param layout destination layout
 */
void dvz_fixture_offscreen_transition(
    DvzFixtureOffscreen* fixture, DvzImages* img, VkAccessFlags2 access, VkImageLayout layout)
{
    ANN(fixture);
    ANN(img);

    DvzCommands* cmds = &fixture->cmds;
    DvzBarriers* barriers = &fixture->barriers;

    dvz_cmd_reset(cmds);
    dvz_cmd_begin(cmds);

    DvzBarrierImage* bimg = dvz_barriers_image(barriers, dvz_image_handle(img, 0));
    ANN(bimg);
    dvz_barrier_image_stage(bimg, VK_PIPELINE_STAGE_2_NONE, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT);
    dvz_barrier_image_access(bimg, 0, access);
    dvz_barrier_image_layout(bimg, 0, layout);
    dvz_cmd_barriers(cmds, barriers);
    dvz_cmd_end(cmds);
    dvz_cmd_submit(cmds);
}



/**
 * Save the fixture color target as a PNG screenshot.
 *
 * @param fixture the offscreen fixture
 * @param filename output PNG path
 */
void dvz_fixture_offscreen_png(DvzFixtureOffscreen* fixture, const char* filename)
{
    ANN(fixture);

    log_debug("starting screenshot into %s", filename);

    DvzBuffer* staging = &fixture->staging;
    if (staging->vk_buffer != VK_NULL_HANDLE)
    {
        dvz_buffer_destroy(staging);
    }

    DvzSize screenshot_size = fixture->width * fixture->height * 4;
    ASSERT(screenshot_size > 0);

    DvzDevice* device = dvz_fixture_gpu_device(fixture->gpu);
    DvzVma* allocator = dvz_fixture_gpu_alloc(fixture->gpu);
    ANN(device);
    ANN(allocator);

    dvz_buffer(device, allocator, staging);
    dvz_buffer_size(staging, screenshot_size);
    dvz_buffer_flags(
        staging, VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
    dvz_buffer_usage(staging, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    dvz_buffer_create(staging);

    DvzCommands* cmds = &fixture->cmds;
    DvzBarriers* barriers = &fixture->barriers;
    dvz_cmd_reset(cmds);
    dvz_cmd_begin(cmds);

    DvzBarrierImage* bimg = dvz_barriers_image(barriers, dvz_image_handle(&fixture->color, 0));
    ANN(bimg);
    dvz_barrier_image_stage(
        bimg, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT);
    dvz_barrier_image_access(
        bimg, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_2_TRANSFER_READ_BIT);
    dvz_barrier_image_layout(
        bimg, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    dvz_cmd_barriers(cmds, barriers);

    DvzImageRegion region = {0};
    dvz_image_region(&region);
    dvz_image_region_extent(&region, fixture->width, fixture->height, 1);
    dvz_cmd_copy_image_to_buffer(
        cmds, dvz_image_handle(&fixture->color, 0), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, &region,
        dvz_buffer_handle(staging), 0);

    dvz_cmd_end(cmds);
    dvz_cmd_submit(cmds);

    uint8_t* screenshot = (uint8_t*)dvz_calloc(fixture->width * fixture->height, 4);
    dvz_buffer_download(staging, 0, screenshot_size, screenshot);
    dvz_write_png(filename, fixture->width, fixture->height, screenshot);
    dvz_free(screenshot);
    dvz_buffer_destroy(staging);
}
