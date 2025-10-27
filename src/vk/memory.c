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
#include "datoviz/vk/device.h"
#include "datoviz/vk/gpu.h"
#include "datoviz/vk/memory.h"
#include "vulkan/vulkan_core.h"
MUTE_ON
#include "vk_mem_alloc.h"
MUTE_OFF
#include "macros.h"
#include "types.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static VmaAllocatorCreateFlagBits _set_vma_flags(DvzDevice* device)
{
    ANN(device);

    VmaAllocatorCreateFlagBits vma_flags = 0;
    if (dvz_device_has_extension(device, VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME))
        vma_flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;
    if (dvz_device_has_extension(device, "VK_KHR_bind_memory2"))
        vma_flags |= VMA_ALLOCATOR_CREATE_KHR_BIND_MEMORY2_BIT;
    if (dvz_device_has_extension(device, VK_KHR_MAINTENANCE_4_EXTENSION_NAME))
        vma_flags |= VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE4_BIT;
    if (dvz_device_has_extension(device, VK_KHR_MAINTENANCE_5_EXTENSION_NAME))
        vma_flags |= VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE5_BIT;
    if (dvz_device_has_extension(device, VK_EXT_MEMORY_BUDGET_EXTENSION_NAME))
        vma_flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
    if (dvz_device_has_extension(device, VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME))
        vma_flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    if (dvz_device_has_extension(device, VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME))
        vma_flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT;
    if (dvz_device_has_extension(device, VK_AMD_DEVICE_COHERENT_MEMORY_EXTENSION_NAME))
        vma_flags |= VMA_ALLOCATOR_CREATE_AMD_DEVICE_COHERENT_MEMORY_BIT;
    if (dvz_device_has_extension(device, "VK_KHR_external_memory_win32"))
        vma_flags |= VMA_ALLOCATOR_CREATE_KHR_EXTERNAL_MEMORY_WIN32_BIT;

    return vma_flags;
}



#define ENSURE_EXTERNAL                                                                           \
    if (allocator->external == 0)                                                                 \
    {                                                                                             \
        log_warn("unable to use external feature as the external flag was not set at allocator "  \
                 "creation");                                                                     \
        return -1;                                                                                \
    }



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

int dvz_device_allocator(
    DvzDevice* device, VkExternalMemoryHandleTypeFlagsKHR external, DvzVma* allocator)
{
    ANN(device);
    ANN(allocator);

    DvzGpu* gpu = device->gpu;
    ANN(gpu);

    allocator->device = device;
    allocator->external = external;

    VmaAllocatorCreateFlagBits vma_flags = _set_vma_flags(device);

    VmaAllocatorCreateInfo info = {0};
    info.flags = vma_flags | VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
    info.vulkanApiVersion = gpu->instance->vk_version;
    info.physicalDevice = gpu->pdevice;
    info.device = device->vk_device;
    info.instance = gpu->instance->vk_instance;
    // info.pVulkanFunctions = &vulkanFunctions;

    // If the external is set, set it to all memory types, to be used to all allocations.
    VkExternalMemoryHandleTypeFlagsKHR types[VK_MAX_MEMORY_TYPES] = {0};
    if (external != 0)
    {
        for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++)
        {
            types[i] = external;
        }
        info.pTypeExternalMemoryHandleTypes = types;
    }

    log_trace("creating allocator...");
    VK_RETURN_RESULT(vmaCreateAllocator(&info, &allocator->vma));
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

    // External memory.
    VkExternalMemoryBufferCreateInfo external_info = {0};
    if (allocator->external != 0)
    {
        external_info.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO;
        external_info.handleTypes = allocator->external;
        info->pNext = &external_info;
    }

    // TODO: queue families

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



void* dvz_allocator_map(DvzVma* allocator, DvzAllocation* alloc)
{
    ANN(allocator);
    ANN(alloc);
    uint32_t* ptr = NULL;
    vmaMapMemory(allocator->vma, alloc->alloc, (void**)&ptr);
    ANN(ptr);
    return ptr;
}



void dvz_allocator_unmap(DvzVma* allocator, DvzAllocation* alloc)
{
    ANN(allocator);
    ANN(alloc);
    vmaUnmapMemory(allocator->vma, alloc->alloc);
}



/*************************************************************************************************/
/*  External                                                                                     */
/*************************************************************************************************/

int dvz_allocator_export(DvzVma* allocator, DvzAllocation* alloc, int* handle)
{
    ANN(allocator);
    ANN(allocator->device);
    ANN(allocator->device->gpu);
    ANN(allocator->device->gpu->instance);
    ANN(alloc);
    ANN(handle);

    // NOTE: need device extension: VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT
    ENSURE_EXTERNAL

    VkDevice vkd = allocator->device->vk_device;
    ANNVK(vkd);

#if OS_MACOS || OS_LINUX
    if (!dvz_device_has_extension(allocator->device, VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME))
    {
        log_error("VK_KHR_external_memory_fd extension not enabled on device; cannot export "
                  "memory FD");
        return -1;
    }

    VkMemoryGetFdInfoKHR info = {.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR};
    info.memory = alloc->info.deviceMemory;
    ANNVK(info.memory);
    info.handleType = allocator->external;
    if (info.handleType == 0)
    {
        log_error(
            "the allocator must have been created with a VkExternalMemoryHandleTypeFlagsKHR flag");
        return -1;
    }

    LOAD_VK_DEVICE_FUNC(vkd, vkGetMemoryFdKHR);
    VK_RETURN_RESULT(vkGetMemoryFdKHR_d(vkd, &info, handle));

#elif OS_WINDOWS
    // TODO
    log_error("Windows external support not yet implemented");
    int out = 0;

#else
    int out = -1;
#endif

    return out;
}



int dvz_allocator_import_buffer(
    DvzVma* allocator, VkBufferCreateInfo* info, VmaAllocationCreateFlags flags, int handle,
    DvzAllocation* alloc, VkBuffer* vk_buffer)
{
    ANN(allocator);
    ANN(info);
    ANN(alloc);
    ANN(vk_buffer);

    if (handle == 0)
    {
        log_error("handle cannot be 0, aborting external buffer import");
        return 1;
    }

    ENSURE_EXTERNAL

    // Set the external info structure to the VkBufferCreateInfo struct.
    VkExternalMemoryBufferCreateInfoKHR external_info = {
        .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO_KHR};
    external_info.handleTypes = allocator->external;
    if (info->pNext != NULL)
    {
        log_error("info.pNext must be NULL, otherwise need to iterate through the next chain and "
                  "set external_info to the last one. PR welcome");
    }
    else
    {
        info->pNext = &external_info;
    }

    // VMA allocation create info.
    VmaAllocationCreateInfo alloc_info = {0};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
    alloc_info.flags = flags;

#if OS_MACOS || OS_LINUX
    VkImportMemoryFdInfoKHR import_info = {.sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR};
    import_info.handleType = allocator->external;
    import_info.fd = handle;
#elif OS_WINDOWS
    // TODO
    log_error("Windows external support not yet implemented");
    int out = 0;

#else
    int out = -1;
#endif

    log_trace("creating buffer...");
    VK_RETURN_RESULT(vmaCreateDedicatedBuffer(
        allocator->vma, info, &alloc_info, &import_info, vk_buffer, &alloc->alloc, &alloc->info));
    log_trace("buffer created");

    // Get the memory flags found by VMA and store them in the DvzBuffer instance.
    vmaGetMemoryTypeProperties(allocator->vma, alloc->info.memoryType, &alloc->memory_flags);

    // Store the alignment requirement in the DvzBuffer.
    VkMemoryRequirements req = {0};
    vkGetBufferMemoryRequirements(allocator->device->vk_device, *vk_buffer, &req);
    alloc->alignment = req.alignment;

    return out;
}



int dvz_allocator_import_image(
    DvzVma* allocator, VkImageCreateInfo* info, VmaAllocationCreateFlags flags, int handle,
    DvzAllocation* alloc, VkImage* vk_image)
{
    ANN(allocator);
    ANN(info);
    ANN(alloc);
    ANN(vk_image);

    ENSURE_EXTERNAL

    // Set the external info structure to the VkImageCreateInfo struct.
    VkExternalMemoryImageCreateInfoKHR external_info = {
        .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO_KHR};
    external_info.handleTypes = allocator->external;
    if (info->pNext != NULL)
    {
        log_error("info.pNext must be NULL, otherwise need to iterate through the next chain and "
                  "set external_info to the last one. PR welcome");
    }
    else
    {
        info->pNext = &external_info;
    }

    // VMA allocation create info.
    VmaAllocationCreateInfo alloc_info = {0};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
    alloc_info.flags = flags;

#if OS_MACOS || OS_LINUX
    VkImportMemoryFdInfoKHR import_info = {.sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR};
    import_info.handleType = allocator->external;
    import_info.fd = handle;
#elif OS_WINDOWS
    // TODO
    log_error("Windows external support not yet implemented");
    int out = 0;

#else
    int out = -1;
#endif

    log_trace("creating image...");
    VK_RETURN_RESULT(vmaCreateDedicatedImage(
        allocator->vma, info, &alloc_info, &import_info, vk_image, &alloc->alloc, &alloc->info));
    log_trace("image created");

    // Get the memory flags found by VMA and store them in the DvzImage instance.
    vmaGetMemoryTypeProperties(allocator->vma, alloc->info.memoryType, &alloc->memory_flags);

    // Store the alignment requirement in the DvzImage.
    VkMemoryRequirements req = {0};
    vkGetImageMemoryRequirements(allocator->device->vk_device, *vk_image, &req);
    alloc->alignment = req.alignment;

    return out;
}



/*************************************************************************************************/
/*  Destruction                                                                                  */
/*************************************************************************************************/

void dvz_allocator_destroy_buffer(DvzVma* allocator, DvzAllocation* alloc, VkBuffer vk_buffer)
{
    ANN(allocator);
    ANN(alloc);
    if (vk_buffer != VK_NULL_HANDLE)
        vmaDestroyBuffer(allocator->vma, vk_buffer, alloc->alloc);
}



void dvz_allocator_destroy_image(DvzVma* allocator, DvzAllocation* alloc, VkImage vk_image)
{
    ANN(allocator);
    ANN(alloc);
    if (vk_image != VK_NULL_HANDLE)
        vmaDestroyImage(allocator->vma, vk_image, alloc->alloc);
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
