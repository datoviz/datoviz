/*************************************************************************************************/
/*  Client                                                                                       */
/*************************************************************************************************/

#include "client.h"
#include "common.h"



/*************************************************************************************************/
/*  Util functions                                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Client functions                                                                             */
/*************************************************************************************************/

DvzClient* dvz_client(void)
{
    DvzClient* client = calloc(1, sizeof(DvzClient));

    client->deq = dvz_deq(1);
    // create queue
    // create async queue
    // start background thread
    // default callbacks
    //     create window
    //     close window

    return client;
}



void dvz_client_window(DvzClient* client, DvzId id, uint32_t width, uint32_t height, int flags)
{
    // async, enqueue
    ASSERT(client != NULL);
}



void dvz_client_event(DvzClient* client, DvzClientEvent ev)
{
    ASSERT(client != NULL);
    // enqueue event
}



void dvz_client_callback(
    DvzClient* client, DvzClientEventType type, DvzClientCallbackMode mode,
    DvzClientCallback callback, const void* user_data)
{
    ASSERT(client != NULL);
}



void dvz_client_frame(DvzClient* client)
{
    ASSERT(client != NULL);
    // for each window
    //     process glfw events
    //     detect mouse and keyboard events
    //     enqueue mouse and keyboard events
    // dequeue events
    // for each event
    //     for each callback registered for that event type
    //         if sync: call it
    //         if async: enqueue the event to the async queue
}



void dvz_client_run(DvzClient* client, uint64_t n_frames) { ASSERT(client != NULL); }



void dvz_client_destroy(DvzClient* client)
{
    ASSERT(client != NULL);
    dvz_deq_destroy(&client->deq);
    FREE(client);
    // stop background thread
}
