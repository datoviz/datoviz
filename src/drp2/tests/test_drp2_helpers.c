/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  DRP2 test helpers                                                                            */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "../_stream.h"
#include "test_drp2_helpers.h"

#if DVZ_DRP2_HAS_VKLITE
#include "_log.h"
#include "datoviz/vk/device.h"
#include "datoviz/vk/instance.h"
#endif



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

bool drp2_test_captured_log_contains(const TstContext* suite, const char* needle)
{
    ANN(suite);
    ANN(needle);
    for (uint32_t i = 0; i < tst_log_capture_count(suite); i++)
    {
        const TstLogRecord* rec = tst_log_capture_get(suite, i);
        if (rec != NULL && strstr(rec->message, needle) != NULL)
            return true;
    }
    return false;
}



bool drp2_test_create_render_pipeline(
    DvzDrp2CommandStream* stream, uint64_t id, uint64_t vertex_shader_module_id,
    uint64_t fragment_shader_module_id, uint32_t vertex_buffer_slots)
{
    DvzDrp2RenderPipelineDesc desc = dvz_drp2_render_pipeline_desc();
    desc.id = id;
    desc.vertex_shader_module_id = vertex_shader_module_id;
    desc.fragment_shader_module_id = fragment_shader_module_id;
    desc.vertex_buffer_slots = vertex_buffer_slots;
    return dvz_drp2_stream_create_render_pipeline(stream, &desc);
}



bool drp2_test_create_render_pipeline_with_bind_group_layout(
    DvzDrp2CommandStream* stream, uint64_t id, uint64_t vertex_shader_module_id,
    uint64_t fragment_shader_module_id, uint32_t vertex_buffer_slots,
    uint64_t bind_group_layout_id)
{
    DvzDrp2RenderPipelineDesc desc = dvz_drp2_render_pipeline_desc();
    desc.id = id;
    desc.vertex_shader_module_id = vertex_shader_module_id;
    desc.fragment_shader_module_id = fragment_shader_module_id;
    desc.vertex_buffer_slots = vertex_buffer_slots;
    if (bind_group_layout_id != 0)
    {
        desc.bind_group_layout_count = 1;
        desc.bind_group_layout_ids = &bind_group_layout_id;
    }
    return dvz_drp2_stream_create_render_pipeline(stream, &desc);
}



#if DVZ_DRP2_HAS_VKLITE
typedef struct
{
    DvzGpuCtx* gpu_ctx;
    DvzDrp2Runtime* runtime;
    uint32_t validation_errors_before;
    bool available;
    const char* skip_reason;
} DvzDrp2VkliteFixture;



/**
 * Return a GPU context configuration for DRP2 vklite execution tests.
 *
 * @return GPU context configuration with required Vulkan 1.3 features
 */
static DvzGpuCtxConfig _drp2_vklite_gpu_ctx_config(void)
{
    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    return gpu_cfg;
}



bool drp2_test_vklite_runtime_available(void)
{
    DvzInstanceConfig cfg = dvz_instance_config();
    cfg.flags = 0;
    DvzInstance* instance = dvz_instance_create(&cfg);
    if (instance == NULL)
    {
        log_warn("DRP2 vklite runtime unavailable because Vulkan instance creation failed");
        return false;
    }
    dvz_instance_destroy(instance);
    return true;
}



void* drp2_test_vklite_fixture_create(TstSuite* suite, uint32_t worker_index)
{
    (void)suite;
    (void)worker_index;

    DvzDrp2VkliteFixture* fixture =
        (DvzDrp2VkliteFixture*)dvz_calloc(1, sizeof(DvzDrp2VkliteFixture));
    ANN(fixture);

    if (!drp2_test_vklite_runtime_available())
    {
        fixture->skip_reason = "Vulkan instance creation failed";
        return fixture;
    }

    DvzGpuCtxConfig gpu_cfg = _drp2_vklite_gpu_ctx_config();
    fixture->gpu_ctx = dvz_gpu_ctx(&gpu_cfg);
    if (fixture->gpu_ctx == NULL)
    {
        fixture->skip_reason = "GPU context creation failed";
        return fixture;
    }

    DvzDrp2RuntimeConfig runtime_cfg = dvz_drp2_runtime_vklite_config(
        dvz_gpu_ctx_device(fixture->gpu_ctx), dvz_gpu_ctx_alloc(fixture->gpu_ctx));
    fixture->runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    if (fixture->runtime == NULL)
    {
        fixture->skip_reason = "DRP2 runtime creation failed";
        return fixture;
    }

    fixture->available = true;
    return fixture;
}



void drp2_test_vklite_fixture_destroy(void* fixture_ptr)
{
    DvzDrp2VkliteFixture* fixture = (DvzDrp2VkliteFixture*)fixture_ptr;
    if (fixture == NULL)
        return;
    if (fixture->gpu_ctx != NULL)
    {
        DvzInstance* instance = dvz_gpu_ctx_instance(fixture->gpu_ctx);
        if (instance != NULL && dvz_instance_handle(instance) != VK_NULL_HANDLE)
            volkLoadInstance(dvz_instance_handle(instance));
    }
    if (fixture->runtime != NULL)
    {
        dvz_drp2_runtime_destroy(fixture->runtime);
        fixture->runtime = NULL;
    }
    if (fixture->gpu_ctx != NULL)
    {
        dvz_gpu_ctx_destroy(fixture->gpu_ctx);
        fixture->gpu_ctx = NULL;
    }
    dvz_free(fixture);
}



DvzDrp2Runtime* drp2_test_vklite_fixture_runtime(TstContext* suite, DvzGpuCtx** out_gpu_ctx)
{
    ANN(suite);
    DvzDrp2VkliteFixture* fixture =
        (DvzDrp2VkliteFixture*)tst_context_fixture(suite, TST_DRP2_VKLITE_FIXTURE);
    if (fixture == NULL)
    {
        tst_skip(suite, "DRP2 vklite fixture unavailable");
        return NULL;
    }
    if (!fixture->available)
    {
        tst_skip(suite, fixture->skip_reason != NULL ? fixture->skip_reason : "GPU unavailable");
        return NULL;
    }
    dvz_drp2_runtime_reset(fixture->runtime);
    if (out_gpu_ctx != NULL)
        *out_gpu_ctx = fixture->gpu_ctx;
    return fixture->runtime;
}



/**
 * Record the validation-error count before one shared-fixture DRP2 test.
 *
 * @param suite Active test context.
 * @param item Active test case.
 * @return Zero.
 */
int drp2_test_vklite_validation_setup(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    DvzDrp2VkliteFixture* fixture =
        (DvzDrp2VkliteFixture*)tst_context_fixture(suite, TST_DRP2_VKLITE_FIXTURE);
    if (fixture != NULL && fixture->available && fixture->gpu_ctx != NULL)
        fixture->validation_errors_before = dvz_gpu_ctx_error_count(fixture->gpu_ctx);
    return 0;
}



/**
 * Wait for shared GPU work and attribute new validation errors to the active test.
 *
 * @param suite Active test context.
 * @param item Active test case.
 * @return Zero when no validation error was added, otherwise nonzero.
 */
int drp2_test_vklite_validation_teardown(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    DvzDrp2VkliteFixture* fixture =
        (DvzDrp2VkliteFixture*)tst_context_fixture(suite, TST_DRP2_VKLITE_FIXTURE);
    if (fixture == NULL || !fixture->available || fixture->gpu_ctx == NULL)
        return 0;

    DvzDevice* device = dvz_gpu_ctx_device(fixture->gpu_ctx);
    if (device != NULL)
        dvz_device_wait(device);
    uint32_t validation_errors_after = dvz_gpu_ctx_error_count(fixture->gpu_ctx);
    if (validation_errors_after != fixture->validation_errors_before)
    {
        log_error(
            "Vulkan validation errors increased during %s (%u -> %u)", item->name,
            fixture->validation_errors_before, validation_errors_after);
        return 1;
    }
    return 0;
}



/**
 * Check that a test has not added a Vulkan validation error to its GPU context.
 *
 * @param suite Active test context.
 * @param gpu_ctx GPU context used by the test.
 * @return Whether the validation count still matches the test baseline.
 */
bool drp2_test_vklite_validation_clean(TstContext* suite, DvzGpuCtx* gpu_ctx)
{
    ANN(suite);
    ANN(gpu_ctx);
    DvzDrp2VkliteFixture* fixture =
        (DvzDrp2VkliteFixture*)tst_context_fixture(suite, TST_DRP2_VKLITE_FIXTURE);
    uint32_t baseline = 0;
    if (fixture != NULL && fixture->available && fixture->gpu_ctx == gpu_ctx)
        baseline = fixture->validation_errors_before;
    return dvz_gpu_ctx_error_count(gpu_ctx) == baseline;
}
#endif



DvzDrp2CommandStream* drp2_test_valid_render_stream(void)
{
    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    dvz_drp2_stream_hello_renderer(stream, "test-client");
    dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer");
    dvz_drp2_stream_create_buffer(
        stream, 1, 16, DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_VERTEX);
    dvz_drp2_stream_write_buffer_base64(stream, 1, 0, 16, "AAAAAAAAAAAAAAAAAAAAAA==");
    dvz_drp2_stream_create_shader_module(stream, 2, "vertex", "@vertex fn main() {}");
    dvz_drp2_stream_create_shader_module(stream, 3, "fragment", "@fragment fn main() {}");
    drp2_test_create_render_pipeline(stream, 4, 2, 3, 1);
    dvz_drp2_stream_create_texture_2d(stream, 5, 4, 4);
    dvz_drp2_stream_begin_command_encoder(stream, 6);
    dvz_drp2_stream_begin_render_pass(stream, 7, 6, 5);
    dvz_drp2_stream_set_pipeline(stream, 7, 4);
    dvz_drp2_stream_set_vertex_buffer(stream, 7, 0, 1, 0);
    dvz_drp2_stream_draw(stream, 7, 3, 1, 0, 0);
    dvz_drp2_stream_end_render_pass(stream, 7);
    dvz_drp2_stream_finish_command_encoder(stream, 6, 8);
    dvz_drp2_stream_queue_submit(stream, 8, 9);
    return stream;
}



DvzDrp2CommandStream* drp2_test_valid_indexed_render_stream(void)
{
    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    dvz_drp2_stream_hello_renderer(stream, "test-client");
    dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer");
    dvz_drp2_stream_create_shader_module(stream, 2, "vertex", "@vertex fn main() {}");
    dvz_drp2_stream_create_shader_module(stream, 3, "fragment", "@fragment fn main() {}");
    drp2_test_create_render_pipeline(stream, 4, 2, 3, 1);
    dvz_drp2_stream_create_buffer(stream, 11, 64, DVZ_DRP2_BUFFER_USAGE_VERTEX);
    dvz_drp2_stream_create_buffer(stream, 12, 64, DVZ_DRP2_BUFFER_USAGE_INDEX);
    dvz_drp2_stream_create_texture_2d(stream, 5, 4, 4);
    dvz_drp2_stream_begin_command_encoder(stream, 6);
    dvz_drp2_stream_begin_render_pass(stream, 7, 6, 5);
    dvz_drp2_stream_set_pipeline(stream, 7, 4);
    dvz_drp2_stream_set_vertex_buffer(stream, 7, 0, 11, 0);
    dvz_drp2_stream_set_index_buffer(stream, 7, 12, "uint16", 0);
    dvz_drp2_stream_draw_indexed(stream, 7, 3, 1, 0, 0, 0);
    dvz_drp2_stream_end_render_pass(stream, 7);
    dvz_drp2_stream_finish_command_encoder(stream, 6, 8);
    return stream;
}



DvzDrp2CommandStream* drp2_test_valid_compute_stream(void)
{
    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    dvz_drp2_stream_hello_renderer(stream, "test-client");
    dvz_drp2_stream_renderer_hello_reply(stream, "test-renderer");
    dvz_drp2_stream_create_shader_module(stream, 9000, "COMPUTE", "@compute fn main() {}");
    dvz_drp2_stream_create_compute_pipeline(stream, 20, 9000);
    dvz_drp2_stream_begin_command_encoder(stream, 1);
    dvz_drp2_stream_begin_compute_pass(stream, 2, 1);
    dvz_drp2_stream_set_pipeline(stream, 2, 20);
    dvz_drp2_stream_dispatch_workgroups(stream, 2, 1, 1, 1);
    dvz_drp2_stream_end_compute_pass(stream, 2);
    dvz_drp2_stream_finish_command_encoder(stream, 1, 3);
    return stream;
}



DvzStreamFrame drp2_test_stream_frame(uintptr_t seed, uint32_t width, uint32_t height)
{
    DvzStreamFrame frame = {0};
    frame.image = (VkImage)(seed + 1);
    frame.image_view = (VkImageView)(seed + 2);
    frame.command_buffer = (VkCommandBuffer)(seed + 3);
    frame.extent = (VkExtent2D){width, height};
    frame.color_format = VK_FORMAT_R8G8B8A8_UNORM;
    frame.image_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    frame.usage = DVZ_STREAM_FRAME_USAGE_RENDER_TARGET | DVZ_STREAM_FRAME_USAGE_COPY_DST;
    frame.command_buffer_recording = true;
    frame.image_borrowed = true;
    frame.image_view_borrowed = true;
    frame.command_buffer_borrowed = true;
    return frame;
}
