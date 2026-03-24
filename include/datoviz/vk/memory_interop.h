/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Memory interop                                                                              */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/vk/memory.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

EXTERN_C_ON



/*************************************************************************************************/
/*  External-memory interop API                                                                  */
/*************************************************************************************************/

/**
 * Return the external-handle type configured on an allocator.
 *
 * This helper belongs to the narrower external-memory interop surface rather than the stable
 * allocation path.
 *
 * @param allocator the allocator
 * @return external memory handle type flags (0 when disabled)
 */
DVZ_EXPORT VkExternalMemoryHandleTypeFlagsKHR dvz_allocator_external(DvzVma* allocator);



/**
 * Export an allocation for another GPU API.
 *
 * This function is part of the advanced external-memory interop surface. It is intended for
 * specialized paths such as video/interop plumbing rather than ordinary Datoviz allocation use.
 *
 * @param allocator the allocator
 * @param alloc the allocation
 * @param[out] handle the exported handle pointing to that allocation
 */
DVZ_EXPORT int dvz_allocator_export(DvzVma* allocator, DvzAllocation* alloc, int* handle);



/**
 * Return the Vulkan device-memory handle backing an allocation.
 *
 * This is an advanced interop helper for external-memory workflows. Regular Datoviz callers
 * should prefer the allocator/map/copy helpers in `memory.h` instead of depending on raw memory
 * handles.
 *
 * @param alloc the allocation
 * @returns Vulkan device memory handle
 */
DVZ_EXPORT VkDeviceMemory dvz_allocation_memory(DvzAllocation* alloc);



/**
 * Import an external GPU data pointer to a Vulkan buffer.
 *
 * This function belongs to the advanced external-memory interop surface, not the stable
 * low-level allocation path.
 *
 * !!! warning
 *     This function remains experimental on the current branch. The import-buffer test path is
 *     still documented as unreliable and should not be treated as a stable v0.4 contract yet.
 *
 * @param allocator the allocator
 * @param info the buffer creation
 * @param info the buffer creation info Vulkan struct
 * @param flags Datoviz allocation policy flags
 * @param handle the handle to import
 * @param[out] alloc the created allocation
 * @param[out] vk_buffer the created VkBuffer handle
 */
DVZ_EXPORT int dvz_allocator_import_buffer(
    DvzVma* allocator, VkBufferCreateInfo* info, DvzAllocationFlags flags, int handle,
    DvzAllocation* alloc, VkBuffer* vk_buffer);



/**
 * Import an external GPU data pointer to a Vulkan image.
 *
 * This function belongs to the advanced external-memory interop surface, not the stable
 * low-level allocation path.
 *
 * @param allocator the allocator
 * @param info the image creation
 * @param info the image creation info Vulkan struct
 * @param flags Datoviz allocation policy flags
 * @param handle the handle to import
 * @param[out] alloc the created allocation
 * @param[out] vk_image the created VkImage handle
 */
DVZ_EXPORT int dvz_allocator_import_image(
    DvzVma* allocator, VkImageCreateInfo* info, DvzAllocationFlags flags, int handle,
    DvzAllocation* alloc, VkImage* vk_image);



EXTERN_C_OFF
