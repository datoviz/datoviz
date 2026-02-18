/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Swapchain internals                                                                          */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <vulkan/vulkan_core.h>

#include "datoviz/vklite/surface.h"
#include "datoviz/vklite/swapchain.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

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
