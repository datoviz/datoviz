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

const uint32_t WIDTH = DVZ_PROTO_WIDTH;
const uint32_t HEIGHT = DVZ_PROTO_HEIGHT;
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
        .maxBitrate = 8 * 1000 * 1000,
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
        .pNext = &ctrlInfo,
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
        vkCmdResetQueryPool(cmdsv.cmds[0], queryPool, 0, 1);

        vkCmdBeginVideoCodingKHR(cmdsv.cmds[0], &beginInfo);
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
/*  Entry-point                                                                                  */
/*************************************************************************************************/

int test_video(TstSuite* suite)
{
    ANN(suite);

    const char* tags = "video";

    TEST_SIMPLE(test_video_1);



    return 0;
}
