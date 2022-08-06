/*************************************************************************************************/
/*  Surface                                                                                      */
/*************************************************************************************************/

#include "surface.h"
#include "common.h"
#include "glfw_utils.h"
#include "vklite_utils.h"
#include "window.h"



/*************************************************************************************************/
/*  Surface                                                                                      */
/*************************************************************************************************/

VkSurfaceKHR dvz_window_surface(DvzHost* host, DvzWindow* window)
{
    ASSERT(host != NULL);
    ASSERT(window != NULL);
    ASSERT(window->backend_window != NULL);

    VkSurfaceKHR surface = {0};
    VkResult res = glfwCreateWindowSurface(host->instance, window->backend_window, NULL, &surface);
    if (res != VK_SUCCESS)
        log_error("error creating the GLFW surface, result was %d", res);

    return surface;
}



void dvz_gpu_create_with_surface(DvzGpu* gpu)
{
    ASSERT(gpu != NULL);
    DvzHost* host = gpu->host;
    ASSERT(host != NULL);

    // HACK: temporarily create a blank window so that we can create a GPU with surface rendering
    // capabilities.
    DvzWindow window = dvz_window(host->backend, 10, 10, DVZ_WINDOW_FLAGS_HIDDEN);
    VkSurfaceKHR surface = dvz_window_surface(host, &window);
    ASSERT(surface != VK_NULL_HANDLE);
    dvz_gpu_create(gpu, surface);

    dvz_window_destroy(&window);
    dvz_surface_destroy(host, surface);
}



void dvz_surface_destroy(DvzHost* host, VkSurfaceKHR surface)
{
    ASSERT(host != NULL);
    if (surface != VK_NULL_HANDLE)
    {
        log_trace("destroy surface");
        vkDestroySurfaceKHR(host->instance, surface, NULL);
    }
}
