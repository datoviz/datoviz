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
typedef struct DvzWindowExternalSurfaceInfo DvzWindowExternalSurfaceInfo;

typedef bool (*DvzWindowBackendProbe)(DvzWindowBackend* backend, DvzWindowHost* host);
typedef bool (*DvzWindowBackendCreate)(
    DvzWindowBackend* backend, DvzWindow* window, const DvzWindowConfig* config);
typedef void (*DvzWindowBackendDestroy)(DvzWindowBackend* backend, DvzWindow* window);
typedef void (*DvzWindowBackendPoll)(DvzWindowBackend* backend, DvzWindowHost* host);
typedef void (*DvzWindowBackendRequestFrame)(DvzWindowBackend* backend, DvzWindow* window);
typedef bool (*DvzWindowBackendShouldClose)(
    const DvzWindowBackend* backend, const DvzWindow* window);
typedef uint32_t (*DvzWindowBackendRequiredExtensionCount)(
    DvzWindowBackend* backend, DvzWindowHost* host);
typedef const char* (*DvzWindowBackendRequiredExtensionAt)(
    DvzWindowBackend* backend, DvzWindowHost* host, uint32_t index);



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
    DvzWindowBackendShouldClose should_close;
    DvzWindowBackendRequiredExtensionCount required_extension_count;
    DvzWindowBackendRequiredExtensionAt required_extension_at;
};



struct DvzWindowBackend
{
    const char* name;
    DvzBackend type;
    void* user_data;
    DvzWindowBackendProcs procs;
};



struct DvzWindowExternalSurfaceInfo
{
    VkInstance instance;
    VkSurfaceKHR surface;
    VkExtent2D extent;
    float scale_x;
    float scale_y;
    bool owned_by_datoviz;
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
 * Initialize GLFW so instance extensions can be queried before a window exists.
 *
 * @returns true when GLFW is initialized, false when the backend is unavailable
 */
DVZ_EXPORT bool dvz_window_glfw_init(void);



/**
 * Configure the required Vulkan instance extensions for the wrap backend.
 *
 * @param host host that owns the wrap backend state
 * @param count number of extension names passed in extensions
 * @param extensions extension-name array or NULL when count is zero
 * @returns 0 on success, -1 on invalid input or allocation failure
 */
DVZ_EXPORT int
dvz_window_wrap_set_required_extensions(DvzWindowHost* host, uint32_t count, const char* const* extensions);



/**
 * Attach an externally-created Vulkan surface to a wrap window.
 *
 * @param window target window created with DVZ_BACKEND_WRAP
 * @param info external surface description
 * @returns 0 on success, -1 on invalid args/backend mismatch/invalid handles
 */
DVZ_EXPORT int
dvz_window_wrap_attach_surface(DvzWindow* window, const DvzWindowExternalSurfaceInfo* info);



/**
 * Update the externally-managed Vulkan surface associated with a wrap window.
 *
 * @param window target window created with DVZ_BACKEND_WRAP
 * @param info external surface description
 * @returns 0 on success, -1 on invalid args/backend mismatch/rejected update
 */
DVZ_EXPORT int
dvz_window_wrap_update_surface(DvzWindow* window, const DvzWindowExternalSurfaceInfo* info);



/**
 * Detach the external Vulkan surface from a wrap window.
 *
 * @param window target window created with DVZ_BACKEND_WRAP
 */
DVZ_EXPORT void dvz_window_wrap_detach_surface(DvzWindow* window);



/**
 * Query the required Vulkan instance extension count for a backend.
 *
 * @param host host that contains the backend registry
 * @param backend backend to query
 * @returns required extension count, or 0 on unavailable backend
 */
DVZ_EXPORT uint32_t
dvz_window_host_required_extension_count(DvzWindowHost* host, DvzBackend backend);



/**
 * Query backend-required Vulkan instance extension names.
 *
 * @param host host that contains the backend registry
 * @param backend backend to query
 * @param capacity maximum number of names that can be written in out_extensions
 * @param out_extensions output array of extension names
 * @returns number of names written, or -1 on invalid input/backend unavailable
 */
DVZ_EXPORT int dvz_window_host_required_extensions(
    DvzWindowHost* host, DvzBackend backend, uint32_t capacity, const char** out_extensions);



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
 * Register the built-in wrap backend on the host.
 *
 * @param host host that should expose the wrap backend
 */
DVZ_EXPORT void dvz_window_register_wrap_backend(DvzWindowHost* host);



/**
 * Register the Qt backend on the host (if provided by downstream code).
 *
 * @param host host that should expose the Qt backend
 */
DVZ_EXPORT void dvz_window_register_qt_backend(DvzWindowHost* host);



EXTERN_C_OFF
