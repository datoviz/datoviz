/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Window API                                                                                   */
/*************************************************************************************************/

#include "window.h"
#include "backend.h"
#include "common.h"



/*************************************************************************************************/
/*  Window                                                                                       */
/*************************************************************************************************/

DvzWindow dvz_window(DvzBackend backend, uint32_t width, uint32_t height, int flags)
{
    ASSERT(backend != DVZ_BACKEND_NONE);
    ASSERT(width > 0);
    ASSERT(height > 0);

    DvzWindow window = {0};
    dvz_obj_init(&window.obj);
    window.obj.type = DVZ_OBJECT_TYPE_WINDOW;

    window.width = width;
    window.height = height;
    window.backend = backend;

    // Create the window, depending on the backend.
    window.backend_window = dvz_backend_window(backend, width, height, flags);

    // Set the initial size.
    dvz_backend_get_window_size(&window, &window.width, &window.height);

    // NOTE: poll the framebuffer size
    dvz_backend_get_framebuffer_size(
        &window, &window.framebuffer_width, &window.framebuffer_height);

    dvz_obj_created(&window.obj);
    return window;
}



void dvz_window_poll_size(DvzWindow* window)
{
    ANN(window);
    dvz_backend_get_window_size(window, &window->width, &window->height);
    dvz_backend_get_framebuffer_size(
        window, &window->framebuffer_width, &window->framebuffer_height);
}



void dvz_window_set_size(DvzWindow* window, uint32_t width, uint32_t height)
{
    ANN(window);
    dvz_backend_set_window_size(window, width, height);
    dvz_backend_get_window_size(window, &window->width, &window->height);
}



void dvz_window_destroy(DvzWindow* window)
{
    if (window == NULL || window->obj.status == DVZ_OBJECT_STATUS_DESTROYED)
    {
        log_trace("skip destruction of already-destroyed window");
        return;
    }
    ANN(window);

    dvz_backend_window_clear_callbacks(window->backend, window->backend_window);

    log_debug("destroy the window");
    dvz_backend_window_destroy(window->backend, window->backend_window);
    dvz_obj_destroyed(&window->obj);
}
