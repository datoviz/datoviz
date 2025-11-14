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



#if DVZ_WITH_GLFW
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

typedef struct DvzGlfwBackendState
{
    bool initialized;
    uint32_t window_count;
} DvzGlfwBackendState;

static DvzGlfwBackendState _dvz_glfw_state = {0};

static DvzWindow* _dvz_glfw_window(GLFWwindow* handle)
{
    return (DvzWindow*)glfwGetWindowUserPointer(handle);
}



static bool _dvz_glfw_init(void)
{
    if (_dvz_glfw_state.initialized)
        return true;
    if (!glfwInit())
    {
        log_error("glfwInit() failed");
        return false;
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    _dvz_glfw_state.initialized = true;
    return true;
}



static void _dvz_glfw_shutdown(void)
{
    if (_dvz_glfw_state.window_count == 0 && _dvz_glfw_state.initialized)
    {
        glfwTerminate();
        _dvz_glfw_state.initialized = false;
    }
}



static void _dvz_glfw_emit_pointer(
    GLFWwindow* handle, DvzPointerEventType type, DvzPointerButton button, int mods)
{
    DvzWindow* window = _dvz_glfw_window(handle);
    if (window == NULL)
        return;
    DvzInputRouter* router = dvz_window_backend_router(window);
    if (router == NULL)
        return;
    double xpos = 0.0;
    double ypos = 0.0;
    glfwGetCursorPos(handle, &xpos, &ypos);
    int win_width = 0;
    int win_height = 0;
    glfwGetWindowSize(handle, &win_width, &win_height);
    dvz_pointer_emit_position(
        router, type, (float)xpos, (float)ypos, (float)win_width, (float)win_height, button, mods,
        window->surface.scale_x, dvz_input_timestamp_ns(), dvz_window_user_data(window));
}



static void _dvz_glfw_cursor_pos_callback(GLFWwindow* handle, double xpos, double ypos)
{
    (void)xpos;
    (void)ypos;
    _dvz_glfw_emit_pointer(handle, DVZ_POINTER_EVENT_MOVE, DVZ_POINTER_BUTTON_NONE, 0);
}



static void _dvz_glfw_mouse_button_callback(GLFWwindow* handle, int button, int action, int mods)
{
    DvzPointerButton dvz_button = dvz_pointer_button_from_glfw(button);
    DvzPointerEventType type =
        (action == GLFW_PRESS) ? DVZ_POINTER_EVENT_PRESSED : DVZ_POINTER_EVENT_RELEASED;
    _dvz_glfw_emit_pointer(handle, type, dvz_button, mods);
}



static void _dvz_glfw_scroll_callback(GLFWwindow* handle, double dx, double dy)
{
    DvzWindow* window = _dvz_glfw_window(handle);
    if (window == NULL)
        return;
    DvzInputRouter* router = dvz_window_backend_router(window);
    if (router == NULL)
        return;
    double xpos = 0.0;
    double ypos = 0.0;
    glfwGetCursorPos(handle, &xpos, &ypos);
    int win_width = 0;
    int win_height = 0;
    glfwGetWindowSize(handle, &win_width, &win_height);
    dvz_pointer_emit_wheel(
        router, (float)xpos, (float)ypos, (float)win_width, (float)win_height, (float)dx,
        (float)dy, 0, window->surface.scale_x, dvz_input_timestamp_ns(),
        dvz_window_user_data(window));
}



static void _dvz_glfw_key_callback(GLFWwindow* handle, int key, int scancode, int action, int mods)
{
    (void)scancode;
    DvzWindow* window = _dvz_glfw_window(handle);
    if (window == NULL)
        return;
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



static void _dvz_glfw_framebuffer_callback(GLFWwindow* handle, int width, int height)
{
    DvzWindow* window = _dvz_glfw_window(handle);
    if (window == NULL)
        return;
    int win_width = 0;
    int win_height = 0;
    glfwGetWindowSize(handle, &win_width, &win_height);
    float scale_x = 1.f;
    float scale_y = 1.f;
    glfwGetWindowContentScale(handle, &scale_x, &scale_y);
    dvz_window_backend_emit_resize(
        window, (uint32_t)width, (uint32_t)height, (uint32_t)win_width, (uint32_t)win_height,
        scale_x, scale_y);
}



static void _dvz_glfw_scale_callback(GLFWwindow* handle, float scale_x, float scale_y)
{
    DvzWindow* window = _dvz_glfw_window(handle);
    if (window == NULL)
        return;
    dvz_window_backend_emit_scale(window, scale_x, scale_y);
}



static bool _dvz_glfw_probe(DvzWindowBackend* backend, DvzWindowHost* host)
{
    (void)backend;
    (void)host;
    return true;
}



static bool
_dvz_glfw_create(DvzWindowBackend* backend, DvzWindow* window, const DvzWindowConfig* config)
{
    (void)backend;
    ANN(window);
    ANN(config);
    if (!_dvz_glfw_init())
        return false;
    GLFWwindow* handle = glfwCreateWindow(
        (int)config->width, (int)config->height, config->title ? config->title : "", NULL, NULL);
    if (handle == NULL)
    {
        log_error("failed to create GLFW window");
        return false;
    }
    glfwSetWindowUserPointer(handle, window);
    glfwSetCursorPosCallback(handle, _dvz_glfw_cursor_pos_callback);
    glfwSetMouseButtonCallback(handle, _dvz_glfw_mouse_button_callback);
    glfwSetScrollCallback(handle, _dvz_glfw_scroll_callback);
    glfwSetKeyCallback(handle, _dvz_glfw_key_callback);
    glfwSetFramebufferSizeCallback(handle, _dvz_glfw_framebuffer_callback);
    glfwSetWindowContentScaleCallback(handle, _dvz_glfw_scale_callback);
    dvz_window_backend_set_handle(window, handle);
    int fb_width = 0;
    int fb_height = 0;
    glfwGetFramebufferSize(handle, &fb_width, &fb_height);
    int win_width = 0;
    int win_height = 0;
    glfwGetWindowSize(handle, &win_width, &win_height);
    float scale_x = 1.f;
    float scale_y = 1.f;
    glfwGetWindowContentScale(handle, &scale_x, &scale_y);
    dvz_window_backend_emit_resize(
        window, (uint32_t)fb_width, (uint32_t)fb_height, (uint32_t)win_width, (uint32_t)win_height,
        scale_x, scale_y);
    _dvz_glfw_state.window_count++;
    return true;
}



static void _dvz_glfw_destroy(DvzWindowBackend* backend, DvzWindow* window)
{
    (void)backend;
    if (window == NULL)
        return;
    GLFWwindow* handle = (GLFWwindow*)dvz_window_backend_handle(window);
    if (handle != NULL)
        glfwDestroyWindow(handle);
    if (_dvz_glfw_state.window_count > 0)
        _dvz_glfw_state.window_count--;
    _dvz_glfw_shutdown();
}



static void _dvz_glfw_poll(DvzWindowBackend* backend, DvzWindowHost* host)
{
    (void)backend;
    (void)host;
    if (_dvz_glfw_state.initialized)
        glfwPollEvents();
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
        .user_data = &_dvz_glfw_state,
        .procs =
            {
                .probe = _dvz_glfw_probe,
                .create = _dvz_glfw_create,
                .destroy = _dvz_glfw_destroy,
                .poll = _dvz_glfw_poll,
                .request_frame = NULL,
            },
    };
    dvz_window_host_register_backend(host, &backend);
}

#else

static bool _dvz_glfw_disabled_probe(DvzWindowBackend* backend, DvzWindowHost* host)
{
    (void)backend;
    (void)host;
    return false;
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
                .probe = _dvz_glfw_disabled_probe,
                .create = NULL,
                .destroy = NULL,
                .poll = NULL,
                .request_frame = NULL,
            },
    };
    dvz_window_host_register_backend(host, &backend);
}

#endif
