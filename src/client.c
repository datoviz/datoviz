/*************************************************************************************************/
/*  Client                                                                                       */
/*************************************************************************************************/

#include "client.h"
#include "client_utils.h"
#include "common.h"
#include "glfw_utils.h"
#include "window.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static uint64_t count_windows(DvzClient* client)
{
    ASSERT(client != NULL);
    DvzContainerIterator iter = dvz_container_iterator(&client->windows);
    DvzWindow* window = NULL;
    uint64_t count = 0;
    while (iter.item != NULL)
    {
        window = (DvzWindow*)iter.item;
        ASSERT(window != NULL);
        if (dvz_obj_is_created(&window->obj))
            count++;
        dvz_container_iter(&iter);
    }
    return count;
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

    if (payload->mode == DVZ_CLIENT_CALLBACK_SYNC)
    {
        payload->callback(client, *ev, payload->user_data);
    }
    else if (payload->mode == DVZ_CLIENT_CALLBACK_ASYNC)
    {
        // TODO: enqueue callback to async queue
    }

    return;
}



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

    create_client_window(client, ev->window_id, width, height, ev->content.w.flags);
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

    DvzWindow* window = id2window(client, ev->window_id);
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

DvzClient* dvz_client(DvzBackend backend)
{
    backend_init(backend);

    DvzClient* client = calloc(1, sizeof(DvzClient));
    client->backend = backend;
    client->map = dvz_map();

    // Create the window container.
    client->windows =
        dvz_container(DVZ_CONTAINER_DEFAULT_COUNT, sizeof(DvzWindow), DVZ_OBJECT_TYPE_WINDOW);

    // Create queue.
    client->deq = dvz_deq(1);

    // A single proc handling all events.
    dvz_deq_proc(&client->deq, 0, 1, (uint32_t[]){0});

    // Register a deq callback.
    dvz_deq_callback(
        &client->deq, 0, (int)DVZ_CLIENT_EVENT_WINDOW_CREATE, _callback_window_create, client);

    // TODO: create async queue
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
    DvzClientCallback callback, void* user_data)
{
    ASSERT(client != NULL);

    // TODO
    if (mode == DVZ_CLIENT_CALLBACK_ASYNC)
    {
        log_error("async callbacks are not yet implemented, falling back to sync callbacks");
        mode = DVZ_CLIENT_CALLBACK_SYNC;
    }

    DvzClientPayload* payload = &client->callbacks[client->callback_count++];
    payload->client = client;
    payload->callback = callback;
    payload->user_data = user_data;
    payload->mode = mode;
    dvz_deq_callback(&client->deq, 0, (int)type, _deq_callback, payload);
}



void dvz_client_process(DvzClient* client)
{
    ASSERT(client != NULL);
    dvz_deq_dequeue_batch(&client->deq, 0);
}



int dvz_client_frame(DvzClient* client)
{
    ASSERT(client != NULL);

    // Poll backend events (mouse, keyboard...).
    // TODO: enqueue mouse and keyboard events
    backend_poll_events(client->backend);

    // Dequeue and process events.
    dvz_client_process(client);

    // Loop over the windows.
    DvzContainerIterator iter = dvz_container_iterator(&client->windows);
    DvzWindow* window = NULL;
    uint64_t count = 0;

    DvzClientEvent frame_ev = {0};
    frame_ev.type = DVZ_CLIENT_EVENT_FRAME;

    while (iter.item != NULL)
    {
        window = (DvzWindow*)iter.item;
        ASSERT(window != NULL);

        // Skip non-created windows.
        if (!dvz_obj_is_created(&window->obj))
        {
            dvz_container_iter(&iter);
            continue;
        }
        // Skip windows that should be closed.
        if (backend_should_close(window) || window->obj.status == DVZ_OBJECT_STATUS_NEED_DESTROY)
        {
            dvz_container_iter(&iter);
            continue;
        }
        // Skip inactive windows.
        if (window->obj.status == DVZ_OBJECT_STATUS_INACTIVE)
        {
            dvz_container_iter(&iter);
            continue;
        }

        // Enqueue a FRAME event on active windows.
        frame_ev.window_id = window2id(window);
        frame_ev.content.f.frame_idx = client->frame_idx;
        dvz_client_event(client, frame_ev);

        // Count the number of active windows.
        count++;
        dvz_container_iter(&iter);
    }

    // Dequeue and process events, again (after sending the FRAME event to the active windows).
    dvz_client_process(client);

    // Return the number of active windows.
    return count;
}



void dvz_client_run(DvzClient* client, uint64_t n_frames)
{
    ASSERT(client != NULL);
    log_trace("start client event loop with %d frames", n_frames);
    int window_count = 0;
    uint64_t n = (n_frames > 0 ? n_frames : INFINITY);
    for (client->frame_idx = 0; client->frame_idx < n; client->frame_idx++)
    {
        window_count = dvz_client_frame(client);
        log_trace(
            "running client frame #%d with %d active windows", client->frame_idx, window_count);
        if (window_count == 0)
            break;
    }
    log_trace("stop client event loop after %d/%d frames", client->frame_idx + 1, n);
}



void dvz_client_destroy(DvzClient* client)
{
    ASSERT(client != NULL);

    dvz_deq_destroy(&client->deq);

    CONTAINER_DESTROY_ITEMS(DvzWindow, client->windows, dvz_window_destroy)
    dvz_container_destroy(&client->windows);

    dvz_map_destroy(client->map);

    // TODO: stop background thread

    backend_terminate(client->backend);

    FREE(client);
}
