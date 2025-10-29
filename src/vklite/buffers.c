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

#include "../src/vk/macros.h"
#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "datoviz/common/macros.h"
#include "datoviz/common/obj.h"
#include "datoviz/vk/device.h"
#include "datoviz/vk/memory.h"
#include "datoviz/vk/queues.h"
#include "datoviz/vklite/buffers.h"
#include "types.h"
#include "vulkan/vulkan_core.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

void dvz_buffer(DvzDevice* device, DvzVma* allocator, DvzBuffer* buffer)
{
    ANN(device);
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



int dvz_buffer_create(DvzBuffer* buffer)
{
    ANN(buffer);
    ANN(buffer->device);

    DvzVma* allocator = buffer->allocator;
    ANN(allocator);

    VkBufferCreateInfo buf_info = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buf_info.size = buffer->req_size;
    buf_info.usage = buffer->req_usage;
    int out = dvz_allocator_buffer(allocator, &buf_info, 0, &buffer->alloc, &buffer->vk_buffer);

    dvz_obj_created(&buffer->obj);
    return out;
}



void dvz_buffer_resize(DvzBuffer* buffer, DvzSize size)
{
    // ANN(buffer);
    // DvzGpu* gpu = buffer->gpu;
    // if (size <= buffer->size)
    // {
    //     log_trace(
    //         "skip buffer resizing as the buffer size is large enough:"
    //         "(requested %s, is %s already)",
    //         pretty_size(buffer->size), pretty_size(size));
    //     return;
    // }
    // log_debug("[SLOW] resize buffer to size %s", pretty_size(size));

    // // Create the new buffer with the new size.
    // DvzBuffer new_buffer = dvz_buffer(gpu);
    // _buffer_copy(buffer, &new_buffer);
    // // Make sure we can copy to the new buffer.
    // bool proceed = true;
    // if ((new_buffer.usage & VK_BUFFER_USAGE_TRANSFER_DST_BIT) == 0)
    // {
    //     log_warn("buffer was not created with VK_BUFFER_USAGE_TRANSFER_DST_BIT and therefore the
    //     "
    //              "data cannot be kept while resizing it");
    //     proceed = false;
    // }
    // new_buffer.size = size;
    // _buffer_create(&new_buffer);

    // if (new_buffer.vk_buffer == VK_NULL_HANDLE)
    // {
    //     return;
    // }

    // // At this point, the new buffer is empty.

    // // Handle permanent mapping.
    // void* old_mmap = buffer->mmap;
    // if (buffer->mmap != NULL)
    // {
    //     // Unmap the to-be-deleted buffer.
    //     dvz_buffer_unmap(buffer);
    //     // NOTE: buffer->mmap remains not NULL but invalid: it will need to be reset to a new
    //     // mapped region after creation of the new buffer.
    //     buffer->mmap = NULL;
    // }

    // // If a DvzCommands object was passed for the data transfer, transfer the data from the
    // // old buffer to the new, by flushing the corresponding queue and waiting for completion.

    // // HACK: use queue 0 for transfers (convention)
    // // DvzCommands cmds_ = dvz_commands(gpu, 0, 1);
    // DvzCommands* cmds = &gpu->cmd;
    // if (proceed)
    // {
    //     uint32_t queue_idx = cmds->queue_idx;
    //     log_debug("copying data from the old buffer to the new one before destroying the old
    //     one"); ASSERT(queue_idx < gpu->queues.queue_count); ASSERT(size >= buffer->size);

    //     dvz_cmd_reset(cmds, 0);
    //     dvz_cmd_begin(cmds, 0);
    //     dvz_cmd_copy_buffer(cmds, 0, buffer, 0, &new_buffer, 0, buffer->size);
    //     dvz_cmd_end(cmds, 0);

    //     VkQueue queue = gpu->queues.queues[queue_idx];
    //     dvz_cmd_submit_sync(cmds, 0);
    //     vkQueueWaitIdle(queue);
    // }

    // // Delete the old buffer after the transfer has finished.
    // _buffer_destroy(buffer);

    // // Update the existing buffer's size.
    // buffer->size = new_buffer.size;
    // ASSERT(buffer->size == size);

    // // Update the existing DvzBuffer struct with the newly-created Vulkan objects.
    // buffer->vk_buffer = new_buffer.vk_buffer;
    // // buffer->device_memory = new_buffer.device_memory;
    // buffer->vma = new_buffer.vma;

    // ASSERT(buffer->vk_buffer != VK_NULL_HANDLE);
    // // ASSERT(buffer->device_memory != VK_NULL_HANDLE);

    // // If the existing buffer was already mapped, we need to remap the new buffer.
    // if (old_mmap != NULL)
    // {
    //     buffer->mmap = dvz_buffer_map(buffer, 0, VK_WHOLE_SIZE);
    //     // Make sure the permanent memmap has been updated after the buffer resize.
    //     ASSERT(buffer->mmap != old_mmap);
    // }
}



int dvz_buffer_map(DvzBuffer* buffer)
{
    ANN(buffer);
    buffer->alloc.mmap = dvz_allocator_map(buffer->allocator, &buffer->alloc);
    return buffer->alloc.mmap != NULL ? 0 : 1;
}



void dvz_buffer_unmap(DvzBuffer* buffer)
{
    ANN(buffer);
    dvz_allocator_unmap(buffer->allocator, &buffer->alloc);
    buffer->alloc.mmap = NULL;
}



void dvz_buffer_destroy(DvzBuffer* buffer)
{
    ANN(buffer);
    ANN(buffer->device);

    DvzVma* allocator = buffer->allocator;
    ANN(allocator);

    dvz_allocator_destroy_buffer(allocator, &buffer->alloc, buffer->vk_buffer);
    dvz_obj_destroyed(&buffer->obj);
}
