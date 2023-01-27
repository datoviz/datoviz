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



void dvz_app_mouse(DvzApp* app, DvzClientCallback on_mouse, void* user_data)
{
    ANN(app);
    dvz_client_callback(
        app->client, DVZ_CLIENT_EVENT_MOUSE, DVZ_CLIENT_CALLBACK_SYNC, on_mouse, user_data);
}



void dvz_app_keyboard(DvzApp* app, DvzClientCallback on_keyboard, void* user_data)
{
    ANN(app);
    dvz_client_callback(
        app->client, DVZ_CLIENT_EVENT_KEYBOARD, DVZ_CLIENT_CALLBACK_SYNC, on_keyboard, user_data);
}



void dvz_app_resize(DvzApp* app, DvzClientCallback on_resize, void* user_data)
{
    ANN(app);
    dvz_client_callback(
        app->client, DVZ_CLIENT_EVENT_WINDOW_RESIZE, DVZ_CLIENT_CALLBACK_SYNC, on_resize,
        user_data);
}



DvzTimerItem* dvz_app_timer(
    DvzApp* app, double delay, double period, uint64_t max_count, DvzTimerCallback on_timer,
    void* user_data)
{
    ANN(app);
    ANN(app->timer);

    DvzTimerItem* item = dvz_timer_new(app->timer, delay, period, max_count);
    dvz_timer_callback(app->timer, item, on_timer, user_data);

    return item;
}



void dvz_app_run(DvzApp* app, uint64_t n_frames)
{
    ANN(app);
    dvz_presenter_submit(app->prt, app->rqr);
    dvz_client_run(app->client, n_frames);
}



void dvz_app_destroy(DvzApp* app)
{
    ANN(app);
    dvz_client_destroy(app->client);
    dvz_presenter_destroy(app->prt);
    dvz_requester_destroy(app->rqr);
    dvz_renderer_destroy(app->rd);
    dvz_gpu_destroy(app->gpu);
    dvz_host_destroy(app->host);
}
