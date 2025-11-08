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

#include <float.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vk_video/vulkan_video_codec_h264std.h>        // level/profile enums (STD_VIDEO_*)
#include <vk_video/vulkan_video_codec_h264std_encode.h> // std header name+version macros
#include <vk_video/vulkan_video_codecs_common.h>

#include "../../vk/macros.h"
#include "../../vk/tests/test_vk.h"
#include "../../vklite/tests/test_vklite.h"
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
#include "nvEncodeAPI.h"
#include "test_video.h"
#include "testing.h"
#include "vulkan_core.h"
#include <volk.h>



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_ALIGN16(x) (((x) + 15u) & ~15u)

// H.264 encoders operate on 16×16 macroblocks, so align render targets to multiples of 16 to avoid
// validation/runtime failures when programming SPS/PPS.
const uint32_t WIDTH = DVZ_ALIGN16(DVZ_PROTO_WIDTH);
const uint32_t HEIGHT = DVZ_ALIGN16(DVZ_PROTO_HEIGHT);
const uint32_t FPS = 30;
const uint32_t SECS = 5;
const uint32_t NFR = FPS * SECS;                              // 150
const VkDeviceSize BITSTREAM_FRAME_SIZE = WIDTH * HEIGHT * 2; // generous per-frame budget

typedef struct VideoBitstreamFeedback
{
    uint64_t offset;
    uint64_t bytes;
} VideoBitstreamFeedback;

// --- Minimal SPS / PPS defaults ---
static const StdVideoH264SequenceParameterSet sps = {
    .seq_parameter_set_id = 0,
    .profile_idc = STD_VIDEO_H264_PROFILE_IDC_MAIN,
    .level_idc = STD_VIDEO_H264_LEVEL_IDC_4_1,
    .chroma_format_idc = STD_VIDEO_H264_CHROMA_FORMAT_IDC_420,
    .bit_depth_luma_minus8 = 0,
    .bit_depth_chroma_minus8 = 0,
    .pic_width_in_mbs_minus1 = (WIDTH / 16) - 1,
    .pic_height_in_map_units_minus1 = (HEIGHT / 16) - 1,
    .log2_max_frame_num_minus4 = 0,
    .pic_order_cnt_type = 0,
    .log2_max_pic_order_cnt_lsb_minus4 = 0,
};

static const StdVideoH264PictureParameterSet pps = {
    .pic_parameter_set_id = 0,
    .seq_parameter_set_id = 0,
    .num_ref_idx_l0_default_active_minus1 = 0,
    .num_ref_idx_l1_default_active_minus1 = 0,
};



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static uint32_t
find_memory_type(VkPhysicalDevice phys, uint32_t type_bits, VkMemoryPropertyFlags required_props)
{
    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(phys, &mem_props);

    // First pass: must satisfy both the type mask and all required properties.
    for (uint32_t i = 0; i < mem_props.memoryTypeCount; ++i)
    {
        const uint32_t type_supported = (type_bits & (1u << i)) != 0u;
        const VkMemoryPropertyFlags flags = mem_props.memoryTypes[i].propertyFlags;
        if (type_supported && (flags & required_props) == required_props)
        {
            return i;
        }
    }

    // Fallback: pick any supported type (useful when caller passes 0 required_props).
    for (uint32_t i = 0; i < mem_props.memoryTypeCount; ++i)
    {
        if (type_bits & (1u << i))
        {
            return i;
        }
    }

    assert(!"No suitable memory type found");
    return UINT32_MAX;
}


static VkDeviceSize align_up(VkDeviceSize value, VkDeviceSize alignment)
{
    if (alignment == 0)
    {
        return value;
    }
    return (value + alignment - 1) & ~(alignment - 1);
}


static bool
device_supports_extensions(VkPhysicalDevice pdev, const char** required, uint32_t count)
{
    if (count == 0)
    {
        return true;
    }

    uint32_t available = 0;
    VK_CHECK_RESULT(vkEnumerateDeviceExtensionProperties(pdev, NULL, &available, NULL));

    VkExtensionProperties* props = NULL;
    if (available > 0)
    {
        props = (VkExtensionProperties*)calloc(available, sizeof(VkExtensionProperties));
        ANN(props);
        VK_CHECK_RESULT(vkEnumerateDeviceExtensionProperties(pdev, NULL, &available, props));
    }

    bool ok = true;
    for (uint32_t i = 0; i < count && ok; ++i)
    {
        bool found = false;
        for (uint32_t j = 0; j < available; ++j)
        {
            if (strncmp(required[i], props[j].extensionName, VK_MAX_EXTENSION_NAME_SIZE) == 0)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            ok = false;
        }
    }

    free(props);
    return ok;
}


static bool find_video_encode_queue(VkPhysicalDevice phys, uint32_t* queue_family)
{
    ANN(queue_family);

    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties2(phys, &count, NULL);
    if (count == 0)
    {
        return false;
    }

    VkQueueFamilyProperties2* props =
        (VkQueueFamilyProperties2*)calloc(count, sizeof(VkQueueFamilyProperties2));
    ANN(props);
    VkQueueFamilyVideoPropertiesKHR* video_props =
        (VkQueueFamilyVideoPropertiesKHR*)calloc(count, sizeof(VkQueueFamilyVideoPropertiesKHR));
    ANN(video_props);

    for (uint32_t i = 0; i < count; ++i)
    {
        props[i].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
        props[i].pNext = &video_props[i];
        video_props[i].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_VIDEO_PROPERTIES_KHR;
        video_props[i].pNext = NULL;
    }

    vkGetPhysicalDeviceQueueFamilyProperties2(phys, &count, props);

    bool found = false;
    for (uint32_t i = 0; i < count; ++i)
    {
        const VkQueueFlags flags = props[i].queueFamilyProperties.queueFlags;
        if ((flags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR) == 0)
        {
            continue;
        }
        if ((video_props[i].videoCodecOperations & VK_VIDEO_CODEC_OPERATION_ENCODE_H264_BIT_KHR) ==
            0)
        {
            continue;
        }
        *queue_family = i;
        found = true;
        break;
    }

    free(video_props);
    free(props);
    return found;
}


static bool pick_video_encode_device(
    VkInstance instance, const char** required_exts, uint32_t ext_count,
    VkPhysicalDevice* out_phys, uint32_t* out_queue_family)
{
    ANN(out_phys);
    ANN(out_queue_family);

    uint32_t device_count = 0;
    VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &device_count, NULL));
    if (device_count == 0)
    {
        return false;
    }

    VkPhysicalDevice* devices = (VkPhysicalDevice*)calloc(device_count, sizeof(VkPhysicalDevice));
    ANN(devices);
    VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &device_count, devices));

    bool found = false;
    for (uint32_t i = 0; i < device_count; ++i)
    {
        if (!device_supports_extensions(devices[i], required_exts, ext_count))
        {
            continue;
        }

        uint32_t qf = 0;
        if (!find_video_encode_queue(devices[i], &qf))
        {
            continue;
        }

        *out_phys = devices[i];
        *out_queue_family = qf;
        found = true;
        break;
    }

    free(devices);
    return found;
}



static bool video_encode_supported(DvzBootstrap* bootstrap)
{
    ANN(bootstrap);

    DvzGpu* gpu = bootstrap->gpu;
    ANN(gpu);

    const char* extensions[] = {
        VK_KHR_VIDEO_QUEUE_EXTENSION_NAME,
        VK_KHR_VIDEO_ENCODE_QUEUE_EXTENSION_NAME,
        VK_KHR_VIDEO_ENCODE_H264_EXTENSION_NAME,
    };

    for (uint32_t i = 0; i < DVZ_ARRAY_COUNT(extensions); ++i)
    {
        if (!dvz_gpu_has_extension(gpu, extensions[i]))
        {
            log_warn("Skipping test_video_1: GPU missing extension %s", extensions[i]);
            return false;
        }
    }

    DvzQueueCaps* caps = dvz_gpu_queue_caps(gpu);
    ANN(caps);
    for (uint32_t q = 0; q < caps->family_count; ++q)
    {
        if (caps->flags[q] & VK_QUEUE_VIDEO_ENCODE_BIT_KHR)
        {
            return true;
        }
    }

    log_warn("Skipping test_video_1: no queue exposes VK_QUEUE_VIDEO_ENCODE_BIT_KHR");
    return false;
}



/*************************************************************************************************/
/*  Video tests                                                                                  */
/*************************************************************************************************/

int test_video_1(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    // Bootstrap.
    DvzBootstrap bootstrap = {0};
    dvz_bootstrap(&bootstrap, DVZ_BOOTSTRAP_MANUAL_CREATE_DEVICE);

    if (!video_encode_supported(&bootstrap))
    {
        log_warn("Video encode stack unavailable on this GPU, skipping test_video_1");
        return 0;
    }

    DvzDevice* device = &bootstrap.device;
    ANN(device);
    dvz_device_request_extension(device, VK_KHR_VIDEO_QUEUE_EXTENSION_NAME);
    dvz_device_request_extension(device, VK_KHR_VIDEO_ENCODE_QUEUE_EXTENSION_NAME);
    dvz_device_request_extension(device, VK_KHR_VIDEO_ENCODE_H264_EXTENSION_NAME);

    // Create a device with support for dynamic rendering.
    VkPhysicalDeviceVulkan13Features* features = dvz_device_request_features13(device);
    features->dynamicRendering = true;
    features->synchronization2 = true;
    AT(dvz_device_create(device) == 0);
    dvz_device_allocator(device, 0, &bootstrap.allocator);



    VkDevice vkdev = device->vk_device;
    ANNVK(vkdev);

    VkPhysicalDevice pdev = bootstrap.gpu->pdevice;
    ANNVK(pdev);

    DvzQueue* queue = dvz_device_queue(device, DVZ_QUEUE_MAIN);
    ANN(queue);
    ANNVK(queue->vk_queue);
    uint32_t qf = dvz_queue_family(queue);

    DvzQueue* queuev = dvz_device_queue(device, DVZ_QUEUE_VIDEO_ENCODE);
    ANN(queuev);
    ANNVK(queuev->vk_queue);
    uint32_t qfv = queuev->family_idx;



    // Graphics setup.
    DvzGraphics graphics = {0};
    dvz_graphics(device, &graphics);

    // Shaders.
    DvzShader vs = {0};
    DvzShader fs = {0};
    DvzSize vs_size = 0;
    DvzSize fs_size = 0;
    uint32_t* vs_spv = dvz_test_shader_load("hello_triangle.vert.spv", &vs_size);
    uint32_t* fs_spv = dvz_test_shader_load("hello_triangle.frag.spv", &fs_size);
    ANN(vs_spv);
    ANN(fs_spv);
    dvz_shader(device, vs_size, vs_spv, &vs);
    dvz_shader(device, fs_size, fs_spv, &fs);
    dvz_graphics_shader(&graphics, VK_SHADER_STAGE_VERTEX_BIT, dvz_shader_handle(&vs));
    dvz_graphics_shader(&graphics, VK_SHADER_STAGE_FRAGMENT_BIT, dvz_shader_handle(&fs));

    // Slots.
    DvzSlots slots = {0};
    dvz_slots(&bootstrap.device, &slots);
    AT(dvz_slots_create(&slots) == 0);
    dvz_graphics_layout(&graphics, dvz_slots_handle(&slots));

    // Attachments.
    dvz_graphics_attachment_color(&graphics, 0, VK_FORMAT_R8G8B8A8_UNORM);
    dvz_graphics_blend_color(
        &graphics, 0, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        VK_BLEND_OP_ADD, 0xF);
    dvz_graphics_blend_alpha(
        &graphics, 0, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD);

    // Fixed state.
    dvz_graphics_primitive(
        &graphics, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, DVZ_GRAPHICS_FLAGS_FIXED);

    // Dynamic state.
    dvz_graphics_viewport(&graphics, 0, 0, WIDTH, HEIGHT, 0, 1, DVZ_GRAPHICS_FLAGS_DYNAMIC);
    dvz_graphics_scissor(&graphics, 0, 0, WIDTH, HEIGHT, DVZ_GRAPHICS_FLAGS_DYNAMIC);

    // Graphics pipeline creation.
    AT(dvz_graphics_create(&graphics) == 0);

    // Rendering.
    DvzRendering rendering = {0};
    dvz_rendering(&rendering);
    dvz_rendering_area(&rendering, 0, 0, WIDTH, HEIGHT);

    // Image to render to.
    DvzImages img = {0};
    dvz_images(&bootstrap.device, &bootstrap.allocator, VK_IMAGE_TYPE_2D, 1, &img);
    dvz_images_format(&img, VK_FORMAT_R8G8B8A8_UNORM);
    dvz_images_size(&img, WIDTH, HEIGHT, 1);
    dvz_images_usage(&img, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    dvz_images_create(&img);

    // Image views.
    DvzImageViews view = {0};
    dvz_image_views(&img, &view);
    dvz_image_views_create(&view);

    // Attachments.
    DvzAttachment* attachment = dvz_rendering_color(&rendering, 0);
    dvz_attachment_image(
        attachment, dvz_image_views_handle(&view, 0), VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL);
    dvz_attachment_ops(attachment, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);
    dvz_attachment_clear(attachment, (VkClearValue){.color.float32 = {.1, .2, .3, 1}});

    // Image barrier.
    DvzBarriers barriers = {0};
    dvz_barriers(&barriers);

    // Image transition.
    DvzBarrierImage* bimg = dvz_barriers_image(&barriers, dvz_image_handle(&img, 0));
    ANN(bimg);
    dvz_barrier_image_stage(
        bimg, VK_PIPELINE_STAGE_2_NONE, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
    dvz_barrier_image_access(bimg, 0, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
    dvz_barrier_image_layout(bimg, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    // Command buffer.
    DvzCommands cmds = {0};
    dvz_commands(device, queue, 1, &cmds);
    dvz_cmd_begin(&cmds);
    dvz_cmd_barriers(&cmds, 0, &barriers);
    dvz_cmd_rendering_begin(&cmds, 0, &rendering);
    dvz_cmd_bind_graphics(&cmds, 0, &graphics);
    dvz_cmd_draw(&cmds, 0, 0, 3, 0, 1);
    dvz_cmd_rendering_end(&cmds, 0);
    dvz_cmd_end(&cmds);

    // Submit the command buffer.
    dvz_cmd_submit(&cmds);

    //////////////


    DvzImages nv12_tmp = {0};
    dvz_images(&bootstrap.device, &bootstrap.allocator, VK_IMAGE_TYPE_2D, 1, &nv12_tmp);
    dvz_images_flags(
        &nv12_tmp, VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT | VK_IMAGE_CREATE_EXTENDED_USAGE_BIT);
    dvz_images_format(&nv12_tmp, VK_FORMAT_G8_B8R8_2PLANE_420_UNORM);
    dvz_images_usage(&nv12_tmp, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    dvz_images_size(&nv12_tmp, WIDTH, HEIGHT, 1);
    dvz_images_create(&nv12_tmp);

    VkImageView nv12_y_view = {0};
    VkImageView nv12_uv_view = {0};

    VkImageViewCreateInfo yViewCI = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = nv12_tmp.vk_images[0],
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VK_FORMAT_R8_UNORM, // luma plane
        .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    };

    VkImageViewCreateInfo uvViewCI = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = nv12_tmp.vk_images[0],
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VK_FORMAT_R8G8_UNORM, // chroma plane
        .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_PLANE_1_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    };

    VK_CHECK_RESULT(vkCreateImageView(vkdev, &yViewCI, NULL, &nv12_y_view));
    VK_CHECK_RESULT(vkCreateImageView(vkdev, &uvViewCI, NULL, &nv12_uv_view));



    // binding 0: sampled RGBA source
    DvzSampler sampler = {0};
    dvz_sampler(device, &sampler);
    AT(dvz_sampler_create(&sampler) == 0);


    // Create a basic compute shader.

    DvzSize cs_size = 0;
    DvzShader cs = {0};
    uint32_t* cs_spv = dvz_test_shader_load("rgb_to_nv12.comp.spv", &cs_size);

    // Shaders.
    dvz_shader(device, cs_size, cs_spv, &cs);

    DvzCompute compute = {0};
    dvz_compute(device, &compute);
    // --- SLOTS (descriptor set layout) ------------------------------
    DvzSlots slots_c = {0};
    dvz_slots(device, &slots_c);

    // binding 0 → input sampled RGBA image
    dvz_slots_binding(
        &slots_c, 0, 0, 1, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    // binding 1 → Y plane output image
    dvz_slots_binding(
        &slots_c, 0, 1, 1, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

    // binding 2 → UV plane output image
    dvz_slots_binding(
        &slots_c, 0, 2, 1, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

    dvz_slots_create(&slots_c);
    dvz_compute_layout(&compute, dvz_slots_handle(&slots_c));

    // Create a compute pipeline.
    dvz_compute_shader(&compute, dvz_shader_handle(&cs));
    AT(dvz_compute_create(&compute) == 0);

    // --- DESCRIPTORS (actual resource bindings) ---------------------
    DvzDescriptors descs_c = {0};
    dvz_descriptors(&slots_c, &descs_c);

    dvz_descriptors_image(
        &descs_c, 0, 0, 0, VK_IMAGE_LAYOUT_GENERAL, dvz_image_views_handle(&view, 0),
        sampler.vk_sampler);

    // binding 1: Y plane (plane 0 of NV12)
    dvz_descriptors_image(&descs_c, 0, 1, 0, VK_IMAGE_LAYOUT_GENERAL, nv12_y_view, VK_NULL_HANDLE);

    // binding 2: UV plane (plane 1 of NV12)
    dvz_descriptors_image(
        &descs_c, 0, 2, 0, VK_IMAGE_LAYOUT_GENERAL, nv12_uv_view, VK_NULL_HANDLE);



    //////////////////////

    // Codec-specific H.264 encode profile info
    VkVideoEncodeH264ProfileInfoKHR h264Profile = {
        .sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_PROFILE_INFO_KHR,
        .pNext = NULL,
        // Choose a base H.264 profile; most drivers support MAIN or HIGH.
        .stdProfileIdc = STD_VIDEO_H264_PROFILE_IDC_MAIN,
    };

    // 8-bit 4:2:0 H.264 encode profile
    VkVideoProfileInfoKHR videoProfile = {
        .sType = VK_STRUCTURE_TYPE_VIDEO_PROFILE_INFO_KHR,
        .pNext = &h264Profile,
        .videoCodecOperation = VK_VIDEO_CODEC_OPERATION_ENCODE_H264_BIT_KHR,
        .chromaSubsampling = VK_VIDEO_CHROMA_SUBSAMPLING_420_BIT_KHR,
        .lumaBitDepth = VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR,
        .chromaBitDepth = VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR,
    };

    // Required: std header name + version for the chosen codec
    VkExtensionProperties h264StdHeader = {
        .extensionName = VK_STD_VULKAN_VIDEO_CODEC_H264_ENCODE_EXTENSION_NAME,
        .specVersion = VK_STD_VULKAN_VIDEO_CODEC_H264_ENCODE_SPEC_VERSION,
    };

    // Optional but recommended: H.264-specific create info (can be empty defaults)
    VkVideoEncodeH264SessionCreateInfoKHR h264CreateInfo = {
        .sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_SESSION_CREATE_INFO_KHR, .pNext = NULL,
        // You can set .useMaxLevelIdc / .maxLevelIdc if you want to clamp.
        // Leaving defaults is fine for bring-up.
    };

    VkVideoSessionCreateInfoKHR sessionCI = {
        .sType = VK_STRUCTURE_TYPE_VIDEO_SESSION_CREATE_INFO_KHR,
        .pNext = &h264CreateInfo, // <— codec-specific struct, OK
        .queueFamilyIndex = qfv,  // encode-capable qfam
        .flags = 0,
        .pVideoProfile = &videoProfile, // <— REQUIRED
        .pictureFormat = VK_FORMAT_G8_B8R8_2PLANE_420_UNORM,
        .maxCodedExtent = (VkExtent2D){WIDTH, HEIGHT},
        .referencePictureFormat = VK_FORMAT_UNDEFINED, // no refs for intra-only
        .maxDpbSlots = 0,
        .maxActiveReferencePictures = 0,
        .pStdHeaderVersion = &h264StdHeader, // <— REQUIRED
    };

    VkVideoSessionKHR session = VK_NULL_HANDLE;
    VK_CHECK_RESULT(vkCreateVideoSessionKHR(vkdev, &sessionCI, NULL, &session));
    ANNVK(session);

    // 1) Add-info: NOT in pNext chain of paramsCI.
    VkVideoEncodeH264SessionParametersAddInfoKHR addInfo = {
        .sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_SESSION_PARAMETERS_ADD_INFO_KHR,
        .pNext = NULL,
        .stdSPSCount = 1,
        .pStdSPSs = &sps,
        .stdPPSCount = 1,
        .pStdPPSs = &pps,
    };

    // 2) H.264 session-parameters create-info: pNext = NULL.
    //    Provide addInfo via pParametersAddInfo.
    VkVideoEncodeH264SessionParametersCreateInfoKHR h264ParamsCI = {
        .sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_SESSION_PARAMETERS_CREATE_INFO_KHR,
        .pNext = NULL, // <-- important
        .maxStdSPSCount = 1,
        .maxStdPPSCount = 1,
        .pParametersAddInfo = &addInfo, // <-- here, not in pNext
    };

    // 3) Top-level session-parameters CI: pNext points to *h264ParamsCI only*.
    VkVideoSessionParametersCreateInfoKHR paramsCI = {
        .sType = VK_STRUCTURE_TYPE_VIDEO_SESSION_PARAMETERS_CREATE_INFO_KHR,
        .pNext = &h264ParamsCI, // <-- ONLY this in pNext chain
        .videoSession = session,
        .videoSessionParametersTemplate = VK_NULL_HANDLE,
    };

    uint32_t bindCount = 0;

    // First query the count.
    vkGetVideoSessionMemoryRequirementsKHR(vkdev, session, &bindCount, NULL);

    // Allocate array.
    VkVideoSessionMemoryRequirementsKHR* memReqs =
        calloc(bindCount, sizeof(VkVideoSessionMemoryRequirementsKHR));

    // Initialize each element's sType BEFORE the call.
    for (uint32_t i = 0; i < bindCount; ++i)
    {
        memReqs[i].sType = VK_STRUCTURE_TYPE_VIDEO_SESSION_MEMORY_REQUIREMENTS_KHR;
        memReqs[i].pNext = NULL;
    }

    // Now fill them.
    vkGetVideoSessionMemoryRequirementsKHR(vkdev, session, &bindCount, memReqs);

    // Prepare bind structs.
    VkBindVideoSessionMemoryInfoKHR* binds =
        calloc(bindCount, sizeof(VkBindVideoSessionMemoryInfoKHR));

    for (uint32_t i = 0; i < bindCount; ++i)
    {
        VkMemoryAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = memReqs[i].memoryRequirements.size,
            .memoryTypeIndex = find_memory_type(
                pdev, memReqs[i].memoryRequirements.memoryTypeBits,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
        };
        VK_CHECK_RESULT(vkAllocateMemory(vkdev, &allocInfo, NULL, &binds[i].memory));

        binds[i].sType = VK_STRUCTURE_TYPE_BIND_VIDEO_SESSION_MEMORY_INFO_KHR;
        binds[i].pNext = NULL;
        binds[i].memoryBindIndex = memReqs[i].memoryBindIndex;
        binds[i].memoryOffset = 0;
        binds[i].memorySize = memReqs[i].memoryRequirements.size;
    }

    // Bind everything to the session.
    VK_CHECK_RESULT(vkBindVideoSessionMemoryKHR(vkdev, session, bindCount, binds));

    free(memReqs);
    free(binds);

    VkVideoSessionParametersKHR sessionParams = VK_NULL_HANDLE;
    VK_CHECK_RESULT(vkCreateVideoSessionParametersKHR(vkdev, &paramsCI, NULL, &sessionParams));

    // Grab the Annex B SPS/PPS blob once so the bitstream has proper headers.
    VkVideoEncodeH264SessionParametersGetInfoKHR h264ParamsGetInfo = {
        .sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_SESSION_PARAMETERS_GET_INFO_KHR,
        .pNext = NULL,
        .writeStdSPS = VK_TRUE,
        .writeStdPPS = VK_TRUE,
        .stdSPSId = sps.seq_parameter_set_id,
        .stdPPSId = pps.pic_parameter_set_id,
    };
    VkVideoEncodeSessionParametersGetInfoKHR paramsGetInfo = {
        .sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_SESSION_PARAMETERS_GET_INFO_KHR,
        .pNext = &h264ParamsGetInfo,
        .videoSessionParameters = sessionParams,
    };
    size_t headerSize = 0;
    VK_CHECK_RESULT(
        vkGetEncodedVideoSessionParametersKHR(vkdev, &paramsGetInfo, NULL, &headerSize, NULL));
    AT(headerSize > 0);
    uint8_t* headerBlob = (uint8_t*)calloc(headerSize, sizeof(uint8_t));
    ANN(headerBlob);
    VK_CHECK_RESULT(vkGetEncodedVideoSessionParametersKHR(
        vkdev, &paramsGetInfo, NULL, &headerSize, headerBlob));

    FILE* f = fopen("out.h264", "wb");
    ANN(f);
    fwrite(headerBlob, 1, headerSize, f);
    free(headerBlob);

    VkVideoProfileListInfoKHR profileList = {
        .sType = VK_STRUCTURE_TYPE_VIDEO_PROFILE_LIST_INFO_KHR,
        .pNext = NULL,
        .profileCount = 1,
        .pProfiles = &videoProfile,
    };

    VkDeviceSize bitstreamSize = BITSTREAM_FRAME_SIZE;
    VkBufferCreateInfo bufCI = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = bitstreamSize,
        .usage = VK_BUFFER_USAGE_VIDEO_ENCODE_DST_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .pNext = &profileList,
    };
    VkBuffer bitstream;
    VK_CHECK_RESULT(vkCreateBuffer(vkdev, &bufCI, NULL, &bitstream));

    // Allocate host-visible memory so we can read the encoded bytes
    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(vkdev, bitstream, &req);
    VkMemoryAllocateInfo alloc = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = req.size,
        .memoryTypeIndex = find_memory_type(
            pdev, req.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
    };
    VkDeviceMemory bitstreamMem;
    vkAllocateMemory(vkdev, &alloc, NULL, &bitstreamMem);
    vkBindBufferMemory(vkdev, bitstream, bitstreamMem, 0);

    VkQueryPool queryPool = VK_NULL_HANDLE;
    VkQueryPoolVideoEncodeFeedbackCreateInfoKHR queryFeedback = {
        .sType = VK_STRUCTURE_TYPE_QUERY_POOL_VIDEO_ENCODE_FEEDBACK_CREATE_INFO_KHR,
        .pNext = &videoProfile,
        .encodeFeedbackFlags = VK_VIDEO_ENCODE_FEEDBACK_BITSTREAM_BUFFER_OFFSET_BIT_KHR |
                               VK_VIDEO_ENCODE_FEEDBACK_BITSTREAM_BYTES_WRITTEN_BIT_KHR,
    };
    VkQueryPoolCreateInfo queryInfo = {
        .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
        .pNext = &queryFeedback,
        .flags = 0,
        .queryType = VK_QUERY_TYPE_VIDEO_ENCODE_FEEDBACK_KHR,
        .queryCount = 1,
    };
    VK_CHECK_RESULT(vkCreateQueryPool(vkdev, &queryInfo, NULL, &queryPool));


    // --- NV12 image for the encoder ---
    DvzImages nv12 = {0};
    dvz_images(&bootstrap.device, &bootstrap.allocator, VK_IMAGE_TYPE_2D, 1, &nv12);
    dvz_images_format(&nv12, VK_FORMAT_G8_B8R8_2PLANE_420_UNORM);
    dvz_images_size(&nv12, WIDTH, HEIGHT, 1);
    // must be writable by compute + readable by encoder:
    dvz_images_usage(
        &nv12, VK_IMAGE_USAGE_VIDEO_ENCODE_SRC_BIT_KHR | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    nv12.info.pNext = &profileList;
    nv12.info.sharingMode = VK_SHARING_MODE_CONCURRENT;
    nv12.info.queueFamilyIndexCount = 2;
    nv12.info.pQueueFamilyIndices = (uint32_t[]){qf, qfv};
    dvz_images_create(&nv12);

    // Create **plane views** (one for Y, one for interleaved UV)
    DvzImageViews nv12_views = {0};
    dvz_image_views(&nv12, &nv12_views);
    // If your helper doesn’t support plane views directly, create raw VkImageViews with
    // aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT for Y, and PLANE_1 for UV.
    dvz_image_views_create(&nv12_views);



    VkImageMemoryBarrier2 nvbarriers[2] = {
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
            .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_GENERAL,
            .image = nv12_tmp.vk_images[0],
            .subresourceRange =
                {
                    .aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT | VK_IMAGE_ASPECT_PLANE_1_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
        },
    };

    // Command buffer.
    DvzCommands cmds2 = {0};
    dvz_commands(device, queue, 1, &cmds2);
    dvz_cmd_begin(&cmds2);

    vkCmdPipelineBarrier2(
        cmds2.cmds[0], &(VkDependencyInfo){
                           .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                           .imageMemoryBarrierCount = 1,
                           .pImageMemoryBarriers = nvbarriers,
                       });


    // Bind and dispatch compute
    dvz_cmd_bind_compute(&cmds2, 0, &compute);
    dvz_cmd_bind_descriptors(&cmds2, 0, VK_PIPELINE_BIND_POINT_COMPUTE, &descs_c, 0, 1, 0, NULL);
    uint32_t gx = (WIDTH + 15) / 16;
    uint32_t gy = (HEIGHT + 15) / 16;
    dvz_cmd_dispatch(&cmds2, 0, gx, gy, 1);



    vkCmdPipelineBarrier2(
        cmds2.cmds[0], &(VkDependencyInfo){
                           .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                           .imageMemoryBarrierCount = 2,
                           .pImageMemoryBarriers =
                               (VkImageMemoryBarrier2[]){
                                   {
                                       // tmp → transfer src
                                       .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                                       .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                                       .dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
                                       .srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
                                       .dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
                                       .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
                                       .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                       .image = nv12_tmp.vk_images[0],
                                       .subresourceRange =
                                           {
                                               .aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT |
                                                             VK_IMAGE_ASPECT_PLANE_1_BIT,
                                               .levelCount = 1,
                                               .layerCount = 1,
                                           },
                                   },
                                   {
                                       // encode_img → transfer dst
                                       .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                                       .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
                                       .dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
                                       .srcAccessMask = 0,
                                       .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                                       .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                                       .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                       .image = nv12.vk_images[0],
                                       .subresourceRange =
                                           {
                                               .aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT |
                                                             VK_IMAGE_ASPECT_PLANE_1_BIT,
                                               .levelCount = 1,
                                               .layerCount = 1,
                                           },
                                   },
                               },
                       });


    // Plane 0 = Y
    VkImageCopy2 regionY = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_COPY_2,
        .srcSubresource =
            {
                .aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        .dstSubresource =
            {
                .aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        .extent = {WIDTH, HEIGHT, 1}, // Y plane full res
    };

    // Plane 1 = UV (half resolution)
    VkImageCopy2 regionUV = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_COPY_2,
        .srcSubresource =
            {
                .aspectMask = VK_IMAGE_ASPECT_PLANE_1_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        .dstSubresource =
            {
                .aspectMask = VK_IMAGE_ASPECT_PLANE_1_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        .extent = {WIDTH / 2, HEIGHT / 2, 1}, // chroma plane is 2×2 subsampled
    };

    VkCopyImageInfo2 copyInfo = {
        .sType = VK_STRUCTURE_TYPE_COPY_IMAGE_INFO_2,
        .srcImage = nv12_tmp.vk_images[0],
        .srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .dstImage = nv12.vk_images[0],
        .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .regionCount = 2,
        .pRegions = (VkImageCopy2[]){regionY, regionUV},
    };

    vkCmdCopyImage2(cmds2.cmds[0], &copyInfo);



    dvz_cmd_end(&cmds2);
    dvz_cmd_submit(&cmds2);



    // Command buffer reused for encoding work.
    DvzCommands cmdsv = {0};
    dvz_commands(device, queuev, 1, &cmdsv);
    dvz_cmd_begin(&cmdsv);
    vkCmdPipelineBarrier2(
        cmdsv.cmds[0], &(VkDependencyInfo){
                           .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                           .imageMemoryBarrierCount = 1,
                           .pImageMemoryBarriers =
                               (VkImageMemoryBarrier2[]){
                                   {
                                       .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                                       .srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
                                       .dstStageMask = VK_PIPELINE_STAGE_2_VIDEO_ENCODE_BIT_KHR,
                                       .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                                       .dstAccessMask = VK_ACCESS_2_VIDEO_ENCODE_READ_BIT_KHR,
                                       .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                       .newLayout = VK_IMAGE_LAYOUT_VIDEO_ENCODE_SRC_KHR,
                                       .image = nv12.vk_images[0],
                                       .subresourceRange =
                                           {
                                               .aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT |
                                                             VK_IMAGE_ASPECT_PLANE_1_BIT,
                                               .levelCount = 1,
                                               .layerCount = 1,
                                           },
                                   },
                               },
                       });
    dvz_cmd_end(&cmdsv);
    dvz_cmd_submit(&cmdsv);
    dvz_cmd_reset(&cmdsv);

    //------------------------------------------------------------
    // Encode control structures reused per-frame
    //------------------------------------------------------------

    VkVideoEncodeRateControlLayerInfoKHR rcLayer = {
        .sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_RATE_CONTROL_LAYER_INFO_KHR,
        .pNext = NULL,
        .averageBitrate = 5 * 1000 * 1000, // 5 Mbps
        .maxBitrate = 5 * 1000 * 1000,
        .frameRateNumerator = 30,
        .frameRateDenominator = 1,
    };

    VkVideoEncodeRateControlInfoKHR rcInfo = {
        .sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_RATE_CONTROL_INFO_KHR,
        .pNext = NULL,
        .flags = 0,
        .rateControlMode = VK_VIDEO_ENCODE_RATE_CONTROL_MODE_CBR_BIT_KHR,
        .layerCount = 1,
        .pLayers = &rcLayer,
        .virtualBufferSizeInMs = 1000,
        .initialVirtualBufferSizeInMs = 1000,
    };

    VkVideoCodingControlInfoKHR ctrlInfo = {
        .sType = VK_STRUCTURE_TYPE_VIDEO_CODING_CONTROL_INFO_KHR,
        .pNext = &rcInfo,
        .flags = VK_VIDEO_CODING_CONTROL_ENCODE_RATE_CONTROL_BIT_KHR,
    };

    // 5. Begin-coding info itself
    VkVideoBeginCodingInfoKHR beginInfo = {
        .sType = VK_STRUCTURE_TYPE_VIDEO_BEGIN_CODING_INFO_KHR,
        .pNext = &rcInfo,
        .flags = 0,
        .videoSession = session,                 // VkVideoSessionKHR
        .videoSessionParameters = sessionParams, // VkVideoSessionParametersKHR
        .referenceSlotCount = 0,
        .pReferenceSlots = NULL,
    };


    ///////////////

    //------------------------------------------------------------
    // Define the source picture (NV12 image to encode)
    //------------------------------------------------------------
    VkVideoPictureResourceInfoKHR srcPic = {
        .sType = VK_STRUCTURE_TYPE_VIDEO_PICTURE_RESOURCE_INFO_KHR,
        .pNext = NULL,
        .codedOffset = {0, 0},
        .codedExtent = {WIDTH, HEIGHT},
        .baseArrayLayer = 0,
        .imageViewBinding = nv12_views.vk_views[0], // VkImageView for the NV12 image
    };

    VkVideoEndCodingInfoKHR endInfo = {
        .sType = VK_STRUCTURE_TYPE_VIDEO_END_CODING_INFO_KHR,
        .pNext = NULL,
        .flags = 0,
    };

    for (uint32_t frame = 0; frame < NFR; ++frame)
    {
        dvz_cmd_begin(&cmdsv);
        vkCmdBeginVideoCodingKHR(cmdsv.cmds[0], &beginInfo);

        // NOTE: DO NOT UNCOMMENT THIS
        // return 1;

        if (frame == 0)
        {
            vkCmdControlVideoCodingKHR(cmdsv.cmds[0], &ctrlInfo);
        }

        // ------------------------------
        // Std *picture* info (frame-wide)
        // ------------------------------
        StdVideoEncodeH264PictureInfo stdPic = {
            .seq_parameter_set_id = sps.seq_parameter_set_id,
            .pic_parameter_set_id = pps.pic_parameter_set_id,
            .idr_pic_id = frame,
            .primary_pic_type = STD_VIDEO_H264_PICTURE_TYPE_I,
            .frame_num = frame,
            .PicOrderCnt = 0,
            .temporal_id = 0,
            .pRefLists = NULL,
        };
        stdPic.flags.IdrPicFlag = 1;
        stdPic.flags.is_reference = 1;

        // ---------------------------------
        // Std *slice* header (one full slice)
        // ---------------------------------
        StdVideoEncodeH264SliceHeader stdSlice = {
            .first_mb_in_slice = 0,
            .slice_type = STD_VIDEO_H264_SLICE_TYPE_I,
            .slice_alpha_c0_offset_div2 = 0,
            .slice_beta_offset_div2 = 0,
            .slice_qp_delta = 0,
            .cabac_init_idc = STD_VIDEO_H264_CABAC_INIT_IDC_0,
            .disable_deblocking_filter_idc = STD_VIDEO_H264_DISABLE_DEBLOCKING_FILTER_IDC_DISABLED,
            .pWeightTable = NULL,
        };

        // ------------------------------
        // Wire into Vulkan encode structs
        // ------------------------------
        VkVideoEncodeH264NaluSliceInfoKHR nalu = {
            .sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_NALU_SLICE_INFO_KHR,
            .pNext = NULL,
            .constantQp = 0,
            .pStdSliceHeader = &stdSlice,
        };

        VkVideoEncodeH264PictureInfoKHR h264PicInfo = {
            .sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_PICTURE_INFO_KHR,
            .pNext = NULL,
            .naluSliceEntryCount = 1,
            .pNaluSliceEntries = &nalu,
            .pStdPictureInfo = &stdPic,
        };

        VkVideoEncodeInfoKHR encodeInfo = {
            .sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_INFO_KHR,
            .pNext = &h264PicInfo,
            .flags = 0,
            .dstBuffer = bitstream,
            .dstBufferOffset = 0,
            .dstBufferRange = bitstreamSize,
            .srcPictureResource = srcPic,
            .pSetupReferenceSlot = NULL,
            .referenceSlotCount = 0,
            .pReferenceSlots = NULL,
            .precedingExternallyEncodedBytes = 0,
        };

        vkCmdBeginQuery(cmdsv.cmds[0], queryPool, 0, 0);
        vkCmdEncodeVideoKHR(cmdsv.cmds[0], &encodeInfo);

        VkBufferMemoryBarrier2 bitstreamBarrier = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_VIDEO_ENCODE_BIT_KHR,
            .srcAccessMask = VK_ACCESS_2_VIDEO_ENCODE_WRITE_BIT_KHR,
            .dstStageMask = VK_PIPELINE_STAGE_2_HOST_BIT,
            .dstAccessMask = VK_ACCESS_2_HOST_READ_BIT,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer = bitstream,
            .offset = 0,
            .size = bitstreamSize,
        };

        vkCmdPipelineBarrier2(
            cmdsv.cmds[0], &(VkDependencyInfo){
                               .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                               .bufferMemoryBarrierCount = 1,
                               .pBufferMemoryBarriers = &bitstreamBarrier,
                           });

        vkCmdEndQuery(cmdsv.cmds[0], queryPool, 0);
        vkCmdEndVideoCodingKHR(cmdsv.cmds[0], &endInfo);
        dvz_cmd_end(&cmdsv);

        // DEBUG: KEEP THIS, DO NOT REMOVE
        return 1;

        dvz_cmd_submit(&cmdsv);

        VideoBitstreamFeedback feedback = {0};
        VK_CHECK_RESULT(vkGetQueryPoolResults(
            vkdev, queryPool, 0, 1, sizeof(feedback), &feedback, sizeof(feedback),
            VK_QUERY_RESULT_WAIT_BIT | VK_QUERY_RESULT_64_BIT));

        AT(feedback.bytes > 0);
        AT(feedback.offset + feedback.bytes <= bitstreamSize);

        void* data = NULL;
        VK_CHECK_RESULT(
            vkMapMemory(vkdev, bitstreamMem, feedback.offset, feedback.bytes, 0, &data));
        fwrite(data, 1, feedback.bytes, f);
        vkUnmapMemory(vkdev, bitstreamMem);

        dvz_cmd_reset(&cmdsv);
    }

    fclose(f);
    if (queryPool != VK_NULL_HANDLE)
    {
        vkDestroyQueryPool(vkdev, queryPool, NULL);
    }
    vkDestroyBuffer(vkdev, bitstream, NULL);
    vkFreeMemory(vkdev, bitstreamMem, NULL);

    // void* data = NULL;
    // vkMapMemory(vkdev, bitstreamMem, 0, bitstreamSize, 0, &data);
    // FILE* f = fopen("out.h264", "wb");
    // fwrite(data, 1, bitstreamSize, f);
    // fclose(f);
    // vkUnmapMemory(vkdev, bitstreamMem);



    {
        // // Staging buffer for screenshot.
        // DvzBuffer staging = {0};
        // DvzSize screenshot_size = WIDTH * HEIGHT * 4;
        // dvz_buffer(&bootstrap.device, &bootstrap.allocator, &staging);
        // dvz_buffer_size(&staging, screenshot_size);
        // dvz_buffer_flags(
        //     &staging, VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT |
        //     VMA_ALLOCATION_CREATE_MAPPED_BIT);
        // dvz_buffer_usage(&staging, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
        // dvz_buffer_create(&staging);

        // // Screenshot.
        // dvz_cmd_reset(&cmds);
        // dvz_cmd_begin(&cmds);

        // // Layout transition.
        // dvz_barrier_image_stage(
        //     bimg, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        //     VK_PIPELINE_STAGE_2_TRANSFER_BIT);
        // dvz_barrier_image_access(
        //     bimg, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_2_TRANSFER_READ_BIT);
        // dvz_barrier_image_layout(
        //     bimg, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        //     VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        // dvz_cmd_barriers(&cmds, 0, &barriers);

        // // Copy image to buffer.
        // DvzImageRegion region = {0};
        // dvz_image_region(&region);
        // dvz_image_region_extent(&region, WIDTH, HEIGHT, 1);
        // dvz_cmd_copy_image_to_buffer(
        //     &cmds, 0, dvz_image_handle(&img, 0), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, &region,
        //     dvz_buffer_handle(&staging), 0);

        // // End the command buffer.
        // dvz_cmd_end(&cmds);

        // // Submit the command buffer.
        // dvz_cmd_submit(&cmds);

        // // Recover the screenshot.
        // uint8_t* screenshot = (uint8_t*)dvz_calloc(WIDTH * HEIGHT, 4);
        // dvz_buffer_download(&staging, 0, screenshot_size, screenshot);
        // dvz_write_png("build/video.png", WIDTH, HEIGHT, screenshot);

        // // Cleanup.
        // log_debug("cleanup");
        // dvz_image_views_destroy(&view);
        // dvz_images_destroy(&img);
        // dvz_buffer_destroy(&staging);
        // dvz_shader_destroy(&vs);
        // dvz_shader_destroy(&fs);
        // dvz_slots_destroy(&slots);
        // dvz_graphics_destroy(&graphics);
        // dvz_bootstrap_destroy(&bootstrap);
        // dvz_free(vs_spv);
        // dvz_free(fs_spv);
        // dvz_free(screenshot);
    }
    return bootstrap.instance.n_errors > 0;
}


/*************************************************************************************************/
/*  Minimal raw Vulkan video encode                                                              */
/*************************************************************************************************/

int test_video_2(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    const uint32_t VIDEO_WIDTH = 256;
    const uint32_t VIDEO_HEIGHT = 256;
    const uint32_t FRAME_COUNT = 8;
    const VkDeviceSize BASE_FRAME_CHUNK = 256u * 1024u;
    const uint8_t Y_VALUE = 0x80;
    const uint8_t UV_VALUE = 0x80;
    const char* OUTPUT_PATH = "out_static.h264";

    bool skipped = false;
    bool success = false;

    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physical = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue videoQueue = VK_NULL_HANDLE;
    uint32_t videoQueueFamily = 0;
    VkCommandPool cmdPool = VK_NULL_HANDLE;
    VkCommandBuffer cmdBuf = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    VkBuffer bitstreamBuffer = VK_NULL_HANDLE;
    VkDeviceMemory bitstreamMemory = VK_NULL_HANDLE;
    VkImage nv12Image = VK_NULL_HANDLE;
    VkDeviceMemory nv12Memory = VK_NULL_HANDLE;
    VkImageView nv12View = VK_NULL_HANDLE;
    VkVideoSessionKHR videoSession = VK_NULL_HANDLE;
    VkVideoSessionParametersKHR sessionParams = VK_NULL_HANDLE;
    VkQueryPool queryPool = VK_NULL_HANDLE;
    VkDeviceMemory* sessionMemoryBlocks = NULL;
    uint32_t sessionMemoryCount = 0;
    FILE* out = NULL;

    VkFormat encodeFormat = VK_FORMAT_UNDEFINED;
    VkDeviceSize bitstreamChunk = 0;
    VkDeviceSize bitstreamSize = 0;

    VK_CHECK_RESULT(volkInitialize());

    const char* instanceExts[] = {
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
    };

    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pApplicationName = "datoviz-video2",
        .applicationVersion = VK_MAKE_VERSION(0, 4, 0),
        .pEngineName = "datoviz",
        .engineVersion = VK_MAKE_VERSION(0, 4, 0),
        .apiVersion = VK_API_VERSION_1_3,
    };

    VkInstanceCreateInfo instanceCI = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
        .enabledExtensionCount = DVZ_ARRAY_COUNT(instanceExts),
        .ppEnabledExtensionNames = instanceExts,
    };

    VK_CHECK_RESULT(vkCreateInstance(&instanceCI, NULL, &instance));
    volkLoadInstance(instance);

    const char* deviceExts[] = {
        VK_KHR_VIDEO_QUEUE_EXTENSION_NAME,
        VK_KHR_VIDEO_ENCODE_QUEUE_EXTENSION_NAME,
        VK_KHR_VIDEO_ENCODE_H264_EXTENSION_NAME,
    };

    if (!pick_video_encode_device(
            instance, deviceExts, DVZ_ARRAY_COUNT(deviceExts), &physical, &videoQueueFamily))
    {
        log_warn(
            "Skipping test_video_2: no device exposes VK_QUEUE_VIDEO_ENCODE_BIT_KHR with H.264.");
        skipped = true;
        goto cleanup;
    }

    VkPhysicalDeviceSynchronization2Features sync2 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
        .pNext = NULL,
    };
    VkPhysicalDeviceFeatures2 features2 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &sync2,
    };
    vkGetPhysicalDeviceFeatures2(physical, &features2);
    if (!sync2.synchronization2)
    {
        log_warn("Skipping test_video_2: synchronization2 feature missing.");
        skipped = true;
        goto cleanup;
    }
    sync2.synchronization2 = VK_TRUE;

    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .queueFamilyIndex = videoQueueFamily,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority,
    };

    VkDeviceCreateInfo deviceCI = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &features2,
        .flags = 0,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueInfo,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
        .enabledExtensionCount = DVZ_ARRAY_COUNT(deviceExts),
        .ppEnabledExtensionNames = deviceExts,
        .pEnabledFeatures = NULL,
    };

    VK_CHECK_RESULT(vkCreateDevice(physical, &deviceCI, NULL, &device));
    volkLoadDevice(device);
    vkGetDeviceQueue(device, videoQueueFamily, 0, &videoQueue);
    ANNVK(videoQueue);

    VkCommandPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = NULL,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = videoQueueFamily,
    };
    VK_CHECK_RESULT(vkCreateCommandPool(device, &poolInfo, NULL, &cmdPool));

    VkCommandBufferAllocateInfo cmdAlloc = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = NULL,
        .commandPool = cmdPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdAlloc, &cmdBuf));

    VkFenceCreateInfo fenceInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
    };
    VK_CHECK_RESULT(vkCreateFence(device, &fenceInfo, NULL, &fence));

    VkPhysicalDeviceProperties props = {0};
    vkGetPhysicalDeviceProperties(physical, &props);

    VkVideoEncodeH264ProfileInfoKHR h264Profile = {
        .sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_PROFILE_INFO_KHR,
        .pNext = NULL,
        .stdProfileIdc = STD_VIDEO_H264_PROFILE_IDC_MAIN,
    };

    VkVideoProfileInfoKHR videoProfile = {
        .sType = VK_STRUCTURE_TYPE_VIDEO_PROFILE_INFO_KHR,
        .pNext = &h264Profile,
        .videoCodecOperation = VK_VIDEO_CODEC_OPERATION_ENCODE_H264_BIT_KHR,
        .chromaSubsampling = VK_VIDEO_CHROMA_SUBSAMPLING_420_BIT_KHR,
        .lumaBitDepth = VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR,
        .chromaBitDepth = VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR,
    };

    VkVideoEncodeH264CapabilitiesKHR h264Caps = {
        .sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_CAPABILITIES_KHR,
        .pNext = NULL,
    };
    VkVideoEncodeCapabilitiesKHR encodeCaps = {
        .sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_CAPABILITIES_KHR,
        .pNext = &h264Caps,
    };
    VkVideoCapabilitiesKHR videoCaps = {
        .sType = VK_STRUCTURE_TYPE_VIDEO_CAPABILITIES_KHR,
        .pNext = &encodeCaps,
    };
    VK_CHECK_RESULT(vkGetPhysicalDeviceVideoCapabilitiesKHR(physical, &videoProfile, &videoCaps));

    bitstreamChunk = align_up(BASE_FRAME_CHUNK, videoCaps.minBitstreamBufferOffsetAlignment);
    bitstreamSize = bitstreamChunk * FRAME_COUNT;
    AT(bitstreamSize > 0);

    VkPhysicalDeviceVideoFormatInfoKHR formatInfo = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VIDEO_FORMAT_INFO_KHR,
        .pNext = &videoProfile,
        .imageUsage = VK_IMAGE_USAGE_VIDEO_ENCODE_SRC_BIT_KHR | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        // .format = VK_FORMAT_UNDEFINED,
    };
    uint32_t formatCount = 0;
    VK_CHECK_RESULT(
        vkGetPhysicalDeviceVideoFormatPropertiesKHR(physical, &formatInfo, &formatCount, NULL));
    if (formatCount == 0)
    {
        log_warn("Skipping test_video_2: no compatible video encode formats reported.");
        skipped = true;
        goto cleanup;
    }
    VkVideoFormatPropertiesKHR* formatProps =
        (VkVideoFormatPropertiesKHR*)calloc(formatCount, sizeof(VkVideoFormatPropertiesKHR));
    ANN(formatProps);
    for (uint32_t i = 0; i < formatCount; ++i)
    {
        formatProps[i].sType = VK_STRUCTURE_TYPE_VIDEO_FORMAT_PROPERTIES_KHR;
        formatProps[i].pNext = NULL;
    }
    VK_CHECK_RESULT(vkGetPhysicalDeviceVideoFormatPropertiesKHR(
        physical, &formatInfo, &formatCount, formatProps));
    for (uint32_t i = 0; i < formatCount; ++i)
    {
        if (formatProps[i].format == VK_FORMAT_G8_B8R8_2PLANE_420_UNORM)
        {
            encodeFormat = formatProps[i].format;
            break;
        }
    }
    if (encodeFormat == VK_FORMAT_UNDEFINED)
    {
        encodeFormat = formatProps[0].format;
    }
    free(formatProps);

    VkVideoProfileListInfoKHR profileList = {
        .sType = VK_STRUCTURE_TYPE_VIDEO_PROFILE_LIST_INFO_KHR,
        .pNext = NULL,
        .profileCount = 1,
        .pProfiles = &videoProfile,
    };

    VkDeviceSize yPlaneSize = VIDEO_WIDTH * VIDEO_HEIGHT;
    VkDeviceSize uvPlaneSize = (VIDEO_WIDTH * VIDEO_HEIGHT) / 2;
    VkDeviceSize uvOffset = align_up(yPlaneSize, props.limits.optimalBufferCopyOffsetAlignment);
    VkDeviceSize stagingSize = uvOffset + uvPlaneSize;

    VkBufferCreateInfo stagingCI = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .size = stagingSize,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    VK_CHECK_RESULT(vkCreateBuffer(device, &stagingCI, NULL, &stagingBuffer));

    VkMemoryRequirements stagingReq = {0};
    vkGetBufferMemoryRequirements(device, stagingBuffer, &stagingReq);
    VkMemoryAllocateInfo stagingAlloc = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = NULL,
        .allocationSize = stagingReq.size,
        .memoryTypeIndex = find_memory_type(
            physical, stagingReq.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
    };
    VK_CHECK_RESULT(vkAllocateMemory(device, &stagingAlloc, NULL, &stagingMemory));
    VK_CHECK_RESULT(vkBindBufferMemory(device, stagingBuffer, stagingMemory, 0));

    void* mapped = NULL;
    VK_CHECK_RESULT(vkMapMemory(device, stagingMemory, 0, stagingSize, 0, &mapped));
    uint8_t* stagingBytes = (uint8_t*)mapped;
    memset(stagingBytes, Y_VALUE, (size_t)yPlaneSize);
    if (uvOffset > yPlaneSize)
    {
        memset(stagingBytes + yPlaneSize, Y_VALUE, (size_t)(uvOffset - yPlaneSize));
    }
    uint8_t* uvPlane = stagingBytes + uvOffset;
    for (VkDeviceSize i = 0; i < uvPlaneSize; i += 2)
    {
        uvPlane[i + 0] = UV_VALUE;
        uvPlane[i + 1] = UV_VALUE;
    }
    vkUnmapMemory(device, stagingMemory);

    VkImageCreateInfo imageCI = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = &profileList,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = encodeFormat,
        .extent = (VkExtent3D){VIDEO_WIDTH, VIDEO_HEIGHT, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_VIDEO_ENCODE_SRC_BIT_KHR | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    VK_CHECK_RESULT(vkCreateImage(device, &imageCI, NULL, &nv12Image));

    VkMemoryRequirements imageReq = {0};
    vkGetImageMemoryRequirements(device, nv12Image, &imageReq);
    VkMemoryAllocateInfo imageAlloc = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = NULL,
        .allocationSize = imageReq.size,
        .memoryTypeIndex = find_memory_type(
            physical, imageReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    };
    VK_CHECK_RESULT(vkAllocateMemory(device, &imageAlloc, NULL, &nv12Memory));
    VK_CHECK_RESULT(vkBindImageMemory(device, nv12Image, nv12Memory, 0));

    VkImageViewCreateInfo viewCI = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = &profileList,
        .image = nv12Image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = encodeFormat,
        .components =
            {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
        .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    };
    VK_CHECK_RESULT(vkCreateImageView(device, &viewCI, NULL, &nv12View));

    VkBufferCreateInfo bitstreamCI = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = &profileList,
        .flags = 0,
        .size = bitstreamSize,
        .usage = VK_BUFFER_USAGE_VIDEO_ENCODE_DST_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    VK_CHECK_RESULT(vkCreateBuffer(device, &bitstreamCI, NULL, &bitstreamBuffer));

    VkMemoryRequirements bitstreamReq = {0};
    vkGetBufferMemoryRequirements(device, bitstreamBuffer, &bitstreamReq);
    VkMemoryAllocateInfo bitstreamAlloc = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = NULL,
        .allocationSize = bitstreamReq.size,
        .memoryTypeIndex = find_memory_type(
            physical, bitstreamReq.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
    };
    VK_CHECK_RESULT(vkAllocateMemory(device, &bitstreamAlloc, NULL, &bitstreamMemory));
    VK_CHECK_RESULT(vkBindBufferMemory(device, bitstreamBuffer, bitstreamMemory, 0));

    StdVideoH264SequenceParameterSet sps_local = {
        .profile_idc = STD_VIDEO_H264_PROFILE_IDC_MAIN,
        .level_idc = STD_VIDEO_H264_LEVEL_IDC_4_1,
        .chroma_format_idc = STD_VIDEO_H264_CHROMA_FORMAT_IDC_420,
        .seq_parameter_set_id = 0,
        .bit_depth_luma_minus8 = 0,
        .bit_depth_chroma_minus8 = 0,
        .log2_max_frame_num_minus4 = 0,
        .pic_order_cnt_type = STD_VIDEO_H264_POC_TYPE_0,
        .offset_for_non_ref_pic = 0,
        .offset_for_top_to_bottom_field = 0,
        .log2_max_pic_order_cnt_lsb_minus4 = 0,
        .num_ref_frames_in_pic_order_cnt_cycle = 0,
        .max_num_ref_frames = 1,
        .pic_width_in_mbs_minus1 = (VIDEO_WIDTH / 16) - 1,
        .pic_height_in_map_units_minus1 = (VIDEO_HEIGHT / 16) - 1,
        .frame_crop_left_offset = 0,
        .frame_crop_right_offset = 0,
        .frame_crop_top_offset = 0,
        .frame_crop_bottom_offset = 0,
        .pOffsetForRefFrame = NULL,
        .pScalingLists = NULL,
        .pSequenceParameterSetVui = NULL,
    };
    sps_local.flags.frame_mbs_only_flag = 1;
    sps_local.flags.direct_8x8_inference_flag = 1;

    StdVideoH264PictureParameterSet pps_local = {
        .seq_parameter_set_id = sps_local.seq_parameter_set_id,
        .pic_parameter_set_id = 0,
        .num_ref_idx_l0_default_active_minus1 = 0,
        .num_ref_idx_l1_default_active_minus1 = 0,
        .weighted_bipred_idc = STD_VIDEO_H264_WEIGHTED_BIPRED_IDC_DEFAULT,
        .pic_init_qp_minus26 = 0,
        .pic_init_qs_minus26 = 0,
        .chroma_qp_index_offset = 0,
        .second_chroma_qp_index_offset = 0,
        .pScalingLists = NULL,
    };
    pps_local.flags.deblocking_filter_control_present_flag = 1;

    VkExtensionProperties h264StdHeader = {
        .extensionName = VK_STD_VULKAN_VIDEO_CODEC_H264_ENCODE_EXTENSION_NAME,
        .specVersion = VK_STD_VULKAN_VIDEO_CODEC_H264_ENCODE_SPEC_VERSION,
    };

    VkVideoEncodeH264SessionCreateInfoKHR h264SessionInfo = {
        .sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_SESSION_CREATE_INFO_KHR,
        .pNext = NULL,
        .useMaxLevelIdc = VK_FALSE,
        .maxLevelIdc = sps_local.level_idc,
    };

    VkVideoSessionCreateInfoKHR sessionCI = {
        .sType = VK_STRUCTURE_TYPE_VIDEO_SESSION_CREATE_INFO_KHR,
        .pNext = &h264SessionInfo,
        .queueFamilyIndex = videoQueueFamily,
        .flags = 0,
        .pVideoProfile = &videoProfile,
        .pictureFormat = encodeFormat,
        .maxCodedExtent = (VkExtent2D){VIDEO_WIDTH, VIDEO_HEIGHT},
        .referencePictureFormat = VK_FORMAT_UNDEFINED,
        .maxDpbSlots = 0,
        .maxActiveReferencePictures = 0,
        .pStdHeaderVersion = &h264StdHeader,
    };
    VK_CHECK_RESULT(vkCreateVideoSessionKHR(device, &sessionCI, NULL, &videoSession));

    vkGetVideoSessionMemoryRequirementsKHR(device, videoSession, &sessionMemoryCount, NULL);
    if (sessionMemoryCount > 0)
    {
        VkVideoSessionMemoryRequirementsKHR* memReqs =
            (VkVideoSessionMemoryRequirementsKHR*)calloc(
                sessionMemoryCount, sizeof(VkVideoSessionMemoryRequirementsKHR));
        ANN(memReqs);
        for (uint32_t i = 0; i < sessionMemoryCount; ++i)
        {
            memReqs[i].sType = VK_STRUCTURE_TYPE_VIDEO_SESSION_MEMORY_REQUIREMENTS_KHR;
            memReqs[i].pNext = NULL;
        }
        VK_CHECK_RESULT(vkGetVideoSessionMemoryRequirementsKHR(
            device, videoSession, &sessionMemoryCount, memReqs));

        VkBindVideoSessionMemoryInfoKHR* binds = (VkBindVideoSessionMemoryInfoKHR*)calloc(
            sessionMemoryCount, sizeof(VkBindVideoSessionMemoryInfoKHR));
        ANN(binds);
        sessionMemoryBlocks = (VkDeviceMemory*)calloc(sessionMemoryCount, sizeof(VkDeviceMemory));
        ANN(sessionMemoryBlocks);

        for (uint32_t i = 0; i < sessionMemoryCount; ++i)
        {
            VkMemoryAllocateInfo memAlloc = {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .pNext = NULL,
                .allocationSize = memReqs[i].memoryRequirements.size,
                .memoryTypeIndex = find_memory_type(
                    physical, memReqs[i].memoryRequirements.memoryTypeBits,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
            };
            VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, NULL, &sessionMemoryBlocks[i]));
            binds[i].sType = VK_STRUCTURE_TYPE_BIND_VIDEO_SESSION_MEMORY_INFO_KHR;
            binds[i].pNext = NULL;
            binds[i].memory = sessionMemoryBlocks[i];
            binds[i].memoryBindIndex = memReqs[i].memoryBindIndex;
            binds[i].memoryOffset = 0;
            binds[i].memorySize = memReqs[i].memoryRequirements.size;
        }

        VK_CHECK_RESULT(
            vkBindVideoSessionMemoryKHR(device, videoSession, sessionMemoryCount, binds));

        free(binds);
        free(memReqs);
    }

    VkVideoEncodeH264SessionParametersAddInfoKHR addInfo = {
        .sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_SESSION_PARAMETERS_ADD_INFO_KHR,
        .pNext = NULL,
        .stdSPSCount = 1,
        .pStdSPSs = &sps_local,
        .stdPPSCount = 1,
        .pStdPPSs = &pps_local,
    };

    VkVideoEncodeH264SessionParametersCreateInfoKHR h264ParamsInfo = {
        .sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_SESSION_PARAMETERS_CREATE_INFO_KHR,
        .pNext = NULL,
        .maxStdSPSCount = 1,
        .maxStdPPSCount = 1,
        .pParametersAddInfo = &addInfo,
    };

    VkVideoSessionParametersCreateInfoKHR paramsCI = {
        .sType = VK_STRUCTURE_TYPE_VIDEO_SESSION_PARAMETERS_CREATE_INFO_KHR,
        .pNext = &h264ParamsInfo,
        .videoSession = videoSession,
        .videoSessionParametersTemplate = VK_NULL_HANDLE,
    };
    VK_CHECK_RESULT(vkCreateVideoSessionParametersKHR(device, &paramsCI, NULL, &sessionParams));

    VkVideoEncodeH264SessionParametersGetInfoKHR h264GetInfo = {
        .sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_SESSION_PARAMETERS_GET_INFO_KHR,
        .pNext = NULL,
        .writeStdSPS = VK_TRUE,
        .writeStdPPS = VK_TRUE,
        .stdSPSId = sps_local.seq_parameter_set_id,
        .stdPPSId = pps_local.pic_parameter_set_id,
    };
    VkVideoEncodeSessionParametersGetInfoKHR paramsGetInfo = {
        .sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_SESSION_PARAMETERS_GET_INFO_KHR,
        .pNext = &h264GetInfo,
        .videoSessionParameters = sessionParams,
    };
    size_t headerSize = 0;
    VK_CHECK_RESULT(
        vkGetEncodedVideoSessionParametersKHR(device, &paramsGetInfo, NULL, &headerSize, NULL));
    AT(headerSize > 0);

    uint8_t* headerBlob = (uint8_t*)calloc(headerSize, sizeof(uint8_t));
    ANN(headerBlob);
    VK_CHECK_RESULT(vkGetEncodedVideoSessionParametersKHR(
        device, &paramsGetInfo, NULL, &headerSize, headerBlob));

    out = fopen(OUTPUT_PATH, "wb");
    ANN(out);
    AT(fwrite(headerBlob, 1, headerSize, out) == headerSize);
    free(headerBlob);

    VkQueryPoolVideoEncodeFeedbackCreateInfoKHR queryFeedback = {
        .sType = VK_STRUCTURE_TYPE_QUERY_POOL_VIDEO_ENCODE_FEEDBACK_CREATE_INFO_KHR,
        .pNext = &videoProfile,
        .encodeFeedbackFlags = VK_VIDEO_ENCODE_FEEDBACK_BITSTREAM_BUFFER_OFFSET_BIT_KHR |
                               VK_VIDEO_ENCODE_FEEDBACK_BITSTREAM_BYTES_WRITTEN_BIT_KHR,
    };
    VkQueryPoolCreateInfo queryInfo = {
        .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
        .pNext = &queryFeedback,
        .flags = 0,
        .queryType = VK_QUERY_TYPE_VIDEO_ENCODE_FEEDBACK_KHR,
        .queryCount = FRAME_COUNT,
    };
    VK_CHECK_RESULT(vkCreateQueryPool(device, &queryInfo, NULL, &queryPool));

    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = NULL,
    };
    VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuf, &beginInfo));

    vkCmdResetQueryPool(cmdBuf, queryPool, 0, FRAME_COUNT);

    VkImageMemoryBarrier2 imgToTransfer = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext = NULL,
        .srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
        .srcAccessMask = 0,
        .dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
        .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = nv12Image,
        .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT | VK_IMAGE_ASPECT_PLANE_1_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    };
    VkDependencyInfo depToTransfer = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = NULL,
        .memoryBarrierCount = 0,
        .pMemoryBarriers = NULL,
        .bufferMemoryBarrierCount = 0,
        .pBufferMemoryBarriers = NULL,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &imgToTransfer,
    };
    vkCmdPipelineBarrier2(cmdBuf, &depToTransfer);

    VkBufferImageCopy copies[2] = {0};
    copies[0].bufferOffset = 0;
    copies[0].bufferRowLength = 0;
    copies[0].bufferImageHeight = 0;
    copies[0].imageSubresource.aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT;
    copies[0].imageSubresource.mipLevel = 0;
    copies[0].imageSubresource.baseArrayLayer = 0;
    copies[0].imageSubresource.layerCount = 1;
    copies[0].imageOffset = (VkOffset3D){0, 0, 0};
    copies[0].imageExtent = (VkExtent3D){VIDEO_WIDTH, VIDEO_HEIGHT, 1};

    copies[1].bufferOffset = uvOffset;
    copies[1].bufferRowLength = 0;
    copies[1].bufferImageHeight = 0;
    copies[1].imageSubresource.aspectMask = VK_IMAGE_ASPECT_PLANE_1_BIT;
    copies[1].imageSubresource.mipLevel = 0;
    copies[1].imageSubresource.baseArrayLayer = 0;
    copies[1].imageSubresource.layerCount = 1;
    copies[1].imageOffset = (VkOffset3D){0, 0, 0};
    copies[1].imageExtent = (VkExtent3D){VIDEO_WIDTH / 2, VIDEO_HEIGHT / 2, 1};

    vkCmdCopyBufferToImage(
        cmdBuf, stagingBuffer, nv12Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        DVZ_ARRAY_COUNT(copies), copies);

    VkImageMemoryBarrier2 imgToEncode = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext = NULL,
        .srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
        .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_VIDEO_ENCODE_BIT_KHR,
        .dstAccessMask = VK_ACCESS_2_VIDEO_ENCODE_READ_BIT_KHR,
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_VIDEO_ENCODE_SRC_KHR,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = nv12Image,
        .subresourceRange = imgToTransfer.subresourceRange,
    };
    VkDependencyInfo depToEncode = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = NULL,
        .memoryBarrierCount = 0,
        .pMemoryBarriers = NULL,
        .bufferMemoryBarrierCount = 0,
        .pBufferMemoryBarriers = NULL,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &imgToEncode,
    };
    vkCmdPipelineBarrier2(cmdBuf, &depToEncode);

    VkVideoPictureResourceInfoKHR srcPicture = {
        .sType = VK_STRUCTURE_TYPE_VIDEO_PICTURE_RESOURCE_INFO_KHR,
        .pNext = NULL,
        .codedOffset = (VkOffset2D){0, 0},
        .codedExtent = (VkExtent2D){VIDEO_WIDTH, VIDEO_HEIGHT},
        .baseArrayLayer = 0,
        .imageViewBinding = nv12View,
    };

    VkVideoEncodeRateControlLayerInfoKHR rcLayer = {
        .sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_RATE_CONTROL_LAYER_INFO_KHR,
        .pNext = NULL,
        .averageBitrate = 4ull * 1000ull * 1000ull,
        .maxBitrate = 4ull * 1000ull * 1000ull,
        .frameRateNumerator = 30,
        .frameRateDenominator = 1,
    };

    VkVideoEncodeRateControlInfoKHR rcInfo = {
        .sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_RATE_CONTROL_INFO_KHR,
        .pNext = NULL,
        .flags = 0,
        .rateControlMode = VK_VIDEO_ENCODE_RATE_CONTROL_MODE_CBR_BIT_KHR,
        .layerCount = 1,
        .pLayers = &rcLayer,
        .virtualBufferSizeInMs = 1000,
        .initialVirtualBufferSizeInMs = 1000,
    };

    VkVideoBeginCodingInfoKHR beginCoding = {
        .sType = VK_STRUCTURE_TYPE_VIDEO_BEGIN_CODING_INFO_KHR,
        .pNext = &rcInfo,
        .flags = 0,
        .videoSession = videoSession,
        .videoSessionParameters = sessionParams,
        .referenceSlotCount = 0,
        .pReferenceSlots = NULL,
    };

    VkVideoEndCodingInfoKHR endCoding = {
        .sType = VK_STRUCTURE_TYPE_VIDEO_END_CODING_INFO_KHR,
        .pNext = NULL,
        .flags = 0,
    };

    vkCmdBeginVideoCodingKHR(cmdBuf, &beginCoding);

    for (uint32_t frame = 0; frame < FRAME_COUNT; ++frame)
    {
        StdVideoEncodeH264PictureInfo stdPic = {
            .seq_parameter_set_id = sps_local.seq_parameter_set_id,
            .pic_parameter_set_id = pps_local.pic_parameter_set_id,
            .idr_pic_id = frame,
            .primary_pic_type = STD_VIDEO_H264_PICTURE_TYPE_IDR,
            .frame_num = frame,
            .PicOrderCnt = 0,
            .temporal_id = 0,
            .pRefLists = NULL,
        };
        stdPic.flags.IdrPicFlag = 1;
        stdPic.flags.is_reference = 1;

        StdVideoEncodeH264SliceHeader stdSlice = {
            .flags =
                {
                    .direct_spatial_mv_pred_flag = 0,
                    .num_ref_idx_active_override_flag = 0,
                },
            .first_mb_in_slice = 0,
            .slice_type = STD_VIDEO_H264_SLICE_TYPE_I,
            .slice_alpha_c0_offset_div2 = 0,
            .slice_beta_offset_div2 = 0,
            .slice_qp_delta = 0,
            .cabac_init_idc = STD_VIDEO_H264_CABAC_INIT_IDC_0,
            .disable_deblocking_filter_idc = STD_VIDEO_H264_DISABLE_DEBLOCKING_FILTER_IDC_DISABLED,
            .pWeightTable = NULL,
        };

        VkVideoEncodeH264NaluSliceInfoKHR naluInfo = {
            .sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_NALU_SLICE_INFO_KHR,
            .pNext = NULL,
            .constantQp = 26,
            .pStdSliceHeader = &stdSlice,
        };

        VkVideoEncodeH264PictureInfoKHR h264PicInfo = {
            .sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_PICTURE_INFO_KHR,
            .pNext = NULL,
            .naluSliceEntryCount = 1,
            .pNaluSliceEntries = &naluInfo,
            .pStdPictureInfo = &stdPic,
        };

        VkVideoEncodeInfoKHR encodeInfo = {
            .sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_INFO_KHR,
            .pNext = &h264PicInfo,
            .flags = 0,
            .dstBuffer = bitstreamBuffer,
            .dstBufferOffset = frame * bitstreamChunk,
            .dstBufferRange = bitstreamChunk,
            .srcPictureResource = srcPicture,
            .pSetupReferenceSlot = NULL,
            .referenceSlotCount = 0,
            .pReferenceSlots = NULL,
            .precedingExternallyEncodedBytes = 0,
        };

        vkCmdBeginQuery(cmdBuf, queryPool, frame, 0);
        vkCmdEncodeVideoKHR(cmdBuf, &encodeInfo);
        vkCmdEndQuery(cmdBuf, queryPool, frame);
    }

    vkCmdEndVideoCodingKHR(cmdBuf, &endCoding);

    VkBufferMemoryBarrier2 bitstreamBarrier = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
        .pNext = NULL,
        .srcStageMask = VK_PIPELINE_STAGE_2_VIDEO_ENCODE_BIT_KHR,
        .srcAccessMask = VK_ACCESS_2_VIDEO_ENCODE_WRITE_BIT_KHR,
        .dstStageMask = VK_PIPELINE_STAGE_2_HOST_BIT,
        .dstAccessMask = VK_ACCESS_2_HOST_READ_BIT,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .buffer = bitstreamBuffer,
        .offset = 0,
        .size = bitstreamSize,
    };
    VkDependencyInfo depBitstream = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = NULL,
        .memoryBarrierCount = 0,
        .pMemoryBarriers = NULL,
        .bufferMemoryBarrierCount = 1,
        .pBufferMemoryBarriers = &bitstreamBarrier,
        .imageMemoryBarrierCount = 0,
        .pImageMemoryBarriers = NULL,
    };
    vkCmdPipelineBarrier2(cmdBuf, &depBitstream);

    VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuf));

    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = NULL,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = NULL,
        .pWaitDstStageMask = NULL,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmdBuf,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = NULL,
    };
    VK_CHECK_RESULT(vkQueueSubmit(videoQueue, 1, &submitInfo, fence));
    VK_CHECK_RESULT(vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX));

    void* bitstreamPtr = NULL;
    VK_CHECK_RESULT(vkMapMemory(device, bitstreamMemory, 0, bitstreamSize, 0, &bitstreamPtr));
    uint8_t* bitstreamBytes = (uint8_t*)bitstreamPtr;

    for (uint32_t frame = 0; frame < FRAME_COUNT; ++frame)
    {
        VideoBitstreamFeedback feedback = {0};
        VK_CHECK_RESULT(vkGetQueryPoolResults(
            device, queryPool, frame, 1, sizeof(feedback), &feedback, sizeof(feedback),
            VK_QUERY_RESULT_WAIT_BIT | VK_QUERY_RESULT_64_BIT));
        if (feedback.bytes == 0)
        {
            continue;
        }
        AT(feedback.offset + feedback.bytes <= bitstreamSize);
        AT(fwrite(bitstreamBytes + feedback.offset, 1, feedback.bytes, out) == feedback.bytes);
    }

    vkUnmapMemory(device, bitstreamMemory);
    fflush(out);

    log_info("test_video_2 wrote %s", OUTPUT_PATH);
    success = true;

cleanup:
    if (device != VK_NULL_HANDLE)
    {
        vkDeviceWaitIdle(device);
    }

    if (out != NULL)
    {
        fclose(out);
        out = NULL;
    }

    if (device != VK_NULL_HANDLE && queryPool != VK_NULL_HANDLE)
    {
        vkDestroyQueryPool(device, queryPool, NULL);
    }
    if (device != VK_NULL_HANDLE && sessionParams != VK_NULL_HANDLE)
    {
        vkDestroyVideoSessionParametersKHR(device, sessionParams, NULL);
    }
    if (device != VK_NULL_HANDLE && videoSession != VK_NULL_HANDLE)
    {
        vkDestroyVideoSessionKHR(device, videoSession, NULL);
    }
    if (device != VK_NULL_HANDLE && sessionMemoryBlocks != NULL)
    {
        for (uint32_t i = 0; i < sessionMemoryCount; ++i)
        {
            if (sessionMemoryBlocks[i] != VK_NULL_HANDLE)
            {
                vkFreeMemory(device, sessionMemoryBlocks[i], NULL);
            }
        }
    }
    if (device != VK_NULL_HANDLE && nv12View != VK_NULL_HANDLE)
    {
        vkDestroyImageView(device, nv12View, NULL);
    }
    if (device != VK_NULL_HANDLE && nv12Image != VK_NULL_HANDLE)
    {
        vkDestroyImage(device, nv12Image, NULL);
    }
    if (device != VK_NULL_HANDLE && nv12Memory != VK_NULL_HANDLE)
    {
        vkFreeMemory(device, nv12Memory, NULL);
    }
    if (device != VK_NULL_HANDLE && bitstreamBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(device, bitstreamBuffer, NULL);
    }
    if (device != VK_NULL_HANDLE && bitstreamMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(device, bitstreamMemory, NULL);
    }
    if (device != VK_NULL_HANDLE && stagingBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(device, stagingBuffer, NULL);
    }
    if (device != VK_NULL_HANDLE && stagingMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(device, stagingMemory, NULL);
    }
    if (device != VK_NULL_HANDLE && fence != VK_NULL_HANDLE)
    {
        vkDestroyFence(device, fence, NULL);
    }
    if (device != VK_NULL_HANDLE && cmdPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(device, cmdPool, NULL);
    }
    if (sessionMemoryBlocks != NULL)
    {
        free(sessionMemoryBlocks);
        sessionMemoryBlocks = NULL;
    }
    if (device != VK_NULL_HANDLE)
    {
        vkDestroyDevice(device, NULL);
    }
    if (instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(instance, NULL);
    }

    if (skipped)
    {
        return 0;
    }
    return success ? 0 : 1;
}



/*************************************************************************************************/
/*  Entry-point                                                                                  */
/*************************************************************************************************/

int test_video(TstSuite* suite)
{
    ANN(suite);

    const char* tags = "video";

    TEST_SIMPLE(test_video_1);
    TEST_SIMPLE(test_video_2);



    return 0;
}
