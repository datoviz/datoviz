#include "../include/visky/canvas.h"
#include "../include/visky/context.h"
#include "../src/vklite2_utils.h"
#include <stdlib.h>


/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Canvas creation                                                                              */
/*************************************************************************************************/

VklCanvas* vkl_canvas(VklGpu* gpu, uint32_t width, uint32_t height)
{
    ASSERT(gpu != NULL);
    VklApp* app = gpu->app;

    ASSERT(app != NULL);
    if (app->canvases == NULL)
    {
        INSTANCES_INIT(
            VklCanvas, app, canvases, max_canvases, VKL_MAX_WINDOWS, VKL_OBJECT_TYPE_CANVAS)
    }

    INSTANCE_NEW(VklCanvas, canvas, app->canvases, app->max_canvases)
    canvas->app = app;
    canvas->width = width;
    canvas->height = height;

    INSTANCES_INIT(
        VklCommands, canvas, commands, max_commands, VKL_MAX_COMMANDS, VKL_OBJECT_TYPE_COMMANDS)
    INSTANCES_INIT(
        VklRenderpass, canvas, renderpasses, max_renderpasses, VKL_MAX_RENDERPASSES,
        VKL_OBJECT_TYPE_RENDERPASS)
    INSTANCES_INIT(
        VklSemaphores, canvas, semaphores, max_semaphores, VKL_MAX_SEMAPHORES,
        VKL_OBJECT_TYPE_SEMAPHORES)
    INSTANCES_INIT(VklFences, canvas, fences, max_fences, VKL_MAX_FENCES, VKL_OBJECT_TYPE_FENCES)
    INSTANCES_INIT(
        VklSwapchain, canvas, swapchains, max_swapchains, VKL_MAX_WINDOWS,
        VKL_OBJECT_TYPE_SWAPCHAIN)
    INSTANCES_INIT(
        VklFramebuffers, canvas, framebuffers, max_framebuffers, VKL_MAX_FRAMEBUFFERS,
        VKL_OBJECT_TYPE_FRAMEBUFFER)


    // TODO: create semaphores, fences, swap chain, renderpass, etc.

    return canvas;
}



/*************************************************************************************************/
/*  Event loop                                                                                   */
/*************************************************************************************************/

void vkl_canvas_frame(VklCanvas* canvas)
{
    ASSERT(canvas != NULL);

    // TODO
    // call EVENT callbacks (for backends only), which may enqueue some events
    // FRAME callbacks (rarely used)
    // check canvas.need_refill (atomic)
    // if refill needed, wait for current fence, and call the refill callbacks
}



void vkl_canvas_frame_submit(VklCanvas* canvas)
{
    ASSERT(canvas != NULL);
    // TODO
    // loop over all canvas commands on the RENDER queue (skip inactive ones)
    // add them to a new Submit
    // send the command associated to the current swapchain image
    // if resize, call RESIZE callback before cmd_reset
    // between send and present, call POST_SEND callback
}



void vkl_app_begin(VklApp* app)
{
    ASSERT(app != NULL);
    // TODO
    // start timer
    // start timer thread
}



void vkl_app_end(VklApp* app)
{
    ASSERT(app != NULL);
    // TODO
    // stop timer
    // stop timer thread and join it
    // destroy app
}



void vkl_app_run(VklApp* app, uint64_t frame_count)
{
    ASSERT(app != NULL);
    if (frame_count == 0)
        frame_count = UINT64_MAX;
    ASSERT(frame_count > 0);

    vkl_app_begin(app);

    VklCanvas* canvas = NULL;

    // Main loop.
    uint32_t n_canvas_active = 0;
    for (uint64_t frame_idx = 0; frame_idx < frame_count; frame_idx++)
    {
        log_trace("frame %d/%d", frame_idx, frame_count);
        n_canvas_active = 0;

        // Loop over the canvases.
        for (uint32_t canvas_idx = 0; canvas_idx < app->max_canvases; canvas_idx++)
        {
            // Get the current canvas.
            canvas = &app->canvases[canvas_idx];
            ASSERT(canvas != NULL);
            if (canvas->obj.status == VKL_OBJECT_STATUS_NONE)
                break;
            if (canvas->obj.status < VKL_OBJECT_STATUS_CREATED)
                continue;
            ASSERT(canvas->obj.status >= VKL_OBJECT_STATUS_CREATED);

            // Frame logic.
            log_trace("frame logic for canvas #%d", canvas_idx);
            vkl_canvas_frame(canvas);

            // Destroy the canvas if needed.
            if (backend_window_should_close(app->backend, canvas->window->backend_window))
                canvas->window->obj.status = VKL_OBJECT_STATUS_NEED_DESTROY;
            if (canvas->obj.status == VKL_OBJECT_STATUS_NEED_DESTROY)
            {
                log_trace("destroying canvas #%d", canvas_idx);
                // Wait for all GPUs to be idle.
                vkl_app_wait(app);

                // Destroy the canvas.
                vkl_canvas_destroy(canvas);
                continue;
            }

            // Submit the command buffers and swapchain logic.
            log_trace("submitting frame for canvas #%d", canvas_idx);
            vkl_canvas_frame_submit(canvas);
            n_canvas_active++;
        }

        // TODO: this has never been tested with multiple GPUs yet.
        VklGpu* gpu = NULL;
        VklContext* ctx = NULL;
        for (uint32_t gpu_idx = 0; gpu_idx < app->gpu_count; gpu_idx++)
        {
            gpu = &app->gpus[gpu_idx];
            if (gpu->obj.status < VKL_OBJECT_STATUS_CREATED)
                break;
            ctx = gpu->context;

            // Process the pending transfer tasks.
            if (ctx->obj.status >= VKL_OBJECT_STATUS_CREATED)
            {
                log_trace("processing transfers for GPU #%d", gpu_idx);
                vkl_transfer_loop(ctx, false);
            }

            // IMPORTANT: we need to wait for the present queue to be idle, otherwise the GPU hangs
            // when waiting for fences (not sure why). The problem only arises when using different
            // queues for command buffer submission and swapchain present.
            if (gpu->queues.queues[VKL_DEFAULT_QUEUE_PRESENT] !=
                gpu->queues.queues[VKL_DEFAULT_QUEUE_RENDER])
            {
                vkl_gpu_queue_wait(gpu, VKL_DEFAULT_QUEUE_PRESENT);
            }
        }

        // Close the application if all canvases have been closed.
        if (n_canvas_active == 0)
            break;
    }
    log_trace("end main loop");

    vkl_app_end(app);
}



/*************************************************************************************************/
/*  Canvas destruction                                                                           */
/*************************************************************************************************/

void vkl_canvas_destroy(VklCanvas* canvas)
{
    if (canvas == NULL || canvas->obj.status == VKL_OBJECT_STATUS_DESTROYED)
    {
        log_trace("skip destruction of already-destroyed canvas");
        return;
    }

    // Destroy the window
    if (canvas->window != NULL)
    {
        vkl_window_destroy(canvas->window);
    }

    // TODO
    // join the background thread


    log_trace("canvas destroy commands");
    for (uint32_t i = 0; i < canvas->max_commands; i++)
    {
        if (canvas->commands[i].obj.status == VKL_OBJECT_STATUS_NONE)
            break;
        vkl_commands_destroy(&canvas->commands[i]);
    }
    INSTANCES_DESTROY(canvas->commands)


    log_trace("canvas destroy renderpass(es)");
    for (uint32_t i = 0; i < canvas->max_renderpasses; i++)
    {
        if (canvas->renderpasses[i].obj.status == VKL_OBJECT_STATUS_NONE)
            break;
        vkl_renderpass_destroy(&canvas->renderpasses[i]);
    }
    INSTANCES_DESTROY(canvas->renderpasses)


    log_trace("canvas destroy semaphores");
    for (uint32_t i = 0; i < canvas->max_semaphores; i++)
    {
        if (canvas->semaphores[i].obj.status == VKL_OBJECT_STATUS_NONE)
            break;
        vkl_semaphores_destroy(&canvas->semaphores[i]);
    }
    INSTANCES_DESTROY(canvas->semaphores)


    log_trace("canvas destroy fences");
    for (uint32_t i = 0; i < canvas->max_fences; i++)
    {
        if (canvas->fences[i].obj.status == VKL_OBJECT_STATUS_NONE)
            break;
        vkl_fences_destroy(&canvas->fences[i]);
    }
    INSTANCES_DESTROY(canvas->fences)


    log_trace("canvas destroy swapchains");
    for (uint32_t i = 0; i < canvas->max_swapchains; i++)
    {
        if (canvas->swapchains[i].obj.status == VKL_OBJECT_STATUS_NONE)
            break;
        vkl_swapchain_destroy(&canvas->swapchains[i]);
    }
    INSTANCES_DESTROY(canvas->swapchains)


    log_trace("canvas destroy framebuffers");
    for (uint32_t i = 0; i < canvas->max_framebuffers; i++)
    {
        if (canvas->framebuffers[i].obj.status == VKL_OBJECT_STATUS_NONE)
            break;
        vkl_framebuffers_destroy(&canvas->framebuffers[i]);
    }
    INSTANCES_DESTROY(canvas->framebuffers)


    obj_destroyed(&canvas->obj);
}



void vkl_canvases_destroy(uint32_t canvas_count, VklCanvas* canvases)
{
    for (uint32_t i = 0; i < canvas_count; i++)
    {
        if (canvases[i].obj.status == VKL_OBJECT_STATUS_NONE)
            break;
        vkl_canvas_destroy(&canvases[i]);
    }
}
