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

#include <stddef.h>
#include <stdint.h>
#include <volk.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_device.h"
#include "_instance.h"
#include "_log.h"
#include "_memory.h"
#include "datoviz/common/macros.h"
#include "datoviz/vk/memory.h"
#include "datoviz/vk/memory_interop.h"

MUTE_ON
MUTE_OFF
#include "macros.h"
#if OS_WINDOWS
#include <vulkan/vulkan_win32.h>
#include <windows.h>
#endif



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

/**
 * Convert public Datoviz allocation policy flags to internal VMA creation flags.
 *
 * @param flags Datoviz allocation policy flags
 * @return equivalent VMA allocation creation flags
 */
static VmaAllocationCreateFlags _dvz_to_vma_allocation_flags(DvzAllocationFlags flags)
{
    VmaAllocationCreateFlags out = 0;
    if ((flags & DVZ_ALLOC_DEDICATED_MEMORY) != 0)
        out |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    if ((flags & DVZ_ALLOC_MAPPED) != 0)
        out |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
    if ((flags & DVZ_ALLOC_HOST_ACCESS_SEQUENTIAL_WRITE) != 0)
        out |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    if ((flags & DVZ_ALLOC_HOST_ACCESS_RANDOM) != 0)
        out |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
    if ((flags & DVZ_ALLOC_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD) != 0)
        out |= VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT;
    return out;
}



static VmaAllocatorCreateFlags _set_vma_flags(DvzDevice* device)
{
    ANN(device);

    VmaAllocatorCreateFlags vma_flags = 0;
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
        log_warn(                                                                                 \
            "unable to use external feature as the external flag was not set at allocator "       \
            "creation");                                                                          \
        return -1;                                                                                \
    }



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Allocate an empty allocator wrapper.
 *
 * @return allocated allocator wrapper, or NULL on allocation failure
 */
DvzVma* dvz_allocator_create(void)
{
    DvzVma* allocator = (DvzVma*)dvz_calloc(1, sizeof(DvzVma));
    ANN(allocator);
    return allocator;
}



/**
 * Free an allocator wrapper allocated by dvz_allocator_create().
 *
 * @param allocator allocator wrapper to free
 */
void dvz_allocator_free(DvzVma* allocator)
{
    if (allocator == NULL)
    {
        return;
    }
    dvz_free(allocator);
}



/**
 * Allocate an empty allocation wrapper.
 *
 * @return allocated allocation wrapper, or NULL on allocation failure
 */
DvzAllocation* dvz_allocation_create(void)
{
    DvzAllocation* alloc = (DvzAllocation*)dvz_calloc(1, sizeof(DvzAllocation));
    ANN(alloc);
    return alloc;
}



/**
 * Free an allocation wrapper allocated by dvz_allocation_create().
 *
 * @param alloc allocation wrapper to free
 */
void dvz_allocation_free(DvzAllocation* alloc)
{
    if (alloc == NULL)
    {
        return;
    }
    dvz_free(alloc);
}



/**
 * Return the device associated with an allocator.
 *
 * @param allocator the allocator
 * @return associated device
 */
DvzDevice* dvz_allocator_device(DvzVma* allocator)
{
    ANN(allocator);
    return allocator->device;
}



/**
 * Return the external-handle type configured on an allocator.
 *
 * @param allocator the allocator
 * @return external memory handle type flags (0 when disabled)
 */
VkExternalMemoryHandleTypeFlagsKHR dvz_allocator_external(DvzVma* allocator)
{
    ANN(allocator);
    return allocator->external;
}



/**
 * Return the mapped pointer currently associated with an allocation.
 *
 * @param alloc the allocation
 * @return mapped pointer or NULL
 */
void* dvz_allocation_mapped(DvzAllocation* alloc)
{
    ANN(alloc);
    return alloc->mmap;
}



/**
 * Return the allocation policy flags currently associated with an allocation.
 *
 * @param alloc the allocation
 * @return allocation policy flags
 */
DvzAllocationFlags dvz_allocation_flags(DvzAllocation* alloc)
{
    ANN(alloc);
    return alloc->flags;
}



/**
 * Test whether a flag set contains all requested allocation policy flags.
 *
 * @param flags flag set to test
 * @param test flags that must all be present
 * @return true when every flag in test is set in flags
 */
bool dvz_allocation_flags_contains(DvzAllocationFlags flags, DvzAllocationFlags test)
{
    return (flags & test) == test;
}



/**
 * Update the allocation policy flags used by higher-level wrappers.
 *
 * @param alloc the allocation
 * @param flags allocation policy flags
 */
void dvz_allocation_set_flags(DvzAllocation* alloc, DvzAllocationFlags flags)
{
    ANN(alloc);
    alloc->flags = flags;
}



/**
 * Return the Vulkan device memory handle of an allocation.
 *
 * @param alloc the allocation
 * @return Vulkan device memory handle
 */
VkDeviceMemory dvz_allocation_memory(DvzAllocation* alloc)
{
    ANN(alloc);
    return alloc->info.deviceMemory;
}



/**
 * Return the allocation size, in bytes.
 *
 * @param alloc the allocation
 * @return allocation size in bytes
 */
VkDeviceSize dvz_allocation_size(DvzAllocation* alloc)
{
    ANN(alloc);
    return alloc->info.size;
}



int dvz_device_allocator(
    DvzDevice* device, VkExternalMemoryHandleTypeFlagsKHR external, DvzVma* allocator)
{
    ANN(device);
    ANN(allocator);

    DvzGpu* gpu = device->gpu;
    ANN(gpu);

    allocator->device = device;
    allocator->external = external;

    VmaAllocatorCreateFlags vma_flags = _set_vma_flags(device);

    VmaVulkanFunctions funcs = {0};

    VmaAllocatorCreateInfo info = {0};
    info.flags = vma_flags | VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
    info.vulkanApiVersion = gpu->instance->vk_version;
    info.physicalDevice = gpu->pdevice;
    info.device = device->vk_device;
    info.instance = gpu->instance->vk_instance;

#if defined(VOLK_HEADER_VERSION)
    VkResult import_res = vmaImportVulkanFunctionsFromVolk(&info, &funcs);
    if (check_result(import_res) != 0)
        return 1;
#else
    funcs.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    funcs.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
#endif

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
    info.pVulkanFunctions = &funcs;
    VK_RETURN_RESULT(vmaCreateAllocator(&info, &allocator->vma));
    if (out == 0)
        log_trace("allocator created");

    return out;
}



int dvz_allocator_buffer(
    DvzVma* allocator, VkBufferCreateInfo* info, DvzAllocationFlags flags, DvzAllocation* alloc,
    VkBuffer* vk_buffer)
{
    ANN(allocator);
    ANN(info);
    ANN(alloc);
    ANN(vk_buffer);

    VmaAllocationCreateInfo alloc_info = {0};
    VmaMemoryUsage usage = VMA_MEMORY_USAGE_AUTO;
    if (dvz_allocation_flags_contains(flags, DVZ_ALLOC_HOST_ACCESS_SEQUENTIAL_WRITE))
    {
        usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        alloc_info.requiredFlags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    }
    if (dvz_allocation_flags_contains(flags, DVZ_ALLOC_HOST_ACCESS_RANDOM))
    {
        usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        alloc_info.requiredFlags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    }
    alloc_info.usage = alloc->usage = usage;
    alloc->flags = flags;
    alloc_info.flags = _dvz_to_vma_allocation_flags(flags);

    // External memory.
    VkExternalMemoryBufferCreateInfo external_info = {0};
    VkBufferCreateInfo info_local = *info;
    if (allocator->external != 0)
    {
        external_info.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO;
        external_info.handleTypes = allocator->external;
        external_info.pNext = info_local.pNext;
        info_local.pNext = &external_info;
    }

    // TODO: queue families

    log_trace("creating buffer...");
    VK_RETURN_RESULT(vmaCreateBuffer(
        allocator->vma, &info_local, &alloc_info, vk_buffer, &alloc->alloc, &alloc->info));
    if (out == 0)
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
    DvzVma* allocator, VkImageCreateInfo* info, DvzAllocationFlags flags, DvzAllocation* alloc,
    VkImage* vk_image)
{
    ANN(allocator);
    ANN(info);
    ANN(alloc);
    ANN(vk_image);

    VmaAllocationCreateInfo alloc_info = {0};
    alloc_info.usage = alloc->usage = VMA_MEMORY_USAGE_AUTO;
    alloc->flags = flags;
    alloc_info.flags = _dvz_to_vma_allocation_flags(flags);

    log_trace("creating image...");
    VK_RETURN_RESULT(
        vmaCreateImage(allocator->vma, info, &alloc_info, vk_image, &alloc->alloc, &alloc->info));
    if (out == 0)
        log_trace("image created");

    return out;
}



void* dvz_allocator_map(DvzVma* allocator, DvzAllocation* alloc)
{
    ANN(allocator);
    ANN(alloc);
    VkResult res = VK_SUCCESS;
    uint32_t* ptr = NULL;
    res = vmaMapMemory(allocator->vma, alloc->alloc, (void**)&ptr);
    if (res != VK_SUCCESS || ptr == NULL)
    {
        log_warn("unable to map allocation memory (VkResult=%d)", (int)res);
        alloc->mmap = NULL;
        return NULL;
    }
    alloc->mmap = ptr;
    return ptr;
}



void dvz_allocator_unmap(DvzVma* allocator, DvzAllocation* alloc)
{
    ANN(allocator);
    ANN(alloc);
    vmaUnmapMemory(allocator->vma, alloc->alloc);
    alloc->mmap = NULL;
}



/*************************************************************************************************/
/*  Flush / Invalidate                                                                           */
/*************************************************************************************************/


/**
 * Flush mapped CPU memory ranges so the device sees the writes.
 *
 * @param allocator the allocator
 * @param alloc the allocation
 * @param offset the byte offset within the allocation
 * @param size the number of bytes to flush
 * @returns 0 on success, -1 on failure
 */
int dvz_allocator_flush(
    DvzVma* allocator, DvzAllocation* alloc, VkDeviceSize offset, VkDeviceSize size)
{
    ANN(allocator);
    ANN(alloc);

    VkResult res = vmaFlushAllocation(allocator->vma, alloc->alloc, offset, size);
    return res == VK_SUCCESS ? 0 : -1;
}


/**
 * Invalidate mapped CPU memory ranges so the CPU sees the latest device writes.
 *
 * @param allocator the allocator
 * @param alloc the allocation
 * @param offset the byte offset within the allocation
 * @param size the number of bytes to invalidate
 * @returns 0 on success, -1 on failure
 */
int dvz_allocator_invalidate(
    DvzVma* allocator, DvzAllocation* alloc, VkDeviceSize offset, VkDeviceSize size)
{
    ANN(allocator);
    ANN(alloc);

    VkResult res = vmaInvalidateAllocation(allocator->vma, alloc->alloc, offset, size);
    return res == VK_SUCCESS ? 0 : -1;
}


/*************************************************************************************************/
/*  Copy helpers                                                                                 */
/*************************************************************************************************/


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
int dvz_allocator_copy_to(
    DvzVma* allocator, DvzAllocation* alloc, VkDeviceSize offset, const void* data,
    VkDeviceSize size)
{
    ANN(allocator);
    ANN(alloc);
    ANN(data);
    if (size == 0)
    {
        return 0;
    }

    VkResult res = vmaCopyMemoryToAllocation(allocator->vma, data, alloc->alloc, offset, size);
    return res == VK_SUCCESS ? 0 : -1;
}


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
int dvz_allocator_copy_from(
    DvzVma* allocator, DvzAllocation* alloc, VkDeviceSize offset, void* data, VkDeviceSize size)
{
    ANN(allocator);
    ANN(alloc);
    ANN(data);
    if (size == 0)
    {
        return 0;
    }

    VkResult res = vmaCopyAllocationToMemory(allocator->vma, alloc->alloc, offset, data, size);
    return res == VK_SUCCESS ? 0 : -1;
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

#if OS_UNIX
    if (!dvz_device_has_extension(allocator->device, VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME))
    {
        log_error(
            "VK_KHR_external_memory_fd extension not enabled on device; cannot export "
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

    VK_RETURN_RESULT(vkGetMemoryFdKHR(vkd, &info, handle));

#elif OS_WINDOWS
    if (!dvz_device_has_extension(allocator->device, VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME))
    {
        log_error(
            "VK_KHR_external_memory_win32 extension not enabled on device; cannot export "
            "memory handle");
        return -1;
    }

    VkMemoryGetWin32HandleInfoKHR info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR,
    };
    info.memory = alloc->info.deviceMemory;
    ANNVK(info.memory);
    info.handleType = allocator->external;
    if (info.handleType == 0)
    {
        log_error(
            "the allocator must have been created with a VkExternalMemoryHandleTypeFlagsKHR flag");
        return -1;
    }

    HANDLE win32_handle = NULL;
    VK_RETURN_RESULT(vkGetMemoryWin32HandleKHR(vkd, &info, &win32_handle));
    *handle = (int)(uintptr_t)win32_handle;

#else
    int out = -1;
#endif

    return out;
}



int dvz_allocator_import_buffer(
    DvzVma* allocator, VkBufferCreateInfo* info, DvzAllocationFlags flags, int handle,
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
        log_error(
            "info.pNext must be NULL, otherwise need to iterate through the next chain and "
            "set external_info to the last one. PR welcome");
    }
    else
    {
        info->pNext = &external_info;
    }

    // VMA allocation create info.
    VmaAllocationCreateInfo alloc_info = {0};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
    alloc->flags = flags;
    alloc_info.flags = _dvz_to_vma_allocation_flags(flags);

#if OS_UNIX
    VkImportMemoryFdInfoKHR import_info = {.sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR};
    import_info.handleType = allocator->external;
    import_info.fd = handle;
#elif OS_WINDOWS
    VkImportMemoryWin32HandleInfoKHR import_info = {
        .sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR,
    };
    import_info.handleType = allocator->external;
    import_info.handle = (HANDLE)(uintptr_t)handle;

#else
    int out = -1;
#endif

    log_trace("creating buffer...");
    VK_RETURN_RESULT(vmaCreateDedicatedBuffer(
        allocator->vma, info, &alloc_info, &import_info, vk_buffer, &alloc->alloc, &alloc->info));
    if (out == 0)
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
    DvzVma* allocator, VkImageCreateInfo* info, DvzAllocationFlags flags, int handle,
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
        log_error(
            "info.pNext must be NULL, otherwise need to iterate through the next chain and "
            "set external_info to the last one. PR welcome");
    }
    else
    {
        info->pNext = &external_info;
    }

    // VMA allocation create info.
    VmaAllocationCreateInfo alloc_info = {0};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
    alloc->flags = flags;
    alloc_info.flags = _dvz_to_vma_allocation_flags(flags);

#if OS_UNIX
    VkImportMemoryFdInfoKHR import_info = {.sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR};
    import_info.handleType = allocator->external;
    import_info.fd = handle;
#elif OS_WINDOWS
    VkImportMemoryWin32HandleInfoKHR import_info = {
        .sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR,
    };
    import_info.handleType = allocator->external;
    import_info.handle = (HANDLE)(uintptr_t)handle;

#else
    int out = -1;
#endif

    log_trace("creating image...");
    VK_RETURN_RESULT(vmaCreateDedicatedImage(
        allocator->vma, info, &alloc_info, &import_info, vk_image, &alloc->alloc, &alloc->info));
    if (out == 0)
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
