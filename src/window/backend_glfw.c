/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  GLFW backend                                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "datoviz/input/keyboard.h"
#include "datoviz/input/pointer.h"
#include "datoviz/window.h"
#include "window_internal.h"


#include <math.h>
#include <stdlib.h>
#include <volk.h>


#if DVZ_HAS_GLFW
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

typedef struct DvzGlfwBackendState
{
    bool initialized;
    uint32_t window_count;
} DvzGlfwBackendState;

static DvzGlfwBackendState _glfw_state = {0};

static DvzWindow* _glfw_window(GLFWwindow* handle)
{
    return (DvzWindow*)glfwGetWindowUserPointer(handle);
}



/**
 * Log the latest GLFW error message when available.
 */
static void _glfw_log_error(const char* context)
{
    const char* message = NULL;
    int code = glfwGetError(&message);
    if (message != NULL)
    {
        log_error("%s (GLFW %d: %s)", context, code, message);
    }
    else
    {
        log_error("%s", context);
    }
}



static bool _glfw_init(void)
{
    if (_glfw_state.initialized)
        return true;
#if defined(__APPLE__)
#ifdef GLFW_COCOA_MENUBAR
    glfwInitHint(GLFW_COCOA_MENUBAR, GLFW_TRUE);
#endif
#ifdef GLFW_COCOA_CHDIR_RESOURCES
    glfwInitHint(GLFW_COCOA_CHDIR_RESOURCES, GLFW_FALSE);
#endif
#endif
    if (!glfwInit())
    {
        _glfw_log_error("glfwInit() failed");
        return false;
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    _glfw_state.initialized = true;
    return true;
}



/**
 * Return the macOS fallback Vulkan instance extension count.
 *
 * @return fallback extension count on macOS, otherwise zero
 */
static uint32_t _glfw_macos_fallback_required_extension_count(void)
{
#if OS_MACOS
    return 2;
#else
    return 0;
#endif
}



/**
 * Return one macOS fallback Vulkan instance extension name.
 *
 * @param index extension index to resolve
 * @return extension name at index, or NULL when unavailable
 */
static const char* _glfw_macos_fallback_required_extension_at(uint32_t index)
{
#if OS_MACOS
    static const char* extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        "VK_EXT_metal_surface",
    };
    return index < 2 ? extensions[index] : NULL;
#else
    (void)index;
    return NULL;
#endif
}



static void _glfw_shutdown(void)
{
    if (_glfw_state.window_count == 0 && _glfw_state.initialized)
    {
        glfwTerminate();
        _glfw_state.initialized = false;
    }
}



/**
 * Apply Datoviz GLFW window hints shared by every GLFW backend window.
 */
static void _glfw_apply_window_hints(void)
{
#if defined(GLFW_X11_CLASS_NAME) && defined(GLFW_X11_INSTANCE_NAME)
    glfwWindowHintString(GLFW_X11_CLASS_NAME, "datoviz");
    glfwWindowHintString(GLFW_X11_INSTANCE_NAME, "datoviz");
#endif
}



/**
 * Parse the optional display-scale override from the environment.
 *
 * @return positive display scale override, or zero when unset or invalid
 */
static float _glfw_display_scale_override(void)
{
    const char* value = getenv("DVZ_DISPLAY_SCALE");
    if (value == NULL || value[0] == '\0')
        return 0.0f;
    char* end = NULL;
    float scale = strtof(value, &end);
    if (end == value || !isfinite(scale) || scale <= 0.0f)
    {
        log_warn("ignoring invalid DVZ_DISPLAY_SCALE=%s", value);
        return 0.0f;
    }
    return scale;
}



/**
 * Return the monitor that best overlaps the current GLFW window.
 *
 * @param handle GLFW window handle
 * @return best monitor, or primary monitor when no overlap can be resolved
 */
static GLFWmonitor* _glfw_window_monitor(GLFWwindow* handle)
{
    ANN(handle);
    int wx = 0;
    int wy = 0;
    int ww = 0;
    int wh = 0;
    glfwGetWindowPos(handle, &wx, &wy);
    glfwGetWindowSize(handle, &ww, &wh);

    int monitor_count = 0;
    GLFWmonitor** monitors = glfwGetMonitors(&monitor_count);
    if (monitors == NULL || monitor_count <= 0)
        return glfwGetPrimaryMonitor();

    GLFWmonitor* best = NULL;
    int best_area = -1;
    for (int i = 0; i < monitor_count; i++)
    {
        int mx = 0;
        int my = 0;
        int mw = 0;
        int mh = 0;
        glfwGetMonitorWorkarea(monitors[i], &mx, &my, &mw, &mh);
        int left = wx > mx ? wx : mx;
        int top = wy > my ? wy : my;
        int right = wx + ww < mx + mw ? wx + ww : mx + mw;
        int bottom = wy + wh < my + mh ? wy + wh : my + mh;
        int area = 0;
        if (right > left && bottom > top)
            area = (right - left) * (bottom - top);
        if (area > best_area)
        {
            best = monitors[i];
            best_area = area;
        }
    }
    return best != NULL ? best : glfwGetPrimaryMonitor();
}



/**
 * Fill monitor scale and raw-DPI inputs for the monitor hosting a GLFW window.
 *
 * @param handle GLFW window handle
 * @param inputs scale inputs to update
 */
static void _glfw_fill_monitor_scale_inputs(GLFWwindow* handle, DvzWindowScaleInputs* inputs)
{
    ANN(handle);
    ANN(inputs);
    GLFWmonitor* monitor = _glfw_window_monitor(handle);
    if (monitor == NULL)
        return;

    float monitor_scale_x = 1.0f;
    float monitor_scale_y = 1.0f;
    glfwGetMonitorContentScale(monitor, &monitor_scale_x, &monitor_scale_y);
    inputs->monitor_scale_x = monitor_scale_x;
    inputs->monitor_scale_y = monitor_scale_y;

}



/**
 * Resolve an effective content scale for a GLFW window.
 *
 * @param handle GLFW window handle
 * @param raw_scale_x GLFW-reported window content scale in X
 * @param raw_scale_y GLFW-reported window content scale in Y
 * @param out_x resolved horizontal scale
 * @param out_y resolved vertical scale
 */
static void _glfw_effective_content_scale(
    GLFWwindow* handle, float raw_scale_x, float raw_scale_y, float* out_x, float* out_y)
{
    ANN(handle);
    ANN(out_x);
    ANN(out_y);

    int fb_width = 0;
    int fb_height = 0;
    glfwGetFramebufferSize(handle, &fb_width, &fb_height);
    int win_width = 0;
    int win_height = 0;
    glfwGetWindowSize(handle, &win_width, &win_height);

    DvzWindowScaleInputs inputs = {
        .window_scale_x = raw_scale_x,
        .window_scale_y = raw_scale_y,
        .framebuffer_width = fb_width > 0 ? (uint32_t)fb_width : 0,
        .framebuffer_height = fb_height > 0 ? (uint32_t)fb_height : 0,
        .window_width = win_width > 0 ? (uint32_t)win_width : 0,
        .window_height = win_height > 0 ? (uint32_t)win_height : 0,
        .override_scale = _glfw_display_scale_override(),
    };
    _glfw_fill_monitor_scale_inputs(handle, &inputs);
    _dvz_window_effective_content_scale(&inputs, out_x, out_y);
}



/**
 * Return a GLFW policy compatible with the current platform.
 *
 * @param policy requested policy
 * @return policy to pass into shared metrics resolution
 */
static DvzHiDpiPolicy _glfw_platform_policy(DvzHiDpiPolicy policy)
{
#if OS_MACOS
    if (policy == DVZ_HIDPI_AUTO || policy == DVZ_HIDPI_NATIVE_WINDOW)
        return DVZ_HIDPI_FRAMEBUFFER;
#endif
    return policy;
}



/**
 * Return a GLFW window extent from positive integer dimensions.
 *
 * @param width raw width
 * @param height raw height
 * @return sanitized extent
 */
static DvzExtent _glfw_extent(int width, int height)
{
    return (DvzExtent){
        .width = width > 0 ? (uint32_t)width : 0,
        .height = height > 0 ? (uint32_t)height : 0,
    };
}



/**
 * Return the requested logical extent stored in a window configuration.
 *
 * @param config window configuration
 * @return requested logical extent
 */
static DvzExtent _glfw_requested_logical_extent(const DvzWindowConfig* config)
{
    ANN(config);
    return (DvzExtent){
        .width = config->width > 0 ? config->width : DVZ_WINDOW_DEFAULT_WIDTH,
        .height = config->height > 0 ? config->height : DVZ_WINDOW_DEFAULT_HEIGHT,
    };
}



/**
 * Query raw GLFW state and resolve normalized Datoviz window metrics.
 *
 * @param handle GLFW window handle
 * @param requested_logical_size requested logical size, or zero to derive it from native metrics
 * @param policy requested HiDPI policy
 * @param previous_generation previous metrics generation
 * @param out_native_size optional raw native size output
 * @return normalized metrics snapshot
 */
static DvzWindowMetrics _glfw_query_metrics(
    GLFWwindow* handle, DvzExtent requested_logical_size, DvzHiDpiPolicy policy,
    uint64_t previous_generation, DvzExtent* out_native_size)
{
    ANN(handle);
    int fb_width = 0;
    int fb_height = 0;
    glfwGetFramebufferSize(handle, &fb_width, &fb_height);
    int win_width = 0;
    int win_height = 0;
    glfwGetWindowSize(handle, &win_width, &win_height);
    float scale_x = 1.0f;
    float scale_y = 1.0f;
    glfwGetWindowContentScale(handle, &scale_x, &scale_y);
    _glfw_effective_content_scale(handle, scale_x, scale_y, &scale_x, &scale_y);

    DvzExtent native_size = _glfw_extent(win_width, win_height);
    if (out_native_size != NULL)
        *out_native_size = native_size;

    DvzWindowMetricsInputs inputs = {
        .requested_logical_size = requested_logical_size,
        .native_size = native_size,
        .framebuffer_size = _glfw_extent(fb_width, fb_height),
        .content_scale = {.x = scale_x, .y = scale_y},
        .requested_policy = _glfw_platform_policy(policy),
        .previous_generation = previous_generation,
    };
    DvzWindowMetrics metrics = {0};
    _dvz_window_metrics_resolve(&inputs, &metrics);
    return metrics;
}



/**
 * Return whether two extents differ.
 *
 * @param a first extent
 * @param b second extent
 * @return whether the extents differ
 */
static bool _glfw_extent_differs(DvzExtent a, DvzExtent b)
{
    return a.width != b.width || a.height != b.height;
}



/**
 * Resolve metrics and resize native-window HiDPI backends when a logical size must be preserved.
 *
 * @param handle GLFW window handle
 * @param requested_logical_size requested logical size
 * @param policy requested HiDPI policy
 * @return metrics after any native resize
 */
static DvzWindowMetrics _glfw_configure_initial_metrics(
    GLFWwindow* handle, DvzExtent requested_logical_size, DvzHiDpiPolicy policy)
{
    ANN(handle);
    DvzExtent observed_native_size = {0};
    DvzWindowMetrics metrics =
        _glfw_query_metrics(handle, requested_logical_size, policy, 0, &observed_native_size);
    if (metrics.active_hidpi_policy == DVZ_HIDPI_NATIVE_WINDOW &&
        _glfw_extent_differs(observed_native_size, metrics.native_size))
    {
        glfwSetWindowSize(
            handle, (int)metrics.native_size.width, (int)metrics.native_size.height);
        metrics = _glfw_query_metrics(
            handle, (DvzExtent){0}, policy, 0, NULL);
    }
    return metrics;
}



/**
 * Requery GLFW metrics and emit the normalized resize event.
 *
 * @param handle GLFW window handle
 */
static void _glfw_emit_metrics(GLFWwindow* handle)
{
    DvzWindow* window = _glfw_window(handle);
    if (window == NULL)
        return;
    DvzWindowMetrics metrics = _glfw_query_metrics(
        handle, (DvzExtent){0}, window->config.hidpi_policy, window->metrics.generation, NULL);
    _dvz_window_backend_emit_metrics(window, &metrics);
}



/**
 * Convert GLFW native pointer coordinates to Datoviz logical coordinates.
 *
 * @param window Datoviz window
 * @param native_x GLFW native x coordinate
 * @param native_y GLFW native y coordinate
 * @param out_x logical x coordinate
 * @param out_y logical y coordinate
 * @param out_width logical window width
 * @param out_height logical window height
 */
static void _glfw_pointer_logical(
    const DvzWindow* window, double native_x, double native_y, float* out_x, float* out_y,
    float* out_width, float* out_height)
{
    ANN(window);
    ANN(out_x);
    ANN(out_y);
    ANN(out_width);
    ANN(out_height);
    const DvzWindowMetrics* metrics = dvz_window_metrics(window);
    ANN(metrics);
    float sx = metrics->native_to_logical.x > 0.0f ? metrics->native_to_logical.x : 1.0f;
    float sy = metrics->native_to_logical.y > 0.0f ? metrics->native_to_logical.y : 1.0f;
    *out_x = (float)native_x * sx;
    *out_y = (float)native_y * sy;
    *out_width = metrics->logical_size.width > 0 ? (float)metrics->logical_size.width : 1.0f;
    *out_height = metrics->logical_size.height > 0 ? (float)metrics->logical_size.height : 1.0f;
}



static void
_glfw_emit_pointer(GLFWwindow* handle, DvzPointerEventType type, DvzPointerButton button, int mods)
{
    DvzWindow* window = _glfw_window(handle);
    if (window == NULL)
        return;
    DvzInputRouter* router = dvz_window_backend_router(window);
    if (router == NULL)
        return;
    double xpos = 0.0;
    double ypos = 0.0;
    glfwGetCursorPos(handle, &xpos, &ypos);
    float logical_x = 0.0f;
    float logical_y = 0.0f;
    float logical_width = 0.0f;
    float logical_height = 0.0f;
    _glfw_pointer_logical(
        window, xpos, ypos, &logical_x, &logical_y, &logical_width, &logical_height);
    dvz_pointer_emit_position(
        router, type, logical_x, logical_y, logical_width, logical_height, button, mods,
        window->surface.scale_x, dvz_input_timestamp_ns(), dvz_window_user_data(window));
}



static void _glfw_cursor_pos_callback(GLFWwindow* handle, double xpos, double ypos)
{
    DvzWindow* window = _glfw_window(handle);
    if (window != NULL && window->glfw_input_callbacks.cursor_pos != NULL &&
        window->glfw_input_callbacks.cursor_pos(
            window, xpos, ypos, window->glfw_input_user_data))
    {
        return;
    }
    _glfw_emit_pointer(handle, DVZ_POINTER_EVENT_MOVE, DVZ_POINTER_BUTTON_NONE, 0);
}



static void _glfw_mouse_button_callback(GLFWwindow* handle, int button, int action, int mods)
{
    DvzWindow* window = _glfw_window(handle);
    if (window != NULL && window->glfw_input_callbacks.mouse_button != NULL &&
        window->glfw_input_callbacks.mouse_button(
            window, button, action, mods, window->glfw_input_user_data))
    {
        return;
    }
    DvzPointerButton dvz_button = dvz_pointer_button_from_glfw(button);
    DvzPointerEventType type =
        (action == GLFW_PRESS) ? DVZ_POINTER_EVENT_PRESS : DVZ_POINTER_EVENT_RELEASE;
    _glfw_emit_pointer(handle, type, dvz_button, mods);
}



static void _glfw_scroll_callback(GLFWwindow* handle, double dx, double dy)
{
    DvzWindow* window = _glfw_window(handle);
    if (window == NULL)
        return;
    if (window->glfw_input_callbacks.scroll != NULL &&
        window->glfw_input_callbacks.scroll(window, dx, dy, window->glfw_input_user_data))
    {
        return;
    }
    DvzInputRouter* router = dvz_window_backend_router(window);
    if (router == NULL)
        return;
    double xpos = 0.0;
    double ypos = 0.0;
    glfwGetCursorPos(handle, &xpos, &ypos);
    float logical_x = 0.0f;
    float logical_y = 0.0f;
    float logical_width = 0.0f;
    float logical_height = 0.0f;
    _glfw_pointer_logical(
        window, xpos, ypos, &logical_x, &logical_y, &logical_width, &logical_height);
#if defined(__APPLE__)
    /* Normalize macOS scroll-wheel direction so the input layer sees a single
     * sign convention across platforms. */
    dy = -dy;
#endif
    dvz_pointer_emit_wheel(
        router, logical_x, logical_y, logical_width, logical_height, (float)dx, (float)dy, 0,
        window->surface.scale_x, dvz_input_timestamp_ns(), dvz_window_user_data(window));
}



static void _glfw_key_callback(GLFWwindow* handle, int key, int scancode, int action, int mods)
{
    DvzWindow* window = _glfw_window(handle);
    if (window == NULL)
        return;
    if (window->glfw_input_callbacks.key != NULL &&
        window->glfw_input_callbacks.key(
            window, key, scancode, action, mods, window->glfw_input_user_data))
    {
        return;
    }
    DvzInputRouter* router = dvz_window_backend_router(window);
    if (router == NULL)
        return;
    DvzKeyboardEventType type = DVZ_KEYBOARD_EVENT_NONE;
    if (action == GLFW_PRESS)
        type = DVZ_KEYBOARD_EVENT_PRESS;
    else if (action == GLFW_RELEASE)
        type = DVZ_KEYBOARD_EVENT_RELEASE;
    else if (action == GLFW_REPEAT)
        type = DVZ_KEYBOARD_EVENT_REPEAT;
    if (type == DVZ_KEYBOARD_EVENT_NONE)
        return;
    dvz_keyboard_emit(router, type, (DvzKeyCode)key, mods, dvz_window_user_data(window));
}



static void _glfw_char_callback(GLFWwindow* handle, unsigned int codepoint)
{
    DvzWindow* window = _glfw_window(handle);
    if (window == NULL)
        return;
    if (window->glfw_input_callbacks.character != NULL)
        (void)window->glfw_input_callbacks.character(
            window, (uint32_t)codepoint, window->glfw_input_user_data);
}



static void _glfw_framebuffer_callback(GLFWwindow* handle, int width, int height)
{
    (void)width;
    (void)height;
    _glfw_emit_metrics(handle);
}



static void _glfw_window_size_callback(GLFWwindow* handle, int width, int height)
{
    (void)width;
    (void)height;
    _glfw_emit_metrics(handle);
}



static void _glfw_window_close_callback(GLFWwindow* handle)
{
    glfwSetWindowShouldClose(handle, GLFW_TRUE);
    glfwPostEmptyEvent();
}



static void _glfw_scale_callback(GLFWwindow* handle, float scale_x, float scale_y)
{
    DvzWindow* window = _glfw_window(handle);
    if (window == NULL)
        return;
    (void)scale_x;
    (void)scale_y;
    DvzExtent requested_logical_size = window->metrics.logical_size;
    DvzWindowMetrics metrics = _glfw_configure_initial_metrics(
        handle, requested_logical_size, window->config.hidpi_policy);
    metrics.generation = window->metrics.generation + 1;
    _dvz_window_backend_emit_metrics(window, &metrics);
}



static bool _glfw_probe(DvzWindowBackend* backend, DvzWindowHost* host)
{
    (void)backend;
    (void)host;
    return true;
}



/**
 * Return the number of Vulkan instance extensions required by GLFW.
 *
 * @param backend GLFW backend descriptor
 * @param host window host querying extension requirements
 * @return number of required extensions, or 0 when unavailable
 */
static uint32_t _glfw_required_extension_count(DvzWindowBackend* backend, DvzWindowHost* host)
{
    (void)backend;
    (void)host;
    if (!_glfw_init())
        return 0;
    uint32_t ext_count = 0;
    const char** extensions = glfwGetRequiredInstanceExtensions(&ext_count);
    if (extensions == NULL || ext_count == 0)
        return _glfw_macos_fallback_required_extension_count();
    return ext_count;
}



/**
 * Return one Vulkan instance extension name required by GLFW.
 *
 * @param backend GLFW backend descriptor
 * @param host window host querying extension requirements
 * @param index extension index to resolve
 * @return extension name at index, or NULL when unavailable
 */
static const char*
_glfw_required_extension_at(DvzWindowBackend* backend, DvzWindowHost* host, uint32_t index)
{
    (void)backend;
    (void)host;
    if (!_glfw_init())
        return NULL;
    uint32_t ext_count = 0;
    const char** extensions = glfwGetRequiredInstanceExtensions(&ext_count);
    if (extensions == NULL || ext_count == 0)
        return _glfw_macos_fallback_required_extension_at(index);
    if (index >= ext_count)
        return NULL;
    return extensions[index];
}



static bool
_glfw_create(DvzWindowBackend* backend, DvzWindow* window, const DvzWindowConfig* config)
{
    (void)backend;
    ANN(window);
    ANN(config);
    if (!_glfw_init())
        return false;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, config->resizable ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    _glfw_apply_window_hints();
    GLFWwindow* handle = glfwCreateWindow(
        (int)config->width, (int)config->height, config->title ? config->title : "", NULL, NULL);
    if (handle == NULL)
    {
        _glfw_log_error("failed to create GLFW window");
        return false;
    }
    glfwSetWindowUserPointer(handle, window);
    if (config->has_position)
        glfwSetWindowPos(handle, config->x, config->y);

    DvzWindowMetrics metrics = _glfw_configure_initial_metrics(
        handle, _glfw_requested_logical_extent(config), config->hidpi_policy);

    glfwSetCursorPosCallback(handle, _glfw_cursor_pos_callback);
    glfwSetMouseButtonCallback(handle, _glfw_mouse_button_callback);
    glfwSetScrollCallback(handle, _glfw_scroll_callback);
    glfwSetKeyCallback(handle, _glfw_key_callback);
    glfwSetCharCallback(handle, _glfw_char_callback);
    glfwSetFramebufferSizeCallback(handle, _glfw_framebuffer_callback);
    glfwSetWindowSizeCallback(handle, _glfw_window_size_callback);
    glfwSetWindowCloseCallback(handle, _glfw_window_close_callback);
    glfwSetWindowContentScaleCallback(handle, _glfw_scale_callback);
    dvz_window_backend_set_handle(window, handle);

    VkInstance vk_instance = volkGetLoadedInstance();
    if (vk_instance == VK_NULL_HANDLE)
    {
        log_error("Vulkan instance not loaded before creating GLFW surface");
        glfwDestroyWindow(handle);
        return false;
    }

    VkSurfaceKHR vk_surface = VK_NULL_HANDLE;
    VkResult res = glfwCreateWindowSurface(vk_instance, handle, NULL, &vk_surface);
    if (res != VK_SUCCESS)
    {
        log_error("glfwCreateWindowSurface() failed (%d)", res);
        glfwDestroyWindow(handle);
        return false;
    }

    DvzWindowSurface* surface = dvz_window_backend_surface(window);
    ANN(surface);
    surface->instance = vk_instance;
    surface->surface = vk_surface;
    _dvz_window_backend_emit_metrics(window, &metrics);
    if (config->visible)
        glfwShowWindow(handle);
    _glfw_state.window_count++;
    return true;
}



static void _glfw_destroy(DvzWindowBackend* backend, DvzWindow* window)
{
    (void)backend;
    if (window == NULL)
        return;
    DvzWindowSurface* surface = dvz_window_backend_surface(window);
    ANN(surface);
    if (surface->surface != VK_NULL_HANDLE && surface->instance != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(surface->instance, surface->surface, NULL);
        surface->surface = VK_NULL_HANDLE;
        surface->instance = VK_NULL_HANDLE;
    }
    GLFWwindow* handle = (GLFWwindow*)dvz_window_backend_handle(window);
    if (handle != NULL)
        glfwDestroyWindow(handle);
    if (_glfw_state.window_count > 0)
        _glfw_state.window_count--;
    _glfw_shutdown();
}



static void _glfw_poll(DvzWindowBackend* backend, DvzWindowHost* host)
{
    (void)backend;
    (void)host;
    if (_glfw_state.initialized)
        glfwPollEvents();
}



/**
 * Wait for the next GLFW event.
 *
 * @param backend GLFW backend descriptor
 * @param host host owning active GLFW windows
 */
static void _glfw_wait(DvzWindowBackend* backend, DvzWindowHost* host)
{
    (void)backend;
    (void)host;
    if (_glfw_state.initialized)
        glfwWaitEvents();
}



/**
 * Wait for the next GLFW event up to a timeout.
 *
 * @param backend GLFW backend descriptor
 * @param host host owning active GLFW windows
 * @param seconds maximum wait duration in seconds
 */
static void _glfw_wait_timeout(DvzWindowBackend* backend, DvzWindowHost* host, double seconds)
{
    (void)backend;
    (void)host;
    if (!_glfw_state.initialized)
        return;
    if (!(seconds > 0.0))
    {
        glfwPollEvents();
        return;
    }
    glfwWaitEventsTimeout(seconds);
}



/**
 * Wake GLFW event waits after a frame request.
 *
 * @param backend GLFW backend descriptor
 * @param window window requesting a frame
 */
static void _glfw_request_frame(DvzWindowBackend* backend, DvzWindow* window)
{
    (void)backend;
    (void)window;
    if (_glfw_state.initialized)
        glfwPostEmptyEvent();
}



static bool _glfw_should_close(const DvzWindowBackend* backend, const DvzWindow* window)
{
    (void)backend;
    if (window == NULL)
        return true;
    GLFWwindow* handle = (GLFWwindow*)dvz_window_backend_handle(window);
    if (handle == NULL)
        return true;
    return glfwWindowShouldClose(handle) != 0;
}



/**
 * Register the GLFW backend.
 */
void dvz_window_register_glfw_backend(DvzWindowHost* host)
{
    ANN(host);
    DvzWindowBackend backend = {
        .name = "glfw",
        .type = DVZ_BACKEND_GLFW,
        .user_data = &_glfw_state,
        .procs =
            {
                .probe = _glfw_probe,
                .create = _glfw_create,
                .destroy = _glfw_destroy,
                .poll = _glfw_poll,
                .wait = _glfw_wait,
                .wait_timeout = _glfw_wait_timeout,
                .request_frame = _glfw_request_frame,
                .should_close = _glfw_should_close,
                .required_extension_count = _glfw_required_extension_count,
                .required_extension_at = _glfw_required_extension_at,
            },
    };
    dvz_window_host_register_backend(host, &backend);
}

#else

static bool _glfw_disabled_probe(DvzWindowBackend* backend, DvzWindowHost* host)
{
    (void)backend;
    (void)host;
    return false;
}



/**
 * Return GLFW extension requirements when GLFW support is disabled.
 *
 * @param backend disabled backend descriptor
 * @param host window host querying extension requirements
 * @return always 0 when GLFW is disabled
 */
static uint32_t
_glfw_disabled_required_extension_count(DvzWindowBackend* backend, DvzWindowHost* host)
{
    (void)backend;
    (void)host;
    return 0;
}



/**
 * Return one GLFW extension name when GLFW support is disabled.
 *
 * @param backend disabled backend descriptor
 * @param host window host querying extension requirements
 * @param index extension index to resolve
 * @return always NULL when GLFW is disabled
 */
static const char*
_glfw_disabled_required_extension_at(DvzWindowBackend* backend, DvzWindowHost* host, uint32_t index)
{
    (void)backend;
    (void)host;
    (void)index;
    return NULL;
}



/**
 * Register a disabled GLFW backend placeholder.
 */
void dvz_window_register_glfw_backend(DvzWindowHost* host)
{
    ANN(host);
    DvzWindowBackend backend = {
        .name = "glfw",
        .type = DVZ_BACKEND_GLFW,
        .user_data = NULL,
        .procs =
            {
                .probe = _glfw_disabled_probe,
                .create = NULL,
                .destroy = NULL,
                .poll = NULL,
                .wait = NULL,
                .wait_timeout = NULL,
                .request_frame = NULL,
                .required_extension_count = _glfw_disabled_required_extension_count,
                .required_extension_at = _glfw_disabled_required_extension_at,
            },
    };
    dvz_window_host_register_backend(host, &backend);
}



#endif


/**
 * Ensure GLFW is initialized so extensions can be queried before a window exists.
 *
 * @returns true when GLFW is initialized, false otherwise
 */
DVZ_EXPORT bool dvz_window_glfw_init(void)
{
#if DVZ_HAS_GLFW
    return _glfw_init();
#else
    log_warn("GLFW backend disabled, cannot initialize");
    return false;
#endif
}
