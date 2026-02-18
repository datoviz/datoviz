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



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzInstance DvzInstance;
typedef struct DvzDevice DvzDevice;
typedef struct DvzSurface DvzSurface;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzSurface
{
    VkPhysicalDevice physical_device;
    VkSurfaceKHR handle;
    uint32_t queue_family;
    VkExtent2D extent;
    VkExtent2D extent_hint;
    bool has_extent_hint;
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
 * Initialize a surface wrapper from instance + GPU index selection.
 *
 * @param surface surface wrapper to initialize
 * @param instance source instance used to resolve the GPU
 * @param gpu_index selected GPU index in the instance
 * @param queue_family queue family used for present support queries
 * @return true when initialization succeeds
 */
DVZ_EXPORT bool dvz_surface_init_from_instance(
    DvzSurface* surface, DvzInstance* instance, uint32_t gpu_index, uint32_t queue_family);



/**
 * Initialize a surface wrapper from a logical device.
 *
 * @param surface surface wrapper to initialize
 * @param device logical device used to resolve its physical GPU
 * @param queue_family queue family used for present support queries
 * @return true when initialization succeeds
 */
DVZ_EXPORT bool
dvz_surface_init_from_device(DvzSurface* surface, DvzDevice* device, uint32_t queue_family);



/**
 * Attach a native surface created by the window module to a surface wrapper.
 *
 * @param surface surface wrapper to configure
 * @param surface_khr native Vulkan surface handle owned by the window module
 * @param extent_hint optional extent used when the surface reports variable extent
 * @return true when the wrapper accepts the native surface
 */
DVZ_EXPORT bool dvz_surface_wrap_native(
    DvzSurface* surface, VkSurfaceKHR surface_khr, const VkExtent2D* extent_hint);



/**
 * Update the extent hint used when a wrapped surface reports variable extent.
 *
 * @param surface surface wrapper to update
 * @param extent_hint optional extent override, NULL clears the hint
 */
DVZ_EXPORT void dvz_surface_set_extent_hint(DvzSurface* surface, const VkExtent2D* extent_hint);



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
