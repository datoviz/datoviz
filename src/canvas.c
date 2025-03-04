/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Canvas                                                                                       */
/*************************************************************************************************/

#include "canvas.h"
#include "canvas_utils.h"
#include "common.h"
#include "datoviz_defaults.h"
#include "host.h"
#include "vklite.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

// NOTE: wrapper for DvzCanvasRefill callback type.
static void _blank_refill(DvzCanvas* canvas, DvzCommands* cmds, uint32_t idx, void* user_data)
{
    ANN(canvas)
    ANN(cmds)
    blank_commands(
        canvas->render.renderpass, &canvas->render.framebuffers, canvas->render.swapchain.images,
        &canvas->render.depth, cmds, idx, user_data);
}



DvzCanvas
dvz_canvas(DvzGpu* gpu, DvzRenderpass* renderpass, uint32_t width, uint32_t height, int flags)
{
    // WARNING: ensure that the pointer renderpass will live through the duration of the canvas
    // lifecycle. If a pointer to a structure, that structure should not be returned as a function,
    // otherwise it will be copied and the pointer will be invalid.

    ANN(gpu);

    if (width == 0 && height == 0)
    {
        log_debug("The canvas size is null, it will have to be set correctly before creation.");
    }

    DvzCanvas canvas = {0};
    canvas.obj.type = DVZ_OBJECT_TYPE_CANVAS;
    canvas.gpu = gpu;
    canvas.flags = flags;
    canvas.format = DVZ_DEFAULT_FORMAT;
    canvas.refill = _blank_refill;

    canvas.width = width;
    canvas.height = height;

    canvas.render.frame_timestamps = (DvzTime*)calloc(DVZ_MAX_TIMESTAMPS, sizeof(DvzTime));
    ANN(canvas.render.frame_timestamps);

    canvas.render.renderpass = renderpass;
    // ASSERT(dvz_obj_is_created(&renderpass->obj));

    dvz_obj_init(&canvas.obj);
    return canvas;
}



void dvz_canvas_create(DvzCanvas* canvas, DvzSurface surface)
{
    ANN(canvas);

    DvzGpu* gpu = canvas->gpu;
    ANN(gpu);

    DvzHost* host = gpu->host;
    ANN(host);

    log_trace("creating the canvas");

    uint32_t width = canvas->width;
    uint32_t height = canvas->height;

    ASSERT(surface.surface != VK_NULL_HANDLE);
    canvas->surface = surface;

    bool vsync = (canvas->flags & DVZ_CANVAS_FLAGS_VSYNC) != 0;

    // Make the swapchain.
    make_swapchain(
        gpu, canvas->surface, &canvas->render.swapchain, DVZ_MIN_SWAPCHAIN_IMAGE_COUNT, vsync);

    // Number of swapchain images.
    uint32_t img_count = canvas->render.swapchain.img_count;

    // Make depth buffer image.
    make_depth(gpu, &canvas->render.depth, img_count, width, height);

    // Make staging image.
    make_staging(gpu, &canvas->render.staging, canvas->format, width, height);
    dvz_images_transition(&canvas->render.staging);

    // Size of the capture buffer.
    canvas->size = width * height * 3;
    canvas->rgb = (uint8_t*)calloc(canvas->size, 1);

    // Make framebuffers.
    make_framebuffers(
        gpu, &canvas->render.framebuffers, canvas->render.renderpass, //
        canvas->render.swapchain.images, &canvas->render.depth);

    // Make synchronization objects.
    make_sync(gpu, &canvas->sync, img_count);

    // Command buffer.
    canvas->cmds = dvz_commands(canvas->gpu, DVZ_DEFAULT_QUEUE_RENDER, img_count);
    // for (uint32_t i = 0; i < canvas->cmds.count; i++)
    //     canvas->refill(canvas, &canvas->cmds, i);

    // Default submit object.
    canvas->render.submit = dvz_submit(canvas->gpu);

    dvz_obj_created(&canvas->obj);
    log_trace("canvas created with size %dx%d)", width, height);
}



void dvz_canvas_reset(DvzCanvas* canvas)
{
    ANN(canvas);
    dvz_gpu_wait(canvas->gpu);

    canvas->cur_frame = 0;
    canvas->frame_idx = 0;
}



void dvz_canvas_recreate(DvzCanvas* canvas)
{
    ANN(canvas);

    DvzGpu* gpu = canvas->gpu;
    DvzSwapchain* swapchain = &canvas->render.swapchain;
    DvzFramebuffers* framebuffers = &canvas->render.framebuffers;
    DvzRenderpass* renderpass = canvas->render.renderpass;

    ANN(gpu);
    ANN(swapchain);
    ANN(framebuffers);
    ANN(renderpass);

    log_debug("recreate the canvas");

    // Wait until the device is ready and the window fully resized.
    dvz_gpu_wait(gpu);

    // Destroy swapchain resources.
    dvz_framebuffers_destroy(framebuffers);
    dvz_images_destroy(&canvas->render.depth);
    dvz_images_destroy(canvas->render.swapchain.images);

    // Recreate the swapchain. This will automatically set the swapchain->images new
    // size.
    dvz_swapchain_recreate(swapchain);

    // Find the new framebuffer size as determined by the swapchain recreation.
    uint32_t width = swapchain->images->shape[0];
    uint32_t height = swapchain->images->shape[1];

    // Ensure the canvas size is updated as well.
    canvas->width = width;
    canvas->height = height;

    // Need to recreate the depth image with the new size.
    uvec3 shape = {width, height, 1};
    dvz_images_size(&canvas->render.depth, shape);
    dvz_images_create(&canvas->render.depth);

    // Recreate the framebuffers with the new size.
    for (uint32_t i = 0; i < framebuffers->attachment_count; i++)
    {
        ASSERT(framebuffers->attachments[i]->shape[0] == width);
        ASSERT(framebuffers->attachments[i]->shape[1] == height);
    }
    dvz_framebuffers_create(framebuffers, renderpass);

    // Resize the staging buffer.
    dvz_images_resize(&canvas->render.staging, shape);
    dvz_images_transition(&canvas->render.staging);

    // Resize the image buffer.
    canvas->size = width * height * 3;
    REALLOC(uint8_t*, canvas->rgb, canvas->size);
}



void dvz_canvas_refill(DvzCanvas* canvas, DvzCanvasRefill refill, void* user_data)
{
    ANN(canvas);
    ANN(refill);
    canvas->refill = refill;
    canvas->refill_data = user_data;
}



void dvz_canvas_begin(DvzCanvas* canvas, DvzCommands* cmds, uint32_t idx)
{
    ANN(canvas);
    DvzGpu* gpu = canvas->gpu;
    ANN(gpu);
    dvz_cmd_begin(cmds, idx);
    dvz_cmd_begin_renderpass(cmds, idx, canvas->render.renderpass, &canvas->render.framebuffers);
}



void dvz_canvas_viewport(
    DvzCanvas* canvas, DvzCommands* cmds, uint32_t idx, vec2 offset, vec2 size)
{
    ANN(canvas);

    // A value of 0 = full canvas.
    float width = size[0], height = size[1];
    width = width > 0 ? width : (float)canvas->width;
    height = height > 0 ? height : (float)canvas->height;

    ASSERT(width > 0);
    ASSERT(height > 0);

    dvz_cmd_viewport(
        cmds, idx,
        (VkViewport){.x = offset[0],
                     .y = offset[1],
                     .width = width,
                     .height = height,
                     // WARNING: do not forget this otherwise depth testing may not work!
                     .minDepth = 0,
                     .maxDepth = 1});
}



void dvz_canvas_end(DvzCanvas* canvas, DvzCommands* cmds, uint32_t idx)
{
    ANN(canvas);
    ANN(cmds);
    dvz_cmd_end_renderpass(cmds, idx);
    dvz_cmd_end(cmds, idx);
}



uint8_t* dvz_canvas_download(DvzCanvas* canvas)
{
    // NOTE: the caller should NOT free the returned pointer, the canvas will do it upon
    // destruction.
    ANN(canvas);

    DvzGpu* gpu = canvas->gpu;
    ANN(gpu);

    DvzImages* images = canvas->render.swapchain.images;
    ANN(images);

    ASSERT(dvz_obj_is_created(&canvas->render.staging.obj));
    ANN(canvas->rgb);
    ASSERT(canvas->size > 0);

    // Start the image transition command buffers.
    log_trace("starting canvas download");
    DvzCommands cmds = dvz_commands(gpu, DVZ_DEFAULT_QUEUE_TRANSFER, 1);
    dvz_cmd_begin(&cmds, 0);

    DvzBarrier barrier = dvz_barrier(gpu);
    dvz_barrier_stages(&barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    dvz_barrier_images(&barrier, canvas->render.swapchain.images);
    dvz_barrier_images_layout(
        &barrier, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    dvz_barrier_images_access(&barrier, VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT);
    dvz_cmd_barrier(&cmds, 0, &barrier);

    // Copy the image to the staging image.
    dvz_cmd_copy_image(&cmds, 0, canvas->render.swapchain.images, &canvas->render.staging);

    dvz_barrier_images_layout(
        &barrier, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    dvz_barrier_images_access(&barrier, VK_ACCESS_TRANSFER_READ_BIT, 0);
    dvz_cmd_barrier(&cmds, 0, &barrier);

    // End the cmds and submit them.
    dvz_cmd_end(&cmds, 0);
    dvz_cmd_submit_sync(&cmds, 0);

    // Now, copy the staging image into CPU memory.
    dvz_images_download(&canvas->render.staging, 0, 1, true, false, canvas->rgb);

    return canvas->rgb;
}



void dvz_canvas_timestamps(
    DvzCanvas* canvas, uint32_t count, uint64_t* seconds, uint64_t* nanoseconds)
{
    ANN(canvas);
    if (canvas->obj.status == DVZ_OBJECT_STATUS_DESTROYED)
    {
        log_warn("impossible to recover the timestamps of a destroyed canvas");
        return;
    }
    if (count > DVZ_MAX_TIMESTAMPS)
    {
        log_warn("requesting %u timestamps but only %u are recorded", count, DVZ_MAX_TIMESTAMPS);
        count = DVZ_MAX_TIMESTAMPS;
    }
    ASSERT(count <= DVZ_MAX_TIMESTAMPS);

    ANN(seconds);
    ANN(nanoseconds);

    int32_t idx = (int32_t)canvas->render.frame_time_idx - (int32_t)count;
    if (idx < 0)
        idx += DVZ_MAX_TIMESTAMPS;
    idx = idx % DVZ_MAX_TIMESTAMPS;
    ASSERT((0 <= idx) && (idx < DVZ_MAX_TIMESTAMPS));

    DvzTime* times = canvas->render.frame_timestamps;
    ANN(times);

    for (uint32_t i = 0; i < count; i++)
    {
        idx = idx % DVZ_MAX_TIMESTAMPS;
        // log_error("%d %d %d", i, idx, DVZ_MAX_TIMESTAMPS);
        ASSERT((0 <= idx) && (idx < DVZ_MAX_TIMESTAMPS));

        seconds[i] = times[idx].seconds;
        nanoseconds[i] = times[idx].nanoseconds;

        idx++;
    }
}



void dvz_canvas_destroy(DvzCanvas* canvas)
{
    if (canvas == NULL || canvas->obj.status != DVZ_OBJECT_STATUS_CREATED)
    {
        log_trace(
            "skip destruction of already-destroyed canvas with status %d", canvas->obj.status);
        return;
    }
    log_debug("destroy the canvas with status %d", canvas->obj.status);

    ANN(canvas);

    DvzGpu* gpu = canvas->gpu;
    ANN(gpu);

    DvzHost* host = gpu->host;
    ANN(host);

    // Wait until all pending events have been processed.
    // backend_poll_events(host->backend);

    // Wait on the GPU.
    dvz_gpu_wait(gpu);

    // Destroy the graphics.
    log_trace("canvas destroy graphics pipelines");

    // Destroy the images.
    dvz_images_destroy(canvas->render.swapchain.images);
    dvz_images_destroy(&canvas->render.depth);
    dvz_images_destroy(&canvas->render.staging);

    // Destroy the image buffer.
    FREE(canvas->rgb);

    // Destroy the swapchain.
    log_trace("canvas destroy swapchain");
    dvz_swapchain_destroy(&canvas->render.swapchain);

    // Destroy the framebuffers.
    log_trace("canvas destroy framebuffers");
    dvz_framebuffers_destroy(&canvas->render.framebuffers);

    // Destroy the semaphores.
    log_trace("canvas destroy semaphores");
    dvz_semaphores_destroy(&canvas->sync.sem_img_available);
    dvz_semaphores_destroy(&canvas->sync.sem_render_finished);

    // Destroy the fences.
    log_trace("canvas destroy fences");
    dvz_fences_destroy(&canvas->sync.fences_render_finished);

    FREE(canvas->render.swapchain.images);
    FREE(canvas->render.frame_timestamps);

    dvz_obj_destroyed(&canvas->obj);
}
