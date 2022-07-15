/*************************************************************************************************/
/*  Testing client                                                                               */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_client.h"
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
/*  Client test utils                                                                            */
/*************************************************************************************************/

static void _create_window(DvzClient* client, DvzId id)
{
    ASSERT(client != NULL);

    // Enqueue a window creation event.
    DvzClientEvent ev = {
        .window_id = id,
        .type = DVZ_CLIENT_EVENT_WINDOW_CREATE,
        .content.w.width = WIDTH,
        .content.w.height = HEIGHT};
    dvz_client_event(client, ev);
}



/*************************************************************************************************/
/*  Client tests                                                                                 */
/*************************************************************************************************/

int test_client_1(TstSuite* suite)
{
    DvzClient* client = dvz_client(BACKEND);

    // Create a window.
    _create_window(client, WID);

    // Dequeue and process all pending events.
    dvz_client_process(client);

    dvz_client_destroy(client);
    return 0;
}



int test_client_2(TstSuite* suite)
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
