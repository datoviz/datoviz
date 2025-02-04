/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Surface                                                                                      */
/*************************************************************************************************/

#ifndef DVZ_HEADER_SURFACE
#define DVZ_HEADER_SURFACE



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "common.h"
#include "host.h"
#include "window.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzSurface DvzSurface;

// Forward declarations.
typedef struct DvzGpu DvzGpu;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzSurface
{
    DvzGpu* gpu;
    VkSurfaceKHR surface;
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

EXTERN_C_ON

/**
 * Create a surface out of a window.
 *
 * @param host the host
 * @param window the window
 * @returns a surface
 */
DvzSurface dvz_window_surface(DvzHost* host, DvzWindow* window);



/**
 * Create a GPU with a temporary invisible mock window in order to get Vulkan capabilities.
 *
 * @param gpu the GPU
 */
void dvz_gpu_create_with_surface(DvzGpu* gpu);



/**
 * Destroy a surface.
 *
 * @param surface the surface to destroy
 */
void dvz_surface_destroy(DvzHost* host, DvzSurface surface);



EXTERN_C_OFF

#endif
