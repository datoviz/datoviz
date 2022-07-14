/*************************************************************************************************/
/*  Client                                                                                       */
/*************************************************************************************************/

#include "client.h"
#include "_glfw.h"
#include "common.h"
#include "window.h"



/*************************************************************************************************/
/*  Callback functions                                                                           */
/*************************************************************************************************/

static void _callback_window_create(DvzDeq* deq, void* item, void* user_data)
{
    ASSERT(deq != NULL);

    ASSERT(user_data != NULL);
    DvzClient* client = (DvzClient*)user_data;

    ASSERT(item != NULL);
    DvzClientEvent* ev = (DvzClientEvent*)item;
    ASSERT(ev->type == DVZ_CLIENT_EVENT_WINDOW_CREATE);

    uint32_t width = ev->content.w.width;
    uint32_t height = ev->content.w.height;

    log_debug("client: create window #%d (%dx%d)", ev->window_id, width, height);

    // HACK: improve this
    DvzWindow* window = dvz_container_alloc(&client->windows);
    *window = dvz_window(DVZ_BACKEND_GLFW, width, height, 0);

    dvz_map_add(client->map, ev->window_id, DVZ_OBJECT_TYPE_WINDOW, window);
}



static void _callback_window_delete(DvzDeq* deq, void* item, void* user_data)
{
    ASSERT(deq != NULL);

    ASSERT(user_data != NULL);
    DvzClient* client = (DvzClient*)user_data;

    ASSERT(item != NULL);
    DvzClientEvent* ev = (DvzClientEvent*)item;
    ASSERT(ev->type == DVZ_CLIENT_EVENT_WINDOW_DELETE);

    log_debug("client: delete window #%d", ev->window_id);

    DvzWindow* window = dvz_map_get(client->map, ev->window_id);
    if (window == NULL)
    {
        log_warn("window #%d not found", ev->window_id);
    }
    else
    {
        dvz_window_destroy(window);
    }
}



/*************************************************************************************************/
/*  Client functions                                                                             */
/*************************************************************************************************/

DvzClient* dvz_client(void)
{
    backend_init(DVZ_BACKEND_GLFW);

    DvzClient* client = calloc(1, sizeof(DvzClient));

    client->map = dvz_map();

    // Create the window container.
    client->windows =
        dvz_container(DVZ_CONTAINER_DEFAULT_COUNT, sizeof(DvzWindow), DVZ_OBJECT_TYPE_WINDOW);

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

    CONTAINER_DESTROY_ITEMS(DvzWindow, client->windows, dvz_window_destroy)
    dvz_container_destroy(&client->windows);

    dvz_map_destroy(client->map);

    // stop background thread

    backend_terminate(DVZ_BACKEND_GLFW);

    FREE(client);
}
