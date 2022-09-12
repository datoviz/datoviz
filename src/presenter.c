/*************************************************************************************************/
/*  Presenter                                                                                    */
/*************************************************************************************************/

#include "presenter.h"
#include "_list.h"
#include "_map.h"
#include "canvas.h"
#include "canvas_utils.h"
#include "client_utils.h"
#include "gui.h"
#include "request.h"
#include "surface.h"
#include "vklite.h"
#include "vklite_utils.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static void _create_canvas(DvzPresenter* prt, DvzRequest rq)
{
    ANN(prt);

    DvzClient* client = prt->client;
    ANN(client);

    DvzRenderer* rd = prt->rd;
    ANN(rd);

    DvzGpu* gpu = rd->gpu;
    ANN(gpu);

    DvzHost* host = gpu->host;
    ANN(host);

    // When the client receives a REQUEST event with a canvas creation command, it will *also*
    // create a window in the client with the same id and size. The canvas and window will be
    // linked together via a surface.

    // Retrieve the canvas that was just created by the renderer in _requester_callback().
    DvzCanvas* canvas = dvz_renderer_canvas(rd, rq.id);

    // Distinguish between canvas size and screen size.
    uint32_t screen_width = rq.content.canvas.screen_width;
    uint32_t screen_height = rq.content.canvas.screen_height;
    ASSERT(screen_width > 0);
    ASSERT(screen_height > 0);

    // Create a client window.
    // NOTE: the window's id in the Client matches the canvas's id in the Renderer.
    DvzWindow* window = create_client_window(client, rq.id, screen_width, screen_height, 0);

    // Once the window has been created, we can request the framebuffer size. It was set up
    // automatically when creating the window.
    canvas->width = window->framebuffer_width;
    canvas->height = window->framebuffer_height;

    // Create a surface (requires the renderer's GPU).
    DvzSurface surface = dvz_window_surface(host, window);

    // Finally, associate the canvas with the created window surface.

    // NOTE: This call does not in the renderer, because we need the surface which depends on the
    // client, and the renderer is agnostic wrt the client. Also, we need to know the framebuffer
    // size, which also requires the window (so depends on the client as well).
    dvz_canvas_create(canvas, surface);

    // HACK: keep track of the created surface so that we can destroy it when destroying the
    // presenter. An alternative may be to iterate through all canvases and destroy their surfaces,
    // but that requires a method to iterate over a map, which we don't have right now (would be
    // straightforward to do though, in _map.cpp).
    dvz_list_append(prt->surfaces, (DvzListItem){.p = &canvas->surface});

    // Create the canvas recorder.
    ASSERT(dvz_obj_is_created(&canvas->render.swapchain.obj));
    canvas->recorder = dvz_recorder(canvas->render.swapchain.img_count, 0);

    // Create the associated GUI window if requested.
    bool has_gui = (rq.flags & DVZ_CANVAS_FLAGS_IMGUI);
    if (has_gui)
    {
        ANN(prt->gui);

        // Create the GUI window.
        DvzGuiWindow* gui_window = dvz_gui_window(
            prt->gui, window, canvas->render.swapchain.images, DVZ_DEFAULT_QUEUE_RENDER);
        // NOTE: save the ID in the GUI window so that we can retrieve it in the GUI callback
        // helper.
        gui_window->obj.id = rq.id;

        // Associate it to the ID.
        dvz_map_add(prt->maps.guis, rq.id, 0, (void*)gui_window);
    }
}



static void _delete_canvas(DvzPresenter* prt, DvzId id)
{
    ANN(prt);

    DvzClient* client = prt->client;
    ANN(client);

    DvzRenderer* rd = prt->rd;
    ANN(rd);

    DvzGpu* gpu = rd->gpu;
    ANN(gpu);

    DvzHost* host = gpu->host;
    ANN(host);

    // Start canvas destruction.
    DvzCanvas* canvas = dvz_renderer_canvas(rd, id);

    // First, destroy the surface.
    // HACK: remove the surface from the list, as we won't have to destroy it when destroying
    // the presenter.
    // WARNING: we need the canvas object to be not destroyed yet as we use the pointer to its
    // surface to remove it from the list.
    dvz_list_remove_pointer(prt->surfaces, (void*)&canvas->surface);
    dvz_surface_destroy(host, canvas->surface);

    // Then, destroy the canvas.
    ANN(canvas);
    dvz_canvas_destroy(canvas);

    // Destroy the GUI window if it exists.
    DvzGuiWindow* gui_window = dvz_map_get(prt->maps.guis, id);
    if (gui_window != NULL)
        dvz_gui_window_destroy(gui_window);

    // Destroy the window.
    DvzWindow* window = id2window(client, id);
    ANN(window);
    dvz_map_remove(client->map, id);
    dvz_window_destroy(window);
}



/*************************************************************************************************/
/*  Request callbacks                                                                            */
/*************************************************************************************************/

// This function is called when a CANVAS creation request is received. The renderer independently
// receives the request and creates the object, but the presenter needs to tell the client to
// create an associated window with a surface.
// NOTE: this function must be called AFTER the request has been processed by the renderer.
static void _canvas_request(DvzPresenter* prt, DvzRequest rq)
{
    ANN(prt);

    DvzClient* client = prt->client;
    ANN(client);

    DvzRenderer* rd = prt->rd;
    ANN(rd);

    DvzGpu* gpu = rd->gpu;
    ANN(gpu);

    DvzHost* host = gpu->host;
    ANN(host);

    switch (rq.action)
    {

        // Create a canvas.
    case DVZ_REQUEST_ACTION_CREATE:;

        log_debug("process canvas creation request");
        _create_canvas(prt, rq);
        break;

        // Delete a canvas.
    case DVZ_REQUEST_ACTION_DELETE:;

        log_debug("process canvas deletion request");
        _delete_canvas(prt, rq.id);
        break;

    default:
        break;
    }
}



static void _record_command(DvzRenderer* rd, DvzCanvas* canvas, uint32_t img_idx)
{
    ANN(rd);
    ANN(canvas);
    ANN(canvas->recorder);
    if (canvas->recorder->count > 0)
    {
        log_debug("record the commands in the command buffer");
        dvz_cmd_reset(&canvas->cmds, img_idx);
        dvz_recorder_set(canvas->recorder, rd, &canvas->cmds, img_idx);
    }
    else
    {
        log_debug("record blank commands in the command buffer");
        blank_commands(
            canvas->render.renderpass, &canvas->render.framebuffers, &canvas->cmds, img_idx, NULL);
        dvz_recorder_set(canvas->recorder, rd, &canvas->cmds, img_idx);
    }
}



/*************************************************************************************************/
/*  Callbacks                                                                                    */
/*************************************************************************************************/

// This function is called when the Client receives a REQUESTS event. It will route the
// requests to the underlying renderer, and also create associated Client objects such as
// windows associated to canvases.
static void _requester_callback(DvzClient* client, DvzClientEvent ev)
{
    ANN(client);

    DvzPresenter* prt = (DvzPresenter*)ev.user_data;
    ANN(prt);

    DvzRenderer* rd = prt->rd;
    ANN(rd);

    ASSERT(ev.type == DVZ_CLIENT_EVENT_REQUESTS);

    // Get the array of requests.
    uint32_t count = ev.content.r.request_count;
    ASSERT(count > 0);

    DvzRequest* requests = (DvzRequest*)ev.content.r.requests;
    ANN(requests);

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



static void _frame_callback(DvzClient* client, DvzClientEvent ev)
{
    ANN(client);

    DvzPresenter* prt = (DvzPresenter*)ev.user_data;
    ANN(prt);

    dvz_presenter_frame(prt, ev.window_id);
}



static void
_gui_callback(DvzPresenter* prt, DvzGuiWindow* gui_window, DvzSubmit* submit, uint32_t img_idx)
{
    ANN(prt);
    ANN(gui_window);
    ANN(submit);

    // Begin recording the GUI command buffer.
    dvz_gui_window_begin(gui_window, img_idx);

    // Call the user-specified GUI callbacks.
    DvzGuiCallbackPayload* payload = NULL;
    uint32_t n = dvz_list_count(prt->callbacks);
    for (uint32_t i = 0; i < n; i++)
    {
        payload = (DvzGuiCallbackPayload*)dvz_list_get(prt->callbacks, i).p;
        // NOTE: only call the GUI callbacks registered for the requested window (using the
        // ID).
        if (payload->window_id == gui_window->obj.id)
        {
            payload->callback(gui_window, payload->user_data);
        }
    }

    // Stop recording the GUI command buffer.
    dvz_gui_window_end(gui_window, img_idx);

    // Add the command buffer to the Submit instance.
    dvz_submit_commands(submit, &gui_window->cmds);
}



/*************************************************************************************************/
/*  Presenter */
/*************************************************************************************************/

DvzPresenter* dvz_presenter(DvzRenderer* rd, DvzClient* client, int flags)
{
    ANN(rd);
    ANN(client);

    DvzPresenter* prt = calloc(1, sizeof(DvzPresenter));
    ANN(prt);

    prt->rd = rd;
    prt->client = client;
    prt->flags = flags;

    // Register a REQUESTS callback which submits pending requests to the renderer.
    dvz_client_callback(
        client, DVZ_CLIENT_EVENT_REQUESTS, DVZ_CLIENT_CALLBACK_SYNC, _requester_callback, prt);

    // Register a FRAME callback which calls dvz_presenter_frame().
    dvz_client_callback(
        client, DVZ_CLIENT_EVENT_FRAME, DVZ_CLIENT_CALLBACK_SYNC, _frame_callback, prt);

    // Create the GUI instance if needed.
    bool has_gui = (flags & DVZ_CANVAS_FLAGS_IMGUI);
    if (has_gui)
    {
        prt->gui = dvz_gui(rd->gpu, DVZ_DEFAULT_QUEUE_RENDER, DVZ_GUI_FLAGS_NONE);
    }

    // Mappings.
    prt->maps.guis = dvz_map();

    // List of GUI callbacks.
    prt->callbacks = dvz_list();

    // List of canvas surfaces created by the presenter, and that should be destroyed when the
    // presenter is destroyed.
    prt->surfaces = dvz_list();

    return prt;
}



void dvz_presenter_gui(
    DvzPresenter* prt, DvzId window_id, DvzGuiCallback callback, void* user_data)
{
    ANN(prt);
    ASSERT(window_id != 0);
    ANN(callback);

    log_debug("add GUI callback to window 0x%" PRIx64 "");
    DvzGuiCallbackPayload* payload =
        (DvzGuiCallbackPayload*)calloc(1, sizeof(DvzGuiCallbackPayload));
    payload->window_id = window_id;
    payload->callback = callback;
    payload->user_data = user_data;
    dvz_list_append(prt->callbacks, (DvzListItem){.p = (void*)payload});
}



void dvz_presenter_frame(DvzPresenter* prt, DvzId window_id)
{
    ANN(prt);

    DvzClient* client = prt->client;
    ANN(client);

    DvzRenderer* rd = prt->rd;
    ANN(rd);

    DvzGpu* gpu = rd->gpu;
    ANN(gpu);

    DvzHost* host = gpu->host;
    ANN(host);

    DvzContext* ctx = rd->ctx;
    ANN(ctx);

    // Retrieve the window from its id.
    DvzWindow* window = id2window(client, window_id);
    ANN(window);

    // Retrieve the canvas from its id.
    DvzCanvas* canvas = dvz_renderer_canvas(rd, window_id);
    ANN(canvas);

    // Retrieve the canvas' recorder.
    DvzRecorder* recorder = canvas->recorder;
    ANN(recorder);

    uint64_t frame_idx = client->frame_idx;
    log_trace("frame %d, window #%x", frame_idx, window_id);

    // Swapchain logic.

    DvzSwapchain* swapchain = &canvas->render.swapchain;
    DvzFences* fences = &canvas->sync.fences_render_finished;
    DvzFences* fences_bak = &canvas->sync.fences_flight;
    DvzSemaphores* sem_img_available = &canvas->sync.sem_img_available;
    DvzSemaphores* sem_render_finished = &canvas->sync.sem_render_finished;
    DvzCommands* cmds = &canvas->cmds;
    DvzSubmit* submit = &canvas->render.submit;
    DvzGuiWindow* gui_window = (DvzGuiWindow*)dvz_map_get(prt->maps.guis, window_id);

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

        // Recreate the canvas. The new framebuffer size will be stored in canvas->width/height.
        dvz_canvas_recreate(canvas);

        // Resuize the GUI window if it exists.
        if (gui_window != NULL)
        {
            dvz_gui_window_resize(gui_window, canvas->width, canvas->height);
        }

        // Emit a client Resize event.
        // NOTE: the width and height of the RESIZE event come from the window's
        // dvz_window_poll_size(), because this is the screen size, not the framebuffer size.
        dvz_client_event(
            client, (DvzClientEvent){
                        .type = DVZ_CLIENT_EVENT_WINDOW_RESIZE,
                        .window_id = window_id,

                        // Canvas size.
                        .content.w.framebuffer_width = canvas->width,
                        .content.w.framebuffer_height = canvas->height,

                        // Window size.
                        .content.w.screen_width = window->width,
                        .content.w.screen_height = window->height,
                    });

        // Need to refill the command buffers.
        // Ensure we reset the refill flag to force reloading.
        dvz_recorder_set_dirty(recorder);
        for (uint32_t i = 0; i < cmds->count; i++)
        {
            _record_command(rd, canvas, i);
        }
    }
    else
    {
        dvz_fences_copy(fences, canvas->cur_frame, fences_bak, swapchain->img_idx);

        // At every frame, we refill the command buffer, unless it was already refilled
        // previously (caching system built into the recorder).
        if (dvz_recorder_is_dirty(recorder, swapchain->img_idx))
        {
            _record_command(rd, canvas, swapchain->img_idx);
        }

        // Reset the Submit instance before adding the command buffers.
        dvz_submit_reset(submit);

        // First, we submit the cmds on that image
        dvz_submit_commands(submit, cmds);

        // Then, we submit the GUI command buffer.
        if (gui_window != NULL && dvz_list_count(prt->callbacks) > 0)
        {
            _gui_callback(prt, gui_window, submit, swapchain->img_idx);
        }

        // We send the submission.
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
    // need to go through the pending requests again in the requester (eg those raise in the
    // RESIZE callbacks)?

    // UPFILL: when there is a command refill + data uploads in the same batch, register
    // the cmd buf at the moment when the GPU-blocking upload really occurs
}



void dvz_presenter_submit(DvzPresenter* prt, DvzRequester* rqr)
{
    ANN(prt);
    ANN(rqr);
    ANN(prt->client);

    uint32_t count = 0;
    DvzRequest* requests = dvz_requester_flush(rqr, &count);
    // NOTE: the presenter will need to FREE the requests array.

    ASSERT(count > 0);
    ANN(requests);

    // Submit the requests to the client's event loop. Will be processed by
    // _requester_callback(), which will also free the requests array.
    dvz_client_event(
        prt->client, (DvzClientEvent){
                         .type = DVZ_CLIENT_EVENT_REQUESTS,
                         .content.r.request_count = count,
                         .content.r.requests = requests});
}



void dvz_presenter_destroy(DvzPresenter* prt)
{
    ANN(prt);
    ANN(prt->callbacks);

    // Go through all remaining surfaces to destroy them, as they were created by the
    // presenter, not by the renderer, so they won't be destroyed by the renderer destruction
    // code.
    DvzRenderer* rd = prt->rd;
    DvzList* surfaces = prt->surfaces;
    uint64_t n = dvz_list_count(surfaces);
    for (uint64_t i = 0; i < n; i++)
    {
        dvz_surface_destroy(rd->gpu->host, *((DvzSurface*)dvz_list_get(surfaces, i).p));
    }

    // Destroy the GuiWindow map.
    dvz_map_destroy(prt->maps.guis);

    // Destroy the GUI.
    if (prt->gui != NULL)
        dvz_gui_destroy(prt->gui);

    // Free the callback payloads.
    DvzGuiCallbackPayload* payload = NULL;
    for (uint32_t i = 0; i < prt->callbacks->count; i++)
    {
        payload = (DvzGuiCallbackPayload*)(dvz_list_get(prt->callbacks, i).p);
        ANN(payload);
        FREE(payload);
    }

    // Destroy the list of GUI callbacks.
    dvz_list_destroy(prt->callbacks);

    // Destroy the list of surfaces.
    dvz_list_destroy(prt->surfaces);

    FREE(prt);
}
