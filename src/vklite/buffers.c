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
#if OS_UNIX
#include <unistd.h>
#endif

#include "_vk_utils.h"
#include "_alloc.h"
#include "_assertions.h"
#include "_buffers.h"
#include "_compat.h"
#include "_log.h"
#include "obj.h"
#include "datoviz/common/functions.h"
#include "datoviz/math/types.h"
#include "datoviz/vk/device.h"
#include "datoviz/vk/memory.h"
#include "datoviz/vk/memory_interop.h"
#include "datoviz/vk/queues.h"
#include "datoviz/vklite/commands.h"
#include "datoviz/vklite/buffers.h"
#include "datoviz/vklite/graphics.h"
#include "datoviz/vklite/sync.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

#define DVZ_INTEROP_BUFFER_EXPORT_CONFIG_KNOWN_FLAGS 0u



static bool _interop_buffer_export_config_validate(
    const DvzInteropBufferExportConfig* config)
{
    if (config == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(
            config, DvzInteropBufferExportConfig,
            DVZ_INTEROP_BUFFER_EXPORT_CONFIG_KNOWN_FLAGS))
    {
        log_error("invalid DvzInteropBufferExportConfig ABI prologue");
        return false;
    }
    return true;
}



DvzInteropBufferExportConfig dvz_interop_buffer_export_config(void)
{
    return (DvzInteropBufferExportConfig){
        DVZ_STRUCT_INIT_FIELDS(DvzInteropBufferExportConfig),
    };
}



/**
 * Close an exported interop handle on Unix failure paths.
 *
 * @param handle exported handle, or -1 when absent
 */
static void _interop_close_exported_handle(DvzExternalHandle handle)
{
    dvz_external_handle_close(handle);
}



/**
 * Fill the Vulkan physical-device UUID in an interop export descriptor.
 *
 * @param buffer live buffer whose device owns the physical device
 * @param[out] out export descriptor to update
 */
static void _interop_buffer_export_device_uuid(DvzBuffer* buffer, DvzInteropBufferExport* out)
{
    ANN(buffer);
    ANN(buffer->device);
    ANN(out);

    VkPhysicalDeviceIDProperties id = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES,
    };
    VkPhysicalDeviceProperties2 props = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        .pNext = &id,
    };
    vkGetPhysicalDeviceProperties2(dvz_device_physical_device(buffer->device), &props);
    dvz_memcpy(out->device_uuid, sizeof(out->device_uuid), id.deviceUUID, VK_UUID_SIZE);
    out->device_uuid_valid = 1;
}



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
    if (dvz_obj_is_created(&buffer->obj))
    {
        log_error("cannot create a buffer twice without destroying it first");
        return 1;
    }
    if (buffer->req_size == 0)
    {
        log_error("cannot create a buffer with zero size");
        return 1;
    }
    if (buffer->req_usage == 0)
    {
        log_error("cannot create a buffer without usage flags");
        return 1;
    }

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
    if (out != 0)
    {
        if (buffer->vk_buffer != VK_NULL_HANDLE)
        {
            dvz_allocator_destroy_buffer(allocator, buffer->alloc, buffer->vk_buffer);
            buffer->vk_buffer = VK_NULL_HANDLE;
        }
        dvz_allocation_free(buffer->alloc);
        buffer->alloc = NULL;
        return out;
    }

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



VkBufferUsageFlags dvz_buffer_usage_value(DvzBuffer* buffer)
{
    ANN(buffer);
    return buffer->req_usage;
}



/**
 * Export a vklite buffer and package external interop metadata.
 *
 * @param buffer the live Vulkan-owned buffer
 * @param config logical export range and optional timeline semaphore metadata
 * @param[out] out export descriptor
 * @return 0 on success, -1 on failure
 */
int dvz_interop_buffer_export_from_buffer(
    DvzBuffer* buffer, const DvzInteropBufferExportConfig* config,
    DvzInteropBufferExport* out)
{
    ANN(buffer);
    ANN(out);

    dvz_memset(out, sizeof(*out), 0, sizeof(*out));
    out->memory_handle = -1;
    out->semaphore_handle = -1;
    if (!_interop_buffer_export_config_validate(config))
        return -1;

    if (!dvz_obj_is_created(&buffer->obj) || buffer->vk_buffer == VK_NULL_HANDLE ||
        buffer->alloc == NULL)
    {
        log_error("cannot export an uncreated interop buffer");
        return -1;
    }
    if (buffer->allocator == NULL || dvz_allocator_external(buffer->allocator) == 0)
    {
        log_error("cannot export an interop buffer without an exportable allocator");
        return -1;
    }

    uint64_t allocation_size = (uint64_t)dvz_allocation_size(buffer->alloc);
    uint64_t logical_size = (uint64_t)buffer->req_size;
    if (logical_size == 0 || allocation_size == 0)
    {
        log_error("cannot export an empty interop buffer");
        return -1;
    }
    if (logical_size > allocation_size)
    {
        logical_size = allocation_size;
    }

    uint64_t offset = config != NULL ? config->offset : 0;
    if (offset >= logical_size)
    {
        log_error("interop buffer export offset exceeds the logical buffer size");
        return -1;
    }

    uint64_t size = config != NULL ? config->size : 0;
    if (size == 0)
    {
        size = logical_size - offset;
    }
    if (size == 0 || size > logical_size - offset)
    {
        log_error("interop buffer export range exceeds the logical buffer size");
        return -1;
    }

    DvzExternalHandle semaphore_handle = DVZ_EXTERNAL_HANDLE_INVALID;
    uint32_t semaphore_handle_type = 0;
    uint64_t semaphore_value = 0;
    uint32_t drp2_usage = 0;
    uint32_t flags = 0;
    if (config != NULL)
    {
        drp2_usage = config->drp2_usage;
        flags = config->export_flags;
        semaphore_value = config->semaphore_value;
        if (config->semaphore != NULL)
        {
            if (config->semaphore_handle_type == 0)
            {
                log_error("interop semaphore export requires an external semaphore handle type");
                return -1;
            }
            semaphore_handle_type = config->semaphore_handle_type;
            semaphore_handle = dvz_semaphore_export(config->semaphore, semaphore_handle_type);
            if (semaphore_handle == DVZ_EXTERNAL_HANDLE_INVALID)
            {
                log_error("failed to export interop semaphore handle");
                return -1;
            }
        }
    }

    if (dvz_interop_buffer_export(
            buffer->allocator, buffer->alloc, offset, size, buffer->req_usage, semaphore_handle,
            semaphore_handle_type, semaphore_value, out) != 0)
    {
        _interop_close_exported_handle(semaphore_handle);
        return -1;
    }

    out->version = DVZ_INTEROP_BUFFER_EXPORT_VERSION;
    out->usage = buffer->req_usage;
    out->vk_usage = buffer->req_usage;
    out->drp2_usage = drp2_usage;
    out->flags = flags;
    _interop_buffer_export_device_uuid(buffer, out);

    return 0;
}


/**
 * Resolve synchronization2 destination scopes for an interop-buffer consumer.
 *
 * @param consumer declared Vulkan consumer
 * @param[out] stage destination pipeline stage
 * @param[out] access destination access mask
 * @return true when the consumer is supported
 */
static bool _interop_buffer_consumer_sync(
    DvzInteropBufferConsumer consumer, VkPipelineStageFlags2* stage, VkAccessFlags2* access,
    VkBufferUsageFlags* usage)
{
    ANN(stage);
    ANN(access);
    ANN(usage);

    switch (consumer)
    {
    case DVZ_INTEROP_BUFFER_CONSUMER_VERTEX_ATTRIBUTE_READ:
        *stage = VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT;
        *access = VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
        *usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        return true;
    case DVZ_INTEROP_BUFFER_CONSUMER_TRANSFER_READ:
        *stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        *access = VK_ACCESS_2_TRANSFER_READ_BIT;
        *usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        return true;
    default:
        log_error("unknown interop buffer consumer (%d)", consumer);
        return false;
    }
}



/**
 * Wait on a timeline semaphore before Vulkan consumes externally written interop-buffer data.
 *
 * @param device logical device owning the main Vulkan queue
 * @param buffer buffer whose contents were written externally
 * @param size byte size of the synchronized buffer range
 * @param semaphore timeline semaphore signaled by the external API
 * @param value timeline value to wait on
 * @param consumer declared Vulkan consumer
 * @return true on success
 */
bool dvz_interop_buffer_wait_timeline_for_consumer(
    DvzDevice* device, DvzBuffer* buffer, uint64_t size, DvzSemaphore* semaphore, uint64_t value,
    DvzInteropBufferConsumer consumer)
{
    ANN(device);
    ANN(buffer);
    ANN(semaphore);
    ASSERT(size > 0);

    VkPipelineStageFlags2 dst_stage = VK_PIPELINE_STAGE_2_NONE;
    VkAccessFlags2 dst_access = VK_ACCESS_2_NONE;
    VkBufferUsageFlags required_usage = 0;
    if (!_interop_buffer_consumer_sync(consumer, &dst_stage, &dst_access, &required_usage))
        return false;

    VkSemaphore vk_semaphore = dvz_semaphore_handle(semaphore);
    if (vk_semaphore == VK_NULL_HANDLE)
    {
        log_error("cannot wait on an uncreated interop semaphore");
        return false;
    }
    if (!dvz_obj_is_created(&buffer->obj) || buffer->vk_buffer == VK_NULL_HANDLE)
    {
        log_error("cannot synchronize an uncreated interop buffer");
        return false;
    }
    if (size > buffer->req_size)
    {
        log_error("interop synchronization range exceeds the buffer size");
        return false;
    }
    if ((buffer->req_usage & required_usage) == 0)
    {
        log_error("interop buffer lacks usage required by declared consumer");
        return false;
    }

    DvzQueue* queue = dvz_device_queue(device, DVZ_QUEUE_MAIN);
    if (queue == NULL)
    {
        log_error("main Vulkan queue unavailable for interop buffer wait");
        return false;
    }

    DvzCommands* cmds = dvz_commands_create_wrapper();
    ANN(cmds);
    dvz_commands(device, queue, 1, cmds);
    if (dvz_commands_count(cmds) == 0)
    {
        log_error("failed to allocate command buffer for interop buffer wait");
        dvz_commands_free(cmds);
        return false;
    }

    bool ok = false;
    if (dvz_cmd_begin_result(cmds) == 0)
    {
        DvzBarriers barriers = {0};
        dvz_barriers(&barriers);
        DvzBarrierBuffer* bbuf =
            dvz_barriers_buffer(&barriers, dvz_buffer_handle(buffer), 0, (VkDeviceSize)size);
        dvz_barrier_buffer_stage(bbuf, VK_PIPELINE_STAGE_2_NONE, dst_stage);
        dvz_barrier_buffer_access(bbuf, VK_ACCESS_2_NONE, dst_access);
        dvz_cmd_barriers(cmds, &barriers);

        if (dvz_cmd_end_result(cmds) == 0)
        {
            DvzSubmit* submit = dvz_submit_create_wrapper();
            ANN(submit);
            dvz_submit(submit);
            dvz_submit_wait(submit, vk_semaphore, value, dst_stage);
            dvz_submit_command(submit, dvz_commands_handle(cmds));
            VkResult res = (VkResult)dvz_submit_send(
                submit, dvz_queue_handle(queue), VK_NULL_HANDLE);
            if (res == VK_SUCCESS)
            {
                dvz_queue_wait(queue);
                ok = true;
            }
            else
            {
                log_error("Vulkan interop buffer wait submit failed (%d)", res);
            }
            dvz_submit_free(submit);
        }
    }

    dvz_commands_destroy(cmds);
    dvz_commands_free(cmds);
    return ok;
}



/**
 * Preserve the vertex-input-only interop wait compatibility helper.
 *
 * @param device logical device owning the main Vulkan queue
 * @param buffer buffer whose contents were written externally
 * @param size byte size of the synchronized buffer range
 * @param semaphore timeline semaphore signaled by the external API
 * @param value timeline value to wait on
 * @return true on success
 */
bool dvz_interop_buffer_wait_timeline(
    DvzDevice* device, DvzBuffer* buffer, uint64_t size, DvzSemaphore* semaphore, uint64_t value)
{
    return dvz_interop_buffer_wait_timeline_for_consumer(
        device, buffer, size, semaphore, value,
        DVZ_INTEROP_BUFFER_CONSUMER_VERTEX_ATTRIBUTE_READ);
}



/**
 * Signal an interop timeline semaphore from the main queue after transfer reads.
 *
 * @param device logical device owning the main Vulkan queue
 * @param buffer buffer whose transfer reads have completed in main-queue order
 * @param size byte size of the synchronized buffer range
 * @param semaphore timeline semaphore to signal from the GPU queue
 * @param value monotonically increasing timeline value to signal
 * @return true on success
 */
bool dvz_interop_buffer_signal_timeline_after_transfer(
    DvzDevice* device, DvzBuffer* buffer, uint64_t size, DvzSemaphore* semaphore, uint64_t value)
{
    ANN(device);
    ANN(buffer);
    ANN(semaphore);
    ASSERT(size > 0);

    VkSemaphore vk_semaphore = dvz_semaphore_handle(semaphore);
    if (vk_semaphore == VK_NULL_HANDLE)
    {
        log_error("cannot signal an uncreated interop semaphore");
        return false;
    }
    if (!dvz_obj_is_created(&buffer->obj) || buffer->vk_buffer == VK_NULL_HANDLE)
    {
        log_error("cannot synchronize an uncreated interop buffer");
        return false;
    }
    if (size > buffer->req_size)
    {
        log_error("interop synchronization range exceeds the buffer size");
        return false;
    }
    if ((buffer->req_usage & VK_BUFFER_USAGE_TRANSFER_SRC_BIT) == 0)
    {
        log_error("interop buffer lacks transfer-source usage for release signal");
        return false;
    }

    DvzQueue* queue = dvz_device_queue(device, DVZ_QUEUE_MAIN);
    if (queue == NULL)
    {
        log_error("main Vulkan queue unavailable for interop buffer signal");
        return false;
    }

    DvzCommands* cmds = dvz_commands_create_wrapper();
    ANN(cmds);
    dvz_commands(device, queue, 1, cmds);
    if (dvz_commands_count(cmds) == 0)
    {
        log_error("failed to allocate command buffer for interop buffer signal");
        dvz_commands_free(cmds);
        return false;
    }

    bool ok = false;
    if (dvz_cmd_begin_result(cmds) == 0)
    {
        DvzBarriers barriers = {0};
        dvz_barriers(&barriers);
        DvzBarrierBuffer* bbuf =
            dvz_barriers_buffer(&barriers, dvz_buffer_handle(buffer), 0, (VkDeviceSize)size);
        dvz_barrier_buffer_stage(bbuf, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_PIPELINE_STAGE_2_NONE);
        dvz_barrier_buffer_access(bbuf, VK_ACCESS_2_TRANSFER_READ_BIT, VK_ACCESS_2_NONE);
        dvz_cmd_barriers(cmds, &barriers);

        if (dvz_cmd_end_result(cmds) == 0)
        {
            DvzSubmit* submit = dvz_submit_create_wrapper();
            ANN(submit);
            dvz_submit(submit);
            dvz_submit_command(submit, dvz_commands_handle(cmds));
            dvz_submit_signal(submit, vk_semaphore, value, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT);
            VkResult res = (VkResult)dvz_submit_send(
                submit, dvz_queue_handle(queue), VK_NULL_HANDLE);
            if (res == VK_SUCCESS)
            {
                dvz_queue_wait(queue);
                ok = true;
            }
            else
            {
                log_error("Vulkan interop buffer signal submit failed (%d)", res);
            }
            dvz_submit_free(submit);
        }
    }

    dvz_commands_destroy(cmds);
    dvz_commands_free(cmds);
    return ok;
}




void dvz_buffer_resize(DvzBuffer* buffer, DvzSize size)
{
    ANN(buffer);

    if (size <= buffer->req_size)
    {
        char current_size_str[64] = {0};
        char requested_size_str[64] = {0};
        log_trace(
            "skip buffer resizing as the buffer size is large enough:"
            "(currently %s, requested %s)",
            dvz_pretty_size(buffer->req_size, current_size_str, sizeof(current_size_str)),
            dvz_pretty_size(size, requested_size_str, sizeof(requested_size_str)));
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

    char size_str[64] = {0};
    log_trace("buffer upload of %s", dvz_pretty_size(size, size_str, sizeof(size_str)));
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

    char size_str[64] = {0};
    log_trace("buffer download of %s", dvz_pretty_size(size, size_str, sizeof(size_str)));
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



void
dvz_cmd_bind_index_buffer(DvzCommands* cmds, DvzBuffer* buffer, DvzSize offset, VkIndexType index_type)
{
    ANN(cmds);
    ANN(buffer);

    VkCommandBuffer cmd = dvz_commands_handle(cmds);
    ANNVK(cmd);

    vkCmdBindIndexBuffer(cmd, dvz_buffer_handle(buffer), offset, index_type);
}
