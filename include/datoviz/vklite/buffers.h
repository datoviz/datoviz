/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Buffers                                                                                      */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/common/macros.h"
#include "datoviz/math/types.h"
#include "datoviz/vk/memory.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_MAX_BUFFER_VIEWS 4



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzDevice DvzDevice;
typedef struct DvzVma DvzVma;
typedef struct DvzCommands DvzCommands;

typedef struct DvzBuffer DvzBuffer;
typedef struct DvzBufferViews DvzBufferViews;


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

EXTERN_C_ON



/**
 * Allocate an empty buffer wrapper.
 *
 * Heap-allocated wrappers follow the same lifecycle as stack-owned wrappers:
 * initialize with dvz_buffer(), configure, call dvz_buffer_create() once, then
 * destroy before any recreate and free only if this wrapper came from
 * dvz_buffer_create_wrapper().
 *
 * @return allocated buffer wrapper, or NULL on allocation failure
 */
DVZ_EXPORT DvzBuffer* dvz_buffer_create_wrapper(void);



/**
 * Free a buffer wrapper allocated by dvz_buffer_create_wrapper().
 *
 * @param buffer buffer wrapper to free
 */
DVZ_EXPORT void dvz_buffer_free(DvzBuffer* buffer);



/**
 * Return the current allocated size of a buffer, in bytes.
 *
 * @param buffer the buffer
 * @return allocated size in bytes
 */
DVZ_EXPORT DvzSize dvz_buffer_allocated_size(DvzBuffer* buffer);



/**
 * Initialize a GPU buffer.
 *
 * This prepares the wrapper for configuration. Call dvz_buffer_create() once
 * after setting the desired size, usage, and allocation flags. Recreating a
 * live buffer requires dvz_buffer_destroy() first.
 *
 * @param device logical device that owns the Vulkan buffer; must outlive `buffer`
 * @param allocator allocator used for buffer memory; must outlive the created buffer
 * @param[out] buffer the initialized buffer
 */
DVZ_EXPORT void dvz_buffer(DvzDevice* device, DvzVma* allocator, DvzBuffer* buffer);



/**
 * Set the buffer size.
 *
 * @param buffer initialized, not-yet-created buffer wrapper
 * @param size the buffer size, in bytes
 */
DVZ_EXPORT void dvz_buffer_size(DvzBuffer* buffer, DvzSize size);



/**
 * Set the buffer usage.
 *
 * @param buffer initialized, not-yet-created buffer wrapper
 * @param usage Vulkan usage flags for the buffer
 */
DVZ_EXPORT void dvz_buffer_usage(DvzBuffer* buffer, VkBufferUsageFlags usage);



/**
 * Set the allocation policy flags used when the buffer creates its memory.
 *
 * @param buffer initialized, not-yet-created buffer wrapper
 * @param flags Datoviz allocation policy flags
 */
DVZ_EXPORT void dvz_buffer_flags(DvzBuffer* buffer, DvzAllocationFlags flags);



/**
 * Create the buffer after it has been set.
 *
 * This function creates the wrapped Vulkan buffer exactly once per live
 * wrapper. Call dvz_buffer_destroy() before attempting to create it again.
 *
 * @param buffer configured, not-yet-created buffer wrapper
 * @return 0 on success, non-zero on Vulkan or Datoviz state failure
 */
DVZ_EXPORT int dvz_buffer_create(DvzBuffer* buffer);



/**
 * Return a Vulkan handle to a buffer.
 *
 * @param buffer live buffer wrapper
 * @return borrowed Vulkan buffer handle, or `VK_NULL_HANDLE` when no buffer is live
 */
DVZ_EXPORT VkBuffer dvz_buffer_handle(DvzBuffer* buffer);



/**
 * Return the requested logical size of a buffer, in bytes.
 *
 * @param buffer initialized buffer wrapper
 * @return requested logical size in bytes
 */
DVZ_EXPORT DvzSize dvz_buffer_size_value(DvzBuffer* buffer);



/**
 * Return the requested Vulkan usage flags of a buffer.
 *
 * @param buffer initialized buffer wrapper
 * @return requested Vulkan buffer usage flags
 */
DVZ_EXPORT VkBufferUsageFlags dvz_buffer_usage_value(DvzBuffer* buffer);



/**
 * Resize a buffer.
 *
 * The requested logical size is updated only on a valid buffer. This helper does not preserve
 * existing contents, remap host pointers, or recreate a live Vulkan buffer; destroy and create the
 * buffer again after changing size. Shrinking follows the same recreate contract as growing.
 *
 * @param buffer the buffer
 * @param size the new buffer size, in bytes
 */
DVZ_EXPORT void dvz_buffer_resize(DvzBuffer* buffer, DvzSize size);



/**
 * Memmap a GPU buffer.
 *
 * @param buffer live host-visible buffer
 * @return 0 on success, non-zero on Vulkan or Datoviz state failure
 */
DVZ_EXPORT int dvz_buffer_map(DvzBuffer* buffer);



/**
 * Unmap a GPU buffer.
 *
 * @param buffer mapped host-visible buffer; no action is taken when it is not mapped
 */
DVZ_EXPORT void dvz_buffer_unmap(DvzBuffer* buffer);



/**
 * Upload data to a GPU buffer.
 *
 * !!! important
 *     This function does **not** use any GPU synchronization primitive: this is the responsibility
 *     of the caller.
 *
 * @param buffer live host-visible destination buffer
 * @param offset the offset within the buffer, in bytes
 * @param size number of bytes to copy
 * @param data source host-memory region containing at least `size` bytes
 */
DVZ_EXPORT void
dvz_buffer_upload(DvzBuffer* buffer, DvzSize offset, DvzSize size, const void* data);



/**
 * Download a buffer data to the CPU.
 *
 * !!! important
 *     This function does **not** use any GPU synchronization primitive: this is the responsibility
 *     of the caller.
 *
 * @param buffer live host-visible source buffer
 * @param offset the offset within the buffer, in bytes
 * @param size the size of the region to download, in bytes
 * @param[out] data destination host-memory region with capacity for at least `size` bytes
 */
DVZ_EXPORT void dvz_buffer_download(DvzBuffer* buffer, DvzSize offset, DvzSize size, void* data);



/**
 * Destroy a buffer.
 *
 * This releases the wrapped Vulkan buffer and returns the wrapper to a reusable
 * initialized state.
 *
 * @param buffer live buffer to destroy; the wrapper itself is retained
 */
DVZ_EXPORT void dvz_buffer_destroy(DvzBuffer* buffer);



/**
 * Allocate an empty buffer-views wrapper.
 *
 * @return allocated buffer-views wrapper, or NULL on allocation failure
 */
DVZ_EXPORT DvzBufferViews* dvz_buffer_views_create(void);



/**
 * Create buffer views on an existing GPU buffer.
 *
 * @param buffer live buffer borrowed by the views; it must outlive `views`
 * @param count the number of successive views
 * @param offset byte offset of the first view within `buffer`
 * @param size the size of each view, in bytes
 * @param alignment the alignment requirement for the view offsets
 * @param[out] views the created buffer views
 */
DVZ_EXPORT void dvz_buffer_views(
    DvzBuffer* buffer, uint32_t count, //
    DvzSize offset, DvzSize size, DvzSize alignment, DvzBufferViews* views);



/**
 * Return the number of logical views configured in a buffer-views wrapper.
 *
 * @param views the buffer views
 * @return the number of configured views
 */
DVZ_EXPORT uint32_t dvz_buffer_views_count(DvzBufferViews* views);



/**
 * Return the size in bytes of each configured logical view.
 *
 * @param views the buffer views
 * @return the logical view size in bytes
 */
DVZ_EXPORT DvzSize dvz_buffer_views_size(DvzBufferViews* views);



/**
 * Return the aligned stride in bytes between successive views.
 *
 * @param views the buffer views
 * @return the aligned stride in bytes, or 0 when no alignment was requested
 */
DVZ_EXPORT DvzSize dvz_buffer_views_aligned_size(DvzBufferViews* views);



/**
 * Return the byte offset of a configured view.
 *
 * @param views the buffer views
 * @param idx the logical view index
 * @return the byte offset of the selected view
 */
DVZ_EXPORT DvzSize dvz_buffer_views_offset(DvzBufferViews* views, uint32_t idx);



/**
 * Free a buffer-views wrapper allocated by dvz_buffer_views_create().
 *
 * @param views buffer-views wrapper to free
 */
DVZ_EXPORT void dvz_buffer_views_free(DvzBufferViews* views);



/**
 * Bind vertex buffers before recording draw commands.
 *
 * @param cmds the command buffers
 * @param first_binding the index of the first vertex binding
 * @param binding_count the number of bindings
 * @param buffers the "binding_count" buffers to bind
 * @param offsets array of `binding_count` byte offsets, one per buffer
 */
DVZ_EXPORT void dvz_cmd_bind_vertex_buffers(
    DvzCommands* cmds, uint32_t first_binding, uint32_t binding_count, DvzBuffer* buffers,
    DvzSize* offsets);



/**
 * Bind an index buffer.
 *
 * @param cmds the command buffers
 * @param buffer the index buffer
 * @param offset byte offset within the index buffer
 * @param index_type the Vulkan index type
 */
DVZ_EXPORT void
dvz_cmd_bind_index_buffer(DvzCommands* cmds, DvzBuffer* buffer, DvzSize offset, VkIndexType index_type);



EXTERN_C_OFF
