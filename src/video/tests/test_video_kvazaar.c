/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Kvazaar (CPU fallback) video tests                                                           */
/*************************************************************************************************/

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <volk.h>

#include "../encoder.h"
#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "datoviz/common/macros.h"
#include "datoviz/vk/device.h"
#include "datoviz/vk/gpu.h"
#include "test_video.h"
#include "test_video_common.h"
#include "testing.h"

#if !DVZ_HAS_KVZ
#error "test_video_kvazaar.c should only be compiled when DVZ_HAS_KVZ=ON"
#endif

/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

typedef struct
{
    VkInstance instance;
    VkPhysicalDevice phys;
    VkDevice device;
    uint32_t queue_family;
    VkQueue queue;
    VkCommandPool cmd_pool;
    VkCommandBuffer cmd;
    VkImage image;
    VkDeviceMemory memory;
    VkDeviceSize memory_size;
    VkImageLayout image_layout;
} KvzCpuCtx;

static bool kvz_cpu_pick_memory(
    VkPhysicalDevice phys, uint32_t type_bits, VkMemoryPropertyFlags flags, uint32_t* index)
{
    VkPhysicalDeviceMemoryProperties props;
    vkGetPhysicalDeviceMemoryProperties(phys, &props);
    for (uint32_t i = 0; i < props.memoryTypeCount; ++i)
    {
        if ((type_bits & (1u << i)) && (props.memoryTypes[i].propertyFlags & flags) == flags)
        {
            *index = i;
            return true;
        }
    }
    return false;
}

static void kvz_cpu_ctx_destroy(KvzCpuCtx* ctx)
{
    if (!ctx)
    {
        return;
    }
    if (ctx->device != VK_NULL_HANDLE)
    {
        vkDeviceWaitIdle(ctx->device);
    }
    if (ctx->memory != VK_NULL_HANDLE)
    {
        vkFreeMemory(ctx->device, ctx->memory, NULL);
        ctx->memory = VK_NULL_HANDLE;
    }
    if (ctx->image != VK_NULL_HANDLE)
    {
        vkDestroyImage(ctx->device, ctx->image, NULL);
        ctx->image = VK_NULL_HANDLE;
    }
    if (ctx->cmd_pool != VK_NULL_HANDLE)
    {
        if (ctx->cmd != VK_NULL_HANDLE)
        {
            vkFreeCommandBuffers(ctx->device, ctx->cmd_pool, 1, &ctx->cmd);
            ctx->cmd = VK_NULL_HANDLE;
        }
        vkDestroyCommandPool(ctx->device, ctx->cmd_pool, NULL);
        ctx->cmd_pool = VK_NULL_HANDLE;
    }
    if (ctx->device != VK_NULL_HANDLE)
    {
        vkDestroyDevice(ctx->device, NULL);
        ctx->device = VK_NULL_HANDLE;
    }
    if (ctx->instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(ctx->instance, NULL);
        ctx->instance = VK_NULL_HANDLE;
    }
    dvz_memset(ctx, sizeof(*ctx), 0, sizeof(*ctx));
}

static bool kvz_cpu_ctx_init(KvzCpuCtx* ctx)
{
    ANN(ctx);
    if (volkInitialize() != VK_SUCCESS)
    {
        return false;
    }
    dvz_memset(ctx, sizeof(*ctx), 0, sizeof(*ctx));

    VkApplicationInfo app = {.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "kvazaar_cpu_test";
    app.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo ici = {.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    ici.pApplicationInfo = &app;
    if (vkCreateInstance(&ici, NULL, &ctx->instance) != VK_SUCCESS)
    {
        return false;
    }
    volkLoadInstance(ctx->instance);

    uint32_t gpu_count = 0;
    if (vkEnumeratePhysicalDevices(ctx->instance, &gpu_count, NULL) != VK_SUCCESS ||
        gpu_count == 0)
    {
        return false;
    }
    VkPhysicalDevice* devices =
        (VkPhysicalDevice*)dvz_malloc(sizeof(VkPhysicalDevice) * gpu_count);
    if (!devices)
    {
        return false;
    }
    if (vkEnumeratePhysicalDevices(ctx->instance, &gpu_count, devices) != VK_SUCCESS ||
        gpu_count == 0)
    {
        dvz_free(devices);
        return false;
    }
    ctx->phys = devices[0];
    dvz_free(devices);

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(ctx->phys, &queue_family_count, NULL);
    VkQueueFamilyProperties* queue_props = (VkQueueFamilyProperties*)dvz_malloc(
        queue_family_count * sizeof(VkQueueFamilyProperties));
    if (!queue_props)
    {
        return false;
    }
    vkGetPhysicalDeviceQueueFamilyProperties(ctx->phys, &queue_family_count, queue_props);
    ctx->queue_family = 0;
    for (uint32_t i = 0; i < queue_family_count; ++i)
    {
        if (queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            ctx->queue_family = i;
            break;
        }
    }
    dvz_free(queue_props);

    float priority = 1.0f;
    VkDeviceQueueCreateInfo queue_info = {.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queue_info.queueFamilyIndex = ctx->queue_family;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = &priority;

    VkDeviceCreateInfo device_info = {.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    device_info.queueCreateInfoCount = 1;
    device_info.pQueueCreateInfos = &queue_info;
    if (vkCreateDevice(ctx->phys, &device_info, NULL, &ctx->device) != VK_SUCCESS)
    {
        return false;
    }
    volkLoadDevice(ctx->device);
    vkGetDeviceQueue(ctx->device, ctx->queue_family, 0, &ctx->queue);

    VkCommandPoolCreateInfo pool_info = {.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pool_info.queueFamilyIndex = ctx->queue_family;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    if (vkCreateCommandPool(ctx->device, &pool_info, NULL, &ctx->cmd_pool) != VK_SUCCESS)
    {
        return false;
    }

    VkCommandBufferAllocateInfo cmd_alloc = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cmd_alloc.commandPool = ctx->cmd_pool;
    cmd_alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_alloc.commandBufferCount = 1;
    if (vkAllocateCommandBuffers(ctx->device, &cmd_alloc, &ctx->cmd) != VK_SUCCESS)
    {
        return false;
    }

    VkImageCreateInfo image_info = {.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = VK_FORMAT_R8G8B8A8_UNORM;
    image_info.extent.width = DVZ_TEST_VIDEO_WIDTH;
    image_info.extent.height = DVZ_TEST_VIDEO_HEIGHT;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.tiling = VK_IMAGE_TILING_LINEAR;
    image_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (vkCreateImage(ctx->device, &image_info, NULL, &ctx->image) != VK_SUCCESS)
    {
        return false;
    }

    VkMemoryRequirements mem_req;
    vkGetImageMemoryRequirements(ctx->device, ctx->image, &mem_req);
    uint32_t mem_index = 0;
    if (!kvz_cpu_pick_memory(
            ctx->phys, mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &mem_index))
    {
        return false;
    }
    VkMemoryAllocateInfo alloc_info = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    alloc_info.allocationSize = mem_req.size;
    alloc_info.memoryTypeIndex = mem_index;
    if (vkAllocateMemory(ctx->device, &alloc_info, NULL, &ctx->memory) != VK_SUCCESS)
    {
        return false;
    }
    if (vkBindImageMemory(ctx->device, ctx->image, ctx->memory, 0) != VK_SUCCESS)
    {
        return false;
    }
    ctx->memory_size = mem_req.size;
    ctx->image_layout = VK_IMAGE_LAYOUT_UNDEFINED;

    return true;
}

static bool kvz_cpu_record_clear(KvzCpuCtx* ctx, const VkClearColorValue* clr)
{
    ANN(ctx);
    ANN(clr);
    VkCommandBufferBeginInfo begin_info = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (vkBeginCommandBuffer(ctx->cmd, &begin_info) != VK_SUCCESS)
    {
        return false;
    }
    VkImageMemoryBarrier barrier = {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.oldLayout = ctx->image_layout;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.image = ctx->image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(
        ctx->cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0,
        NULL, 1, &barrier);

    VkImageSubresourceRange range = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1};
    vkCmdClearColorImage(
        ctx->cmd, ctx->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, clr, 1, &range);

    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    vkCmdPipelineBarrier(
        ctx->cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL,
        0, NULL, 1, &barrier);

    if (vkEndCommandBuffer(ctx->cmd) != VK_SUCCESS)
    {
        return false;
    }

    VkCommandBufferSubmitInfo cmd_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .commandBuffer = ctx->cmd,
    };
    VkSubmitInfo2 submit = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &cmd_info,
    };
    if (vkQueueSubmit2(ctx->queue, 1, &submit, VK_NULL_HANDLE) != VK_SUCCESS)
    {
        return false;
    }
    if (vkQueueWaitIdle(ctx->queue) != VK_SUCCESS)
    {
        return false;
    }
    ctx->image_layout = VK_IMAGE_LAYOUT_GENERAL;
    return true;
}

/*************************************************************************************************/
/*  Test                                                                                         */
/*************************************************************************************************/

int test_video_kvazaar(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    KvzCpuCtx ctx = {0};
    if (!kvz_cpu_ctx_init(&ctx))
    {
        log_warn("kvazaar CPU fallback test skipped (unable to initialize Vulkan)");
        kvz_cpu_ctx_destroy(&ctx);
        return 0;
    }

    int rc = 0;
    const char* mp4_path = "video_kvazaar.mp4";
    DvzDevice* device = (DvzDevice*)dvz_calloc(1, sizeof(DvzDevice));
    DvzGpu* gpu = (DvzGpu*)dvz_calloc(1, sizeof(DvzGpu));
    if (!device || !gpu)
    {
        log_error("failed to allocate temporary dvz device for kvazaar test");
        dvz_free(device);
        dvz_free(gpu);
        kvz_cpu_ctx_destroy(&ctx);
        return 1;
    }
    device->vk_device = ctx.device;
    device->gpu = gpu;
    gpu->pdevice = ctx.phys;

    DvzVideoEncoderConfig cfg = dvz_video_encoder_default_config();
    cfg.width = DVZ_TEST_VIDEO_WIDTH;
    cfg.height = DVZ_TEST_VIDEO_HEIGHT;
    cfg.fps = DVZ_TEST_VIDEO_FPS;
    cfg.codec = DVZ_VIDEO_CODEC_HEVC;
    cfg.mux = DVZ_VIDEO_MUX_MP4_STREAMING;
    cfg.mp4_path = mp4_path;
    cfg.backend = "kvazaar";
    cfg.raw_path = NULL;

    DvzVideoEncoder* encoder = dvz_video_encoder_create(device, &cfg);
    if (!encoder)
    {
        log_error("failed to create kvazaar encoder");
        rc = 1;
        goto cleanup;
    }
    if (dvz_video_encoder_start(encoder, ctx.image, ctx.memory, ctx.memory_size, -1, -1, NULL) !=
        0)
    {
        rc = 1;
        goto cleanup;
    }
    for (uint32_t frame = 0; frame < DVZ_TEST_VIDEO_FRAMES; ++frame)
    {
        VkClearColorValue clr = dvz_test_video_clear_color(frame, DVZ_TEST_VIDEO_FRAMES);
        if (!kvz_cpu_record_clear(&ctx, &clr))
        {
            rc = 1;
            goto cleanup;
        }
        if (dvz_video_encoder_submit(encoder, 0) != 0)
        {
            rc = 1;
            goto cleanup;
        }
        dvz_test_video_progress((int)(frame + 1), DVZ_TEST_VIDEO_FRAMES);
    }
    dvz_video_encoder_stop(encoder);
    dvz_video_encoder_destroy(encoder);
    encoder = NULL;

    FILE* fp = fopen(mp4_path, "rb");
    if (!fp)
    {
        log_error("kvazaar CPU test missing mp4 output");
        rc = 1;
        goto cleanup;
    }
    if (fseek(fp, 0, SEEK_END) != 0)
    {
        fclose(fp);
        rc = 1;
        goto cleanup;
    }
    long size = ftell(fp);
    fclose(fp);
    if (size <= 0)
    {
        log_error("kvazaar CPU test wrote empty mp4");
        rc = 1;
        goto cleanup;
    }

cleanup:
    if (encoder)
    {
        dvz_video_encoder_destroy(encoder);
    }
    dvz_free(device);
    dvz_free(gpu);
    kvz_cpu_ctx_destroy(&ctx);
    return rc;
}
