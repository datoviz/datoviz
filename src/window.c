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

    // Create the window, depending on the backend.
    window.backend_window = dvz_backend_window(backend, width, height, flags);

    window.backend = backend;

    // Set the initial position.
    backend_get_window_position(&window, &window.xpos, &window.ypos);
    window._xpos = window.xpos;
    window._ypos = window.ypos;

    // Set the initial size.

    dvz_backend_get_window_size(&window, &window.width, &window.height);
    window._width = window.width;
    window._height = window.height;

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



void dvz_window_fullscreen(DvzWindow* window, bool fullscreen)
{
    if (window->is_fullscreen != fullscreen)
    {
        if (fullscreen)
        {
            // Save position and size.
            backend_get_window_position(window, &window->_xpos, &window->_ypos);
            backend_get_window_size(window, &window->_width, &window->_height);

            backend_set_fullscreen(window);

            // Update window struct with current values.
            window->is_fullscreen = true;
            backend_get_window_position(window, &window->xpos, &window->ypos);
            dvz_window_poll_size(window);
        }
        else
        {
            // Restore window from saved position and size.
            backend_set_window(
                window, window->_xpos, window->_ypos, window->_width, window->_height);

            // Update window struct with current values.
            window->is_fullscreen = false;
            backend_get_window_position(window, &window->xpos, &window->ypos);
            dvz_window_poll_size(window);
        }
    }
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
