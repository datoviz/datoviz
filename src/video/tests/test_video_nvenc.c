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
#include "test_video_common.h"
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

#define WIDTH   DVZ_TEST_VIDEO_WIDTH
#define HEIGHT  DVZ_TEST_VIDEO_HEIGHT
#define FPS     DVZ_TEST_VIDEO_FPS
#define SECONDS DVZ_TEST_VIDEO_SECONDS
#define NFRAMES DVZ_TEST_VIDEO_FRAMES



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
        VkClearColorValue clr = dvz_test_video_clear_color((uint32_t)frame, NFRAMES);
        uint64_t signal_value = vk_render_frame_and_sync(&vk, &clr);

        if (dvz_video_encoder_submit(encoder, signal_value) != 0)
        {
            rc = 1;
            goto cleanup;
        }

        dvz_test_video_progress(frame + 1, NFRAMES);
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
