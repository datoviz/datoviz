/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Client input                                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "client_input.h"
#include "backend.h"
#include "client.h"
#include "client_utils.h"
#include "common.h"
#include "datoviz_types.h"
#include "fifo.h"
#include "input.h"
#include "keyboard.h"
#include "mouse.h"
#include "window.h"



/*************************************************************************************************/
/*  Input callbacks                                                                              */
/*************************************************************************************************/

static void _on_mouse(DvzMouse* mouse, DvzMouseEvent* ev)
{
    ANN(mouse);

    DvzWindow* window = (DvzWindow*)ev->user_data;
    ANN(window);

    // Do not process input events if the window is being captured by ImGui.
    if (window->is_captured)
        return;

    DvzClient* client = window->client;
    ANN(client);

    DvzClientEvent cev = {0};
    cev.type = DVZ_CLIENT_EVENT_MOUSE;
    cev.window_id = window->obj.id;
    cev.content.m = *ev;

    // Content scale, used for mouse event localization.
    if (window->width > 0)
        ev->content_scale = cev.content_scale = window->framebuffer_width / window->width;

    dvz_client_event(client, cev);
}



static void _on_keyboard(DvzKeyboard* keyboard, DvzKeyboardEvent* ev)
{
    ANN(keyboard);

    DvzWindow* window = (DvzWindow*)ev->user_data;
    ANN(window);

    // Do not process input events if the window is being captured by ImGui.
    if (window->is_captured)
        return;

    DvzClient* client = window->client;
    ANN(client);

    DvzClientEvent cev = {0};
    cev.type = DVZ_CLIENT_EVENT_KEYBOARD;
    cev.window_id = window->obj.id;
    cev.content.k = *ev;
    dvz_client_event(client, cev);
}



/*************************************************************************************************/
/*  Window creation client callback                                                              */
/*************************************************************************************************/

static void _create_window_input(DvzDeq* deq, void* item, void* user_data)
{
    ANN(deq);

    DvzClient* client = (DvzClient*)user_data;
    ANN(client);

    DvzClientEvent* ev = (DvzClientEvent*)item;
    ANN(ev);
    ASSERT(ev->type == DVZ_CLIENT_EVENT_WINDOW_CREATE);

    DvzId id = ev->window_id;

    // Retrieve the created window with id2window().
    DvzWindow* window = id2window(client, id);
    ANN(window);

    dvz_window_input(window);
}



static void _delete_window_input(DvzDeq* deq, void* item, void* user_data)
{
    ANN(deq);

    DvzClient* client = (DvzClient*)user_data;
    ANN(client);

    DvzClientEvent* ev = (DvzClientEvent*)item;
    ANN(ev);
    ASSERT(ev->type == DVZ_CLIENT_EVENT_WINDOW_DELETE);

    DvzId id = ev->window_id;

    // Retrieve the window with id2window().
    DvzWindow* window = id2window(client, id);
    if (window != NULL && window->input != NULL)
        dvz_input_destroy(window->input);
}



/*************************************************************************************************/
/*  Client input functions                                                                       */
/*************************************************************************************************/

void dvz_window_input(DvzWindow* window)
{
    ANN(window);

    // Create the input and associate it to the window.
    // This call uses the GLFW callback functions to update the Mouse and Keyboard state machines.
    window->input = dvz_input(window);

    // Route all backend events from all windows to the unified client queue.

    // Register mouse callbacks that enqueue events to the client queue.
    DvzMouse* mouse = dvz_input_mouse(window->input);
    dvz_mouse_callback(mouse, DVZ_MOUSE_EVENT_ALL, _on_mouse, window);

    // Register keyboard callbacks that enqueue events to the client queue.
    DvzKeyboard* keyboard = dvz_input_keyboard(window->input);
    dvz_keyboard_callback(keyboard, DVZ_KEYBOARD_EVENT_PRESS, _on_keyboard, window);
    dvz_keyboard_callback(keyboard, DVZ_KEYBOARD_EVENT_RELEASE, _on_keyboard, window);
    dvz_keyboard_callback(keyboard, DVZ_KEYBOARD_EVENT_REPEAT, _on_keyboard, window);
}



void dvz_client_input(DvzClient* client)
{
    ANN(client);

    // Register a window_create callback that is called after the default one.
    dvz_deq_callback(
        client->deq, 0, (int)DVZ_CLIENT_EVENT_WINDOW_CREATE, _create_window_input, client);

    // Delete the input upon window deletion.
    // NOTE: DVZ_CLIENT_EVENT_WINDOW_DELETE is marked as a reverse callback event, so this callback
    // deleting the input will be called *before* the window destruction.
    dvz_deq_callback(
        client->deq, 0, (int)DVZ_CLIENT_EVENT_WINDOW_DELETE, _delete_window_input, client);
}
