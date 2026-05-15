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
 * Advanced allocator interop surface.
 *
 * This header is an intentional low-level escape hatch for external-memory workflows.
 * It is narrower than `memory.h` and should be included only by code that explicitly needs
 * raw memory handles, export/import plumbing, or external-handle inspection.
 *
 * Interop direction matters. The current CUDA/CuPy-owned GPU pointer -> Vulkan import path has
 * proved unreliable on the active branch and should not be the primary design target. Prioritize
 * Vulkan-owned allocations exported through external memory handles and then imported by CUDA/CuPy,
 * using explicit external synchronization around cross-API access.
 *
 * NVIDIA CIG (`VK_NV_external_compute_queue` / CUDA-in-Graphics contexts) is not required for that
 * Vulkan -> CUDA/CuPy memory-sharing route. Treat it as an optional NVIDIA-only scheduling
 * experiment rather than a dependency of the external-memory design.
 *
 * The declarations below remain public on purpose even when some of them are lightly used on the
 * current branch, because allocator interop hardening is expected to continue here rather than
 * through private vk-only helpers.
 */

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
 * This declaration remains intentionally public even though the active branch currently exercises
 * import paths only narrowly; future allocator-interop hardening is expected to build on this
 * surface rather than reintroducing private entry points.
 *
 * !!! warning
 *     This CUDA/CuPy-owned pointer -> Vulkan import direction remains experimental on the current
 *     branch. It is unreliable in practice and should not be treated as a stable v0.4 contract.
 *     Prefer the opposite direction: create/export Vulkan memory first, then import the external
 *     memory handle into CUDA/CuPy.
 *
 * @param allocator the allocator
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
 * This declaration remains intentionally public even though the active branch does not yet rely on
 * image import broadly; future allocator-interop hardening is expected to build on this surface
 * rather than reintroducing private entry points.
 *
 * @param allocator the allocator
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
