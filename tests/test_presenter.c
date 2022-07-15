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
    // GPU-side.
    DvzHost* host = dvz_host(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = make_gpu(host);
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
    dvz_host_destroy(host);

    return 0;
}



int test_presenter_2(TstSuite* suite)
{
    // GPU-side.
    DvzHost* host = dvz_host(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = make_gpu(host);
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
    dvz_host_destroy(host);

    return 0;
}
