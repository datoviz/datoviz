/*************************************************************************************************/
/*  Testing client                                                                               */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_client.h"
#include "_glfw.h"
#include "client.h"
#include "test.h"
#include "test_vklite.h"
#include "testing.h"


/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define BACKEND DVZ_BACKEND_GLFW
#define WIDTH   800
#define HEIGHT  600



/*************************************************************************************************/
/*  Client test utils                                                                            */
/*************************************************************************************************/

static void _create_window(DvzClient* client)
{
    ASSERT(client != NULL);

    // Enqueue a window creation event.
    DvzClientEvent ev = {
        .window_id = 10,
        .type = DVZ_CLIENT_EVENT_WINDOW_CREATE,
        .content.w.width = WIDTH,
        .content.w.height = HEIGHT};
    dvz_client_event(client, ev);
}



static void _deq_callback(DvzDeq* deq, void* item, void* user_data)
{
    ASSERT(deq != NULL);

    DvzClientPayload* payload = (DvzClientPayload*)user_data;
    ASSERT(payload != NULL);

    DvzClient* client = payload->client;
    ASSERT(client != NULL);

    DvzClientEvent* ev = (DvzClientEvent*)item;
    ASSERT(ev != NULL);

    payload->callback(client, *ev, payload->user_data);

    return;
}



/*************************************************************************************************/
/*  Client tests                                                                                 */
/*************************************************************************************************/

int test_client_1(TstSuite* suite)
{
    DvzClient* client = dvz_client(BACKEND);

    // Create a window.
    _create_window(client);

    // Dequeue and process all pending events.
    dvz_client_process(client);

    dvz_client_destroy(client);
    return 0;
}



int test_client_2(TstSuite* suite)
{
    DvzClient* client = dvz_client(BACKEND);

    // Create a window.
    _create_window(client);

    // Dequeue and process all pending events.
    dvz_client_run(client, N_FRAMES);

    dvz_client_destroy(client);
    return 0;
}
