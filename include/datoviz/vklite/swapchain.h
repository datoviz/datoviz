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



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

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



struct DvzSwapchain
{
    VkPhysicalDevice physical_device;
    DvzSurface* surface;
    VkDevice device;
    VkSwapchainKHR handle;
    DvzSwapchainConfig config;
    VkExtent2D extent;
    VkFormat image_format;
    VkColorSpaceKHR color_space;
    VkPresentModeKHR present_mode;
    uint32_t image_count;
    VkImage* images;
    VkImageView* image_views;
    uint32_t current_image;
    bool ready;
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

EXTERN_C_ON



/**
 * Initialize a swapchain wrapper from a logical device and surface.
 *
 * @param swapchain swapchain wrapper to initialize
 * @param device logical device used to resolve the physical GPU
 * @param surface surface wrapper used for capability and extent data
 * @return true when initialization succeeds
 * @note This call does not bind a VkDevice. Call dvz_swapchain_device() before recreate/acquire/present.
 */
DVZ_EXPORT bool
dvz_swapchain_init_from_device(DvzSwapchain* swapchain, DvzDevice* device, DvzSurface* surface);



/**
 * Bind the Vulkan logical device used by swapchain create/destroy/acquire paths.
 *
 * @param swapchain swapchain wrapper to configure
 * @param device logical device used to issue swapchain API calls
 * @return true when binding succeeds
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



EXTERN_C_OFF
