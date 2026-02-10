/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Surface                                                                                      */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <vulkan/vulkan_core.h>

#include "datoviz/common/macros.h"
#include "datoviz/window/types.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzGpu DvzGpu;
typedef struct DvzSurface DvzSurface;
typedef struct DvzWindow DvzWindow;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzSurface
{
    DvzGpu* gpu;
    DvzWindow* window;
    VkSurfaceKHR handle;
    uint32_t queue_family;
    VkExtent2D extent;
    VkSurfaceCapabilitiesKHR capabilities;
    uint32_t format_count;
    VkSurfaceFormatKHR* formats;
    uint32_t present_mode_count;
    VkPresentModeKHR* present_modes;
    VkSurfaceFormatKHR preferred_format;
    VkPresentModeKHR preferred_present_mode;
    bool ready;
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

EXTERN_C_ON



/**
 * Initialize a surface wrapper for a GPU queue family.
 *
 * @param surface surface wrapper to initialize
 * @param gpu physical GPU queried for capabilities
 * @param queue_family queue family used for present support queries
 * @return true when initialization succeeds
 */
DVZ_EXPORT bool dvz_surface_init(DvzSurface* surface, DvzGpu* gpu, uint32_t queue_family);



/**
 * Attach a native surface created by the window module to a surface wrapper.
 *
 * @param surface surface wrapper to configure
 * @param surface_khr native Vulkan surface handle owned by the window module
 * @param window window owning the native surface
 * @return true when the wrapper accepts the native surface
 */
DVZ_EXPORT bool
dvz_surface_wrap_native(DvzSurface* surface, VkSurfaceKHR surface_khr, DvzWindow* window);



/**
 * Refresh cached capabilities, formats, and present modes.
 *
 * @param surface surface wrapper to refresh
 * @return true when refresh succeeds
 */
DVZ_EXPORT bool dvz_surface_refresh(DvzSurface* surface);



/**
 * Destroy a surface wrapper cache.
 *
 * @param surface surface wrapper to destroy
 */
DVZ_EXPORT void dvz_surface_destroy(DvzSurface* surface);



EXTERN_C_OFF
