/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Surface                                                                                      */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <volk.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "_surface.h"
#include "datoviz/vk/device.h"
#include "datoviz/vk/gpu.h"
#include "datoviz/vklite/surface.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static bool _surface_init_with_physical_device(
    DvzSurface* surface, VkPhysicalDevice physical_device, uint32_t queue_family)
{
    ANN(surface);
    if (physical_device == VK_NULL_HANDLE)
    {
        log_error("cannot initialize surface wrapper with null physical device");
        return false;
    }

    if (surface->formats != NULL || surface->present_modes != NULL)
    {
        dvz_surface_destroy(surface);
    }

    dvz_memset(surface, sizeof(*surface), 0, sizeof(*surface));
    surface->physical_device = physical_device;
    surface->queue_family = queue_family;
    surface->handle = VK_NULL_HANDLE;
    surface->extent = (VkExtent2D){0, 0};
    surface->extent_hint = (VkExtent2D){0, 0};
    surface->has_extent_hint = false;
    surface->preferred_format = (VkSurfaceFormatKHR){
        .format = VK_FORMAT_B8G8R8A8_UNORM,
        .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
    };
    surface->preferred_present_mode = VK_PRESENT_MODE_FIFO_KHR;
    surface->ready = false;
    return true;
}



static void _surface_cache_clear(DvzSurface* surface)
{
    ANN(surface);

    dvz_free(surface->formats);
    surface->formats = NULL;
    surface->format_count = 0;

    dvz_free(surface->present_modes);
    surface->present_modes = NULL;
    surface->present_mode_count = 0;
}



static uint32_t _surface_clamp_extent(uint32_t value, uint32_t min, uint32_t max)
{
    if (value < min)
    {
        return min;
    }
    if (value > max)
    {
        return max;
    }
    return value;
}



static void _surface_pick_defaults(DvzSurface* surface)
{
    ANN(surface);

    surface->preferred_format = (VkSurfaceFormatKHR){
        .format = VK_FORMAT_B8G8R8A8_UNORM,
        .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
    };
    if (surface->format_count > 0)
    {
        surface->preferred_format = surface->formats[0];
        for (uint32_t i = 0; i < surface->format_count; i++)
        {
            VkSurfaceFormatKHR candidate = surface->formats[i];
            if (
                candidate.format == VK_FORMAT_B8G8R8A8_UNORM &&
                candidate.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                surface->preferred_format = candidate;
                break;
            }
        }
    }

    surface->preferred_present_mode = VK_PRESENT_MODE_FIFO_KHR;
    if (surface->present_mode_count > 0)
    {
        surface->preferred_present_mode = surface->present_modes[0];
        for (uint32_t i = 0; i < surface->present_mode_count; i++)
        {
            VkPresentModeKHR candidate = surface->present_modes[i];
            if (candidate == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                surface->preferred_present_mode = candidate;
                return;
            }
            if (candidate == VK_PRESENT_MODE_FIFO_KHR)
            {
                surface->preferred_present_mode = candidate;
            }
        }
    }
}



static bool _surface_update_extent(DvzSurface* surface)
{
    ANN(surface);

    VkExtent2D extent = surface->capabilities.currentExtent;
    if (extent.width != UINT32_MAX && extent.height != UINT32_MAX)
    {
        surface->extent = extent;
        return true;
    }

    if (!surface->has_extent_hint)
    {
        surface->extent = (VkExtent2D){0, 0};
        return true;
    }

    extent = surface->extent_hint;
    extent.width = _surface_clamp_extent(
        extent.width, surface->capabilities.minImageExtent.width,
        surface->capabilities.maxImageExtent.width);
    extent.height = _surface_clamp_extent(
        extent.height, surface->capabilities.minImageExtent.height,
        surface->capabilities.maxImageExtent.height);
    surface->extent = extent;
    return true;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Allocate an empty surface wrapper.
 *
 * @return allocated surface wrapper, or NULL on allocation failure
 */
DvzSurface* dvz_surface_create_wrapper(void)
{
    DvzSurface* surface = (DvzSurface*)dvz_calloc(1, sizeof(DvzSurface));
    ANN(surface);
    return surface;
}



/**
 * Initialize a surface wrapper from instance + GPU index selection.
 *
 * @param surface surface wrapper to initialize
 * @param instance source instance used to resolve the GPU
 * @param gpu_index selected GPU index in the instance
 * @param queue_family queue family used for present support queries
 * @return true when initialization succeeds
 */
bool dvz_surface_init_from_instance(
    DvzSurface* surface, DvzInstance* instance, uint32_t gpu_index, uint32_t queue_family)
{
    ANN(surface);
    ANN(instance);

    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    if (!dvz_instance_gpu_handle(instance, gpu_index, &physical_device))
    {
        return false;
    }
    return _surface_init_with_physical_device(surface, physical_device, queue_family);
}



/**
 * Initialize a surface wrapper from a logical device.
 *
 * @param surface surface wrapper to initialize
 * @param device logical device used to resolve its physical GPU
 * @param queue_family queue family used for present support queries
 * @return true when initialization succeeds
 */
bool dvz_surface_init_from_device(DvzSurface* surface, DvzDevice* device, uint32_t queue_family)
{
    ANN(surface);
    ANN(device);
    return _surface_init_with_physical_device(
        surface, dvz_device_physical_device(device), queue_family);
}



/**
 * Attach a native surface created by the window module to a surface wrapper.
 *
 * @param surface surface wrapper to configure
 * @param surface_khr native Vulkan surface handle owned by the window module
 * @param extent_hint optional extent used when the surface reports variable extent
 * @return true when the wrapper accepts the native surface
 */
bool dvz_surface_wrap_native(
    DvzSurface* surface, VkSurfaceKHR surface_khr, const VkExtent2D* extent_hint)
{
    ANN(surface);

    if (surface_khr == VK_NULL_HANDLE)
    {
        log_error("cannot wrap a null Vulkan surface");
        return false;
    }

    surface->handle = surface_khr;
    surface->extent_hint = (VkExtent2D){0, 0};
    surface->has_extent_hint = false;
    if (extent_hint != NULL)
    {
        surface->extent_hint = *extent_hint;
        surface->has_extent_hint = true;
    }
    return dvz_surface_refresh(surface);
}



/**
 * Update the extent hint used when a wrapped surface reports variable extent.
 *
 * @param surface surface wrapper to update
 * @param extent_hint optional extent override, NULL clears the hint
 */
void dvz_surface_set_extent_hint(DvzSurface* surface, const VkExtent2D* extent_hint)
{
    ANN(surface);
    if (extent_hint != NULL)
    {
        surface->extent_hint = *extent_hint;
        surface->has_extent_hint = true;
    }
    else
    {
        surface->extent_hint = (VkExtent2D){0, 0};
        surface->has_extent_hint = false;
    }
}



/**
 * Refresh cached capabilities, formats, and present modes.
 *
 * @param surface surface wrapper to refresh
 * @return true when refresh succeeds
 */
bool dvz_surface_refresh(DvzSurface* surface)
{
    ANN(surface);
    if (surface->physical_device == VK_NULL_HANDLE)
    {
        log_error("cannot refresh surface wrapper with null physical device");
        return false;
    }
    surface->ready = false;

    if (surface->handle == VK_NULL_HANDLE)
    {
        log_error("cannot refresh surface wrapper without a native handle");
        return false;
    }

    _surface_cache_clear(surface);

    VkBool32 supports_present = VK_FALSE;
    VkResult res = vkGetPhysicalDeviceSurfaceSupportKHR(
        surface->physical_device, surface->queue_family, surface->handle, &supports_present);
    if (res != VK_SUCCESS)
    {
        log_error("surface present support query failed (%d)", res);
        return false;
    }
    if (!supports_present)
    {
        log_error("queue family %u has no present support for the wrapped surface", surface->queue_family);
        return false;
    }

    res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        surface->physical_device, surface->handle, &surface->capabilities);
    if (res != VK_SUCCESS)
    {
        log_error("surface capability query failed (%d)", res);
        return false;
    }

    res = vkGetPhysicalDeviceSurfaceFormatsKHR(
        surface->physical_device, surface->handle, &surface->format_count, NULL);
    if (res != VK_SUCCESS)
    {
        log_error("surface format count query failed (%d)", res);
        return false;
    }
    if (surface->format_count > 0)
    {
        surface->formats = (VkSurfaceFormatKHR*)dvz_calloc(surface->format_count, sizeof(VkSurfaceFormatKHR));
        ANN(surface->formats);
        res = vkGetPhysicalDeviceSurfaceFormatsKHR(
            surface->physical_device, surface->handle, &surface->format_count, surface->formats);
        if (res != VK_SUCCESS)
        {
            log_error("surface format query failed (%d)", res);
            _surface_cache_clear(surface);
            return false;
        }
    }

    res = vkGetPhysicalDeviceSurfacePresentModesKHR(
        surface->physical_device, surface->handle, &surface->present_mode_count, NULL);
    if (res != VK_SUCCESS)
    {
        log_error("surface present mode count query failed (%d)", res);
        _surface_cache_clear(surface);
        return false;
    }
    if (surface->present_mode_count > 0)
    {
        surface->present_modes =
            (VkPresentModeKHR*)dvz_calloc(surface->present_mode_count, sizeof(VkPresentModeKHR));
        ANN(surface->present_modes);
        res = vkGetPhysicalDeviceSurfacePresentModesKHR(
            surface->physical_device, surface->handle, &surface->present_mode_count,
            surface->present_modes);
        if (res != VK_SUCCESS)
        {
            log_error("surface present mode query failed (%d)", res);
            _surface_cache_clear(surface);
            return false;
        }
    }

    _surface_pick_defaults(surface);
    _surface_update_extent(surface);
    surface->ready = true;
    return true;
}



/**
 * Return whether the surface wrapper currently has a valid cached state.
 *
 * @param surface surface wrapper
 * @return true when the wrapper is ready for swapchain queries
 */
bool dvz_surface_ready(const DvzSurface* surface)
{
    ANN(surface);
    return surface->ready;
}



/**
 * Return the wrapped native Vulkan surface handle.
 *
 * @param surface surface wrapper
 * @return wrapped VkSurfaceKHR handle or VK_NULL_HANDLE
 */
VkSurfaceKHR dvz_surface_handle(const DvzSurface* surface)
{
    ANN(surface);
    return surface->handle;
}



/**
 * Return the cached surface capabilities snapshot.
 *
 * @param surface surface wrapper
 * @return cached capabilities value
 */
VkSurfaceCapabilitiesKHR dvz_surface_capabilities(const DvzSurface* surface)
{
    ANN(surface);
    return surface->capabilities;
}



/**
 * Return the number of cached supported surface formats.
 *
 * @param surface surface wrapper
 * @return number of cached formats
 */
uint32_t dvz_surface_format_count(const DvzSurface* surface)
{
    ANN(surface);
    return surface->format_count;
}



/**
 * Fetch a cached supported format by index.
 *
 * @param surface surface wrapper
 * @param format_idx format index in the cached list
 * @param[out] format output cached format
 * @return true when the index is valid
 */
bool dvz_surface_format(const DvzSurface* surface, uint32_t format_idx, VkSurfaceFormatKHR* format)
{
    ANN(surface);
    ANN(format);

    if (format_idx >= surface->format_count || surface->formats == NULL)
    {
        return false;
    }
    *format = surface->formats[format_idx];
    return true;
}



/**
 * Return the preferred surface format selected during refresh.
 *
 * @param surface surface wrapper
 * @return preferred cached format
 */
VkSurfaceFormatKHR dvz_surface_preferred_format(const DvzSurface* surface)
{
    ANN(surface);
    return surface->preferred_format;
}



/**
 * Return the number of cached supported present modes.
 *
 * @param surface surface wrapper
 * @return number of cached present modes
 */
uint32_t dvz_surface_present_mode_count(const DvzSurface* surface)
{
    ANN(surface);
    return surface->present_mode_count;
}



/**
 * Fetch a cached supported present mode by index.
 *
 * @param surface surface wrapper
 * @param mode_idx present mode index in the cached list
 * @param[out] mode output cached present mode
 * @return true when the index is valid
 */
bool dvz_surface_present_mode(const DvzSurface* surface, uint32_t mode_idx, VkPresentModeKHR* mode)
{
    ANN(surface);
    ANN(mode);

    if (mode_idx >= surface->present_mode_count || surface->present_modes == NULL)
    {
        return false;
    }
    *mode = surface->present_modes[mode_idx];
    return true;
}



/**
 * Return whether a present mode is supported by the cached list.
 *
 * @param surface surface wrapper
 * @param mode present mode to query
 * @return true when the mode exists in the cached list
 */
bool dvz_surface_has_present_mode(const DvzSurface* surface, VkPresentModeKHR mode)
{
    ANN(surface);

    VkPresentModeKHR candidate = VK_PRESENT_MODE_FIFO_KHR;
    for (uint32_t i = 0; i < dvz_surface_present_mode_count(surface); i++)
    {
        if (!dvz_surface_present_mode(surface, i, &candidate))
        {
            return false;
        }
        if (candidate == mode)
        {
            return true;
        }
    }
    return false;
}



/**
 * Return the preferred present mode selected during refresh.
 *
 * @param surface surface wrapper
 * @return preferred cached present mode
 */
VkPresentModeKHR dvz_surface_preferred_present_mode(const DvzSurface* surface)
{
    ANN(surface);
    return surface->preferred_present_mode;
}



/**
 * Return the current cached extent resolved during refresh.
 *
 * @param surface surface wrapper
 * @return cached surface extent
 */
VkExtent2D dvz_surface_extent(const DvzSurface* surface)
{
    ANN(surface);
    return surface->extent;
}



/**
 * Destroy a surface wrapper cache.
 *
 * @param surface surface wrapper to destroy
 */
void dvz_surface_destroy(DvzSurface* surface)
{
    if (surface == NULL)
    {
        return;
    }

    _surface_cache_clear(surface);
    surface->capabilities = (VkSurfaceCapabilitiesKHR){0};
    surface->preferred_format = (VkSurfaceFormatKHR){0};
    surface->preferred_present_mode = VK_PRESENT_MODE_FIFO_KHR;
    surface->extent = (VkExtent2D){0, 0};
    surface->extent_hint = (VkExtent2D){0, 0};
    surface->has_extent_hint = false;
    surface->handle = VK_NULL_HANDLE;
    surface->queue_family = 0;
    surface->physical_device = VK_NULL_HANDLE;
    surface->ready = false;
}



/**
 * Free a surface wrapper allocated by dvz_surface_create().
 *
 * @param surface surface wrapper to free
 */
void dvz_surface_free(DvzSurface* surface)
{
    if (surface == NULL)
    {
        return;
    }
    dvz_free(surface);
}
