/*************************************************************************************************/
/*  App                                                                                          */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/app.h"
#include "../render_utils.h"
#include "client.h"
#include "fileio.h"
#include "gui.h"
#include "host.h"
#include "presenter.h"
#include "renderer.h"
#include "request.h"
#include "timer.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define BACKEND DVZ_BACKEND_GLFW



/*************************************************************************************************/
/*  Util functions                                                                               */
/*************************************************************************************************/

static void _on_frame(DvzClient* client, DvzClientEvent ev)
{
    ANN(client);

    DvzApp* app = (DvzApp*)ev.user_data;
    ANN(app);

    // The timer callbacks are called here.
    dvz_timer_tick(app->timer, ev.content.f.time);
}



/*************************************************************************************************/
/*  App functions                                                                                */
/*************************************************************************************************/

DvzApp* dvz_app(int flags)
{
    DvzApp* app = (DvzApp*)calloc(1, sizeof(DvzApp));

    app->host = dvz_host(BACKEND);
    ANN(app->host);

    app->gpu = make_gpu(app->host);
    ANN(app->gpu);

    app->rd = dvz_renderer(app->gpu, flags);
    ANN(app->rd);

    app->client = dvz_client(BACKEND);
    ANN(app->client);

    app->prt = dvz_presenter(app->rd, app->client, DVZ_CANVAS_FLAGS_IMGUI);
    ANN(app->prt);

    app->batch = dvz_batch();
    ANN(app->batch);

    app->timer = dvz_timer();
    ANN(app->timer);

    // List of GUI callbacks.
    app->callbacks = dvz_list();

    // Send the pending requests to the presenter at every frame.
    dvz_app_onframe(app, _on_frame, app);

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
    dvz_client_frame(app->client);
}



void dvz_app_onframe(DvzApp* app, DvzClientCallback on_frame, void* user_data)
{
    ANN(app);
    dvz_client_callback(
        app->client, DVZ_CLIENT_EVENT_FRAME, DVZ_CLIENT_CALLBACK_SYNC, on_frame, user_data);
}



void dvz_app_onmouse(DvzApp* app, DvzClientCallback on_mouse, void* user_data)
{
    ANN(app);
    dvz_client_callback(
        app->client, DVZ_CLIENT_EVENT_MOUSE, DVZ_CLIENT_CALLBACK_SYNC, on_mouse, user_data);
}



void dvz_app_onkeyboard(DvzApp* app, DvzClientCallback on_keyboard, void* user_data)
{
    ANN(app);
    dvz_client_callback(
        app->client, DVZ_CLIENT_EVENT_KEYBOARD, DVZ_CLIENT_CALLBACK_SYNC, on_keyboard, user_data);
}



void dvz_app_onresize(DvzApp* app, DvzClientCallback on_resize, void* user_data)
{
    ANN(app);
    dvz_client_callback(
        app->client, DVZ_CLIENT_EVENT_WINDOW_RESIZE, DVZ_CLIENT_CALLBACK_SYNC, on_resize,
        user_data);
}



static void _timer_callback(DvzTimer* timer, DvzTimerEvent ev)
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
    ANN(app->timer);

    DvzTimerItem* item = dvz_timer_new(app->timer, delay, period, max_count);
    dvz_timer_callback(app->timer, item, _timer_callback, app);

    return item;
}



void dvz_app_ontimer(DvzApp* app, DvzClientCallback on_timer, void* user_data)
{
    ANN(app);
    ANN(app->client);

    dvz_client_callback(
        app->client, DVZ_CLIENT_EVENT_TIMER, DVZ_CLIENT_CALLBACK_SYNC, on_timer, user_data);
}



void dvz_app_submit(DvzApp* app)
{
    ANN(app);
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
    DvzId canvas_id = payload->canvas_id;
    DvzAppGui callback = payload->callback;
    void* user_data = payload->user_data;

    callback(app, canvas_id, user_data);
}

void dvz_app_gui(DvzApp* app, DvzId canvas_id, DvzAppGui callback, void* user_data)
{
    ANN(app);

    DvzPresenter* prt = app->prt;
    ANN(prt);

    DvzAppGuiPayload* payload = (DvzAppGuiPayload*)calloc(1, sizeof(DvzAppGuiPayload));
    payload->canvas_id = canvas_id;
    payload->callback = callback;
    payload->user_data = user_data;
    dvz_list_append(app->callbacks, (DvzListItem){.p = (void*)payload});

    dvz_presenter_gui(prt, canvas_id, _gui_callback, payload);
}



void dvz_app_run(DvzApp* app, uint64_t n_frames)
{
    ANN(app);
    ANN(app->client);
    ANN(app->batch);
    ANN(app->prt);
    ANN(app->prt->rd);
    ANN(app->prt->rd->ctx);

    // Emit a window init event.
    dvz_client_event(app->client, (DvzClientEvent){.type = DVZ_CLIENT_EVENT_INIT});

    // Submit all pending requests that were emitted during the initialization of the application,
    // before calling dvz_app_run().
    dvz_app_submit(app);

    // Start the event loop.
    app->is_running = true;
    dvz_client_run(app->client, n_frames);
    app->is_running = false;

    dvz_context_wait(app->prt->rd->ctx);
}



void dvz_app_screenshot(DvzApp* app, DvzId canvas_id, const char* filename)
{
    // NOTE: the app must have run before.

    ANN(app);
    DvzRenderer* rd = app->rd;
    ANN(rd);

    DvzCanvas* canvas = dvz_renderer_canvas(rd, canvas_id);
    ANN(canvas);

    // Get the canvas image buffer.
    uint8_t* rgb = dvz_canvas_download(canvas);

    // Save to a PNG.
    dvz_write_png(filename, canvas->width, canvas->height, rgb);
}



void dvz_app_destroy(DvzApp* app)
{
    ANN(app);
    dvz_client_destroy(app->client);
    dvz_presenter_destroy(app->prt);
    dvz_timer_destroy(app->timer);
    dvz_batch_destroy(app->batch);
    dvz_renderer_destroy(app->rd);
    dvz_gpu_destroy(app->gpu);
    dvz_host_destroy(app->host);

    // Free the callback payloads.
    DvzAppGuiPayload* payload = NULL;
    for (uint32_t i = 0; i < app->callbacks->count; i++)
    {
        payload = (DvzAppGuiPayload*)(dvz_list_get(app->callbacks, i).p);
        ANN(payload);
        FREE(payload);
    }

    // Destroy the list of GUI callbacks.
    dvz_list_destroy(app->callbacks);

    FREE(app);
}
