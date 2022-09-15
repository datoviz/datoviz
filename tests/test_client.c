/*************************************************************************************************/
/*  Testing client                                                                               */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_client.h"
#include "client.h"
#include "glfw_utils.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"


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
    ANN(client);

    // Enqueue a window creation event.
    DvzClientEvent ev = {
        .window_id = id,
        .type = DVZ_CLIENT_EVENT_WINDOW_CREATE,
        .content.w.screen_width = WIDTH,
        .content.w.screen_height = HEIGHT};
    dvz_client_event(client, ev);
}



static void _delete_window(DvzClient* client, DvzId id)
{
    ANN(client);

    // Enqueue a window creation event.
    DvzClientEvent ev = {
        .window_id = id,
        .type = DVZ_CLIENT_EVENT_WINDOW_REQUEST_DELETE,
    };
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
    DvzClient* client = dvz_client(BACKEND);

    _create_window(client, WID);
    _create_window(client, WID + 1);

    dvz_client_run(client, 5);

    _delete_window(client, WID + 1);
    dvz_client_run(client, 5);

    _delete_window(client, WID);
    dvz_client_run(client, N_FRAMES);

    dvz_client_destroy(client);
    return 0;
}
