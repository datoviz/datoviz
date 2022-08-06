/*************************************************************************************************/
/*  Testing client input                                                                         */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_client_input.h"
#include "client.h"
#include "client_input.h"
#include "glfw_utils.h"
#include "input.h"
#include "keyboard.h"
#include "mouse.h"
#include "test.h"
#include "testing.h"
#include "time.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define DEBUG_TEST (getenv("DVZ_DEBUG") != NULL)
#define N_FRAMES   (DEBUG_TEST ? 0 : 5)



/*************************************************************************************************/
/*  Client input tests                                                                           */
/*************************************************************************************************/

static void _on_mouse(DvzClient* client, DvzClientEvent ev)
{
    ASSERT(client != NULL);
    log_debug("mouse event %d", ev.content.m.type);
}

static void _on_keyboard(DvzClient* client, DvzClientEvent ev)
{
    ASSERT(client != NULL);
    log_debug("keyboard event %d", ev.content.k.type);
}

int test_client_input(TstSuite* suite)
{
    DvzClient* client = dvz_client(DVZ_BACKEND_GLFW);
    dvz_client_input(client);
    dvz_client_callback(client, DVZ_CLIENT_EVENT_MOUSE, DVZ_CLIENT_CALLBACK_SYNC, _on_mouse, NULL);
    dvz_client_callback(
        client, DVZ_CLIENT_EVENT_KEYBOARD, DVZ_CLIENT_CALLBACK_SYNC, _on_keyboard, NULL);

    DvzId id = 10;

    // Enqueue a window creation event.
    DvzClientEvent ev = {
        .window_id = id,
        .type = DVZ_CLIENT_EVENT_WINDOW_CREATE,
        .content.w.width = 800,
        .content.w.height = 600};
    dvz_client_event(client, ev);

    // Dequeue and process all pending events.
    dvz_client_process(client);

    dvz_client_run(client, N_FRAMES);

    dvz_client_destroy(client);
    return 0;
}
