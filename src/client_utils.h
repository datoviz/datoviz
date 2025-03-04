/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

#ifndef DVZ_HEADER_CLIENT_UTILS
#define DVZ_HEADER_CLIENT_UTILS



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "backend.h"
#include "client.h"
#include "common.h"
#include "window.h"



/*************************************************************************************************/
/*  Client utils                                                                                 */
/*************************************************************************************************/

static DvzWindow* id2window(DvzClient* client, DvzId id)
{
    ANN(client);
    DvzWindow* window = dvz_map_get(client->map, id);
    return window;
}



static DvzId window2id(DvzWindow* window)
{
    ANN(window);
    return (DvzId)window->obj.id;
}



static DvzWindow*
create_client_window(DvzClient* client, DvzId id, uint32_t width, uint32_t height, int flags)
{
    ANN(client);
    ASSERT(width > 0);
    ASSERT(height > 0);
    ASSERT(id != DVZ_ID_NONE);

    DvzWindow* window = (DvzWindow*)dvz_container_alloc(&client->windows);
    *window = dvz_window(client->backend, width, height, flags);

    // Register the window id.
    window->obj.id = (uint64_t)id;
    window->client = client;
    dvz_map_add(client->map, id, DVZ_OBJECT_TYPE_WINDOW, window);

    return window;
}



static void delete_client_window(DvzClient* client, DvzId id)
{
    ANN(client);
    ASSERT(id != DVZ_ID_NONE);

    DvzWindow* window = id2window(client, id);
    if (window == NULL)
    {
        log_warn("window 0x%" PRIx64 " not found", id);
        return;
    }
    ANN(window);

    dvz_map_remove(client->map, id);
    dvz_window_destroy(window);
}



// static void request_delete_windows(DvzClient* client)
// {
//     // Emit a request_delete event for all remaining windows.
//     ANN(client);

//     // Loop over the windows.
//     DvzContainerIterator iter = dvz_container_iterator(&client->windows);
//     DvzWindow* window = NULL;

//     // Request delete event.
//     DvzClientEvent ev = {0};
//     ev.type = DVZ_CLIENT_EVENT_WINDOW_REQUEST_DELETE;

//     while (iter.item != NULL)
//     {
//         window = (DvzWindow*)iter.item;
//         if (window != NULL)
//         {
//             // Emit a request delete event to all windows.
//             ev.window_id = window->obj.id;
//             log_trace("emit request_delete for window 0x%" PRIx64, ev.window_id);
//             dvz_client_event(client, ev);
//         }
//         dvz_container_iter(&iter);
//     }
// }



/*************************************************************************************************/
/*  Callback functions                                                                           */
/*************************************************************************************************/

static void _callback_window_create(DvzDeq* deq, void* item, void* user_data)
{
    ANN(deq);

    ANN(user_data);
    DvzClient* client = (DvzClient*)user_data;

    ANN(item);
    DvzClientEvent* ev = (DvzClientEvent*)item;
    ASSERT(ev->type == DVZ_CLIENT_EVENT_WINDOW_CREATE);

    uint32_t width = ev->content.w.screen_width;
    uint32_t height = ev->content.w.screen_height;

    log_debug("client: create window #%d (%dx%d)", ev->window_id, width, height);

    create_client_window(client, ev->window_id, width, height, ev->content.w.flags);
}



static void _callback_window_delete(DvzDeq* deq, void* item, void* user_data)
{
    ANN(deq);

    ANN(user_data);
    DvzClient* client = (DvzClient*)user_data;

    ANN(item);
    DvzClientEvent* ev = (DvzClientEvent*)item;
    ASSERT(ev->type == DVZ_CLIENT_EVENT_WINDOW_DELETE);

    log_debug("client: delete window 0x%" PRIx64, ev->window_id);

    delete_client_window(client, ev->window_id);
}



#endif
