/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  DRP2 vklite runtime tests                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#if !OS_WINDOWS
#include <unistd.h>
#endif

#include "_alloc.h"
#include "_assertions.h"
#include "../_stream.h"
#include "datoviz/drp2.h"
#include "test_drp2.h"
#include "test_drp2_helpers.h"
#include "testing.h"

#if DVZ_DRP2_HAS_VKLITE
#include "_log.h"
#include "../_runtime.h"
#include "../../vklite/_buffers.h"
#include "datoviz/vk/device.h"
#include "datoviz/vk/gpu_ctx.h"
#include "datoviz/vk/instance.h"
#include "datoviz/vk/memory_interop.h"
#include "datoviz/vk/queues.h"
#include "datoviz/vklite/buffers.h"
#include "datoviz/vklite/commands.h"
#include "datoviz/vklite/sync.h"

bool _dvz_drp2_runtime_vklite_download_buffer(
    DvzDrp2Runtime* runtime, uint64_t buffer_id, uint64_t offset, uint64_t size, void* data);
#endif

#if DVZ_DRP2_HAS_VKLITE && DVZ_HAS_CUDA
#include <cuda.h>
#include <cuda_runtime_api.h>
#endif



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/


/**
 * Return the smallest multisample count supported for an RGBA8 color attachment.
 *
 * @param ctx GPU context owning the selected physical device.
 * @return A supported multisample count, or one when multisampling is unavailable.
 */
static uint32_t _supported_color_sample_count(DvzGpuCtx* ctx)
{
    ANN(ctx);
    DvzDevice* device = dvz_gpu_ctx_device(ctx);
    if (device == NULL)
        return 1;
    VkPhysicalDevice physical_device = dvz_device_physical_device(device);
    if (physical_device == VK_NULL_HANDLE)
        return 1;

    VkImageFormatProperties props = {0};
    VkResult result = vkGetPhysicalDeviceImageFormatProperties(
        physical_device, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TYPE_2D,
        VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 0, &props);
    if (result != VK_SUCCESS)
        return 1;

    const VkSampleCountFlagBits candidates[] = {
        VK_SAMPLE_COUNT_2_BIT,  VK_SAMPLE_COUNT_4_BIT,  VK_SAMPLE_COUNT_8_BIT,
        VK_SAMPLE_COUNT_16_BIT, VK_SAMPLE_COUNT_32_BIT, VK_SAMPLE_COUNT_64_BIT,
    };
    for (uint32_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); i++)
    {
        if ((props.sampleCounts & candidates[i]) != 0)
            return (uint32_t)candidates[i];
    }
    return 1;
}



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/


#if DVZ_DRP2_HAS_VKLITE && DVZ_HAS_CUDA
/**
 * Report CUDA driver errors with a readable label.
 *
 * @param res CUDA driver result code
 * @param label operation label used in the diagnostic
 * @return 0 on success, 1 on CUDA error
 */
static int _drp2_cuda_check(CUresult res, const char* label)
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



/**
 * Find the Vulkan physical device that matches a CUDA device UUID.
 *
 * @param instance Vulkan instance used to enumerate physical devices
 * @param cu_device CUDA device whose UUID should be matched
 * @param[out] out_gpu_index matched Vulkan GPU index
 * @return true when a matching Vulkan device was found
 */
static bool _drp2_cuda_find_vulkan_gpu(
    DvzInstance* instance, CUdevice cu_device, uint32_t* out_gpu_index)
{
    ANN(instance);
    ANN(out_gpu_index);
    *out_gpu_index = UINT32_MAX;

    CUuuid cu_uuid = {0};
    if (_drp2_cuda_check(cuDeviceGetUuid(&cu_uuid, cu_device), "cuDeviceGetUuid"))
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



/**
 * Wait for a CUDA signal and make its writes visible to Vulkan vertex input.
 *
 * @param device logical Vulkan device owning the main queue
 * @param buffer buffer whose contents were written by CUDA
 * @param size byte size of the synchronized buffer range
 * @param wait_semaphore timeline semaphore to wait on
 * @param wait_value timeline value signaled by CUDA
 * @return true on success
 */
static bool _drp2_cuda_wait_vertex_buffer(
    DvzDevice* device, DvzBuffer* buffer, VkDeviceSize size, VkSemaphore wait_semaphore,
    uint64_t wait_value)
{
    ANN(device);
    ANN(buffer);
    ASSERT(size > 0);
    ASSERT(wait_semaphore != VK_NULL_HANDLE);

    DvzQueue* queue = dvz_device_queue(device, DVZ_QUEUE_MAIN);
    if (queue == NULL)
    {
        log_error("main Vulkan queue unavailable for CUDA vertex-buffer wait");
        return false;
    }

    DvzCommands* cmds = dvz_commands_create_wrapper();
    ANN(cmds);
    dvz_commands(device, queue, 1, cmds);
    if (dvz_commands_count(cmds) == 0)
    {
        log_error("failed to allocate command buffer for CUDA vertex-buffer wait");
        dvz_commands_free(cmds);
        return false;
    }

    bool ok = false;
    if (dvz_cmd_begin_result(cmds) == 0)
    {
        DvzBarriers barriers = {0};
        dvz_barriers(&barriers);
        DvzBarrierBuffer* bbuf =
            dvz_barriers_buffer(&barriers, dvz_buffer_handle(buffer), 0, size);
        dvz_barrier_buffer_stage(
            bbuf, VK_PIPELINE_STAGE_2_NONE, VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT);
        dvz_barrier_buffer_access(
            bbuf, VK_ACCESS_2_MEMORY_WRITE_BIT, VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT);
        dvz_cmd_barriers(cmds, &barriers);

        if (dvz_cmd_end_result(cmds) == 0)
        {
            DvzSubmit* submit = dvz_submit_create_wrapper();
            ANN(submit);
            dvz_submit(submit);
            dvz_submit_wait(
                submit, wait_semaphore, wait_value,
                VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT);
            dvz_submit_command(submit, dvz_commands_handle(cmds));
            VkResult res = (VkResult)dvz_submit_send(
                submit, dvz_queue_handle(queue), VK_NULL_HANDLE);
            if (res == VK_SUCCESS)
            {
                dvz_queue_wait(queue);
                ok = true;
            }
            else
            {
                log_error("Vulkan CUDA vertex-buffer wait submit failed (%d)", res);
            }
            dvz_submit_free(submit);
        }
    }

    dvz_commands_destroy(cmds);
    dvz_commands_free(cmds);
    return ok;
}
#endif



#if DVZ_DRP2_HAS_VKLITE
/**
 * Validate that the vklite runtime can sample from a DRP2 3D texture.
 *
 * @param suite the test suite.
 * @param item the test item.
 * @return 0 on success.
 */
int test_drp2_runtime_vklite_samples_3d_texture(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzGpuCtx* ctx = NULL;
    DvzDrp2Runtime* runtime = drp2_test_vklite_fixture_runtime(suite, &ctx);
    if (runtime == NULL)
        return 0;
    ANN(ctx);

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);
    static const uint8_t voxels[2 * 2 * 2 * 4] = {
        255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255,
        255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255,
    };

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 1, "VERTEX", "glsl",
        "#version 450\nvec2 p[3]=vec2[](vec2(-1,-1),vec2(3,-1),vec2(-1,3));"
        "void main(){gl_Position=vec4(p[gl_VertexIndex],0,1);}"));
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 2, "FRAGMENT", "glsl",
        "#version 450\nlayout(set=0,binding=0)uniform texture3D tex;"
        "layout(set=0,binding=1)uniform sampler samp;"
        "layout(location=0)out vec4 color;"
        "void main(){color=texture(sampler3D(tex,samp),vec3(0.5));}"));
    AT(dvz_drp2_stream_create_texture_sampler_bind_group_layout(stream, 3));
    AT(drp2_test_create_render_pipeline_with_bind_group_layout(stream, 4, 1, 2, 0, 3));
    AT(dvz_drp2_stream_create_sampler(stream, 5));
    AT(dvz_drp2_stream_create_texture_3d_format_usage(
        stream, 6, 2, 2, 2, DVZ_FORMAT_R8G8B8A8_UNORM,
        DVZ_DRP2_TEXTURE_USAGE_COPY_DST | DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING));
    AT(dvz_drp2_stream_write_texture_3d_borrowed(
        stream, 6, 0, 0, 0, 0, 2, 2, 2, 8, 2, voxels));
    AT(dvz_drp2_stream_create_texture_sampler_bind_group(stream, 7, 3, 6, 5));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 8, 2, 2,
        DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC));
    AT(dvz_drp2_stream_create_buffer(
        stream, 9, 16, DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 10));
    AT(dvz_drp2_stream_begin_render_pass(stream, 11, 10, 8));
    AT(dvz_drp2_stream_set_pipeline(stream, 11, 4));
    AT(dvz_drp2_stream_set_bind_group(stream, 11, 0, 7));
    AT(dvz_drp2_stream_draw(stream, 11, 3, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(stream, 11));
    AT(dvz_drp2_stream_copy_texture_to_buffer(stream, 10, 8, 9, 0, 2, 2, 8, 2));
    AT(dvz_drp2_stream_finish_command_encoder(stream, 10, 12));
    AT(dvz_drp2_stream_queue_submit(stream, 12, 13));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(drp2_test_vklite_validation_clean(suite, ctx));

    uint8_t downloaded[16] = {0};
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, 9, 0, 16, downloaded));
    for (uint32_t i = 0; i < 16; i += 4)
    {
        AT(downloaded[i + 0] == 255);
        AT(downloaded[i + 1] == 0);
        AT(downloaded[i + 2] == 0);
        AT(downloaded[i + 3] == 255);
    }

    dvz_drp2_stream_destroy(stream);
    return 0;
}
#endif



#if DVZ_DRP2_HAS_VKLITE
int test_drp2_runtime_vklite_executes_resource_commands(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzGpuCtx* ctx = NULL;
    DvzDrp2Runtime* runtime = drp2_test_vklite_fixture_runtime(suite, &ctx);
    if (runtime == NULL)
        return 0;
    ANN(ctx);

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_buffer(
        stream, 1, 16, DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_WRITE));
    AT(dvz_drp2_stream_write_buffer_base64(stream, 1, 0, 16, "AQIDBAUGBwgJCgsMDQ4PEA=="));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 2, 2, 2,
        DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_COPY_DST));
    AT(dvz_drp2_stream_destroy_buffer(stream, 1));
    AT(dvz_drp2_stream_destroy_texture(stream, 2));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(drp2_test_vklite_validation_clean(suite, ctx));

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_vklite_writes_buffer_contents(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzGpuCtx* ctx = NULL;
    DvzDrp2Runtime* runtime = drp2_test_vklite_fixture_runtime(suite, &ctx);
    if (runtime == NULL)
        return 0;
    ANN(ctx);

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_buffer(
        stream, 1, 32,
        DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ |
            DVZ_DRP2_BUFFER_USAGE_MAP_WRITE));
    AT(dvz_drp2_stream_write_buffer_base64(stream, 1, 8, 16, "AQIDBAUGBwgJCgsMDQ4PEA=="));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(drp2_test_vklite_validation_clean(suite, ctx));

    uint8_t expected[16] = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    uint8_t downloaded[16] = {0};
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, 1, 8, 16, downloaded));
    for (uint32_t i = 0; i < 16; i++)
    {
        AT(downloaded[i] == expected[i]);
    }

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_vklite_copies_buffer_contents(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzGpuCtx* ctx = NULL;
    DvzDrp2Runtime* runtime = drp2_test_vklite_fixture_runtime(suite, &ctx);
    if (runtime == NULL)
        return 0;
    ANN(ctx);

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_buffer(
        stream, 1, 32,
        DVZ_DRP2_BUFFER_USAGE_COPY_SRC | DVZ_DRP2_BUFFER_USAGE_COPY_DST |
            DVZ_DRP2_BUFFER_USAGE_MAP_WRITE));
    AT(dvz_drp2_stream_create_buffer(
        stream, 2, 32, DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ));
    AT(dvz_drp2_stream_write_buffer_base64(stream, 1, 4, 16, "AQIDBAUGBwgJCgsMDQ4PEA=="));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 10));
    AT(dvz_drp2_stream_copy_buffer_to_buffer(stream, 10, 1, 4, 2, 12, 16));
    AT(dvz_drp2_stream_finish_command_encoder(stream, 10, 11));
    AT(dvz_drp2_stream_queue_submit(stream, 11, 12));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(drp2_test_vklite_validation_clean(suite, ctx));

    uint8_t expected[16] = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    uint8_t downloaded[16] = {0};
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, 2, 12, 16, downloaded));
    for (uint32_t i = 0; i < 16; i++)
    {
        AT(downloaded[i] == expected[i]);
    }

    dvz_drp2_stream_destroy(stream);
    return 0;
}


int test_drp2_runtime_vklite_uses_external_buffer(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzGpuCtx* ctx = NULL;
    DvzDrp2Runtime* runtime = drp2_test_vklite_fixture_runtime(suite, &ctx);
    if (runtime == NULL)
        return 0;
    ANN(ctx);
    DvzBuffer* external = dvz_buffer_create_wrapper();
    ANN(external);
    dvz_buffer(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx), external);
    dvz_buffer_size(external, 32);
    dvz_buffer_usage(external, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    dvz_buffer_flags(external, DVZ_ALLOC_HOST_ACCESS_SEQUENTIAL_WRITE);
    AT(dvz_buffer_create(external) == 0);

    uint8_t source[16] = {
        31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16};
    dvz_buffer_upload(external, 4, 16, source);

    DvzDrp2ExternalBufferDesc desc = {
        DVZ_STRUCT_INIT_FIELDS(DvzDrp2ExternalBufferDesc),
        .buffer = external,
        .size = 32,
        .usage = DVZ_DRP2_BUFFER_USAGE_COPY_SRC | DVZ_DRP2_BUFFER_USAGE_VERTEX,
    };
    AT(dvz_drp2_runtime_register_external_buffer(runtime, 1, &desc));

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_buffer(
        stream, 2, 32, DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 10));
    AT(dvz_drp2_stream_copy_buffer_to_buffer(stream, 10, 1, 4, 2, 8, 16));
    AT(dvz_drp2_stream_finish_command_encoder(stream, 10, 11));
    AT(dvz_drp2_stream_queue_submit(stream, 11, 12));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(drp2_test_vklite_validation_clean(suite, ctx));

    uint8_t downloaded[16] = {0};
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, 2, 8, 16, downloaded));
    for (uint32_t i = 0; i < 16; i++)
    {
        AT(downloaded[i] == source[i]);
    }

    dvz_drp2_stream_destroy(stream);
    dvz_drp2_runtime_reset(runtime);
    dvz_buffer_destroy(external);
    dvz_buffer_free(external);
    return 0;
}



int test_drp2_runtime_vklite_external_buffer_timeline_copy(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

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
        tst_skip(suite, "Vulkan timeline semaphore or synchronization2 context unavailable");
        return 0;
    }
    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    if (runtime == NULL)
    {
        dvz_gpu_ctx_destroy(ctx);
        tst_skip(suite, "DRP2 vklite runtime unavailable for timeline context");
        return 0;
    }

    DvzBuffer* external = dvz_buffer_create_wrapper();
    ANN(external);
    dvz_buffer(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx), external);
    dvz_buffer_size(external, 16);
    dvz_buffer_usage(external, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    dvz_buffer_flags(external, DVZ_ALLOC_HOST_ACCESS_SEQUENTIAL_WRITE);
    AT(dvz_buffer_create(external) == 0);
    const uint8_t source[] = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    dvz_buffer_upload(external, 0, sizeof(source), source);

    DvzDrp2ExternalBufferDesc buffer = dvz_drp2_external_buffer_desc();
    buffer.buffer = external;
    buffer.size = sizeof(source);
    buffer.usage = DVZ_DRP2_BUFFER_USAGE_COPY_SRC;
    AT(dvz_drp2_runtime_register_external_buffer(runtime, 1, &buffer));

    DvzSemaphore* semaphore = dvz_semaphore_create_wrapper();
    ANN(semaphore);
    dvz_semaphore_timeline(dvz_gpu_ctx_device(ctx), 0, semaphore, 0);
    dvz_semaphore_signal(semaphore, 4);
    DvzDrp2ExternalBufferTimelineDesc timeline =
        dvz_drp2_external_buffer_timeline_desc();
    timeline.semaphore = semaphore;
    timeline.wait_value = 4;
    timeline.signal_value = 5;
    AT(dvz_drp2_runtime_arm_external_buffer_timeline(runtime, 1, &timeline));
    AT(dvz_drp2_runtime_external_buffer_timeline_pending(runtime, 1));

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 2, 2, 2, DVZ_DRP2_TEXTURE_USAGE_COPY_DST));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 3));
    AT(dvz_drp2_stream_copy_buffer_to_texture(stream, 3, 1, 0, 2, 2, 1, 8, 1));
    AT(dvz_drp2_stream_finish_command_encoder(stream, 3, 4));
    AT(dvz_drp2_stream_queue_submit(stream, 4, 5));
    AT(dvz_drp2_runtime_execute(runtime, stream).ok);
    dvz_semaphore_wait(semaphore, 5);
    AT(dvz_semaphore_query(semaphore) >= 5);
    AT(!dvz_drp2_runtime_external_buffer_timeline_pending(runtime, 1));
    AT(drp2_test_vklite_validation_clean(suite, ctx));

    dvz_drp2_stream_destroy(stream);
    dvz_drp2_runtime_destroy(runtime);
    dvz_semaphore_destroy(semaphore);
    dvz_semaphore_free(semaphore);
    dvz_buffer_destroy(external);
    dvz_buffer_free(external);
    uint32_t err_count = dvz_gpu_ctx_error_count(ctx);
    dvz_gpu_ctx_destroy(ctx);
    return err_count > 0;
}



#if DVZ_HAS_CUDA
int test_drp2_runtime_vklite_draws_cuda_external_vertex_buffer(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

#if !OS_UNIX
    log_warn("DRP2 CUDA external vertex-buffer test skipped: opaque FD path is Unix-only");
    tst_skip(suite, "opaque FD path is Unix-only");
    return 0;
#else
    if (!drp2_test_vklite_runtime_available())
    {
        tst_skip(suite, "Vulkan instance creation failed");
        return 0;
    }

    cudaError_t cerr;
    CUdevice cu_device = 0;
    int device_count = 0;
    cerr = cudaGetDeviceCount(&device_count);
    if (cerr != cudaSuccess || device_count == 0)
    {
        log_warn(
            "DRP2 CUDA external vertex-buffer test skipped: no CUDA devices found (%s)",
            cudaGetErrorString(cerr));
        tst_skip(suite, "no CUDA devices found");
        return 0;
    }
    if (_drp2_cuda_check(cuInit(0), "cuInit"))
        return 1;
    if (_drp2_cuda_check(cuDeviceGet(&cu_device, 0), "cuDeviceGet"))
        return 1;

    typedef struct DvzDrp2CudaVertex
    {
        float pos[2];
    } DvzDrp2CudaVertex;

    const DvzDrp2CudaVertex vertices[3] = {
        {{-1.0f, -1.0f}},
        {{3.0f, -1.0f}},
        {{-1.0f, 3.0f}},
    };
    const uint64_t vertex_size = sizeof(vertices);

    int out = 0;
    int memory_fd = -1;
    int semaphore_fd = -1;
    DvzInstance* instance = NULL;
    DvzDevice* device = NULL;
    DvzVma* allocator = NULL;
    DvzBuffer* external = NULL;
    DvzDrp2Runtime* runtime = NULL;
    DvzDrp2CommandStream* stream = NULL;
    DvzSemaphore* interop_semaphore = NULL;
    cudaExternalMemory_t cuda_mem = NULL;
    cudaExternalSemaphore_t cuda_semaphore = NULL;
    void* cuda_ptr = NULL;

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
    if (!_drp2_cuda_find_vulkan_gpu(instance, cu_device, &vk_gpu_index))
    {
        log_warn(
            "DRP2 CUDA external vertex-buffer test skipped: no Vulkan GPU matches CUDA device 0");
        tst_skip(suite, "no Vulkan GPU matches CUDA device 0");
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
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_device_config_set_features13(&dcfg, &features13);
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
        log_warn("DRP2 CUDA external vertex-buffer test skipped: external semaphore FD missing");
        tst_skip(suite, "external semaphore FD missing");
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

    external = dvz_buffer_create_wrapper();
    ANN(external);
    dvz_buffer(device, allocator, external);
    dvz_buffer_size(external, vertex_size);
    dvz_buffer_usage(external, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    dvz_buffer_flags(external, DVZ_ALLOC_DEDICATED_MEMORY);
    if (dvz_buffer_create(external) != 0)
    {
        log_error("failed to create exportable DRP2 CUDA vertex buffer");
        out = 1;
        goto cleanup;
    }

    interop_semaphore = dvz_semaphore_create_wrapper();
    ANN(interop_semaphore);
    dvz_semaphore_timeline(
        device, 0, interop_semaphore, VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT);
    semaphore_fd =
        dvz_semaphore_export_fd(interop_semaphore, VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT);
    if (semaphore_fd < 0)
    {
        log_error("failed to export DRP2 CUDA timeline semaphore FD");
        out = 1;
        goto cleanup;
    }

    struct cudaExternalSemaphoreHandleDesc sem_desc = {0};
    sem_desc.type = cudaExternalSemaphoreHandleTypeTimelineSemaphoreFd;
    sem_desc.handle.fd = semaphore_fd;
    cerr = cudaImportExternalSemaphore(&cuda_semaphore, &sem_desc);
    if (cerr != cudaSuccess)
    {
        log_error("cudaImportExternalSemaphore failed: %s", cudaGetErrorString(cerr));
        out = 1;
        goto cleanup;
    }
    semaphore_fd = -1;

    dvz_allocator_export(allocator, external->alloc, &memory_fd);
    if (memory_fd < 0)
    {
        log_error("failed to export DRP2 CUDA vertex-buffer memory FD");
        out = 1;
        goto cleanup;
    }

    struct cudaExternalMemoryHandleDesc mem_desc = {0};
    mem_desc.type = cudaExternalMemoryHandleTypeOpaqueFd;
    mem_desc.handle.fd = memory_fd;
    mem_desc.size = dvz_buffer_allocated_size(external);
    cerr = cudaImportExternalMemory(&cuda_mem, &mem_desc);
    if (cerr != cudaSuccess)
    {
        log_error("cudaImportExternalMemory failed: %s", cudaGetErrorString(cerr));
        out = 1;
        goto cleanup;
    }
    memory_fd = -1;

    struct cudaExternalMemoryBufferDesc buffer_desc = {0};
    buffer_desc.offset = 0;
    buffer_desc.size = vertex_size;
    cerr = cudaExternalMemoryGetMappedBuffer(&cuda_ptr, cuda_mem, &buffer_desc);
    if (cerr != cudaSuccess)
    {
        log_error("cudaExternalMemoryGetMappedBuffer failed: %s", cudaGetErrorString(cerr));
        out = 1;
        goto cleanup;
    }

    cerr = cudaMemcpy(cuda_ptr, vertices, vertex_size, cudaMemcpyHostToDevice);
    if (cerr != cudaSuccess)
    {
        log_error("cudaMemcpy to external vertex buffer failed: %s", cudaGetErrorString(cerr));
        out = 1;
        goto cleanup;
    }
    struct cudaExternalSemaphoreSignalParams signal_params = {0};
    signal_params.params.fence.value = 1;
    cerr = cudaSignalExternalSemaphoresAsync(&cuda_semaphore, &signal_params, 1, 0);
    if (cerr != cudaSuccess)
    {
        log_error("cudaSignalExternalSemaphoresAsync failed: %s", cudaGetErrorString(cerr));
        out = 1;
        goto cleanup;
    }
    cerr = cudaDeviceSynchronize();
    if (cerr != cudaSuccess)
    {
        log_error("cudaDeviceSynchronize failed: %s", cudaGetErrorString(cerr));
        out = 1;
        goto cleanup;
    }
    if (!_drp2_cuda_wait_vertex_buffer(
            device, external, vertex_size, dvz_semaphore_handle(interop_semaphore), 1))
    {
        out = 1;
        goto cleanup;
    }

    DvzDrp2RuntimeConfig cfg = dvz_drp2_runtime_vklite_config(device, allocator);
    runtime = dvz_drp2_runtime_vklite(&cfg);
    ANN(runtime);
    DvzDrp2ExternalBufferDesc desc = {
        DVZ_STRUCT_INIT_FIELDS(DvzDrp2ExternalBufferDesc),
        .buffer = external,
        .size = vertex_size,
        .usage = DVZ_DRP2_BUFFER_USAGE_VERTEX,
    };
    AT(dvz_drp2_runtime_register_external_buffer(runtime, 1, &desc));

    uint32_t binding_stride = sizeof(DvzDrp2CudaVertex);
    uint32_t binding_step = DVZ_DRP2_VERTEX_STEP_MODE_VERTEX;
    uint32_t attr_binding = 0;
    uint32_t attr_location = 0;
    DvzFormat attr_format = DVZ_FORMAT_R32G32_SFLOAT;
    uint32_t attr_offset = 0;

    stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 2, "VERTEX", "glsl",
        "#version 450\nlayout(location=0)in vec2 pos;"
        "void main(){gl_Position=vec4(pos,0,1);}"));
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 3, "FRAGMENT", "glsl",
        "#version 450\nlayout(location=0)out vec4 color;"
        "void main(){color=vec4(1,0,0,1);}"));
    DvzDrp2RenderPipelineDesc pipeline = dvz_drp2_render_pipeline_desc();
    pipeline.id = 4;
    pipeline.vertex_shader_module_id = 2;
    pipeline.fragment_shader_module_id = 3;
    pipeline.vertex_buffer_slots = 1;
    pipeline.binding_count = 1;
    pipeline.binding_strides = &binding_stride;
    pipeline.binding_step_modes = &binding_step;
    pipeline.attr_count = 1;
    pipeline.attr_bindings = &attr_binding;
    pipeline.attr_locations = &attr_location;
    pipeline.attr_formats = &attr_format;
    pipeline.attr_offsets = &attr_offset;
    AT(dvz_drp2_stream_create_render_pipeline(stream, &pipeline));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 5, 2, 2,
        DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC));
    AT(dvz_drp2_stream_create_buffer(
        stream, 6, 4, DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 10));
    AT(dvz_drp2_stream_begin_render_pass_clear(stream, 11, 10, 5, 0, 0, 0, 1));
    AT(dvz_drp2_stream_set_pipeline(stream, 11, 4));
    AT(dvz_drp2_stream_set_vertex_buffer(stream, 11, 0, 1, 0));
    AT(dvz_drp2_stream_draw(stream, 11, 3, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(stream, 11));
    AT(dvz_drp2_stream_copy_texture_to_buffer(stream, 10, 5, 6, 0, 1, 1, 4, 1));
    AT(dvz_drp2_stream_finish_command_encoder(stream, 10, 12));
    AT(dvz_drp2_stream_queue_submit(stream, 12, 13));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);

    uint8_t downloaded[4] = {0};
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, 6, 0, 4, downloaded));
    AT(downloaded[0] == 255);
    AT(downloaded[1] == 0);
    AT(downloaded[2] == 0);
    AT(downloaded[3] == 255);

cleanup:
    if (stream != NULL)
        dvz_drp2_stream_destroy(stream);
    if (runtime != NULL)
        dvz_drp2_runtime_destroy(runtime);
    if (cuda_ptr != NULL)
        cudaFree(cuda_ptr);
    if (cuda_mem != NULL)
        cudaDestroyExternalMemory(cuda_mem);
    if (cuda_semaphore != NULL)
        cudaDestroyExternalSemaphore(cuda_semaphore);
    if (memory_fd >= 0)
        close(memory_fd);
    if (semaphore_fd >= 0)
        close(semaphore_fd);
    if (interop_semaphore != NULL)
    {
        dvz_semaphore_destroy(interop_semaphore);
        dvz_semaphore_free(interop_semaphore);
    }
    if (external != NULL)
    {
        dvz_buffer_destroy(external);
        dvz_buffer_free(external);
    }
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
#endif



int test_drp2_runtime_vklite_writes_texture_contents(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzGpuCtx* ctx = NULL;
    DvzDrp2Runtime* runtime = drp2_test_vklite_fixture_runtime(suite, &ctx);
    if (runtime == NULL)
        return 0;
    ANN(ctx);

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 1, 2, 2, DVZ_DRP2_TEXTURE_USAGE_COPY_DST | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC));
    AT(dvz_drp2_stream_write_texture_2d_base64(
        stream, 1, 0, 2, 2, 8, 2, "AQIDBAUGBwgJCgsMDQ4PEA=="));
    AT(dvz_drp2_stream_create_buffer(
        stream, 2, 16, DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 10));
    AT(dvz_drp2_stream_copy_texture_to_buffer(stream, 10, 1, 2, 0, 2, 2, 8, 2));
    AT(dvz_drp2_stream_finish_command_encoder(stream, 10, 11));
    AT(dvz_drp2_stream_queue_submit(stream, 11, 12));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(drp2_test_vklite_validation_clean(suite, ctx));

    uint8_t expected[16] = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    uint8_t downloaded[16] = {0};
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, 2, 0, 16, downloaded));
    for (uint32_t i = 0; i < 16; i++)
    {
        AT(downloaded[i] == expected[i]);
    }

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_vklite_copies_buffer_to_texture(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzGpuCtx* ctx = NULL;
    DvzDrp2Runtime* runtime = drp2_test_vklite_fixture_runtime(suite, &ctx);
    if (runtime == NULL)
        return 0;
    ANN(ctx);

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_buffer(
        stream, 1, 16,
        DVZ_DRP2_BUFFER_USAGE_COPY_SRC | DVZ_DRP2_BUFFER_USAGE_COPY_DST |
            DVZ_DRP2_BUFFER_USAGE_MAP_WRITE));
    AT(dvz_drp2_stream_write_buffer_base64(stream, 1, 0, 16, "AQIDBAUGBwgJCgsMDQ4PEA=="));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 2, 2, 2, DVZ_DRP2_TEXTURE_USAGE_COPY_DST | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC));
    AT(dvz_drp2_stream_create_buffer(
        stream, 3, 16, DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 10));
    AT(dvz_drp2_stream_copy_buffer_to_texture(stream, 10, 1, 0, 2, 2, 2, 8, 2));
    AT(dvz_drp2_stream_copy_texture_to_buffer(stream, 10, 2, 3, 0, 2, 2, 8, 2));
    AT(dvz_drp2_stream_finish_command_encoder(stream, 10, 11));
    AT(dvz_drp2_stream_queue_submit(stream, 11, 12));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(drp2_test_vklite_validation_clean(suite, ctx));

    uint8_t expected[16] = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    uint8_t downloaded[16] = {0};
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, 3, 0, 16, downloaded));
    for (uint32_t i = 0; i < 16; i++)
    {
        AT(downloaded[i] == expected[i]);
    }

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_vklite_copies_texture_to_texture(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzGpuCtx* ctx = NULL;
    DvzDrp2Runtime* runtime = drp2_test_vklite_fixture_runtime(suite, &ctx);
    if (runtime == NULL)
        return 0;
    ANN(ctx);

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 1, 2, 2, DVZ_DRP2_TEXTURE_USAGE_COPY_DST | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 2, 2, 2, DVZ_DRP2_TEXTURE_USAGE_COPY_DST | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC));
    AT(dvz_drp2_stream_write_texture_2d_base64(
        stream, 1, 0, 2, 2, 8, 2, "AQIDBAUGBwgJCgsMDQ4PEA=="));
    AT(dvz_drp2_stream_create_buffer(
        stream, 3, 16, DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 10));
    AT(dvz_drp2_stream_copy_texture_to_texture(stream, 10, 1, 2, 2, 2));
    AT(dvz_drp2_stream_copy_texture_to_buffer(stream, 10, 2, 3, 0, 2, 2, 8, 2));
    AT(dvz_drp2_stream_finish_command_encoder(stream, 10, 11));
    AT(dvz_drp2_stream_queue_submit(stream, 11, 12));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(drp2_test_vklite_validation_clean(suite, ctx));

    uint8_t expected[16] = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    uint8_t downloaded[16] = {0};
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, 3, 0, 16, downloaded));
    for (uint32_t i = 0; i < 16; i++)
    {
        AT(downloaded[i] == expected[i]);
    }

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_vklite_creates_glsl_shader_modules(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzGpuCtx* ctx = NULL;
    DvzDrp2Runtime* runtime = drp2_test_vklite_fixture_runtime(suite, &ctx);
    if (runtime == NULL)
        return 0;
    ANN(ctx);

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 1, "VERTEX", "glsl",
        "#version 450\nvoid main(){gl_Position=vec4(0.0,0.0,0.0,1.0);}"));
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 2, "FRAGMENT", "glsl",
        "#version 450\nlayout(location=0)out vec4 color;void main(){color=vec4(1.0);}"));
    AT(dvz_drp2_stream_destroy_shader_module(stream, 1));
    AT(dvz_drp2_stream_destroy_shader_module(stream, 2));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(drp2_test_vklite_validation_clean(suite, ctx));

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_vklite_creates_render_pipeline(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzGpuCtx* ctx = NULL;
    DvzDrp2Runtime* runtime = drp2_test_vklite_fixture_runtime(suite, &ctx);
    if (runtime == NULL)
        return 0;
    ANN(ctx);

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 1, "VERTEX", "glsl",
        "#version 450\nvoid main(){gl_Position=vec4(0.0,0.0,0.0,1.0);}"));
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 2, "FRAGMENT", "glsl",
        "#version 450\nlayout(location=0)out vec4 color;void main(){color=vec4(1.0);}"));
    AT(drp2_test_create_render_pipeline(stream, 3, 1, 2, 0));
    AT(dvz_drp2_stream_destroy_render_pipeline(stream, 3));
    AT(dvz_drp2_stream_destroy_shader_module(stream, 1));
    AT(dvz_drp2_stream_destroy_shader_module(stream, 2));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(drp2_test_vklite_validation_clean(suite, ctx));

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_vklite_rejects_invalid_glsl_shader(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    if (!drp2_test_vklite_runtime_available())
    {
        tst_skip(suite, "Vulkan instance creation failed");
        return 0;
    }

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("test_drp2_runtime_vklite_rejects_invalid_glsl_shader skipped: no GPU");
        tst_skip(suite, "no GPU");
        return 0;
    }

    DvzDrp2RuntimeConfig cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&cfg);
    ANN(runtime);

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    /* Intentionally broken GLSL — missing version directive and garbled syntax. */
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 1, "VERTEX", "glsl", "this is not valid glsl {}}}}}"));

    tst_log_capture_begin(suite);
    tst_expect_error_begin(suite);
    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(tst_expect_error_end(suite) == 0);
    AT(!result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_INVALID_ARGUMENT);
    AT(drp2_test_captured_log_contains(suite, "GLSL compilation failed"));

    dvz_drp2_stream_destroy(stream);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



int test_drp2_runtime_vklite_rejects_pipeline_with_failed_shader(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    if (!drp2_test_vklite_runtime_available())
    {
        tst_skip(suite, "Vulkan instance creation failed");
        return 0;
    }

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn(
            "test_drp2_runtime_vklite_rejects_pipeline_with_failed_shader skipped: no GPU");
        tst_skip(suite, "no GPU");
        return 0;
    }

    DvzDrp2RuntimeConfig cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&cfg);
    ANN(runtime);

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    /* Valid fragment, broken vertex — pipeline creation must fail cleanly. */
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 1, "VERTEX", "glsl", "this is not valid glsl {}}}}}"));
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 2, "FRAGMENT", "glsl",
        "#version 450\nlayout(location=0)out vec4 c;void main(){c=vec4(1.0);}"));
    AT(drp2_test_create_render_pipeline(stream, 3, 1, 2, 0));

    tst_log_capture_begin(suite);
    tst_expect_error_begin(suite);
    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(tst_expect_error_end(suite) == 0);
    AT(!result.ok);
    /* Runtime must not crash or leave a NULL pipeline object registered. */
    AT(drp2_test_vklite_validation_clean(suite, ctx));

    dvz_drp2_stream_destroy(stream);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



int test_drp2_runtime_vklite_destroy_after_partial_failure(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    if (!drp2_test_vklite_runtime_available())
    {
        tst_skip(suite, "Vulkan instance creation failed");
        return 0;
    }

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("test_drp2_runtime_vklite_destroy_after_partial_failure skipped: no GPU");
        tst_skip(suite, "no GPU");
        return 0;
    }

    DvzDrp2RuntimeConfig cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&cfg);
    ANN(runtime);

    /* Execute a stream that fails mid-way (bad GLSL). */
    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 1, "VERTEX", "glsl", "this is not valid glsl {}}}}}"));

    tst_log_capture_begin(suite);
    tst_expect_error_begin(suite);
    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(tst_expect_error_end(suite) == 0);
    AT(!result.ok);
    dvz_drp2_stream_destroy(stream);

    /* Destroy the runtime after the failed execution — must not crash or leak. */
    dvz_drp2_runtime_destroy(runtime);
    AT(drp2_test_vklite_validation_clean(suite, ctx));

    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



int test_drp2_runtime_vklite_reallocates_object_table_safely(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzGpuCtx* ctx = NULL;
    DvzDrp2Runtime* runtime = drp2_test_vklite_fixture_runtime(suite, &ctx);
    if (runtime == NULL)
        return 0;
    ANN(ctx);

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 1, "VERTEX", "glsl",
        "#version 450\nvec2 p[3]=vec2[](vec2(-1,-1),vec2(3,-1),vec2(-1,3));"
        "void main(){gl_Position=vec4(p[gl_VertexIndex],0,1);}"));
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 2, "FRAGMENT", "glsl",
        "#version 450\nlayout(location=0)out vec4 color;void main(){color=vec4(1.0);}"));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 4, 2, 2,
        DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC));

    /* Fill the initial 64-slot table; the pipeline created below must grow it. */
    for (uint64_t id = 100; id < 161; id++)
    {
        AT(dvz_drp2_stream_create_sampler(stream, id));
    }

    AT(drp2_test_create_render_pipeline(stream, 3, 1, 2, 0));
    AT(dvz_drp2_stream_create_buffer(
        stream, 5, 4, DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ));

    AT(dvz_drp2_stream_begin_command_encoder(stream, 10));
    AT(dvz_drp2_stream_begin_render_pass(stream, 11, 10, 4));
    AT(dvz_drp2_stream_set_pipeline(stream, 11, 3));
    AT(dvz_drp2_stream_draw(stream, 11, 3, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(stream, 11));
    AT(dvz_drp2_stream_copy_texture_to_buffer(stream, 10, 4, 5, 0, 1, 1, 4, 1));
    AT(dvz_drp2_stream_finish_command_encoder(stream, 10, 12));
    AT(dvz_drp2_stream_queue_submit(stream, 12, 13));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(drp2_test_vklite_validation_clean(suite, ctx));

    uint8_t downloaded[4] = {0};
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, 5, 0, 4, downloaded));
    AT(downloaded[0] == 255);
    AT(downloaded[1] == 255);
    AT(downloaded[2] == 255);
    AT(downloaded[3] == 255);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_vklite_draws_render_pass(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzGpuCtx* ctx = NULL;
    DvzDrp2Runtime* runtime = drp2_test_vklite_fixture_runtime(suite, &ctx);
    if (runtime == NULL)
        return 0;
    ANN(ctx);

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 1, "VERTEX", "glsl",
        "#version 450\nvec2 p[3]=vec2[](vec2(-1,-1),vec2(3,-1),vec2(-1,3));"
        "void main(){gl_Position=vec4(p[gl_VertexIndex],0,1);}"));
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 2, "FRAGMENT", "glsl",
        "#version 450\nlayout(location=0)out vec4 color;void main(){color=vec4(1.0);}"));
    AT(drp2_test_create_render_pipeline(stream, 3, 1, 2, 0));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 4, 2, 2,
        DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC));
    AT(dvz_drp2_stream_create_buffer(
        stream, 5, 4, DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 10));
    AT(dvz_drp2_stream_begin_render_pass(stream, 11, 10, 4));
    AT(dvz_drp2_stream_set_pipeline(stream, 11, 3));
    AT(dvz_drp2_stream_draw(stream, 11, 3, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(stream, 11));
    AT(dvz_drp2_stream_copy_texture_to_buffer(stream, 10, 4, 5, 0, 1, 1, 4, 1));
    AT(dvz_drp2_stream_finish_command_encoder(stream, 10, 12));
    AT(dvz_drp2_stream_queue_submit(stream, 12, 13));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(drp2_test_vklite_validation_clean(suite, ctx));

    uint8_t downloaded[4] = {0};
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, 5, 0, 4, downloaded));
    AT(downloaded[0] == 255);
    AT(downloaded[1] == 255);
    AT(downloaded[2] == 255);
    AT(downloaded[3] == 255);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_vklite_render_area_independent_from_viewport(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzGpuCtx* ctx = NULL;
    DvzDrp2Runtime* runtime = drp2_test_vklite_fixture_runtime(suite, &ctx);
    if (runtime == NULL)
        return 0;
    ANN(ctx);

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);
    static const uint8_t magenta[16] = {
        255, 0, 255, 255, 255, 0, 255, 255,
        255, 0, 255, 255, 255, 0, 255, 255,
    };

    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 4, 2, 2,
        DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_COPY_DST |
            DVZ_DRP2_TEXTURE_USAGE_COPY_SRC));
    AT(dvz_drp2_stream_write_texture_2d_borrowed(stream, 4, 0, 2, 2, 8, 2, magenta));
    AT(dvz_drp2_stream_create_buffer(
        stream, 5, 16, DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 10));
    DvzDrp2RenderPassDesc desc = dvz_drp2_render_pass_desc();
    desc.id = 11;
    desc.encoder_id = 10;
    desc.render_area_px[2] = 2;
    desc.render_area_px[3] = 2;
    desc.viewport_px[0] = 1.0f;
    desc.viewport_px[2] = 1.0f;
    desc.viewport_px[3] = 2.0f;
    desc.scissor_px[0] = 1.0f;
    desc.scissor_px[2] = 1.0f;
    desc.scissor_px[3] = 2.0f;
    desc.color_attachments[0].texture_id = 4;
    desc.color_attachments[0].load_op = DVZ_DRP2_ATTACHMENT_LOAD_CLEAR;
    desc.color_attachments[0].store_op = DVZ_DRP2_ATTACHMENT_STORE_STORE;
    desc.color_attachments[0].access = DVZ_DRP2_ATTACHMENT_ACCESS_WRITE;
    desc.color_attachments[0].clear = true;
    desc.color_attachments[0].clear_color[0] = 1.0f;
    desc.color_attachments[0].clear_color[3] = 1.0f;
    AT(dvz_drp2_stream_begin_render_pass_desc(stream, &desc));
    AT(dvz_drp2_stream_end_render_pass(stream, 11));
    AT(dvz_drp2_stream_copy_texture_to_buffer(stream, 10, 4, 5, 0, 2, 2, 8, 2));
    AT(dvz_drp2_stream_finish_command_encoder(stream, 10, 12));
    AT(dvz_drp2_stream_queue_submit(stream, 12, 13));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(drp2_test_vklite_validation_clean(suite, ctx));

    uint8_t downloaded[16] = {0};
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, 5, 0, 16, downloaded));
    for (uint32_t i = 0; i < sizeof(downloaded); i += 4)
    {
        AT(downloaded[i + 0] == 255);
        AT(downloaded[i + 1] == 0);
        AT(downloaded[i + 2] == 0);
        AT(downloaded[i + 3] == 255);
    }

    dvz_drp2_stream_destroy(stream);
    return 0;
}


int test_drp2_runtime_vklite_draws_named_depth_render_pass(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzGpuCtx* ctx = NULL;
    DvzDrp2Runtime* runtime = drp2_test_vklite_fixture_runtime(suite, &ctx);
    if (runtime == NULL)
        return 0;
    ANN(ctx);

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 1, "VERTEX", "glsl",
        "#version 450\nvec2 p[3]=vec2[](vec2(-1,-1),vec2(3,-1),vec2(-1,3));"
        "void main(){gl_Position=vec4(p[gl_VertexIndex],0,1);}"));
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 2, "FRAGMENT", "glsl",
        "#version 450\nlayout(location=0)out vec4 color;void main(){color=vec4(1.0);}"));
    AT(drp2_test_create_render_pipeline(stream, 3, 1, 2, 0));
    AT(dvz_drp2_stream_pipeline_set_depth_state(stream, true, DVZ_COMPARE_OP_LESS_OR_EQUAL));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 4, 2, 2,
        DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC));
    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        stream, 5, 2, 2, DVZ_FORMAT_D32_SFLOAT, DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT));
    AT(dvz_drp2_stream_create_buffer(
        stream, 6, 4, DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 10));
    AT(dvz_drp2_stream_begin_render_pass_clear(stream, 11, 10, 4, 0, 0, 0, 1));
    AT(dvz_drp2_stream_begin_render_pass_set_depth_texture(stream, 5, 1.0f));
    AT(dvz_drp2_stream_set_pipeline(stream, 11, 3));
    AT(dvz_drp2_stream_draw(stream, 11, 3, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(stream, 11));
    AT(dvz_drp2_stream_copy_texture_to_buffer(stream, 10, 4, 6, 0, 1, 1, 4, 1));
    AT(dvz_drp2_stream_finish_command_encoder(stream, 10, 12));
    AT(dvz_drp2_stream_queue_submit(stream, 12, 13));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(drp2_test_vklite_validation_clean(suite, ctx));

    uint8_t downloaded[4] = {0};
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, 6, 0, 4, downloaded));
    AT(downloaded[0] == 255);
    AT(downloaded[1] == 255);
    AT(downloaded[2] == 255);
    AT(downloaded[3] == 255);

    dvz_drp2_stream_destroy(stream);
    return 0;
}


int test_drp2_runtime_vklite_draws_msaa_resolve_render_pass(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzGpuCtx* ctx = NULL;
    DvzDrp2Runtime* runtime = drp2_test_vklite_fixture_runtime(suite, &ctx);
    if (runtime == NULL)
        return 0;
    ANN(ctx);

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 1, "VERTEX", "glsl",
        "#version 450\nvec2 p[3]=vec2[](vec2(-1,-1),vec2(3,-1),vec2(-1,3));"
        "void main(){gl_Position=vec4(p[gl_VertexIndex],0,1);}"));
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 2, "FRAGMENT", "glsl",
        "#version 450\nlayout(location=0)out vec4 color;"
        "void main(){color=vec4(0,0,1,1);}"));
    AT(drp2_test_create_render_pipeline(stream, 3, 1, 2, 0));
    uint32_t sample_count = _supported_color_sample_count(ctx);
    if (sample_count == 1)
    {
        tst_skip(suite, "multisampled RGBA8 color attachments are unsupported");
        dvz_drp2_stream_destroy(stream);
        return 0;
    }
    AT(dvz_drp2_stream_pipeline_set_multisampling(stream, sample_count, false));
    DvzDrp2TextureDesc msaa_desc = dvz_drp2_texture_desc();
    msaa_desc.id = 4;
    msaa_desc.width = 2;
    msaa_desc.height = 2;
    msaa_desc.depth = 1;
    msaa_desc.format = DVZ_FORMAT_R8G8B8A8_UNORM;
    msaa_desc.usage = DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT;
    msaa_desc.sample_count = sample_count;
    AT(dvz_drp2_stream_create_texture(stream, &msaa_desc));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 5, 2, 2,
        DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC));
    AT(dvz_drp2_stream_create_buffer(
        stream, 6, 4, DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 10));
    AT(dvz_drp2_stream_begin_render_pass_clear(stream, 11, 10, 4, 0, 0, 0, 1));
    AT(dvz_drp2_stream_begin_render_pass_set_color_attachment_resolve(
        stream, 0, 5, VK_RESOLVE_MODE_AVERAGE_BIT));
    AT(dvz_drp2_stream_set_pipeline(stream, 11, 3));
    AT(dvz_drp2_stream_draw(stream, 11, 3, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(stream, 11));
    AT(dvz_drp2_stream_copy_texture_to_buffer(stream, 10, 5, 6, 0, 1, 1, 4, 1));
    AT(dvz_drp2_stream_finish_command_encoder(stream, 10, 12));
    AT(dvz_drp2_stream_queue_submit(stream, 12, 13));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(drp2_test_vklite_validation_clean(suite, ctx));

    uint8_t downloaded[4] = {0};
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, 6, 0, 4, downloaded));
    AT(downloaded[0] == 0);
    AT(downloaded[1] == 0);
    AT(downloaded[2] == 255);
    AT(downloaded[3] == 255);

    dvz_drp2_stream_destroy(stream);
    return 0;
}


/**
 * Execute a rendered rg32uint query-style payload and verify its 8-byte readback.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_drp2_runtime_vklite_draws_rg32uint_readback(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzGpuCtx* ctx = NULL;
    DvzDrp2Runtime* runtime = drp2_test_vklite_fixture_runtime(suite, &ctx);
    if (runtime == NULL)
        return 0;
    ANN(ctx);

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 1, "VERTEX", "glsl",
        "#version 450\nvec2 p[3]=vec2[](vec2(-1,-1),vec2(3,-1),vec2(-1,3));"
        "void main(){gl_Position=vec4(p[gl_VertexIndex],0,1);}"));
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 2, "FRAGMENT", "glsl",
        "#version 450\nlayout(location=0)out uvec2 query;"
        "void main(){query=uvec2(0x11223344u,0x55667788u);}"));
    AT(drp2_test_create_render_pipeline(stream, 3, 1, 2, 0));
    AT(dvz_drp2_stream_pipeline_set_color_target(stream, 0, DVZ_FORMAT_R32G32_UINT));
    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        stream, 4, 1, 1, DVZ_FORMAT_R32G32_UINT,
        DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC));
    AT(dvz_drp2_stream_create_buffer(
        stream, 5, 8, DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 10));
    AT(dvz_drp2_stream_begin_render_pass_clear(stream, 11, 10, 4, 0, 0, 0, 0));
    AT(dvz_drp2_stream_set_pipeline(stream, 11, 3));
    AT(dvz_drp2_stream_draw(stream, 11, 3, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(stream, 11));
    AT(dvz_drp2_stream_copy_texture_to_buffer(stream, 10, 4, 5, 0, 1, 1, 8, 1));
    AT(dvz_drp2_stream_finish_command_encoder(stream, 10, 12));
    AT(dvz_drp2_stream_queue_submit_readback(stream, 12, 13, 5, 0, 8));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(drp2_test_vklite_validation_clean(suite, ctx));

    uint32_t downloaded[2] = {0};
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, 5, 0, sizeof(downloaded), downloaded));
    AT(downloaded[0] == 0x11223344u);
    AT(downloaded[1] == 0x55667788u);

    dvz_drp2_stream_destroy(stream);
    return 0;
}


int test_drp2_runtime_vklite_draws_multi_color_render_pass(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzGpuCtx* ctx = NULL;
    DvzDrp2Runtime* runtime = drp2_test_vklite_fixture_runtime(suite, &ctx);
    if (runtime == NULL)
        return 0;
    ANN(ctx);

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 1, "VERTEX", "glsl",
        "#version 450\nvec2 p[3]=vec2[](vec2(-1,-1),vec2(3,-1),vec2(-1,3));"
        "void main(){gl_Position=vec4(p[gl_VertexIndex],0,1);}"));
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 2, "FRAGMENT", "glsl",
        "#version 450\nlayout(location=0)out vec4 c0;layout(location=1)out vec4 c1;"
        "void main(){c0=vec4(1,0,0,1);c1=vec4(0,1,0,1);}"));
    AT(drp2_test_create_render_pipeline(stream, 3, 1, 2, 0));
    AT(dvz_drp2_stream_pipeline_set_color_target(stream, 1, DVZ_FORMAT_R8G8B8A8_UNORM));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 4, 2, 2,
        DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 5, 2, 2,
        DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC));
    AT(dvz_drp2_stream_create_buffer(
        stream, 6, 4, DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ));
    AT(dvz_drp2_stream_create_buffer(
        stream, 7, 4, DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 10));
    AT(dvz_drp2_stream_begin_render_pass_clear(stream, 11, 10, 4, 0, 0, 0, 1));
    AT(dvz_drp2_stream_begin_render_pass_add_color_attachment(
        stream, 5, 0, 0, 0, 1, true));
    AT(dvz_drp2_stream_set_pipeline(stream, 11, 3));
    AT(dvz_drp2_stream_draw(stream, 11, 3, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(stream, 11));
    AT(dvz_drp2_stream_copy_texture_to_buffer(stream, 10, 4, 6, 0, 1, 1, 4, 1));
    AT(dvz_drp2_stream_copy_texture_to_buffer(stream, 10, 5, 7, 0, 1, 1, 4, 1));
    AT(dvz_drp2_stream_finish_command_encoder(stream, 10, 12));
    AT(dvz_drp2_stream_queue_submit(stream, 12, 13));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(drp2_test_vklite_validation_clean(suite, ctx));

    uint8_t red[4] = {0};
    uint8_t green[4] = {0};
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, 6, 0, 4, red));
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, 7, 0, 4, green));
    AT(red[0] == 255);
    AT(red[1] == 0);
    AT(green[0] == 0);
    AT(green[1] == 255);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



/**
 * Execute WBOIT-shaped RGBA16F/R16F accumulation and resolve passes through vklite.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_drp2_runtime_vklite_draws_wboit_format_passes(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    if (!drp2_test_vklite_runtime_available())
    {
        tst_skip(suite, "Vulkan instance creation failed");
        return 0;
    }

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceFeatures features10 = {0};
    features10.independentBlend = true;
    dvz_gpu_ctx_config_features10(&gpu_cfg, &features10);
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("DRP2 WBOIT format pass test skipped because GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzDrp2RuntimeConfig cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&cfg);
    ANN(runtime);

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));

    AT(dvz_drp2_stream_create_sampler(stream, 2));

    DvzDrp2BindGroupLayoutEntry layout_entries[3] = {
        {
            .binding = 0,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
            .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
            .access = DVZ_DRP2_BINDING_ACCESS_READ,
        },
        {
            .binding = 1,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
            .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
            .access = DVZ_DRP2_BINDING_ACCESS_READ,
        },
        {
            .binding = 2,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
            .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
            .access = DVZ_DRP2_BINDING_ACCESS_READ,
        },
    };
    AT(dvz_drp2_stream_create_bind_group_layout_entries(stream, 3, 3, layout_entries));

    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 10, "VERTEX", "glsl",
        "#version 450\nvec2 p[3]=vec2[](vec2(-1,-1),vec2(3,-1),vec2(-1,3));"
        "void main(){gl_Position=vec4(p[gl_VertexIndex],0,1);}"));
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 11, "FRAGMENT", "glsl",
        "#version 450\nlayout(location=0)out vec4 accum;"
        "layout(location=1)out float reveal;"
        "void main(){accum=vec4(1,0,0,1);reveal=1;}"));
    AT(drp2_test_create_render_pipeline(stream, 12, 10, 11, 0));
    AT(dvz_drp2_stream_pipeline_set_color_target(
        stream, 0, DVZ_FORMAT_R16G16B16A16_SFLOAT));
    AT(dvz_drp2_stream_pipeline_set_color_target(stream, 1, DVZ_FORMAT_R16_SFLOAT));
    AT(dvz_drp2_stream_pipeline_set_color_blend(
        stream, 0, DVZ_BLEND_FACTOR_ONE, DVZ_BLEND_FACTOR_ONE, DVZ_BLEND_OP_ADD,
        DVZ_BLEND_FACTOR_ONE, DVZ_BLEND_FACTOR_ONE, DVZ_BLEND_OP_ADD,
        DVZ_MASK_COLOR_R | DVZ_MASK_COLOR_G | DVZ_MASK_COLOR_B |
            DVZ_MASK_COLOR_A));
    AT(dvz_drp2_stream_pipeline_set_color_blend(
        stream, 1, DVZ_BLEND_FACTOR_ONE, DVZ_BLEND_FACTOR_ONE, DVZ_BLEND_OP_ADD,
        DVZ_BLEND_FACTOR_ONE, DVZ_BLEND_FACTOR_ONE, DVZ_BLEND_OP_ADD, DVZ_MASK_COLOR_R));

    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 20, "VERTEX", "glsl",
        "#version 450\nvec2 p[3]=vec2[](vec2(-1,-1),vec2(3,-1),vec2(-1,3));"
        "void main(){gl_Position=vec4(p[gl_VertexIndex],0,1);}"));
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 21, "FRAGMENT", "glsl",
        "#version 450\nlayout(set=0,binding=0)uniform texture2D accum_tex;"
        "layout(set=0,binding=1)uniform texture2D reveal_tex;"
        "layout(set=0,binding=2)uniform sampler samp;"
        "layout(location=0)out vec4 color;"
        "void main(){vec4 a=texelFetch(sampler2D(accum_tex,samp),ivec2(0),0);"
        "float r=texelFetch(sampler2D(reveal_tex,samp),ivec2(0),0).r;"
        "color=vec4(a.r,r,0,1);}"));
    AT(drp2_test_create_render_pipeline_with_bind_group_layout(
        stream, 22, 20, 21, 0, 3));

    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        stream, 30, 2, 2, DVZ_FORMAT_R16G16B16A16_SFLOAT,
        DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING));
    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        stream, 31, 2, 2, DVZ_FORMAT_R16_SFLOAT,
        DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 32, 2, 2,
        DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC));
    AT(dvz_drp2_stream_create_buffer(
        stream, 33, 4, DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ));

    DvzDrp2BindGroupEntry bind_entries[3] = {
        {
            .binding = 0,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
            .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
            .resource_id = 30,
        },
        {
            .binding = 1,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
            .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
            .resource_id = 31,
        },
        {
            .binding = 2,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
            .resource_kind = DVZ_DRP2_BINDING_RESOURCE_SAMPLER,
            .resource_id = 2,
        },
    };
    AT(dvz_drp2_stream_create_bind_group_entries(stream, 34, 3, 3, bind_entries));

    AT(dvz_drp2_stream_begin_command_encoder(stream, 40));
    AT(dvz_drp2_stream_begin_render_pass_clear(stream, 41, 40, 30, 0, 0, 0, 0));
    AT(dvz_drp2_stream_begin_render_pass_add_color_attachment(
        stream, 31, 0, 0, 0, 0, true));
    AT(dvz_drp2_stream_set_pipeline(stream, 41, 12));
    AT(dvz_drp2_stream_draw(stream, 41, 3, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(stream, 41));

    AT(dvz_drp2_stream_begin_render_pass_clear(stream, 42, 40, 32, 0, 0, 0, 1));
    AT(dvz_drp2_stream_set_pipeline(stream, 42, 22));
    AT(dvz_drp2_stream_set_bind_group(stream, 42, 0, 34));
    AT(dvz_drp2_stream_draw(stream, 42, 3, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(stream, 42));
    AT(dvz_drp2_stream_copy_texture_to_buffer(stream, 40, 32, 33, 0, 1, 1, 4, 1));
    AT(dvz_drp2_stream_finish_command_encoder(stream, 40, 43));
    AT(dvz_drp2_stream_queue_submit(stream, 43, 44));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(drp2_test_vklite_validation_clean(suite, ctx));

    uint8_t resolved[4] = {0};
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, 33, 0, 4, resolved));
    AT(resolved[0] == 255);
    AT(resolved[1] == 255);
    AT(resolved[2] == 0);
    AT(resolved[3] == 255);

    dvz_drp2_stream_destroy(stream);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}


/**
 * Execute a depth-peeling-shaped ping/pong multi-pass stream through vklite.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_drp2_runtime_vklite_draws_depth_peeling_shape(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    if (!drp2_test_vklite_runtime_available())
    {
        tst_skip(suite, "Vulkan instance creation failed");
        return 0;
    }

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceFeatures features10 = {0};
    features10.independentBlend = true;
    dvz_gpu_ctx_config_features10(&gpu_cfg, &features10);
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("DRP2 depth peeling shape test skipped because GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzDrp2RuntimeConfig cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&cfg);
    ANN(runtime);

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));

    AT(dvz_drp2_stream_create_sampler(stream, 2));

    DvzDrp2BindGroupLayoutEntry sampled_entries[4] = {
        {
            .binding = 0,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
            .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
            .access = DVZ_DRP2_BINDING_ACCESS_READ,
        },
        {
            .binding = 1,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
            .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
            .access = DVZ_DRP2_BINDING_ACCESS_READ,
        },
        {
            .binding = 2,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
            .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
            .access = DVZ_DRP2_BINDING_ACCESS_READ,
        },
        {
            .binding = 3,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
            .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
            .access = DVZ_DRP2_BINDING_ACCESS_READ,
        },
    };
    AT(dvz_drp2_stream_create_bind_group_layout_entries(stream, 3, 4, sampled_entries));

    const char* fullscreen_vs =
        "#version 450\nvec2 p[3]=vec2[](vec2(-1,-1),vec2(3,-1),vec2(-1,3));"
        "void main(){gl_Position=vec4(p[gl_VertexIndex],0,1);}";
    AT(dvz_drp2_stream_create_shader_module_format(stream, 10, "VERTEX", "glsl", fullscreen_vs));
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 11, "FRAGMENT", "glsl",
        "#version 450\nlayout(location=0)out vec4 color;"
        "void main(){color=vec4(0.05,0.05,0.05,1.0);}"));
    AT(drp2_test_create_render_pipeline(stream, 12, 10, 11, 0));
    AT(dvz_drp2_stream_pipeline_set_depth_state(stream, true, DVZ_COMPARE_OP_LESS_OR_EQUAL));

    AT(dvz_drp2_stream_create_shader_module_format(stream, 20, "VERTEX", "glsl", fullscreen_vs));
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 21, "FRAGMENT", "glsl",
        "#version 450\nlayout(location=0)out vec4 front_accum;"
        "layout(location=1)out vec4 back_accum;layout(location=2)out vec4 depth_pair;"
        "void main(){front_accum=vec4(0.25,0,0,1);back_accum=vec4(0,0.25,0,1);"
        "depth_pair=vec4(gl_FragCoord.z,1.0-gl_FragCoord.z,0,1);}"));
    AT(drp2_test_create_render_pipeline(stream, 22, 20, 21, 0));
    AT(dvz_drp2_stream_pipeline_set_color_target(
        stream, 0, DVZ_FORMAT_R16G16B16A16_SFLOAT));
    AT(dvz_drp2_stream_pipeline_set_color_target(
        stream, 1, DVZ_FORMAT_R16G16B16A16_SFLOAT));
    AT(dvz_drp2_stream_pipeline_set_color_target(
        stream, 2, DVZ_FORMAT_R16G16B16A16_SFLOAT));
    AT(dvz_drp2_stream_pipeline_set_depth_state(stream, false, DVZ_COMPARE_OP_LESS_OR_EQUAL));
    AT(dvz_drp2_stream_pipeline_set_raster_state(
        stream, DVZ_CULL_MODE_BACK, DVZ_FRONT_FACE_COUNTER_CLOCKWISE));

    AT(dvz_drp2_stream_create_shader_module_format(stream, 30, "VERTEX", "glsl", fullscreen_vs));
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 31, "FRAGMENT", "glsl",
        "#version 450\nlayout(set=0,binding=0)uniform texture2D prev_front;"
        "layout(set=0,binding=1)uniform texture2D prev_back;"
        "layout(set=0,binding=2)uniform texture2D prev_depth;"
        "layout(set=0,binding=3)uniform sampler samp;"
        "layout(location=0)out vec4 front_accum;layout(location=1)out vec4 back_accum;"
        "layout(location=2)out vec4 depth_pair;"
        "void main(){ivec2 uv=ivec2(gl_FragCoord.xy);"
        "vec4 f=texelFetch(sampler2D(prev_front,samp),uv,0);"
        "vec4 b=texelFetch(sampler2D(prev_back,samp),uv,0);"
        "vec4 d=texelFetch(sampler2D(prev_depth,samp),uv,0);"
        "front_accum=f+vec4(0.25,0,0,0);back_accum=b+vec4(0,0.25,0,0);"
        "depth_pair=d+vec4(0.05,0.05,0,0);}"));
    AT(drp2_test_create_render_pipeline_with_bind_group_layout(
        stream, 32, 30, 31, 0, 3));
    AT(dvz_drp2_stream_pipeline_set_color_target(
        stream, 0, DVZ_FORMAT_R16G16B16A16_SFLOAT));
    AT(dvz_drp2_stream_pipeline_set_color_target(
        stream, 1, DVZ_FORMAT_R16G16B16A16_SFLOAT));
    AT(dvz_drp2_stream_pipeline_set_color_target(
        stream, 2, DVZ_FORMAT_R16G16B16A16_SFLOAT));
    AT(dvz_drp2_stream_pipeline_set_depth_state(stream, false, DVZ_COMPARE_OP_LESS_OR_EQUAL));
    AT(dvz_drp2_stream_pipeline_set_raster_state(
        stream, DVZ_CULL_MODE_FRONT, DVZ_FRONT_FACE_COUNTER_CLOCKWISE));

    AT(dvz_drp2_stream_create_shader_module_format(stream, 40, "VERTEX", "glsl", fullscreen_vs));
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 41, "FRAGMENT", "glsl",
        "#version 450\nlayout(set=0,binding=0)uniform texture2D front_accum;"
        "layout(set=0,binding=1)uniform texture2D back_accum;"
        "layout(set=0,binding=2)uniform texture2D depth_pair;"
        "layout(set=0,binding=3)uniform sampler samp;"
        "layout(location=0)out vec4 color;"
        "void main(){ivec2 uv=ivec2(gl_FragCoord.xy);"
        "vec4 f=texelFetch(sampler2D(front_accum,samp),uv,0);"
        "vec4 b=texelFetch(sampler2D(back_accum,samp),uv,0);"
        "vec4 d=texelFetch(sampler2D(depth_pair,samp),uv,0);"
        "color=vec4(f.r,b.g,d.r,1.0);}"));
    AT(drp2_test_create_render_pipeline_with_bind_group_layout(
        stream, 42, 40, 41, 0, 3));

    uint32_t sampled_attachment =
        DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING;
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 50, 2, 2,
        DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC));
    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        stream, 51, 2, 2, DVZ_FORMAT_D32_SFLOAT, DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT));
    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        stream, 52, 2, 2, DVZ_FORMAT_R16G16B16A16_SFLOAT, sampled_attachment));
    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        stream, 53, 2, 2, DVZ_FORMAT_R16G16B16A16_SFLOAT, sampled_attachment));
    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        stream, 54, 2, 2, DVZ_FORMAT_R16G16B16A16_SFLOAT, sampled_attachment));
    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        stream, 55, 2, 2, DVZ_FORMAT_R16G16B16A16_SFLOAT, sampled_attachment));
    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        stream, 56, 2, 2, DVZ_FORMAT_R16G16B16A16_SFLOAT, sampled_attachment));
    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        stream, 57, 2, 2, DVZ_FORMAT_R16G16B16A16_SFLOAT, sampled_attachment));
    AT(dvz_drp2_stream_create_buffer(
        stream, 70, 4, DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ));

    DvzDrp2BindGroupEntry peel_entries[4] = {
        {
            .binding = 0,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
            .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
            .resource_id = 52,
        },
        {
            .binding = 1,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
            .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
            .resource_id = 53,
        },
        {
            .binding = 2,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
            .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
            .resource_id = 54,
        },
        {
            .binding = 3,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
            .resource_kind = DVZ_DRP2_BINDING_RESOURCE_SAMPLER,
            .resource_id = 2,
        },
    };
    AT(dvz_drp2_stream_create_bind_group_entries(stream, 60, 3, 4, peel_entries));

    DvzDrp2BindGroupEntry composite_entries[4] = {
        {
            .binding = 0,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
            .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
            .resource_id = 55,
        },
        {
            .binding = 1,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
            .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
            .resource_id = 56,
        },
        {
            .binding = 2,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
            .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
            .resource_id = 57,
        },
        {
            .binding = 3,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
            .resource_kind = DVZ_DRP2_BINDING_RESOURCE_SAMPLER,
            .resource_id = 2,
        },
    };
    AT(dvz_drp2_stream_create_bind_group_entries(stream, 61, 3, 4, composite_entries));

    AT(dvz_drp2_stream_begin_command_encoder(stream, 80));
    AT(dvz_drp2_stream_begin_render_pass_clear(stream, 81, 80, 50, 0, 0, 0, 1));
    AT(dvz_drp2_stream_begin_render_pass_set_depth_texture(stream, 51, 1.0f));
    AT(dvz_drp2_stream_set_pipeline(stream, 81, 12));
    AT(dvz_drp2_stream_draw(stream, 81, 3, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(stream, 81));

    AT(dvz_drp2_stream_begin_render_pass_clear(stream, 82, 80, 52, 0, 0, 0, 0));
    AT(dvz_drp2_stream_begin_render_pass_add_color_attachment(stream, 53, 0, 0, 0, 0, true));
    AT(dvz_drp2_stream_begin_render_pass_add_color_attachment(stream, 54, 0, 0, 0, 0, true));
    AT(dvz_drp2_stream_begin_render_pass_set_depth_texture(stream, 51, 1.0f));
    AT(dvz_drp2_stream_begin_render_pass_set_depth_ops(
        stream, DVZ_DRP2_ATTACHMENT_LOAD_LOAD, DVZ_DRP2_ATTACHMENT_STORE_DONT_CARE));
    AT(dvz_drp2_stream_begin_render_pass_set_depth_access(
        stream, DVZ_DRP2_ATTACHMENT_ACCESS_READ));
    AT(dvz_drp2_stream_set_pipeline(stream, 82, 22));
    AT(dvz_drp2_stream_draw(stream, 82, 3, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(stream, 82));

    AT(dvz_drp2_stream_begin_render_pass_clear(stream, 83, 80, 55, 0, 0, 0, 0));
    AT(dvz_drp2_stream_begin_render_pass_add_color_attachment(stream, 56, 0, 0, 0, 0, true));
    AT(dvz_drp2_stream_begin_render_pass_add_color_attachment(stream, 57, 0, 0, 0, 0, true));
    AT(dvz_drp2_stream_begin_render_pass_set_depth_texture(stream, 51, 1.0f));
    AT(dvz_drp2_stream_begin_render_pass_set_depth_ops(
        stream, DVZ_DRP2_ATTACHMENT_LOAD_LOAD, DVZ_DRP2_ATTACHMENT_STORE_DONT_CARE));
    AT(dvz_drp2_stream_begin_render_pass_set_depth_access(
        stream, DVZ_DRP2_ATTACHMENT_ACCESS_READ));
    AT(dvz_drp2_stream_set_pipeline(stream, 83, 32));
    AT(dvz_drp2_stream_set_bind_group(stream, 83, 0, 60));
    AT(dvz_drp2_stream_draw(stream, 83, 3, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(stream, 83));

    AT(dvz_drp2_stream_begin_render_pass_clear(stream, 84, 80, 50, 0, 0, 0, 1));
    AT(dvz_drp2_stream_begin_render_pass_set_color_attachment_ops(
        stream, 0, DVZ_DRP2_ATTACHMENT_LOAD_LOAD, DVZ_DRP2_ATTACHMENT_STORE_STORE));
    AT(dvz_drp2_stream_set_pipeline(stream, 84, 42));
    AT(dvz_drp2_stream_set_bind_group(stream, 84, 0, 61));
    AT(dvz_drp2_stream_draw(stream, 84, 3, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(stream, 84));
    AT(dvz_drp2_stream_copy_texture_to_buffer(stream, 80, 50, 70, 0, 1, 1, 4, 1));
    AT(dvz_drp2_stream_finish_command_encoder(stream, 80, 85));
    AT(dvz_drp2_stream_queue_submit(stream, 85, 86));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(drp2_test_vklite_validation_clean(suite, ctx));

    uint8_t resolved[4] = {0};
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, 70, 0, 4, resolved));
    AT(resolved[0] > 0);
    AT(resolved[1] > 0);
    AT(resolved[2] > 0);
    AT(resolved[3] == 255);

    Drp2VkliteObject* resolved_target = _vklite_find(runtime->vklite_state, 50);
    Drp2VkliteObject* opaque_depth = _vklite_find(runtime->vklite_state, 51);
    Drp2VkliteObject* front_ping = _vklite_find(runtime->vklite_state, 52);
    Drp2VkliteObject* back_ping = _vklite_find(runtime->vklite_state, 53);
    Drp2VkliteObject* depth_ping = _vklite_find(runtime->vklite_state, 54);
    Drp2VkliteObject* front_pong = _vklite_find(runtime->vklite_state, 55);
    Drp2VkliteObject* back_pong = _vklite_find(runtime->vklite_state, 56);
    Drp2VkliteObject* depth_pong = _vklite_find(runtime->vklite_state, 57);
    ANN(resolved_target);
    ANN(opaque_depth);
    ANN(front_ping);
    ANN(back_ping);
    ANN(depth_ping);
    ANN(front_pong);
    ANN(back_pong);
    ANN(depth_pong);
    AT(resolved_target->texture_access == DRP2_TEXTURE_ACCESS_TRANSFER_READ);
    AT(resolved_target->image_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    AT(opaque_depth->texture_access == DRP2_TEXTURE_ACCESS_DEPTH_ATTACHMENT_READ);
    AT(opaque_depth->image_layout == VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL);
    AT(front_ping->texture_access == DRP2_TEXTURE_ACCESS_SAMPLED_READ);
    AT(back_ping->texture_access == DRP2_TEXTURE_ACCESS_SAMPLED_READ);
    AT(depth_ping->texture_access == DRP2_TEXTURE_ACCESS_SAMPLED_READ);
    AT(front_pong->texture_access == DRP2_TEXTURE_ACCESS_SAMPLED_READ);
    AT(back_pong->texture_access == DRP2_TEXTURE_ACCESS_SAMPLED_READ);
    AT(depth_pong->texture_access == DRP2_TEXTURE_ACCESS_SAMPLED_READ);

    dvz_drp2_stream_destroy(stream);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



/**
 * Execute a pass that samples a named depth texture while attaching it read-only.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_drp2_runtime_vklite_samples_read_only_active_depth(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    if (!drp2_test_vklite_runtime_available())
    {
        tst_skip(suite, "Vulkan instance creation failed");
        return 0;
    }

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("DRP2 active depth sampling test skipped because GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzDrp2RuntimeConfig cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&cfg);
    ANN(runtime);

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));

    const char* fullscreen_vs =
        "#version 450\nvec2 p[3]=vec2[](vec2(-1,-1),vec2(3,-1),vec2(-1,3));"
        "void main(){gl_Position=vec4(p[gl_VertexIndex],0.25,1);}";
    AT(dvz_drp2_stream_create_shader_module_format(stream, 10, "VERTEX", "glsl", fullscreen_vs));
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 11, "FRAGMENT", "glsl",
        "#version 450\nlayout(location=0)out vec4 color;"
        "void main(){color=vec4(1,0,0,1);}"));
    AT(drp2_test_create_render_pipeline(stream, 12, 10, 11, 0));
    AT(dvz_drp2_stream_pipeline_set_depth_state(stream, true, DVZ_COMPARE_OP_LESS_OR_EQUAL));

    AT(dvz_drp2_stream_create_shader_module_format(stream, 20, "VERTEX", "glsl", fullscreen_vs));
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 21, "FRAGMENT", "glsl",
        "#version 450\nlayout(set=0,binding=0)uniform texture2D depth_tex;"
        "layout(set=0,binding=1)uniform sampler samp;"
        "layout(location=0)out vec4 color;"
        "void main(){float d=texelFetch(sampler2D(depth_tex,samp),ivec2(0),0).r;"
        "color=vec4(d,d,d,1);}"));

    DvzDrp2BindGroupLayoutEntry layout_entries[2] = {
        {
            .binding = 0,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
            .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
            .access = DVZ_DRP2_BINDING_ACCESS_READ,
        },
        {
            .binding = 1,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
            .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
            .access = DVZ_DRP2_BINDING_ACCESS_READ,
        },
    };
    AT(dvz_drp2_stream_create_bind_group_layout_entries(stream, 30, 2, layout_entries));
    AT(drp2_test_create_render_pipeline_with_bind_group_layout(
        stream, 22, 20, 21, 0, 30));
    AT(dvz_drp2_stream_pipeline_set_depth_state(stream, false, DVZ_COMPARE_OP_ALWAYS));

    AT(dvz_drp2_stream_create_sampler(stream, 31));
    uint32_t depth_usage =
        DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING;
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 50, 2, 2, DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT));
    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        stream, 51, 2, 2, DVZ_FORMAT_D32_SFLOAT, depth_usage));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 52, 2, 2,
        DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC));
    AT(dvz_drp2_stream_create_buffer(
        stream, 60, 4, DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ));

    DvzDrp2BindGroupEntry bind_entries[2] = {
        {
            .binding = 0,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
            .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
            .resource_id = 51,
        },
        {
            .binding = 1,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
            .resource_kind = DVZ_DRP2_BINDING_RESOURCE_SAMPLER,
            .resource_id = 31,
        },
    };
    AT(dvz_drp2_stream_create_bind_group_entries(stream, 32, 30, 2, bind_entries));

    AT(dvz_drp2_stream_begin_command_encoder(stream, 70));
    AT(dvz_drp2_stream_begin_render_pass_clear(stream, 71, 70, 50, 0, 0, 0, 1));
    AT(dvz_drp2_stream_begin_render_pass_set_depth_texture(stream, 51, 1.0f));
    AT(dvz_drp2_stream_set_pipeline(stream, 71, 12));
    AT(dvz_drp2_stream_draw(stream, 71, 3, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(stream, 71));

    AT(dvz_drp2_stream_begin_render_pass_clear(stream, 72, 70, 52, 0, 0, 0, 1));
    AT(dvz_drp2_stream_begin_render_pass_set_depth_texture(stream, 51, 1.0f));
    AT(dvz_drp2_stream_begin_render_pass_set_depth_ops(
        stream, DVZ_DRP2_ATTACHMENT_LOAD_LOAD, DVZ_DRP2_ATTACHMENT_STORE_DONT_CARE));
    AT(dvz_drp2_stream_begin_render_pass_set_depth_access(
        stream, DVZ_DRP2_ATTACHMENT_ACCESS_READ));
    AT(dvz_drp2_stream_set_pipeline(stream, 72, 22));
    AT(dvz_drp2_stream_set_bind_group(stream, 72, 0, 32));
    AT(dvz_drp2_stream_draw(stream, 72, 3, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(stream, 72));
    AT(dvz_drp2_stream_copy_texture_to_buffer(stream, 70, 52, 60, 0, 1, 1, 4, 1));
    AT(dvz_drp2_stream_finish_command_encoder(stream, 70, 73));
    AT(dvz_drp2_stream_queue_submit(stream, 73, 74));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(drp2_test_vklite_validation_clean(suite, ctx));

    uint8_t resolved[4] = {0};
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, 60, 0, 4, resolved));
    AT(resolved[0] > 0);
    AT(resolved[3] == 255);

    Drp2VkliteObject* depth = _vklite_find(runtime->vklite_state, 51);
    ANN(depth);
    AT(depth->texture_access == DRP2_TEXTURE_ACCESS_DEPTH_ATTACHMENT_READ);
    AT(depth->image_layout == VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL);

    dvz_drp2_stream_destroy(stream);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}


/**
 * Ensure unused bind groups do not transition textures for unrelated render passes.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_drp2_runtime_vklite_ignores_unused_render_pass_bind_groups(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzGpuCtx* ctx = NULL;
    DvzDrp2Runtime* runtime = drp2_test_vklite_fixture_runtime(suite, &ctx);
    if (runtime == NULL)
        return 0;
    ANN(ctx);

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));

    const char* fullscreen_vs =
        "#version 450\nvec2 p[3]=vec2[](vec2(-1,-1),vec2(3,-1),vec2(-1,3));"
        "void main(){gl_Position=vec4(p[gl_VertexIndex],0.25,1);}";
    AT(dvz_drp2_stream_create_shader_module_format(stream, 10, "VERTEX", "glsl", fullscreen_vs));
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 11, "FRAGMENT", "glsl",
        "#version 450\nlayout(location=0)out vec4 color;"
        "void main(){color=vec4(1,0,0,1);}"));
    AT(drp2_test_create_render_pipeline(stream, 12, 10, 11, 0));
    AT(dvz_drp2_stream_pipeline_set_depth_state(stream, true, DVZ_COMPARE_OP_LESS_OR_EQUAL));
    AT(drp2_test_create_render_pipeline(stream, 13, 10, 11, 0));

    DvzDrp2BindGroupLayoutEntry layout_entries[2] = {
        {
            .binding = 0,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
            .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
            .access = DVZ_DRP2_BINDING_ACCESS_READ,
        },
        {
            .binding = 1,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
            .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
            .access = DVZ_DRP2_BINDING_ACCESS_READ,
        },
    };
    AT(dvz_drp2_stream_create_bind_group_layout_entries(stream, 30, 2, layout_entries));
    AT(dvz_drp2_stream_create_sampler(stream, 31));
    uint32_t depth_usage =
        DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING;
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 50, 2, 2, DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT));
    AT(dvz_drp2_stream_create_texture_2d_format_usage(
        stream, 51, 2, 2, DVZ_FORMAT_D32_SFLOAT, depth_usage));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 52, 2, 2, DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT));

    DvzDrp2BindGroupEntry bind_entries[2] = {
        {
            .binding = 0,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
            .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
            .resource_id = 51,
        },
        {
            .binding = 1,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
            .resource_kind = DVZ_DRP2_BINDING_RESOURCE_SAMPLER,
            .resource_id = 31,
        },
    };
    AT(dvz_drp2_stream_create_bind_group_entries(stream, 32, 30, 2, bind_entries));

    AT(dvz_drp2_stream_begin_command_encoder(stream, 70));
    AT(dvz_drp2_stream_begin_render_pass_clear(stream, 71, 70, 50, 0, 0, 0, 1));
    AT(dvz_drp2_stream_begin_render_pass_set_depth_texture(stream, 51, 1.0f));
    AT(dvz_drp2_stream_set_pipeline(stream, 71, 12));
    AT(dvz_drp2_stream_draw(stream, 71, 3, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(stream, 71));

    AT(dvz_drp2_stream_begin_render_pass_clear(stream, 72, 70, 52, 0, 0, 0, 1));
    AT(dvz_drp2_stream_set_pipeline(stream, 72, 13));
    AT(dvz_drp2_stream_draw(stream, 72, 3, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(stream, 72));
    AT(dvz_drp2_stream_finish_command_encoder(stream, 70, 73));
    AT(dvz_drp2_stream_queue_submit(stream, 73, 74));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(drp2_test_vklite_validation_clean(suite, ctx));

    Drp2VkliteObject* depth = _vklite_find(runtime->vklite_state, 51);
    ANN(depth);
    AT(depth->texture_access == DRP2_TEXTURE_ACCESS_DEPTH_ATTACHMENT);
    AT(depth->image_layout == VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL);

    dvz_drp2_stream_destroy(stream);
    return 0;
}



int test_drp2_runtime_vklite_samples_then_copies_texture(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzGpuCtx* ctx = NULL;
    DvzDrp2Runtime* runtime = drp2_test_vklite_fixture_runtime(suite, &ctx);
    if (runtime == NULL)
        return 0;
    ANN(ctx);

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 1, "VERTEX", "glsl",
        "#version 450\nvec2 p[3]=vec2[](vec2(-1,-1),vec2(3,-1),vec2(-1,3));"
        "void main(){gl_Position=vec4(p[gl_VertexIndex],0,1);}"));
    AT(dvz_drp2_stream_create_shader_module_format(
        stream, 2, "FRAGMENT", "glsl",
        "#version 450\nlayout(set=0,binding=0)uniform texture2D tex;"
        "layout(set=0,binding=1)uniform sampler samp;"
        "layout(location=0)out vec4 color;"
        "void main(){color=texture(sampler2D(tex,samp),vec2(0.5));}"));
    AT(dvz_drp2_stream_create_texture_sampler_bind_group_layout(stream, 3));
    AT(drp2_test_create_render_pipeline_with_bind_group_layout(stream, 4, 1, 2, 0, 3));
    AT(dvz_drp2_stream_create_sampler(stream, 5));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 6, 2, 2,
        DVZ_DRP2_TEXTURE_USAGE_COPY_DST | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC |
            DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING));
    AT(dvz_drp2_stream_write_texture_2d_base64(
        stream, 6, 0, 2, 2, 8, 2, "/wAA//8AAP//AAD//wAA/w=="));
    AT(dvz_drp2_stream_create_texture_sampler_bind_group(stream, 7, 3, 6, 5));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 8, 2, 2,
        DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC));
    AT(dvz_drp2_stream_create_buffer(
        stream, 9, 16, DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ));
    AT(dvz_drp2_stream_begin_command_encoder(stream, 10));
    AT(dvz_drp2_stream_begin_render_pass(stream, 11, 10, 8));
    AT(dvz_drp2_stream_set_pipeline(stream, 11, 4));
    AT(dvz_drp2_stream_set_bind_group(stream, 11, 0, 7));
    AT(dvz_drp2_stream_draw(stream, 11, 3, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(stream, 11));
    AT(dvz_drp2_stream_copy_texture_to_buffer(stream, 10, 6, 9, 0, 2, 2, 8, 2));
    AT(dvz_drp2_stream_finish_command_encoder(stream, 10, 12));
    AT(dvz_drp2_stream_queue_submit(stream, 12, 13));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(drp2_test_vklite_validation_clean(suite, ctx));

    uint8_t downloaded[16] = {0};
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, 9, 0, 16, downloaded));
    for (uint32_t i = 0; i < 16; i += 4)
    {
        AT(downloaded[i + 0] == 255);
        AT(downloaded[i + 1] == 0);
        AT(downloaded[i + 2] == 0);
        AT(downloaded[i + 3] == 255);
    }

    dvz_drp2_stream_destroy(stream);
    return 0;
}



/**
 * Ensure texture recreation refreshes descriptors for existing dependent bind groups.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
int test_drp2_runtime_vklite_refreshes_bind_group_after_texture_recreate(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzGpuCtx* ctx = NULL;
    DvzDrp2Runtime* runtime = drp2_test_vklite_fixture_runtime(suite, &ctx);
    if (runtime == NULL)
        return 0;
    ANN(ctx);

    uint8_t red[16] = {0};
    for (uint32_t i = 0; i < sizeof(red); i += 4)
    {
        red[i + 0] = 255;
        red[i + 3] = 255;
    }

    DvzDrp2CommandStream* setup = dvz_drp2_stream();
    ANN(setup);
    AT(dvz_drp2_stream_hello_renderer(setup, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(setup, "test-renderer"));
    AT(dvz_drp2_stream_create_shader_module_format(
        setup, 1, "VERTEX", "glsl",
        "#version 450\nvec2 p[3]=vec2[](vec2(-1,-1),vec2(3,-1),vec2(-1,3));"
        "void main(){gl_Position=vec4(p[gl_VertexIndex],0,1);}"));
    AT(dvz_drp2_stream_create_shader_module_format(
        setup, 2, "FRAGMENT", "glsl",
        "#version 450\nlayout(set=0,binding=0)uniform texture2D tex;"
        "layout(set=0,binding=1)uniform sampler samp;"
        "layout(location=0)out vec4 color;"
        "void main(){color=texture(sampler2D(tex,samp),vec2(0.5));}"));
    AT(dvz_drp2_stream_create_texture_sampler_bind_group_layout(setup, 3));
    AT(drp2_test_create_render_pipeline_with_bind_group_layout(setup, 4, 1, 2, 0, 3));
    AT(dvz_drp2_stream_create_sampler(setup, 5));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        setup, 6, 2, 2,
        DVZ_DRP2_TEXTURE_USAGE_COPY_DST | DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING));
    AT(dvz_drp2_stream_write_texture_2d_borrowed(setup, 6, 0, 2, 2, 8, 2, red));
    AT(dvz_drp2_stream_create_texture_sampler_bind_group(setup, 7, 3, 6, 5));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        setup, 8, 2, 2,
        DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC));
    AT(dvz_drp2_stream_begin_command_encoder(setup, 9));
    AT(dvz_drp2_stream_begin_render_pass(setup, 10, 9, 8));
    AT(dvz_drp2_stream_set_pipeline(setup, 10, 4));
    AT(dvz_drp2_stream_set_bind_group(setup, 10, 0, 7));
    AT(dvz_drp2_stream_draw(setup, 10, 3, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(setup, 10));
    AT(dvz_drp2_stream_finish_command_encoder(setup, 9, 11));
    AT(dvz_drp2_stream_queue_submit(setup, 11, 12));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, setup);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);

    uint8_t green[64] = {0};
    for (uint32_t i = 0; i < sizeof(green); i += 4)
    {
        green[i + 1] = 255;
        green[i + 3] = 255;
    }

    DvzDrp2CommandStream* resized = dvz_drp2_stream();
    ANN(resized);
    AT(dvz_drp2_stream_create_texture_2d_usage(
        resized, 6, 4, 4,
        DVZ_DRP2_TEXTURE_USAGE_COPY_DST | DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING));
    AT(dvz_drp2_stream_write_texture_2d_borrowed(resized, 6, 0, 4, 4, 16, 4, green));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        resized, 18, 2, 2,
        DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC));
    AT(dvz_drp2_stream_create_buffer(
        resized, 19, 16, DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ));
    AT(dvz_drp2_stream_begin_command_encoder(resized, 20));
    AT(dvz_drp2_stream_begin_render_pass(resized, 21, 20, 18));
    AT(dvz_drp2_stream_set_pipeline(resized, 21, 4));
    AT(dvz_drp2_stream_set_bind_group(resized, 21, 0, 7));
    AT(dvz_drp2_stream_draw(resized, 21, 3, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(resized, 21));
    AT(dvz_drp2_stream_copy_texture_to_buffer(resized, 20, 18, 19, 0, 2, 2, 8, 2));
    AT(dvz_drp2_stream_finish_command_encoder(resized, 20, 22));
    AT(dvz_drp2_stream_queue_submit(resized, 22, 23));

    result = dvz_drp2_runtime_execute(runtime, resized);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(drp2_test_vklite_validation_clean(suite, ctx));

    uint8_t downloaded[16] = {0};
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, 19, 0, 16, downloaded));
    for (uint32_t i = 0; i < sizeof(downloaded); i += 4)
    {
        AT(downloaded[i + 0] == 0);
        AT(downloaded[i + 1] == 255);
        AT(downloaded[i + 2] == 0);
        AT(downloaded[i + 3] == 255);
    }

    dvz_drp2_stream_destroy(resized);
    dvz_drp2_stream_destroy(setup);
    return 0;
}



/**
 * Ensure buffer and sampler recreation refresh descriptors for existing bind groups.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
int test_drp2_runtime_vklite_refreshes_bind_group_after_buffer_sampler_recreate(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzGpuCtx* ctx = NULL;
    DvzDrp2Runtime* runtime = drp2_test_vklite_fixture_runtime(suite, &ctx);
    if (runtime == NULL)
        return 0;
    ANN(ctx);

    DvzDrp2BindGroupLayoutEntry storage_layout = {
        .binding = 0,
        .binding_type = DVZ_DRP2_BINDING_TYPE_STORAGE_BUFFER,
        .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
        .access = DVZ_DRP2_BINDING_ACCESS_READ,
    };
    DvzDrp2BindGroupEntry storage_entry = {
        .binding = 0,
        .binding_type = DVZ_DRP2_BINDING_TYPE_STORAGE_BUFFER,
        .resource_kind = DVZ_DRP2_BINDING_RESOURCE_BUFFER,
        .resource_id = 6,
        .offset = 0,
        .size = 16,
    };
    const uint64_t bind_group_layouts[3] = {3, 4, 10};

    float zero[4] = {0};
    uint8_t blue_texture[16] = {0};
    for (uint32_t i = 0; i < sizeof(blue_texture); i += 4)
    {
        blue_texture[i + 2] = 255;
        blue_texture[i + 3] = 255;
    }

    DvzDrp2CommandStream* setup = dvz_drp2_stream();
    ANN(setup);
    AT(dvz_drp2_stream_hello_renderer(setup, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(setup, "test-renderer"));
    AT(dvz_drp2_stream_create_shader_module_format(
        setup, 1, "VERTEX", "glsl",
        "#version 450\nvec2 p[3]=vec2[](vec2(-1,-1),vec2(3,-1),vec2(-1,3));"
        "void main(){gl_Position=vec4(p[gl_VertexIndex],0,1);}"));
    AT(dvz_drp2_stream_create_shader_module_format(
        setup, 2, "FRAGMENT", "glsl",
        "#version 450\n"
        "layout(set=0,binding=0)uniform Ubo{vec4 u_color;}ubo;"
        "layout(set=1,binding=0)readonly buffer Sbo{vec4 s_color;}sbo;"
        "layout(set=2,binding=0)uniform texture2D tex;"
        "layout(set=2,binding=1)uniform sampler samp;"
        "layout(location=0)out vec4 color;"
        "void main(){vec4 t=texture(sampler2D(tex,samp),vec2(0.5));"
        "color=vec4(ubo.u_color.r,sbo.s_color.g,t.b,1.0);}"));
    AT(dvz_drp2_stream_create_uniform_bind_group_layout(setup, 3));
    AT(dvz_drp2_stream_create_bind_group_layout_entries(setup, 4, 1, &storage_layout));
    AT(dvz_drp2_stream_create_texture_sampler_bind_group_layout(setup, 10));
    AT(drp2_test_create_render_pipeline_with_bind_group_layout(setup, 11, 1, 2, 0, 3));
    AT(dvz_drp2_stream_pipeline_set_bind_group_layouts(setup, 3, bind_group_layouts));
    AT(dvz_drp2_stream_create_buffer(
        setup, 5, 16, DVZ_DRP2_BUFFER_USAGE_UNIFORM | DVZ_DRP2_BUFFER_USAGE_COPY_DST));
    AT(dvz_drp2_stream_write_buffer_bytes(setup, 5, 0, sizeof(zero), zero));
    AT(dvz_drp2_stream_create_buffer(
        setup, 6, 16, DVZ_DRP2_BUFFER_USAGE_STORAGE | DVZ_DRP2_BUFFER_USAGE_COPY_DST));
    AT(dvz_drp2_stream_write_buffer_bytes(setup, 6, 0, sizeof(zero), zero));
    AT(dvz_drp2_stream_create_sampler(setup, 7));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        setup, 8, 2, 2,
        DVZ_DRP2_TEXTURE_USAGE_COPY_DST | DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING));
    AT(dvz_drp2_stream_write_texture_2d_borrowed(setup, 8, 0, 2, 2, 8, 2, blue_texture));
    AT(dvz_drp2_stream_create_uniform_bind_group(setup, 12, 3, 5, 0, 16));
    AT(dvz_drp2_stream_create_bind_group_entries(setup, 13, 4, 1, &storage_entry));
    AT(dvz_drp2_stream_create_texture_sampler_bind_group(setup, 14, 10, 8, 7));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        setup, 15, 2, 2, DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT));
    AT(dvz_drp2_stream_begin_command_encoder(setup, 16));
    AT(dvz_drp2_stream_begin_render_pass(setup, 17, 16, 15));
    AT(dvz_drp2_stream_set_pipeline(setup, 17, 11));
    AT(dvz_drp2_stream_set_bind_group(setup, 17, 0, 12));
    AT(dvz_drp2_stream_set_bind_group(setup, 17, 1, 13));
    AT(dvz_drp2_stream_set_bind_group(setup, 17, 2, 14));
    AT(dvz_drp2_stream_draw(setup, 17, 3, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(setup, 17));
    AT(dvz_drp2_stream_finish_command_encoder(setup, 16, 18));
    AT(dvz_drp2_stream_queue_submit(setup, 18, 19));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, setup);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);

    float red[4] = {1, 0, 0, 1};
    float green[4] = {0, 1, 0, 1};

    DvzDrp2CommandStream* recreated = dvz_drp2_stream();
    ANN(recreated);
    AT(dvz_drp2_stream_create_buffer(
        recreated, 5, 16, DVZ_DRP2_BUFFER_USAGE_UNIFORM | DVZ_DRP2_BUFFER_USAGE_COPY_DST));
    AT(dvz_drp2_stream_write_buffer_bytes(recreated, 5, 0, sizeof(red), red));
    AT(dvz_drp2_stream_create_buffer(
        recreated, 6, 16, DVZ_DRP2_BUFFER_USAGE_STORAGE | DVZ_DRP2_BUFFER_USAGE_COPY_DST));
    AT(dvz_drp2_stream_write_buffer_bytes(recreated, 6, 0, sizeof(green), green));
    AT(dvz_drp2_stream_create_sampler(recreated, 7));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        recreated, 20, 2, 2,
        DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC));
    AT(dvz_drp2_stream_create_buffer(
        recreated, 21, 4, DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ));
    AT(dvz_drp2_stream_begin_command_encoder(recreated, 30));
    AT(dvz_drp2_stream_begin_render_pass(recreated, 31, 30, 20));
    AT(dvz_drp2_stream_set_pipeline(recreated, 31, 11));
    AT(dvz_drp2_stream_set_bind_group(recreated, 31, 0, 12));
    AT(dvz_drp2_stream_set_bind_group(recreated, 31, 1, 13));
    AT(dvz_drp2_stream_set_bind_group(recreated, 31, 2, 14));
    AT(dvz_drp2_stream_draw(recreated, 31, 3, 1, 0, 0));
    AT(dvz_drp2_stream_end_render_pass(recreated, 31));
    AT(dvz_drp2_stream_copy_texture_to_buffer(recreated, 30, 20, 21, 0, 1, 1, 4, 1));
    AT(dvz_drp2_stream_finish_command_encoder(recreated, 30, 32));
    AT(dvz_drp2_stream_queue_submit(recreated, 32, 33));

    result = dvz_drp2_runtime_execute(runtime, recreated);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(drp2_test_vklite_validation_clean(suite, ctx));

    uint8_t downloaded[4] = {0};
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, 21, 0, sizeof(downloaded), downloaded));
    AT(downloaded[0] == 255);
    AT(downloaded[1] == 255);
    AT(downloaded[2] == 255);
    AT(downloaded[3] == 255);

    dvz_drp2_stream_destroy(recreated);
    dvz_drp2_stream_destroy(setup);
    return 0;
}



/**
 * Ensure descriptor refresh defers retired descriptors while a borrowed frame is active.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
int test_drp2_runtime_vklite_refresh_defers_retired_descriptors(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzGpuCtx* ctx = NULL;
    DvzDrp2Runtime* runtime = drp2_test_vklite_fixture_runtime(suite, &ctx);
    if (runtime == NULL)
        return 0;
    ANN(ctx);

    uint8_t rgba[16] = {0};
    for (uint32_t i = 0; i < sizeof(rgba); i += 4)
    {
        rgba[i + 0] = 255;
        rgba[i + 3] = 255;
    }

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_texture_sampler_bind_group_layout(stream, 3));
    AT(dvz_drp2_stream_create_sampler(stream, 5));
    AT(dvz_drp2_stream_create_texture_2d_usage(
        stream, 6, 2, 2,
        DVZ_DRP2_TEXTURE_USAGE_COPY_DST | DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING));
    AT(dvz_drp2_stream_write_texture_2d_borrowed(stream, 6, 0, 2, 2, 8, 2, rgba));
    AT(dvz_drp2_stream_create_texture_sampler_bind_group(stream, 7, 3, 6, 5));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(runtime->vklite_state != NULL);

    Drp2VkliteState* state = runtime->vklite_state;
    Drp2VkliteObject* bind_group = _vklite_find(state, 7);
    ANN(bind_group);
    AT(bind_group->descriptors != NULL);
    DvzDescriptors* previous = bind_group->descriptors;

    VkCommandBuffer command_buffer = (VkCommandBuffer)(uintptr_t)0x123;
    state->active_borrowed_command_buffer = command_buffer;
    result = _vklite_refresh_dependent_bind_groups(state, 6, 99);
    AT(result.ok);
    AT(bind_group->descriptors != NULL);
    AT(bind_group->descriptors != previous);
    AT(state->deferred_count == 1);
    AT(state->deferred[0].command_buffer == command_buffer);
    AT(state->deferred[0].object.descriptors == previous);

    _vklite_flush_deferred_for_command_buffer(state, command_buffer);
    AT(state->deferred_count == 0);
    state->active_borrowed_command_buffer = VK_NULL_HANDLE;

    dvz_drp2_stream_destroy(stream);
    return 0;
}
#endif



/* ---- New regression tests ---- */

#if DVZ_DRP2_HAS_VKLITE
int test_drp2_write_buffer_bytes_large_payload_executes(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzGpuCtx* ctx = NULL;
    DvzDrp2Runtime* runtime = drp2_test_vklite_fixture_runtime(suite, &ctx);
    if (runtime == NULL)
        return 0;
    ANN(ctx);

    /* 3000 floats = 12000 bytes — well above the old 4096-byte dvz_strdup cap. */
    const uint32_t N    = 3000;
    const uint64_t SIZE = N * sizeof(float);
    float* upload = (float*)dvz_malloc(SIZE);
    ANN(upload);
    for (uint32_t i = 0; i < N; i++)
        upload[i] = (float)i;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);
    AT(dvz_drp2_stream_hello_renderer(stream, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer"));
    AT(dvz_drp2_stream_create_buffer(
        stream, 1, SIZE,
        DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ));
    AT(dvz_drp2_stream_write_buffer_bytes(stream, 1, 0, SIZE, upload));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(drp2_test_vklite_validation_clean(suite, ctx));

    float* downloaded = (float*)dvz_malloc(SIZE);
    ANN(downloaded);
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, 1, 0, SIZE, downloaded));
    for (uint32_t i = 0; i < N; i++)
    {
        AT(downloaded[i] == upload[i]);
    }

    dvz_free(downloaded);
    dvz_free(upload);
    dvz_drp2_stream_destroy(stream);
    return 0;
}
#endif
