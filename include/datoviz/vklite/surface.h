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
#include "datoviz/vk/vulkan.h"

#include "datoviz/common/macros.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzInstance DvzInstance;
typedef struct DvzDevice DvzDevice;
typedef struct DvzSurface DvzSurface;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

EXTERN_C_ON



/**
 * Allocate an empty surface wrapper.
 *
 * @return allocated surface wrapper, or NULL on allocation failure
 */
DVZ_EXPORT DvzSurface* dvz_surface_create_wrapper(void);



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
 * Return whether the surface wrapper currently has a valid cached state.
 *
 * @param surface surface wrapper
 * @return true when the wrapper is ready for swapchain queries
 */
DVZ_EXPORT bool dvz_surface_ready(const DvzSurface* surface);



/**
 * Return the wrapped native Vulkan surface handle.
 *
 * @param surface surface wrapper
 * @return wrapped VkSurfaceKHR handle or VK_NULL_HANDLE
 */
DVZ_EXPORT VkSurfaceKHR dvz_surface_handle(const DvzSurface* surface);



/**
 * Return the cached surface capabilities snapshot.
 *
 * @param surface surface wrapper
 * @return cached capabilities value
 */
DVZ_EXPORT VkSurfaceCapabilitiesKHR dvz_surface_capabilities(const DvzSurface* surface);



/**
 * Return the number of cached supported surface formats.
 *
 * @param surface surface wrapper
 * @return number of cached formats
 */
DVZ_EXPORT uint32_t dvz_surface_format_count(const DvzSurface* surface);



/**
 * Fetch a cached supported format by index.
 *
 * @param surface surface wrapper
 * @param format_idx format index in the cached list
 * @param[out] format output cached format
 * @return true when the index is valid
 */
DVZ_EXPORT bool
dvz_surface_format(const DvzSurface* surface, uint32_t format_idx, VkSurfaceFormatKHR* format);



/**
 * Return the preferred surface format selected during refresh.
 *
 * @param surface surface wrapper
 * @return preferred cached format
 */
DVZ_EXPORT VkSurfaceFormatKHR dvz_surface_preferred_format(const DvzSurface* surface);



/**
 * Return the number of cached supported present modes.
 *
 * @param surface surface wrapper
 * @return number of cached present modes
 */
DVZ_EXPORT uint32_t dvz_surface_present_mode_count(const DvzSurface* surface);



/**
 * Fetch a cached supported present mode by index.
 *
 * @param surface surface wrapper
 * @param mode_idx present mode index in the cached list
 * @param[out] mode output cached present mode
 * @return true when the index is valid
 */
DVZ_EXPORT bool
dvz_surface_present_mode(const DvzSurface* surface, uint32_t mode_idx, VkPresentModeKHR* mode);



/**
 * Return whether a present mode is supported by the cached list.
 *
 * @param surface surface wrapper
 * @param mode present mode to query
 * @return true when the mode exists in the cached list
 */
DVZ_EXPORT bool dvz_surface_has_present_mode(const DvzSurface* surface, VkPresentModeKHR mode);



/**
 * Return the preferred present mode selected during refresh.
 *
 * @param surface surface wrapper
 * @return preferred cached present mode
 */
DVZ_EXPORT VkPresentModeKHR dvz_surface_preferred_present_mode(const DvzSurface* surface);



/**
 * Return the current cached extent resolved during refresh.
 *
 * @param surface surface wrapper
 * @return cached surface extent
 */
DVZ_EXPORT VkExtent2D dvz_surface_extent(const DvzSurface* surface);



/**
 * Destroy a surface wrapper cache.
 *
 * @param surface surface wrapper to destroy
 */
DVZ_EXPORT void dvz_surface_destroy(DvzSurface* surface);



/**
 * Free a surface wrapper allocated by dvz_surface_create().
 *
 * @param surface surface wrapper to free
 */
DVZ_EXPORT void dvz_surface_free(DvzSurface* surface);



EXTERN_C_OFF
