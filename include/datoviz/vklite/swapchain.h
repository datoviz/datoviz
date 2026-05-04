/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Swapchain                                                                                    */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <vulkan/vulkan_core.h>

#include "datoviz/common/macros.h"
#include "datoviz/math/types.h"
#include "datoviz/vklite/surface.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzDevice DvzDevice;
typedef struct DvzSwapchain DvzSwapchain;
typedef struct DvzSwapchainConfig DvzSwapchainConfig;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum DvzPresentStatus
{
    DVZ_PRESENT_STATUS_OK = 0,
    DVZ_PRESENT_STATUS_RECREATE,
    DVZ_PRESENT_STATUS_SKIP_ZERO_EXTENT,
    DVZ_PRESENT_STATUS_DEVICE_LOST,
    DVZ_PRESENT_STATUS_ERROR,
} DvzPresentStatus;



struct DvzSwapchainConfig
{
    VkFormat image_format;
    VkColorSpaceKHR color_space;
    VkPresentModeKHR present_mode;
    VkImageUsageFlags image_usage;
    VkCompositeAlphaFlagBitsKHR composite_alpha;
    uint32_t min_image_count;
    bool clipped;
};


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

EXTERN_C_ON



/**
 * Allocate an empty swapchain wrapper.
 *
 * @return allocated swapchain wrapper, or NULL on allocation failure
 */
DVZ_EXPORT DvzSwapchain* dvz_swapchain_create_wrapper(void);



/**
 * Initialize a swapchain wrapper from a logical device and surface.
 *
 * @param swapchain swapchain wrapper to initialize
 * @param device logical device used to resolve the physical GPU
 * @param surface surface wrapper used for capability and extent data
 * @return true when initialization succeeds
 * @note This call also binds the Vulkan logical device handle used by recreate/acquire/present.
 */
DVZ_EXPORT bool
dvz_swapchain_init_from_device(DvzSwapchain* swapchain, DvzDevice* device, DvzSurface* surface);



/**
 * Bind or override the Vulkan logical device used by swapchain create/destroy/acquire paths.
 *
 * @param swapchain swapchain wrapper to configure
 * @param device logical device used to issue swapchain API calls
 * @return true when binding succeeds
 * @note Rebinding a live swapchain is rejected; destroy the swapchain resources first.
 */
DVZ_EXPORT bool dvz_swapchain_device(DvzSwapchain* swapchain, VkDevice device);



/**
 * Set swapchain creation parameters.
 *
 * @param swapchain swapchain wrapper to configure
 * @param config desired swapchain configuration
 * @return true when configuration is accepted
 */
DVZ_EXPORT bool dvz_swapchain_config(DvzSwapchain* swapchain, DvzSwapchainConfig config);



/**
 * Return whether the swapchain wrapper currently owns valid Vulkan resources.
 *
 * @param swapchain swapchain wrapper
 * @return true when swapchain resources are ready for acquire/present
 */
DVZ_EXPORT bool dvz_swapchain_ready(const DvzSwapchain* swapchain);



/**
 * Return the wrapped Vulkan swapchain handle.
 *
 * @param swapchain swapchain wrapper
 * @return wrapped VkSwapchainKHR handle or VK_NULL_HANDLE
 */
DVZ_EXPORT VkSwapchainKHR dvz_swapchain_handle(const DvzSwapchain* swapchain);



/**
 * Return the number of swapchain images in the current recreation state.
 *
 * @param swapchain swapchain wrapper
 * @return swapchain image count
 */
DVZ_EXPORT uint32_t dvz_swapchain_image_count(const DvzSwapchain* swapchain);



/**
 * Return the resolved image format from the latest recreate.
 *
 * @param swapchain swapchain wrapper
 * @return resolved image format
 */
DVZ_EXPORT VkFormat dvz_swapchain_image_format(const DvzSwapchain* swapchain);



/**
 * Return the resolved color space from the latest recreate.
 *
 * @param swapchain swapchain wrapper
 * @return resolved color space
 */
DVZ_EXPORT VkColorSpaceKHR dvz_swapchain_color_space(const DvzSwapchain* swapchain);



/**
 * Return the resolved present mode from the latest recreate.
 *
 * @param swapchain swapchain wrapper
 * @return resolved present mode
 */
DVZ_EXPORT VkPresentModeKHR dvz_swapchain_present_mode(const DvzSwapchain* swapchain);



/**
 * Return the currently configured swapchain creation parameters.
 *
 * @param swapchain swapchain wrapper
 * @return currently stored config
 */
DVZ_EXPORT DvzSwapchainConfig dvz_swapchain_get_config(const DvzSwapchain* swapchain);



/**
 * Return the current swapchain extent from the latest recreate.
 *
 * @param swapchain swapchain wrapper
 * @return current resolved extent
 */
DVZ_EXPORT VkExtent2D dvz_swapchain_extent(const DvzSwapchain* swapchain);



/**
 * Fetch a swapchain image handle by index.
 *
 * @param swapchain swapchain wrapper
 * @param image_idx image index
 * @param[out] image output image handle
 * @return true when the index is valid
 */
DVZ_EXPORT bool
dvz_swapchain_image(const DvzSwapchain* swapchain, uint32_t image_idx, VkImage* image);



/**
 * Fetch a swapchain image view handle by index.
 *
 * @param swapchain swapchain wrapper
 * @param image_idx image index
 * @param[out] image_view output image view handle
 * @return true when the index is valid
 */
DVZ_EXPORT bool
dvz_swapchain_image_view(const DvzSwapchain* swapchain, uint32_t image_idx, VkImageView* image_view);



/**
 * Recreate swapchain images and image views for a new extent.
 *
 * @param swapchain swapchain wrapper to recreate
 * @param size target extent as {width, height}
 * @return present status mapping recreate outcome
 * @note Returns DVZ_PRESENT_STATUS_ERROR when swapchain device binding is missing.
 */
DVZ_EXPORT DvzPresentStatus dvz_swapchain_recreate(DvzSwapchain* swapchain, uvec2 size);



/**
 * Acquire the next image index from the swapchain.
 *
 * @param swapchain swapchain wrapper
 * @param image_available semaphore signaled by Vulkan when image is available
 * @param timeout_ns timeout value passed to Vulkan acquire call
 * @param[out] image_idx output image index
 * @return present status mapping acquire outcome
 */
DVZ_EXPORT DvzPresentStatus dvz_swapchain_acquire(
    DvzSwapchain* swapchain, VkSemaphore image_available, uint64_t timeout_ns, uint32_t* image_idx);



/**
 * Present a previously rendered image.
 *
 * @param swapchain swapchain wrapper
 * @param present_queue queue used for present submission
 * @param image_idx image index to present
 * @param render_finished semaphore waited before presentation
 * @return present status mapping present outcome
 */
DVZ_EXPORT DvzPresentStatus dvz_swapchain_present(
    DvzSwapchain* swapchain, VkQueue present_queue, uint32_t image_idx, VkSemaphore render_finished);



/**
 * Destroy swapchain resources owned by vklite.
 *
 * @param swapchain swapchain wrapper to destroy
 */
DVZ_EXPORT void dvz_swapchain_destroy(DvzSwapchain* swapchain);



/**
 * Free a swapchain wrapper allocated by dvz_swapchain_create().
 *
 * @param swapchain swapchain wrapper to free
 */
DVZ_EXPORT void dvz_swapchain_free(DvzSwapchain* swapchain);



EXTERN_C_OFF
