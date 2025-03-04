/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing client input                                                                         */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_client_input.h"
#include "backend.h"
#include "client.h"
#include "client_input.h"
#include "datoviz_types.h"
#include "keyboard.h"
#include "mouse.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"
#include "time.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Client input tests                                                                           */
/*************************************************************************************************/

static void _on_mouse(DvzClient* client, DvzClientEvent ev)
{
    ANN(client);
    log_debug("mouse event %d", ev.content.m.type);
}

static void _on_keyboard(DvzClient* client, DvzClientEvent ev)
{
    ANN(client);
    log_debug("keyboard event %d", ev.content.k.type);

    // Test: close on ESC.
    if (ev.content.k.key == DVZ_KEY_ESCAPE)
    {
        DvzClientEvent cev = {
            .window_id = ev.window_id,
            .type = DVZ_CLIENT_EVENT_WINDOW_DELETE,
        };
        dvz_client_event(client, cev);
    }
}

int test_client_input(TstSuite* suite, TstItem* tstitem)
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
        .content.w.screen_width = 800,
        .content.w.screen_height = 600};
    dvz_client_event(client, ev);

    // Dequeue and process all pending events.
    dvz_client_process(client);

    dvz_client_run(client, N_FRAMES);

    dvz_client_destroy(client);
    return 0;
}
