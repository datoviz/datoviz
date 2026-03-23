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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <volk.h>

#include "_vk_utils.h"
#include "_alloc.h"
#include "_assertions.h"
#include "_buffers.h"
#include "_compat.h"
#include "_log.h"
#include "datoviz/common/obj.h"
#include "datoviz/math/types.h"
#include "datoviz/vk/memory.h"
#include "datoviz/vklite/buffers.h"
#include "datoviz/vklite/graphics.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Allocate an empty buffer wrapper.
 *
 * @return allocated buffer wrapper, or NULL on allocation failure
 */
DvzBuffer* dvz_buffer_create_wrapper(void)
{
    DvzBuffer* buffer = (DvzBuffer*)dvz_calloc(1, sizeof(DvzBuffer));
    ANN(buffer);
    return buffer;
}



/**
 * Free a buffer wrapper allocated by dvz_buffer_create_wrapper().
 *
 * @param buffer buffer wrapper to free
 */
void dvz_buffer_free(DvzBuffer* buffer)
{
    if (buffer == NULL)
    {
        return;
    }
    dvz_free(buffer);
}



/**
 * Return the current allocated size of a buffer, in bytes.
 *
 * @param buffer the buffer
 * @return allocated size in bytes
 */
DvzSize dvz_buffer_allocated_size(DvzBuffer* buffer)
{
    ANN(buffer);
    if (buffer->alloc == NULL)
    {
        return 0;
    }
    return (DvzSize)dvz_allocation_size(buffer->alloc);
}



void dvz_buffer(DvzDevice* device, DvzVma* allocator, DvzBuffer* buffer)
{
    ANN(device);
    ANN(allocator);
    ANN(buffer);
    dvz_memset(buffer, sizeof(*buffer), 0, sizeof(*buffer));
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



void dvz_buffer_flags(DvzBuffer* buffer, DvzAllocationFlags flags)
{
    ANN(buffer);
    buffer->req_alloc_flags = flags;
}



int dvz_buffer_create(DvzBuffer* buffer)
{
    ANN(buffer);
    ANN(buffer->device);

    DvzVma* allocator = buffer->allocator;
    ANN(allocator);
    DvzDevice* allocator_device = dvz_allocator_device(allocator);
    ANN(allocator_device);
    ASSERT(allocator_device == buffer->device);

    VkBufferCreateInfo info = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    info.size = buffer->req_size;
    info.usage = buffer->req_usage;
    if (buffer->alloc == NULL)
    {
        buffer->alloc = dvz_allocation_create();
        ANN(buffer->alloc);
    }
    DvzAllocationFlags alloc_flags = buffer->req_alloc_flags;
    dvz_allocation_set_flags(buffer->alloc, alloc_flags);
    int out = dvz_allocator_buffer(
        allocator, &info, alloc_flags, buffer->alloc, &buffer->vk_buffer);

    dvz_obj_created(&buffer->obj);
    return out;
}



VkBuffer dvz_buffer_handle(DvzBuffer* buffer)
{
    ANN(buffer);
    return buffer->vk_buffer;
}



/**
 * Return the requested logical size of a buffer, in bytes.
 *
 * @param buffer the buffer
 * @return requested size in bytes
 */
DvzSize dvz_buffer_size_value(DvzBuffer* buffer)
{
    ANN(buffer);
    return buffer->req_size;
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

    bool mapped = buffer->alloc != NULL && dvz_allocation_mapped(buffer->alloc) != NULL;

    dvz_buffer_destroy(buffer);

    dvz_buffer_size(buffer, size);
    dvz_buffer_create(buffer);

    if (mapped)
        dvz_buffer_map(buffer);
}



int dvz_buffer_map(DvzBuffer* buffer)
{
    ANN(buffer);
    if (buffer->alloc == NULL)
    {
        return -1;
    }
    if (dvz_allocation_mapped(buffer->alloc) != NULL)
        return -1;
    log_trace("mapping buffer memory");
    (void)dvz_allocator_map(buffer->allocator, buffer->alloc);
    return dvz_allocation_mapped(buffer->alloc) != NULL ? 0 : 1;
}



void dvz_buffer_unmap(DvzBuffer* buffer)
{
    ANN(buffer);
    if (buffer->alloc == NULL)
    {
        return;
    }
    if (dvz_allocation_mapped(buffer->alloc) == NULL)
        return;
    log_trace("unmapping buffer memory");
    dvz_allocator_unmap(buffer->allocator, buffer->alloc);
}



void dvz_buffer_upload(DvzBuffer* buffer, DvzSize offset, DvzSize size, const void* data)
{
    ANN(buffer);
    ANN(data);
    ASSERT(size > 0);
    ANN(buffer->alloc);
    if (offset + size > dvz_allocation_size(buffer->alloc))
    {
        log_error("the data is too large for the buffer");
        return;
    }

    log_trace("buffer upload of %s", dvz_pretty_size(size));
    if (dvz_allocator_copy_to(buffer->allocator, buffer->alloc, offset, data, size) != 0)
    {
        log_error("failed to upload data to buffer");
        return;
    }
}



void dvz_buffer_download(DvzBuffer* buffer, DvzSize offset, DvzSize size, void* data)
{
    ANN(buffer);
    ANN(data);
    ASSERT(size > 0);

    log_trace("buffer download of %s", dvz_pretty_size(size));
    if (dvz_allocator_copy_from(buffer->allocator, buffer->alloc, offset, data, size) != 0)
    {
        log_error("failed to download data from buffer");
        return;
    }
}



void dvz_buffer_destroy(DvzBuffer* buffer)
{
    ANN(buffer);
    if (!dvz_obj_is_created(&buffer->obj))
    {
        log_trace("skip destruction of already-destroyed buffer");
        return;
    }
    ANN(buffer->device);

    DvzVma* allocator = buffer->allocator;
    ANN(allocator);

    if (buffer->alloc != NULL)
    {
        dvz_buffer_unmap(buffer);
    }

    log_trace("destroying buffer...");
    if (buffer->alloc != NULL)
    {
        dvz_allocator_destroy_buffer(allocator, buffer->alloc, buffer->vk_buffer);
        dvz_allocation_free(buffer->alloc);
        buffer->alloc = NULL;
    }
    buffer->vk_buffer = VK_NULL_HANDLE;
    dvz_obj_destroyed(&buffer->obj);
    log_trace("buffer destroyed");
}



/**
 * Allocate an empty buffer-views wrapper.
 *
 * @return allocated buffer-views wrapper, or NULL on allocation failure
 */
DvzBufferViews* dvz_buffer_views_create(void)
{
    DvzBufferViews* views = (DvzBufferViews*)dvz_calloc(1, sizeof(DvzBufferViews));
    ANN(views);
    return views;
}



/**
 * Create buffer views on an existing GPU buffer.
 *
 * @param buffer the buffer
 * @param count the number of successive views
 * @param offset the offset within the buffer
 * @param size the size of each view, in bytes
 * @param alignment the alignment requirement for the view offsets
 * @param[out] views the created buffer views
 */
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



/**
 * Return the number of logical views configured in a buffer-views wrapper.
 *
 * @param views the buffer views
 * @return the number of configured views
 */
uint32_t dvz_buffer_views_count(DvzBufferViews* views)
{
    ANN(views);
    return views->count;
}



/**
 * Return the size in bytes of each configured logical view.
 *
 * @param views the buffer views
 * @return the logical view size in bytes
 */
DvzSize dvz_buffer_views_size(DvzBufferViews* views)
{
    ANN(views);
    return views->size;
}



/**
 * Return the aligned stride in bytes between successive views.
 *
 * @param views the buffer views
 * @return the aligned stride in bytes, or 0 when no alignment was requested
 */
DvzSize dvz_buffer_views_aligned_size(DvzBufferViews* views)
{
    ANN(views);
    return views->aligned_size;
}



/**
 * Return the byte offset of a configured view.
 *
 * @param views the buffer views
 * @param idx the logical view index
 * @return the byte offset of the selected view
 */
DvzSize dvz_buffer_views_offset(DvzBufferViews* views, uint32_t idx)
{
    ANN(views);
    ASSERT(idx < views->count);
    return views->offsets[idx];
}



/**
 * Free a buffer-views wrapper allocated by dvz_buffer_views_create().
 *
 * @param views buffer-views wrapper to free
 */
void dvz_buffer_views_free(DvzBufferViews* views)
{
    if (views == NULL)
    {
        return;
    }
    dvz_free(views);
}



/**
 * Bind vertex buffers before recording draw commands.
 *
 * @param cmds the command buffers
 * @param first_binding the index of the first vertex binding
 * @param binding_count the number of bindings
 * @param buffers the "binding_count" buffers to bind
 * @param offsets the offsets within each buffer
 */
void dvz_cmd_bind_vertex_buffers(
    DvzCommands* cmds, uint32_t first_binding, uint32_t binding_count, DvzBuffer* buffers,
    DvzSize* offsets)
{
    ANN(cmds);
    ANN(buffers);
    ANN(offsets);
    ASSERT(binding_count > 0);

    VkCommandBuffer cmd = dvz_commands_handle(cmds);
    ANNVK(cmd);

    VkBuffer vk_buffers[DVZ_MAX_VERTEX_BINDINGS] = {0};
    for (uint32_t i = 0; i < binding_count; i++)
    {
        vk_buffers[i] = dvz_buffer_handle(&buffers[i]);
    }
    vkCmdBindVertexBuffers(cmd, first_binding, binding_count, vk_buffers, offsets);
}



void dvz_cmd_bind_index_buffer(DvzCommands* cmds, DvzBuffer* buffer, DvzSize offset)
{
    ANN(cmds);
    ANN(buffer);

    VkCommandBuffer cmd = dvz_commands_handle(cmds);
    ANNVK(cmd);

    vkCmdBindIndexBuffer(cmd, dvz_buffer_handle(buffer), offset, VK_INDEX_TYPE_UINT32);
}
