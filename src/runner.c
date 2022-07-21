/*************************************************************************************************/
/*  Runner                                                                                       */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "../include/datoviz/runner.h"
#include "../include/datoviz/canvas.h"
#include "../include/datoviz/map.h"
#include "../include/datoviz/vklite.h"
#include "canvas_utils.h"
#include "runner_utils.h"
#include "vklite_utils.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_RUNNER_DEFAULT_FRAME_COUNT 0

// Return codes for dvz_runner_frame()
// 0: the frame ran successfully
// -1: an error occurred, need to continue the loop as normally as possible
// 1: need to stop the runner
#define DVZ_RUNNER_FRAME_RETURN_OK    0
#define DVZ_RUNNER_FRAME_RETURN_ERROR -1
#define DVZ_RUNNER_FRAME_RETURN_STOP  1



/*************************************************************************************************/
/*  Runner utils                                                                                 */
/*************************************************************************************************/

static void _process_record_requests(DvzRenderer* rd, DvzCanvas* canvas, uint32_t img_idx)
{
    ASSERT(rd != NULL);
    ASSERT(canvas != NULL);


    // Blank canvas by default.
    if (rd->req_count == 0)
    {
        log_debug("default command buffer refill with blank canvas for image #%d", img_idx);
        blank_commands(canvas, &canvas->cmds, img_idx, canvas->refill_data);
        return;
    }

    // Otherwise, process all buffered commands.
    DvzRequest* rq = NULL;
    DvzPipe* pipe = NULL;
    for (uint32_t i = 0; i < rd->req_count; i++)
    {
        rq = &rd->reqs[i];
        ASSERT(rq != NULL);
        switch (rq->type)
        {

        case DVZ_REQUEST_OBJECT_BEGIN:
            dvz_cmd_reset(&canvas->cmds, img_idx);
            dvz_canvas_begin(canvas, &canvas->cmds, img_idx);
            break;

        case DVZ_REQUEST_OBJECT_VIEWPORT:
            dvz_canvas_viewport(
                canvas, &canvas->cmds, img_idx, //
                rq->content.record_viewport.offset, rq->content.record_viewport.shape);
            break;

        case DVZ_REQUEST_OBJECT_DRAW:

            pipe = (DvzPipe*)dvz_map_get(rd->map, rq->content.record_draw.graphics);
            ASSERT(pipe != NULL);

            dvz_pipe_draw(
                pipe, &canvas->cmds, img_idx, //
                rq->content.record_draw.first_vertex, rq->content.record_draw.vertex_count);
            break;

        case DVZ_REQUEST_OBJECT_END:
            dvz_canvas_end(canvas, &canvas->cmds, img_idx);
            break;

        default:
            log_error("unknown record request #%d with type %d", i, rq->type);
            break;
        }
    }
}



static void _default_refill(DvzDeq* deq, void* item, void* user_data)
{
    ASSERT(item != NULL);
    ASSERT(user_data != NULL);
    DvzCanvasEventRefill* ev = (DvzCanvasEventRefill*)item;

    DvzRunner* runner = (DvzRunner*)user_data;
    DvzRenderer* rd = runner->renderer;

    DvzCanvas* canvas = ev->canvas;
    ASSERT(canvas != NULL);

    // Process the buffered record requests.
    uint32_t img_idx = canvas->render.swapchain.img_idx;
    _process_record_requests(rd, canvas, img_idx);
}



static int _deq_size(DvzRunner* runner)
{
    ASSERT(runner != NULL);
    int size = 0;
    size += dvz_fifo_size(&runner->deq.queues[DVZ_RUNNER_DEQ_FRAME]);
    size += dvz_fifo_size(&runner->deq.queues[DVZ_RUNNER_DEQ_MAIN]);
    size += dvz_fifo_size(&runner->deq.queues[DVZ_RUNNER_DEQ_REFILL]);
    return size;
}



/*************************************************************************************************/
/*  Runner                                                                                       */
/*************************************************************************************************/

DvzRunner* dvz_runner(DvzRenderer* renderer)
{
    ASSERT(renderer != NULL);

    DvzRunner* runner = calloc(1, sizeof(DvzRunner)); // will be FREE-ed by dvz_runner_destroy();
    runner->renderer = renderer;

    // Requester.
    runner->requester = calloc(1, sizeof(DvzRequester));
    *runner->requester = dvz_requester();

    // Deq with 4 queues: FRAME, MAIN, REFILL, PRESENT
    runner->deq = dvz_deq(4);
    dvz_deq_proc(&runner->deq, 0, 1, (uint32_t[]){DVZ_RUNNER_DEQ_FRAME});
    dvz_deq_proc(&runner->deq, 1, 1, (uint32_t[]){DVZ_RUNNER_DEQ_MAIN});
    dvz_deq_proc(&runner->deq, 2, 1, (uint32_t[]){DVZ_RUNNER_DEQ_REFILL});
    dvz_deq_proc(&runner->deq, 3, 1, (uint32_t[]){DVZ_RUNNER_DEQ_PRESENT});

    // FRAME queue.
    dvz_deq_proc_batch_callback(
        &runner->deq, DVZ_RUNNER_DEQ_FRAME, (int)DVZ_RUNNER_CANVAS_FRAME, _callback_frame, runner);


    // MAIN callbacks.

    // Recreate.
    dvz_deq_callback(
        &runner->deq, DVZ_RUNNER_DEQ_MAIN, (int)DVZ_RUNNER_CANVAS_RECREATE, _callback_recreate,
        runner);

    // Upfill.
    dvz_deq_callback(
        &runner->deq, DVZ_RUNNER_DEQ_MAIN, (int)DVZ_RUNNER_CANVAS_UPFILL, _callback_upfill,
        runner);

    // Call dvz_transfers_frame() in the main thread, at every frame, with the current canvas
    // swapchain image index.
    dvz_deq_callback(
        &runner->deq, DVZ_RUNNER_DEQ_MAIN, (int)DVZ_RUNNER_CANVAS_FRAME, _callback_transfers,
        runner);


    // Process rendering requests.
    dvz_deq_callback(
        &runner->deq, DVZ_RUNNER_DEQ_MAIN, (int)DVZ_RUNNER_REQUEST, _callback_request, runner);


    // REFILL queue.
    dvz_deq_callback(
        &runner->deq, DVZ_RUNNER_DEQ_REFILL, (int)DVZ_RUNNER_CANVAS_TO_REFILL, _callback_to_refill,
        runner);

    // REFILL_WRAP, that calls the REFILL callback if the current cmd buf is not blocked.
    dvz_deq_callback(
        &runner->deq, DVZ_RUNNER_DEQ_REFILL, (int)DVZ_RUNNER_CANVAS_REFILL_WRAP, //
        _callback_refill_wrap, runner);

    // Default refill callback.
    // NOTE: this is a default callback: it will be discarded if the user registers other command
    // buffer refill callbacks.
    dvz_deq_callback_default(
        &runner->deq, DVZ_RUNNER_DEQ_REFILL, (int)DVZ_RUNNER_CANVAS_REFILL, _default_refill,
        runner);


    // PRESENT queue.
    // Present callbacks.
    dvz_deq_callback(
        &runner->deq, DVZ_RUNNER_DEQ_PRESENT, (int)DVZ_RUNNER_CANVAS_PRESENT, _callback_present,
        runner);

    return runner;
}



/*************************************************************************************************/
/*  Runner event loop                                                                            */
/*************************************************************************************************/

// Run one frame for all active canvases, process all MAIN events, and perform all pending data
// copies.
int dvz_runner_frame(DvzRunner* runner)
{
    ASSERT(runner != NULL);
    ASSERT(runner->renderer != NULL);
    ASSERT(runner->renderer->gpu != NULL);
    ASSERT(runner->renderer->gpu->host != NULL);

    log_trace("frame #%06d", runner->global_frame_idx);

    // Go through all canvases to find out which are active, and enqueue a FRAME event for them.
    uint32_t n_canvas_running = _enqueue_frames(runner);

    // Dequeue all items until all queues are empty (depth first dequeue)
    //
    // NOTES: This is the call when most of the logic happens!
    while (_deq_size(runner) > 0)
    {
        // First, this call may dequeue a FRAME item, for which the callbacks will be called
        // immediately. The FRAME callbacks may enqueue REFILL items (third queue) or
        // ADD/REMOVE/VISIBLE/ACTIVE items (second queue).
        // They may also enqueue TRANSFER items, to be processed directly in the background
        // transfer thread. However, COPY transfers may be enqueued, to be handled separately later
        // in the run_frame().
        log_trace("dequeue batch frame");
        dvz_deq_dequeue_batch(&runner->deq, DVZ_RUNNER_DEQ_FRAME);

        // Then, dequeue MAIN items. The ADD/VISIBLE/ACTIVE/RESIZE callbacks may be called.
        // NOTE: pending data transfers (copies and dup transfers) happen here, in a FRAME callback
        // in the MAIN queue (main thread).
        log_trace("dequeue batch main");
        dvz_deq_dequeue_batch(&runner->deq, DVZ_RUNNER_DEQ_MAIN);

        // Refill canvases if needed.
        log_trace("dequeue batch refill");
        dvz_deq_dequeue_batch(&runner->deq, DVZ_RUNNER_DEQ_REFILL);
    }

    // Swapchain presentation.
    log_trace("dequeue batch present");
    dvz_deq_dequeue_batch(&runner->deq, DVZ_RUNNER_DEQ_PRESENT);

    _gpu_sync_hack(runner->renderer->gpu->host);

    // If no canvas is running, stop the event loop.
    if (n_canvas_running == 0)
        return DVZ_RUNNER_FRAME_RETURN_STOP;

    return 0;
}



/*************************************************************************************************/
/*  Runner functions                                                                             */
/*************************************************************************************************/

void dvz_runner_request(DvzRunner* runner, DvzRequest request)
{
    ASSERT(runner != NULL);

    DvzRequest* req = (DvzRequest*)calloc(1, sizeof(DvzRequest));
    *req = request;
    dvz_deq_enqueue(&runner->deq, DVZ_RUNNER_DEQ_MAIN, (int)DVZ_RUNNER_REQUEST, req);
}



void dvz_runner_requests(DvzRunner* runner, uint32_t count, DvzRequest* requests)
{
    ASSERT(runner != NULL);
    if (count > 0)
        ASSERT(requests != NULL);

    // Submit the recording requests.
    for (uint32_t i = 0; i < count; i++)
    {
        dvz_runner_request(runner, requests[i]);
    }
}



void dvz_runner_requester(DvzRunner* runner, DvzRequester* requester)
{
    ASSERT(runner != NULL);
    dvz_runner_requests(runner, requester->count, requester->requests);
}



int dvz_runner_loop(DvzRunner* runner, uint64_t frame_count)
{
    ASSERT(runner != NULL);

    int ret = 0, ret_prev = 0;
    // runner->state = DVZ_RUNNER_STATE_RUNNING;

    log_debug("runner loop with %d frames", frame_count);

    // NOTE: there is the global frame index for the event loop, but every frame has its own local
    // frame index too.
    for (                                                           //
        runner->global_frame_idx = 0;                               //
        frame_count == 0 || runner->global_frame_idx < frame_count; //
        runner->global_frame_idx++)                                 //
    {

        log_trace("event loop, global frame #%d", runner->global_frame_idx);
        ret = dvz_runner_frame(runner);

        // Stop the event loop if the return code of dvz_runner_frame() requires it.
        // HACK: requires the return code to be STOP for 2 consecutive frames, otherwise
        // it is impossible to bootstrap a canvas when running the loop for the first time.
        if (ret == DVZ_RUNNER_FRAME_RETURN_STOP && ret_prev == DVZ_RUNNER_FRAME_RETURN_STOP)
        {
            // log_debug("end event loop");
            break;
        }

        ret_prev = ret;
    }
    log_debug("end event loop after %d frames", runner->global_frame_idx);
    // runner->state = DVZ_RUNNER_STATE_PAUSED;

    // Wait.
    _run_flush(runner);

    return 0;
}



void dvz_runner_destroy(DvzRunner* runner)
{
    ASSERT(runner != NULL);
    ASSERT(runner->renderer != NULL);
    ASSERT(runner->renderer->workspace != NULL);

    log_debug("destroy runner instance");

    // Wait.
    _run_flush(runner);

    // Block the input of all canvases.
    {
        DvzContainerIterator iterator =
            dvz_container_iterator(&runner->renderer->workspace->canvases);
        DvzCanvas* canvas = NULL;
        while (iterator.item != NULL)
        {
            canvas = iterator.item;
            ASSERT(canvas != NULL);
            if (!dvz_obj_is_created(&canvas->obj))
                break;

            dvz_input_block(&canvas->input, true);
            dvz_container_iter(&iterator);
        }
    }

    dvz_deq_destroy(&runner->deq);

    dvz_requester_destroy(runner->requester);
    FREE(runner->requester);

    FREE(runner);
}
