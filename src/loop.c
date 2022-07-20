/*************************************************************************************************/
/*  Simple loop                                                                                  */
/*************************************************************************************************/

#include "loop.h"
#include "_glfw.h"
#include "canvas.h"
#include "common.h"
#include "gui.h"
#include "host.h"
#include "surface.h"
#include "vklite.h"
#include "window.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzLoop* dvz_loop(DvzGpu* gpu, uint32_t width, uint32_t height, int flags)
{
    ASSERT(gpu != NULL);
    DvzLoop* loop = calloc(1, sizeof(DvzLoop));
    loop->flags = flags;
    loop->gpu = gpu;

    // Create the window and surface.
    // NOTE: glfw hard-coded for now
    loop->window = dvz_window(DVZ_BACKEND_GLFW, width, height, flags);
    loop->surface = dvz_window_surface(gpu->host, &loop->window);

    // Create the renderpass.
    loop->renderpass =
        dvz_gpu_renderpass(gpu, DVZ_DEFAULT_CLEAR_COLOR, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    // Create the canvas.
    loop->canvas = dvz_canvas(gpu, &loop->renderpass, width, height, 0);
    dvz_canvas_create(&loop->canvas, loop->surface);

    // Create GUI objects if needed.
    if ((flags & DVZ_CANVAS_FLAGS_IMGUI))
    {
        loop->gui = dvz_gui(gpu, DVZ_DEFAULT_QUEUE_RENDER);

        loop->gui_window = dvz_gui_window(
            loop->gui, &loop->window, loop->canvas.render.swapchain.images,
            DVZ_DEFAULT_QUEUE_RENDER);
    }

    return loop;
}



void dvz_loop_refill(DvzLoop* loop, DvzCanvasRefill callback, void* user_data)
{
    ASSERT(loop != NULL);
    dvz_canvas_refill(&loop->canvas, callback, user_data);
}



void dvz_loop_overlay(DvzLoop* loop, DvzLoopOverlay callback, void* user_data)
{
    ASSERT(loop != NULL);
    loop->overlay = callback;
    loop->overlay_data = user_data;
}



int dvz_loop_frame(DvzLoop* loop)
{
    ASSERT(loop != NULL);

    DvzGpu* gpu = loop->gpu;
    ASSERT(gpu != NULL);

    DvzCanvas* canvas = &loop->canvas;
    DvzWindow* window = &loop->window;

    ASSERT(canvas != NULL);
    ASSERT(window != NULL);
    ASSERT(canvas->refill != NULL);

    DvzSwapchain* swapchain = &canvas->render.swapchain;
    DvzFramebuffers* framebuffers = &canvas->render.framebuffers;
    DvzRenderpass* renderpass = canvas->render.renderpass;
    DvzFences* fences = &canvas->sync.fences_render_finished;
    DvzFences* fences_bak = &canvas->sync.fences_flight;
    DvzSemaphores* sem_img_available = &canvas->sync.sem_img_available;
    DvzSemaphores* sem_render_finished = &canvas->sync.sem_render_finished;
    DvzCommands* cmds = &canvas->cmds;
    DvzSubmit* submit = &canvas->render.submit;

    ASSERT(swapchain != NULL);
    ASSERT(framebuffers != NULL);
    ASSERT(renderpass != NULL);
    ASSERT(fences != NULL);
    ASSERT(fences_bak != NULL);
    ASSERT(sem_img_available != NULL);
    ASSERT(sem_render_finished != NULL);
    ASSERT(cmds != NULL);
    ASSERT(submit != NULL);

    // At the beginning, fill the command buffer.
    if (loop->frame_idx == 0)
    {
        for (uint32_t i = 0; i < cmds->count; i++)
        {
            dvz_cmd_reset(cmds, i);
            canvas->refill(canvas, cmds, i, canvas->refill_user_data);
        }
    }

    // Main loop->
    // log_debug("iteration %d", frame);

    backend_poll_events(gpu->host->backend);

    if (backend_should_close(window) || window->obj.status == DVZ_OBJECT_STATUS_NEED_DESTROY)
        return -1;

    // Wait for fence.
    dvz_fences_wait(fences, canvas->cur_frame);

    // We acquire the next swapchain image.
    dvz_swapchain_acquire(swapchain, sem_img_available, canvas->cur_frame, NULL, 0);
    if (swapchain->obj.status == DVZ_OBJECT_STATUS_INVALID)
    {
        dvz_gpu_wait(gpu);
        return 0;
    }
    // Handle resizing.
    else if (swapchain->obj.status == DVZ_OBJECT_STATUS_NEED_RECREATE)
    {
        log_trace("recreating the swapchain");

        // Wait until the device is ready and the window fully resized.
        // Framebuffer new size.
        dvz_gpu_wait(gpu);
        dvz_window_poll_size(window);

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

        // Need to recreate the depth image with the new size.
        dvz_images_size(&canvas->render.depth, (uvec3){width, height, 1});
        dvz_images_create(&canvas->render.depth);

        // Recreate the framebuffers with the new size.
        ASSERT(framebuffers->attachments[0]->shape[0] == width);
        ASSERT(framebuffers->attachments[0]->shape[1] == height);
        dvz_framebuffers_create(framebuffers, renderpass);

        // TODO: canvas_recreate??
        // TODO: recreate gui framebuffers if any

        // Need to refill the command buffers.
        for (uint32_t i = 0; i < cmds->count; i++)
        {
            dvz_cmd_reset(cmds, i);
            canvas->refill(canvas, cmds, i, canvas->refill_user_data);
        }
    }
    else
    {
        dvz_fences_copy(fences, canvas->cur_frame, fences_bak, swapchain->img_idx);

        // Reset the Submit instance before adding the command buffers.
        dvz_submit_reset(submit);

        // Then, we submit the cmds on that image
        dvz_submit_commands(submit, cmds);

        // Custom callback for overlay filling.
        if (canvas->fill_overlay)
        {
            canvas->fill_overlay(canvas, canvas->fill_overlay_user_data);
        }

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

    return 0; // if -1, stop the loop
}



void dvz_loop_run(DvzLoop* loop, uint64_t n_frames)
{
    ASSERT(loop != NULL);

    uint64_t n = (n_frames > 0 ? n_frames : INFINITY);
    for (loop->frame_idx = 0; loop->frame_idx < n; loop->frame_idx++)
    {
        log_trace("running loop frame #%d", loop->frame_idx);
        if (dvz_loop_frame(loop))
            break;
    }
}



void dvz_loop_destroy(DvzLoop* loop)
{
    ASSERT(loop != NULL);
    dvz_renderpass_destroy(&loop->renderpass);
    dvz_surface_destroy(loop->gpu->host, loop->surface);
    dvz_canvas_destroy(&loop->canvas);
    dvz_window_destroy(&loop->window);
    if (loop->gui != NULL)
        dvz_gui_destroy(loop->gui);
    if (loop->gui_window != NULL)
        dvz_gui_window_destroy(loop->gui_window);
    FREE(loop);
}
