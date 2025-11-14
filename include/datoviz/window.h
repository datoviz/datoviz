/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Window API                                                                                   */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>

#include "datoviz/input/router.h"
#include "datoviz/window/backend.h"
#include "datoviz/window/types.h"



EXTERN_C_ON

/*************************************************************************************************/
/*  Window host                                                                                  */
/*************************************************************************************************/

/**
 * Create a window host that stores available backends and owned windows.
 *
 * @returns pointer to the newly allocated host
 */
DVZ_EXPORT DvzWindowHost* dvz_window_host(void);



/**
 * Destroy a window host and all windows associated with it.
 *
 * @param host host returned by dvz_window_host()
 */
DVZ_EXPORT void dvz_window_host_destroy(DvzWindowHost* host);



/**
 * Register a backend so it can be used during window creation.
 *
 * @param host host that receives the backend
 * @param backend backend descriptor containing the callback table
 */
DVZ_EXPORT void dvz_window_host_register_backend(
    DvzWindowHost* host, const DvzWindowBackend* backend);



/**
 * Poll every registered backend for events.
 *
 * @param host host whose backends should be polled
 */
DVZ_EXPORT void dvz_window_host_poll(DvzWindowHost* host);



/**
 * Request a frame for the given window.
 *
 * @param host hosting the window
 * @param window window requiring a new frame
 */
DVZ_EXPORT void dvz_window_host_request_frame(DvzWindowHost* host, DvzWindow* window);



/*************************************************************************************************/
/*  Window instances                                                                             */
/*************************************************************************************************/

/**
 * Return the default configuration used when callers pass NULL.
 *
 * @returns sensible defaults (width, height, title, scale)
 */
DVZ_EXPORT DvzWindowConfig dvz_window_default_config(void);



/**
 * Create a window using the preferred backend.
 *
 * @param host host that manages the window
 * @param backend preferred backend (falls back to headless)
 * @param config window configuration or NULL for defaults
 * @returns pointer to the created window or NULL on failure
 */
DVZ_EXPORT DvzWindow* dvz_window_create(
    DvzWindowHost* host, DvzBackend backend, const DvzWindowConfig* config);



/**
 * Destroy a window and free backend resources.
 *
 * @param window window returned by dvz_window_create()
 */
DVZ_EXPORT void dvz_window_destroy(DvzWindow* window);



/**
 * Return the cached surface information for the window.
 *
 * @param window window to query
 * @returns pointer to the surface data owned by the window
 */
DVZ_EXPORT const DvzWindowSurface* dvz_window_surface(const DvzWindow* window);



/**
 * Retrieve the router used to emit input events for the window.
 *
 * @param window window owning the router
 * @returns input router pointer
 */
DVZ_EXPORT DvzInputRouter* dvz_window_router(DvzWindow* window);



/**
 * Store an opaque user data pointer on the window.
 *
 * @param window destination window
 * @param user_data pointer copied verbatim
 */
DVZ_EXPORT void dvz_window_set_user_data(DvzWindow* window, void* user_data);



/**
 * Read the user data pointer previously stored on a window.
 *
 * @param window window queried for user data
 * @returns pointer passed to dvz_window_set_user_data()
 */
DVZ_EXPORT void* dvz_window_user_data(const DvzWindow* window);



/**
 * Check whether a pending frame has yet to be processed.
 *
 * @param window window to query
 * @returns true if dvz_window_host_request_frame() is still outstanding
 */
DVZ_EXPORT bool dvz_window_frame_pending(const DvzWindow* window);



/**
 * Return the backend currently serving the window.
 *
 * @param window window whose backend is requested
 * @returns backend identifier (GLFW, OFFSCREEN, etc.)
 */
DVZ_EXPORT DvzBackend dvz_window_backend_type(const DvzWindow* window);



EXTERN_C_OFF
