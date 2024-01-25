/*************************************************************************************************/
/*  Canvas utils                                                                                 */
/*************************************************************************************************/

#ifndef DVZ_HEADER_CANVAS_UTILS
#define DVZ_HEADER_CANVAS_UTILS



/*************************************************************************************************/
/*  Imports                                                                                      */
/*************************************************************************************************/

// #include "board.h"
#include "canvas.h"
#include "render_utils.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static void make_sync(DvzGpu* gpu, DvzSync* sync, uint32_t img_count)
{
    ANN(gpu);
    ANN(sync);
    log_trace("making sync objects");

    uint32_t frames_in_flight = DVZ_MAX_FRAMES_IN_FLIGHT;

    sync->sem_img_available = dvz_semaphores(gpu, frames_in_flight);
    sync->sem_render_finished = dvz_semaphores(gpu, frames_in_flight);
    sync->present_semaphores = &sync->sem_render_finished;
    sync->fences_render_finished = dvz_fences(gpu, frames_in_flight, true);
    sync->fences_flight.gpu = gpu;
    sync->fences_flight.count = img_count;
}



// Submit the command buffers, + swapchain synchronization + presentation if not offscreen.
static void canvas_render(DvzCanvas* canvas)
{
    ANN(canvas);

    DvzSubmit* s = &canvas->render.submit;
    uint32_t f = canvas->cur_frame;
    uint32_t img_idx = canvas->render.swapchain.img_idx;
    log_trace("render canvas frame #%d", img_idx);

    // Keep track of the fence associated to the current swapchain image.
    dvz_fences_copy(
        &canvas->sync.fences_render_finished, f, //
        &canvas->sync.fences_flight, img_idx);

    // Reset the Submit instance before adding the command buffers.
    dvz_submit_reset(s);

    // Render command buffers empty? Fill them with blank color by default.
    if (canvas->cmds.obj.status != DVZ_OBJECT_STATUS_CREATED)
    {
        log_debug("empty command buffers, filling with blank color");
        for (uint32_t i = 0; i < canvas->render.swapchain.img_count; i++)
            blank_commands(
                canvas->render.renderpass, &canvas->render.swapchain, &canvas->render.framebuffers,
                &canvas->cmds, i, canvas->refill_data);
    }

    ASSERT(canvas->cmds.obj.status == DVZ_OBJECT_STATUS_CREATED);
    // Add the command buffers to the submit instance.
    dvz_submit_commands(s, &canvas->cmds);

    if (s->commands_count == 0)
    {
        log_error("no recorded command buffers");
        return;
    }

    dvz_submit_wait_semaphores(
        s, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, //
        &canvas->sync.sem_img_available, f);

    // Once the render is finished, we signal another semaphore.
    dvz_submit_signal_semaphores(s, &canvas->sync.sem_render_finished, f);

    // SEND callbacks and send the Submit instance.

    // Send the Submit instance.
    dvz_submit_send(s, img_idx, &canvas->sync.fences_render_finished, f);

    // Once the image is rendered, we present the swapchain image.
    // The semaphore used for waiting during presentation may be changed by the canvas
    // callbacks.
    dvz_swapchain_present(
        &canvas->render.swapchain, 1,    //
        canvas->sync.present_semaphores, // waiting semaphore, alias to sem_render_finished
        CLIP(f, 0, canvas->sync.present_semaphores->count - 1));

    canvas->cur_frame = (f + 1) % canvas->sync.fences_render_finished.count;
}



#endif
