/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  GLFW input                                                                                   */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "input.h"
#include "backend.h"
#include "common.h"
#include "datoviz.h"
#include "datoviz_app.h"
#include "datoviz_types.h"
#include "keyboard.h"
#include "mouse.h"
#include "window.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static DvzMouseButton _from_glfw_button(int button)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
        return DVZ_MOUSE_BUTTON_LEFT;
    else if (button == GLFW_MOUSE_BUTTON_RIGHT)
        return DVZ_MOUSE_BUTTON_RIGHT;
    else if (button == GLFW_MOUSE_BUTTON_MIDDLE)
        return DVZ_MOUSE_BUTTON_MIDDLE;
    else
        return DVZ_MOUSE_BUTTON_NONE;
}



// Update the mouse time.
static inline void _clock(DvzInput* input)
{
    ANN(input);
    dvz_mouse_tick(input->mouse, dvz_clock_get(&input->clock));
}



static void _glfw_move_callback(GLFWwindow* window, double xpos, double ypos)
{
    ANN(window);
    DvzInput* input = (DvzInput*)glfwGetWindowUserPointer(window);
    ANN(input);
    ANN(input->mouse);
    ANN(input->keyboard);
    _clock(input);

    // TODO: implement debouncer or throttler
    vec2 pos = {xpos, ypos};
    dvz_mouse_move(input->mouse, pos, input->keyboard->mods);
}



static void _glfw_button_callback(GLFWwindow* window, int b, int action, int mods)
{
    ANN(window);
    DvzInput* input = (DvzInput*)glfwGetWindowUserPointer(window);
    ANN(input);
    ANN(input->mouse);
    ANN(input->keyboard);
    _clock(input);

    DvzMouseButton button = _from_glfw_button(b);
    if (action == GLFW_PRESS)
    {
        dvz_mouse_press(input->mouse, button, input->keyboard->mods);
    }
    else if (action == GLFW_RELEASE)
    {
        dvz_mouse_release(input->mouse, button, input->keyboard->mods);
    }
}



static void _glfw_wheel_callback(GLFWwindow* window, double dx, double dy)
{
    ANN(window);
    DvzInput* input = (DvzInput*)glfwGetWindowUserPointer(window);
    ANN(input);
    ANN(input->mouse);
    ANN(input->keyboard);
    _clock(input);

    vec2 dir = {dx, dy};
    dvz_mouse_wheel(input->mouse, dir, input->keyboard->mods);
}



static void _glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    ANN(window);
    DvzInput* input = (DvzInput*)glfwGetWindowUserPointer(window);
    ANN(input);
    ANN(input->mouse);
    ANN(input->keyboard);
    _clock(input);

    // IMPORTANT NOTE: here, we assume that the GLFW keycode and the Datoviz DvzKeyCode match!
    if (action == GLFW_PRESS)
        dvz_keyboard_press(input->keyboard, (DvzKeyCode)key);
    else if (action == GLFW_RELEASE)
        dvz_keyboard_release(input->keyboard, (DvzKeyCode)key);
    else if (action == GLFW_REPEAT)
        dvz_keyboard_repeat(input->keyboard, (DvzKeyCode)key);
}



/*************************************************************************************************/
/*  Window input functions                                                                       */
/*************************************************************************************************/

DvzInput* dvz_input(DvzWindow* window)
{
    ANN(window);

    DvzInput* input = (DvzInput*)calloc(1, sizeof(DvzInput));
    input->mouse = dvz_mouse();
    input->keyboard = dvz_keyboard();
    input->clock = dvz_clock();
    input->window = window;

    GLFWwindow* w = window->backend_window;
    ANN(w);

    glfwSetWindowUserPointer(w, input);

    glfwSetCursorPosCallback(w, _glfw_move_callback);
    glfwSetMouseButtonCallback(w, _glfw_button_callback);
    glfwSetScrollCallback(w, _glfw_wheel_callback);
    glfwSetKeyCallback(w, _glfw_key_callback);

    return input;
}



DvzMouse* dvz_input_mouse(DvzInput* input)
{
    ANN(input);
    return input->mouse;
}



DvzKeyboard* dvz_input_keyboard(DvzInput* input)
{
    ANN(input);
    return input->keyboard;
}



void dvz_input_destroy(DvzInput* input)
{
    ANN(input);
    log_trace("destroy the input");

    dvz_backend_poll_events(DVZ_BACKEND_GLFW);

    ANN(input->window);
    GLFWwindow* w = input->window->backend_window;
    ANN(w);

    dvz_backend_window_clear_callbacks(DVZ_BACKEND_GLFW, w);

    dvz_mouse_destroy(input->mouse);
    dvz_keyboard_destroy(input->keyboard);
    FREE(input);
}



/*************************************************************************************************/
/*  Synchronous input functions                                                                  */
/*************************************************************************************************/

void dvz_window_mouse(DvzWindow* window, double* x, double* y, DvzMouseButton* button)
{
    ANN(window);

    GLFWwindow* w = (GLFWwindow*)window->backend_window;
    ANN(w);

    // Get mouse position.
    if (x != NULL && y != NULL)
        glfwGetCursorPos(w, x, y);

    // Get mouse pressed button.
    if (button != NULL)
    {
        // NOTE: this function signature can only report 1 pressed button at a time.
        if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
            *button = DVZ_MOUSE_BUTTON_LEFT;

        if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS)
            *button = DVZ_MOUSE_BUTTON_MIDDLE;

        if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
            *button = DVZ_MOUSE_BUTTON_RIGHT;
    }
}



void dvz_window_keyboard(DvzWindow* window, DvzKeyCode* key)
{
    ANN(window);
    if (key == NULL)
        return;

    GLFWwindow* w = (GLFWwindow*)window->backend_window;
    ANN(w);

    DvzKeyCode k = (DvzKeyCode)0;

    k = DVZ_KEY_SPACE;
    if (glfwGetKey(w, k) == GLFW_PRESS)
    {
        *key = k;
        return;
    }

    k = DVZ_KEY_APOSTROPHE;
    if (glfwGetKey(w, k) == GLFW_PRESS)
    {
        *key = k;
        return;
    }

    for (k = DVZ_KEY_COMMA; (int)k <= (int)DVZ_KEY_GRAVE_ACCENT; k = (DvzKeyCode)(k + 1))
    {
        if (glfwGetKey(w, k) == GLFW_PRESS)
        {
            *key = k;
            return;
        }
    }

    for (k = DVZ_KEY_ESCAPE; (int)k <= (int)DVZ_KEY_LAST; k = (DvzKeyCode)(k + 1))
    {
        if (glfwGetKey(w, k) == GLFW_PRESS)
        {
            *key = k;
            return;
        }
    }
}



/*************************************************************************************************/
/*  Time functions                                                                               */
/*************************************************************************************************/

void dvz_time(DvzTime* time)
{
    ANN(time)

#if OS_WINDOWS

    LARGE_INTEGER frequency, counter;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    uint64_t nanoseconds = (long long unsigned int)(counter.QuadPart * 1000000000) /
                           (long long unsigned int)frequency.QuadPart;
    time->seconds = nanoseconds / 1000000000;
    time->nanoseconds = nanoseconds % 1000000000;

#elif OS_MACOS

    static mach_timebase_info_data_t timebase;
    if (timebase.denom == 0)
    {
        mach_timebase_info(&timebase);
    }
    uint64_t t = mach_absolute_time();
    uint64_t nanoseconds = t * timebase.numer / timebase.denom;
    time->seconds = nanoseconds / 1000000000;
    time->nanoseconds = nanoseconds % 1000000000;

#elif OS_LINUX

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    time->seconds = (uint64_t)ts.tv_sec;
    time->nanoseconds = (uint64_t)ts.tv_nsec;

#endif

    // dvz_time_print(time);
}



void dvz_time_print(DvzTime* time)
{
    ANN(time);
    printf(
        "%03" PRIu64 " s %03" PRIu64 " ms %03" PRIu64 " us %03" PRIu64 " ns\n", time->seconds,
        time->nanoseconds / 1000000,          // Milliseconds
        (time->nanoseconds % 1000000) / 1000, // Microseconds
        time->nanoseconds % 1000              // Nanoseconds
    );
}
