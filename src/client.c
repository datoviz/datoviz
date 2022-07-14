/*************************************************************************************************/
/*  Client                                                                                       */
/*************************************************************************************************/

#include "client.h"
#include "common.h"



/*************************************************************************************************/
/*  Callback functions                                                                           */
/*************************************************************************************************/

static void _callback_window_create(DvzDeq* deq, void* item, void* user_data)
{
    ASSERT(deq != NULL);

    ASSERT(item != NULL);
    DvzClientEvent* ev = (DvzClientEvent*)item;

    ASSERT(ev->type == DVZ_CLIENT_EVENT_WINDOW_CREATE);
    uint32_t width = ev->content.w.width;
    uint32_t height = ev->content.w.height;

    log_debug("window create (%dx%d)", width, height);
}



/*************************************************************************************************/
/*  Client functions                                                                             */
/*************************************************************************************************/

DvzClient* dvz_client(void)
{
    DvzClient* client = calloc(1, sizeof(DvzClient));

    // Create queue.
    client->deq = dvz_deq(1);

    // A single proc handling all events.
    dvz_deq_proc(&client->deq, 0, 1, (uint32_t[]){0});

    // Register a proc callback.
    dvz_deq_callback(
        &client->deq, 0, (int)DVZ_CLIENT_EVENT_WINDOW_CREATE, _callback_window_create, client);


    // create async queue
    // start background thread
    // default callbacks
    //     create window
    //     close window

    return client;
}



void dvz_client_event(DvzClient* client, DvzClientEvent ev)
{
    ASSERT(client != NULL);

    // Enqueue event.
    DvzClientEvent* pev = calloc(1, sizeof(DvzClientEvent));
    *pev = ev;

    dvz_deq_enqueue(&client->deq, 0, (int)ev.type, pev);
}



void dvz_client_callback(
    DvzClient* client, DvzClientEventType type, DvzClientCallbackMode mode,
    DvzClientCallback callback, const void* user_data)
{
    ASSERT(client != NULL);
}



void dvz_client_process(DvzClient* client)
{
    ASSERT(client != NULL);
    dvz_deq_dequeue_batch(&client->deq, 0);
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
