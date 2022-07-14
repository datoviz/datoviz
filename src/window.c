/*************************************************************************************************/
/*  Window API                                                                                   */
/*************************************************************************************************/

#include "window.h"
#include "_glfw.h"
#include "common.h"
#include "host.h"
#include "vklite.h"
#include "vkutils.h"



/*************************************************************************************************/
/*  Window                                                                                       */
/*************************************************************************************************/

void dvz_window_create(DvzHost* host, DvzWindow* window, uint32_t width, uint32_t height)
{
    ASSERT(window != NULL);

    ASSERT(window->obj.type == DVZ_OBJECT_TYPE_WINDOW);
    ASSERT(window->obj.status == DVZ_OBJECT_STATUS_ALLOC);

    window->width = width;
    window->height = height;
    window->close_on_esc = true;

    // Create the window, depending on the backend.
    window->backend_window = backend_window(
        host ? host->instance : 0, host ? host->backend : DVZ_BACKEND_GLFW, width, height, window,
        &window->surface);

    if (window->surface == NULL)
    {
        log_warn("could not create window surface");
        // dvz_window_destroy(window);
        // return NULL;
    }

    backend_window_get_size(
        window,                          //
        &window->width, &window->height, //
        &window->framebuffer_width, &window->framebuffer_height);
}



DvzWindow* dvz_window(DvzHost* host, uint32_t width, uint32_t height)
{
    DvzWindow* window = NULL;
    if (host != NULL)
        window = dvz_container_alloc(&host->windows);
    else
        window = calloc(1, sizeof(DvzWindow));
    ASSERT(window != NULL);
    dvz_window_create(host, window, width, height);
    window->host = host;
    return window;
}



void dvz_window_poll_size(DvzWindow* window)
{
    ASSERT(window != NULL);
    // ASSERT(window->host != NULL);

    backend_window_get_size(
        window,                          //
        &window->width, &window->height, //
        &window->framebuffer_width, &window->framebuffer_height);
}



void dvz_window_set_size(DvzWindow* window, uint32_t width, uint32_t height)
{
    ASSERT(window != NULL);
    // ASSERT(window->host != NULL);
    backend_window_set_size(window, width, height);

    backend_window_get_size(
        window,                          //
        &window->width, &window->height, //
        &window->framebuffer_width, &window->framebuffer_height);
}



void dvz_window_poll_events(DvzWindow* window)
{
    ASSERT(window != NULL);
    // ASSERT(window->host != NULL);
    if (window->host)
        backend_poll_events(window->host);
}



void dvz_window_destroy(DvzWindow* window)
{
    if (window == NULL || window->obj.status == DVZ_OBJECT_STATUS_DESTROYED)
    {
        log_trace("skip destruction of already-destroyed window");
        return;
    }
    ASSERT(window != NULL);
    // ASSERT(window->host != NULL);
    backend_window_destroy(window);
    dvz_obj_destroyed(&window->obj);
}
