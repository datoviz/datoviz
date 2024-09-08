/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  GLFW utils                                                                                 */
/*************************************************************************************************/

#ifndef DVZ_HEADER_GLFW
#define DVZ_HEADER_GLFW



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#ifndef HAS_GLFW
#define HAS_GLFW 0
#endif
#if HAS_GLFW
#include <GLFW/glfw3.h>
#endif

#include "window.h"



/*************************************************************************************************/
/*  Backend-specific initialization */
/*************************************************************************************************/

static void _glfw_error(int error_code, const char* description)
{
    log_error("glfw error code #%d: %s", error_code, description);
}



static void backend_init(DvzBackend backend)
{
    ASSERT(backend != DVZ_BACKEND_NONE);

    switch (backend)
    {
    case DVZ_BACKEND_GLFW:
    {
#if HAS_GLFW
        log_debug("initialize glfw");
        glfwSetErrorCallback(_glfw_error);
        if (!glfwInit())
        {
            exit(1);
        }
#endif
        break;
    }
    default:
        break;
    }
}



static void backend_terminate(DvzBackend backend)
{
    ASSERT(backend != DVZ_BACKEND_NONE);

    switch (backend)
    {
    case DVZ_BACKEND_GLFW:
    {
#if HAS_GLFW
        log_debug("terminate glfw");
        glfwTerminate();
#endif
        break;
    }
    default:
        break;
    }
}



/*************************************************************************************************/
/*  Backend-specific code                                                                        */
/*************************************************************************************************/

// static void
// _glfw_esc_callback(GLFWwindow* backend_window, int key, int scancode, int action, int mods)
// {
// #if HAS_GLFW
//     // WARNING: this callback is only valid for DvzWindows that are not wrapped inside a
//     DvzCanvas
//     // This is because the DvzCanvas has its own glfw keyboard callback, and there can be
//     only 1. DvzWindow* window = (DvzWindow*)glfwGetWindowUserPointer(backend_window);
//     ANN(window);
//     if (window->close_on_esc && action == GLFW_PRESS && key == GLFW_KEY_ESCAPE)
//     {
//         window->obj.status = DVZ_OBJECT_STATUS_NEED_DESTROY;
//     }
// #endif
// }



static void* backend_window(DvzBackend backend, uint32_t width, uint32_t height, int flags)
{
    ASSERT(backend != DVZ_BACKEND_NONE);

    ASSERT(width > 0);
    ASSERT(height > 0);

    log_trace("create window with size %dx%d", width, height);

    switch (backend)
    {
    case DVZ_BACKEND_GLFW:
    {
#if HAS_GLFW
        log_trace("init glfw if needed");
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        ASSERT(width > 0);
        ASSERT(height > 0);

        // Invisible window.
        if ((flags & DVZ_WINDOW_FLAGS_HIDDEN))
        {
            glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        }

        GLFWwindow* bwin = glfwCreateWindow((int)width, (int)height, APPLICATION_NAME, NULL, NULL);
        log_trace("created glfw window %x", (uint64_t)bwin);
        ANN(bwin);

        // Visible window.
        if ((flags & DVZ_WINDOW_FLAGS_HIDDEN))
        {
            glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
        }

        // glfwSetWindowUserPointer(bwin, window);
        // Callback that marks the window to close if ESC is pressed, but only if
        // DvzWindow.close_on_esc=true
        // glfwSetKeyCallback(bwin, _glfw_esc_callback);

        return bwin;
#endif
        break;
    }
    default:
        break;
    }

    return NULL;
}



static void backend_poll_events(DvzBackend backend)
{
    ASSERT(backend != DVZ_BACKEND_NONE);

    switch (backend)
    {
    case DVZ_BACKEND_GLFW:
    {
#if HAS_GLFW
        glfwPollEvents();
#endif
        break;
    }
    default:
        break;
    }
}



static void backend_wait(DvzBackend backend)
{
    ASSERT(backend != DVZ_BACKEND_NONE);

    switch (backend)
    {
    case DVZ_BACKEND_GLFW:
    {
#if HAS_GLFW
        glfwWaitEvents();
#endif
        break;
    }
    default:
        break;
    }
}



static void backend_window_clear_callbacks(DvzBackend backend, void* bwin)
{
    ASSERT(backend != DVZ_BACKEND_NONE);
    ANN(bwin);

    log_trace("removing window input callbacks");
    switch (backend)
    {
    case DVZ_BACKEND_GLFW:
    {
#if HAS_GLFW
        GLFWwindow* window = (GLFWwindow*)bwin;

        glfwSetWindowFocusCallback(window, NULL);
        glfwSetCursorEnterCallback(window, NULL);
        glfwSetCursorPosCallback(window, NULL);
        glfwSetMouseButtonCallback(window, NULL);
        glfwSetScrollCallback(window, NULL);
        glfwSetKeyCallback(window, NULL);
        glfwSetCharCallback(window, NULL);

        glfwPollEvents();
#endif
        break;
    }
    default:
        break;
    }
}



static void backend_window_destroy(DvzBackend backend, void* bwin)
{
    ASSERT(backend != DVZ_BACKEND_NONE);
    ANN(bwin);

    // NOTE TODO: need to vkDeviceWaitIdle(device) on all devices before calling this
    log_trace("starting destruction of backend window...");
    switch (backend)
    {
    case DVZ_BACKEND_GLFW:
    {
#if HAS_GLFW
        // NOTE: this call leads to a crash with GLFW when input events (eg mouse) are still in the
        // queue while the window is being destroyed.
        // glfwPollEvents();
        log_trace("destroy GLFW window %x", (uint64_t)bwin);
        glfwDestroyWindow((GLFWwindow*)bwin);
#endif
        break;
    }
    default:
        break;
    }

    log_trace("backend window destroyed");
}



/*************************************************************************************************/
/*  DvzWindow helpers                                                                            */
/*************************************************************************************************/

static void backend_set_window_size(DvzWindow* window, uint32_t width, uint32_t height)
{
    log_trace("setting the size of backend window...");

    ANN(window);
    void* bwin = window->backend_window;

    DvzBackend backend = window->backend;
    ASSERT(backend != DVZ_BACKEND_NONE);

    switch (backend)
    {
    case DVZ_BACKEND_GLFW:
    {
#if HAS_GLFW
        ANN(bwin);
        int w = (int)width, h = (int)height;
        log_trace("set window size to %dx%d", w, h);
        glfwSetWindowSize((GLFWwindow*)bwin, w, h);
#endif
        break;
    }
    default:
        break;
    }
}



static void
backend_get_window_size(DvzWindow* window, uint32_t* window_width, uint32_t* window_height)
{
    log_trace("determining the size of backend window...");

    ANN(window);
    void* bwin = window->backend_window;

    DvzBackend backend = window->backend;
    ASSERT(backend != DVZ_BACKEND_NONE);

    switch (backend)
    {
    case DVZ_BACKEND_GLFW:
    {
#if HAS_GLFW
        int w, h;
        ANN(bwin);

        // Get window size.
        glfwGetWindowSize((GLFWwindow*)bwin, &w, &h);
        while (w == 0 || h == 0)
        {
            log_trace("waiting for end of window resize event");
            glfwGetWindowSize((GLFWwindow*)bwin, &w, &h);
            glfwWaitEvents();
        }
        ASSERT(w > 0);
        ASSERT(h > 0);
        *window_width = (uint32_t)w;
        *window_height = (uint32_t)h;
        log_trace("window size is %dx%d", w, h);
#endif
        break;
    }

    default:
        break;
    }
}



static void backend_get_framebuffer_size(
    DvzWindow* window, uint32_t* framebuffer_width, uint32_t* framebuffer_height)
{
    log_trace("determining the size of backend window...");

    ANN(window);

    DvzBackend backend = window->backend;
    ASSERT(backend != DVZ_BACKEND_NONE);

    switch (backend)
    {
    case DVZ_BACKEND_GLFW:
    {
#if HAS_GLFW
        int w, h;
        void* bwin = window->backend_window;
        ANN(bwin);

        // Get framebuffer size.
        glfwGetFramebufferSize((GLFWwindow*)bwin, &w, &h);
        while (w == 0 || h == 0)
        {
            log_trace("waiting for end of framebuffer resize event");
            glfwGetFramebufferSize((GLFWwindow*)bwin, &w, &h);
            glfwWaitEvents();
        }
        ASSERT(w > 0);
        ASSERT(h > 0);
        *framebuffer_width = (uint32_t)w;
        *framebuffer_height = (uint32_t)h;
        log_trace("framebuffer size is %dx%d", w, h);
#endif
        break;
    }

    default:
        break;
    }
}



static bool backend_should_close(DvzWindow* window)
{
    ANN(window);
    void* bwin = window->backend_window;

    DvzBackend backend = window->backend;
    ASSERT(backend != DVZ_BACKEND_NONE);

    switch (backend)
    {
    case DVZ_BACKEND_GLFW:
    {
#if HAS_GLFW
        if (bwin != NULL)
            return glfwWindowShouldClose((GLFWwindow*)bwin);
#endif
        break;
    }
    default:
        break;
    }
    return false;
}



static void backend_loop(DvzWindow* window, uint64_t max_frames)
{
    ANN(window);
    void* bwin = window->backend_window;

    DvzBackend backend = window->backend;
    ASSERT(backend != DVZ_BACKEND_NONE);

    switch (backend)
    {
    case DVZ_BACKEND_GLFW:
    {
#if HAS_GLFW
        for (uint64_t i = 0; max_frames == 0 || i < max_frames; i++)
        {
            if (glfwWindowShouldClose((GLFWwindow*)bwin))
                break;
            glfwPollEvents();
        }
#endif
        break;
    }
    default:
        break;
    }
}



#endif
