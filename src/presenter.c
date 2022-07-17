/*************************************************************************************************/
/*  Presenter                                                                                    */
/*************************************************************************************************/

#include "../include/datoviz/presenter.h"
#include "../include/datoviz/canvas.h"
#include "../include/datoviz/map.h"
#include "../include/datoviz/request.h"
#include "../include/datoviz/surface.h"
#include "../include/datoviz/vklite.h"
#include "canvas_utils.h"
#include "client_utils.h"
#include "vklite_utils.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

// This function is called when a CANVAS creation request is received. The renderer independently
// receives the request and creates the object, but the presenter needs to tell the client to
// create an associated window with a surface.
// NOTE: this function must be called AFTER the request has been processed by the renderer.
static void _canvas_request(DvzPresenter* prt, DvzRequest rq)
{
    ASSERT(prt != NULL);

    DvzClient* client = prt->client;
    ASSERT(client != NULL);

    DvzRenderer* rd = prt->rd;
    ASSERT(rd != NULL);

    DvzGpu* gpu = rd->gpu;
    ASSERT(gpu != NULL);

    DvzHost* host = gpu->host;
    ASSERT(host != NULL);

    switch (rq.action)
    {
    case DVZ_REQUEST_ACTION_CREATE:;

        // When the client receives a REQUEST event with a canvas creation command, it will *also*
        // create a window in the client with the same id and size. The canvas and window will be
        // linked together via a surface.

        // Retrieve the canvas that was just created by the renderer in _requester_callback().
        DvzCanvas* canvas = dvz_renderer_canvas(rd, rq.id);

        // TODO: canvas.screen_width/height because this is the window size, not the framebuffer
        // size
        uint32_t width = rq.content.canvas.width;
        uint32_t height = rq.content.canvas.height;

        // Create a client window.
        // NOTE: the window's id in the Client matches the canvas's id in the Renderer.
        DvzWindow* window = create_client_window(client, rq.id, width, height, 0);

        // Create a surface (requires the renderer's GPU).
        VkSurfaceKHR surface = dvz_window_surface(host, window);

        // Finally, associate the canvas with the created window surface.
        dvz_canvas_create(canvas, surface);

        // Refill function for the canvas.
        // dvz_canvas_refill(canvas, _fill_canvas, (void*)prt);

        break;
    default:
        break;
    }
}



static void _record_command(DvzRenderer* rd, DvzCanvas* canvas, uint32_t img_idx)
{
    ASSERT(rd != NULL);
    ASSERT(canvas != NULL);
    if (canvas->recorder != NULL)
    {
        dvz_cmd_reset(&canvas->cmds, img_idx);
        dvz_recorder_set(canvas->recorder, rd, &canvas->cmds, img_idx);
    }
    else
    {
        blank_commands(canvas, &canvas->cmds, img_idx, NULL);
    }
}



/*************************************************************************************************/
/*  Callbacks                                                                                    */
/*************************************************************************************************/

// This function is called when the Client receives a REQUESTS event. It will route the requests to
// the underlying renderer, and also create associated Client objects such as windows associated to
// canvases.
static void _requester_callback(DvzClient* client, DvzClientEvent ev, void* user_data)
{
    ASSERT(client != NULL);
    ASSERT(user_data != NULL);
    DvzPresenter* prt = (DvzPresenter*)user_data;

    DvzRenderer* rd = prt->rd;
    ASSERT(rd != NULL);

    ASSERT(ev.type == DVZ_CLIENT_EVENT_REQUESTS);

    // Get the array of requests.
    uint32_t count = ev.content.r.request_count;
    ASSERT(count > 0);

    DvzRequest* requests = (DvzRequest*)ev.content.r.requests;
    ASSERT(requests != NULL);

    // Submit the pending requests to the renderer.
    log_debug("renderer processes %d requests", count);

    // Go through all pending requests.
    for (uint32_t i = 0; i < count; i++)
    {
        // Process each request immediately in the renderer.
        dvz_renderer_request(rd, requests[i]);

        // CANVAS requests need special care, as the client may need to manage corresponding
        // windows.
        if (requests[i].type == DVZ_REQUEST_OBJECT_CANVAS)
        {
            _canvas_request(prt, requests[i]);
        }
        // Here, new canvases have been properly created with an underlying window and surface.
    }

    // Finally, we can FREE the requests pointer.
    FREE(requests);
}



static void _frame_callback(DvzClient* client, DvzClientEvent ev, void* user_data)
{
    ASSERT(client != NULL);
    ASSERT(user_data != NULL);
    DvzPresenter* prt = (DvzPresenter*)user_data;

    dvz_presenter_frame(prt, ev.window_id);
}



/*************************************************************************************************/
/*  Presenter                                                                                    */
/*************************************************************************************************/

DvzPresenter* dvz_presenter(DvzRenderer* rd, DvzClient* client)
{
    ASSERT(rd != NULL);
    ASSERT(client != NULL);

    DvzPresenter* prt = calloc(1, sizeof(DvzPresenter));
    ASSERT(prt != NULL);

    prt->rd = rd;
    prt->client = client;

    // Register a REQUESTS callback which submits pending requests to the renderer.
    dvz_client_callback(
        client, DVZ_CLIENT_EVENT_REQUESTS, DVZ_CLIENT_CALLBACK_SYNC, _requester_callback, prt);

    // Register a FRAME callback which calls dvz_presenter_frame().
    dvz_client_callback(
        client, DVZ_CLIENT_EVENT_FRAME, DVZ_CLIENT_CALLBACK_SYNC, _frame_callback, prt);

    return prt;
}



void dvz_presenter_frame(DvzPresenter* prt, DvzId window_id)
{
    ASSERT(prt != NULL);

    DvzClient* client = prt->client;
    ASSERT(client != NULL);

    DvzRenderer* rd = prt->rd;
    ASSERT(rd != NULL);

    DvzGpu* gpu = rd->gpu;
    ASSERT(gpu != NULL);

    DvzHost* host = gpu->host;
    ASSERT(host != NULL);

    DvzContext* ctx = rd->ctx;
    ASSERT(ctx != NULL);

    // Retrieve the window from its id.
    DvzWindow* window = id2window(client, window_id);
    ASSERT(window != NULL);

    // Retrieve the canvas from its id.
    DvzCanvas* canvas = dvz_renderer_canvas(rd, window_id);
    ASSERT(canvas != NULL);

    uint64_t frame_idx = client->frame_idx;
    log_trace("frame %d, window #%x", frame_idx, window_id);

    // Swapchain logic.

    DvzSwapchain* swapchain = &canvas->render.swapchain;
    DvzFramebuffers* framebuffers = &canvas->render.framebuffers;
    DvzRenderpass* renderpass = &gpu->renderpass;
    DvzFences* fences = &canvas->sync.fences_render_finished;
    DvzFences* fences_bak = &canvas->sync.fences_flight;
    DvzSemaphores* sem_img_available = &canvas->sync.sem_img_available;
    DvzSemaphores* sem_render_finished = &canvas->sync.sem_render_finished;
    DvzCommands* cmds = &canvas->cmds;
    DvzSubmit* submit = &canvas->render.submit;

    // Wait for fence.
    dvz_fences_wait(fences, canvas->cur_frame);

    // We acquire the next swapchain image.
    dvz_swapchain_acquire(swapchain, sem_img_available, canvas->cur_frame, NULL, 0);
    if (swapchain->obj.status == DVZ_OBJECT_STATUS_INVALID)
    {
        dvz_gpu_wait(gpu);
        return;
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

        // Ensure the canvas size is updated as well.
        canvas->width = width;
        canvas->height = height;

        // Need to recreate the depth image with the new size.
        dvz_images_size(&canvas->render.depth, (uvec3){width, height, 1});
        dvz_images_create(&canvas->render.depth);

        // Recreate the framebuffers with the new size.
        ASSERT(framebuffers->attachments[0]->shape[0] == width);
        ASSERT(framebuffers->attachments[0]->shape[1] == height);
        dvz_framebuffers_create(framebuffers, renderpass);

        // Emit a client Resize event.
        dvz_client_event(
            client, (DvzClientEvent){
                        .type = DVZ_CLIENT_EVENT_WINDOW_RESIZE,
                        .window_id = window_id,
                        .content.w.width = width,
                        .content.w.height = height});

        // Need to refill the command buffers.
        if (canvas->recorder)
            // Ensure we reset the refill flag to force reloading.
            dvz_recorder_need_refill(canvas->recorder);
        for (uint32_t i = 0; i < cmds->count; i++)
        {
            _record_command(rd, canvas, i);
        }
    }
    else
    {
        dvz_fences_copy(fences, canvas->cur_frame, fences_bak, swapchain->img_idx);

        // At every frame, we submit the command buffer, unless it was already submitted previously
        // (caching system built into the recorder).
        if (canvas->recorder && dvz_recorder_is_dirty(canvas->recorder, swapchain->img_idx))
        {
            _record_command(rd, canvas, swapchain->img_idx);
        }

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

    // HACK: improve this: img_idx depends on the canvas, but this function does not...
    // DUP transfers must be refactored.
    dvz_transfers_frame(&ctx->transfers, 0);

    // TODO:
    // need to go through the pending requests again in the requester (eg those raise in the RESIZE
    // callbacks)?

    // UPFILL: when there is a command refill + data uploads in the same batch, register
    // the cmd buf at the moment when the GPU-blocking upload really occurs
}



void dvz_presenter_submit(DvzPresenter* prt, DvzRequester* rqr)
{
    ASSERT(prt != NULL);
    ASSERT(rqr != NULL);
    ASSERT(prt->client != NULL);

    uint32_t count = 0;
    DvzRequest* requests = dvz_requester_flush(rqr, &count);
    // NOTE: the presenter will need to FREE the requests array.

    ASSERT(count > 0);
    ASSERT(requests != NULL);

    // Submit the requests to the client's event loop. Will be processed by _requester_callback(),
    // which will also free the requests array.
    dvz_client_event(
        prt->client, (DvzClientEvent){
                         .type = DVZ_CLIENT_EVENT_REQUESTS,
                         .content.r.request_count = count,
                         .content.r.requests = requests});
}



void dvz_presenter_destroy(DvzPresenter* prt)
{
    ASSERT(prt != NULL);
    FREE(prt);
}
