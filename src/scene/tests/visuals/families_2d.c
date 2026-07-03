/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene 2D visual family tests                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "common.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_scene_point_emit_glsl_executes(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzGpuCtx* ctx = NULL;
    DvzDrp2Runtime* runtime = _scene_graph_fixture_runtime(suite, &ctx);
    if (runtime == NULL)
        return 0;
    ANN(ctx);

    /* Build scene */
    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, &desc);
    AT(panel != NULL);
    DvzVisual* visual = dvz_point(scene, 0);
    AT(visual != NULL);

    vec3 positions[3] = {
        {-0.5f, -0.5f, 0.0f},
        {0.5f, -0.5f, 0.0f},
        {0.0f, 0.5f, 0.0f},
    };
    DvzColor colors[3] = {{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}};
    float sizes[3]     = {10.0f, 20.0f, 15.0f};

    AT(dvz_visual_set_data(visual, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 3) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 3) == 0);
    AT(dvz_panel_add_visual(
           panel, visual,
           &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc),
               .controller_mode = DVZ_CONTROLLER_APPLY_ISOTROPIC_LOCAL,
           }) == 0);
    AT(panel->visuals[0].controller_mode == DVZ_CONTROLLER_APPLY_ISOTROPIC_LOCAL);

    /* Emit with GLSL */
    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);

    /* Execute on GPU */
    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Execute the scene sphere visual GLSL path through the vklite runtime when available.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_sphere_emit_glsl_executes(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzGpuCtx* ctx = NULL;
    DvzDrp2Runtime* runtime = _scene_graph_fixture_runtime(suite, &ctx);
    if (runtime == NULL)
        return 0;
    ANN(ctx);

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 96, 96, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    DvzVisual* sphere = dvz_sphere(scene, DVZ_SPHERE_FLAGS_LIGHTING);
    AT(sphere != NULL);

    vec3 positions[3] = {
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

    AT(dvz_visual_set_data(sphere, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(sphere, "color", colors, 3) == 0);
    AT(dvz_visual_set_data(sphere, "radius", sizes, 3) == 0);
    AT(dvz_panel_add_visual(panel, sphere, NULL) == 0);

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

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify that sphere rendering mode state is retained and uploaded through material params.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_sphere_mode(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzVisual* sphere = dvz_sphere(scene, DVZ_SPHERE_FLAGS_LIGHTING);
    AT(sphere != NULL);
    AT(_visual_family_state(sphere)->sphere_mode == DVZ_SPHERE_MODE_FAST_IMPOSTOR);
    AT(_visual_family_state(sphere)->material_params.depth_cue_extra[3] == (float)DVZ_SPHERE_MODE_FAST_IMPOSTOR);

    AT(dvz_sphere_set_mode(sphere, DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR) == 0);
    AT(_visual_family_state(sphere)->sphere_mode == DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR);
    AT(_visual_family_state(sphere)->material_params.depth_cue_extra[3] == (float)DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR);
    AT(_visual_family_state(sphere)->material_params_dirty);

    AT(_scene_visuals_set_phong_material(
           sphere, (float[3]){0.0f, 0.0f, 1.0f}, 0.2f, 0.7f, 0.8f, 64.0f) == 0);
    AT(_visual_family_state(sphere)->sphere_mode == DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR);
    AT(_visual_family_state(sphere)->material_params.depth_cue_extra[3] == (float)DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR);
    AT(dvz_sphere_set_mode(sphere, DVZ_SPHERE_MODE_FAST_IMPOSTOR) == 0);
    AT(_visual_family_state(sphere)->material_params.depth_cue_extra[3] == (float)DVZ_SPHERE_MODE_FAST_IMPOSTOR);

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
int test_scene_segment_caps(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    DvzVisual* visual = dvz_segment(scene, 0);
    AT(visual != NULL);
    AT(_visual_family_state(visual)->segment.start_cap == DVZ_SEGMENT_CAP_BUTT);
    AT(_visual_family_state(visual)->segment.end_cap == DVZ_SEGMENT_CAP_BUTT);
    AT(_visual_family_state(visual)->material_params.params[0] == (float)DVZ_SEGMENT_CAP_BUTT);
    AT(_visual_family_state(visual)->material_params.params[1] == (float)DVZ_SEGMENT_CAP_BUTT);

    AT(dvz_segment_set_caps(visual, DVZ_SEGMENT_CAP_ROUND, DVZ_SEGMENT_CAP_SQUARE) == 0);
    AT(_visual_family_state(visual)->segment.start_cap == DVZ_SEGMENT_CAP_ROUND);
    AT(_visual_family_state(visual)->segment.end_cap == DVZ_SEGMENT_CAP_SQUARE);
    AT(_visual_family_state(visual)->material_params.params[0] == (float)DVZ_SEGMENT_CAP_ROUND);
    AT(_visual_family_state(visual)->material_params.params[1] == (float)DVZ_SEGMENT_CAP_SQUARE);

    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_segment_set_caps(visual, (DvzSegmentCap)99, DVZ_SEGMENT_CAP_BUTT) < 0);

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify path cap/join defaults and retained updates.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_path_stroke_style(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    DvzVisual* visual = dvz_path(scene, 0);
    AT(visual != NULL);
    AT(_visual_family_state(visual)->path.cap_start == DVZ_SEGMENT_CAP_ROUND);
    AT(_visual_family_state(visual)->path.cap_end == DVZ_SEGMENT_CAP_ROUND);
    AT(_visual_family_state(visual)->path.join == DVZ_PATH_JOIN_ROUND);
    AT(_visual_family_state(visual)->path.miter_limit == 4.0f);
    AT(_visual_family_state(visual)->material_params.params[0] == (float)DVZ_SEGMENT_CAP_ROUND);
    AT(_visual_family_state(visual)->material_params.params[1] == (float)DVZ_SEGMENT_CAP_ROUND);
    AT(_visual_family_state(visual)->material_params.params[2] == (float)DVZ_PATH_JOIN_ROUND);
    AT(_visual_family_state(visual)->material_params.params[3] == 4.0f);

    AT(dvz_path_set_caps(visual, DVZ_SEGMENT_CAP_BUTT, DVZ_SEGMENT_CAP_SQUARE) == 0);
    AT(dvz_path_set_join(visual, DVZ_PATH_JOIN_MITER, 2.5f) == 0);
    AT(_visual_family_state(visual)->path.cap_start == DVZ_SEGMENT_CAP_BUTT);
    AT(_visual_family_state(visual)->path.cap_end == DVZ_SEGMENT_CAP_SQUARE);
    AT(_visual_family_state(visual)->path.join == DVZ_PATH_JOIN_MITER);
    AT(_visual_family_state(visual)->path.miter_limit == 2.5f);
    AT(_visual_family_state(visual)->material_params.params[0] == (float)DVZ_SEGMENT_CAP_BUTT);
    AT(_visual_family_state(visual)->material_params.params[1] == (float)DVZ_SEGMENT_CAP_SQUARE);
    AT(_visual_family_state(visual)->material_params.params[2] == (float)DVZ_PATH_JOIN_MITER);
    AT(_visual_family_state(visual)->material_params.params[3] == 2.5f);

    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_path_set_caps(visual, (DvzSegmentCap)99, DVZ_SEGMENT_CAP_BUTT) < 0);
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_path_set_join(visual, (DvzPathJoin)99, 4.0f) < 0);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_path_set_join(visual, DVZ_PATH_JOIN_BEVEL, 0.0f) < 0);

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
int test_scene_segment_emit_glsl(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
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
    AT(dvz_visual_set_data(visual, "stroke_width_px", stroke_widths, 2) == 0);
    AT(dvz_segment_set_caps(visual, DVZ_SEGMENT_CAP_BUTT, DVZ_SEGMENT_CAP_ROUND) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzFramePlan* plan = dvz_frame_plan("figure.segment.contract", 0);
    ANN(plan);
    AT(_scene_emit_panel_render(figure, 0, plan, "figure_0"));
    const DvzFramePlanNode* render = dvz_frame_plan_node_get(plan, 0);
    ANN(render);
    AT(dvz_frame_plan_render_pass_role(render) == DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE);
    AT(render->u.render.visual_count == 1);
    AT(render->u.render.visual_metadata[0].renderable_kind == DVZ_RENDERABLE_STROKE_QUAD);
    AT(
        render->u.render.visual_metadata[0].draw_blend_policy ==
        DVZ_SCENE_BLEND_POLICY_SEGMENT_COVERAGE);

    DvzScenePassContract pass_contract = {0};
    AT(_scene_pass_contract_from_render(plan, panel, render, NULL, &pass_contract));
    AT(pass_contract.draw_count == 1);
    AT(pass_contract.draws[0].blend_policy == DVZ_SCENE_BLEND_POLICY_SEGMENT_COVERAGE);
    AT(pass_contract.draws[0].blend_target_count == 1);
    AT(pass_contract.draws[0].blend_targets[0].blend_enabled);
    AT(
        pass_contract.draws[0].blend_targets[0].src_color_blend_factor ==
        DVZ_BLEND_FACTOR_SRC_ALPHA);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
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
                AT(cmd->u.create_render_pipeline.topology == DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
                AT(cmd->u.create_render_pipeline.binding_count == 4);
                AT(cmd->u.create_render_pipeline.attr_count == 4);
                AT(cmd->u.create_render_pipeline.color_targets[0].blend_enabled);
                AT(
                    cmd->u.create_render_pipeline.color_targets[0].src_color_blend_factor ==
                    DVZ_BLEND_FACTOR_SRC_ALPHA);
                AT(
                    cmd->u.create_render_pipeline.color_targets[0].dst_color_blend_factor ==
                    DVZ_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
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

    dvz_frame_plan_destroy(plan);
    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify vector defaults, style updates, and endpoint-derived bounds.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_vector_style_and_bounds(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    DvzVisual* visual = dvz_vector(scene, 0);
    AT(visual != NULL);
    AT(visual->type == DVZ_VISUAL_TYPE_VECTOR);
    AT(_visual_family_state(visual)->vector.scale == 1.0f);
    AT(_visual_family_state(visual)->vector.anchor == DVZ_VECTOR_ANCHOR_TAIL);
    AT(_visual_family_state(visual)->vector.start_cap == DVZ_SEGMENT_CAP_NONE);
    AT(_visual_family_state(visual)->vector.end_cap == DVZ_SEGMENT_CAP_TRIANGLE_OUT);
    AT(_visual_family_state(visual)->material_params.params[0] == (float)DVZ_SEGMENT_CAP_NONE);
    AT(_visual_family_state(visual)->material_params.params[1] == (float)DVZ_SEGMENT_CAP_TRIANGLE_OUT);
    AT(_visual_family_state(visual)->segment.start_cap == 0);
    AT(_visual_family_state(visual)->segment.end_cap == 0);
    AT(_visual_family_state(visual)->path.cap_start == 0);
    AT(_visual_family_state(visual)->path.cap_end == 0);
    AT(_visual_family_state(visual)->path.join == 0);

    DvzVectorStyle style = dvz_vector_style();
    style.scale = 2.0f;
    style.anchor = DVZ_VECTOR_ANCHOR_CENTER;
    style.start_cap = DVZ_SEGMENT_CAP_BUTT;
    style.end_cap = DVZ_SEGMENT_CAP_ROUND;
    style.join = DVZ_PATH_JOIN_MITER;
    style.miter_limit = 2.5f;
    AT(dvz_vector_set_style(visual, &style) == 0);
    AT(_visual_family_state(visual)->vector.scale == 2.0f);
    AT(_visual_family_state(visual)->vector.anchor == DVZ_VECTOR_ANCHOR_CENTER);
    AT(_visual_family_state(visual)->vector.start_cap == DVZ_SEGMENT_CAP_BUTT);
    AT(_visual_family_state(visual)->vector.end_cap == DVZ_SEGMENT_CAP_ROUND);
    AT(_visual_family_state(visual)->vector.join == DVZ_PATH_JOIN_MITER);
    AT(_visual_family_state(visual)->vector.miter_limit == 2.5f);
    AT(_visual_family_state(visual)->segment.start_cap == 0);
    AT(_visual_family_state(visual)->segment.end_cap == 0);
    AT(_visual_family_state(visual)->path.cap_start == 0);
    AT(_visual_family_state(visual)->path.cap_end == 0);
    AT(_visual_family_state(visual)->path.join == 0);

    float positions[] = {
        0.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
    };
    float vectors[] = {
        1.0f, 0.0f, 0.0f,
        0.0f, 2.0f, 0.0f,
    };
    DvzColor colors[2] = {{255, 255, 255, 255}, {255, 0, 0, 255}};
    float widths[2] = {4.0f, 6.0f};
    AT(dvz_visual_set_data(visual, "position", positions, 2) == 0);
    AT(dvz_visual_set_data(visual, "vector", vectors, 2) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 2) == 0);
    AT(dvz_visual_set_data(visual, "stroke_width_px", widths, 2) == 0);

    DvzBounds bounds = {0};
    AT(dvz_visual_bounds(visual, &bounds) == 0);
    AT(_scene_visuals_bounds_expect(&bounds, 2, -1.0, -1.0, 0.0, +1.0, +3.0, 0.0) == 0);

    AT(dvz_vector_set_style(visual, NULL) == 0);
    AT(_visual_family_state(visual)->vector.scale == 1.0f);
    AT(_visual_family_state(visual)->vector.anchor == DVZ_VECTOR_ANCHOR_TAIL);
    AT(_visual_family_state(visual)->vector.start_cap == DVZ_SEGMENT_CAP_NONE);
    AT(_visual_family_state(visual)->vector.end_cap == DVZ_SEGMENT_CAP_TRIANGLE_OUT);

    AT_EXPECTED_ERROR_STRICT(suite, dvz_vector_set_style(visual, &(DvzVectorStyle){DVZ_STRUCT_INIT_FIELDS(DvzVectorStyle),
                                                            .scale = 1.0f,
                                                            .anchor = (DvzVectorAnchor)99,
                                                            .start_cap = DVZ_SEGMENT_CAP_NONE,
                                                            .end_cap = DVZ_SEGMENT_CAP_BUTT,
                                                            .join = DVZ_PATH_JOIN_ROUND,
                                                            .miter_limit = 4.0f,
                                                        }) < 0);

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify straight vector visuals lower to the segment stroke pipeline.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_vector_emit_glsl(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    DvzVisual* visual = dvz_vector(scene, 0);
    AT(visual != NULL);

    float positions[] = {
        -0.8f, -0.4f, 0.0f,
        -0.2f,  0.2f, 0.0f,
    };
    float vectors[] = {
        1.0f, 0.2f, 0.0f,
        0.8f, 0.0f, 0.0f,
    };
    DvzColor colors[2] = {{255, 64, 32, 255}, {64, 160, 255, 255}};
    float stroke_widths[2] = {8.0f, 5.0f};

    AT(dvz_visual_set_data(visual, "position", positions, 2) == 0);
    AT(dvz_visual_set_data(visual, "vector", vectors, 2) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 2) == 0);
    AT(dvz_visual_set_data(visual, "stroke_width_px", stroke_widths, 2) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);
    AT(_visual_family_state(visual)->segment.gpu.vertex_count == 0);
    AT(_visual_family_state(visual)->segment.gpu.index_count == 0);
    AT(_visual_family_state(visual)->path.gpu.vertex_count == 0);
    AT(_visual_family_state(visual)->path.gpu.index_count == 0);
    AT(_visual_family_state(visual)->vector.stroke_gpu.vertex_count == 8);
    AT(_visual_family_state(visual)->vector.stroke_gpu.index_count == 12);

    bool found_pipeline = false;
    bool found_set_index = false;
    bool found_draw_indexed = false;
    uint32_t set_vertex_buffer_count = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        ANN(cmd);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.create_render_pipeline.id);
            if (label != NULL && strstr(label, "_pipe_segmentg") == label)
                found_pipeline = true;
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

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify curved vector visuals lower to the path stroke pipeline.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_vector_curved_emit_glsl(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    DvzVisual* visual = dvz_vector(scene, 0);
    AT(visual != NULL);

    vec3 positions[5] = {
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
    AT(dvz_visual_set_data(visual, "stroke_width_px", stroke_widths, 5) == 0);
    AT(dvz_vector_set_subpaths(visual, 2, subpaths) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);
    AT(_visual_family_state(visual)->segment.gpu.vertex_count == 0);
    AT(_visual_family_state(visual)->segment.gpu.index_count == 0);
    AT(_visual_family_state(visual)->path.gpu.vertex_count == 0);
    AT(_visual_family_state(visual)->path.gpu.index_count == 0);
    AT(_visual_family_state(visual)->vector.subpath_count == 2);
    AT(_visual_family_state(visual)->vector.subpath_lengths != NULL);
    AT(_visual_family_state(visual)->vector.path_gpu.vertex_count == 12);
    AT(_visual_family_state(visual)->vector.path_gpu.index_count == 18);

    bool found_pipeline = false;
    bool found_set_index = false;
    bool found_draw_indexed = false;
    bool found_path_distance_buffer = false;
    uint32_t set_vertex_buffer_count = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        ANN(cmd);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.create_render_pipeline.id);
            if (label != NULL && strstr(label, "_pipe_pathg") == label)
            {
                found_pipeline = true;
                AT(cmd->u.create_render_pipeline.binding_count == 8);
                AT(cmd->u.create_render_pipeline.attr_count == 8);
            }
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_CREATE_BUFFER)
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.create_buffer.id);
            if (label != NULL && strstr(label, "path_distance") != NULL)
            {
                found_path_distance_buffer = true;
                AT(cmd->u.create_buffer.size == 12 * sizeof(float));
                AT((cmd->u.create_buffer.usage & DVZ_DRP2_BUFFER_USAGE_VERTEX) != 0);
            }
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER)
            set_vertex_buffer_count++;
        else if (cmd->type == DVZ_DRP2_COMMAND_SET_INDEX_BUFFER)
            found_set_index = strcmp(cmd->u.set_index_buffer.index_format, "uint32") == 0;
        else if (cmd->type == DVZ_DRP2_COMMAND_DRAW_INDEXED)
            found_draw_indexed = cmd->u.draw_indexed.index_count == 18;
    }

    AT(found_pipeline);
    AT(found_path_distance_buffer);
    AT(found_set_index);
    AT(found_draw_indexed);
    AT(set_vertex_buffer_count == 8);
    AT(_stream_write_buffer_range_count(stream, 0, sizeof(DvzSceneMaterialParams)) >= 1);

    _test_scene_stream_destroy(stream);
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
int test_scene_point_like_lowering_policy(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScenePointLikeLoweringDesc lowering = {0};
    AT(_scene_point_like_lowering_desc(
        DVZ_SCENE_POINT_LIKE_POINT, DVZ_SCENE_SHADER_FORMAT_GLSL, 3, &lowering));
    AT(lowering.kind == DVZ_SCENE_POINT_LIKE_POINT);
    AT(lowering.lowering == DVZ_SCENE_POINT_LIKE_LOWERING_NATIVE_POINTS);
    AT(lowering.topology == DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST);
    AT(lowering.vertex_step_mode == DVZ_DRP2_VERTEX_STEP_MODE_VERTEX);
    AT(lowering.draw_vertex_count == 3);
    AT(lowering.draw_instance_count == 1);

    AT(_scene_point_like_lowering_desc(
        DVZ_SCENE_POINT_LIKE_PIXEL, DVZ_SCENE_SHADER_FORMAT_WGSL, 3, &lowering));
    AT(lowering.kind == DVZ_SCENE_POINT_LIKE_PIXEL);
    AT(lowering.lowering == DVZ_SCENE_POINT_LIKE_LOWERING_INSTANCED_QUADS);
    AT(lowering.topology == DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    AT(lowering.vertex_step_mode == DVZ_DRP2_VERTEX_STEP_MODE_INSTANCE);
    AT(lowering.draw_vertex_count == 6);
    AT(lowering.draw_instance_count == 3);

    return 0;
}



/**
 * Verify retained splat visual attributes and value validation.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_splat_api_and_attrs(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* visual = dvz_splat(scene, 0);
    ANN(visual);

    AT(visual->type == DVZ_VISUAL_TYPE_SPLAT);
    AT(visual->alpha_mode == DVZ_ALPHA_BLENDED);
    AT(visual->depth_test_enabled);

    vec3 positions[2] = {{-0.25f, 0.0f, 0.0f}, {+0.25f, 0.0f, 0.0f}};
    DvzColor colors[2] = {{255, 0, 0, 128}, {0, 255, 0, 192}};
    vec2 sigma[2] = {{4.0f, 8.0f}, {6.0f, 3.0f}};
    float angles[2] = {0.0f, 0.5f};
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = 2},
        {.attr_name = "color", .data = colors, .item_count = 2},
        {.attr_name = "sigma", .data = sigma, .item_count = 2},
        {.attr_name = "angle", .data = angles, .item_count = 2},
    };
    AT(dvz_visual_set_data_many(visual, updates, 4) == 0);

    DvzVisualDataView view = {0};
    AT(dvz_visual_data(visual, "sigma", &view) == 0);
    AT(view.item_count == 2);
    AT(view.item_size == 2 * sizeof(float));
    AT(dvz_visual_data(visual, "angle", &view) == 0);
    AT(view.item_count == 2);
    AT(view.item_size == sizeof(float));

    vec2 bad_sigma[2] = {{4.0f, 0.0f}, {6.0f, 3.0f}};
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_data(visual, "sigma", bad_sigma, 2) == -1);
    float bad_angles[2] = {0.0f, NAN};
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_data(visual, "angle", bad_angles, 2) == -1);

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify splat visuals lower to instanced Gaussian screen-space quads.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_splat_emit_instanced_quads(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    vec3 positions[3] = {
        {-0.5f, -0.4f, 0.0f},
        { 0.0f,  0.4f, 0.0f},
        { 0.5f, -0.4f, 0.0f},
    };
    DvzColor colors[3] = {{255, 0, 0, 192}, {0, 180, 255, 192}, {255, 255, 255, 192}};
    vec2 sigma[3] = {{5.0f, 5.0f}, {8.0f, 4.0f}, {3.0f, 9.0f}};
    float angles[3] = {0.0f, 0.6f, -0.4f};

    for (uint32_t pass = 0; pass < 2; pass++)
    {
        bool wgsl = pass == 1;
        DvzScene* scene = dvz_scene();
        AT(scene != NULL);
        DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
        AT(figure != NULL);
        DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
        AT(panel != NULL);
        DvzVisual* visual = dvz_splat(scene, 0);
        AT(visual != NULL);

        AT(dvz_visual_set_data(visual, "position", positions, 3) == 0);
        AT(dvz_visual_set_data(visual, "color", colors, 3) == 0);
        AT(dvz_visual_set_data(visual, "sigma", sigma, 3) == 0);
        AT(dvz_visual_set_data(visual, "angle", angles, 3) == 0);
        AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

        DvzCapabilitySnapshot caps = dvz_capability_snapshot();
        caps.supports_color_blending = true;
        if (wgsl)
        {
            caps.shader_format_wgsl = true;
            caps.shader_format_glsl = false;
            caps.max_vertex_buffers = 16;
            caps.max_bind_groups = 4;
            caps.max_buffer_size = 256 * 1024 * 1024;
        }

        DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
        emit_cfg.shader_format =
            wgsl ? DVZ_SCENE_SHADER_FORMAT_WGSL : DVZ_SCENE_SHADER_FORMAT_GLSL;

        DvzDiagnosticReport report;
        dvz_diagnostic_report_init(&report);
        DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
        AT(dvz_diagnostic_report_count(&report) == 0);
        ANN(stream);

        bool found_pipeline = false;
        bool found_draw = false;
        bool found_sigma_buffer = false;
        bool found_angle_buffer = false;
        const uint32_t count = dvz_drp2_stream_count(stream);
        for (uint32_t i = 0; i < count; i++)
        {
            const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
            ANN(command);
            if (command->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
            {
                if (command->u.create_render_pipeline.binding_count != 4)
                    continue;
                found_pipeline = true;
                AT(command->u.create_render_pipeline.topology ==
                   DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
                AT(command->u.create_render_pipeline.binding_count == 4);
                AT(command->u.create_render_pipeline.binding_strides[0] == 3 * sizeof(float));
                AT(command->u.create_render_pipeline.binding_strides[1] == 4 * sizeof(uint8_t));
                AT(command->u.create_render_pipeline.binding_strides[2] == 2 * sizeof(float));
                AT(command->u.create_render_pipeline.binding_strides[3] == sizeof(float));
                AT(command->u.create_render_pipeline.binding_step_modes[0] ==
                   DVZ_DRP2_VERTEX_STEP_MODE_INSTANCE);
                AT(command->u.create_render_pipeline.binding_step_modes[1] ==
                   DVZ_DRP2_VERTEX_STEP_MODE_INSTANCE);
                AT(command->u.create_render_pipeline.binding_step_modes[2] ==
                   DVZ_DRP2_VERTEX_STEP_MODE_INSTANCE);
                AT(command->u.create_render_pipeline.binding_step_modes[3] ==
                   DVZ_DRP2_VERTEX_STEP_MODE_INSTANCE);
            }
            else if (command->type == DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER &&
                     command->u.set_vertex_buffer.slot == 2)
            {
                found_sigma_buffer = true;
            }
            else if (command->type == DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER &&
                     command->u.set_vertex_buffer.slot == 3)
            {
                found_angle_buffer = true;
            }
            else if (command->type == DVZ_DRP2_COMMAND_DRAW)
            {
                found_draw = found_draw ||
                             (command->u.draw.vertex_count == 6 &&
                              command->u.draw.instance_count == 3);
            }
        }
        AT(found_pipeline);
        AT(found_sigma_buffer);
        AT(found_angle_buffer);
        AT(found_draw);

        char* json = dvz_drp2_stream_json(stream, wgsl ? "scene_splat_wgsl" : "scene_splat_glsl");
        ANN(json);
        if (wgsl)
            AT(strstr(json, "\"format\": \"wgsl\"") != NULL);
        AT(strstr(json, "\"topology\": \"triangle-list\"") != NULL);
        AT(strstr(json, "\"step_mode\": \"instance\"") != NULL);
        AT(strstr(json, "\"vertex_count\": 6") != NULL);
        AT(strstr(json, "\"instance_count\": 3") != NULL);
        dvz_drp2_stream_json_destroy(json);

        _test_scene_stream_destroy(stream);
        dvz_scene_destroy(scene);
    }

    return 0;
}



/**
 * Verify GLSL point visuals keep native point-list draw semantics.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_point_emit_glsl_native_points(TstContext* suite, const TstCase* item)
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

    vec3 positions[3] = {
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

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();

    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
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
            AT(command->u.create_render_pipeline.topology == DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST);
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

    _test_scene_stream_destroy(stream);
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
int test_scene_point_style_emits_glsl_and_wgsl(TstContext* suite, const TstCase* item)
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

    vec3 positions[2] = {{-0.25f, 0.0f, 0.0f}, {+0.25f, 0.0f, 0.0f}};
    DvzColor colors[2] = {{255, 80, 40, 255}, {80, 160, 255, 255}};
    float sizes[2] = {18.0f, 22.0f};
    AT(dvz_visual_set_data(visual, "position", positions, 2) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 2) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 2) == 0);
    AT(dvz_point_set_style(
           visual,
           &(DvzPointStyleDesc){DVZ_STRUCT_INIT_FIELDS(DvzPointStyleDesc),
               .edge_color = {0, 0, 0, 255},
               .stroke_width_px = 3.0f,
               .aspect = DVZ_SHAPE_ASPECT_OUTLINE,
           }) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    DvzDrp2CommandStream* glsl_stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(glsl_stream);
    AT(_stream_has_render_pipeline_label(
        glsl_stream, "_pipe_point_styleg_coverage_blend_depth"));

    bool found_material_bg = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(glsl_stream); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(glsl_stream, i);
        ANN(command);
        if (command->type == DVZ_DRP2_COMMAND_SET_BIND_GROUP)
            found_material_bg = found_material_bg || command->u.set_bind_group.slot == 1;
    }
    AT(found_material_bg);
    _test_scene_stream_destroy(glsl_stream);

    caps.shader_format_wgsl = true;
    caps.shader_format_glsl = false;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups = 4;
    caps.max_buffer_size = 256 * 1024 * 1024;
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_WGSL;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* wgsl_stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(wgsl_stream);
    AT(_stream_has_render_pipeline_label(
        wgsl_stream, "_pipe_point_stylew_coverage_blend_depth"));
    char* json = dvz_drp2_stream_json(wgsl_stream, "scene_point_style_wgsl_from_c");
    ANN(json);
    AT(strstr(json, "\"format\": \"wgsl\"") != NULL);
    AT(strstr(json, "line_width") != NULL);
    AT(strstr(json, "line_width > 0.0") != NULL);
    dvz_drp2_stream_json_destroy(json);
    _test_scene_stream_destroy(wgsl_stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_point_filled_no_stroke_uses_fill_shader(TstContext* suite, const TstCase* item)
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

    vec3 position[1] = {{0.0f, 0.0f, 0.0f}};
    DvzColor color[1] = {{255, 80, 40, 255}};
    float diameter_px[1] = {24.0f};
    AT(dvz_visual_set_data(visual, "position", position, 1) == 0);
    AT(dvz_visual_set_data(visual, "color", color, 1) == 0);
    AT(dvz_visual_set_data(visual, "diameter_px", diameter_px, 1) == 0);

    DvzPointStyleDesc style = dvz_point_style_desc();
    style.stroke_width_px = 0.0f;
    style.aspect = DVZ_SHAPE_ASPECT_FILLED;
    AT(dvz_point_set_style(visual, &style) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);
    AT(_stream_has_render_pipeline_label(stream, "_pipe_pointg_coverage_blend_depth"));
    AT(!_stream_has_render_pipeline_label(stream, "_pipe_point_styleg_coverage_blend_depth"));

    bool found_material_bg = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        ANN(command);
        if (command->type == DVZ_DRP2_COMMAND_SET_BIND_GROUP)
            found_material_bg = found_material_bg || command->u.set_bind_group.slot == 1;
    }
    AT(!found_material_bg);

    _test_scene_stream_destroy(stream);
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
int test_scene_marker_api_and_emit_glsl(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    DvzVisual* visual = dvz_marker(scene, 0);
    AT(visual != NULL);
    AT(visual->type == DVZ_VISUAL_TYPE_MARKER);
    AT(DVZ_MARKER_SHAPE_ROUNDED_RECT == 20);
    AT((uint32_t)DVZ_SYMBOL_ROUNDED_RECT == (uint32_t)DVZ_MARKER_SHAPE_ROUNDED_RECT);

    DvzSymbolSet* symbol_set = dvz_symbol_set(scene, 0);
    AT(symbol_set != NULL);
    AT(dvz_marker_set_symbols(visual, symbol_set) == 0);
    uint8_t bitmap_rgba[2 * 2 * 4] = {
        255, 0, 0, 255,
        0, 255, 0, 255,
        0, 0, 255, 255,
        255, 255, 255, 255,
    };
    uint8_t sdf[2 * 2] = {0, 96, 160, 255};
    uint8_t msdf[2 * 2 * 3] = {
        0, 0, 0,
        96, 96, 96,
        160, 160, 160,
        255, 255, 255,
    };
    DvzSymbolImageDesc symbol_desc = dvz_symbol_image_desc();
    symbol_desc.distance_range_px = 4.0f;
    const DvzSymbolId bitmap_symbol =
        dvz_symbol_bitmap(symbol_set, "bitmap", bitmap_rgba, 2, 2, NULL);
    const DvzSymbolId sdf_symbol = dvz_symbol_sdf(symbol_set, "sdf", sdf, 2, 2, &symbol_desc);
    const DvzSymbolId msdf_symbol = dvz_symbol_msdf(symbol_set, "msdf", msdf, 2, 2, &symbol_desc);
    AT(bitmap_symbol != DVZ_SYMBOL_ID_INVALID);
    AT(sdf_symbol == bitmap_symbol + 1);
    AT(msdf_symbol == sdf_symbol + 1);
    AT(symbol_set->source_count == 3);
    AT(symbol_set->atlas_pages[DVZ_SYMBOL_SOURCE_BITMAP].width == 4);
    AT(symbol_set->atlas_pages[DVZ_SYMBOL_SOURCE_BITMAP].height == 4);
    AT(symbol_set->atlas_pages[DVZ_SYMBOL_SOURCE_BITMAP].channels == 4);
    AT(symbol_set->atlas_pages[DVZ_SYMBOL_SOURCE_BITMAP].data[20] == 255);
    AT(symbol_set->atlas_pages[DVZ_SYMBOL_SOURCE_SDF].width == 4);
    AT(symbol_set->atlas_pages[DVZ_SYMBOL_SOURCE_SDF].height == 4);
    AT(symbol_set->atlas_pages[DVZ_SYMBOL_SOURCE_SDF].channels == 1);
    AT(symbol_set->atlas_pages[DVZ_SYMBOL_SOURCE_SDF].data[9] == 160);
    AT(symbol_set->atlas_pages[DVZ_SYMBOL_SOURCE_MSDF].width == 4);
    AT(symbol_set->atlas_pages[DVZ_SYMBOL_SOURCE_MSDF].height == 4);
    AT(symbol_set->atlas_pages[DVZ_SYMBOL_SOURCE_MSDF].channels == 4);
    AT(symbol_set->atlas_pages[DVZ_SYMBOL_SOURCE_MSDF].data[36] == 160);
    AT(symbol_set->atlas_pages[DVZ_SYMBOL_SOURCE_MSDF].data[43] == 255);

    vec3 positions[5] = {
        {-0.50f, 0.0f, 0.0f},
        {-0.25f, 0.0f, 0.0f},
        {0.00f, 0.0f, 0.0f},
        {+0.25f, 0.0f, 0.0f},
        {+0.50f, 0.0f, 0.0f},
    };
    DvzColor colors[5] = {
        {255, 80, 40, 255},
        {80, 255, 120, 255},
        {80, 120, 255, 255},
        {255, 210, 64, 255},
        {210, 96, 255, 255},
    };
    float sizes[5] = {18.0f, 22.0f, 26.0f, 30.0f, 34.0f};
    float angles[5] = {0.0f, 0.25f, 0.5f, 0.0f, 0.75f};
    DvzSymbolId symbols[5] = {
        dvz_symbol_builtin(symbol_set, DVZ_SYMBOL_DISC),
        dvz_symbol_builtin(symbol_set, DVZ_SYMBOL_DIAMOND),
        dvz_symbol_builtin(symbol_set, DVZ_SYMBOL_ARROW),
        dvz_symbol_builtin(symbol_set, DVZ_SYMBOL_HEART),
        dvz_symbol_builtin(symbol_set, DVZ_SYMBOL_ROUNDED_RECT),
    };
    for (uint32_t i = 0; i < 5; i++)
        AT(symbols[i] != DVZ_SYMBOL_ID_INVALID);
    AT(dvz_visual_set_data(visual, "position", positions, 5) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 5) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 5) == 0);
    AT(dvz_visual_set_data(visual, "angle", angles, 5) == 0);
    AT(dvz_visual_set_data(visual, "symbol", symbols, 5) == 0);
    AT(visual->attr_count == 5);
    const int shape_idx = _attr_index(visual, "shape");
    AT(shape_idx >= 0);
    const uint32_t* stored_symbols = (const uint32_t*)visual->attrs[shape_idx].data;
    ANN(stored_symbols);
    AT(stored_symbols[2] == DVZ_SYMBOL_ARROW);
    uint32_t unavailable_symbols[5] = {
        DVZ_SYMBOL_RING,
        DVZ_SYMBOL_RING,
        DVZ_SYMBOL_RING,
        DVZ_SYMBOL_RING,
        DVZ_SYMBOL_RING,
    };
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_visual_set_data(visual, "symbol", unavailable_symbols, 5) == -1);
    unavailable_symbols[0] = bitmap_symbol;
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_visual_set_data(visual, "symbol", unavailable_symbols, 5) == -1);
    AT(dvz_marker_set_symbol(visual, DVZ_SYMBOL_RING) == 0);
    stored_symbols = (const uint32_t*)visual->attrs[shape_idx].data;
    ANN(stored_symbols);
    AT(stored_symbols[0] == DVZ_SYMBOL_RING);
    AT(dvz_marker_set_style(
           visual,
           &(DvzMarkerStyle){DVZ_STRUCT_INIT_FIELDS(DvzMarkerStyle),
               .edge_color = {0, 0, 0, 255},
               .stroke_width_px = 2.0f,
               .aspect = DVZ_SHAPE_ASPECT_OUTLINE,
           }) == 0);
    tst_log_capture_begin(suite);
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_visual_set_data(visual, "symbol", symbols, 2) == -1);
    AT(_captured_log_contains(suite, "item_count"));
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzVisual* bitmap_visual = dvz_marker(scene, 0);
    AT(bitmap_visual != NULL);
    AT(dvz_marker_set_symbols(bitmap_visual, symbol_set) == 0);
    vec3 bitmap_positions[2] = {{-0.30f, -0.35f, 0.0f}, {+0.30f, -0.35f, 0.0f}};
    DvzColor bitmap_colors[2] = {{255, 255, 255, 255}, {128, 255, 255, 200}};
    float bitmap_sizes[2] = {20.0f, 28.0f};
    float bitmap_angles[2] = {0.0f, 0.5f};
    DvzSymbolId bitmap_symbols[2] = {bitmap_symbol, bitmap_symbol};
    AT(dvz_visual_set_data(bitmap_visual, "position", bitmap_positions, 2) == 0);
    AT(dvz_visual_set_data(bitmap_visual, "color", bitmap_colors, 2) == 0);
    AT(dvz_visual_set_data(bitmap_visual, "size", bitmap_sizes, 2) == 0);
    AT(dvz_visual_set_data(bitmap_visual, "angle", bitmap_angles, 2) == 0);
    AT(dvz_visual_set_data(bitmap_visual, "symbol", bitmap_symbols, 2) == 0);
    const int tex_rect_idx = _attr_index(bitmap_visual, "tex_rect");
    AT(tex_rect_idx >= 0);
    const float* stored_tex_rects = (const float*)bitmap_visual->attrs[tex_rect_idx].data;
    ANN(stored_tex_rects);
    AT(bitmap_visual->attrs[tex_rect_idx].item_count == 2);
    AC(stored_tex_rects[0], 0.375, 1e-6);
    AC(stored_tex_rects[1], 0.375, 1e-6);
    AC(stored_tex_rects[2], 0.625, 1e-6);
    AC(stored_tex_rects[3], 0.625, 1e-6);
    AC(stored_tex_rects[4], 0.375, 1e-6);
    AC(stored_tex_rects[5], 0.375, 1e-6);
    AC(stored_tex_rects[6], 0.625, 1e-6);
    AC(stored_tex_rects[7], 0.625, 1e-6);
    AT(dvz_panel_add_visual(panel, bitmap_visual, NULL) == 0);

    DvzVisual* sdf_visual = dvz_marker(scene, 0);
    AT(sdf_visual != NULL);
    AT(dvz_marker_set_symbols(sdf_visual, symbol_set) == 0);
    vec3 sdf_positions[2] = {{-0.30f, +0.35f, 0.0f}, {+0.30f, +0.35f, 0.0f}};
    DvzColor sdf_colors[2] = {{255, 255, 255, 255}, {255, 128, 255, 200}};
    float sdf_sizes[2] = {20.0f, 28.0f};
    float sdf_angles[2] = {0.0f, 0.25f};
    DvzSymbolId sdf_symbols[2] = {sdf_symbol, sdf_symbol};
    AT(dvz_visual_set_data(sdf_visual, "position", sdf_positions, 2) == 0);
    AT(dvz_visual_set_data(sdf_visual, "color", sdf_colors, 2) == 0);
    AT(dvz_visual_set_data(sdf_visual, "size", sdf_sizes, 2) == 0);
    AT(dvz_visual_set_data(sdf_visual, "angle", sdf_angles, 2) == 0);
    AT(dvz_visual_set_data(sdf_visual, "symbol", sdf_symbols, 2) == 0);
    AT(_visual_family_state(sdf_visual)->symbol_source_kind == DVZ_SYMBOL_SOURCE_SDF);
    AT(_visual_family_state(sdf_visual)->glyph_atlas_encoding == DVZ_TEXT_ATLAS_ENCODING_SDF_ALPHA);
    AC(_visual_family_state(sdf_visual)->glyph_distance_range_px, 4.0, 1e-6);
    AT(dvz_panel_add_visual(panel, sdf_visual, NULL) == 0);

    DvzVisual* msdf_visual = dvz_marker(scene, 0);
    AT(msdf_visual != NULL);
    AT(dvz_marker_set_symbols(msdf_visual, symbol_set) == 0);
    vec3 msdf_positions[2] = {{-0.30f, +0.10f, 0.0f}, {+0.30f, +0.10f, 0.0f}};
    DvzColor msdf_colors[2] = {{255, 255, 255, 255}, {255, 255, 128, 200}};
    float msdf_sizes[2] = {20.0f, 28.0f};
    float msdf_angles[2] = {0.0f, 0.75f};
    DvzSymbolId msdf_symbols[2] = {msdf_symbol, msdf_symbol};
    AT(dvz_visual_set_data(msdf_visual, "position", msdf_positions, 2) == 0);
    AT(dvz_visual_set_data(msdf_visual, "color", msdf_colors, 2) == 0);
    AT(dvz_visual_set_data(msdf_visual, "size", msdf_sizes, 2) == 0);
    AT(dvz_visual_set_data(msdf_visual, "angle", msdf_angles, 2) == 0);
    AT(dvz_visual_set_data(msdf_visual, "symbol", msdf_symbols, 2) == 0);
    AT(_visual_family_state(msdf_visual)->symbol_source_kind == DVZ_SYMBOL_SOURCE_MSDF);
    AT(_visual_family_state(msdf_visual)->glyph_atlas_encoding == DVZ_TEXT_ATLAS_ENCODING_MSDF_RGB);
    AC(_visual_family_state(msdf_visual)->glyph_distance_range_px, 4.0, 1e-6);
    AT(dvz_panel_add_visual(panel, msdf_visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);
    AT(_stream_has_render_pipeline_label(stream, "_pipe_markerg_coverage_blend_depth"));
    AT(_stream_has_render_pipeline_label(
        stream, "_pipe_marker_bitmapg_coverage_blend_depth"));
    AT(_stream_has_render_pipeline_label(
        stream, "_pipe_marker_distanceg_coverage_blend_depth"));

    bool found_pipeline = false;
    bool found_bitmap_pipeline = false;
    bool found_bitmap_image_layout = false;
    bool found_distance_glyph_layout = false;
    bool found_material_bg = false;
    bool found_set1_bg = false;
    bool found_draw = false;
    bool found_bitmap_draw = false;
    bool found_texture = false;
    bool found_texture_upload = false;
    bool found_sdf_texture = false;
    bool found_sdf_texture_upload = false;
    bool found_msdf_texture = false;
    bool found_msdf_texture_upload = false;
    uint32_t rgba_srgb_texture_count = 0;
    uint32_t rgba_data_texture_count = 0;
    uint32_t r8_data_texture_count = 0;
    bool found_vertex_slots[6] = {false};
    uint32_t set_vertex_buffer_count = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        ANN(command);
        if (command->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE &&
            command->u.create_render_pipeline.binding_count == 5)
        {
            found_pipeline = true;
            AT(command->u.create_render_pipeline.attr_count == 5);
            AT(command->u.create_render_pipeline.topology == DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST);
            AT(command->u.create_render_pipeline.attr_locations[4] == 4);
            AT(command->u.create_render_pipeline.attr_formats[4] == DVZ_FORMAT_R32_UINT);
        }
        else if (command->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE &&
                 command->u.create_render_pipeline.binding_count == 6)
        {
            const char* pipeline_label =
                dvz_drp2_stream_label(stream, command->u.create_render_pipeline.id);
            ANN(pipeline_label);
            const char* set1_label = dvz_drp2_stream_label(
                stream, command->u.create_render_pipeline.bind_group_layout_ids[1]);
            ANN(set1_label);
            found_bitmap_pipeline = true;
            AT(command->u.create_render_pipeline.attr_count == 6);
            AT(command->u.create_render_pipeline.topology == DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST);
            AT(command->u.create_render_pipeline.attr_locations[5] == 6);
            AT(command->u.create_render_pipeline.attr_formats[5] ==
               DVZ_FORMAT_R32G32B32A32_SFLOAT);
            AT(command->u.create_render_pipeline.bind_group_layout_count == 2);
            if (strstr(pipeline_label, "_pipe_marker_bitmapg") == pipeline_label)
            {
                AT(strcmp(set1_label, "_bgl_img") == 0);
                found_bitmap_image_layout = true;
            }
            else if (strstr(pipeline_label, "_pipe_marker_distanceg") == pipeline_label)
            {
                AT(strcmp(set1_label, "_bgl_glyph") == 0);
                found_distance_glyph_layout = true;
            }
        }
        else if (command->type == DVZ_DRP2_COMMAND_SET_BIND_GROUP)
        {
            found_material_bg = found_material_bg || command->u.set_bind_group.slot == 1;
            found_set1_bg = found_set1_bg || command->u.set_bind_group.slot == 1;
        }
        else if (command->type == DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER)
        {
            set_vertex_buffer_count++;
            AT(command->u.set_vertex_buffer.buffer_id != 0);
            AT(command->u.set_vertex_buffer.offset == 0);
            AT(command->u.set_vertex_buffer.slot < 6);
            found_vertex_slots[command->u.set_vertex_buffer.slot] = true;
        }
        else if (command->type == DVZ_DRP2_COMMAND_DRAW)
        {
            found_draw = found_draw || command->u.draw.vertex_count == 5;
            found_bitmap_draw = found_bitmap_draw || command->u.draw.vertex_count == 2;
            AT(command->u.draw.instance_count == 1);
        }
        else if (command->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE)
        {
            if (command->u.create_texture.format == DVZ_FORMAT_R8G8B8A8_UNORM &&
                command->u.create_texture.width == 4 && command->u.create_texture.height == 4 &&
                command->u.create_texture.depth == 1)
            {
                if (command->u.create_texture.color_role == DVZ_DRP2_COLOR_ROLE_SRGB_COLOR)
                    rgba_srgb_texture_count++;
                if (command->u.create_texture.color_role == DVZ_DRP2_COLOR_ROLE_DATA)
                    rgba_data_texture_count++;
            }
            if (command->u.create_texture.format == DVZ_FORMAT_R8_UNORM &&
                command->u.create_texture.width == 4 && command->u.create_texture.height == 4 &&
                command->u.create_texture.depth == 1 &&
                command->u.create_texture.color_role == DVZ_DRP2_COLOR_ROLE_DATA)
            {
                r8_data_texture_count++;
            }
            found_texture =
                found_texture || (command->u.create_texture.format == DVZ_FORMAT_R8G8B8A8_UNORM &&
                                  command->u.create_texture.width == 4 &&
                                  command->u.create_texture.height == 4 &&
                                  command->u.create_texture.depth == 1);
            found_sdf_texture =
                found_sdf_texture || (command->u.create_texture.format == DVZ_FORMAT_R8_UNORM &&
                                      command->u.create_texture.width == 4 &&
                                      command->u.create_texture.height == 4 &&
                                      command->u.create_texture.depth == 1);
            found_msdf_texture =
                found_msdf_texture || (command->u.create_texture.format == DVZ_FORMAT_R8G8B8A8_UNORM &&
                                       command->u.create_texture.width == 4 &&
                                       command->u.create_texture.height == 4 &&
                                       command->u.create_texture.depth == 1);
        }
        else if (command->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE)
        {
            found_texture_upload =
                found_texture_upload || (command->u.write_texture.width == 4 &&
                                         command->u.write_texture.height == 4 &&
                                         command->u.write_texture.depth == 1 &&
                                         command->u.write_texture.bytes_per_row == 4 * 4);
            found_sdf_texture_upload =
                found_sdf_texture_upload || (command->u.write_texture.width == 4 &&
                                             command->u.write_texture.height == 4 &&
                                             command->u.write_texture.depth == 1 &&
                                             command->u.write_texture.bytes_per_row == 4);
            found_msdf_texture_upload =
                found_msdf_texture_upload || (command->u.write_texture.width == 4 &&
                                              command->u.write_texture.height == 4 &&
                                              command->u.write_texture.depth == 1 &&
                                              command->u.write_texture.bytes_per_row == 4 * 4);
        }
    }
    AT(found_pipeline);
    AT(found_bitmap_pipeline);
    AT(found_bitmap_image_layout);
    AT(found_distance_glyph_layout);
    AT(found_material_bg);
    AT(found_set1_bg);
    AT(set_vertex_buffer_count == 23);
    for (uint32_t slot = 0; slot < 6; slot++)
        AT(found_vertex_slots[slot]);
    AT(found_draw);
    AT(found_bitmap_draw);
    AT(found_texture);
    AT(found_texture_upload);
    AT(found_sdf_texture);
    AT(found_sdf_texture_upload);
    AT(found_msdf_texture);
    AT(found_msdf_texture_upload);
    AT(rgba_srgb_texture_count >= 1);
    AT(rgba_data_texture_count >= 1);
    AT(r8_data_texture_count >= 1);

#if defined(DVZ_HAS_MSDF_SVG) && DVZ_HAS_MSDF_SVG
    const char* star_path =
        "M50,10 L61.8,35.5 L90,42 L69,61 L75,90 L50,75 L25,90 L31,61 L10,42 L38.2,35.5 Z";
    const DvzSymbolId svg_symbol =
        dvz_symbol_svg_path(symbol_set, "svg-star", star_path, 32, 32, &symbol_desc);
    AT(svg_symbol != DVZ_SYMBOL_ID_INVALID);
    AT(symbol_set->source_count == 4);
    AT(symbol_set->sources[symbol_set->source_count - 1].kind == DVZ_SYMBOL_SOURCE_MSDF);
    AT(symbol_set->sources[symbol_set->source_count - 1].width == 32);
    AT(symbol_set->sources[symbol_set->source_count - 1].height == 32);
    AT(symbol_set->sources[symbol_set->source_count - 1].channels == 3);
    AC(symbol_set->sources[symbol_set->source_count - 1].distance_range_px, 4.0, 1e-6);
#endif

    _test_scene_stream_destroy(stream);
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
int test_scene_pixel_emit_glsl_native_square_points(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    DvzVisual* visual = dvz_pixel(scene, 0);
    AT(visual != NULL);

    vec3 positions[2] = {{-0.25f, 0.0f, 0.0f}, {+0.25f, 0.0f, 0.0f}};
    DvzColor colors[2] = {{255, 0, 0, 255}, {0, 255, 0, 255}};
    float sizes[2] = {8.0f, 12.0f};
    AT(dvz_visual_set_data(visual, "position", positions, 2) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 2) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 2) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
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
            AT(command->u.create_render_pipeline.topology == DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST);
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

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}
