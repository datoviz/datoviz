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
#include <vulkan/vulkan.h>

#include "vk_mem_alloc.h"

#include "datoviz/common/macros.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzDevice DvzDevice;
typedef struct DvzVma DvzVma;
typedef struct DvzAllocation DvzAllocation;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

EXTERN_C_ON



/**
 * Create an allocator.
 *
 * @param device the device
 * @param[out] allocator the allocator
 */
DVZ_EXPORT int dvz_device_allocator(DvzDevice* device, DvzVma* allocator);



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
 * Destroy an allocator.
 *
 * @param allocator the allocator.
 */
DVZ_EXPORT void dvz_allocator_destroy(DvzVma* allocator);



EXTERN_C_OFF
