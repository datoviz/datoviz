/*************************************************************************************************/
/*  Canvas                                                                                       */
/*************************************************************************************************/

#include "canvas.h"
#include "canvas_utils.h"
#include "common.h"
#include "host.h"
#include "vklite.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzCanvas
dvz_canvas(DvzGpu* gpu, DvzRenderpass* renderpass, uint32_t width, uint32_t height, int flags)
{
    ASSERT(gpu != NULL);
    ASSERT(width > 0);
    ASSERT(height > 0);

    DvzCanvas canvas = {0};
    canvas.obj.type = DVZ_OBJECT_TYPE_CANVAS;
    canvas.gpu = gpu;
    canvas.flags = flags;
    canvas.format = DVZ_DEFAULT_FORMAT;
    canvas.refill = blank_commands;

    canvas.width = width;
    canvas.height = height;

    canvas.render.renderpass = renderpass;
    ASSERT(dvz_obj_is_created(&renderpass->obj));

    dvz_obj_init(&canvas.obj);
    return canvas;
}



void dvz_canvas_create(DvzCanvas* canvas, VkSurfaceKHR surface)
{
    ASSERT(canvas != NULL);

    DvzGpu* gpu = canvas->gpu;
    ASSERT(gpu != NULL);

    DvzHost* host = gpu->host;
    ASSERT(host != NULL);

    log_trace("creating the canvas");

    uint32_t width = canvas->width;
    uint32_t height = canvas->height;

    ASSERT(surface != VK_NULL_HANDLE);
    canvas->surface = surface;

    // Make the swapchain.
    make_swapchain(gpu, canvas->surface, &canvas->render.swapchain, DVZ_MIN_SWAPCHAIN_IMAGE_COUNT);

    // Make depth buffer image.
    make_depth(gpu, &canvas->render.depth, width, height);

    // Make staging image.
    make_staging(gpu, &canvas->render.staging, canvas->format, width, height);

    // Make framebuffers.
    make_framebuffers(
        gpu, &canvas->render.framebuffers, canvas->render.renderpass, //
        canvas->render.swapchain.images, &canvas->render.depth);

    // Make synchronization objects.
    make_sync(gpu, &canvas->sync, canvas->render.swapchain.img_count);

    // Command buffer.
    canvas->cmds =
        dvz_commands(canvas->gpu, DVZ_DEFAULT_QUEUE_RENDER, canvas->render.swapchain.img_count);
    // for (uint32_t i = 0; i < canvas->cmds.count; i++)
    //     canvas->refill(canvas, &canvas->cmds, i);

    // Default submit object.
    canvas->render.submit = dvz_submit(canvas->gpu);

    dvz_obj_created(&canvas->obj);
    log_trace("canvas created with size %dx%d)", width, height);
}



void dvz_canvas_reset(DvzCanvas* canvas)
{
    ASSERT(canvas != NULL);
    dvz_gpu_wait(canvas->gpu);

    canvas->cur_frame = 0;
    canvas->frame_idx = 0;
}



void dvz_canvas_recreate(DvzCanvas* canvas)
{
    ASSERT(canvas != NULL);
    DvzGpu* gpu = canvas->gpu;
    DvzSwapchain* swapchain = &canvas->render.swapchain;
    DvzFramebuffers* framebuffers = &canvas->render.framebuffers;
    DvzRenderpass* renderpass = canvas->render.renderpass;

    ASSERT(gpu != NULL);
    ASSERT(swapchain != NULL);
    ASSERT(framebuffers != NULL);
    ASSERT(renderpass != NULL);

    log_trace("recreate canvas after resize");
    DvzHost* host = gpu->host;
    ASSERT(host != NULL);

    // Wait until the device is ready and the window fully resized.
    dvz_gpu_wait(gpu);

    // Destroy swapchain resources.
    dvz_framebuffers_destroy(&canvas->render.framebuffers);
    dvz_images_destroy(&canvas->render.depth);
    dvz_images_destroy(canvas->render.swapchain.images);

    // Recreate the swapchain. This will automatically set the swapchain->images new size.
    // The new swpachain's size is determined by the surface's size, which is queried via Vulkan.
    dvz_swapchain_recreate(swapchain);

    // Find the new framebuffer size as determined by the swapchain recreation.
    uint32_t width = swapchain->images->shape[0];
    uint32_t height = swapchain->images->shape[1];

    canvas->width = width;
    canvas->height = height;

    // Check that we use the same DvzImages struct here.
    ASSERT(swapchain->images == framebuffers->attachments[0]);

    // Need to recreate the depth image with the new size.
    dvz_images_size(&canvas->render.depth, (uvec3){width, height, 1});
    dvz_images_create(&canvas->render.depth);

    // Recreate the framebuffers with the new size.
    for (uint32_t i = 0; i < framebuffers->attachment_count; i++)
    {
        ASSERT(framebuffers->attachments[i]->shape[0] == width);
        ASSERT(framebuffers->attachments[i]->shape[1] == height);
    }
    dvz_framebuffers_create(framebuffers, renderpass);

    // Recreate the semaphores.
    dvz_gpu_wait(canvas->gpu);
    dvz_semaphores_recreate(&canvas->sync.sem_img_available);
    dvz_semaphores_recreate(&canvas->sync.sem_render_finished);
    canvas->sync.present_semaphores = &canvas->sync.sem_render_finished;
}



void dvz_canvas_refill(DvzCanvas* canvas, DvzCanvasRefill refill, void* user_data)
{
    ASSERT(canvas != NULL);
    ASSERT(refill != NULL);
    canvas->refill = refill;
    canvas->refill_user_data = user_data;
}



void dvz_canvas_begin(DvzCanvas* canvas, DvzCommands* cmds, uint32_t idx)
{
    ASSERT(canvas != NULL);
    DvzGpu* gpu = canvas->gpu;
    ASSERT(gpu != NULL);
    dvz_cmd_begin(cmds, idx);
    dvz_cmd_begin_renderpass(cmds, idx, canvas->render.renderpass, &canvas->render.framebuffers);
}



void dvz_canvas_viewport(
    DvzCanvas* canvas, DvzCommands* cmds, uint32_t idx, vec2 offset, vec2 size)
{
    ASSERT(canvas != NULL);

    // A value of 0 = full canvas.
    float width = size[0], height = size[1];
    width = width > 0 ? width : (float)canvas->width;
    height = height > 0 ? height : (float)canvas->height;

    ASSERT(width > 0);
    ASSERT(height > 0);

    dvz_cmd_viewport(
        cmds, idx, (VkViewport){.x = offset[0], .y = offset[1], .width = width, .height = height});
}



void dvz_canvas_end(DvzCanvas* canvas, DvzCommands* cmds, uint32_t idx)
{
    ASSERT(canvas != NULL);
    ASSERT(cmds != NULL);
    dvz_cmd_end_renderpass(cmds, idx);
    dvz_cmd_end(cmds, idx);
}



void dvz_canvas_destroy(DvzCanvas* canvas)
{
    if (canvas == NULL || canvas->obj.status != DVZ_OBJECT_STATUS_CREATED)
    {
        log_trace(
            "skip destruction of already-destroyed canvas with status %d", canvas->obj.status);
        return;
    }
    log_debug("destroying canvas with status %d", canvas->obj.status);

    ASSERT(canvas != NULL);

    DvzGpu* gpu = canvas->gpu;
    ASSERT(gpu != NULL);

    DvzHost* host = gpu->host;
    ASSERT(host != NULL);

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

    // Destroy the swapchain.
    log_trace("canvas destroy swapchain");
    dvz_swapchain_destroy(&canvas->render.swapchain);

    // Destroy the framebuffers.
    log_trace("canvas destroy framebuffers");
    dvz_framebuffers_destroy(&canvas->render.framebuffers);

    // Destroy the Dear ImGui context if it was initialized.

    // HACK: we should NOT destroy imgui when using multiple DvzApp, since Dear ImGui uses
    // global context shared by all DvzApps. In practice, this is for now equivalent to using the
    // offscreen backend (which does not support Dear ImGui at the moment anyway).
    // if (canvas->app->backend != DVZ_BACKEND_OFFSCREEN)
    //     dvz_imgui_destroy();

    // Destroy the semaphores.
    log_trace("canvas destroy semaphores");
    dvz_semaphores_destroy(&canvas->sync.sem_img_available);
    dvz_semaphores_destroy(&canvas->sync.sem_render_finished);

    // Destroy the fences.
    log_trace("canvas destroy fences");
    dvz_fences_destroy(&canvas->sync.fences_render_finished);

    FREE(canvas->render.swapchain.images);

    // NOTE: the Input destruction must occur AFTER the window destruction, otherwise the window
    // glfw callbacks might enqueue input events to a destroyed deq, causing a segfault.
    // dvz_input_destroy(&canvas->input);

    dvz_obj_destroyed(&canvas->obj);
}
