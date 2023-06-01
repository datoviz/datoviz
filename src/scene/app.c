/*************************************************************************************************/
/*  App                                                                                          */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/app.h"
#include "../render_utils.h"
#include "client.h"
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

    dvz_presenter_submit(app->prt, app->rqr);

    // The timer callbacks are called here.
    dvz_timer_tick(app->timer, ev.content.f.time);
}



/*************************************************************************************************/
/*  App functions                                                                                */
/*************************************************************************************************/

DvzApp* dvz_app(void)
{
    DvzApp* app = (DvzApp*)calloc(1, sizeof(DvzApp));

    app->host = dvz_host(BACKEND);
    ANN(app->host);

    app->gpu = make_gpu(app->host);
    ANN(app->gpu);

    app->rd = dvz_renderer(app->gpu, 0);
    ANN(app->rd);

    app->client = dvz_client(BACKEND);
    ANN(app->client);

    app->prt = dvz_presenter(app->rd, app->client, DVZ_CANVAS_FLAGS_IMGUI);
    ANN(app->prt);

    app->rqr = dvz_requester();
    ANN(app->rqr);

    // NOTE: we need to manually begin recording the requester, otherwise requests won't be
    // automatically recorded in the requester batch.
    dvz_requester_begin(app->rqr);

    app->timer = dvz_timer();
    ANN(app->timer);

    // Send the pending requests to the presenter at every frame.
    dvz_client_callback(
        app->client, DVZ_CLIENT_EVENT_FRAME, DVZ_CLIENT_CALLBACK_SYNC, _on_frame, app);

    return app;
}



DvzRequester* dvz_app_requester(DvzApp* app)
{
    ANN(app);
    return app->rqr;
}



void dvz_app_frame(DvzApp* app)
{
    ANN(app);
    dvz_client_frame(app->client);
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



void dvz_app_run(DvzApp* app, uint64_t n_frames)
{
    ANN(app);

    // End the requester batch just before running the event loop. This way we ensure the requester
    // is ready to be flushed in dvz_presenter_submit().
    dvz_requester_end(app->rqr, NULL);

    // Submit all pending requests that were emitted during the initialization of the application,
    // before calling dvz_app_run().
    dvz_presenter_submit(app->prt, app->rqr);

    // Start the event loop.
    dvz_client_run(app->client, n_frames);
}



void dvz_app_destroy(DvzApp* app)
{
    ANN(app);
    dvz_client_destroy(app->client);
    dvz_presenter_destroy(app->prt);
    dvz_timer_destroy(app->timer);
    dvz_requester_destroy(app->rqr);
    dvz_renderer_destroy(app->rd);
    dvz_gpu_destroy(app->gpu);
    dvz_host_destroy(app->host);
}
