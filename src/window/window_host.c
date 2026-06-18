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

#include <math.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "_time_utils.h"
#include "datoviz/input/pointer.h"
#include "datoviz/window.h"
#include "window_internal.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_WINDOW_BACKEND_INIT_CAP  4
#define DVZ_WINDOW_INSTANCE_INIT_CAP 4
#define DVZ_WINDOW_CONFIG_KNOWN_FLAGS 0u
#define DVZ_WINDOW_EXTERNAL_SURFACE_INFO_KNOWN_FLAGS 0u
#define DVZ_WINDOW_BASE_DPI 96.0f
#define DVZ_WINDOW_MIN_RAW_DPI_SCALE 1.25f
#define DVZ_WINDOW_MAX_EFFECTIVE_SCALE 4.0f



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
static void _window_wrap_state_clear(DvzWindowHost* host);
static bool _window_backend_slot_has_window(
    const DvzWindowHost* host, const DvzWindowBackendSlot* slot);
static void _window_host_clear_frame_pending(DvzWindowHost* host);
static float _window_scale_candidate(float value);
static float _window_raw_dpi_scale(uint32_t pixels, uint32_t mm);



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
    dvz_window_register_wrap_backend(host);
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



/**
 * Return whether a backend slot owns at least one live window.
 *
 * @param host host whose windows are inspected
 * @param slot backend slot to match
 * @return true when at least one window uses the slot
 */
static bool
_window_backend_slot_has_window(const DvzWindowHost* host, const DvzWindowBackendSlot* slot)
{
    ANN(host);
    ANN(slot);
    for (uint32_t i = 0; i < host->window_count; i++)
    {
        if (host->windows[i] != NULL && host->windows[i]->backend_slot == slot)
            return true;
    }
    return false;
}



/**
 * Clear pending frame request flags after an event-processing wait/poll.
 *
 * @param host host whose windows are updated
 */
static void _window_host_clear_frame_pending(DvzWindowHost* host)
{
    ANN(host);
    for (uint32_t i = 0; i < host->window_count; i++)
    {
        host->windows[i]->frame_pending = false;
    }
}



/**
 * Return a bounded scale candidate, or one when unusable.
 *
 * @param value scale candidate
 * @return bounded scale candidate no smaller than one
 */
static float _window_scale_candidate(float value)
{
    if (!isfinite(value) || value <= 1.0f)
        return 1.0f;
    if (value > DVZ_WINDOW_MAX_EFFECTIVE_SCALE)
        return DVZ_WINDOW_MAX_EFFECTIVE_SCALE;
    return value;
}



/**
 * Estimate display scale from raw monitor DPI.
 *
 * @param pixels monitor dimension in physical pixels
 * @param mm monitor physical dimension in millimeters
 * @return bounded raw-DPI scale, or one when unavailable or low-DPI
 */
static float _window_raw_dpi_scale(uint32_t pixels, uint32_t mm)
{
    if (pixels == 0 || mm == 0)
        return 1.0f;
    float inches = (float)mm / 25.4f;
    if (!isfinite(inches) || inches <= 0.0f)
        return 1.0f;
    float scale = ((float)pixels / inches) / DVZ_WINDOW_BASE_DPI;
    if (!isfinite(scale) || scale < DVZ_WINDOW_MIN_RAW_DPI_SCALE)
        return 1.0f;
    return _window_scale_candidate(scale);
}



/**
 * Resolve an effective content scale from backend, framebuffer, monitor, and DPI signals.
 *
 * @param inputs scale signal bundle
 * @param out_x resolved horizontal content scale
 * @param out_y resolved vertical content scale
 */
void _dvz_window_effective_content_scale(
    const DvzWindowScaleInputs* inputs, float* out_x, float* out_y)
{
    ANN(inputs);
    ANN(out_x);
    ANN(out_y);

    float sx = _window_scale_candidate(inputs->override_scale);
    float sy = sx;
    if (sx <= 1.0f)
    {
        sx = _window_scale_candidate(inputs->window_scale_x);
        sy = _window_scale_candidate(inputs->window_scale_y);

        if (inputs->window_width > 0 && inputs->framebuffer_width > 0)
        {
            float ratio = (float)inputs->framebuffer_width / (float)inputs->window_width;
            float candidate = _window_scale_candidate(ratio);
            if (candidate > sx)
                sx = candidate;
        }
        if (inputs->window_height > 0 && inputs->framebuffer_height > 0)
        {
            float ratio = (float)inputs->framebuffer_height / (float)inputs->window_height;
            float candidate = _window_scale_candidate(ratio);
            if (candidate > sy)
                sy = candidate;
        }

        float candidate = _window_scale_candidate(inputs->monitor_scale_x);
        if (candidate > sx)
            sx = candidate;
        candidate = _window_scale_candidate(inputs->monitor_scale_y);
        if (candidate > sy)
            sy = candidate;

        candidate =
            _window_raw_dpi_scale(inputs->monitor_pixel_width, inputs->monitor_width_mm);
        if (candidate > sx)
            sx = candidate;
        candidate =
            _window_raw_dpi_scale(inputs->monitor_pixel_height, inputs->monitor_height_mm);
        if (candidate > sy)
            sy = candidate;
    }

    *out_x = sx;
    *out_y = sy;
}



/**
 * Release extension strings owned by the wrap backend state.
 *
 * @param host window host whose wrap state is cleared
 */
static void _window_wrap_state_clear(DvzWindowHost* host)
{
    ANN(host);
    DvzWindowWrapBackendState* state = &host->wrap_state;
    if (state->extensions == NULL)
    {
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
    window->surface.format = VK_FORMAT_UNDEFINED;
    window->surface.color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    window->backend_owns_surface = false;
}



static bool _window_config_validate(const DvzWindowConfig* config)
{
    if (config == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(config, DvzWindowConfig, DVZ_WINDOW_CONFIG_KNOWN_FLAGS))
    {
        log_error("invalid DvzWindowConfig ABI prologue");
        return false;
    }
    return true;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return the default configuration for a window.
 */
DvzWindowConfig dvz_window_config(void)
{
    DvzWindowConfig config = {
        DVZ_STRUCT_INIT_FIELDS(DvzWindowConfig),
        .width = DVZ_WINDOW_DEFAULT_WIDTH,
        .height = DVZ_WINDOW_DEFAULT_HEIGHT,
        .title = DVZ_WINDOW_DEFAULT_TITLE,
        .resizable = true,
        .visible = true,
        .user_scale = 1.f,
    };
    return config;
}



/**
 * Return the default external-surface descriptor.
 */
DvzWindowExternalSurfaceInfo dvz_window_external_surface_info(void)
{
    return (DvzWindowExternalSurfaceInfo){
        DVZ_STRUCT_INIT_FIELDS(DvzWindowExternalSurfaceInfo),
    };
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
    _window_wrap_state_clear(host);
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
    if (!_window_config_validate(config))
        return NULL;

    DvzWindowConfig chosen = config ? *config : dvz_window_config();
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
    window->gesture_handler = dvz_pointer_gesture_handler(window->router);
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
    dvz_pointer_gesture_handler_destroy(window->gesture_handler);
    window->gesture_handler = NULL;
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
    _window_host_clear_frame_pending(host);
}



/**
 * Wait for events from the first backend that supports blocking waits.
 *
 * @param host host whose active backend should wait for events
 */
void dvz_window_host_wait(DvzWindowHost* host)
{
    ANN(host);
    for (uint32_t i = 0; i < host->backend_count; i++)
    {
        DvzWindowBackendSlot* slot = &host->backends[i];
        if (!slot->available || !_window_backend_slot_has_window(host, slot))
            continue;
        if (slot->backend.procs.wait != NULL)
        {
            slot->backend.procs.wait(&slot->backend, host);
            _window_host_clear_frame_pending(host);
            return;
        }
        if (slot->backend.procs.poll != NULL)
        {
            dvz_sleep_us(1000);
            slot->backend.procs.poll(&slot->backend, host);
            _window_host_clear_frame_pending(host);
            return;
        }
    }

    dvz_sleep_us(1000);
    _window_host_clear_frame_pending(host);
}



/**
 * Wait for events from the first backend that supports timeout waits.
 *
 * @param host host whose active backend should wait for events
 * @param seconds maximum wait duration in seconds
 */
void dvz_window_host_wait_timeout(DvzWindowHost* host, double seconds)
{
    ANN(host);
    if (!(seconds > 0.0))
    {
        dvz_window_host_poll(host);
        return;
    }

    for (uint32_t i = 0; i < host->backend_count; i++)
    {
        DvzWindowBackendSlot* slot = &host->backends[i];
        if (!slot->available || !_window_backend_slot_has_window(host, slot))
            continue;
        if (slot->backend.procs.wait_timeout != NULL)
        {
            slot->backend.procs.wait_timeout(&slot->backend, host, seconds);
            _window_host_clear_frame_pending(host);
            return;
        }
        if (slot->backend.procs.wait != NULL)
        {
            slot->backend.procs.wait(&slot->backend, host);
            _window_host_clear_frame_pending(host);
            return;
        }
        if (slot->backend.procs.poll != NULL)
        {
            double sleep_us_f = seconds * 1000000.0;
            int sleep_us = sleep_us_f > 1000000.0 ? 1000000 : (int)sleep_us_f;
            if (sleep_us < 1000)
                sleep_us = 1000;
            dvz_sleep_us(sleep_us);
            slot->backend.procs.poll(&slot->backend, host);
            _window_host_clear_frame_pending(host);
            return;
        }
    }

    double sleep_us_f = seconds * 1000000.0;
    int sleep_us = sleep_us_f > 1000000.0 ? 1000000 : (int)sleep_us_f;
    if (sleep_us < 1000)
        sleep_us = 1000;
    dvz_sleep_us(sleep_us);
    _window_host_clear_frame_pending(host);
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
 * Return whether the window has been requested to close.
 */
bool dvz_window_should_close(const DvzWindow* window)
{
    ANN(window);
    if (window->backend_slot == NULL || window->backend_slot->backend.procs.should_close == NULL)
        return false;
    return window->backend_slot->backend.procs.should_close(
        &window->backend_slot->backend, window);
}



/**
 * Query how many Vulkan instance extensions are required by a given backend.
 *
 * @param host host that contains the backend registry
 * @param backend backend identifier to query
 * @return required extension count, or 0 when backend is unavailable
 */
uint32_t dvz_window_host_required_extension_count(DvzWindowHost* host, DvzBackend backend)
{
    if (host == NULL)
        return 0;
    DvzWindowBackendSlot* slot = _window_find_slot(host, backend);
    if (slot == NULL || !slot->available || slot->backend.procs.required_extension_count == NULL)
        return 0;
    return slot->backend.procs.required_extension_count(&slot->backend, host);
}



/**
 * Query backend-required Vulkan instance extension names.
 *
 * @param host host that contains the backend registry
 * @param backend backend identifier to query
 * @param capacity maximum number of entries writable to out_extensions
 * @param out_extensions output array receiving extension names
 * @return number of written extensions, or -1 on invalid input/backend unavailable
 */
int dvz_window_host_required_extensions(
    DvzWindowHost* host, DvzBackend backend, uint32_t capacity, const char** out_extensions)
{
    if (host == NULL)
        return -1;
    if (capacity > 0 && out_extensions == NULL)
        return -1;

    DvzWindowBackendSlot* slot = _window_find_slot(host, backend);
    if (slot == NULL || !slot->available || slot->backend.procs.required_extension_count == NULL ||
        slot->backend.procs.required_extension_at == NULL)
    {
        return -1;
    }

    uint32_t required = slot->backend.procs.required_extension_count(&slot->backend, host);
    uint32_t written = required < capacity ? required : capacity;
    for (uint32_t i = 0; i < written; i++)
    {
        const char* extension = slot->backend.procs.required_extension_at(&slot->backend, host, i);
        if (extension == NULL)
            return -1;
        out_extensions[i] = extension;
    }
    return (int)written;
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
 * Register raw GLFW input callbacks for integrations that must see events before Datoviz routing.
 */
void dvz_window_glfw_set_input_callbacks(
    DvzWindow* window, const DvzWindowGlfwInputCallbacks* callbacks, void* user_data)
{
    ANN(window);
    if (callbacks != NULL)
        window->glfw_input_callbacks = *callbacks;
    else
        dvz_memset(&window->glfw_input_callbacks, sizeof(window->glfw_input_callbacks), 0,
                   sizeof(window->glfw_input_callbacks));
    window->glfw_input_user_data = callbacks != NULL ? user_data : NULL;
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
