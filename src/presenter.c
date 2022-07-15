/*************************************************************************************************/
/*  Presenter                                                                                    */
/*************************************************************************************************/

#include "../include/datoviz/presenter.h"
#include "../include/datoviz/canvas.h"
#include "../include/datoviz/map.h"
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
static void _presenter_request(DvzPresenter* prt, DvzRequest rq)
{
    ASSERT(prt != NULL);

    DvzClient* client = prt->client;
    ASSERT(client != NULL);

    DvzRenderer* rnd = prt->rnd;
    ASSERT(rnd != NULL);

    DvzGpu* gpu = rnd->gpu;
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
        DvzCanvas* canvas = dvz_renderer_canvas(rnd, rq.id);

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

        break;
    default:
        break;
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

    ASSERT(ev.type == DVZ_CLIENT_EVENT_REQUESTS);

    DvzRequester* rqr = (DvzRequester*)ev.content.r.requests;
    ASSERT(rqr != NULL);

    DvzRenderer* rnd = prt->rnd;
    ASSERT(rnd != NULL);

    // Submit the pending requests to the renderer.
    log_debug("renderer processes %d requests", rqr->count);
    // CANVAS creation requests should be immediately processed here.
    dvz_renderer_requests(rnd, rqr->count, rqr->requests);

    // Some rendering requests need to be processed by the presenter/client, such as canvas
    // creation, deletion, etc.
    for (uint32_t i = 0; i < rqr->count; i++)
    {
        if (rqr->requests[i].type == DVZ_REQUEST_OBJECT_CANVAS)
        {
            _presenter_request(prt, rqr->requests[i]);
        }
    }
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

DvzPresenter* dvz_presenter(DvzRenderer* rnd)
{
    ASSERT(rnd != NULL);
    DvzPresenter* prt = calloc(1, sizeof(DvzPresenter));
    prt->rnd = rnd;
    return prt;
}



void dvz_presenter_frame(DvzPresenter* prt, DvzId window_id)
{
    ASSERT(prt != NULL);

    DvzClient* client = prt->client;
    ASSERT(client != NULL);

    DvzRenderer* rnd = prt->rnd;
    ASSERT(rnd != NULL);

    DvzWindow* window = id2window(client, window_id);
    ASSERT(window != NULL);

    uint64_t frame_idx = client->frame_idx;
    log_debug("frame %d, window #%x", frame_idx, window_id);

    // go through the pending requests in the requester
    // submit them to the renderer
    // special handling of canvas requests
    //     handle them also in the client
    //     canvas creation
    //         create a window with the client
    //         window <-> canvas pointer references

    // do the swapchain logic
    //     go through the client windows
    //         get the associated canvas in DvzWindow.canvas void* pointer
    //         acquire swapchain, recreate canvas if out of date
    //         if resized, issue RESIZE command to the renderer
    //         the user may have registered RESIZE callbacks in the client that will submit
    //         specific requests if window to close, issue DELETE canvas command to the renderer



    // DvzSwapchain* swapchain = &canvas->render.swapchain;
    // DvzFramebuffers* framebuffers = &canvas->render.framebuffers;
    // DvzRenderpass* renderpass = &canvas->render.renderpass;
    // DvzFences* fences = &canvas->sync.fences_render_finished;
    // DvzFences* fences_bak = &canvas->sync.fences_flight;
    // DvzSemaphores* sem_img_available = &canvas->sync.sem_img_available;
    // DvzSemaphores* sem_render_finished = &canvas->sync.sem_render_finished;
    // DvzCommands* cmds = &canvas->cmds;
    // DvzSubmit* submit = &canvas->render.submit;


    // // Wait for fence.
    // dvz_fences_wait(fences, canvas->cur_frame);

    // // We acquire the next swapchain image.
    // dvz_swapchain_acquire(swapchain, sem_img_available, canvas->cur_frame, NULL, 0);
    // if (swapchain->obj.status == DVZ_OBJECT_STATUS_INVALID)
    // {
    //     dvz_gpu_wait(gpu);
    //     break;
    // }
    // // Handle resizing.
    // else if (swapchain->obj.status == DVZ_OBJECT_STATUS_NEED_RECREATE)
    // {
    //     log_trace("recreating the swapchain");

    //     // Wait until the device is ready and the window fully resized.
    //     // Framebuffer new size.
    //     dvz_gpu_wait(gpu);
    //     dvz_window_poll_size(window);

    //     // Destroy swapchain resources.
    //     dvz_framebuffers_destroy(framebuffers);
    //     dvz_images_destroy(&canvas->render.depth);
    //     dvz_images_destroy(canvas->render.swapchain.images);

    //     // Recreate the swapchain. This will automatically set the swapchain->images new
    //     // size.
    //     dvz_swapchain_recreate(swapchain);
    //     // Find the new framebuffer size as determined by the swapchain recreation.
    //     uint32_t width = swapchain->images->shape[0];
    //     uint32_t height = swapchain->images->shape[1];

    //     // Need to recreate the depth image with the new size.
    //     dvz_images_size(&canvas->render.depth, (uvec3){width, height, 1});
    //     dvz_images_create(&canvas->render.depth);

    //     // Recreate the framebuffers with the new size.
    //     ASSERT(framebuffers->attachments[0]->shape[0] == width);
    //     ASSERT(framebuffers->attachments[0]->shape[1] == height);
    //     dvz_framebuffers_create(framebuffers, renderpass);

    //     // Need to refill the command buffers.
    //     for (uint32_t i = 0; i < cmds->count; i++)
    //     {
    //         dvz_cmd_reset(cmds, i);
    //         canvas->refill(canvas, cmds, i);
    //     }
    // }
    // else
    // {
    //     dvz_fences_copy(fences, canvas->cur_frame, fences_bak, swapchain->img_idx);

    //     // Reset the Submit instance before adding the command buffers.
    //     dvz_submit_reset(submit);

    //     // Then, we submit the cmds on that image
    //     dvz_submit_commands(submit, cmds);
    //     dvz_submit_wait_semaphores(
    //         submit, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, sem_img_available,
    //         canvas->cur_frame);
    //     // Once the render is finished, we signal another semaphore.
    //     dvz_submit_signal_semaphores(submit, sem_render_finished, canvas->cur_frame);
    //     dvz_submit_send(submit, swapchain->img_idx, fences, canvas->cur_frame);

    //     // Once the image is rendered, we present the swapchain image.
    //     dvz_swapchain_present(swapchain, 1, sem_render_finished, canvas->cur_frame);

    //     canvas->cur_frame = (canvas->cur_frame + 1) % DVZ_MAX_FRAMES_IN_FLIGHT;
    // }

    // // IMPORTANT: we need to wait for the present queue to be idle, otherwise the GPU hangs
    // // when waiting for fences (not sure why). The problem only arises when using different
    // // queues for command buffer submission and swapchain present.
    // dvz_queue_wait(gpu, DVZ_DEFAULT_QUEUE_PRESENT);



    // need to go through the pending requests again in the requester (eg those raise in the RESIZE
    // callbacks)
    // UPFILL: when there is a command refill + data uploads in the same batch, register
    // the cmd buf at the moment when the GPU-blocking upload really occurs
}



void dvz_presenter_client(DvzPresenter* prt, DvzClient* client)
{
    ASSERT(prt != NULL);
    ASSERT(client != NULL);

    prt->client = client;

    // Register a REQUESTS callback which submits pending requests to the renderer.
    dvz_client_callback(
        client, DVZ_CLIENT_EVENT_REQUESTS, DVZ_CLIENT_CALLBACK_SYNC, _requester_callback, prt);

    // Register a FRAME callback which calls dvz_presenter_frame().
    dvz_client_callback(
        client, DVZ_CLIENT_EVENT_FRAME, DVZ_CLIENT_CALLBACK_SYNC, _frame_callback, prt);
}



void dvz_presenter_destroy(DvzPresenter* prt)
{
    ASSERT(prt != NULL);
    FREE(prt);
}
