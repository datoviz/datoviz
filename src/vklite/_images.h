/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Images internals                                                                             */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/common/obj.h"
#include "datoviz/vk/memory.h"
#include "datoviz/vklite/images.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzImages
{
    DvzObject obj;
    DvzDevice* device;
    DvzVma* allocator;

    uint32_t count;
    bool is_swapchain;

    VkImageCreateInfo info;
    DvzAllocationFlags req_alloc_flags;

    VkImage vk_images[DVZ_MAX_IMAGES];
    DvzAllocation* allocs[DVZ_MAX_IMAGES];
};



struct DvzImageViews
{
    DvzObject obj;
    DvzDevice* device;
    DvzImages* img;

    VkImageViewCreateInfo info;

    VkImageView vk_views[DVZ_MAX_IMAGES];
};



struct DvzImageBlit
{
    VkBlitImageInfo2 info;
    VkImageBlit2 blit;
};



struct DvzImageCopy
{
    VkCopyImageInfo2 info;
    VkImageCopy2 copy;
};
