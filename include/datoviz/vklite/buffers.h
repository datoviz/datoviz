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
#include "vk_mem_alloc.h"
#include <volk.h>



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
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzBufferViews
{
    DvzBuffer* buffer;
    uint32_t count;
    DvzSize size;
    DvzSize aligned_size; // NOTE: is non-null only for aligned arrays
    DvzSize alignment;
    DvzSize offsets[DVZ_MAX_BUFFER_VIEWS];
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

EXTERN_C_ON



/**
 * Initialize a GPU buffer.
 *
 * @param device the device
 * @param allocator the VMA allocator
 * @param[out] buffer the initialized buffer
 */
DVZ_EXPORT void dvz_buffer(DvzDevice* device, DvzVma* allocator, DvzBuffer* buffer);



/**
 * Set the buffer size.
 *
 * @param buffer the buffer
 * @param size the buffer size, in bytes
 */
DVZ_EXPORT void dvz_buffer_size(DvzBuffer* buffer, DvzSize size);



/**
 * Set the buffer usage.
 *
 * @param buffer the buffer
 * @param usage the buffer usage
 */
DVZ_EXPORT void dvz_buffer_usage(DvzBuffer* buffer, VkBufferUsageFlags usage);



/**
 * Set the VMA creation flags.
 *
 * @param buffer the buffer
 * @param flags the flags
 */
DVZ_EXPORT void dvz_buffer_flags(DvzBuffer* buffer, VmaAllocationCreateFlags flags);



/**
 * Create the buffer after it has been set.
 *
 * @param buffer the buffer
 * @returns the Vulkan creation result code
 */
DVZ_EXPORT int dvz_buffer_create(DvzBuffer* buffer);



/**
 * Return a Vulkan handle to a buffer.
 *
 * @param buffer the buffer
 * @returns the Vulkan buffer handle
 */
DVZ_EXPORT VkBuffer dvz_buffer_handle(DvzBuffer* buffer);



/**
 * Resize a buffer.
 *
 * @param buffer the buffer
 * @param size the new buffer size, in bytes
 */
DVZ_EXPORT void dvz_buffer_resize(DvzBuffer* buffer, DvzSize size);



/**
 * Memmap a GPU buffer.
 *
 * @param buffer
 * @returns the result code
 */
DVZ_EXPORT int dvz_buffer_map(DvzBuffer* buffer);



/**
 * Unmap a GPU buffer.
 *
 * @param buffer
 */
DVZ_EXPORT void dvz_buffer_unmap(DvzBuffer* buffer);



/**
 * Upload data to a GPU buffer.
 *
 * !!! important
 *     This function does **not** use any GPU synchronization primitive: this is the responsibility
 *     of the caller.
 *
 * @param buffer the buffer
 * @param offset the offset within the buffer, in bytes
 * @param size the buffer size, in bytes
 * @param data the data to upload
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
 * @param buffer the buffer
 * @param offset the offset within the buffer, in bytes
 * @param size the size of the region to download, in bytes
 * @param[out] data (array) the buffer to download on (must be allocated with the appropriate size)
 */
DVZ_EXPORT void dvz_buffer_download(DvzBuffer* buffer, DvzSize offset, DvzSize size, void* data);



/**
 * Destroy a buffer
 *
 * @param buffer the buffer
 */
DVZ_EXPORT void dvz_buffer_destroy(DvzBuffer* buffer);



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
DVZ_EXPORT void dvz_buffer_views(
    DvzBuffer* buffer, uint32_t count, //
    DvzSize offset, DvzSize size, DvzSize alignment, DvzBufferViews* views);



/**
 * Bind vertex buffers before recording draw commands.
 *
 * @param cmds the command buffers
 * @param idx the command buffer index
 * @param first_binding the index of the first vertex binding
 * @param binding_count the number of bindings
 * @param buffers the "binding_count" buffers to bind
 * @param offsets the offsets within each buffer
 */
DVZ_EXPORT void dvz_cmd_bind_vertex_buffers(
    DvzCommands* cmds, uint32_t idx, uint32_t first_binding, uint32_t binding_count,
    DvzBuffer* buffers, DvzSize* offsets);



/**
 * Bind an index buffer.
 *
 * @param cmds the command buffers
 * @param idx the command buffer index
 * @param buffer the index buffer
 * @param offset the offset within the index buffer
 */
DVZ_EXPORT void
dvz_cmd_bind_index_buffer(DvzCommands* cmds, uint32_t idx, DvzBuffer* buffer, DvzSize offset);



EXTERN_C_OFF
