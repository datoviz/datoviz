/*************************************************************************************************/
/*  GLFW input                                                                                   */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "input.h"
#include "common.h"
#include "glfw_utils.h"
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
        dvz_mouse_press(input->mouse, button, input->keyboard->mods);
    else if (action == GLFW_RELEASE)
        dvz_mouse_release(input->mouse, button, input->keyboard->mods);
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
    dvz_mouse_destroy(input->mouse);
    dvz_keyboard_destroy(input->keyboard);
    FREE(input);
}
