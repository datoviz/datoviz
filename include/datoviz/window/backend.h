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
 */
DVZ_EXPORT DvzInputRouter* dvz_window_backend_router(DvzWindow* window);



/**
 * Access the surface description for mutation.
 */
DVZ_EXPORT DvzWindowSurface* dvz_window_backend_surface(DvzWindow* window);



/**
 * Store a backend-specific handle on the window.
 */
DVZ_EXPORT void dvz_window_backend_set_handle(DvzWindow* window, void* handle);



/**
 * Retrieve the backend-specific handle stored with the window.
 */
DVZ_EXPORT void* dvz_window_backend_handle(const DvzWindow* window);



/**
 * Store additional backend data on the window.
 */
DVZ_EXPORT void dvz_window_backend_set_payload(DvzWindow* window, void* payload);



/**
 * Retrieve backend payload associated with the window.
 */
DVZ_EXPORT void* dvz_window_backend_payload(const DvzWindow* window);



/**
 * Emit a resize event and refresh the cached surface state.
 */
DVZ_EXPORT void dvz_window_backend_emit_resize(
    DvzWindow* window, uint32_t framebuffer_width, uint32_t framebuffer_height,
    uint32_t window_width, uint32_t window_height, float content_scale_x, float content_scale_y);



/**
 * Emit a content scale event.
 */
DVZ_EXPORT void
dvz_window_backend_emit_scale(DvzWindow* window, float content_scale_x, float content_scale_y);



/*************************************************************************************************/
/*  Backend registration                                                                         */
/*************************************************************************************************/

DVZ_EXPORT void dvz_window_register_headless_backend(DvzWindowHost* host);
DVZ_EXPORT void dvz_window_register_glfw_backend(DvzWindowHost* host);
DVZ_EXPORT void dvz_window_register_qt_backend(DvzWindowHost* host);



EXTERN_C_OFF
