/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Swapchain                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <volk.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "_swapchain.h"
#include "datoviz/vk/device.h"
#include "datoviz/vklite/swapchain.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_SWAPCHAIN_DEFAULT_IMAGE_USAGE                                                        \
    (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT)



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static bool _swapchain_init_with_physical_device(
    DvzSwapchain* swapchain, VkPhysicalDevice physical_device, VkDevice device, DvzSurface* surface)
{
    ANN(swapchain);
    if (physical_device == VK_NULL_HANDLE)
    {
        log_error("cannot initialize swapchain wrapper with null physical device");
        return false;
    }
    ANN(surface);

    if (swapchain->handle != VK_NULL_HANDLE || swapchain->images != NULL || swapchain->image_views != NULL)
    {
        dvz_swapchain_destroy(swapchain);
    }

    dvz_memset(swapchain, sizeof(*swapchain), 0, sizeof(*swapchain));
    swapchain->physical_device = physical_device;
    swapchain->surface = surface;
    swapchain->device = device;
    swapchain->handle = VK_NULL_HANDLE;
    swapchain->config = (DvzSwapchainConfig){
        .image_format = VK_FORMAT_UNDEFINED,
        .color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        .present_mode = VK_PRESENT_MODE_FIFO_KHR,
        .image_usage = DVZ_SWAPCHAIN_DEFAULT_IMAGE_USAGE,
        .composite_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .min_image_count = 0,
        .clipped = true,
    };
    swapchain->image_format = VK_FORMAT_UNDEFINED;
    swapchain->color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swapchain->present_mode = VK_PRESENT_MODE_FIFO_KHR;
    swapchain->ready = false;
    return true;
}



static DvzPresentStatus _swapchain_status_from_result(VkResult result)
{
    switch (result)
    {
    case VK_SUCCESS:
        return DVZ_PRESENT_STATUS_OK;
    case VK_SUBOPTIMAL_KHR:
    case VK_ERROR_OUT_OF_DATE_KHR:
        return DVZ_PRESENT_STATUS_RECREATE;
    case VK_ERROR_DEVICE_LOST:
        return DVZ_PRESENT_STATUS_DEVICE_LOST;
    default:
        return DVZ_PRESENT_STATUS_ERROR;
    }
}



static const char* _swapchain_status_name(DvzPresentStatus status)
{
    switch (status)
    {
    case DVZ_PRESENT_STATUS_OK:
        return "OK";
    case DVZ_PRESENT_STATUS_RECREATE:
        return "RECREATE";
    case DVZ_PRESENT_STATUS_SKIP_ZERO_EXTENT:
        return "SKIP_ZERO_EXTENT";
    case DVZ_PRESENT_STATUS_DEVICE_LOST:
        return "DEVICE_LOST";
    case DVZ_PRESENT_STATUS_ERROR:
    default:
        return "ERROR";
    }
}



static uint32_t _swapchain_clamp_extent(uint32_t value, uint32_t min, uint32_t max)
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



static VkSurfaceFormatKHR _swapchain_resolve_format(const DvzSwapchain* swapchain)
{
    ANN(swapchain);
    ANN(swapchain->surface);

    VkSurfaceFormatKHR resolved = dvz_surface_preferred_format(swapchain->surface);
    VkFormat requested_format = swapchain->config.image_format;
    VkColorSpaceKHR requested_space = swapchain->config.color_space;
    uint32_t format_count = dvz_surface_format_count(swapchain->surface);

    if (requested_format == VK_FORMAT_UNDEFINED || format_count == 0)
    {
        return resolved;
    }

    for (uint32_t i = 0; i < format_count; i++)
    {
        VkSurfaceFormatKHR candidate = {0};
        if (!dvz_surface_format(swapchain->surface, i, &candidate))
        {
            continue;
        }
        if (candidate.format != requested_format)
        {
            continue;
        }
        if (requested_space == 0 || candidate.colorSpace == requested_space)
        {
            return candidate;
        }
    }

    return resolved;
}



static VkPresentModeKHR _swapchain_resolve_present_mode(const DvzSwapchain* swapchain)
{
    ANN(swapchain);
    ANN(swapchain->surface);

    VkPresentModeKHR requested = swapchain->config.present_mode;
    if (dvz_surface_has_present_mode(swapchain->surface, requested))
    {
        return requested;
    }

    log_warn("requested present mode %d unsupported; using preferred mode", requested);
    return dvz_surface_preferred_present_mode(swapchain->surface);
}



static VkExtent2D _swapchain_resolve_extent(const DvzSwapchain* swapchain, uvec2 size)
{
    ANN(swapchain);
    ANN(swapchain->surface);

    VkSurfaceCapabilitiesKHR caps = dvz_surface_capabilities(swapchain->surface);
    if (caps.currentExtent.width != UINT32_MAX && caps.currentExtent.height != UINT32_MAX)
    {
        return caps.currentExtent;
    }

    VkExtent2D extent = {
        .width = size[0],
        .height = size[1],
    };
    extent.width =
        _swapchain_clamp_extent(extent.width, caps.minImageExtent.width, caps.maxImageExtent.width);
    extent.height = _swapchain_clamp_extent(
        extent.height, caps.minImageExtent.height, caps.maxImageExtent.height);
    return extent;
}



static uint32_t _swapchain_resolve_image_count(const DvzSwapchain* swapchain)
{
    ANN(swapchain);
    ANN(swapchain->surface);

    VkSurfaceCapabilitiesKHR caps = dvz_surface_capabilities(swapchain->surface);
    uint32_t count = swapchain->config.min_image_count;
    if (count == 0)
    {
        count = caps.minImageCount + 1;
    }
    if (count < caps.minImageCount)
    {
        count = caps.minImageCount;
    }
    if (caps.maxImageCount > 0 && count > caps.maxImageCount)
    {
        count = caps.maxImageCount;
    }
    return count;
}



static bool _swapchain_config_is_zeroed(DvzSwapchainConfig config)
{
    return (
        config.image_format == VK_FORMAT_UNDEFINED && config.color_space == 0 &&
        config.present_mode == 0 && config.image_usage == 0 && config.composite_alpha == 0 &&
        config.min_image_count == 0 && !config.clipped);
}



/**
 * Destroy swapchain image arrays and optional image views.
 *
 * @param device logical device used to destroy image views
 * @param image_count number of image/view entries
 * @param images swapchain image array pointer
 * @param image_views swapchain image view array pointer
 */
static void _swapchain_destroy_image_arrays(
    VkDevice device, uint32_t image_count, VkImage** images, VkImageView** image_views)
{
    ANN(images);
    ANN(image_views);

    if (*image_views != NULL && device != VK_NULL_HANDLE)
    {
        for (uint32_t i = 0; i < image_count; i++)
        {
            if ((*image_views)[i] != VK_NULL_HANDLE)
            {
                vkDestroyImageView(device, (*image_views)[i], NULL);
            }
        }
    }

    dvz_free(*image_views);
    *image_views = NULL;
    dvz_free(*images);
    *images = NULL;
}



static void _swapchain_destroy_views(DvzSwapchain* swapchain)
{
    ANN(swapchain);

    _swapchain_destroy_image_arrays(
        swapchain->device, swapchain->image_count, &swapchain->images, &swapchain->image_views);
    swapchain->image_count = 0;
}



/**
 * Build image views for a swapchain image array.
 *
 * @param device logical device used to create image views
 * @param image_count number of images in the array
 * @param images swapchain image array
 * @param format image format used to create views
 * @param[out] image_views allocated view array
 * @return true when all image views are created
 */
static bool _swapchain_build_views(
    VkDevice device, uint32_t image_count, VkImage* images, VkFormat format, VkImageView** image_views)
{
    ANN(images);
    ANN(image_views);

    if (device == VK_NULL_HANDLE || image_count == 0)
    {
        return false;
    }

    *image_views = (VkImageView*)dvz_calloc(image_count, sizeof(VkImageView));
    ANN(*image_views);

    for (uint32_t i = 0; i < image_count; i++)
    {
        VkImageViewCreateInfo view_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = format,
            .subresourceRange =
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
        };

        VkResult res = vkCreateImageView(device, &view_info, NULL, &(*image_views)[i]);
        if (res != VK_SUCCESS)
        {
            log_error("swapchain image view creation failed (%d)", res);
            return false;
        }
    }

    return true;
}



static DvzPresentStatus _swapchain_refresh_surface(DvzSwapchain* swapchain)
{
    ANN(swapchain);
    ANN(swapchain->surface);

    if (!dvz_surface_refresh(swapchain->surface))
    {
        return DVZ_PRESENT_STATUS_ERROR;
    }

    if (!dvz_surface_ready(swapchain->surface))
    {
        return DVZ_PRESENT_STATUS_ERROR;
    }

    return DVZ_PRESENT_STATUS_OK;
}



/**
 * Wait until the swapchain device is idle before destroying present resources.
 *
 * @param swapchain swapchain wrapper being torn down
 */
static void _swapchain_wait_idle(DvzSwapchain* swapchain)
{
    ANN(swapchain);

    if (swapchain->device == VK_NULL_HANDLE)
    {
        return;
    }
    if (
        swapchain->handle == VK_NULL_HANDLE && swapchain->image_views == NULL && swapchain->images == NULL)
    {
        return;
    }

    // Teardown can follow an acquire/present cycle immediately in tests and shutdown paths.
    vkDeviceWaitIdle(swapchain->device);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Allocate an empty swapchain wrapper.
 *
 * @return allocated swapchain wrapper, or NULL on allocation failure
 */
DvzSwapchain* dvz_swapchain_create_wrapper(void)
{
    DvzSwapchain* swapchain = (DvzSwapchain*)dvz_calloc(1, sizeof(DvzSwapchain));
    ANN(swapchain);
    return swapchain;
}



/**
 * Initialize a swapchain wrapper from a logical device and surface.
 *
 * @param swapchain swapchain wrapper to initialize
 * @param device logical device used to resolve the physical GPU
 * @param surface surface wrapper used for capability and extent data
 * @return true when initialization succeeds
 */
bool dvz_swapchain_init_from_device(
    DvzSwapchain* swapchain, DvzDevice* device, DvzSurface* surface)
{
    ANN(swapchain);
    ANN(device);
    ANN(surface);
    return _swapchain_init_with_physical_device(
        swapchain, dvz_device_physical_device(device), dvz_device_handle(device), surface);
}



/**
 * Bind the Vulkan logical device used by swapchain create/destroy/acquire paths.
 *
 * @param swapchain swapchain wrapper to configure
 * @param device logical device used to issue swapchain API calls
 * @return true when binding succeeds
 */
bool dvz_swapchain_device(DvzSwapchain* swapchain, VkDevice device)
{
    ANN(swapchain);
    if (device == VK_NULL_HANDLE)
    {
        log_error("cannot bind null VkDevice to swapchain");
        return false;
    }
    if (
        swapchain->ready && swapchain->handle != VK_NULL_HANDLE && swapchain->device != VK_NULL_HANDLE &&
        swapchain->device != device)
    {
        log_error("cannot rebind a live swapchain to a different VkDevice");
        return false;
    }
    swapchain->device = device;
    return true;
}



/**
 * Set swapchain creation parameters.
 *
 * @param swapchain swapchain wrapper to configure
 * @param config desired swapchain configuration
 * @return true when configuration is accepted
 */
bool dvz_swapchain_config(DvzSwapchain* swapchain, DvzSwapchainConfig config)
{
    ANN(swapchain);

    bool is_zeroed = _swapchain_config_is_zeroed(config);
    swapchain->config = config;
    if (is_zeroed)
    {
        swapchain->config.clipped = true;
    }
    if (swapchain->config.color_space == 0)
    {
        swapchain->config.color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    }
    if (is_zeroed)
    {
        swapchain->config.present_mode = VK_PRESENT_MODE_FIFO_KHR;
    }
    if (swapchain->config.image_usage == 0)
    {
        swapchain->config.image_usage = DVZ_SWAPCHAIN_DEFAULT_IMAGE_USAGE;
    }
    if (swapchain->config.composite_alpha == 0)
    {
        swapchain->config.composite_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    }
    return true;
}



/**
 * Return whether the swapchain wrapper currently owns valid Vulkan resources.
 *
 * @param swapchain swapchain wrapper
 * @return true when swapchain resources are ready for acquire/present
 */
bool dvz_swapchain_ready(const DvzSwapchain* swapchain)
{
    ANN(swapchain);
    return swapchain->ready;
}



/**
 * Return the wrapped Vulkan swapchain handle.
 *
 * @param swapchain swapchain wrapper
 * @return wrapped VkSwapchainKHR handle or VK_NULL_HANDLE
 */
VkSwapchainKHR dvz_swapchain_handle(const DvzSwapchain* swapchain)
{
    ANN(swapchain);
    return swapchain->handle;
}



/**
 * Return the number of swapchain images in the current recreation state.
 *
 * @param swapchain swapchain wrapper
 * @return swapchain image count
 */
uint32_t dvz_swapchain_image_count(const DvzSwapchain* swapchain)
{
    ANN(swapchain);
    return swapchain->image_count;
}



/**
 * Return the resolved image format from the latest recreate.
 *
 * @param swapchain swapchain wrapper
 * @return resolved image format
 */
VkFormat dvz_swapchain_image_format(const DvzSwapchain* swapchain)
{
    ANN(swapchain);
    return swapchain->image_format;
}



/**
 * Return the resolved color space from the latest recreate.
 *
 * @param swapchain swapchain wrapper
 * @return resolved color space
 */
VkColorSpaceKHR dvz_swapchain_color_space(const DvzSwapchain* swapchain)
{
    ANN(swapchain);
    return swapchain->color_space;
}



/**
 * Return the resolved present mode from the latest recreate.
 *
 * @param swapchain swapchain wrapper
 * @return resolved present mode
 */
VkPresentModeKHR dvz_swapchain_present_mode(const DvzSwapchain* swapchain)
{
    ANN(swapchain);
    return swapchain->present_mode;
}



/**
 * Return the currently configured swapchain creation parameters.
 *
 * @param swapchain swapchain wrapper
 * @return currently stored config
 */
DvzSwapchainConfig dvz_swapchain_get_config(const DvzSwapchain* swapchain)
{
    ANN(swapchain);
    return swapchain->config;
}



/**
 * Return the current swapchain extent from the latest recreate.
 *
 * @param swapchain swapchain wrapper
 * @return current resolved extent
 */
VkExtent2D dvz_swapchain_extent(const DvzSwapchain* swapchain)
{
    ANN(swapchain);
    return swapchain->extent;
}



/**
 * Fetch a swapchain image handle by index.
 *
 * @param swapchain swapchain wrapper
 * @param image_idx image index
 * @param[out] image output image handle
 * @return true when the index is valid
 */
bool dvz_swapchain_image(const DvzSwapchain* swapchain, uint32_t image_idx, VkImage* image)
{
    ANN(swapchain);
    ANN(image);

    if (swapchain->images == NULL || image_idx >= swapchain->image_count)
    {
        return false;
    }
    *image = swapchain->images[image_idx];
    return true;
}



/**
 * Fetch a swapchain image view handle by index.
 *
 * @param swapchain swapchain wrapper
 * @param image_idx image index
 * @param[out] image_view output image view handle
 * @return true when the index is valid
 */
bool dvz_swapchain_image_view(
    const DvzSwapchain* swapchain, uint32_t image_idx, VkImageView* image_view)
{
    ANN(swapchain);
    ANN(image_view);

    if (swapchain->image_views == NULL || image_idx >= swapchain->image_count)
    {
        return false;
    }
    *image_view = swapchain->image_views[image_idx];
    return true;
}



/**
 * Recreate swapchain images and image views for a new extent.
 *
 * @param swapchain swapchain wrapper to recreate
 * @param size target extent as {width, height}
 * @return present status mapping recreate outcome
 */
DvzPresentStatus dvz_swapchain_recreate(DvzSwapchain* swapchain, uvec2 size)
{
    ANN(swapchain);
    if (swapchain->physical_device == VK_NULL_HANDLE)
    {
        log_error("swapchain recreate requires a valid physical device handle");
        return DVZ_PRESENT_STATUS_ERROR;
    }
    ANN(swapchain->surface);

    if (dvz_surface_handle(swapchain->surface) == VK_NULL_HANDLE)
    {
        log_error("swapchain recreate requires a valid wrapped surface");
        return DVZ_PRESENT_STATUS_ERROR;
    }
    if (swapchain->device == VK_NULL_HANDLE)
    {
        log_error("swapchain recreate requires a valid device handle");
        return DVZ_PRESENT_STATUS_ERROR;
    }

    DvzPresentStatus refresh_status = _swapchain_refresh_surface(swapchain);
    if (refresh_status != DVZ_PRESENT_STATUS_OK)
    {
        return refresh_status;
    }

    VkExtent2D extent = _swapchain_resolve_extent(swapchain, size);
    if (extent.width == 0 || extent.height == 0)
    {
        return DVZ_PRESENT_STATUS_SKIP_ZERO_EXTENT;
    }

    VkSurfaceFormatKHR format = _swapchain_resolve_format(swapchain);
    VkPresentModeKHR present_mode = _swapchain_resolve_present_mode(swapchain);
    uint32_t min_image_count = _swapchain_resolve_image_count(swapchain);
    VkSurfaceCapabilitiesKHR caps = dvz_surface_capabilities(swapchain->surface);

    VkSwapchainCreateInfoKHR create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = dvz_surface_handle(swapchain->surface),
        .minImageCount = min_image_count,
        .imageFormat = format.format,
        .imageColorSpace = format.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = swapchain->config.image_usage,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = caps.currentTransform,
        .compositeAlpha = swapchain->config.composite_alpha,
        .presentMode = present_mode,
        .clipped = swapchain->config.clipped,
        .oldSwapchain = swapchain->handle,
    };

    VkSwapchainKHR old_swapchain = swapchain->handle;
    VkSwapchainKHR new_swapchain = VK_NULL_HANDLE;
    uint32_t new_image_count = 0;
    VkImage* new_images = NULL;
    VkImageView* new_image_views = NULL;

    VkResult res = vkCreateSwapchainKHR(swapchain->device, &create_info, NULL, &new_swapchain);
    DvzPresentStatus status = _swapchain_status_from_result(res);
    if (status != DVZ_PRESENT_STATUS_OK)
    {
        log_error(
            "swapchain create failed (swapchain=%p extent=%ux%u vk=%d status=%s)",
            (void*)swapchain, extent.width, extent.height, res, _swapchain_status_name(status));
        return status;
    }

    res = vkGetSwapchainImagesKHR(swapchain->device, new_swapchain, &new_image_count, NULL);
    status = _swapchain_status_from_result(res);
    if (status != DVZ_PRESENT_STATUS_OK)
    {
        log_error(
            "swapchain image count query failed (swapchain=%p vk=%d status=%s)", (void*)swapchain,
            res, _swapchain_status_name(status));
        vkDestroySwapchainKHR(swapchain->device, new_swapchain, NULL);
        return status;
    }
    if (new_image_count == 0)
    {
        log_error("swapchain image count query returned zero images");
        vkDestroySwapchainKHR(swapchain->device, new_swapchain, NULL);
        return DVZ_PRESENT_STATUS_ERROR;
    }

    new_images = (VkImage*)dvz_calloc(new_image_count, sizeof(VkImage));
    ANN(new_images);
    res = vkGetSwapchainImagesKHR(
        swapchain->device, new_swapchain, &new_image_count, new_images);
    status = _swapchain_status_from_result(res);
    if (status != DVZ_PRESENT_STATUS_OK)
    {
        log_error(
            "swapchain image query failed (swapchain=%p vk=%d status=%s)", (void*)swapchain, res,
            _swapchain_status_name(status));
        _swapchain_destroy_image_arrays(swapchain->device, new_image_count, &new_images, &new_image_views);
        vkDestroySwapchainKHR(swapchain->device, new_swapchain, NULL);
        return status;
    }

    if (!_swapchain_build_views(
            swapchain->device, new_image_count, new_images, format.format, &new_image_views))
    {
        _swapchain_destroy_image_arrays(swapchain->device, new_image_count, &new_images, &new_image_views);
        vkDestroySwapchainKHR(swapchain->device, new_swapchain, NULL);
        return DVZ_PRESENT_STATUS_ERROR;
    }

    _swapchain_destroy_views(swapchain);
    if (old_swapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(swapchain->device, old_swapchain, NULL);
    }

    swapchain->handle = new_swapchain;
    swapchain->image_count = new_image_count;
    swapchain->images = new_images;
    swapchain->image_views = new_image_views;
    swapchain->extent = extent;
    swapchain->image_format = format.format;
    swapchain->color_space = format.colorSpace;
    swapchain->present_mode = present_mode;
    swapchain->current_image = UINT32_MAX;
    swapchain->ready = true;
    return DVZ_PRESENT_STATUS_OK;
}



/**
 * Acquire the next image index from the swapchain.
 *
 * @param swapchain swapchain wrapper
 * @param image_available semaphore signaled by Vulkan when image is available
 * @param timeout_ns timeout value passed to Vulkan acquire call
 * @param[out] image_idx output image index
 * @return present status mapping acquire outcome
 */
DvzPresentStatus dvz_swapchain_acquire(
    DvzSwapchain* swapchain, VkSemaphore image_available, uint64_t timeout_ns, uint32_t* image_idx)
{
    ANN(swapchain);
    ANN(image_idx);
    *image_idx = UINT32_MAX;

    if (
        !swapchain->ready || swapchain->handle == VK_NULL_HANDLE ||
        swapchain->device == VK_NULL_HANDLE)
    {
        log_error(
            "swapchain acquire invalid state (ready=%d has_handle=%d has_device=%d)",
            swapchain->ready ? 1 : 0, swapchain->handle != VK_NULL_HANDLE ? 1 : 0,
            swapchain->device != VK_NULL_HANDLE ? 1 : 0);
        swapchain->current_image = UINT32_MAX;
        return DVZ_PRESENT_STATUS_ERROR;
    }
    if (swapchain->extent.width == 0 || swapchain->extent.height == 0)
    {
        log_warn(
            "swapchain acquire skipped because extent is zero (%ux%u)", swapchain->extent.width,
            swapchain->extent.height);
        swapchain->current_image = UINT32_MAX;
        return DVZ_PRESENT_STATUS_SKIP_ZERO_EXTENT;
    }

    VkResult res = vkAcquireNextImageKHR(
        swapchain->device, swapchain->handle, timeout_ns, image_available, VK_NULL_HANDLE,
        image_idx);
    DvzPresentStatus status = _swapchain_status_from_result(res);
    if (status == DVZ_PRESENT_STATUS_OK)
    {
        swapchain->current_image = *image_idx;
        return status;
    }
    swapchain->current_image = UINT32_MAX;
    if (status == DVZ_PRESENT_STATUS_RECREATE)
    {
        log_trace(
            "swapchain acquire requires recreate (swapchain=%p image=%u vk=%d status=%s)",
            (void*)swapchain, *image_idx, res, _swapchain_status_name(status));
        return status;
    }
    log_error(
        "swapchain acquire failed (swapchain=%p image=%u vk=%d status=%s)", (void*)swapchain,
        *image_idx, res, _swapchain_status_name(status));
    return status;
}



/**
 * Present a previously rendered image.
 *
 * @param swapchain swapchain wrapper
 * @param present_queue queue used for present submission
 * @param image_idx image index to present
 * @param render_finished semaphore waited before presentation
 * @return present status mapping present outcome
 */
DvzPresentStatus dvz_swapchain_present(
    DvzSwapchain* swapchain, VkQueue present_queue, uint32_t image_idx, VkSemaphore render_finished)
{
    ANN(swapchain);

    if (!swapchain->ready || swapchain->handle == VK_NULL_HANDLE || present_queue == VK_NULL_HANDLE)
    {
        log_error(
            "swapchain present invalid state (ready=%d has_handle=%d has_queue=%d image=%u)",
            swapchain->ready ? 1 : 0, swapchain->handle != VK_NULL_HANDLE ? 1 : 0,
            present_queue != VK_NULL_HANDLE ? 1 : 0, image_idx);
        return DVZ_PRESENT_STATUS_ERROR;
    }
    if (image_idx >= swapchain->image_count)
    {
        log_error(
            "swapchain present image index %u out of range (image_count=%u)", image_idx,
            swapchain->image_count);
        return DVZ_PRESENT_STATUS_ERROR;
    }

    VkSemaphore wait_semaphores[] = {render_finished};
    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = (render_finished == VK_NULL_HANDLE) ? 0 : 1,
        .pWaitSemaphores = (render_finished == VK_NULL_HANDLE) ? NULL : wait_semaphores,
        .swapchainCount = 1,
        .pSwapchains = &swapchain->handle,
        .pImageIndices = &image_idx,
    };

    VkResult res = vkQueuePresentKHR(present_queue, &present_info);
    DvzPresentStatus status = _swapchain_status_from_result(res);
    if (status == DVZ_PRESENT_STATUS_OK)
    {
        return status;
    }
    if (status == DVZ_PRESENT_STATUS_RECREATE)
    {
        log_trace(
            "swapchain present requires recreate (swapchain=%p image=%u vk=%d status=%s)",
            (void*)swapchain, image_idx, res, _swapchain_status_name(status));
        return status;
    }
    log_error(
        "swapchain present failed (swapchain=%p image=%u vk=%d status=%s)", (void*)swapchain,
        image_idx, res, _swapchain_status_name(status));
    return status;
}



/**
 * Destroy swapchain resources owned by vklite.
 *
 * @param swapchain swapchain wrapper to destroy
 */
void dvz_swapchain_destroy(DvzSwapchain* swapchain)
{
    if (swapchain == NULL)
    {
        return;
    }

    // Swapchain does not use DvzObject lifecycle tracking (lifecycle is managed by the canvas
    // owner). Idempotency is guaranteed by the handle != VK_NULL_HANDLE guard on the Vulkan
    // call and by nulling the handle immediately after.
    _swapchain_wait_idle(swapchain);
    _swapchain_destroy_views(swapchain);

    if (swapchain->handle != VK_NULL_HANDLE && swapchain->device != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(swapchain->device, swapchain->handle, NULL);
    }

    swapchain->handle = VK_NULL_HANDLE;
    swapchain->extent = (VkExtent2D){0, 0};
    swapchain->image_format = VK_FORMAT_UNDEFINED;
    swapchain->color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swapchain->present_mode = VK_PRESENT_MODE_FIFO_KHR;
    swapchain->current_image = UINT32_MAX;
    swapchain->ready = false;
}



/**
 * Free a swapchain wrapper allocated by dvz_swapchain_create().
 *
 * @param swapchain swapchain wrapper to free
 */
void dvz_swapchain_free(DvzSwapchain* swapchain)
{
    if (swapchain == NULL)
    {
        return;
    }
    dvz_free(swapchain);
}
