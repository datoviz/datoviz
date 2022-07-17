/*************************************************************************************************/
/*  Testing presenter */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_presenter.h"
#include "_glfw.h"
#include "client.h"
#include "presenter.h"
#include "test.h"
#include "test_vklite.h"
#include "testing.h"


/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define BACKEND DVZ_BACKEND_GLFW
#define WIDTH   800
#define HEIGHT  600
#define WID     10



/*************************************************************************************************/
/*  Test utils                                                                                   */
/*************************************************************************************************/

static void _callback_resize(DvzClient* client, DvzClientEvent ev, void* user_data)
{
    uint32_t width = ev.content.w.width;
    uint32_t height = ev.content.w.height;
    log_info("window %x resized to %dx%d", ev.window_id, width, height);
}



/*************************************************************************************************/
/*  Presenter tests                                                                              */
/*************************************************************************************************/

int test_presenter_1(TstSuite* suite)
{
    ASSERT(suite != NULL);

    // GPU-side.
    DvzHost* host = get_host(suite);

    DvzGpu* gpu = make_gpu(host);
    ASSERT(gpu != NULL);

    // Create a renderer.
    DvzRenderer* rnd = dvz_renderer(gpu, 0);

    // Client-side.
    DvzClient* client = dvz_client(BACKEND);
    DvzRequester rqr = dvz_requester();
    DvzRequest req = {0};

    // Presenter linking the renderer and the client.
    DvzPresenter* prt = dvz_presenter(rnd);
    dvz_presenter_client(prt, client);

    dvz_client_callback(
        client, DVZ_CLIENT_EVENT_WINDOW_RESIZE, DVZ_CLIENT_CALLBACK_SYNC, _callback_resize, NULL);

    // Start.

    // Make a canvas creation request.
    req = dvz_create_canvas(&rqr, WIDTH, HEIGHT, DVZ_DEFAULT_CLEAR_COLOR, 0);
    dvz_requester_add(&rqr, req);

    // Submit a client event with type REQUESTS and with a pointer to the requester.
    // The Presenter will register a REQUESTS callback sending the requests to the underlying
    // renderer.
    // TODO: improve this, pass an array of requests instead of a pointer to the Requester?
    dvz_client_event(
        client, (DvzClientEvent){.type = DVZ_CLIENT_EVENT_REQUESTS, .content.r.requests = &rqr});

    // Dequeue and process all pending events.
    dvz_client_run(client, N_FRAMES);

    // End.


    // Destroying all objects.
    dvz_client_destroy(client);
    dvz_requester_destroy(&rqr);

    dvz_renderer_destroy(rnd);
    dvz_presenter_destroy(prt);
    dvz_gpu_destroy(gpu);

    return 0;
}



int test_presenter_2(TstSuite* suite)
{
    ASSERT(suite != NULL);

    // GPU-side.
    DvzHost* host = get_host(suite);

    DvzGpu* gpu = make_gpu(host);
    ASSERT(gpu != NULL);

    // Create a renderer.
    DvzRenderer* rnd = dvz_renderer(gpu, 0);

    // Client-side.
    DvzClient* client = dvz_client(BACKEND);
    DvzRequester rqr = dvz_requester();
    DvzRequest req = {0};

    // Presenter linking the renderer and the client.
    DvzPresenter* prt = dvz_presenter(rnd);
    dvz_presenter_client(prt, client);

    dvz_client_callback(
        client, DVZ_CLIENT_EVENT_WINDOW_RESIZE, DVZ_CLIENT_CALLBACK_SYNC, _callback_resize, NULL);

    // Make rendering requests.

    {
        // Make a canvas creation request.
        req = dvz_create_canvas(&rqr, WIDTH, HEIGHT, DVZ_DEFAULT_CLEAR_COLOR, 0);
        dvz_requester_add(&rqr, req);

        // Canvas id.
        DvzId canvas_id = req.id;

        // Create a graphics.
        req = dvz_create_graphics(&rqr, canvas_id, DVZ_GRAPHICS_TRIANGLE, 0);
        dvz_requester_add(&rqr, req);
        DvzId graphics_id = req.id;

        // Create the vertex buffer dat.
        req = dvz_create_dat(&rqr, DVZ_BUFFER_TYPE_VERTEX, 3 * sizeof(DvzVertex), 0);
        dvz_requester_add(&rqr, req);
        DvzId dat_id = req.id;

        // Bind the vertex buffer dat to the graphics pipe.
        req = dvz_set_vertex(&rqr, graphics_id, dat_id);
        dvz_requester_add(&rqr, req);

        // Upload the triangle data.
        DvzVertex data[] = {
            {{-1, -1, 0}, {255, 0, 0, 255}},
            {{+1, -1, 0}, {0, 255, 0, 255}},
            {{+0, +1, 0}, {0, 0, 255, 255}},
        };
        req = dvz_upload_dat(&rqr, dat_id, 0, sizeof(data), data);
        dvz_requester_add(&rqr, req);

        // Binding #0: MVP.
        req = dvz_create_dat(&rqr, DVZ_BUFFER_TYPE_UNIFORM, sizeof(DvzMVP), 0);
        dvz_requester_add(&rqr, req);
        DvzId mvp_id = req.id;

        req = dvz_bind_dat(&rqr, graphics_id, 0, mvp_id);
        dvz_requester_add(&rqr, req);

        DvzMVP mvp = dvz_mvp_default();
        // dvz_show_base64(sizeof(mvp), &mvp);
        req = dvz_upload_dat(&rqr, mvp_id, 0, sizeof(DvzMVP), &mvp);
        dvz_requester_add(&rqr, req);

        // Binding #1: viewport.
        req = dvz_create_dat(&rqr, DVZ_BUFFER_TYPE_UNIFORM, sizeof(DvzViewport), 0);
        dvz_requester_add(&rqr, req);
        DvzId viewport_id = req.id;

        req = dvz_bind_dat(&rqr, graphics_id, 1, viewport_id);
        dvz_requester_add(&rqr, req);

        DvzViewport viewport = dvz_viewport_default(WIDTH, HEIGHT);
        // dvz_show_base64(sizeof(viewport), &viewport);
        req = dvz_upload_dat(&rqr, viewport_id, 0, sizeof(DvzViewport), &viewport);
        dvz_requester_add(&rqr, req);


        // Command buffer.
        req = dvz_record_begin(&rqr, canvas_id);
        dvz_requester_add(&rqr, req);

        req = dvz_record_viewport(&rqr, canvas_id, DVZ_DEFAULT_VIEWPORT, DVZ_DEFAULT_VIEWPORT);
        dvz_requester_add(&rqr, req);

        req = dvz_record_draw(&rqr, canvas_id, graphics_id, 0, 3);
        dvz_requester_add(&rqr, req);

        req = dvz_record_end(&rqr, canvas_id);
        dvz_requester_add(&rqr, req);
    }

    // Submit a client event with type REQUESTS and with a pointer to the requester.
    // The Presenter will register a REQUESTS callback sending the requests to the underlying
    // renderer.
    // TODO: improve this, pass an array of requests instead of a pointer to the Requester?
    dvz_client_event(
        client, (DvzClientEvent){.type = DVZ_CLIENT_EVENT_REQUESTS, .content.r.requests = &rqr});

    // Dequeue and process all pending events.
    dvz_client_run(client, N_FRAMES);

    // End.


    // Destroying all objects.
    dvz_client_destroy(client);
    dvz_requester_destroy(&rqr);

    dvz_renderer_destroy(rnd);
    dvz_presenter_destroy(prt);
    dvz_gpu_destroy(gpu);

    return 0;
}
