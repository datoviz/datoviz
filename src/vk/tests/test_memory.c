/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing memory                                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <float.h>
#include <inttypes.h>
#include <stdbool.h>
#include <unistd.h>

#include "../types.h"
#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"

#if HAS_CUDA
#include <cuda_runtime_api.h>

#include <cuda.h>
#endif

#include "datoviz/vk/device.h"
#include "datoviz/vk/gpu.h"
#include "datoviz/vk/instance.h"
#include "datoviz/vk/memory.h"
#include "datoviz/vk/queues.h"
#include "test_vk.h"
#include "testing.h"
#include "vulkan/vulkan_core.h"



/*************************************************************************************************/
/*  Memory tests                                                                                 */
/*************************************************************************************************/

int test_memory_1(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    // Create an instance.
    DvzInstance instance = {0};
    dvz_instance(&instance, DVZ_INSTANCE_VALIDATION_FLAGS);
    dvz_instance_create(&instance, VK_API_VERSION_1_3);

    // Obtain a GPU.
    uint32_t count = 0;
    DvzGpu* gpus = dvz_instance_gpus(&instance, &count);
    DvzGpu* gpu = &gpus[0];

    // Query the queues.
    DvzQueueCaps* qc = dvz_gpu_queue_caps(gpu);

    // Initialize a device.
    DvzDevice device = {0};
    dvz_gpu_device(gpu, &device);
    dvz_queues(qc, &device.queues);

    // Create the device.
    dvz_device_create(&device);

    // Allocator.
    DvzVma allocator = {0};
    dvz_device_allocator(&device, 0, &allocator);

    // Buffer allocation.
    VkBuffer vk_buffer = VK_NULL_HANDLE;
    VkBufferCreateInfo buf_info = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buf_info.size = 65536;
    buf_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    DvzAllocation buf_alloc = {0};
    dvz_allocator_buffer(&allocator, &buf_info, 0, &buf_alloc, &vk_buffer);

    // Image allocation.
    VkImage vk_image = VK_NULL_HANDLE;
    VkImageCreateInfo img_info = {.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    img_info.imageType = VK_IMAGE_TYPE_2D;
    img_info.extent.width = 800;
    img_info.extent.height = 600;
    img_info.extent.depth = 1;
    img_info.mipLevels = 1;
    img_info.arrayLayers = 1;
    img_info.format = VK_FORMAT_R8G8B8A8_UNORM;
    img_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    img_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    img_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    img_info.samples = VK_SAMPLE_COUNT_1_BIT;
    DvzAllocation img_alloc = {0};
    dvz_allocator_image(&allocator, &img_info, 0, &img_alloc, &vk_image);

    // Resource destruction.
    dvz_allocator_destroy_buffer(&allocator, &buf_alloc, vk_buffer);
    dvz_allocator_destroy_image(&allocator, &img_alloc, vk_image);

    // Cleanup.
    dvz_allocator_destroy(&allocator);
    dvz_device_destroy(&device);
    dvz_instance_destroy(&instance);
    return 0;
}



/*************************************************************************************************/
/*  CUDA interop tests                                                                           */
/*************************************************************************************************/

void launch_add1_kernel(uint32_t* dev_ptr, size_t count);

int test_memory_cuda(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

#if HAS_CUDA
    cudaError_t cerr;
    int device_count = 0;
    cerr = cudaGetDeviceCount(&device_count);
    if (cerr != cudaSuccess || device_count == 0)
    {
        log_error("No CUDA devices found: %s", cudaGetErrorString(cerr));
        return -1;
    }
    log_info("CUDA reports %d device(s)", device_count);

    const VkExternalMemoryHandleTypeFlagBits handle_type =
        VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
    ASSERT(handle_type != 0);

    const size_t N = 1024;
    const size_t SIZE = N * sizeof(uint32_t);

    int out = 0;

    /******************* Vulkan setup *******************/
    DvzInstance instance = {0};
    dvz_instance(&instance, 0);
    // IMPORTANT: need external memory instance extension.
    dvz_instance_request_extension(&instance, VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME);
    dvz_instance_create(&instance, VK_API_VERSION_1_3);

    // Obtain a GPU.
    uint32_t count = 0;
    DvzGpu* gpus = dvz_instance_gpus(&instance, &count);
    DvzGpu* gpu = &gpus[0];

    // Query the queues.
    DvzQueueCaps* qc = dvz_gpu_queue_caps(gpu);

    // Initialize a device.
    DvzDevice device = {0};
    dvz_gpu_device(gpu, &device);
    dvz_queues(qc, &device.queues);
    // IMPORTANT: need external memory device extension.
    dvz_device_request_extension(&device, VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME);
    dvz_device_create(&device);

    // Memory allocator.
    DvzVma allocator = {0};
    // IMPORTANT: need to pass the external memory handle type when creating the allocator.
    dvz_device_allocator(&device, handle_type, &allocator);

    // Create a mappable buffer.
    VkBufferCreateInfo buf_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = SIZE,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
    };
    VkBuffer vk_buffer = VK_NULL_HANDLE;
    DvzAllocation alloc = {0};
    dvz_allocator_buffer(
        &allocator, &buf_info,
        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT,
        &alloc, &vk_buffer);

    /******************* Initialize data on Vulkan side *******************/
    log_trace("mapping and sending data to the buffer");
    uint32_t* ptr = (uint32_t*)dvz_allocator_map(&allocator, &alloc);
    ANN(ptr);
    for (uint32_t i = 0; i < N; i++)
        ptr[i] = i;
    dvz_allocator_unmap(&allocator, &alloc);
    log_trace("data copied");

    /******************* Export memory FD *******************/
    int fd = -1;
    dvz_allocator_export(&allocator, &alloc, &fd);
    if (fd < 0)
    {
        log_error("Failed to export Vulkan memory FD");
        goto cleanup_vulkan;
    }
    else
    {
        log_trace("Vulkan memory allocation successfully exported");
    }

    /******************* Import into CUDA *******************/
    cudaExternalMemory_t cuda_mem = {0};
    struct cudaExternalMemoryHandleDesc handle_desc = {0};
    handle_desc.type = cudaExternalMemoryHandleTypeOpaqueFd;
    handle_desc.handle.fd = fd;
    handle_desc.size = SIZE;

    cerr = cudaImportExternalMemory(&cuda_mem, &handle_desc);
    if (cerr != cudaSuccess)
    {
        log_error("cudaImportExternalMemory failed: %s", cudaGetErrorString(cerr));
        goto cleanup_fd;
    }

    void* cuda_ptr = NULL;
    struct cudaExternalMemoryBufferDesc buf_desc = {0};
    buf_desc.offset = 0;
    buf_desc.size = SIZE;
    cerr = cudaExternalMemoryGetMappedBuffer(&cuda_ptr, cuda_mem, &buf_desc);
    if (cerr != cudaSuccess)
    {
        log_error("cudaExternalMemoryGetMappedBuffer failed: %s", cudaGetErrorString(cerr));
        goto cleanup_cuda_mem;
    }

    /******************* CUDA modifies Vulkan memory *******************/
    launch_add1_kernel((uint32_t*)cuda_ptr, N);
    cudaDeviceSynchronize();

    /******************* Check result from Vulkan side *******************/
    ptr = (uint32_t*)dvz_allocator_map(&allocator, &alloc);
    for (uint32_t i = 0; i < N; i++)
    {
        if (ptr[i] != i + 1)
        {
            log_error("Mismatch after CUDA write at %u: got %u expected %u", i, ptr[i], i + 1);
            out = 1;
            break;
        }
    }
    dvz_allocator_unmap(&allocator, &alloc);
    if (out == 0)
        log_info("Vulkan->CUDA path verified OK (CUDA write visible in Vulkan)");

    /******************* Vulkan modifies data again *******************/
    ptr = (uint32_t*)dvz_allocator_map(&allocator, &alloc);
    for (uint32_t i = 0; i < N; i++)
        ptr[i] += 1; // add 1 again (now should be i + 2)
    dvz_allocator_unmap(&allocator, &alloc);
    vkDeviceWaitIdle(device.vk_device); // ensure write visible

    /******************* CUDA reads and checks *******************/
    uint32_t* host_copy = (uint32_t*)malloc(SIZE);
    ANN(host_copy);
    cerr = cudaMemcpy(host_copy, cuda_ptr, SIZE, cudaMemcpyDeviceToHost);
    if (cerr != cudaSuccess)
        log_error("cudaMemcpyDeviceToHost failed: %s", cudaGetErrorString(cerr));
    else
    {
        for (uint32_t i = 0; i < N; i++)
        {
            if (host_copy[i] != i + 2)
            {
                log_error(
                    "Mismatch after Vulkan write at %u: got %u expected %u", i, host_copy[i],
                    i + 2);
                out = 2;
                break;
            }
        }
        if (out == 0)
            log_info("CUDA->Vulkan->CUDA sync verified OK (Vulkan write visible in CUDA)");
    }
    free(host_copy);

    /******************* Cleanup *******************/
cleanup_cuda_mem:
    cudaDestroyExternalMemory(cuda_mem);
cleanup_fd:
    close(fd);
cleanup_vulkan:
    dvz_allocator_destroy_buffer(&allocator, &alloc, vk_buffer);
    dvz_allocator_destroy(&allocator);
    dvz_device_destroy(&device);
    dvz_instance_destroy(&instance);

    return out;
#else
    log_warn("test_memory_cuda skipped because HAS_CUDA=0");
    return -1;
#endif
}
