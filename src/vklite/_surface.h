/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Surface internals                                                                            */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <vulkan/vulkan_core.h>



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

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
