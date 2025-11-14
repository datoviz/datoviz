/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Window host                                                                                  */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "datoviz/window.h"
#include "window_internal.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_WINDOW_BACKEND_INIT_CAP  4
#define DVZ_WINDOW_INSTANCE_INIT_CAP 4



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzWindowBackendSlot DvzWindowBackendSlot;



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

static void _window_register_builtins(DvzWindowHost* host);

static DvzWindowBackendSlot* _window_pick_backend(DvzWindowHost* host, DvzBackend backend);

static void _window_array_add(DvzWindowHost* host, DvzWindow* window);

static void _window_array_remove(DvzWindowHost* host, DvzWindow* window);

static void _window_host_clear_windows(DvzWindowHost* host);



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static void
_window_reserve(void** array, uint32_t* capacity, size_t item_size, uint32_t min_capacity)
{
    ANN(array);
    ANN(capacity);
    if (*capacity >= min_capacity)
        return;
    uint32_t new_capacity = (*capacity == 0) ? 1 : *capacity;
    while (new_capacity < min_capacity)
    {
        new_capacity *= 2;
    }
    void* ptr = dvz_realloc(*array, item_size * new_capacity);
    ANN(ptr);
    dvz_memset(
        POINTER_OFFSET(ptr, item_size * (*capacity)), item_size * (new_capacity - *capacity), 0,
        item_size * (new_capacity - *capacity));
    *array = ptr;
    *capacity = new_capacity;
}



static void _window_register_builtins(DvzWindowHost* host)
{
    ANN(host);
    dvz_window_register_headless_backend(host);
    dvz_window_register_glfw_backend(host);
}



static DvzWindowBackendSlot* _window_find_slot(DvzWindowHost* host, DvzBackend backend)
{
    ANN(host);
    for (uint32_t i = 0; i < host->backend_count; i++)
    {
        DvzWindowBackendSlot* slot = &host->backends[i];
        if (slot->backend.type == backend)
            return slot;
    }
    return NULL;
}



static DvzWindowBackendSlot* _window_pick_backend(DvzWindowHost* host, DvzBackend backend)
{
    ANN(host);
    DvzBackend requested = backend;
    if (requested == DVZ_BACKEND_NONE)
        requested = DVZ_BACKEND_GLFW;

    DvzWindowBackendSlot* slot = _window_find_slot(host, requested);
    if (slot != NULL && slot->available)
        return slot;

    if (requested != DVZ_BACKEND_OFFSCREEN)
    {
        log_warn("backend %d unavailable, falling back to offscreen", (int)requested);
        slot = _window_find_slot(host, DVZ_BACKEND_OFFSCREEN);
        if (slot != NULL && slot->available)
            return slot;
    }

    if (requested != DVZ_BACKEND_NONE)
        log_error("no available backend matching request %d", (int)requested);
    return NULL;
}



static void _window_array_add(DvzWindowHost* host, DvzWindow* window)
{
    ANN(host);
    ANN(window);
    _window_reserve(
        (void**)&host->windows, &host->window_capacity, sizeof(DvzWindow*),
        host->window_count + 1);
    host->windows[host->window_count++] = window;
}



static void _window_array_remove(DvzWindowHost* host, DvzWindow* window)
{
    ANN(host);
    ANN(window);
    for (uint32_t i = 0; i < host->window_count; i++)
    {
        if (host->windows[i] == window)
        {
            host->windows[i] = host->windows[host->window_count - 1];
            host->window_count--;
            return;
        }
    }
}



static void _window_host_clear_windows(DvzWindowHost* host)
{
    ANN(host);
    while (host->window_count > 0)
    {
        dvz_window_destroy(host->windows[host->window_count - 1]);
    }
}



static void _window_setup_config(DvzWindow* window, const DvzWindowConfig* config)
{
    ANN(window);
    ANN(config);
    window->config = *config;
    const char* title = config->title != NULL ? config->title : DVZ_WINDOW_DEFAULT_TITLE;
    dvz_strlcpy(window->title, title, sizeof(window->title));
    window->config.title = window->title;
    float user_scale = config->user_scale > 0.f ? config->user_scale : 1.f;
    window->config.user_scale = user_scale;
    window->surface.extent.width = config->width > 0 ? config->width : DVZ_WINDOW_DEFAULT_WIDTH;
    window->surface.extent.height =
        config->height > 0 ? config->height : DVZ_WINDOW_DEFAULT_HEIGHT;
    window->surface.scale_x = user_scale;
    window->surface.scale_y = user_scale;
    window->surface.instance = VK_NULL_HANDLE;
    window->surface.surface = VK_NULL_HANDLE;
    window->surface.format = VK_FORMAT_B8G8R8A8_UNORM;
    window->surface.color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return the default configuration for a window.
 */
DvzWindowConfig dvz_window_default_config(void)
{
    DvzWindowConfig config = {
        .width = DVZ_WINDOW_DEFAULT_WIDTH,
        .height = DVZ_WINDOW_DEFAULT_HEIGHT,
        .title = DVZ_WINDOW_DEFAULT_TITLE,
        .resizable = true,
        .user_scale = 1.f,
    };
    return config;
}



/**
 * Create a new window host.
 */
DvzWindowHost* dvz_window_host(void)
{
    DvzWindowHost* host = dvz_calloc(1, sizeof(DvzWindowHost));
    ANN(host);
    host->backend_capacity = DVZ_WINDOW_BACKEND_INIT_CAP;
    host->window_capacity = DVZ_WINDOW_INSTANCE_INIT_CAP;
    host->backends = dvz_calloc(host->backend_capacity, sizeof(DvzWindowBackendSlot));
    host->windows = dvz_calloc(host->window_capacity, sizeof(DvzWindow*));
    ANN(host->backends);
    ANN(host->windows);
    _window_register_builtins(host);
    return host;
}



/**
 * Destroy a window host and all windows owned by it.
 */
void dvz_window_host_destroy(DvzWindowHost* host)
{
    if (host == NULL)
        return;
    _window_host_clear_windows(host);
    dvz_free(host->windows);
    dvz_free(host->backends);
    dvz_free(host);
}



/**
 * Register a backend with the window host.
 */
void dvz_window_host_register_backend(DvzWindowHost* host, const DvzWindowBackend* backend)
{
    ANN(host);
    ANN(backend);
    _window_reserve(
        (void**)&host->backends, &host->backend_capacity, sizeof(DvzWindowBackendSlot),
        host->backend_count + 1);
    DvzWindowBackendSlot* slot = &host->backends[host->backend_count++];
    dvz_memset(slot, sizeof(*slot), 0, sizeof(*slot));
    slot->backend = *backend;
    if (slot->backend.procs.probe != NULL)
    {
        slot->available = slot->backend.procs.probe(&slot->backend, host);
        slot->probed = true;
    }
    else
    {
        slot->available = true;
        slot->probed = true;
    }
}



/**
 * Create a new window instance.
 */
DvzWindow*
dvz_window_create(DvzWindowHost* host, DvzBackend backend, const DvzWindowConfig* config)
{
    ANN(host);
    DvzWindowConfig chosen = config ? *config : dvz_window_default_config();
    DvzWindowBackendSlot* slot = _window_pick_backend(host, backend);
    if (slot == NULL || !slot->available || slot->backend.procs.create == NULL)
    {
        log_error("cannot create window, backend unavailable");
        return NULL;
    }

    DvzWindow* window = dvz_calloc(1, sizeof(DvzWindow));
    ANN(window);
    window->host = host;
    window->router = dvz_input_router();
    ANN(window->router);
    _window_setup_config(window, &chosen);
    window->backend_slot = slot;
    if (!slot->backend.procs.create(&slot->backend, window, &window->config))
    {
        log_error("window creation failed on backend %s", slot->backend.name);
        slot->available = false;
        dvz_input_router_destroy(window->router);
        dvz_free(window);
        if (slot->backend.type != DVZ_BACKEND_OFFSCREEN)
            return dvz_window_create(host, DVZ_BACKEND_OFFSCREEN, &chosen);
        return NULL;
    }
    _window_array_add(host, window);
    return window;
}



/**
 * Destroy an existing window.
 */
void dvz_window_destroy(DvzWindow* window)
{
    if (window == NULL)
        return;
    if (window->backend_slot != NULL && window->backend_slot->backend.procs.destroy != NULL)
        window->backend_slot->backend.procs.destroy(&window->backend_slot->backend, window);
    dvz_input_router_destroy(window->router);
    _window_array_remove(window->host, window);
    dvz_free(window);
}



/**
 * Access the surface description associated with a window.
 */
const DvzWindowSurface* dvz_window_surface(const DvzWindow* window)
{
    ANN(window);
    return &window->surface;
}



/**
 * Access the router associated with the window.
 */
DvzInputRouter* dvz_window_router(DvzWindow* window)
{
    ANN(window);
    return window->router;
}



/**
 * Attach user data to the window.
 */
void dvz_window_set_user_data(DvzWindow* window, void* user_data)
{
    ANN(window);
    window->user_data = user_data;
}



/**
 * Retrieve user data associated with the window.
 */
void* dvz_window_user_data(const DvzWindow* window)
{
    ANN(window);
    return window->user_data;
}



/**
 * Poll the host backends and process window events.
 */
void dvz_window_host_poll(DvzWindowHost* host)
{
    ANN(host);
    for (uint32_t i = 0; i < host->backend_count; i++)
    {
        DvzWindowBackendSlot* slot = &host->backends[i];
        if (slot->available && slot->backend.procs.poll != NULL)
            slot->backend.procs.poll(&slot->backend, host);
    }
    for (uint32_t i = 0; i < host->window_count; i++)
    {
        host->windows[i]->frame_pending = false;
    }
}



/**
 * Request a frame from a window backend.
 */
void dvz_window_host_request_frame(DvzWindowHost* host, DvzWindow* window)
{
    ANN(host);
    ANN(window);
    window->frame_pending = true;
    if (window->backend_slot != NULL && window->backend_slot->backend.procs.request_frame != NULL)
        window->backend_slot->backend.procs.request_frame(&window->backend_slot->backend, window);
}



/**
 * Whether the window has a frame request pending.
 */
bool dvz_window_frame_pending(const DvzWindow* window)
{
    ANN(window);
    return window->frame_pending;
}



/**
 * Return the backend type associated with the window.
 */
DvzBackend dvz_window_backend_type(const DvzWindow* window)
{
    ANN(window);
    return (window->backend_slot != NULL) ? window->backend_slot->backend.type : DVZ_BACKEND_NONE;
}



/**
 * Access the router from backend code.
 */
DvzInputRouter* dvz_window_backend_router(DvzWindow* window) { return dvz_window_router(window); }



/**
 * Access the surface from backend code.
 */
DvzWindowSurface* dvz_window_backend_surface(DvzWindow* window)
{
    ANN(window);
    return &window->surface;
}



/**
 * Store the native backend handle.
 */
void dvz_window_backend_set_handle(DvzWindow* window, void* handle)
{
    ANN(window);
    window->backend_handle = handle;
}



/**
 * Retrieve the native backend handle.
 */
void* dvz_window_backend_handle(const DvzWindow* window)
{
    ANN(window);
    return window->backend_handle;
}



/**
 * Store backend payload on the window.
 */
void dvz_window_backend_set_payload(DvzWindow* window, void* payload)
{
    ANN(window);
    window->backend_payload = payload;
}



/**
 * Retrieve backend payload stored with the window.
 */
void* dvz_window_backend_payload(const DvzWindow* window)
{
    ANN(window);
    return window->backend_payload;
}



/**
 * Emit a resize event and refresh the cached surface values.
 */
void dvz_window_backend_emit_resize(
    DvzWindow* window, uint32_t framebuffer_width, uint32_t framebuffer_height,
    uint32_t window_width, uint32_t window_height, float content_scale_x, float content_scale_y)
{
    ANN(window);
    ANN(window->router);
    DvzWindowSurface* surface = &window->surface;
    surface->extent.width = framebuffer_width;
    surface->extent.height = framebuffer_height;
    surface->scale_x = content_scale_x;
    surface->scale_y = content_scale_y;
    DvzInputResizeEvent resize = {
        .framebuffer_width = framebuffer_width,
        .framebuffer_height = framebuffer_height,
        .window_width = window_width,
        .window_height = window_height,
        .content_scale_x = content_scale_x,
        .content_scale_y = content_scale_y,
    };
    dvz_input_emit_resize(window->router, &resize);
}



/**
 * Emit a scale event on the router.
 */
void dvz_window_backend_emit_scale(DvzWindow* window, float content_scale_x, float content_scale_y)
{
    ANN(window);
    ANN(window->router);
    window->surface.scale_x = content_scale_x;
    window->surface.scale_y = content_scale_y;
    DvzInputScaleEvent scale = {
        .content_scale_x = content_scale_x, .content_scale_y = content_scale_y};
    dvz_input_emit_scale(window->router, &scale);
}
