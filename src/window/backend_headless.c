/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Headless backend                                                                             */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_assertions.h"
#include "datoviz/window.h"
#include "datoviz/window/backend.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static bool _headless_probe(DvzWindowBackend* backend, DvzWindowHost* host)
{
    (void)backend;
    (void)host;
    return true;
}



static bool
_headless_create(DvzWindowBackend* backend, DvzWindow* window, const DvzWindowConfig* config)
{
    (void)backend;
    ANN(window);
    ANN(config);
    DvzWindowSurface* surface = dvz_window_backend_surface(window);
    ANN(surface);
    surface->extent.width = config->width;
    surface->extent.height = config->height;
    surface->scale_x = config->user_scale;
    surface->scale_y = config->user_scale;
    dvz_window_backend_emit_resize(
        window, config->width, config->height, config->width, config->height, config->user_scale,
        config->user_scale);
    return true;
}



static void _headless_destroy(DvzWindowBackend* backend, DvzWindow* window)
{
    (void)backend;
    (void)window;
}



/**
 * Register the built-in headless backend.
 */
void dvz_window_register_headless_backend(DvzWindowHost* host)
{
    ANN(host);
    DvzWindowBackend backend = {
        .name = "headless",
        .type = DVZ_BACKEND_OFFSCREEN,
        .user_data = NULL,
        .procs =
            {
                .probe = _headless_probe,
                .create = _headless_create,
                .destroy = _headless_destroy,
                .poll = NULL,
                .request_frame = NULL,
            },
    };
    dvz_window_host_register_backend(host, &backend);
}
