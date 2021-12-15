/*************************************************************************************************/
/*  Canvas                                                                                       */
/*************************************************************************************************/

#include "canvas.h"
#include "_glfw.h"
#include "canvas_utils.h"
#include "common.h"
#include "vklite.h"
#include "window.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzCanvas dvz_canvas(DvzGpu* gpu, uint32_t width, uint32_t height, int flags)
{
    ASSERT(gpu != NULL);
    ASSERT(width > 0);
    ASSERT(height > 0);

    DvzCanvas canvas = {0};
    canvas.gpu = gpu;
    canvas.flags = flags;
    canvas.width = width;
    canvas.height = height;
    canvas.format = DVZ_DEFAULT_FORMAT;
    canvas.refill = blank_commands;

    dvz_obj_init(&canvas.obj);
    return canvas;
}



void dvz_canvas_create(DvzCanvas* canvas)
{
    ASSERT(canvas != NULL);
    DvzGpu* gpu = canvas->gpu;
    ASSERT(gpu != NULL);
    DvzHost* host = gpu->host;
    ASSERT(host != NULL);

    log_trace("creating the canvas");

    // Make the window.
    canvas->window = dvz_window(host, canvas->width, canvas->height);

    // Make the renderpass.
    make_renderpass(
        gpu, &canvas->render.renderpass, DVZ_DEFAULT_FORMAT, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        get_clear_color(DVZ_DEFAULT_CLEAR_COLOR));

    // Make the swapchain.
    make_swapchain(gpu, canvas->window, &canvas->render.swapchain, DVZ_MIN_SWAPCHAIN_IMAGE_COUNT);

    // Make depth buffer image.
    make_depth(gpu, &canvas->render.depth, canvas->width, canvas->height);

    // Make staging image.
    make_staging(gpu, &canvas->render.staging, canvas->format, canvas->width, canvas->height);

    // Make framebuffers.
    make_framebuffers(
        gpu, &canvas->render.framebuffers, &canvas->render.renderpass,
        canvas->render.swapchain.images, &canvas->render.depth);

    // Make synchronization objects.
    make_sync(gpu, &canvas->sync, canvas->render.swapchain.img_count);

    // Command buffer.
    canvas->cmds =
        dvz_commands(canvas->gpu, DVZ_DEFAULT_QUEUE_RENDER, canvas->render.swapchain.img_count);
    // for (uint32_t i = 0; i < canvas->cmds.count; i++)
    //     canvas->refill(canvas, &canvas->cmds, i);

    // Default submit.
    canvas->render.submit = dvz_submit(canvas->gpu);

    // Input.
    log_trace("creating canvas input");
    canvas->input = dvz_input();
    dvz_input_attach(&canvas->input, canvas->window);

    dvz_obj_created(&canvas->obj);
    log_trace("canvas created with size %dx%d", canvas->width, canvas->height);
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
    DvzWindow* window = canvas->window;
    DvzGpu* gpu = canvas->gpu;
    DvzSwapchain* swapchain = &canvas->render.swapchain;
    DvzFramebuffers* framebuffers = &canvas->render.framebuffers;
    DvzRenderpass* renderpass = &canvas->render.renderpass;

    ASSERT(window != NULL);
    ASSERT(gpu != NULL);
    ASSERT(swapchain != NULL);
    ASSERT(framebuffers != NULL);
    ASSERT(renderpass != NULL);

    log_trace("recreate canvas after resize");
    DvzHost* host = gpu->host;
    ASSERT(host != NULL);

    // Wait until the device is ready and the window fully resized.
    // Framebuffer new size.
    uint32_t width, height;
    backend_window_get_size(window, &window->width, &window->height, &width, &height);
    dvz_gpu_wait(gpu);

    // Destroy swapchain resources.
    dvz_framebuffers_destroy(&canvas->render.framebuffers);
    dvz_images_destroy(&canvas->render.depth);
    dvz_images_destroy(canvas->render.swapchain.images);

    // Recreate the swapchain. This will automatically set the swapchain->images new size.
    dvz_swapchain_recreate(swapchain);

    // Find the new framebuffer size as determined by the swapchain recreation.
    width = swapchain->images->shape[0];
    height = swapchain->images->shape[1];

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



void dvz_canvas_refill(DvzCanvas* canvas, DvzCanvasRefill refill)
{
    ASSERT(canvas != NULL);
    ASSERT(refill != NULL);
    canvas->refill = refill;
}



void dvz_canvas_loop(DvzCanvas* canvas, uint64_t n_frames)
{
    ASSERT(canvas != NULL);

    DvzWindow* window = canvas->window;
    ASSERT(window != NULL);

    DvzGpu* gpu = canvas->gpu;
    ASSERT(gpu != NULL);

    DvzSwapchain* swapchain = &canvas->render.swapchain;
    DvzFramebuffers* framebuffers = &canvas->render.framebuffers;
    DvzRenderpass* renderpass = &canvas->render.renderpass;
    DvzFences* fences = &canvas->sync.fences_render_finished;
    DvzFences* fences_bak = &canvas->sync.fences_flight;
    DvzSemaphores* sem_img_available = &canvas->sync.sem_img_available;
    DvzSemaphores* sem_render_finished = &canvas->sync.sem_render_finished;
    DvzCommands* cmds = &canvas->cmds;
    DvzSubmit* submit = &canvas->render.submit;

    // Need to fill the command buffers.
    for (uint32_t i = 0; i < cmds->count; i++)
    {
        dvz_cmd_reset(cmds, i);
        canvas->refill(canvas, cmds, i);
    }

    for (uint32_t frame = 0; n_frames == 0 || frame < n_frames; frame++)
    {
        log_debug("iteration %d", frame);

        backend_poll_events(window);

        if (backend_window_should_close(window) ||
            window->obj.status == DVZ_OBJECT_STATUS_NEED_DESTROY)
            break;

        // Wait for fence.
        dvz_fences_wait(fences, canvas->cur_frame);

        // We acquire the next swapchain image.
        dvz_swapchain_acquire(swapchain, sem_img_available, canvas->cur_frame, NULL, 0);
        if (swapchain->obj.status == DVZ_OBJECT_STATUS_INVALID)
        {
            dvz_gpu_wait(gpu);
            break;
        }
        // Handle resizing.
        else if (swapchain->obj.status == DVZ_OBJECT_STATUS_NEED_RECREATE)
        {
            log_trace("recreating the swapchain");

            // Wait until the device is ready and the window fully resized.
            // Framebuffer new size.
            uint32_t width, height;
            backend_window_get_size(
                window,                          //
                &window->width, &window->height, //
                &width, &height);
            dvz_gpu_wait(gpu);

            // Destroy swapchain resources.
            dvz_framebuffers_destroy(framebuffers);
            dvz_images_destroy(&canvas->render.depth);
            dvz_images_destroy(canvas->render.swapchain.images);

            // Recreate the swapchain. This will automatically set the swapchain->images new
            // size.
            dvz_swapchain_recreate(swapchain);
            // Find the new framebuffer size as determined by the swapchain recreation.
            width = swapchain->images->shape[0];
            height = swapchain->images->shape[1];

            // Need to recreate the depth image with the new size.
            dvz_images_size(&canvas->render.depth, (uvec3){width, height, 1});
            dvz_images_create(&canvas->render.depth);

            // Recreate the framebuffers with the new size.
            ASSERT(framebuffers->attachments[0]->shape[0] == width);
            ASSERT(framebuffers->attachments[0]->shape[1] == height);
            dvz_framebuffers_create(framebuffers, renderpass);

            // Need to refill the command buffers.
            for (uint32_t i = 0; i < cmds->count; i++)
            {
                dvz_cmd_reset(cmds, i);
                canvas->refill(canvas, cmds, i);
            }
        }
        else
        {
            dvz_fences_copy(fences, canvas->cur_frame, fences_bak, swapchain->img_idx);

            // Reset the Submit instance before adding the command buffers.
            dvz_submit_reset(submit);

            // Then, we submit the cmds on that image
            dvz_submit_commands(submit, cmds);
            dvz_submit_wait_semaphores(
                submit, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, sem_img_available,
                canvas->cur_frame);
            // Once the render is finished, we signal another semaphore.
            dvz_submit_signal_semaphores(submit, sem_render_finished, canvas->cur_frame);
            dvz_submit_send(submit, swapchain->img_idx, fences, canvas->cur_frame);

            // Once the image is rendered, we present the swapchain image.
            dvz_swapchain_present(swapchain, 1, sem_render_finished, canvas->cur_frame);

            canvas->cur_frame = (canvas->cur_frame + 1) % DVZ_MAX_FRAMES_IN_FLIGHT;
        }

        // IMPORTANT: we need to wait for the present queue to be idle, otherwise the GPU hangs
        // when waiting for fences (not sure why). The problem only arises when using different
        // queues for command buffer submission and swapchain present.
        dvz_queue_wait(gpu, DVZ_DEFAULT_QUEUE_PRESENT);
    }

    dvz_gpu_wait(gpu);
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
    backend_poll_events(canvas->window);

    // Wait on the GPU.
    dvz_gpu_wait(gpu);

    // Destroy the graphics.
    log_trace("canvas destroy graphics pipelines");

    // Destroy the images.
    dvz_images_destroy(canvas->render.swapchain.images);
    dvz_images_destroy(&canvas->render.depth);
    dvz_images_destroy(&canvas->render.staging);

    // Destroy the renderpasses.
    log_trace("canvas destroy renderpass");
    dvz_renderpass_destroy(&canvas->render.renderpass);

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

    // Destroy the window.
    log_trace("canvas destroy window");
    if (canvas->window != NULL)
    {
        dvz_window_destroy(canvas->window);
    }

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
    dvz_input_destroy(&canvas->input);

    dvz_obj_destroyed(&canvas->obj);
}
