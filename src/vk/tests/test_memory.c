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

#include <inttypes.h>
#include <string.h>
#if !OS_WINDOWS
#include <unistd.h>
#endif

#include "../_device.h"
#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include <volk.h>

#if DVZ_HAS_CUDA
#include <cuda.h>
#include <cuda_runtime_api.h>
#endif

#include "datoviz/drp2/enums.h"
#include "datoviz/vk/gpu_ctx.h"
#include "datoviz/vk/device.h"
#include "datoviz/vk/gpu.h"
#include "datoviz/vk/instance.h"
#include "datoviz/vk/memory.h"
#include "datoviz/vk/memory_interop.h"
#include "datoviz/vk/queues.h"
#include "datoviz/vklite/buffers.h"
#include "datoviz/vklite/commands.h"
#include "datoviz/vklite/sync.h"
#include "test_vk.h"
#include "testing.h"

#if DVZ_HAS_CUDA
static int cuda_check(CUresult res, const char* label);



/**
 * Submit a one-shot Vulkan buffer copy on the main queue.
 *
 * @param device logical Vulkan device owning the queue and command pool
 * @param src source Vulkan buffer
 * @param dst destination Vulkan buffer
 * @param size number of bytes to copy
 * @return true on success, false on error
 */
static bool _cuda_import_copy_buffer(
    DvzDevice* device, VkBuffer src, VkBuffer dst, VkDeviceSize size, VkSemaphore wait_semaphore,
    uint64_t wait_value, VkSemaphore signal_semaphore, uint64_t signal_value)
{
    ANN(device);
    ASSERT(src != VK_NULL_HANDLE);
    ASSERT(dst != VK_NULL_HANDLE);
    ASSERT(size > 0);

    DvzQueue* queue = dvz_device_queue(device, DVZ_QUEUE_MAIN);
    if (queue == NULL)
    {
        log_error("main Vulkan queue unavailable for CUDA import copy");
        return false;
    }

    DvzCommands* cmds = dvz_commands_create_wrapper();
    ANN(cmds);
    dvz_commands(device, queue, 1, cmds);
    if (dvz_commands_count(cmds) == 0)
    {
        log_error("failed to allocate command buffer for CUDA import copy");
        dvz_commands_free(cmds);
        return false;
    }

    bool ok = false;
    VkBufferCopy copy = {.srcOffset = 0, .dstOffset = 0, .size = size};
    if (dvz_cmd_begin_result(cmds) == 0)
    {
        if (wait_semaphore != VK_NULL_HANDLE)
        {
            DvzBarriers barriers = {0};
            dvz_barriers(&barriers);
            DvzBarrierBuffer* bbuf = dvz_barriers_buffer(&barriers, src, 0, size);
            dvz_barrier_buffer_stage(
                bbuf, VK_PIPELINE_STAGE_2_NONE, VK_PIPELINE_STAGE_2_TRANSFER_BIT);
            dvz_barrier_buffer_access(
                bbuf, VK_ACCESS_2_MEMORY_WRITE_BIT, VK_ACCESS_2_TRANSFER_READ_BIT);
            dvz_cmd_barriers(cmds, &barriers);
        }
        vkCmdCopyBuffer(dvz_commands_handle(cmds), src, dst, 1, &copy);
        if (signal_semaphore != VK_NULL_HANDLE)
        {
            DvzBarriers barriers = {0};
            dvz_barriers(&barriers);
            DvzBarrierBuffer* bbuf = dvz_barriers_buffer(&barriers, dst, 0, size);
            dvz_barrier_buffer_stage(
                bbuf, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_PIPELINE_STAGE_2_NONE);
            dvz_barrier_buffer_access(
                bbuf, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_ACCESS_2_MEMORY_READ_BIT);
            dvz_cmd_barriers(cmds, &barriers);
        }
        if (dvz_cmd_end_result(cmds) == 0)
        {
            if (wait_semaphore == VK_NULL_HANDLE && signal_semaphore == VK_NULL_HANDLE)
            {
                ok = dvz_cmd_submit_result(cmds) == 0;
            }
            else
            {
                DvzSubmit* submit = dvz_submit_create_wrapper();
                ANN(submit);
                dvz_submit(submit);
                if (wait_semaphore != VK_NULL_HANDLE)
                {
                    dvz_submit_wait(
                        submit, wait_semaphore, wait_value, VK_PIPELINE_STAGE_2_TRANSFER_BIT);
                }
                dvz_submit_command(submit, dvz_commands_handle(cmds));
                if (signal_semaphore != VK_NULL_HANDLE)
                {
                    dvz_submit_signal(
                        submit, signal_semaphore, signal_value, VK_PIPELINE_STAGE_2_TRANSFER_BIT);
                }
                VkResult res =
                    (VkResult)dvz_submit_send(submit, dvz_queue_handle(queue), VK_NULL_HANDLE);
                if (res == VK_SUCCESS)
                {
                    dvz_queue_wait(queue);
                    ok = true;
                }
                else
                {
                    log_error("Vulkan CUDA-import copy submit failed (%d)", res);
                }
                dvz_submit_free(submit);
            }
        }
    }

    dvz_commands_destroy(cmds);
    dvz_commands_free(cmds);
    return ok;
}



/**
 * Find the Vulkan physical device that corresponds to a CUDA device.
 *
 * @param instance Vulkan instance used to enumerate physical devices
 * @param cu_device CUDA device whose UUID should be matched
 * @param[out] out_gpu_index matched Vulkan GPU index
 * @return true when a matching Vulkan device was found, false otherwise
 */
static bool
_cuda_import_find_vulkan_gpu(DvzInstance* instance, CUdevice cu_device, uint32_t* out_gpu_index)
{
    ANN(instance);
    ANN(out_gpu_index);
    *out_gpu_index = UINT32_MAX;

    CUuuid cu_uuid = {0};
    if (cuda_check(cuDeviceGetUuid(&cu_uuid, cu_device), "cuDeviceGetUuid"))
        return false;

    uint32_t gpu_count = dvz_instance_gpu_count(instance);
    for (uint32_t i = 0; i < gpu_count; i++)
    {
        VkPhysicalDevice pdevice = VK_NULL_HANDLE;
        if (!dvz_instance_gpu_handle(instance, i, &pdevice) || pdevice == VK_NULL_HANDLE)
            continue;

        VkPhysicalDeviceIDProperties id = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES};
        VkPhysicalDeviceProperties2 props = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, .pNext = &id};
        vkGetPhysicalDeviceProperties2(pdevice, &props);

        if (memcmp(id.deviceUUID, cu_uuid.bytes, VK_UUID_SIZE) == 0)
        {
            *out_gpu_index = i;
            log_debug("matched CUDA device to Vulkan GPU %u (%s)", i, props.properties.deviceName);
            return true;
        }
    }

    return false;
}



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

int test_memory_1(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    // Bootstrap.
    DvzGpuCtxConfig cfg = dvz_gpu_ctx_config();
    DvzGpuCtx* ctx = dvz_gpu_ctx(&cfg);
    ANN(ctx);

    uint32_t gpu_index = dvz_gpu_ctx_gpu_index(ctx);
    AT(gpu_index != UINT32_MAX);
    DvzGpuInfo gpu_info = {0};
    AT(dvz_gpu_ctx_gpu_info(ctx, &gpu_info));

    DvzVma* allocator = dvz_gpu_ctx_alloc(ctx);
    ANN(allocator);

    // Buffer allocation.
    VkBuffer vk_buffer = VK_NULL_HANDLE;
    VkBufferCreateInfo buf_info = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buf_info.size = 65536;
    buf_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    DvzAllocation* buf_alloc = dvz_allocation_create();
    ANN(buf_alloc);
    dvz_allocator_buffer(allocator, &buf_info, 0, buf_alloc, &vk_buffer);

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
    DvzAllocation* img_alloc = dvz_allocation_create();
    ANN(img_alloc);
    dvz_allocator_image(allocator, &img_info, 0, img_alloc, &vk_image);

    // Resource destruction.
    dvz_allocator_destroy_buffer(allocator, buf_alloc, vk_buffer);
    dvz_allocator_destroy_image(allocator, img_alloc, vk_image);
    dvz_allocation_free(buf_alloc);
    dvz_allocation_free(img_alloc);

    // Cleanup.
    uint32_t err_count = dvz_gpu_ctx_error_count(ctx);
    dvz_gpu_ctx_destroy(ctx);

    return err_count > 0;
}



/**
 * Verify Vulkan-only timeline handoff for transfer reads without external-FD support.
 *
 * @param suite the test suite
 * @param tstitem the test item
 * @return 0 on success
 */
int test_memory_interop_buffer_timeline(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzGpuCtxConfig cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan12Features features12 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .timelineSemaphore = true,
    };
    dvz_gpu_ctx_config_features12(&cfg, &features12);
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .synchronization2 = true,
    };
    dvz_gpu_ctx_config_features13(&cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&cfg);
    if (ctx == NULL)
    {
        tst_skip(suite, "Vulkan timeline semaphores unavailable");
        return 0;
    }

    DvzDevice* device = dvz_gpu_ctx_device(ctx);
    DvzVma* allocator = dvz_gpu_ctx_alloc(ctx);
    DvzBuffer* buffer = dvz_buffer_create_wrapper();
    DvzSemaphore* semaphore = dvz_semaphore_create_wrapper();
    ANN(device);
    ANN(allocator);
    ANN(buffer);
    ANN(semaphore);

    dvz_buffer(device, allocator, buffer);
    dvz_buffer_size(buffer, 256);
    dvz_buffer_usage(buffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    AT(dvz_buffer_create(buffer) == 0);
    dvz_semaphore_timeline(device, 0, semaphore, 0);

    dvz_semaphore_signal(semaphore, 1);
    AT_EXPECTED_ERROR_STRICT(
        suite, !dvz_interop_buffer_wait_timeline_for_consumer(
                   device, buffer, 257, semaphore, 1,
                   DVZ_INTEROP_BUFFER_CONSUMER_TRANSFER_READ));
    AT(dvz_interop_buffer_wait_timeline_for_consumer(
        device, buffer, 256, semaphore, 1, DVZ_INTEROP_BUFFER_CONSUMER_TRANSFER_READ));
    AT_EXPECTED_ERROR_STRICT(
        suite,
        !dvz_interop_buffer_signal_timeline_after_transfer(
            device, buffer, 257, semaphore, 2));
    AT(dvz_interop_buffer_signal_timeline_after_transfer(device, buffer, 256, semaphore, 2));
    AT(dvz_semaphore_query(semaphore) == 2);

    dvz_semaphore_signal(semaphore, 3);
    AT(dvz_interop_buffer_wait_timeline(device, buffer, 256, semaphore, 3));

    dvz_semaphore_destroy(semaphore);
    dvz_semaphore_free(semaphore);
    dvz_buffer_destroy(buffer);
    dvz_buffer_free(buffer);

    uint32_t err_count = dvz_gpu_ctx_error_count(ctx);
    dvz_gpu_ctx_destroy(ctx);
    return err_count > 0;
}



/**
 * Verify the low-level exported buffer metadata package for CUDA/CuPy interop.
 *
 * @param suite the test suite
 * @param tstitem the test item
 * @return 0 on success
 */
int test_memory_interop_buffer_export(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

#if !OS_UNIX
    log_debug("test_memory_interop_buffer_export skipped: opaque FD path is Unix-only");
    tst_skip(suite, "opaque FD path is Unix-only");
    return 0;
#else
    int out = 0;
    DvzInstance* instance = NULL;
    DvzDevice* device = NULL;
    DvzVma* allocator = NULL;
    DvzAllocation* alloc = NULL;
    VkBuffer vk_buffer = VK_NULL_HANDLE;
    DvzSemaphore* semaphore = NULL;
    DvzBuffer* buffer = NULL;
    DvzGpuCtx* interop_ctx = NULL;
    int semaphore_fd = -1;
    DvzInteropBufferExport export_desc = {.memory_handle = -1, .semaphore_handle = -1};

    DvzInstanceConfig icfg = dvz_instance_config();
    icfg.flags = 0;
    dvz_instance_config_request_extension(
        &icfg, VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME);
    instance = dvz_instance_create(&icfg);
    if (instance == NULL)
    {
        out = 1;
        goto cleanup;
    }

    uint32_t gpu_count = dvz_instance_gpu_count(instance);
    if (gpu_count == 0)
    {
        log_debug("test_memory_interop_buffer_export skipped: no Vulkan GPU available");
        tst_skip(suite, "no Vulkan GPU available");
        goto cleanup;
    }

    DvzQueueCaps qc = {0};
    AT(dvz_instance_gpu_queue_caps(instance, 0, &qc));
    DvzQueues queues = {0};
    dvz_queues(&qc, &queues);

    DvzDeviceConfig dcfg = dvz_device_config(instance);
    dvz_device_config_set_gpu_index(&dcfg, 0);
    VkPhysicalDeviceVulkan12Features features12 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    features12.timelineSemaphore = true;
    dvz_device_config_set_features12(&dcfg, &features12);
    for (uint32_t i = 0; i < queues.queue_count; i++)
    {
        DvzQueue* queue = &queues.queues[i];
        dvz_device_config_request_queue(&dcfg, queue->family_idx, 1);
    }
    dvz_device_config_request_extension(&dcfg, VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME);
    dvz_device_config_request_extension(&dcfg, VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME);
    dvz_device_config_request_extension(&dcfg, VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME);
    dvz_device_config_request_extension(&dcfg, VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME);
    device = dvz_device_create(&dcfg);
    if (device == NULL)
    {
        out = 1;
        goto cleanup;
    }
    if (!dvz_device_has_extension(device, VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME))
    {
        log_debug("test_memory_interop_buffer_export skipped: external semaphore FD unsupported");
        tst_skip(suite, "external semaphore FD unsupported");
        goto cleanup;
    }

    allocator = dvz_allocator_create();
    ANN(allocator);
    if (dvz_device_allocator(
            device, VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT, allocator) != 0)
    {
        out = 1;
        goto cleanup;
    }

    VkBufferCreateInfo buf_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = 256,
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
    };
    alloc = dvz_allocation_create();
    ANN(alloc);
    if (dvz_allocator_buffer(allocator, &buf_info, DVZ_ALLOC_DEDICATED_MEMORY, alloc, &vk_buffer) !=
        0)
    {
        out = 1;
        goto cleanup;
    }

    semaphore = dvz_semaphore_create_wrapper();
    ANN(semaphore);
    dvz_semaphore_timeline(
        device, 0, semaphore, VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT);
    semaphore_fd =
        dvz_semaphore_export_fd(semaphore, VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT);
    if (semaphore_fd < 0)
    {
        out = 1;
        goto cleanup;
    }

    AT(dvz_interop_buffer_export(
           allocator, alloc, 16, 128, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, semaphore_fd,
           VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT, 7, &export_desc) == 0);
    AT(export_desc.version == DVZ_INTEROP_BUFFER_EXPORT_VERSION);
    AT(export_desc.memory_handle >= 0);
    AT(export_desc.memory_handle_type == VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT);
    AT(export_desc.allocation_size >= 256);
    AT(export_desc.offset == 16);
    AT(export_desc.size == 128);
    AT(export_desc.usage == VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    AT(export_desc.vk_usage == VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    AT(export_desc.drp2_usage == VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    AT(export_desc.flags == DVZ_ALLOC_DEDICATED_MEMORY);
    AT(export_desc.device_uuid_valid == 1);
    AT(export_desc.semaphore_handle == semaphore_fd);
    AT(export_desc.semaphore_handle_type == VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT);
    AT(export_desc.semaphore_value == 7);
    close(export_desc.memory_handle);
    export_desc.memory_handle = -1;
    export_desc.semaphore_handle = -1;

    buffer = dvz_buffer_create_wrapper();
    ANN(buffer);
    dvz_buffer(device, allocator, buffer);
    dvz_buffer_size(buffer, 256);
    dvz_buffer_usage(
        buffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    dvz_buffer_flags(buffer, DVZ_ALLOC_DEDICATED_MEMORY);
    AT(dvz_buffer_create(buffer) == 0);

    DvzInteropBufferExportConfig export_cfg = {
        DVZ_STRUCT_INIT_FIELDS(DvzInteropBufferExportConfig),
        .offset = 32,
        .size = 0,
        .drp2_usage = DVZ_DRP2_BUFFER_USAGE_VERTEX | DVZ_DRP2_BUFFER_USAGE_STORAGE,
        .export_flags = 123,
        .semaphore = semaphore,
        .semaphore_handle_type = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT,
        .semaphore_value = 9,
    };
    DvzInteropBufferExportConfig invalid_export_cfg = export_cfg;
    invalid_export_cfg.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_interop_buffer_export_from_buffer(buffer, &invalid_export_cfg, &export_desc) < 0);
    invalid_export_cfg = export_cfg;
    invalid_export_cfg.flags = 1;
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_interop_buffer_export_from_buffer(buffer, &invalid_export_cfg, &export_desc) < 0);
    AT(dvz_interop_buffer_export_from_buffer(buffer, &export_cfg, &export_desc) == 0);
    AT(export_desc.version == DVZ_INTEROP_BUFFER_EXPORT_VERSION);
    AT(export_desc.memory_handle >= 0);
    AT(export_desc.memory_handle_type == VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT);
    AT(export_desc.allocation_size >= 256);
    AT(export_desc.offset == 32);
    AT(export_desc.size == 224);
    AT(export_desc.usage ==
       (VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT));
    AT(export_desc.vk_usage == export_desc.usage);
    AT(export_desc.drp2_usage ==
       (DVZ_DRP2_BUFFER_USAGE_VERTEX | DVZ_DRP2_BUFFER_USAGE_STORAGE));
    AT(export_desc.flags == 123);
    AT(export_desc.device_uuid_valid == 1);
    AT(export_desc.semaphore_handle >= 0);
    AT(export_desc.semaphore_handle_type == VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT);
    AT(export_desc.semaphore_value == 9);

    dvz_semaphore_signal(semaphore, 11);
    AT(dvz_interop_buffer_wait_timeline_for_consumer(
        device, buffer, export_desc.size, semaphore, 11,
        DVZ_INTEROP_BUFFER_CONSUMER_TRANSFER_READ));
    AT(dvz_interop_buffer_signal_timeline_after_transfer(
        device, buffer, export_desc.size, semaphore, 12));
    AT(dvz_semaphore_query(semaphore) == 12);

    interop_ctx = dvz_interop_gpu_ctx(0, VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT);
    AT(interop_ctx != NULL);
    AT(dvz_allocator_external(dvz_gpu_ctx_alloc(interop_ctx)) ==
       VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT);

cleanup:
    if (export_desc.memory_handle >= 0)
        close(export_desc.memory_handle);
    if (export_desc.semaphore_handle >= 0 && export_desc.semaphore_handle != semaphore_fd)
        close(export_desc.semaphore_handle);
    if (semaphore_fd >= 0)
        close(semaphore_fd);
    if (buffer != NULL)
    {
        dvz_buffer_destroy(buffer);
        dvz_buffer_free(buffer);
    }
    if (interop_ctx != NULL)
        dvz_gpu_ctx_destroy(interop_ctx);
    if (semaphore != NULL)
    {
        dvz_semaphore_destroy(semaphore);
        dvz_semaphore_free(semaphore);
    }
    if (vk_buffer != VK_NULL_HANDLE)
        dvz_allocator_destroy_buffer(allocator, alloc, vk_buffer);
    if (alloc != NULL)
        dvz_allocation_free(alloc);
    if (allocator != NULL)
    {
        dvz_allocator_destroy(allocator);
        dvz_allocator_free(allocator);
    }
    if (device != NULL)
        dvz_device_destroy(device);
    if (instance != NULL)
        dvz_instance_destroy(instance);
    return out;
#endif
}



/*************************************************************************************************/
/*  CUDA interop tests                                                                           */
/*************************************************************************************************/

void launch_add1_kernel(uint32_t* dev_ptr, size_t count);

int test_memory_cuda_1(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

#if DVZ_HAS_CUDA
    cudaError_t cerr;
    CUdevice cu_device = 0;
    int device_count = 0;
    cerr = cudaGetDeviceCount(&device_count);
    if (cerr != cudaSuccess || device_count == 0)
    {
        log_debug(
            "test_memory_cuda_1 skipped: no CUDA devices found (%s)", cudaGetErrorString(cerr));
        tst_skip(suite, "no CUDA devices found");
        return 0;
    }
    log_debug("CUDA reports %d device(s)", device_count);
    if (cuda_check(cuInit(0), "cuInit"))
        return 1;
    if (cuda_check(cuDeviceGet(&cu_device, 0), "cuDeviceGet"))
        return 1;

    const VkExternalMemoryHandleTypeFlagBits handle_type =
        VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
    ASSERT(handle_type != 0);

    const size_t N = 1024;
    const size_t SIZE = N * sizeof(uint32_t);

    int out = 0;
    DvzInstance* instance = NULL;
    DvzDevice* device = NULL;
    DvzVma* allocator = NULL;
    DvzAllocation* alloc = NULL;
    DvzAllocation* staging_read_alloc = NULL;
    DvzAllocation* staging_write_alloc = NULL;
    VkBuffer vk_buffer = VK_NULL_HANDLE;
    VkBuffer staging_read_buffer = VK_NULL_HANDLE;
    VkBuffer staging_write_buffer = VK_NULL_HANDLE;
    DvzSemaphore* interop_semaphore = NULL;
    cudaExternalSemaphore_t cuda_semaphore = NULL;
    int fd = -1;
    int semaphore_fd = -1;

    /******************* Vulkan setup *******************/
    DvzInstanceConfig icfg = dvz_instance_config();
    icfg.flags = 0;
    // IMPORTANT: need external memory instance extension.
    dvz_instance_config_request_extension(
        &icfg, VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME);
    instance = dvz_instance_create(&icfg);
    if (instance == NULL)
    {
        out = 1;
        goto cleanup_vulkan;
    }

    uint32_t vk_gpu_index = UINT32_MAX;
    if (!_cuda_import_find_vulkan_gpu(instance, cu_device, &vk_gpu_index))
    {
        log_debug("test_memory_cuda_1 skipped: no Vulkan physical device matches CUDA device 0");
        tst_skip(suite, "no Vulkan physical device matches CUDA device 0");
        out = 0;
        goto cleanup_vulkan;
    }

    // Query the queues.
    DvzQueueCaps qc = {0};
    AT(dvz_instance_gpu_queue_caps(instance, vk_gpu_index, &qc));

    // Initialize a device.
    DvzQueues queues = {0};
    dvz_queues(&qc, &queues);
    DvzDeviceConfig dcfg = dvz_device_config(instance);
    dvz_device_config_set_gpu_index(&dcfg, vk_gpu_index);
    VkPhysicalDeviceVulkan12Features features12 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    features12.timelineSemaphore = true;
    dvz_device_config_set_features12(&dcfg, &features12);
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.synchronization2 = true;
    dvz_device_config_set_features13(&dcfg, &features13);
    for (uint32_t i = 0; i < queues.queue_count; i++)
    {
        DvzQueue* queue = &queues.queues[i];
        dvz_device_config_request_queue(&dcfg, queue->family_idx, 1);
    }
    // IMPORTANT: need external memory device extension.
    dvz_device_config_request_extension(&dcfg, VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME);
    dvz_device_config_request_extension(&dcfg, VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME);
    dvz_device_config_request_extension(&dcfg, VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME);
    device = dvz_device_create(&dcfg);
    if (device == NULL)
    {
        out = 1;
        goto cleanup_vulkan;
    }
    if (!dvz_device_has_extension(device, VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME))
    {
        log_debug("test_memory_cuda_1 skipped: Vulkan external semaphore FD unsupported");
        tst_skip(suite, "external semaphore FD unsupported");
        out = 0;
        goto cleanup_vulkan;
    }

    interop_semaphore = dvz_semaphore_create_wrapper();
    ANN(interop_semaphore);
    dvz_semaphore_timeline(
        device, 0, interop_semaphore, VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT);
    semaphore_fd =
        dvz_semaphore_export_fd(interop_semaphore, VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT);
    if (semaphore_fd < 0)
    {
        log_error("failed to export Vulkan timeline semaphore FD");
        out = 1;
        goto cleanup_vulkan;
    }

    struct cudaExternalSemaphoreHandleDesc sem_desc = {0};
    sem_desc.type = cudaExternalSemaphoreHandleTypeTimelineSemaphoreFd;
    sem_desc.handle.fd = semaphore_fd;
    cerr = cudaImportExternalSemaphore(&cuda_semaphore, &sem_desc);
    if (cerr != cudaSuccess)
    {
        log_error("cudaImportExternalSemaphore failed: %s", cudaGetErrorString(cerr));
        out = 1;
        goto cleanup_vulkan;
    }
    // CUDA assumes ownership of the opaque semaphore FD after successful import on Linux.
    semaphore_fd = -1;

    // Memory allocator.
    allocator = dvz_allocator_create();
    ANN(allocator);
    // IMPORTANT: need to pass the external memory handle type when creating the allocator.
    dvz_device_allocator(device, handle_type, allocator);

    // Create a device-local exportable buffer.
    VkBufferCreateInfo buf_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = SIZE,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    };
    alloc = dvz_allocation_create();
    ANN(alloc);
    out = dvz_allocator_buffer(allocator, &buf_info, DVZ_ALLOC_DEDICATED_MEMORY, alloc, &vk_buffer);
    if (out != 0)
    {
        log_error("failed to create exportable Vulkan buffer");
        goto cleanup_vulkan;
    }
    const VkDeviceSize allocation_size = dvz_allocation_size(alloc);
    if (allocation_size < SIZE)
    {
        log_error("exportable Vulkan buffer allocation is smaller than requested");
        out = 1;
        goto cleanup_vulkan;
    }

    VkBufferCreateInfo staging_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = SIZE,
        .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
    };
    staging_read_alloc = dvz_allocation_create();
    ANN(staging_read_alloc);
    out = dvz_allocator_buffer(
        allocator, &staging_info, DVZ_ALLOC_HOST_ACCESS_RANDOM, staging_read_alloc,
        &staging_read_buffer);
    if (out != 0)
    {
        log_error("failed to create Vulkan staging readback buffer");
        goto cleanup_vulkan;
    }

    staging_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    staging_write_alloc = dvz_allocation_create();
    ANN(staging_write_alloc);
    out = dvz_allocator_buffer(
        allocator, &staging_info, DVZ_ALLOC_HOST_ACCESS_SEQUENTIAL_WRITE, staging_write_alloc,
        &staging_write_buffer);
    if (out != 0)
    {
        log_error("failed to create Vulkan staging upload buffer");
        goto cleanup_vulkan;
    }

    /******************* Initialize data on Vulkan side *******************/
    log_trace("staging initial data to the exportable buffer");
    uint32_t* host_init = (uint32_t*)dvz_malloc(SIZE);
    ANN(host_init);
    for (uint32_t i = 0; i < N; i++)
        host_init[i] = i;
    if (dvz_allocator_copy_to(allocator, staging_write_alloc, 0, host_init, SIZE) != 0)
    {
        dvz_free(host_init);
        log_error("failed to write Vulkan staging upload buffer");
        out = 1;
        goto cleanup_vulkan;
    }
    dvz_free(host_init);
    if (!_cuda_import_copy_buffer(
            device, staging_write_buffer, vk_buffer, SIZE, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 0))
    {
        out = 1;
        goto cleanup_vulkan;
    }
    log_trace("data copied");

    /******************* Export memory FD *******************/
    dvz_allocator_export(allocator, alloc, &fd);
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
    handle_desc.size = allocation_size;

    cerr = cudaImportExternalMemory(&cuda_mem, &handle_desc);
    if (cerr != cudaSuccess)
    {
        log_error("cudaImportExternalMemory failed: %s", cudaGetErrorString(cerr));
        goto cleanup_fd;
    }
    // CUDA assumes ownership of the opaque FD after successful import on Linux.
    fd = -1;

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
    struct cudaExternalSemaphoreSignalParams signal_params = {0};
    signal_params.params.fence.value = 1;
    cerr = cudaSignalExternalSemaphoresAsync(&cuda_semaphore, &signal_params, 1, 0);
    if (cerr != cudaSuccess)
    {
        log_error("cudaSignalExternalSemaphoresAsync failed: %s", cudaGetErrorString(cerr));
        out = 1;
        goto cleanup_cuda_mem;
    }
    cerr = cudaDeviceSynchronize();
    if (cerr != cudaSuccess)
    {
        log_error("cudaDeviceSynchronize failed: %s", cudaGetErrorString(cerr));
        out = 1;
        goto cleanup_cuda_mem;
    }

    /******************* Check result from Vulkan side after semaphore wait *******************/
    if (!_cuda_import_copy_buffer(
            device, vk_buffer, staging_read_buffer, SIZE, dvz_semaphore_handle(interop_semaphore),
            1, VK_NULL_HANDLE, 0))
    {
        out = 1;
        goto cleanup_cuda_mem;
    }
    uint32_t* host_verify = (uint32_t*)dvz_malloc(SIZE);
    ANN(host_verify);
    if (dvz_allocator_copy_from(allocator, staging_read_alloc, 0, host_verify, SIZE) != 0)
    {
        log_error("failed to read Vulkan staging buffer");
        dvz_free(host_verify);
        out = 1;
        goto cleanup_cuda_mem;
    }
    for (uint32_t i = 0; i < N; i++)
    {
        if (host_verify[i] != i + 1)
        {
            log_error(
                "Mismatch after CUDA write at %u: got %u expected %u", i, host_verify[i], i + 1);
            out = 1;
            break;
        }
    }
    dvz_free(host_verify);
    if (out == 0)
        log_debug("Vulkan->CUDA semaphore path verified OK (CUDA write visible in Vulkan)");
    else
        goto cleanup_cuda_mem;

    /******************* Vulkan modifies data again *******************/
    uint32_t* host_upload = (uint32_t*)dvz_malloc(SIZE);
    ANN(host_upload);
    for (uint32_t i = 0; i < N; i++)
        host_upload[i] = i + 2;
    if (dvz_allocator_copy_to(allocator, staging_write_alloc, 0, host_upload, SIZE) != 0)
    {
        dvz_free(host_upload);
        log_error("failed to write Vulkan staging upload buffer");
        out = 2;
        goto cleanup_cuda_mem;
    }
    dvz_free(host_upload);
    if (!_cuda_import_copy_buffer(
            device, staging_write_buffer, vk_buffer, SIZE, VK_NULL_HANDLE, 0,
            dvz_semaphore_handle(interop_semaphore), 2))
    {
        out = 2;
        goto cleanup_cuda_mem;
    }

    /******************* CUDA reads and checks *******************/
    struct cudaExternalSemaphoreWaitParams wait_params = {0};
    wait_params.params.fence.value = 2;
    cerr = cudaWaitExternalSemaphoresAsync(&cuda_semaphore, &wait_params, 1, 0);
    if (cerr != cudaSuccess)
    {
        log_error("cudaWaitExternalSemaphoresAsync failed: %s", cudaGetErrorString(cerr));
        out = 2;
        goto cleanup_cuda_mem;
    }
    cerr = cudaDeviceSynchronize();
    if (cerr != cudaSuccess)
    {
        log_error("cudaDeviceSynchronize failed: %s", cudaGetErrorString(cerr));
        out = 2;
        goto cleanup_cuda_mem;
    }
    uint32_t* host_copy = (uint32_t*)dvz_malloc(SIZE);
    ANN(host_copy);
    cerr = cudaMemcpy(host_copy, cuda_ptr, SIZE, cudaMemcpyDeviceToHost);
    if (cerr != cudaSuccess)
    {
        log_error("cudaMemcpyDeviceToHost failed: %s", cudaGetErrorString(cerr));
        out = 2;
    }
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
            log_debug("CUDA->Vulkan->CUDA sync verified OK (Vulkan write visible in CUDA)");
    }
    dvz_free(host_copy);

    /******************* Cleanup *******************/
cleanup_cuda_mem:
    cudaDestroyExternalMemory(cuda_mem);
cleanup_fd:
#if OS_UNIX
    if (fd >= 0)
        close(fd);
#endif
cleanup_vulkan:
    if (cuda_semaphore != NULL)
    {
        cudaDestroyExternalSemaphore(cuda_semaphore);
        cuda_semaphore = NULL;
    }
#if OS_UNIX
    if (semaphore_fd >= 0)
        close(semaphore_fd);
#endif
    if (staging_read_buffer != VK_NULL_HANDLE)
        dvz_allocator_destroy_buffer(allocator, staging_read_alloc, staging_read_buffer);
    if (staging_read_alloc != NULL)
        dvz_allocation_free(staging_read_alloc);
    if (staging_write_buffer != VK_NULL_HANDLE)
        dvz_allocator_destroy_buffer(allocator, staging_write_alloc, staging_write_buffer);
    if (staging_write_alloc != NULL)
        dvz_allocation_free(staging_write_alloc);
    if (alloc != NULL)
    {
        dvz_allocator_destroy_buffer(allocator, alloc, vk_buffer);
        dvz_allocation_free(alloc);
    }
    if (interop_semaphore != NULL)
    {
        dvz_semaphore_destroy(interop_semaphore);
        dvz_semaphore_free(interop_semaphore);
    }
    if (allocator != NULL)
    {
        dvz_allocator_destroy(allocator);
        dvz_allocator_free(allocator);
    }
    if (device != NULL)
    {
        dvz_device_destroy(device);
    }
    if (instance != NULL)
    {
        dvz_instance_destroy(instance);
    }

    return out;
#else
    log_debug("test_memory_cuda skipped because DVZ_HAS_CUDA=0");
    tst_skip(suite, "CUDA support unavailable");
    return 0;
#endif
}



int test_memory_cuda_2(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

#if DVZ_HAS_CUDA
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

    DvzInstance* instance = NULL;
    DvzDevice* device = NULL;
    DvzVma* allocator = NULL;
    DvzAllocation* imported_alloc = NULL;
    DvzAllocation* staging_read_alloc = NULL;
    DvzAllocation* staging_write_alloc = NULL;
    VkBuffer imported_buffer = VK_NULL_HANDLE;
    VkBuffer staging_read_buffer = VK_NULL_HANDLE;
    VkBuffer staging_write_buffer = VK_NULL_HANDLE;
    DvzSemaphore* interop_semaphore = NULL;
    CUexternalSemaphore cuda_semaphore = NULL;
    int semaphore_fd = -1;

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

    uint32_t* host_init = (uint32_t*)dvz_malloc(SIZE);
    if (host_init == NULL)
    {
        log_error("unable to allocate host staging buffer for CUDA initialization");
        out = 1;
        goto cleanup;
    }
    for (uint32_t i = 0; i < N; i++)
        host_init[i] = i * 7 + 3;

    curet = cuMemcpyHtoD(cuda_ptr, host_init, SIZE);
    dvz_free(host_init);
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
    DvzInstanceConfig icfg = dvz_instance_config();
    icfg.flags = 0;
    dvz_instance_config_request_extension(
        &icfg, VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME);
    instance = dvz_instance_create(&icfg);
    if (instance == NULL)
    {
        out = 1;
        goto cleanup;
    }

    uint32_t vk_gpu_index = UINT32_MAX;
    if (!_cuda_import_find_vulkan_gpu(instance, cu_device, &vk_gpu_index))
    {
        log_debug("test_memory_cuda_2 skipped: no Vulkan physical device matches CUDA device 0");
        tst_skip(suite, "no Vulkan physical device matches CUDA device 0");
        out = 0;
        goto cleanup;
    }

    DvzQueueCaps qc = {0};
    AT(dvz_instance_gpu_queue_caps(instance, vk_gpu_index, &qc));

    DvzQueues queues = {0};
    dvz_queues(&qc, &queues);
    DvzDeviceConfig dcfg = dvz_device_config(instance);
    dvz_device_config_set_gpu_index(&dcfg, vk_gpu_index);
    VkPhysicalDeviceVulkan12Features features12 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    features12.timelineSemaphore = true;
    dvz_device_config_set_features12(&dcfg, &features12);
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.synchronization2 = true;
    dvz_device_config_set_features13(&dcfg, &features13);
    for (uint32_t i = 0; i < queues.queue_count; i++)
    {
        DvzQueue* queue = &queues.queues[i];
        dvz_device_config_request_queue(&dcfg, queue->family_idx, 1);
    }
    dvz_device_config_request_extension(&dcfg, VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME);
    dvz_device_config_request_extension(&dcfg, VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME);
    dvz_device_config_request_extension(&dcfg, VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME);
    device = dvz_device_create(&dcfg);
    if (device == NULL)
    {
        out = 1;
        goto cleanup;
    }
    if (!dvz_device_has_extension(device, VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME))
    {
        log_debug("test_memory_cuda_2 skipped: Vulkan external semaphore FD unsupported");
        tst_skip(suite, "external semaphore FD unsupported");
        out = 0;
        goto cleanup_vulkan;
    }

    interop_semaphore = dvz_semaphore_create_wrapper();
    ANN(interop_semaphore);
    dvz_semaphore_timeline(
        device, 0, interop_semaphore, VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT);
    semaphore_fd =
        dvz_semaphore_export_fd(interop_semaphore, VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT);
    if (semaphore_fd < 0)
    {
        log_error("failed to export Vulkan timeline semaphore FD");
        out = 1;
        goto cleanup_vulkan;
    }

    CUDA_EXTERNAL_SEMAPHORE_HANDLE_DESC sem_desc = {
        .type = CU_EXTERNAL_SEMAPHORE_HANDLE_TYPE_TIMELINE_SEMAPHORE_FD,
        .handle = {.fd = semaphore_fd},
    };
    if (cuda_check(
            cuImportExternalSemaphore(&cuda_semaphore, &sem_desc), "cuImportExternalSemaphore"))
    {
        out = 1;
        goto cleanup_vulkan;
    }
    semaphore_fd = -1;

    allocator = dvz_allocator_create();
    ANN(allocator);
    dvz_device_allocator(device, handle_type, allocator);

    VkMemoryFdPropertiesKHR fd_props = {.sType = VK_STRUCTURE_TYPE_MEMORY_FD_PROPERTIES_KHR};
    VkResult fd_res =
        vkGetMemoryFdPropertiesKHR(dvz_device_handle(device), handle_type, fd, &fd_props);
    if (fd_res != VK_SUCCESS)
    {
        log_debug(
            "test_memory_cuda_2 skipped: CUDA exported memory FD is not importable by Vulkan "
            "(vkGetMemoryFdPropertiesKHR=%d)",
            fd_res);
        tst_skip(suite, "CUDA exported memory FD is not importable by Vulkan");
        out = 0;
        goto cleanup_vulkan;
    }

    imported_alloc = dvz_allocation_create();
    ANN(imported_alloc);
    VkBufferCreateInfo buf_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = alloc_size,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    };
    out = dvz_allocator_import_buffer(
        allocator, &buf_info, DVZ_ALLOC_DEDICATED_MEMORY, fd, imported_alloc, &imported_buffer);
    if (out != 0)
    {
        log_error("dvz_allocator_import_buffer failed");
        goto cleanup_vulkan;
    }

    // Successful Vulkan FD import transfers ownership of the file descriptor to Vulkan.
    fd = -1;

    VkBufferCreateInfo staging_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = SIZE,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
    };

    staging_read_alloc = dvz_allocation_create();
    ANN(staging_read_alloc);
    out = dvz_allocator_buffer(
        allocator, &staging_info, DVZ_ALLOC_HOST_ACCESS_RANDOM, staging_read_alloc,
        &staging_read_buffer);
    if (out != 0)
    {
        log_error("failed to create Vulkan staging readback buffer");
        goto cleanup_vulkan;
    }

    staging_write_alloc = dvz_allocation_create();
    ANN(staging_write_alloc);
    out = dvz_allocator_buffer(
        allocator, &staging_info, DVZ_ALLOC_HOST_ACCESS_RANDOM, staging_write_alloc,
        &staging_write_buffer);
    if (out != 0)
    {
        log_error("failed to create Vulkan staging upload buffer");
        goto cleanup_vulkan;
    }

    /******************* Validate Vulkan view through a staging copy *******************/
    CUDA_EXTERNAL_SEMAPHORE_SIGNAL_PARAMS cuda_signal_params = {0};
    cuda_signal_params.params.fence.value = 1;
    if (cuda_check(
            cuSignalExternalSemaphoresAsync(&cuda_semaphore, &cuda_signal_params, 1, 0),
            "cuSignalExternalSemaphoresAsync"))
    {
        out = 2;
        goto cleanup_vulkan;
    }
    if (cuda_check(cuCtxSynchronize(), "cuCtxSynchronize (after semaphore signal)"))
    {
        out = 2;
        goto cleanup_vulkan;
    }

    if (!_cuda_import_copy_buffer(
            device, imported_buffer, staging_read_buffer, SIZE,
            dvz_semaphore_handle(interop_semaphore), 1, VK_NULL_HANDLE, 0))
    {
        out = 2;
        goto cleanup_vulkan;
    }

    uint32_t* host_verify = (uint32_t*)dvz_malloc(SIZE);
    if (host_verify == NULL)
    {
        log_error("unable to allocate host buffer for Vulkan readback verification");
        out = 2;
        goto cleanup_vulkan;
    }
    if (dvz_allocator_copy_from(allocator, staging_read_alloc, 0, host_verify, SIZE) != 0)
    {
        log_error("failed to read Vulkan staging buffer");
        dvz_free(host_verify);
        out = 2;
        goto cleanup_vulkan;
    }

    for (uint32_t i = 0; i < N; i++)
    {
        uint32_t expected = i * 7 + 3;
        if (host_verify[i] != expected)
        {
            log_error(
                "Mismatch right after import at %u: got %u expected %u", i, host_verify[i],
                expected);
            out = 2;
            break;
        }
    }
    dvz_free(host_verify);
    host_verify = NULL;
    if (out != 0)
        goto cleanup_vulkan;

    /******************* Modify from Vulkan through a staging upload *******************/
    uint32_t* host_upload = (uint32_t*)dvz_malloc(SIZE);
    if (host_upload == NULL)
    {
        log_error("unable to allocate host staging buffer for Vulkan upload");
        out = 3;
        goto cleanup_vulkan;
    }
    for (uint32_t i = 0; i < N; i++)
        host_upload[i] = i * 7 + 3 + vulkan_delta;
    if (dvz_allocator_copy_to(allocator, staging_write_alloc, 0, host_upload, SIZE) != 0)
    {
        log_error("failed to write Vulkan staging buffer");
        dvz_free(host_upload);
        out = 3;
        goto cleanup_vulkan;
    }
    dvz_free(host_upload);
    host_upload = NULL;

    if (!_cuda_import_copy_buffer(
            device, staging_write_buffer, imported_buffer, SIZE, VK_NULL_HANDLE, 0,
            dvz_semaphore_handle(interop_semaphore), 2))
    {
        out = 3;
        goto cleanup_vulkan;
    }

    /******************* Check from CUDA *******************/
    CUDA_EXTERNAL_SEMAPHORE_WAIT_PARAMS cuda_wait_params = {0};
    cuda_wait_params.params.fence.value = 2;
    if (cuda_check(
            cuWaitExternalSemaphoresAsync(&cuda_semaphore, &cuda_wait_params, 1, 0),
            "cuWaitExternalSemaphoresAsync"))
    {
        out = 3;
        goto cleanup_vulkan;
    }
    if (cuda_check(cuCtxSynchronize(), "cuCtxSynchronize (after semaphore wait)"))
    {
        out = 3;
        goto cleanup_vulkan;
    }

    host_verify = (uint32_t*)dvz_malloc(SIZE);
    if (host_verify == NULL)
    {
        log_error("unable to allocate host buffer for CUDA verification");
        out = 3;
        goto cleanup_vulkan;
    }
    curet = cuMemcpyDtoH(host_verify, cuda_ptr, SIZE);
    if (cuda_check(curet, "cuMemcpyDtoH"))
    {
        dvz_free(host_verify);
        out = 3;
        goto cleanup_vulkan;
    }

    curet = cuCtxSynchronize();
    if (cuda_check(curet, "cuCtxSynchronize (after device readback)"))
    {
        dvz_free(host_verify);
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
        log_debug("CUDA->Vulkan->CUDA import path verified OK");

    dvz_free(host_verify);

cleanup_vulkan:
    if (cuda_semaphore != NULL)
    {
        cuDestroyExternalSemaphore(cuda_semaphore);
        cuda_semaphore = NULL;
    }
    if (semaphore_fd >= 0)
    {
        close(semaphore_fd);
    }
    if (staging_write_buffer != VK_NULL_HANDLE)
        dvz_allocator_destroy_buffer(allocator, staging_write_alloc, staging_write_buffer);
    if (staging_write_alloc != NULL)
        dvz_allocation_free(staging_write_alloc);
    if (staging_read_buffer != VK_NULL_HANDLE)
        dvz_allocator_destroy_buffer(allocator, staging_read_alloc, staging_read_buffer);
    if (staging_read_alloc != NULL)
        dvz_allocation_free(staging_read_alloc);
    if (imported_buffer != VK_NULL_HANDLE)
        dvz_allocator_destroy_buffer(allocator, imported_alloc, imported_buffer);
    if (imported_alloc != NULL)
        dvz_allocation_free(imported_alloc);
    if (interop_semaphore != NULL)
    {
        dvz_semaphore_destroy(interop_semaphore);
        dvz_semaphore_free(interop_semaphore);
    }
    if (allocator != NULL)
    {
        dvz_allocator_destroy(allocator);
        dvz_allocator_free(allocator);
    }
    if (device != NULL)
    {
        dvz_device_destroy(device);
    }
    if (instance != NULL)
    {
        dvz_instance_destroy(instance);
    }

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
    log_debug("test_memory_cuda_2 skipped because DVZ_HAS_CUDA=0");
    tst_skip(suite, "CUDA support unavailable");
    return 0;
#endif
}
