/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Surface                                                                                      */
/*************************************************************************************************/

#include "surface.h"
#include "backend.h"
#include "common.h"
#include "vklite.h"
#include "window.h"



/*************************************************************************************************/
/*  Surface                                                                                      */
/*************************************************************************************************/

DvzSurface dvz_window_surface(DvzHost* host, DvzWindow* window)
{
    ANN(host);
    ANN(window);
    ANN(window->backend_window);

    DvzSurface surface = {0};
    VkResult res =
        glfwCreateWindowSurface(host->instance, window->backend_window, NULL, &surface.surface);
    if (res != VK_SUCCESS)
        log_error("error creating the GLFW surface, result was %d", res);

    return surface;
}



void dvz_gpu_create_with_surface(DvzGpu* gpu)
{
    ANN(gpu);
    DvzHost* host = gpu->host;
    ANN(host);

    // HACK: temporarily create a blank window so that we can create a GPU with surface rendering
    // capabilities.
    DvzWindow window = dvz_window(host->backend, 10, 10, DVZ_WINDOW_FLAGS_HIDDEN);
    DvzSurface surface = dvz_window_surface(host, &window);
    surface.gpu = gpu;
    ASSERT(surface.surface != VK_NULL_HANDLE);
    dvz_gpu_create(gpu, surface.surface);

    dvz_surface_destroy(host, surface);
    dvz_window_destroy(&window);
}



void dvz_surface_destroy(DvzHost* host, DvzSurface surface)
{
    ANN(host);
    if (surface.surface != VK_NULL_HANDLE)
    {
        log_trace("destroy surface");
        vkDestroySurfaceKHR(host->instance, surface.surface, NULL);
    }
}
