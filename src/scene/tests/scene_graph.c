/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene graph tests                                                                             */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <math.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "../_frame_plan.h"
#include "../_scene.h"
#include "../_scene_emit.h"
#include "../_technique.h"
#include "../_visual_pipeline.h"
#include "../../drp2/_stream.h"
#include "datoviz/drp2.h"
#include "datoviz/math/_cglm.h"
#include "datoviz/scene.h"
#include "datoviz/vklite/buffers.h"
#include "datoviz/vk/gpu_ctx.h"
#include "helpers.h"
#include "test_scene.h"
#include "testing.h"

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
bool _dvz_drp2_runtime_vklite_download_buffer(
    DvzDrp2Runtime* runtime, uint64_t buffer_id, uint64_t offset, uint64_t size, void* out);
#endif




/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return whether a command creates the scene common bind-group layout.
 *
 * @param cmd the command to inspect
 * @return whether the command creates the common MVP/viewport layout
 */
static bool _is_scene_common_bind_group_layout(const DvzDrp2Command* cmd)
{
    ANN(cmd);
    if (cmd->type != DVZ_DRP2_COMMAND_CREATE_BIND_GROUP_LAYOUT)
        return false;
    if (cmd->u.create_bind_group_layout.entry_count != 2)
        return false;
    return cmd->u.create_bind_group_layout.entries[0].binding == 0 &&
           cmd->u.create_bind_group_layout.entries[1].binding == 1 &&
           cmd->u.create_bind_group_layout.entries[0].binding_type ==
               DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER &&
           cmd->u.create_bind_group_layout.entries[1].binding_type ==
               DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER;
}



/**
 * Find the scene common bind-group layout id in an emitted stream.
 *
 * @param stream the emitted DRP2 stream
 * @return the common bind-group layout id, or zero when absent
 */
static uint64_t _stream_scene_common_layout_id(const DvzDrp2CommandStream* stream)
{
    ANN(stream);
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd != NULL && _is_scene_common_bind_group_layout(cmd))
            return cmd->u.create_bind_group_layout.id;
    }
    return 0;
}



/**
 * Find the layout id used by a bind group in an emitted stream.
 *
 * @param stream the emitted DRP2 stream
 * @param bind_group_id the bind group id
 * @return the bind group's layout id, or zero when absent
 */
static uint64_t
_stream_bind_group_layout_id(const DvzDrp2CommandStream* stream, uint64_t bind_group_id)
{
    ANN(stream);
    if (bind_group_id == 0)
        return 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd != NULL && cmd->type == DVZ_DRP2_COMMAND_CREATE_BIND_GROUP &&
            cmd->u.create_bind_group.id == bind_group_id)
        {
            return cmd->u.create_bind_group.bind_group_layout_id;
        }
    }
    return 0;
}



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_scene_point_emit_glsl_executes(TstSuite* suite, TstItem* item)
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
        log_warn("test_scene_point_emit_glsl_executes skipped: GPU context creation failed");
        return 0;
    }

    /* Build scene */
    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    AT(panel != NULL);
    DvzVisual* visual = dvz_point(scene, 0);
    AT(visual != NULL);

    float positions[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f,
    };
    DvzColor colors[3] = {{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}};
    float sizes[3]     = {10.0f, 20.0f, 15.0f};

    AT(dvz_visual_set_data(visual, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 3) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 3) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    /* Emit with GLSL */
    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);

    /* Execute on GPU */
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
    dvz_scene_destroy(scene);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



/**
 * Verify the scene point visual backend lowering decision.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_point_like_lowering_policy(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScenePointLikeLoweringDesc lowering = {0};
    AT(_scene_point_like_lowering_desc(
        DVZ_SCENE_POINT_LIKE_POINT, DVZ_SCENE_SHADER_FORMAT_GLSL, 3, &lowering));
    AT(lowering.kind == DVZ_SCENE_POINT_LIKE_POINT);
    AT(lowering.lowering == DVZ_SCENE_POINT_LIKE_LOWERING_NATIVE_POINTS);
    AT(lowering.topology == VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
    AT(lowering.vertex_step_mode == DVZ_DRP2_VERTEX_STEP_MODE_VERTEX);
    AT(lowering.draw_vertex_count == 3);
    AT(lowering.draw_instance_count == 1);

    AT(_scene_point_like_lowering_desc(
        DVZ_SCENE_POINT_LIKE_PIXEL, DVZ_SCENE_SHADER_FORMAT_WGSL, 3, &lowering));
    AT(lowering.kind == DVZ_SCENE_POINT_LIKE_PIXEL);
    AT(lowering.lowering == DVZ_SCENE_POINT_LIKE_LOWERING_INSTANCED_QUADS);
    AT(lowering.topology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    AT(lowering.vertex_step_mode == DVZ_DRP2_VERTEX_STEP_MODE_INSTANCE);
    AT(lowering.draw_vertex_count == 6);
    AT(lowering.draw_instance_count == 3);

    return 0;
}



/**
 * Verify GLSL point visuals keep native point-list draw semantics.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_point_emit_glsl_native_points(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    DvzVisual* visual = dvz_point(scene, 0);
    AT(visual != NULL);

    float positions[3][3] = {
        {-0.5f, -0.4f, 0.0f},
        { 0.0f,  0.4f, 0.0f},
        { 0.5f, -0.4f, 0.0f},
    };
    DvzColor colors[3] = {{255, 0, 0, 255}, {0, 180, 255, 255}, {255, 255, 255, 255}};
    float sizes[3] = {8.0f, 14.0f, 20.0f};

    AT(dvz_visual_set_data(visual, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 3) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 3) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);

    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    bool found_pipeline = false;
    bool found_draw = false;
    const uint32_t count = dvz_drp2_stream_count(stream);
    for (uint32_t i = 0; i < count; i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        ANN(command);
        if (command->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            found_pipeline = true;
            AT(command->u.create_render_pipeline.topology == VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
            AT(command->u.create_render_pipeline.binding_count == 3);
            AT(command->u.create_render_pipeline.binding_step_modes[0] ==
               DVZ_DRP2_VERTEX_STEP_MODE_VERTEX);
            AT(command->u.create_render_pipeline.binding_step_modes[1] ==
               DVZ_DRP2_VERTEX_STEP_MODE_VERTEX);
            AT(command->u.create_render_pipeline.binding_step_modes[2] ==
               DVZ_DRP2_VERTEX_STEP_MODE_VERTEX);
        }
        else if (command->type == DVZ_DRP2_COMMAND_DRAW)
        {
            found_draw = true;
            AT(command->u.draw.vertex_count == 3);
            AT(command->u.draw.instance_count == 1);
        }
    }
    AT(found_pipeline);
    AT(found_draw);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}



static int _scene_primitive_emit_executes(DvzPrimitiveTopology topology, uint32_t vertex_count)
{
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
        return 0;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    AT(panel != NULL);
    DvzVisual* visual = dvz_primitive(scene, topology, 0);
    AT(visual != NULL);

    /* Build vertex_count positions on a unit triangle / strip path; details don't matter. */
    float* positions = dvz_calloc(vertex_count * 3, sizeof(float));
    uint8_t (*colors)[4] = dvz_calloc(vertex_count, 4);
    for (uint32_t i = 0; i < vertex_count; i++)
    {
        positions[i * 3 + 0] = (float)i / (float)vertex_count - 0.5f;
        positions[i * 3 + 1] = (i % 2 == 0) ? -0.4f : 0.4f;
        positions[i * 3 + 2] = 0.0f;
        colors[i][0] = (uint8_t)(255 * i / vertex_count);
        colors[i][1] = 128;
        colors[i][2] = (uint8_t)(255 - 255 * i / vertex_count);
        colors[i][3] = 255;
    }
    AT(dvz_visual_set_data(visual, "position", positions, vertex_count) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, vertex_count) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);

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
    dvz_scene_destroy(scene);
    dvz_gpu_ctx_destroy(ctx);
    dvz_free(positions);
    dvz_free(colors);
    return 0;
}


static int _scene_path_emit_executes(uint32_t vertex_count)
{
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
        return 0;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    DvzVisual* visual = dvz_path(scene, 0);
    AT(visual != NULL);

    float* positions = dvz_calloc(vertex_count * 3, sizeof(float));
    uint8_t (*colors)[4] = dvz_calloc(vertex_count, 4);
    for (uint32_t i = 0; i < vertex_count; i++)
    {
        positions[i * 3 + 0] = (float)i / (float)vertex_count - 0.5f;
        positions[i * 3 + 1] = (i % 2 == 0) ? -0.4f : 0.4f;
        positions[i * 3 + 2] = 0.0f;
        colors[i][0] = 255;
        colors[i][1] = (uint8_t)(255 * i / vertex_count);
        colors[i][2] = 64;
        colors[i][3] = 255;
    }
    AT(dvz_visual_set_data(visual, "position", positions, vertex_count) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, vertex_count) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);

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
    dvz_scene_destroy(scene);
    dvz_gpu_ctx_destroy(ctx);
    dvz_free(positions);
    dvz_free(colors);
    return 0;
}


static int _scene_mesh_emit_executes(void)
{
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
        return 0;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    DvzVisual* visual = dvz_mesh(scene, 0);
    AT(visual != NULL);

    float positions[4][3] = {
        {-0.8f, -0.8f, 0.0f}, {-0.8f, 0.8f, 0.0f},
        {0.8f, -0.8f, 0.0f},  {0.8f, 0.8f, 0.0f},
    };
    float normals[4][3] = {
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
    };
    DvzIndex indices[6] = {0, 1, 2, 2, 1, 3};

    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)));

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "normal", normals, 4) == 0);
    AT(dvz_visual_set_buffer(visual, "index", index_buffer));
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);

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
    dvz_scene_destroy(scene);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}


int test_scene_primitive_triangle_list_glsl_executes(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;
    return _scene_primitive_emit_executes(DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 3);
}


int test_scene_point_emit_wgsl_instanced_quads(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    DvzVisual* visual = dvz_point(scene, 0);
    AT(visual != NULL);

    float positions[3][3] = {
        {-0.5f, -0.4f, 0.0f},
        { 0.0f,  0.4f, 0.0f},
        { 0.5f, -0.4f, 0.0f},
    };
    DvzColor colors[3] = {{255, 0, 0, 255}, {0, 180, 255, 255}, {255, 255, 255, 255}};
    float sizes[3] = {8.0f, 14.0f, 20.0f};

    AT(dvz_visual_set_data(visual, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 3) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 3) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_wgsl = true;
    caps.shader_format_glsl = false;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups = 4;
    caps.max_buffer_size = 256 * 1024 * 1024;

    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_WGSL;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    bool found_pipeline = false;
    bool found_draw = false;
    const uint32_t count = dvz_drp2_stream_count(stream);
    for (uint32_t i = 0; i < count; i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        ANN(command);
        if (command->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            found_pipeline = true;
            AT(command->u.create_render_pipeline.topology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
            AT(command->u.create_render_pipeline.binding_count == 3);
            AT(command->u.create_render_pipeline.binding_step_modes[0] ==
               DVZ_DRP2_VERTEX_STEP_MODE_INSTANCE);
            AT(command->u.create_render_pipeline.binding_step_modes[1] ==
               DVZ_DRP2_VERTEX_STEP_MODE_INSTANCE);
            AT(command->u.create_render_pipeline.binding_step_modes[2] ==
               DVZ_DRP2_VERTEX_STEP_MODE_INSTANCE);
        }
        else if (command->type == DVZ_DRP2_COMMAND_DRAW)
        {
            found_draw = true;
            AT(command->u.draw.vertex_count == 6);
            AT(command->u.draw.instance_count == 3);
        }
    }
    AT(found_pipeline);
    AT(found_draw);

    char* json = dvz_drp2_stream_json(stream, "scene_point_wgsl_from_c");
    ANN(json);
    AT(strstr(json, "\"format\": \"wgsl\"") != NULL);
    AT(strstr(json, "\"topology\": \"triangle-list\"") != NULL);
    AT(strstr(json, "\"step_mode\": \"instance\"") != NULL);
    AT(strstr(json, "\"vertex_count\": 6") != NULL);
    AT(strstr(json, "\"instance_count\": 3") != NULL);
    AT(strstr(json, "quad_corner") != NULL);
    dvz_drp2_stream_json_destroy(json);
    AT(_assert_stream_matches_fixture(
           stream, "scene_point_wgsl_from_c",
           "spec/drp2/fixtures/positive/scene_point_wgsl_from_c.json") == 0);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_pixel_emit_wgsl_instanced_quads(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    DvzVisual* visual = dvz_pixel(scene, 0);
    AT(visual != NULL);

    float positions[3][3] = {
        {-0.5f, -0.4f, 0.0f},
        { 0.0f,  0.4f, 0.0f},
        { 0.5f, -0.4f, 0.0f},
    };
    DvzColor colors[3] = {{255, 0, 0, 255}, {0, 180, 255, 255}, {255, 255, 255, 255}};
    float sizes[3] = {8.0f, 14.0f, 20.0f};

    AT(dvz_visual_set_data(visual, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 3) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 3) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_wgsl = true;
    caps.shader_format_glsl = false;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups = 4;
    caps.max_buffer_size = 256 * 1024 * 1024;

    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_WGSL;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    bool found_pipeline = false;
    bool found_draw = false;
    const uint32_t count = dvz_drp2_stream_count(stream);
    for (uint32_t i = 0; i < count; i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        ANN(command);
        if (command->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            found_pipeline = true;
            AT(command->u.create_render_pipeline.topology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
            AT(command->u.create_render_pipeline.binding_count == 3);
            AT(command->u.create_render_pipeline.binding_step_modes[0] ==
               DVZ_DRP2_VERTEX_STEP_MODE_INSTANCE);
            AT(command->u.create_render_pipeline.binding_step_modes[1] ==
               DVZ_DRP2_VERTEX_STEP_MODE_INSTANCE);
            AT(command->u.create_render_pipeline.binding_step_modes[2] ==
               DVZ_DRP2_VERTEX_STEP_MODE_INSTANCE);
        }
        else if (command->type == DVZ_DRP2_COMMAND_DRAW)
        {
            found_draw = true;
            AT(command->u.draw.vertex_count == 6);
            AT(command->u.draw.instance_count == 3);
        }
    }
    AT(found_pipeline);
    AT(found_draw);

    char* json = dvz_drp2_stream_json(stream, "scene_pixel_wgsl_from_c");
    ANN(json);
    AT(strstr(json, "\"format\": \"wgsl\"") != NULL);
    AT(strstr(json, "\"topology\": \"triangle-list\"") != NULL);
    AT(strstr(json, "\"step_mode\": \"instance\"") != NULL);
    AT(strstr(json, "\"vertex_count\": 6") != NULL);
    AT(strstr(json, "\"instance_count\": 3") != NULL);
    AT(strstr(json, "quad_corner") != NULL);
    AT(strstr(json, "dot(input.corner") == NULL);
    dvz_drp2_stream_json_destroy(json);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_primitive_line_strip_glsl_executes(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;
    return _scene_primitive_emit_executes(DVZ_PRIMITIVE_TOPOLOGY_LINE_STRIP, 4);
}


int test_scene_primitive_triangle_list_emit_wgsl(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    DvzVisual* visual = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    AT(visual != NULL);

    float positions[3][3] = {
        {-0.6f, -0.5f, 0.0f},
        { 0.6f, -0.5f, 0.0f},
        { 0.0f,  0.6f, 0.0f},
    };
    DvzColor colors[3] = {{255, 0, 0, 255}, {0, 180, 255, 255}, {255, 255, 255, 255}};

    AT(dvz_visual_set_data(visual, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 3) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_wgsl = true;
    caps.shader_format_glsl = false;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups = 4;
    caps.max_buffer_size = 256 * 1024 * 1024;

    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_WGSL;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    char* json = dvz_drp2_stream_json(stream, "scene_primitive_wgsl_from_c");
    ANN(json);
    AT(strstr(json, "\"format\": \"wgsl\"") != NULL);
    AT(strstr(json, "\"format\": \"glsl\"") == NULL);
    AT(strstr(json, "@vertex") != NULL);
    AT(strstr(json, "@fragment") != NULL);
    AT(strstr(json, "\"vertex_buffers\": [") != NULL);
    AT(strstr(json, "\"binding_type\": \"uniform_buffer\"") != NULL);
    AT(strstr(json, "VertexIn") != NULL);

    dvz_drp2_stream_json_destroy(json);
    AT(_assert_stream_matches_fixture(
           stream, "scene_primitive_wgsl_from_c",
           "spec/drp2/fixtures/positive/scene_primitive_wgsl_from_c.json") == 0);
    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_mesh_indexed_default_color_emits_draw_indexed(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_mesh(scene, 0);
    ANN(visual);

    float positions[4][3] = {
        {-0.8f, -0.8f, 0.0f}, {-0.8f, 0.8f, 0.0f},
        {0.8f, -0.8f, 0.0f},  {0.8f, 0.8f, 0.0f},
    };
    float normals[4][3] = {
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
    };
    DvzIndex indices[6] = {0, 1, 2, 2, 1, 3};

    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)));

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "normal", normals, 4) == 0);
    AT(dvz_visual_set_buffer(visual, "index", index_buffer));
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);
    AT(visual->mesh_default_color);
    bool found_color_attr = false;
    for (uint32_t i = 0; i < visual->attr_count; i++)
        found_color_attr = found_color_attr || strcmp(visual->attrs[i].name, "color") == 0;
    AT(found_color_attr);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    bool found_set_index = false;
    bool found_draw_indexed = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd->type == DVZ_DRP2_COMMAND_SET_INDEX_BUFFER)
            found_set_index = strcmp(cmd->u.set_index_buffer.index_format, "uint32") == 0;
        if (cmd->type == DVZ_DRP2_COMMAND_DRAW_INDEXED)
            found_draw_indexed = cmd->u.draw_indexed.index_count == 6;
    }
    AT(found_set_index);
    AT(found_draw_indexed);
    AT(_stream_set_vertex_buffer_count(stream) == 3);
    AT(_stream_write_buffer_range_count(stream, 0, sizeof(DvzSceneMaterialParams)) == 1);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Ensure lit mesh scene renders request depth attachments and depth-enabled pipelines.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */


int test_scene_mesh_emits_depth_attachment(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_mesh(scene, 0);
    ANN(visual);

    float positions[4][3] = {
        {-0.8f, -0.8f, 0.1f}, {-0.8f, 0.8f, 0.1f},
        {0.8f, -0.8f, 0.1f},  {0.8f, 0.8f, 0.1f},
    };
    float normals[4][3] = {
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
    };
    DvzIndex indices[6] = {0, 1, 2, 2, 1, 3};

    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)));

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "normal", normals, 4) == 0);
    AT(dvz_visual_set_buffer(visual, "index", index_buffer));
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    bool found_depth_pass = false;
    bool found_named_depth_pass = false;
    bool found_named_depth_texture = false;
    bool found_depth_pipeline = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE)
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.create_texture.id);
            found_named_depth_texture =
                found_named_depth_texture ||
                (label != NULL && strcmp(label, "fig0_p0.depth") == 0 &&
                 cmd->u.create_texture.format == VK_FORMAT_D32_SFLOAT);
        }
        if (cmd->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS)
        {
            found_depth_pass = found_depth_pass || cmd->u.begin_render_pass.has_depth_attachment;
            found_named_depth_pass =
                found_named_depth_pass || cmd->u.begin_render_pass.depth_texture_id != 0;
        }
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            found_depth_pipeline =
                found_depth_pipeline ||
                (cmd->u.create_render_pipeline.has_depth_attachment &&
                 cmd->u.create_render_pipeline.depth_write_enabled &&
                 cmd->u.create_render_pipeline.depth_compare_op == VK_COMPARE_OP_LESS_OR_EQUAL);
        }
    }
    AT(found_depth_pass);
    AT(!found_named_depth_pass);
    AT(!found_named_depth_texture);
    AT(found_depth_pipeline);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_indexed_primitive_emits_draw_indexed(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    ANN(visual);

    float positions[4][3] = {
        {-0.8f, -0.8f, 0.0f}, {-0.8f, 0.8f, 0.0f},
        {0.8f, -0.8f, 0.0f},  {0.8f, 0.8f, 0.0f},
    };
    DvzColor colors[4] = {
        {255, 0, 0, 255}, {255, 0, 0, 255}, {255, 0, 0, 255}, {255, 0, 0, 255},
    };
    float normals[4][3] = {
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
    };
    DvzIndex indices[6] = {0, 1, 2, 2, 1, 3};

    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)));

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 4) == 0);
    AT(dvz_visual_set_data(visual, "normal", normals, 4) == 0);
    AT(dvz_visual_set_buffer(visual, "index", index_buffer));
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    if (stream == NULL && dvz_diagnostic_report_count(&report) > 0)
        log_error("%s", dvz_diagnostic_report_get(&report, 0));
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    bool found_set_index = false;
    bool found_draw_indexed = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd->type == DVZ_DRP2_COMMAND_SET_INDEX_BUFFER)
            found_set_index = strcmp(cmd->u.set_index_buffer.index_format, "uint32") == 0;
        if (cmd->type == DVZ_DRP2_COMMAND_DRAW_INDEXED)
            found_draw_indexed = cmd->u.draw_indexed.index_count == 6;
    }
    AT(found_set_index);
    AT(found_draw_indexed);
    AT(_stream_set_vertex_buffer_count(stream) == 3);
    AT(_stream_write_buffer_range_count(stream, 0, sizeof(DvzSceneMaterialParams)) == 1);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_shared_index_buffer_emits_one_upload(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual0 = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    DvzVisual* visual1 = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    ANN(visual0);
    ANN(visual1);

    float positions0[4][3] = {
        {-0.8f, -0.8f, 0.0f}, {-0.8f, 0.0f, 0.0f},
        {-0.2f, -0.8f, 0.0f}, {-0.2f, 0.0f, 0.0f},
    };
    float positions1[4][3] = {
        {0.2f, 0.0f, 0.0f}, {0.2f, 0.8f, 0.0f},
        {0.8f, 0.0f, 0.0f}, {0.8f, 0.8f, 0.0f},
    };
    DvzColor colors0[4] = {
        {255, 0, 0, 255}, {255, 0, 0, 255}, {255, 0, 0, 255}, {255, 0, 0, 255},
    };
    DvzColor colors1[4] = {
        {0, 0, 255, 255}, {0, 0, 255, 255}, {0, 0, 255, 255}, {0, 0, 255, 255},
    };
    DvzIndex indices[6] = {0, 1, 2, 2, 1, 3};

    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)));

    AT(dvz_visual_set_data(visual0, "position", positions0, 4) == 0);
    AT(dvz_visual_set_data(visual0, "color", colors0, 4) == 0);
    AT(dvz_visual_set_data(visual1, "position", positions1, 4) == 0);
    AT(dvz_visual_set_data(visual1, "color", colors1, 4) == 0);
    AT(dvz_visual_set_buffer(visual0, "index", index_buffer));
    AT(dvz_visual_set_buffer(visual1, "index", index_buffer));
    AT(dvz_panel_add_visual(panel, visual0, NULL) == 0);
    AT(dvz_panel_add_visual(panel, visual1, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    AT(_stream_create_buffer_size_count(stream, sizeof(indices)) == 1);
    AT(_stream_write_buffer_range_count(stream, 0, sizeof(indices)) == 1);
    AT(_stream_set_index_buffer_count(stream) == 2);
    AT(_stream_draw_indexed_count(stream) == 2);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_mesh_glsl_executes(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;
    return _scene_mesh_emit_executes();
}


int test_scene_path_glsl_executes(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;
    return _scene_path_emit_executes(4);
}


int test_scene_image_glsl_executes(TstSuite* suite, TstItem* item)
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
        return 0;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    AT(panel != NULL);
    DvzVisual* visual = dvz_image(scene, 0);
    AT(visual != NULL);

    /* TRIANGLE_STRIP: TL, BL, TR, BR */
    float positions[4][3] = {
        {-0.5f, -0.5f, 0.0f},
        {-0.5f,  0.5f, 0.0f},
        { 0.5f, -0.5f, 0.0f},
        { 0.5f,  0.5f, 0.0f},
    };
    float texcoords[4][2] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };
    /* 4x4 RGBA8 checker pattern. */
    uint8_t pixels[4 * 4 * 4];
    for (uint32_t y = 0; y < 4; y++)
    {
        for (uint32_t x = 0; x < 4; x++)
        {
            uint32_t i = (y * 4 + x) * 4;
            uint8_t v = ((x ^ y) & 1) ? 255 : 0;
            pixels[i+0] = v; pixels[i+1] = v; pixels[i+2] = v; pixels[i+3] = 255;
        }
    }

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_texture(visual, pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);

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
    dvz_scene_destroy(scene);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}


int test_scene_json(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzScene*  scene  = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    DvzPanel*  panel  = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    DvzVisual* visual = dvz_point(scene, 0);

    float positions[] = {-0.5f, -0.5f, 0.0f, 0.5f, 0.5f, 0.0f};
    dvz_visual_set_data(visual, "position", positions, 2);
    dvz_panel_add_visual(panel, visual, NULL);

    char* json = dvz_scene_json(scene);
    AT(json != NULL);
    AT(strstr(json, "\"figures\"") != NULL);
    AT(strstr(json, "\"fig0\"") != NULL);
    AT(strstr(json, "\"point\"") != NULL);
    AT(strstr(json, "\"position\"") != NULL);
    AT(strstr(json, "\"item_count\":2") != NULL);
    AT(strstr(json, "\"data\":\"") != NULL); /* base64 data present */

    dvz_scene_json_destroy(json);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_json_includes_field_dirty_metadata(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);
    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);

    float positions[4][3] = {
        {-0.5f, -0.5f, 0.0f}, {-0.5f, 0.5f, 0.0f},
        { 0.5f, -0.5f, 0.0f}, { 0.5f, 0.5f, 0.0f},
    };
    float texcoords[4][2] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };
    AT(dvz_visual_set_data(image, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(image, "texcoords", texcoords, 4) == 0);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R8_UINT,
                   .semantic = DVZ_FIELD_SEMANTIC_LABEL,
                   .width = 4,
                   .height = 4,
                   .depth = 1,
               });
    ANN(field);
    uint8_t base[16] = {0};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = base, .bytes_per_row = 4, .rows_per_image = 4}));
    AT(dvz_visual_set_field(image, "field", field));
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = dvz_figure_emit(figure, &caps, &report);
    ANN(stream);
    dvz_drp2_stream_destroy(stream);

    uint8_t patch[2] = {1, 2};
    AT(dvz_sampled_field_update_region(
        field, (DvzFieldRegion){.x = 1, .y = 2, .z = 0, .width = 2, .height = 1, .depth = 1},
        &(DvzFieldDataView){.data = patch, .bytes_per_row = 2, .rows_per_image = 1}));

    char* json = dvz_scene_json(scene);
    ANN(json);
    AT(strstr(json, "\"dirty\":{\"pending\":true,\"full\":false,\"region\":{\"x\":1,\"y\":2,\"z\":0,\"width\":2,\"height\":1,\"depth\":1}}") != NULL);
    AT(strstr(json, "\"field_state\":{\"pending\":true,\"full\":false,\"region\":{\"x\":1,\"y\":2,\"z\":0,\"width\":2,\"height\":1,\"depth\":1}}") != NULL);
    dvz_scene_json_destroy(json);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_json_includes_buffer_binding_metadata(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);
    DvzVisual* visual = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    ANN(visual);

    float positions[3][3] = {
        {-0.5f, -0.5f, 0.0f},
        {0.5f, -0.5f, 0.0f},
        {0.0f, 0.5f, 0.0f},
    };
    DvzColor colors[3] = {
        {255, 0, 0, 255},
        {0, 255, 0, 255},
        {0, 0, 255, 255},
    };
    DvzIndex indices[3] = {0, 1, 2};

    DvzSceneBuffer* buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){.usage = DVZ_SCENE_BUFFER_USAGE_INDEX, .stride = sizeof(DvzIndex)});
    ANN(buffer);
    AT(dvz_scene_buffer_set_data(buffer, indices, sizeof(indices)));
    AT(dvz_visual_set_data(visual, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 3) == 0);
    AT(dvz_visual_set_buffer(visual, "index", buffer));
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    char* json = dvz_scene_json(scene);
    ANN(json);
    AT(strstr(json, "\"buffers\":[") != NULL);
    AT(strstr(json, "\"id\":\"b0\"") != NULL);
    AT(strstr(json, "\"usage\":2") != NULL);
    AT(strstr(json, "\"stride\":4") != NULL);
    AT(strstr(json, "\"byte_size\":12") != NULL);
    AT(strstr(json, "\"dirty\":{\"pending\":true}") != NULL);
    AT(strstr(json, "\"buffer\":{\"id\":\"b0\",\"slot\":\"index\"}") != NULL);
    dvz_scene_json_destroy(json);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_z_layer_orders_emit(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    /* Two point visuals on one panel: behind=3 verts (z=-1), front=5 verts (z=+1).
     * Add front first, behind second, so insertion order ≠ z order. After phase 1
     * both visuals draw inside one render pass; the behind visual (z=-1) must draw
     * before the front visual (z=+1). */
    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});

    float pos5[5 * 3] = {0};
    float pos3[3 * 3] = {0};
    DvzColor col[5] = {0};
    float sz[5] = {0};

    DvzVisual* v_front  = dvz_point(scene, 0);  /* z=+1, 5 verts */
    DvzVisual* v_behind = dvz_point(scene, 0);  /* z=-1, 3 verts */

    AT(dvz_visual_set_data(v_front, "position", pos5, 5) == 0);
    AT(dvz_visual_set_data(v_front, "color",    col,  5) == 0);
    AT(dvz_visual_set_data(v_front, "size",     sz,   5) == 0);
    AT(dvz_visual_set_data(v_behind, "position", pos3, 3) == 0);
    AT(dvz_visual_set_data(v_behind, "color",    col,  3) == 0);
    AT(dvz_visual_set_data(v_behind, "size",     sz,   3) == 0);

    AT(dvz_panel_add_visual(panel, v_front,  &(DvzVisualAttachDesc){.z_layer = +1}) == 0);
    AT(dvz_panel_add_visual(panel, v_behind, &(DvzVisualAttachDesc){.z_layer = -1}) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups    = 4;
    caps.max_buffer_size    = 256 * 1024 * 1024;

    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);

    char* json = dvz_drp2_stream_json(stream, "z_layer_order");
    ANN(json);

    /* Both draws appear in the single render pass (pass_id 10001).
     * The behind visual (3 verts, z=-1) must appear before the front visual (5 verts, z=+1). */
    const char* draw3 = strstr(json, "\"cmd\": \"Draw\", \"pass_id\": 10001, \"vertex_count\": 3");
    const char* draw5 = strstr(json, "\"cmd\": \"Draw\", \"pass_id\": 10001, \"vertex_count\": 5");
    AT(draw3 != NULL);
    AT(draw5 != NULL);
    AT(draw3 < draw5);  /* behind drawn first */

    dvz_drp2_stream_json_destroy(json);
    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_background_color_creates_fixed_quad(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});

    /* Initially no visuals. */
    AT(panel->visual_count == 0);
    AT(panel->background_visual == NULL);

    /* First call: creates a hidden background visual at z_layer=-1, FIXED. */
    dvz_panel_set_background_color(panel, 0.1f, 0.2f, 0.3f, 1.0f);
    AT(panel->visual_count == 1);
    ANN(panel->background_visual);
    AT(panel->visuals[0].visual == panel->background_visual);
    AT(panel->visuals[0].z_layer == -1);
    AT(panel->visuals[0].controller_mode == DVZ_CONTROLLER_FIXED);

    /* Second call with a different color: updates in place, no new visual. */
    DvzVisual* before = panel->background_visual;
    dvz_panel_set_background_color(panel, 0.9f, 0.8f, 0.7f, 1.0f);
    AT(panel->visual_count == 1);
    AT(panel->background_visual == before);

    /* A regular visual added afterwards has default attach (z=0, APPLY) and lands
     * in front of the background per stable z-sort. */
    float pos[3 * 3] = {0};
    DvzColor col[3]  = {0};
    float sz[3]      = {0};
    DvzVisual* v = dvz_point(scene, 0);
    AT(dvz_visual_set_data(v, "position", pos, 3) == 0);
    AT(dvz_visual_set_data(v, "color", col, 3) == 0);
    AT(dvz_visual_set_data(v, "size", sz, 3) == 0);
    AT(dvz_panel_add_visual(panel, v, NULL) == 0);
    AT(panel->visual_count == 2);
    AT(panel->visuals[1].z_layer == 0);
    AT(panel->visuals[1].controller_mode == DVZ_CONTROLLER_APPLY);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_controller_mode_fixed_emits_separate_mvp(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    /* One panel with a panzoom (APPLY) and a FIXED visual: the converter must allocate
     * two MVP UBOs, one per controller_mode. APPLY gets the panzoom MVP, FIXED gets
     * identity, and writes never overwrite each other. */
    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    /* Note: we don't actually run a panzoom here — the controller_mode flag alone
     * determines whether the converter writes identity or the controller MVP. */

    float pos[3 * 3] = {0};
    DvzColor col[3] = {0};
    float sz[3] = {0};

    DvzVisual* v_apply = dvz_point(scene, 0);
    DvzVisual* v_fixed = dvz_point(scene, 0);
    AT(dvz_visual_set_data(v_apply, "position", pos, 3) == 0);
    AT(dvz_visual_set_data(v_apply, "color", col, 3) == 0);
    AT(dvz_visual_set_data(v_apply, "size", sz, 3) == 0);
    AT(dvz_visual_set_data(v_fixed, "position", pos, 3) == 0);
    AT(dvz_visual_set_data(v_fixed, "color", col, 3) == 0);
    AT(dvz_visual_set_data(v_fixed, "size", sz, 3) == 0);

    AT(dvz_panel_add_visual(panel, v_apply, NULL) == 0);
    AT(dvz_panel_add_visual(panel, v_fixed,
                            &(DvzVisualAttachDesc){.controller_mode = DVZ_CONTROLLER_FIXED}) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups    = 4;
    caps.max_buffer_size    = 256 * 1024 * 1024;

    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    char* json = dvz_drp2_stream_json(stream, "controller_mode_test");
    ANN(json);

    /* Two distinct MVP UBOs (size 208 = sizeof(DvzMVP) + std140 padding) must be
     * created, one for APPLY and one for FIXED. The common set now also carries a
     * panel viewport uniform, so FIXED common bind groups are panel-scoped. */
    uint32_t mvp_buffers = 0;
    const char* p = json;
    while ((p = strstr(p, "\"size\": 208, \"usage\": [\"COPY_DST\"")) != NULL)
    {
        mvp_buffers++;
        p += 1;
    }
    AT(mvp_buffers == 2);

    dvz_drp2_stream_json_destroy(json);
    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_panel_one_pass_per_panel(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});

    float pos[3 * 3] = {0};
    DvzColor col[3]  = {0};
    float sz[3]      = {0};

    DvzVisual* v0 = dvz_point(scene, 0);
    DvzVisual* v1 = dvz_point(scene, 0);
    DvzVisual* v2 = dvz_point(scene, 0);
    AT(dvz_visual_set_data(v0, "position", pos, 3) == 0);
    AT(dvz_visual_set_data(v0, "color",    col, 3) == 0);
    AT(dvz_visual_set_data(v0, "size",     sz,  3) == 0);
    AT(dvz_visual_set_data(v1, "position", pos, 3) == 0);
    AT(dvz_visual_set_data(v1, "color",    col, 3) == 0);
    AT(dvz_visual_set_data(v1, "size",     sz,  3) == 0);
    AT(dvz_visual_set_data(v2, "position", pos, 3) == 0);
    AT(dvz_visual_set_data(v2, "color",    col, 3) == 0);
    AT(dvz_visual_set_data(v2, "size",     sz,  3) == 0);
    AT(dvz_panel_add_visual(panel, v0, NULL) == 0);
    AT(dvz_panel_add_visual(panel, v1, NULL) == 0);
    AT(dvz_panel_add_visual(panel, v2, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups    = 4;
    caps.max_buffer_size    = 256 * 1024 * 1024;

    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    /* Exactly one BeginRenderPass and three Draws in that pass. */
    uint32_t pass_count = 0, draw_count = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS)
            pass_count++;
        if (cmd->type == DVZ_DRP2_COMMAND_DRAW)
            draw_count++;
    }
    AT(pass_count == 1);
    AT(draw_count == 3);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_multi_panel_reuses_fixed_pipeline_and_bind_group_state(
    TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    DvzPanel* left = dvz_panel(figure, (DvzPanelDesc){0, 0, 0.5f, 1});
    DvzPanel* right = dvz_panel(figure, (DvzPanelDesc){0.5f, 0, 0.5f, 1});

    float pos_l[3] = {-0.5f, 0.0f, 0.0f};
    float pos_r[3] = {0.5f, 0.0f, 0.0f};
    DvzColor col = {255, 255, 255, 255};
    float sz = 6.0f;

    DvzVisual* vl = dvz_point(scene, 0);
    DvzVisual* vr = dvz_point(scene, 0);
    AT(dvz_visual_set_data(vl, "position", pos_l, 1) == 0);
    AT(dvz_visual_set_data(vl, "color", &col, 1) == 0);
    AT(dvz_visual_set_data(vl, "size", &sz, 1) == 0);
    AT(dvz_visual_set_data(vr, "position", pos_r, 1) == 0);
    AT(dvz_visual_set_data(vr, "color", &col, 1) == 0);
    AT(dvz_visual_set_data(vr, "size", &sz, 1) == 0);

    DvzVisualAttachDesc fixed = {.controller_mode = DVZ_CONTROLLER_FIXED};
    AT(dvz_panel_add_visual(left, vl, &fixed) == 0);
    AT(dvz_panel_add_visual(right, vr, &fixed) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups = 4;
    caps.max_buffer_size = 256 * 1024 * 1024;

    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    uint32_t pass_count = 0, draw_count = 0, pipeline_count = 0, bind_group_count = 0;
    uint32_t viewport_count = 0, scissor_count = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS)
            pass_count++;
        if (cmd->type == DVZ_DRP2_COMMAND_DRAW)
            draw_count++;
        if (cmd->type == DVZ_DRP2_COMMAND_SET_PIPELINE)
            pipeline_count++;
        if (cmd->type == DVZ_DRP2_COMMAND_SET_BIND_GROUP)
            bind_group_count++;
        if (cmd->type == DVZ_DRP2_COMMAND_SET_VIEWPORT)
            viewport_count++;
        if (cmd->type == DVZ_DRP2_COMMAND_SET_SCISSOR)
            scissor_count++;
    }
    AT(pass_count == 1);
    AT(draw_count == 2);
    AT(pipeline_count == 1);
    AT(bind_group_count == 2);
    AT(viewport_count == 2);
    AT(scissor_count == 2);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_multi_panel_glsl_emits_viewport_scissor_commands(
    TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 128, 64, 0);
    DvzPanel* left = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 0.5f, 1.0f});
    DvzPanel* right = dvz_panel(figure, (DvzPanelDesc){0.5f, 0.0f, 0.5f, 1.0f});

    float pos_l[3] = {-0.6f, 0.0f, 0.0f};
    float pos_r[3] = {0.6f, 0.0f, 0.0f};
    DvzColor col = {255, 255, 255, 255};
    float sz = 5.0f;

    DvzVisual* vl = dvz_point(scene, 0);
    DvzVisual* vr = dvz_point(scene, 0);
    AT(dvz_visual_set_data(vl, "position", pos_l, 1) == 0);
    AT(dvz_visual_set_data(vl, "color", &col, 1) == 0);
    AT(dvz_visual_set_data(vl, "size", &sz, 1) == 0);
    AT(dvz_visual_set_data(vr, "position", pos_r, 1) == 0);
    AT(dvz_visual_set_data(vr, "color", &col, 1) == 0);
    AT(dvz_visual_set_data(vr, "size", &sz, 1) == 0);
    AT(dvz_panel_add_visual(left, vl, NULL) == 0);
    AT(dvz_panel_add_visual(right, vr, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups = 4;
    caps.max_buffer_size = 256 * 1024 * 1024;

    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    uint32_t pass_count = 0, viewport_count = 0, scissor_count = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS)
        {
            pass_count++;
            AC(cmd->u.begin_render_pass.viewport[0], 0.0f, 1e-6f);
            AC(cmd->u.begin_render_pass.viewport[1], 0.0f, 1e-6f);
            AC(cmd->u.begin_render_pass.viewport[2], 1.0f, 1e-6f);
            AC(cmd->u.begin_render_pass.viewport[3], 1.0f, 1e-6f);
            AT(cmd->u.begin_render_pass.clear);
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_SET_VIEWPORT)
        {
            if (viewport_count == 0)
            {
                AC(cmd->u.set_viewport.viewport[0], 0.0f, 1e-6f);
                AC(cmd->u.set_viewport.viewport[1], 0.0f, 1e-6f);
                AC(cmd->u.set_viewport.viewport[2], 0.5f, 1e-6f);
                AC(cmd->u.set_viewport.viewport[3], 1.0f, 1e-6f);
            }
            else if (viewport_count == 1)
            {
                AC(cmd->u.set_viewport.viewport[0], 0.5f, 1e-6f);
                AC(cmd->u.set_viewport.viewport[1], 0.0f, 1e-6f);
                AC(cmd->u.set_viewport.viewport[2], 0.5f, 1e-6f);
                AC(cmd->u.set_viewport.viewport[3], 1.0f, 1e-6f);
            }
            viewport_count++;
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_SET_SCISSOR)
        {
            if (scissor_count == 0)
            {
                AC(cmd->u.set_scissor.scissor[0], 0.0f, 1e-6f);
                AC(cmd->u.set_scissor.scissor[1], 0.0f, 1e-6f);
                AC(cmd->u.set_scissor.scissor[2], 0.5f, 1e-6f);
                AC(cmd->u.set_scissor.scissor[3], 1.0f, 1e-6f);
            }
            else if (scissor_count == 1)
            {
                AC(cmd->u.set_scissor.scissor[0], 0.5f, 1e-6f);
                AC(cmd->u.set_scissor.scissor[1], 0.0f, 1e-6f);
                AC(cmd->u.set_scissor.scissor[2], 0.5f, 1e-6f);
                AC(cmd->u.set_scissor.scissor[3], 1.0f, 1e-6f);
            }
            scissor_count++;
        }
    }

    AT(pass_count == 1);
    AT(viewport_count == 2);
    AT(scissor_count == 2);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_rejects_cross_scene_visual(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene_a = dvz_scene();
    DvzScene* scene_b = dvz_scene();
    ANN(scene_a);
    ANN(scene_b);

    DvzFigure* figure = dvz_figure(scene_a, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);
    DvzVisual* foreign = dvz_point(scene_b, 0);
    ANN(foreign);

    AT(dvz_panel_add_visual(panel, foreign, NULL) == -1);

    dvz_scene_destroy(scene_b);
    dvz_scene_destroy(scene_a);
    return 0;
}


int test_scene_rejects_unsupported_point_attribute(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);

    float opacity[2] = {0.25f, 0.75f};
    tst_log_capture_begin(suite);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_data(visual, "opacity", opacity, 2) == -1);
    AT(_captured_log_contains(suite, "unsupported point visual attribute 'opacity'"));

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_visual_attr_source_and_mutability_metadata(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);

    AT(dvz_visual_attr_source(visual, "position") == DVZ_VISUAL_ATTR_SOURCE_PER_ITEM);
    AT(dvz_visual_attr_mutability(visual, "position") == DVZ_VISUAL_ATTR_MUTABILITY_DYNAMIC);

    AT(dvz_visual_set_attr_mutability(
           visual, "position", DVZ_VISUAL_ATTR_MUTABILITY_STREAMING) == 0);
    AT(dvz_visual_attr_mutability(visual, "position") ==
       DVZ_VISUAL_ATTR_MUTABILITY_STREAMING);

    AT(dvz_visual_set_attr_source(visual, "color", DVZ_VISUAL_ATTR_SOURCE_CONSTANT) == 0);
    AT(dvz_visual_attr_source(visual, "color") == DVZ_VISUAL_ATTR_SOURCE_CONSTANT);

    vec3 positions[2] = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
    AT(dvz_visual_set_data(visual, "position", positions, 2) == 0);
    tst_log_capture_begin(suite);
    AT_EXPECTED_ERROR_STRICT(
        suite,
        dvz_visual_set_attr_source(visual, "position", DVZ_VISUAL_ATTR_SOURCE_CONSTANT) == -1);
    AT(_captured_log_contains(suite, "does not accept source"));

    DvzColor colors[2] = {{255, 0, 0, 255}, {0, 255, 0, 255}};
    tst_log_capture_begin(suite);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_data(visual, "color", colors, 2) == -1);
    AT(_captured_log_contains(suite, "dense data requires PER_ITEM source"));

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_point_external_position_buffer_emits_no_upload(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);

    DvzSceneBufferDesc desc = {
        .usage = DVZ_SCENE_BUFFER_USAGE_VERTEX,
        .stride = sizeof(vec3),
        .byte_size = 3 * sizeof(vec3),
    };
    DvzSceneBuffer* position = dvz_scene_buffer(scene, &desc);
    ANN(position);
    AT(dvz_visual_set_attr_buffer(visual, "position", position, 0, 3));

    DvzColor colors[3] = {{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}};
    float sizes[3] = {4.0f, 5.0f, 6.0f};
    AT(dvz_visual_set_data(visual, "color", colors, 3) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 3) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);

    uint64_t position_buffer_id = 0;
    uint32_t create_count = 0;
    uint32_t write_count = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        ANN(cmd);
        if (cmd->type == DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER &&
            cmd->u.set_vertex_buffer.slot == 0)
        {
            position_buffer_id = cmd->u.set_vertex_buffer.buffer_id;
        }
    }
    AT(position_buffer_id != 0);

    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        ANN(cmd);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_BUFFER &&
            cmd->u.create_buffer.id == position_buffer_id)
        {
            create_count++;
        }
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_BUFFER &&
            cmd->u.write_buffer.buffer_id == position_buffer_id)
        {
            write_count++;
        }
    }
    AT(create_count == 0);
    AT(write_count == 0);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_point_external_position_buffer_executes(TstSuite* suite, TstItem* item)
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
            "test_scene_point_external_position_buffer_executes skipped: GPU context creation "
            "failed");
        return 0;
    }

    vec3 positions[3] = {
        {-0.5f, -0.5f, 0.0f},
        {+0.5f, -0.5f, 0.0f},
        { 0.0f, +0.5f, 0.0f},
    };
    uint64_t position_bytes = sizeof(positions);

    DvzBuffer* runtime_position = dvz_buffer_create_wrapper();
    ANN(runtime_position);
    dvz_buffer(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx), runtime_position);
    dvz_buffer_size(runtime_position, position_bytes);
    dvz_buffer_usage(runtime_position, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    dvz_buffer_flags(runtime_position, DVZ_ALLOC_HOST_ACCESS_SEQUENTIAL_WRITE);
    AT(dvz_buffer_create(runtime_position) == 0);
    dvz_buffer_upload(runtime_position, 0, position_bytes, positions);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);

    DvzSceneBufferDesc desc = {
        .usage = DVZ_SCENE_BUFFER_USAGE_VERTEX,
        .stride = sizeof(vec3),
        .byte_size = position_bytes,
    };
    DvzSceneBuffer* scene_position = dvz_scene_buffer(scene, &desc);
    ANN(scene_position);
    AT(dvz_visual_set_attr_buffer(visual, "position", scene_position, 0, 3));

    DvzColor colors[3] = {{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}};
    float sizes[3] = {8.0f, 8.0f, 8.0f};
    AT(dvz_visual_set_data(visual, "color", colors, 3) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 3) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    uint64_t position_buffer_id = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        ANN(cmd);
        if (cmd->type == DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER &&
            cmd->u.set_vertex_buffer.slot == 0)
        {
            position_buffer_id = cmd->u.set_vertex_buffer.buffer_id;
        }
    }
    AT(position_buffer_id != 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);
    DvzDrp2ExternalBufferDesc external = {
        .buffer = runtime_position,
        .size = position_bytes,
        .usage = DVZ_DRP2_BUFFER_USAGE_VERTEX,
    };
    AT(dvz_drp2_runtime_register_external_buffer(runtime, position_buffer_id, &external));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    dvz_drp2_runtime_destroy(runtime);
    dvz_drp2_stream_destroy(stream);
    dvz_buffer_destroy(runtime_position);
    dvz_buffer_free(runtime_position);
    dvz_scene_destroy(scene);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}


int test_scene_point_rejects_texcoords_attribute(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);

    float uv[2] = {0.0f, 0.0f};
    tst_log_capture_begin(suite);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_data(visual, "texcoords", uv, 1) == -1);
    AT(_captured_log_contains(suite, "unsupported point visual attribute 'texcoords'"));

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_primitive_rejects_size_attribute(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* visual = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    ANN(visual);

    float sz[1] = {10.0f};
    tst_log_capture_begin(suite);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_data(visual, "size", sz, 1) == -1);
    AT(_captured_log_contains(suite, "unsupported primitive visual attribute 'size'"));

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_path_rejects_size_attribute(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* visual = dvz_path(scene, 0);
    ANN(visual);

    float sz[1] = {10.0f};
    tst_log_capture_begin(suite);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_data(visual, "size", sz, 1) == -1);
    AT(_captured_log_contains(suite, "unsupported path visual attribute 'size'"));

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_image_rejects_size_attribute(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* visual = dvz_image(scene, 0);
    ANN(visual);

    float sz[1] = {10.0f};
    tst_log_capture_begin(suite);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_data(visual, "size", sz, 1) == -1);
    AT(_captured_log_contains(suite, "unsupported image visual attribute 'size'"));

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_emit_warns_visual_with_no_position(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    /* Emit with no position set — should warn but not crash. */
    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    tst_log_capture_begin(suite);
    DvzDrp2CommandStream* stream = dvz_figure_emit(figure, &caps, &report);
    AT(stream != NULL);
    AT(_captured_log_contains(suite, "has no 'position' data"));

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_rejects_mismatched_point_attribute_counts(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);

    float positions[2 * 3] = {
        -0.25f, 0.00f, 0.0f,
         0.25f, 0.00f, 0.0f,
    };
    DvzColor color = {255, 0, 0, 255};

    AT(dvz_visual_set_data(visual, "position", positions, 2) == 0);

    tst_log_capture_begin(suite);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_data(visual, "color", &color, 1) == -1);
    AT(_captured_log_contains(suite, "item_count 1 does not match existing attribute 'position'"));

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_point_visual_resizes_existing_attributes(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);

    float positions3[3 * 3] = {
        -0.50f, 0.00f, 0.0f,
         0.00f, 0.00f, 0.0f,
         0.50f, 0.00f, 0.0f,
    };
    DvzColor colors3[3] = {
        {255, 0, 0, 255},
        {0, 255, 0, 255},
        {0, 0, 255, 255},
    };
    float sizes3[3] = {3.0f, 4.0f, 5.0f};

    float positions2[2 * 3] = {
        -0.25f, 0.00f, 0.0f,
         0.25f, 0.00f, 0.0f,
    };
    DvzColor colors2[2] = {
        {255, 255, 0, 255},
        {0, 255, 255, 255},
    };
    float sizes2[2] = {6.0f, 7.0f};

    AT(dvz_visual_set_data(visual, "position", positions3, 3) == 0);
    AT(dvz_visual_set_data(visual, "color", colors3, 3) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes3, 3) == 0);

    DvzVisualDataUpdate partial[] = {
        {.attr_name = "position", .data = positions2, .item_count = 2},
    };
    tst_log_capture_begin(suite);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_data_many(visual, partial, 1) == -1);
    AT(_captured_log_contains(suite, "omits existing attribute 'color'"));

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions2, .item_count = 2},
        {.attr_name = "color", .data = colors2, .item_count = 2},
        {.attr_name = "size", .data = sizes2, .item_count = 2},
    };
    AT(dvz_visual_set_data_many(visual, updates, 3) == 0);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_rejects_range_update_without_full_allocation(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);

    float update[3] = {0.5f, 0.0f, 0.0f};

    tst_log_capture_begin(suite);
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_visual_set_data_range(visual, "position", update, 0, 1) == -1);
    AT(_captured_log_contains(suite, "range update requires prior full allocation"));

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_rejects_mutation_while_emitted_stream_is_live(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);

    float positions[2 * 3] = {-0.25f, 0.0f, 0.0f, 0.25f, 0.0f, 0.0f};
    DvzColor colors[2] = {{255, 0, 0, 255}, {0, 255, 0, 255}};
    float sizes[2] = {8.0f, 8.0f};
    AT(dvz_visual_set_data(visual, "position", positions, 2) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 2) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 2) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_wgsl = true;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = dvz_figure_emit(figure, &caps, &report);
    AT(stream != NULL);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(scene->outstanding_emitted_streams == 1);

    float update[2 * 3] = {-0.5f, 0.1f, 0.0f, 0.5f, 0.1f, 0.0f};
    tst_log_capture_begin(suite);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_data(visual, "position", update, 2) == -1);
    AT(_captured_log_contains(suite, "cannot mutate scene visual data while an emitted stream is still live"));

    dvz_drp2_stream_destroy(stream);
    AT(scene->outstanding_emitted_streams == 0);
    AT(dvz_visual_set_data(visual, "position", update, 2) == 0);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_rejects_scale_binding_while_emitted_stream_is_live(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);
    DvzScale* scale = dvz_scale(scene, NULL);
    ANN(scale);

    float positions[4][3] = {
        {-0.5f, -0.5f, 0.0f}, {-0.5f, 0.5f, 0.0f},
        {0.5f, -0.5f, 0.0f}, {0.5f, 0.5f, 0.0f},
    };
    float texcoords[4][2] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };
    uint8_t pixels[4 * 4 * 4] = {0};
    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_RGBA8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_COLOR,
                   .width = 4,
                   .height = 4,
                   .depth = 1,
               });
    ANN(field);

    AT(dvz_visual_set_data(image, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(image, "texcoords", texcoords, 4) == 0);
    AT(dvz_sampled_field_set_data(
           field, &(DvzFieldDataView){.data = pixels, .bytes_per_row = 16, .rows_per_image = 4}));
    AT(dvz_visual_set_field(image, "field", field));
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = dvz_figure_emit(figure, &caps, &report);
    AT(stream != NULL);

    tst_log_capture_begin(suite);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_scale(image, "colormap", scale) == -1);
    AT(_captured_log_contains(suite, "destroy the stream first"));

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_rejects_range_mutation_while_emitted_stream_is_live(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);

    float positions[4 * 3] = {-0.75f, 0.0f, 0.0f, -0.25f, 0.0f, 0.0f, 0.25f, 0.0f, 0.0f, 0.75f, 0.0f, 0.0f};
    DvzColor colors[4] = {{255, 0, 0, 255}, {255, 0, 0, 255}, {255, 0, 0, 255}, {255, 0, 0, 255}};
    float sizes[4] = {8.0f, 8.0f, 8.0f, 8.0f};
    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 4) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 4) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_wgsl = true;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = dvz_figure_emit(figure, &caps, &report);
    AT(stream != NULL);
    AT(scene->outstanding_emitted_streams == 1);

    float update[2 * 3] = {-0.1f, 0.2f, 0.0f, 0.1f, 0.2f, 0.0f};
    tst_log_capture_begin(suite);
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_visual_set_data_range(visual, "position", update, 1, 2) == -1);
    AT(_captured_log_contains(suite, "cannot mutate scene visual data while an emitted stream is still live"));

    dvz_drp2_stream_destroy(stream);
    AT(scene->outstanding_emitted_streams == 0);
    AT(dvz_visual_set_data_range(visual, "position", update, 1, 2) == 0);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_rejects_destroy_while_emitted_stream_is_live(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);

    float position[3] = {0.0f, 0.0f, 0.0f};
    DvzColor color = {255, 255, 0, 255};
    float size = 12.0f;
    AT(dvz_visual_set_data(visual, "position", position, 1) == 0);
    AT(dvz_visual_set_data(visual, "color", &color, 1) == 0);
    AT(dvz_visual_set_data(visual, "size", &size, 1) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_wgsl = true;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = dvz_figure_emit(figure, &caps, &report);
    AT(stream != NULL);
    AT(scene->outstanding_emitted_streams == 1);

    tst_log_capture_begin(suite);
    tst_expect_error_begin(suite);
    dvz_scene_destroy(scene);
    AT(tst_expect_error_end(suite) == 0);
    AT(_captured_log_contains(suite, "cannot destroy scene-owned visual data while an emitted stream is still live"));
    AT(scene->outstanding_emitted_streams == 1);

    dvz_drp2_stream_destroy(stream);
    AT(scene->outstanding_emitted_streams == 0);
    dvz_scene_destroy(scene);
    return 0;
}


static int
test_scene_rejects_visual_destroy_while_emitted_stream_is_live(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);

    float positions[2 * 3] = {-0.25f, 0.0f, 0.0f, 0.25f, 0.0f, 0.0f};
    DvzColor colors[2] = {{255, 0, 0, 255}, {0, 255, 0, 255}};
    float sizes[2] = {8.0f, 8.0f};
    AT(dvz_visual_set_data(visual, "position", positions, 2) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 2) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 2) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);
    AT(visual->scene == scene);
    AT(visual->attr_count == 3);
    for (uint32_t i = 0; i < visual->attr_count; i++)
        AT(visual->attrs[i].data != NULL);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_wgsl = true;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = dvz_figure_emit(figure, &caps, &report);
    AT(stream != NULL);
    AT(scene->outstanding_emitted_streams == 1);

    tst_log_capture_begin(suite);
    tst_expect_error_begin(suite);
    dvz_visual_destroy(visual);
    AT(tst_expect_error_end(suite) == 0);
    AT(_captured_log_contains(suite, "cannot destroy scene-owned visual data while an emitted stream is still live"));
    AT(scene->outstanding_emitted_streams == 1);
    AT(visual->scene == scene);
    AT(visual->attr_count == 3);
    for (uint32_t i = 0; i < visual->attr_count; i++)
        AT(visual->attrs[i].data != NULL);

    dvz_drp2_stream_destroy(stream);
    AT(scene->outstanding_emitted_streams == 0);

    dvz_visual_destroy(visual);
    AT(visual->scene == NULL);
    AT(visual->attr_count == 0);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_live_stream_count_tracks_multiple_emits(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);

    float positions[2 * 3] = {-0.25f, 0.0f, 0.0f, 0.25f, 0.0f, 0.0f};
    DvzColor colors[2] = {{255, 0, 0, 255}, {0, 255, 0, 255}};
    float sizes[2] = {8.0f, 8.0f};
    AT(dvz_visual_set_data(visual, "position", positions, 2) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 2) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 2) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_wgsl = true;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 = dvz_figure_emit(figure, &caps, &report);
    AT(stream1 != NULL);
    AT(scene->outstanding_emitted_streams == 1);

    float update[2] = {9.0f, 10.0f};
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_data_range(visual, "size", update, 0, 2) == -1);

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream2 = dvz_figure_emit(figure, &caps, &report);
    AT(stream2 != NULL);
    AT(scene->outstanding_emitted_streams == 2);

    dvz_drp2_stream_destroy(stream1);
    AT(scene->outstanding_emitted_streams == 1);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_data_range(visual, "size", update, 0, 2) == -1);

    dvz_drp2_stream_destroy(stream2);
    AT(scene->outstanding_emitted_streams == 0);
    AT(dvz_visual_set_data_range(visual, "size", update, 0, 2) == 0);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_point_emit(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    /* Build a minimal scene: one figure, one full-frame panel, one point visual. */
    DvzScene* scene = dvz_scene();
    AT(scene != NULL);

    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    AT(figure != NULL);

    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    AT(panel != NULL);

    DvzVisual* visual = dvz_point(scene, 0);
    AT(visual != NULL);

    float positions[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f,
    };
    DvzColor colors[3] = {{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}};
    float sizes[3]     = {10.0f, 20.0f, 15.0f};

    int rc = dvz_visual_set_data(visual, "position", positions, 3);
    AT(rc == 0);
    rc = dvz_visual_set_data(visual, "color", colors, 3);
    AT(rc == 0);
    rc = dvz_visual_set_data(visual, "size", sizes, 3);
    AT(rc == 0);

    rc = dvz_panel_add_visual(panel, visual, NULL);
    AT(rc == 0);

    /* Emit the DRP2 command stream. */
    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_wgsl = true;
    caps.max_vertex_buffers = 8;
    caps.max_bind_groups    = 4;
    caps.max_buffer_size    = 256 * 1024 * 1024;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_figure_emit(figure, &caps, &report);

    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);
    AT(dvz_drp2_stream_count(stream) > 0);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_path_emit(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    DvzVisual* visual = dvz_path(scene, 0);
    AT(visual != NULL);

    float positions[4][3] = {
        {-0.75f, -0.25f, 0.0f},
        {-0.25f, 0.25f, 0.0f},
        {0.25f, -0.25f, 0.0f},
        {0.75f, 0.25f, 0.0f},
    };
    DvzColor colors[4] = {
        {255, 0, 0, 255},
        {255, 255, 0, 255},
        {0, 255, 255, 255},
        {0, 128, 255, 255},
    };

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 4) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);

    bool found_pipeline = false;
    uint32_t count = dvz_drp2_stream_count(stream);
    for (uint32_t i = 0; i < count; i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd != NULL && cmd->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            found_pipeline = true;
            AT(cmd->u.create_render_pipeline.topology == VK_PRIMITIVE_TOPOLOGY_LINE_STRIP);
            AT(cmd->u.create_render_pipeline.binding_count == 2);
            AT(cmd->u.create_render_pipeline.attr_count == 2);
            break;
        }
    }
    AT(found_pipeline);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_image_emit(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    AT(panel != NULL);
    DvzVisual* visual = dvz_image(scene, 0);
    AT(visual != NULL);

    float positions[4][3] = {
        {-0.5f, -0.5f, 0.0f}, {-0.5f, 0.5f, 0.0f},
        { 0.5f, -0.5f, 0.0f}, { 0.5f, 0.5f, 0.0f},
    };
    float texcoords[4][2] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };
    uint8_t pixels[4 * 4 * 4];
    dvz_memset(pixels, sizeof(pixels), 128, sizeof(pixels));

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_texture(visual, pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_figure_emit(figure, &caps, &report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);
    AT(dvz_drp2_stream_count(stream) > 0);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_image_emit_wgsl(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    DvzVisual* visual = dvz_image(scene, 0);
    AT(visual != NULL);

    float positions[4][3] = {
        {-0.5f, -0.5f, 0.0f}, {-0.5f, 0.5f, 0.0f},
        { 0.5f, -0.5f, 0.0f}, { 0.5f, 0.5f, 0.0f},
    };
    float texcoords[4][2] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };
    uint8_t pixels[4 * 4 * 4];
    dvz_memset(pixels, sizeof(pixels), 128, sizeof(pixels));

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_texture(visual, pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_wgsl = true;
    caps.shader_format_glsl = false;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups = 4;
    caps.max_buffer_size = 256 * 1024 * 1024;

    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_WGSL;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    char* json = dvz_drp2_stream_json(stream, "scene_image_wgsl_from_c");
    ANN(json);
    AT(strstr(json, "\"format\": \"wgsl\"") != NULL);
    AT(strstr(json, "\"format\": \"glsl\"") == NULL);
    AT(strstr(json, "texture_2d<f32>") != NULL);
    AT(strstr(json, "textureSample") != NULL);
    AT(strstr(json, "@group(1) @binding(0)") != NULL);
    AT(strstr(json, "@group(1) @binding(1)") != NULL);
    AT(strstr(json, "\"bind_group_layout_ids\": [") != NULL);
    AT(strstr(json, "\"vertex_buffers\": [") != NULL);

    dvz_drp2_stream_json_destroy(json);
    AT(_assert_stream_matches_fixture(
           stream, "scene_image_wgsl_from_c",
           "spec/drp2/fixtures/positive/scene_image_wgsl_from_c.json") == 0);
    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_image_emit_uses_common_and_texture_sets(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    DvzVisual* visual = dvz_image(scene, 0);
    AT(visual != NULL);

    float positions[4][3] = {
        {-0.5f, -0.5f, 0.0f}, {-0.5f, 0.5f, 0.0f},
        { 0.5f, -0.5f, 0.0f}, { 0.5f, 0.5f, 0.0f},
    };
    float texcoords[4][2] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };
    uint8_t pixels[4 * 4 * 4];
    dvz_memset(pixels, sizeof(pixels), 128, sizeof(pixels));

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_texture(visual, pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups = 4;
    caps.max_buffer_size = 256 * 1024 * 1024;

    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    bool found_pipeline = false;
    bool found_common_layout = false;
    bool found_common_bind = false;
    bool found_viewport_write = false;
    bool found_texture_bind = false;
    uint32_t count = dvz_drp2_stream_count(stream);
    for (uint32_t i = 0; i < count; i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd == NULL)
            continue;
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            found_pipeline = true;
            AT(cmd->u.create_render_pipeline.bind_group_layout_count >= 2);
            AT(cmd->u.create_render_pipeline.bind_group_layout_ids[0] != 0);
            AT(cmd->u.create_render_pipeline.bind_group_layout_ids[1] != 0);
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_CREATE_BIND_GROUP_LAYOUT)
        {
            if (cmd->u.create_bind_group_layout.entry_count == 2 &&
                cmd->u.create_bind_group_layout.entries[0].binding == 0 &&
                cmd->u.create_bind_group_layout.entries[1].binding == 1 &&
                cmd->u.create_bind_group_layout.entries[0].binding_type ==
                    DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER &&
                cmd->u.create_bind_group_layout.entries[1].binding_type ==
                    DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER)
            {
                found_common_layout = true;
            }
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_SET_BIND_GROUP)
        {
            if (cmd->u.set_bind_group.slot == 0)
                found_common_bind = true;
            if (cmd->u.set_bind_group.slot == 1)
                found_texture_bind = true;
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_WRITE_BUFFER &&
                 cmd->u.write_buffer.size == sizeof(DvzSceneViewportUniform))
        {
            const DvzSceneViewportUniform* viewport =
                (const DvzSceneViewportUniform*)cmd->u.write_buffer.data_raw;
            ANN(viewport);
            AC(viewport->x, 0.0f, 1e-6f);
            AC(viewport->y, 0.0f, 1e-6f);
            AC(viewport->width, 64.0f, 1e-6f);
            AC(viewport->height, 64.0f, 1e-6f);
            found_viewport_write = true;
        }
    }
    AT(found_pipeline);
    AT(found_common_layout);
    AT(found_common_bind);
    AT(found_viewport_write);
    AT(found_texture_bind);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure scene visuals keep shared data in set 0 and visual-specific data in set 1.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_scene_visual_common_binding_layout_order(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);

    float point_pos[3][3] = {
        {-0.8f, -0.8f, 0.0f}, {-0.7f, -0.8f, 0.0f}, {-0.75f, -0.7f, 0.0f},
    };
    DvzColor point_color[3] = {
        {255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255},
    };
    float point_size[3] = {8.0f, 8.0f, 8.0f};
    DvzVisual* point = dvz_point(scene, 0);
    ANN(point);
    AT(dvz_visual_set_data(point, "position", point_pos, 3) == 0);
    AT(dvz_visual_set_data(point, "color", point_color, 3) == 0);
    AT(dvz_visual_set_data(point, "size", point_size, 3) == 0);
    AT(dvz_panel_add_visual(panel, point, NULL) == 0);

    float prim_pos[3][3] = {
        {-0.5f, -0.8f, 0.0f}, {-0.3f, -0.8f, 0.0f}, {-0.4f, -0.6f, 0.0f},
    };
    DvzColor prim_color[3] = {
        {255, 255, 0, 255}, {255, 255, 0, 255}, {255, 255, 0, 255},
    };
    DvzVisual* primitive = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    ANN(primitive);
    AT(dvz_visual_set_data(primitive, "position", prim_pos, 3) == 0);
    AT(dvz_visual_set_data(primitive, "color", prim_color, 3) == 0);
    AT(dvz_panel_add_visual(panel, primitive, NULL) == 0);

    float path_pos[4][3] = {
        {-0.2f, -0.8f, 0.0f}, {-0.1f, -0.7f, 0.0f},
        {0.0f, -0.8f, 0.0f},  {0.1f, -0.7f, 0.0f},
    };
    DvzColor path_color[4] = {
        {0, 255, 255, 255}, {0, 255, 255, 255},
        {0, 255, 255, 255}, {0, 255, 255, 255},
    };
    DvzVisual* path = dvz_path(scene, 0);
    ANN(path);
    AT(dvz_visual_set_data(path, "position", path_pos, 4) == 0);
    AT(dvz_visual_set_data(path, "color", path_color, 4) == 0);
    AT(dvz_panel_add_visual(panel, path, NULL) == 0);

    float image_pos[4][3] = {
        {0.2f, -0.8f, 0.0f}, {0.2f, -0.6f, 0.0f},
        {0.4f, -0.8f, 0.0f}, {0.4f, -0.6f, 0.0f},
    };
    float image_uv[4][2] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };
    uint8_t pixels[4 * 4 * 4];
    dvz_memset(pixels, sizeof(pixels), 255, sizeof(pixels));
    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);
    AT(dvz_visual_set_data(image, "position", image_pos, 4) == 0);
    AT(dvz_visual_set_data(image, "texcoords", image_uv, 4) == 0);
    AT(dvz_visual_set_texture(image, pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);

    float mesh_pos[4][3] = {
        {0.55f, -0.8f, 0.0f}, {0.55f, -0.6f, 0.0f},
        {0.75f, -0.8f, 0.0f}, {0.75f, -0.6f, 0.0f},
    };
    float mesh_normal[4][3] = {
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
    };
    DvzIndex mesh_index[6] = {0, 1, 2, 2, 1, 3};
    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, mesh_index, sizeof(mesh_index)));
    DvzVisual* mesh = dvz_mesh(scene, 0);
    ANN(mesh);
    AT(dvz_visual_set_data(mesh, "position", mesh_pos, 4) == 0);
    AT(dvz_visual_set_data(mesh, "normal", mesh_normal, 4) == 0);
    AT(dvz_visual_set_buffer(mesh, "index", index_buffer));
    AT(dvz_panel_add_visual(panel, mesh, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups = 4;
    caps.max_buffer_size = 256 * 1024 * 1024;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    uint64_t common_layout_id = _stream_scene_common_layout_id(stream);
    AT(common_layout_id != 0);

    bool found_point_pipeline = false;
    bool found_primitive_pipeline = false;
    bool found_path_pipeline = false;
    bool found_image_pipeline = false;
    bool found_lit_mesh_pipeline = false;
    bool found_common_bind = false;
    bool found_visual_bind = false;

    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd == NULL)
            continue;
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            const uint32_t topology = cmd->u.create_render_pipeline.topology;
            const uint32_t slots = cmd->u.create_render_pipeline.vertex_buffer_slots;
            const uint32_t layout_count = cmd->u.create_render_pipeline.bind_group_layout_count;
            AT(layout_count >= 1);
            AT(cmd->u.create_render_pipeline.bind_group_layout_ids[0] == common_layout_id);

            if (slots == 3 && topology == VK_PRIMITIVE_TOPOLOGY_POINT_LIST)
            {
                AT(layout_count == 1);
                found_point_pipeline = true;
            }
            else if (slots == 2 && topology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            {
                AT(layout_count == 1);
                found_primitive_pipeline = true;
            }
            else if (slots == 2 && topology == VK_PRIMITIVE_TOPOLOGY_LINE_STRIP)
            {
                AT(layout_count == 1);
                found_path_pipeline = true;
            }
            else if (slots == 2 && topology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP)
            {
                AT(layout_count == 2);
                AT(cmd->u.create_render_pipeline.bind_group_layout_ids[1] != common_layout_id);
                found_image_pipeline = true;
            }
            else if (slots == 3 && topology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            {
                AT(layout_count == 2);
                AT(cmd->u.create_render_pipeline.bind_group_layout_ids[1] != common_layout_id);
                found_lit_mesh_pipeline = true;
            }
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_SET_BIND_GROUP)
        {
            uint64_t layout_id =
                _stream_bind_group_layout_id(stream, cmd->u.set_bind_group.bind_group_id);
            if (cmd->u.set_bind_group.slot == 0)
            {
                AT(layout_id == common_layout_id);
                found_common_bind = true;
            }
            else if (cmd->u.set_bind_group.slot == 1)
            {
                AT(layout_id != 0);
                AT(layout_id != common_layout_id);
                found_visual_bind = true;
            }
        }
    }

    AT(found_point_pipeline);
    AT(found_primitive_pipeline);
    AT(found_path_pipeline);
    AT(found_image_pipeline);
    AT(found_lit_mesh_pipeline);
    AT(found_common_bind);
    AT(found_visual_bind);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Check that an empty panel emits an explicit clear-only render pass.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */


int test_scene_empty_figure_emit_clear_only(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    AT(panel != NULL);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.clear_color[0] = 0.05f;
    emit_cfg.clear_color[1] = 0.06f;
    emit_cfg.clear_color[2] = 0.07f;
    emit_cfg.clear_color[3] = 1.0f;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);

    bool found_begin = false;
    bool found_end = false;
    bool found_draw = false;
    bool found_pipeline = false;
    uint32_t count = dvz_drp2_stream_count(stream);
    for (uint32_t i = 0; i < count; i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd == NULL)
            continue;
        if (cmd->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS)
        {
            found_begin = true;
            AC(cmd->u.begin_render_pass.clear_color[0], 0.05f, 1e-6f);
            AC(cmd->u.begin_render_pass.clear_color[1], 0.06f, 1e-6f);
            AC(cmd->u.begin_render_pass.clear_color[2], 0.07f, 1e-6f);
            AC(cmd->u.begin_render_pass.clear_color[3], 1.0f, 1e-6f);
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_END_RENDER_PASS)
        {
            found_end = true;
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_DRAW)
        {
            found_draw = true;
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_SET_PIPELINE)
        {
            found_pipeline = true;
        }
    }
    AT(found_begin);
    AT(found_end);
    AT(!found_draw);
    AT(!found_pipeline);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}



/* ---- New regression tests ---- */


int test_scene_point_emit_has_vertex_layout(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    AT(panel != NULL);
    DvzVisual* visual = dvz_point(scene, 0);
    AT(visual != NULL);

    float positions[] = {-0.5f, -0.5f, 0.0f,  0.5f, -0.5f, 0.0f,  0.0f, 0.5f, 0.0f};
    DvzColor colors[3] = {{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}};
    float sizes[3] = {10.0f, 10.0f, 10.0f};
    AT(dvz_visual_set_data(visual, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 3) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 3) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);

    /* Find the CREATE_RENDER_PIPELINE command and verify it has vertex layout. */
    bool found_pipeline = false;
    uint32_t count = dvz_drp2_stream_count(stream);
    for (uint32_t i = 0; i < count; i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd != NULL && cmd->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            found_pipeline = true;
            AT(cmd->u.create_render_pipeline.binding_count > 0);
            AT(cmd->u.create_render_pipeline.attr_count > 0);
            AT(cmd->u.create_render_pipeline.binding_strides[0] > 0);
            break;
        }
    }
    AT(found_pipeline);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_indexed_primitive_shading_updates_runtime(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    ANN(visual);

    float positions[4][3] = {
        {-0.8f, -0.8f, 0.0f}, {-0.8f, 0.8f, 0.0f},
        {0.8f, -0.8f, 0.0f},  {0.8f, 0.8f, 0.0f},
    };
    DvzColor colors[4] = {
        {255, 0, 0, 255}, {255, 0, 0, 255}, {255, 0, 0, 255}, {255, 0, 0, 255},
    };
    float normals[4][3] = {
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
    };
    DvzIndex indices[6] = {0, 1, 2, 2, 1, 3};

    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)));

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 4) == 0);
    AT(dvz_visual_set_data(visual, "normal", normals, 4) == 0);
    AT(dvz_visual_set_buffer(visual, "index", index_buffer));
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);
    AT(dvz_visual_set_primitive_shading(
           visual,
           &(DvzPrimitiveShadingDesc){
               .light_direction = {0.0f, 0.0f, 1.0f},
               .ambient = 0.0f,
               .diffuse = 0.0f,
           }) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    DvzDrp2Runtime* runtime = NULL;
    if (ctx != NULL)
    {
        DvzDrp2RuntimeConfig runtime_cfg =
            dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
        runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
        ANN(runtime);
    }

    DvzDrp2CommandStream* stream0 = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    ANN(stream0);
    AT(_stream_set_vertex_buffer_count(stream0) == 3);
    AT(_stream_write_buffer_range_count(stream0, 0, sizeof(DvzSceneMaterialParams)) == 1);
    if (runtime != NULL)
    {
        DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream0);
        AT(result.ok);
        AT(dvz_gpu_ctx_error_count(ctx) == 0);
    }
    dvz_drp2_stream_destroy(stream0);
    stream0 = NULL;

    AT(dvz_visual_set_primitive_shading(
           visual,
           &(DvzPrimitiveShadingDesc){
               .light_direction = {0.0f, 0.0f, 1.0f},
               .ambient = 1.0f,
               .diffuse = 0.0f,
           }) == 0);

    DvzDrp2CommandStream* stream1 = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    ANN(stream1);
    AT(_stream_set_vertex_buffer_count(stream1) == 3);
    AT(_stream_write_buffer_range_count(stream1, 0, sizeof(DvzSceneMaterialParams)) == 1);

    if (runtime != NULL)
    {
        DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream1);
        AT(result.ok);
        AT(dvz_gpu_ctx_error_count(ctx) == 0);
    }

    if (runtime != NULL)
    {
        dvz_drp2_runtime_destroy(runtime);
        runtime = NULL;
    }
    if (ctx != NULL)
    {
        dvz_gpu_ctx_destroy(ctx);
        ctx = NULL;
    }

    dvz_drp2_stream_destroy(stream1);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_point_large_count_executes(TstSuite* suite, TstItem* item)
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
        log_warn("test_scene_point_large_count_executes skipped: GPU context creation failed");
        return 0;
    }

    /* 1000 points — same as hello_scatter, exercises large buffer upload path. */
    const uint32_t N = 1000;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    AT(panel != NULL);
    DvzVisual* visual = dvz_point(scene, 0);
    AT(visual != NULL);

    float* positions = (float*)dvz_malloc(N * 3 * sizeof(float));
    DvzColor* colors = (DvzColor*)dvz_malloc(N * sizeof(DvzColor));
    float* sizes    = (float*)dvz_malloc(N * sizeof(float));
    ANN(positions); ANN(colors); ANN(sizes);

    for (uint32_t i = 0; i < N; i++)
    {
        positions[3 * i + 0] = -1.0f + 2.0f * (float)i / (float)(N - 1);
        positions[3 * i + 1] = 0.0f;
        positions[3 * i + 2] = 0.0f;
        colors[i][0] = 255;
        colors[i][1] = (uint8_t)(i % 256);
        colors[i][2] = 0;
        colors[i][3] = 255;
        sizes[i] = 4.0f;
    }

    AT(dvz_visual_set_data(visual, "position", positions, N) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, N) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, N) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    dvz_free(positions);
    dvz_free(colors);
    dvz_free(sizes);
    dvz_drp2_runtime_destroy(runtime);
    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}


int test_scene_second_emit_no_uploads_when_not_dirty(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    AT(panel != NULL);
    DvzVisual* visual = dvz_point(scene, 0);
    AT(visual != NULL);

    float positions[] = {-0.5f, 0.0f, 0.0f,  0.5f, 0.0f, 0.0f};
    DvzColor colors[2] = {{255, 0, 0, 255}, {0, 255, 0, 255}};
    float sizes[2] = {8.0f, 8.0f};
    AT(dvz_visual_set_data(visual, "position", positions, 2) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 2) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 2) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_wgsl = true;

    /* First emit — dirty, must produce WRITE_BUFFER commands. */
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 = dvz_figure_emit(figure, &caps, &report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream1 != NULL);

    uint32_t wb_count1 = _stream_visual_write_buffer_count(stream1);
    AT(wb_count1 > 0);
    dvz_drp2_stream_destroy(stream1);

    /* Second emit — nothing dirty, so no WRITE_BUFFER commands should be emitted. */
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream2 = dvz_figure_emit(figure, &caps, &report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream2 != NULL);

    uint32_t wb_count2 = 0;
    bool found_draw = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream2); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream2, i);
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_BUFFER &&
            cmd->u.write_buffer.size != sizeof(DvzMVP) &&
            cmd->u.write_buffer.size != sizeof(DvzSceneViewportUniform))
        {
            wb_count2++;
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_DRAW)
            found_draw = true;
    }
    AT(wb_count2 == 0);
    AT(found_draw);

    dvz_drp2_stream_destroy(stream2);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_partial_update_uploads_only_range(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    AT(panel != NULL);
    DvzVisual* visual = dvz_point(scene, 0);
    AT(visual != NULL);

    const uint32_t N = 20;
    float positions[20 * 3];
    DvzColor colors[20];
    float sizes[20];
    for (uint32_t i = 0; i < N; i++)
    {
        positions[3 * i]     = (float)i / (float)N * 2.0f - 1.0f;
        positions[3 * i + 1] = 0.0f;
        positions[3 * i + 2] = 0.0f;
        colors[i][0] = 255; colors[i][1] = 0; colors[i][2] = 0; colors[i][3] = 255;
        sizes[i] = 5.0f;
    }
    AT(dvz_visual_set_data(visual, "position", positions, N) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, N) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, N) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_wgsl = true;

    /* First emit clears dirty flags. */
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 = dvz_figure_emit(figure, &caps, &report);
    AT(stream1 != NULL);
    dvz_drp2_stream_destroy(stream1);

    /* Partial update: items 5–9 only (first_item=5, item_count=5). */
    float new_pos[5 * 3];
    for (uint32_t i = 0; i < 5; i++)
    {
        new_pos[3 * i]     = 0.5f;
        new_pos[3 * i + 1] = 0.5f;
        new_pos[3 * i + 2] = 0.0f;
    }
    AT(dvz_visual_set_data_range(visual, "position", new_pos, 5, 5) == 0);

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream2 = dvz_figure_emit(figure, &caps, &report);
    AT(stream2 != NULL);

    /* Find the position WRITE_BUFFER and verify it covers only the partial range. */
    /* Position attribute size = 3 floats × 4 bytes = 12 bytes per item. */
    const uint64_t item_size    = 3 * sizeof(float);
    const uint64_t expected_off = 5 * item_size;        /* items 0-4 untouched */
    const uint64_t expected_sz  = 5 * item_size;        /* 5 items updated     */
    bool found_partial = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream2); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream2, i);
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_BUFFER &&
            cmd->u.write_buffer.offset == expected_off &&
            cmd->u.write_buffer.size == expected_sz)
        {
            found_partial = true;
            break;
        }
    }
    AT(found_partial);

    dvz_drp2_stream_destroy(stream2);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_repeated_partial_updates_across_frames(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    AT(panel != NULL);
    DvzVisual* visual = dvz_point(scene, 0);
    AT(visual != NULL);

    const uint32_t N = 20;
    float positions[20 * 3];
    DvzColor colors[20];
    float sizes[20];
    for (uint32_t i = 0; i < N; i++)
    {
        positions[3 * i]     = (float)i / (float)N * 2.0f - 1.0f;
        positions[3 * i + 1] = 0.0f;
        positions[3 * i + 2] = 0.0f;
        colors[i][0]         = 255;
        colors[i][1]         = 0;
        colors[i][2]         = 0;
        colors[i][3]         = 255;
        sizes[i]             = 5.0f;
    }
    AT(dvz_visual_set_data(visual, "position", positions, N) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, N) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, N) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_wgsl = true;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 = dvz_figure_emit(figure, &caps, &report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream1 != NULL);
    dvz_drp2_stream_destroy(stream1);

    const uint64_t item_size = 3 * sizeof(float);

    float frame2_pos[3 * 3] = {
        -0.25f, 0.25f, 0.0f,
        -0.15f, 0.25f, 0.0f,
        -0.05f, 0.25f, 0.0f,
    };
    uint64_t frame2_offset = 2 * item_size;
    uint64_t frame2_size = 3 * item_size;
    AT(dvz_visual_set_data_range(visual, "position", frame2_pos, 2, 3) == 0);

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream2 = dvz_figure_emit(figure, &caps, &report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream2 != NULL);
    AT(_stream_visual_write_buffer_count(stream2) == 1);
    AT(_stream_write_buffer_range_count(stream2, frame2_offset, frame2_size) == 1);
    dvz_drp2_stream_destroy(stream2);

    float frame3_pos[2 * 3] = {
        0.25f, -0.25f, 0.0f,
        0.35f, -0.25f, 0.0f,
    };
    uint64_t frame3_offset = 10 * item_size;
    uint64_t frame3_size = 2 * item_size;
    AT(dvz_visual_set_data_range(visual, "position", frame3_pos, 10, 2) == 0);

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream3 = dvz_figure_emit(figure, &caps, &report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream3 != NULL);
    AT(_stream_visual_write_buffer_count(stream3) == 1);
    AT(_stream_write_buffer_range_count(stream3, frame2_offset, frame2_size) == 0);
    AT(_stream_write_buffer_range_count(stream3, frame3_offset, frame3_size) == 1);

    dvz_drp2_stream_destroy(stream3);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_partial_update_merges_ranges_before_emit(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    AT(panel != NULL);
    DvzVisual* visual = dvz_point(scene, 0);
    AT(visual != NULL);

    const uint32_t N = 20;
    float positions[20 * 3];
    DvzColor colors[20];
    float sizes[20];
    for (uint32_t i = 0; i < N; i++)
    {
        positions[3 * i]     = (float)i / (float)N * 2.0f - 1.0f;
        positions[3 * i + 1] = 0.0f;
        positions[3 * i + 2] = 0.0f;
        colors[i][0]         = 0;
        colors[i][1]         = 255;
        colors[i][2]         = 0;
        colors[i][3]         = 255;
        sizes[i]             = 5.0f;
    }
    AT(dvz_visual_set_data(visual, "position", positions, N) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, N) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, N) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_wgsl = true;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 = dvz_figure_emit(figure, &caps, &report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream1 != NULL);
    dvz_drp2_stream_destroy(stream1);

    float update_a[2 * 3] = {
        -0.75f, 0.1f, 0.0f,
        -0.65f, 0.1f, 0.0f,
    };
    float update_b[3 * 3] = {
        0.15f, 0.1f, 0.0f,
        0.25f, 0.1f, 0.0f,
        0.35f, 0.1f, 0.0f,
    };
    AT(dvz_visual_set_data_range(visual, "position", update_a, 2, 2) == 0);
    AT(dvz_visual_set_data_range(visual, "position", update_b, 8, 3) == 0);

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream2 = dvz_figure_emit(figure, &caps, &report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream2 != NULL);

    const uint64_t item_size = 3 * sizeof(float);
    const uint64_t expected_offset = 2 * item_size;
    const uint64_t expected_size = 9 * item_size;
    AT(_stream_visual_write_buffer_count(stream2) == 1);
    AT(_stream_write_buffer_range_count(stream2, expected_offset, expected_size) == 1);

    dvz_drp2_stream_destroy(stream2);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_multiple_panels_multiple_point_visuals_emit(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 128, 64, 0);
    AT(figure != NULL);
    DvzPanel* left = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 0.5f, 1.0f});
    DvzPanel* right = dvz_panel(figure, (DvzPanelDesc){0.5f, 0.0f, 0.5f, 1.0f});
    AT(left != NULL);
    AT(right != NULL);

    DvzVisual* visual_a = dvz_point(scene, 0);
    DvzVisual* visual_b = dvz_point(scene, 0);
    AT(visual_a != NULL);
    AT(visual_b != NULL);

    float pos_a[2 * 3] = {
        -0.75f, 0.0f, 0.0f,
        -0.60f, 0.0f, 0.0f,
    };
    float pos_b[3 * 3] = {
        0.15f, 0.0f, 0.0f,
        0.30f, 0.0f, 0.0f,
        0.45f, 0.0f, 0.0f,
    };
    DvzColor color_a[2] = {{255, 0, 0, 255}, {255, 0, 0, 255}};
    DvzColor color_b[3] = {
        {0, 255, 0, 255},
        {0, 255, 0, 255},
        {0, 255, 0, 255},
    };
    float size_a[2] = {5.0f, 5.0f};
    float size_b[3] = {6.0f, 6.0f, 6.0f};

    AT(dvz_visual_set_data(visual_a, "position", pos_a, 2) == 0);
    AT(dvz_visual_set_data(visual_a, "color", color_a, 2) == 0);
    AT(dvz_visual_set_data(visual_a, "size", size_a, 2) == 0);
    AT(dvz_visual_set_data(visual_b, "position", pos_b, 3) == 0);
    AT(dvz_visual_set_data(visual_b, "color", color_b, 3) == 0);
    AT(dvz_visual_set_data(visual_b, "size", size_b, 3) == 0);
    AT(dvz_panel_add_visual(left, visual_a, NULL) == 0);
    AT(dvz_panel_add_visual(right, visual_b, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_wgsl = true;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 = dvz_figure_emit(figure, &caps, &report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream1 != NULL);
    AT(_stream_visual_write_buffer_count(stream1) == 6);
    AT(_stream_set_vertex_buffer_count(stream1) == 6);
    AT(_stream_draw_count(stream1) == 2);
    uint32_t begin_render_pass_count = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream1); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream1, i);
        if (cmd == NULL || cmd->type != DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS)
            continue;
        if (begin_render_pass_count == 0)
        {
            AC(cmd->u.begin_render_pass.viewport[0], 0.0f, 1e-6f);
            AC(cmd->u.begin_render_pass.viewport[1], 0.0f, 1e-6f);
            AC(cmd->u.begin_render_pass.viewport[2], 0.5f, 1e-6f);
            AC(cmd->u.begin_render_pass.viewport[3], 1.0f, 1e-6f);
            AT(cmd->u.begin_render_pass.clear);
        }
        else if (begin_render_pass_count == 1)
        {
            AC(cmd->u.begin_render_pass.viewport[0], 0.5f, 1e-6f);
            AC(cmd->u.begin_render_pass.viewport[1], 0.0f, 1e-6f);
            AC(cmd->u.begin_render_pass.viewport[2], 0.5f, 1e-6f);
            AC(cmd->u.begin_render_pass.viewport[3], 1.0f, 1e-6f);
            AT(!cmd->u.begin_render_pass.clear);
        }
        begin_render_pass_count++;
    }
    AT(begin_render_pass_count == 2);
    dvz_drp2_stream_destroy(stream1);

    float size_update[2] = {10.0f, 11.0f};
    AT(dvz_visual_set_data_range(visual_b, "size", size_update, 1, 2) == 0);

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream2 = dvz_figure_emit(figure, &caps, &report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream2 != NULL);
    AT(_stream_visual_write_buffer_count(stream2) == 1);
    AT(_stream_write_buffer_range_count(stream2, sizeof(float), 2 * sizeof(float)) == 1);
    AT(_stream_set_vertex_buffer_count(stream2) == 6);
    AT(_stream_draw_count(stream2) == 2);

    dvz_drp2_stream_destroy(stream2);
    dvz_scene_destroy(scene);
    return 0;
}


static bool _command_allowed_inside_render_pass(DvzDrp2CommandType type)
{
    switch (type)
    {
    case DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS:
    case DVZ_DRP2_COMMAND_SET_VIEWPORT:
    case DVZ_DRP2_COMMAND_SET_SCISSOR:
    case DVZ_DRP2_COMMAND_SET_PIPELINE:
    case DVZ_DRP2_COMMAND_SET_BIND_GROUP:
    case DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER:
    case DVZ_DRP2_COMMAND_SET_INDEX_BUFFER:
    case DVZ_DRP2_COMMAND_DRAW:
    case DVZ_DRP2_COMMAND_DRAW_INDEXED:
    case DVZ_DRP2_COMMAND_END_RENDER_PASS:
        return true;
    default:
        return false;
    }
}


int test_scene_render_pass_scope_excludes_resource_commands(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    DvzVisual* visual = dvz_point(scene, 0);
    AT(visual != NULL);

    float positions[3][3] = {
        {-0.5f, -0.5f, 0.0f},
        {+0.5f, -0.5f, 0.0f},
        { 0.0f, +0.5f, 0.0f},
    };
    DvzColor colors[3] = {{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}};
    float sizes[3] = {10.0f, 12.0f, 14.0f};

    AT(dvz_visual_set_data(visual, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 3) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 3) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);

    bool in_render_pass = false;
    uint32_t pass_count = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        ANN(cmd);
        if (cmd->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS)
        {
            AT(!in_render_pass);
            in_render_pass = true;
            pass_count++;
        }

        if (in_render_pass)
            AT(_command_allowed_inside_render_pass(cmd->type));

        if (cmd->type == DVZ_DRP2_COMMAND_END_RENDER_PASS)
            in_render_pass = false;
    }

    AT(!in_render_pass);
    AT(pass_count > 0);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify visual alpha mode storage and validation.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_visual_alpha_mode(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzVisual* visual = dvz_mesh(scene, 0);
    AT(visual != NULL);

    AT(dvz_visual_alpha_mode(visual) == DVZ_ALPHA_OPAQUE);
    AT(dvz_visual_set_alpha_mode(visual, DVZ_ALPHA_BLENDED) == 0);
    AT(dvz_visual_alpha_mode(visual) == DVZ_ALPHA_BLENDED);
    AT(dvz_visual_set_alpha_mode(visual, DVZ_ALPHA_WBOIT) == 0);
    AT(dvz_visual_alpha_mode(visual) == DVZ_ALPHA_WBOIT);
    AT(dvz_visual_set_alpha_mode(visual, DVZ_ALPHA_DEPTH_PEEL) == 0);
    AT(dvz_visual_alpha_mode(visual) == DVZ_ALPHA_DEPTH_PEEL);
    AT(dvz_visual_set_alpha_mode(visual, DVZ_ALPHA_MASK) == 0);
    AT(dvz_visual_alpha_mode(visual) == DVZ_ALPHA_MASK);
#ifndef __clang_analyzer__
    volatile int invalid_mode = (int)DVZ_ALPHA_MASK + 1;
    AT_EXPECTED_ERROR_STRICT(
        suite,
        dvz_visual_set_alpha_mode(
            visual, (DvzAlphaMode)invalid_mode) == -1);
#endif
    AT(dvz_visual_alpha_mode(visual) == DVZ_ALPHA_MASK);

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify internal material state defaults and compatibility setter synchronization.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_visual_internal_material_state(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzVisual* point = dvz_point(scene, 0);
    DvzVisual* mesh = dvz_mesh(scene, 0);
    DvzVisual* volume = dvz_volume(scene, 0);
    AT(point != NULL);
    AT(mesh != NULL);
    AT(volume != NULL);

    AT(point->material.kind == DVZ_MATERIAL_KIND_UNLIT);
    AT(mesh->material.kind == DVZ_MATERIAL_KIND_LIT);
    AT(volume->material.kind == DVZ_MATERIAL_KIND_VOLUME);
    AT(mesh->material.alpha_mode == DVZ_ALPHA_OPAQUE);
    AT(mesh->material.opacity == 1.0f);
    AT(mesh->material.light_direction[2] == 1.0f);
    AT(mesh->material.ambient == 0.2f);
    AT(mesh->material.diffuse == 0.8f);
    AT(!mesh->material.depth_cue_enabled);
    AT(mesh->material.depth_cue_mode == DVZ_DEPTH_CUE_NONE);
    AT(mesh->material.depth_cue_metric == DVZ_DEPTH_CUE_METRIC_CLIP_DEPTH);
    AT(mesh->material.depth_cue_falloff == DVZ_DEPTH_CUE_FALLOFF_LINEAR);
    AT(mesh->material.depth_cue_near == 0.0f);
    AT(mesh->material.depth_cue_far == 1.0f);
    AT(mesh->material.depth_cue_strength == 1.0f);
    AT(mesh->material.depth_cue_density == 3.0f);
    AT(mesh->material.depth_cue_background[3] == 1.0f);
    AT(mesh->material_params.depth_cue[1] == 1.0f);
    AT(mesh->material_params.depth_cue[2] == 0.0f);
    AT(mesh->material_params.depth_cue_extra[2] == 3.0f);
    AT(mesh->material.scalar_scale == 1.0f);

    uint64_t point_material_version = point->material.version;
    AT(dvz_visual_set_depth_cue(
           point,
           &(DvzDepthCueDesc){
               .mode = DVZ_DEPTH_CUE_FADE_TO_BACKGROUND,
               .metric = DVZ_DEPTH_CUE_METRIC_EYE_DISTANCE,
               .falloff = DVZ_DEPTH_CUE_FALLOFF_EXPONENTIAL,
               .near_depth = 0.1f,
               .far_depth = 0.8f,
               .strength = 0.5f,
               .density = 2.0f,
               .background_color = {0.02f, 0.04f, 0.06f, 1.0f},
           }) == 0);
    AT(point->material.depth_cue_enabled);
    AT(point->material.depth_cue_mode == DVZ_DEPTH_CUE_FADE_TO_BACKGROUND);
    AT(point->material.depth_cue_metric == DVZ_DEPTH_CUE_METRIC_EYE_DISTANCE);
    AT(point->material.depth_cue_falloff == DVZ_DEPTH_CUE_FALLOFF_EXPONENTIAL);
    AT(point->material_params.depth_cue[0] == 0.1f);
    AT(point->material_params.depth_cue[1] == 0.8f);
    AT(point->material_params.depth_cue[2] == 0.5f);
    AT(point->material_params.depth_cue[3] == (float)DVZ_DEPTH_CUE_FADE_TO_BACKGROUND);
    AT(point->material_params.depth_cue_extra[0] == (float)DVZ_DEPTH_CUE_METRIC_EYE_DISTANCE);
    AT(point->material_params.depth_cue_extra[1] == (float)DVZ_DEPTH_CUE_FALLOFF_EXPONENTIAL);
    AT(point->material_params.depth_cue_extra[2] == 2.0f);
    AT(point->material_params.depth_cue_color[2] == 0.06f);
    AT(point->material.version > point_material_version);
    AT(dvz_visual_set_depth_cue(point, NULL) == 0);
    AT(!point->material.depth_cue_enabled);
    AT(point->material_params.depth_cue[2] == 0.0f);

    AT(dvz_visual_set_alpha_mode(mesh, DVZ_ALPHA_WBOIT) == 0);
    AT(mesh->alpha_mode == DVZ_ALPHA_WBOIT);
    AT(mesh->material.alpha_mode == DVZ_ALPHA_WBOIT);
    uint64_t material_version = mesh->material.version;
    AT(material_version > 0);

    AT(dvz_visual_set_primitive_shading(
           mesh,
           &(DvzPrimitiveShadingDesc){
               .light_direction = {1.0f, 2.0f, 3.0f},
               .ambient = 0.35f,
               .diffuse = 0.65f,
           }) == 0);
    AT(mesh->material_params.light_direction[0] == 1.0f);
    AT(mesh->material_params.light_direction[1] == 2.0f);
    AT(mesh->material_params.light_direction[2] == 3.0f);
    AT(mesh->material_params.params[0] == 0.35f);
    AT(mesh->material_params.params[1] == 0.65f);
    AT(mesh->material.light_direction[0] == 1.0f);
    AT(mesh->material.light_direction[1] == 2.0f);
    AT(mesh->material.light_direction[2] == 3.0f);
    AT(mesh->material.ambient == 0.35f);
    AT(mesh->material.diffuse == 0.65f);
    AT(mesh->material.version > material_version);
    material_version = mesh->material.version;

    AT(dvz_visual_set_depth_cue(
           mesh,
           &(DvzDepthCueDesc){
               .mode = DVZ_DEPTH_CUE_DESATURATE,
               .near_depth = 0.25f,
               .far_depth = 0.9f,
               .strength = 0.75f,
               .background_color = {0.1f, 0.2f, 0.3f, 1.0f},
           }) == 0);
    AT(mesh->material.depth_cue_enabled);
    AT(mesh->material.depth_cue_mode == DVZ_DEPTH_CUE_DESATURATE);
    AT(mesh->material.depth_cue_near == 0.25f);
    AT(mesh->material.depth_cue_far == 0.9f);
    AT(mesh->material.depth_cue_strength == 0.75f);
    AT(mesh->material.depth_cue_background[2] == 0.3f);
    AT(mesh->material_params.depth_cue[0] == 0.25f);
    AT(mesh->material_params.depth_cue[1] == 0.9f);
    AT(mesh->material_params.depth_cue[2] == 0.75f);
    AT(mesh->material_params.depth_cue[3] == (float)DVZ_DEPTH_CUE_DESATURATE);
    AT(mesh->material_params.depth_cue_color[1] == 0.2f);
    AT(mesh->material.version > material_version);

    material_version = mesh->material.version;
    AT(dvz_visual_set_depth_cue(mesh, NULL) == 0);
    AT(!mesh->material.depth_cue_enabled);
    AT(mesh->material.depth_cue_mode == DVZ_DEPTH_CUE_NONE);
    AT(mesh->material_params.depth_cue[2] == 0.0f);
    AT(mesh->material.version > material_version);

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify internal pass capability resolution for current retained visual families.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_visual_pass_capabilities(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    DvzVisual* point = dvz_point(scene, 0);
    DvzVisual* pixel = dvz_pixel(scene, 0);
    DvzVisual* primitive = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    DvzVisual* path = dvz_path(scene, 0);
    DvzVisual* fixed_primitive = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    DvzVisual* mesh = dvz_mesh(scene, 0);
    DvzVisual* image = dvz_image(scene, 0);
    DvzVisual* volume = dvz_volume(scene, 0);
    AT(point != NULL);
    AT(pixel != NULL);
    AT(primitive != NULL);
    AT(path != NULL);
    AT(fixed_primitive != NULL);
    AT(mesh != NULL);
    AT(image != NULL);
    AT(volume != NULL);

    float normals[3][3] = {
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
    };
    AT(dvz_visual_set_data(mesh, "normal", normals, 3) == 0);
    AT(dvz_visual_set_depth_cue(
           point,
           &(DvzDepthCueDesc){
               .mode = DVZ_DEPTH_CUE_DARKEN,
               .near_depth = 0.1f,
               .far_depth = 0.9f,
               .strength = 0.5f,
               .background_color = {0.0f, 0.0f, 0.0f, 1.0f},
           }) == 0);
    AT(dvz_visual_set_depth_cue(
           mesh,
           &(DvzDepthCueDesc){
               .mode = DVZ_DEPTH_CUE_DARKEN,
               .near_depth = 0.1f,
               .far_depth = 0.9f,
               .strength = 0.5f,
               .background_color = {0.0f, 0.0f, 0.0f, 1.0f},
           }) == 0);
    AT(dvz_visual_set_alpha_mode(point, DVZ_ALPHA_WBOIT) == 0);
    AT(dvz_visual_set_alpha_mode(volume, DVZ_ALPHA_BLENDED) == 0);

    DvzVisualAttachDesc fixed = {
        .z_layer = 0,
        .controller_mode = DVZ_CONTROLLER_FIXED,
    };
    AT(dvz_panel_add_visual(panel, point, NULL) == 0);
    AT(dvz_panel_add_visual(panel, pixel, NULL) == 0);
    AT(dvz_panel_add_visual(panel, primitive, NULL) == 0);
    AT(dvz_panel_add_visual(panel, path, NULL) == 0);
    AT(dvz_panel_add_visual(panel, fixed_primitive, &fixed) == 0);
    AT(dvz_panel_add_visual(panel, mesh, NULL) == 0);
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);
    AT(dvz_panel_add_visual(panel, volume, NULL) == 0);

    DvzSceneVisualPassCaps caps = {0};
    DvzSceneGBufferPlan gbuffer = {0};
    _scene_technique_gbuffer_plan_init(&gbuffer);

    AT(_scene_visual_pass_caps_from_visual(point, &panel->visuals[0], &caps));
    AT(caps.kind == DVZ_SCENE_VISUAL_DESC_POINT);
    AT(caps.draws_in_wboit_pass);
    AT(!caps.draws_in_opaque_pass);
    AT(caps.writes_color);
    AT(!caps.writes_depth);
    AT(caps.can_write_depth);
    AT(caps.can_depth_test);
    AT(caps.needs_depth_attachment);
    AT(!caps.eligible_for_depth_postprocess);
    AT(!caps.eligible_for_gbuffer);
    AT(caps.uses_common_set);
    AT(caps.uses_material_set);
    AT(caps.supports_depth_cue);
    AT(caps.depth_cue_enabled);
    AT(!_scene_technique_gbuffer_plan_add_visual(&gbuffer, point, &panel->visuals[0]));

    AT(_scene_visual_pass_caps_from_visual(pixel, &panel->visuals[1], &caps));
    AT(caps.kind == DVZ_SCENE_VISUAL_DESC_PIXEL);
    AT(caps.draws_in_opaque_pass);
    AT(caps.writes_color);
    AT(caps.writes_depth);
    AT(caps.can_depth_test);
    AT(caps.needs_depth_attachment);
    AT(caps.eligible_for_depth_postprocess);
    AT(!caps.eligible_for_gbuffer);
    AT(caps.uses_common_set);
    AT(!caps.uses_material_set);
    AT(caps.supports_depth_cue);
    AT(!caps.depth_cue_enabled);
    AT(!_scene_technique_gbuffer_plan_add_visual(&gbuffer, pixel, &panel->visuals[1]));

    AT(_scene_visual_pass_caps_from_visual(primitive, &panel->visuals[2], &caps));
    AT(caps.kind == DVZ_SCENE_VISUAL_DESC_PRIMITIVE);
    AT(caps.draws_in_opaque_pass);
    AT(caps.writes_color);
    AT(caps.writes_depth);
    AT(caps.can_write_depth);
    AT(caps.can_depth_test);
    AT(caps.needs_depth_attachment);
    AT(caps.eligible_for_depth_postprocess);
    AT(!caps.eligible_for_gbuffer);
    AT(!caps.has_normals);
    AT(!caps.supports_depth_cue);
    AT(!_scene_technique_gbuffer_plan_add_visual(&gbuffer, primitive, &panel->visuals[2]));

    AT(_scene_visual_pass_caps_from_visual(path, &panel->visuals[3], &caps));
    AT(caps.kind == DVZ_SCENE_VISUAL_DESC_PRIMITIVE);
    AT(caps.draws_in_opaque_pass);
    AT(caps.writes_depth);
    AT(caps.eligible_for_depth_postprocess);
    AT(!caps.has_normals);
    AT(!caps.eligible_for_gbuffer);
    AT(!_scene_technique_gbuffer_plan_add_visual(&gbuffer, path, &panel->visuals[3]));

    AT(_scene_visual_pass_caps_from_visual(fixed_primitive, &panel->visuals[4], &caps));
    AT(caps.fixed_controller);
    AT(caps.writes_color);
    AT(!caps.writes_depth);
    AT(!caps.can_write_depth);
    AT(!caps.can_depth_test);
    AT(!caps.needs_depth_attachment);
    AT(!caps.eligible_for_depth_postprocess);
    AT(!caps.eligible_for_gbuffer);
    AT(!_scene_technique_gbuffer_plan_add_visual(
        &gbuffer, fixed_primitive, &panel->visuals[4]));

    AT(_scene_visual_pass_caps_from_visual(mesh, &panel->visuals[5], &caps));
    AT(caps.kind == DVZ_SCENE_VISUAL_DESC_PRIMITIVE);
    AT(caps.has_normals);
    AT(caps.writes_depth);
    AT(caps.eligible_for_depth_postprocess);
    AT(caps.eligible_for_gbuffer);
    AT(caps.needs_material_layout);
    AT(caps.uses_material_set);
    AT(caps.supports_depth_cue);
    AT(caps.depth_cue_enabled);
    AT(_scene_technique_gbuffer_plan_add_visual(&gbuffer, mesh, &panel->visuals[5]));
    AT(gbuffer.enabled);
    AT(gbuffer.needs_depth);
    AT(gbuffer.needs_normal);
    AT(!gbuffer.needs_object_id);
    AT(gbuffer.producer_count == 1);

    AT(_scene_visual_pass_caps_from_visual(image, &panel->visuals[6], &caps));
    AT(caps.kind == DVZ_SCENE_VISUAL_DESC_IMAGE);
    AT(caps.draws_in_opaque_pass);
    AT(caps.writes_color);
    AT(!caps.writes_depth);
    AT(!caps.needs_depth_attachment);
    AT(!caps.eligible_for_depth_postprocess);
    AT(!caps.eligible_for_gbuffer);
    AT(caps.uses_image_set);
    AT(!_scene_technique_gbuffer_plan_add_visual(&gbuffer, image, &panel->visuals[6]));
    AT(gbuffer.producer_count == 1);

    AT(_scene_visual_pass_caps_from_visual(volume, &panel->visuals[7], &caps));
    AT(caps.kind == DVZ_SCENE_VISUAL_DESC_VOLUME);
    AT(caps.draws_in_transparent_blend_pass);
    AT(!caps.draws_in_opaque_pass);
    AT(caps.uses_source_over_blend);
    AT(caps.writes_color);
    AT(!caps.writes_depth);
    AT(caps.samples_depth);
    AT(caps.needs_depth_attachment);
    AT(!caps.eligible_for_depth_postprocess);
    AT(!caps.eligible_for_gbuffer);
    AT(caps.uses_volume_set);
    AT(!_scene_technique_gbuffer_plan_add_visual(&gbuffer, volume, &panel->visuals[7]));
    AT(gbuffer.producer_count == 1);

    DvzSceneVisualDesc desc = {
        .kind = DVZ_SCENE_VISUAL_DESC_PRIMITIVE,
        .has_normal = true,
        .material_buffer_id = 42,
    };
    AT(_scene_visual_pass_caps_from_desc(
        &desc, DVZ_ALPHA_BLENDED, DVZ_CONTROLLER_APPLY, &caps));
    AT(caps.draws_in_opaque_pass);
    AT(caps.uses_source_over_blend);
    AT(caps.writes_depth);
    AT(caps.eligible_for_depth_postprocess);
    AT(caps.eligible_for_gbuffer);
    AT(caps.needs_material_layout);
    AT(caps.uses_material_set);
    AT(caps.supports_depth_cue);

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify eligible mesh visuals lower an internal G-buffer graph pass to DRP2.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_gbuffer_runtime_lowering(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    DvzVisual* mesh = dvz_mesh(scene, 0);
    AT(mesh != NULL);

    float positions[4][3] = {
        {-0.5f, -0.5f, 0.0f},
        {0.5f, -0.5f, 0.0f},
        {-0.5f, 0.5f, 0.0f},
        {0.5f, 0.5f, 0.0f},
    };
    float normals[4][3] = {
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
    };
    DvzIndex indices[6] = {0, 1, 2, 2, 1, 3};
    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)));

    AT(dvz_visual_set_data(mesh, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(mesh, "normal", normals, 4) == 0);
    AT(dvz_visual_set_buffer(mesh, "index", index_buffer));
    AT(dvz_panel_add_visual(panel, mesh, NULL) == 0);

    AT(!_scene_technique_gbuffer_enabled(scene, panel));
    DvzFramePlan* default_plan = dvz_frame_plan("figure.gbuffer.default", 0);
    ANN(default_plan);
    _scene_emit_panel_render(figure, 0, default_plan, "figure_0");
    AT(dvz_frame_plan_node_count(default_plan) == 1);
    const DvzFramePlanNode* default_node = dvz_frame_plan_node_get(default_plan, 0);
    ANN(default_node);
    AT(dvz_frame_plan_render_pass_role(default_node) == DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE);
    dvz_frame_plan_destroy(default_plan);

    _scene_technique_state_enable_gbuffer(&panel->techniques, true);
    AT(_scene_technique_gbuffer_enabled(scene, panel));

    DvzFramePlan* plan = dvz_frame_plan("figure.gbuffer", 0);
    ANN(plan);
    _scene_emit_panel_render(figure, 0, plan, "figure_0");
    AT(dvz_frame_plan_node_count(plan) == 2);
    const DvzFramePlanNode* gbuffer_node = dvz_frame_plan_node_get(plan, 0);
    const DvzFramePlanNode* opaque_node = dvz_frame_plan_node_get(plan, 1);
    ANN(gbuffer_node);
    ANN(opaque_node);
    AT(dvz_frame_plan_render_pass_role(gbuffer_node) == DVZ_FRAME_PLAN_RENDER_PASS_GBUFFER);
    AT(dvz_frame_plan_render_pass_role(opaque_node) == DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE);
    AT(gbuffer_node->u.render.visual_count == 1);
    AT(opaque_node->u.render.visual_count == 1);
    AT(dvz_frame_plan_graph_resource_count(plan) == 4);
    AT(dvz_frame_plan_graph_pass_count(plan) == 2);
    const DvzFrameGraphPass* gbuffer_pass = dvz_frame_plan_graph_pass_get(plan, 0);
    ANN(gbuffer_pass);
    AT(strcmp(gbuffer_pass->work_label, "gbuffer") == 0);
    AT(gbuffer_pass->color_attachment_count == 1);
    AT(gbuffer_pass->has_depth_attachment);
    AT(strcmp(gbuffer_pass->color_attachments[0].resource_id, "figure_0_p0.gbuffer.normal") == 0);
    AT(strcmp(gbuffer_pass->depth_attachment.resource_id, "figure_0_p0.gbuffer.depth") == 0);

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.target_width = 64;
    cfg.target_height = 64;
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);
    DvzDrp2ValidationResult validation = dvz_drp2_validate_stream(stream);
    AT(validation.ok);

    bool found_normal_texture = false;
    bool found_depth_texture = false;
    bool found_gbuffer_pass = false;
    bool found_gbuffer_pipeline = false;
    uint64_t normal_id = 0;
    uint64_t depth_id = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        ANN(cmd);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE)
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.create_texture.id);
            if (label != NULL && strcmp(label, "fig0_p0.gbuffer.normal") == 0)
            {
                normal_id = cmd->u.create_texture.id;
                found_normal_texture =
                    cmd->u.create_texture.format == VK_FORMAT_R16G16B16A16_SFLOAT &&
                    (cmd->u.create_texture.usage &
                     DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT) != 0 &&
                    (cmd->u.create_texture.usage & DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING) != 0;
            }
            if (label != NULL && strcmp(label, "fig0_p0.gbuffer.depth") == 0)
            {
                depth_id = cmd->u.create_texture.id;
                found_depth_texture =
                    cmd->u.create_texture.format == VK_FORMAT_D32_SFLOAT &&
                    (cmd->u.create_texture.usage &
                     DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT) != 0 &&
                    (cmd->u.create_texture.usage & DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING) != 0;
            }
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS)
        {
            found_gbuffer_pass =
                found_gbuffer_pass ||
                 (normal_id != 0 && depth_id != 0 &&
                 cmd->u.begin_render_pass.texture_id == normal_id &&
                 cmd->u.begin_render_pass.depth_texture_id == depth_id);
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.create_render_pipeline.id);
            found_gbuffer_pipeline =
                found_gbuffer_pipeline ||
                (label != NULL && strstr(label, "_pipe_gbuffer") != NULL &&
                 cmd->u.create_render_pipeline.color_targets[0].format ==
                     VK_FORMAT_R16G16B16A16_SFLOAT &&
                 cmd->u.create_render_pipeline.has_depth_attachment &&
                 cmd->u.create_render_pipeline.depth_write_enabled);
        }
    }
    AT(found_normal_texture);
    AT(found_depth_texture);
    AT(found_gbuffer_pass);
    AT(found_gbuffer_pipeline);

    dvz_drp2_stream_destroy(stream);
    dvz_frame_plan_destroy(plan);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify point panels can lower an EDL post-process graph pass to DRP2.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_edl_runtime_lowering(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    DvzVisual* points = dvz_point(scene, 0);
    AT(points != NULL);
    float positions[3][3] = {
        {-0.35f, -0.20f, 0.1f},
        {+0.20f, +0.05f, 0.3f},
        {+0.05f, +0.35f, 0.6f},
    };
    DvzColor colors[3] = {
        {255, 80, 60, 255},
        {80, 220, 120, 255},
        {80, 140, 255, 255},
    };
    float sizes[3] = {18.0f, 22.0f, 26.0f};
    AT(dvz_visual_set_data(points, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(points, "color", colors, 3) == 0);
    AT(dvz_visual_set_data(points, "size", sizes, 3) == 0);
    AT(dvz_panel_add_visual(panel, points, NULL) == 0);

    DvzFramePlan* default_plan = dvz_frame_plan("figure.edl.default", 0);
    ANN(default_plan);
    _scene_emit_panel_render(figure, 0, default_plan, "figure_0");
    AT(dvz_frame_plan_node_count(default_plan) == 1);
    AT(dvz_frame_plan_graph_pass_count(default_plan) == 0);
    dvz_frame_plan_destroy(default_plan);

    AT(dvz_panel_set_edl(
        panel, &(DvzEdlDesc){.radius = 2.0f, .strength = 55.0f, .depth_scale = 1.0f}));

    DvzFramePlan* plan = dvz_frame_plan("figure.edl", 0);
    ANN(plan);
    _scene_emit_panel_render(figure, 0, plan, "figure_0");
    AT(dvz_frame_plan_node_count(plan) == 3);
    const DvzFramePlanNode* opaque_node = dvz_frame_plan_node_get(plan, 0);
    const DvzFramePlanNode* upload_node = dvz_frame_plan_node_get(plan, 1);
    const DvzFramePlanNode* edl_node = dvz_frame_plan_node_get(plan, 2);
    ANN(opaque_node);
    ANN(upload_node);
    ANN(edl_node);
    AT(dvz_frame_plan_render_pass_role(opaque_node) == DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE);
    AT(dvz_frame_plan_node_type(upload_node) == DVZ_FRAME_PLAN_NODE_UPLOAD);
    AT(dvz_frame_plan_render_pass_role(edl_node) == DVZ_FRAME_PLAN_RENDER_PASS_EDL_RESOLVE);
    AT(strcmp(upload_node->u.upload.resource_id, "figure_0_p0.edl.params") == 0);
    AT(upload_node->u.upload.byte_size == sizeof(DvzSceneEdlUniform));
    AT(dvz_frame_plan_graph_resource_count(plan) == 3);
    AT(dvz_frame_plan_graph_pass_count(plan) == 2);
    const DvzFrameGraphPass* opaque_pass = dvz_frame_plan_graph_pass_get(plan, 0);
    const DvzFrameGraphPass* edl_pass = dvz_frame_plan_graph_pass_get(plan, 1);
    ANN(opaque_pass);
    ANN(edl_pass);
    AT(strcmp(opaque_pass->work_label, "opaque") == 0);
    AT(strcmp(edl_pass->work_label, "edl_resolve") == 0);
    AT(opaque_pass->has_depth_attachment);
    AT(strcmp(opaque_pass->color_attachments[0].resource_id, "figure_0_p0.edl.color") == 0);
    AT(strcmp(opaque_pass->depth_attachment.resource_id, "figure_0_p0.edl.depth") == 0);
    AT(edl_pass->read_count == 2);
    AT(strcmp(edl_pass->reads[0].resource_id, "figure_0_p0.edl.color") == 0);
    AT(strcmp(edl_pass->reads[1].resource_id, "figure_0_p0.edl.depth") == 0);

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.target_width = 64;
    cfg.target_height = 64;
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);
    DvzDrp2ValidationResult validation = dvz_drp2_validate_stream(stream);
    AT(validation.ok);

    bool found_color_texture = false;
    bool found_depth_texture = false;
    bool found_params_upload = false;
    bool found_edl_pipeline = false;
    bool found_edl_bind_group = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        ANN(cmd);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE)
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.create_texture.id);
            found_color_texture =
                found_color_texture ||
                (label != NULL && strcmp(label, "fig0_p0.edl.color") == 0 &&
                 cmd->u.create_texture.format == VK_FORMAT_R8G8B8A8_UNORM);
            found_depth_texture =
                found_depth_texture ||
                (label != NULL && strcmp(label, "fig0_p0.edl.depth") == 0 &&
                 cmd->u.create_texture.format == VK_FORMAT_D32_SFLOAT);
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_WRITE_BUFFER)
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.write_buffer.buffer_id);
            found_params_upload =
                found_params_upload ||
                (label != NULL && strcmp(label, "fig0_p0.edl.params") == 0 &&
                 cmd->u.write_buffer.size == sizeof(DvzSceneEdlUniform));
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.create_render_pipeline.id);
            found_edl_pipeline =
                found_edl_pipeline ||
                (label != NULL && strstr(label, "_pipe_edl_resolve") != NULL);
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_CREATE_BIND_GROUP)
        {
            found_edl_bind_group =
                found_edl_bind_group || cmd->u.create_bind_group.entry_count == 4;
        }
    }
    AT(found_color_texture);
    AT(found_depth_texture);
    AT(found_params_upload);
    AT(found_edl_pipeline);
    AT(found_edl_bind_group);

    dvz_drp2_stream_destroy(stream);
    dvz_frame_plan_destroy(plan);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify ordinary blended alpha stays on the final target with a source-over blend pipeline.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_visual_alpha_mode_standard_blend(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    DvzVisual* opaque = dvz_point(scene, 0);
    DvzVisual* blended = dvz_point(scene, 0);
    AT(opaque != NULL);
    AT(blended != NULL);

    float positions[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f,
    };
    DvzColor opaque_colors[3] = {{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}};
    DvzColor blended_colors[3] = {{255, 0, 0, 128}, {0, 255, 0, 128}, {0, 0, 255, 128}};
    float sizes[3] = {10.0f, 12.0f, 14.0f};

    AT(dvz_visual_set_data(opaque, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(opaque, "color", opaque_colors, 3) == 0);
    AT(dvz_visual_set_data(opaque, "size", sizes, 3) == 0);
    AT(dvz_visual_set_data(blended, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(blended, "color", blended_colors, 3) == 0);
    AT(dvz_visual_set_data(blended, "size", sizes, 3) == 0);
    AT(dvz_visual_set_alpha_mode(blended, DVZ_ALPHA_BLENDED) == 0);
    AT(dvz_panel_add_visual(panel, opaque, NULL) == 0);
    AT(dvz_panel_add_visual(panel, blended, NULL) == 0);

    DvzFramePlan* plan = dvz_frame_plan("figure.alpha.standard", 0);
    ANN(plan);
    _scene_emit_panel_render(figure, 0, plan, "figure_0");
    AT(dvz_frame_plan_node_count(plan) == 1);
    const DvzFramePlanNode* render_node = dvz_frame_plan_node_get(plan, 0);
    ANN(render_node);
    AT(dvz_frame_plan_render_pass_role(render_node) == DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE);
    AT(render_node->u.render.visual_count == 2);
    AT(render_node->u.render.visual_metadata[1].alpha_mode == DVZ_ALPHA_BLENDED);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_figure_emit(figure, &caps, &report);
    AT(stream == NULL);
    AT(dvz_diagnostic_report_count(&report) == 1);
    const char* message = dvz_diagnostic_report_get(&report, 0);
    AT(message != NULL);
    AT(strstr(message, "alpha blending requires") != NULL);

    caps.supports_color_blending = true;
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.target_width = 64;
    cfg.target_height = 64;
    dvz_diagnostic_report_init(&report);

    stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);
    DvzDrp2ValidationResult validation = dvz_drp2_validate_stream(stream);
    AT(validation.ok);

    bool has_standard_blend_pipeline = false;
    uint32_t begin_pass_count = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        ANN(command);
        if (command->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            has_standard_blend_pipeline =
                has_standard_blend_pipeline ||
                (command->u.create_render_pipeline.color_targets[0].blend_enabled &&
                 command->u.create_render_pipeline.color_targets[0].src_color_blend_factor ==
                     VK_BLEND_FACTOR_SRC_ALPHA &&
                 command->u.create_render_pipeline.color_targets[0].dst_color_blend_factor ==
                     VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
        }
        else if (command->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS)
            begin_pass_count++;
    }
    AT(has_standard_blend_pipeline);
    AT(begin_pass_count == 1);

    dvz_drp2_stream_destroy(stream);
    dvz_frame_plan_destroy(plan);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify visual alpha mode splits retained panel rendering into WBOIT pass roles.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_visual_alpha_mode_splits_frame_plan_passes(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    DvzVisual* opaque = dvz_point(scene, 0);
    DvzVisual* transparent = dvz_point(scene, 0);
    AT(opaque != NULL);
    AT(transparent != NULL);

    float positions[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f,
    };
    DvzColor opaque_colors[3] = {{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}};
    DvzColor transparent_colors[3] = {{255, 0, 0, 128}, {0, 255, 0, 128}, {0, 0, 255, 128}};
    float sizes[3] = {10.0f, 12.0f, 14.0f};

    AT(dvz_visual_set_data(opaque, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(opaque, "color", opaque_colors, 3) == 0);
    AT(dvz_visual_set_data(opaque, "size", sizes, 3) == 0);
    AT(dvz_visual_set_data(transparent, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(transparent, "color", transparent_colors, 3) == 0);
    AT(dvz_visual_set_data(transparent, "size", sizes, 3) == 0);
    AT(dvz_visual_set_alpha_mode(transparent, DVZ_ALPHA_WBOIT) == 0);
    AT(dvz_panel_add_visual(panel, opaque, NULL) == 0);
    AT(dvz_panel_add_visual(panel, transparent, NULL) == 0);

    DvzFramePlan* plan = dvz_frame_plan("figure.alpha.split", 0);
    ANN(plan);
    _scene_emit_panel_render(figure, 0, plan, "figure_0");

    AT(dvz_frame_plan_node_count(plan) == 3);
    const DvzFramePlanNode* opaque_node = dvz_frame_plan_node_get(plan, 0);
    const DvzFramePlanNode* accum_node = dvz_frame_plan_node_get(plan, 1);
    const DvzFramePlanNode* resolve_node = dvz_frame_plan_node_get(plan, 2);
    ANN(opaque_node);
    ANN(accum_node);
    ANN(resolve_node);
    AT(dvz_frame_plan_render_pass_role(opaque_node) == DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE);
    AT(
        dvz_frame_plan_render_pass_role(accum_node) ==
        DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION);
    AT(dvz_frame_plan_render_pass_role(resolve_node) == DVZ_FRAME_PLAN_RENDER_PASS_WBOIT_RESOLVE);
    AT(opaque_node->u.render.visual_count == 1);
    AT(accum_node->u.render.visual_count == 1);
    AT(resolve_node->u.render.visual_count == 0);
    AT(opaque_node->u.render.visual_metadata[0].alpha_mode == DVZ_ALPHA_OPAQUE);
    AT(accum_node->u.render.visual_metadata[0].alpha_mode == DVZ_ALPHA_WBOIT);
    AT(strcmp(opaque_node->u.render.render_target_id, "rt") == 0);
    AT(strcmp(accum_node->u.render.render_target_id, "rt.wboit_accum") == 0);
    AT(strcmp(resolve_node->u.render.render_target_id, "rt") == 0);
    AT(dvz_frame_plan_graph_resource_count(plan) == 4);
    AT(dvz_frame_plan_graph_pass_count(plan) == 3);

    const DvzFrameGraphResource* accum_resource = dvz_frame_plan_graph_resource_get(plan, 1);
    const DvzFrameGraphResource* weight_resource = dvz_frame_plan_graph_resource_get(plan, 2);
    const DvzFrameGraphResource* depth_resource = dvz_frame_plan_graph_resource_get(plan, 3);
    ANN(accum_resource);
    ANN(weight_resource);
    ANN(depth_resource);
    AT(strcmp(accum_resource->id, "figure_0_p0.wboit.accum") == 0);
    AT(strcmp(weight_resource->id, "figure_0_p0.wboit.weight") == 0);
    AT(strcmp(depth_resource->id, "figure_0_p0.depth") == 0);
    AT(accum_resource->usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT);
    AT(accum_resource->usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED);
    AT(depth_resource->usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_DEPTH_ATTACHMENT);
    AT(depth_resource->usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED);

    const DvzFrameGraphPass* opaque_pass = dvz_frame_plan_graph_pass_get(plan, 0);
    const DvzFrameGraphPass* accum_pass = dvz_frame_plan_graph_pass_get(plan, 1);
    const DvzFrameGraphPass* resolve_pass = dvz_frame_plan_graph_pass_get(plan, 2);
    ANN(opaque_pass);
    ANN(accum_pass);
    ANN(resolve_pass);
    AT(strcmp(opaque_pass->work_label, "opaque") == 0);
    AT(strcmp(accum_pass->work_label, "wboit_accum") == 0);
    AT(strcmp(resolve_pass->work_label, "wboit_resolve") == 0);
    AT(opaque_pass->has_depth_attachment);
    AT(accum_pass->color_attachment_count == 2);
    AT(accum_pass->has_depth_attachment);
    AT(resolve_pass->read_count == 2);
    AT(resolve_pass->color_attachment_count == 1);
    AT(dvz_frame_plan_graph_dependency_count(plan) == 4);
    bool has_accum_resolve_dependency = false;
    bool has_depth_dependency = false;
    for (uint32_t i = 0; i < dvz_frame_plan_graph_dependency_count(plan); i++)
    {
        DvzFrameGraphDependency dep = {0};
        AT(dvz_frame_plan_graph_dependency_get(plan, i, &dep));
        if (
            strcmp(dep.producer_pass_id, "figure_0_p0.wboit.accum") == 0 &&
            strcmp(dep.consumer_pass_id, "figure_0_p0.wboit.resolve") == 0)
            has_accum_resolve_dependency = true;
        if (
            strcmp(dep.producer_pass_id, "figure_0_p0.opaque") == 0 &&
            strcmp(dep.consumer_pass_id, "figure_0_p0.wboit.accum") == 0 &&
            strcmp(dep.resource_id, "figure_0_p0.depth") == 0)
            has_depth_dependency = true;
    }
    AT(has_accum_resolve_dependency);
    AT(has_depth_dependency);

    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    AT(dvz_frame_plan_graph_validate(plan, &report));
    AT(dvz_diagnostic_report_count(&report) == 0);

    dvz_frame_plan_destroy(plan);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify depth-peel alpha mode expands retained panel rendering into graph passes.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_visual_alpha_mode_depth_peel_frame_plan(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    DvzVisual* opaque = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    DvzVisual* transparent = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    AT(opaque != NULL);
    AT(transparent != NULL);

    float positions[3][3] = {
        {-0.5f, -0.5f, 0.0f},
        {0.5f, -0.5f, 0.0f},
        {0.0f, 0.5f, 0.0f},
    };
    DvzColor opaque_colors[3] = {
        {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255}};
    DvzColor transparent_colors[3] = {
        {255, 0, 0, 128}, {0, 255, 0, 128}, {0, 0, 255, 128}};

    AT(dvz_visual_set_data(opaque, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(opaque, "color", opaque_colors, 3) == 0);
    AT(dvz_visual_set_data(transparent, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(transparent, "color", transparent_colors, 3) == 0);
    AT(dvz_visual_set_alpha_mode(transparent, DVZ_ALPHA_DEPTH_PEEL) == 0);
    AT(dvz_panel_add_visual(panel, opaque, NULL) == 0);
    AT(dvz_panel_add_visual(panel, transparent, NULL) == 0);

    DvzFramePlan* plan = dvz_frame_plan("figure.alpha.depth_peel", 0);
    ANN(plan);
    _scene_emit_panel_render(figure, 0, plan, "figure_0");

    AT(dvz_frame_plan_node_count(plan) == 4);
    const DvzFramePlanNode* opaque_node = dvz_frame_plan_node_get(plan, 0);
    const DvzFramePlanNode* init_node = dvz_frame_plan_node_get(plan, 1);
    const DvzFramePlanNode* iter_node = dvz_frame_plan_node_get(plan, 2);
    const DvzFramePlanNode* composite_node = dvz_frame_plan_node_get(plan, 3);
    ANN(opaque_node);
    ANN(init_node);
    ANN(iter_node);
    ANN(composite_node);
    AT(dvz_frame_plan_render_pass_role(opaque_node) == DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE);
    AT(dvz_frame_plan_render_pass_role(init_node) == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT);
    AT(dvz_frame_plan_render_pass_role(iter_node) == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER);
    AT(
        dvz_frame_plan_render_pass_role(composite_node) ==
        DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_COMPOSITE);
    AT(opaque_node->u.render.visual_count == 1);
    AT(init_node->u.render.visual_count == 1);
    AT(iter_node->u.render.visual_count == 1);
    AT(composite_node->u.render.visual_count == 0);
    AT(init_node->u.render.visual_metadata[0].alpha_mode == DVZ_ALPHA_DEPTH_PEEL);
    AT(iter_node->u.render.visual_metadata[0].alpha_mode == DVZ_ALPHA_DEPTH_PEEL);

    AT(dvz_frame_plan_graph_resource_count(plan) == 8);
    AT(dvz_frame_plan_graph_pass_count(plan) == 4);
    const DvzFrameGraphResource* depth_resource = dvz_frame_plan_graph_resource_get(plan, 1);
    const DvzFrameGraphResource* front_ping = dvz_frame_plan_graph_resource_get(plan, 2);
    ANN(depth_resource);
    ANN(front_ping);
    AT(strcmp(depth_resource->id, "figure_0_p0.depth.opaque") == 0);
    AT(depth_resource->format == VK_FORMAT_D32_SFLOAT);
    AT(strcmp(front_ping->id, "figure_0_p0.peel.front_ping") == 0);
    AT(front_ping->format == VK_FORMAT_R16G16B16A16_SFLOAT);

    const DvzFrameGraphPass* init_pass = dvz_frame_plan_graph_pass_get(plan, 1);
    const DvzFrameGraphPass* iter_pass = dvz_frame_plan_graph_pass_get(plan, 2);
    const DvzFrameGraphPass* composite_pass = dvz_frame_plan_graph_pass_get(plan, 3);
    ANN(init_pass);
    ANN(iter_pass);
    ANN(composite_pass);
    AT(strcmp(init_pass->work_label, "depth_peel_init") == 0);
    AT(strcmp(iter_pass->work_label, "depth_peel_iter") == 0);
    AT(strcmp(composite_pass->work_label, "depth_peel_composite") == 0);
    AT(init_pass->color_attachment_count == 3);
    AT(iter_pass->read_count == 0);
    AT(iter_pass->color_attachment_count == 3);
    AT(composite_pass->read_count == 3);
    AT(strcmp(composite_pass->reads[0].resource_id, "figure_0_p0.peel.front_ping") == 0);
    AT(composite_pass->color_attachment_count == 1);

    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    AT(dvz_frame_plan_graph_validate(plan, &report));
    AT(dvz_diagnostic_report_count(&report) == 0);

    dvz_frame_plan_destroy(plan);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify depth-peel alpha mode lowers to an executable DRP2 multi-pass shape.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_visual_alpha_mode_emits_depth_peel_drp2(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    DvzVisual* opaque = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    DvzVisual* transparent = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    AT(opaque != NULL);
    AT(transparent != NULL);

    float positions[3][3] = {
        {-0.5f, -0.5f, 0.0f},
        {0.5f, -0.5f, 0.0f},
        {0.0f, 0.5f, 0.0f},
    };
    DvzColor opaque_colors[3] = {
        {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255}};
    DvzColor transparent_colors[3] = {
        {255, 0, 0, 128}, {0, 255, 0, 128}, {0, 0, 255, 128}};

    AT(dvz_visual_set_data(opaque, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(opaque, "color", opaque_colors, 3) == 0);
    AT(dvz_visual_set_data(transparent, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(transparent, "color", transparent_colors, 3) == 0);
    AT(dvz_visual_set_alpha_mode(transparent, DVZ_ALPHA_DEPTH_PEEL) == 0);
    AT(dvz_panel_add_visual(panel, opaque, NULL) == 0);
    AT(dvz_panel_add_visual(panel, transparent, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.max_color_attachments = 3;
    caps.render_target_format_rgba16float = true;
    caps.supports_render_target_sampling = true;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.target_width = 64;
    cfg.target_height = 64;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);
    DvzDrp2ValidationResult validation = dvz_drp2_validate_stream(stream);
    AT(validation.ok);

    bool has_front_ping = false;
    bool has_front_pong = false;
    bool has_depth_texture = false;
    bool has_three_target_pipeline = false;
    bool has_composite_pipeline = false;
    bool has_blended_composite_pipeline = false;
    bool has_composite_bind_group = false;
    uint32_t begin_pass_count = 0;
    uint32_t triple_attachment_passes = 0;
    uint32_t sampled_bind_group_count = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        ANN(command);
        if (command->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE)
        {
            const char* label = dvz_drp2_stream_label(stream, command->u.create_texture.id);
            has_front_ping =
                has_front_ping || (label != NULL &&
                                   strcmp(label, "fig0_p0.peel.front_ping") == 0);
            has_front_pong =
                has_front_pong || (label != NULL &&
                                   strcmp(label, "fig0_p0.peel.front_pong") == 0);
            has_depth_texture =
                has_depth_texture ||
                (label != NULL && strcmp(label, "fig0_p0.depth.opaque") == 0 &&
                 command->u.create_texture.format == VK_FORMAT_D32_SFLOAT);
        }
        else if (command->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            has_three_target_pipeline =
                has_three_target_pipeline ||
                command->u.create_render_pipeline.color_target_count == 3;
            has_composite_pipeline =
                has_composite_pipeline ||
                (command->u.create_render_pipeline.bind_group_layout_count == 1 &&
                 command->u.create_render_pipeline.vertex_buffer_slots == 0);
            has_blended_composite_pipeline =
                has_blended_composite_pipeline ||
                (command->u.create_render_pipeline.bind_group_layout_count == 1 &&
                 command->u.create_render_pipeline.vertex_buffer_slots == 0 &&
                 command->u.create_render_pipeline.color_targets[0].blend_enabled &&
                 command->u.create_render_pipeline.color_targets[0].src_color_blend_factor ==
                     VK_BLEND_FACTOR_SRC_ALPHA);
        }
        else if (command->type == DVZ_DRP2_COMMAND_CREATE_BIND_GROUP)
        {
            if (command->u.create_bind_group.entry_count == 4)
                sampled_bind_group_count++;
            has_composite_bind_group = has_composite_bind_group ||
                                       command->u.create_bind_group.entry_count == 4;
        }
        else if (command->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS)
        {
            begin_pass_count++;
            if (command->u.begin_render_pass.color_attachment_count == 3)
                triple_attachment_passes++;
        }
    }

    AT(has_front_ping);
    AT(has_front_pong);
    AT(has_depth_texture);
    AT(has_three_target_pipeline);
    AT(has_composite_pipeline);
    AT(has_blended_composite_pipeline);
    AT(has_composite_bind_group);
    AT(sampled_bind_group_count >= 1);
    AT(begin_pass_count == 4);
    AT(triple_attachment_passes == 2);

    dvz_diagnostic_report_init(&report);
    cfg.runtime_resource_scope_id = UINT64_C(0x7b);
    DvzDrp2CommandStream* scoped_stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    ANN(scoped_stream);
    AT(dvz_diagnostic_report_count(&report) == 0);

    bool has_scoped_front_ping = false;
    bool has_scoped_composite_bind_group = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(scoped_stream); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(scoped_stream, i);
        ANN(command);
        if (command->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE)
        {
            const char* label = dvz_drp2_stream_label(scoped_stream, command->u.create_texture.id);
            has_scoped_front_ping =
                has_scoped_front_ping ||
                (label != NULL &&
                 strcmp(label, "fig0_p0.peel.front_ping_scope_000000000000007b") == 0);
        }
        else if (command->type == DVZ_DRP2_COMMAND_CREATE_BIND_GROUP)
        {
            const char* label =
                dvz_drp2_stream_label(scoped_stream, command->u.create_bind_group.id);
            has_scoped_composite_bind_group =
                has_scoped_composite_bind_group ||
                (label != NULL &&
                 strcmp(label, "_bg_depth_peel_composite_scope_000000000000007b") == 0);
        }
    }
    AT(has_scoped_front_ping);
    AT(has_scoped_composite_bind_group);

    dvz_diagnostic_report_init(&report);
    cfg.target_width = 96;
    cfg.target_height = 48;
    DvzDrp2CommandStream* scoped_resize_stream =
        dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    ANN(scoped_resize_stream);
    AT(dvz_diagnostic_report_count(&report) == 0);

    bool resized_front_ping = false;
    bool rebuilt_composite_bind_group = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(scoped_resize_stream); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(scoped_resize_stream, i);
        ANN(command);
        if (command->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE)
        {
            const char* label =
                dvz_drp2_stream_label(scoped_resize_stream, command->u.create_texture.id);
            resized_front_ping =
                resized_front_ping ||
                (label != NULL &&
                 strcmp(label, "fig0_p0.peel.front_ping_scope_000000000000007b") == 0 &&
                 command->u.create_texture.width == 96 &&
                 command->u.create_texture.height == 48);
        }
        else if (command->type == DVZ_DRP2_COMMAND_CREATE_BIND_GROUP)
        {
            const char* label =
                dvz_drp2_stream_label(scoped_resize_stream, command->u.create_bind_group.id);
            rebuilt_composite_bind_group =
                rebuilt_composite_bind_group ||
                (label != NULL &&
                 strcmp(label, "_bg_depth_peel_composite_scope_000000000000007b") == 0 &&
                 command->u.create_bind_group.id != 0);
        }
    }
    AT(resized_front_ping);
    AT(!rebuilt_composite_bind_group);

    DvzDrp2RuntimeConfig runtime_cfg = dvz_drp2_runtime_vklite_config(NULL, NULL);
    runtime_cfg.semantic_only = true;
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);
    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    result = dvz_drp2_runtime_execute(runtime, scoped_stream);
    AT(result.ok);
    result = dvz_drp2_runtime_execute(runtime, scoped_resize_stream);
    if (!result.ok)
    {
        const DvzDrp2Command* failed =
            dvz_drp2_stream_get(scoped_resize_stream, result.command_index);
        log_error(
            "depth peel resize stream failed: code=%d command=%" PRIu32 " type=%d", result.code,
            result.command_index, failed != NULL ? (int)failed->type : -1);
        return 1;
    }
    dvz_drp2_runtime_destroy(runtime);

    dvz_drp2_stream_destroy(scoped_resize_stream);
    dvz_drp2_stream_destroy(scoped_stream);
    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify WBOIT alpha requests require explicit WBOIT-capable runtime facts.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_visual_alpha_mode_requires_wboit_capabilities(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    AT(panel != NULL);
    DvzVisual* visual = dvz_point(scene, 0);
    AT(visual != NULL);

    float positions[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f,
    };
    DvzColor colors[3] = {{255, 0, 0, 128}, {0, 255, 0, 128}, {0, 0, 255, 128}};
    float sizes[3] = {10.0f, 12.0f, 14.0f};

    AT(dvz_visual_set_data(visual, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 3) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 3) == 0);
    AT(dvz_visual_set_alpha_mode(visual, DVZ_ALPHA_WBOIT) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_figure_emit(figure, &caps, &report);
    AT(stream == NULL);
    AT(dvz_diagnostic_report_count(&report) == 1);
    const char* message = dvz_diagnostic_report_get(&report, 0);
    AT(message != NULL);
    AT(strstr(message, "WBOIT requires") != NULL);

    caps.max_color_attachments = 2;
    caps.render_target_format_rgba16float = true;
    caps.render_target_format_r16float = true;
    caps.supports_render_target_sampling = true;
    caps.supports_color_blending = true;
    dvz_diagnostic_report_init(&report);

    stream = dvz_figure_emit(figure, &caps, &report);
    AT(stream != NULL);
    AT(dvz_diagnostic_report_count(&report) == 0);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify WBOIT primitive visuals lower to an explicit WBOIT DRP2 command shape.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_visual_alpha_mode_emits_wboit_drp2(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    DvzVisual* opaque = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    DvzVisual* transparent = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    AT(opaque != NULL);
    AT(transparent != NULL);

    float positions[3][3] = {
        {-0.5f, -0.5f, 0.0f},
        {0.5f, -0.5f, 0.0f},
        {0.0f, 0.5f, 0.0f},
    };
    DvzColor opaque_colors[3] = {
        {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255}};
    DvzColor transparent_colors[3] = {
        {255, 0, 0, 128}, {0, 255, 0, 128}, {0, 0, 255, 128}};

    AT(dvz_visual_set_data(opaque, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(opaque, "color", opaque_colors, 3) == 0);
    AT(dvz_visual_set_data(transparent, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(transparent, "color", transparent_colors, 3) == 0);
    AT(dvz_visual_set_alpha_mode(transparent, DVZ_ALPHA_WBOIT) == 0);
    AT(dvz_panel_add_visual(panel, opaque, NULL) == 0);
    AT(dvz_panel_add_visual(panel, transparent, NULL) == 0);
    dvz_panel_set_background_color(panel, 0.05f, 0.05f, 0.08f, 1.0f);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.max_color_attachments = 2;
    caps.render_target_format_rgba16float = true;
    caps.render_target_format_r16float = true;
    caps.supports_render_target_sampling = true;
    caps.supports_color_blending = true;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.target_width = 64;
    cfg.target_height = 64;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);
    DvzDrp2ValidationResult validation = dvz_drp2_validate_stream(stream);
    AT(validation.ok);

    bool has_accum_texture = false;
    bool has_weight_texture = false;
    bool has_named_depth_texture = false;
    bool has_graph_accum_texture = false;
    bool has_graph_weight_texture = false;
    bool has_graph_accum_usage = false;
    bool has_graph_weight_usage = false;
    bool has_graph_depth_usage = false;
    uint64_t graph_accum_texture_id = 0;
    uint64_t graph_weight_texture_id = 0;
    bool has_accum_pipeline = false;
    bool has_resolve_pipeline = false;
    bool has_opaque_depth_pipeline = false;
    bool has_fixed_background_depth_pipeline = false;
    bool has_accum_pass = false;
    bool has_resolve_bind_group = false;
    bool resolve_bind_group_samples_graph_targets = false;
    uint32_t begin_pass_count = 0;
    uint64_t begin_pass_textures[3] = {0};
    uint32_t begin_pass_color_counts[3] = {0};
    bool begin_pass_clears[3] = {0};
    bool begin_pass_depths[3] = {0};
    uint64_t begin_pass_depth_textures[3] = {0};
    DvzDrp2AttachmentLoadOp begin_pass_depth_loads[3] = {0};
    DvzDrp2AttachmentAccess begin_pass_depth_access[3] = {0};
    uint64_t begin_pass_second_attachment_textures[3] = {0};
    bool begin_pass_second_attachment_clears[3] = {0};

    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        ANN(command);
        if (command->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE)
        {
            has_accum_texture =
                has_accum_texture ||
                command->u.create_texture.format == VK_FORMAT_R16G16B16A16_SFLOAT;
            has_weight_texture =
                has_weight_texture || command->u.create_texture.format == VK_FORMAT_R16_SFLOAT;
            const char* label = dvz_drp2_stream_label(stream, command->u.create_texture.id);
            has_named_depth_texture =
                has_named_depth_texture ||
                (label != NULL && strcmp(label, "fig0_p0.depth") == 0 &&
                 command->u.create_texture.format == VK_FORMAT_D32_SFLOAT);
            has_graph_accum_texture =
                has_graph_accum_texture ||
                (label != NULL && strcmp(label, "fig0_p0.wboit.accum") == 0);
            if (label != NULL && strcmp(label, "fig0_p0.wboit.accum") == 0)
                graph_accum_texture_id = command->u.create_texture.id;
            has_graph_weight_texture =
                has_graph_weight_texture ||
                (label != NULL && strcmp(label, "fig0_p0.wboit.weight") == 0);
            if (label != NULL && strcmp(label, "fig0_p0.wboit.weight") == 0)
                graph_weight_texture_id = command->u.create_texture.id;
            has_graph_accum_usage =
                has_graph_accum_usage ||
                (label != NULL && strcmp(label, "fig0_p0.wboit.accum") == 0 &&
                 (command->u.create_texture.usage &
                  (DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT |
                   DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING)) ==
                     (DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT |
                      DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING));
            has_graph_weight_usage =
                has_graph_weight_usage ||
                (label != NULL && strcmp(label, "fig0_p0.wboit.weight") == 0 &&
                 (command->u.create_texture.usage &
                  (DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT |
                   DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING)) ==
                     (DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT |
                      DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING));
            has_graph_depth_usage =
                has_graph_depth_usage ||
                (label != NULL && strcmp(label, "fig0_p0.depth") == 0 &&
                 (command->u.create_texture.usage &
                  DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT) != 0);
        }
        else if (command->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            has_accum_pipeline =
                has_accum_pipeline ||
                (command->u.create_render_pipeline.color_target_count == 2 &&
                 command->u.create_render_pipeline.color_targets[0].blend_enabled &&
                 command->u.create_render_pipeline.color_targets[1].blend_enabled);
            has_resolve_pipeline =
                has_resolve_pipeline ||
                (command->u.create_render_pipeline.color_target_count == 1 &&
                 command->u.create_render_pipeline.color_targets[0].blend_enabled &&
                 command->u.create_render_pipeline.color_targets[0].src_color_blend_factor ==
                     VK_BLEND_FACTOR_SRC_ALPHA);
            has_opaque_depth_pipeline =
                has_opaque_depth_pipeline ||
                (command->u.create_render_pipeline.has_depth_attachment &&
                 command->u.create_render_pipeline.depth_write_enabled &&
                 command->u.create_render_pipeline.depth_compare_op == VK_COMPARE_OP_LESS_OR_EQUAL &&
                 command->u.create_render_pipeline.vertex_buffer_slots == 2 &&
                 command->u.create_render_pipeline.topology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
            has_fixed_background_depth_pipeline =
                has_fixed_background_depth_pipeline ||
                (command->u.create_render_pipeline.has_depth_attachment &&
                 !command->u.create_render_pipeline.depth_write_enabled &&
                 command->u.create_render_pipeline.depth_compare_op == VK_COMPARE_OP_ALWAYS &&
                 command->u.create_render_pipeline.vertex_buffer_slots == 2 &&
                 command->u.create_render_pipeline.topology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
        }
        else if (command->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS)
        {
            if (begin_pass_count < 3)
            {
                begin_pass_textures[begin_pass_count] =
                    command->u.begin_render_pass.texture_id;
                begin_pass_color_counts[begin_pass_count] =
                    command->u.begin_render_pass.color_attachment_count;
                begin_pass_clears[begin_pass_count] = command->u.begin_render_pass.clear;
                begin_pass_depths[begin_pass_count] =
                    command->u.begin_render_pass.has_depth_attachment;
                begin_pass_depth_textures[begin_pass_count] =
                    command->u.begin_render_pass.depth_texture_id;
                begin_pass_depth_loads[begin_pass_count] =
                    command->u.begin_render_pass.depth_load_op;
                begin_pass_depth_access[begin_pass_count] =
                    command->u.begin_render_pass.depth_access;
                if (command->u.begin_render_pass.color_attachment_count > 1)
                {
                    begin_pass_second_attachment_textures[begin_pass_count] =
                        command->u.begin_render_pass.color_attachments[1].texture_id;
                    begin_pass_second_attachment_clears[begin_pass_count] =
                        command->u.begin_render_pass.color_attachments[1].clear;
                }
            }
            begin_pass_count++;
            has_accum_pass =
                has_accum_pass || command->u.begin_render_pass.color_attachment_count == 2;
        }
        else if (command->type == DVZ_DRP2_COMMAND_CREATE_BIND_GROUP)
        {
            has_resolve_bind_group =
                has_resolve_bind_group || command->u.create_bind_group.entry_count == 3;
            if (command->u.create_bind_group.entry_count == 3)
            {
                resolve_bind_group_samples_graph_targets =
                    resolve_bind_group_samples_graph_targets ||
                    (command->u.create_bind_group.entries[0].resource_id ==
                         graph_accum_texture_id &&
                     command->u.create_bind_group.entries[1].resource_id ==
                         graph_weight_texture_id);
            }
        }
    }

    AT(has_accum_texture);
    AT(has_weight_texture);
    AT(has_named_depth_texture);
    AT(has_graph_accum_texture);
    AT(has_graph_weight_texture);
    AT(graph_accum_texture_id != 0);
    AT(graph_weight_texture_id != 0);
    AT(has_graph_accum_usage);
    AT(has_graph_weight_usage);
    AT(has_graph_depth_usage);
    AT(has_accum_pipeline);
    AT(has_resolve_pipeline);
    AT(has_opaque_depth_pipeline);
    AT(has_fixed_background_depth_pipeline);
    AT(has_accum_pass);
    AT(has_resolve_bind_group);
    AT(resolve_bind_group_samples_graph_targets);
    AT(begin_pass_count == 3);
    AT(begin_pass_color_counts[0] == 1);
    AT(begin_pass_color_counts[1] == 2);
    AT(begin_pass_color_counts[2] == 1);
    AT(begin_pass_textures[1] == graph_accum_texture_id);
    AT(begin_pass_second_attachment_textures[1] == graph_weight_texture_id);
    AT(begin_pass_textures[1] != begin_pass_textures[0]);
    AT(begin_pass_textures[1] != begin_pass_textures[2]);
    AT(begin_pass_clears[0]);
    AT(begin_pass_clears[1]);
    AT(begin_pass_depths[0]);
    AT(begin_pass_depths[1]);
    AT(!begin_pass_depths[2]);
    AT(begin_pass_depth_textures[0] != 0);
    AT(begin_pass_depth_textures[0] == begin_pass_depth_textures[1]);
    AT(begin_pass_depth_textures[2] == 0);
    AT(begin_pass_depth_loads[0] == DVZ_DRP2_ATTACHMENT_LOAD_CLEAR);
    AT(begin_pass_depth_loads[1] == DVZ_DRP2_ATTACHMENT_LOAD_LOAD);
    AT(begin_pass_depth_access[0] == DVZ_DRP2_ATTACHMENT_ACCESS_WRITE);
    AT(begin_pass_depth_access[1] == DVZ_DRP2_ATTACHMENT_ACCESS_READ);
    AT(begin_pass_second_attachment_clears[1]);
    AT(!begin_pass_clears[2]);
    AT(begin_pass_textures[0] != 0);
    AT(begin_pass_textures[0] == begin_pass_textures[2]);
    AT(begin_pass_textures[1] != begin_pass_textures[0]);

    dvz_diagnostic_report_init(&report);
    cfg.runtime_resource_scope_id = UINT64_C(0x7c);
    DvzDrp2CommandStream* scoped_stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    ANN(scoped_stream);
    AT(dvz_diagnostic_report_count(&report) == 0);

    uint64_t scoped_accum_texture_id = 0;
    uint64_t scoped_weight_texture_id = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(scoped_stream); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(scoped_stream, i);
        ANN(command);
        if (command->type != DVZ_DRP2_COMMAND_CREATE_TEXTURE)
            continue;
        const char* label = dvz_drp2_stream_label(scoped_stream, command->u.create_texture.id);
        if (label != NULL &&
            strcmp(label, "fig0_p0.wboit.accum_scope_000000000000007c") == 0)
            scoped_accum_texture_id = command->u.create_texture.id;
        else if (
            label != NULL &&
            strcmp(label, "fig0_p0.wboit.weight_scope_000000000000007c") == 0)
            scoped_weight_texture_id = command->u.create_texture.id;
    }
    AT(scoped_accum_texture_id != 0);
    AT(scoped_weight_texture_id != 0);

    bool has_scoped_resolve_bind_group = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(scoped_stream); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(scoped_stream, i);
        ANN(command);
        if (command->type != DVZ_DRP2_COMMAND_CREATE_BIND_GROUP ||
            command->u.create_bind_group.entry_count != 3)
            continue;
        has_scoped_resolve_bind_group =
            has_scoped_resolve_bind_group ||
            (command->u.create_bind_group.entries[0].resource_id == scoped_accum_texture_id &&
             command->u.create_bind_group.entries[1].resource_id == scoped_weight_texture_id);
    }
    AT(has_scoped_resolve_bind_group);

    dvz_diagnostic_report_init(&report);
    cfg.target_width = 96;
    cfg.target_height = 48;
    DvzDrp2CommandStream* scoped_resize_stream =
        dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    ANN(scoped_resize_stream);
    AT(dvz_diagnostic_report_count(&report) == 0);

    uint64_t resized_accum_texture_id = 0;
    uint64_t resized_weight_texture_id = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(scoped_resize_stream); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(scoped_resize_stream, i);
        ANN(command);
        if (command->type != DVZ_DRP2_COMMAND_CREATE_TEXTURE)
            continue;
        const char* label =
            dvz_drp2_stream_label(scoped_resize_stream, command->u.create_texture.id);
        if (label != NULL &&
            strcmp(label, "fig0_p0.wboit.accum_scope_000000000000007c") == 0 &&
            command->u.create_texture.width == 96 && command->u.create_texture.height == 48)
            resized_accum_texture_id = command->u.create_texture.id;
        else if (
            label != NULL &&
            strcmp(label, "fig0_p0.wboit.weight_scope_000000000000007c") == 0 &&
            command->u.create_texture.width == 96 && command->u.create_texture.height == 48)
            resized_weight_texture_id = command->u.create_texture.id;
    }
    AT(resized_accum_texture_id != 0);
    AT(resized_weight_texture_id != 0);

    bool rebuilt_resolve_bind_group = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(scoped_resize_stream); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(scoped_resize_stream, i);
        ANN(command);
        if (command->type != DVZ_DRP2_COMMAND_CREATE_BIND_GROUP ||
            command->u.create_bind_group.entry_count != 3)
            continue;
        rebuilt_resolve_bind_group =
            rebuilt_resolve_bind_group ||
            (command->u.create_bind_group.entries[0].resource_id == resized_accum_texture_id &&
             command->u.create_bind_group.entries[1].resource_id == resized_weight_texture_id);
    }
    AT(!rebuilt_resolve_bind_group);

    DvzDrp2RuntimeConfig runtime_cfg = dvz_drp2_runtime_vklite_config(NULL, NULL);
    runtime_cfg.semantic_only = true;
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);
    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    result = dvz_drp2_runtime_execute(runtime, scoped_stream);
    AT(result.ok);
    result = dvz_drp2_runtime_execute(runtime, scoped_resize_stream);
    if (!result.ok)
    {
        const DvzDrp2Command* failed =
            dvz_drp2_stream_get(scoped_resize_stream, result.command_index);
        log_error(
            "WBOIT resize stream failed: code=%d command=%" PRIu32 " type=%d", result.code,
            result.command_index, failed != NULL ? (int)failed->type : -1);
        return 1;
    }
    dvz_drp2_runtime_destroy(runtime);

    dvz_drp2_stream_destroy(scoped_resize_stream);
    dvz_drp2_stream_destroy(scoped_stream);
    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Execute the scene WBOIT DRP2 path through the vklite runtime when a GPU is available.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_visual_alpha_mode_wboit_glsl_executes(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

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
        return 0;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    DvzVisual* transparent = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    AT(transparent != NULL);
    float positions[3][3] = {
        {-0.6f, -0.6f, 0.0f},
        {0.6f, -0.6f, 0.0f},
        {0.0f, 0.6f, 0.0f},
    };
    float normals[3][3] = {
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
    };
    DvzColor colors[3] = {{255, 0, 0, 128}, {0, 255, 0, 128}, {0, 0, 255, 128}};
    AT(dvz_visual_set_data(transparent, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(transparent, "color", colors, 3) == 0);
    AT(dvz_visual_set_data(transparent, "normal", normals, 3) == 0);
    AT(dvz_visual_set_primitive_shading(
           transparent,
           &(DvzPrimitiveShadingDesc){
               .light_direction = {0.0f, 0.0f, 1.0f},
               .ambient = 0.25f,
               .diffuse = 0.75f,
           }) == 0);
    AT(dvz_visual_set_alpha_mode(transparent, DVZ_ALPHA_WBOIT) == 0);
    AT(dvz_panel_add_visual(panel, transparent, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.max_color_attachments = 2;
    caps.render_target_format_rgba16float = true;
    caps.render_target_format_r16float = true;
    caps.supports_render_target_sampling = true;
    caps.supports_color_blending = true;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.target_width = 64;
    cfg.target_height = 64;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);

    uint64_t final_target_id = 0;
    uint32_t begin_render_pass_count = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        ANN(command);
        if (command->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS)
        {
            begin_render_pass_count++;
            if (final_target_id == 0)
                final_target_id = command->u.begin_render_pass.texture_id;
        }
    }
    AT(begin_render_pass_count == 3);
    AT(final_target_id != 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    const uint64_t readback_buffer_id = 9001;
    const uint64_t encoder_id = 9002;
    const uint64_t command_buffer_id = 9003;
    const uint64_t submission_id = 9004;
    const uint32_t width = 64;
    const uint32_t height = 64;
    const uint64_t byte_size = width * height * 4;
    DvzDrp2CommandStream* readback = dvz_drp2_stream();
    ANN(readback);
    AT(dvz_drp2_stream_create_buffer(
        readback, readback_buffer_id, byte_size,
        DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ));
    AT(dvz_drp2_stream_begin_command_encoder(readback, encoder_id));
    AT(dvz_drp2_stream_copy_texture_to_buffer(
        readback, encoder_id, final_target_id, readback_buffer_id, 0, width, height,
        width * 4, height));
    AT(dvz_drp2_stream_finish_command_encoder(readback, encoder_id, command_buffer_id));
    AT(dvz_drp2_stream_queue_submit(readback, command_buffer_id, submission_id));

    result = dvz_drp2_runtime_execute(runtime, readback);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    uint8_t pixels[64 * 64 * 4] = {0};
    AT(_dvz_drp2_runtime_vklite_download_buffer(
        runtime, readback_buffer_id, 0, byte_size, pixels));
    bool has_resolved_color = false;
    for (uint32_t i = 0; i < width * height; i++)
    {
        uint8_t r = pixels[4 * i + 0];
        uint8_t g = pixels[4 * i + 1];
        uint8_t b = pixels[4 * i + 2];
        uint8_t a = pixels[4 * i + 3];
        has_resolved_color = has_resolved_color || (a > 0 && (r > 0 || g > 0 || b > 0));
    }
    AT(has_resolved_color);

    dvz_drp2_stream_destroy(readback);
    dvz_drp2_runtime_destroy(runtime);
    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}


/**
 * Execute the scene depth-peeling DRP2 path through the vklite runtime when a GPU is available.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_visual_alpha_mode_depth_peel_glsl_executes(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

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
        return 0;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    DvzVisual* transparent = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    AT(transparent != NULL);
    dvz_panel_set_background_color(panel, 0.05f, 0.05f, 0.08f, 1.0f);
    float positions[3][3] = {
        {-0.6f, -0.6f, 0.0f},
        {0.6f, -0.6f, 0.0f},
        {0.0f, 0.6f, 0.0f},
    };
    DvzColor colors[3] = {{255, 0, 0, 192}, {0, 255, 0, 192}, {0, 0, 255, 192}};
    AT(dvz_visual_set_data(transparent, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(transparent, "color", colors, 3) == 0);
    AT(dvz_visual_set_alpha_mode(transparent, DVZ_ALPHA_DEPTH_PEEL) == 0);
    AT(dvz_panel_add_visual(panel, transparent, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.max_color_attachments = 3;
    caps.render_target_format_rgba16float = true;
    caps.supports_render_target_sampling = true;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.target_width = 64;
    cfg.target_height = 64;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);

    uint64_t final_target_id = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        ANN(command);
        if (command->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS && final_target_id == 0)
            final_target_id = command->u.begin_render_pass.texture_id;
    }
    AT(final_target_id != 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    const uint64_t readback_buffer_id = 9101;
    const uint64_t encoder_id = 9102;
    const uint64_t command_buffer_id = 9103;
    const uint64_t submission_id = 9104;
    const uint32_t width = 64;
    const uint32_t height = 64;
    const uint64_t byte_size = width * height * 4;
    DvzDrp2CommandStream* readback = dvz_drp2_stream();
    ANN(readback);
    AT(dvz_drp2_stream_create_buffer(
        readback, readback_buffer_id, byte_size,
        DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ));
    AT(dvz_drp2_stream_begin_command_encoder(readback, encoder_id));
    AT(dvz_drp2_stream_copy_texture_to_buffer(
        readback, encoder_id, final_target_id, readback_buffer_id, 0, width, height,
        width * 4, height));
    AT(dvz_drp2_stream_finish_command_encoder(readback, encoder_id, command_buffer_id));
    AT(dvz_drp2_stream_queue_submit(readback, command_buffer_id, submission_id));

    result = dvz_drp2_runtime_execute(runtime, readback);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    uint8_t pixels[64 * 64 * 4] = {0};
    AT(_dvz_drp2_runtime_vklite_download_buffer(
        runtime, readback_buffer_id, 0, byte_size, pixels));
    AT(pixels[0] > 0 || pixels[1] > 0 || pixels[2] > 0);

    dvz_drp2_stream_destroy(readback);
    dvz_drp2_runtime_destroy(runtime);
    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}


/**
 * Register scene graph tests.
 *
 * @param suite the active test suite
 * @return 0 on success
 */
int test_scene_graph(TstSuite* suite)
{
    ANN(suite);
    const char* tags = "scene";

    TEST_SIMPLE(test_scene_point_emit_glsl_executes);
    TEST_SIMPLE(test_scene_point_like_lowering_policy);
    TEST_SIMPLE(test_scene_point_emit_glsl_native_points);
    TEST_SIMPLE(test_scene_point_emit_wgsl_instanced_quads);
    TEST_SIMPLE(test_scene_pixel_emit_wgsl_instanced_quads);
    TEST_SIMPLE(test_scene_primitive_triangle_list_glsl_executes);
    TEST_SIMPLE(test_scene_primitive_line_strip_glsl_executes);
    TEST_SIMPLE(test_scene_primitive_triangle_list_emit_wgsl);
    TEST_SIMPLE(test_scene_mesh_indexed_default_color_emits_draw_indexed);
    TEST_SIMPLE(test_scene_mesh_emits_depth_attachment);
    TEST_SIMPLE(test_scene_indexed_primitive_emits_draw_indexed);
    TEST_SIMPLE(test_scene_shared_index_buffer_emits_one_upload);
    TEST_SIMPLE(test_scene_mesh_glsl_executes);
    TEST_SIMPLE(test_scene_path_glsl_executes);
    TEST_SIMPLE(test_scene_image_glsl_executes);
    TEST_SIMPLE(test_scene_json);
    TEST_SIMPLE(test_scene_json_includes_field_dirty_metadata);
    TEST_SIMPLE(test_scene_json_includes_buffer_binding_metadata);
    TEST_SIMPLE(test_scene_z_layer_orders_emit);
    TEST_SIMPLE(test_scene_background_color_creates_fixed_quad);
    TEST_SIMPLE(test_scene_controller_mode_fixed_emits_separate_mvp);
    TEST_SIMPLE(test_scene_panel_one_pass_per_panel);
    TEST_SIMPLE(test_scene_multi_panel_reuses_fixed_pipeline_and_bind_group_state);
    TEST_SIMPLE(test_scene_multi_panel_glsl_emits_viewport_scissor_commands);
    TEST_SIMPLE(test_scene_rejects_cross_scene_visual);
    TEST_SIMPLE(test_scene_rejects_unsupported_point_attribute);
    TEST_SIMPLE(test_scene_visual_alpha_mode);
    TEST_SIMPLE(test_scene_visual_internal_material_state);
    TEST_SIMPLE(test_scene_visual_pass_capabilities);
    TEST_SIMPLE(test_scene_gbuffer_runtime_lowering);
    TEST_SIMPLE(test_scene_edl_runtime_lowering);
    TEST_SIMPLE(test_scene_visual_alpha_mode_standard_blend);
    TEST_SIMPLE(test_scene_visual_alpha_mode_splits_frame_plan_passes);
    TEST_SIMPLE(test_scene_visual_alpha_mode_depth_peel_frame_plan);
    TEST_SIMPLE(test_scene_visual_alpha_mode_emits_depth_peel_drp2);
    TEST_SIMPLE(test_scene_visual_alpha_mode_requires_wboit_capabilities);
    TEST_SIMPLE(test_scene_visual_alpha_mode_emits_wboit_drp2);
    TEST_SIMPLE(test_scene_visual_alpha_mode_wboit_glsl_executes);
    TEST_SIMPLE(test_scene_visual_alpha_mode_depth_peel_glsl_executes);
    TEST_SIMPLE(test_scene_visual_attr_source_and_mutability_metadata);
    TEST_SIMPLE(test_scene_point_external_position_buffer_emits_no_upload);
    TEST_SIMPLE(test_scene_point_external_position_buffer_executes);
    TEST_SIMPLE(test_scene_point_rejects_texcoords_attribute);
    TEST_SIMPLE(test_scene_primitive_rejects_size_attribute);
    TEST_SIMPLE(test_scene_path_rejects_size_attribute);
    TEST_SIMPLE(test_scene_image_rejects_size_attribute);
    TEST_SIMPLE(test_scene_emit_warns_visual_with_no_position);
    TEST_SIMPLE(test_scene_rejects_mismatched_point_attribute_counts);
    TEST_SIMPLE(test_scene_rejects_range_update_without_full_allocation);
    TEST_SIMPLE(test_scene_rejects_mutation_while_emitted_stream_is_live);
    TEST_SIMPLE(test_scene_rejects_scale_binding_while_emitted_stream_is_live);
    TEST_SIMPLE(test_scene_rejects_range_mutation_while_emitted_stream_is_live);
    TEST_SIMPLE(test_scene_rejects_destroy_while_emitted_stream_is_live);
    TEST_SIMPLE(test_scene_rejects_visual_destroy_while_emitted_stream_is_live);
    TEST_SIMPLE(test_scene_live_stream_count_tracks_multiple_emits);
    TEST_SIMPLE(test_scene_point_emit);
    TEST_SIMPLE(test_scene_path_emit);
    TEST_SIMPLE(test_scene_image_emit);
    TEST_SIMPLE(test_scene_image_emit_wgsl);
    TEST_SIMPLE(test_scene_image_emit_uses_common_and_texture_sets);
    TEST_SIMPLE(test_scene_visual_common_binding_layout_order);
    TEST_SIMPLE(test_scene_empty_figure_emit_clear_only);
    TEST_SIMPLE(test_scene_point_emit_has_vertex_layout);
    TEST_SIMPLE(test_scene_point_visual_resizes_existing_attributes);
    TEST_SIMPLE(test_scene_indexed_primitive_shading_updates_runtime);
    TEST_SIMPLE(test_scene_point_large_count_executes);
    TEST_SIMPLE(test_scene_second_emit_no_uploads_when_not_dirty);
    TEST_SIMPLE(test_scene_partial_update_uploads_only_range);
    TEST_SIMPLE(test_scene_repeated_partial_updates_across_frames);
    TEST_SIMPLE(test_scene_partial_update_merges_ranges_before_emit);
    TEST_SIMPLE(test_scene_multiple_panels_multiple_point_visuals_emit);
    TEST_SIMPLE(test_scene_render_pass_scope_excludes_resource_commands);

    return 0;
}
