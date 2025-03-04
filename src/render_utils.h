/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Render utils                                                                                 */
/*************************************************************************************************/

#ifndef DVZ_HEADER_RENDER_UTILS
#define DVZ_HEADER_RENDER_UTILS



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz_defaults.h"
#include "resources_utils.h"
#include "surface.h"
#include "vklite.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#ifdef LANG_CPP
cvec4 BACKGROUND = {102, 153, 204, 255};
#else
#define BACKGROUND (cvec4){102, 153, 204, 255}
#endif



/*************************************************************************************************/
/*  Render utils                                                                                 */
/*************************************************************************************************/

static void
make_images(DvzGpu* gpu, DvzImages* images, DvzFormat format, uint32_t width, uint32_t height)
{
    ANN(gpu);
    ANN(images);
    ASSERT(width > 0);
    ASSERT(height > 0);

    log_trace("making images");
    *images = dvz_images(gpu, VK_IMAGE_TYPE_2D, 1);

    dvz_images_format(images, (VkFormat)format);
    uvec3 size = {width, height, 1};
    dvz_images_size(images, size);
    dvz_images_tiling(images, VK_IMAGE_TILING_OPTIMAL);
    dvz_images_usage(
        images, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    dvz_images_memory(images, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    dvz_images_aspect(images, VK_IMAGE_ASPECT_COLOR_BIT);
    dvz_images_layout(images, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    dvz_images_queue_access(images, DVZ_DEFAULT_QUEUE_RENDER);
    dvz_images_queue_access(images, DVZ_DEFAULT_QUEUE_TRANSFER);
    dvz_images_create(images);
}



static void
make_depth(DvzGpu* gpu, DvzImages* depth, uint32_t img_count, uint32_t width, uint32_t height)
{
    ANN(gpu);
    ANN(depth);
    ASSERT(width > 0);
    ASSERT(height > 0);
    ASSERT(img_count >= 1);

    log_trace("making depth image");
    *depth = dvz_images(gpu, VK_IMAGE_TYPE_2D, img_count);
    uvec3 size = {width, height, 1};

    dvz_images_format(depth, VK_FORMAT_D32_SFLOAT);
    dvz_images_size(depth, size);
    dvz_images_tiling(depth, VK_IMAGE_TILING_OPTIMAL);
    dvz_images_usage(depth, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    // | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT);
    dvz_images_memory(depth, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    // | VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT);
    dvz_images_layout(depth, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    dvz_images_aspect(depth, VK_IMAGE_ASPECT_DEPTH_BIT);
    dvz_images_queue_access(depth, 0);

    // // HACK: lazily allocated image
    // for (uint32_t i = 0; i < depth->count; i++)
    //     depth->vma[i].usage = VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED;

    dvz_images_create(depth);
    log_trace("done making depth image");
}



static void
make_staging(DvzGpu* gpu, DvzImages* staging, DvzFormat format, uint32_t width, uint32_t height)
{
    ANN(gpu);
    ANN(staging);
    ASSERT(format != 0);
    ASSERT(width > 0);
    ASSERT(height > 0);

    log_trace("making staging images");
    *staging = dvz_images(gpu, VK_IMAGE_TYPE_2D, 1);
    uvec3 size = {width, height, 1};

    dvz_images_format(staging, (VkFormat)format);
    dvz_images_size(staging, size);
    dvz_images_tiling(staging, VK_IMAGE_TILING_LINEAR);
    dvz_images_usage(staging, VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    dvz_images_layout(staging, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    dvz_images_queue_access(staging, DVZ_DEFAULT_QUEUE_TRANSFER);
    // dvz_images_memory(
    //     staging, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    dvz_images_vma_usage(staging, VMA_MEMORY_USAGE_CPU_ONLY);
    dvz_images_create(staging);
}



static void make_framebuffers(
    DvzGpu* gpu, DvzFramebuffers* framebuffers, DvzRenderpass* renderpass, //
    DvzImages* images, DvzImages* depth)
{
    ANN(gpu);
    ANN(framebuffers);
    ANN(renderpass);
    ANN(images);
    ANN(depth);
    log_trace("making framebuffers");

    *framebuffers = dvz_framebuffers(gpu);
    dvz_framebuffers_attachment(framebuffers, 0, images);
    dvz_framebuffers_attachment(framebuffers, 1, depth);
    dvz_framebuffers_create(framebuffers, renderpass);
}



static DvzRenderpass offscreen_renderpass(DvzGpu* gpu)
{
    return dvz_gpu_renderpass(gpu, BACKGROUND, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
}



static DvzRenderpass desktop_renderpass(DvzGpu* gpu)
{
    return dvz_gpu_renderpass(gpu, BACKGROUND, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
}



static void make_swapchain(
    DvzGpu* gpu, DvzSurface surface, DvzSwapchain* swapchain, uint32_t min_img_count, bool vsync)
{
    ANN(swapchain);
    log_trace("making swapchain");

    *swapchain = dvz_swapchain(gpu, surface.surface, min_img_count);
    dvz_swapchain_format(swapchain, (VkFormat)DVZ_DEFAULT_FORMAT);
    dvz_swapchain_present_mode(
        swapchain, vsync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR);
    dvz_swapchain_create(swapchain);

    ANN(swapchain->images);

    // Create a staging buffer, to be used by screencast and screenshot.
    // It is automatically recreated upon resize.
    // _screencast_staging(canvas);
}



static DvzBarrier make_barrier(DvzImages* images)
{
    ANN(images);
    DvzBarrier barrier = dvz_barrier(images->gpu);
    dvz_barrier_stages(
        &barrier, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    dvz_barrier_images(&barrier, images);
    dvz_barrier_images_layout(
        &barrier, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    dvz_barrier_images_access(&barrier, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

    return barrier;
}



static DvzBarrier make_depth_barrier(DvzImages* images)
{
    ANN(images);
    DvzBarrier barrier = dvz_barrier(images->gpu);
    dvz_barrier_stages(
        &barrier, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);
    dvz_barrier_images(&barrier, images);
    dvz_barrier_images_layout(
        &barrier, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
    dvz_barrier_images_access(&barrier, 0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
    dvz_barrier_images_aspect(&barrier, VK_IMAGE_ASPECT_DEPTH_BIT);

    return barrier;
}



static void blank_commands(
    DvzRenderpass* renderpass, DvzFramebuffers* framebuffers, DvzImages* images, DvzImages* depth,
    DvzCommands* cmds, uint32_t cmd_idx, void* user_data)
{
    ANN(renderpass);
    ANN(framebuffers);
    ANN(images);
    ANN(depth);
    ANN(cmds);

    DvzBarrier barrier = make_barrier(images);
    DvzBarrier barrier_depth = make_depth_barrier(depth);

    log_trace("starting blank commands");
    dvz_cmd_begin(cmds, cmd_idx);

    dvz_cmd_barrier(cmds, cmd_idx, &barrier);
    dvz_cmd_barrier(cmds, cmd_idx, &barrier_depth);

    dvz_cmd_begin_renderpass(cmds, cmd_idx, renderpass, framebuffers);
    dvz_cmd_end_renderpass(cmds, cmd_idx);

    dvz_cmd_end(cmds, cmd_idx);
}



static void* screenshot(DvzImages* images, VkDeviceSize bytes_per_component)
{
    // NOTE: the caller must free the output

    DvzGpu* gpu = images->gpu;

    // Create the staging image.
    log_debug("starting creation of staging image");
    DvzImages staging_struct = dvz_images(gpu, VK_IMAGE_TYPE_2D, 1);
    DvzImages* staging = (DvzImages*)calloc(1, sizeof(DvzImages));
    *staging = staging_struct;
    dvz_images_format(staging, images->format);
    dvz_images_size(staging, images->shape);
    dvz_images_tiling(staging, VK_IMAGE_TILING_LINEAR);
    dvz_images_usage(staging, VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    dvz_images_layout(staging, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    // dvz_images_memory(
    //     staging, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    dvz_images_vma_usage(staging, VMA_MEMORY_USAGE_CPU_ONLY);
    dvz_images_create(staging);

    // Start the image transition command buffers.
    log_trace("starting screenshot");
    DvzCommands cmds = dvz_commands(gpu, 0, 1);
    dvz_cmd_begin(&cmds, 0);

    DvzBarrier barrier = dvz_barrier(gpu);
    dvz_barrier_stages(&barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    dvz_barrier_images(&barrier, staging);
    dvz_barrier_images_layout(
        &barrier, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    dvz_barrier_images_access(&barrier, 0, VK_ACCESS_TRANSFER_WRITE_BIT);
    dvz_cmd_barrier(&cmds, 0, &barrier);

    // Copy the image to the staging image.
    dvz_cmd_copy_image(&cmds, 0, images, staging);

    dvz_barrier_images_layout(
        &barrier, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
    dvz_barrier_images_access(&barrier, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT);
    dvz_cmd_barrier(&cmds, 0, &barrier);

    // End the cmds and submit them.
    dvz_cmd_end(&cmds, 0);
    dvz_cmd_submit_sync(&cmds, 0);

    // Now, copy the staging image into CPU memory.
    void* rgb = calloc(images->shape[0] * images->shape[1], 3 * bytes_per_component);
    dvz_images_download(staging, 0, bytes_per_component, true, false, rgb);

    dvz_images_destroy(staging);
    FREE(staging);
    return rgb;
}



static void default_clear_color(int flags, cvec4 clear_color)
{
    bool white = ((flags & DVZ_RENDERER_FLAGS_WHITE_BACKGROUND) > 0);
    clear_color[0] = 255;
    clear_color[1] = 255;
    clear_color[2] = 255;
    clear_color[3] = 0;
    if (!white)
    {
        memset(clear_color, 0, 4);
    }
    else
    {
        log_debug("using a white background in all canvases");
    }
}



#endif
