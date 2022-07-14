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
#include "testing.h"


/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  800
#define HEIGHT 600



/*************************************************************************************************/
/*  Client tests                                                                                 */
/*************************************************************************************************/

int test_client_1(TstSuite* suite)
{
    DvzClient* client = dvz_client();

    // Enqueue a window creation event.
    DvzClientEvent ev = {
        .window_id = 10,
        .type = DVZ_CLIENT_EVENT_WINDOW_CREATE,
        .content.w.width = WIDTH,
        .content.w.height = HEIGHT};
    dvz_client_event(client, ev);

    // Dequeue and process all pending events.
    dvz_client_process(client);

    dvz_client_destroy(client);
    return 0;
}
