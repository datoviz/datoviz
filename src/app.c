/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  App                                                                                          */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "app.h"
#include "board.h"
#include "client.h"
#include "datoviz.h"
#include "datoviz_protocol.h"
#include "env_utils.h"
#include "fileio.h"
#include "gui.h"
#include "host.h"
#include "input.h"
#include "presenter.h"
#include "render_utils.h"
#include "renderer.h"
#include "timer.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define BACKEND DVZ_BACKEND_GLFW



/*************************************************************************************************/
/*  Util functions                                                                               */
/*************************************************************************************************/

static void _on_frame(DvzApp* app, DvzId window_id, DvzFrameEvent ev)
{
    ANN(app);

    // The timer callbacks are called here.
    dvz_timer_tick(app->timer, ev.time);
}



/*************************************************************************************************/
/*  Callback utils                                                                               */
/*************************************************************************************************/

// Generic function pointer.
typedef void (*function_pointer)(void);

// Callback payload to convert from client callback to app callback.
typedef struct Payload Payload;
struct Payload
{
    DvzClientEventType et;
    DvzApp* app;
    function_pointer callback;
    void* user_data;
};

static Payload*
_make_payload(DvzClientEventType et, DvzApp* app, function_pointer callback, void* user_data)
{
    Payload* payload = (Payload*)calloc(1, sizeof(Payload));
    payload->et = et;
    payload->app = app;
    payload->callback = callback;
    payload->user_data = user_data;

    // NOTE: keep track of the heap-allocated payload so that the app destructor can free it at the
    // end.
    dvz_list_append(app->payloads, (DvzListItem){.p = (void*)payload});

    return payload;
}


// Conversion from client callback to app callback.
static void _client_callback(DvzClient* client, DvzClientEvent ev)
{
    ANN(client);
    ANN(ev.user_data);

    Payload payload = *(Payload*)ev.user_data;
    // FREE(ev.user_data);

    if (payload.et != ev.type)
        return;

    // NOTE: detect when callbacks are called while the app is being stopped.
    // This prevents assertion crashes in callbacks upon app closing.
    if (dvz_atomic_get(client->to_stop) == 1)
    {
        log_debug("prevent client callback from being called while the app is stopping");
        return;
    }

    DvzApp* app = payload.app;
    ANN(app);

    function_pointer callback = payload.callback;
    ANN(callback);

    DvzId window_id = ev.window_id;
    // NOTE: the window_id should be set except for timer events (global to all windows).
    if (ev.type != DVZ_CLIENT_EVENT_TIMER)
        ASSERT(window_id != DVZ_ID_NONE);

    // Mouse callback.
    if (ev.type == DVZ_CLIENT_EVENT_MOUSE)
    {
        DvzAppMouseCallback cb = (DvzAppMouseCallback)callback;
        // Pass the user_data to the mouse event.
        ev.content.m.user_data = payload.user_data;
        // Call the mouse callback.
        cb(app, window_id, ev.content.m);
    }

    // Keyboard callback.
    if (ev.type == DVZ_CLIENT_EVENT_KEYBOARD)
    {
        DvzAppKeyboardCallback cb = (DvzAppKeyboardCallback)callback;
        // Pass the user_data to the keyboard event.
        ev.content.k.user_data = payload.user_data;
        // Call the keyboard callback.
        cb(app, window_id, &ev.content.k);
    }

    // Resize callback.
    if (ev.type == DVZ_CLIENT_EVENT_WINDOW_RESIZE)
    {
        DvzAppResizeCallback cb = (DvzAppResizeCallback)callback;
        // Pass the user_data to the resize event.
        ev.content.w.user_data = payload.user_data;
        // Call the keyboard callback.
        cb(app, window_id, ev.content.w);
    }

    // Frame callback.
    if (ev.type == DVZ_CLIENT_EVENT_FRAME)
    {
        DvzAppFrameCallback cb = (DvzAppFrameCallback)callback;
        // Pass the user_data to the frame event.
        ev.content.f.user_data = payload.user_data;
        // Call the frame callback.
        cb(app, window_id, ev.content.f);
    }

    // Timer callback.
    if (ev.type == DVZ_CLIENT_EVENT_TIMER)
    {
        DvzAppTimerCallback cb = (DvzAppTimerCallback)callback;
        // Pass the user_data to the timer event.
        ev.content.t.user_data = payload.user_data;
        // Call the timer callback.
        cb(app, window_id, ev.content.t);
    }
}



/*************************************************************************************************/
/*  App functions                                                                                */
/*************************************************************************************************/

DvzApp* dvz_app(int flags)
{
    // Set number of threads from DVZ_NUM_THREADS env variable.
    dvz_threads_default();

    DvzApp* app = (DvzApp*)calloc(1, sizeof(DvzApp));

    DvzBackend backend = BACKEND;
    bool offscreen = (flags & DVZ_APP_FLAGS_OFFSCREEN) != 0;
    // NOTE: this call can modify offscreen (force set to true) if DVZ_CAPTURE_PNG is set
    char* capture = capture_png(&offscreen);

    if (offscreen)
    {
        backend = DVZ_BACKEND_OFFSCREEN;
    }
    app->host = dvz_host(backend);
    ANN(app->host);
    dvz_host_create(app->host);

    app->gpu = make_gpu(app->host);
    ANN(app->gpu);

    app->rd = dvz_renderer(app->gpu, flags);
    ANN(app->rd);

    if (!offscreen)
    {
        app->client = dvz_client(backend);
        ANN(app->client);

        app->prt = dvz_presenter(app->rd, app->client, DVZ_CANVAS_FLAGS_IMGUI);
        ANN(app->prt);
    }
    else
    {
        app->offscreen_gui = dvz_gui(app->gpu, DVZ_DEFAULT_QUEUE_RENDER, DVZ_GUI_FLAGS_OFFSCREEN);
        app->offscreen_guis = dvz_map();
        // app->offscreen_gui_window = dvz_gui_offscreen(app->offscreen_gui, canvas.images, 0);
    }

    app->batch = dvz_batch();
    ANN(app->batch);
    app->batch->flags = flags; // Pass the app flags to the batch flags.

    app->timer = dvz_timer();
    ANN(app->timer);

    // List of callback payloads we need to allocate on the heap and to destroy at the end.
    app->payloads = dvz_list();

    // Send the pending requests to the presenter at every frame.
    dvz_app_on_frame(app, _on_frame, app);

    return app;
}



DvzBatch* dvz_app_batch(DvzApp* app)
{
    ANN(app);
    return app->batch;
}



void dvz_app_frame(DvzApp* app)
{
    ANN(app);
    if (app->client)
        dvz_client_frame(app->client);
}



void dvz_app_on_frame(DvzApp* app, DvzAppFrameCallback on_frame, void* user_data)
{
    ANN(app);
    if (!app->client)
        return;
    Payload* payload =
        _make_payload(DVZ_CLIENT_EVENT_FRAME, app, (function_pointer)on_frame, user_data);
    dvz_client_callback(
        app->client, DVZ_CLIENT_EVENT_FRAME, DVZ_CLIENT_CALLBACK_SYNC, //
        _client_callback, payload);
}



void dvz_app_on_mouse(DvzApp* app, DvzAppMouseCallback on_mouse, void* user_data)
{
    ANN(app);
    if (!app->client)
        return;
    Payload* payload =
        _make_payload(DVZ_CLIENT_EVENT_MOUSE, app, (function_pointer)on_mouse, user_data);
    dvz_client_callback(
        app->client, DVZ_CLIENT_EVENT_MOUSE, DVZ_CLIENT_CALLBACK_SYNC, //
        _client_callback, payload);
}



void dvz_app_on_keyboard(DvzApp* app, DvzAppKeyboardCallback on_keyboard, void* user_data)
{
    ANN(app);
    if (!app->client)
        return;
    Payload* payload =
        _make_payload(DVZ_CLIENT_EVENT_KEYBOARD, app, (function_pointer)on_keyboard, user_data);
    dvz_client_callback(
        app->client, DVZ_CLIENT_EVENT_KEYBOARD, DVZ_CLIENT_CALLBACK_SYNC, //
        _client_callback, payload);
}



void dvz_app_on_resize(DvzApp* app, DvzAppResizeCallback on_resize, void* user_data)
{
    ANN(app);
    if (!app->client)
        return;
    Payload* payload =
        _make_payload(DVZ_CLIENT_EVENT_WINDOW_RESIZE, app, (function_pointer)on_resize, user_data);
    dvz_client_callback(
        app->client, DVZ_CLIENT_EVENT_WINDOW_RESIZE, DVZ_CLIENT_CALLBACK_SYNC, //
        _client_callback, payload);
}



void dvz_app_mouse(DvzApp* app, DvzId canvas_id, double* x, double* y, DvzMouseButton* button)
{
    ANN(app);
    DvzWindow* window = dvz_client_window(app->client, canvas_id);
    if (window == NULL)
    {
        log_error("canvas #%" PRIx64 " does not exist");
        return;
    }
    ANN(window);
    dvz_window_mouse(window, x, y, button);
}



void dvz_app_keyboard(DvzApp* app, DvzId canvas_id, DvzKeyCode* key)
{
    ANN(app);
    DvzWindow* window = dvz_client_window(app->client, canvas_id);
    if (window == NULL)
    {
        log_error("canvas #%" PRIx64 " does not exist");
        return;
    }
    ANN(window);
    dvz_window_keyboard(window, key);
}



static void _timer_callback(DvzTimer* timer, DvzInternalTimerEvent ev)
{
    ANN(timer);
    ANN(ev.item);

    DvzApp* app = (DvzApp*)ev.user_data;
    ANN(app);
    ANN(app->client);
    if (!app->is_running)
    {
        log_debug("skip timer event because the app is no longer running");
        return;
    }

    // Emit a client TIMER event.
    DvzClientEvent cev = {
        .type = DVZ_CLIENT_EVENT_TIMER,
        .content =
            {
                .t =
                    {
                        .timer_item = ev.item,
                        .timer_idx = ev.item->timer_idx,
                        .time = ev.time,
                        .step_idx = ev.item->count,
                    },
            },
    };
    dvz_client_event(app->client, cev);
}

DvzTimerItem* dvz_app_timer(DvzApp* app, double delay, double period, uint64_t max_count)
{
    ANN(app);
    if (!app->client)
        return NULL;

    ANN(app->timer);

    DvzTimerItem* item = dvz_timer_new(app->timer, delay, period, max_count);
    dvz_timer_callback(app->timer, item, _timer_callback, app);

    return item;
}



void dvz_app_on_timer(DvzApp* app, DvzAppTimerCallback on_timer, void* user_data)
{
    ANN(app);
    if (!app->client)
        return;
    ANN(app->client);

    Payload* payload =
        _make_payload(DVZ_CLIENT_EVENT_TIMER, app, (function_pointer)on_timer, user_data);
    dvz_client_callback(
        app->client, DVZ_CLIENT_EVENT_TIMER, DVZ_CLIENT_CALLBACK_SYNC, //
        _client_callback, payload);
}



void dvz_app_submit(DvzApp* app)
{
    ANN(app);
    if (!app->prt)
        return;
    ANN(app->prt);

    DvzBatch* batch = app->batch;
    ANN(batch);

    // NOTE: this fixes a memory leak because we make a copy of the batch, expecting it
    // to be freed by the event loop, but empty batches are NOT freed. So we don't make a
    // useless copy for empty batches, and we don't submit them at all.
    if (dvz_batch_size(batch) == 0)
    {
        return;
    }

    // NOTE: we copy the application batch because it will be destroyed and freed by
    // _requester_callback() in presenter.c, after it is processed by the renderer.
    dvz_presenter_submit(app->prt, dvz_batch_copy(batch));
    dvz_batch_clear(batch);
}



static inline void _gui_callback(DvzGuiWindow* gui_window, void* internal_payload)
{
    ANN(gui_window);

    DvzAppGuiPayload* payload = (DvzAppGuiPayload*)internal_payload;
    ANN(payload);

    DvzApp* app = payload->app;
    ANN(app);

    DvzId canvas_id = payload->canvas_id;
    ASSERT(canvas_id != DVZ_ID_NONE);

    DvzAppGuiCallback callback = payload->callback;
    void* user_data = payload->user_data;

    DvzGuiEvent ev = {
        .gui_window = gui_window,
        .user_data = user_data,
    };
    callback(app, canvas_id, ev);
}

void dvz_app_gui(DvzApp* app, DvzId canvas_id, DvzAppGuiCallback callback, void* user_data)
{
    ANN(app);
    if (!app->prt)
        return;

    DvzPresenter* prt = app->prt;
    ANN(prt);

    DvzAppGuiPayload* payload = (DvzAppGuiPayload*)calloc(1, sizeof(DvzAppGuiPayload));
    payload->app = app;
    payload->canvas_id = canvas_id;
    payload->callback = callback;
    payload->user_data = user_data;
    dvz_list_append(app->payloads, (DvzListItem){.p = (void*)payload});

    dvz_presenter_gui(prt, canvas_id, _gui_callback, payload);
}



void dvz_app_run(DvzApp* app, uint64_t frame_count)
{
    ANN(app);
    ANN(app->batch);
    ANN(app->host);

    if (app->client)
    {
        ANN(app->prt);
        ANN(app->prt->rd);
        ANN(app->prt->rd->ctx);

        // Emit a window init event.
        dvz_client_event(app->client, (DvzClientEvent){.type = DVZ_CLIENT_EVENT_INIT});

        // Submit all pending requests that were emitted during the initialization of the
        // application, before calling dvz_app_run().
        dvz_app_submit(app);

        // Start the event loop.
        app->is_running = true;
        dvz_client_run(app->client, frame_count);
        app->is_running = false;

        dvz_context_wait(app->prt->rd->ctx);
    }

    // Offscreen mini event loop.
    else if (app->host->backend == DVZ_BACKEND_OFFSCREEN)
    {
        log_trace("run offscreen app, discarding frame_count=%d in dvz_app_run()", frame_count);

        DvzRenderer* rd = app->rd;
        ANN(rd);

        DvzBatch* batch = app->batch;
        ANN(batch);

        // Process the requests.
        dvz_renderer_requests(app->rd, dvz_batch_size(batch), dvz_batch_requests(batch));

        // Set the DVZ_CAPTURE_PNG environment variable to automatically save a screenshot when
        // running.
        char* capture = capture_png(NULL);

        // Canvas screenshot.
        DvzCanvas* canvas = dvz_renderer_canvas(app->rd, DVZ_ID_NONE);
        if (canvas != NULL)
        {
            DvzId canvas_id = canvas->obj.id;
            // Apply a board update request, this submits the recorded command buffer to the GPU.
            dvz_renderer_request(app->rd, dvz_update_canvas(batch, canvas_id));

            if (capture != NULL)
            {
                dvz_app_screenshot(app, canvas_id, capture);
            }
        }
    }
}



void dvz_app_screenshot(DvzApp* app, DvzId canvas_id, const char* filename)
{
    // NOTE: the app must have run before.

    ANN(app);
    DvzRenderer* rd = app->rd;
    ANN(rd);

    ASSERT(canvas_id != DVZ_ID_NONE);

    if (app->host->backend == DVZ_BACKEND_GLFW)
    {
        DvzCanvas* canvas = dvz_renderer_canvas(rd, canvas_id);
        ANN(canvas);

        // Get the canvas image buffer.
        uint8_t* rgb = dvz_canvas_download(canvas);

        // Save to a PNG.
        dvz_write_png(filename, canvas->width, canvas->height, rgb);
    }
    else if (app->host->backend == DVZ_BACKEND_OFFSCREEN)
    {
        DvzCanvas* board = dvz_renderer_canvas(rd, canvas_id);
        ANN(board);

        // Get the board image buffer.
        uint8_t* rgb = dvz_board_alloc(board);
        dvz_board_download(board, board->size, rgb);

        // Save to a PNG.
        dvz_write_png(filename, board->width, board->height, rgb);
        // dvz_board_free(board);

        log_info("screenshot saved to %s (%s)", filename, pretty_size(dvz_file_size(filename)));
    }
}



void dvz_app_timestamps(
    DvzApp* app, DvzId canvas_id, uint32_t count, uint64_t* seconds, uint64_t* nanoseconds)
{
    ANN(app);
    ANN(seconds);
    ANN(nanoseconds);

    DvzRenderer* rd = app->rd;
    ANN(rd);

    dvz_canvas_timestamps(dvz_renderer_canvas(rd, canvas_id), count, seconds, nanoseconds);
}



void dvz_app_wait(DvzApp* app)
{
    ANN(app);
    ANN(app->gpu);
    dvz_gpu_wait(app->gpu);
}



void dvz_app_destroy(DvzApp* app)
{
    ANN(app);

    if (app->client)
    {
        dvz_client_destroy(app->client);
        dvz_presenter_destroy(app->prt);
    }

    if (app->offscreen_gui != NULL)
    {
        dvz_gui_destroy(app->offscreen_gui);
    }

    if (app->offscreen_guis != NULL)
    {
        dvz_map_destroy(app->offscreen_guis);
    }

    dvz_timer_destroy(app->timer);
    dvz_batch_destroy(app->batch);
    dvz_renderer_destroy(app->rd);
    dvz_gpu_destroy(app->gpu);
    dvz_host_destroy(app->host);

    // Free the callback payloads.
    void* payload = NULL;
    for (uint32_t i = 0; i < app->payloads->count; i++)
    {
        payload = dvz_list_get(app->payloads, i).p;
        ANN(payload);
        FREE(payload);
    }

    // Destroy the list of callback payloads.
    dvz_list_destroy(app->payloads);

    FREE(app);
}



void dvz_free(void* pointer) { FREE(pointer); }
