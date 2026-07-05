/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Buffers internals                                                                            */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "obj.h"
#include "datoviz/vk/memory.h"
#include "datoviz/vklite/buffers.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzBuffer
{
    DvzObject obj;
    DvzDevice* device;
    DvzVma* allocator;

    DvzSize req_size;
    VkBufferUsageFlags req_usage;
    DvzAllocationFlags req_alloc_flags;

    VkBuffer vk_buffer;
    DvzAllocation* alloc;
};



struct DvzBufferViews
{
    DvzBuffer* buffer;
    uint32_t count;
    DvzSize size;
    DvzSize aligned_size; // NOTE: is non-null only for aligned arrays
    DvzSize alignment;
    DvzSize offsets[DVZ_MAX_BUFFER_VIEWS];
};
