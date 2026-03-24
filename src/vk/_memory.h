/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Memory internals                                                                             */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/vk/memory.h"

MUTE_ON
#include "vk_mem_alloc.h"
MUTE_OFF



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzVma
{
    DvzDevice* device;
    VmaAllocator vma;
    VkExternalMemoryHandleTypeFlagsKHR external;
};



struct DvzAllocation
{
    VmaMemoryUsage usage;
    DvzAllocationFlags flags;
    VmaAllocationInfo info;
    VmaAllocation alloc;
    VkMemoryPropertyFlags memory_flags;
    DvzSize alignment; // alignment required by Vulkan
    void* mmap;
};
