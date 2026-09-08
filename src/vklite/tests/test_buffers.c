/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing buffers                                                                              */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>
#include <string.h>
#include <volk.h>

#include "../_buffers.h"
#include "../../vk/_memory.h"

#include "test_vk.h"
#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "datoviz/vk/gpu_ctx.h"
#include "datoviz/vk/device.h"
#include "datoviz/vklite/buffers.h"
#include "test_vklite.h"
#include "datoviz/math/types.h"
#include "datoviz/vk/instance.h"
#include "testing.h"
#include "vulkan_core.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define MAP_OFFSET 64
#define MAP_SIZE   1024



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_vklite_buffers_1(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    // Bootstrap.
    DvzGpuCtxConfig cfg = dvz_testing_gpu_ctx_config(suite);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&cfg);
    ANN(ctx);

    DvzBuffer* buffer = dvz_buffer_create_wrapper();
    ANN(buffer);
    DvzSize size = 65536;

    dvz_buffer(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx), buffer);
    dvz_buffer_size(buffer, size);
    dvz_buffer_flags(buffer, DVZ_ALLOC_HOST_ACCESS_SEQUENTIAL_WRITE);
    dvz_buffer_usage(buffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    AT(dvz_buffer_size_value(buffer) == size);
    dvz_buffer_create(buffer);

    // Map the buffer.
    dvz_buffer_map(buffer);

    // Create some data.
    uint8_t data[MAP_SIZE] = {0};
    DvzSize msize = MAP_SIZE;
    for (uint32_t i = 0; i < MAP_SIZE; i++)
    {
        data[i] = i;
    }
    DvzSize offset = MAP_OFFSET;

    // Upload the data and check.
    dvz_buffer_upload(buffer, offset, msize, data);
    AT(data[10] == 10);

    // Reset the data.
    dvz_buffer_unmap(buffer);
    dvz_memset(data, msize, 0, msize);
    AT(data[10] == 0);

    // Download the data and check again.
    dvz_buffer_download(buffer, offset, msize, data);
    AT(data[10] == 10);


    // RESIZING.

    // No-op as buffer is smaller.
    AT(dvz_buffer_resize(buffer, size / 2) == 0);

    // Download the data and check again.
    dvz_buffer_download(buffer, offset, msize, data);
    AT(data[10] == 10);

    // Buffer recreated if size is larger.
    AT(dvz_buffer_resize(buffer, 2 * size) == 0);

    // Growth replaces storage; its contents are undefined until the next upload.
    AT(dvz_buffer_allocated_size(buffer) == 2 * size);
    AT(dvz_buffer_handle(buffer) != VK_NULL_HANDLE);

    // Cleanup.
    dvz_buffer_destroy(buffer);
    dvz_buffer_destroy(buffer);
    AT(dvz_buffer_handle(buffer) == VK_NULL_HANDLE);

    AT(dvz_buffer_create(buffer) == 0);
    AT(dvz_buffer_handle(buffer) != VK_NULL_HANDLE);
    AT(dvz_buffer_allocated_size(buffer) == 2 * size);
    dvz_buffer_destroy(buffer);
    AT(dvz_buffer_handle(buffer) == VK_NULL_HANDLE);

    dvz_buffer_free(buffer);
    uint32_t err_count = dvz_gpu_ctx_error_count(ctx);
    dvz_gpu_ctx_destroy(ctx);

    return err_count > 0;
}



int test_vklite_buffer_views(TstContext* suite, const TstCase* tstitem)
{
    // Bootstrap.
    DvzGpuCtxConfig cfg = dvz_testing_gpu_ctx_config(suite);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&cfg);
    ANN(ctx);

    DvzBuffer* buffer = dvz_buffer_create_wrapper();
    ANN(buffer);
    DvzSize size = 65536;

    dvz_buffer(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx), buffer);
    dvz_buffer_size(buffer, size);
    dvz_buffer_flags(buffer, DVZ_ALLOC_HOST_ACCESS_SEQUENTIAL_WRITE);
    dvz_buffer_usage(buffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    AT(dvz_buffer_size_value(buffer) == size);
    dvz_buffer_create(buffer);

    DvzBufferViews* views = dvz_buffer_views_create();
    ANN(views);
    DvzSize offset = 33;
    DvzSize vsize = 7;
    DvzSize alignment = 16;
    dvz_buffer_views(buffer, 3, offset, vsize, alignment, views);
    AT(dvz_buffer_views_count(views) == 3);
    AT(dvz_buffer_views_size(views) == vsize);
    AT(dvz_buffer_views_aligned_size(views) == 16);
    AT(dvz_buffer_views_offset(views, 0) == 48);
    AT(dvz_buffer_views_offset(views, 1) == 64);
    AT(dvz_buffer_views_offset(views, 2) == 80);

    // Cleanup.
    dvz_buffer_destroy(buffer);
    dvz_buffer_destroy(buffer);
    AT(dvz_buffer_handle(buffer) == VK_NULL_HANDLE);
    dvz_buffer_views_free(views);
    dvz_buffer_free(buffer);
    uint32_t err_count = dvz_gpu_ctx_error_count(ctx);
    dvz_gpu_ctx_destroy(ctx);

    return err_count > 0;
}



int test_vklite_buffer_create_requires_destroy(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzGpuCtxConfig cfg = dvz_testing_gpu_ctx_config(suite);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&cfg);
    ANN(ctx);

    DvzBuffer* buffer = dvz_buffer_create_wrapper();
    ANN(buffer);

    dvz_buffer(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx), buffer);
    dvz_buffer_size(buffer, 0);
    dvz_buffer_usage(buffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    dvz_buffer_flags(buffer, DVZ_ALLOC_HOST_ACCESS_SEQUENTIAL_WRITE);

    AT_EXPECTED_ERROR_STRICT(suite, dvz_buffer_create(buffer) != 0);
    AT(dvz_buffer_handle(buffer) == VK_NULL_HANDLE);

    dvz_buffer_destroy(buffer);
    AT(dvz_buffer_handle(buffer) == VK_NULL_HANDLE);

    dvz_buffer_size(buffer, 4096);
    AT(dvz_buffer_create(buffer) == 0);
    AT(dvz_buffer_handle(buffer) != VK_NULL_HANDLE);

    AT_EXPECTED_ERROR_STRICT(suite, dvz_buffer_create(buffer) != 0);
    AT(dvz_buffer_handle(buffer) != VK_NULL_HANDLE);

    dvz_buffer_destroy(buffer);
    AT(dvz_buffer_handle(buffer) == VK_NULL_HANDLE);

    dvz_buffer_size(buffer, 8192);
    AT(dvz_buffer_create(buffer) == 0);
    AT(dvz_buffer_handle(buffer) != VK_NULL_HANDLE);

    dvz_buffer_destroy(buffer);
    dvz_buffer_free(buffer);
    uint32_t err_count = dvz_gpu_ctx_error_count(ctx);
    dvz_gpu_ctx_destroy(ctx);

    return err_count > 0;
}



/**
 * Reject heap allocation while preparing a replacement buffer.
 *
 * @param count element count
 * @param size element size
 * @return NULL
 */
static void* _resize_reject_calloc(DvzSize count, DvzSize size)
{
    (void)count;
    (void)size;
    return NULL;
}



/**
 * Preserve a mapped live buffer when replacement allocation fails, then allow retry.
 *
 * @param suite test context
 * @param item test case
 * @return zero on success
 */
int test_vklite_buffer_resize_allocation_failure(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    DvzGpuCtxConfig cfg = dvz_testing_gpu_ctx_config(suite);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&cfg);
    ANN(ctx);
    DvzBuffer* buffer = dvz_buffer_create_wrapper();
    ANN(buffer);
    dvz_buffer(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx), buffer);
    dvz_buffer_size(buffer, 4096);
    dvz_buffer_usage(buffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    dvz_buffer_flags(buffer, DVZ_ALLOC_HOST_ACCESS_SEQUENTIAL_WRITE);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_buffer_resize(buffer, 8192) != 0);
    AT(dvz_buffer_size_value(buffer) == 4096);
    AT(dvz_buffer_create(buffer) == 0);
    AT(dvz_buffer_map(buffer) == 0);
    uint8_t expected[16] = {1, 2, 3, 4};
    dvz_buffer_upload(buffer, 0, sizeof(expected), expected);
    VkBuffer previous_handle = dvz_buffer_handle(buffer);
    DvzAllocation* previous_allocation = buffer->alloc;
    void* previous_mapping = dvz_allocation_mapped(buffer->alloc);
    DvzSize previous_size = dvz_buffer_allocated_size(buffer);

    const DvzAllocator* previous_allocator = dvz_get_allocator();
    DvzAllocator failing_allocator = *previous_allocator;
    failing_allocator.calloc_fn = _resize_reject_calloc;
    dvz_set_allocator(&failing_allocator);
    int result = dvz_buffer_resize(buffer, 8192);
    dvz_set_allocator(previous_allocator);
    AT(result != 0);

    AT(dvz_buffer_handle(buffer) == previous_handle);
    AT(buffer->alloc == previous_allocation);
    AT(dvz_allocation_mapped(buffer->alloc) == previous_mapping);
    AT(dvz_buffer_size_value(buffer) == 4096);
    AT(dvz_buffer_allocated_size(buffer) == previous_size);
    uint8_t actual[16] = {0};
    dvz_buffer_download(buffer, 0, sizeof(actual), actual);
    AT(memcmp(actual, expected, sizeof(actual)) == 0);

    AT_EXPECTED_ERROR_STRICT(suite, dvz_buffer_resize(buffer, 0) != 0);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_buffer_resize(NULL, 4096) != 0);
    AT(dvz_buffer_resize(buffer, 4096) == 0);
    AT(dvz_buffer_resize(buffer, 2048) == 0);
    AT(dvz_buffer_handle(buffer) == previous_handle);
    AT(dvz_buffer_size_value(buffer) == 4096);
    AT(dvz_allocation_mapped(buffer->alloc) == previous_mapping);

    AT(dvz_buffer_resize(buffer, 8192) == 0);
    AT(dvz_buffer_handle(buffer) != VK_NULL_HANDLE);
    AT(dvz_buffer_size_value(buffer) == 8192);
    AT(dvz_allocation_mapped(buffer->alloc) != NULL);
    dvz_buffer_upload(buffer, 0, sizeof(expected), expected);
    dvz_buffer_download(buffer, 0, sizeof(actual), actual);
    AT(memcmp(actual, expected, sizeof(actual)) == 0);
    dvz_buffer_unmap(buffer);
    AT(dvz_buffer_resize(buffer, 16384) == 0);
    AT(dvz_allocation_mapped(buffer->alloc) == NULL);
    AT(dvz_buffer_size_value(buffer) == 16384);
    dvz_buffer_destroy(buffer);
    dvz_buffer_destroy(buffer);
    dvz_buffer_free(buffer);
    uint32_t errors = dvz_gpu_ctx_error_count(ctx);
    dvz_gpu_ctx_destroy(ctx);
    return errors > 0;
}



/**
 * Forward a test allocator's device-memory allocation through the active Vulkan dispatch.
 *
 * @param device Vulkan device
 * @param info allocation request
 * @param callbacks host allocator callbacks
 * @param memory returned device memory
 * @return Vulkan allocation result
 */
static VKAPI_ATTR VkResult VKAPI_CALL _resize_forward_allocate(
    VkDevice device, const VkMemoryAllocateInfo* info, const VkAllocationCallbacks* callbacks,
    VkDeviceMemory* memory)
{
    return vkAllocateMemory(device, info, callbacks, memory);
}



/**
 * Forward a test allocator's memory map through the active Vulkan dispatch.
 *
 * @param device Vulkan device
 * @param memory device memory to map
 * @param offset byte offset
 * @param size byte count
 * @param flags mapping flags
 * @param data returned host pointer
 * @return Vulkan mapping result
 */
static VKAPI_ATTR VkResult VKAPI_CALL _resize_forward_map(
    VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size,
    VkMemoryMapFlags flags, void** data)
{
    return vkMapMemory(device, memory, offset, size, flags, data);
}



/**
 * Reject replacement device-memory allocation without changing the existing allocation.
 *
 * @param device unused device
 * @param info unused request
 * @param callbacks unused host callbacks
 * @param memory returned null handle
 * @return out-of-device-memory error
 */
static VKAPI_ATTR VkResult VKAPI_CALL _resize_reject_allocate(
    VkDevice device, const VkMemoryAllocateInfo* info, const VkAllocationCallbacks* callbacks,
    VkDeviceMemory* memory)
{
    (void)device;
    (void)info;
    (void)callbacks;
    *memory = VK_NULL_HANDLE;
    return VK_ERROR_OUT_OF_DEVICE_MEMORY;
}



/**
 * Reject mapping of newly allocated replacement memory.
 *
 * @param device unused device
 * @param memory unused allocation
 * @param offset unused offset
 * @param size unused size
 * @param flags unused flags
 * @param data returned null pointer
 * @return memory-map failure
 */
static VKAPI_ATTR VkResult VKAPI_CALL _resize_reject_map(
    VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size,
    VkMemoryMapFlags flags, void** data)
{
    (void)device;
    (void)memory;
    (void)offset;
    (void)size;
    (void)flags;
    *data = NULL;
    return VK_ERROR_MEMORY_MAP_FAILED;
}



/**
 * Preserve mapped buffer state and release staged resources after provider failures.
 *
 * @param suite test context
 * @param item test case
 * @return zero on success
 */
int test_vklite_buffer_resize_provider_failure(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    DvzGpuCtxConfig cfg = dvz_testing_gpu_ctx_config(suite);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&cfg);
    ANN(ctx);
    DvzDevice* device = dvz_gpu_ctx_device(ctx);
    VmaVulkanFunctions funcs = {0};
    VmaAllocatorCreateInfo info = {
        .instance = dvz_instance_handle(dvz_gpu_ctx_instance(ctx)),
        .physicalDevice = dvz_device_physical_device(device),
        .device = dvz_device_handle(device),
        .vulkanApiVersion = VK_API_VERSION_1_0,
    };
    VkResult imported = vmaImportVulkanFunctionsFromVolk(&info, &funcs);
    AT(imported == VK_SUCCESS);
    funcs.vkAllocateMemory = _resize_forward_allocate;
    funcs.vkMapMemory = _resize_forward_map;
    info.pVulkanFunctions = &funcs;
    DvzVma allocator = {.device = device};
    VkResult created = vmaCreateAllocator(&info, &allocator.vma);
    AT(created == VK_SUCCESS);

    DvzBuffer* buffer = dvz_buffer_create_wrapper();
    ANN(buffer);
    dvz_buffer(device, &allocator, buffer);
    dvz_buffer_size(buffer, 4096);
    dvz_buffer_usage(buffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    dvz_buffer_flags(buffer, DVZ_ALLOC_DEDICATED_MEMORY | DVZ_ALLOC_HOST_ACCESS_SEQUENTIAL_WRITE);
    AT(dvz_buffer_create(buffer) == 0);
    AT(dvz_buffer_map(buffer) == 0);
    uint8_t expected[16] = {5, 6, 7, 8};
    dvz_buffer_upload(buffer, 0, sizeof(expected), expected);
    VkBuffer previous_handle = dvz_buffer_handle(buffer);
    DvzAllocation* previous_allocation = buffer->alloc;
    void* previous_mapping = dvz_allocation_mapped(buffer->alloc);

    for (uint32_t fail_map = 0; fail_map < 2; fail_map++)
    {
        PFN_vkAllocateMemory allocate = vkAllocateMemory;
        PFN_vkMapMemory map = vkMapMemory;
        if (!fail_map)
            tst_expect_error_begin(suite);
        if (fail_map)
            vkMapMemory = _resize_reject_map;
        else
            vkAllocateMemory = _resize_reject_allocate;
        int result = dvz_buffer_resize(buffer, 8192);
        vkAllocateMemory = allocate;
        vkMapMemory = map;
        int expected_errors = fail_map ? 0 : tst_expect_error_end(suite);
        AT(expected_errors == 0);
        AT(result != 0);
        AT(dvz_buffer_handle(buffer) == previous_handle);
        AT(buffer->alloc == previous_allocation);
        AT(dvz_buffer_size_value(buffer) == 4096);
        AT(dvz_allocation_mapped(buffer->alloc) == previous_mapping);
        uint8_t actual[16] = {0};
        dvz_buffer_download(buffer, 0, sizeof(actual), actual);
        AT(memcmp(actual, expected, sizeof(actual)) == 0);
        VmaTotalStatistics stats = {0};
        vmaCalculateStatistics(allocator.vma, &stats);
        AT(stats.total.statistics.allocationCount == 1);
    }
    AT(dvz_buffer_resize(buffer, 8192) == 0);
    AT(dvz_buffer_handle(buffer) != previous_handle);
    AT(dvz_buffer_size_value(buffer) == 8192);
    AT(dvz_allocation_mapped(buffer->alloc) != NULL);
    dvz_buffer_destroy(buffer);
    dvz_buffer_free(buffer);
    dvz_allocator_destroy(&allocator);
    uint32_t errors = dvz_gpu_ctx_error_count(ctx);
    dvz_gpu_ctx_destroy(ctx);
    return errors > 0;
}
