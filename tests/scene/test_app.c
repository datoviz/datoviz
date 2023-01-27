/*************************************************************************************************/
/*  Testing app                                                                                  */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_app.h"
#include "canvas.h"
#include "scene/app.h"
#include "scene/arcball.h"
#include "scene/camera.h"
#include "scene/panzoom.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  800
#define HEIGHT 600



/*************************************************************************************************/
/*  Test utils                                                                                   */
/*************************************************************************************************/

typedef struct PanzoomStruct PanzoomStruct;
struct PanzoomStruct
{
    DvzApp* app;
    DvzId mvp_id;
    DvzMVP mvp;
    DvzPanzoom* pz;
};



/*************************************************************************************************/
/*  App tests                                                                                    */
/*************************************************************************************************/

static void _on_mouse(DvzClient* client, DvzClientEvent ev)
{
    ANN(client);

    PanzoomStruct* ps = (PanzoomStruct*)ev.user_data;
    ANN(ps);

    DvzPanzoom* pz = ps->pz;
    ANN(pz);

    DvzPresenter* prt = ps->app->prt;
    ANN(prt);

    DvzRequester* rqr = ps->app->rqr;
    ANN(rqr);

    DvzMVP* mvp = &ps->mvp;
    ANN(mvp);

    DvzId mvp_id = ps->mvp_id;

    // Dragging: pan.
    if (ev.content.m.type == DVZ_MOUSE_EVENT_DRAG)
    {
        if (ev.content.m.content.d.button == DVZ_MOUSE_BUTTON_LEFT)
        {
            dvz_panzoom_pan_shift(pz, ev.content.m.content.d.shift, (vec2){0});
        }
        else if (ev.content.m.content.d.button == DVZ_MOUSE_BUTTON_RIGHT)
        {
            dvz_panzoom_zoom_shift(
                pz, ev.content.m.content.d.shift, ev.content.m.content.d.press_pos);
        }
    }

    // Stop dragging.
    if (ev.content.m.type == DVZ_MOUSE_EVENT_DRAG_STOP)
    {
        dvz_panzoom_end(pz);
    }

    // Mouse wheel.
    if (ev.content.m.type == DVZ_MOUSE_EVENT_WHEEL)
    {
        dvz_panzoom_zoom_wheel(pz, ev.content.m.content.w.dir, ev.content.m.content.w.pos);
    }

    // Double-click
    if (ev.content.m.type == DVZ_MOUSE_EVENT_DOUBLE_CLICK)
    {
        dvz_panzoom_reset(pz);
    }

    // Update the MVP matrices.
    *mvp = dvz_mvp_default();
    dvz_panzoom_mvp(pz, mvp);

    // Submit a dat upload request with the new MVP matrices.
    dvz_requester_add(rqr, dvz_upload_dat(rqr, mvp_id, 0, sizeof(DvzMVP), mvp));
}

static void _scatter_resize(DvzClient* client, DvzClientEvent ev)
{
    ANN(client);

    uint32_t width = ev.content.w.screen_width;
    uint32_t height = ev.content.w.screen_height;
    log_info("window 0x%" PRIx64 " resized to %dx%d", ev.window_id, width, height);

    DvzPanzoom* pz = (DvzPanzoom*)ev.user_data;
    ANN(pz);
    dvz_panzoom_resize(pz, width, height);
}

int test_app_1(TstSuite* suite)
{
    ANN(suite);

    // Create app objects.
    DvzApp* app = dvz_app();
    DvzRequester* rqr = dvz_app_requester(app);

    const uint32_t n = 52;
    GraphicsWrapper wrapper = {0};
    graphics_request(rqr, n, &wrapper, DVZ_CANVAS_FLAGS_FPS);
    void* data = graphics_scatter(rqr, wrapper.dat_id, n);

    // Panzoom callback.
    DvzPanzoom* pz = dvz_panzoom(WIDTH, HEIGHT, 0);
    PanzoomStruct ps = {
        .mvp_id = wrapper.mvp_id,
        .app = app,
        .pz = pz,
    };
    dvz_app_mouse(app, _on_mouse, &ps);
    dvz_app_resize(app, _scatter_resize, pz);

    dvz_app_run(app, N_FRAMES);

    dvz_panzoom_destroy(pz);
    dvz_app_destroy(app);

    FREE(data);
    return 0;
}
