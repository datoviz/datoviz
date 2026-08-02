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

#include "obj.h"
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
    VkExternalMemoryImageCreateInfo external_info;
    DvzAllocationFlags req_alloc_flags;

    VkImage vk_images[DVZ_MAX_IMAGES];
    DvzAllocation* allocs[DVZ_MAX_IMAGES];
};


/**
 * Configure external-memory handle compatibility for image allocation.
 *
 * This stays inline because Canvas also configures DvzImages while building as a separate Windows
 * DLL layer; keeping the helper header-local avoids an internal cross-DLL symbol dependency.
 *
 * @param images Image wrapper.
 * @param handle_types External-memory handle types, or zero to disable.
 */
static inline void _dvz_images_external(
    DvzImages* images, VkExternalMemoryHandleTypeFlagsKHR handle_types)
{
    ANN(images);
    images->external_info = (VkExternalMemoryImageCreateInfo){
        .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO,
        .handleTypes = handle_types,
    };
    images->info.pNext = handle_types != 0 ? &images->external_info : NULL;
}



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
