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

#include <cuda.h>
// #include <vk_video/vulkan_video_codec_h264std.h>        // level/profile enums (STD_VIDEO_*)
// #include <vk_video/vulkan_video_codec_h264std_encode.h> // std header name+version macros
// #include <vk_video/vulkan_video_codecs_common.h>
#include <volk.h>

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



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

// #define DVZ_ALIGN16(x) (((x) + 15u) & ~15u)

// // H.264 encoders operate on 16×16 macroblocks, so align render targets to multiples of 16 to
// avoid
// // validation/runtime failures when programming SPS/PPS.
// const uint32_t WIDTH = DVZ_ALIGN16(DVZ_PROTO_WIDTH);
// const uint32_t HEIGHT = DVZ_ALIGN16(DVZ_PROTO_HEIGHT);
// const uint32_t FPS = 30;
// const uint32_t SECS = 5;
// const uint32_t NFR = FPS * SECS;                              // 150
// const VkDeviceSize BITSTREAM_FRAME_SIZE = WIDTH * HEIGHT * 2; // generous per-frame budget

// typedef struct VideoBitstreamFeedback
// {
//     uint64_t offset;
//     uint64_t bytes;
// } VideoBitstreamFeedback;

// // --- Minimal SPS / PPS defaults ---
// static const StdVideoH264SequenceParameterSet sps = {
//     .seq_parameter_set_id = 0,
//     .profile_idc = STD_VIDEO_H264_PROFILE_IDC_MAIN,
//     .level_idc = STD_VIDEO_H264_LEVEL_IDC_4_1,
//     .chroma_format_idc = STD_VIDEO_H264_CHROMA_FORMAT_IDC_420,
//     .bit_depth_luma_minus8 = 0,
//     .bit_depth_chroma_minus8 = 0,
//     .pic_width_in_mbs_minus1 = (WIDTH / 16) - 1,
//     .pic_height_in_map_units_minus1 = (HEIGHT / 16) - 1,
//     .log2_max_frame_num_minus4 = 0,
//     .pic_order_cnt_type = 0,
//     .log2_max_pic_order_cnt_lsb_minus4 = 0,
// };

// static const StdVideoH264PictureParameterSet pps = {
//     .pic_parameter_set_id = 0,
//     .seq_parameter_set_id = 0,
//     .num_ref_idx_l0_default_active_minus1 = 0,
//     .num_ref_idx_l1_default_active_minus1 = 0,
// };



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

// static uint32_t
// find_memory_type(VkPhysicalDevice phys, uint32_t type_bits, VkMemoryPropertyFlags
// required_props)
// {
//     VkPhysicalDeviceMemoryProperties mem_props;
//     vkGetPhysicalDeviceMemoryProperties(phys, &mem_props);

//     // First pass: must satisfy both the type mask and all required properties.
//     for (uint32_t i = 0; i < mem_props.memoryTypeCount; ++i)
//     {
//         const uint32_t type_supported = (type_bits & (1u << i)) != 0u;
//         const VkMemoryPropertyFlags flags = mem_props.memoryTypes[i].propertyFlags;
//         if (type_supported && (flags & required_props) == required_props)
//         {
//             return i;
//         }
//     }

//     // Fallback: pick any supported type (useful when caller passes 0 required_props).
//     for (uint32_t i = 0; i < mem_props.memoryTypeCount; ++i)
//     {
//         if (type_bits & (1u << i))
//         {
//             return i;
//         }
//     }

//     assert(!"No suitable memory type found");
//     return UINT32_MAX;
// }


// static VkDeviceSize align_up(VkDeviceSize value, VkDeviceSize alignment)
// {
//     if (alignment == 0)
//     {
//         return value;
//     }
//     return (value + alignment - 1) & ~(alignment - 1);
// }


// static bool
// device_supports_extensions(VkPhysicalDevice pdev, const char** required, uint32_t count)
// {
//     if (count == 0)
//     {
//         return true;
//     }

//     uint32_t available = 0;
//     VK_CHECK_RESULT(vkEnumerateDeviceExtensionProperties(pdev, NULL, &available, NULL));

//     VkExtensionProperties* props = NULL;
//     if (available > 0)
//     {
//         props = (VkExtensionProperties*)calloc(available, sizeof(VkExtensionProperties));
//         ANN(props);
//         VK_CHECK_RESULT(vkEnumerateDeviceExtensionProperties(pdev, NULL, &available, props));
//     }

//     bool ok = true;
//     for (uint32_t i = 0; i < count && ok; ++i)
//     {
//         bool found = false;
//         for (uint32_t j = 0; j < available; ++j)
//         {
//             if (strncmp(required[i], props[j].extensionName, VK_MAX_EXTENSION_NAME_SIZE) == 0)
//             {
//                 found = true;
//                 break;
//             }
//         }
//         if (!found)
//         {
//             ok = false;
//         }
//     }

//     free(props);
//     return ok;
// }


// static bool find_video_encode_queue(VkPhysicalDevice phys, uint32_t* queue_family)
// {
//     ANN(queue_family);

//     uint32_t count = 0;
//     vkGetPhysicalDeviceQueueFamilyProperties2(phys, &count, NULL);
//     if (count == 0)
//     {
//         return false;
//     }

//     VkQueueFamilyProperties2* props =
//         (VkQueueFamilyProperties2*)calloc(count, sizeof(VkQueueFamilyProperties2));
//     ANN(props);
//     VkQueueFamilyVideoPropertiesKHR* video_props =
//         (VkQueueFamilyVideoPropertiesKHR*)calloc(count,
//         sizeof(VkQueueFamilyVideoPropertiesKHR));
//     ANN(video_props);

//     for (uint32_t i = 0; i < count; ++i)
//     {
//         props[i].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
//         props[i].pNext = &video_props[i];
//         video_props[i].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_VIDEO_PROPERTIES_KHR;
//         video_props[i].pNext = NULL;
//     }

//     vkGetPhysicalDeviceQueueFamilyProperties2(phys, &count, props);

//     bool found = false;
//     for (uint32_t i = 0; i < count; ++i)
//     {
//         const VkQueueFlags flags = props[i].queueFamilyProperties.queueFlags;
//         if ((flags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR) == 0)
//         {
//             continue;
//         }
//         if ((video_props[i].videoCodecOperations & VK_VIDEO_CODEC_OPERATION_ENCODE_H264_BIT_KHR)
//         ==
//             0)
//         {
//             continue;
//         }
//         *queue_family = i;
//         found = true;
//         break;
//     }

//     free(video_props);
//     free(props);
//     return found;
// }


// static bool pick_video_encode_device(
//     VkInstance instance, const char** required_exts, uint32_t ext_count,
//     VkPhysicalDevice* out_phys, uint32_t* out_queue_family)
// {
//     ANN(out_phys);
//     ANN(out_queue_family);

//     uint32_t device_count = 0;
//     VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &device_count, NULL));
//     if (device_count == 0)
//     {
//         return false;
//     }

//     VkPhysicalDevice* devices = (VkPhysicalDevice*)calloc(device_count,
//     sizeof(VkPhysicalDevice)); ANN(devices);
//     VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &device_count, devices));

//     bool found = false;
//     for (uint32_t i = 0; i < device_count; ++i)
//     {
//         if (!device_supports_extensions(devices[i], required_exts, ext_count))
//         {
//             continue;
//         }

//         uint32_t qf = 0;
//         if (!find_video_encode_queue(devices[i], &qf))
//         {
//             continue;
//         }

//         *out_phys = devices[i];
//         *out_queue_family = qf;
//         found = true;
//         break;
//     }

//     free(devices);
//     return found;
// }



// static bool video_encode_supported(DvzBootstrap* bootstrap)
// {
//     ANN(bootstrap);

//     DvzGpu* gpu = bootstrap->gpu;
//     ANN(gpu);

//     const char* extensions[] = {
//         VK_KHR_VIDEO_QUEUE_EXTENSION_NAME,
//         VK_KHR_VIDEO_ENCODE_QUEUE_EXTENSION_NAME,
//         VK_KHR_VIDEO_ENCODE_H264_EXTENSION_NAME,
//     };

//     for (uint32_t i = 0; i < DVZ_ARRAY_COUNT(extensions); ++i)
//     {
//         if (!dvz_gpu_has_extension(gpu, extensions[i]))
//         {
//             log_warn("Skipping test_video_1: GPU missing extension %s", extensions[i]);
//             return false;
//         }
//     }

//     DvzQueueCaps* caps = dvz_gpu_queue_caps(gpu);
//     ANN(caps);
//     for (uint32_t q = 0; q < caps->family_count; ++q)
//     {
//         if (caps->flags[q] & VK_QUEUE_VIDEO_ENCODE_BIT_KHR)
//         {
//             return true;
//         }
//     }

//     log_warn("Skipping test_video_1: no queue exposes VK_QUEUE_VIDEO_ENCODE_BIT_KHR");
//     return false;
// }



/*************************************************************************************************/
/*  Video tests                                                                                  */
/*************************************************************************************************/

int test_video_1(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    // // Bootstrap.
    // DvzBootstrap bootstrap = {0};
    // dvz_bootstrap(&bootstrap, DVZ_BOOTSTRAP_MANUAL_CREATE_DEVICE);

    // if (!video_encode_supported(&bootstrap))
    // {
    //     log_warn("Video encode stack unavailable on this GPU, skipping test_video_1");
    //     return 0;
    // }

    // DvzDevice* device = &bootstrap.device;
    // ANN(device);
    // dvz_device_request_extension(device, VK_KHR_VIDEO_QUEUE_EXTENSION_NAME);
    // dvz_device_request_extension(device, VK_KHR_VIDEO_ENCODE_QUEUE_EXTENSION_NAME);
    // dvz_device_request_extension(device, VK_KHR_VIDEO_ENCODE_H264_EXTENSION_NAME);

    // // Create a device with support for dynamic rendering.
    // VkPhysicalDeviceVulkan13Features* features = dvz_device_request_features13(device);
    // features->dynamicRendering = true;
    // features->synchronization2 = true;
    // AT(dvz_device_create(device) == 0);
    // dvz_device_allocator(device, 0, &bootstrap.allocator);



    // VkDevice vkdev = device->vk_device;
    // ANNVK(vkdev);

    // VkPhysicalDevice pdev = bootstrap.gpu->pdevice;
    // ANNVK(pdev);

    // DvzQueue* queue = dvz_device_queue(device, DVZ_QUEUE_MAIN);
    // ANN(queue);
    // ANNVK(queue->vk_queue);
    // uint32_t qf = dvz_queue_family(queue);

    // DvzQueue* queuev = dvz_device_queue(device, DVZ_QUEUE_VIDEO_ENCODE);
    // ANN(queuev);
    // ANNVK(queuev->vk_queue);
    // uint32_t qfv = queuev->family_idx;



    // // Graphics setup.
    // DvzGraphics graphics = {0};
    // dvz_graphics(device, &graphics);

    // // Shaders.
    // DvzShader vs = {0};
    // DvzShader fs = {0};
    // DvzSize vs_size = 0;
    // DvzSize fs_size = 0;
    // uint32_t* vs_spv = dvz_test_shader_load("hello_triangle.vert.spv", &vs_size);
    // uint32_t* fs_spv = dvz_test_shader_load("hello_triangle.frag.spv", &fs_size);
    // ANN(vs_spv);
    // ANN(fs_spv);
    // dvz_shader(device, vs_size, vs_spv, &vs);
    // dvz_shader(device, fs_size, fs_spv, &fs);
    // dvz_graphics_shader(&graphics, VK_SHADER_STAGE_VERTEX_BIT, dvz_shader_handle(&vs));
    // dvz_graphics_shader(&graphics, VK_SHADER_STAGE_FRAGMENT_BIT, dvz_shader_handle(&fs));

    // // Slots.
    // DvzSlots slots = {0};
    // dvz_slots(&bootstrap.device, &slots);
    // AT(dvz_slots_create(&slots) == 0);
    // dvz_graphics_layout(&graphics, dvz_slots_handle(&slots));

    // // Attachments.
    // dvz_graphics_attachment_color(&graphics, 0, VK_FORMAT_R8G8B8A8_UNORM);
    // dvz_graphics_blend_color(
    //     &graphics, 0, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    //     VK_BLEND_OP_ADD, 0xF);
    // dvz_graphics_blend_alpha(
    //     &graphics, 0, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD);

    // // Fixed state.
    // dvz_graphics_primitive(
    //     &graphics, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, DVZ_GRAPHICS_FLAGS_FIXED);

    // // Dynamic state.
    // dvz_graphics_viewport(&graphics, 0, 0, WIDTH, HEIGHT, 0, 1, DVZ_GRAPHICS_FLAGS_DYNAMIC);
    // dvz_graphics_scissor(&graphics, 0, 0, WIDTH, HEIGHT, DVZ_GRAPHICS_FLAGS_DYNAMIC);

    // // Graphics pipeline creation.
    // AT(dvz_graphics_create(&graphics) == 0);

    // // Rendering.
    // DvzRendering rendering = {0};
    // dvz_rendering(&rendering);
    // dvz_rendering_area(&rendering, 0, 0, WIDTH, HEIGHT);

    // // Image to render to.
    // DvzImages img = {0};
    // dvz_images(&bootstrap.device, &bootstrap.allocator, VK_IMAGE_TYPE_2D, 1, &img);
    // dvz_images_format(&img, VK_FORMAT_R8G8B8A8_UNORM);
    // dvz_images_size(&img, WIDTH, HEIGHT, 1);
    // dvz_images_usage(&img, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    // dvz_images_create(&img);

    // // Image views.
    // DvzImageViews view = {0};
    // dvz_image_views(&img, &view);
    // dvz_image_views_create(&view);

    // // Attachments.
    // DvzAttachment* attachment = dvz_rendering_color(&rendering, 0);
    // dvz_attachment_image(
    //     attachment, dvz_image_views_handle(&view, 0), VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL);
    // dvz_attachment_ops(attachment, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);
    // dvz_attachment_clear(attachment, (VkClearValue){.color.float32 = {.1, .2, .3, 1}});

    // // Image barrier.
    // DvzBarriers barriers = {0};
    // dvz_barriers(&barriers);

    // // Image transition.
    // DvzBarrierImage* bimg = dvz_barriers_image(&barriers, dvz_image_handle(&img, 0));
    // ANN(bimg);
    // dvz_barrier_image_stage(
    //     bimg, VK_PIPELINE_STAGE_2_NONE, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
    // dvz_barrier_image_access(bimg, 0, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
    // dvz_barrier_image_layout(bimg, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    // // Command buffer.
    // DvzCommands cmds = {0};
    // dvz_commands(device, queue, 1, &cmds);
    // dvz_cmd_begin(&cmds);
    // dvz_cmd_barriers(&cmds, 0, &barriers);
    // dvz_cmd_rendering_begin(&cmds, 0, &rendering);
    // dvz_cmd_bind_graphics(&cmds, 0, &graphics);
    // dvz_cmd_draw(&cmds, 0, 0, 3, 0, 1);
    // dvz_cmd_rendering_end(&cmds, 0);
    // dvz_cmd_end(&cmds);

    // // Submit the command buffer.
    // dvz_cmd_submit(&cmds);

    // //////////////


    // DvzImages nv12_tmp = {0};
    // dvz_images(&bootstrap.device, &bootstrap.allocator, VK_IMAGE_TYPE_2D, 1, &nv12_tmp);
    // dvz_images_flags(
    //     &nv12_tmp, VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT | VK_IMAGE_CREATE_EXTENDED_USAGE_BIT);
    // dvz_images_format(&nv12_tmp, VK_FORMAT_G8_B8R8_2PLANE_420_UNORM);
    // dvz_images_usage(&nv12_tmp, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    // dvz_images_size(&nv12_tmp, WIDTH, HEIGHT, 1);
    // dvz_images_create(&nv12_tmp);

    // VkImageView nv12_y_view = {0};
    // VkImageView nv12_uv_view = {0};

    // VkImageViewCreateInfo yViewCI = {
    //     .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    //     .image = nv12_tmp.vk_images[0],
    //     .viewType = VK_IMAGE_VIEW_TYPE_2D,
    //     .format = VK_FORMAT_R8_UNORM, // luma plane
    //     .subresourceRange =
    //         {
    //             .aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT,
    //             .baseMipLevel = 0,
    //             .levelCount = 1,
    //             .baseArrayLayer = 0,
    //             .layerCount = 1,
    //         },
    // };

    // VkImageViewCreateInfo uvViewCI = {
    //     .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    //     .image = nv12_tmp.vk_images[0],
    //     .viewType = VK_IMAGE_VIEW_TYPE_2D,
    //     .format = VK_FORMAT_R8G8_UNORM, // chroma plane
    //     .subresourceRange =
    //         {
    //             .aspectMask = VK_IMAGE_ASPECT_PLANE_1_BIT,
    //             .baseMipLevel = 0,
    //             .levelCount = 1,
    //             .baseArrayLayer = 0,
    //             .layerCount = 1,
    //         },
    // };

    // VK_CHECK_RESULT(vkCreateImageView(vkdev, &yViewCI, NULL, &nv12_y_view));
    // VK_CHECK_RESULT(vkCreateImageView(vkdev, &uvViewCI, NULL, &nv12_uv_view));



    // // binding 0: sampled RGBA source
    // DvzSampler sampler = {0};
    // dvz_sampler(device, &sampler);
    // AT(dvz_sampler_create(&sampler) == 0);


    // // Create a basic compute shader.

    // DvzSize cs_size = 0;
    // DvzShader cs = {0};
    // uint32_t* cs_spv = dvz_test_shader_load("rgb_to_nv12.comp.spv", &cs_size);

    // // Shaders.
    // dvz_shader(device, cs_size, cs_spv, &cs);

    // DvzCompute compute = {0};
    // dvz_compute(device, &compute);
    // // --- SLOTS (descriptor set layout) ------------------------------
    // DvzSlots slots_c = {0};
    // dvz_slots(device, &slots_c);

    // // binding 0 → input sampled RGBA image
    // dvz_slots_binding(
    //     &slots_c, 0, 0, 1, VK_SHADER_STAGE_COMPUTE_BIT,
    //     VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    // // binding 1 → Y plane output image
    // dvz_slots_binding(
    //     &slots_c, 0, 1, 1, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

    // // binding 2 → UV plane output image
    // dvz_slots_binding(
    //     &slots_c, 0, 2, 1, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

    // dvz_slots_create(&slots_c);
    // dvz_compute_layout(&compute, dvz_slots_handle(&slots_c));

    // // Create a compute pipeline.
    // dvz_compute_shader(&compute, dvz_shader_handle(&cs));
    // AT(dvz_compute_create(&compute) == 0);

    // // --- DESCRIPTORS (actual resource bindings) ---------------------
    // DvzDescriptors descs_c = {0};
    // dvz_descriptors(&slots_c, &descs_c);

    // dvz_descriptors_image(
    //     &descs_c, 0, 0, 0, VK_IMAGE_LAYOUT_GENERAL, dvz_image_views_handle(&view, 0),
    //     sampler.vk_sampler);

    // // binding 1: Y plane (plane 0 of NV12)
    // dvz_descriptors_image(&descs_c, 0, 1, 0, VK_IMAGE_LAYOUT_GENERAL, nv12_y_view,
    // VK_NULL_HANDLE);

    // // binding 2: UV plane (plane 1 of NV12)
    // dvz_descriptors_image(
    //     &descs_c, 0, 2, 0, VK_IMAGE_LAYOUT_GENERAL, nv12_uv_view, VK_NULL_HANDLE);



    // //////////////////////

    // // Codec-specific H.264 encode profile info
    // VkVideoEncodeH264ProfileInfoKHR h264Profile = {
    //     .sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_PROFILE_INFO_KHR,
    //     .pNext = NULL,
    //     // Choose a base H.264 profile; most drivers support MAIN or HIGH.
    //     .stdProfileIdc = STD_VIDEO_H264_PROFILE_IDC_MAIN,
    // };

    // // 8-bit 4:2:0 H.264 encode profile
    // VkVideoProfileInfoKHR videoProfile = {
    //     .sType = VK_STRUCTURE_TYPE_VIDEO_PROFILE_INFO_KHR,
    //     .pNext = &h264Profile,
    //     .videoCodecOperation = VK_VIDEO_CODEC_OPERATION_ENCODE_H264_BIT_KHR,
    //     .chromaSubsampling = VK_VIDEO_CHROMA_SUBSAMPLING_420_BIT_KHR,
    //     .lumaBitDepth = VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR,
    //     .chromaBitDepth = VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR,
    // };

    // // Required: std header name + version for the chosen codec
    // VkExtensionProperties h264StdHeader = {
    //     .extensionName = VK_STD_VULKAN_VIDEO_CODEC_H264_ENCODE_EXTENSION_NAME,
    //     .specVersion = VK_STD_VULKAN_VIDEO_CODEC_H264_ENCODE_SPEC_VERSION,
    // };

    // // Optional but recommended: H.264-specific create info (can be empty defaults)
    // VkVideoEncodeH264SessionCreateInfoKHR h264CreateInfo = {
    //     .sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_SESSION_CREATE_INFO_KHR, .pNext = NULL,
    //     // You can set .useMaxLevelIdc / .maxLevelIdc if you want to clamp.
    //     // Leaving defaults is fine for bring-up.
    // };

    // VkVideoSessionCreateInfoKHR sessionCI = {
    //     .sType = VK_STRUCTURE_TYPE_VIDEO_SESSION_CREATE_INFO_KHR,
    //     .pNext = &h264CreateInfo, // <— codec-specific struct, OK
    //     .queueFamilyIndex = qfv,  // encode-capable qfam
    //     .flags = 0,
    //     .pVideoProfile = &videoProfile, // <— REQUIRED
    //     .pictureFormat = VK_FORMAT_G8_B8R8_2PLANE_420_UNORM,
    //     .maxCodedExtent = (VkExtent2D){WIDTH, HEIGHT},
    //     .referencePictureFormat = VK_FORMAT_UNDEFINED, // no refs for intra-only
    //     .maxDpbSlots = 0,
    //     .maxActiveReferencePictures = 0,
    //     .pStdHeaderVersion = &h264StdHeader, // <— REQUIRED
    // };

    // VkVideoSessionKHR session = VK_NULL_HANDLE;
    // VK_CHECK_RESULT(vkCreateVideoSessionKHR(vkdev, &sessionCI, NULL, &session));
    // ANNVK(session);

    // // 1) Add-info: NOT in pNext chain of paramsCI.
    // VkVideoEncodeH264SessionParametersAddInfoKHR addInfo = {
    //     .sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_SESSION_PARAMETERS_ADD_INFO_KHR,
    //     .pNext = NULL,
    //     .stdSPSCount = 1,
    //     .pStdSPSs = &sps,
    //     .stdPPSCount = 1,
    //     .pStdPPSs = &pps,
    // };

    // // 2) H.264 session-parameters create-info: pNext = NULL.
    // //    Provide addInfo via pParametersAddInfo.
    // VkVideoEncodeH264SessionParametersCreateInfoKHR h264ParamsCI = {
    //     .sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_SESSION_PARAMETERS_CREATE_INFO_KHR,
    //     .pNext = NULL, // <-- important
    //     .maxStdSPSCount = 1,
    //     .maxStdPPSCount = 1,
    //     .pParametersAddInfo = &addInfo, // <-- here, not in pNext
    // };

    // // 3) Top-level session-parameters CI: pNext points to *h264ParamsCI only*.
    // VkVideoSessionParametersCreateInfoKHR paramsCI = {
    //     .sType = VK_STRUCTURE_TYPE_VIDEO_SESSION_PARAMETERS_CREATE_INFO_KHR,
    //     .pNext = &h264ParamsCI, // <-- ONLY this in pNext chain
    //     .videoSession = session,
    //     .videoSessionParametersTemplate = VK_NULL_HANDLE,
    // };

    // uint32_t bindCount = 0;

    // // First query the count.
    // vkGetVideoSessionMemoryRequirementsKHR(vkdev, session, &bindCount, NULL);

    // // Allocate array.
    // VkVideoSessionMemoryRequirementsKHR* memReqs =
    //     calloc(bindCount, sizeof(VkVideoSessionMemoryRequirementsKHR));

    // // Initialize each element's sType BEFORE the call.
    // for (uint32_t i = 0; i < bindCount; ++i)
    // {
    //     memReqs[i].sType = VK_STRUCTURE_TYPE_VIDEO_SESSION_MEMORY_REQUIREMENTS_KHR;
    //     memReqs[i].pNext = NULL;
    // }

    // // Now fill them.
    // vkGetVideoSessionMemoryRequirementsKHR(vkdev, session, &bindCount, memReqs);

    // // Prepare bind structs.
    // VkBindVideoSessionMemoryInfoKHR* binds =
    //     calloc(bindCount, sizeof(VkBindVideoSessionMemoryInfoKHR));

    // for (uint32_t i = 0; i < bindCount; ++i)
    // {
    //     VkMemoryAllocateInfo allocInfo = {
    //         .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    //         .allocationSize = memReqs[i].memoryRequirements.size,
    //         .memoryTypeIndex = find_memory_type(
    //             pdev, memReqs[i].memoryRequirements.memoryTypeBits,
    //             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    //     };
    //     VK_CHECK_RESULT(vkAllocateMemory(vkdev, &allocInfo, NULL, &binds[i].memory));

    //     binds[i].sType = VK_STRUCTURE_TYPE_BIND_VIDEO_SESSION_MEMORY_INFO_KHR;
    //     binds[i].pNext = NULL;
    //     binds[i].memoryBindIndex = memReqs[i].memoryBindIndex;
    //     binds[i].memoryOffset = 0;
    //     binds[i].memorySize = memReqs[i].memoryRequirements.size;
    // }

    // // Bind everything to the session.
    // VK_CHECK_RESULT(vkBindVideoSessionMemoryKHR(vkdev, session, bindCount, binds));

    // free(memReqs);
    // free(binds);

    // VkVideoSessionParametersKHR sessionParams = VK_NULL_HANDLE;
    // VK_CHECK_RESULT(vkCreateVideoSessionParametersKHR(vkdev, &paramsCI, NULL, &sessionParams));

    // // Grab the Annex B SPS/PPS blob once so the bitstream has proper headers.
    // VkVideoEncodeH264SessionParametersGetInfoKHR h264ParamsGetInfo = {
    //     .sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_SESSION_PARAMETERS_GET_INFO_KHR,
    //     .pNext = NULL,
    //     .writeStdSPS = VK_TRUE,
    //     .writeStdPPS = VK_TRUE,
    //     .stdSPSId = sps.seq_parameter_set_id,
    //     .stdPPSId = pps.pic_parameter_set_id,
    // };
    // VkVideoEncodeSessionParametersGetInfoKHR paramsGetInfo = {
    //     .sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_SESSION_PARAMETERS_GET_INFO_KHR,
    //     .pNext = &h264ParamsGetInfo,
    //     .videoSessionParameters = sessionParams,
    // };
    // size_t headerSize = 0;
    // VK_CHECK_RESULT(
    //     vkGetEncodedVideoSessionParametersKHR(vkdev, &paramsGetInfo, NULL, &headerSize, NULL));
    // AT(headerSize > 0);
    // uint8_t* headerBlob = (uint8_t*)calloc(headerSize, sizeof(uint8_t));
    // ANN(headerBlob);
    // VK_CHECK_RESULT(vkGetEncodedVideoSessionParametersKHR(
    //     vkdev, &paramsGetInfo, NULL, &headerSize, headerBlob));

    // FILE* f = fopen("out.h264", "wb");
    // ANN(f);
    // fwrite(headerBlob, 1, headerSize, f);
    // free(headerBlob);

    // VkVideoProfileListInfoKHR profileList = {
    //     .sType = VK_STRUCTURE_TYPE_VIDEO_PROFILE_LIST_INFO_KHR,
    //     .pNext = NULL,
    //     .profileCount = 1,
    //     .pProfiles = &videoProfile,
    // };

    // VkDeviceSize bitstreamSize = BITSTREAM_FRAME_SIZE;
    // VkBufferCreateInfo bufCI = {
    //     .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    //     .size = bitstreamSize,
    //     .usage = VK_BUFFER_USAGE_VIDEO_ENCODE_DST_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    //     .pNext = &profileList,
    // };
    // VkBuffer bitstream;
    // VK_CHECK_RESULT(vkCreateBuffer(vkdev, &bufCI, NULL, &bitstream));

    // // Allocate host-visible memory so we can read the encoded bytes
    // VkMemoryRequirements req;
    // vkGetBufferMemoryRequirements(vkdev, bitstream, &req);
    // VkMemoryAllocateInfo alloc = {
    //     .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    //     .allocationSize = req.size,
    //     .memoryTypeIndex = find_memory_type(
    //         pdev, req.memoryTypeBits,
    //         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
    // };
    // VkDeviceMemory bitstreamMem;
    // vkAllocateMemory(vkdev, &alloc, NULL, &bitstreamMem);
    // vkBindBufferMemory(vkdev, bitstream, bitstreamMem, 0);

    // VkQueryPool queryPool = VK_NULL_HANDLE;
    // VkQueryPoolVideoEncodeFeedbackCreateInfoKHR queryFeedback = {
    //     .sType = VK_STRUCTURE_TYPE_QUERY_POOL_VIDEO_ENCODE_FEEDBACK_CREATE_INFO_KHR,
    //     .pNext = &videoProfile,
    //     .encodeFeedbackFlags = VK_VIDEO_ENCODE_FEEDBACK_BITSTREAM_BUFFER_OFFSET_BIT_KHR |
    //                            VK_VIDEO_ENCODE_FEEDBACK_BITSTREAM_BYTES_WRITTEN_BIT_KHR,
    // };
    // VkQueryPoolCreateInfo queryInfo = {
    //     .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
    //     .pNext = &queryFeedback,
    //     .flags = 0,
    //     .queryType = VK_QUERY_TYPE_VIDEO_ENCODE_FEEDBACK_KHR,
    //     .queryCount = 1,
    // };
    // VK_CHECK_RESULT(vkCreateQueryPool(vkdev, &queryInfo, NULL, &queryPool));


    // // --- NV12 image for the encoder ---
    // DvzImages nv12 = {0};
    // dvz_images(&bootstrap.device, &bootstrap.allocator, VK_IMAGE_TYPE_2D, 1, &nv12);
    // dvz_images_format(&nv12, VK_FORMAT_G8_B8R8_2PLANE_420_UNORM);
    // dvz_images_size(&nv12, WIDTH, HEIGHT, 1);
    // // must be writable by compute + readable by encoder:
    // dvz_images_usage(
    //     &nv12, VK_IMAGE_USAGE_VIDEO_ENCODE_SRC_BIT_KHR | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    // nv12.info.pNext = &profileList;
    // nv12.info.sharingMode = VK_SHARING_MODE_CONCURRENT;
    // nv12.info.queueFamilyIndexCount = 2;
    // nv12.info.pQueueFamilyIndices = (uint32_t[]){qf, qfv};
    // dvz_images_create(&nv12);

    // // Create **plane views** (one for Y, one for interleaved UV)
    // DvzImageViews nv12_views = {0};
    // dvz_image_views(&nv12, &nv12_views);
    // // If your helper doesn’t support plane views directly, create raw VkImageViews with
    // // aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT for Y, and PLANE_1 for UV.
    // dvz_image_views_create(&nv12_views);



    // VkImageMemoryBarrier2 nvbarriers[2] = {
    //     {
    //         .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
    //         .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
    //         .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
    //         .srcAccessMask = 0,
    //         .dstAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
    //         .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    //         .newLayout = VK_IMAGE_LAYOUT_GENERAL,
    //         .image = nv12_tmp.vk_images[0],
    //         .subresourceRange =
    //             {
    //                 .aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT | VK_IMAGE_ASPECT_PLANE_1_BIT,
    //                 .baseMipLevel = 0,
    //                 .levelCount = 1,
    //                 .baseArrayLayer = 0,
    //                 .layerCount = 1,
    //             },
    //     },
    // };

    // // Command buffer.
    // DvzCommands cmds2 = {0};
    // dvz_commands(device, queue, 1, &cmds2);
    // dvz_cmd_begin(&cmds2);

    // vkCmdPipelineBarrier2(
    //     cmds2.cmds[0], &(VkDependencyInfo){
    //                        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
    //                        .imageMemoryBarrierCount = 1,
    //                        .pImageMemoryBarriers = nvbarriers,
    //                    });


    // // Bind and dispatch compute
    // dvz_cmd_bind_compute(&cmds2, 0, &compute);
    // dvz_cmd_bind_descriptors(&cmds2, 0, VK_PIPELINE_BIND_POINT_COMPUTE, &descs_c, 0, 1, 0,
    // NULL); uint32_t gx = (WIDTH + 15) / 16; uint32_t gy = (HEIGHT + 15) / 16;
    // dvz_cmd_dispatch(&cmds2, 0, gx, gy, 1);



    // vkCmdPipelineBarrier2(
    //     cmds2.cmds[0], &(VkDependencyInfo){
    //                        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
    //                        .imageMemoryBarrierCount = 2,
    //                        .pImageMemoryBarriers =
    //                            (VkImageMemoryBarrier2[]){
    //                                {
    //                                    // tmp → transfer src
    //                                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
    //                                    .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
    //                                    .dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
    //                                    .srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
    //                                    .dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
    //                                    .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
    //                                    .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    //                                    .image = nv12_tmp.vk_images[0],
    //                                    .subresourceRange =
    //                                        {
    //                                            .aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT |
    //                                                          VK_IMAGE_ASPECT_PLANE_1_BIT,
    //                                            .levelCount = 1,
    //                                            .layerCount = 1,
    //                                        },
    //                                },
    //                                {
    //                                    // encode_img → transfer dst
    //                                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
    //                                    .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
    //                                    .dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
    //                                    .srcAccessMask = 0,
    //                                    .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
    //                                    .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    //                                    .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    //                                    .image = nv12.vk_images[0],
    //                                    .subresourceRange =
    //                                        {
    //                                            .aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT |
    //                                                          VK_IMAGE_ASPECT_PLANE_1_BIT,
    //                                            .levelCount = 1,
    //                                            .layerCount = 1,
    //                                        },
    //                                },
    //                            },
    //                    });


    // // Plane 0 = Y
    // VkImageCopy2 regionY = {
    //     .sType = VK_STRUCTURE_TYPE_IMAGE_COPY_2,
    //     .srcSubresource =
    //         {
    //             .aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT,
    //             .mipLevel = 0,
    //             .baseArrayLayer = 0,
    //             .layerCount = 1,
    //         },
    //     .dstSubresource =
    //         {
    //             .aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT,
    //             .mipLevel = 0,
    //             .baseArrayLayer = 0,
    //             .layerCount = 1,
    //         },
    //     .extent = {WIDTH, HEIGHT, 1}, // Y plane full res
    // };

    // // Plane 1 = UV (half resolution)
    // VkImageCopy2 regionUV = {
    //     .sType = VK_STRUCTURE_TYPE_IMAGE_COPY_2,
    //     .srcSubresource =
    //         {
    //             .aspectMask = VK_IMAGE_ASPECT_PLANE_1_BIT,
    //             .mipLevel = 0,
    //             .baseArrayLayer = 0,
    //             .layerCount = 1,
    //         },
    //     .dstSubresource =
    //         {
    //             .aspectMask = VK_IMAGE_ASPECT_PLANE_1_BIT,
    //             .mipLevel = 0,
    //             .baseArrayLayer = 0,
    //             .layerCount = 1,
    //         },
    //     .extent = {WIDTH / 2, HEIGHT / 2, 1}, // chroma plane is 2×2 subsampled
    // };

    // VkCopyImageInfo2 copyInfo = {
    //     .sType = VK_STRUCTURE_TYPE_COPY_IMAGE_INFO_2,
    //     .srcImage = nv12_tmp.vk_images[0],
    //     .srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    //     .dstImage = nv12.vk_images[0],
    //     .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    //     .regionCount = 2,
    //     .pRegions = (VkImageCopy2[]){regionY, regionUV},
    // };

    // vkCmdCopyImage2(cmds2.cmds[0], &copyInfo);



    // dvz_cmd_end(&cmds2);
    // dvz_cmd_submit(&cmds2);



    // // Command buffer reused for encoding work.
    // DvzCommands cmdsv = {0};
    // dvz_commands(device, queuev, 1, &cmdsv);
    // dvz_cmd_begin(&cmdsv);
    // vkCmdPipelineBarrier2(
    //     cmdsv.cmds[0], &(VkDependencyInfo){
    //                        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
    //                        .imageMemoryBarrierCount = 1,
    //                        .pImageMemoryBarriers =
    //                            (VkImageMemoryBarrier2[]){
    //                                {
    //                                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
    //                                    .srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
    //                                    .dstStageMask = VK_PIPELINE_STAGE_2_VIDEO_ENCODE_BIT_KHR,
    //                                    .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
    //                                    .dstAccessMask = VK_ACCESS_2_VIDEO_ENCODE_READ_BIT_KHR,
    //                                    .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    //                                    .newLayout = VK_IMAGE_LAYOUT_VIDEO_ENCODE_SRC_KHR,
    //                                    .image = nv12.vk_images[0],
    //                                    .subresourceRange =
    //                                        {
    //                                            .aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT |
    //                                                          VK_IMAGE_ASPECT_PLANE_1_BIT,
    //                                            .levelCount = 1,
    //                                            .layerCount = 1,
    //                                        },
    //                                },
    //                            },
    //                    });
    // dvz_cmd_end(&cmdsv);
    // dvz_cmd_submit(&cmdsv);
    // dvz_cmd_reset(&cmdsv);

    // //------------------------------------------------------------
    // // Encode control structures reused per-frame
    // //------------------------------------------------------------

    // VkVideoEncodeRateControlLayerInfoKHR rcLayer = {
    //     .sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_RATE_CONTROL_LAYER_INFO_KHR,
    //     .pNext = NULL,
    //     .averageBitrate = 5 * 1000 * 1000, // 5 Mbps
    //     .maxBitrate = 5 * 1000 * 1000,
    //     .frameRateNumerator = 30,
    //     .frameRateDenominator = 1,
    // };

    // VkVideoEncodeRateControlInfoKHR rcInfo = {
    //     .sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_RATE_CONTROL_INFO_KHR,
    //     .pNext = NULL,
    //     .flags = 0,
    //     .rateControlMode = VK_VIDEO_ENCODE_RATE_CONTROL_MODE_CBR_BIT_KHR,
    //     .layerCount = 1,
    //     .pLayers = &rcLayer,
    //     .virtualBufferSizeInMs = 1000,
    //     .initialVirtualBufferSizeInMs = 1000,
    // };

    // VkVideoCodingControlInfoKHR ctrlInfo = {
    //     .sType = VK_STRUCTURE_TYPE_VIDEO_CODING_CONTROL_INFO_KHR,
    //     .pNext = &rcInfo,
    //     .flags = VK_VIDEO_CODING_CONTROL_ENCODE_RATE_CONTROL_BIT_KHR,
    // };

    // // 5. Begin-coding info itself
    // VkVideoBeginCodingInfoKHR beginInfo = {
    //     .sType = VK_STRUCTURE_TYPE_VIDEO_BEGIN_CODING_INFO_KHR,
    //     .pNext = &rcInfo,
    //     .flags = 0,
    //     .videoSession = session,                 // VkVideoSessionKHR
    //     .videoSessionParameters = sessionParams, // VkVideoSessionParametersKHR
    //     .referenceSlotCount = 0,
    //     .pReferenceSlots = NULL,
    // };


    // ///////////////

    // //------------------------------------------------------------
    // // Define the source picture (NV12 image to encode)
    // //------------------------------------------------------------
    // VkVideoPictureResourceInfoKHR srcPic = {
    //     .sType = VK_STRUCTURE_TYPE_VIDEO_PICTURE_RESOURCE_INFO_KHR,
    //     .pNext = NULL,
    //     .codedOffset = {0, 0},
    //     .codedExtent = {WIDTH, HEIGHT},
    //     .baseArrayLayer = 0,
    //     .imageViewBinding = nv12_views.vk_views[0], // VkImageView for the NV12 image
    // };

    // VkVideoEndCodingInfoKHR endInfo = {
    //     .sType = VK_STRUCTURE_TYPE_VIDEO_END_CODING_INFO_KHR,
    //     .pNext = NULL,
    //     .flags = 0,
    // };

    // for (uint32_t frame = 0; frame < NFR; ++frame)
    // {
    //     dvz_cmd_begin(&cmdsv);
    //     vkCmdBeginVideoCodingKHR(cmdsv.cmds[0], &beginInfo);

    //     // NOTE: DO NOT UNCOMMENT THIS
    //     // return 1;

    //     if (frame == 0)
    //     {
    //         vkCmdControlVideoCodingKHR(cmdsv.cmds[0], &ctrlInfo);
    //     }

    //     // ------------------------------
    //     // Std *picture* info (frame-wide)
    //     // ------------------------------
    //     StdVideoEncodeH264PictureInfo stdPic = {
    //         .seq_parameter_set_id = sps.seq_parameter_set_id,
    //         .pic_parameter_set_id = pps.pic_parameter_set_id,
    //         .idr_pic_id = frame,
    //         .primary_pic_type = STD_VIDEO_H264_PICTURE_TYPE_I,
    //         .frame_num = frame,
    //         .PicOrderCnt = 0,
    //         .temporal_id = 0,
    //         .pRefLists = NULL,
    //     };
    //     stdPic.flags.IdrPicFlag = 1;
    //     stdPic.flags.is_reference = 1;

    //     // ---------------------------------
    //     // Std *slice* header (one full slice)
    //     // ---------------------------------
    //     StdVideoEncodeH264SliceHeader stdSlice = {
    //         .first_mb_in_slice = 0,
    //         .slice_type = STD_VIDEO_H264_SLICE_TYPE_I,
    //         .slice_alpha_c0_offset_div2 = 0,
    //         .slice_beta_offset_div2 = 0,
    //         .slice_qp_delta = 0,
    //         .cabac_init_idc = STD_VIDEO_H264_CABAC_INIT_IDC_0,
    //         .disable_deblocking_filter_idc =
    //         STD_VIDEO_H264_DISABLE_DEBLOCKING_FILTER_IDC_DISABLED, .pWeightTable = NULL,
    //     };

    //     // ------------------------------
    //     // Wire into Vulkan encode structs
    //     // ------------------------------
    //     VkVideoEncodeH264NaluSliceInfoKHR nalu = {
    //         .sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_NALU_SLICE_INFO_KHR,
    //         .pNext = NULL,
    //         .constantQp = 0,
    //         .pStdSliceHeader = &stdSlice,
    //     };

    //     VkVideoEncodeH264PictureInfoKHR h264PicInfo = {
    //         .sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_PICTURE_INFO_KHR,
    //         .pNext = NULL,
    //         .naluSliceEntryCount = 1,
    //         .pNaluSliceEntries = &nalu,
    //         .pStdPictureInfo = &stdPic,
    //     };

    //     VkVideoEncodeInfoKHR encodeInfo = {
    //         .sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_INFO_KHR,
    //         .pNext = &h264PicInfo,
    //         .flags = 0,
    //         .dstBuffer = bitstream,
    //         .dstBufferOffset = 0,
    //         .dstBufferRange = bitstreamSize,
    //         .srcPictureResource = srcPic,
    //         .pSetupReferenceSlot = NULL,
    //         .referenceSlotCount = 0,
    //         .pReferenceSlots = NULL,
    //         .precedingExternallyEncodedBytes = 0,
    //     };

    //     vkCmdBeginQuery(cmdsv.cmds[0], queryPool, 0, 0);
    //     vkCmdEncodeVideoKHR(cmdsv.cmds[0], &encodeInfo);

    //     VkBufferMemoryBarrier2 bitstreamBarrier = {
    //         .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
    //         .srcStageMask = VK_PIPELINE_STAGE_2_VIDEO_ENCODE_BIT_KHR,
    //         .srcAccessMask = VK_ACCESS_2_VIDEO_ENCODE_WRITE_BIT_KHR,
    //         .dstStageMask = VK_PIPELINE_STAGE_2_HOST_BIT,
    //         .dstAccessMask = VK_ACCESS_2_HOST_READ_BIT,
    //         .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    //         .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    //         .buffer = bitstream,
    //         .offset = 0,
    //         .size = bitstreamSize,
    //     };

    //     vkCmdPipelineBarrier2(
    //         cmdsv.cmds[0], &(VkDependencyInfo){
    //                            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
    //                            .bufferMemoryBarrierCount = 1,
    //                            .pBufferMemoryBarriers = &bitstreamBarrier,
    //                        });

    //     vkCmdEndQuery(cmdsv.cmds[0], queryPool, 0);
    //     vkCmdEndVideoCodingKHR(cmdsv.cmds[0], &endInfo);
    //     dvz_cmd_end(&cmdsv);

    //     // DEBUG: KEEP THIS, DO NOT REMOVE
    //     return 1;

    //     dvz_cmd_submit(&cmdsv);

    //     VideoBitstreamFeedback feedback = {0};
    //     VK_CHECK_RESULT(vkGetQueryPoolResults(
    //         vkdev, queryPool, 0, 1, sizeof(feedback), &feedback, sizeof(feedback),
    //         VK_QUERY_RESULT_WAIT_BIT | VK_QUERY_RESULT_64_BIT));

    //     AT(feedback.bytes > 0);
    //     AT(feedback.offset + feedback.bytes <= bitstreamSize);

    //     void* data = NULL;
    //     VK_CHECK_RESULT(
    //         vkMapMemory(vkdev, bitstreamMem, feedback.offset, feedback.bytes, 0, &data));
    //     fwrite(data, 1, feedback.bytes, f);
    //     vkUnmapMemory(vkdev, bitstreamMem);

    //     dvz_cmd_reset(&cmdsv);
    // }

    // fclose(f);
    // if (queryPool != VK_NULL_HANDLE)
    // {
    //     vkDestroyQueryPool(vkdev, queryPool, NULL);
    // }
    // vkDestroyBuffer(vkdev, bitstream, NULL);
    // vkFreeMemory(vkdev, bitstreamMem, NULL);

    // // void* data = NULL;
    // // vkMapMemory(vkdev, bitstreamMem, 0, bitstreamSize, 0, &data);
    // // FILE* f = fopen("out.h264", "wb");
    // // fwrite(data, 1, bitstreamSize, f);
    // // fclose(f);
    // // vkUnmapMemory(vkdev, bitstreamMem);



    // {
    //     // // Staging buffer for screenshot.
    //     // DvzBuffer staging = {0};
    //     // DvzSize screenshot_size = WIDTH * HEIGHT * 4;
    //     // dvz_buffer(&bootstrap.device, &bootstrap.allocator, &staging);
    //     // dvz_buffer_size(&staging, screenshot_size);
    //     // dvz_buffer_flags(
    //     //     &staging, VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT |
    //     //     VMA_ALLOCATION_CREATE_MAPPED_BIT);
    //     // dvz_buffer_usage(&staging, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    //     // dvz_buffer_create(&staging);

    //     // // Screenshot.
    //     // dvz_cmd_reset(&cmds);
    //     // dvz_cmd_begin(&cmds);

    //     // // Layout transition.
    //     // dvz_barrier_image_stage(
    //     //     bimg, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
    //     //     VK_PIPELINE_STAGE_2_TRANSFER_BIT);
    //     // dvz_barrier_image_access(
    //     //     bimg, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_2_TRANSFER_READ_BIT);
    //     // dvz_barrier_image_layout(
    //     //     bimg, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    //     //     VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    //     // dvz_cmd_barriers(&cmds, 0, &barriers);

    //     // // Copy image to buffer.
    //     // DvzImageRegion region = {0};
    //     // dvz_image_region(&region);
    //     // dvz_image_region_extent(&region, WIDTH, HEIGHT, 1);
    //     // dvz_cmd_copy_image_to_buffer(
    //     //     &cmds, 0, dvz_image_handle(&img, 0), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    //     &region,
    //     //     dvz_buffer_handle(&staging), 0);

    //     // // End the command buffer.
    //     // dvz_cmd_end(&cmds);

    //     // // Submit the command buffer.
    //     // dvz_cmd_submit(&cmds);

    //     // // Recover the screenshot.
    //     // uint8_t* screenshot = (uint8_t*)dvz_calloc(WIDTH * HEIGHT, 4);
    //     // dvz_buffer_download(&staging, 0, screenshot_size, screenshot);
    //     // dvz_write_png("build/video.png", WIDTH, HEIGHT, screenshot);

    //     // // Cleanup.
    //     // log_debug("cleanup");
    //     // dvz_image_views_destroy(&view);
    //     // dvz_images_destroy(&img);
    //     // dvz_buffer_destroy(&staging);
    //     // dvz_shader_destroy(&vs);
    //     // dvz_shader_destroy(&fs);
    //     // dvz_slots_destroy(&slots);
    //     // dvz_graphics_destroy(&graphics);
    //     // dvz_bootstrap_destroy(&bootstrap);
    //     // dvz_free(vs_spv);
    //     // dvz_free(fs_spv);
    //     // dvz_free(screenshot);
    // }
    // return bootstrap.instance.n_errors > 0;

    return 0;
}



/*************************************************************************************************/
/*  NVENC video encode                                                                           */
/*************************************************************************************************/


// ====== Params ======
#define WIDTH   1920
#define HEIGHT  1080
#define FPS     60
#define SECONDS 5
#define NFRAMES (FPS * SECONDS)

// Solid color to clear with Vulkan (uint8)
#define CLEAR_R 0
#define CLEAR_G 128
#define CLEAR_B 255
#define CLEAR_A 255

static VkClearColorValue frame_clear_color(uint32_t frame_idx, uint32_t total_frames)
{
    VkClearColorValue clr = {.float32 = {CLEAR_R / 255.0f, CLEAR_G / 255.0f, CLEAR_B / 255.0f, 1.0f}};
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

// NV12 pitch alignment for NVENC (use 256 for safety)
#define PITCH_ALIGN 256

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
#define CU_CHECK(x)                                                                               \
    do                                                                                            \
    {                                                                                             \
        CUresult _e = (x);                                                                        \
        if (_e != CUDA_SUCCESS)                                                                   \
        {                                                                                         \
            const char* _s = NULL;                                                                \
            cuGetErrorName(_e, &_s);                                                              \
            fprintf(stderr, "CUDA error %s at %s:%d\n", _s ? _s : "?", __FILE__, __LINE__);       \
            exit(1);                                                                              \
        }                                                                                         \
    } while (0)
#define NVENC_API_CALL(x)                                                                         \
    do                                                                                            \
    {                                                                                             \
        NVENCSTATUS _s = (x);                                                                     \
        if (_s != NV_ENC_SUCCESS)                                                                 \
        {                                                                                         \
            fprintf(stderr, "NVENC error %d at %s:%d\n", (int)_s, __FILE__, __LINE__);            \
            exit(1);                                                                              \
        }                                                                                         \
    } while (0)

// ====== Minimal Vulkan loader for external FD function ======
static PFN_vkGetMemoryFdKHR vkGetMemoryFdKHR_ptr = NULL;
static PFN_vkGetSemaphoreFdKHR vkGetSemaphoreFdKHR_ptr = NULL;

// ====== CUDA kernel: RGBA8 -> NV12 ======
static const char* ptx_rgba_to_nv12 =
    "// PTX will be JIT-compiled from CUDA C kernel embedded below if preferred.\n";

// For simplicity and readability, we compile a small kernel with NVRTC-like approach ordinarily.
// Here, we provide a precompiled kernel via fatbin in real projects. To keep this file standalone,
// we'll use cuModuleLoadDataEx with CUDA C source compiled by NVRTC is not allowed (no NVRTC).
// So we implement the kernel with inline PTX? That’s cumbersome.
// Instead: use the Driver API to load a small cubin/ptx string compiled offline.
// To keep this example portable, we implement the kernel via cuLaunchKernel on a simple device
// function using CUDA's JIT from PTX embedded as a string. Provide PTX for a basic kernel.

// PTX for a simple RGBA8->NV12 kernel (sm_80+). It assumes:
// - src: uchar4* pitch_src (bytes per row = src_pitch)
// - dst_y: uint8_t* pitch_dst (bytes per row = dst_pitch)
// - dst_uv: immediately after Y plane at dst_base + dst_pitch*HEIGHT
// Kernel writes one pixel per thread; UV plane updated by threads where both coordinates are even.
// It computes a simple BT.601-ish luma approximation and lightweight chroma contribution.
static const char* PTX =
    ".version 7.8\n"
    ".target sm_52\n"
    ".address_size 64\n"
    "\n"
    ".visible .entry rgba2nv12(\n"
    "    .param .u64 param_src,\n"
    "    .param .u32 param_src_pitch,\n"
    "    .param .u64 param_dst,\n"
    "    .param .u32 param_dst_pitch,\n"
    "    .param .u32 param_width,\n"
    "    .param .u32 param_height)\n"
    "{\n"
    "    .reg .pred %p<4>;\n"
    "    .reg .b32  %r<60>;\n"
    "    .reg .b64  %rd<12>;\n"
    "\n"
    "    ld.param.u64 %rd0, [param_src];\n"
    "    ld.param.u32 %r0,  [param_src_pitch];\n"
    "    ld.param.u64 %rd1, [param_dst];\n"
    "    ld.param.u32 %r1,  [param_dst_pitch];\n"
    "    ld.param.u32 %r2,  [param_width];\n"
    "    ld.param.u32 %r3,  [param_height];\n"
    "\n"
    "    mov.u32 %r10, %ctaid.x;\n"
    "    mov.u32 %r11, %ctaid.y;\n"
    "    mov.u32 %r12, %ntid.x;\n"
    "    mov.u32 %r13, %ntid.y;\n"
    "    mov.u32 %r14, %tid.x;\n"
    "    mov.u32 %r15, %tid.y;\n"
    "\n"
    "    mad.lo.u32 %r16, %r10, %r12, %r14;\n"
    "    mad.lo.u32 %r17, %r11, %r13, %r15;\n"
    "\n"
    "    setp.ge.u32 %p0, %r16, %r2;\n"
    "    @%p0 bra DONE;\n"
    "    setp.ge.u32 %p1, %r17, %r3;\n"
    "    @%p1 bra DONE;\n"
    "\n"
    "    mul.lo.u32 %r18, %r17, %r0;\n"
    "    shl.b32 %r19, %r16, 2;\n"
    "    add.u32 %r18, %r18, %r19;\n"
    "    cvt.u64.u32 %rd2, %r18;\n"
    "    add.u64 %rd2, %rd0, %rd2;\n"
    "\n"
    "    ld.global.u8 %r20, [%rd2];\n"
    "    ld.global.u8 %r21, [%rd2+1];\n"
    "    ld.global.u8 %r22, [%rd2+2];\n"
    "\n"
    "    cvt.u32.u8 %r23, %r20;\n"
    "    cvt.u32.u8 %r24, %r21;\n"
    "    cvt.u32.u8 %r25, %r22;\n"
    "\n"
    "    add.u32 %r26, %r24, %r24;\n"
    "    add.u32 %r26, %r26, %r23;\n"
    "    add.u32 %r26, %r26, %r25;\n"
    "    shr.u32 %r26, %r26, 2;\n"
    "    add.u32 %r26, %r26, 16;\n"
    "    min.u32 %r26, %r26, 255;\n"
    "\n"
    "    mul.lo.u32 %r27, %r17, %r1;\n"
    "    add.u32 %r27, %r27, %r16;\n"
    "    cvt.u64.u32 %rd3, %r27;\n"
    "    add.u64 %rd3, %rd1, %rd3;\n"
    "    st.global.u8 [%rd3], %r26;\n"
    "\n"
    "    and.b32 %r30, %r16, 1;\n"
    "    and.b32 %r31, %r17, 1;\n"
    "    or.b32 %r32, %r30, %r31;\n"
    "    setp.ne.u32 %p2, %r32, 0;\n"
    "    @%p2 bra DONE;\n"
    "\n"
    "    shr.u32 %r33, %r16, 1;\n"
    "    shr.u32 %r34, %r17, 1;\n"
    "\n"
    "    mul.lo.u32 %r35, %r3, %r1;\n"
    "    cvt.u64.u32 %rd4, %r35;\n"
    "    add.u64 %rd4, %rd1, %rd4;\n"
    "\n"
    "    mul.lo.u32 %r36, %r34, %r1;\n"
    "    shl.b32 %r37, %r33, 1;\n"
    "    add.u32 %r36, %r36, %r37;\n"
    "    cvt.u64.u32 %rd5, %r36;\n"
    "    add.u64 %rd4, %rd4, %rd5;\n"
    "\n"
    "    add.u32 %r40, %r33, %r34;\n"
    "    add.u32 %r40, %r40, 128;\n"
    "    min.u32 %r40, %r40, 255;\n"
    "\n"
    "    add.u32 %r41, %r23, 128;\n"
    "    min.u32 %r41, %r41, 255;\n"
    "\n"
    "    st.global.u8 [%rd4], %r40;\n"
    "    st.global.u8 [%rd4+1], %r41;\n"
    "\n"
    "DONE:\n"
    "    ret;\n"
    "}\n";


// ====== Utility: align up ======
static inline uint32_t align_up(uint32_t v, uint32_t a) { return (v + a - 1) & ~(a - 1); }

// ====== Vulkan objects ======
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
} VulkanCtx;

// ====== NVENC dynamic API list ======
static NV_ENCODE_API_FUNCTION_LIST g_nvenc = {0};

static bool nvenc_guid_equal(const GUID* a, const GUID* b)
{
    ANN(a);
    ANN(b);
    return memcmp(a, b, sizeof(GUID)) == 0;
}

static bool nvenc_supports_codec(void* hEncoder, const GUID* codec)
{
    ANN(codec);
    if (hEncoder == NULL || g_nvenc.nvEncGetEncodeGUIDCount == NULL || g_nvenc.nvEncGetEncodeGUIDs == NULL)
    {
        return false;
    }

    uint32_t count = 0;
    if (g_nvenc.nvEncGetEncodeGUIDCount(hEncoder, &count) != NV_ENC_SUCCESS || count == 0)
    {
        return false;
    }

    GUID* guids = (GUID*)calloc(count, sizeof(GUID));
    if (guids == NULL)
    {
        return false;
    }

    uint32_t written = 0;
    NVENCSTATUS st = g_nvenc.nvEncGetEncodeGUIDs(hEncoder, guids, count, &written);
    bool supported = false;
    if (st == NV_ENC_SUCCESS)
    {
        for (uint32_t i = 0; i < written; i++)
        {
            if (nvenc_guid_equal(&guids[i], codec))
            {
                supported = true;
                break;
            }
        }
    }

    free(guids);
    return supported;
}

// ====== Minimal Vulkan setup & image creation/clear ======
static void vk_init_and_make_image(VulkanCtx* vk)
{
    VK_CHECK(volkInitialize());
    memset(vk, 0, sizeof(*vk));
    vk->memory_fd = -1;
    vk->semaphore_fd = -1;

    // Instance with no layers, minimal ext (we'll fetch device funcs later)
    VkApplicationInfo app = {.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "vk_cuda_nvenc_h265";
    app.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo ici = {.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    ici.pApplicationInfo = &app;
    VK_CHECK(vkCreateInstance(&ici, NULL, &vk->instance));
    volkLoadInstance(vk->instance);

    // Pick first physical device with required extensions for external memory FD
    uint32_t np = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(vk->instance, &np, NULL));
    VkPhysicalDevice* pds = (VkPhysicalDevice*)malloc(sizeof(VkPhysicalDevice) * np);
    VK_CHECK(vkEnumeratePhysicalDevices(vk->instance, &np, pds));
    vk->phys = pds[0];
    free(pds);

    // Find a queue family with graphics
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

    float prio = 1.0f;
    VkDeviceQueueCreateInfo qci = {.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    qci.queueFamilyIndex = vk->queueFamily;
    qci.queueCount = 1;
    qci.pQueuePriorities = &prio;

    const char* devExts[] = {
        VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
        VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,
        VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME,
        VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME,
    };

    VkDeviceCreateInfo dci = {.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    dci.queueCreateInfoCount = 1;
    dci.pQueueCreateInfos = &qci;
    dci.enabledExtensionCount = (uint32_t)(sizeof(devExts) / sizeof(devExts[0]));
    dci.ppEnabledExtensionNames = devExts;

    VK_CHECK(vkCreateDevice(vk->phys, &dci, NULL, &vk->device));
    volkLoadDevice(vk->device);
    vkGetDeviceQueue(vk->device, vk->queueFamily, 0, &vk->queue);

    // Load vkGetMemoryFdKHR
    vkGetMemoryFdKHR_ptr =
        (PFN_vkGetMemoryFdKHR)vkGetDeviceProcAddr(vk->device, "vkGetMemoryFdKHR");
    if (!vkGetMemoryFdKHR_ptr)
    {
        fprintf(stderr, "vkGetMemoryFdKHR not found\n");
        exit(1);
    }
    vkGetSemaphoreFdKHR_ptr =
        (PFN_vkGetSemaphoreFdKHR)vkGetDeviceProcAddr(vk->device, "vkGetSemaphoreFdKHR");
    if (!vkGetSemaphoreFdKHR_ptr)
    {
        fprintf(stderr, "vkGetSemaphoreFdKHR not found\n");
        exit(1);
    }

    // Command pool and buffer
    VkCommandPoolCreateInfo cpci = {.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    cpci.queueFamilyIndex = vk->queueFamily;
    cpci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK(vkCreateCommandPool(vk->device, &cpci, NULL, &vk->cmdPool));

    VkCommandBufferAllocateInfo cbai = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cbai.commandPool = vk->cmdPool;
    cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = 1;
    VK_CHECK(vkAllocateCommandBuffers(vk->device, &cbai, &vk->cmd));

    VkFenceCreateInfo fci = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    VK_CHECK(vkCreateFence(vk->device, &fci, NULL, &vk->fence));

    VkExportSemaphoreCreateInfo esci = {
        .sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO,
        .handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT,
    };
    VkSemaphoreCreateInfo sci = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = &esci,
    };
    VK_CHECK(vkCreateSemaphore(vk->device, &sci, NULL, &vk->semaphore));
    VkSemaphoreGetFdInfoKHR semfd = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR,
        .semaphore = vk->semaphore,
        .handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT,
    };
    VK_CHECK(vkGetSemaphoreFdKHR_ptr(vk->device, &semfd, &vk->semaphore_fd));
    if (vk->semaphore_fd < 0)
    {
        fprintf(stderr, "Failed to export semaphore fd\n");
        exit(1);
    }

    // Create exportable RGBA8 image
    VkExternalMemoryImageCreateInfo emici = {
        .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO};
    emici.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;

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

    // Pick memory type device-local
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

    // Export memory FD
    VkMemoryGetFdInfoKHR getfd = {.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR};
    getfd.memory = vk->memory;
    getfd.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
    VK_CHECK(vkGetMemoryFdKHR_ptr(vk->device, &getfd, &vk->memory_fd));
    if (vk->memory_fd < 0)
    {
        fprintf(stderr, "Failed to export memory FD\n");
        exit(1);
    }
}

static void vk_render_frame_and_sync(VulkanCtx* vk, const VkClearColorValue* clr)
{
    ANN(vk);
    ANNVK(vk->cmd);
    ANN(clr);

    VK_CHECK(vkResetCommandBuffer(vk->cmd, 0));

    VkCommandBufferBeginInfo begin = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_CHECK(vkBeginCommandBuffer(vk->cmd, &begin));

    VkPipelineStageFlags2 src_stage =
        (vk->image_layout == VK_IMAGE_LAYOUT_UNDEFINED)
            ? VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT
            : VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    VkAccessFlags2 src_access =
        (vk->image_layout == VK_IMAGE_LAYOUT_UNDEFINED)
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
    vkCmdClearColorImage(
        vk->cmd, vk->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, clr, 1, &range);

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

    VK_CHECK(vkResetFences(vk->device, 1, &vk->fence));
    VkSemaphore signal_sems[1] = {vk->semaphore};
    VkSubmitInfo submit = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &vk->cmd,
        .signalSemaphoreCount = (vk->semaphore != VK_NULL_HANDLE) ? 1 : 0,
        .pSignalSemaphores = (vk->semaphore != VK_NULL_HANDLE) ? signal_sems : NULL,
    };
    VK_CHECK(vkQueueSubmit(vk->queue, 1, &submit, vk->fence));
    VK_CHECK(vkWaitForFences(vk->device, 1, &vk->fence, VK_TRUE, UINT64_MAX));

    vk->image_layout = VK_IMAGE_LAYOUT_GENERAL;
}

// ====== CUDA: import Vulkan memory as external memory, map to array ======
typedef struct
{
    CUcontext cuCtx;
    CUexternalMemory extMem;
    CUmipmappedArray mmArr;
    CUarray arr0;
    CUmodule cuMod;
    CUfunction cuFun;
    CUstream stream;
} CudaCtx;

static void cuda_import_vk_memory(CudaCtx* cu, int mem_fd, size_t alloc_size)
{
    memset(cu, 0, sizeof(*cu));
    CUdevice dev;
    CU_CHECK(cuInit(0));
    CU_CHECK(cuDeviceGet(&dev, 0));
    CU_CHECK(cuCtxCreate(&cu->cuCtx, 0, dev));
    CU_CHECK(cuStreamCreate(&cu->stream, CU_STREAM_DEFAULT));

    CUDA_EXTERNAL_MEMORY_HANDLE_DESC hdesc = {0};
    hdesc.type = CU_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD;
    hdesc.handle.fd = mem_fd;
    hdesc.size = alloc_size;
    CU_CHECK(cuImportExternalMemory(&cu->extMem, &hdesc));

    CUDA_EXTERNAL_MEMORY_MIPMAPPED_ARRAY_DESC mdesc = {0};
    mdesc.offset = 0;
    mdesc.numLevels = 1;
    mdesc.arrayDesc.Width = WIDTH;
    mdesc.arrayDesc.Height = HEIGHT;
    mdesc.arrayDesc.Depth = 0;
    mdesc.arrayDesc.NumChannels = 4;
    mdesc.arrayDesc.Format = CU_AD_FORMAT_UNSIGNED_INT8;
    mdesc.arrayDesc.Flags = 0; // no surface flags, we will memcpy from array

    CU_CHECK(cuExternalMemoryGetMappedMipmappedArray(&cu->mmArr, cu->extMem, &mdesc));
    CU_CHECK(cuMipmappedArrayGetLevel(&cu->arr0, cu->mmArr, 0));

    // Load PTX module and fetch kernel
    CU_CHECK(cuModuleLoadDataEx(&cu->cuMod, PTX, 0, NULL, NULL));
    CU_CHECK(cuModuleGetFunction(&cu->cuFun, cu->cuMod, "rgba2nv12"));
}

// ====== NVENC helpers ======
static void nvenc_load_api()
{
    memset(&g_nvenc, 0, sizeof(g_nvenc));
    g_nvenc.version = NV_ENCODE_API_FUNCTION_LIST_VER;
    NVENC_API_CALL(NvEncodeAPICreateInstance(&g_nvenc));
}

typedef struct
{
    void* dummy; // reserved
} NvEncOpaque;

typedef struct
{
    void* enc; // NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS managed handle
    void* hEncoder;
    NV_ENC_INITIALIZE_PARAMS init;
    NV_ENC_CONFIG encCfg;
} NvEncCtx;

static void nvenc_open_session_cuda(NvEncCtx* nctx, CUcontext cuCtx)
{
    memset(nctx, 0, sizeof(*nctx));

    NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS open = {0};
    open.version = NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS_VER;
    open.device = cuCtx;
    open.deviceType = NV_ENC_DEVICE_TYPE_CUDA;
    open.apiVersion = NVENCAPI_VERSION;

    NV_ENC_INPUT_RESOURCE_OPENGL_TEX* dummy = NULL;
    (void)dummy;
    NVENC_API_CALL(g_nvenc.nvEncOpenEncodeSessionEx(&open, &nctx->hEncoder));
}

static void nvenc_init_hevc(NvEncCtx* nctx, uint32_t width, uint32_t height, uint32_t fps)
{
    // Fill defaults
    memset(&nctx->init, 0, sizeof(nctx->init));
    memset(&nctx->encCfg, 0, sizeof(nctx->encCfg));
    nctx->init.version = NV_ENC_INITIALIZE_PARAMS_VER;
    nctx->encCfg.version = NV_ENC_CONFIG_VER;
    nctx->init.tuningInfo = NV_ENC_TUNING_INFO_HIGH_QUALITY;

    GUID codec = NV_ENC_CODEC_HEVC_GUID;
    GUID preset = NV_ENC_PRESET_P5_GUID; // HQ preset

    // Get preset config
    NV_ENC_PRESET_CONFIG pcfg = {0};
    pcfg.version = NV_ENC_PRESET_CONFIG_VER;
    pcfg.presetCfg.version = NV_ENC_CONFIG_VER;
    NVENCSTATUS preset_status = NV_ENC_SUCCESS;
    if (g_nvenc.nvEncGetEncodePresetConfigEx != NULL)
    {
        preset_status = g_nvenc.nvEncGetEncodePresetConfigEx(
            nctx->hEncoder, codec, preset, nctx->init.tuningInfo, &pcfg);
    }
    else
    {
        preset_status = g_nvenc.nvEncGetEncodePresetConfig(nctx->hEncoder, codec, preset, &pcfg);
    }
    NVENC_API_CALL(preset_status);

    nctx->encCfg = pcfg.presetCfg;
    nctx->encCfg.version = NV_ENC_CONFIG_VER;
    nctx->encCfg.rcParams.enableLookahead = 0;
    nctx->encCfg.rcParams.lookaheadDepth = 0;
    nctx->encCfg.rcParams.multiPass = NV_ENC_MULTI_PASS_DISABLED;

    // Use const QP for static content
    nctx->encCfg.rcParams.rateControlMode = NV_ENC_PARAMS_RC_CONSTQP;
    nctx->encCfg.rcParams.constQP.qpIntra = 20;
    nctx->encCfg.rcParams.constQP.qpInterP = 20;
    nctx->encCfg.rcParams.constQP.qpInterB = 22;

    nctx->init.encodeGUID = codec;
    nctx->init.presetGUID = preset;
    nctx->init.encodeWidth = width;
    nctx->init.encodeHeight = height;
    nctx->init.maxEncodeWidth = width;
    nctx->init.maxEncodeHeight = height;
    nctx->init.darWidth = width;
    nctx->init.darHeight = height;
    nctx->init.frameRateNum = fps;
    nctx->init.frameRateDen = 1;
    nctx->init.enablePTD = 1;
    nctx->init.reportSliceOffsets = 0;
    nctx->init.enableEncodeAsync = 0; // simpler sync path
    nctx->init.enableSubFrameWrite = 0;
    nctx->init.encodeConfig = &nctx->encCfg;
    nctx->init.enableOutputInVidmem = 0;

    // Important: input format NV12 pitch-linear
    nctx->init.encodeConfig->profileGUID = NV_ENC_HEVC_PROFILE_MAIN_GUID;
    nctx->init.encodeConfig->gopLength = fps * 2; // not critical here
    nctx->init.encodeConfig->frameIntervalP = 1;

    NVENC_API_CALL(g_nvenc.nvEncInitializeEncoder(nctx->hEncoder, &nctx->init));
}

// Allocate a single contiguous NV12 device buffer: size = pitch*height (Y) + pitch*(height/2) (UV)
typedef struct
{
    CUdeviceptr dptr; // base of NV12
    uint32_t pitch;   // bytes per row (for both Y and UV rows)
    size_t size;      // total bytes
} Nv12Buf;

static void alloc_nv12(Nv12Buf* nb, uint32_t w, uint32_t h, uint32_t pitch_align)
{
    uint32_t pitch = align_up(w, pitch_align);
    size_t y_bytes = (size_t)pitch * h;
    size_t uv_bytes = (size_t)pitch * (h / 2);
    size_t total = y_bytes + uv_bytes;
    CU_CHECK(cuMemAlloc(&nb->dptr, total));
    nb->pitch = pitch;
    nb->size = total;
}

// Allocate a device linear RGBA buffer with chosen pitch
typedef struct
{
    CUdeviceptr dptr;
    uint32_t pitch;
    size_t size;
} RgbaBuf;

static void alloc_rgba(RgbaBuf* rb, uint32_t w, uint32_t h, uint32_t pitch_align)
{
    uint32_t pitch = align_up(w * 4, pitch_align);
    size_t total = (size_t)pitch * h;
    CU_CHECK(cuMemAlloc(&rb->dptr, total));
    rb->pitch = pitch;
    rb->size = total;
}

// Copy from CUarray (imported Vulkan image) to linear RGBA device buffer (device→device)
static void copy_array_to_linear_rgba(CudaCtx* cu, RgbaBuf* rb, uint32_t w, uint32_t h)
{
    ANN(cu);
    CUDA_MEMCPY2D c2d = {0};
    c2d.srcMemoryType = CU_MEMORYTYPE_ARRAY;
    c2d.srcArray = cu->arr0;
    c2d.dstMemoryType = CU_MEMORYTYPE_DEVICE;
    c2d.dstDevice = rb->dptr;
    c2d.dstPitch = rb->pitch;
    c2d.WidthInBytes = w * 4;
    c2d.Height = h;
    CU_CHECK(cuMemcpy2DAsync(&c2d, cu->stream)); // device-to-device
}

// Launch RGBA->NV12 kernel
static void launch_rgba_to_nv12(CudaCtx* cu, RgbaBuf* rb, Nv12Buf* nb, uint32_t w, uint32_t h)
{
    // Kernel processes one pixel per thread -> blocks = ceil(w/Bx), ceil(h/By)
    const int Bx = 32, By = 16;
    int gx = ((int)w + Bx - 1) / Bx;
    int gy = ((int)h + By - 1) / By;

    void* args[] = {(void*)&rb->dptr,  (void*)&rb->pitch, (void*)&nb->dptr,
                    (void*)&nb->pitch, (void*)&w,         (void*)&h};

    CU_CHECK(cuLaunchKernel(cu->cuFun, gx, gy, 1, Bx, By, 1, 0, cu->stream, args, NULL));
    CU_CHECK(cuStreamSynchronize(cu->stream));
}

// NVENC resource registration / bitstream management
typedef struct
{
    NV_ENC_REGISTERED_PTR regPtr;
    NV_ENC_INPUT_PTR mappedInput;
    NV_ENC_OUTPUT_PTR bitstreams[4];
    FILE* fp;
} NvEncIO;

static void nvenc_create_bitstreams(NvEncCtx* nctx, NvEncIO* io, int count)
{
    for (int i = 0; i < count; i++)
    {
        NV_ENC_CREATE_BITSTREAM_BUFFER c = {0};
        c.version = NV_ENC_CREATE_BITSTREAM_BUFFER_VER;
        NVENC_API_CALL(g_nvenc.nvEncCreateBitstreamBuffer(nctx->hEncoder, &c));
        io->bitstreams[i] = c.bitstreamBuffer;
    }
}

static void nvenc_register_input(
    NvEncCtx* nctx, NvEncIO* io, CUdeviceptr nv12_base, uint32_t pitch, uint32_t w, uint32_t h)
{
    NV_ENC_REGISTER_RESOURCE rr = {0};
    rr.version = NV_ENC_REGISTER_RESOURCE_VER;
    rr.resourceType = NV_ENC_INPUT_RESOURCE_TYPE_CUDADEVICEPTR;
    rr.resourceToRegister = (void*)nv12_base;
    rr.width = w;
    rr.height = h;
    rr.pitch = pitch;
    rr.bufferFormat = NV_ENC_BUFFER_FORMAT_NV12;
    rr.bufferUsage = 0;
    NVENC_API_CALL(g_nvenc.nvEncRegisterResource(nctx->hEncoder, &rr));
    io->regPtr = rr.registeredResource;
}

static NV_ENC_INPUT_PTR nvenc_map_input(NvEncCtx* nctx, NvEncIO* io)
{
    NV_ENC_MAP_INPUT_RESOURCE m = {0};
    m.version = NV_ENC_MAP_INPUT_RESOURCE_VER;
    m.registeredResource = io->regPtr;
    NVENC_API_CALL(g_nvenc.nvEncMapInputResource(nctx->hEncoder, &m));
    io->mappedInput = m.mappedResource;
    return io->mappedInput;
}

static void nvenc_unmap_input(NvEncCtx* nctx, NvEncIO* io)
{
    if (io->mappedInput)
    {
        NVENC_API_CALL(g_nvenc.nvEncUnmapInputResource(nctx->hEncoder, io->mappedInput));
        io->mappedInput = NULL;
    }
}

static void nvenc_write_spspps(NvEncCtx* nctx, NvEncIO* io)
{
    // Optional: prepend sequence params
    if (g_nvenc.nvEncGetSequenceParams == NULL)
    {
        return;
    }
    uint8_t header[1024];
    uint32_t header_size = 0;
    NV_ENC_SEQUENCE_PARAM_PAYLOAD sps = {0};
    sps.version = NV_ENC_SEQUENCE_PARAM_PAYLOAD_VER;
    sps.inBufferSize = sizeof(header);
    sps.spsppsBuffer = header;
    sps.outSPSPPSPayloadSize = &header_size;
    NVENCSTATUS st = g_nvenc.nvEncGetSequenceParams(nctx->hEncoder, &sps);
    if (st == NV_ENC_SUCCESS && header_size > 0)
    {
        fwrite(header, 1, header_size, io->fp);
    }
}

static void nvenc_encode_frame(
    NvEncCtx* nctx, NvEncIO* io, NV_ENC_INPUT_PTR in, NV_ENC_OUTPUT_PTR out, uint32_t frame_idx)
{
    NV_ENC_PIC_PARAMS pp = {0};
    pp.version = NV_ENC_PIC_PARAMS_VER;
    pp.inputBuffer = in;
    pp.bufferFmt = NV_ENC_BUFFER_FORMAT_NV12;
    pp.inputWidth = WIDTH;
    pp.inputHeight = HEIGHT;
    pp.outputBitstream = out;
    pp.pictureStruct = NV_ENC_PIC_STRUCT_FRAME;
    pp.inputTimeStamp = frame_idx;
    if (frame_idx == 0)
    {
        pp.encodePicFlags |= NV_ENC_PIC_FLAG_FORCEIDR;
    }
    NVENC_API_CALL(g_nvenc.nvEncEncodePicture(nctx->hEncoder, &pp));

    NV_ENC_LOCK_BITSTREAM lb = {0};
    lb.version = NV_ENC_LOCK_BITSTREAM_VER;
    lb.outputBitstream = out;
    lb.doNotWait = 0;
    NVENC_API_CALL(g_nvenc.nvEncLockBitstream(nctx->hEncoder, &lb));

    fwrite(lb.bitstreamBufferPtr, 1, lb.bitstreamSizeInBytes, io->fp);

    NVENC_API_CALL(g_nvenc.nvEncUnlockBitstream(nctx->hEncoder, out));
}

static void nvenc_flush(NvEncCtx* nctx)
{
    NV_ENC_PIC_PARAMS eos = {0};
    eos.version = NV_ENC_PIC_PARAMS_VER;
    eos.encodePicFlags = NV_ENC_PIC_FLAG_EOS;
    NVENC_API_CALL(g_nvenc.nvEncEncodePicture(nctx->hEncoder, &eos));
}

static void nvenc_destroy(NvEncCtx* nctx, NvEncIO* io)
{
    // Destroy bitstream buffers
    for (int i = 0; i < 4; i++)
        if (io->bitstreams[i])
        {
            g_nvenc.nvEncDestroyBitstreamBuffer(nctx->hEncoder, io->bitstreams[i]);
            io->bitstreams[i] = NULL;
        }
    if (io->regPtr)
    {
        g_nvenc.nvEncUnregisterResource(nctx->hEncoder, io->regPtr);
        io->regPtr = NULL;
    }
    if (nctx->hEncoder)
    {
        g_nvenc.nvEncDestroyEncoder(nctx->hEncoder);
        nctx->hEncoder = NULL;
    }
}


// ====== Minimal video-encoder API used by test_video_2 ======
typedef enum
{
    DVZ_VIDEO_CODEC_HEVC = 0,
} DvzVideoCodec;

typedef struct
{
    uint32_t width;
    uint32_t height;
    uint32_t fps;
    VkFormat color_format;
    DvzVideoCodec codec;
    int flags;
} DvzVideoEncoderConfig;

typedef struct DvzVideoEncoder
{
    DvzDevice* device;
    DvzVideoEncoderConfig cfg;
    bool started;
    uint32_t frame_idx;

    VkImage image;
    VkDeviceMemory memory;
    VkDeviceSize memory_size;
    int memory_fd;

    bool cuda_ready;
    bool nvenc_ready;
    bool wait_semaphore_ready;

    FILE* fp;

    CudaCtx cuda;
    NvEncCtx nvenc;
    NvEncIO io;
    RgbaBuf rgba;
    Nv12Buf nv12;
    NV_ENC_INPUT_PTR mapped_in;
    NV_ENC_OUTPUT_PTR bitstream;
    CUexternalSemaphore wait_semaphore;
    int wait_semaphore_fd;
} DvzVideoEncoder;

static DvzVideoEncoderConfig dvz_video_encoder_default_config(void)
{
    DvzVideoEncoderConfig cfg = {
        .width = WIDTH,
        .height = HEIGHT,
        .fps = FPS,
        .color_format = VK_FORMAT_R8G8B8A8_UNORM,
        .codec = DVZ_VIDEO_CODEC_HEVC,
        .flags = 0,
    };
    return cfg;
}

static const GUID* dvz_video_encoder_codec_guid(DvzVideoCodec codec)
{
    switch (codec)
    {
    case DVZ_VIDEO_CODEC_HEVC:
    default:
        return &NV_ENC_CODEC_HEVC_GUID;
    }
}

static void dvz_video_encoder_release(DvzVideoEncoder* enc)
{
    if (!enc)
    {
        return;
    }

    if (enc->nvenc_ready)
    {
        nvenc_unmap_input(&enc->nvenc, &enc->io);
        nvenc_destroy(&enc->nvenc, &enc->io);
        memset(&enc->nvenc, 0, sizeof(enc->nvenc));
        memset(&enc->io, 0, sizeof(enc->io));
        enc->mapped_in = NULL;
        enc->bitstream = NULL;
        enc->nvenc_ready = false;
    }

    if (enc->rgba.dptr)
    {
        cuMemFree(enc->rgba.dptr);
        enc->rgba.dptr = 0;
        enc->rgba.pitch = 0;
        enc->rgba.size = 0;
    }
    if (enc->nv12.dptr)
    {
        cuMemFree(enc->nv12.dptr);
        enc->nv12.dptr = 0;
        enc->nv12.pitch = 0;
        enc->nv12.size = 0;
    }

    if (enc->wait_semaphore_ready)
    {
        cuDestroyExternalSemaphore(enc->wait_semaphore);
        enc->wait_semaphore = NULL;
    }
    enc->wait_semaphore_ready = false;
    enc->wait_semaphore_fd = -1;

    if (enc->cuda.cuMod)
        cuModuleUnload(enc->cuda.cuMod);
    if (enc->cuda.mmArr)
        cuMipmappedArrayDestroy(enc->cuda.mmArr);
    if (enc->cuda.extMem)
        cuDestroyExternalMemory(enc->cuda.extMem);
    if (enc->cuda.stream)
    {
        CU_CHECK(cuStreamDestroy(enc->cuda.stream));
        enc->cuda.stream = NULL;
    }
    if (enc->cuda.cuCtx)
        cuCtxDestroy(enc->cuda.cuCtx);
    memset(&enc->cuda, 0, sizeof(enc->cuda));
    enc->cuda_ready = false;

    enc->fp = NULL;
    enc->image = VK_NULL_HANDLE;
    enc->memory = VK_NULL_HANDLE;
    enc->memory_size = 0;
    enc->memory_fd = -1;
    enc->started = false;
    enc->frame_idx = 0;
}

static DvzVideoEncoder* dvz_video_encoder_create(DvzDevice* device, const DvzVideoEncoderConfig* cfg)
{
    DvzVideoEncoder* enc = (DvzVideoEncoder*)calloc(1, sizeof(DvzVideoEncoder));
    ANN(enc);
    enc->device = device;
    enc->cfg = cfg ? *cfg : dvz_video_encoder_default_config();
    enc->memory_fd = -1;
    enc->wait_semaphore_fd = -1;
    return enc;
}

static int dvz_video_encoder_start(
    DvzVideoEncoder* enc,
    VkImage image,
    VkDeviceMemory memory,
    VkDeviceSize memory_size,
    int memory_fd,
    int wait_semaphore_fd,
    FILE* bitstream_out)
{
    ANN(enc);
    ANN(bitstream_out);
    if (enc->started)
    {
        log_error("video encoder already started");
        return -1;
    }

    int rc = 0;
    enc->image = image;
    enc->memory = memory;
    enc->memory_size = memory_size;
    enc->memory_fd = memory_fd;
    enc->wait_semaphore_fd = wait_semaphore_fd;
    enc->fp = bitstream_out;
    enc->io.fp = bitstream_out;

    nvenc_load_api();

    cuda_import_vk_memory(&enc->cuda, memory_fd, memory_size);
    enc->cuda_ready = true;

    if (wait_semaphore_fd >= 0)
    {
        CUDA_EXTERNAL_SEMAPHORE_HANDLE_DESC shdesc = {0};
        shdesc.type = CU_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD;
        shdesc.handle.fd = wait_semaphore_fd;
        CU_CHECK(cuImportExternalSemaphore(&enc->wait_semaphore, &shdesc));
        enc->wait_semaphore_ready = true;
    }

    nvenc_open_session_cuda(&enc->nvenc, enc->cuda.cuCtx);
    if (!nvenc_supports_codec(enc->nvenc.hEncoder, dvz_video_encoder_codec_guid(enc->cfg.codec)))
    {
        log_warn("requested video codec not supported");
        rc = -1;
        goto fail;
    }
    nvenc_init_hevc(&enc->nvenc, enc->cfg.width, enc->cfg.height, enc->cfg.fps);
    enc->nvenc_ready = true;

    alloc_rgba(&enc->rgba, enc->cfg.width, enc->cfg.height, PITCH_ALIGN);
    alloc_nv12(&enc->nv12, enc->cfg.width, enc->cfg.height, PITCH_ALIGN);

    nvenc_register_input(
        &enc->nvenc, &enc->io, enc->nv12.dptr, enc->nv12.pitch, enc->cfg.width, enc->cfg.height);
    nvenc_create_bitstreams(&enc->nvenc, &enc->io, 1);
    enc->bitstream = enc->io.bitstreams[0];
    enc->mapped_in = nvenc_map_input(&enc->nvenc, &enc->io);
    nvenc_write_spspps(&enc->nvenc, &enc->io);

    enc->frame_idx = 0;
    enc->started = true;
    return 0;

fail:
    dvz_video_encoder_release(enc);
    return rc;
}

static int dvz_video_encoder_submit(
    DvzVideoEncoder* enc,
    VkSemaphore render_done,
    VkPipelineStageFlags wait_stage,
    VkImageLayout current_layout,
    VkFence encode_done)
{
    (void)render_done;
    (void)wait_stage;
    (void)current_layout;
    (void)encode_done;
    ANN(enc);
    if (!enc->started)
    {
        return -1;
    }

    if (enc->wait_semaphore_ready)
    {
        CUDA_EXTERNAL_SEMAPHORE_WAIT_PARAMS wait_params = {0};
        CU_CHECK(
            cuWaitExternalSemaphoresAsync(&enc->wait_semaphore, &wait_params, 1, enc->cuda.stream));
    }

    copy_array_to_linear_rgba(&enc->cuda, &enc->rgba, enc->cfg.width, enc->cfg.height);
    launch_rgba_to_nv12(&enc->cuda, &enc->rgba, &enc->nv12, enc->cfg.width, enc->cfg.height);
    nvenc_encode_frame(&enc->nvenc, &enc->io, enc->mapped_in, enc->bitstream, enc->frame_idx++);
    return 0;
}

static int dvz_video_encoder_stop(DvzVideoEncoder* enc)
{
    if (!enc)
    {
        return 0;
    }

    if (enc->started && enc->nvenc_ready)
    {
        nvenc_flush(&enc->nvenc);
    }
    dvz_video_encoder_release(enc);
    return 0;
}

static void dvz_video_encoder_destroy(DvzVideoEncoder* enc)
{
    if (!enc)
    {
        return;
    }
    dvz_video_encoder_stop(enc);
    free(enc);
}



int test_video_2(TstSuite* suite, TstItem* tstitem)
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
    // At the moment we just allocate an exportable VkImage and fill it once. When integrating the real
    // Datoviz renderer, the Vulkan context above needs to be fed with the renderer-managed device,
    // command queues, and the offscreen color attachment you already render into. Conceptually:
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
    // You keep ownership of the renderer; test_video_2 now feeds those handles to dvz_video_encoder_* so
    // it can import the memory once, keep CUDA/NVENC objects alive, and consume whatever Datoviz draws.
    // -------------------------------------------------------------------------------------------------

    // Query allocation size for CUDA import
    VkMemoryRequirements memReq;
    vkGetImageMemoryRequirements(vk.device, vk.image, &memReq);

    bitstream_fp = fopen("out.h265", "wb");
    if (!bitstream_fp)
    {
        perror("fopen out.h265");
        rc = 1;
        goto cleanup;
    }

    DvzVideoEncoderConfig vcfg = dvz_video_encoder_default_config();
    encoder = dvz_video_encoder_create(NULL, &vcfg);
    if (!encoder)
    {
        rc = 1;
        goto cleanup;
    }
    if (dvz_video_encoder_start(
            encoder, vk.image, vk.memory, memReq.size, vk.memory_fd, vk.semaphore_fd, bitstream_fp) !=
        0)
    {
        skip_encode = true;
        goto cleanup;
    }

    for (int frame = 0; frame < NFRAMES; ++frame)
    {
        // Render/copy path:
        // 1. Record and submit a command buffer that clears the offscreen image and transitions it back
        //    to GENERAL layout so CUDA can read from it.
        // 2. Hand control to dvz_video_encoder_submit(), which copies the VkImage via CUDA and feeds
        //    NVENC without re-importing resources.
        VkClearColorValue clr = frame_clear_color((uint32_t)frame, NFRAMES);
        vk_render_frame_and_sync(&vk, &clr);

        if (dvz_video_encoder_submit(
                encoder,
                vk.semaphore,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                vk.image_layout,
                VK_NULL_HANDLE) != 0)
        {
            rc = 1;
            goto cleanup;
        }
    }

    dvz_video_encoder_stop(encoder);
    encoded = true;

cleanup:
    if (encoder)
    {
        dvz_video_encoder_destroy(encoder);
        encoder = NULL;
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
        fprintf(stderr, "Wrote out.h265 (%dx%d @ %dfps, %d frames)\n", WIDTH, HEIGHT, FPS, NFRAMES);
    }
    if (skip_encode)
    {
        return 0;
    }
    return rc;
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
