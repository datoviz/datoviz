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
#include "_loader.h"
#include "_log.h"
#include "datoviz/input/keyboard.h"
#include "datoviz/input/pointer.h"
#include "datoviz/input/router.h"
#include "datoviz/window.h"
#include "window_internal.h"
#include "_window_wake.h"


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
    PFN_vkGetInstanceProcAddr loader = _dvz_vulkan_loader();
    if (loader == NULL)
    {
        log_error("unable to initialize Vulkan before GLFW");
        return false;
    }
    glfwInitVulkanLoader(loader);
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
    // Allow automation to use a separate window-manager rule without affecting manual launches.
    const char* instance = getenv("DVZ_WINDOW_INSTANCE");
    glfwWindowHintString(
        GLFW_X11_INSTANCE_NAME, instance != NULL && instance[0] != '\0' ? instance : "datoviz");
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
 * Return the refresh rate of the monitor that best overlaps a GLFW window.
 *
 * @param handle GLFW window handle
 * @return refresh rate in Hz, or zero when unavailable
 */
static uint32_t _glfw_window_refresh_rate(GLFWwindow* handle)
{
    ANN(handle);
    GLFWmonitor* monitor = _glfw_window_monitor(handle);
    if (monitor == NULL)
        return 0;
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    return mode != NULL && mode->refreshRate > 0 ? (uint32_t)mode->refreshRate : 0;
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
        .refresh_rate_hz = _glfw_window_refresh_rate(handle),
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
 * Fit one initial native window frame to its current monitor workarea.
 *
 * @param handle GLFW window handle after initial HiDPI sizing
 */
static void _glfw_fit_workarea(GLFWwindow* handle)
{
    GLFWmonitor* monitor = _glfw_window_monitor(handle);
    if (monitor == NULL)
        return;
    int x = 0, y = 0, width = 0, height = 0;
    int left = 0, top = 0, right = 0, bottom = 0;
    int mx = 0, my = 0, mw = 0, mh = 0;
    glfwGetWindowPos(handle, &x, &y);
    glfwGetWindowSize(handle, &width, &height);
    glfwGetWindowFrameSize(handle, &left, &top, &right, &bottom);
    glfwGetMonitorWorkarea(monitor, &mx, &my, &mw, &mh);
    int32_t bounds[4] = {x, y, width, height};
    const int32_t workarea[4] = {mx, my, mw, mh};
    const int32_t frame[4] = {left, top, right, bottom};
    _dvz_window_fit_workarea(bounds, workarea, frame);
    if (bounds[2] != width || bounds[3] != height)
        glfwSetWindowSize(handle, bounds[2], bounds[3]);
    if (bounds[0] != x || bounds[1] != y)
        glfwSetWindowPos(handle, bounds[0], bounds[1]);
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
    DvzHostInputEvent event = {.type = DVZ_HOST_INPUT_METRICS, .metrics = metrics,
                              .timestamp_ns = dvz_input_timestamp_ns()};
    (void)dvz_window_emit_host_input(window, &event, DVZ_HOST_INPUT_LIVE);
}



static int _glfw_gui_mods(GLFWwindow* handle);



/**
 * Capture a cursor position before any GUI consumption.
 * @param handle GLFW host
 * @param xpos native client x
 * @param ypos native client y
 */
static void _glfw_cursor_pos_callback(GLFWwindow* handle, double xpos, double ypos)
{
    DvzWindow* window = _glfw_window(handle);
    DvzHostInputEvent event = {.type = DVZ_HOST_INPUT_MOVE, .x = xpos, .y = ypos,
                              .mods = window != NULL ? window->glfw_mods : 0,
                              .timestamp_ns = dvz_input_timestamp_ns()};
    (void)dvz_window_emit_host_input(window, &event, DVZ_HOST_INPUT_LIVE);
}



/** Capture every native mouse-button transition before GUI arbitration. */
static void _glfw_mouse_button_callback(GLFWwindow* handle, int button, int action, int mods)
{
    DvzWindow* window = _glfw_window(handle);
    if (window == NULL || window->input_state.replaying)
        return;
    if (action == GLFW_RELEASE && _dvz_window_cancelled_release(window, false, button))
        return;
    if (action == GLFW_PRESS && button >= 0 && button < 8)
        window->input_cancelled_buttons &= ~(1u << button);
    DvzHostInputEvent event = {.type = DVZ_HOST_INPUT_BUTTON, .button = button,
                              .action = action, .mods = mods, .gui_mods = _glfw_gui_mods(handle),
                              .timestamp_ns = dvz_input_timestamp_ns()};
    glfwGetCursorPos(handle, &event.x, &event.y);
    (void)dvz_window_emit_host_input(window, &event, DVZ_HOST_INPUT_LIVE);
}



/** Capture raw fractional GLFW wheel offsets before platform normalization. */
static void _glfw_scroll_callback(GLFWwindow* handle, double dx, double dy)
{
    DvzWindow* window = _glfw_window(handle);
    if (window == NULL || window->input_state.replaying)
        return;
    DvzHostInputEvent event = {.type = DVZ_HOST_INPUT_WHEEL, .dx = dx, .dy = dy,
                              .mods = window->glfw_mods,
                              .timestamp_ns = dvz_input_timestamp_ns()};
    glfwGetCursorPos(handle, &event.x, &event.y);
    (void)dvz_window_emit_host_input(window, &event, DVZ_HOST_INPUT_LIVE);
}



/**
 * Observe the layout token used by GUI letter shortcuts without fabricating a Unicode key.
 * This mirrors the GLFW GUI adapter's ASCII layout translation, preserving keypad identity.
 * @param key physical GLFW token
 * @param scancode native scan code
 * @return observed GUI token, with physical fallback for unrepresentable layout keys
 */
static int _glfw_layout_key(int key, int scancode)
{
    if (key >= GLFW_KEY_KP_0 && key <= GLFW_KEY_KP_EQUAL)
        return key;
    GLFWerrorfun previous = glfwSetErrorCallback(NULL);
    const char* name = glfwGetKeyName(key, scancode);
    glfwSetErrorCallback(previous);
    (void)glfwGetError(NULL);
    if (name == NULL || name[0] == 0 || name[1] != 0)
        return key;
    char c = name[0];
    if (c >= '0' && c <= '9')
        return GLFW_KEY_0 + c - '0';
    if (c >= 'A' && c <= 'Z')
        return GLFW_KEY_A + c - 'A';
    if (c >= 'a' && c <= 'z')
        return GLFW_KEY_A + c - 'a';
    const char* names = "`-=[]\\,;\'./";
    const int keys[] = {GLFW_KEY_GRAVE_ACCENT, GLFW_KEY_MINUS, GLFW_KEY_EQUAL,
                        GLFW_KEY_LEFT_BRACKET, GLFW_KEY_RIGHT_BRACKET, GLFW_KEY_BACKSLASH,
                        GLFW_KEY_COMMA, GLFW_KEY_SEMICOLON, GLFW_KEY_APOSTROPHE,
                        GLFW_KEY_PERIOD, GLFW_KEY_SLASH};
    const char* found = strchr(names, c);
    return found != NULL ? keys[found - names] : key;
}



/**
 * Snapshot the modifier state seen by the GUI, independently of callback modifier quirks.
 * @param handle native host
 * @return effective modifier mask
 */
static int _glfw_gui_mods(GLFWwindow* handle)
{
    int mods = 0;
    if (glfwGetKey(handle, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
        glfwGetKey(handle, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS)
        mods |= GLFW_MOD_CONTROL;
    if (glfwGetKey(handle, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
        glfwGetKey(handle, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)
        mods |= GLFW_MOD_SHIFT;
    if (glfwGetKey(handle, GLFW_KEY_LEFT_ALT) == GLFW_PRESS ||
        glfwGetKey(handle, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS)
        mods |= GLFW_MOD_ALT;
    if (glfwGetKey(handle, GLFW_KEY_LEFT_SUPER) == GLFW_PRESS ||
        glfwGetKey(handle, GLFW_KEY_RIGHT_SUPER) == GLFW_PRESS)
        mods |= GLFW_MOD_SUPER;
    return mods;
}



/** Capture physical key state and the observed GUI-layout token independently. */
static void _glfw_key_callback(GLFWwindow* handle, int key, int scancode, int action, int mods)
{
    DvzWindow* window = _glfw_window(handle);
    if (window == NULL || window->input_state.replaying)
        return;
    if (action == GLFW_RELEASE && _dvz_window_cancelled_release(window, true, key))
        return;
    if (action == GLFW_PRESS && key >= 0 && key < 349)
        window->input_cancelled_keys[key] = false;
    DvzHostInputEvent event = {.type = DVZ_HOST_INPUT_KEY, .key = key,
                              .logical_key = _glfw_layout_key(key, scancode),
                              .gui_mods = _glfw_gui_mods(handle),
                              .scancode = scancode, .action = action, .mods = mods,
                              .timestamp_ns = dvz_input_timestamp_ns()};
    (void)dvz_window_emit_host_input(window, &event, DVZ_HOST_INPUT_LIVE);
}



static uint32_t _glfw_utf8_encode(uint32_t codepoint, char out[4])
{
    ANN(out);
    if (codepoint <= 0x7fu)
    {
        out[0] = (char)codepoint;
        return 1;
    }
    if (codepoint <= 0x7ffu)
    {
        out[0] = (char)(0xc0u | (codepoint >> 6));
        out[1] = (char)(0x80u | (codepoint & 0x3fu));
        return 2;
    }
    if (codepoint >= 0xd800u && codepoint <= 0xdfffu)
        return 0;
    if (codepoint <= 0xffffu)
    {
        out[0] = (char)(0xe0u | (codepoint >> 12));
        out[1] = (char)(0x80u | ((codepoint >> 6) & 0x3fu));
        out[2] = (char)(0x80u | (codepoint & 0x3fu));
        return 3;
    }
    if (codepoint <= 0x10ffffu)
    {
        out[0] = (char)(0xf0u | (codepoint >> 18));
        out[1] = (char)(0x80u | ((codepoint >> 12) & 0x3fu));
        out[2] = (char)(0x80u | ((codepoint >> 6) & 0x3fu));
        out[3] = (char)(0x80u | (codepoint & 0x3fu));
        return 4;
    }
    return 0;
}



/** Capture a native committed Unicode scalar as one borrowed UTF-8 span. */
static void _glfw_char_callback(GLFWwindow* handle, unsigned int codepoint)
{
    DvzWindow* window = _glfw_window(handle);
    if (window == NULL || window->input_state.replaying)
        return;
    char utf8[4] = {0};
    uint32_t size = _glfw_utf8_encode(codepoint, utf8);
    DvzHostInputEvent event = {.type = DVZ_HOST_INPUT_TEXT, .utf8 = utf8,
                              .byte_size = size, .mods = window->glfw_mods,
                              .timestamp_ns = dvz_input_timestamp_ns()};
    (void)dvz_window_emit_host_input(window, &event, DVZ_HOST_INPUT_LIVE);
}



/** Capture native focus independently from GUI widget focus. */
static void _glfw_focus_callback(GLFWwindow* handle, int focused)
{
    DvzHostInputEvent event = {.type = DVZ_HOST_INPUT_FOCUS, .focused = focused != 0,
                              .timestamp_ns = dvz_input_timestamp_ns()};
    (void)dvz_window_emit_host_input(_glfw_window(handle), &event, DVZ_HOST_INPUT_LIVE);
}



/** Capture native pointer enter/leave without synthesizing cursor motion. */
static void _glfw_enter_callback(GLFWwindow* handle, int entered)
{
    DvzHostInputEvent event = {.type = DVZ_HOST_INPUT_ENTER, .entered = entered != 0,
                              .timestamp_ns = dvz_input_timestamp_ns()};
    (void)dvz_window_emit_host_input(_glfw_window(handle), &event, DVZ_HOST_INPUT_LIVE);
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



/**
 * Refresh window metrics after a GLFW window moves between monitors.
 *
 * @param handle GLFW window handle
 * @param x new window x coordinate
 * @param y new window y coordinate
 */
static void _glfw_window_pos_callback(GLFWwindow* handle, int x, int y)
{
    (void)x;
    (void)y;
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
    DvzHostInputEvent event = {.type = DVZ_HOST_INPUT_METRICS, .metrics = metrics,
                              .timestamp_ns = dvz_input_timestamp_ns()};
    (void)dvz_window_emit_host_input(window, &event, DVZ_HOST_INPUT_LIVE);
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
    if (config->fit_workarea)
    {
        // Workareas and frame extents use GLFW window coordinates, not framebuffer pixels.
        _glfw_fit_workarea(handle);
        metrics = _glfw_query_metrics(handle, (DvzExtent){0}, config->hidpi_policy, 0, NULL);
    }

    glfwSetCursorPosCallback(handle, _glfw_cursor_pos_callback);
    glfwSetMouseButtonCallback(handle, _glfw_mouse_button_callback);
    glfwSetScrollCallback(handle, _glfw_scroll_callback);
    glfwSetKeyCallback(handle, _glfw_key_callback);
    glfwSetCharCallback(handle, _glfw_char_callback);
    glfwSetWindowFocusCallback(handle, _glfw_focus_callback);
    glfwSetCursorEnterCallback(handle, _glfw_enter_callback);
    window->input_state.focused = glfwGetWindowAttrib(handle, GLFW_FOCUSED) != 0;
    window->input_state.entered = glfwGetWindowAttrib(handle, GLFW_HOVERED) != 0;
    glfwGetCursorPos(handle, &window->input_state.x, &window->input_state.y);
    for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; key++)
    {
        window->input_keys[key] = glfwGetKey(handle, key) == GLFW_PRESS;
        if (window->input_keys[key])
            window->input_state.held_key_count++;
    }
    window->input_state.mods = window->glfw_mods = _glfw_gui_mods(handle);
    for (int button = 0; button <= GLFW_MOUSE_BUTTON_LAST; button++)
        if (glfwGetMouseButton(handle, button) == GLFW_PRESS)
            window->input_state.buttons |= 1u << button;
    glfwSetFramebufferSizeCallback(handle, _glfw_framebuffer_callback);
    glfwSetWindowSizeCallback(handle, _glfw_window_size_callback);
    glfwSetWindowPosCallback(handle, _glfw_window_pos_callback);
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
    DvzHostInputEvent event = {.type = DVZ_HOST_INPUT_METRICS, .metrics = metrics,
                              .timestamp_ns = dvz_input_timestamp_ns()};
    (void)dvz_window_emit_host_input(window, &event, DVZ_HOST_INPUT_LIVE);
    if (config->visible)
    {
        glfwShowWindow(handle);
        if (config->fit_workarea)
        {
            // Some window managers report decoration extents only after the window is mapped.
            _glfw_fit_workarea(handle);
            metrics = _glfw_query_metrics(handle, (DvzExtent){0}, config->hidpi_policy, 0, NULL);
            _dvz_window_backend_emit_metrics(window, &metrics);
        }
    }
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
    {
#if defined(__APPLE__)
        glfwSetWindowShouldClose(handle, GLFW_TRUE);
        glfwHideWindow(handle);
        if (_glfw_state.initialized)
        {
            glfwPollEvents();
            glfwWaitEventsTimeout(0.001);
        }
#endif
        glfwDestroyWindow(handle);
        if (_glfw_state.initialized)
        {
            glfwPollEvents();
#if defined(__APPLE__)
            for (uint32_t i = 0; i < 4; i++)
                glfwWaitEventsTimeout(0.001);
#endif
        }
    }
    if (_glfw_state.window_count > 0)
        _glfw_state.window_count--;
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
 * Restore observable live input state after exclusive replay without inventing button presses.
 * @param window input host, ignored for non-GLFW backends
 */
void _dvz_window_glfw_refresh_input(DvzWindow* window)
{
#if DVZ_HAS_GLFW
    if (window == NULL || dvz_window_backend_type(window) != DVZ_BACKEND_GLFW)
        return;
    GLFWwindow* handle = (GLFWwindow*)window->backend_handle;
    if (handle == NULL)
        return;
    window->input_state.focused = glfwGetWindowAttrib(handle, GLFW_FOCUSED) != 0;
    window->input_state.entered = glfwGetWindowAttrib(handle, GLFW_HOVERED) != 0;
    glfwGetCursorPos(handle, &window->input_state.x, &window->input_state.y);
    window->input_state.held_key_count = 0;
    window->input_state.buttons = 0;
    window->input_cancelled_buttons = 0;
    for (int key = 0; key <= GLFW_KEY_LAST; key++)
    {
        window->input_keys[key] = key >= GLFW_KEY_SPACE && glfwGetKey(handle, key) == GLFW_PRESS;
        window->input_cancelled_keys[key] = false;
        if (window->input_keys[key])
            window->input_state.held_key_count++;
    }
    for (int button = 0; button <= GLFW_MOUSE_BUTTON_LAST; button++)
        if (glfwGetMouseButton(handle, button) == GLFW_PRESS)
            window->input_state.buttons |= 1u << button;
    window->input_state.mods = window->glfw_mods = _glfw_gui_mods(handle);
    // Reconnect the GUI backend's cached hover/focus after suppressed physical callbacks.
    if (window->input_dispatch != NULL)
    {
        DvzHostInputEvent event = {.type = DVZ_HOST_INPUT_FOCUS,
                                  .focused = window->input_state.focused,
                                  .timestamp_ns = dvz_input_timestamp_ns()};
        (void)window->input_dispatch(window, &event, window->input_dispatch_data);
        event.type = DVZ_HOST_INPUT_ENTER;
        event.entered = window->input_state.entered;
        (void)window->input_dispatch(window, &event, window->input_dispatch_data);
    }
#else
    (void)window;
#endif
}



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


/**
 * Terminate GLFW when no GLFW windows remain.
 */
void _dvz_window_glfw_shutdown(void)
{
#if DVZ_HAS_GLFW
    _glfw_shutdown();
#endif
}



/**
 * Interrupt a GLFW event wait from any thread without touching window-owned state.
 */
void _dvz_window_glfw_wake(void)
{
#if DVZ_HAS_GLFW
    if (_glfw_state.initialized)
        glfwPostEmptyEvent();
#endif
}
