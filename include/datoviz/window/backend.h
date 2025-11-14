/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Window backend                                                                               */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>

#include "datoviz/common/macros.h"
#include "datoviz/window/types.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzWindowBackend DvzWindowBackend;
typedef struct DvzWindowBackendProcs DvzWindowBackendProcs;

typedef bool (*DvzWindowBackendProbe)(DvzWindowBackend* backend, DvzWindowHost* host);
typedef bool (*DvzWindowBackendCreate)(
    DvzWindowBackend* backend, DvzWindow* window, const DvzWindowConfig* config);
typedef void (*DvzWindowBackendDestroy)(DvzWindowBackend* backend, DvzWindow* window);
typedef void (*DvzWindowBackendPoll)(DvzWindowBackend* backend, DvzWindowHost* host);
typedef void (*DvzWindowBackendRequestFrame)(DvzWindowBackend* backend, DvzWindow* window);



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzWindowBackendProcs
{
    DvzWindowBackendProbe probe;
    DvzWindowBackendCreate create;
    DvzWindowBackendDestroy destroy;
    DvzWindowBackendPoll poll;
    DvzWindowBackendRequestFrame request_frame;
};



struct DvzWindowBackend
{
    const char* name;
    DvzBackend type;
    void* user_data;
    DvzWindowBackendProcs procs;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Backend helpers                                                                              */
/*************************************************************************************************/

/**
 * Access the router attached to the window.
 *
 * @param window window that owns the router
 * @returns pointer to the router used for input emission
 */
DVZ_EXPORT DvzInputRouter* dvz_window_backend_router(DvzWindow* window);



/**
 * Access the surface description for mutation.
 *
 * @param window window whose surface is requested
 * @returns pointer to the surface owned by the window
 */
DVZ_EXPORT DvzWindowSurface* dvz_window_backend_surface(DvzWindow* window);



/**
 * Store a backend-specific handle on the window.
 *
 * @param window target window
 * @param handle native handle owned by the backend
 */
DVZ_EXPORT void dvz_window_backend_set_handle(DvzWindow* window, void* handle);



/**
 * Retrieve the backend-specific handle stored with the window.
 *
 * @param window window queried for the handle
 * @returns pointer previously set with dvz_window_backend_set_handle()
 */
DVZ_EXPORT void* dvz_window_backend_handle(const DvzWindow* window);



/**
 * Store additional backend data on the window.
 *
 * @param window window to mutate
 * @param payload opaque backend payload pointer
 */
DVZ_EXPORT void dvz_window_backend_set_payload(DvzWindow* window, void* payload);



/**
 * Retrieve backend payload associated with the window.
 *
 * @param window window queried for payload
 * @returns payload pointer or NULL
 */
DVZ_EXPORT void* dvz_window_backend_payload(const DvzWindow* window);



/**
 * Emit a resize event and refresh the cached surface state.
 *
 * @param window window whose router receives the event
 * @param framebuffer_width framebuffer width in pixels
 * @param framebuffer_height framebuffer height in pixels
 * @param window_width logical window width
 * @param window_height logical window height
 * @param content_scale_x horizontal content scale
 * @param content_scale_y vertical content scale
 */
DVZ_EXPORT void dvz_window_backend_emit_resize(
    DvzWindow* window, uint32_t framebuffer_width, uint32_t framebuffer_height,
    uint32_t window_width, uint32_t window_height, float content_scale_x, float content_scale_y);



/**
 * Emit a content scale event.
 *
 * @param window window whose router receives the event
 * @param content_scale_x horizontal content scale
 * @param content_scale_y vertical content scale
 */
DVZ_EXPORT void
dvz_window_backend_emit_scale(DvzWindow* window, float content_scale_x, float content_scale_y);



/*************************************************************************************************/
/*  Backend registration                                                                         */
/*************************************************************************************************/

/**
 * Register the built-in headless backend on the host.
 *
 * @param host host that should expose the headless backend
 */
DVZ_EXPORT void dvz_window_register_headless_backend(DvzWindowHost* host);



/**
 * Register the GLFW backend on the host.
 *
 * @param host host that should expose the GLFW backend
 */
DVZ_EXPORT void dvz_window_register_glfw_backend(DvzWindowHost* host);



/**
 * Register the Qt backend on the host (if provided by downstream code).
 *
 * @param host host that should expose the Qt backend
 */
DVZ_EXPORT void dvz_window_register_qt_backend(DvzWindowHost* host);



EXTERN_C_OFF
