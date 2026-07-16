/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene visual runtime contract tests                                                         */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "common.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

static const DvzDrp2Command*
_scene_test_first_draw_command(const DvzDrp2CommandStream* stream)
{
    ANN(stream);
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd != NULL && cmd->type == DVZ_DRP2_COMMAND_DRAW)
            return cmd;
    }
    return NULL;
}



int test_scene_point_emit_has_vertex_layout(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, &desc);
    AT(panel != NULL);
    DvzVisual* visual = dvz_point(scene, 0);
    AT(visual != NULL);

    vec3 positions[3] = {{-0.5f, -0.5f, 0.0f}, {0.5f, -0.5f, 0.0f}, {0.0f, 0.5f, 0.0f}};
    DvzColor colors[3] = {{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}};
    float sizes[3] = {10.0f, 10.0f, 10.0f};
    AT(dvz_visual_set_data(visual, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 3) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 3) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
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

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify point item ranges lower to native point-list draw offsets.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_point_item_range_emit_glsl(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);

    vec3 positions[5] = {
        {-0.8f, 0.0f, 0.0f}, {-0.4f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f},
        { 0.4f, 0.0f, 0.0f}, { 0.8f, 0.0f, 0.0f},
    };
    DvzColor colors[5] = {
        {255, 0, 0, 255}, {255, 128, 0, 255}, {255, 255, 0, 255},
        {0, 255, 0, 255}, {0, 128, 255, 255},
    };
    float sizes[5] = {6.0f, 7.0f, 8.0f, 9.0f, 10.0f};
    AT(dvz_visual_set_data(visual, "position", positions, 5) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 5) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 5) == 0);
    AT(dvz_visual_set_item_range(visual, 1, 3) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);

    const DvzDrp2Command* draw = _scene_test_first_draw_command(stream);
    ANN(draw);
    AT(draw->u.draw.first_vertex == 1);
    AT(draw->u.draw.vertex_count == 3);
    AT(draw->u.draw.first_instance == 0);
    AT(draw->u.draw.instance_count == 1);

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify point item ranges lower to instanced-quad draw offsets.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_point_item_range_emit_wgsl(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);

    vec3 positions[5] = {
        {-0.8f, 0.0f, 0.0f}, {-0.4f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f},
        { 0.4f, 0.0f, 0.0f}, { 0.8f, 0.0f, 0.0f},
    };
    DvzColor colors[5] = {
        {255, 0, 0, 255}, {255, 128, 0, 255}, {255, 255, 0, 255},
        {0, 255, 0, 255}, {0, 128, 255, 255},
    };
    float sizes[5] = {6.0f, 7.0f, 8.0f, 9.0f, 10.0f};
    AT(dvz_visual_set_data(visual, "position", positions, 5) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 5) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 5) == 0);
    AT(dvz_visual_set_item_range(visual, 2, 2) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_wgsl = true;
    caps.shader_format_glsl = false;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups = 4;
    caps.max_buffer_size = 256 * 1024 * 1024;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_WGSL;

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);

    const DvzDrp2Command* draw = _scene_test_first_draw_command(stream);
    ANN(draw);
    AT(draw->u.draw.first_vertex == 0);
    AT(draw->u.draw.vertex_count == 6);
    AT(draw->u.draw.first_instance == 2);
    AT(draw->u.draw.instance_count == 2);

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify empty and cleared point item ranges affect draw contribution only.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_point_item_range_empty_clear_no_reupload(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);

    vec3 positions[4] = {
        {-0.6f, 0.0f, 0.0f}, {-0.2f, 0.0f, 0.0f},
        { 0.2f, 0.0f, 0.0f}, { 0.6f, 0.0f, 0.0f},
    };
    DvzColor colors[4] = {
        {255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}, {255, 255, 255, 255},
    };
    float sizes[4] = {6.0f, 7.0f, 8.0f, 9.0f};
    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 4) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 4) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream0 = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
    ANN(stream0);
    AT(dvz_diagnostic_report_count(&report) == 0);
    _test_scene_stream_destroy(stream0);

    AT(dvz_visual_set_item_range(visual, 4, 0) == 0);
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
    ANN(stream1);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(_stream_visual_write_buffer_count(stream1) == 0);
    const DvzDrp2Command* draw1 = _scene_test_first_draw_command(stream1);
    ANN(draw1);
    AT(draw1->u.draw.first_vertex == 4);
    AT(draw1->u.draw.vertex_count == 0);
    _test_scene_stream_destroy(stream1);

    AT(dvz_visual_set_item_range(visual, 1, 2) == 0);
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream2 = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
    ANN(stream2);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(_stream_visual_write_buffer_count(stream2) == 0);
    const DvzDrp2Command* draw2 = _scene_test_first_draw_command(stream2);
    ANN(draw2);
    AT(draw2->u.draw.first_vertex == 1);
    AT(draw2->u.draw.vertex_count == 2);
    _test_scene_stream_destroy(stream2);

    AT(dvz_visual_clear_item_range(visual) == DVZ_OK);
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream3 = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
    ANN(stream3);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(_stream_visual_write_buffer_count(stream3) == 0);
    const DvzDrp2Command* draw3 = _scene_test_first_draw_command(stream3);
    ANN(draw3);
    AT(draw3->u.draw.first_vertex == 0);
    AT(draw3->u.draw.vertex_count == 4);
    _test_scene_stream_destroy(stream3);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_indexed_primitive_material_updates_runtime(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_GRAPH_REQUIRE_VKLITE(suite);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    ANN(visual);

    vec3 positions[4] = {
        {-0.8f, -0.8f, 0.0f}, {-0.8f, 0.8f, 0.0f},
        {0.8f, -0.8f, 0.0f},  {0.8f, 0.8f, 0.0f},
    };
    DvzColor colors[4] = {
        {255, 0, 0, 255}, {255, 0, 0, 255}, {255, 0, 0, 255}, {255, 0, 0, 255},
    };
    vec3 normals[4] = {
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
    };
    DvzIndex indices[6] = {0, 1, 2, 2, 1, 3};

    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)) == DVZ_OK);

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 4) == 0);
    AT(dvz_visual_set_data(visual, "normal", normals, 4) == 0);
    AT(dvz_visual_set_buffer(visual, "index", index_buffer) == DVZ_OK);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);
    AT(_scene_visuals_set_phong_material(
           visual, (float[3]){0.0f, 0.0f, 1.0f}, 0.0f, 0.0f, 0.25f, 32.0f) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
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
    DvzDrp2Runtime* runtime = NULL;
    if (ctx != NULL)
    {
        DvzDrp2RuntimeConfig runtime_cfg =
            dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
        runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
        ANN(runtime);
    }

    DvzDrp2CommandStream* stream0 = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
    ANN(stream0);
    AT(_stream_set_vertex_buffer_count(stream0) == 3);
    AT(_stream_write_buffer_range_count(stream0, 0, sizeof(DvzSceneMaterialParams)) == 1);
    if (runtime != NULL)
    {
        DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream0);
        AT(result.ok);
        AT(dvz_gpu_ctx_error_count(ctx) == 0);
    }
    _test_scene_stream_destroy(stream0);
    stream0 = NULL;

    AT(_scene_visuals_set_phong_material(
           visual, (float[3]){0.0f, 0.0f, 1.0f}, 1.0f, 0.0f, 0.25f, 32.0f) == 0);

    DvzDrp2CommandStream* stream1 = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
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

    _test_scene_stream_destroy(stream1);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_point_large_count_executes(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_GRAPH_REQUIRE_VKLITE(suite);

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
        log_warn("test_scene_point_large_count_executes skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    /* 1000 points — same as hello_scatter, exercises large buffer upload path. */
    const uint32_t N = 1000;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, &desc);
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
        colors[i] = dvz_color_rgba(255, (uint8_t)(i % 256), 0, 255);
        sizes[i] = 4.0f;
    }

    AT(dvz_visual_set_data(visual, "position", positions, N) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, N) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, N) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
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
    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}


int test_scene_second_emit_no_uploads_when_not_dirty(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, &desc);
    AT(panel != NULL);
    DvzVisual* visual = dvz_point(scene, 0);
    AT(visual != NULL);

    vec3 positions[2] = {{-0.5f, 0.0f, 0.0f}, {0.5f, 0.0f, 0.0f}};
    DvzColor colors[2] = {{255, 0, 0, 255}, {0, 255, 0, 255}};
    float sizes[2] = {8.0f, 8.0f};
    AT(dvz_visual_set_data(visual, "position", positions, 2) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 2) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 2) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_wgsl = true;

    /* First emit — dirty, must produce WRITE_BUFFER commands. */
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 = _test_scene_emit_stream(figure, &caps, &report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream1 != NULL);

    uint32_t wb_count1 = _stream_visual_write_buffer_count(stream1);
    AT(wb_count1 > 0);
    _test_scene_stream_destroy(stream1);

    /* Second emit — nothing dirty, so no WRITE_BUFFER commands should be emitted. */
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream2 = _test_scene_emit_stream(figure, &caps, &report);
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

    _test_scene_stream_destroy(stream2);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_runtime_emitter_reset_reemits_payloads(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, &desc);
    AT(panel != NULL);
    DvzVisual* visual = dvz_point(scene, 0);
    AT(visual != NULL);

    vec3 positions[2] = {{-0.5f, 0.0f, 0.0f}, {0.5f, 0.0f, 0.0f}};
    DvzColor colors[2] = {{255, 0, 0, 255}, {0, 255, 0, 255}};
    float sizes[2] = {8.0f, 8.0f};
    AT(dvz_visual_set_data(visual, "position", positions, 2) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 2) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 2) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_wgsl = true;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 = _test_scene_emit_stream(figure, &caps, &report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream1 != NULL);
    AT(_stream_visual_write_buffer_count(stream1) > 0);
    _test_scene_stream_destroy(stream1);

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream2 = _test_scene_emit_stream(figure, &caps, &report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream2 != NULL);
    AT(_stream_visual_write_buffer_count(stream2) == 0);
    _test_scene_stream_destroy(stream2);

    AT(_scene_runtime_emitter_reset(scene));

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream3 = _test_scene_emit_stream(figure, &caps, &report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream3 != NULL);
    AT(_stream_visual_write_buffer_count(stream3) > 0);

    bool found_create_buffer = false;
    bool found_draw = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream3); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream3, i);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_BUFFER)
            found_create_buffer = true;
        else if (cmd->type == DVZ_DRP2_COMMAND_DRAW)
            found_draw = true;
    }
    AT(found_create_buffer);
    AT(found_draw);
    AT(dvz_drp2_validate_stream(stream3).ok);

    _test_scene_stream_destroy(stream3);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure retained volume parameter mutations keep on-demand scheduling dirty until emitted.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_pending_render_work_tracks_volume_state(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 2,
                   .height = 2,
                   .depth = 2,
               });
    AT(field != NULL);
    const uint8_t voxels[8] = {255, 255, 255, 255, 255, 255, 255, 255};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView), .data = voxels, .bytes_per_row = 2, .rows_per_image = 2}) == DVZ_OK);

    DvzVisual* volume = dvz_volume(scene, 0);
    AT(volume != NULL);
    AT(dvz_visual_set_field(volume, "field", field) == DVZ_OK);
    AT(dvz_volume_set_render_mode(volume, DVZ_VOLUME_RENDER_MIP) == 0);
    AT(dvz_volume_set_step_count(volume, 16) == 0);
    AT(dvz_panel_add_visual(panel, volume, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_glsl = true;
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    DvzDiagnosticReport report;

    AT(_scene_figure_has_pending_render_work(figure));
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream1 != NULL);
    _test_scene_stream_destroy(stream1);
    AT(!_scene_figure_has_pending_render_work(figure));

    AT(dvz_volume_set_opacity(volume, 0.35f) == 0);
    AT(_scene_figure_has_pending_render_work(figure));
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream2 = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream2 != NULL);
    _test_scene_stream_destroy(stream2);
    AT(!_scene_figure_has_pending_render_work(figure));

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify unlit background primitives do not keep app scheduling work pending.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_pending_render_work_clears_unlit_background(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    AT(dvz_panel_set_background_color(panel, dvz_color_from_unit(0.02f, 0.03f, 0.04f, 1.0f))
       == DVZ_OK);
    AT(panel->background_visual != NULL);
    AT(_scene_figure_has_pending_render_work(figure));

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_glsl = true;
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = _test_scene_emit_stream(figure, &caps, &report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);
    _test_scene_stream_destroy(stream);

    AT(!_scene_figure_has_pending_render_work(figure));

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_hidden_visual_first_visible_later_uploads(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    DvzVisual* visual = dvz_point(scene, 0);
    AT(visual != NULL);

    vec3 positions[2] = {{-0.5f, 0.0f, 0.0f}, {0.5f, 0.0f, 0.0f}};
    DvzColor colors[2] = {{255, 0, 0, 255}, {0, 255, 0, 255}};
    float sizes[2] = {8.0f, 8.0f};
    AT(dvz_visual_set_data(visual, "position", positions, 2) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 2) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 2) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);
    AT(dvz_visual_set_visible(visual, false) == DVZ_OK);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_wgsl = true;
    DvzDiagnosticReport report;

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 = _test_scene_emit_stream(figure, &caps, &report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream1 != NULL);
    AT(_stream_visual_write_buffer_count(stream1) == 0);
    _test_scene_stream_destroy(stream1);

    AT(dvz_visual_set_visible(visual, true) == DVZ_OK);
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream2 = _test_scene_emit_stream(figure, &caps, &report);
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

    _test_scene_stream_destroy(stream2);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_hidden_indexed_mesh_first_visible_later_uploads(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    DvzVisual* visual = dvz_mesh(scene, 0);
    AT(visual != NULL);

    vec3 positions[4] = {
        {-0.5f, -0.5f, 0.0f},
        {+0.5f, -0.5f, 0.0f},
        {-0.5f, +0.5f, 0.0f},
        {+0.5f, +0.5f, 0.0f},
    };
    vec3 normals[4] = {
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
        scene, &(DvzSceneBufferDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc), .usage = DVZ_SCENE_BUFFER_USAGE_INDEX, .stride = sizeof(DvzIndex)});
    AT(index_buffer != NULL);
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)) == DVZ_OK);
    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "normal", normals, 4) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 4) == 0);
    AT(dvz_visual_set_buffer(visual, "index", index_buffer) == DVZ_OK);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);
    AT(dvz_visual_set_visible(visual, false) == DVZ_OK);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_glsl = true;
    DvzDiagnosticReport report;

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 = _test_scene_emit_stream(figure, &caps, &report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream1 != NULL);
    AT(_stream_visual_write_buffer_count(stream1) == 0);
    _test_scene_stream_destroy(stream1);

    AT(dvz_visual_set_visible(visual, true) == DVZ_OK);
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream2 = _test_scene_emit_stream(figure, &caps, &report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream2 != NULL);
    AT(_stream_visual_write_buffer_count(stream2) > 0);
    _test_scene_stream_destroy(stream2);

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream3 = _test_scene_emit_stream(figure, &caps, &report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream3 != NULL);
    AT(_stream_visual_write_buffer_count(stream3) == 0);
    _test_scene_stream_destroy(stream3);

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
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_GRAPH_REQUIRE_VKLITE(suite);

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
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
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
        field, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView), .data = voxels, .bytes_per_row = 2, .rows_per_image = 2}) == DVZ_OK);

    DvzVisual* volume = dvz_volume(scene, 0);
    DvzVisual* slice = dvz_volume(scene, 0);
    DvzVisual* mesh = dvz_mesh(scene, 0);
    AT(volume != NULL);
    AT(slice != NULL);
    AT(mesh != NULL);
    AT(dvz_visual_set_field(volume, "field", field) == DVZ_OK);
    AT(dvz_visual_set_field(slice, "field", field) == DVZ_OK);
    AT(dvz_volume_set_render_mode(volume, DVZ_VOLUME_RENDER_MIP) == 0);
    AT(dvz_volume_set_render_mode(slice, DVZ_VOLUME_RENDER_SLICE) == 0);
    AT(dvz_visual_set_scene_occluder(volume, true) == 0);
    AT(dvz_visual_set_scene_occluded(slice, true) == 0);

    vec3 positions[4] = {
        {-0.5f, -0.5f, 0.1f},
        {+0.5f, -0.5f, 0.1f},
        {-0.5f, +0.5f, 0.1f},
        {+0.5f, +0.5f, 0.1f},
    };
    vec3 normals[4] = {
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
        &(DvzSceneBufferDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
            .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
            .stride = sizeof(DvzIndex),
        });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)) == DVZ_OK);
    AT(dvz_visual_set_data(mesh, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(mesh, "normal", normals, 4) == 0);
    AT(dvz_visual_set_data(mesh, "color", colors, 4) == 0);
    AT(dvz_visual_set_buffer(mesh, "index", index_buffer) == DVZ_OK);
    AT(dvz_visual_set_alpha_mode(mesh, DVZ_ALPHA_WBOIT) == 0);
    AT(dvz_visual_set_depth_test(mesh, true) == 0);
    AT(dvz_visual_set_scene_occluder(mesh, true) == 0);
    AT(dvz_visual_set_visible(mesh, false) == DVZ_OK);

    AT(dvz_panel_add_visual(panel, volume, NULL) == 0);
    AT(dvz_panel_add_visual(panel, slice, NULL) == 0);
    AT(dvz_panel_add_visual(panel, mesh, NULL) == 0);
    AT(dvz_panel_set_scene_occlusion(
           panel,
           &(DvzSceneOcclusionDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneOcclusionDesc),
               .enabled = true,
               .depth_bias = 0.0005f,
               .soft_edge = 0.01f,
               .hidden_alpha = 0.2f,
           }) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
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
    DvzDrp2CommandStream* stream0 = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    ANN(stream0);
    AT(dvz_diagnostic_report_count(&report) == 0);
    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream0);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    _test_scene_stream_destroy(stream0);

    AT(dvz_visual_set_visible(mesh, true) == DVZ_OK);
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
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

    _test_scene_stream_destroy(stream1);
    dvz_drp2_runtime_destroy(runtime);
    dvz_scene_destroy(scene);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}


int test_scene_partial_update_uploads_only_range(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, &desc);
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
        colors[i] = dvz_color_rgb(255, 0, 0);
        sizes[i] = 5.0f;
    }
    AT(dvz_visual_set_data(visual, "position", positions, N) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, N) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, N) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_wgsl = true;

    /* First emit clears dirty flags. */
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 = _test_scene_emit_stream(figure, &caps, &report);
    AT(stream1 != NULL);
    _test_scene_stream_destroy(stream1);

    /* Partial update: items 5–9 only (first_item=5, item_count=5). */
    float new_pos[5 * 3];
    for (uint32_t i = 0; i < 5; i++)
    {
        new_pos[3 * i]     = 0.5f;
        new_pos[3 * i + 1] = 0.5f;
        new_pos[3 * i + 2] = 0.0f;
    }
    AT(dvz_visual_set_data_range(visual, "position", 5, new_pos, 5) == 0);

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream2 = _test_scene_emit_stream(figure, &caps, &report);
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

    _test_scene_stream_destroy(stream2);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_repeated_partial_updates_across_frames(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, &desc);
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
        colors[i]             = dvz_color_rgb(255, 0, 0);
        sizes[i]             = 5.0f;
    }
    AT(dvz_visual_set_data(visual, "position", positions, N) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, N) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, N) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_wgsl = true;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 = _test_scene_emit_stream(figure, &caps, &report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream1 != NULL);
    _test_scene_stream_destroy(stream1);

    const uint64_t item_size = 3 * sizeof(float);

    float frame2_pos[3 * 3] = {
        -0.25f, 0.25f, 0.0f,
        -0.15f, 0.25f, 0.0f,
        -0.05f, 0.25f, 0.0f,
    };
    uint64_t frame2_offset = 2 * item_size;
    uint64_t frame2_size = 3 * item_size;
    AT(dvz_visual_set_data_range(visual, "position", 2, frame2_pos, 3) == 0);

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream2 = _test_scene_emit_stream(figure, &caps, &report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream2 != NULL);
    AT(_stream_visual_write_buffer_count(stream2) == 1);
    AT(_stream_write_buffer_range_count(stream2, frame2_offset, frame2_size) == 1);
    _test_scene_stream_destroy(stream2);

    float frame3_pos[2 * 3] = {
        0.25f, -0.25f, 0.0f,
        0.35f, -0.25f, 0.0f,
    };
    uint64_t frame3_offset = 10 * item_size;
    uint64_t frame3_size = 2 * item_size;
    AT(dvz_visual_set_data_range(visual, "position", 10, frame3_pos, 2) == 0);

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream3 = _test_scene_emit_stream(figure, &caps, &report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream3 != NULL);
    AT(_stream_visual_write_buffer_count(stream3) == 1);
    AT(_stream_write_buffer_range_count(stream3, frame2_offset, frame2_size) == 0);
    AT(_stream_write_buffer_range_count(stream3, frame3_offset, frame3_size) == 1);

    _test_scene_stream_destroy(stream3);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_partial_update_merges_ranges_before_emit(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, &desc);
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
        colors[i]             = dvz_color_rgb(0, 255, 0);
        sizes[i]             = 5.0f;
    }
    AT(dvz_visual_set_data(visual, "position", positions, N) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, N) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, N) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_wgsl = true;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 = _test_scene_emit_stream(figure, &caps, &report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream1 != NULL);
    _test_scene_stream_destroy(stream1);

    float update_a[2 * 3] = {
        -0.75f, 0.1f, 0.0f,
        -0.65f, 0.1f, 0.0f,
    };
    float update_b[3 * 3] = {
        0.15f, 0.1f, 0.0f,
        0.25f, 0.1f, 0.0f,
        0.35f, 0.1f, 0.0f,
    };
    AT(dvz_visual_set_data_range(visual, "position", 2, update_a, 2) == 0);
    AT(dvz_visual_set_data_range(visual, "position", 8, update_b, 3) == 0);

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream2 = _test_scene_emit_stream(figure, &caps, &report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream2 != NULL);

    const uint64_t item_size = 3 * sizeof(float);
    const uint64_t expected_offset = 2 * item_size;
    const uint64_t expected_size = 9 * item_size;
    AT(_stream_visual_write_buffer_count(stream2) == 1);
    AT(_stream_write_buffer_range_count(stream2, expected_offset, expected_size) == 1);

    _test_scene_stream_destroy(stream2);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_multiple_panels_multiple_point_visuals_emit(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 128, 64, 0);
    AT(figure != NULL);
    DvzPanel* left = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 0.5f, 1.0f});
    DvzPanel* right = dvz_panel(figure, &(DvzPanelDesc){0.5f, 0.0f, 0.5f, 1.0f});
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

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_wgsl = true;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 = _test_scene_emit_stream(figure, &caps, &report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream1 != NULL);
    AT(_stream_visual_write_buffer_count(stream1) == 6);
    AT(_stream_set_vertex_buffer_count(stream1) == 6);
    AT(_stream_draw_count(stream1) == 2);
    uint32_t begin_render_pass_count = 0;
    uint32_t clear_begin_render_pass_count = 0;
    uint32_t viewport_count = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream1); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream1, i);
        if (cmd == NULL)
            continue;
        if (cmd->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS)
        {
            if (cmd->u.begin_render_pass.has_explicit_rects)
            {
                AT(cmd->u.begin_render_pass.render_area_px[0] == 0);
                AT(cmd->u.begin_render_pass.render_area_px[1] == 0);
                AT(cmd->u.begin_render_pass.render_area_px[2] > 0);
                AT(cmd->u.begin_render_pass.render_area_px[3] > 0);
            }
            else
            {
                AC(cmd->u.begin_render_pass.viewport[0], 0.0f, 1e-6f);
                AC(cmd->u.begin_render_pass.viewport[1], 0.0f, 1e-6f);
                AC(cmd->u.begin_render_pass.viewport[2], 1.0f, 1e-6f);
                AC(cmd->u.begin_render_pass.viewport[3], 1.0f, 1e-6f);
            }
            if (cmd->u.begin_render_pass.clear)
                clear_begin_render_pass_count++;
            begin_render_pass_count++;
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_SET_VIEWPORT)
        {
            if (viewport_count == 0)
            {
                AC(cmd->u.set_viewport.viewport[0], 0.0f, 1e-6f);
                AC(cmd->u.set_viewport.viewport[1], 0.0f, 1e-6f);
                AC(cmd->u.set_viewport.viewport[2], 2.0f, 1e-6f);
                AC(cmd->u.set_viewport.viewport[3], 4.0f, 1e-6f);
            }
            else if (viewport_count == 1)
            {
                AC(cmd->u.set_viewport.viewport[0], 2.0f, 1e-6f);
                AC(cmd->u.set_viewport.viewport[1], 0.0f, 1e-6f);
                AC(cmd->u.set_viewport.viewport[2], 2.0f, 1e-6f);
                AC(cmd->u.set_viewport.viewport[3], 4.0f, 1e-6f);
            }
            viewport_count++;
        }
    }
    AT(begin_render_pass_count >= 1);
    AT(clear_begin_render_pass_count >= 1);
    AT(viewport_count == 2);
    _test_scene_stream_destroy(stream1);

    float size_update[2] = {10.0f, 11.0f};
    AT(dvz_visual_set_data_range(visual_b, "size", 1, size_update, 2) == 0);

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream2 = _test_scene_emit_stream(figure, &caps, &report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream2 != NULL);
    AT(_stream_visual_write_buffer_count(stream2) == 1);
    AT(_stream_write_buffer_range_count(stream2, sizeof(float), 2 * sizeof(float)) == 1);
    AT(_stream_set_vertex_buffer_count(stream2) == 6);
    AT(_stream_draw_count(stream2) == 2);

    _test_scene_stream_destroy(stream2);
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
int test_scene_visual_alpha_mode(TstContext* suite, const TstCase* item)
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
 * Verify future visual transform/shader descriptor validation.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_visual_shader_transform_future_compat(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzVisualTransformDesc transform = dvz_visual_transform_desc();
    AT(transform.struct_size == DVZ_STRUCT_SIZE(DvzVisualTransformDesc));
    AT(transform.flags == 0);
    AT(transform.kind == DVZ_VISUAL_TRANSFORM_NONE);
    AT(transform.input_space == DVZ_VISUAL_TRANSFORM_SPACE_DATA);
    AT(transform.output_space == DVZ_VISUAL_TRANSFORM_SPACE_VISUAL);
    AC(transform.matrix[0][0], 1.0f, 1e-6);

    DvzVisualShaderDesc shader = dvz_visual_shader_desc();
    AT(shader.struct_size == DVZ_STRUCT_SIZE(DvzVisualShaderDesc));
    AT(shader.flags == 0);
    AT(shader.kind == DVZ_VISUAL_SHADER_NONE);
    AT(shader.vertex_source == DVZ_VISUAL_SHADER_SOURCE_NONE);
    AT(shader.fragment_source == DVZ_VISUAL_SHADER_SOURCE_NONE);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);

    AT(dvz_visual_set_transform_desc(visual, NULL) == 0);
    AT(visual->transform_desc.kind == DVZ_VISUAL_TRANSFORM_NONE);
    AT(dvz_visual_set_transform_desc(visual, &transform) == 0);
    AT(visual->transform_desc.kind == DVZ_VISUAL_TRANSFORM_NONE);

    DvzVisualTransformDesc invalid_transform = dvz_visual_transform_desc();
    invalid_transform.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_visual_set_transform_desc(visual, &invalid_transform) < 0);

    invalid_transform = dvz_visual_transform_desc();
    invalid_transform.flags = 1;
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_visual_set_transform_desc(visual, &invalid_transform) < 0);

    invalid_transform = dvz_visual_transform_desc();
    invalid_transform.kind = DVZ_VISUAL_TRANSFORM_NONLINEAR;
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_visual_set_transform_desc(visual, &invalid_transform) < 0);
    AT(visual->transform_desc.kind == DVZ_VISUAL_TRANSFORM_NONE);

    invalid_transform = dvz_visual_transform_desc();
    invalid_transform.kind = DVZ_VISUAL_TRANSFORM_CUSTOM;
    invalid_transform.transform_id = 7;
    invalid_transform.label = "future-custom-transform";
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_visual_set_transform_desc(visual, &invalid_transform) < 0);

    AT(dvz_visual_set_shader_desc(visual, NULL) == 0);
    AT(visual->shader_desc.kind == DVZ_VISUAL_SHADER_NONE);
    AT(dvz_visual_set_shader_desc(visual, &shader) == 0);
    AT(visual->shader_desc.kind == DVZ_VISUAL_SHADER_NONE);

    DvzVisualShaderDesc invalid_shader = dvz_visual_shader_desc();
    invalid_shader.struct_size = DVZ_STRUCT_SIZE(DvzVisualShaderDesc) - 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_shader_desc(visual, &invalid_shader) < 0);

    invalid_shader = dvz_visual_shader_desc();
    invalid_shader.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_shader_desc(visual, &invalid_shader) < 0);

    invalid_shader = dvz_visual_shader_desc();
    invalid_shader.vertex_source = DVZ_VISUAL_SHADER_SOURCE_GLSL;
    invalid_shader.vertex_code = "void main() {}";
    invalid_shader.vertex_code_size = 14;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_shader_desc(visual, &invalid_shader) < 0);

    invalid_shader = dvz_visual_shader_desc();
    invalid_shader.kind = DVZ_VISUAL_SHADER_CUSTOM_FAMILY;
    invalid_shader.family = "future.custom";
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_shader_desc(visual, &invalid_shader) < 0);
    AT(visual->shader_desc.kind == DVZ_VISUAL_SHADER_NONE);

    invalid_shader = dvz_visual_shader_desc();
    invalid_shader.kind = DVZ_VISUAL_SHADER_BUILTIN_REPLACEMENT;
    invalid_shader.fragment_source = DVZ_VISUAL_SHADER_SOURCE_WGSL;
    invalid_shader.fragment_code = "@fragment fn main() {}";
    invalid_shader.fragment_code_size = 22;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_shader_desc(visual, &invalid_shader) < 0);

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
int test_scene_visual_depth_test(TstContext* suite, const TstCase* item)
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

    DvzFigure* figure = dvz_figure(scene, 96, 96, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel_full(figure);
    AT(panel != NULL);
    DvzVisual* point = dvz_point(scene, 0);
    AT(point != NULL);

    vec3 positions[1] = {{0.0f, 0.0f, 0.0f}};
    DvzColor colors[1] = {{80, 180, 240, 220}};
    float diameters[1] = {8.0f};
    AT(dvz_visual_set_data(point, "position", positions, 1) == 0);
    AT(dvz_visual_set_data(point, "color", colors, 1) == 0);
    AT(dvz_visual_set_data(point, "diameter_px", diameters, 1) == 0);
    AT(dvz_visual_set_alpha_mode(point, DVZ_ALPHA_BLENDED) == 0);
    AT(dvz_visual_set_depth_test(point, false) == 0);
    AT(dvz_panel_add_visual(panel, point, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.supports_color_blending = true;
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    emit_cfg.target_width = 96;
    emit_cfg.target_height = 96;

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);
    _test_scene_stream_destroy(stream);

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
int test_scene_visual_scene_occlusion_flags(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0, 0, 1, 1});
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
           &(DvzSceneOcclusionDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneOcclusionDesc),
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
int test_scene_visual_scene_occlusion_frame_plan(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    DvzVisual* occluder = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    DvzVisual* occluded = dvz_point(scene, 0);
    AT(occluder != NULL);
    AT(occluded != NULL);

    vec3 positions[3] = {
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
           &(DvzSceneOcclusionDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneOcclusionDesc),
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
    AT(occlusion_contract.attachments[0].format == DVZ_FORMAT_R32_SFLOAT);
    AT(occlusion_contract.attachments[0].sample_count == 1);
    AT(occlusion_contract.attachments[1].format == DVZ_FORMAT_D32_SFLOAT);
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
int test_scene_visual_scene_occlusion_emits_drp2(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    DvzVisual* occluder = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    DvzVisual* occluded = dvz_point(scene, 0);
    AT(occluder != NULL);
    AT(occluded != NULL);

    vec3 positions[3] = {
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
           &(DvzSceneOcclusionDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneOcclusionDesc),
               .enabled = true,
               .depth_bias = 0.001f,
               .soft_edge = 0.01f,
               .hidden_alpha = 0.2f,
           }) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.supports_render_target_sampling = true;
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.target_width = 64;
    cfg.target_height = 64;

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
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
                 command->u.create_texture.format == DVZ_FORMAT_R32_SFLOAT);
            has_scene_z =
                has_scene_z ||
                (label != NULL && strcmp(label, "fig0_p0.scene_occlusion.z") == 0 &&
                 command->u.create_texture.format == DVZ_FORMAT_D32_SFLOAT);
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
                 command->u.create_render_pipeline.depth_compare_op == DVZ_COMPARE_OP_LESS_OR_EQUAL);
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

    _test_scene_stream_destroy(stream);
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
int test_scene_volume_slice_uses_volume_occlusion(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
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
        field, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView), .data = voxels, .bytes_per_row = 2, .rows_per_image = 2}) == DVZ_OK);

    DvzVisual* volume = dvz_volume(scene, 0);
    DvzVisual* slice = dvz_volume(scene, 0);
    AT(volume != NULL);
    AT(slice != NULL);
    AT(dvz_visual_set_field(volume, "field", field) == DVZ_OK);
    AT(dvz_visual_set_field(slice, "field", field) == DVZ_OK);
    AT(dvz_volume_set_render_mode(volume, DVZ_VOLUME_RENDER_MIP) == 0);
    AT(dvz_volume_set_step_count(volume, 16) == 0);
    AT(dvz_volume_set_render_mode(slice, DVZ_VOLUME_RENDER_SLICE) == 0);
    AT(dvz_visual_set_volume_occluded(slice, true) == 0);
    AT(dvz_panel_add_visual(panel, volume, NULL) == 0);
    AT(dvz_panel_add_visual(panel, slice, NULL) == 0);
    AT(dvz_panel_set_volume_occluder(
           panel, volume,
           &(DvzVolumeOcclusionDesc){DVZ_STRUCT_INIT_FIELDS(DvzVolumeOcclusionDesc),
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
    AT(volume_contract.attachments[0].format == DVZ_FORMAT_R32_SFLOAT);
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

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.supports_render_target_sampling = true;
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.target_width = 64;
    cfg.target_height = 64;

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
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

    _test_scene_stream_destroy(stream);
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
int test_scene_volume_slice_uses_generic_scene_occlusion(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
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
        field, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView), .data = voxels, .bytes_per_row = 2, .rows_per_image = 2}) == DVZ_OK);

    DvzVisual* volume = dvz_volume(scene, 0);
    DvzVisual* slice = dvz_volume(scene, 0);
    AT(volume != NULL);
    AT(slice != NULL);
    AT(dvz_visual_set_field(volume, "field", field) == DVZ_OK);
    AT(dvz_visual_set_field(slice, "field", field) == DVZ_OK);
    AT(dvz_volume_set_render_mode(volume, DVZ_VOLUME_RENDER_MIP) == 0);
    AT(dvz_volume_set_step_count(volume, 16) == 0);
    AT(dvz_volume_set_render_mode(slice, DVZ_VOLUME_RENDER_SLICE) == 0);
    AT(dvz_visual_set_scene_occluder(volume, true) == 0);
    AT(dvz_visual_set_scene_occluded(slice, true) == 0);
    AT(dvz_panel_add_visual(panel, volume, NULL) == 0);
    AT(dvz_panel_add_visual(panel, slice, NULL) == 0);
    AT(dvz_panel_set_volume_occluder(
           panel, volume,
           &(DvzVolumeOcclusionDesc){DVZ_STRUCT_INIT_FIELDS(DvzVolumeOcclusionDesc),
               .enabled = true,
               .alpha_threshold = 0.01f,
               .fade_distance = 0.04f,
               .occluded_alpha = 0.2f,
           }) == 0);
    AT(dvz_panel_set_scene_occlusion(
           panel,
           &(DvzSceneOcclusionDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneOcclusionDesc),
               .enabled = true,
               .depth_bias = 0.0005f,
               .soft_edge = 0.01f,
               .hidden_alpha = 0.2f,
           }) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.supports_render_target_sampling = true;
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.target_width = 64;
    cfg.target_height = 64;

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
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

    _test_scene_stream_destroy(stream);
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
int test_scene_visual_internal_material_state(TstContext* suite, const TstCase* item)
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
    AT(mesh->material.light_direction[0] == -0.45f);
    AT(mesh->material.light_direction[1] == +0.35f);
    AT(mesh->material.light_direction[2] == 0.82f);
    AT(mesh->material.ambient == 0.24f);
    AT(mesh->material.diffuse == 0.82f);
    AT(!mesh->material.depth_cue_enabled);
    AT(mesh->material.depth_cue_mode == DVZ_DEPTH_CUE_NONE);
    AT(mesh->material.depth_cue_metric == DVZ_DEPTH_CUE_METRIC_CLIP_DEPTH);
    AT(mesh->material.depth_cue_falloff == DVZ_DEPTH_CUE_FALLOFF_LINEAR);
    AT(mesh->material.depth_cue_near == 0.0f);
    AT(mesh->material.depth_cue_far == 1.0f);
    AT(mesh->material.depth_cue_strength == 1.0f);
    AT(mesh->material.depth_cue_density == 3.0f);
    AT(mesh->material.depth_cue_background[3] == 1.0f);
    AT(_visual_family_state(mesh)->material_params.depth_cue[1] == 1.0f);
    AT(_visual_family_state(mesh)->material_params.depth_cue[2] == 0.0f);
    AT(_visual_family_state(mesh)->material_params.depth_cue_extra[2] == 3.0f);
    AT(mesh->material.scalar_scale == 1.0f);

    uint64_t point_material_version = point->material.version;
    AT(dvz_visual_set_depth_cue(
           point,
           &(DvzDepthCueDesc){DVZ_STRUCT_INIT_FIELDS(DvzDepthCueDesc),
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
    AT(_visual_family_state(point)->material_params.depth_cue[0] == 0.1f);
    AT(_visual_family_state(point)->material_params.depth_cue[1] == 0.8f);
    AT(_visual_family_state(point)->material_params.depth_cue[2] == 0.5f);
    AT(_visual_family_state(point)->material_params.depth_cue[3] == (float)DVZ_DEPTH_CUE_FADE_TO_BACKGROUND);
    AT(_visual_family_state(point)->material_params.depth_cue_extra[0] == (float)DVZ_DEPTH_CUE_METRIC_EYE_DISTANCE);
    AT(_visual_family_state(point)->material_params.depth_cue_extra[1] == (float)DVZ_DEPTH_CUE_FALLOFF_EXPONENTIAL);
    AT(_visual_family_state(point)->material_params.depth_cue_extra[2] == 2.0f);
    AT(_visual_family_state(point)->material_params.depth_cue_color[2] == 0.06f);
    AT(point->material.version > point_material_version);
    AT(dvz_visual_set_depth_cue(point, NULL) == 0);
    AT(!point->material.depth_cue_enabled);
    AT(_visual_family_state(point)->material_params.depth_cue[2] == 0.0f);

    AT(dvz_visual_set_alpha_mode(mesh, DVZ_ALPHA_WBOIT) == 0);
    AT(mesh->alpha_mode == DVZ_ALPHA_WBOIT);
    AT(mesh->material.alpha_mode == DVZ_ALPHA_WBOIT);
    uint64_t material_version = mesh->material.version;
    AT(material_version > 0);

    AT(_scene_visuals_set_phong_material(
           mesh, (float[3]){1.0f, 2.0f, 3.0f}, 0.35f, 0.65f, 0.25f, 32.0f) == 0);
    AT(_visual_family_state(mesh)->material_params.light_direction[0] == 1.0f);
    AT(_visual_family_state(mesh)->material_params.light_direction[1] == 2.0f);
    AT(_visual_family_state(mesh)->material_params.light_direction[2] == 3.0f);
    AT(_visual_family_state(mesh)->material_params.params[0] == 0.35f);
    AT(_visual_family_state(mesh)->material_params.params[1] == 0.65f);
    AT(mesh->material.light_direction[0] == 1.0f);
    AT(mesh->material.light_direction[1] == 2.0f);
    AT(mesh->material.light_direction[2] == 3.0f);
    AT(mesh->material.ambient == 0.35f);
    AT(mesh->material.diffuse == 0.65f);
    AT(mesh->material.version > material_version);
    material_version = mesh->material.version;

    AT(dvz_visual_set_depth_cue(
           mesh,
           &(DvzDepthCueDesc){DVZ_STRUCT_INIT_FIELDS(DvzDepthCueDesc),
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
    AT(_visual_family_state(mesh)->material_params.depth_cue[0] == 0.25f);
    AT(_visual_family_state(mesh)->material_params.depth_cue[1] == 0.9f);
    AT(_visual_family_state(mesh)->material_params.depth_cue[2] == 0.75f);
    AT(_visual_family_state(mesh)->material_params.depth_cue[3] == (float)DVZ_DEPTH_CUE_DESATURATE);
    AT(_visual_family_state(mesh)->material_params.depth_cue_color[1] == 0.2f);
    AT(mesh->material.version > material_version);

    material_version = mesh->material.version;
    AT(dvz_visual_set_depth_cue(mesh, NULL) == 0);
    AT(!mesh->material.depth_cue_enabled);
    AT(mesh->material.depth_cue_mode == DVZ_DEPTH_CUE_NONE);
    AT(_visual_family_state(mesh)->material_params.depth_cue[2] == 0.0f);
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
int test_scene_visual_material_setter(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzMaterialDesc defaults = dvz_material_desc();
    AT(defaults.model == DVZ_MATERIAL_MODEL_PHONG);
    AT(defaults.alpha_mode == DVZ_ALPHA_OPAQUE);
    AT(defaults.opacity == 1.0f);
    AT(defaults.base_color_factor[0] == 1.0f);
    AT(defaults.base_color_factor[3] == 1.0f);
    AT(defaults.light_direction[0] == -0.45f);
    AT(defaults.light_direction[1] == +0.35f);
    AT(defaults.light_direction[2] == 0.82f);
    AT(defaults.phong.ambient == 0.24f);
    AT(defaults.phong.diffuse == 0.82f);
    AT(defaults.phong.specular == 0.24f);
    AT(defaults.phong.shininess == 26.0f);
    AT(defaults.standard.roughness == 0.62f);
    AT(defaults.standard.specular == 0.34f);
    AT(defaults.standard.rim_strength == 0.10f);
    DvzMaterialDesc phong_defaults = dvz_phong_material_desc();
    AT(phong_defaults.model == DVZ_MATERIAL_MODEL_PHONG);
    DvzMaterialDesc standard_defaults = dvz_standard_material_desc();
    AT(standard_defaults.model == DVZ_MATERIAL_MODEL_STANDARD);
    AT(standard_defaults.standard.roughness == 0.62f);
    DvzMaterialDesc limb_defaults = dvz_limb_material_desc();
    AT(limb_defaults.model == DVZ_MATERIAL_MODEL_LIMB);
    AT(limb_defaults.alpha_mode == DVZ_ALPHA_BLENDED);
    AT(limb_defaults.opacity == 0.12f);
    AT(limb_defaults.limb.falloff == 4.0f);
    AT(limb_defaults.limb.sun_bias == 0.05f);
    AT(limb_defaults.limb.terminator_width == 0.18f);
    AT(limb_defaults.limb.night_factor == 0.08f);

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

    DvzMaterialDesc phong = dvz_phong_material_desc();
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
    AT(_visual_family_state(mesh)->material_params.params[0] == 0.15f);
    AT(_visual_family_state(mesh)->material_params.params[1] == 0.70f);
    AT(_visual_family_state(mesh)->material_params.params[2] == 0.40f);
    AT(_visual_family_state(mesh)->material_params.params[3] == 48.0f);
    AT(_visual_family_state(mesh)->material_params.model[0] == (float)DVZ_MATERIAL_MODEL_PHONG);
    AT(_visual_family_state(mesh)->material_params.model[1] == 0.5f);
    AT(_visual_family_state(mesh)->material_params.base_color_factor[0] == 0.75f);
    AT(mesh->material.version > version);

    DvzMaterialDesc standard = dvz_standard_material_desc();
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
    AT(_visual_family_state(mesh)->material_params.params[0] > 0.0f);
    AT(_visual_family_state(mesh)->material_params.params[1] > 0.0f);
    AT(_visual_family_state(mesh)->material_params.params[2] == 0.6f);
    AT(_visual_family_state(mesh)->material_params.params[3] > 1.0f);
    AT(_visual_family_state(mesh)->material_params.model[0] == (float)DVZ_MATERIAL_MODEL_STANDARD);
    AT(_visual_family_state(mesh)->material_params.model[1] == 0.9f);
    AT(_visual_family_state(mesh)->material_params.standard_params[0] == 0.25f);
    AT(_visual_family_state(mesh)->material_params.standard_params[1] == 0.6f);
    AT(_visual_family_state(mesh)->material_params.standard_params[2] == 0.2f);
    AT(_visual_family_state(mesh)->material_params.standard_params[3] == 0.3f);
    AT(_visual_family_state(mesh)->material_params.emissive_rim[1] == 0.05f);
    AT(mesh->material.version > version);

    DvzMaterialDesc limb = dvz_limb_material_desc();
    limb.opacity = 0.20f;
    limb.limb.falloff = 5.0f;
    limb.limb.sun_bias = -0.10f;
    limb.limb.terminator_width = 0.25f;
    limb.limb.night_factor = 0.04f;
    version = mesh->material.version;
    AT(dvz_visual_set_material(mesh, &limb) == 0);
    AT(mesh->material.model == DVZ_MATERIAL_MODEL_LIMB);
    AT(mesh->alpha_mode == DVZ_ALPHA_BLENDED);
    AT(mesh->material.opacity == 0.20f);
    AT(mesh->material.limb_falloff == 5.0f);
    AT(mesh->material.limb_sun_bias == -0.10f);
    AT(mesh->material.limb_terminator_width == 0.25f);
    AT(mesh->material.limb_night_factor == 0.04f);
    AT(_visual_family_state(mesh)->material_params.model[0] == (float)DVZ_MATERIAL_MODEL_LIMB);
    AT(_visual_family_state(mesh)->material_params.limb_params[0] == 5.0f);
    AT(_visual_family_state(mesh)->material_params.limb_params[1] == -0.10f);
    AT(_visual_family_state(mesh)->material_params.limb_params[2] == 0.25f);
    AT(_visual_family_state(mesh)->material_params.limb_params[3] == 0.04f);
    AT(mesh->material.version > version);

    AT(dvz_visual_set_depth_cue(
           mesh,
           &(DvzDepthCueDesc){DVZ_STRUCT_INIT_FIELDS(DvzDepthCueDesc),
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
    AT(_visual_family_state(mesh)->material_params.depth_cue[2] == 0.4f);
    AT(_visual_family_state(mesh)->material_params.params[0] == 0.24f);
    AT(_visual_family_state(mesh)->material_params.params[1] == 0.82f);

    AT(_scene_visuals_set_phong_material(
           sphere, (float[3]){0.0f, 1.0f, 0.0f}, 0.3f, 0.6f, 0.2f, 16.0f) == 0);
    AT(sphere->material.model == DVZ_MATERIAL_MODEL_PHONG);
    AT(sphere->material.ambient == 0.3f);
    AT(_visual_family_state(sphere)->material_params.params[3] == 16.0f);
    AT(_visual_family_state(sphere)->material_params.depth_cue_extra[3] == (float)DVZ_SPHERE_MODE_FAST_IMPOSTOR);

#ifndef __clang_analyzer__
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_material(point, &phong) == -1);
    DvzMaterialDesc bad = dvz_material_desc();
    bad.opacity = 2.0f;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_material(mesh, &bad) == -1);
    bad = dvz_limb_material_desc();
    bad.limb.falloff = 0.0f;
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
int test_scene_pixel_depth_cue_toggle_switches_pipeline(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    DvzVisual* pixel = dvz_pixel(scene, 0);
    AT(scene != NULL);
    AT(figure != NULL);
    AT(panel != NULL);
    AT(pixel != NULL);

    vec3 positions[3] = {
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

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_glsl = true;

    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    AT(dvz_visual_set_depth_cue(
           pixel,
           &(DvzDepthCueDesc){DVZ_STRUCT_INIT_FIELDS(DvzDepthCueDesc),
               .mode = DVZ_DEPTH_CUE_DARKEN,
               .near_depth = 0.0f,
               .far_depth = 1.0f,
               .strength = 0.5f,
               .background_color = {0.0f, 0.0f, 0.0f, 1.0f},
           }) == 0);

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* cue_stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(cue_stream);
    AT(_stream_has_render_pipeline_label_part(cue_stream, "_pipe_pixel_cueg_depth"));
    AT(!_stream_has_render_pipeline_label_part(cue_stream, "_pipe_pixelg_depth"));
    _test_scene_stream_destroy(cue_stream);

    AT(dvz_visual_set_depth_cue(pixel, NULL) == 0);
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* plain_stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(plain_stream);
    AT(_stream_has_render_pipeline_label_part(plain_stream, "_pipe_pixelg_depth"));
    AT(!_stream_has_render_pipeline_label_part(plain_stream, "_pipe_pixel_cueg_depth"));

    _test_scene_stream_destroy(plain_stream);
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
int test_scene_visual_pass_capabilities(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    DvzVisual* point = dvz_point(scene, 0);
    DvzVisual* pixel = dvz_pixel(scene, 0);
    DvzVisual* splat = dvz_splat(scene, 0);
    DvzVisual* primitive = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    DvzVisual* path = dvz_path(scene, 0);
    DvzVisual* fixed_primitive = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    DvzVisual* mesh = dvz_mesh(scene, 0);
    DvzVisual* sphere = dvz_sphere(scene, DVZ_SPHERE_FLAGS_LIGHTING);
    DvzVisual* image = dvz_image(scene, 0);
    DvzVisual* volume = dvz_volume(scene, 0);
    AT(point != NULL);
    AT(pixel != NULL);
    AT(splat != NULL);
    AT(primitive != NULL);
    AT(path != NULL);
    AT(fixed_primitive != NULL);
    AT(mesh != NULL);
    AT(sphere != NULL);
    AT(image != NULL);
    AT(volume != NULL);

    vec3 normals[3] = {
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
    };
    AT(dvz_visual_set_data(mesh, "normal", normals, 3) == 0);
    AT(dvz_visual_set_depth_cue(
           point,
           &(DvzDepthCueDesc){DVZ_STRUCT_INIT_FIELDS(DvzDepthCueDesc),
               .mode = DVZ_DEPTH_CUE_DARKEN,
               .near_depth = 0.1f,
               .far_depth = 0.9f,
               .strength = 0.5f,
               .background_color = {0.0f, 0.0f, 0.0f, 1.0f},
           }) == 0);
    AT(dvz_visual_set_depth_cue(
           mesh,
           &(DvzDepthCueDesc){DVZ_STRUCT_INIT_FIELDS(DvzDepthCueDesc),
               .mode = DVZ_DEPTH_CUE_DARKEN,
               .near_depth = 0.1f,
               .far_depth = 0.9f,
               .strength = 0.5f,
               .background_color = {0.0f, 0.0f, 0.0f, 1.0f},
           }) == 0);
    AT(dvz_visual_set_alpha_mode(point, DVZ_ALPHA_WBOIT) == 0);
    AT(dvz_visual_set_alpha_mode(volume, DVZ_ALPHA_BLENDED) == 0);

    DvzVisualAttachDesc fixed = dvz_visual_attach_desc();
    fixed.z_layer = 0;
    fixed.controller_mode = DVZ_CONTROLLER_FIXED;
    fixed.coord_space = DVZ_VISUAL_COORD_VIEW;
    AT(dvz_panel_add_visual(panel, point, NULL) == 0);
    AT(dvz_panel_add_visual(panel, pixel, NULL) == 0);
    AT(dvz_panel_add_visual(panel, primitive, NULL) == 0);
    AT(dvz_panel_add_visual(panel, path, NULL) == 0);
    AT(dvz_panel_add_visual(panel, fixed_primitive, &fixed) == 0);
    AT(dvz_panel_add_visual(panel, mesh, NULL) == 0);
    AT(dvz_panel_add_visual(panel, sphere, NULL) == 0);
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);
    AT(dvz_panel_add_visual(panel, volume, NULL) == 0);
    AT(dvz_panel_add_visual(panel, splat, NULL) == 0);

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

    AT(_scene_visual_pass_caps_from_visual(splat, &panel->visuals[9], &caps));
    AT(caps.kind == DVZ_SCENE_VISUAL_DESC_SPLAT);
    AT(caps.draws_in_transparent_blend_pass);
    AT(!caps.draws_in_opaque_pass);
    AT(caps.uses_source_over_blend);
    AT(caps.writes_color);
    AT(!caps.writes_depth);
    AT(caps.can_write_depth);
    AT(caps.can_depth_test);
    AT(caps.needs_depth_attachment);
    AT(!caps.eligible_for_depth_postprocess);
    AT(!caps.eligible_for_gbuffer);
    AT(caps.uses_common_set);
    AT(!caps.uses_material_set);
    AT(!caps.supports_depth_cue);
    AT(!_scene_technique_gbuffer_plan_add_visual(&gbuffer, splat, &panel->visuals[9]));
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
 * Verify that every active visual type is registered in the family-op table.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_visual_family_registry_coverage(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    const DvzVisualType active_types[] = {
        DVZ_VISUAL_TYPE_POINT,     DVZ_VISUAL_TYPE_PIXEL,  DVZ_VISUAL_TYPE_MARKER,
        DVZ_VISUAL_TYPE_SEGMENT,   DVZ_VISUAL_TYPE_PATH,   DVZ_VISUAL_TYPE_IMAGE,
        DVZ_VISUAL_TYPE_MESH,      DVZ_VISUAL_TYPE_VOLUME, DVZ_VISUAL_TYPE_PRIMITIVE,
        DVZ_VISUAL_TYPE_SPHERE,    DVZ_VISUAL_TYPE_GLYPH,  DVZ_VISUAL_TYPE_TEXT,
        DVZ_VISUAL_TYPE_LABELS,    DVZ_VISUAL_TYPE_SPLAT,  DVZ_VISUAL_TYPE_VECTOR,
    };

    AT(_scene_visual_family_ops_count() == DVZ_ARRAY_COUNT(active_types));
    AT(!_scene_visual_family_ops_registered(DVZ_VISUAL_TYPE_NONE));

    for (uint32_t i = 0; i < DVZ_ARRAY_COUNT(active_types); i++)
    {
        const DvzVisualFamilyOps* ops = _scene_visual_family_ops(active_types[i]);
        ANN(ops);
        AT(ops->type == active_types[i]);
        ANN(ops->name);
        AT(ops->name[0] != '\0');
        ANN(ops->resolve_lowering);
        ANN(ops->resolve_bounds);
        ANN(ops->resolve_pass_caps);
        ANN(ops->resolve_bind_desc);
        ANN(ops->resolve_pipeline_desc);
        ANN(ops->resolve_shader_desc);
        ANN(ops->resolve_draw_desc);
        AT(_scene_visual_family_ops_registered(active_types[i]));
    }

    for (uint32_t i = 0; i < _scene_visual_family_ops_count(); i++)
    {
        const DvzVisualFamilyOps* ops = _scene_visual_family_ops_at(i);
        ANN(ops);
        AT(_scene_visual_family_ops(ops->type) == ops);
    }
    AT(_scene_visual_family_ops_at(_scene_visual_family_ops_count()) == NULL);

    return 0;
}
