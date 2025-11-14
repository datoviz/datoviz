/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Qt backend (stub)                                                                            */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_assertions.h"
#include "_log.h"
#include "datoviz/window.h"
#include "datoviz/window/backend.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static bool _qt_probe(DvzWindowBackend* backend, DvzWindowHost* host)
{
    (void)backend;
    (void)host;
    log_debug("Qt backend not implemented yet");
    return false;
}



static bool _qt_create(DvzWindowBackend* backend, DvzWindow* window, const DvzWindowConfig* config)
{
    (void)backend;
    (void)window;
    (void)config;
    return false;
}



static void _qt_destroy(DvzWindowBackend* backend, DvzWindow* window)
{
    (void)backend;
    (void)window;
}



/**
 * Register the Qt backend placeholder.
 */
void dvz_window_register_qt_backend(DvzWindowHost* host)
{
    ANN(host);
    DvzWindowBackend backend = {
        .name = "qt",
        .type = DVZ_BACKEND_QT,
        .user_data = NULL,
        .procs =
            {
                .probe = _qt_probe,
                .create = _qt_create,
                .destroy = _qt_destroy,
                .poll = NULL,
                .request_frame = NULL,
            },
    };
    dvz_window_host_register_backend(host, &backend);
}
