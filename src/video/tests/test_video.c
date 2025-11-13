/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing video                                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#define _GNU_SOURCE

#include <assert.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <float.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <volk.h>

#include "../../vk/macros.h"
#include "../../vk/tests/test_vk.h"
#include "../../vklite/tests/test_vklite.h"
#include "../encoder.h"
#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "datoviz/common/macros.h"
#include "datoviz/vk/device.h"
#include "datoviz/vk/gpu.h"
#include "datoviz/vk/macros.h"
#include "datoviz/vk/queues.h"
#include "datoviz/vklite/compute.h"
#include "datoviz/vklite/graphics.h"
#include "datoviz/vklite/images.h"
#include "datoviz/vklite/proto.h"
#include "datoviz/vklite/rendering.h"
#include "datoviz/vklite/sampler.h"
#include "datoviz/vklite/slots.h"
#include "test_video.h"
#include "testing.h"
#include "vulkan_core.h"



// ====== Params ======
// Video settings cheat sheet:
// - Prefer HEVC/H.265 when hardware encoders exist (NVENC, VideoToolbox, VA-API). Fall back to
// H.264
//   High profile for max compatibility or use kvazaar/x264 as CPU software encoders.
// - 1080p60 interactive captures: target 12-18 Mb/s (CQP 20/22/24 or CRF 18-20), GOP length = 2s,
// 2
//   B-frames. Expects ~90-135 MB per minute once muxed into MP4.
// - Social media masters (H.264 High profile):
//     * 1080p30: 8-10 Mb/s
//     * 1080p60: 12-15 Mb/s
//     * 4K30: 25-35 Mb/s (15-20 Mb/s if HEVC is allowed)
//     * Vertical 1080x1920@30: 6-8 Mb/s
// - Platform backends:
//     * Linux/Windows + NVIDIA: CUDA external memory + NVENC.
//     * macOS: MoltenVK-exported IOSurface → Metal blit/compute → VideoToolbox.
//     * Linux Intel/AMD: exportable VkImage → VA-API/AMF.
//     * CPU fallback: convert to planar YUV420 and feed kvazaar (BSD) or x264/x265 (GPL).
// - Audio/mux later; this harness simply emits Annex B bitstreams (out.h265).



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH   1920
#define HEIGHT  1080
#define FPS     30
#define SECONDS 5
#define NFRAMES (FPS * SECONDS)

// Solid color to clear with Vulkan (uint8)
#define CLEAR_R 0
#define CLEAR_G 128
#define CLEAR_B 255
#define CLEAR_A 255



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

// ====== Error helpers ======
#define VK_CHECK(x)                                                                               \
    do                                                                                            \
    {                                                                                             \
        VkResult _e = (x);                                                                        \
        if (_e != VK_SUCCESS)                                                                     \
        {                                                                                         \
            fprintf(stderr, "Vulkan error %d at %s:%d\n", _e, __FILE__, __LINE__);                \
            exit(1);                                                                              \
        }                                                                                         \
    } while (0)



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static void video_progress(int frame, int total)
{
    const int width = 40;
    float ratio = (total > 0) ? (float)frame / (float)total : 1.0f;
    if (ratio > 1.0f)
        ratio = 1.0f;
    int filled = (int)(ratio * width);
    printf("\r[");
    for (int i = 0; i < width; ++i)
    {
        putchar(i < filled ? '#' : ' ');
    }
    printf("] %d/%d", frame, total);
    fflush(stdout);
    if (frame >= total)
    {
        printf("\n");
    }
}



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct
{
    VkInstance instance;
    VkPhysicalDevice phys;
    VkDevice device;
    uint32_t queueFamily;
    VkQueue queue;
    VkCommandPool cmdPool;
    VkCommandBuffer cmd;
    VkImage image;
    VkDeviceMemory memory;
    int memory_fd;
    VkSemaphore semaphore;
    int semaphore_fd;
    VkFence fence;
    VkImageLayout image_layout;
    uint64_t timeline_value;
} VulkanCtx;



/*************************************************************************************************/
/*  Test video                                                                                   */
/*************************************************************************************************/

static VkClearColorValue frame_clear_color(uint32_t frame_idx, uint32_t total_frames)
{
    VkClearColorValue clr = {
        .float32 = {CLEAR_R / 255.0f, CLEAR_G / 255.0f, CLEAR_B / 255.0f, 1.0f}};
    if (total_frames == 0)
    {
        return clr;
    }

    float u = (float)(frame_idx % total_frames) / (float)total_frames;
    float h = u * 6.0f;
    uint32_t sector = (uint32_t)h;
    if (sector >= 6)
    {
        sector = 5;
    }
    float f = h - (float)sector;

    float r = 0.0f, g = 0.0f, b = 0.0f;
    switch (sector)
    {
    case 0:
        r = 1.0f;
        g = f;
        b = 0.0f;
        break;
    case 1:
        r = 1.0f - f;
        g = 1.0f;
        b = 0.0f;
        break;
    case 2:
        r = 0.0f;
        g = 1.0f;
        b = f;
        break;
    case 3:
        r = 0.0f;
        g = 1.0f - f;
        b = 1.0f;
        break;
    case 4:
        r = f;
        g = 0.0f;
        b = 1.0f;
        break;
    default:
        r = 1.0f;
        g = 0.0f;
        b = 1.0f - f;
        break;
    }

    clr.float32[0] = r;
    clr.float32[1] = g;
    clr.float32[2] = b;
    clr.float32[3] = 1.0f;
    return clr;
}

static void vk_init_and_make_image(VulkanCtx* vk);

static uint64_t vk_render_frame_and_sync(VulkanCtx* vk, const VkClearColorValue* clr);

static void vk_init_and_make_image(VulkanCtx* vk)
{
    VK_CHECK(volkInitialize());
    memset(vk, 0, sizeof(*vk));
    vk->memory_fd = -1;
    vk->semaphore_fd = -1;
    vk->timeline_value = 0;

    VkApplicationInfo app = {.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "vk_cuda_nvenc";
    app.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo ici = {.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    ici.pApplicationInfo = &app;
    VK_CHECK(vkCreateInstance(&ici, NULL, &vk->instance));
    volkLoadInstance(vk->instance);

    uint32_t np = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(vk->instance, &np, NULL));
    VkPhysicalDevice* pds = (VkPhysicalDevice*)malloc(sizeof(VkPhysicalDevice) * np);
    VK_CHECK(vkEnumeratePhysicalDevices(vk->instance, &np, pds));
    vk->phys = pds[0];
    free(pds);

    uint32_t nqf = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(vk->phys, &nqf, NULL);
    VkQueueFamilyProperties* qfp = (VkQueueFamilyProperties*)malloc(sizeof(*qfp) * nqf);
    vkGetPhysicalDeviceQueueFamilyProperties(vk->phys, &nqf, qfp);
    vk->queueFamily = 0;
    for (uint32_t i = 0; i < nqf; i++)
    {
        if (qfp[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            vk->queueFamily = i;
            break;
        }
    }
    free(qfp);

    float priority = 1.0f;
    VkDeviceQueueCreateInfo qci = {.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    qci.queueFamilyIndex = vk->queueFamily;
    qci.queueCount = 1;
    qci.pQueuePriorities = &priority;

    const char* dev_exts[] = {
        VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
        VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,
        VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME,
        VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME,
    };

    VkPhysicalDeviceVulkan13Features features_1_3 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .synchronization2 = VK_TRUE,
    };
    VkPhysicalDeviceTimelineSemaphoreFeatures timeline_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES,
        .pNext = &features_1_3,
        .timelineSemaphore = VK_TRUE,
    };

    VkDeviceCreateInfo dci = {.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    dci.queueCreateInfoCount = 1;
    dci.pQueueCreateInfos = &qci;
    dci.enabledExtensionCount = (uint32_t)(sizeof(dev_exts) / sizeof(dev_exts[0]));
    dci.ppEnabledExtensionNames = dev_exts;
    dci.pNext = &timeline_features;
    VK_CHECK(vkCreateDevice(vk->phys, &dci, NULL, &vk->device));
    volkLoadDevice(vk->device);

    if (!vkGetMemoryFdKHR || !vkGetSemaphoreFdKHR)
    {
        fprintf(stderr, "Required external memory/semaphore extensions not available\n");
        exit(1);
    }

    vkGetDeviceQueue(vk->device, vk->queueFamily, 0, &vk->queue);

    VkCommandPoolCreateInfo cpci = {.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    cpci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cpci.queueFamilyIndex = vk->queueFamily;
    VK_CHECK(vkCreateCommandPool(vk->device, &cpci, NULL, &vk->cmdPool));

    VkCommandBufferAllocateInfo cbai = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cbai.commandPool = vk->cmdPool;
    cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = 1;
    VK_CHECK(vkAllocateCommandBuffers(vk->device, &cbai, &vk->cmd));

    VkSemaphoreTypeCreateInfo timeline_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
        .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
        .initialValue = 0,
    };
    VkExportSemaphoreCreateInfo export_semaphore = {
        .sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO,
        .pNext = &timeline_info,
        .handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT,
    };
    VkSemaphoreCreateInfo sci = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    sci.pNext = &export_semaphore;
    VK_CHECK(vkCreateSemaphore(vk->device, &sci, NULL, &vk->semaphore));

    VkFenceCreateInfo fci = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    VK_CHECK(vkCreateFence(vk->device, &fci, NULL, &vk->fence));

    VkExternalMemoryImageCreateInfo emici = {
        .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO,
        .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT,
    };
    VkImageCreateInfo ici2 = {.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    ici2.pNext = &emici;
    ici2.imageType = VK_IMAGE_TYPE_2D;
    ici2.format = VK_FORMAT_R8G8B8A8_UNORM;
    ici2.extent.width = WIDTH;
    ici2.extent.height = HEIGHT;
    ici2.extent.depth = 1;
    ici2.mipLevels = 1;
    ici2.arrayLayers = 1;
    ici2.samples = VK_SAMPLE_COUNT_1_BIT;
    ici2.tiling = VK_IMAGE_TILING_OPTIMAL;
    ici2.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                 VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    ici2.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VK_CHECK(vkCreateImage(vk->device, &ici2, NULL, &vk->image));

    VkMemoryRequirements memReq;
    vkGetImageMemoryRequirements(vk->device, vk->image, &memReq);

    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(vk->phys, &memProps);
    uint32_t memTypeIdx = UINT32_MAX;
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++)
    {
        if ((memReq.memoryTypeBits & (1u << i)) &&
            (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
        {
            memTypeIdx = i;
            break;
        }
    }
    if (memTypeIdx == UINT32_MAX)
    {
        fprintf(stderr, "No suitable memory type\n");
        exit(1);
    }

    VkExportMemoryAllocateInfo emai = {.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO};
    emai.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;

    VkMemoryAllocateInfo mai = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    mai.pNext = &emai;
    mai.allocationSize = memReq.size;
    mai.memoryTypeIndex = memTypeIdx;

    VK_CHECK(vkAllocateMemory(vk->device, &mai, NULL, &vk->memory));
    VK_CHECK(vkBindImageMemory(vk->device, vk->image, vk->memory, 0));
    vk->image_layout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkMemoryGetFdInfoKHR getfd = {.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR};
    getfd.memory = vk->memory;
    getfd.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
    VK_CHECK(vkGetMemoryFdKHR(vk->device, &getfd, &vk->memory_fd));
    if (vk->memory_fd < 0)
    {
        fprintf(stderr, "Failed to export memory FD\n");
        exit(1);
    }

    VkSemaphoreGetFdInfoKHR sfd = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR};
    sfd.semaphore = vk->semaphore;
    sfd.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
    VK_CHECK(vkGetSemaphoreFdKHR(vk->device, &sfd, &vk->semaphore_fd));
}

static uint64_t vk_render_frame_and_sync(VulkanCtx* vk, const VkClearColorValue* clr)
{
    ANN(vk);
    ANNVK(vk->cmd);
    ANN(clr);

    VK_CHECK(vkWaitForFences(vk->device, 1, &vk->fence, VK_TRUE, UINT64_MAX));
    VK_CHECK(vkResetFences(vk->device, 1, &vk->fence));
    VK_CHECK(vkResetCommandBuffer(vk->cmd, 0));

    VkCommandBufferBeginInfo begin = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_CHECK(vkBeginCommandBuffer(vk->cmd, &begin));

    VkPipelineStageFlags2 src_stage = (vk->image_layout == VK_IMAGE_LAYOUT_UNDEFINED)
                                          ? VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT
                                          : VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    VkAccessFlags2 src_access = (vk->image_layout == VK_IMAGE_LAYOUT_UNDEFINED)
                                    ? 0
                                    : (VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT);

    VkImageMemoryBarrier2 pre = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = src_stage,
        .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .srcAccessMask = src_access,
        .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .oldLayout = vk->image_layout,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .image = vk->image,
        .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};
    VkDependencyInfo dep_pre = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &pre};
    vkCmdPipelineBarrier2(vk->cmd, &dep_pre);

    VkImageSubresourceRange range = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    vkCmdClearColorImage(vk->cmd, vk->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, clr, 1, &range);

    VkImageMemoryBarrier2 post = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_GENERAL,
        .image = vk->image,
        .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};
    VkDependencyInfo dep_post = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &post};
    vkCmdPipelineBarrier2(vk->cmd, &dep_post);

    VK_CHECK(vkEndCommandBuffer(vk->cmd));

    uint64_t signal_value = ++vk->timeline_value;
    VkTimelineSemaphoreSubmitInfo timeline_info = {
        .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
        .pNext = NULL,
        .waitSemaphoreValueCount = 0,
        .pWaitSemaphoreValues = NULL,
        .signalSemaphoreValueCount = (vk->semaphore != VK_NULL_HANDLE) ? 1u : 0u,
        .pSignalSemaphoreValues = (vk->semaphore != VK_NULL_HANDLE) ? &signal_value : NULL,
    };
    VkSemaphore signal_sems[1] = {vk->semaphore};
    VkSubmitInfo submit = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = (vk->semaphore != VK_NULL_HANDLE) ? &timeline_info : NULL,
        .commandBufferCount = 1,
        .pCommandBuffers = &vk->cmd,
        .signalSemaphoreCount = (vk->semaphore != VK_NULL_HANDLE) ? 1u : 0u,
        .pSignalSemaphores = (vk->semaphore != VK_NULL_HANDLE) ? signal_sems : NULL,
    };
    VK_CHECK(vkQueueSubmit(vk->queue, 1, &submit, vk->fence));

    vk->image_layout = VK_IMAGE_LAYOUT_GENERAL;
    return signal_value;
}



/*************************************************************************************************/
/*  NVENC video encode                                                                           */
/*************************************************************************************************/

int test_video_nvenc(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    int rc = 0;
    bool skip_encode = false;
    bool encoded = false;

    FILE* bitstream_fp = NULL;
    DvzVideoEncoder* encoder = NULL;

    VulkanCtx vk;
    vk_init_and_make_image(&vk);
    // -------------------------------------------------------------------------------------------------
    // Placeholder for Datoviz renderer hookup:
    //
    // At the moment we just allocate an exportable VkImage and fill it once. When integrating the
    // real Datoviz renderer, the Vulkan context above needs to be fed with the renderer-managed
    // device, command queues, and the offscreen color attachment you already render into.
    // Conceptually:
    //
    // ```c
    // DvzRenderer* renderer = init_datoviz_renderer(...);
    // DvzRenderPass* offscreen = dvz_renderer_offscreen_pass(renderer, WIDTH, HEIGHT);
    // VkImage offscreen_color = dvz_render_pass_color_attachment(offscreen, 0);
    // vk.image = offscreen_color;
    // vk.memory = dvz_renderer_export_memory(offscreen_color);
    // vk.memory_fd = dvz_renderer_export_fd(offscreen_color);
    // ```
    //
    // You keep ownership of the renderer; test_video_2 now feeds those handles to
    // dvz_video_encoder_* so it can import the memory once, keep CUDA/NVENC objects alive, and
    // consume whatever Datoviz draws.
    // -------------------------------------------------------------------------------------------------

    // Query allocation size for CUDA import
    VkMemoryRequirements memReq;
    vkGetImageMemoryRequirements(vk.device, vk.image, &memReq);

    DvzVideoEncoderConfig vcfg = dvz_video_encoder_default_config();
    vcfg.mp4_path = "video_nvenc.mp4";
    vcfg.raw_path = "video_nvenc.h26x";
    encoder = dvz_video_encoder_create(NULL, &vcfg);
    if (!encoder)
    {
        rc = 1;
        goto cleanup;
    }
    if (vcfg.mux == DVZ_VIDEO_MUX_MP4_POST || vcfg.mux == DVZ_VIDEO_MUX_NONE)
    {
        bitstream_fp = fopen(vcfg.raw_path, "wb");
        if (!bitstream_fp)
        {
            perror("fopen raw bitstream");
            rc = 1;
            goto cleanup;
        }
    }
    if (dvz_video_encoder_start(
            encoder, vk.image, vk.memory, memReq.size, vk.memory_fd, vk.semaphore_fd,
            bitstream_fp) != 0)
    {
        skip_encode = true;
        goto cleanup;
    }

    for (int frame = 0; frame < NFRAMES; ++frame)
    {
        // Render/copy path:
        // 1. Record and submit a command buffer that clears the offscreen image and transitions it
        // back
        //    to GENERAL layout so CUDA can read from it.
        // 2. Hand control to dvz_video_encoder_submit(), which copies the VkImage via CUDA and
        // feeds
        //    NVENC without re-importing resources.
        VkClearColorValue clr = frame_clear_color((uint32_t)frame, NFRAMES);
        uint64_t signal_value = vk_render_frame_and_sync(&vk, &clr);

        if (dvz_video_encoder_submit(encoder, signal_value) != 0)
        {
            rc = 1;
            goto cleanup;
        }

        video_progress(frame + 1, NFRAMES);
    }

    dvz_video_encoder_stop(encoder);
    encoded = true;

cleanup:
    if (encoder)
    {
        dvz_video_encoder_destroy(encoder);
        encoder = NULL;
    }
    if (vk.device != VK_NULL_HANDLE)
    {
        vkDeviceWaitIdle(vk.device);
    }
    if (bitstream_fp)
    {
        fclose(bitstream_fp);
        bitstream_fp = NULL;
    }
    if (vk.memory_fd >= 0)
        close(vk.memory_fd);
    if (vk.semaphore_fd >= 0)
        close(vk.semaphore_fd);
    if (vk.memory)
        vkFreeMemory(vk.device, vk.memory, NULL);
    if (vk.image)
        vkDestroyImage(vk.device, vk.image, NULL);
    if (vk.semaphore)
        vkDestroySemaphore(vk.device, vk.semaphore, NULL);
    if (vk.cmdPool)
        vkDestroyCommandPool(vk.device, vk.cmdPool, NULL);
    if (vk.fence)
        vkDestroyFence(vk.device, vk.fence, NULL);
    if (vk.device)
        vkDestroyDevice(vk.device, NULL);
    if (vk.instance)
        vkDestroyInstance(vk.instance, NULL);
    if (!skip_encode && encoded)
    {
        const char* mp4_path = vcfg.mp4_path ? vcfg.mp4_path : "out.mp4";
        fprintf(
            stderr, "Wrote %s (%dx%d @ %dfps, %d frames)\n", mp4_path, WIDTH, HEIGHT, FPS,
            NFRAMES);
    }
    if (skip_encode)
    {
        return 0;
    }
    return rc;
}

#if DVZ_HAS_KVZ

#define KVZ_CPU_WIDTH  WIDTH
#define KVZ_CPU_HEIGHT HEIGHT
#define KVZ_CPU_FPS    FPS
#define KVZ_CPU_FRAMES NFRAMES

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
    memset(ctx, 0, sizeof(*ctx));
}

static bool kvz_cpu_ctx_init(KvzCpuCtx* ctx)
{
    ANN(ctx);
    if (volkInitialize() != VK_SUCCESS)
    {
        return false;
    }
    memset(ctx, 0, sizeof(*ctx));

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
    VkPhysicalDevice* devices = (VkPhysicalDevice*)malloc(sizeof(VkPhysicalDevice) * gpu_count);
    if (!devices)
    {
        return false;
    }
    if (vkEnumeratePhysicalDevices(ctx->instance, &gpu_count, devices) != VK_SUCCESS ||
        gpu_count == 0)
    {
        free(devices);
        return false;
    }
    ctx->phys = devices[0];
    free(devices);

    uint32_t family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(ctx->phys, &family_count, NULL);
    VkQueueFamilyProperties* families =
        (VkQueueFamilyProperties*)malloc(sizeof(VkQueueFamilyProperties) * family_count);
    if (!families)
    {
        return false;
    }
    vkGetPhysicalDeviceQueueFamilyProperties(ctx->phys, &family_count, families);
    ctx->queue_family = 0;
    for (uint32_t i = 0; i < family_count; ++i)
    {
        if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            ctx->queue_family = i;
            break;
        }
    }
    free(families);

    float priority = 1.0f;
    VkDeviceQueueCreateInfo qci = {.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    qci.queueFamilyIndex = ctx->queue_family;
    qci.queueCount = 1;
    qci.pQueuePriorities = &priority;

    VkDeviceCreateInfo dci = {.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    dci.queueCreateInfoCount = 1;
    dci.pQueueCreateInfos = &qci;
    if (vkCreateDevice(ctx->phys, &dci, NULL, &ctx->device) != VK_SUCCESS)
    {
        return false;
    }
    volkLoadDevice(ctx->device);
    vkGetDeviceQueue(ctx->device, ctx->queue_family, 0, &ctx->queue);

    VkCommandPoolCreateInfo cpci = {.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    cpci.queueFamilyIndex = ctx->queue_family;
    cpci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    if (vkCreateCommandPool(ctx->device, &cpci, NULL, &ctx->cmd_pool) != VK_SUCCESS)
    {
        return false;
    }
    VkCommandBufferAllocateInfo cbai = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cbai.commandPool = ctx->cmd_pool;
    cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = 1;
    if (vkAllocateCommandBuffers(ctx->device, &cbai, &ctx->cmd) != VK_SUCCESS)
    {
        return false;
    }

    VkImageCreateInfo ici2 = {.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    ici2.imageType = VK_IMAGE_TYPE_2D;
    ici2.format = VK_FORMAT_R8G8B8A8_UNORM;
    ici2.extent.width = KVZ_CPU_WIDTH;
    ici2.extent.height = KVZ_CPU_HEIGHT;
    ici2.extent.depth = 1;
    ici2.mipLevels = 1;
    ici2.arrayLayers = 1;
    ici2.samples = VK_SAMPLE_COUNT_1_BIT;
    ici2.tiling = VK_IMAGE_TILING_LINEAR;
    ici2.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    ici2.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (vkCreateImage(ctx->device, &ici2, NULL, &ctx->image) != VK_SUCCESS)
    {
        return false;
    }

    VkMemoryRequirements mem_req;
    vkGetImageMemoryRequirements(ctx->device, ctx->image, &mem_req);
    uint32_t mem_index = UINT32_MAX;
    if (!kvz_cpu_pick_memory(
            ctx->phys, mem_req.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &mem_index))
    {
        log_error("kvazaar CPU test: no HOST_VISIBLE|HOST_COHERENT memory type available");
        return false;
    }

    VkMemoryAllocateInfo mai = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    mai.allocationSize = mem_req.size;
    mai.memoryTypeIndex = mem_index;
    if (vkAllocateMemory(ctx->device, &mai, NULL, &ctx->memory) != VK_SUCCESS)
    {
        return false;
    }
    ctx->memory_size = mem_req.size;
    if (vkBindImageMemory(ctx->device, ctx->image, ctx->memory, 0) != VK_SUCCESS)
    {
        return false;
    }
    ctx->image_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    return true;
}

static bool kvz_cpu_record_clear(KvzCpuCtx* ctx, const VkClearColorValue* clr)
{
    ANN(ctx);
    ANN(clr);
    if (vkResetCommandBuffer(ctx->cmd, 0) != VK_SUCCESS)
    {
        return false;
    }
    VkCommandBufferBeginInfo begin = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (vkBeginCommandBuffer(ctx->cmd, &begin) != VK_SUCCESS)
    {
        return false;
    }

    VkImageMemoryBarrier barrier1 = {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier1.oldLayout = ctx->image_layout;
    barrier1.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier1.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier1.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier1.image = ctx->image;
    barrier1.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier1.subresourceRange.baseMipLevel = 0;
    barrier1.subresourceRange.levelCount = 1;
    barrier1.subresourceRange.baseArrayLayer = 0;
    barrier1.subresourceRange.layerCount = 1;
    VkPipelineStageFlags src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    if (ctx->image_layout != VK_IMAGE_LAYOUT_UNDEFINED)
    {
        src_stage = VK_PIPELINE_STAGE_HOST_BIT;
        barrier1.srcAccessMask = VK_ACCESS_HOST_READ_BIT | VK_ACCESS_HOST_WRITE_BIT;
    }
    VkPipelineStageFlags dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    barrier1.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(ctx->cmd, src_stage, dst_stage, 0, 0, NULL, 0, NULL, 1, &barrier1);

    VkImageSubresourceRange range = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1};
    vkCmdClearColorImage(
        ctx->cmd, ctx->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, clr, 1, &range);

    VkImageMemoryBarrier barrier2 = {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier2.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier2.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier2.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier2.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    barrier2.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier2.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier2.image = ctx->image;
    barrier2.subresourceRange = range;
    vkCmdPipelineBarrier(
        ctx->cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 0, NULL, 0, NULL,
        1, &barrier2);

    if (vkEndCommandBuffer(ctx->cmd) != VK_SUCCESS)
    {
        return false;
    }
    VkSubmitInfo submit = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &ctx->cmd;
    if (vkQueueSubmit(ctx->queue, 1, &submit, VK_NULL_HANDLE) != VK_SUCCESS)
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

#endif // DVZ_HAS_KVZ

int test_video_kvazaar(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);
#if !DVZ_HAS_KVZ
    log_warn("kvazaar backend disabled at build time; skipping CPU fallback test");
    return 0;
#else
    KvzCpuCtx ctx = {0};
    if (!kvz_cpu_ctx_init(&ctx))
    {
        log_warn("kvazaar CPU fallback test skipped (unable to initialize Vulkan)");
        kvz_cpu_ctx_destroy(&ctx);
        return 0;
    }

    int rc = 0;
    const char* raw_path = "video_kvazaar.h26x";
    DvzDevice* device = (DvzDevice*)calloc(1, sizeof(DvzDevice));
    DvzGpu* gpu = (DvzGpu*)calloc(1, sizeof(DvzGpu));
    if (!device || !gpu)
    {
        log_error("failed to allocate temporary dvz device for kvazaar test");
        free(device);
        free(gpu);
        kvz_cpu_ctx_destroy(&ctx);
        return 1;
    }
    device->vk_device = ctx.device;
    device->gpu = gpu;
    gpu->pdevice = ctx.phys;

    DvzVideoEncoderConfig cfg = dvz_video_encoder_default_config();
    cfg.width = KVZ_CPU_WIDTH;
    cfg.height = KVZ_CPU_HEIGHT;
    cfg.fps = KVZ_CPU_FPS;
    cfg.codec = DVZ_VIDEO_CODEC_HEVC;
    cfg.mux = DVZ_VIDEO_MUX_MP4_POST;
    cfg.mp4_path = "video_kvazaar.mp4";
    cfg.backend = "kvazaar";
    cfg.raw_path = raw_path;

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
    for (uint32_t frame = 0; frame < KVZ_CPU_FRAMES; ++frame)
    {
        VkClearColorValue clr = frame_clear_color(frame, KVZ_CPU_FRAMES);
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
        video_progress((int)(frame + 1), KVZ_CPU_FRAMES);
    }
    dvz_video_encoder_stop(encoder);
    dvz_video_encoder_destroy(encoder);
    encoder = NULL;

    FILE* fp = fopen(raw_path, "rb");
    if (!fp)
    {
        log_error("kvazaar CPU test missing output bitstream");
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
        log_error("kvazaar CPU test wrote empty bitstream");
        rc = 1;
        goto cleanup;
    }
    remove(raw_path);

cleanup:
    if (encoder)
    {
        dvz_video_encoder_destroy(encoder);
    }
    free(device);
    free(gpu);
    kvz_cpu_ctx_destroy(&ctx);
    if (rc != 0)
    {
        return rc;
    }
    return 0;
#endif
}



/*************************************************************************************************/
/*  Entry-point                                                                                  */
/*************************************************************************************************/

int test_video(TstSuite* suite)
{
    ANN(suite);

    const char* tags = "video";

    TEST_SIMPLE(test_video_1);
    TEST_SIMPLE(test_video_nvenc);
    TEST_SIMPLE(test_video_kvazaar);



    return 0;
}
