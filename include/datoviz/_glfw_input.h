/*************************************************************************************************/
/*  Vklite utils                                                                                 */
/*************************************************************************************************/

#ifndef DVZ_HEADER_GLFW_INPUT
#define DVZ_HEADER_GLFW_INPUT



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_glfw.h"
#include "common.h"
#include "input.h"
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



static void _glfw_move_callback(GLFWwindow* window, double xpos, double ypos)
{
    ASSERT(window != NULL);
    DvzInput* input = (DvzInput*)glfwGetWindowUserPointer(window);
    ASSERT(input != NULL);

    // NOTE: throttle the mouse move calls to avoid clogging the queue.
    double time = dvz_clock_get(&input->clock);
    if (time - input->mouse.last_move < DVZ_MOUSE_MOVE_MIN_DELAY)
        return;
    dvz_deq_discard(&input->deq, DVZ_INPUT_DEQ_MOUSE, DVZ_MOUSE_MOVE_MAX_PENDING);

    DvzEvent ev = {0};
    ev.content.m.pos[0] = xpos;
    ev.content.m.pos[1] = ypos;
    ev.mods = input->mouse.mods;
    dvz_input_event(input, DVZ_EVENT_MOUSE_MOVE, ev, false);

    input->mouse.last_move = time;
}



static void _glfw_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    ASSERT(window != NULL);
    DvzInput* input = (DvzInput*)glfwGetWindowUserPointer(window);
    DvzEvent ev = {0};
    ev.content.b.button = _from_glfw_button(button);
    ev.mods = mods;
    DvzEventType evtype = action == GLFW_PRESS ? DVZ_EVENT_MOUSE_PRESS : DVZ_EVENT_MOUSE_RELEASE;
    dvz_input_event(input, evtype, ev, false);
}



static void _glfw_wheel_callback(GLFWwindow* window, double dx, double dy)
{
    ASSERT(window != NULL);
    DvzInput* input = (DvzInput*)glfwGetWindowUserPointer(window);
    DvzEvent ev = {0};

    ev.content.w.dir[0] = dx;
    ev.content.w.dir[1] = dy;
    ev.mods = input->mouse.mods;
    dvz_input_event(input, DVZ_EVENT_MOUSE_WHEEL, ev, false);
}



static void _glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    ASSERT(window != NULL);
    DvzInput* input = (DvzInput*)glfwGetWindowUserPointer(window);
    DvzEvent ev = {0};

    DvzEventType type =
        action == GLFW_RELEASE ? DVZ_EVENT_KEYBOARD_RELEASE : DVZ_EVENT_KEYBOARD_PRESS;

    // NOTE: we use the GLFW key codes here, should actually do a proper mapping between GLFW
    // key codes and Datoviz key codes.
    ev.content.k.key_code = key;
    ev.mods = mods;

    dvz_input_event(input, type, ev, false);
}



/*************************************************************************************************/
/*  Input-window binding                                                                         */
/*************************************************************************************************/

static void backend_attach(DvzInput* input, DvzWindow* window)
{
    ASSERT(input != NULL);
    ASSERT(window != NULL);

    if (window->backend_window != NULL)
        glfwSetWindowUserPointer(window->backend_window, input);
}



static void backend_attach_mouse(DvzMouse* mouse, DvzWindow* window)
{
    ASSERT(mouse != NULL);
    ASSERT(window != NULL);

    GLFWwindow* w = window->backend_window;
    ASSERT(w != NULL);

    // Register the mouse move callback.
    // TODO: comment?? if commented, see _glfw_frame_callback
    glfwSetCursorPosCallback(w, _glfw_move_callback);

    // Register the mouse button callback.
    glfwSetMouseButtonCallback(w, _glfw_button_callback);

    // Register the mouse wheel callback.
    glfwSetScrollCallback(w, _glfw_wheel_callback);
}



static void backend_attach_keyboard(DvzKeyboard* keyboard, DvzWindow* window)
{
    ASSERT(keyboard != NULL);
    ASSERT(window != NULL);

    GLFWwindow* w = window->backend_window;
    ASSERT(w != NULL);

    glfwSetKeyCallback(w, _glfw_key_callback);
}



#endif
