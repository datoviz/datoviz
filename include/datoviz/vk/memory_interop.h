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

#include <stdbool.h>

#include "datoviz/vk/memory.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzBuffer DvzBuffer;
typedef struct DvzDevice DvzDevice;
typedef struct DvzGpuCtx DvzGpuCtx;
typedef struct DvzSemaphore DvzSemaphore;

typedef struct DvzInteropBufferExport DvzInteropBufferExport;
typedef struct DvzInteropBufferExportConfig DvzInteropBufferExportConfig;



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_INTEROP_BUFFER_EXPORT_VERSION 1



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzInteropBufferExport
{
    uint32_t version;
    int memory_handle;
    uint32_t memory_handle_type;
    uint64_t allocation_size;
    uint64_t offset;
    uint64_t size;
    uint32_t usage;
    uint32_t vk_usage;
    uint32_t drp2_usage;
    uint32_t flags;
    uint32_t device_uuid_valid;
    uint8_t device_uuid[VK_UUID_SIZE];
    int semaphore_handle;
    uint32_t semaphore_handle_type;
    uint64_t semaphore_value;
};



struct DvzInteropBufferExportConfig
{
    uint32_t struct_size;
    uint32_t flags;
    uint64_t offset;
    uint64_t size;
    uint32_t drp2_usage;
    uint32_t export_flags;
    DvzSemaphore* semaphore;
    uint32_t semaphore_handle_type;
    uint64_t semaphore_value;
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

EXTERN_C_ON



/*************************************************************************************************/
/*  External-memory interop API                                                                  */
/*************************************************************************************************/

/**
 * Return a default interop buffer export configuration.
 *
 * @return default export configuration
 */
DVZ_EXPORT DvzInteropBufferExportConfig dvz_interop_buffer_export_config(void);


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
 * Create an advanced GPU context for Vulkan-owned CUDA/CuPy interop buffers.
 *
 * This helper is binding substrate, not the final high-level Python API. It creates a Datoviz GPU
 * context whose allocator exports Vulkan memory with `memory_handle_type`, and whose device has the
 * external-memory, external-semaphore, and timeline-semaphore support needed by the Linux/NVIDIA
 * CuPy smoke. Destroy the returned context with dvz_gpu_ctx_destroy().
 *
 * @param gpu_index Vulkan physical-device index to use
 * @param memory_handle_type external memory handle type for exported allocations
 * @return owned GPU context, or NULL on failure
 */
DVZ_EXPORT DvzGpuCtx* dvz_interop_gpu_ctx(
    uint32_t gpu_index, VkExternalMemoryHandleTypeFlagsKHR memory_handle_type);


/**
 * Create an advanced GPU context for CUDA/CuPy interop that also supports presentation.
 *
 * This is the configurable variant of dvz_interop_gpu_ctx(). It keeps the same external-memory
 * allocator policy and timeline-semaphore setup, then optionally adds caller-provided Vulkan
 * instance extensions and canvas/swapchain device extensions. Use it when a borrowed interop GPU
 * context must also back a visible GLFW or hosted-surface canvas.
 *
 * @param gpu_index Vulkan physical-device index to use
 * @param memory_handle_type external memory handle type for exported allocations
 * @param instance_extension_count number of Vulkan instance extension names
 * @param instance_extensions extension-name array, or NULL when count is zero
 * @param enable_canvas_extensions whether swapchain/surface device extensions should be requested
 * @return owned GPU context, or NULL on failure
 */
DVZ_EXPORT DvzGpuCtx* dvz_interop_gpu_ctx_ex(
    uint32_t gpu_index, VkExternalMemoryHandleTypeFlagsKHR memory_handle_type,
    uint32_t instance_extension_count, const char* const* instance_extensions,
    bool enable_canvas_extensions);



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
 * Export a Vulkan-owned buffer allocation and package external interop metadata.
 *
 * This helper transfers ownership of `out->memory_handle` to the caller on success. The optional
 * `semaphore_handle` is copied into the descriptor as metadata; ownership of that handle remains
 * defined by the call site that exported it.
 *
 * @param allocator the allocator configured for external-memory export
 * @param alloc the Vulkan-owned allocation backing the buffer
 * @param offset byte offset of the logical buffer view within the allocation
 * @param size logical buffer-view size in bytes
 * @param usage DRP2/Vulkan buffer usage flags expected by the consumer
 * @param semaphore_handle optional exported external semaphore handle, or -1 when absent
 * @param semaphore_handle_type external semaphore handle type, or 0 when absent
 * @param semaphore_value timeline semaphore value associated with the export
 * @param[out] out export descriptor
 * @return 0 on success, -1 on failure
 */
DVZ_EXPORT int dvz_interop_buffer_export(
    DvzVma* allocator, DvzAllocation* alloc, uint64_t offset, uint64_t size, uint32_t usage,
    int semaphore_handle, uint32_t semaphore_handle_type, uint64_t semaphore_value,
    DvzInteropBufferExport* out);



/**
 * Export a vklite buffer and package external interop metadata.
 *
 * This is the preferred public helper for CUDA/CuPy import of Datoviz/Vulkan-owned buffers. It
 * avoids exposing private `DvzBuffer` allocation internals to examples and bindings. On success,
 * ownership of `out->memory_handle` transfers to the caller. If `config->semaphore` is non-NULL,
 * this function also exports a semaphore handle and transfers `out->semaphore_handle` ownership to
 * the caller.
 *
 * @param buffer the live Vulkan-owned buffer
 * @param config logical export range and optional timeline semaphore metadata
 * @param[out] out export descriptor
 * @return 0 on success, -1 on failure
 */
DVZ_EXPORT int dvz_interop_buffer_export_from_buffer(
    DvzBuffer* buffer, const DvzInteropBufferExportConfig* config,
    DvzInteropBufferExport* out);



/**
 * Wait on a timeline semaphore before Vulkan reads an interop buffer as vertex input.
 *
 * This helper is the explicit Vulkan-side synchronization point for CUDA/CuPy writes into a
 * Vulkan-owned exported buffer. It submits a short barrier command on the main queue that waits for
 * `semaphore` to reach `value`, then makes external writes visible to vertex-attribute reads.
 *
 * @param device logical device owning the main Vulkan queue
 * @param buffer buffer whose contents were written externally
 * @param size byte size of the synchronized buffer range
 * @param semaphore timeline semaphore signaled by the external API
 * @param value timeline value to wait on
 * @return true on success
 */
DVZ_EXPORT bool dvz_interop_buffer_wait_timeline(
    DvzDevice* device, DvzBuffer* buffer, uint64_t size, DvzSemaphore* semaphore, uint64_t value);



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
