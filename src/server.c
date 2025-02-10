/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Server                                                                                       */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "server.h"
#include "board.h"
#include "datoviz.h"
#include "datoviz_protocol.h"
#include "env_utils.h"
#include "host.h"
#include "keyboard.h"
#include "mouse.h"
#include "render_utils.h"
#include "renderer.h"



/*************************************************************************************************/
/*  Server functions                                                                             */
/*************************************************************************************************/

DvzServer* dvz_server(int flags)
{
    DvzServer* server = (DvzServer*)calloc(1, sizeof(DvzServer*));
    ANN(server);

    server->host = dvz_host(DVZ_BACKEND_OFFSCREEN);
    ANN(server->host);
    dvz_host_create(server->host);

    server->gpu = make_gpu(server->host);
    ANN(server->gpu);

    server->rd = dvz_renderer(server->gpu, flags);
    ANN(server->rd);

    server->mouse = dvz_mouse();
    server->keyboard = dvz_keyboard();

    return server;
}



void dvz_server_submit(DvzServer* server, DvzBatch* batch)
{
    ANN(server);
    ANN(batch);

    uint32_t count = dvz_batch_size(batch);
    ASSERT(count > 0);

    DvzRequest* requests = dvz_batch_requests(batch);
    ANN(requests);

    DvzRenderer* rd = server->rd;
    ANN(rd);

    // Submit the pending requests to the renderer.
    log_debug("server processes %d requests", count);

    // Go through all pending requests.
    for (uint32_t i = 0; i < count; i++)
    {
        // Process each request immediately in the renderer.
        dvz_renderer_request(rd, requests[i]);
    }
}



void dvz_server_mouse(DvzServer* server, DvzId canvas_id, DvzMouseEvent ev)
{
    ANN(server);
    ANN(server->mouse);
    dvz_mouse_event(server->mouse, ev);
}



void dvz_server_keyboard(DvzServer* server, DvzId canvas_id, DvzKeyboardEvent ev)
{
    ANN(server);
    ANN(server->keyboard);
    dvz_keyboard_event(server->keyboard, ev);
}



void dvz_server_resize(DvzServer* server, DvzId canvas_id, uint32_t width, uint32_t height)
{
    ANN(server);
    // NOTE: may not be needed if using a BOARD_RESIZE request, but would need to merge board and
    // canvas first.
}



// NOTE: the caller MUST NOT free the output.
uint8_t* dvz_server_grab(DvzServer* server, DvzId canvas_id, int flags)
{
    ANN(server);
    DvzRenderer* rd = server->rd;
    ANN(rd);

    DvzCanvas* canvas = dvz_renderer_canvas(server->rd, canvas_id);
    ANN(canvas);
    ASSERT(dvz_obj_is_created(&canvas->obj));

    // Trigger an update.
    dvz_cmd_submit_sync(&canvas->cmds, DVZ_DEFAULT_QUEUE_RENDER);

    // Grab the image.
    DvzSize size = 0;
    uint8_t* rgb = dvz_renderer_image(rd, canvas_id, &size, NULL);

    // Return it (pointer should be managed by the renderer).
    return rgb;
}



void dvz_server_destroy(DvzServer* server)
{
    ANN(server); //

    dvz_mouse_destroy(server->mouse);
    dvz_keyboard_destroy(server->keyboard);
    dvz_renderer_destroy(server->rd);
    dvz_gpu_destroy(server->gpu);
    dvz_host_destroy(server->host);

    FREE(server);
}
