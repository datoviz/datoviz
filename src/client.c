/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Client                                                                                       */
/*************************************************************************************************/

#include "client.h"
#include "backend.h"
#include "client_utils.h"
#include "common.h"
#include "fifo.h"
#include "window.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

// static uint64_t count_windows(DvzClient* client)
// {
//     ANN(client);
//     DvzContainerIterator iter = dvz_container_iterator(&client->windows);
//     DvzWindow* window = NULL;
//     uint64_t count = 0;
//     while (iter.item != NULL)
//     {
//         window = (DvzWindow*)iter.item;
//         ANN(window);
//         if (dvz_obj_is_created(&window->obj))
//             count++;
//         dvz_container_iter(&iter);
//     }
//     return count;
// }



static void _deq_callback(DvzDeq* deq, void* item, void* user_data)
{
    ANN(deq);

    DvzClientPayload* payload = (DvzClientPayload*)user_data;
    ANN(payload);

    DvzClient* client = payload->client;
    ANN(client);

    DvzClientEvent* ev = (DvzClientEvent*)item;
    ANN(ev);

    if (payload->mode == DVZ_CLIENT_CALLBACK_SYNC)
    {
        ev->user_data = payload->user_data;
        payload->callback(client, *ev);
    }
    else if (payload->mode == DVZ_CLIENT_CALLBACK_ASYNC)
    {
        // TODO: enqueue callback to async queue
    }

    return;
}



static inline bool _should_close(DvzWindow* window)
{
    ANN(window);
    return dvz_backend_should_close(window) ||
           window->obj.status == DVZ_OBJECT_STATUS_NEED_DESTROY;
}



/*************************************************************************************************/
/*  Client functions                                                                             */
/*************************************************************************************************/

DvzClient* dvz_client(DvzBackend backend)
{
    // backend_init(backend);

    DvzClient* client = calloc(1, sizeof(DvzClient));
    client->backend = backend;
    client->map = dvz_map();
    client->clock = dvz_clock();
    client->to_stop = dvz_atomic();

    // Create the window container.
    client->windows =
        dvz_container(DVZ_CONTAINER_DEFAULT_COUNT, sizeof(DvzWindow), DVZ_OBJECT_TYPE_WINDOW);

    // Create queue.
    client->deq = dvz_deq(1, sizeof(DvzClientEvent));

    // A single proc handling all events.
    dvz_deq_proc(client->deq, 0, 1, (uint32_t[]){0});

    // Register the callback.
    dvz_deq_callback(
        client->deq, 0, (int)DVZ_CLIENT_EVENT_WINDOW_CREATE, _callback_window_create, client);

    // Delete window callback, in reverse order because the client's default close callback
    // (destroying the window) must be called after the other delete callbacks registered by the
    // other components (client_input, canvas...).
    dvz_deq_callback(
        client->deq, 0, (int)DVZ_CLIENT_EVENT_WINDOW_DELETE, _callback_window_delete, client);
    dvz_deq_order(client->deq, (int)DVZ_CLIENT_EVENT_WINDOW_DELETE, DVZ_DEQ_ORDER_REVERSE);

    // Ty default, the client registers a callback to request_close, that just destroys the window.

    // dvz_deq_callback(
    //     client->deq, 0, (int)DVZ_CLIENT_EVENT_WINDOW_REQUEST_DELETE,
    //     _callback_window_request_delete, client);

    // TODO: create async queue
    // start background thread

    return client;
}



void dvz_client_event(DvzClient* client, DvzClientEvent ev)
{
    ANN(client);
    dvz_deq_enqueue(client->deq, 0, (int)ev.type, &ev);
}



void dvz_client_callback(
    DvzClient* client, DvzClientEventType type, DvzClientCallbackMode mode,
    DvzClientCallback callback, void* user_data)
{
    ANN(client);

    // TODO: async callbacks
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
    dvz_deq_callback(client->deq, 0, (int)type, _deq_callback, payload);
}



void dvz_client_process(DvzClient* client)
{
    ANN(client);
    dvz_deq_dequeue_batch(client->deq, 0);
}



int dvz_client_frame(DvzClient* client)
{
    ANN(client);

    // Poll backend events (mouse, keyboard...).
    dvz_backend_poll_events(client->backend);

    // Dequeue and process events.
    dvz_client_process(client);
    if (dvz_atomic_get(client->to_stop) == 1)
        return 0;

    // Loop over the windows.
    DvzContainerIterator iter = dvz_container_iterator(&client->windows);
    DvzWindow* window = NULL;
    uint64_t count = 0;

    DvzClientEvent frame_ev = {0};
    frame_ev.type = DVZ_CLIENT_EVENT_FRAME;
    bool to_close = false;

    while (iter.item != NULL)
    {
        window = (DvzWindow*)iter.item;
        ANN(window);

        // Skip non-created windows.
        if (!dvz_obj_is_created(&window->obj))
        {
            dvz_container_iter(&iter);
            continue;
        }
        // Skip windows that should be closed.
        if (_should_close(window))
        {
            // Do nothing, just skip the FRAME.
            to_close = true;
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
        frame_ev.content.f.time = dvz_clock_get(&client->clock);
        frame_ev.content.f.interval = dvz_clock_interval(&client->clock);
        dvz_clock_tick(&client->clock);
        dvz_client_event(client, frame_ev);

        // Count the number of active windows.
        count++;
        dvz_container_iter(&iter);
    }

    // Dequeue and process events, again (after sending the FRAME event to the active windows).
    dvz_client_process(client);

    // Collect the windows that should close.
    if (to_close)
    {
        iter = dvz_container_iterator(&client->windows);
        while (iter.item != NULL)
        {
            window = (DvzWindow*)iter.item;
            ANN(window);
            // Skip non-created windows.
            if (dvz_obj_is_created(&window->obj) && _should_close(window))
            {
                // Emit a window delete event.
                dvz_client_event(
                    client, (DvzClientEvent){
                                .type = DVZ_CLIENT_EVENT_WINDOW_DELETE,
                                .window_id = window->obj.id,
                            });
            }
            dvz_container_iter(&iter);
        }

        // Process all window delete events.
        dvz_client_process(client);
    }

    // Return the number of active windows.
    return count;
}



DvzWindow* dvz_client_window(DvzClient* client, DvzId id)
{
    ANN(client);
    DvzContainerIterator iter = dvz_container_iterator(&client->windows);
    DvzWindow* window = NULL;
    while (iter.item != NULL)
    {
        window = (DvzWindow*)iter.item;
        ANN(window);
        if (dvz_obj_is_created(&window->obj) && window->obj.id == id)
        {
            return window;
        }
        dvz_container_iter(&iter);
    }
    return NULL;
}



void dvz_client_run(DvzClient* client, uint64_t frame_count)
{
    ANN(client);
    dvz_atomic_set(client->to_stop, 0);
    log_trace("start client event loop with %d frames", frame_count);
    int window_count = 0;
    client->frame_count = frame_count;
    for (client->frame_idx = 0;                                      //
         frame_count > 0 ? (client->frame_idx < frame_count) : true; //
         client->frame_idx++)
    {
        window_count = dvz_client_frame(client);
        log_trace(
            "running client frame #%d with %d active windows", client->frame_idx, window_count);
        if (window_count == 0)
        {
            log_trace("stop event loop because there are no windows left");
            break;
        }
    }

    dvz_client_stop(client);
    log_trace("stop client event loop after %d/%d frames", client->frame_idx + 1, frame_count);
}



static void* client_thread(void* user_data)
{
    DvzClient* client = (DvzClient*)user_data;
    ANN(client);
    log_trace("start client event loop in background thread");
    dvz_client_run(client, client->frame_count);
    return NULL;
}

void dvz_client_thread(DvzClient* client, uint64_t frame_count)
{
    ANN(client);
    client->frame_count = frame_count;
    log_trace("start client thread");
    client->thread = dvz_thread(client_thread, (void*)client);
}



void dvz_client_stop(DvzClient* client)
{
    ANN(client);
    log_trace("request client stop");
    dvz_atomic_set(client->to_stop, 1);
}



void dvz_client_join(DvzClient* client)
{
    ANN(client);
    if (client->thread != NULL)
    {
        // Wait until the event loop is done or stopped.
        log_trace("joining on client thread");
        dvz_thread_join(client->thread);
        client->thread = NULL;
    }
}



void dvz_client_destroy(DvzClient* client)
{
    ANN(client);
    log_trace("destroy the client");

    // Delete all remaining windows.
    DvzContainerIterator iter = dvz_container_iterator(&client->windows);
    DvzWindow* window = NULL;
    while (iter.item != NULL)
    {
        window = (DvzWindow*)iter.item;
        ANN(window);

        if (dvz_obj_is_created(&window->obj))
        {
            // Emit a window delete event.
            dvz_client_event(
                client, (DvzClientEvent){
                            .type = DVZ_CLIENT_EVENT_WINDOW_DELETE,
                            .window_id = window->obj.id,
                        });
        }
        dvz_container_iter(&iter);
    }

    // NOTE: used to call a destruction callback registered by the presenter, to destroy the GUI
    // *before* the backend glfw is destroyed. This is because imgui requires glfw when
    // unregistering its input callbacks.
    dvz_client_event(client, (DvzClientEvent){.type = DVZ_CLIENT_EVENT_DESTROY});

    dvz_client_process(client);


    // Join the thread if any.
    dvz_client_stop(client);
    dvz_client_join(client);

    // Destroy the deq.
    dvz_deq_destroy(client->deq);

    CONTAINER_DESTROY_ITEMS(DvzWindow, client->windows, dvz_window_destroy)
    dvz_container_destroy(&client->windows);

    dvz_map_destroy(client->map);

    // TODO: stop background thread for async callbacks

    // NOTE: the host is responsible for terminating the backend.
    // backend_terminate(client->backend);

    dvz_atomic_destroy(client->to_stop);
    FREE(client);
    log_trace("client destroyed");
}
