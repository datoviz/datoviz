#include "../include/datoviz/runner.h"
#include "../include/datoviz/canvas.h"
#include "../include/datoviz/vklite.h"
#include "canvas_utils.h"
#include "runner_utils.h"
#include "vklite_utils.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_RUN_DEFAULT_FRAME_COUNT 0

// Return codes for dvz_runner_frame()
// 0: the frame ran successfully
// -1: an error occurred, need to continue the loop as normally as possible
// 1: need to stop the runner
#define DVZ_RUN_FRAME_RETURN_OK    0
#define DVZ_RUN_FRAME_RETURN_ERROR -1
#define DVZ_RUN_FRAME_RETURN_STOP  1



/*************************************************************************************************/
/*  Run creation                                                                                 */
/*************************************************************************************************/

static void _default_refill(DvzDeq* deq, void* item, void* user_data)
{
    ASSERT(item != NULL);
    ASSERT(user_data != NULL);
    DvzCanvasEventRefill* ev = (DvzCanvasEventRefill*)item;

    DvzCanvas* canvas = ev->canvas;
    ASSERT(canvas != NULL);

    // Blank canvas by default.
    uint32_t img_idx = canvas->render.swapchain.img_idx;
    log_debug("default command buffer refill with blank canvas for image #%d", img_idx);
    DvzCommands* cmds = &canvas->cmds;
    blank_commands(canvas, cmds, img_idx);
}

DvzRunner* dvz_runner(DvzHost* host)
{
    ASSERT(host != NULL); //

    DvzRunner* runner = calloc(1, sizeof(DvzRunner)); // will be FREE-ed by dvz_runner_destroy();
    runner->host = host;

    // Deq with 4 queues: FRAME, MAIN, REFILL, PRESENT
    runner->deq = dvz_deq(4);
    dvz_deq_proc(&runner->deq, 0, 1, (uint32_t[]){DVZ_RUN_DEQ_FRAME});
    dvz_deq_proc(&runner->deq, 1, 1, (uint32_t[]){DVZ_RUN_DEQ_MAIN});
    dvz_deq_proc(&runner->deq, 2, 1, (uint32_t[]){DVZ_RUN_DEQ_REFILL});
    dvz_deq_proc(&runner->deq, 3, 1, (uint32_t[]){DVZ_RUN_DEQ_PRESENT});

    // FRAME queue.

    dvz_deq_proc_batch_callback(
        &runner->deq, DVZ_RUN_DEQ_FRAME, (int)DVZ_RUN_CANVAS_FRAME, _callback_frame, runner);



    // MAIN callbacks.

    // New canvas.
    dvz_deq_callback(
        &runner->deq, DVZ_RUN_DEQ_MAIN, (int)DVZ_RUN_CANVAS_NEW, _callback_new, runner);

    // Delete canvas.
    dvz_deq_callback(
        &runner->deq, DVZ_RUN_DEQ_MAIN, (int)DVZ_RUN_CANVAS_DELETE, _callback_delete, runner);

    // Clear color.
    dvz_deq_callback(
        &runner->deq, DVZ_RUN_DEQ_MAIN, (int)DVZ_RUN_CANVAS_CLEAR_COLOR, _callback_clear_color,
        runner);

    // Recreate.
    dvz_deq_callback(
        &runner->deq, DVZ_RUN_DEQ_MAIN, (int)DVZ_RUN_CANVAS_RECREATE, _callback_recreate, runner);

    // Upfill.
    dvz_deq_callback(
        &runner->deq, DVZ_RUN_DEQ_MAIN, (int)DVZ_RUN_CANVAS_UPFILL, _callback_upfill, runner);

    // Call dvz_transfers_frame() in the main thread, at every frame, with the current canvas
    // swapchain image index.
    dvz_deq_callback(
        &runner->deq, DVZ_RUN_DEQ_MAIN, (int)DVZ_RUN_CANVAS_FRAME, _callback_transfers, runner);



    // REFILL queue.

    dvz_deq_callback(
        &runner->deq, DVZ_RUN_DEQ_REFILL, (int)DVZ_RUN_CANVAS_TO_REFILL, _callback_to_refill,
        runner);

    // REFILL_WRAP, that calls the REFILL callback if the current cmd buf is not blocked.
    dvz_deq_callback(
        &runner->deq, DVZ_RUN_DEQ_REFILL, (int)DVZ_RUN_CANVAS_REFILL_WRAP, //
        _callback_refill_wrap, runner);

    // Default refill callback.
    // NOTE: this is a default callback: it will be discarded if the user registers other command
    // buffer refill callbacks.
    dvz_deq_callback_default(
        &runner->deq, DVZ_RUN_DEQ_REFILL, (int)DVZ_RUN_CANVAS_REFILL, _default_refill, runner);



    // PRESENT queue.

    // Present callbacks.
    dvz_deq_callback(
        &runner->deq, DVZ_RUN_DEQ_PRESENT, (int)DVZ_RUN_CANVAS_PRESENT, _callback_present, runner);

    runner->state = DVZ_RUN_STATE_PAUSED;

    return runner;
}



/*************************************************************************************************/
/*  Run event loop                                                                               */
/*************************************************************************************************/

static int _deq_size(DvzRunner* runner)
{
    ASSERT(runner != NULL);
    int size = 0;
    size += dvz_fifo_size(&runner->deq.queues[DVZ_RUN_DEQ_FRAME]);
    size += dvz_fifo_size(&runner->deq.queues[DVZ_RUN_DEQ_MAIN]);
    size += dvz_fifo_size(&runner->deq.queues[DVZ_RUN_DEQ_REFILL]);
    return size;
}

// Run one frame for all active canvases, process all MAIN events, and perform all pending data
// copies.
int dvz_runner_frame(DvzRunner* runner)
{
    ASSERT(runner != NULL);

    DvzHost* host = runner->host;
    ASSERT(host != NULL);

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
        dvz_deq_dequeue_batch(&runner->deq, DVZ_RUN_DEQ_FRAME);

        // Then, dequeue MAIN items. The ADD/VISIBLE/ACTIVE/RESIZE callbacks may be called.
        // NOTE: pending data transfers (copies and dup transfers) happen here, in a FRAME callback
        // in the MAIN queue (main thread).
        log_trace("dequeue batch main");
        dvz_deq_dequeue_batch(&runner->deq, DVZ_RUN_DEQ_MAIN);

        // Refill canvases if needed.
        log_trace("dequeue batch refill");
        dvz_deq_dequeue_batch(&runner->deq, DVZ_RUN_DEQ_REFILL);
    }

    // Swapchain presentation.
    log_trace("dequeue batch present");
    dvz_deq_dequeue_batch(&runner->deq, DVZ_RUN_DEQ_PRESENT);

    _gpu_sync_hack(host);

    // DEBUG
    // dvz_sleep(100);

    // If no canvas is running, stop the event loop.
    if (n_canvas_running == 0)
        return DVZ_RUN_FRAME_RETURN_STOP;

    return 0;
}



/*************************************************************************************************/
/*  Dat upfill                                                                                   */
/*************************************************************************************************/

// void dvz_dat_upfill(
//     DvzCanvas* canvas, DvzDat* dat, VkDeviceSize offset, VkDeviceSize size, void* data)
// {
//     ASSERT(canvas != NULL);
//     ASSERT(canvas->gpu != NULL);
//     ASSERT(dat != NULL);
//     ASSERT(data != NULL);
//     ASSERT(size > 0);

//     _enqueue_upfill(canvas->host->runner, canvas, dat, offset, size, data);
// }



/*************************************************************************************************/
/*  Run functions                                                                                */
/*************************************************************************************************/

int dvz_runner_loop(DvzRunner* runner, uint64_t frame_count)
{
    ASSERT(runner != NULL);

    int ret = 0;
    uint64_t n = frame_count > 0 ? frame_count : UINT64_MAX;

    runner->state = DVZ_RUN_STATE_RUNNING;

    log_debug("runner loop with %d frames", frame_count);

    // NOTE: there is the global frame index for the event loop, but every frame has its own local
    // frame index too.
    for (runner->global_frame_idx = 0; runner->global_frame_idx < n; runner->global_frame_idx++)
    {
        log_trace("event loop, global frame #%d", runner->global_frame_idx);
        ret = dvz_runner_frame(runner);

        // Stop the event loop if the return code of dvz_runner_frame() requires it.
        if (ret == DVZ_RUN_FRAME_RETURN_STOP)
        {
            // log_debug("end event loop");
            break;
        }
    }
    log_debug("end event loop after %d frames", runner->global_frame_idx);
    runner->state = DVZ_RUN_STATE_PAUSED;

    // Wait.
    _run_flush(runner);

    return 0;
}



// void dvz_runner_setup(DvzRunner* runner, uint64_t frame_count, bool offscreen, char* filepath)
// {
//     ASSERT(runner != NULL);

//     DvzAutorun* autorun = &runner->autorun;
//     ASSERT(autorun != NULL);

//     autorun->filepath = filepath;
//     autorun->offscreen = offscreen;
//     autorun->frame_count = frame_count;

//     autorun->enable = _autorun_is_set(autorun);
// }



// void dvz_runner_setupenv(DvzRunner* runner)
// {
//     ASSERT(runner != NULL);

//     DvzAutorun* autorun = &runner->autorun;
//     ASSERT(autorun != NULL);

//     char* s = NULL;

//     // Offscreen?
//     s = getenv("DVZ_RUN_OFFSCREEN");
//     if (s)
//         autorun->offscreen = true;

//     // Number of frames.
//     s = getenv("DVZ_RUN_FRAMES");
//     if (s)
//         autorun->frame_count = strtoull(s, NULL, 10);

//     // Screenshot and video.
//     s = getenv("DVZ_RUN_SAVE");
//     if (s)
//         strncpy(autorun->filepath, s, DVZ_PATH_MAX_LEN);

//     // Enable the autorun if and only if at least one of the autorun fields is not blank.
//     autorun->enable = _autorun_is_set(autorun);
// }



// int dvz_runner_auto(DvzRunner* runner)
// {
//     ASSERT(runner != NULL);

//     dvz_runner_setupenv(runner);
//     if (runner->autorun.enable)
//     {
//         _autorun_launch(runner);
//     }
//     else
//     {
//         dvz_runner_loop(runner, DVZ_RUN_DEFAULT_FRAME_COUNT);
//     }

//     return 0;
// }



void dvz_runner_destroy(DvzRunner* runner)
{
    ASSERT(runner != NULL);
    ASSERT(runner->renderer != NULL);
    ASSERT(runner->renderer->workspace != NULL);

    // DvzHost* host = runner->host;
    // ASSERT(host != NULL);
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

    FREE(runner);
    // host->runner = NULL;
}
