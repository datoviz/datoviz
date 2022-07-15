/*************************************************************************************************/
/*  Canvas                                                                                       */
/*************************************************************************************************/

#include "canvas_window.h"
#include "_glfw.h"
#include "canvas.h"
#include "common.h"
#include "host.h"
#include "vklite.h"
#include "window.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

void dvz_canvas_loop(DvzCanvas* canvas, DvzWindow* window, uint64_t n_frames)
{
    ASSERT(canvas != NULL);
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

        backend_poll_events(gpu->host->backend);

        if (backend_should_close(window) || window->obj.status == DVZ_OBJECT_STATUS_NEED_DESTROY)
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
