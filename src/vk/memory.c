/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Memory                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "_alloc.h"
#include "datoviz/common/macros.h"
#include "datoviz/vk/gpu.h"
#include "vulkan/vulkan_core.h"
MUTE_ON
#include "datoviz/vk/memory.h"
MUTE_OFF
#include "macros.h"
#include "types.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static int _set_vma_flags(DvzGpu* gpu)
{
    ANN(gpu);

    uint32_t extension_count = 0;
    char** extensions = dvz_gpu_supported_extensions(gpu, &extension_count);

    int vma_flags = 0;
    if (dvz_gpu_has_extension(gpu, VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME))
        vma_flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;
    if (dvz_gpu_has_extension(gpu, "VK_KHR_bind_memory2"))
        vma_flags |= VMA_ALLOCATOR_CREATE_KHR_BIND_MEMORY2_BIT;
    if (dvz_gpu_has_extension(gpu, VK_KHR_MAINTENANCE_4_EXTENSION_NAME))
        vma_flags |= VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE4_BIT;
    if (dvz_gpu_has_extension(gpu, VK_KHR_MAINTENANCE_5_EXTENSION_NAME))
        vma_flags |= VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE5_BIT;
    if (dvz_gpu_has_extension(gpu, VK_EXT_MEMORY_BUDGET_EXTENSION_NAME))
        vma_flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
    if (dvz_gpu_has_extension(gpu, VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME))
        vma_flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    if (dvz_gpu_has_extension(gpu, VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME))
        vma_flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT;
    if (dvz_gpu_has_extension(gpu, VK_AMD_DEVICE_COHERENT_MEMORY_EXTENSION_NAME))
        vma_flags |= VMA_ALLOCATOR_CREATE_AMD_DEVICE_COHERENT_MEMORY_BIT;
    if (dvz_gpu_has_extension(gpu, "VK_KHR_external_memory_win32"))
        vma_flags |= VMA_ALLOCATOR_CREATE_KHR_EXTERNAL_MEMORY_WIN32_BIT;

    return vma_flags;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

int dvz_device_allocator(DvzDevice* device, DvzVma* allocator)
{
    ANN(device);
    ANN(allocator);

    DvzGpu* gpu = device->gpu;
    ANN(gpu);

    allocator->device = device;

    int vma_flags = _set_vma_flags(gpu);

    VmaAllocatorCreateInfo allocatorCreateInfo = {0};
    allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
    allocatorCreateInfo.vulkanApiVersion = gpu->instance->vk_version;
    allocatorCreateInfo.physicalDevice = gpu->pdevice;
    allocatorCreateInfo.device = device->vk_device;
    allocatorCreateInfo.instance = gpu->instance->vk_instance;
    // allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

    log_trace("creating allocator...");
    VK_RETURN_RESULT(vmaCreateAllocator(&allocatorCreateInfo, &allocator->vma));
    log_trace("allocator created");

    return out;
}



int dvz_allocator_buffer(
    DvzVma* allocator, VkBufferCreateInfo* info, VmaAllocationCreateFlags flags,
    DvzAllocation* alloc, VkBuffer* vk_buffer)
{
    ANN(allocator);
    ANN(info);
    ANN(alloc);
    ANN(vk_buffer);

    VmaAllocationCreateInfo alloc_info = {0};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
    alloc_info.flags = flags;

    log_trace("creating buffer...");
    VK_RETURN_RESULT(vmaCreateBuffer(
        allocator->vma, info, &alloc_info, vk_buffer, &alloc->alloc, &alloc->info));
    log_trace("buffer created");

    // Get the memory flags found by VMA and store them in the DvzBuffer instance.
    vmaGetMemoryTypeProperties(allocator->vma, alloc->info.memoryType, &alloc->memory_flags);

    // Store the alignment requirement in the DvzBuffer.
    VkMemoryRequirements req = {0};
    vkGetBufferMemoryRequirements(allocator->device->vk_device, *vk_buffer, &req);
    alloc->alignment = req.alignment;

    return out;
}



int dvz_allocator_image(
    DvzVma* allocator, VkImageCreateInfo* info, VmaAllocationCreateFlags flags,
    DvzAllocation* alloc, VkImage* vk_image)
{
    ANN(allocator);
    ANN(info);
    ANN(alloc);
    ANN(vk_image);

    VmaAllocationCreateInfo alloc_info = {0};
    alloc_info.usage = alloc->usage = VMA_MEMORY_USAGE_AUTO;
    alloc_info.flags = alloc->flags = flags;

    VK_RETURN_RESULT(
        vmaCreateImage(allocator->vma, info, &alloc_info, vk_image, &alloc->alloc, &alloc->info));

    return out;
}



int dvz_allocator_export(DvzVma* allocator, DvzAllocation* alloc, int* handle)
{
    ANN(allocator);
    ANN(alloc);
    ANN(handle);

    return 0;
}



int dvz_allocator_import_buffer(
    DvzVma* allocator, VkBufferCreateInfo* info, VmaAllocationCreateFlags flags, int handle,
    DvzAllocation* alloc, VkBuffer* vk_buffer)
{
    ANN(allocator);
    ANN(info);
    ANN(alloc);
    ANN(vk_buffer);

    return 0;
}



int dvz_allocator_import_image(
    DvzVma* allocator, VkImageCreateInfo* info, VmaAllocationCreateFlags flags, int handle,
    DvzAllocation* alloc, VkImage* vk_image)
{
    ANN(allocator);
    ANN(info);
    ANN(alloc);
    ANN(vk_image);

    return 0;
}



void dvz_allocator_buffer_destroy(DvzVma* allocator, DvzAllocation* alloc, VkBuffer vk_buffer)
{
    vmaDestroyBuffer(allocator->vma, vk_buffer, alloc->alloc);
}



void dvz_allocator_destroy(DvzVma* allocator)
{
    ANN(allocator);
    if (allocator->vma != NULL)
    {
        log_trace("destroyed allocator...");
        vmaDestroyAllocator(allocator->vma);
        allocator->vma = NULL;
        log_trace("allocator destroyed");
    }
}
