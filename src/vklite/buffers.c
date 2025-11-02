/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Buffers                                                                                      */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "datoviz/common/obj.h"
#include "datoviz/vk/memory.h"
#include "datoviz/vk/queues.h"
#include "datoviz/vklite/buffers.h"
#include "types.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

void dvz_buffer(DvzDevice* device, DvzVma* allocator, DvzBuffer* buffer)
{
    ANN(device);
    ANN(allocator);
    ANN(buffer);

    buffer->device = device;
    buffer->allocator = allocator;
    dvz_obj_init(&buffer->obj);
}



void dvz_buffer_size(DvzBuffer* buffer, DvzSize size)
{
    ANN(buffer);
    buffer->req_size = size;
}



void dvz_buffer_usage(DvzBuffer* buffer, VkBufferUsageFlags usage)
{
    ANN(buffer);
    buffer->req_usage = usage;
}



void dvz_buffer_flags(DvzBuffer* buffer, VmaAllocationCreateFlags flags)
{
    ANN(buffer);
    buffer->alloc.flags = flags;
}



int dvz_buffer_create(DvzBuffer* buffer)
{
    ANN(buffer);
    ANN(buffer->device);

    DvzVma* allocator = buffer->allocator;
    ANN(allocator);

    VkBufferCreateInfo info = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    info.size = buffer->req_size;
    info.usage = buffer->req_usage;
    int out = dvz_allocator_buffer(
        allocator, &info, buffer->alloc.flags, &buffer->alloc, &buffer->vk_buffer);

    dvz_obj_created(&buffer->obj);
    return out;
}



void dvz_buffer_resize(DvzBuffer* buffer, DvzSize size)
{
    ANN(buffer);

    if (size <= buffer->req_size)
    {
        log_trace(
            "skip buffer resizing as the buffer size is large enough:"
            "(currently %s, requested %s)",
            dvz_pretty_size(buffer->req_size), dvz_pretty_size(size));
        return;
    }

    bool mapped = buffer->alloc.mmap != NULL;

    dvz_buffer_destroy(buffer);

    dvz_buffer_size(buffer, size);
    dvz_buffer_create(buffer);

    if (mapped)
        dvz_buffer_map(buffer);
}



int dvz_buffer_map(DvzBuffer* buffer)
{
    ANN(buffer);
    if (buffer->alloc.mmap != NULL)
        return -1;
    log_trace("mapping buffer memory");
    buffer->alloc.mmap = dvz_allocator_map(buffer->allocator, &buffer->alloc);
    return buffer->alloc.mmap != NULL ? 0 : 1;
}



void dvz_buffer_unmap(DvzBuffer* buffer)
{
    ANN(buffer);
    if (buffer->alloc.mmap == NULL)
        return;
    log_trace("unmapping buffer memory");
    dvz_allocator_unmap(buffer->allocator, &buffer->alloc);
    buffer->alloc.mmap = NULL;
}



void dvz_buffer_upload(DvzBuffer* buffer, DvzSize offset, DvzSize size, const void* data)
{
    ANN(buffer);
    ANN(data);
    ASSERT(size > 0);
    if (offset + size > buffer->alloc.info.size)
    {
        log_error("the data is too large for the buffer");
        return;
    }

    bool need_unmap = false;
    if (buffer->alloc.mmap == NULL)
    {
        dvz_buffer_map(buffer);
        need_unmap = true;
    }

    ANN(buffer->alloc.mmap);
    log_trace("buffer upload of %s", dvz_pretty_size(size));
    dvz_memcpy(POINTER_OFFSET(buffer->alloc.mmap, offset), size, data, size);

    if (need_unmap)
    {
        dvz_buffer_unmap(buffer);
    }
}



void dvz_buffer_download(DvzBuffer* buffer, DvzSize offset, DvzSize size, void* data)
{
    ANN(buffer);
    ANN(data);
    ASSERT(size > 0);

    bool need_unmap = false;
    if (buffer->alloc.mmap == NULL)
    {
        dvz_buffer_map(buffer);
        need_unmap = true;
    }

    ANN(buffer->alloc.mmap);
    log_trace("buffer download of %s", dvz_pretty_size(size));
    dvz_memcpy(data, size, POINTER_OFFSET(buffer->alloc.mmap, offset), size);

    if (need_unmap)
    {
        dvz_buffer_unmap(buffer);
    }
}



void dvz_buffer_destroy(DvzBuffer* buffer)
{
    ANN(buffer);
    ANN(buffer->device);

    DvzVma* allocator = buffer->allocator;
    ANN(allocator);

    dvz_buffer_unmap(buffer);

    log_trace("destroying buffer...");
    dvz_allocator_destroy_buffer(allocator, &buffer->alloc, buffer->vk_buffer);
    dvz_obj_destroyed(&buffer->obj);
    log_trace("buffer destroyed");
}



void dvz_buffer_views(
    DvzBuffer* buffer, uint32_t count, //
    DvzSize offset, DvzSize size, DvzSize alignment, DvzBufferViews* views)
{
    ANN(buffer);
    ANN(buffer->device);
    ASSERT(count <= DVZ_MAX_BUFFER_VIEWS);

    views->buffer = buffer;
    views->count = count;
    views->size = size;
    views->alignment = alignment;

    DvzSize offset_req = offset;
    if (alignment > 0)
    {
        // Aligned size for uniform buffers.
        views->aligned_size = dvz_aligned_size(size, alignment);
        // Align the offset.
        offset = dvz_aligned_size(offset, alignment);
        ASSERT(offset >= offset_req);
        ASSERT(views->aligned_size >= views->size);
        // Align the size.
        size = views->aligned_size;
    }

    // Compute the offsets.
    for (uint32_t i = 0; i < count; i++)
    {
        views->offsets[i] = offset + i * size;
        if (alignment > 0)
        {
            ASSERT(views->offsets[i] % alignment == 0);
        }
    }
}
