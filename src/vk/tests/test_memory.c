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
#if !OS_WINDOWS
#include <unistd.h>
#endif

#include "../types.h"
#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"

#if HAS_CUDA
#include <cuda_runtime_api.h>

#include <cuda.h>
#endif

#include "datoviz/vk/bootstrap.h"
#include "datoviz/vk/gpu.h"
#include "datoviz/vk/instance.h"
#include "datoviz/vk/memory.h"
#include "datoviz/vk/queues.h"
#include "test_vk.h"
#include "testing.h"

#if HAS_CUDA
static int cuda_check(CUresult res, const char* label)
{
    if (res != CUDA_SUCCESS)
    {
        const char* name = NULL;
        const char* desc = NULL;
        cuGetErrorName(res, &name);
        cuGetErrorString(res, &desc);
        log_error(
            "%s failed: %s (%s)", label, (name != NULL) ? name : "CUDA_ERROR",
            (desc != NULL) ? desc : "no description");
        return 1;
    }
    return 0;
}
#endif



/*************************************************************************************************/
/*  Memory tests                                                                                 */
/*************************************************************************************************/

int test_memory_1(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    // Bootstrap.
    DvzBootstrap bootstrap = {0};
    dvz_bootstrap(&bootstrap, 0);

    DvzGpu* gpu = dvz_bootstrap_gpu(&bootstrap);
    ANN(gpu);

    DvzVma* allocator = dvz_bootstrap_allocator(&bootstrap);
    ANN(allocator);

    // Buffer allocation.
    VkBuffer vk_buffer = VK_NULL_HANDLE;
    VkBufferCreateInfo buf_info = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buf_info.size = 65536;
    buf_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    DvzAllocation buf_alloc = {0};
    dvz_allocator_buffer(allocator, &buf_info, 0, &buf_alloc, &vk_buffer);

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
    img_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    img_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    img_info.samples = VK_SAMPLE_COUNT_1_BIT;
    DvzAllocation img_alloc = {0};
    dvz_allocator_image(allocator, &img_info, 0, &img_alloc, &vk_image);

    // Resource destruction.
    dvz_allocator_destroy_buffer(allocator, &buf_alloc, vk_buffer);
    dvz_allocator_destroy_image(allocator, &img_alloc, vk_image);

    // Cleanup.
    dvz_bootstrap_destroy(&bootstrap);

    RETURN_VALIDATION
}



/*************************************************************************************************/
/*  CUDA interop tests                                                                           */
/*************************************************************************************************/

void launch_add1_kernel(uint32_t* dev_ptr, size_t count);

int test_memory_cuda_1(TstSuite* suite, TstItem* tstitem)
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



int test_memory_cuda_2(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

#if HAS_CUDA
    int out = 0;
    CUresult curet = CUDA_SUCCESS;
    CUdevice cu_device = 0;
    CUcontext cu_context = NULL;
    bool retained_primary = false;
    bool reserved_address = false;
    bool mapped_memory = false;

    CUdeviceptr cuda_ptr = 0;
    CUmemGenericAllocationHandle alloc_handle = 0;
    size_t alloc_size = 0;
    int fd = -1;

    const VkExternalMemoryHandleTypeFlagBits handle_type =
        VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
    ASSERT(handle_type != 0);

    const size_t N = 1024;
    const size_t SIZE = N * sizeof(uint32_t);
    const uint32_t vulkan_delta = 5;

    curet = cuInit(0);
    if (cuda_check(curet, "cuInit"))
    {
        out = 1;
        goto cleanup;
    }

    curet = cuDeviceGet(&cu_device, 0);
    if (cuda_check(curet, "cuDeviceGet"))
    {
        out = 1;
        goto cleanup;
    }

    curet = cuCtxGetCurrent(&cu_context);
    if (curet != CUDA_SUCCESS || cu_context == NULL)
    {
        curet = cuDevicePrimaryCtxRetain(&cu_context, cu_device);
        if (cuda_check(curet, "cuDevicePrimaryCtxRetain"))
        {
            out = 1;
            goto cleanup;
        }
        retained_primary = true;
    }

    curet = cuCtxSetCurrent(cu_context);
    if (cuda_check(curet, "cuCtxSetCurrent"))
    {
        out = 1;
        goto cleanup;
    }

    CUmemAllocationProp prop = {0};
    prop.type = CU_MEM_ALLOCATION_TYPE_PINNED;
    prop.location.type = CU_MEM_LOCATION_TYPE_DEVICE;
    prop.location.id = cu_device;
    prop.requestedHandleTypes = CU_MEM_HANDLE_TYPE_POSIX_FILE_DESCRIPTOR;

    size_t granularity = 0;
    curet = cuMemGetAllocationGranularity(&granularity, &prop, CU_MEM_ALLOC_GRANULARITY_MINIMUM);
    if (cuda_check(curet, "cuMemGetAllocationGranularity"))
    {
        out = 1;
        goto cleanup;
    }

    alloc_size = ((SIZE + granularity - 1) / granularity) * granularity;
    if (alloc_size == 0)
    {
        log_error("invalid allocation size computed for CUDA external memory");
        out = 1;
        goto cleanup;
    }

    curet = cuMemAddressReserve(&cuda_ptr, alloc_size, granularity, 0, 0);
    if (cuda_check(curet, "cuMemAddressReserve"))
    {
        out = 1;
        goto cleanup;
    }
    reserved_address = true;

    curet = cuMemCreate(&alloc_handle, alloc_size, &prop, 0);
    if (cuda_check(curet, "cuMemCreate"))
    {
        out = 1;
        goto cleanup;
    }

    curet = cuMemMap(cuda_ptr, alloc_size, 0, alloc_handle, 0);
    if (cuda_check(curet, "cuMemMap"))
    {
        out = 1;
        goto cleanup;
    }
    mapped_memory = true;

    CUmemAccessDesc access = {0};
    access.location.type = CU_MEM_LOCATION_TYPE_DEVICE;
    access.location.id = cu_device;
    access.flags = CU_MEM_ACCESS_FLAGS_PROT_READWRITE;
    curet = cuMemSetAccess(cuda_ptr, alloc_size, &access, 1);
    if (cuda_check(curet, "cuMemSetAccess"))
    {
        out = 1;
        goto cleanup;
    }

    uint32_t* host_init = (uint32_t*)malloc(SIZE);
    if (host_init == NULL)
    {
        log_error("unable to allocate host staging buffer for CUDA initialization");
        out = 1;
        goto cleanup;
    }
    for (uint32_t i = 0; i < N; i++)
        host_init[i] = i * 7 + 3;

    curet = cuMemcpyHtoD(cuda_ptr, host_init, SIZE);
    free(host_init);
    host_init = NULL;
    if (cuda_check(curet, "cuMemcpyHtoD"))
    {
        out = 1;
        goto cleanup;
    }

    curet = cuCtxSynchronize();
    if (cuda_check(curet, "cuCtxSynchronize (after init copy)"))
    {
        out = 1;
        goto cleanup;
    }

    curet = cuMemExportToShareableHandle(
        &fd, alloc_handle, CU_MEM_HANDLE_TYPE_POSIX_FILE_DESCRIPTOR, 0);
    if (cuda_check(curet, "cuMemExportToShareableHandle"))
    {
        out = 1;
        goto cleanup;
    }

    /******************* Vulkan setup *******************/
    DvzInstance instance = {0};
    dvz_instance(&instance, 0);
    dvz_instance_request_extension(&instance, VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME);
    dvz_instance_create(&instance, VK_API_VERSION_1_3);

    uint32_t gpu_count = 0;
    DvzGpu* gpus = dvz_instance_gpus(&instance, &gpu_count);
    ANN(gpus);
    DvzGpu* gpu = &gpus[0];

    DvzQueueCaps* qc = dvz_gpu_queue_caps(gpu);

    DvzDevice device = {0};
    dvz_gpu_device(gpu, &device);
    dvz_queues(qc, &device.queues);
    dvz_device_request_extension(&device, VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME);
    dvz_device_create(&device);

    DvzVma allocator = {0};
    dvz_device_allocator(&device, handle_type, &allocator);

    VkBuffer vk_buffer = VK_NULL_HANDLE;
    DvzAllocation alloc = {0};
    VkBufferCreateInfo buf_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = SIZE,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
    };
    out = dvz_allocator_import_buffer(
        &allocator, &buf_info,
        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT,
        fd, &alloc, &vk_buffer);
    if (out != 0)
    {
        log_error("dvz_allocator_import_buffer failed");
        goto cleanup_vulkan;
    }

    close(fd);
    fd = -1;

    // BUG: this test fails here, the buffer appears to be all zeros, as if the import didn't work.

    /******************* Validate Vulkan view *******************/
    uint32_t* ptr = (uint32_t*)dvz_allocator_map(&allocator, &alloc);
    ANN(ptr);
    for (uint32_t i = 0; i < N; i++)
    {
        uint32_t expected = i * 7 + 3;
        if (ptr[i] != expected)
        {
            log_error(
                "Mismatch right after import at %u: got %u expected %u", i, ptr[i], expected);
            out = 2;
            break;
        }
    }
    dvz_allocator_unmap(&allocator, &alloc);
    if (out != 0)
        goto cleanup_vulkan;

    /******************* Modify from Vulkan *******************/
    ptr = (uint32_t*)dvz_allocator_map(&allocator, &alloc);
    ANN(ptr);
    for (uint32_t i = 0; i < N; i++)
        ptr[i] += vulkan_delta;
    dvz_allocator_unmap(&allocator, &alloc);
    vkDeviceWaitIdle(device.vk_device);

    /******************* Check from CUDA *******************/
    uint32_t* host_verify = (uint32_t*)malloc(SIZE);
    if (host_verify == NULL)
    {
        log_error("unable to allocate host buffer for CUDA verification");
        out = 3;
        goto cleanup_vulkan;
    }
    curet = cuMemcpyDtoH(host_verify, cuda_ptr, SIZE);
    if (cuda_check(curet, "cuMemcpyDtoH"))
    {
        free(host_verify);
        out = 3;
        goto cleanup_vulkan;
    }

    curet = cuCtxSynchronize();
    if (cuda_check(curet, "cuCtxSynchronize (after device readback)"))
    {
        free(host_verify);
        out = 3;
        goto cleanup_vulkan;
    }

    for (uint32_t i = 0; i < N; i++)
    {
        uint32_t expected = i * 7 + 3 + vulkan_delta;
        if (host_verify[i] != expected)
        {
            log_error(
                "Mismatch after Vulkan write at %u: got %u expected %u", i, host_verify[i],
                expected);
            out = 4;
            break;
        }
    }
    if (out == 0)
        log_info("CUDA->Vulkan->CUDA import path verified OK");

    free(host_verify);

cleanup_vulkan:
    if (vk_buffer != VK_NULL_HANDLE)
        dvz_allocator_destroy_buffer(&allocator, &alloc, vk_buffer);
    if (allocator.vma != NULL)
        dvz_allocator_destroy(&allocator);
    dvz_device_destroy(&device);
    dvz_instance_destroy(&instance);

cleanup:
    if (fd >= 0)
        close(fd);
    if (mapped_memory)
        cuMemUnmap(cuda_ptr, alloc_size);
    if (cuda_ptr != 0 && reserved_address)
        cuMemAddressFree(cuda_ptr, alloc_size);
    if (alloc_handle != 0)
        cuMemRelease(alloc_handle);
    if (retained_primary)
        cuDevicePrimaryCtxRelease(cu_device);

    return out;
#else
    log_warn("test_memory_cuda_2 skipped because HAS_CUDA=0");
    return -1;
#endif
}
