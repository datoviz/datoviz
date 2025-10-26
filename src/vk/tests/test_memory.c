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

#include "../types.h"
#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"

#if HAS_CUDA
#include <cuda_runtime_api.h>
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

int test_memory_cuda(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

#if HAS_CUDA
    // Placeholder: the CUDA allocator import/export test will be implemented later.
    int device_count = 0;
    cudaError_t err = cudaGetDeviceCount(&device_count);

    if(err != cudaSuccess)
    {
        log_error("CUDA runtime error: %s", cudaGetErrorString(err));
        return -1;
    }

    log_debug("CUDA runtime detected %d device(s); interop test TBD.", device_count);
    return (device_count > 0) ? 0 : -1;
#else
    log_warn("test_memory_cuda skipped because HAS_CUDA=0");
    return -1;
#endif
}
