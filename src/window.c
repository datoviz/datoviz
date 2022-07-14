/*************************************************************************************************/
/*  Window API                                                                                   */
/*************************************************************************************************/

#include "window.h"
#include "_glfw.h"
#include "common.h"



/*************************************************************************************************/
/*  Window                                                                                       */
/*************************************************************************************************/

DvzWindow dvz_window(DvzBackend backend, uint32_t width, uint32_t height, int flags)
{
    DvzWindow window = {0};
    dvz_obj_init(&window.obj);
    window.obj.type = DVZ_OBJECT_TYPE_WINDOW;

    window.width = width;
    window.height = height;
    // window.close_on_esc = true;

    // Create the window, depending on the backend.
    window.backend_window = backend_window(backend, width, height, flags);

    // Set initialize size.
    backend_get_window_size(&window, &window.width, &window.height);

    dvz_obj_created(&window.obj);
    return window;
}



void dvz_window_poll_size(DvzWindow* window)
{
    ASSERT(window != NULL);
    backend_get_window_size(window, &window->width, &window->height);
}



void dvz_window_set_size(DvzWindow* window, uint32_t width, uint32_t height)
{
    ASSERT(window != NULL);
    backend_set_window_size(window, width, height);
    backend_get_window_size(window, &window->width, &window->height);
}



void dvz_window_destroy(DvzWindow* window)
{
    if (window == NULL || window->obj.status == DVZ_OBJECT_STATUS_DESTROYED)
    {
        log_trace("skip destruction of already-destroyed window");
        return;
    }
    ASSERT(window != NULL);
    backend_window_destroy(window->backend, window->backend_window);
    dvz_obj_destroyed(&window->obj);
}
