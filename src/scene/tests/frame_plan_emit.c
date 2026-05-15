/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene FramePlan emission tests                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "../_frame_plan_emit.h"
#include "../_scene.h"
#include "../../drp2/_stream.h"
#include "datoviz/canvas.h"
#include "datoviz/drp2.h"
#include "datoviz/scene.h"
#include "datoviz/vk/gpu_ctx.h"
#include "datoviz/vk/instance.h"
#include "datoviz/window.h"
#include "helpers.h"
#include "test_scene.h"
#include "testing.h"

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
bool _dvz_drp2_runtime_vklite_download_buffer(
    DvzDrp2Runtime* runtime, uint64_t buffer_id, uint64_t offset, uint64_t size, void* out);
#endif




/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_frame_plan_emit_drp2_static_render(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.convert", 10);
    ANN(plan);

    AT(dvz_frame_plan_upload(plan, "buf.point.position", 0, 16, "point.position"));
    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(plan, "visual.point.0"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_frame_plan_emit_drp2(plan, &caps, &report);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);
    DvzDrp2ValidationResult validation = dvz_drp2_validate_stream(stream);
    AT(validation.ok);
    AT(dvz_drp2_stream_count(stream) == 16);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 0)) == DVZ_DRP2_COMMAND_HELLO_RENDERER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 2)) == DVZ_DRP2_COMMAND_CREATE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 4)) ==
       DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 6)) ==
       DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 15)) == DVZ_DRP2_COMMAND_QUEUE_SUBMIT);

    char* json = dvz_drp2_stream_json(stream, "scene_static_render_from_c");
    ANN(json);
    AT(strstr(json, "\"cmd\": \"CreateRenderPipeline\"") != NULL);
    AT(strstr(json, "\"cmd\": \"CreateTexture\"") != NULL);
    AT(strstr(json, "\"cmd\": \"Draw\"") != NULL);
    AT(strstr(json, "\"command_buffer_ids\": [4]") != NULL);

    dvz_drp2_stream_json_destroy(json);
    AT(_assert_stream_matches_fixture(
           stream, "scene_static_render_from_c",
           "spec/drp2/fixtures/positive/scene_static_render_from_c.json") == 0);
    dvz_drp2_stream_destroy(stream);
    dvz_frame_plan_destroy(plan);
    return 0;
}


int test_frame_plan_emit_drp2_static_render_glsl(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.convert.glsl", 15);
    ANN(plan);

    AT(dvz_frame_plan_upload(plan, "buf.point.position", 0, 16, "point.position"));
    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(plan, "visual.point.0"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_frame_plan_emit_drp2_ex(plan, &caps, &report, &cfg);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);
    DvzDrp2ValidationResult validation = dvz_drp2_validate_stream(stream);
    AT(validation.ok);
    AT(dvz_drp2_stream_count(stream) == 16);

    char* json = dvz_drp2_stream_json(stream, "scene_static_render_glsl_from_c");
    ANN(json);
    AT(strstr(json, "\"format\": \"glsl\"") != NULL);
    AT(strstr(json, "#version 450\\nlayout(location=0)in vec3 pos;") != NULL);
    AT(strstr(json, "\"format\": \"wgsl\"") == NULL);

    dvz_drp2_stream_json_destroy(json);
    dvz_drp2_stream_destroy(stream);
    dvz_frame_plan_destroy(plan);
    return 0;
}


int test_frame_plan_emit_drp2_rejects_unsupported_shader_format(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.convert.unsupported_shader", 17);
    ANN(plan);

    AT(dvz_frame_plan_upload(plan, "buf.point.position", 0, 16, "point.position"));
    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(plan, "visual.point.0"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = false;
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_frame_plan_emit_drp2_ex(plan, &caps, &report, &cfg);
    AT(stream == NULL);
    AT(dvz_diagnostic_report_count(&report) == 1);
    AT(strcmp(dvz_diagnostic_report_get(&report, 0), "unsupported shader format") == 0);

    dvz_frame_plan_destroy(plan);
    return 0;
}


int test_frame_plan_emit_drp2_rejects_small_caps(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.convert.small_caps", 18);
    ANN(plan);

    AT(dvz_frame_plan_upload(plan, "buf.point.position", 0, 16, "point.position"));
    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(plan, "visual.point.0"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    dvz_capability_snapshot_default(&caps);
    caps.max_buffer_size = 8;
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_frame_plan_emit_drp2(plan, &caps, &report);
    AT(stream == NULL);
    AT(dvz_diagnostic_report_count(&report) == 1);
    AT(strcmp(dvz_diagnostic_report_get(&report, 0), "upload buffer exceeds max_buffer_size") == 0);

    dvz_diagnostic_report_init(&report);
    dvz_capability_snapshot_default(&caps);
    caps.max_texture_dimension_2d = 3;
    stream = dvz_frame_plan_emit_drp2(plan, &caps, &report);
    AT(stream == NULL);
    AT(dvz_diagnostic_report_count(&report) == 1);
    AT(strcmp(
           dvz_diagnostic_report_get(&report, 0),
           "max_texture_dimension_2d is too small for fixture render target") == 0);

    dvz_diagnostic_report_init(&report);
    dvz_capability_snapshot_default(&caps);
    caps.max_vertex_buffers = 0;
    stream = dvz_frame_plan_emit_drp2(plan, &caps, &report);
    AT(stream == NULL);
    AT(dvz_diagnostic_report_count(&report) == 1);
    AT(strcmp(
           dvz_diagnostic_report_get(&report, 0),
           "max_vertex_buffers is too small for fixture render pipeline") == 0);

    DvzFramePlan* texture_plan = dvz_frame_plan("figure.convert.small_bind_group_caps", 19);
    ANN(texture_plan);
    AT(dvz_frame_plan_upload(texture_plan, "tex.image.rgba", 0, 16, "image.rgba"));
    AT(dvz_frame_plan_render(texture_plan, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(texture_plan, "visual.image.0"));

    dvz_diagnostic_report_init(&report);
    dvz_capability_snapshot_default(&caps);
    caps.max_bind_groups = 0;
    stream = dvz_frame_plan_emit_drp2(texture_plan, &caps, &report);
    AT(stream == NULL);
    AT(dvz_diagnostic_report_count(&report) == 1);
    AT(strcmp(
           dvz_diagnostic_report_get(&report, 0),
           "max_bind_groups is too small for fixture bind groups") == 0);

    dvz_frame_plan_destroy(texture_plan);
    dvz_frame_plan_destroy(plan);
    return 0;
}


int test_frame_plan_emit_drp2_static_render_glsl_executes(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("scene GLSL DRP2 execution test skipped because GPU context creation failed");
        return 0;
    }

    DvzFramePlan* plan = dvz_frame_plan("figure.convert.glsl.execute", 16);
    ANN(plan);
    AT(dvz_frame_plan_upload(plan, "buf.point.position", 0, 16, "point.position"));
    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(plan, "visual.point.0"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_frame_plan_emit_drp2_ex(plan, &caps, &report, &emit_cfg);
    ANN(stream);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    dvz_drp2_runtime_destroy(runtime);
    dvz_drp2_stream_destroy(stream);
    dvz_frame_plan_destroy(plan);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}


int test_frame_plan_emit_drp2_readback_glsl_executes(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("scene readback DRP2 execution test skipped because GPU context creation failed");
        return 0;
    }

    DvzFramePlan* plan = dvz_frame_plan("figure.readback.glsl.execute", 20);
    ANN(plan);
    AT(dvz_frame_plan_upload(plan, "buf.point.position", 0, 16, "point.position"));
    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(plan, "visual.pickable.0"));
    AT(dvz_frame_plan_copy(plan, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(plan, "buf.pick.readback", "request.pick.0"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_frame_plan_emit_drp2_ex(plan, &caps, &report, &emit_cfg);
    ANN(stream);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);

    uint8_t downloaded[4] = {0};
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, 12, 0, 4, downloaded));
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    dvz_drp2_runtime_destroy(runtime);
    dvz_drp2_stream_destroy(stream);
    dvz_frame_plan_destroy(plan);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}


int test_frame_plan_emitter_runtime_two_frames_glsl_executes(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

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
            "scene runtime-mode DRP2 execution test skipped because GPU context creation failed");
        return 0;
    }

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);

    DvzFramePlan* frame0 = dvz_frame_plan("figure.runtime.glsl.execute", 0);
    DvzFramePlan* frame1 = dvz_frame_plan("figure.runtime.glsl.execute", 1);
    ANN(frame0);
    ANN(frame1);
    AT(dvz_frame_plan_upload(frame0, "buf.point.position", 0, 16, "point.position.0"));
    AT(dvz_frame_plan_render(frame0, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame0, "visual.pickable.0"));
    AT(dvz_frame_plan_copy(frame0, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame0, "buf.pick.readback", "request.pick.0"));

    AT(dvz_frame_plan_upload(frame1, "buf.point.position", 0, 16, "point.position.1"));
    AT(dvz_frame_plan_render(frame1, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame1, "visual.pickable.0"));
    AT(dvz_frame_plan_copy(frame1, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame1, "buf.pick.readback", "request.pick.1"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream0 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame0, &caps, &report, &emit_cfg);
    ANN(stream0);
    DvzDrp2CommandStream* stream1 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame1, &caps, &report, &emit_cfg);
    ANN(stream1);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream0);
    AT(result.ok);
    result = dvz_drp2_runtime_execute(runtime, stream1);
    AT(result.ok);

    uint8_t downloaded[4] = {0};
    uint64_t rb_id = dvz_frame_plan_emitter_object_id(emitter, "_rb");
    AT(rb_id != 0);
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, rb_id, 0, 4, downloaded));
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    dvz_drp2_runtime_destroy(runtime);
    dvz_drp2_stream_destroy(stream1);
    dvz_drp2_stream_destroy(stream0);
    dvz_frame_plan_destroy(frame1);
    dvz_frame_plan_destroy(frame0);
    dvz_frame_plan_emitter_destroy(emitter);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}


int test_frame_plan_emitter_runtime_dynamic_two_frames_glsl_executes(
    TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

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
            "scene dynamic runtime-mode DRP2 execution test skipped because GPU context creation "
            "failed");
        return 0;
    }

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);

    DvzFramePlan* frame0 = dvz_frame_plan("figure.runtime.dynamic.glsl.execute", 0);
    DvzFramePlan* frame1 = dvz_frame_plan("figure.runtime.dynamic.glsl.execute", 1);
    ANN(frame0);
    ANN(frame1);
    AT(dvz_frame_plan_upload(frame0, "buf.dynamic.position", 0, 16, "point.position.0"));
    AT(dvz_frame_plan_upload(frame0, "buf.dynamic.color", 0, 16, "point.color.0"));
    AT(dvz_frame_plan_render(frame0, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame0, "visual.dynamic.0"));
    AT(dvz_frame_plan_copy(frame0, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame0, "buf.pick.readback", "request.pick.0"));

    AT(dvz_frame_plan_upload(frame1, "buf.dynamic.position", 0, 16, "point.position.1"));
    AT(dvz_frame_plan_upload(frame1, "buf.dynamic.color", 0, 16, "point.color.1"));
    AT(dvz_frame_plan_render(frame1, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame1, "visual.dynamic.0"));
    AT(dvz_frame_plan_copy(frame1, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame1, "buf.pick.readback", "request.pick.1"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream0 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame0, &caps, &report, &emit_cfg);
    ANN(stream0);
    DvzDrp2CommandStream* stream1 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame1, &caps, &report, &emit_cfg);
    ANN(stream1);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream0);
    AT(result.ok);
    result = dvz_drp2_runtime_execute(runtime, stream1);
    AT(result.ok);

    uint8_t downloaded[4] = {0};
    uint64_t rb_id = dvz_frame_plan_emitter_object_id(emitter, "_rb");
    AT(rb_id != 0);
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, rb_id, 0, 4, downloaded));
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    dvz_drp2_runtime_destroy(runtime);
    dvz_drp2_stream_destroy(stream1);
    dvz_drp2_stream_destroy(stream0);
    dvz_frame_plan_destroy(frame1);
    dvz_frame_plan_destroy(frame0);
    dvz_frame_plan_emitter_destroy(emitter);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}


int test_frame_plan_emitter_runtime_texture_two_frames_glsl_executes(
    TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

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
            "scene texture runtime-mode DRP2 execution test skipped because GPU context creation "
            "failed");
        return 0;
    }

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);

    DvzFramePlan* frame0 = dvz_frame_plan("figure.runtime.texture.glsl.execute", 0);
    DvzFramePlan* frame1 = dvz_frame_plan("figure.runtime.texture.glsl.execute", 1);
    ANN(frame0);
    ANN(frame1);
    AT(dvz_frame_plan_upload(frame0, "tex.image.rgba", 0, 16, "image.rgba.0"));
    AT(dvz_frame_plan_render(frame0, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame0, "visual.image.0"));
    AT(dvz_frame_plan_copy(frame0, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame0, "buf.pick.readback", "request.pick.0"));

    AT(dvz_frame_plan_upload(frame1, "tex.image.rgba", 0, 16, "image.rgba.1"));
    AT(dvz_frame_plan_render(frame1, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame1, "visual.image.0"));
    AT(dvz_frame_plan_copy(frame1, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame1, "buf.pick.readback", "request.pick.1"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream0 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame0, &caps, &report, &emit_cfg);
    ANN(stream0);
    DvzDrp2CommandStream* stream1 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame1, &caps, &report, &emit_cfg);
    ANN(stream1);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream0);
    AT(result.ok);
    result = dvz_drp2_runtime_execute(runtime, stream1);
    AT(result.ok);

    uint8_t downloaded[4] = {0};
    uint64_t rb_id = dvz_frame_plan_emitter_object_id(emitter, "_rb");
    AT(rb_id != 0);
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, rb_id, 0, 4, downloaded));
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    dvz_drp2_runtime_destroy(runtime);
    dvz_drp2_stream_destroy(stream1);
    dvz_drp2_stream_destroy(stream0);
    dvz_frame_plan_destroy(frame1);
    dvz_frame_plan_destroy(frame0);
    dvz_frame_plan_emitter_destroy(emitter);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}


int test_frame_plan_emitter_runtime_compute_two_frames_glsl_executes(
    TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

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
            "scene compute runtime-mode DRP2 execution test skipped because GPU context creation "
            "failed");
        return 0;
    }

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);

    DvzFramePlan* frame0 = dvz_frame_plan("figure.runtime.compute.glsl.execute", 0);
    DvzFramePlan* frame1 = dvz_frame_plan("figure.runtime.compute.glsl.execute", 1);
    ANN(frame0);
    ANN(frame1);
    AT(dvz_frame_plan_upload(frame0, "buf.compute.input", 0, 36, "compute.input.0"));
    AT(dvz_frame_plan_compute(frame0, "copy_positions", 1, 1, 1));
    AT(dvz_frame_plan_compute_read(frame0, "buf.compute.input"));
    AT(dvz_frame_plan_compute_write(frame0, "buf.compute.output"));
    AT(dvz_frame_plan_render(frame0, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame0, "visual.compute.0"));
    AT(dvz_frame_plan_copy(frame0, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame0, "buf.pick.readback", "request.pick.0"));

    AT(dvz_frame_plan_upload(frame1, "buf.compute.input", 0, 36, "compute.input.1"));
    AT(dvz_frame_plan_compute(frame1, "copy_positions", 1, 1, 1));
    AT(dvz_frame_plan_compute_read(frame1, "buf.compute.input"));
    AT(dvz_frame_plan_compute_write(frame1, "buf.compute.output"));
    AT(dvz_frame_plan_render(frame1, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame1, "visual.compute.0"));
    AT(dvz_frame_plan_copy(frame1, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame1, "buf.pick.readback", "request.pick.1"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream0 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame0, &caps, &report, &emit_cfg);
    ANN(stream0);
    DvzDrp2CommandStream* stream1 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame1, &caps, &report, &emit_cfg);
    ANN(stream1);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream0);
    AT(result.ok);
    result = dvz_drp2_runtime_execute(runtime, stream1);
    AT(result.ok);

    uint8_t downloaded[4] = {0};
    uint64_t rb_id = dvz_frame_plan_emitter_object_id(emitter, "_rb");
    AT(rb_id != 0);
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, rb_id, 0, 4, downloaded));
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    dvz_drp2_runtime_destroy(runtime);
    dvz_drp2_stream_destroy(stream1);
    dvz_drp2_stream_destroy(stream0);
    dvz_frame_plan_destroy(frame1);
    dvz_frame_plan_destroy(frame0);
    dvz_frame_plan_emitter_destroy(emitter);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}


int test_scene_drp2_offscreen_canvas_frame(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan12Features features12 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    features12.timelineSemaphore = true;
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features12(&gpu_cfg, &features12);
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("scene offscreen canvas test skipped because GPU context creation failed");
        return 0;
    }

    DvzWindowHost* host = dvz_window_host();
    ANN(host);

    DvzWindowConfig window_cfg = dvz_window_default_config();
    window_cfg.title = "scene-drp2-offscreen-canvas";
    window_cfg.width = 64;
    window_cfg.height = 64;
    DvzWindow* window = dvz_window_create(host, DVZ_BACKEND_OFFSCREEN, &window_cfg);
    if (window == NULL || dvz_window_backend_type(window) != DVZ_BACKEND_OFFSCREEN)
    {
        log_warn("scene offscreen canvas test skipped because headless window creation failed");
        dvz_window_host_destroy(host);
        dvz_gpu_ctx_destroy(ctx);
        return 0;
    }

    DvzCanvasConfig canvas_cfg = dvz_canvas_default_config();
    canvas_cfg.window = window;
    canvas_cfg.device = dvz_gpu_ctx_device(ctx);
    canvas_cfg.render_mode = DVZ_CANVAS_RENDER_MODE_OFFSCREEN;
    canvas_cfg.timing_history = 2;
    DvzCanvas* canvas = dvz_canvas_create(&canvas_cfg);
    ANN(canvas);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);

    SceneCanvasDrawState state = {0};
    state.emitter = emitter;
    state.runtime = runtime;
    state.emit_cfg = dvz_frame_plan_emit_config();
    state.emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    state.emit_cfg.external_color_target = true;
    state.emit_cfg.color_target_id = 1;
    state.emit_cfg.fullscreen_triangle = true;
    dvz_capability_snapshot_default(&state.caps);

    dvz_canvas_set_draw_callback(canvas, _scene_canvas_drp2_draw, &state);
    AT(dvz_canvas_frame(canvas) == DVZ_CANVAS_FRAME_READY);
    AT(state.callback_count == 1);
    AT(state.attach_ok);
    AT(state.emit_ok);
    AT(state.direct_target_ok);
    AT(state.execute_ok);
    AT(dvz_canvas_submit(canvas) == 0);
    AT(dvz_canvas_offscreen_runtime_state(canvas) == DVZ_CANVAS_OFFSCREEN_STATE_READY);

    uint32_t width = 0;
    uint32_t height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
    ANN(rgba);
    AT(width == 64);
    AT(height == 64);
    uint32_t bright_count = 0;
    for (uint32_t i = 0; i < width * height; i++)
    {
        uint8_t* pixel = &rgba[4 * i];
        if (pixel[0] > 200 && pixel[1] > 200 && pixel[2] > 200 && pixel[3] > 200)
        {
            bright_count++;
        }
    }
    AT(bright_count > (width * height) / 2);
    dvz_free(rgba);
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    dvz_frame_plan_emitter_destroy(emitter);
    dvz_drp2_runtime_destroy(runtime);
    dvz_canvas_destroy(canvas);
    dvz_window_destroy(window);
    dvz_window_host_destroy(host);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}


int test_frame_plan_emit_drp2_readback(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.readback.convert", 11);
    ANN(plan);

    AT(dvz_frame_plan_upload(plan, "buf.point.position", 0, 16, "point.position"));
    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(plan, "visual.pickable.0"));
    AT(dvz_frame_plan_copy(plan, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(plan, "buf.pick.readback", "request.pick.0"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_frame_plan_emit_drp2(plan, &caps, &report);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);
    DvzDrp2ValidationResult validation = dvz_drp2_validate_stream(stream);
    AT(validation.ok);
    AT(dvz_drp2_stream_count(stream) == 18);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 4)) == DVZ_DRP2_COMMAND_CREATE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 15)) ==
       DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 17)) == DVZ_DRP2_COMMAND_QUEUE_SUBMIT);

    char* json = dvz_drp2_stream_json(stream, "scene_readback_from_c");
    ANN(json);
    AT(strstr(json, "\"cmd\": \"CopyTextureToBuffer\"") != NULL);
    AT(strstr(json, "\"usage\": [\"COPY_DST\", \"MAP_READ\"]") != NULL);
    AT(strstr(json, "\"readbacks\": [ { \"buffer_id\": 12, \"offset\": 0, \"size\": 4 } ]") !=
       NULL);

    dvz_drp2_stream_json_destroy(json);
    AT(_assert_stream_matches_fixture(
           stream, "scene_readback_from_c",
           "spec/drp2/fixtures/positive/scene_readback_from_c.json") == 0);
    dvz_drp2_stream_destroy(stream);
    dvz_frame_plan_destroy(plan);
    return 0;
}


int test_frame_plan_emit_drp2_dynamic_uploads(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.dynamic.convert", 12);
    ANN(plan);

    AT(dvz_frame_plan_upload(plan, "buf.dynamic.position", 0, 16, "point.position.update"));
    AT(dvz_frame_plan_upload(plan, "buf.dynamic.color", 0, 16, "point.color.update"));
    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(plan, "visual.dynamic.0"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_frame_plan_emit_drp2(plan, &caps, &report);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);
    DvzDrp2ValidationResult validation = dvz_drp2_validate_stream(stream);
    AT(validation.ok);
    AT(dvz_drp2_stream_count(stream) == 18);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 2)) == DVZ_DRP2_COMMAND_CREATE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 3)) == DVZ_DRP2_COMMAND_WRITE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 4)) == DVZ_DRP2_COMMAND_CREATE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 5)) == DVZ_DRP2_COMMAND_WRITE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 13)) ==
       DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 17)) == DVZ_DRP2_COMMAND_QUEUE_SUBMIT);

    char* json = dvz_drp2_stream_json(stream, "scene_dynamic_uploads_from_c");
    ANN(json);
    AT(strstr(json, "\"id\": 20") != NULL);
    AT(strstr(json, "\"id\": 21") != NULL);
    AT(strstr(json, "\"buffer_id\": 20") != NULL);
    AT(strstr(json, "\"buffer_id\": 21") != NULL);
    AT(strstr(json, "\"cmd\": \"Draw\"") != NULL);

    dvz_drp2_stream_json_destroy(json);
    AT(_assert_stream_matches_fixture(
           stream, "scene_dynamic_uploads_from_c",
           "spec/drp2/fixtures/positive/scene_dynamic_uploads_from_c.json") == 0);
    dvz_drp2_stream_destroy(stream);
    dvz_frame_plan_destroy(plan);
    return 0;
}


int test_frame_plan_emit_drp2_texture_sampling(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.texture.convert", 13);
    ANN(plan);

    AT(dvz_frame_plan_upload(plan, "tex.image.rgba", 0, 16, "image.rgba"));
    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(plan, "visual.image.0"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_frame_plan_emit_drp2(plan, &caps, &report);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);
    DvzDrp2ValidationResult validation = dvz_drp2_validate_stream(stream);
    AT(validation.ok);
    AT(dvz_drp2_stream_count(stream) == 19);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 2)) == DVZ_DRP2_COMMAND_CREATE_TEXTURE);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 3)) == DVZ_DRP2_COMMAND_WRITE_TEXTURE);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 4)) == DVZ_DRP2_COMMAND_CREATE_SAMPLER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 5)) ==
       DVZ_DRP2_COMMAND_CREATE_BIND_GROUP_LAYOUT);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 9)) ==
       DVZ_DRP2_COMMAND_CREATE_BIND_GROUP);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 14)) == DVZ_DRP2_COMMAND_SET_BIND_GROUP);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 18)) == DVZ_DRP2_COMMAND_QUEUE_SUBMIT);

    char* json = dvz_drp2_stream_json(stream, "scene_texture_sampling_from_c");
    ANN(json);
    AT(strstr(json, "\"cmd\": \"CreateSampler\"") != NULL);
    AT(strstr(json, "\"cmd\": \"CreateBindGroupLayout\"") != NULL);
    AT(strstr(json, "\"cmd\": \"SetBindGroup\"") != NULL);
    AT(strstr(json, "\"usage\": [\"COPY_DST\", \"TEXTURE_BINDING\"]") != NULL);

    dvz_drp2_stream_json_destroy(json);
    AT(_assert_stream_matches_fixture(
           stream, "scene_texture_sampling_from_c",
           "spec/drp2/fixtures/positive/scene_texture_sampling_from_c.json") == 0);
    dvz_drp2_stream_destroy(stream);
    dvz_frame_plan_destroy(plan);
    return 0;
}


int test_frame_plan_emit_drp2_compute_assisted(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.compute.convert", 14);
    ANN(plan);

    AT(dvz_frame_plan_upload(plan, "buf.compute.input", 0, 36, "compute.input"));
    AT(dvz_frame_plan_compute(plan, "copy_positions", 1, 1, 1));
    AT(dvz_frame_plan_compute_read(plan, "buf.compute.input"));
    AT(dvz_frame_plan_compute_write(plan, "buf.compute.output"));
    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(plan, "visual.compute.0"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_frame_plan_emit_drp2(plan, &caps, &report);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);
    DvzDrp2ValidationResult validation = dvz_drp2_validate_stream(stream);
    AT(validation.ok);
    AT(dvz_drp2_stream_count(stream) == 26);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 2)) == DVZ_DRP2_COMMAND_CREATE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 4)) == DVZ_DRP2_COMMAND_CREATE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 7)) ==
       DVZ_DRP2_COMMAND_CREATE_COMPUTE_PIPELINE);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 14)) ==
       DVZ_DRP2_COMMAND_BEGIN_COMPUTE_PASS);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 17)) ==
       DVZ_DRP2_COMMAND_DISPATCH_WORKGROUPS);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 24)) ==
       DVZ_DRP2_COMMAND_FINISH_COMMAND_ENCODER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 25)) == DVZ_DRP2_COMMAND_QUEUE_SUBMIT);

    char* json = dvz_drp2_stream_json(stream, "scene_compute_assisted_from_c");
    ANN(json);
    AT(strstr(json, "\"cmd\": \"BeginComputePass\"") != NULL);
    AT(strstr(json, "\"cmd\": \"DispatchWorkgroups\"") != NULL);
    AT(strstr(json, "\"usage\": [\"VERTEX\", \"STORAGE\"]") != NULL);
    AT(strstr(json, "\"binding_type\": \"storage_buffer\"") != NULL);
    AT(strstr(json, "\"binding\": 0, \"binding_type\": \"storage_buffer\", \"visibility\": "
                    "[\"COMPUTE\"], \"access\": \"read\"") != NULL);
    AT(strstr(json, "\"binding\": 1, \"binding_type\": \"storage_buffer\", \"visibility\": "
                    "[\"COMPUTE\"], \"access\": \"read_write\"") != NULL);

    dvz_drp2_stream_json_destroy(json);
    AT(_assert_stream_matches_fixture(
           stream, "scene_compute_assisted_from_c",
           "spec/drp2/fixtures/positive/scene_compute_assisted_from_c.json") == 0);
    dvz_drp2_stream_destroy(stream);
    dvz_frame_plan_destroy(plan);
    return 0;
}


int test_frame_plan_emitter_runtime_two_frames(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);

    DvzFramePlan* frame0 = dvz_frame_plan("figure.runtime", 0);
    DvzFramePlan* frame1 = dvz_frame_plan("figure.runtime", 1);
    ANN(frame0);
    ANN(frame1);
    AT(dvz_frame_plan_upload(frame0, "buf.point.position", 0, 16, "point.position.0"));
    AT(dvz_frame_plan_render(frame0, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame0, "visual.pickable.0"));
    AT(dvz_frame_plan_copy(frame0, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame0, "buf.pick.readback", "request.pick.0"));

    AT(dvz_frame_plan_upload(frame1, "buf.point.position", 0, 16, "point.position.1"));
    AT(dvz_frame_plan_render(frame1, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame1, "visual.pickable.0"));
    AT(dvz_frame_plan_copy(frame1, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame1, "buf.pick.readback", "request.pick.1"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream0 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame0, &caps, &report, &emit_cfg);
    ANN(stream0);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(dvz_drp2_stream_count(stream0) == 18);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 0)) ==
       DVZ_DRP2_COMMAND_HELLO_RENDERER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 2)) ==
       DVZ_DRP2_COMMAND_CREATE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 16)) ==
       DVZ_DRP2_COMMAND_FINISH_COMMAND_ENCODER);

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame1, &caps, &report, &emit_cfg);
    ANN(stream1);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(dvz_drp2_stream_count(stream1) == 10);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 0)) ==
       DVZ_DRP2_COMMAND_WRITE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 1)) ==
       DVZ_DRP2_COMMAND_BEGIN_COMMAND_ENCODER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 8)) ==
       DVZ_DRP2_COMMAND_FINISH_COMMAND_ENCODER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 9)) ==
       DVZ_DRP2_COMMAND_QUEUE_SUBMIT);

    DvzDrp2RuntimeConfig runtime_cfg = dvz_drp2_runtime_vklite_config(NULL, NULL);
    runtime_cfg.semantic_only = true;
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream0);
    AT(result.ok);
    result = dvz_drp2_runtime_execute(runtime, stream1);
    AT(result.ok);

    dvz_drp2_runtime_destroy(runtime);
    dvz_drp2_stream_destroy(stream1);
    dvz_drp2_stream_destroy(stream0);
    dvz_frame_plan_destroy(frame1);
    dvz_frame_plan_destroy(frame0);
    dvz_frame_plan_emitter_destroy(emitter);
    return 0;
}


int test_frame_plan_emitter_runtime_dynamic_two_frames(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);

    DvzFramePlan* frame0 = dvz_frame_plan("figure.runtime.dynamic", 0);
    DvzFramePlan* frame1 = dvz_frame_plan("figure.runtime.dynamic", 1);
    ANN(frame0);
    ANN(frame1);
    AT(dvz_frame_plan_upload(frame0, "buf.dynamic.position", 0, 16, "point.position.0"));
    AT(dvz_frame_plan_upload(frame0, "buf.dynamic.color", 0, 16, "point.color.0"));
    AT(dvz_frame_plan_render(frame0, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame0, "visual.dynamic.0"));
    AT(dvz_frame_plan_copy(frame0, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame0, "buf.pick.readback", "request.pick.0"));

    AT(dvz_frame_plan_upload(frame1, "buf.dynamic.position", 0, 16, "point.position.1"));
    AT(dvz_frame_plan_upload(frame1, "buf.dynamic.color", 0, 16, "point.color.1"));
    AT(dvz_frame_plan_render(frame1, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame1, "visual.dynamic.0"));
    AT(dvz_frame_plan_copy(frame1, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame1, "buf.pick.readback", "request.pick.1"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream0 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame0, &caps, &report, &emit_cfg);
    ANN(stream0);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(dvz_drp2_stream_count(stream0) == 21);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 2)) ==
       DVZ_DRP2_COMMAND_CREATE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 4)) ==
       DVZ_DRP2_COMMAND_CREATE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 14)) ==
       DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 15)) ==
       DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER);

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame1, &caps, &report, &emit_cfg);
    ANN(stream1);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(dvz_drp2_stream_count(stream1) == 12);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 0)) ==
       DVZ_DRP2_COMMAND_WRITE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 1)) ==
       DVZ_DRP2_COMMAND_WRITE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 5)) ==
       DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 6)) ==
       DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER);

    DvzDrp2RuntimeConfig runtime_cfg = dvz_drp2_runtime_vklite_config(NULL, NULL);
    runtime_cfg.semantic_only = true;
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream0);
    AT(result.ok);
    result = dvz_drp2_runtime_execute(runtime, stream1);
    AT(result.ok);

    dvz_drp2_runtime_destroy(runtime);
    dvz_drp2_stream_destroy(stream1);
    dvz_drp2_stream_destroy(stream0);
    dvz_frame_plan_destroy(frame1);
    dvz_frame_plan_destroy(frame0);
    dvz_frame_plan_emitter_destroy(emitter);
    return 0;
}


int test_frame_plan_emitter_runtime_dynamic_grow_buffer(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);

    DvzFramePlan* frame0 = dvz_frame_plan("figure.runtime.grow", 0);
    DvzFramePlan* frame1 = dvz_frame_plan("figure.runtime.grow", 1);
    ANN(frame0);
    ANN(frame1);

    AT(dvz_frame_plan_upload(frame0, "buf.grow.position", 0, 16, "point.position.0"));
    AT(dvz_frame_plan_render(frame0, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(frame0, "visual.grow.0"));

    AT(dvz_frame_plan_upload(frame1, "buf.grow.position", 0, 256, "point.position.1"));
    AT(dvz_frame_plan_render(frame1, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(frame1, "visual.grow.0"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream0 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame0, &caps, &report, &emit_cfg);
    ANN(stream0);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 2)) ==
       DVZ_DRP2_COMMAND_CREATE_BUFFER);

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame1, &caps, &report, &emit_cfg);
    ANN(stream1);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 0)) ==
       DVZ_DRP2_COMMAND_CREATE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 1)) ==
       DVZ_DRP2_COMMAND_WRITE_BUFFER);

    DvzDrp2RuntimeConfig runtime_cfg = dvz_drp2_runtime_vklite_config(NULL, NULL);
    runtime_cfg.semantic_only = true;
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream0);
    AT(result.ok);
    result = dvz_drp2_runtime_execute(runtime, stream1);
    AT(result.ok);

    dvz_drp2_runtime_destroy(runtime);
    dvz_drp2_stream_destroy(stream1);
    dvz_drp2_stream_destroy(stream0);
    dvz_frame_plan_destroy(frame1);
    dvz_frame_plan_destroy(frame0);
    dvz_frame_plan_emitter_destroy(emitter);
    return 0;
}


/**
 * Ensure runtime texture uploads recreate the logical texture when the full extent changes.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_frame_plan_emitter_runtime_texture_extent_changes(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);

    DvzFramePlan* frame0 = dvz_frame_plan("figure.runtime.texture.resize", 0);
    DvzFramePlan* frame1 = dvz_frame_plan("figure.runtime.texture.resize", 1);
    DvzFramePlan* frame2 = dvz_frame_plan("figure.runtime.texture.resize", 2);
    DvzFramePlan* frame3 = dvz_frame_plan("figure.runtime.texture.resize", 3);
    ANN(frame0);
    ANN(frame1);
    ANN(frame2);
    ANN(frame3);

    AT(dvz_frame_plan_upload(frame0, "tex.resize.rgba", 0, 16, "image.rgba.0"));
    AT(dvz_frame_plan_upload_set_texture_extent(frame0, 2, 2));
    AT(dvz_frame_plan_render(frame0, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame0, "visual.image.resize"));

    AT(dvz_frame_plan_upload(frame1, "tex.resize.rgba", 0, 16, "image.rgba.1"));
    AT(dvz_frame_plan_upload_set_texture_extent(frame1, 2, 2));
    AT(dvz_frame_plan_render(frame1, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame1, "visual.image.resize"));

    AT(dvz_frame_plan_upload(frame2, "tex.resize.rgba", 0, 64, "image.rgba.2"));
    AT(dvz_frame_plan_upload_set_texture_extent(frame2, 4, 4));
    AT(dvz_frame_plan_render(frame2, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame2, "visual.image.resize"));

    AT(dvz_frame_plan_upload(frame3, "tex.partial.rgba", 0, 4, "image.rgba.3"));
    AT(dvz_frame_plan_upload_set_texture_extent(frame3, 1, 1));
    AT(dvz_frame_plan_upload_set_texture_allocation_extent(frame3, 4, 4));
    AT(dvz_frame_plan_upload_set_texture_region(frame3, 3, 3));
    AT(dvz_frame_plan_render(frame3, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame3, "visual.image.partial"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream0 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame0, &caps, &report, &emit_cfg);
    ANN(stream0);
    AT(dvz_diagnostic_report_count(&report) == 0);

    uint64_t tex0 = 0;
    bool created_tex0 = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream0); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream0, i);
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE)
            tex0 = cmd->u.write_texture.texture_id;
    }
    AT(tex0 != 0);
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream0); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream0, i);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE && cmd->u.create_texture.id == tex0)
        {
            created_tex0 = true;
            AT(cmd->u.create_texture.width == 2);
            AT(cmd->u.create_texture.height == 2);
        }
    }
    AT(created_tex0);

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame1, &caps, &report, &emit_cfg);
    ANN(stream1);
    AT(dvz_diagnostic_report_count(&report) == 0);

    bool recreated_tex0 = false;
    bool wrote_tex0 = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream1); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream1, i);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE && cmd->u.create_texture.id == tex0)
            recreated_tex0 = true;
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE &&
            cmd->u.write_texture.texture_id == tex0)
        {
            wrote_tex0 = true;
        }
    }
    AT(!recreated_tex0);
    AT(wrote_tex0);

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream2 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame2, &caps, &report, &emit_cfg);
    ANN(stream2);
    AT(dvz_diagnostic_report_count(&report) == 0);

    uint64_t tex2 = 0;
    bool created_tex2 = false;
    bool wrote_tex2 = false;
    bool rebound_tex2 = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream2); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream2, i);
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE)
            tex2 = cmd->u.write_texture.texture_id;
    }
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream2); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream2, i);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE && cmd->u.create_texture.id == tex2)
        {
            created_tex2 = true;
            AT(cmd->u.create_texture.width == 4);
            AT(cmd->u.create_texture.height == 4);
        }
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE &&
            cmd->u.write_texture.texture_id == tex2)
        {
            wrote_tex2 = true;
        }
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_BIND_GROUP)
        {
            for (uint32_t j = 0; j < cmd->u.create_bind_group.entry_count; j++)
            {
                if (cmd->u.create_bind_group.entries[j].resource_id == tex2)
                    rebound_tex2 = true;
            }
        }
    }
    AT(tex2 != 0);
    AT(tex2 != tex0);
    AT(created_tex2);
    AT(wrote_tex2);
    AT(rebound_tex2);

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream3 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame3, &caps, &report, &emit_cfg);
    ANN(stream3);
    AT(dvz_diagnostic_report_count(&report) == 0);

    uint64_t tex3 = 0;
    bool created_tex3 = false;
    bool wrote_tex3_region = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream3); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream3, i);
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE)
            tex3 = cmd->u.write_texture.texture_id;
    }
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream3); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream3, i);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE && cmd->u.create_texture.id == tex3)
        {
            created_tex3 = true;
            AT(cmd->u.create_texture.width == 4);
            AT(cmd->u.create_texture.height == 4);
        }
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE &&
            cmd->u.write_texture.texture_id == tex3 &&
            cmd->u.write_texture.width == 1 &&
            cmd->u.write_texture.height == 1 &&
            cmd->u.write_texture.origin_x == 3 &&
            cmd->u.write_texture.origin_y == 3)
        {
            wrote_tex3_region = true;
        }
    }
    AT(tex3 != 0);
    AT(created_tex3);
    AT(wrote_tex3_region);

    DvzDrp2RuntimeConfig runtime_cfg = dvz_drp2_runtime_vklite_config(NULL, NULL);
    runtime_cfg.semantic_only = true;
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream0);
    AT(result.ok);
    result = dvz_drp2_runtime_execute(runtime, stream1);
    AT(result.ok);
    result = dvz_drp2_runtime_execute(runtime, stream2);
    AT(result.ok);
    result = dvz_drp2_runtime_execute(runtime, stream3);
    AT(result.ok);

    dvz_drp2_runtime_destroy(runtime);
    dvz_drp2_stream_destroy(stream3);
    dvz_drp2_stream_destroy(stream2);
    dvz_drp2_stream_destroy(stream1);
    dvz_drp2_stream_destroy(stream0);
    dvz_frame_plan_destroy(frame3);
    dvz_frame_plan_destroy(frame2);
    dvz_frame_plan_destroy(frame1);
    dvz_frame_plan_destroy(frame0);
    dvz_frame_plan_emitter_destroy(emitter);
    return 0;
}


/**
 * Ensure persistent emitter object ids can grow beyond the initial resource-map capacity.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_frame_plan_emitter_runtime_object_map_grows(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);

    uint32_t count = DRP2_MAX_FIXTURE_RESOURCES + 17;
    for (uint32_t i = 0; i < count; i++)
    {
        char key[DVZ_SCENE_LABEL_SIZE] = {0};
        dvz_snprintf(key, sizeof(key), "grow.object.%u", i);

        bool is_new = false;
        uint64_t id = _obj_id(emitter, key, &is_new);
        AT(id != 0);
        AT(is_new);
        AT(dvz_frame_plan_emitter_object_id(emitter, key) == id);
    }

    bool is_new = true;
    uint64_t id = _obj_id(emitter, "grow.object.3", &is_new);
    AT(id != 0);
    AT(!is_new);
    AT(dvz_frame_plan_emitter_object_id(emitter, "grow.object.3") == id);

    dvz_frame_plan_emitter_destroy(emitter);
    return 0;
}


int test_frame_plan_emitter_runtime_texture_two_frames(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);

    DvzFramePlan* frame0 = dvz_frame_plan("figure.runtime.texture", 0);
    DvzFramePlan* frame1 = dvz_frame_plan("figure.runtime.texture", 1);
    ANN(frame0);
    ANN(frame1);
    AT(dvz_frame_plan_upload(frame0, "tex.image.rgba", 0, 16, "image.rgba.0"));
    AT(dvz_frame_plan_render(frame0, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame0, "visual.image.0"));
    AT(dvz_frame_plan_copy(frame0, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame0, "buf.pick.readback", "request.pick.0"));

    AT(dvz_frame_plan_upload(frame1, "tex.image.rgba", 0, 16, "image.rgba.1"));
    AT(dvz_frame_plan_render(frame1, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame1, "visual.image.0"));
    AT(dvz_frame_plan_copy(frame1, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame1, "buf.pick.readback", "request.pick.1"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream0 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame0, &caps, &report, &emit_cfg);
    ANN(stream0);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(dvz_drp2_stream_count(stream0) == 21);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 2)) ==
       DVZ_DRP2_COMMAND_CREATE_TEXTURE);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 4)) ==
       DVZ_DRP2_COMMAND_CREATE_SAMPLER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 5)) ==
       DVZ_DRP2_COMMAND_CREATE_BIND_GROUP_LAYOUT);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 9)) ==
       DVZ_DRP2_COMMAND_CREATE_BIND_GROUP);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 15)) ==
       DVZ_DRP2_COMMAND_SET_BIND_GROUP);

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame1, &caps, &report, &emit_cfg);
    ANN(stream1);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(dvz_drp2_stream_count(stream1) == 10);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 0)) ==
       DVZ_DRP2_COMMAND_WRITE_TEXTURE);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 4)) ==
       DVZ_DRP2_COMMAND_SET_BIND_GROUP);

    DvzDrp2RuntimeConfig runtime_cfg = dvz_drp2_runtime_vklite_config(NULL, NULL);
    runtime_cfg.semantic_only = true;
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream0);
    AT(result.ok);
    result = dvz_drp2_runtime_execute(runtime, stream1);
    AT(result.ok);

    dvz_drp2_runtime_destroy(runtime);
    dvz_drp2_stream_destroy(stream1);
    dvz_drp2_stream_destroy(stream0);
    dvz_frame_plan_destroy(frame1);
    dvz_frame_plan_destroy(frame0);
    dvz_frame_plan_emitter_destroy(emitter);
    return 0;
}


int test_frame_plan_emitter_runtime_compute_two_frames(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);

    DvzFramePlan* frame0 = dvz_frame_plan("figure.runtime.compute", 0);
    DvzFramePlan* frame1 = dvz_frame_plan("figure.runtime.compute", 1);
    ANN(frame0);
    ANN(frame1);
    AT(dvz_frame_plan_upload(frame0, "buf.compute.input", 0, 36, "compute.input.0"));
    AT(dvz_frame_plan_compute(frame0, "copy_positions", 1, 1, 1));
    AT(dvz_frame_plan_compute_read(frame0, "buf.compute.input"));
    AT(dvz_frame_plan_compute_write(frame0, "buf.compute.output"));
    AT(dvz_frame_plan_render(frame0, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame0, "visual.compute.0"));
    AT(dvz_frame_plan_copy(frame0, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame0, "buf.pick.readback", "request.pick.0"));

    AT(dvz_frame_plan_upload(frame1, "buf.compute.input", 0, 36, "compute.input.1"));
    AT(dvz_frame_plan_compute(frame1, "copy_positions", 1, 1, 1));
    AT(dvz_frame_plan_compute_read(frame1, "buf.compute.input"));
    AT(dvz_frame_plan_compute_write(frame1, "buf.compute.output"));
    AT(dvz_frame_plan_render(frame1, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame1, "visual.compute.0"));
    AT(dvz_frame_plan_copy(frame1, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame1, "buf.pick.readback", "request.pick.1"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream0 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame0, &caps, &report, &emit_cfg);
    ANN(stream0);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(dvz_drp2_stream_count(stream0) == 28);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 2)) ==
       DVZ_DRP2_COMMAND_CREATE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 4)) ==
       DVZ_DRP2_COMMAND_CREATE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 7)) ==
       DVZ_DRP2_COMMAND_CREATE_COMPUTE_PIPELINE);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 15)) ==
       DVZ_DRP2_COMMAND_BEGIN_COMPUTE_PASS);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 18)) ==
       DVZ_DRP2_COMMAND_DISPATCH_WORKGROUPS);

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame1, &caps, &report, &emit_cfg);
    ANN(stream1);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(dvz_drp2_stream_count(stream1) == 15);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 0)) ==
       DVZ_DRP2_COMMAND_WRITE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 1)) ==
       DVZ_DRP2_COMMAND_BEGIN_COMMAND_ENCODER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 4)) ==
       DVZ_DRP2_COMMAND_SET_BIND_GROUP);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 5)) ==
       DVZ_DRP2_COMMAND_DISPATCH_WORKGROUPS);

    DvzDrp2RuntimeConfig runtime_cfg = dvz_drp2_runtime_vklite_config(NULL, NULL);
    runtime_cfg.semantic_only = true;
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream0);
    AT(result.ok);
    result = dvz_drp2_runtime_execute(runtime, stream1);
    AT(result.ok);

    dvz_drp2_runtime_destroy(runtime);
    dvz_drp2_stream_destroy(stream1);
    dvz_drp2_stream_destroy(stream0);
    dvz_frame_plan_destroy(frame1);
    dvz_frame_plan_destroy(frame0);
    dvz_frame_plan_emitter_destroy(emitter);
    return 0;
}


/**
 * Register scene frameplan emission tests.
 *
 * @param suite the active test suite
 * @return 0 on success
 */
int test_scene_frame_plan_emit(TstSuite* suite)
{
    ANN(suite);
    const char* tags = "scene";

    TEST_SIMPLE(test_frame_plan_emit_drp2_static_render);
    TEST_SIMPLE(test_frame_plan_emit_drp2_static_render_glsl);
    TEST_SIMPLE(test_frame_plan_emit_drp2_rejects_unsupported_shader_format);
    TEST_SIMPLE(test_frame_plan_emit_drp2_rejects_small_caps);
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    TEST_SIMPLE(test_frame_plan_emit_drp2_static_render_glsl_executes);
    TEST_SIMPLE(test_frame_plan_emit_drp2_readback_glsl_executes);
    TEST_SIMPLE(test_frame_plan_emitter_runtime_two_frames_glsl_executes);
    TEST_SIMPLE(test_frame_plan_emitter_runtime_dynamic_two_frames_glsl_executes);
    TEST_SIMPLE(test_frame_plan_emitter_runtime_texture_two_frames_glsl_executes);
    TEST_SIMPLE(test_frame_plan_emitter_runtime_compute_two_frames_glsl_executes);
    TEST_SIMPLE(test_scene_drp2_offscreen_canvas_frame);
#endif
    TEST_SIMPLE(test_frame_plan_emit_drp2_readback);
    TEST_SIMPLE(test_frame_plan_emit_drp2_dynamic_uploads);
    TEST_SIMPLE(test_frame_plan_emit_drp2_texture_sampling);
    TEST_SIMPLE(test_frame_plan_emit_drp2_compute_assisted);
    TEST_SIMPLE(test_frame_plan_emitter_runtime_two_frames);
    TEST_SIMPLE(test_frame_plan_emitter_runtime_dynamic_two_frames);
    TEST_SIMPLE(test_frame_plan_emitter_runtime_dynamic_grow_buffer);
    TEST_SIMPLE(test_frame_plan_emitter_runtime_texture_extent_changes);
    TEST_SIMPLE(test_frame_plan_emitter_runtime_object_map_grows);
    TEST_SIMPLE(test_frame_plan_emitter_runtime_texture_two_frames);
    TEST_SIMPLE(test_frame_plan_emitter_runtime_compute_two_frames);

    return 0;
}
