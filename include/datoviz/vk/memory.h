/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Memory                                                                                      */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "datoviz/common/macros.h"
#include "datoviz/math/types.h"

MUTE_ON
#include "vk_mem_alloc.h"
MUTE_OFF



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzDevice DvzDevice;
typedef struct DvzVma DvzVma;
typedef struct DvzAllocation DvzAllocation;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

EXTERN_C_ON



/**
 * Allocate an empty allocator wrapper.
 *
 * @returns allocated allocator wrapper, or NULL on allocation failure
 */
DVZ_EXPORT DvzVma* dvz_allocator_create(void);



/**
 * Free an allocator wrapper allocated by dvz_allocator_create().
 *
 * @param allocator allocator wrapper to free
 */
DVZ_EXPORT void dvz_allocator_free(DvzVma* allocator);



/**
 * Allocate an empty allocation wrapper.
 *
 * @returns allocated allocation wrapper, or NULL on allocation failure
 */
DVZ_EXPORT DvzAllocation* dvz_allocation_create(void);



/**
 * Free an allocation wrapper allocated by dvz_allocation_create().
 *
 * @param alloc allocation wrapper to free
 */
DVZ_EXPORT void dvz_allocation_free(DvzAllocation* alloc);



/**
 * Return the device associated with an allocator.
 *
 * @param allocator the allocator
 * @returns associated device, or NULL if unset
 */
DVZ_EXPORT DvzDevice* dvz_allocator_device(DvzVma* allocator);



/**
 * Return the external-handle type configured on an allocator.
 *
 * @param allocator the allocator
 * @return external memory handle type flags (0 when disabled)
 */
DVZ_EXPORT VkExternalMemoryHandleTypeFlagsKHR dvz_allocator_external(DvzVma* allocator);



/**
 * Return the mapped pointer currently associated with an allocation.
 *
 * @param alloc the allocation
 * @returns mapped pointer or NULL
 */
DVZ_EXPORT void* dvz_allocation_mapped(DvzAllocation* alloc);



/**
 * Return the allocation-create flags currently associated with an allocation.
 *
 * @param alloc the allocation
 * @return allocation-create flags
 */
DVZ_EXPORT VmaAllocationCreateFlags dvz_allocation_flags(DvzAllocation* alloc);



/**
 * Update the allocation-create flags used by higher-level wrappers.
 *
 * @param alloc the allocation
 * @param flags allocation-create flags
 */
DVZ_EXPORT void dvz_allocation_set_flags(DvzAllocation* alloc, VmaAllocationCreateFlags flags);



/**
 * Return the Vulkan device memory handle of an allocation.
 *
 * @param alloc the allocation
 * @returns Vulkan device memory handle
 */
DVZ_EXPORT VkDeviceMemory dvz_allocation_memory(DvzAllocation* alloc);



/**
 * Return the allocation size, in bytes.
 *
 * @param alloc the allocation
 * @returns allocation size in bytes
 */
DVZ_EXPORT VkDeviceSize dvz_allocation_size(DvzAllocation* alloc);



/**
 * Create an allocator.
 *
 * @param device the device
 * @param export if exporting created allocations, the external memory handle type
 * @param[out] allocator the allocator
 */
DVZ_EXPORT int dvz_device_allocator(
    DvzDevice* device, VkExternalMemoryHandleTypeFlagsKHR export, DvzVma* allocator);



/**
 * Allocate and create a Vulkan buffer.
 *
 * @param allocator the allocator
 * @param info the buffer creation info Vulkan struct
 * @param flags the VMA allocation creation flags
 * @param[out] alloc the created allocation
 * @param[out] vk_buffer the created VkBuffer handle
 */
DVZ_EXPORT int dvz_allocator_buffer(
    DvzVma* allocator, VkBufferCreateInfo* info, VmaAllocationCreateFlags flags,
    DvzAllocation* alloc, VkBuffer* vk_buffer);



/**
 * Allocate and create a Vulkan image.
 *
 * @param allocator the allocator
 * @param info the image creation info Vulkan struct
 * @param flags the VMA allocation creation flags
 * @param[out] alloc the created allocation
 * @param[out] vk_image the created VkImage handle
 */
DVZ_EXPORT int dvz_allocator_image(
    DvzVma* allocator, VkImageCreateInfo* info, VmaAllocationCreateFlags flags,
    DvzAllocation* alloc, VkImage* vk_image);


/**
 * Map an allocation.
 *
 * @param allocator the allocator
 * @param alloc the allocation
 * @returns the mapped pointer
 */
DVZ_EXPORT void* dvz_allocator_map(DvzVma* allocator, DvzAllocation* alloc);



/**
 * Unmap the allocation.
 *
 * @param allocator the allocator
 * @param alloc the allocation
 */
DVZ_EXPORT void dvz_allocator_unmap(DvzVma* allocator, DvzAllocation* alloc);



/**
 * Flush mapped memory ranges so GPU sees the latest CPU writes.
 *
 * @param allocator the allocator
 * @param alloc the allocation
 * @param offset the byte offset within the allocation
 * @param size the number of bytes to flush
 * @returns 0 on success, -1 on failure
 */
DVZ_EXPORT int dvz_allocator_flush(
    DvzVma* allocator, DvzAllocation* alloc, VkDeviceSize offset, VkDeviceSize size);


/**
 * Invalidate mapped memory ranges so the CPU sees the latest GPU writes.
 *
 * @param allocator the allocator
 * @param alloc the allocation
 * @param offset the byte offset within the allocation
 * @param size the number of bytes to invalidate
 * @returns 0 on success, -1 on failure
 */
DVZ_EXPORT int dvz_allocator_invalidate(
    DvzVma* allocator, DvzAllocation* alloc, VkDeviceSize offset, VkDeviceSize size);


/**
 * Copy host memory into an allocation.
 *
 * @param allocator the allocator
 * @param alloc the destination allocation
 * @param offset destination byte offset within the allocation
 * @param data source host pointer
 * @param size number of bytes to copy
 * @return 0 on success, -1 on failure
 */
DVZ_EXPORT int dvz_allocator_copy_to(
    DvzVma* allocator, DvzAllocation* alloc, VkDeviceSize offset, const void* data,
    VkDeviceSize size);



/**
 * Copy memory from an allocation into host memory.
 *
 * @param allocator the allocator
 * @param alloc the source allocation
 * @param offset source byte offset within the allocation
 * @param data destination host pointer
 * @param size number of bytes to copy
 * @return 0 on success, -1 on failure
 */
DVZ_EXPORT int dvz_allocator_copy_from(
    DvzVma* allocator, DvzAllocation* alloc, VkDeviceSize offset, void* data, VkDeviceSize size);



/**
 * Export an allocation for another GPU API.
 *
 * @param allocator the allocator
 * @param alloc the allocation
 * @param[out] handle the exported handle pointing to that allocation
 */
DVZ_EXPORT int dvz_allocator_export(DvzVma* allocator, DvzAllocation* alloc, int* handle);



/**
 * Import an external GPU data pointer to a Vulkan buffer.
 *
 * !!! warning
 *     This function does NOT appear to work for now. test_memory_cuda_2() test fails.
 *
 * @param allocator the allocator
 * @param info the buffer creation
 * @param info the buffer creation info Vulkan struct
 * @param flags the VMA allocation creation flags
 * @param handle the handle to import
 * @param[out] alloc the created allocation
 * @param[out] vk_buffer the created VkBuffer handle
 */
DVZ_EXPORT int dvz_allocator_import_buffer(
    DvzVma* allocator, VkBufferCreateInfo* info, VmaAllocationCreateFlags flags, int handle,
    DvzAllocation* alloc, VkBuffer* vk_buffer);



/**
 * Import an external GPU data pointer to a Vulkan image.
 *
 * @param allocator the allocator
 * @param info the image creation
 * @param info the image creation info Vulkan struct
 * @param flags the VMA allocation creation flags
 * @param handle the handle to import
 * @param[out] alloc the created allocation
 * @param[out] vk_image the created VkImage handle
 */
DVZ_EXPORT int dvz_allocator_import_image(
    DvzVma* allocator, VkImageCreateInfo* info, VmaAllocationCreateFlags flags, int handle,
    DvzAllocation* alloc, VkImage* vk_image);



/**
 * Destroy a buffer allocation.
 *
 * @param allocator the allocator
 * @param alloc the allocation
 * @param vk_buffer the buffer
 */
DVZ_EXPORT void
dvz_allocator_destroy_buffer(DvzVma* allocator, DvzAllocation* alloc, VkBuffer vk_buffer);



/**
 * Destroy a image allocation.
 *
 * @param allocator the allocator
 * @param alloc the allocation
 * @param vk_image the image
 */
DVZ_EXPORT void
dvz_allocator_destroy_image(DvzVma* allocator, DvzAllocation* alloc, VkImage vk_image);



/**
 * Destroy an allocator.
 *
 * @param allocator the allocator.
 */
DVZ_EXPORT void dvz_allocator_destroy(DvzVma* allocator);



EXTERN_C_OFF
