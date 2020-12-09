#include "../include/visky/interact2.h"
#include "utils.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Interact tests                                                                               */
/*************************************************************************************************/

static void _mouse_callback(VklCanvas* canvas, VklEvent ev)
{
    ASSERT(canvas != NULL);
    VklMouseState* mouse = (VklMouseState*)ev.user_data;
    ASSERT(mouse != NULL);
    VklViewport viewport = vkl_viewport_full(canvas);
    vkl_mouse_event(mouse, canvas, viewport, ev);
}

static void _keyboard_callback(VklCanvas* canvas, VklEvent ev)
{
    ASSERT(canvas != NULL);
    VklKeyboardState* keyboard = (VklKeyboardState*)ev.user_data;
    ASSERT(keyboard != NULL);
    VklViewport viewport = vkl_viewport_full(canvas);
    vkl_keyboard_event(keyboard, canvas, viewport, ev);
}

static int vklite2_interact_1(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT);

    VklMouseState mouse = vkl_mouse();
    VklKeyboardState keyboard = vkl_keyboard();

    vkl_event_callback(canvas, VKL_EVENT_MOUSE_MOVE, 0, _mouse_callback, &mouse);
    vkl_event_callback(canvas, VKL_EVENT_MOUSE_BUTTON, 0, _mouse_callback, &mouse);
    vkl_event_callback(canvas, VKL_EVENT_MOUSE_WHEEL, 0, _mouse_callback, &mouse);
    vkl_event_callback(canvas, VKL_EVENT_KEY, 0, _keyboard_callback, &keyboard);

    vkl_app_run(app, 0);

    TEST_END
}
