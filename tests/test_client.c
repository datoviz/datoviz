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

    // Presenter linking the renderer and the client.
    DvzPresenter* prt = dvz_presenter(rnd);
    dvz_presenter_client(prt, client);


    // Start.

    // Create a window.
    _create_window(client, WID);


    // Dequeue and process all pending events.
    dvz_client_run(client, N_FRAMES);

    // Destroying all objects.
    dvz_client_destroy(client);
    dvz_renderer_destroy(rnd);
    dvz_presenter_destroy(prt);
    dvz_gpu_destroy(gpu);
    dvz_host_destroy(host);

    return 0;
}
