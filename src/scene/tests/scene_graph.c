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
#include "../render_contract.h"
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



/**
 * Return whether an emitted stream contains one render pipeline debug label.
 *
 * @param stream the emitted DRP2 stream
 * @param label expected pipeline label
 * @return whether the pipeline label was found
 */
static bool _stream_has_render_pipeline_label(const DvzDrp2CommandStream* stream, const char* label)
{
    ANN(stream);
    ANN(label);
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd == NULL || cmd->type != DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
            continue;
        const char* pipeline_label =
            dvz_drp2_stream_label(stream, cmd->u.create_render_pipeline.id);
        if (pipeline_label != NULL && strcmp(pipeline_label, label) == 0)
            return true;
    }
    return false;
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
 * Execute the scene sphere visual GLSL path through the vklite runtime when available.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_sphere_emit_glsl_executes(TstSuite* suite, TstItem* item)
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
        log_warn("test_scene_sphere_emit_glsl_executes skipped: GPU context creation failed");
        return 0;
    }

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 96, 96, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    DvzVisual* sphere = dvz_sphere(scene, DVZ_SPHERE_FLAGS_LIGHTING);
    AT(sphere != NULL);

    float positions[3][3] = {
        {-0.45f, -0.20f, 0.0f},
        {+0.25f, -0.05f, 0.2f},
        {+0.00f, +0.38f, 0.1f},
    };
    DvzColor colors[3] = {
        {220, 80, 80, 255},
        {80, 190, 120, 255},
        {80, 130, 230, 255},
    };
    float sizes[3] = {0.22f, 0.26f, 0.20f};

    AT(dvz_visual_set_data(sphere, "position", &positions[0][0], 3) == 0);
    AT(dvz_visual_set_data(sphere, "color", colors, 3) == 0);
    AT(dvz_visual_set_data(sphere, "radius", sizes, 3) == 0);
    AT(dvz_panel_add_visual(panel, sphere, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.supports_color_blending = true;
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    emit_cfg.target_width = 96;
    emit_cfg.target_height = 96;

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


/**
 * Verify that sphere rendering mode state is retained and uploaded through material params.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_sphere_mode(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzVisual* sphere = dvz_sphere(scene, DVZ_SPHERE_FLAGS_LIGHTING);
    AT(sphere != NULL);
    AT(sphere->sphere_mode == DVZ_SPHERE_MODE_FAST_IMPOSTOR);
    AT(sphere->material_params.depth_cue_extra[3] == (float)DVZ_SPHERE_MODE_FAST_IMPOSTOR);

    AT(dvz_sphere_mode(sphere, DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR) == 0);
    AT(sphere->sphere_mode == DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR);
    AT(sphere->material_params.depth_cue_extra[3] == (float)DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR);
    AT(sphere->material_params_dirty);

    AT(dvz_visual_set_primitive_shading(
           sphere,
           &(DvzPrimitiveShadingDesc){
               .light_direction = {0.0f, 0.0f, 1.0f},
               .ambient = 0.2f,
               .diffuse = 0.7f,
               .specular = 0.8f,
               .shininess = 64.0f,
           }) == 0);
    AT(sphere->sphere_mode == DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR);
    AT(sphere->material_params.depth_cue_extra[3] == (float)DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR);
    AT(dvz_sphere_mode(sphere, DVZ_SPHERE_MODE_FAST_IMPOSTOR) == 0);
    AT(sphere->material_params.depth_cue_extra[3] == (float)DVZ_SPHERE_MODE_FAST_IMPOSTOR);

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify segment cap defaults and retained cap updates.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_segment_caps(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    DvzVisual* visual = dvz_segment(scene, 0);
    AT(visual != NULL);
    AT(visual->segment.start_cap == DVZ_SEGMENT_CAP_BUTT);
    AT(visual->segment.end_cap == DVZ_SEGMENT_CAP_BUTT);
    AT(visual->material_params.params[0] == (float)DVZ_SEGMENT_CAP_BUTT);
    AT(visual->material_params.params[1] == (float)DVZ_SEGMENT_CAP_BUTT);

    AT(dvz_segment_set_caps(visual, DVZ_SEGMENT_CAP_ROUND, DVZ_SEGMENT_CAP_SQUARE) == 0);
    AT(visual->segment.start_cap == DVZ_SEGMENT_CAP_ROUND);
    AT(visual->segment.end_cap == DVZ_SEGMENT_CAP_SQUARE);
    AT(visual->material_params.params[0] == (float)DVZ_SEGMENT_CAP_ROUND);
    AT(visual->material_params.params[1] == (float)DVZ_SEGMENT_CAP_SQUARE);

    AT(dvz_segment_set_caps(visual, (DvzSegmentCap)99, DVZ_SEGMENT_CAP_BUTT) < 0);

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify segment visuals lower to analytic indexed GLSL quads.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_segment_emit_glsl(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    DvzVisual* visual = dvz_segment(scene, 0);
    AT(visual != NULL);

    float position_start[] = {
        -0.8f, -0.5f, 0.0f,
        -0.2f,  0.5f, 0.0f,
    };
    float position_end[] = {
         0.6f, -0.2f, 0.0f,
         0.8f,  0.4f, 0.0f,
    };
    DvzColor colors[2] = {{255, 64, 32, 255}, {64, 160, 255, 255}};
    float stroke_widths[2] = {8.0f, 4.0f};

    AT(dvz_visual_set_data(visual, "position_start", position_start, 2) == 0);
    AT(dvz_visual_set_data(visual, "position_end", position_end, 2) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 2) == 0);
    AT(dvz_visual_set_data(visual, "stroke_width", stroke_widths, 2) == 0);
    AT(dvz_segment_set_caps(visual, DVZ_SEGMENT_CAP_BUTT, DVZ_SEGMENT_CAP_ROUND) == 0);
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

    bool found_pipeline = false;
    bool found_set_index = false;
    bool found_draw_indexed = false;
    bool found_material_bg = false;
    uint32_t set_vertex_buffer_count = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        ANN(cmd);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.create_render_pipeline.id);
            if (label != NULL && strstr(label, "_pipe_segmentg") == label)
            {
                found_pipeline = true;
                AT(cmd->u.create_render_pipeline.topology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
                AT(cmd->u.create_render_pipeline.binding_count == 4);
                AT(cmd->u.create_render_pipeline.attr_count == 4);
                AT(cmd->u.create_render_pipeline.color_targets[0].blend_enabled);
                AT(
                    cmd->u.create_render_pipeline.color_targets[0].src_color_blend_factor ==
                    VK_BLEND_FACTOR_SRC_ALPHA);
                AT(
                    cmd->u.create_render_pipeline.color_targets[0].dst_color_blend_factor ==
                    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
            }
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER)
            set_vertex_buffer_count++;
        else if (cmd->type == DVZ_DRP2_COMMAND_SET_INDEX_BUFFER)
            found_set_index = strcmp(cmd->u.set_index_buffer.index_format, "uint32") == 0;
        else if (cmd->type == DVZ_DRP2_COMMAND_DRAW_INDEXED)
            found_draw_indexed = cmd->u.draw_indexed.index_count == 12;
    }

    AT(found_pipeline);
    AT(found_set_index);
    AT(found_draw_indexed);
    AT(set_vertex_buffer_count == 4);
    AT(_stream_write_buffer_range_count(stream, 0, sizeof(DvzSceneMaterialParams)) == 1);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
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


/**
 * Verify styled point visuals select the circular stroke shader and material bind group.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_point_style_emits_glsl_and_wgsl(TstSuite* suite, TstItem* item)
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

    float positions[2][3] = {{-0.25f, 0.0f, 0.0f}, {+0.25f, 0.0f, 0.0f}};
    DvzColor colors[2] = {{255, 80, 40, 255}, {80, 160, 255, 255}};
    float sizes[2] = {18.0f, 22.0f};
    AT(dvz_visual_set_data(visual, "position", positions, 2) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 2) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 2) == 0);
    AT(dvz_point_set_style(
           visual,
           &(DvzPointStyleDesc){
               .edge_color = {0, 0, 0, 255},
               .stroke_width = 3.0f,
               .filled = true,
               .stroke = true,
           }) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    DvzDrp2CommandStream* glsl_stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(glsl_stream);
    AT(_stream_has_render_pipeline_label(glsl_stream, "_pipe_point_styleg_depth"));

    bool found_material_bg = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(glsl_stream); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(glsl_stream, i);
        ANN(command);
        if (command->type == DVZ_DRP2_COMMAND_SET_BIND_GROUP)
            found_material_bg = found_material_bg || command->u.set_bind_group.slot == 1;
    }
    AT(found_material_bg);
    dvz_drp2_stream_destroy(glsl_stream);

    caps.shader_format_wgsl = true;
    caps.shader_format_glsl = false;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups = 4;
    caps.max_buffer_size = 256 * 1024 * 1024;
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_WGSL;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* wgsl_stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(wgsl_stream);
    AT(_stream_has_render_pipeline_label(wgsl_stream, "_pipe_point_stylew"));
    char* json = dvz_drp2_stream_json(wgsl_stream, "scene_point_style_wgsl_from_c");
    ANN(json);
    AT(strstr(json, "\"format\": \"wgsl\"") != NULL);
    AT(strstr(json, "line_width") != NULL);
    dvz_drp2_stream_json_destroy(json);
    dvz_drp2_stream_destroy(wgsl_stream);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify marker dense attributes, style validation, and GLSL pipeline emission.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_marker_api_and_emit_glsl(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    DvzVisual* visual = dvz_marker(scene, 0);
    AT(visual != NULL);
    AT(visual->type == DVZ_VISUAL_TYPE_MARKER);

    float positions[3][3] = {
        {-0.35f, 0.0f, 0.0f},
        {+0.00f, 0.0f, 0.0f},
        {+0.35f, 0.0f, 0.0f},
    };
    DvzColor colors[3] = {{255, 80, 40, 255}, {80, 255, 120, 255}, {80, 120, 255, 255}};
    float sizes[3] = {18.0f, 22.0f, 26.0f};
    float angles[3] = {0.0f, 0.25f, 0.5f};
    uint32_t shapes[3] = {
        DVZ_MARKER_SHAPE_DISC,
        DVZ_MARKER_SHAPE_DIAMOND,
        DVZ_MARKER_SHAPE_RING,
    };
    AT(dvz_visual_set_data(visual, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 3) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 3) == 0);
    AT(dvz_visual_set_data(visual, "angle", angles, 3) == 0);
    AT(dvz_visual_set_data(visual, "shape", shapes, 3) == 0);
    AT(visual->attr_count == 5);
    AT(dvz_marker_set_style(
           visual,
           &(DvzMarkerStyle){
               .edge_color = {0, 0, 0, 255},
               .stroke_width = 2.0f,
               .filled = true,
               .stroke = true,
           }) == 0);
    tst_log_capture_begin(suite);
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_visual_set_data(visual, "shape", shapes, 2) == -1);
    AT(_captured_log_contains(suite, "item_count"));
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);
    AT(_stream_has_render_pipeline_label(stream, "_pipe_markerg_depth"));

    bool found_pipeline = false;
    bool found_material_bg = false;
    bool found_draw = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        ANN(command);
        if (command->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE &&
            command->u.create_render_pipeline.binding_count == 5)
        {
            found_pipeline = true;
            AT(command->u.create_render_pipeline.attr_count == 5);
            AT(command->u.create_render_pipeline.topology == VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
            AT(command->u.create_render_pipeline.attr_locations[4] == 4);
            AT(command->u.create_render_pipeline.attr_formats[4] == VK_FORMAT_R32_UINT);
        }
        else if (command->type == DVZ_DRP2_COMMAND_SET_BIND_GROUP)
        {
            found_material_bg = found_material_bg || command->u.set_bind_group.slot == 1;
        }
        else if (command->type == DVZ_DRP2_COMMAND_DRAW)
        {
            found_draw = true;
            AT(command->u.draw.vertex_count == 3);
            AT(command->u.draw.instance_count == 1);
        }
    }
    AT(found_pipeline);
    AT(found_material_bg);
    AT(found_draw);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify GLSL pixel visuals keep native square point-list draw semantics.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_pixel_emit_glsl_native_square_points(TstSuite* suite, TstItem* item)
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

    float positions[2][3] = {{-0.25f, 0.0f, 0.0f}, {+0.25f, 0.0f, 0.0f}};
    DvzColor colors[2] = {{255, 0, 0, 255}, {0, 255, 0, 255}};
    float sizes[2] = {8.0f, 12.0f};
    AT(dvz_visual_set_data(visual, "position", positions, 2) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 2) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 2) == 0);
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
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        ANN(command);
        if (command->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            found_pipeline = true;
            AT(command->u.create_render_pipeline.topology == VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
        }
        else if (command->type == DVZ_DRP2_COMMAND_DRAW)
        {
            found_draw = true;
            AT(command->u.draw.vertex_count == 2);
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
    DvzMaterialDesc material = dvz_material_desc();
    material.model = DVZ_MATERIAL_MODEL_STANDARD;
    material.standard.roughness = 0.35f;
    material.standard.specular = 0.5f;
    material.standard.rim_strength = 0.15f;
    AT(dvz_visual_set_material(visual, &material) == 0);
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


int test_scene_mesh_instance_transform_emits_instanced_draw(TstSuite* suite, TstItem* item)
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
        {-0.25f, -0.25f, 0.0f}, {-0.25f, 0.25f, 0.0f},
        {0.25f, -0.25f, 0.0f},  {0.25f, 0.25f, 0.0f},
    };
    DvzIndex indices[6] = {0, 1, 2, 2, 1, 3};
    float transforms[2][16] = {
        {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, -0.4f, 0, 0, 1},
        {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, +0.4f, 0, 0, 1},
    };

    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)));

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "instance_transform", transforms, 2) == 0);
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

    bool found_instance_pipeline = false;
    bool found_instanced_draw = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            const uint32_t transform_binding = 2;
            found_instance_pipeline =
                cmd->u.create_render_pipeline.binding_count >= 3 &&
                cmd->u.create_render_pipeline.binding_step_modes[transform_binding] ==
                    DVZ_DRP2_VERTEX_STEP_MODE_INSTANCE;
        }
        if (cmd->type == DVZ_DRP2_COMMAND_DRAW_INDEXED)
        {
            found_instanced_draw = cmd->u.draw_indexed.index_count == 6 &&
                                   cmd->u.draw_indexed.instance_count == 2;
        }
    }
    AT(found_instance_pipeline);
    AT(found_instanced_draw);

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


/**
 * Verify line-width path visuals lower to the stroked segment pipeline.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_path_line_width_emit_glsl(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    DvzVisual* visual = dvz_path(scene, 0);
    AT(visual != NULL);

    float positions[5][3] = {
        {-0.75f, -0.25f, 0.0f},
        {-0.35f,  0.25f, 0.0f},
        { 0.00f, -0.10f, 0.0f},
        { 0.35f,  0.35f, 0.0f},
        { 0.75f, -0.25f, 0.0f},
    };
    DvzColor colors[5] = {
        {255, 0, 0, 255},
        {255, 255, 0, 255},
        {0, 255, 255, 255},
        {0, 128, 255, 255},
        {255, 255, 255, 255},
    };
    float stroke_widths[5] = {3.0f, 6.0f, 9.0f, 5.0f, 2.0f};
    uint32_t subpaths[2] = {3, 2};

    AT(dvz_visual_set_data(visual, "position", positions, 5) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 5) == 0);
    AT(dvz_visual_set_data(visual, "stroke_width", stroke_widths, 5) == 0);
    AT(dvz_path_set_subpaths(visual, 2, subpaths) == 0);
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

    bool found_pipeline = false;
    bool found_set_index = false;
    bool found_draw_indexed = false;
    bool found_material_bg = false;
    uint32_t set_vertex_buffer_count = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        ANN(cmd);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.create_render_pipeline.id);
            if (label != NULL && strstr(label, "_pipe_segmentg") == label)
            {
                found_pipeline = true;
                AT(cmd->u.create_render_pipeline.topology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
                AT(cmd->u.create_render_pipeline.binding_count == 4);
                AT(cmd->u.create_render_pipeline.attr_count == 4);
            }
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER)
            set_vertex_buffer_count++;
        else if (cmd->type == DVZ_DRP2_COMMAND_SET_INDEX_BUFFER)
            found_set_index = strcmp(cmd->u.set_index_buffer.index_format, "uint32") == 0;
        else if (cmd->type == DVZ_DRP2_COMMAND_DRAW_INDEXED)
            found_draw_indexed = cmd->u.draw_indexed.index_count == 18;
        else if (cmd->type == DVZ_DRP2_COMMAND_SET_BIND_GROUP)
            found_material_bg = found_material_bg || cmd->u.set_bind_group.slot == 1;
    }

    AT(found_pipeline);
    AT(found_set_index);
    AT(found_draw_indexed);
    AT(found_material_bg);
    AT(set_vertex_buffer_count == 4);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
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


int test_scene_image_multi_item_emit(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_image(scene, 0);
    ANN(visual);

    float positions[2][3] = {{-0.4f, 0.0f, 0.0f}, {+0.4f, 0.0f, 0.0f}};
    float extents[2][2] = {{0.3f, 0.4f}, {0.2f, 0.5f}};
    float tex_rects[2][4] = {{0.0f, 0.0f, 0.5f, 1.0f}, {0.5f, 0.0f, 1.0f, 1.0f}};
    uint8_t pixels[4 * 4 * 4];
    dvz_memset(pixels, sizeof(pixels), 128, sizeof(pixels));

    AT(dvz_visual_set_data(visual, "position", positions, 2) == 0);
    AT(dvz_visual_set_data(visual, "extent", extents, 2) == 0);
    AT(dvz_visual_set_data(visual, "tex_rect", tex_rects, 2) == 0);
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
    ANN(stream);

    uint64_t position_buffer_id = 0;
    uint64_t uv_buffer_id = 0;
    bool found_pipeline = false;
    bool found_draw = false;
    bool found_position_upload = false;
    bool found_uv_upload = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        ANN(cmd);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            found_pipeline =
                cmd->u.create_render_pipeline.vertex_buffer_slots == 2 &&
                cmd->u.create_render_pipeline.topology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST &&
                cmd->u.create_render_pipeline.bind_group_layout_count == 2;
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER)
        {
            if (cmd->u.set_vertex_buffer.slot == 0)
                position_buffer_id = cmd->u.set_vertex_buffer.buffer_id;
            else if (cmd->u.set_vertex_buffer.slot == 1)
                uv_buffer_id = cmd->u.set_vertex_buffer.buffer_id;
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_DRAW)
        {
            found_draw = cmd->u.draw.vertex_count == 12 && cmd->u.draw.instance_count == 1;
        }
    }
    AT(position_buffer_id != 0);
    AT(uv_buffer_id != 0);

    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        ANN(cmd);
        if (cmd->type != DVZ_DRP2_COMMAND_WRITE_BUFFER)
            continue;
        if (cmd->u.write_buffer.buffer_id == position_buffer_id)
            found_position_upload = cmd->u.write_buffer.size == 12 * 3 * sizeof(float);
        if (cmd->u.write_buffer.buffer_id == uv_buffer_id)
            found_uv_upload = cmd->u.write_buffer.size == 12 * 2 * sizeof(float);
    }

    AT(found_pipeline);
    AT(found_draw);
    AT(found_position_upload);
    AT(found_uv_upload);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify glyph visuals emit an MSDF-capable textured triangle pipeline.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_glyph_emit_glsl(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_glyph(scene, 0);
    ANN(visual);

    float positions[6][3] = {
        {-0.5f, -0.5f, 0.0f}, {-0.5f, 0.5f, 0.0f}, {0.5f, -0.5f, 0.0f},
        { 0.5f, -0.5f, 0.0f}, {-0.5f, 0.5f, 0.0f}, {0.5f,  0.5f, 0.0f},
    };
    float texcoords[6][2] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f},
        {1.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f},
    };
    DvzColor colors[6] = {
        {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
        {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    };
    uint8_t pixels[4 * 4 * 4];
    dvz_memset(pixels, sizeof(pixels), 255, sizeof(pixels));

    AT(dvz_visual_set_data(visual, "position", positions, 6) == 0);
    AT(dvz_visual_set_data(visual, "texcoords", texcoords, 6) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 6) == 0);
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
    ANN(stream);
    AT(_stream_has_render_pipeline_label(stream, "_pipe_glyphg"));

    uint64_t position_buffer_id = 0;
    uint64_t uv_buffer_id = 0;
    uint64_t color_buffer_id = 0;
    bool found_pipeline = false;
    bool found_draw = false;
    bool found_texture_bind = false;
    bool found_position_upload = false;
    bool found_uv_upload = false;
    bool found_color_upload = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        ANN(cmd);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            found_pipeline =
                cmd->u.create_render_pipeline.vertex_buffer_slots == 3 &&
                cmd->u.create_render_pipeline.topology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST &&
                cmd->u.create_render_pipeline.binding_count == 3 &&
                cmd->u.create_render_pipeline.attr_count == 3 &&
                cmd->u.create_render_pipeline.bind_group_layout_count == 2;
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER)
        {
            if (cmd->u.set_vertex_buffer.slot == 0)
                position_buffer_id = cmd->u.set_vertex_buffer.buffer_id;
            else if (cmd->u.set_vertex_buffer.slot == 1)
                uv_buffer_id = cmd->u.set_vertex_buffer.buffer_id;
            else if (cmd->u.set_vertex_buffer.slot == 2)
                color_buffer_id = cmd->u.set_vertex_buffer.buffer_id;
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_SET_BIND_GROUP)
        {
            found_texture_bind = found_texture_bind || cmd->u.set_bind_group.slot == 1;
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_DRAW)
        {
            found_draw = cmd->u.draw.vertex_count == 6 && cmd->u.draw.instance_count == 1;
        }
    }
    AT(position_buffer_id != 0);
    AT(uv_buffer_id != 0);
    AT(color_buffer_id != 0);

    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        ANN(cmd);
        if (cmd->type != DVZ_DRP2_COMMAND_WRITE_BUFFER)
            continue;
        if (cmd->u.write_buffer.buffer_id == position_buffer_id)
            found_position_upload = cmd->u.write_buffer.size == 6 * 3 * sizeof(float);
        if (cmd->u.write_buffer.buffer_id == uv_buffer_id)
            found_uv_upload = cmd->u.write_buffer.size == 6 * 2 * sizeof(float);
        if (cmd->u.write_buffer.buffer_id == color_buffer_id)
            found_color_upload = cmd->u.write_buffer.size == 6 * sizeof(DvzColor);
    }

    AT(found_pipeline);
    AT(found_draw);
    AT(found_texture_bind);
    AT(found_position_upload);
    AT(found_uv_upload);
    AT(found_color_upload);

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


int test_scene_hidden_visual_first_visible_later_uploads(TstSuite* suite, TstItem* item)
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

    float positions[] = {-0.5f, 0.0f, 0.0f,  0.5f, 0.0f, 0.0f};
    DvzColor colors[2] = {{255, 0, 0, 255}, {0, 255, 0, 255}};
    float sizes[2] = {8.0f, 8.0f};
    AT(dvz_visual_set_data(visual, "position", positions, 2) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 2) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 2) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);
    dvz_visual_set_visible(visual, false);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_wgsl = true;
    DvzDiagnosticReport report;

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 = dvz_figure_emit(figure, &caps, &report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream1 != NULL);
    AT(_stream_visual_write_buffer_count(stream1) == 0);
    dvz_drp2_stream_destroy(stream1);

    dvz_visual_set_visible(visual, true);
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream2 = dvz_figure_emit(figure, &caps, &report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream2 != NULL);
    AT(_stream_visual_write_buffer_count(stream2) > 0);

    bool found_draw = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream2); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream2, i);
        if (cmd->type == DVZ_DRP2_COMMAND_DRAW)
            found_draw = true;
    }
    AT(found_draw);

    dvz_drp2_stream_destroy(stream2);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_hidden_indexed_mesh_first_visible_later_uploads(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    DvzVisual* visual = dvz_mesh(scene, 0);
    AT(visual != NULL);

    float positions[4][3] = {
        {-0.5f, -0.5f, 0.0f},
        {+0.5f, -0.5f, 0.0f},
        {-0.5f, +0.5f, 0.0f},
        {+0.5f, +0.5f, 0.0f},
    };
    float normals[4][3] = {
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
    };
    DvzColor colors[4] = {
        {255, 0, 0, 255},
        {0, 255, 0, 255},
        {0, 0, 255, 255},
        {255, 255, 0, 255},
    };
    DvzIndex indices[6] = {0, 1, 2, 2, 1, 3};
    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){.usage = DVZ_SCENE_BUFFER_USAGE_INDEX, .stride = sizeof(DvzIndex)});
    AT(index_buffer != NULL);
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)));
    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "normal", normals, 4) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 4) == 0);
    AT(dvz_visual_set_buffer(visual, "index", index_buffer));
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);
    dvz_visual_set_visible(visual, false);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;
    DvzDiagnosticReport report;

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 = dvz_figure_emit(figure, &caps, &report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream1 != NULL);
    AT(_stream_visual_write_buffer_count(stream1) == 0);
    dvz_drp2_stream_destroy(stream1);

    dvz_visual_set_visible(visual, true);
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream2 = dvz_figure_emit(figure, &caps, &report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream2 != NULL);
    AT(_stream_visual_write_buffer_count(stream2) > 0);
    dvz_drp2_stream_destroy(stream2);

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream3 = dvz_figure_emit(figure, &caps, &report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream3 != NULL);
    AT(_stream_visual_write_buffer_count(stream3) == 0);
    dvz_drp2_stream_destroy(stream3);

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Execute a hidden WBOIT mesh becoming visible under scene occlusion across two runtime frames.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_hidden_wboit_mesh_scene_occlusion_two_frames_glsl_executes(
    TstSuite* suite, TstItem* item)
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

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 2,
                   .height = 2,
                   .depth = 2,
               });
    ANN(field);
    const uint8_t voxels[8] = {255, 255, 255, 255, 255, 255, 255, 255};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = voxels, .bytes_per_row = 2, .rows_per_image = 2}));

    DvzVisual* volume = dvz_volume(scene, 0);
    DvzVisual* slice = dvz_volume(scene, 0);
    DvzVisual* mesh = dvz_mesh(scene, 0);
    AT(volume != NULL);
    AT(slice != NULL);
    AT(mesh != NULL);
    AT(dvz_visual_set_field(volume, "field", field));
    AT(dvz_visual_set_field(slice, "field", field));
    AT(dvz_volume_set_render_mode(volume, DVZ_VOLUME_RENDER_MIP) == 0);
    AT(dvz_volume_set_render_mode(slice, DVZ_VOLUME_RENDER_SLICE) == 0);
    AT(dvz_visual_set_scene_occluder(volume, true) == 0);
    AT(dvz_visual_set_scene_occluded(slice, true) == 0);

    float positions[4][3] = {
        {-0.5f, -0.5f, 0.1f},
        {+0.5f, -0.5f, 0.1f},
        {-0.5f, +0.5f, 0.1f},
        {+0.5f, +0.5f, 0.1f},
    };
    float normals[4][3] = {
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
    };
    DvzColor colors[4] = {
        {255, 0, 0, 128},
        {0, 255, 0, 128},
        {0, 0, 255, 128},
        {255, 255, 0, 128},
    };
    DvzIndex indices[6] = {0, 1, 2, 2, 1, 3};
    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene,
        &(DvzSceneBufferDesc){
            .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
            .stride = sizeof(DvzIndex),
        });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)));
    AT(dvz_visual_set_data(mesh, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(mesh, "normal", normals, 4) == 0);
    AT(dvz_visual_set_data(mesh, "color", colors, 4) == 0);
    AT(dvz_visual_set_buffer(mesh, "index", index_buffer));
    AT(dvz_visual_set_alpha_mode(mesh, DVZ_ALPHA_WBOIT) == 0);
    AT(dvz_visual_set_depth_test(mesh, true) == 0);
    AT(dvz_visual_set_scene_occluder(mesh, true) == 0);
    dvz_visual_set_visible(mesh, false);

    AT(dvz_panel_add_visual(panel, volume, NULL) == 0);
    AT(dvz_panel_add_visual(panel, slice, NULL) == 0);
    AT(dvz_panel_add_visual(panel, mesh, NULL) == 0);
    AT(dvz_panel_set_scene_occlusion(
           panel,
           &(DvzSceneOcclusionDesc){
               .enabled = true,
               .depth_bias = 0.0005f,
               .soft_edge = 0.01f,
               .hidden_alpha = 0.2f,
           }) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.max_color_attachments = 2;
    caps.render_target_format_rgba16float = true;
    caps.render_target_format_r16float = true;
    caps.supports_color_blending = true;
    caps.supports_render_target_sampling = true;
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.target_width = 64;
    cfg.target_height = 64;

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream0 = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    ANN(stream0);
    AT(dvz_diagnostic_report_count(&report) == 0);
    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream0);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    dvz_drp2_stream_destroy(stream0);

    dvz_visual_set_visible(mesh, true);
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    ANN(stream1);
    AT(dvz_diagnostic_report_count(&report) == 0);
    result = dvz_drp2_runtime_execute(runtime, stream1);
    if (!result.ok)
    {
        const DvzDrp2Command* failed = dvz_drp2_stream_get(stream1, result.command_index);
        uint64_t id = 0;
        if (failed != NULL && failed->type == DVZ_DRP2_COMMAND_SET_BIND_GROUP)
            id = failed->u.set_bind_group.bind_group_id;
        else if (failed != NULL && failed->type == DVZ_DRP2_COMMAND_CREATE_BIND_GROUP)
            id = failed->u.create_bind_group.id;
        else if (failed != NULL && failed->type == DVZ_DRP2_COMMAND_SET_PIPELINE)
            id = failed->u.set_pipeline.pipeline_id;
        const char* label = id != 0 ? dvz_drp2_stream_label(stream1, id) : NULL;
        log_error(
            "runtime failure code=%d command=%" PRIu32 " type=%d id=%" PRIu64 " label=%s",
            result.code, result.command_index, failed != NULL ? (int)failed->type : -1, id,
            label != NULL ? label : "(none)");
    }
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    dvz_drp2_stream_destroy(stream1);
    dvz_drp2_runtime_destroy(runtime);
    dvz_scene_destroy(scene);
    dvz_gpu_ctx_destroy(ctx);
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
 * Verify visual depth-test storage and mutation.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_visual_depth_test(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzVisual* visual = dvz_mesh(scene, 0);
    AT(visual != NULL);

    AT(dvz_visual_depth_test(visual));
    AT(dvz_visual_set_depth_test(visual, false) == 0);
    AT(!dvz_visual_depth_test(visual));
    AT(dvz_visual_set_depth_test(visual, true) == 0);
    AT(dvz_visual_depth_test(visual));

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify generic scene occlusion flag storage and FramePlan metadata propagation.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_visual_scene_occlusion_flags(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    AT(panel != NULL);

    DvzVisual* occluder = dvz_mesh(scene, 0);
    DvzVisual* occluded = dvz_point(scene, 0);
    AT(occluder != NULL);
    AT(occluded != NULL);

    AT(dvz_visual_set_scene_occluder(occluder, true) == 0);
    AT(dvz_visual_set_scene_occluded(occluded, true) == 0);
    AT(occluder->scene_occluder);
    AT(!occluder->scene_occluded);
    AT(!occluded->scene_occluder);
    AT(occluded->scene_occluded);

    DvzFramePlanVisualMeta occluder_meta = {0};
    DvzFramePlanVisualMeta occluded_meta = {0};
    AT(_scene_visual_frame_plan_metadata(figure, occluder, 0, &occluder_meta));
    AT(_scene_visual_frame_plan_metadata(figure, occluded, 1, &occluded_meta));
    AT(occluder_meta.scene_occluder);
    AT(!occluder_meta.scene_occluded);
    AT(!occluded_meta.scene_occluder);
    AT(occluded_meta.scene_occluded);

    AT(dvz_panel_set_scene_occlusion(
           panel,
           &(DvzSceneOcclusionDesc){
               .enabled = true,
               .depth_bias = 0.001f,
               .soft_edge = 0.01f,
               .hidden_alpha = 0.25f,
           }) == 0);
    AT(panel->scene_occlusion_enabled);
    AT(panel->scene_occlusion.hidden_alpha == 0.25f);
    AT(dvz_panel_set_scene_occlusion(panel, NULL) == 0);
    AT(!panel->scene_occlusion_enabled);

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify scene occlusion prepass ordering and graph sampled reads.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_visual_scene_occlusion_frame_plan(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    DvzVisual* occluder = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    DvzVisual* occluded = dvz_point(scene, 0);
    AT(occluder != NULL);
    AT(occluded != NULL);

    float positions[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f,
    };
    DvzColor colors[3] = {{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}};
    float sizes[3] = {10.0f, 10.0f, 10.0f};

    AT(dvz_visual_set_data(occluder, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(occluder, "color", colors, 3) == 0);
    AT(dvz_visual_set_data(occluded, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(occluded, "color", colors, 3) == 0);
    AT(dvz_visual_set_data(occluded, "size", sizes, 3) == 0);
    AT(dvz_visual_set_scene_occluder(occluder, true) == 0);
    AT(dvz_visual_set_scene_occluded(occluded, true) == 0);
    AT(dvz_panel_add_visual(panel, occluder, NULL) == 0);
    AT(dvz_panel_add_visual(panel, occluded, NULL) == 0);
    AT(dvz_panel_set_scene_occlusion(
           panel,
           &(DvzSceneOcclusionDesc){
               .enabled = true,
               .depth_bias = 0.001f,
               .soft_edge = 0.01f,
               .hidden_alpha = 0.2f,
           }) == 0);

    DvzFramePlan* plan = dvz_frame_plan("figure.scene_occlusion", 0);
    ANN(plan);
    _scene_emit_panel_render(figure, 0, plan, "figure_0");

    AT(dvz_frame_plan_node_count(plan) == 2);
    const DvzFramePlanNode* occlusion_node = dvz_frame_plan_node_get(plan, 0);
    const DvzFramePlanNode* opaque_node = dvz_frame_plan_node_get(plan, 1);
    ANN(occlusion_node);
    ANN(opaque_node);
    AT(
        dvz_frame_plan_render_pass_role(occlusion_node) ==
        DVZ_FRAME_PLAN_RENDER_PASS_SCENE_OCCLUSION);
    AT(dvz_frame_plan_render_pass_role(opaque_node) == DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE);
    AT(occlusion_node->u.render.visual_count == 1);
    AT(opaque_node->u.render.visual_count == 2);
    AT(occlusion_node->u.render.visual_metadata[0].scene_occluder);
    AT(opaque_node->u.render.visual_metadata[1].scene_occluded);
    AT(opaque_node->u.render.visual_metadata[1].has_scene_occlusion);

    AT(dvz_frame_plan_graph_pass_count(plan) == 2);
    const DvzFrameGraphPass* occlusion_pass = dvz_frame_plan_graph_pass_get(plan, 0);
    const DvzFrameGraphPass* opaque_pass = dvz_frame_plan_graph_pass_get(plan, 1);
    ANN(occlusion_pass);
    ANN(opaque_pass);
    AT(strcmp(occlusion_pass->work_label, "scene_occlusion") == 0);
    AT(occlusion_pass->color_attachment_count == 1);
    AT(strcmp(occlusion_pass->color_attachments[0].resource_id,
              "figure_0_p0.scene_occlusion.depth") == 0);
    AT(strcmp(opaque_pass->work_label, "opaque") == 0);
    AT(opaque_pass->read_count == 1);
    AT(strcmp(opaque_pass->reads[0].resource_id, "figure_0_p0.scene_occlusion.depth") == 0);
    AT(opaque_pass->reads[0].usage == DVZ_FRAME_GRAPH_ACCESS_SAMPLED);

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    AT(dvz_frame_plan_graph_validate(plan, &report));
    dvz_diagnostic_report_init(&report);
    AT(_scene_frame_plan_contracts_validate(figure, plan, &report));
    AT(dvz_diagnostic_report_count(&report) == 0);

    DvzScenePassContract occlusion_contract = {0};
    AT(_scene_pass_contract_from_render(
        plan, panel, occlusion_node, occlusion_pass, &occlusion_contract));
    AT(occlusion_contract.draw_count == 1);
    AT(occlusion_contract.draws[0].writes_scene_occlusion_depth);
    AT(occlusion_contract.draws[0].depth_test);
    AT(occlusion_contract.draws[0].depth_write);
    AT(occlusion_contract.color_attachment_count == 1);
    AT(occlusion_contract.has_depth_attachment);
    AT(occlusion_contract.attachments[0].format == VK_FORMAT_R32_SFLOAT);
    AT(occlusion_contract.attachments[0].sample_count == 1);
    AT(occlusion_contract.attachments[1].format == VK_FORMAT_D32_SFLOAT);
    AT(occlusion_contract.attachments[1].write);
    AT(occlusion_contract.attachments[1].clear);
    dvz_diagnostic_report_init(&report);
    AT(_scene_pass_contract_validate(&occlusion_contract, &report));
    AT(dvz_diagnostic_report_count(&report) == 0);

    DvzScenePassContract opaque_contract = {0};
    AT(_scene_pass_contract_from_render(plan, panel, opaque_node, opaque_pass, &opaque_contract));
    AT(opaque_contract.draw_count == 2);
    AT(opaque_contract.draws[1].samples_scene_occlusion);
    AT(opaque_contract.draws[1].needs_scene_occlusion_set);
    dvz_diagnostic_report_init(&report);
    AT(_scene_pass_contract_validate(&opaque_contract, &report));
    AT(dvz_diagnostic_report_count(&report) == 0);

    dvz_frame_plan_destroy(plan);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify scene occlusion lowers to executable DRP2 resources, passes, and bind groups.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_visual_scene_occlusion_emits_drp2(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    DvzVisual* occluder = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    DvzVisual* occluded = dvz_point(scene, 0);
    AT(occluder != NULL);
    AT(occluded != NULL);

    float positions[3][3] = {
        {-0.5f, -0.5f, 0.0f},
        {0.5f, -0.5f, 0.0f},
        {0.0f, 0.5f, 0.0f},
    };
    DvzColor colors[3] = {{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}};
    float sizes[3] = {10.0f, 10.0f, 10.0f};

    AT(dvz_visual_set_data(occluder, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(occluder, "color", colors, 3) == 0);
    AT(dvz_visual_set_data(occluded, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(occluded, "color", colors, 3) == 0);
    AT(dvz_visual_set_data(occluded, "size", sizes, 3) == 0);
    AT(dvz_visual_set_scene_occluder(occluder, true) == 0);
    AT(dvz_visual_set_scene_occluded(occluded, true) == 0);
    AT(dvz_panel_add_visual(panel, occluder, NULL) == 0);
    AT(dvz_panel_add_visual(panel, occluded, NULL) == 0);
    AT(dvz_panel_set_scene_occlusion(
           panel,
           &(DvzSceneOcclusionDesc){
               .enabled = true,
               .depth_bias = 0.001f,
               .soft_edge = 0.01f,
               .hidden_alpha = 0.2f,
           }) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
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

    bool has_scene_depth = false;
    bool has_scene_z = false;
    bool has_scene_depth_pass = false;
    bool has_scene_occluded_pipeline = false;
    bool has_scene_occluder_depth_pipeline = false;
    bool has_scene_occlusion_bind_group = false;
    bool binds_scene_occlusion_group = false;
    bool has_alpha_aware_depth_shader = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        ANN(command);
        if (command->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE)
        {
            const char* label = dvz_drp2_stream_label(stream, command->u.create_texture.id);
            has_scene_depth =
                has_scene_depth ||
                (label != NULL && strcmp(label, "fig0_p0.scene_occlusion.depth") == 0 &&
                 command->u.create_texture.format == VK_FORMAT_R32_SFLOAT);
            has_scene_z =
                has_scene_z ||
                (label != NULL && strcmp(label, "fig0_p0.scene_occlusion.z") == 0 &&
                 command->u.create_texture.format == VK_FORMAT_D32_SFLOAT);
        }
        else if (command->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS)
        {
            has_scene_depth_pass =
                has_scene_depth_pass ||
                (command->u.begin_render_pass.color_attachment_count == 1 &&
                 command->u.begin_render_pass.has_depth_attachment &&
                 command->u.begin_render_pass.clear_color[0] == 1.0f);
        }
        else if (command->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            const char* label = dvz_drp2_stream_label(stream, command->u.create_render_pipeline.id);
            has_scene_occluded_pipeline =
                has_scene_occluded_pipeline ||
                (label != NULL && strstr(label, "scene_occ") != NULL &&
                 command->u.create_render_pipeline.bind_group_layout_count >= 2);
            has_scene_occluder_depth_pipeline =
                has_scene_occluder_depth_pipeline ||
                (label != NULL && strstr(label, "_pipe_scene_occ_prim") != NULL &&
                 command->u.create_render_pipeline.depth_write_enabled &&
                 command->u.create_render_pipeline.depth_compare_op == VK_COMPARE_OP_LESS_OR_EQUAL);
        }
        else if (command->type == DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE)
        {
            has_alpha_aware_depth_shader =
                has_alpha_aware_depth_shader ||
                (command->u.create_shader_module.code != NULL &&
                 strstr(
                     command->u.create_shader_module.code,
                     "DVZ_SCENE_OCCLUSION_DEPTH_COLOR") != NULL);
        }
        else if (command->type == DVZ_DRP2_COMMAND_CREATE_BIND_GROUP)
        {
            const char* label = dvz_drp2_stream_label(stream, command->u.create_bind_group.id);
            has_scene_occlusion_bind_group =
                has_scene_occlusion_bind_group ||
                (label != NULL && strstr(label, "_bg_scene_occ_depth_") != NULL);
        }
        else if (command->type == DVZ_DRP2_COMMAND_SET_BIND_GROUP)
        {
            const char* label =
                dvz_drp2_stream_label(stream, command->u.set_bind_group.bind_group_id);
            binds_scene_occlusion_group =
                binds_scene_occlusion_group ||
                (command->u.set_bind_group.slot == 1 && label != NULL &&
                 strstr(label, "_bg_scene_occ_depth_") != NULL);
        }
    }
    AT(has_scene_depth);
    AT(has_scene_z);
    AT(has_scene_depth_pass);
    AT(has_scene_occluded_pipeline);
    AT(has_scene_occluder_depth_pipeline);
    AT(has_alpha_aware_depth_shader);
    AT(has_scene_occlusion_bind_group);
    AT(binds_scene_occlusion_group);

    DvzDrp2RuntimeConfig runtime_cfg = dvz_drp2_runtime_vklite_config(NULL, NULL);
    runtime_cfg.semantic_only = true;
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);
    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    dvz_drp2_runtime_destroy(runtime);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify a volume front-depth producer can occlude a volume slice through volume occlusion.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_volume_slice_uses_volume_occlusion(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 2,
                   .height = 2,
                   .depth = 2,
               });
    ANN(field);
    const uint8_t voxels[8] = {255, 255, 255, 255, 255, 255, 255, 255};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = voxels, .bytes_per_row = 2, .rows_per_image = 2}));

    DvzVisual* volume = dvz_volume(scene, 0);
    DvzVisual* slice = dvz_volume(scene, 0);
    AT(volume != NULL);
    AT(slice != NULL);
    AT(dvz_visual_set_field(volume, "field", field));
    AT(dvz_visual_set_field(slice, "field", field));
    AT(dvz_volume_set_render_mode(volume, DVZ_VOLUME_RENDER_MIP) == 0);
    AT(dvz_volume_set_step_count(volume, 16) == 0);
    AT(dvz_volume_set_render_mode(slice, DVZ_VOLUME_RENDER_SLICE) == 0);
    AT(dvz_visual_set_volume_occluded(slice, true) == 0);
    AT(dvz_panel_add_visual(panel, volume, NULL) == 0);
    AT(dvz_panel_add_visual(panel, slice, NULL) == 0);
    AT(dvz_panel_set_volume_occluder(
           panel, volume,
           &(DvzVolumeOcclusionDesc){
               .enabled = true,
               .alpha_threshold = 0.01f,
               .fade_distance = 0.04f,
               .occluded_alpha = 0.2f,
           }) == 0);

    DvzFramePlan* plan = dvz_frame_plan("figure.volume_occlusion", 0);
    ANN(plan);
    _scene_emit_panel_render(figure, 0, plan, "figure_0");

    const DvzFramePlanNode* occlusion_node = dvz_frame_plan_node_get(plan, 0);
    const DvzFramePlanNode* opaque_node = dvz_frame_plan_node_get(plan, 1);
    ANN(occlusion_node);
    ANN(opaque_node);
    AT(
        dvz_frame_plan_render_pass_role(occlusion_node) ==
        DVZ_FRAME_PLAN_RENDER_PASS_VOLUME_OCCLUSION);
    AT(dvz_frame_plan_render_pass_role(opaque_node) == DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE);

    const DvzFrameGraphPass* volume_pass = dvz_frame_plan_graph_pass_get(plan, 0);
    const DvzFrameGraphPass* opaque_pass = dvz_frame_plan_graph_pass_get(plan, 1);
    ANN(volume_pass);
    ANN(opaque_pass);
    AT(strcmp(volume_pass->work_label, "volume_occlusion") == 0);
    AT(strcmp(opaque_pass->work_label, "opaque") == 0);

    DvzDiagnosticReport graph_report;
    dvz_diagnostic_report_init(&graph_report);
    AT(dvz_frame_plan_graph_validate(plan, &graph_report));
    dvz_diagnostic_report_init(&graph_report);
    AT(_scene_frame_plan_contracts_validate(figure, plan, &graph_report));
    AT(dvz_diagnostic_report_count(&graph_report) == 0);

    DvzScenePassContract volume_contract = {0};
    AT(_scene_pass_contract_from_render(plan, panel, occlusion_node, volume_pass, &volume_contract));
    AT(volume_contract.draw_count == 1);
    AT(volume_contract.draws[0].writes_volume_occlusion_depth);
    AT(!volume_contract.draws[0].samples_depth);
    AT(volume_contract.color_attachment_count == 1);
    AT(volume_contract.attachments[0].format == VK_FORMAT_R32_SFLOAT);
    AT(volume_contract.attachments[0].sample_count == 1);
    AT(volume_contract.attachments[0].load_op == DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR);
    AT(volume_contract.attachments[0].access == DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE);
    dvz_diagnostic_report_init(&graph_report);
    AT(_scene_pass_contract_validate(&volume_contract, &graph_report));
    AT(dvz_diagnostic_report_count(&graph_report) == 0);

    DvzScenePassContract opaque_contract = {0};
    AT(_scene_pass_contract_from_render(plan, panel, opaque_node, opaque_pass, &opaque_contract));
    AT(opaque_contract.draw_count == 2);
    AT(opaque_contract.draws[1].samples_volume_occlusion);
    AT(opaque_contract.draws[1].needs_volume_set);
    dvz_diagnostic_report_init(&graph_report);
    AT(_scene_pass_contract_validate(&opaque_contract, &graph_report));
    AT(dvz_diagnostic_report_count(&graph_report) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
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

    uint64_t volume_occlusion_depth_id = 0;
    bool has_volume_occlusion_pipeline = false;
    bool has_volume_slice_pipeline = false;
    bool has_scene_occlusion_pipeline = false;
    bool binds_volume_occlusion_depth = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        ANN(command);
        if (command->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE)
        {
            const char* label = dvz_drp2_stream_label(stream, command->u.create_texture.id);
            if (label != NULL && strstr(label, ".volume_occlusion.depth") != NULL)
                volume_occlusion_depth_id = command->u.create_texture.id;
        }
        if (command->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            const char* label = dvz_drp2_stream_label(stream, command->u.create_render_pipeline.id);
            has_volume_occlusion_pipeline =
                has_volume_occlusion_pipeline ||
                (label != NULL && strstr(label, "_pipe_vol_occ") != NULL);
            has_volume_slice_pipeline =
                has_volume_slice_pipeline ||
                (label != NULL && strstr(label, "_pipe_vol_slice") != NULL &&
                 strstr(label, "_scene_occ") == NULL &&
                 command->u.create_render_pipeline.bind_group_layout_count == 2);
            has_scene_occlusion_pipeline =
                has_scene_occlusion_pipeline ||
                (label != NULL && strstr(label, "_pipe_scene_occ") != NULL);
        }
        if (command->type == DVZ_DRP2_COMMAND_CREATE_BIND_GROUP)
        {
            for (uint32_t j = 0; j < command->u.create_bind_group.entry_count; j++)
            {
                const DvzDrp2BindGroupEntry* entry =
                    &command->u.create_bind_group.entries[j];
                binds_volume_occlusion_depth =
                    binds_volume_occlusion_depth ||
                    (volume_occlusion_depth_id != 0 && entry->binding == 3 &&
                     entry->resource_id == volume_occlusion_depth_id);
            }
        }
    }
    AT(volume_occlusion_depth_id != 0);
    AT(has_volume_occlusion_pipeline);
    AT(has_volume_slice_pipeline);
    AT(!has_scene_occlusion_pipeline);
    AT(binds_volume_occlusion_depth);

    dvz_drp2_stream_destroy(stream);
    dvz_frame_plan_destroy(plan);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Verify a volume front-depth producer can occlude a volume slice through generic scene occlusion.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_volume_slice_uses_generic_scene_occlusion(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 2,
                   .height = 2,
                   .depth = 2,
               });
    ANN(field);
    const uint8_t voxels[8] = {255, 255, 255, 255, 255, 255, 255, 255};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = voxels, .bytes_per_row = 2, .rows_per_image = 2}));

    DvzVisual* volume = dvz_volume(scene, 0);
    DvzVisual* slice = dvz_volume(scene, 0);
    AT(volume != NULL);
    AT(slice != NULL);
    AT(dvz_visual_set_field(volume, "field", field));
    AT(dvz_visual_set_field(slice, "field", field));
    AT(dvz_volume_set_render_mode(volume, DVZ_VOLUME_RENDER_MIP) == 0);
    AT(dvz_volume_set_step_count(volume, 16) == 0);
    AT(dvz_volume_set_render_mode(slice, DVZ_VOLUME_RENDER_SLICE) == 0);
    AT(dvz_visual_set_scene_occluder(volume, true) == 0);
    AT(dvz_visual_set_scene_occluded(slice, true) == 0);
    AT(dvz_panel_add_visual(panel, volume, NULL) == 0);
    AT(dvz_panel_add_visual(panel, slice, NULL) == 0);
    AT(dvz_panel_set_volume_occluder(
           panel, volume,
           &(DvzVolumeOcclusionDesc){
               .enabled = true,
               .alpha_threshold = 0.01f,
               .fade_distance = 0.04f,
               .occluded_alpha = 0.2f,
           }) == 0);
    AT(dvz_panel_set_scene_occlusion(
           panel,
           &(DvzSceneOcclusionDesc){
               .enabled = true,
               .depth_bias = 0.0005f,
               .soft_edge = 0.01f,
               .hidden_alpha = 0.2f,
           }) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
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

    bool has_volume_scene_occluder_pipeline = false;
    bool has_volume_slice_scene_occluded_pipeline = false;
    bool has_scene_occlusion_bind_group = false;
    bool binds_scene_occlusion_set2 = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        ANN(command);
        if (command->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            const char* label = dvz_drp2_stream_label(stream, command->u.create_render_pipeline.id);
            has_volume_scene_occluder_pipeline =
                has_volume_scene_occluder_pipeline ||
                (label != NULL && strstr(label, "_pipe_scene_occ_vol") != NULL);
            has_volume_slice_scene_occluded_pipeline =
                has_volume_slice_scene_occluded_pipeline ||
                (label != NULL && strstr(label, "_pipe_vol_slice") != NULL &&
                 strstr(label, "_scene_occ") != NULL &&
                 command->u.create_render_pipeline.bind_group_layout_count >= 3);
        }
        else if (command->type == DVZ_DRP2_COMMAND_CREATE_BIND_GROUP)
        {
            const char* label = dvz_drp2_stream_label(stream, command->u.create_bind_group.id);
            has_scene_occlusion_bind_group =
                has_scene_occlusion_bind_group ||
                (label != NULL && strstr(label, "_bg_scene_occ_depth_") != NULL);
        }
        else if (command->type == DVZ_DRP2_COMMAND_SET_BIND_GROUP)
        {
            const char* label =
                dvz_drp2_stream_label(stream, command->u.set_bind_group.bind_group_id);
            binds_scene_occlusion_set2 =
                binds_scene_occlusion_set2 ||
                (command->u.set_bind_group.slot == 2 && label != NULL &&
                 strstr(label, "_bg_scene_occ_depth_") != NULL);
        }
    }
    AT(has_volume_scene_occluder_pipeline);
    AT(has_volume_slice_scene_occluded_pipeline);
    AT(has_scene_occlusion_bind_group);
    AT(binds_scene_occlusion_set2);

    DvzDrp2RuntimeConfig runtime_cfg = dvz_drp2_runtime_vklite_config(NULL, NULL);
    runtime_cfg.semantic_only = true;
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);
    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    dvz_drp2_runtime_destroy(runtime);

    dvz_drp2_stream_destroy(stream);
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
 * Verify the public material descriptor updates retained state and the current GPU payload.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_visual_material_setter(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzMaterialDesc defaults = dvz_material_desc();
    AT(defaults.model == DVZ_MATERIAL_MODEL_PHONG);
    AT(defaults.alpha_mode == DVZ_ALPHA_OPAQUE);
    AT(defaults.opacity == 1.0f);
    AT(defaults.base_color_factor[0] == 1.0f);
    AT(defaults.base_color_factor[3] == 1.0f);
    AT(defaults.light_direction[2] == 1.0f);
    AT(defaults.phong.ambient == 0.2f);
    AT(defaults.phong.diffuse == 0.8f);
    AT(defaults.phong.specular == 0.25f);
    AT(defaults.phong.shininess == 32.0f);
    AT(defaults.standard.roughness == 0.5f);
    AT(defaults.standard.specular == 0.5f);

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzVisual* mesh = dvz_mesh(scene, 0);
    DvzVisual* sphere = dvz_sphere(scene, DVZ_SPHERE_FLAGS_LIGHTING);
    DvzVisual* point = dvz_point(scene, 0);
    AT(mesh != NULL);
    AT(sphere != NULL);
    AT(point != NULL);
    AT(mesh->material.model == DVZ_MATERIAL_MODEL_PHONG);
    AT(point->material.model == DVZ_MATERIAL_MODEL_UNLIT);

    DvzMaterialDesc phong = dvz_material_desc();
    phong.alpha_mode = DVZ_ALPHA_WBOIT;
    phong.opacity = 0.5f;
    phong.base_color_factor[0] = 0.75f;
    phong.light_direction[0] = 1.0f;
    phong.light_direction[1] = 2.0f;
    phong.light_direction[2] = 3.0f;
    phong.phong.ambient = 0.15f;
    phong.phong.diffuse = 0.70f;
    phong.phong.specular = 0.40f;
    phong.phong.shininess = 48.0f;
    uint64_t version = mesh->material.version;
    AT(dvz_visual_set_material(mesh, &phong) == 0);
    AT(mesh->material.model == DVZ_MATERIAL_MODEL_PHONG);
    AT(mesh->alpha_mode == DVZ_ALPHA_WBOIT);
    AT(mesh->material.alpha_mode == DVZ_ALPHA_WBOIT);
    AT(mesh->material.opacity == 0.5f);
    AT(mesh->material.base_color_factor[0] == 0.75f);
    AT(mesh->material.light_direction[0] == 1.0f);
    AT(mesh->material.light_direction[1] == 2.0f);
    AT(mesh->material.light_direction[2] == 3.0f);
    AT(mesh->material.ambient == 0.15f);
    AT(mesh->material.diffuse == 0.70f);
    AT(mesh->material.specular == 0.40f);
    AT(mesh->material.shininess == 48.0f);
    AT(mesh->material_params.params[0] == 0.15f);
    AT(mesh->material_params.params[1] == 0.70f);
    AT(mesh->material_params.params[2] == 0.40f);
    AT(mesh->material_params.params[3] == 48.0f);
    AT(mesh->material_params.model[0] == (float)DVZ_MATERIAL_MODEL_PHONG);
    AT(mesh->material_params.model[1] == 0.5f);
    AT(mesh->material_params.base_color_factor[0] == 0.75f);
    AT(mesh->material.version > version);

    DvzMaterialDesc standard = dvz_material_desc();
    standard.model = DVZ_MATERIAL_MODEL_STANDARD;
    standard.alpha_mode = DVZ_ALPHA_OPAQUE;
    standard.opacity = 0.9f;
    standard.standard.roughness = 0.25f;
    standard.standard.specular = 0.6f;
    standard.standard.metallic = 0.2f;
    standard.standard.emissive[1] = 0.05f;
    standard.standard.rim_strength = 0.3f;
    version = mesh->material.version;
    AT(dvz_visual_set_material(mesh, &standard) == 0);
    AT(mesh->material.model == DVZ_MATERIAL_MODEL_STANDARD);
    AT(mesh->material.roughness == 0.25f);
    AT(mesh->material.standard_specular == 0.6f);
    AT(mesh->material.metallic == 0.2f);
    AT(mesh->material.emissive[1] == 0.05f);
    AT(mesh->material.rim_strength == 0.3f);
    AT(mesh->material_params.params[0] > 0.0f);
    AT(mesh->material_params.params[1] > 0.0f);
    AT(mesh->material_params.params[2] == 0.6f);
    AT(mesh->material_params.params[3] > 1.0f);
    AT(mesh->material_params.model[0] == (float)DVZ_MATERIAL_MODEL_STANDARD);
    AT(mesh->material_params.model[1] == 0.9f);
    AT(mesh->material_params.standard_params[0] == 0.25f);
    AT(mesh->material_params.standard_params[1] == 0.6f);
    AT(mesh->material_params.standard_params[2] == 0.2f);
    AT(mesh->material_params.standard_params[3] == 0.3f);
    AT(mesh->material_params.emissive_rim[1] == 0.05f);
    AT(mesh->material.version > version);

    AT(dvz_visual_set_depth_cue(
           mesh,
           &(DvzDepthCueDesc){
               .mode = DVZ_DEPTH_CUE_DARKEN,
               .near_depth = 0.1f,
               .far_depth = 0.9f,
               .strength = 0.4f,
               .background_color = {0.0f, 0.0f, 0.0f, 1.0f},
           }) == 0);
    AT(dvz_visual_set_material(mesh, NULL) == 0);
    AT(mesh->material.model == DVZ_MATERIAL_MODEL_PHONG);
    AT(mesh->alpha_mode == DVZ_ALPHA_OPAQUE);
    AT(mesh->material.depth_cue_enabled);
    AT(mesh->material_params.depth_cue[2] == 0.4f);
    AT(mesh->material_params.params[0] == 0.2f);
    AT(mesh->material_params.params[1] == 0.8f);

    AT(dvz_visual_set_primitive_shading(
           sphere,
           &(DvzPrimitiveShadingDesc){
               .light_direction = {0.0f, 1.0f, 0.0f},
               .ambient = 0.3f,
               .diffuse = 0.6f,
               .specular = 0.2f,
               .shininess = 16.0f,
           }) == 0);
    AT(sphere->material.model == DVZ_MATERIAL_MODEL_PHONG);
    AT(sphere->material.ambient == 0.3f);
    AT(sphere->material_params.params[3] == 16.0f);
    AT(sphere->material_params.depth_cue_extra[3] == (float)DVZ_SPHERE_MODE_FAST_IMPOSTOR);

#ifndef __clang_analyzer__
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_material(point, &phong) == -1);
    DvzMaterialDesc bad = dvz_material_desc();
    bad.opacity = 2.0f;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_material(mesh, &bad) == -1);
#endif

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify disabling pixel depth cueing returns to the plain pixel pipeline.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_pixel_depth_cue_toggle_switches_pipeline(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    ANN(item);

    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    DvzVisual* pixel = dvz_pixel(scene, 0);
    AT(scene != NULL);
    AT(figure != NULL);
    AT(panel != NULL);
    AT(pixel != NULL);

    float positions[3][3] = {
        {-0.5f, -0.5f, 0.0f},
        {+0.5f, -0.5f, 0.0f},
        {+0.0f, +0.5f, 0.0f},
    };
    DvzColor colors[3] = {{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}};
    float sizes[3] = {2.0f, 2.0f, 2.0f};
    AT(dvz_visual_set_data(pixel, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(pixel, "color", colors, 3) == 0);
    AT(dvz_visual_set_data(pixel, "size", sizes, 3) == 0);
    AT(dvz_panel_add_visual(panel, pixel, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;

    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    AT(dvz_visual_set_depth_cue(
           pixel,
           &(DvzDepthCueDesc){
               .mode = DVZ_DEPTH_CUE_DARKEN,
               .near_depth = 0.0f,
               .far_depth = 1.0f,
               .strength = 0.5f,
               .background_color = {0.0f, 0.0f, 0.0f, 1.0f},
           }) == 0);

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* cue_stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(cue_stream);
    AT(_stream_has_render_pipeline_label(cue_stream, "_pipe_pixel_cueg_depth"));
    AT(!_stream_has_render_pipeline_label(cue_stream, "_pipe_pixelg_depth"));
    dvz_drp2_stream_destroy(cue_stream);

    AT(dvz_visual_set_depth_cue(pixel, NULL) == 0);
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* plain_stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(plain_stream);
    AT(_stream_has_render_pipeline_label(plain_stream, "_pipe_pixelg_depth"));
    AT(!_stream_has_render_pipeline_label(plain_stream, "_pipe_pixel_cueg_depth"));

    dvz_drp2_stream_destroy(plain_stream);
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
    DvzVisual* sphere = dvz_sphere(scene, DVZ_SPHERE_FLAGS_LIGHTING);
    DvzVisual* image = dvz_image(scene, 0);
    DvzVisual* volume = dvz_volume(scene, 0);
    AT(point != NULL);
    AT(pixel != NULL);
    AT(primitive != NULL);
    AT(path != NULL);
    AT(fixed_primitive != NULL);
    AT(mesh != NULL);
    AT(sphere != NULL);
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
    AT(dvz_panel_add_visual(panel, sphere, NULL) == 0);
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

    AT(_scene_visual_pass_caps_from_visual(sphere, &panel->visuals[6], &caps));
    AT(caps.kind == DVZ_SCENE_VISUAL_DESC_SPHERE);
    AT(caps.draws_in_opaque_pass);
    AT(caps.writes_color);
    AT(caps.writes_depth);
    AT(caps.eligible_for_depth_postprocess);
    AT(caps.eligible_for_gbuffer);
    AT(caps.needs_material_layout);
    AT(caps.uses_material_set);
    AT(caps.supports_depth_cue);
    AT(_scene_technique_gbuffer_plan_add_visual(&gbuffer, sphere, &panel->visuals[6]));
    AT(gbuffer.producer_count == 2);

    AT(_scene_visual_pass_caps_from_visual(image, &panel->visuals[7], &caps));
    AT(caps.kind == DVZ_SCENE_VISUAL_DESC_IMAGE);
    AT(caps.draws_in_opaque_pass);
    AT(caps.writes_color);
    AT(!caps.writes_depth);
    AT(!caps.needs_depth_attachment);
    AT(!caps.eligible_for_depth_postprocess);
    AT(!caps.eligible_for_gbuffer);
    AT(caps.uses_image_set);
    AT(!_scene_technique_gbuffer_plan_add_visual(&gbuffer, image, &panel->visuals[7]));
    AT(gbuffer.producer_count == 2);

    AT(_scene_visual_pass_caps_from_visual(volume, &panel->visuals[8], &caps));
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
    AT(!_scene_technique_gbuffer_plan_add_visual(&gbuffer, volume, &panel->visuals[8]));
    AT(gbuffer.producer_count == 2);

    DvzSceneDrawContract draw_contract = {0};
    AT(_scene_draw_contract_from_visual(
        volume, &panel->visuals[8], DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND,
        &draw_contract));
    AT(draw_contract.visual_type == DVZ_VISUAL_TYPE_VOLUME);
    AT(draw_contract.alpha_mode == DVZ_ALPHA_BLENDED);
    AT(draw_contract.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND);
    AT(!draw_contract.depth_test);
    AT(!draw_contract.depth_write);
    AT(draw_contract.samples_depth);
    AT(draw_contract.needs_common_set);
    AT(draw_contract.needs_volume_set);
    AT(!draw_contract.needs_scene_occlusion_set);

    DvzSceneVisualDesc desc = {
        .kind = DVZ_SCENE_VISUAL_DESC_PRIMITIVE,
        .has_normal = true,
        .depth_test_enabled = true,
        .material_buffer_id = 42,
    };
    AT(_scene_visual_pass_caps_from_desc(
        &desc, DVZ_ALPHA_BLENDED, DVZ_CONTROLLER_APPLY, &caps));
    AT(caps.draws_in_transparent_blend_pass);
    AT(!caps.draws_in_opaque_pass);
    AT(caps.uses_source_over_blend);
    AT(!caps.writes_depth);
    AT(caps.can_depth_test);
    AT(caps.needs_depth_attachment);
    AT(!caps.eligible_for_depth_postprocess);
    AT(!caps.eligible_for_gbuffer);
    AT(caps.needs_material_layout);
    AT(caps.uses_material_set);
    AT(caps.supports_depth_cue);

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify the draw-contract resolver matrix maps visual facts and pass roles consistently.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_draw_contract_resolver_matrix(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzSceneDrawContract contract = {0};
    DvzSceneDrawFacts facts = {
        .visual_type = DVZ_VISUAL_TYPE_MESH,
        .alpha_mode = DVZ_ALPHA_OPAQUE,
        .can_depth_test = true,
        .can_write_depth = true,
        .writes_depth = true,
        .samples_depth = true,
        .uses_common_set = true,
        .uses_material_set = true,
    };
    AT(_scene_draw_contract_resolve(
        &facts, DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE, &contract));
    AT(contract.visual_type == DVZ_VISUAL_TYPE_MESH);
    AT(contract.alpha_mode == DVZ_ALPHA_OPAQUE);
    AT(contract.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE);
    AT(contract.depth_test);
    AT(contract.depth_write);
    AT(!contract.samples_depth);
    AT(
        contract.depth_policy ==
        (DVZ_SCENE_DEPTH_POLICY_TEST | DVZ_SCENE_DEPTH_POLICY_WRITE));
    AT(contract.blend_policy == DVZ_SCENE_BLEND_POLICY_OPAQUE);
    AT(contract.shader_feature_mask == 0);
    AT(
        contract.bind_group_layout_mask ==
        (DVZ_SCENE_BIND_GROUP_REQUIREMENT_COMMON |
         DVZ_SCENE_BIND_GROUP_REQUIREMENT_MATERIAL));
    AT(contract.needs_common_set);
    AT(contract.needs_material_set);

    facts = (DvzSceneDrawFacts){
        .visual_type = DVZ_VISUAL_TYPE_VOLUME,
        .alpha_mode = DVZ_ALPHA_BLENDED,
        .samples_depth = true,
        .volume_occluded = true,
        .scene_occluded = true,
        .uses_common_set = true,
        .uses_volume_set = true,
    };
    AT(_scene_draw_contract_resolve(
        &facts, DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND, &contract));
    AT(contract.visual_type == DVZ_VISUAL_TYPE_VOLUME);
    AT(contract.alpha_mode == DVZ_ALPHA_BLENDED);
    AT(!contract.depth_test);
    AT(!contract.depth_write);
    AT(contract.samples_depth);
    AT(contract.samples_volume_occlusion);
    AT(contract.samples_scene_occlusion);
    AT(contract.depth_policy == DVZ_SCENE_DEPTH_POLICY_SAMPLE);
    AT(contract.blend_policy == DVZ_SCENE_BLEND_POLICY_SOURCE_OVER);
    AT(
        contract.shader_feature_mask ==
        (DVZ_SCENE_SHADER_FEATURE_SAMPLE_DEPTH |
         DVZ_SCENE_SHADER_FEATURE_SAMPLE_VOLUME_OCCLUSION |
         DVZ_SCENE_SHADER_FEATURE_SAMPLE_SCENE_OCCLUSION));
    AT(
        contract.bind_group_layout_mask ==
        (DVZ_SCENE_BIND_GROUP_REQUIREMENT_COMMON |
         DVZ_SCENE_BIND_GROUP_REQUIREMENT_VOLUME |
         DVZ_SCENE_BIND_GROUP_REQUIREMENT_SCENE_OCCLUSION));
    AT(contract.needs_common_set);
    AT(contract.needs_volume_set);
    AT(contract.needs_scene_occlusion_set);

    facts = (DvzSceneDrawFacts){
        .visual_type = DVZ_VISUAL_TYPE_MESH,
        .alpha_mode = DVZ_ALPHA_WBOIT,
        .can_depth_test = true,
        .can_write_depth = true,
        .uses_common_set = true,
        .uses_material_set = true,
    };
    AT(_scene_draw_contract_resolve(
        &facts, DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION, &contract));
    AT(contract.alpha_mode == DVZ_ALPHA_WBOIT);
    AT(contract.depth_test);
    AT(!contract.depth_write);
    AT(!contract.samples_depth);
    AT(contract.depth_policy == DVZ_SCENE_DEPTH_POLICY_TEST);
    AT(contract.blend_policy == DVZ_SCENE_BLEND_POLICY_WBOIT);
    AT(contract.needs_material_set);

    facts = (DvzSceneDrawFacts){
        .visual_type = DVZ_VISUAL_TYPE_MESH,
        .alpha_mode = DVZ_ALPHA_OPAQUE,
        .can_depth_test = true,
        .can_write_depth = true,
        .scene_occluder = true,
        .uses_common_set = true,
    };
    AT(_scene_draw_contract_resolve(
        &facts, DVZ_FRAME_PLAN_RENDER_PASS_SCENE_OCCLUSION, &contract));
    AT(contract.depth_test);
    AT(contract.depth_write);
    AT(contract.writes_scene_occlusion_depth);
    AT(
        contract.depth_policy ==
        (DVZ_SCENE_DEPTH_POLICY_TEST | DVZ_SCENE_DEPTH_POLICY_WRITE));
    AT(contract.blend_policy == DVZ_SCENE_BLEND_POLICY_NONE);
    AT(
        contract.shader_feature_mask ==
        DVZ_SCENE_SHADER_FEATURE_WRITE_SCENE_OCCLUSION);
    AT(!contract.samples_scene_occlusion);
    AT(!contract.needs_scene_occlusion_set);

    return 0;
}


/**
 * Verify every render-pass role has one centralized graph work-label mapping.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_role_work_label_mapping_complete(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    const struct
    {
        DvzFramePlanRenderPassRole role;
        const char* label;
        bool graph_required;
    } rows[] = {
        {DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE, "opaque", false},
        {DVZ_FRAME_PLAN_RENDER_PASS_GBUFFER, "gbuffer", true},
        {DVZ_FRAME_PLAN_RENDER_PASS_VOLUME_OCCLUSION, "volume_occlusion", true},
        {DVZ_FRAME_PLAN_RENDER_PASS_SCENE_OCCLUSION, "scene_occlusion", true},
        {DVZ_FRAME_PLAN_RENDER_PASS_SSAO, "ssao", true},
        {DVZ_FRAME_PLAN_RENDER_PASS_SSAO_BLUR, "ssao_blur", true},
        {DVZ_FRAME_PLAN_RENDER_PASS_SSAO_COMPOSITE, "ssao_composite", true},
        {DVZ_FRAME_PLAN_RENDER_PASS_EDL_RESOLVE, "edl_resolve", true},
        {DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION, "wboit_accum", true},
        {DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND, "transparent_blend", true},
        {DVZ_FRAME_PLAN_RENDER_PASS_WBOIT_RESOLVE, "wboit_resolve", true},
        {DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT, "depth_peel_init", true},
        {DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER, "depth_peel_iter", true},
        {DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_COMPOSITE, "depth_peel_composite", true},
        {DVZ_FRAME_PLAN_RENDER_PASS_PICKING, "picking", false},
    };
    for (uint32_t i = 0; i < sizeof(rows) / sizeof(rows[0]); i++)
    {
        DvzSceneTechniquePassPolicy policy = {0};
        AT(_scene_technique_pass_policy(rows[i].role, &policy));
        AT(policy.role == rows[i].role);
        AT(strcmp(policy.work_label, rows[i].label) == 0);
        AT(policy.graph_required == rows[i].graph_required);
        AT(strcmp(_scene_render_role_work_label(rows[i].role), rows[i].label) == 0);
        AT(_scene_render_role_requires_graph_pass(rows[i].role) == rows[i].graph_required);
    }
    DvzSceneTechniquePassPolicy policy = {0};
    AT(_scene_technique_pass_policy(DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND, &policy));
    AT(policy.source_over_blend);
    AT(!policy.wboit_accumulation);
    AT(!policy.depth_peel);
    AT(!policy.fullscreen_resolve);
    AT(policy.sampled_texture_binding_count == 0);

    AT(_scene_technique_pass_policy(DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION, &policy));
    AT(!policy.source_over_blend);
    AT(policy.wboit_accumulation);
    AT(!policy.depth_peel);
    AT(!policy.fullscreen_resolve);
    AT(policy.sampled_texture_binding_count == 0);

    AT(_scene_technique_pass_policy(DVZ_FRAME_PLAN_RENDER_PASS_WBOIT_RESOLVE, &policy));
    AT(policy.fullscreen_resolve);
    AT(policy.needs_wboit_resolve_layout);
    AT(!policy.needs_depth_peel_sampled_layout);
    AT(policy.sampled_texture_binding_count == 2);

    AT(_scene_technique_pass_policy(DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT, &policy));
    AT(policy.depth_peel);
    AT(!policy.fullscreen_resolve);

    AT(_scene_technique_pass_policy(DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER, &policy));
    AT(policy.depth_peel);
    AT(!policy.fullscreen_resolve);

    AT(_scene_technique_pass_policy(DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_COMPOSITE, &policy));
    AT(!policy.depth_peel);
    AT(policy.fullscreen_resolve);
    AT(!policy.needs_wboit_resolve_layout);
    AT(policy.needs_depth_peel_sampled_layout);
    AT(policy.sampled_texture_binding_count == 3);

    AT(_scene_technique_pass_policy(DVZ_FRAME_PLAN_RENDER_PASS_EDL_RESOLVE, &policy));
    AT(policy.fullscreen_resolve);

    AT(_scene_technique_pass_policy(DVZ_FRAME_PLAN_RENDER_PASS_SSAO_COMPOSITE, &policy));
    AT(policy.fullscreen_resolve);

    return 0;
}


/**
 * Verify invalid passive render contracts are rejected before runtime lowering.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_render_contract_validation_errors(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzDiagnosticReport report = {0};
    DvzScenePassContract contract = {0};

    contract.role = DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND;
    contract.source_over_blend = true;
    contract.draw_count = 1;
    contract.draws[0].alpha_mode = DVZ_ALPHA_BLENDED;
    contract.draws[0].pass_role = DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND;
    contract.draws[0].depth_test = true;
    dvz_diagnostic_report_init(&report);
    AT(!_scene_pass_contract_validate(&contract, &report));
    AT(dvz_diagnostic_report_count(&report) > 0);

    dvz_memset(&contract, sizeof(contract), 0, sizeof(contract));
    contract.role = DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND;
    contract.source_over_blend = true;
    contract.draw_count = 1;
    contract.draws[0].alpha_mode = DVZ_ALPHA_BLENDED;
    contract.draws[0].pass_role = DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND;
    contract.draws[0].depth_write = true;
    dvz_diagnostic_report_init(&report);
    AT(!_scene_pass_contract_validate(&contract, &report));
    AT(dvz_diagnostic_report_count(&report) > 0);

    contract.has_depth_attachment = true;
    contract.attachment_count = 1;
    contract.attachments[0].role = DVZ_SCENE_ATTACHMENT_DEPTH;
    contract.attachments[0].write = true;
    dvz_diagnostic_report_init(&report);
    AT(!_scene_pass_contract_validate(&contract, &report));
    AT(dvz_diagnostic_report_count(&report) > 0);

    dvz_memset(&contract, sizeof(contract), 0, sizeof(contract));
    contract.role = DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND;
    contract.source_over_blend = true;
    contract.draw_count = 1;
    contract.draws[0].alpha_mode = DVZ_ALPHA_BLENDED;
    contract.draws[0].pass_role = DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND;
    contract.draws[0].samples_depth = true;
    contract.has_depth_attachment = true;
    contract.attachment_count = 1;
    contract.attachments[0].role = DVZ_SCENE_ATTACHMENT_DEPTH;
    contract.attachments[0].load_op = DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR;
    contract.attachments[0].access = DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE;
    dvz_diagnostic_report_init(&report);
    AT(!_scene_pass_contract_validate(&contract, &report));
    AT(dvz_diagnostic_report_count(&report) > 0);

    dvz_memset(&contract, sizeof(contract), 0, sizeof(contract));
    contract.role = DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE;
    contract.draw_count = 1;
    contract.draws[0].alpha_mode = DVZ_ALPHA_OPAQUE;
    contract.draws[0].pass_role = DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE;
    contract.draws[0].samples_scene_occlusion = true;
    contract.draws[0].needs_scene_occlusion_set = true;
    dvz_diagnostic_report_init(&report);
    AT(!_scene_pass_contract_validate(&contract, &report));
    AT(dvz_diagnostic_report_count(&report) > 0);

    dvz_memset(&contract, sizeof(contract), 0, sizeof(contract));
    contract.role = DVZ_FRAME_PLAN_RENDER_PASS_WBOIT_RESOLVE;
    contract.fullscreen_resolve = true;
    contract.color_attachment_count = 1;
    dvz_diagnostic_report_init(&report);
    AT(!_scene_pass_contract_validate(&contract, &report));
    AT(dvz_diagnostic_report_count(&report) > 0);

    return 0;
}



/**
 * Verify graph-backed render roles fail contract validation when their graph pass is missing.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_frame_plan_missing_graph_pass_fails_contract(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    DvzFramePlan* plan = dvz_frame_plan("figure_0", 0);
    ANN(plan);
    AT(dvz_frame_plan_render_panel_role(
        plan, "figure_0_p0", "rt.gbuffer.normal", false, panel->desc,
        DVZ_FRAME_PLAN_RENDER_PASS_GBUFFER));

    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    AT(!_scene_frame_plan_contracts_validate(figure, plan, &report));
    AT(dvz_diagnostic_report_count(&report) == 1);
    const char* message = dvz_diagnostic_report_get(&report, 0);
    ANN(message);
    AT(strstr(message, "no matching graph pass") != NULL);

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);
    dvz_diagnostic_report_init(&report);
    AT(!_scene_frame_plan_drp2_contracts_validate(plan, stream, &report));
    AT(dvz_diagnostic_report_count(&report) == 1);
    message = dvz_diagnostic_report_get(&report, 0);
    ANN(message);
    AT(strstr(message, "no matching graph pass") != NULL);

    dvz_drp2_stream_destroy(stream);
    dvz_frame_plan_destroy(plan);
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
 * Verify render-node indices survive FramePlan node storage growth during panel emission.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_frame_plan_node_reallocation_safe(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    _scene_technique_state_enable_gbuffer(&panel->techniques, true);

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

    for (uint32_t i = 0; i < 3; i++)
    {
        DvzVisual* mesh = dvz_mesh(scene, 0);
        AT(mesh != NULL);
        AT(dvz_visual_set_data(mesh, "position", positions, 4) == 0);
        AT(dvz_visual_set_data(mesh, "normal", normals, 4) == 0);
        AT(dvz_visual_set_buffer(mesh, "index", index_buffer));
        AT(dvz_panel_add_visual(panel, mesh, NULL) == 0);
    }

    float point_positions[3][3] = {
        {-0.25f, -0.25f, 0.1f},
        {0.25f, -0.25f, 0.1f},
        {0.0f, 0.25f, 0.1f},
    };
    DvzColor blended_colors[3] = {{255, 0, 0, 128}, {0, 255, 0, 128}, {0, 0, 255, 128}};
    float point_sizes[3] = {10.0f, 12.0f, 14.0f};
    for (uint32_t i = 0; i < 2; i++)
    {
        DvzVisual* blended = dvz_point(scene, 0);
        AT(blended != NULL);
        AT(dvz_visual_set_data(blended, "position", point_positions, 3) == 0);
        AT(dvz_visual_set_data(blended, "color", blended_colors, 3) == 0);
        AT(dvz_visual_set_data(blended, "size", point_sizes, 3) == 0);
        AT(dvz_visual_set_alpha_mode(blended, DVZ_ALPHA_BLENDED) == 0);
        AT(dvz_panel_add_visual(panel, blended, NULL) == 0);
    }
    for (uint32_t i = 0; i < 2; i++)
    {
        DvzVisual* wboit = dvz_point(scene, 0);
        AT(wboit != NULL);
        AT(dvz_visual_set_data(wboit, "position", point_positions, 3) == 0);
        AT(dvz_visual_set_data(wboit, "color", blended_colors, 3) == 0);
        AT(dvz_visual_set_data(wboit, "size", point_sizes, 3) == 0);
        AT(dvz_visual_set_alpha_mode(wboit, DVZ_ALPHA_WBOIT) == 0);
        AT(dvz_panel_add_visual(panel, wboit, NULL) == 0);
    }

    DvzFramePlan* plan = dvz_frame_plan("figure.gbuffer.realloc", 0);
    ANN(plan);
    for (uint32_t i = 0; i + 1 < DVZ_FRAME_PLAN_INITIAL_NODE_CAPACITY; i++)
        AT(dvz_frame_plan_clear_panel(plan, "prefill", "rt", panel->desc));
    AT(dvz_frame_plan_node_count(plan) == DVZ_FRAME_PLAN_INITIAL_NODE_CAPACITY - 1);

    AT(_scene_emit_panel_render(figure, 0, plan, "figure_0"));
    AT(dvz_frame_plan_node_count(plan) == DVZ_FRAME_PLAN_INITIAL_NODE_CAPACITY + 4);

    const DvzFramePlanNode* gbuffer_node =
        dvz_frame_plan_node_get(plan, DVZ_FRAME_PLAN_INITIAL_NODE_CAPACITY - 1);
    const DvzFramePlanNode* opaque_node =
        dvz_frame_plan_node_get(plan, DVZ_FRAME_PLAN_INITIAL_NODE_CAPACITY);
    const DvzFramePlanNode* blended_node =
        dvz_frame_plan_node_get(plan, DVZ_FRAME_PLAN_INITIAL_NODE_CAPACITY + 1);
    const DvzFramePlanNode* wboit_node =
        dvz_frame_plan_node_get(plan, DVZ_FRAME_PLAN_INITIAL_NODE_CAPACITY + 2);
    const DvzFramePlanNode* resolve_node =
        dvz_frame_plan_node_get(plan, DVZ_FRAME_PLAN_INITIAL_NODE_CAPACITY + 3);
    ANN(gbuffer_node);
    ANN(opaque_node);
    ANN(blended_node);
    ANN(wboit_node);
    ANN(resolve_node);
    AT(dvz_frame_plan_render_pass_role(gbuffer_node) == DVZ_FRAME_PLAN_RENDER_PASS_GBUFFER);
    AT(dvz_frame_plan_render_pass_role(opaque_node) == DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE);
    AT(
        dvz_frame_plan_render_pass_role(blended_node) ==
        DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND);
    AT(
        dvz_frame_plan_render_pass_role(wboit_node) ==
        DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION);
    AT(dvz_frame_plan_render_pass_role(resolve_node) == DVZ_FRAME_PLAN_RENDER_PASS_WBOIT_RESOLVE);
    AT(gbuffer_node->u.render.visual_count == 3);
    AT(opaque_node->u.render.visual_count == 3);
    AT(blended_node->u.render.visual_count == 2);
    AT(wboit_node->u.render.visual_count == 2);
    AT(resolve_node->u.render.visual_count == 0);

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    AT(dvz_frame_plan_graph_validate(plan, &report));
    AT(dvz_diagnostic_report_count(&report) == 0);
    dvz_diagnostic_report_init(&report);
    AT(_scene_frame_plan_contracts_validate(figure, plan, &report));
    AT(dvz_diagnostic_report_count(&report) == 0);

    dvz_frame_plan_destroy(plan);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify panel MSAA lowers through graph resources, resolves, and DRP2 pipeline samples.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_msaa_runtime_lowering(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    AT(dvz_panel_set_msaa(
        panel, &(DvzMsaaDesc){.enabled = true, .sample_count = 4, .alpha_to_coverage = true}));

    DvzVisual* sphere = dvz_sphere(scene, 0);
    AT(sphere != NULL);
    float positions[1][3] = {{0.0f, 0.0f, 0.0f}};
    DvzColor colors[1] = {{255, 128, 64, 255}};
    float sizes[1] = {0.35f};
    AT(dvz_visual_set_data(sphere, "position", positions, 1) == 0);
    AT(dvz_visual_set_data(sphere, "color", colors, 1) == 0);
    AT(dvz_visual_set_data(sphere, "size", sizes, 1) == 0);
    AT(dvz_panel_add_visual(panel, sphere, NULL) == 0);

    DvzFramePlan* plan = dvz_frame_plan("figure.msaa", 0);
    ANN(plan);
    _scene_emit_panel_render(figure, 0, plan, "figure_0");
    AT(dvz_frame_plan_node_count(plan) == 1);
    AT(dvz_frame_plan_graph_resource_count(plan) == 3);
    AT(dvz_frame_plan_graph_pass_count(plan) == 1);

    const DvzFrameGraphResource* msaa_color = NULL;
    const DvzFrameGraphResource* depth = NULL;
    for (uint32_t i = 0; i < dvz_frame_plan_graph_resource_count(plan); i++)
    {
        const DvzFrameGraphResource* resource = dvz_frame_plan_graph_resource_get(plan, i);
        ANN(resource);
        if (strcmp(resource->id, "figure_0_p0.msaa.color") == 0)
            msaa_color = resource;
        else if (strcmp(resource->id, "figure_0_p0.depth") == 0)
            depth = resource;
    }
    ANN(msaa_color);
    ANN(depth);
    AT(msaa_color->sample_count == 4);
    AT(depth->sample_count == 4);

    const DvzFrameGraphPass* pass = dvz_frame_plan_graph_pass_get(plan, 0);
    ANN(pass);
    AT(strcmp(pass->work_label, "opaque") == 0);
    AT(pass->color_attachment_count == 1);
    AT(strcmp(pass->color_attachments[0].resource_id, "figure_0_p0.msaa.color") == 0);
    AT(strcmp(pass->color_attachments[0].resolve_resource_id, "rt") == 0);
    AT(pass->color_attachments[0].resolve_mode == VK_RESOLVE_MODE_AVERAGE_BIT);
    AT(pass->has_depth_attachment);
    AT(strcmp(pass->depth_attachment.resource_id, "figure_0_p0.depth") == 0);

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

    uint64_t msaa_texture_id = 0;
    bool found_msaa_texture = false;
    bool found_depth_texture = false;
    bool found_resolve_pass = false;
    bool found_msaa_pipeline = false;
    bool found_sphere_a2c_shader = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        ANN(cmd);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE)
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.create_texture.id);
            if (label != NULL && strcmp(label, "fig0_p0.msaa.color") == 0)
            {
                msaa_texture_id = cmd->u.create_texture.id;
                found_msaa_texture = cmd->u.create_texture.sample_count == 4 &&
                                     (cmd->u.create_texture.usage &
                                      DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT) != 0;
            }
            if (label != NULL && strcmp(label, "fig0_p0.depth") == 0)
            {
                found_depth_texture = cmd->u.create_texture.sample_count == 4 &&
                                      cmd->u.create_texture.format == VK_FORMAT_D32_SFLOAT;
            }
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE)
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.create_shader_module.id);
            found_sphere_a2c_shader =
                found_sphere_a2c_shader ||
                (label != NULL && strcmp(label, "_fs_sphereg_a2c") == 0);
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS)
        {
            found_resolve_pass =
                found_resolve_pass ||
                (cmd->u.begin_render_pass.texture_id == msaa_texture_id &&
                 cmd->u.begin_render_pass.color_attachments[0].resolve_texture_id != 0 &&
                 cmd->u.begin_render_pass.color_attachments[0].resolve_texture_id !=
                     msaa_texture_id &&
                 cmd->u.begin_render_pass.color_attachments[0].resolve_mode ==
                     VK_RESOLVE_MODE_AVERAGE_BIT);
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.create_render_pipeline.id);
            found_msaa_pipeline =
                found_msaa_pipeline ||
                (label != NULL && strstr(label, "_pipe_sphere") != NULL &&
                 cmd->u.create_render_pipeline.sample_count == 4 &&
                 cmd->u.create_render_pipeline.alpha_to_coverage_enabled);
        }
    }
    AT(found_msaa_texture);
    AT(found_depth_texture);
    AT(found_resolve_pass);
    AT(found_msaa_pipeline);
    AT(found_sphere_a2c_shader);

    dvz_drp2_stream_destroy(stream);
    dvz_frame_plan_destroy(plan);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify runtime MSAA emission is lowered to device sample-count capabilities.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_msaa_runtime_capability_lowering(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    AT(dvz_panel_set_msaa(
        panel, &(DvzMsaaDesc){.enabled = true, .sample_count = 16,
                              .alpha_to_coverage = true}));

    DvzVisual* sphere = dvz_sphere(scene, 0);
    AT(sphere != NULL);
    float positions[1][3] = {{0.0f, 0.0f, 0.0f}};
    DvzColor colors[1] = {{255, 128, 64, 255}};
    float sizes[1] = {0.35f};
    AT(dvz_visual_set_data(sphere, "position", positions, 1) == 0);
    AT(dvz_visual_set_data(sphere, "color", colors, 1) == 0);
    AT(dvz_visual_set_data(sphere, "size", sizes, 1) == 0);
    AT(dvz_panel_add_visual(panel, sphere, NULL) == 0);

    DvzFramePlan* plan = dvz_frame_plan("figure.msaa", 0);
    ANN(plan);
    _scene_emit_panel_render(figure, 0, plan, "figure_0");

    const DvzFrameGraphResource* msaa_color = NULL;
    const DvzFrameGraphResource* depth = NULL;
    for (uint32_t i = 0; i < dvz_frame_plan_graph_resource_count(plan); i++)
    {
        const DvzFrameGraphResource* resource = dvz_frame_plan_graph_resource_get(plan, i);
        ANN(resource);
        if (strcmp(resource->id, "figure_0_p0.msaa.color") == 0)
            msaa_color = resource;
        else if (strcmp(resource->id, "figure_0_p0.depth") == 0)
            depth = resource;
    }
    ANN(msaa_color);
    ANN(depth);
    AT(msaa_color->sample_count == 16);
    AT(depth->sample_count == 16);

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.target_width = 64;
    cfg.target_height = 64;
    dvz_capability_snapshot_default(&caps);
    caps.max_color_sample_count = 16;
    caps.max_depth_sample_count = 8;
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) >= 2);
    const char* fallback_message = dvz_diagnostic_report_get(&report, 0);
    ANN(fallback_message);
    AT(strstr(fallback_message, "sample count lowered from 16 to 8") != NULL);
    DvzDrp2ValidationResult validation = dvz_drp2_validate_stream(stream);
    AT(validation.ok);

    bool found_msaa_texture = false;
    bool found_depth_texture = false;
    bool found_msaa_pipeline = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        ANN(cmd);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE)
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.create_texture.id);
            found_msaa_texture =
                found_msaa_texture ||
                (label != NULL && strcmp(label, "fig0_p0.msaa.color") == 0 &&
                 cmd->u.create_texture.sample_count == 8);
            found_depth_texture =
                found_depth_texture ||
                (label != NULL && strcmp(label, "fig0_p0.depth") == 0 &&
                 cmd->u.create_texture.sample_count == 8);
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.create_render_pipeline.id);
            found_msaa_pipeline =
                found_msaa_pipeline ||
                (label != NULL && strstr(label, "_pipe_sphere") != NULL &&
                 cmd->u.create_render_pipeline.sample_count == 8 &&
                 cmd->u.create_render_pipeline.alpha_to_coverage_enabled);
        }
    }
    AT(found_msaa_texture);
    AT(found_depth_texture);
    AT(found_msaa_pipeline);

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
 * Verify point, primitive, and mesh depth producers can feed the EDL post-process branch.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_edl_depth_producer_capabilities(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    float point_positions[3][3] = {
        {-0.90f, +0.45f, 0.0f},
        {-0.65f, +0.45f, 0.1f},
        {-0.78f, +0.70f, 0.2f},
    };
    DvzColor point_colors[3] = {
        {80, 170, 235, 255},
        {80, 170, 235, 255},
        {80, 170, 235, 255},
    };
    float point_sizes[3] = {12.0f, 12.0f, 12.0f};
    DvzVisual* point = dvz_point(scene, 0);
    AT(point != NULL);
    AT(dvz_visual_set_data(point, "position", point_positions, 3) == 0);
    AT(dvz_visual_set_data(point, "color", point_colors, 3) == 0);
    AT(dvz_visual_set_data(point, "size", point_sizes, 3) == 0);
    AT(dvz_panel_add_visual(panel, point, NULL) == 0);

    float primitive_positions[3][3] = {
        {-0.80f, -0.60f, 0.1f},
        {-0.20f, -0.60f, 0.1f},
        {-0.50f, +0.10f, 0.1f},
    };
    DvzColor primitive_colors[3] = {
        {240, 90, 70, 255},
        {240, 90, 70, 255},
        {240, 90, 70, 255},
    };
    DvzVisual* primitive = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    AT(primitive != NULL);
    AT(dvz_visual_set_data(primitive, "position", primitive_positions, 3) == 0);
    AT(dvz_visual_set_data(primitive, "color", primitive_colors, 3) == 0);
    AT(dvz_panel_add_visual(panel, primitive, NULL) == 0);

    float mesh_positions[4][3] = {
        {0.10f, -0.50f, 0.2f},
        {0.70f, -0.50f, 0.2f},
        {0.10f, +0.30f, 0.2f},
        {0.70f, +0.30f, 0.2f},
    };
    float mesh_normals[4][3] = {
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
    };
    DvzIndex mesh_indices[6] = {0, 1, 2, 2, 1, 3};
    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, mesh_indices, sizeof(mesh_indices)));

    DvzVisual* mesh = dvz_mesh(scene, 0);
    AT(mesh != NULL);
    AT(dvz_visual_set_data(mesh, "position", mesh_positions, 4) == 0);
    AT(dvz_visual_set_data(mesh, "normal", mesh_normals, 4) == 0);
    AT(dvz_visual_set_buffer(mesh, "index", index_buffer));
    AT(dvz_panel_add_visual(panel, mesh, NULL) == 0);

    AT(dvz_panel_set_edl(
        panel, &(DvzEdlDesc){.radius = 2.0f, .strength = 55.0f, .depth_scale = 1.0f}));

    DvzFramePlan* plan = dvz_frame_plan("figure.edl.depth_producers", 0);
    ANN(plan);
    _scene_emit_panel_render(figure, 0, plan, "figure_0");
    AT(dvz_frame_plan_node_count(plan) == 3);
    const DvzFramePlanNode* opaque_node = dvz_frame_plan_node_get(plan, 0);
    const DvzFramePlanNode* edl_node = dvz_frame_plan_node_get(plan, 2);
    ANN(opaque_node);
    ANN(edl_node);
    AT(dvz_frame_plan_render_pass_role(opaque_node) == DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE);
    AT(opaque_node->u.render.visual_count == 3);
    AT(dvz_frame_plan_render_pass_role(edl_node) == DVZ_FRAME_PLAN_RENDER_PASS_EDL_RESOLVE);
    AT(dvz_frame_plan_graph_pass_count(plan) == 2);
    const DvzFrameGraphPass* opaque_pass = dvz_frame_plan_graph_pass_get(plan, 0);
    ANN(opaque_pass);
    AT(opaque_pass->has_depth_attachment);
    AT(strcmp(opaque_pass->depth_attachment.resource_id, "figure_0_p0.edl.depth") == 0);

    dvz_frame_plan_destroy(plan);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Verify EDL stays inactive when enabled on visuals without eligible opaque depth producers.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_edl_ignores_ineligible_passes(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* fixed_panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 0.5f, 1.0f});
    DvzPanel* transparent_panel = dvz_panel(figure, (DvzPanelDesc){0.5f, 0.0f, 0.5f, 1.0f});
    AT(fixed_panel != NULL);
    AT(transparent_panel != NULL);

    float positions[3][3] = {
        {-0.5f, -0.5f, 0.0f},
        {+0.5f, -0.5f, 0.0f},
        {0.0f, +0.5f, 0.0f},
    };
    DvzColor colors[3] = {
        {220, 80, 80, 255},
        {220, 80, 80, 255},
        {220, 80, 80, 255},
    };
    DvzVisual* fixed_visual = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    AT(fixed_visual != NULL);
    AT(dvz_visual_set_data(fixed_visual, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(fixed_visual, "color", colors, 3) == 0);
    AT(dvz_panel_add_visual(
           fixed_panel, fixed_visual,
           &(DvzVisualAttachDesc){.controller_mode = DVZ_CONTROLLER_FIXED}) == 0);
    AT(dvz_panel_set_edl(
        fixed_panel, &(DvzEdlDesc){.radius = 2.0f, .strength = 55.0f, .depth_scale = 1.0f}));

    DvzFramePlan* fixed_plan = dvz_frame_plan("figure.edl.fixed", 0);
    ANN(fixed_plan);
    _scene_emit_panel_render(figure, 0, fixed_plan, "figure_0");
    AT(dvz_frame_plan_node_count(fixed_plan) == 1);
    AT(dvz_frame_plan_graph_pass_count(fixed_plan) == 0);

    float point_sizes[3] = {18.0f, 18.0f, 18.0f};
    DvzVisual* transparent_point = dvz_point(scene, 0);
    AT(transparent_point != NULL);
    AT(dvz_visual_set_data(transparent_point, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(transparent_point, "color", colors, 3) == 0);
    AT(dvz_visual_set_data(transparent_point, "size", point_sizes, 3) == 0);
    AT(dvz_visual_set_alpha_mode(transparent_point, DVZ_ALPHA_WBOIT) == 0);
    AT(dvz_panel_add_visual(transparent_panel, transparent_point, NULL) == 0);
    AT(dvz_panel_set_edl(
        transparent_panel,
        &(DvzEdlDesc){.radius = 2.0f, .strength = 55.0f, .depth_scale = 1.0f}));

    DvzFramePlan* transparent_plan = dvz_frame_plan("figure.edl.transparent", 0);
    ANN(transparent_plan);
    _scene_emit_panel_render(figure, 1, transparent_plan, "figure_0");
    bool found_edl_resolve = false;
    bool found_wboit_resolve = false;
    for (uint32_t i = 0; i < dvz_frame_plan_graph_pass_count(transparent_plan); i++)
    {
        const DvzFrameGraphPass* pass = dvz_frame_plan_graph_pass_get(transparent_plan, i);
        ANN(pass);
        found_edl_resolve =
            found_edl_resolve || strcmp(pass->work_label, "edl_resolve") == 0;
        found_wboit_resolve =
            found_wboit_resolve || strcmp(pass->work_label, "wboit_resolve") == 0;
    }
    AT(!found_edl_resolve);
    AT(found_wboit_resolve);

    dvz_frame_plan_destroy(transparent_plan);
    dvz_frame_plan_destroy(fixed_plan);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Verify opt-in SSAO declares a G-buffer-backed graph without changing the default path.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_ssao_graph_foundation(TstSuite* suite, TstItem* item)
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

    AT(!_scene_technique_state_ssao_enabled(&panel->techniques));
    AT(panel->techniques.ssao.radius == 0.5f);
    AT(panel->techniques.ssao.strength == 1.0f);
    AT(panel->techniques.ssao.bias == 0.025f);
    AT(panel->techniques.ssao.sample_count == 16);

    DvzFramePlan* default_plan = dvz_frame_plan("figure.ssao.default", 0);
    ANN(default_plan);
    _scene_emit_panel_render(figure, 0, default_plan, "figure_0");
    AT(dvz_frame_plan_node_count(default_plan) == 1);
    AT(dvz_frame_plan_graph_pass_count(default_plan) == 0);
    dvz_frame_plan_destroy(default_plan);

    AT(_scene_technique_state_set_ssao(
        &panel->techniques,
        &(DvzSceneSsaoDesc){.radius = 1.25f, .strength = 2.0f, .bias = 0.05f,
                            .sample_count = 32}));
    const DvzSceneSsaoTechniqueState* ssao = _scene_technique_ssao_state(scene, panel);
    ANN(ssao);
    AT(ssao->enabled);
    AT(ssao->radius == 1.25f);
    AT(ssao->strength == 2.0f);
    AT(ssao->bias == 0.05f);
    AT(ssao->sample_count == 32);
    AT(!ssao->blur_enabled);

    DvzFramePlan* plan = dvz_frame_plan("figure.ssao", 0);
    ANN(plan);
    _scene_emit_panel_render(figure, 0, plan, "figure_0");
    AT(dvz_frame_plan_node_count(plan) == 5);
    AT(dvz_frame_plan_graph_pass_count(plan) == 4);
    const DvzFramePlanNode* gbuffer_node = dvz_frame_plan_node_get(plan, 0);
    const DvzFramePlanNode* opaque_node = dvz_frame_plan_node_get(plan, 1);
    const DvzFramePlanNode* upload_node = dvz_frame_plan_node_get(plan, 2);
    const DvzFramePlanNode* ssao_node = dvz_frame_plan_node_get(plan, 3);
    const DvzFramePlanNode* composite_node = dvz_frame_plan_node_get(plan, 4);
    ANN(gbuffer_node);
    ANN(opaque_node);
    ANN(upload_node);
    ANN(ssao_node);
    ANN(composite_node);
    AT(dvz_frame_plan_render_pass_role(gbuffer_node) == DVZ_FRAME_PLAN_RENDER_PASS_GBUFFER);
    AT(dvz_frame_plan_render_pass_role(opaque_node) == DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE);
    AT(strcmp(upload_node->u.upload.resource_id, "figure_0_p0.ssao.params") == 0);
    AT(dvz_frame_plan_render_pass_role(ssao_node) == DVZ_FRAME_PLAN_RENDER_PASS_SSAO);
    AT(dvz_frame_plan_render_pass_role(composite_node) ==
       DVZ_FRAME_PLAN_RENDER_PASS_SSAO_COMPOSITE);

    bool found_normal = false;
    bool found_depth = false;
    bool found_occlusion = false;
    for (uint32_t i = 0; i < dvz_frame_plan_graph_resource_count(plan); i++)
    {
        const DvzFrameGraphResource* resource = dvz_frame_plan_graph_resource_get(plan, i);
        ANN(resource);
        found_normal =
            found_normal || strcmp(resource->id, "figure_0_p0.gbuffer.normal") == 0;
        found_depth =
            found_depth || strcmp(resource->id, "figure_0_p0.gbuffer.depth") == 0;
        found_occlusion =
            found_occlusion ||
            (strcmp(resource->id, "figure_0_p0.ssao.occlusion") == 0 &&
             resource->format == VK_FORMAT_R8_UNORM);
    }
    AT(found_normal);
    AT(found_depth);
    AT(found_occlusion);

    const DvzFrameGraphPass* gbuffer_pass = dvz_frame_plan_graph_pass_get(plan, 0);
    const DvzFrameGraphPass* opaque_pass = dvz_frame_plan_graph_pass_get(plan, 1);
    const DvzFrameGraphPass* ssao_pass = dvz_frame_plan_graph_pass_get(plan, 2);
    const DvzFrameGraphPass* composite_pass = dvz_frame_plan_graph_pass_get(plan, 3);
    ANN(gbuffer_pass);
    ANN(opaque_pass);
    ANN(ssao_pass);
    ANN(composite_pass);
    AT(strcmp(gbuffer_pass->work_label, "gbuffer") == 0);
    AT(strcmp(opaque_pass->work_label, "opaque") == 0);
    AT(strcmp(ssao_pass->work_label, "ssao") == 0);
    AT(strcmp(composite_pass->work_label, "ssao_composite") == 0);
    AT(ssao_pass->read_count == 2);
    AT(strcmp(ssao_pass->reads[0].resource_id, "figure_0_p0.gbuffer.normal") == 0);
    AT(strcmp(ssao_pass->reads[1].resource_id, "figure_0_p0.gbuffer.depth") == 0);
    AT(strcmp(ssao_pass->color_attachments[0].resource_id, "figure_0_p0.ssao.occlusion") == 0);
    AT(composite_pass->read_count == 1);
    AT(strcmp(composite_pass->reads[0].resource_id, "figure_0_p0.ssao.occlusion") == 0);
    AT(strcmp(composite_pass->color_attachments[0].resource_id, "rt") == 0);
    AT(composite_pass->color_attachments[0].load_op == DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD);

    dvz_frame_plan_destroy(plan);

    AT(_scene_technique_state_set_ssao(
        &panel->techniques,
        &(DvzSceneSsaoDesc){.radius = 1.25f, .strength = 2.0f, .bias = 0.05f,
                            .sample_count = 32, .blur_enabled = true}));
    plan = dvz_frame_plan("figure.ssao.blur", 0);
    ANN(plan);
    _scene_emit_panel_render(figure, 0, plan, "figure_0");
    AT(dvz_frame_plan_node_count(plan) == 6);
    AT(dvz_frame_plan_graph_pass_count(plan) == 5);
    const DvzFramePlanNode* blur_node = dvz_frame_plan_node_get(plan, 4);
    composite_node = dvz_frame_plan_node_get(plan, 5);
    ANN(blur_node);
    ANN(composite_node);
    AT(dvz_frame_plan_render_pass_role(blur_node) == DVZ_FRAME_PLAN_RENDER_PASS_SSAO_BLUR);
    AT(dvz_frame_plan_render_pass_role(composite_node) ==
       DVZ_FRAME_PLAN_RENDER_PASS_SSAO_COMPOSITE);
    const DvzFrameGraphPass* blur_pass = dvz_frame_plan_graph_pass_get(plan, 3);
    composite_pass = dvz_frame_plan_graph_pass_get(plan, 4);
    ANN(blur_pass);
    ANN(composite_pass);
    AT(strcmp(blur_pass->work_label, "ssao_blur") == 0);
    AT(blur_pass->read_count == 3);
    AT(strcmp(blur_pass->reads[0].resource_id, "figure_0_p0.ssao.occlusion") == 0);
    AT(strcmp(blur_pass->reads[1].resource_id, "figure_0_p0.gbuffer.normal") == 0);
    AT(strcmp(blur_pass->reads[2].resource_id, "figure_0_p0.gbuffer.depth") == 0);
    AT(strcmp(blur_pass->color_attachments[0].resource_id, "figure_0_p0.ssao.blur") == 0);
    AT(strcmp(composite_pass->reads[0].resource_id, "figure_0_p0.ssao.blur") == 0);

    dvz_frame_plan_destroy(plan);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Verify opt-in SSAO lowers its graph resources and fullscreen passes to DRP2.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_ssao_runtime_lowering(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    AT(dvz_panel_set_msaa(
        panel, &(DvzMsaaDesc){.enabled = true, .sample_count = 4, .alpha_to_coverage = true}));

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
    AT(_scene_technique_state_set_ssao(
        &panel->techniques,
        &(DvzSceneSsaoDesc){.radius = 1.25f, .strength = 2.0f, .bias = 0.05f,
                            .sample_count = 16}));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.target_width = 64;
    cfg.target_height = 64;
    dvz_capability_snapshot_default(&caps);
    caps.supports_color_blending = true;
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);
    DvzDrp2ValidationResult validation = dvz_drp2_validate_stream(stream);
    AT(validation.ok);

    bool found_occlusion_texture = false;
    bool found_params_upload = false;
    bool found_ssao_pipeline = false;
    bool found_composite_pipeline = false;
    bool found_ssao_bind_group = false;
    bool found_composite_bind_group = false;
    bool found_msaa_color_texture = false;
    bool found_msaa_render_pipeline = false;
    bool found_single_sample_gbuffer = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        ANN(cmd);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE)
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.create_texture.id);
            found_occlusion_texture =
                found_occlusion_texture ||
                (label != NULL && strcmp(label, "fig0_p0.ssao.occlusion") == 0 &&
                 cmd->u.create_texture.format == VK_FORMAT_R8_UNORM &&
                 (cmd->u.create_texture.usage & DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT) != 0 &&
                 (cmd->u.create_texture.usage & DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING) != 0);
            found_msaa_color_texture =
                found_msaa_color_texture ||
                (label != NULL && strcmp(label, "fig0_p0.msaa.color") == 0 &&
                 cmd->u.create_texture.sample_count == 4);
            found_single_sample_gbuffer =
                found_single_sample_gbuffer ||
                (label != NULL && strcmp(label, "fig0_p0.gbuffer.normal") == 0 &&
                 cmd->u.create_texture.sample_count == 1);
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_WRITE_BUFFER)
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.write_buffer.buffer_id);
            found_params_upload =
                found_params_upload ||
                (label != NULL && strcmp(label, "fig0_p0.ssao.params") == 0 &&
                 cmd->u.write_buffer.size == sizeof(DvzSceneSsaoUniform));
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.create_render_pipeline.id);
            found_ssao_pipeline =
                found_ssao_pipeline ||
                (label != NULL && strstr(label, "_pipe_ssao") != NULL &&
                 strstr(label, "_pipe_ssao_comp") == NULL &&
                 cmd->u.create_render_pipeline.color_targets[0].format == VK_FORMAT_R8_UNORM);
            found_composite_pipeline =
                found_composite_pipeline ||
                (label != NULL && strstr(label, "_pipe_ssao_comp") != NULL &&
                 cmd->u.create_render_pipeline.color_targets[0].blend_enabled);
            found_msaa_render_pipeline =
                found_msaa_render_pipeline || cmd->u.create_render_pipeline.sample_count == 4;
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_CREATE_BIND_GROUP)
        {
            found_ssao_bind_group =
                found_ssao_bind_group || cmd->u.create_bind_group.entry_count == 4;
            found_composite_bind_group =
                found_composite_bind_group || cmd->u.create_bind_group.entry_count == 3;
        }
    }
    AT(found_occlusion_texture);
    AT(found_params_upload);
    AT(found_ssao_pipeline);
    AT(found_composite_pipeline);
    AT(found_ssao_bind_group);
    AT(found_composite_bind_group);
    AT(found_msaa_color_texture);
    AT(found_msaa_render_pipeline);
    AT(found_single_sample_gbuffer);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Execute the scene SSAO DRP2 path through the vklite runtime when a GPU is available.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_ssao_glsl_executes(TstSuite* suite, TstItem* item)
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
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    DvzVisual* mesh = dvz_mesh(scene, 0);
    AT(mesh != NULL);

    float positions[4][3] = {
        {-0.75f, -0.75f, 0.20f},
        {+0.75f, -0.75f, 0.20f},
        {-0.75f, +0.75f, 0.55f},
        {+0.75f, +0.75f, 0.55f},
    };
    float normals[4][3] = {
        {0.0f, -0.35f, 0.94f},
        {0.0f, -0.35f, 0.94f},
        {0.0f, +0.35f, 0.94f},
        {0.0f, +0.35f, 0.94f},
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
    AT(_scene_technique_state_set_ssao(
        &panel->techniques,
        &(DvzSceneSsaoDesc){.radius = 1.0f, .strength = 2.5f, .bias = 0.02f,
                            .sample_count = 16}));

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.supports_color_blending = true;
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    emit_cfg.target_width = 64;
    emit_cfg.target_height = 64;

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


/**
 * Execute SSAO with sphere impostors feeding the G-buffer through the vklite runtime.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_sphere_ssao_glsl_executes(TstSuite* suite, TstItem* item)
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
    DvzFigure* figure = dvz_figure(scene, 96, 96, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    DvzVisual* sphere = dvz_sphere(scene, DVZ_SPHERE_FLAGS_LIGHTING);
    AT(sphere != NULL);
    AT(dvz_sphere_mode(sphere, DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR) == 0);

    float positions[4][3] = {
        {-0.30f, -0.25f, 0.15f},
        {+0.15f, -0.25f, 0.25f},
        {-0.08f, +0.12f, 0.35f},
        {+0.38f, +0.08f, 0.18f},
    };
    DvzColor colors[4] = {
        {210, 75, 75, 255},
        {75, 180, 120, 255},
        {75, 120, 220, 255},
        {220, 190, 75, 255},
    };
    float sizes[4] = {0.28f, 0.28f, 0.26f, 0.24f};

    AT(dvz_visual_set_data(sphere, "position", &positions[0][0], 4) == 0);
    AT(dvz_visual_set_data(sphere, "color", colors, 4) == 0);
    AT(dvz_visual_set_data(sphere, "radius", sizes, 4) == 0);
    AT(dvz_panel_add_visual(panel, sphere, NULL) == 0);
    AT(_scene_technique_state_set_ssao(
        &panel->techniques,
        &(DvzSceneSsaoDesc){.radius = 1.0f, .strength = 2.5f, .bias = 0.02f,
                            .sample_count = 16}));

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.supports_color_blending = true;
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    emit_cfg.target_width = 96;
    emit_cfg.target_height = 96;

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



/**
 * Verify SSAO opt-in is a no-op when no opaque normal-producing visual is present.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_ssao_ignores_ineligible_visuals(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    DvzVisual* primitive = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    AT(primitive != NULL);
    float positions[3][3] = {
        {-0.5f, -0.5f, 0.0f},
        {+0.5f, -0.5f, 0.0f},
        {0.0f, +0.5f, 0.0f},
    };
    DvzColor colors[3] = {
        {220, 80, 80, 255},
        {220, 80, 80, 255},
        {220, 80, 80, 255},
    };
    AT(dvz_visual_set_data(primitive, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(primitive, "color", colors, 3) == 0);
    AT(dvz_panel_add_visual(panel, primitive, NULL) == 0);
    AT(_scene_technique_state_set_ssao(
        &panel->techniques,
        &(DvzSceneSsaoDesc){.radius = 1.25f, .strength = 2.0f, .bias = 0.05f,
                            .sample_count = 32}));

    DvzFramePlan* plan = dvz_frame_plan("figure.ssao.ineligible", 0);
    ANN(plan);
    _scene_emit_panel_render(figure, 0, plan, "figure_0");
    AT(dvz_frame_plan_node_count(plan) == 1);
    AT(dvz_frame_plan_graph_resource_count(plan) == 0);
    AT(dvz_frame_plan_graph_pass_count(plan) == 0);

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
    AT(dvz_frame_plan_node_count(plan) == 2);
    const DvzFramePlanNode* opaque_node = dvz_frame_plan_node_get(plan, 0);
    const DvzFramePlanNode* transparent_node = dvz_frame_plan_node_get(plan, 1);
    ANN(opaque_node);
    ANN(transparent_node);
    AT(dvz_frame_plan_render_pass_role(opaque_node) == DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE);
    AT(dvz_frame_plan_render_pass_role(transparent_node) ==
       DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND);
    AT(opaque_node->u.render.visual_count == 1);
    AT(transparent_node->u.render.visual_count == 1);
    AT(transparent_node->u.render.visual_metadata[0].alpha_mode == DVZ_ALPHA_BLENDED);

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
    bool has_standard_blend_depth_test_pipeline = false;
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
            has_standard_blend_depth_test_pipeline =
                has_standard_blend_depth_test_pipeline ||
                (command->u.create_render_pipeline.color_targets[0].blend_enabled &&
                 command->u.create_render_pipeline.has_depth_attachment &&
                 !command->u.create_render_pipeline.depth_write_enabled &&
                 command->u.create_render_pipeline.depth_compare_op ==
                     VK_COMPARE_OP_LESS_OR_EQUAL);
        }
        else if (command->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS)
            begin_pass_count++;
    }
    AT(has_standard_blend_pipeline);
    AT(has_standard_blend_depth_test_pipeline);
    AT(begin_pass_count == 2);

    dvz_drp2_stream_destroy(stream);
    dvz_frame_plan_destroy(plan);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Verify source-over blended geometry is ordered with blended volume visuals by z layer.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_blended_mesh_orders_after_volume_slice(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 2,
                   .height = 2,
                   .depth = 2,
               });
    ANN(field);
    const uint8_t voxels[8] = {255, 255, 255, 255, 255, 255, 255, 255};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = voxels, .bytes_per_row = 2, .rows_per_image = 2}));

    DvzVisual* volume = dvz_volume(scene, 0);
    DvzVisual* slice = dvz_volume(scene, 0);
    DvzVisual* mesh = dvz_mesh(scene, 0);
    AT(volume != NULL);
    AT(slice != NULL);
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

    AT(dvz_visual_set_field(volume, "field", field));
    AT(dvz_visual_set_field(slice, "field", field));
    AT(dvz_volume_set_render_mode(volume, DVZ_VOLUME_RENDER_MIP) == 0);
    AT(dvz_volume_set_render_mode(slice, DVZ_VOLUME_RENDER_SLICE) == 0);
    AT(dvz_visual_set_alpha_mode(volume, DVZ_ALPHA_BLENDED) == 0);
    AT(dvz_visual_set_alpha_mode(slice, DVZ_ALPHA_BLENDED) == 0);
    AT(dvz_visual_set_data(mesh, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(mesh, "normal", normals, 4) == 0);
    AT(dvz_visual_set_buffer(mesh, "index", index_buffer));
    AT(dvz_visual_set_alpha_mode(mesh, DVZ_ALPHA_BLENDED) == 0);

    AT(dvz_panel_add_visual(
           panel, volume, &(DvzVisualAttachDesc){.z_layer = 0}) == 0);
    AT(dvz_panel_add_visual(
           panel, slice, &(DvzVisualAttachDesc){.z_layer = 1}) == 0);
    AT(dvz_panel_add_visual(
           panel, mesh, &(DvzVisualAttachDesc){.z_layer = 2}) == 0);

    DvzFramePlan* plan = dvz_frame_plan("figure.alpha.volume_mesh", 0);
    ANN(plan);
    _scene_emit_panel_render(figure, 0, plan, "figure_0");

    const DvzFramePlanNode* transparent_node = NULL;
    for (uint32_t i = 0; i < dvz_frame_plan_node_count(plan); i++)
    {
        const DvzFramePlanNode* node = dvz_frame_plan_node_get(plan, i);
        ANN(node);
        if (dvz_frame_plan_render_pass_role(node) ==
            DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND)
        {
            transparent_node = node;
            break;
        }
    }
    ANN(transparent_node);
    AT(transparent_node->u.render.visual_count == 3);
    AT(transparent_node->u.render.has_pass_contract);
    AT(strlen(transparent_node->u.render.pass_contract_id) > 0);
    AT(transparent_node->u.render.visual_metadata[0].visual_index == 0);
    AT(transparent_node->u.render.visual_metadata[1].visual_index == 1);
    AT(transparent_node->u.render.visual_metadata[2].visual_index == 2);
    AT(transparent_node->u.render.visual_metadata[0].visual_type == DVZ_VISUAL_TYPE_VOLUME);
    AT(transparent_node->u.render.visual_metadata[1].visual_type == DVZ_VISUAL_TYPE_VOLUME);
    AT(transparent_node->u.render.visual_metadata[2].visual_type == DVZ_VISUAL_TYPE_MESH);
    AT(transparent_node->u.render.visual_metadata[0].has_draw_contract);
    AT(transparent_node->u.render.visual_metadata[1].has_draw_contract);
    AT(transparent_node->u.render.visual_metadata[2].has_draw_contract);
    AT(strlen(transparent_node->u.render.visual_metadata[0].draw_contract_id) > 0);
    AT(
        transparent_node->u.render.visual_metadata[0].draw_depth_policy ==
        DVZ_SCENE_DEPTH_POLICY_SAMPLE);
    AT(
        transparent_node->u.render.visual_metadata[0].draw_blend_policy ==
        DVZ_SCENE_BLEND_POLICY_SOURCE_OVER);
    AT(
        transparent_node->u.render.visual_metadata[0].draw_bind_group_layout_mask &
        DVZ_SCENE_BIND_GROUP_REQUIREMENT_VOLUME);
    AT(
        transparent_node->u.render.visual_metadata[2].draw_depth_policy ==
        DVZ_SCENE_DEPTH_POLICY_TEST);

    const DvzFrameGraphPass* blend_pass = NULL;
    for (uint32_t i = 0; i < dvz_frame_plan_graph_pass_count(plan); i++)
    {
        const DvzFrameGraphPass* pass = dvz_frame_plan_graph_pass_get(plan, i);
        ANN(pass);
        if (strcmp(pass->work_label, "transparent_blend") == 0)
            blend_pass = pass;
    }
    ANN(blend_pass);
    AT(blend_pass->has_depth_attachment);
    AT(strcmp(blend_pass->depth_attachment.resource_id, "figure_0_p0.depth") == 0);
    AT(blend_pass->depth_attachment.load_op == DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD);
    AT(blend_pass->depth_attachment.access == DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ);

    DvzDiagnosticReport graph_report;
    dvz_diagnostic_report_init(&graph_report);
    AT(dvz_frame_plan_graph_validate(plan, &graph_report));
    dvz_diagnostic_report_init(&graph_report);
    AT(_scene_frame_plan_contracts_validate(figure, plan, &graph_report));
    AT(dvz_diagnostic_report_count(&graph_report) == 0);

    DvzScenePassContract contract = {0};
    AT(_scene_pass_contract_from_render(plan, panel, transparent_node, blend_pass, &contract));
    AT(contract.source_over_blend);
    AT(contract.draw_count == 3);
    AT(contract.color_attachment_count == 1);
    AT(contract.has_depth_attachment);
    AT(contract.needs_common_set);
    AT(contract.needs_volume_set);
    AT(contract.draws[0].samples_depth);
    AT(contract.draws[2].depth_test);
    AT(!contract.draws[2].depth_write);
    dvz_diagnostic_report_init(&graph_report);
    AT(_scene_pass_contract_validate(&contract, &graph_report));
    AT(dvz_diagnostic_report_count(&graph_report) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
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
    dvz_drp2_stream_destroy(stream);

    dvz_frame_plan_destroy(plan);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Verify the volume + slice + source-over mesh occlusion fixture emits consistent contracts.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_blended_mesh_occlusion_contracts(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 2,
                   .height = 2,
                   .depth = 2,
               });
    ANN(field);
    const uint8_t voxels[8] = {255, 255, 255, 255, 255, 255, 255, 255};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = voxels, .bytes_per_row = 2, .rows_per_image = 2}));

    DvzVisual* volume = dvz_volume(scene, 0);
    DvzVisual* slice = dvz_volume(scene, 0);
    DvzVisual* mesh = dvz_mesh(scene, 0);
    AT(volume != NULL);
    AT(slice != NULL);
    AT(mesh != NULL);

    float positions[4][3] = {
        {-0.5f, -0.5f, 0.1f},
        {+0.5f, -0.5f, 0.1f},
        {-0.5f, +0.5f, 0.1f},
        {+0.5f, +0.5f, 0.1f},
    };
    float normals[4][3] = {
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
    };
    DvzColor colors[4] = {
        {255, 0, 0, 128},
        {0, 255, 0, 128},
        {0, 0, 255, 128},
        {255, 255, 0, 128},
    };
    DvzIndex indices[6] = {0, 1, 2, 2, 1, 3};
    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)));

    AT(dvz_visual_set_field(volume, "field", field));
    AT(dvz_visual_set_field(slice, "field", field));
    AT(dvz_volume_set_render_mode(volume, DVZ_VOLUME_RENDER_COMPOSITE) == 0);
    AT(dvz_volume_set_step_count(volume, 16) == 0);
    AT(dvz_volume_set_render_mode(slice, DVZ_VOLUME_RENDER_SLICE) == 0);
    AT(dvz_visual_set_alpha_mode(volume, DVZ_ALPHA_BLENDED) == 0);
    AT(dvz_visual_set_alpha_mode(slice, DVZ_ALPHA_BLENDED) == 0);
    AT(dvz_visual_set_volume_occluded(slice, true) == 0);
    AT(dvz_visual_set_scene_occluded(slice, true) == 0);
    AT(dvz_visual_set_data(mesh, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(mesh, "normal", normals, 4) == 0);
    AT(dvz_visual_set_data(mesh, "color", colors, 4) == 0);
    AT(dvz_visual_set_buffer(mesh, "index", index_buffer));
    AT(dvz_visual_set_alpha_mode(mesh, DVZ_ALPHA_BLENDED) == 0);
    AT(dvz_visual_set_depth_test(mesh, true) == 0);
    AT(dvz_visual_set_scene_occluder(mesh, true) == 0);

    AT(dvz_panel_add_visual(
           panel, volume, &(DvzVisualAttachDesc){.z_layer = 0}) == 0);
    AT(dvz_panel_add_visual(
           panel, slice, &(DvzVisualAttachDesc){.z_layer = 1}) == 0);
    AT(dvz_panel_add_visual(
           panel, mesh, &(DvzVisualAttachDesc){.z_layer = 2}) == 0);
    AT(dvz_panel_set_volume_occluder(
           panel, volume,
           &(DvzVolumeOcclusionDesc){
               .enabled = true,
               .alpha_threshold = 0.01f,
               .fade_distance = 0.04f,
               .occluded_alpha = 0.2f,
           }) == 0);
    AT(dvz_panel_set_scene_occlusion(
           panel,
           &(DvzSceneOcclusionDesc){
               .enabled = true,
               .depth_bias = 0.0005f,
               .soft_edge = 0.01f,
               .hidden_alpha = 0.2f,
           }) == 0);

    DvzFramePlan* plan = dvz_frame_plan("figure.alpha.volume_mesh_occlusion", 0);
    ANN(plan);
    _scene_emit_panel_render(figure, 0, plan, "figure_0");

    const DvzFramePlanNode* blend_node = NULL;
    for (uint32_t i = 0; i < dvz_frame_plan_node_count(plan); i++)
    {
        const DvzFramePlanNode* node = dvz_frame_plan_node_get(plan, i);
        ANN(node);
        if (dvz_frame_plan_render_pass_role(node) == DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND)
            blend_node = node;
    }
    ANN(blend_node);
    AT(blend_node->u.render.visual_count == 3);
    AT(blend_node->u.render.visual_metadata[1].has_volume_occlusion);
    AT(blend_node->u.render.visual_metadata[1].has_scene_occlusion);

    const DvzFrameGraphPass* volume_pass = NULL;
    const DvzFrameGraphPass* scene_pass = NULL;
    const DvzFrameGraphPass* blend_pass = NULL;
    for (uint32_t i = 0; i < dvz_frame_plan_graph_pass_count(plan); i++)
    {
        const DvzFrameGraphPass* pass = dvz_frame_plan_graph_pass_get(plan, i);
        ANN(pass);
        if (strcmp(pass->work_label, "volume_occlusion") == 0)
            volume_pass = pass;
        else if (strcmp(pass->work_label, "scene_occlusion") == 0)
            scene_pass = pass;
        else if (strcmp(pass->work_label, "transparent_blend") == 0)
            blend_pass = pass;
    }
    ANN(volume_pass);
    ANN(scene_pass);
    ANN(blend_pass);
    AT(blend_pass->has_depth_attachment);

    bool reads_volume_occlusion = false;
    bool reads_scene_occlusion = false;
    for (uint32_t i = 0; i < blend_pass->read_count; i++)
    {
        reads_volume_occlusion =
            reads_volume_occlusion ||
            strcmp(blend_pass->reads[i].resource_id,
                   "figure_0_p0.volume_occlusion.depth") == 0;
        reads_scene_occlusion =
            reads_scene_occlusion ||
            strcmp(blend_pass->reads[i].resource_id,
                   "figure_0_p0.scene_occlusion.depth") == 0;
    }
    AT(reads_volume_occlusion);
    AT(reads_scene_occlusion);

    DvzDiagnosticReport graph_report;
    dvz_diagnostic_report_init(&graph_report);
    AT(dvz_frame_plan_graph_validate(plan, &graph_report));

    DvzScenePassContract contract = {0};
    AT(_scene_pass_contract_from_render(plan, panel, blend_node, blend_pass, &contract));
    AT(contract.source_over_blend);
    AT(contract.draw_count == 3);
    AT(contract.draws[1].samples_volume_occlusion);
    AT(contract.draws[1].samples_scene_occlusion);
    AT(contract.draws[1].needs_volume_set);
    AT(contract.draws[1].needs_scene_occlusion_set);
    AT(contract.draws[2].depth_test);
    AT(!contract.draws[2].depth_write);
    dvz_diagnostic_report_init(&graph_report);
    AT(_scene_pass_contract_validate(&contract, &graph_report));
    AT(dvz_diagnostic_report_count(&graph_report) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.max_color_attachments = 3;
    caps.render_target_format_rgba16float = true;
    caps.render_target_format_r16float = true;
    caps.supports_color_blending = true;
    caps.supports_render_target_sampling = true;
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.target_width = 64;
    cfg.target_height = 64;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);
    DvzDrp2ValidationResult validation = dvz_drp2_validate_stream(stream);
    AT(validation.ok);

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
    dvz_diagnostic_report_init(&report);
    AT(_scene_frame_plan_contracts_validate(figure, plan, &report));
    AT(dvz_diagnostic_report_count(&report) == 0);

    DvzScenePassContract opaque_contract = {0};
    AT(_scene_pass_contract_from_render(plan, panel, opaque_node, opaque_pass, &opaque_contract));
    AT(opaque_contract.draw_count == 1);
    AT(opaque_contract.draws[0].depth_write);
    AT(opaque_contract.needs_common_set);
    dvz_diagnostic_report_init(&report);
    AT(_scene_pass_contract_validate(&opaque_contract, &report));
    AT(dvz_diagnostic_report_count(&report) == 0);

    DvzScenePassContract accum_contract = {0};
    AT(_scene_pass_contract_from_render(plan, panel, accum_node, accum_pass, &accum_contract));
    AT(accum_contract.wboit_accumulation);
    AT(accum_contract.draw_count == 1);
    AT(accum_contract.draws[0].alpha_mode == DVZ_ALPHA_WBOIT);
    AT(accum_contract.draws[0].depth_test);
    AT(!accum_contract.draws[0].depth_write);
    AT(accum_contract.color_attachment_count == 2);
    AT(accum_contract.has_depth_attachment);
    AT(accum_contract.attachments[0].format == VK_FORMAT_R16G16B16A16_SFLOAT);
    AT(accum_contract.attachments[1].format == VK_FORMAT_R16_SFLOAT);
    dvz_diagnostic_report_init(&report);
    AT(_scene_pass_contract_validate(&accum_contract, &report));
    AT(dvz_diagnostic_report_count(&report) == 0);

    DvzScenePassContract resolve_contract = {0};
    AT(_scene_pass_contract_from_render(plan, panel, resolve_node, resolve_pass, &resolve_contract));
    AT(resolve_contract.fullscreen_resolve);
    AT(resolve_contract.draw_count == 0);
    AT(resolve_contract.attachment_count == 3);
    AT(resolve_contract.sampled_read_count == 2);
    AT(resolve_contract.needs_wboit_resolve_layout);
    AT(resolve_contract.sampled_texture_binding_count == 2);
    dvz_diagnostic_report_init(&report);
    AT(_scene_pass_contract_validate(&resolve_contract, &report));
    AT(dvz_diagnostic_report_count(&report) == 0);

    dvz_frame_plan_destroy(plan);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Verify WBOIT transparent-only depth-tested draws still declare pass depth.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_visual_alpha_mode_wboit_transparent_only_depth(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    DvzVisual* transparent = dvz_point(scene, 0);
    AT(transparent != NULL);

    float positions[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f,
    };
    DvzColor colors[3] = {{255, 0, 0, 128}, {0, 255, 0, 128}, {0, 0, 255, 128}};
    float sizes[3] = {10.0f, 12.0f, 14.0f};

    AT(dvz_visual_set_data(transparent, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(transparent, "color", colors, 3) == 0);
    AT(dvz_visual_set_data(transparent, "size", sizes, 3) == 0);
    AT(dvz_visual_set_alpha_mode(transparent, DVZ_ALPHA_WBOIT) == 0);
    AT(dvz_panel_add_visual(panel, transparent, NULL) == 0);

    DvzFramePlan* plan = dvz_frame_plan("figure.alpha.wboit.transparent_only", 0);
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
    AT(opaque_node->u.render.visual_count == 0);
    AT(accum_node->u.render.visual_count == 1);

    const DvzFrameGraphPass* opaque_pass = dvz_frame_plan_graph_pass_get(plan, 0);
    const DvzFrameGraphPass* accum_pass = dvz_frame_plan_graph_pass_get(plan, 1);
    ANN(opaque_pass);
    ANN(accum_pass);
    AT(strcmp(opaque_pass->work_label, "opaque") == 0);
    AT(strcmp(accum_pass->work_label, "wboit_accum") == 0);
    AT(!opaque_pass->has_depth_attachment);
    AT(accum_pass->has_depth_attachment);
    AT(strcmp(accum_pass->depth_attachment.resource_id, "figure_0_p0.depth") == 0);
    AT(accum_pass->depth_attachment.load_op == DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR);
    AT(accum_pass->depth_attachment.access == DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE);

    DvzDiagnosticReport graph_report;
    dvz_diagnostic_report_init(&graph_report);
    AT(dvz_frame_plan_graph_validate(plan, &graph_report));
    dvz_diagnostic_report_init(&graph_report);
    AT(_scene_frame_plan_contracts_validate(figure, plan, &graph_report));
    AT(dvz_diagnostic_report_count(&graph_report) == 0);

    DvzScenePassContract accum_contract = {0};
    AT(_scene_pass_contract_from_render(plan, panel, accum_node, accum_pass, &accum_contract));
    AT(accum_contract.wboit_accumulation);
    AT(accum_contract.draw_count == 1);
    AT(accum_contract.draws[0].depth_test);
    AT(!accum_contract.draws[0].depth_write);
    AT(accum_contract.color_attachment_count == 2);
    AT(accum_contract.has_depth_attachment);
    dvz_diagnostic_report_init(&graph_report);
    AT(_scene_pass_contract_validate(&accum_contract, &graph_report));
    AT(dvz_diagnostic_report_count(&graph_report) == 0);

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

    dvz_drp2_stream_destroy(stream);
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
    dvz_diagnostic_report_init(&report);
    AT(_scene_frame_plan_contracts_validate(figure, plan, &report));
    AT(dvz_diagnostic_report_count(&report) == 0);

    DvzScenePassContract init_contract = {0};
    AT(_scene_pass_contract_from_render(plan, panel, init_node, init_pass, &init_contract));
    AT(init_contract.depth_peel);
    AT(init_contract.draw_count == 1);
    AT(init_contract.draws[0].alpha_mode == DVZ_ALPHA_DEPTH_PEEL);
    AT(init_contract.draws[0].depth_test);
    AT(!init_contract.draws[0].depth_write);
    AT(init_contract.color_attachment_count == 3);
    AT(init_contract.has_depth_attachment);
    AT(init_contract.attachments[0].format == VK_FORMAT_R16G16B16A16_SFLOAT);
    dvz_diagnostic_report_init(&report);
    AT(_scene_pass_contract_validate(&init_contract, &report));
    AT(dvz_diagnostic_report_count(&report) == 0);

    DvzScenePassContract iter_contract = {0};
    AT(_scene_pass_contract_from_render(plan, panel, iter_node, iter_pass, &iter_contract));
    AT(iter_contract.depth_peel);
    AT(iter_contract.draw_count == 1);
    AT(iter_contract.draws[0].alpha_mode == DVZ_ALPHA_DEPTH_PEEL);
    AT(iter_contract.draws[0].depth_test);
    AT(!iter_contract.draws[0].depth_write);
    AT(iter_contract.color_attachment_count == 3);
    AT(iter_contract.has_depth_attachment);
    AT(iter_contract.attachments[0].format == VK_FORMAT_R16G16B16A16_SFLOAT);
    dvz_diagnostic_report_init(&report);
    AT(_scene_pass_contract_validate(&iter_contract, &report));
    AT(dvz_diagnostic_report_count(&report) == 0);

    DvzScenePassContract composite_contract = {0};
    AT(_scene_pass_contract_from_render(
        plan, panel, composite_node, composite_pass, &composite_contract));
    AT(composite_contract.fullscreen_resolve);
    AT(composite_contract.draw_count == 0);
    AT(composite_contract.attachment_count == 4);
    AT(composite_contract.sampled_read_count == 3);
    AT(composite_contract.needs_depth_peel_sampled_layout);
    AT(composite_contract.sampled_texture_binding_count == 3);
    dvz_diagnostic_report_init(&report);
    AT(_scene_pass_contract_validate(&composite_contract, &report));
    AT(dvz_diagnostic_report_count(&report) == 0);

    dvz_frame_plan_destroy(plan);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Verify panels mixing WBOIT and depth peeling are rejected with a diagnostic.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_visual_alpha_mode_mixed_oit_rejected(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    DvzVisual* wboit = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    DvzVisual* peel = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    AT(wboit != NULL);
    AT(peel != NULL);

    float positions[3][3] = {
        {-0.5f, -0.5f, 0.0f},
        {0.5f, -0.5f, 0.0f},
        {0.0f, 0.5f, 0.0f},
    };
    DvzColor colors[3] = {{255, 0, 0, 128}, {0, 255, 0, 128}, {0, 0, 255, 128}};
    AT(dvz_visual_set_data(wboit, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(wboit, "color", colors, 3) == 0);
    AT(dvz_visual_set_data(peel, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(peel, "color", colors, 3) == 0);
    AT(dvz_visual_set_alpha_mode(wboit, DVZ_ALPHA_WBOIT) == 0);
    AT(dvz_visual_set_alpha_mode(peel, DVZ_ALPHA_DEPTH_PEEL) == 0);
    AT(dvz_panel_add_visual(panel, wboit, NULL) == 0);
    AT(dvz_panel_add_visual(panel, peel, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.max_color_attachments = 3;
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
    AT(stream == NULL);
    AT(dvz_diagnostic_report_count(&report) == 1);
    const char* message = dvz_diagnostic_report_get(&report, 0);
    ANN(message);
    AT(strstr(message, "mixes WBOIT") != NULL);
    AT(strstr(message, "depth-peel") != NULL);

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
 * Verify scene DRP2 contract validation catches emitted pipeline policy drift.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_drp2_contract_checker_rejects_pipeline_drift(TstSuite* suite, TstItem* item)
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
    DvzColor colors[3] = {{255, 0, 0, 128}, {0, 255, 0, 128}, {0, 0, 255, 128}};
    AT(dvz_visual_set_data(opaque, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(opaque, "color", opaque_colors, 3) == 0);
    AT(dvz_visual_set_data(transparent, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(transparent, "color", colors, 3) == 0);
    AT(dvz_visual_set_alpha_mode(transparent, DVZ_ALPHA_WBOIT) == 0);
    AT(dvz_panel_add_visual(panel, opaque, NULL) == 0);
    AT(dvz_panel_add_visual(panel, transparent, NULL) == 0);

    DvzFramePlan* plan = dvz_frame_plan("figure.contract.drp2", 0);
    ANN(plan);
    _scene_emit_visual_uploads(figure, plan);
    AT(_scene_emit_panel_render(figure, 0, plan, "figure_0"));

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    AT(_scene_frame_plan_contracts_validate(figure, plan, &report));
    AT(dvz_diagnostic_report_count(&report) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.max_color_attachments = 2;
    caps.render_target_format_rgba16float = true;
    caps.render_target_format_r16float = true;
    caps.supports_render_target_sampling = true;
    caps.supports_color_blending = true;

    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.target_width = 64;
    cfg.target_height = 64;

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);
    DvzDrp2CommandStream* stream =
        dvz_frame_plan_emitter_emit_drp2(emitter, plan, &caps, &report, &cfg);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(_scene_frame_plan_drp2_contracts_validate(plan, stream, &report));

    DvzDrp2Command* wboit_pipeline = NULL;
    for (uint32_t i = 0; i < stream->count; i++)
    {
        DvzDrp2Command* command = &stream->commands[i];
        if (command->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE &&
            command->u.create_render_pipeline.color_target_count == 2 &&
            command->u.create_render_pipeline.color_targets[0].blend_enabled)
        {
            wboit_pipeline = command;
            break;
        }
    }
    ANN(wboit_pipeline);

    const DvzDrp2Command original_pipeline_command = *wboit_pipeline;

    wboit_pipeline->u.create_render_pipeline.color_targets[0].blend_enabled = false;
    dvz_diagnostic_report_init(&report);
    AT(!_scene_frame_plan_drp2_contracts_validate(plan, stream, &report));
    AT(dvz_diagnostic_report_count(&report) > 0);

    wboit_pipeline->u.create_render_pipeline = original_pipeline_command.u.create_render_pipeline;
    wboit_pipeline->u.create_render_pipeline.sample_count =
        original_pipeline_command.u.create_render_pipeline.sample_count == 1 ? 2 : 1;
    dvz_diagnostic_report_init(&report);
    AT(!_scene_frame_plan_drp2_contracts_validate(plan, stream, &report));
    AT(dvz_diagnostic_report_count(&report) > 0);

    wboit_pipeline->u.create_render_pipeline = original_pipeline_command.u.create_render_pipeline;
    wboit_pipeline->u.create_render_pipeline.bind_group_layout_count = 0;
    dvz_diagnostic_report_init(&report);
    AT(!_scene_frame_plan_drp2_contracts_validate(plan, stream, &report));
    AT(dvz_diagnostic_report_count(&report) > 0);

    wboit_pipeline->u.create_render_pipeline = original_pipeline_command.u.create_render_pipeline;

    DvzDrp2Command* resolve_bind_group = NULL;
    for (uint32_t i = 0; i < stream->count; i++)
    {
        DvzDrp2Command* command = &stream->commands[i];
        if (command->type == DVZ_DRP2_COMMAND_CREATE_BIND_GROUP &&
            command->u.create_bind_group.entry_count == 3 &&
            command->u.create_bind_group.entries[0].binding_type ==
                DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE &&
            command->u.create_bind_group.entries[1].binding_type ==
                DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE)
        {
            resolve_bind_group = command;
            break;
        }
    }
    ANN(resolve_bind_group);
    resolve_bind_group->u.create_bind_group.entries[0].resource_id =
        resolve_bind_group->u.create_bind_group.entries[1].resource_id;
    dvz_diagnostic_report_init(&report);
    AT(!_scene_frame_plan_drp2_contracts_validate(plan, stream, &report));
    AT(dvz_diagnostic_report_count(&report) > 0);

    dvz_drp2_stream_destroy(stream);
    dvz_frame_plan_emitter_destroy(emitter);
    dvz_frame_plan_destroy(plan);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify retained alpha-mode toggles refresh the semantic DRP2 runtime contract shape.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_alpha_mode_toggle_refreshes_drp2_contracts(TstSuite* suite, TstItem* item)
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
    float shifted[3][3] = {
        {-0.45f, -0.45f, -0.1f},
        {0.55f, -0.45f, -0.1f},
        {0.05f, 0.55f, -0.1f},
    };
    DvzColor opaque_colors[3] = {
        {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255}};
    DvzColor transparent_colors[3] = {
        {255, 0, 0, 128}, {0, 255, 0, 128}, {0, 0, 255, 128}};

    AT(dvz_visual_set_data(opaque, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(opaque, "color", opaque_colors, 3) == 0);
    AT(dvz_visual_set_data(transparent, "position", shifted, 3) == 0);
    AT(dvz_visual_set_data(transparent, "color", transparent_colors, 3) == 0);
    AT(dvz_visual_set_alpha_mode(transparent, DVZ_ALPHA_BLENDED) == 0);
    AT(dvz_panel_add_visual(panel, opaque, NULL) == 0);
    AT(dvz_panel_add_visual(panel, transparent, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.max_color_attachments = 2;
    caps.render_target_format_rgba16float = true;
    caps.render_target_format_r16float = true;
    caps.supports_render_target_sampling = true;
    caps.supports_color_blending = true;

    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.target_width = 64;
    cfg.target_height = 64;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2RuntimeConfig runtime_cfg = dvz_drp2_runtime_vklite_config(NULL, NULL);
    runtime_cfg.semantic_only = true;
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzDrp2CommandStream* source_over0 = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    ANN(source_over0);
    AT(dvz_diagnostic_report_count(&report) == 0);

    bool source_over0_has_wboit = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(source_over0); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(source_over0, i);
        ANN(command);
        source_over0_has_wboit =
            source_over0_has_wboit ||
            (command->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS &&
             command->u.begin_render_pass.color_attachment_count == 2);
    }
    AT(!source_over0_has_wboit);
    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, source_over0);
    AT(result.ok);
    dvz_drp2_stream_destroy(source_over0);

    AT(dvz_visual_set_alpha_mode(transparent, DVZ_ALPHA_WBOIT) == 0);
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* wboit = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    ANN(wboit);
    AT(dvz_diagnostic_report_count(&report) == 0);

    bool wboit_has_accum_pass = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(wboit); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(wboit, i);
        ANN(command);
        wboit_has_accum_pass =
            wboit_has_accum_pass ||
            (command->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS &&
             command->u.begin_render_pass.color_attachment_count == 2);
    }
    AT(wboit_has_accum_pass);
    result = dvz_drp2_runtime_execute(runtime, wboit);
    AT(result.ok);
    dvz_drp2_stream_destroy(wboit);

    AT(dvz_visual_set_alpha_mode(transparent, DVZ_ALPHA_BLENDED) == 0);
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* source_over1 = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    ANN(source_over1);
    AT(dvz_diagnostic_report_count(&report) == 0);

    bool source_over1_has_wboit = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(source_over1); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(source_over1, i);
        ANN(command);
        source_over1_has_wboit =
            source_over1_has_wboit ||
            (command->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS &&
             command->u.begin_render_pass.color_attachment_count == 2);
    }
    AT(!source_over1_has_wboit);
    result = dvz_drp2_runtime_execute(runtime, source_over1);
    AT(result.ok);
    dvz_drp2_stream_destroy(source_over1);
    dvz_drp2_runtime_destroy(runtime);
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
    TEST_SIMPLE(test_scene_sphere_emit_glsl_executes);
    TEST_SIMPLE(test_scene_sphere_mode);
    TEST_SIMPLE(test_scene_segment_caps);
    TEST_SIMPLE(test_scene_segment_emit_glsl);
    TEST_SIMPLE(test_scene_point_like_lowering_policy);
    TEST_SIMPLE(test_scene_point_emit_glsl_native_points);
    TEST_SIMPLE(test_scene_point_style_emits_glsl_and_wgsl);
    TEST_SIMPLE(test_scene_marker_api_and_emit_glsl);
    TEST_SIMPLE(test_scene_pixel_emit_glsl_native_square_points);
    TEST_SIMPLE(test_scene_point_emit_wgsl_instanced_quads);
    TEST_SIMPLE(test_scene_pixel_emit_wgsl_instanced_quads);
    TEST_SIMPLE(test_scene_primitive_triangle_list_glsl_executes);
    TEST_SIMPLE(test_scene_primitive_line_strip_glsl_executes);
    TEST_SIMPLE(test_scene_primitive_triangle_list_emit_wgsl);
    TEST_SIMPLE(test_scene_mesh_indexed_default_color_emits_draw_indexed);
    TEST_SIMPLE(test_scene_mesh_instance_transform_emits_instanced_draw);
    TEST_SIMPLE(test_scene_mesh_emits_depth_attachment);
    TEST_SIMPLE(test_scene_indexed_primitive_emits_draw_indexed);
    TEST_SIMPLE(test_scene_shared_index_buffer_emits_one_upload);
    TEST_SIMPLE(test_scene_mesh_glsl_executes);
    TEST_SIMPLE(test_scene_path_glsl_executes);
    TEST_SIMPLE(test_scene_path_line_width_emit_glsl);
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
    TEST_SIMPLE(test_scene_visual_depth_test);
    TEST_SIMPLE(test_scene_visual_scene_occlusion_flags);
    TEST_SIMPLE(test_scene_visual_scene_occlusion_frame_plan);
    TEST_SIMPLE(test_scene_visual_scene_occlusion_emits_drp2);
    TEST_SIMPLE(test_scene_volume_slice_uses_volume_occlusion);
    TEST_SIMPLE(test_scene_volume_slice_uses_generic_scene_occlusion);
    TEST_SIMPLE(test_scene_visual_internal_material_state);
    TEST_SIMPLE(test_scene_visual_material_setter);
    TEST_SIMPLE(test_scene_pixel_depth_cue_toggle_switches_pipeline);
    TEST_SIMPLE(test_scene_visual_pass_capabilities);
    TEST_SIMPLE(test_scene_draw_contract_resolver_matrix);
    TEST_SIMPLE(test_scene_role_work_label_mapping_complete);
    TEST_SIMPLE(test_scene_render_contract_validation_errors);
    TEST_SIMPLE(test_scene_frame_plan_missing_graph_pass_fails_contract);
    TEST_SIMPLE(test_scene_gbuffer_runtime_lowering);
    TEST_SIMPLE(test_scene_frame_plan_node_reallocation_safe);
    TEST_SIMPLE(test_scene_msaa_runtime_lowering);
    TEST_SIMPLE(test_scene_msaa_runtime_capability_lowering);
    TEST_SIMPLE(test_scene_edl_runtime_lowering);
    TEST_SIMPLE(test_scene_edl_depth_producer_capabilities);
    TEST_SIMPLE(test_scene_edl_ignores_ineligible_passes);
    TEST_SIMPLE(test_scene_ssao_graph_foundation);
    TEST_SIMPLE(test_scene_ssao_runtime_lowering);
    TEST_SIMPLE(test_scene_ssao_glsl_executes);
    TEST_SIMPLE(test_scene_sphere_ssao_glsl_executes);
    TEST_SIMPLE(test_scene_ssao_ignores_ineligible_visuals);
    TEST_SIMPLE(test_scene_visual_alpha_mode_standard_blend);
    TEST_SIMPLE(test_scene_visual_alpha_mode_splits_frame_plan_passes);
    TEST_SIMPLE(test_scene_visual_alpha_mode_wboit_transparent_only_depth);
    TEST_SIMPLE(test_scene_visual_alpha_mode_depth_peel_frame_plan);
    TEST_SIMPLE(test_scene_visual_alpha_mode_mixed_oit_rejected);
    TEST_SIMPLE(test_scene_visual_alpha_mode_emits_depth_peel_drp2);
    TEST_SIMPLE(test_scene_visual_alpha_mode_requires_wboit_capabilities);
    TEST_SIMPLE(test_scene_visual_alpha_mode_emits_wboit_drp2);
    TEST_SIMPLE(test_scene_drp2_contract_checker_rejects_pipeline_drift);
    TEST_SIMPLE(test_scene_alpha_mode_toggle_refreshes_drp2_contracts);
    TEST_SIMPLE(test_scene_visual_alpha_mode_wboit_glsl_executes);
    TEST_SIMPLE(test_scene_visual_alpha_mode_depth_peel_glsl_executes);
    TEST_SIMPLE(test_scene_blended_mesh_orders_after_volume_slice);
    TEST_SIMPLE(test_scene_blended_mesh_occlusion_contracts);
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
    TEST_SIMPLE(test_scene_image_multi_item_emit);
    TEST_SIMPLE(test_scene_glyph_emit_glsl);
    TEST_SIMPLE(test_scene_image_emit_wgsl);
    TEST_SIMPLE(test_scene_image_emit_uses_common_and_texture_sets);
    TEST_SIMPLE(test_scene_visual_common_binding_layout_order);
    TEST_SIMPLE(test_scene_empty_figure_emit_clear_only);
    TEST_SIMPLE(test_scene_point_emit_has_vertex_layout);
    TEST_SIMPLE(test_scene_point_visual_resizes_existing_attributes);
    TEST_SIMPLE(test_scene_indexed_primitive_shading_updates_runtime);
    TEST_SIMPLE(test_scene_point_large_count_executes);
    TEST_SIMPLE(test_scene_second_emit_no_uploads_when_not_dirty);
    TEST_SIMPLE(test_scene_hidden_visual_first_visible_later_uploads);
    TEST_SIMPLE(test_scene_hidden_indexed_mesh_first_visible_later_uploads);
    TEST_SIMPLE(test_scene_hidden_wboit_mesh_scene_occlusion_two_frames_glsl_executes);
    TEST_SIMPLE(test_scene_partial_update_uploads_only_range);
    TEST_SIMPLE(test_scene_repeated_partial_updates_across_frames);
    TEST_SIMPLE(test_scene_partial_update_merges_ranges_before_emit);
    TEST_SIMPLE(test_scene_multiple_panels_multiple_point_visuals_emit);
    TEST_SIMPLE(test_scene_render_pass_scope_excludes_resource_commands);

    return 0;
}
