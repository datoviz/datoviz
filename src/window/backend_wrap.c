/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Wrap backend                                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "datoviz/window.h"
#include "window_internal.h"

#include <volk.h>



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return whether a Vulkan instance/surface pair is valid for wrap backend operations.
 *
 * @param instance Vulkan instance handle
 * @param surface Vulkan surface handle
 * @return true when both handles are null or both are non-null
 */
static bool _wrap_surface_tuple_valid(VkInstance instance, VkSurfaceKHR surface)
{
    bool instance_set = instance != VK_NULL_HANDLE;
    bool surface_set = surface != VK_NULL_HANDLE;
    return instance_set == surface_set;
}



/**
 * Return the wrap backend state associated with a backend descriptor.
 *
 * @param backend backend descriptor whose user_data stores wrap state
 * @return pointer to wrap backend state, or NULL
 */
static DvzWindowWrapBackendState* _wrap_state_from_backend(DvzWindowBackend* backend)
{
    if (backend == NULL || backend->type != DVZ_BACKEND_WRAP)
        return NULL;
    return (DvzWindowWrapBackendState*)backend->user_data;
}



/**
 * Return the wrap backend state associated with a window host.
 *
 * @param host host that owns wrap backend state
 * @return pointer to wrap backend state, or NULL
 */
static DvzWindowWrapBackendState* _wrap_state_from_host(DvzWindowHost* host)
{
    if (host == NULL)
        return NULL;
    return &host->wrap_state;
}



/**
 * Release all extension strings stored in wrap backend state.
 *
 * @param state wrap backend state to clear
 */
static void _wrap_extensions_clear(DvzWindowWrapBackendState* state)
{
    if (state == NULL || state->extensions == NULL)
    {
        if (state != NULL)
            state->extension_count = 0;
        return;
    }
    for (uint32_t i = 0; i < state->extension_count; i++)
    {
        dvz_free(state->extensions[i]);
    }
    dvz_free(state->extensions);
    state->extensions = NULL;
    state->extension_count = 0;
}



/**
 * Copy extension strings into wrap backend state.
 *
 * @param state wrap backend state to populate
 * @param count number of extension names to copy
 * @param extensions input extension-name array
 * @return 0 on success, -1 on allocation/input error
 */
static int
_wrap_extensions_copy(DvzWindowWrapBackendState* state, uint32_t count, const char* const* extensions)
{
    ANN(state);
    if (count == 0)
    {
        _wrap_extensions_clear(state);
        return 0;
    }
    if (extensions == NULL)
        return -1;

    char** copied = dvz_calloc(count, sizeof(char*));
    if (copied == NULL)
        return -1;

    for (uint32_t i = 0; i < count; i++)
    {
        if (extensions[i] == NULL)
        {
            for (uint32_t j = 0; j < i; j++)
                dvz_free(copied[j]);
            dvz_free(copied);
            return -1;
        }
        copied[i] = dvz_strdup(extensions[i]);
        if (copied[i] == NULL)
        {
            for (uint32_t j = 0; j < i; j++)
                dvz_free(copied[j]);
            dvz_free(copied);
            return -1;
        }
    }

    _wrap_extensions_clear(state);
    state->extensions = copied;
    state->extension_count = count;
    return 0;
}



/**
 * Destroy a Vulkan surface only when wrap backend owns it.
 *
 * @param window wrap window whose surface may need destruction
 */
static void _wrap_destroy_owned_surface(DvzWindow* window)
{
    ANN(window);
    DvzWindowSurface* surface = dvz_window_backend_surface(window);
    ANN(surface);
    if (!window->backend_owns_surface)
        return;
    if (surface->instance == VK_NULL_HANDLE || surface->surface == VK_NULL_HANDLE)
        return;
    vkDestroySurfaceKHR(surface->instance, surface->surface, NULL);
}



/**
 * Apply an external-surface update to a wrap window.
 *
 * @param window wrap window receiving the update
 * @param info external surface description
 * @param allow_null_surface whether null instance/surface tuple is accepted
 * @return 0 on success, -1 on invalid input
 */
static int
_wrap_apply_surface_update(DvzWindow* window, const DvzWindowExternalSurfaceInfo* info, bool allow_null_surface)
{
    ANN(window);
    ANN(info);
    if (!_wrap_surface_tuple_valid(info->instance, info->surface))
        return -1;
    if (!allow_null_surface && info->surface == VK_NULL_HANDLE)
        return -1;

    _wrap_destroy_owned_surface(window);

    DvzWindowSurface* surface = dvz_window_backend_surface(window);
    ANN(surface);
    surface->instance = info->instance;
    surface->surface = info->surface;
    surface->extent = info->extent;
    surface->scale_x = info->scale_x;
    surface->scale_y = info->scale_y;
    window->backend_owns_surface = info->owned_by_datoviz;

    dvz_window_backend_emit_resize(
        window, info->extent.width, info->extent.height, info->extent.width, info->extent.height,
        info->scale_x, info->scale_y);
    return 0;
}



/**
 * Probe wrap backend availability.
 *
 * @param backend wrap backend descriptor
 * @param host host owning backend slots
 * @return always true because wrap backend has no platform dependency
 */
static bool _wrap_probe(DvzWindowBackend* backend, DvzWindowHost* host)
{
    (void)backend;
    (void)host;
    return true;
}



/**
 * Initialize a wrap window with logical size and no native surface.
 *
 * @param backend wrap backend descriptor
 * @param window wrap window to initialize
 * @param config window configuration used for initial extent/scale
 * @return true on successful setup
 */
static bool _wrap_create(DvzWindowBackend* backend, DvzWindow* window, const DvzWindowConfig* config)
{
    (void)backend;
    ANN(window);
    ANN(config);
    window->backend_owns_surface = false;
    DvzWindowSurface* surface = dvz_window_backend_surface(window);
    ANN(surface);
    surface->instance = VK_NULL_HANDLE;
    surface->surface = VK_NULL_HANDLE;
    surface->extent.width = config->width;
    surface->extent.height = config->height;
    surface->scale_x = config->user_scale;
    surface->scale_y = config->user_scale;
    dvz_window_backend_emit_resize(
        window, config->width, config->height, config->width, config->height, config->user_scale,
        config->user_scale);
    return true;
}



/**
 * Destroy resources associated with a wrap window.
 *
 * @param backend wrap backend descriptor
 * @param window wrap window to cleanup
 */
static void _wrap_destroy(DvzWindowBackend* backend, DvzWindow* window)
{
    (void)backend;
    if (window == NULL)
        return;
    _wrap_destroy_owned_surface(window);
    DvzWindowSurface* surface = dvz_window_backend_surface(window);
    ANN(surface);
    surface->instance = VK_NULL_HANDLE;
    surface->surface = VK_NULL_HANDLE;
    window->backend_owns_surface = false;
}



/**
 * Return the number of required Vulkan instance extensions configured on wrap backend.
 *
 * @param backend wrap backend descriptor
 * @param host window host querying extension requirements
 * @return number of configured extension names
 */
static uint32_t _wrap_required_extension_count(DvzWindowBackend* backend, DvzWindowHost* host)
{
    (void)host;
    DvzWindowWrapBackendState* state = _wrap_state_from_backend(backend);
    return state != NULL ? state->extension_count : 0;
}



/**
 * Return one required Vulkan instance extension configured on wrap backend.
 *
 * @param backend wrap backend descriptor
 * @param host window host querying extension requirements
 * @param index extension index to resolve
 * @return extension name at index, or NULL when index is invalid
 */
static const char*
_wrap_required_extension_at(DvzWindowBackend* backend, DvzWindowHost* host, uint32_t index)
{
    (void)host;
    DvzWindowWrapBackendState* state = _wrap_state_from_backend(backend);
    if (state == NULL || index >= state->extension_count)
        return NULL;
    return state->extensions[index];
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Register the built-in wrap backend.
 *
 * @param host host that should expose the wrap backend
 */
void dvz_window_register_wrap_backend(DvzWindowHost* host)
{
    ANN(host);
    DvzWindowBackend backend = {
        .name = "wrap",
        .type = DVZ_BACKEND_WRAP,
        .user_data = _wrap_state_from_host(host),
        .procs =
            {
                .probe = _wrap_probe,
                .create = _wrap_create,
                .destroy = _wrap_destroy,
                .poll = NULL,
                .request_frame = NULL,
                .required_extension_count = _wrap_required_extension_count,
                .required_extension_at = _wrap_required_extension_at,
            },
    };
    dvz_window_host_register_backend(host, &backend);
}



/**
 * Configure the required Vulkan instance extensions for the wrap backend.
 *
 * @param host host that owns the wrap backend state
 * @param count number of extension names passed in extensions
 * @param extensions extension-name array or NULL when count is zero
 * @return 0 on success, -1 on invalid input or allocation failure
 */
int dvz_window_wrap_set_required_extensions(
    DvzWindowHost* host, uint32_t count, const char* const* extensions)
{
    DvzWindowWrapBackendState* state = _wrap_state_from_host(host);
    if (state == NULL)
        return -1;
    if (count > 0 && extensions == NULL)
        return -1;
    return _wrap_extensions_copy(state, count, extensions);
}



/**
 * Attach an externally-created Vulkan surface to a wrap window.
 *
 * @param window target window created with DVZ_BACKEND_WRAP
 * @param info external surface description
 * @return 0 on success, -1 on invalid args/backend mismatch/invalid handles
 */
int dvz_window_wrap_attach_surface(DvzWindow* window, const DvzWindowExternalSurfaceInfo* info)
{
    if (window == NULL || info == NULL)
        return -1;
    if (dvz_window_backend_type(window) != DVZ_BACKEND_WRAP)
        return -1;
    return _wrap_apply_surface_update(window, info, false);
}



/**
 * Update the externally-managed Vulkan surface associated with a wrap window.
 *
 * @param window target window created with DVZ_BACKEND_WRAP
 * @param info external surface description
 * @return 0 on success, -1 on invalid args/backend mismatch/rejected update
 */
int dvz_window_wrap_update_surface(DvzWindow* window, const DvzWindowExternalSurfaceInfo* info)
{
    if (window == NULL || info == NULL)
        return -1;
    if (dvz_window_backend_type(window) != DVZ_BACKEND_WRAP)
        return -1;
    return _wrap_apply_surface_update(window, info, true);
}



/**
 * Detach the external Vulkan surface from a wrap window.
 *
 * @param window target window created with DVZ_BACKEND_WRAP
 */
void dvz_window_wrap_detach_surface(DvzWindow* window)
{
    if (window == NULL)
        return;
    if (dvz_window_backend_type(window) != DVZ_BACKEND_WRAP)
        return;
    _wrap_destroy_owned_surface(window);
    DvzWindowSurface* surface = dvz_window_backend_surface(window);
    ANN(surface);
    surface->instance = VK_NULL_HANDLE;
    surface->surface = VK_NULL_HANDLE;
    window->backend_owns_surface = false;
}
