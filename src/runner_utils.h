/*************************************************************************************************/
/*  Runner utils                                                                                 */
/*************************************************************************************************/

#ifndef DVZ_HEADER_RUNNER_UTILS
#define DVZ_HEADER_RUNNER_UTILS



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_glfw.h"
#include "canvas_utils.h"
#include "renderer.h"
#include "runner.h"
#include "vklite.h"
#include "vklite_utils.h"
#include "workspace.h"



/*************************************************************************************************/
/*  Task enqueueing                                                                              */
/*************************************************************************************************/

static void _enqueue_canvas_event(
    DvzRunner* runner, DvzCanvas* canvas, uint32_t deq_idx, DvzCanvasEventType type)
{
    ASSERT(runner != NULL);
    ASSERT(canvas != NULL);

    // Will be FREE-ed by the dequeue batch function in the main loop.
    DvzCanvasEvent* ev = (DvzCanvasEvent*)calloc(1, sizeof(DvzCanvasEvent));
    ev->canvas = canvas;
    dvz_deq_enqueue(&runner->deq, deq_idx, (int)type, ev);
}



static void _enqueue_canvas_frame(DvzRunner* runner, DvzCanvas* canvas, uint32_t q_idx)
{
    ASSERT(runner != NULL);
    ASSERT(canvas != NULL);
    // log_info("enqueue frame");

    DvzCanvasEventFrame* ev = (DvzCanvasEventFrame*)calloc(1, sizeof(DvzCanvasEventFrame));
    ev->canvas = canvas;
    ev->frame_idx = ev->canvas->frame_idx;
    dvz_deq_enqueue(&runner->deq, q_idx, (int)DVZ_RUNNER_CANVAS_FRAME, ev);
}



static void _enqueue_upfill(
    DvzRunner* runner, DvzCanvas* canvas, DvzDat* dat, //
    VkDeviceSize offset, VkDeviceSize size, void* data)
{
    ASSERT(runner != NULL);
    ASSERT(canvas != NULL);

    DvzCanvasEventUpfill* ev = (DvzCanvasEventUpfill*)calloc(1, sizeof(DvzCanvasEventUpfill));
    ev->canvas = canvas;
    ev->dat = dat;
    ev->offset = offset;
    ev->size = size;
    ev->data = data;
    dvz_deq_enqueue(&runner->deq, DVZ_RUNNER_DEQ_MAIN, (int)DVZ_RUNNER_CANVAS_UPFILL, ev);
}



static void _enqueue_to_refill(DvzRunner* runner, DvzCanvas* canvas)
{
    ASSERT(runner != NULL);
    ASSERT(canvas != NULL);

    DvzCanvasEvent* ev = (DvzCanvasEvent*)calloc(1, sizeof(DvzCanvasEvent));
    ev->canvas = canvas;
    dvz_deq_enqueue_first(
        &runner->deq, DVZ_RUNNER_DEQ_REFILL, (int)DVZ_RUNNER_CANVAS_TO_REFILL, ev);
}



static void _enqueue_refill(
    DvzRunner* runner, DvzCanvas* canvas, DvzCommands* cmds, uint32_t cmd_idx, bool wrap)
{
    ASSERT(runner != NULL);
    ASSERT(canvas != NULL);
    // log_debug("enqueue refill #%d", cmd_idx);

    DvzCanvasEventRefill* ev = (DvzCanvasEventRefill*)calloc(1, sizeof(DvzCanvasEventRefill));
    ev->canvas = canvas;
    ev->cmds = cmds;
    ev->cmd_idx = cmd_idx;
    dvz_deq_enqueue(
        &runner->deq, DVZ_RUNNER_DEQ_REFILL,
        (int)(wrap ? DVZ_RUNNER_CANVAS_REFILL_WRAP : DVZ_RUNNER_CANVAS_REFILL), ev);
}



static uint32_t _enqueue_frames(DvzRunner* runner)
{
    ASSERT(runner != NULL);

    DvzRenderer* renderer = runner->renderer;
    ASSERT(renderer != NULL);

    DvzWorkspace* workspace = renderer->workspace;
    ASSERT(workspace != NULL);

    // Go through all canvases.
    uint32_t n_canvas_running = 0;
    DvzCanvas* canvas = NULL;
    DvzContainerIterator iter = dvz_container_iterator(&workspace->canvases);
    log_trace("iterate through canvases");
    while (iter.item != NULL)
    {
        canvas = (DvzCanvas*)iter.item;
        ASSERT(canvas != NULL);
        ASSERT(canvas->obj.type == DVZ_OBJECT_TYPE_CANVAS);

        // NOTE: Canvas is active iff its status is created, and it has the "running" flag.
        // In that case, enqueue a FRAME event for that canvas.
        if (dvz_obj_is_created(&canvas->obj)) // TODO: && canvas->running)
        {
            _enqueue_canvas_frame(runner, canvas, DVZ_RUNNER_DEQ_FRAME);
            n_canvas_running++;
        }

        // NOTE: enqueue a REFILL event at the first frame.
        if (canvas->frame_idx == 0)
        {
            log_debug("refill canvas because frame #0");
            _enqueue_to_refill(runner, canvas);
        }

        dvz_container_iter(&iter);
    }

    return n_canvas_running;
}



/*************************************************************************************************/
/*  Utils for the runner module */
/*************************************************************************************************/

static void _run_flush(DvzRunner* runner)
{
    ASSERT(runner != NULL);
    ASSERT(runner->renderer != NULL);
    ASSERT(runner->renderer->gpu != NULL);
    ASSERT(runner->renderer->gpu->host != NULL);

    DvzHost* host = runner->renderer->gpu->host;
    ASSERT(host != NULL);

    log_debug("flush runner instance");

    backend_poll_events(host);

    // Flush all queues.
    for (uint32_t i = 0; i < 4; i++)
    {
        log_debug("flush deq #%d", i);
        dvz_deq_dequeue_batch(&runner->deq, i);
    }
    // for (uint32_t i = 0; i < 4; i++)
    //     dvz_deq_wait(&runner->deq, i);

    dvz_host_wait(host);
}



static void _gpu_sync_hack(DvzHost* host)
{
    // BIG HACK: this call is required at every frame, otherwise the event loop randomly crashes,
    // fences deadlock (?) and the Vulkan validation layers raise errors, causing the whole system
    // to crash for ~20 seconds. This is probably a ugly hack and I'd appreciate any help from a
    // Vulkan synchronization expert.

    // NOTE: this has never been tested with multiple GPUs yet.
    DvzContainerIterator iterator = dvz_container_iterator(&host->gpus);
    DvzGpu* gpu = NULL;
    while (iterator.item != NULL)
    {
        gpu = (DvzGpu*)iterator.item;
        if (!dvz_obj_is_created(&gpu->obj))
            break;

        // IMPORTANT: we need to wait for the present queue to be idle, otherwise the GPU hangs
        // when waiting for fences (not sure why). The problem only arises when using different
        // queues for command buffer submission and swapchain present. There has be a better
        // way to fix this.
        if (gpu->queues.queues[DVZ_DEFAULT_QUEUE_PRESENT] != VK_NULL_HANDLE &&
            gpu->queues.queues[DVZ_DEFAULT_QUEUE_PRESENT] !=
                gpu->queues.queues[DVZ_DEFAULT_QUEUE_RENDER])
        {
            dvz_queue_wait(gpu, DVZ_DEFAULT_QUEUE_PRESENT);
        }

        dvz_container_iter(&iterator);
    }
}



static bool _canvas_check(DvzCanvas* canvas)
{
    if (!dvz_obj_is_created(&canvas->obj))
    {
        log_debug("skip canvas frame because canvas is invalid");
        return false;
    }
    return true;
}



// Return whether we should refill for the current frame. If so, reset the command buffer.
// The user-specified command buffer refill callback should be called just afterwards.
static bool _should_refill(DvzCanvas* canvas)
{
    ASSERT(canvas != NULL);
    if (!_canvas_check(canvas))
        return false;

    // Default command buffers for the canvas.
    DvzCommands* cmds = &canvas->cmds;
    uint32_t img_idx = canvas->render.swapchain.img_idx;

    // Check if the command buffer for the current swapchain image is blocked.
    bool should_refill = !cmds->blocked[img_idx];

    if (should_refill)
        dvz_cmd_reset(cmds, img_idx);

    return should_refill;
}



// Should be called right after the user-specified refill callback was called.
// This marks the current frame's current buffer as blocked.
static void _refill_done(DvzCanvas* canvas)
{
    ASSERT(canvas != NULL);
    if (!_canvas_check(canvas))
        return;

    DvzCommands* cmds = &canvas->cmds;
    uint32_t img_idx = canvas->render.swapchain.img_idx;

    // ASSERT(dvz_obj_is_created(&cmds->obj));

    // Immediately block the command buffer so that it is not refilled at every frame if that's not
    // useful.
    cmds->blocked[img_idx] = true;
}



// backend-specific
static void _canvas_frame(DvzRunner* runner, DvzCanvas* canvas)
{
    ASSERT(runner != NULL);
    ASSERT(canvas != NULL);

    log_trace("canvas frame #%d", canvas->frame_idx);

    // DvzHost* host = canvas->host;
    // ASSERT(host != NULL);

    // Process only created canvas.
    if (!_canvas_check(canvas))
        return;

    // Poll events.
    if (canvas->window != NULL)
        dvz_window_poll_events(canvas->window);

    // Raise TO_CLOSE if needed.
    // void* backend_window = canvas->window != NULL ? canvas->window->backend_window : NULL;
    if (backend_window_should_close(canvas->window))
    {
        log_trace("delete canvas %d", canvas->obj.id);
        DvzRequest req = dvz_delete_canvas(runner->requester, canvas->obj.id);
        dvz_runner_request(runner, req);

        return;
    }

    // NOTE: swapchain image acquisition happens here

    // We acquire the next swapchain image.
    // NOTE: this call modifies swapchain->img_idx
    // if (!canvas->offscreen)
    dvz_swapchain_acquire(
        &canvas->render.swapchain, &canvas->sync.sem_img_available, //
        canvas->cur_frame, NULL, 0);

    // Wait for fence.
    dvz_fences_wait(&canvas->sync.fences_flight, canvas->render.swapchain.img_idx);

    // If there is a problem with swapchain image acquisition, wait and try again later.
    if (canvas->render.swapchain.obj.status == DVZ_OBJECT_STATUS_INVALID)
    {
        log_trace("swapchain image acquisition failed, waiting and skipping this frame");
        dvz_gpu_wait(canvas->gpu);
        return;
    }

    // If the swapchain needs to be recreated (for example, after a resize), do it.
    if (canvas->render.swapchain.obj.status == DVZ_OBJECT_STATUS_NEED_RECREATE)
    {
        log_trace("swapchain image acquisition failed, enqueing a RECREATE task");
        _enqueue_canvas_event(runner, canvas, DVZ_RUNNER_DEQ_MAIN, DVZ_RUNNER_CANVAS_RECREATE);
        return;
    }

    // _canvas_refill(canvas);
    // At every frame, enqueue a REFILL_WRAP event in the REFILL queue, but the user-specified
    // REFILL callback may be blocked if there was no TO_REFILL event.
    DvzCommands* cmds = &canvas->cmds;
    uint32_t img_idx = canvas->render.swapchain.img_idx;
    _enqueue_refill(runner, canvas, cmds, img_idx, true);

    // If all good, enqueue a PRESENT task for that canvas. The PRESENT callback does the rendering
    // (cmd buf submission) and send the rendered image for presentation to the swapchain.
    // log_info("enqueue present");
    _enqueue_canvas_event(runner, canvas, DVZ_RUNNER_DEQ_PRESENT, DVZ_RUNNER_CANVAS_PRESENT);

    canvas->frame_idx++;
}



/*************************************************************************************************/
/*  Canvas callbacks                                                                             */
/*************************************************************************************************/

// Batch callback from the FRAME event in the FRAME queue.
static void _callback_frame(
    DvzDeq* deq, DvzDeqProcBatchPosition pos, uint32_t item_count, DvzDeqItem* items,
    void* user_data)
{
    ASSERT(deq != NULL);
    log_trace("callback frame");

    DvzRunner* runner = (DvzRunner*)user_data;
    ASSERT(runner != NULL);

    DvzCanvasEventFrame* ev = NULL;
    for (uint32_t i = 0; i < item_count; i++)
    {
        ASSERT(items[i].type == DVZ_RUNNER_CANVAS_FRAME);
        ev = (DvzCanvasEventFrame*)items[i].item;
        ASSERT(ev != NULL);

        // We enqueue another FRAME event, but in the MAIN queue: this is the event the user
        // callbacks will subscribe to.
        _enqueue_canvas_frame(runner, ev->canvas, DVZ_RUNNER_DEQ_MAIN);

        // TODO: optim: if multiple FRAME events for 1 canvas, make sure we call it only once.
        // One frame for one canvas.
        _canvas_frame(runner, ev->canvas);
    }
}



static void _callback_transfers(DvzDeq* deq, void* item, void* user_data)
{
    DvzRunner* runner = (DvzRunner*)user_data;
    ASSERT(runner != NULL);

    DvzRenderer* renderer = runner->renderer;
    ASSERT(renderer != NULL);

    DvzCanvasEventFrame* ev = (DvzCanvasEventFrame*)item;
    ASSERT(item != NULL);

    DvzCanvas* canvas = ev->canvas;
    ASSERT(canvas != NULL);

    uint32_t img_idx = canvas->render.swapchain.img_idx;

    // DvzGpu* gpu = canvas->gpu;

    // DvzContainerIterator iter = dvz_container_iterator(&host->gpus);
    // while (iter.item != NULL)
    // {
    //     gpu = (DvzGpu*)iter.item;
    //     ASSERT(gpu != NULL);
    //     ASSERT(gpu->obj.type == DVZ_OBJECT_TYPE_GPU);
    //     if (!dvz_obj_is_created(&gpu->obj))
    //         break;


    ASSERT(renderer->ctx != NULL);

    // Process the pending data transfers (copies and dup transfers that require
    // synchronization and integration with the event loop).
    dvz_transfers_frame(&renderer->ctx->transfers, img_idx);

    // dvz_container_iter(&iter);
    // }
}



static void _callback_recreate(DvzDeq* deq, void* item, void* user_data)
{
    ASSERT(deq != NULL);
    log_debug("canvas recreate");

    DvzRunner* runner = (DvzRunner*)user_data;
    ASSERT(runner != NULL);

    DvzCanvasEvent* ev = (DvzCanvasEvent*)item;
    ASSERT(item != NULL);
    DvzCanvas* canvas = ev->canvas;

    // Recreate the canvas.
    dvz_canvas_recreate(canvas);

    // Enqueue a REFILL after the canvas recreation.
    _enqueue_to_refill(runner, canvas);
}



static void _callback_to_refill(DvzDeq* deq, void* item, void* user_data)
{
    ASSERT(deq != NULL);
    log_trace("callback to refill");

    DvzCanvasEvent* ev = (DvzCanvasEvent*)item;
    ASSERT(ev != NULL);
    ASSERT(ev->canvas != NULL);
    // _canvas_refill(ev->canvas);

    // Unblock all command buffers so that they are refilled one by one at the next frames.
    memset(ev->canvas->cmds.blocked, 0, sizeof(ev->canvas->cmds.blocked));

    // for (uint32_t i = 0; i < item_count; i++)
    // {
    //     ASSERT(items[i].type == DVZ_RUNNER_CANVAS_TO_REFILL);
    //     ev = item;
    //     ASSERT(ev != NULL);

    //     // TODO: optim: if multiple REFILL events for 1 canvas, make sure we call it only once.
    //     // One frame for one canvas.
    //     _canvas_refill(ev->canvas);
    // }
}



// If the command buffer is not blocked, perform the user REFILL.
static void _callback_refill_wrap(DvzDeq* deq, void* item, void* user_data)
{
    ASSERT(deq != NULL);
    log_trace("callback refill wrap");

    DvzRunner* runner = (DvzRunner*)user_data;
    ASSERT(runner != NULL);

    DvzCanvasEvent* ev = (DvzCanvasEvent*)item;
    ASSERT(ev != NULL);
    ASSERT(ev->canvas != NULL);
    DvzCanvas* canvas = ev->canvas;

    if (_should_refill(canvas))
    {
        // Default command buffers for the canvas.
        DvzCommands* cmds = &canvas->cmds;
        uint32_t img_idx = canvas->render.swapchain.img_idx;

        // HACK: here, we want to call the user callback to REFILL directly. We need to enqueue
        // first and dequeue immediately to be sure the callback is called and the command buffer
        // is filled. Better to have a special deq method to call the callback and bypassing the
        // queue altogether/
        _enqueue_refill(runner, canvas, cmds, img_idx, false);
        // This will call the user callback for REFILL, for the current swapchain image.
        dvz_deq_dequeue(&runner->deq, DVZ_RUNNER_DEQ_REFILL, true);

        _refill_done(canvas);
    }
}



static void _callback_upfill(DvzDeq* deq, void* item, void* user_data)
{
    ASSERT(deq != NULL);
    log_debug("callback upfill");

    DvzCanvasEventUpfill* ev = (DvzCanvasEventUpfill*)item;
    ASSERT(ev != NULL);

    DvzRunner* runner = (DvzRunner*)user_data;
    ASSERT(runner != NULL);

    ASSERT(ev->canvas != NULL);
    ASSERT(ev->canvas->gpu != NULL);
    ASSERT(ev->dat != NULL);
    ASSERT(ev->data != NULL);
    ASSERT(ev->size > 0);

    // Stop rendering.
    dvz_queue_wait(ev->canvas->gpu, DVZ_DEFAULT_QUEUE_RENDER);

    // Resize if needed.
    dvz_dat_resize(ev->dat, ev->size);

    // Upload the data and wait.
    dvz_dat_upload(ev->dat, ev->offset, ev->size, ev->data, true);

    // Enqueue to refill, which will trigger refill in the current frame (in the REFILL dequeue
    // just after the MAIN dequeue which called the current function).
    _enqueue_to_refill(runner, ev->canvas);
}



// backend-specific
static void _callback_present(DvzDeq* deq, void* item, void* user_data)
{
    ASSERT(deq != NULL);
    // frame submission for that canvas: submit cmd bufs, present swapchain

    DvzCanvasEvent* ev = (DvzCanvasEvent*)item;
    ASSERT(ev != NULL);
    DvzCanvas* canvas = ev->canvas;

    // Process only created canvas.
    if (!_canvas_check(canvas))
        return;
    // log_debug("present canvas");

    // Submit the command buffers and make the swapchain rendering.
    canvas_render(canvas);
}



static void _callback_request(DvzDeq* deq, void* item, void* user_data)
{
    ASSERT(deq != NULL);

    DvzRunner* runner = (DvzRunner*)user_data;
    ASSERT(runner != NULL);

    DvzRequest* req = (DvzRequest*)item;
    ASSERT(req != NULL);

    // DEBUG
    // dvz_request_print(req);

    // Canvas destruction: wait before destroying the canvas.
    if (req->action == DVZ_REQUEST_ACTION_DELETE && req->type == DVZ_REQUEST_OBJECT_CANVAS)
        _run_flush(runner);

    // Submit the request to the renderer.
    dvz_renderer_request(runner->renderer, *req);
}



#endif
