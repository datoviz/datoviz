/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene visual graph tests                                                                      */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <float.h>
#include <math.h>

#include "scene_graph_utils.h"
#include "_frame_plan_runtime_internal.h"
#include "core/figure_emit_internal.h"
#include "datoviz/geom.h"
#include "datoviz/vk/memory_interop.h"
#include "datoviz/vklite/sync.h"
#include "domain/polygon_internal.h"
#include "registry/registry.h"
#include "scene_emit/internal.h"
#include "visuals/bounds_internal.h"
#include "_visual_internal.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_scene_textured_mesh_emits_texture_pipeline(TstContext* suite, const TstCase* item);


/**
 * Assert one bounds object exactly enough for deterministic test inputs.
 *
 * @param bounds the bounds object
 * @param dims expected dimension count
 * @param min0 expected lower x coordinate
 * @param min1 expected lower y coordinate
 * @param min2 expected lower z coordinate
 * @param max0 expected upper x coordinate
 * @param max1 expected upper y coordinate
 * @param max2 expected upper z coordinate
 * @return 0 on success, 1 on assertion failure
 */
static int _bounds_expect(
    const DvzBounds* bounds, uint32_t dims, double min0, double min1, double min2, double max0,
    double max1, double max2)
{
    ANN(bounds);
    AT(bounds->valid);
    AT(bounds->dims == dims);
    AC(bounds->min[0], min0, 1e-6);
    AC(bounds->min[1], min1, 1e-6);
    AC(bounds->min[2], min2, 1e-6);
    AC(bounds->max[0], max0, 1e-6);
    AC(bounds->max[1], max1, 1e-6);
    AC(bounds->max[2], max2, 1e-6);
    return 0;
}



/**
 * Apply a Phong material while preserving the current visual alpha mode.
 *
 * @param visual the visual
 * @param light_direction material light direction
 * @param ambient ambient coefficient
 * @param diffuse diffuse coefficient
 * @param specular specular coefficient
 * @param shininess shininess exponent
 * @return 0 on success, -1 on error
 */
static int _test_set_phong_material(
    DvzVisual* visual, const float light_direction[3], float ambient, float diffuse,
    float specular, float shininess)
{
    ANN(visual);
    ANN(light_direction);
    DvzMaterialDesc material = dvz_phong_material_desc();
    material.alpha_mode = dvz_visual_alpha_mode(visual);
    material.light_direction[0] = light_direction[0];
    material.light_direction[1] = light_direction[1];
    material.light_direction[2] = light_direction[2];
    material.phong.ambient = ambient;
    material.phong.diffuse = diffuse;
    material.phong.specular = specular;
    material.phong.shininess = shininess;
    return dvz_visual_set_material(visual, &material);
}



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
    DvzPanel* panel = dvz_panel(figure, desc);
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

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);

    /* Execute on GPU */
    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    dvz_drp2_stream_destroy(stream);
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
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
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

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    dvz_drp2_stream_destroy(stream);
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

    AT(dvz_sphere_mode(sphere, DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR) == 0);
    AT(_visual_family_state(sphere)->sphere_mode == DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR);
    AT(_visual_family_state(sphere)->material_params.depth_cue_extra[3] == (float)DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR);
    AT(_visual_family_state(sphere)->material_params_dirty);

    AT(_test_set_phong_material(
           sphere, (float[3]){0.0f, 0.0f, 1.0f}, 0.2f, 0.7f, 0.8f, 64.0f) == 0);
    AT(_visual_family_state(sphere)->sphere_mode == DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR);
    AT(_visual_family_state(sphere)->material_params.depth_cue_extra[3] == (float)DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR);
    AT(dvz_sphere_mode(sphere, DVZ_SPHERE_MODE_FAST_IMPOSTOR) == 0);
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
        VK_BLEND_FACTOR_SRC_ALPHA);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
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

    dvz_frame_plan_destroy(plan);
    dvz_drp2_stream_destroy(stream);
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
    AT(dvz_visual_set_data(visual, "stroke_width", widths, 2) == 0);

    DvzBounds bounds = {0};
    AT(dvz_visual_bounds(visual, &bounds) == 0);
    AT(_bounds_expect(&bounds, 2, -1.0, -1.0, 0.0, +1.0, +3.0, 0.0) == 0);

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
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
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
    AT(dvz_visual_set_data(visual, "stroke_width", stroke_widths, 2) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
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

    dvz_drp2_stream_destroy(stream);
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
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
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
    AT(dvz_visual_set_data(visual, "stroke_width", stroke_widths, 5) == 0);
    AT(dvz_vector_set_subpaths(visual, 2, subpaths) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
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
                AT(cmd->u.create_render_pipeline.binding_count == 7);
                AT(cmd->u.create_render_pipeline.attr_count == 7);
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
    AT(found_set_index);
    AT(found_draw_indexed);
    AT(set_vertex_buffer_count == 7);
    AT(_stream_write_buffer_range_count(stream, 0, sizeof(DvzSceneMaterialParams)) >= 1);

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
int test_scene_point_like_lowering_policy(TstContext* suite, const TstCase* item)
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
        DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
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
        DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
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
                   VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
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

        dvz_drp2_stream_destroy(stream);
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
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
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
int test_scene_point_style_emits_glsl_and_wgsl(TstContext* suite, const TstCase* item)
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
               .stroke_width = 3.0f,
               .aspect = DVZ_SHAPE_ASPECT_OUTLINE,
           }) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
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
    AT(_stream_has_render_pipeline_label(wgsl_stream, "_pipe_point_stylew_depth"));
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
int test_scene_marker_api_and_emit_glsl(TstContext* suite, const TstCase* item)
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

    vec3 positions[3] = {
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
           &(DvzMarkerStyle){DVZ_STRUCT_INIT_FIELDS(DvzMarkerStyle),
               .edge_color = {0, 0, 0, 255},
               .stroke_width = 2.0f,
               .aspect = DVZ_SHAPE_ASPECT_OUTLINE,
           }) == 0);
    tst_log_capture_begin(suite);
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_visual_set_data(visual, "shape", shapes, 2) == -1);
    AT(_captured_log_contains(suite, "item_count"));
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
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
    bool found_vertex_slots[5] = {false};
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
            AT(command->u.create_render_pipeline.topology == VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
            AT(command->u.create_render_pipeline.attr_locations[4] == 4);
            AT(command->u.create_render_pipeline.attr_formats[4] == VK_FORMAT_R32_UINT);
        }
        else if (command->type == DVZ_DRP2_COMMAND_SET_BIND_GROUP)
        {
            found_material_bg = found_material_bg || command->u.set_bind_group.slot == 1;
        }
        else if (command->type == DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER)
        {
            set_vertex_buffer_count++;
            AT(command->u.set_vertex_buffer.buffer_id != 0);
            AT(command->u.set_vertex_buffer.offset == 0);
            AT(command->u.set_vertex_buffer.slot < 5);
            found_vertex_slots[command->u.set_vertex_buffer.slot] = true;
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
    AT(set_vertex_buffer_count == 5);
    for (uint32_t slot = 0; slot < 5; slot++)
        AT(found_vertex_slots[slot]);
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
int test_scene_pixel_emit_glsl_native_square_points(TstContext* suite, const TstCase* item)
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



static int _scene_primitive_emit_executes(
    TstContext* suite, DvzPrimitiveTopology topology, uint32_t vertex_count)
{
    ANN(suite);

    DvzGpuCtx* ctx = NULL;
    DvzDrp2Runtime* runtime = _scene_graph_fixture_runtime(suite, &ctx);
    if (runtime == NULL)
        return 0;
    ANN(ctx);

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

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    dvz_free(positions);
    dvz_free(colors);
    return 0;
}


static int _scene_path_emit_executes(TstContext* suite, uint32_t vertex_count)
{
    ANN(suite);

    DvzGpuCtx* ctx = NULL;
    DvzDrp2Runtime* runtime = _scene_graph_fixture_runtime(suite, &ctx);
    if (runtime == NULL)
        return 0;
    ANN(ctx);

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

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    dvz_free(positions);
    dvz_free(colors);
    return 0;
}


static int _scene_mesh_emit_executes(TstContext* suite)
{
    ANN(suite);

    DvzGpuCtx* ctx = NULL;
    DvzDrp2Runtime* runtime = _scene_graph_fixture_runtime(suite, &ctx);
    if (runtime == NULL)
        return 0;
    ANN(ctx);

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    DvzVisual* visual = dvz_mesh(scene, 0);
    AT(visual != NULL);

    vec3 positions[4] = {
        {-0.8f, -0.8f, 0.0f}, {-0.8f, 0.8f, 0.0f},
        {0.8f, -0.8f, 0.0f},  {0.8f, 0.8f, 0.0f},
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

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_primitive_triangle_list_glsl_executes(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    return _scene_primitive_emit_executes(suite, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 3);
}


int test_scene_point_emit_wgsl_instanced_quads(TstContext* suite, const TstCase* item)
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


int test_scene_pixel_emit_wgsl_instanced_quads(TstContext* suite, const TstCase* item)
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


int test_scene_primitive_line_strip_glsl_executes(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    return _scene_primitive_emit_executes(suite, DVZ_PRIMITIVE_TOPOLOGY_LINE_STRIP, 4);
}


int test_scene_primitive_triangle_list_emit_wgsl(TstContext* suite, const TstCase* item)
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

    vec3 positions[3] = {
        {-0.6f, -0.5f, 0.0f},
        { 0.6f, -0.5f, 0.0f},
        { 0.0f,  0.6f, 0.0f},
    };
    DvzColor colors[3] = {{255, 0, 0, 255}, {0, 180, 255, 255}, {255, 255, 255, 255}};

    AT(dvz_visual_set_data(visual, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 3) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
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


int test_scene_mesh_indexed_default_color_emits_draw_indexed(TstContext* suite, const TstCase* item)
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

    vec3 positions[4] = {
        {-0.8f, -0.8f, 0.0f}, {-0.8f, 0.8f, 0.0f},
        {0.8f, -0.8f, 0.0f},  {0.8f, 0.8f, 0.0f},
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
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)));

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "normal", normals, 4) == 0);
    AT(dvz_visual_set_buffer(visual, "index", index_buffer));
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);
    AT(_visual_family_state(visual)->mesh_default_color);
    bool found_color_attr = false;
    for (uint32_t i = 0; i < visual->attr_count; i++)
        found_color_attr = found_color_attr || strcmp(visual->attrs[i].name, "color") == 0;
    AT(found_color_attr);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    bool found_set_index = false;
    bool found_draw_indexed = false;
    uint32_t set_index_order = UINT32_MAX;
    uint32_t draw_indexed_order = UINT32_MAX;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd->type == DVZ_DRP2_COMMAND_SET_INDEX_BUFFER)
        {
            found_set_index = strcmp(cmd->u.set_index_buffer.index_format, "uint32") == 0;
            if (found_set_index)
                set_index_order = i;
        }
        if (cmd->type == DVZ_DRP2_COMMAND_DRAW_INDEXED)
        {
            found_draw_indexed = cmd->u.draw_indexed.index_count == 6;
            if (found_draw_indexed)
                draw_indexed_order = i;
        }
    }
    AT(found_set_index);
    AT(found_draw_indexed);
    AT(set_index_order < draw_indexed_order);
    AT(_stream_set_vertex_buffer_count(stream) == 3);
    AT(_stream_write_buffer_range_count(stream, 0, sizeof(DvzSceneMaterialParams)) == 1);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_mesh_instance_transform_emits_instanced_draw(TstContext* suite, const TstCase* item)
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

    vec3 positions[4] = {
        {-0.25f, -0.25f, 0.0f}, {-0.25f, 0.25f, 0.0f},
        {0.25f, -0.25f, 0.0f},  {0.25f, 0.25f, 0.0f},
    };
    DvzIndex indices[6] = {0, 1, 2, 2, 1, 3};
    float transforms[2][16] = {
        {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, -0.4f, 0, 0, 1},
        {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, +0.4f, 0, 0, 1},
    };

    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)));

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "instance_transform", transforms, 2) == 0);
    AT(dvz_visual_set_buffer(visual, "index", index_buffer));
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
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


int test_scene_mesh_emits_depth_attachment(TstContext* suite, const TstCase* item)
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

    vec3 positions[4] = {
        {-0.8f, -0.8f, 0.1f}, {-0.8f, 0.8f, 0.1f},
        {0.8f, -0.8f, 0.1f},  {0.8f, 0.8f, 0.1f},
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
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)));

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "normal", normals, 4) == 0);
    AT(dvz_visual_set_buffer(visual, "index", index_buffer));
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
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


/**
 * Verify textured mesh lowering emits a texture-backed mesh pipeline and draw.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_textured_mesh_emits_texture_pipeline(TstContext* suite, const TstCase* item)
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

    vec3 positions[4] = {
        {-0.8f, -0.8f, 0.0f}, {-0.8f, 0.8f, 0.0f},
        {0.8f, -0.8f, 0.0f},  {0.8f, 0.8f, 0.0f},
    };
    DvzColor colors[4] = {
        {255, 255, 255, 255}, {255, 255, 255, 255},
        {255, 255, 255, 255}, {255, 255, 255, 255},
    };
    vec3 normals[4] = {
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f}, {0.0f, 1.0f},
        {1.0f, 0.0f}, {1.0f, 1.0f},
    };
    DvzIndex indices[6] = {0, 1, 2, 2, 1, 3};
    static const uint8_t pixels[2 * 2 * 4] = {
        255, 0,   0,   255,
        0,   255, 0,   255,
        0,   0,   255, 255,
        255, 255, 255, 255,
    };

    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)));

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_RGBA8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_COLOR,
                   .width = 2,
                   .height = 2,
                   .depth = 1,
               });
    ANN(field);
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView), .data = pixels, .bytes_per_row = 2 * 4, .rows_per_image = 2}));

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 4) == 0);
    AT(dvz_visual_set_data(visual, "normal", normals, 4) == 0);
    AT(dvz_visual_set_data(visual, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_buffer(visual, "index", index_buffer));
    AT(dvz_visual_set_field(visual, "texture", field));
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    bool found_texture = false;
    bool found_upload = false;
    bool found_image_bind_group = false;
    bool found_material_upload = false;
    bool found_draw_indexed = false;
    bool found_depth_pipeline = false;
    uint64_t texture_id = 0;
    uint64_t material_id = 0;

    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        ANN(cmd);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE)
        {
            if (cmd->u.create_texture.format == VK_FORMAT_R8G8B8A8_UNORM &&
                cmd->u.create_texture.width == 2 && cmd->u.create_texture.height == 2 &&
                cmd->u.create_texture.depth == 1)
            {
                found_texture = true;
                texture_id = cmd->u.create_texture.id;
            }
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE)
        {
            found_upload = found_upload || (cmd->u.write_texture.width == 2 &&
                                            cmd->u.write_texture.height == 2 &&
                                            cmd->u.write_texture.depth == 1 &&
                                            cmd->u.write_texture.bytes_per_row == 2 * 4);
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_WRITE_BUFFER)
        {
            if (cmd->u.write_buffer.size == sizeof(DvzSceneMaterialParams))
            {
                found_material_upload = true;
                material_id = cmd->u.write_buffer.buffer_id;
            }
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_CREATE_BIND_GROUP && texture_id != 0)
        {
            bool has_material = false;
            bool has_texture = false;
            bool has_sampler = false;
            for (uint32_t j = 0; j < cmd->u.create_bind_group.entry_count; j++)
            {
                const DvzDrp2BindGroupEntry* entry = &cmd->u.create_bind_group.entries[j];
                has_material =
                    has_material ||
                    (entry->binding == 0 &&
                     entry->binding_type == DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER &&
                     entry->resource_id == material_id);
                has_texture = has_texture ||
                              (entry->binding == 1 &&
                               entry->binding_type == DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE &&
                               entry->resource_id == texture_id);
                has_sampler = has_sampler ||
                              (entry->binding == 2 &&
                               entry->binding_type == DVZ_DRP2_BINDING_TYPE_SAMPLER);
            }
            found_image_bind_group =
                found_image_bind_group || (has_material && has_texture && has_sampler);
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.create_render_pipeline.id);
            found_depth_pipeline =
                found_depth_pipeline ||
                (label != NULL && strstr(label, "_pipe_mesh_textured_t") != NULL &&
                 cmd->u.create_render_pipeline.has_depth_attachment &&
                 cmd->u.create_render_pipeline.depth_write_enabled &&
                 cmd->u.create_render_pipeline.binding_count == 4 &&
                 cmd->u.create_render_pipeline.attr_count == 4);
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_DRAW_INDEXED)
        {
            found_draw_indexed = found_draw_indexed || cmd->u.draw_indexed.index_count == 6;
        }
    }

    AT(found_texture);
    AT(found_upload);
    AT(found_material_upload);
    AT(found_image_bind_group);
    AT(found_depth_pipeline);
    AT(found_draw_indexed);
    AT(_stream_set_vertex_buffer_count(stream) == 4);
    AT(_stream_write_buffer_range_count(stream, 0, sizeof(DvzSceneMaterialParams)) == 1);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_indexed_primitive_emits_draw_indexed(TstContext* suite, const TstCase* item)
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
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)));

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 4) == 0);
    AT(dvz_visual_set_data(visual, "normal", normals, 4) == 0);
    AT(dvz_visual_set_buffer(visual, "index", index_buffer));
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
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


int test_scene_shared_index_buffer_emits_one_upload(TstContext* suite, const TstCase* item)
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

    vec3 positions0[4] = {
        {-0.8f, -0.8f, 0.0f}, {-0.8f, 0.0f, 0.0f},
        {-0.2f, -0.8f, 0.0f}, {-0.2f, 0.0f, 0.0f},
    };
    vec3 positions1[4] = {
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
        scene, &(DvzSceneBufferDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
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

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
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


int test_scene_mesh_glsl_executes(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    return _scene_mesh_emit_executes(suite);
}


int test_scene_path_glsl_executes(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    return _scene_path_emit_executes(suite, 4);
}


/**
 * Verify line-width path visuals lower to the scene.path stroke pipeline.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_path_line_width_emit_glsl(TstContext* suite, const TstCase* item)
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
    AT(dvz_visual_set_data(visual, "stroke_width", stroke_widths, 5) == 0);
    AT(dvz_path_set_subpaths(visual, 2, subpaths) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
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
            if (label != NULL && strstr(label, "_pipe_pathg") == label)
            {
                found_pipeline = true;
                AT(cmd->u.create_render_pipeline.topology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
                AT(cmd->u.create_render_pipeline.binding_count == 7);
                AT(cmd->u.create_render_pipeline.attr_count == 7);
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
    AT(set_vertex_buffer_count == 7);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify repeated first/last path points are lowered as closed joins, not open caps.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_path_repeated_endpoint_closes_subpath(TstContext* suite, const TstCase* item)
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

    vec3 positions[5] = {
        {-0.50f, -0.50f, 0.0f},
        {+0.50f, -0.50f, 0.0f},
        {+0.50f, +0.50f, 0.0f},
        {-0.50f, +0.50f, 0.0f},
        {-0.50f, -0.50f, 0.0f},
    };
    DvzColor colors[5] = {
        {255, 255, 255, 255},
        {255, 255, 255, 255},
        {255, 255, 255, 255},
        {255, 255, 255, 255},
        {255, 255, 255, 255},
    };
    float stroke_widths[5] = {8.0f, 8.0f, 8.0f, 8.0f, 8.0f};
    uint32_t subpath = 5;

    AT(dvz_visual_set_data(visual, "position", positions, 5) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 5) == 0);
    AT(dvz_visual_set_data(visual, "stroke_width", stroke_widths, 5) == 0);
    AT(dvz_path_set_subpaths(visual, 1, &subpath) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzFramePlan* plan = dvz_frame_plan("figure.path.closed", 0);
    AT(plan != NULL);
    _scene_emit_visual_uploads(figure, plan, NULL);

    const uint32_t has_prev = 0x04u;
    const uint32_t has_next = 0x08u;
    const uint32_t subpath_start = 0x10u;
    const uint32_t subpath_end = 0x20u;
    const DvzPathGpuCache* cache = &_visual_family_state(visual)->path.gpu;
    AT(cache->segment_count == 4);
    AT(cache->vertex_count == 16);

    const uint32_t first_start = cache->path_flags[0];
    AT((first_start & has_prev) != 0);
    AT((first_start & has_next) != 0);
    AT((first_start & subpath_start) == 0);
    AC(cache->position_prev[0], positions[3][0], 1e-6);
    AC(cache->position_prev[1], positions[3][1], 1e-6);

    const uint32_t last_end_index = 4 * 3 + 2;
    const uint32_t last_end = cache->path_flags[last_end_index];
    AT((last_end & has_prev) != 0);
    AT((last_end & has_next) != 0);
    AT((last_end & subpath_end) == 0);
    AC(cache->position_next[3 * last_end_index + 0], positions[1][0], 1e-6);
    AC(cache->position_next[3 * last_end_index + 1], positions[1][1], 1e-6);

    dvz_frame_plan_destroy(plan);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_image_glsl_executes(TstContext* suite, const TstCase* item)
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
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    AT(panel != NULL);
    DvzVisual* visual = dvz_image(scene, 0);
    AT(visual != NULL);

    /* TRIANGLE_STRIP: TL, BL, TR, BR */
    vec3 positions[4] = {
        {-0.5f, -0.5f, 0.0f},
        {-0.5f,  0.5f, 0.0f},
        { 0.5f, -0.5f, 0.0f},
        { 0.5f,  0.5f, 0.0f},
    };
    vec2 texcoords[4] = {
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

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_json(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene*  scene  = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    DvzPanel*  panel  = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    DvzVisual* visual = dvz_point(scene, 0);

    vec3 positions[2] = {{-0.5f, -0.5f, 0.0f}, {0.5f, 0.5f, 0.0f}};
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


int test_scene_json_includes_field_dirty_metadata(TstContext* suite, const TstCase* item)
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

    vec3 positions[4] = {
        {-0.5f, -0.5f, 0.0f}, {-0.5f, 0.5f, 0.0f},
        { 0.5f, -0.5f, 0.0f}, { 0.5f, 0.5f, 0.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };
    AT(dvz_visual_set_data(image, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(image, "texcoords", texcoords, 4) == 0);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_RGBA8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_COLOR,
                   .width = 4,
                   .height = 4,
                   .depth = 1,
               });
    ANN(field);
    uint8_t base[4 * 4 * 4] = {0};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView), .data = base, .bytes_per_row = 4 * 4, .rows_per_image = 4}));
    AT(dvz_visual_set_field(image, "field", field));
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = dvz_figure_emit(figure, &caps, &report);
    ANN(stream);
    dvz_drp2_stream_destroy(stream);

    uint8_t patch[2 * 4] = {1, 2, 3, 4, 5, 6, 7, 8};
    AT(dvz_sampled_field_update_region(
        field, (DvzFieldRegion){.x = 1, .y = 2, .z = 0, .width = 2, .height = 1, .depth = 1},
        &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView), .data = patch, .bytes_per_row = 2 * 4, .rows_per_image = 1}));

    char* json = dvz_scene_json(scene);
    ANN(json);
    AT(strstr(json, "\"dirty\":{\"pending\":true,\"full\":false,\"region\":{\"x\":1,\"y\":2,\"z\":0,\"width\":2,\"height\":1,\"depth\":1}}") != NULL);
    AT(strstr(json, "\"field_state\":{\"pending\":true,\"full\":false,\"region\":{\"x\":1,\"y\":2,\"z\":0,\"width\":2,\"height\":1,\"depth\":1}}") != NULL);
    dvz_scene_json_destroy(json);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_json_includes_buffer_binding_metadata(TstContext* suite, const TstCase* item)
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

    vec3 positions[3] = {
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
        scene, &(DvzSceneBufferDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc), .usage = DVZ_SCENE_BUFFER_USAGE_INDEX, .stride = sizeof(DvzIndex)});
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





int test_scene_rejects_cross_scene_visual(TstContext* suite, const TstCase* item)
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


int test_scene_rejects_unsupported_point_attribute(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);
    DvzVisual* marker = dvz_marker(scene, 0);
    ANN(marker);
    DvzVisual* vector = dvz_vector(scene, 0);
    ANN(vector);
    DvzVisual* mesh = dvz_mesh(scene, 0);
    ANN(mesh);

    float opacity[2] = {0.25f, 0.75f};
    tst_log_capture_begin(suite);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_data(visual, "opacity", opacity, 2) == -1);
    AT(_captured_log_contains(suite, "unsupported point visual attribute 'opacity'"));

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_visual_attr_source_and_mutability_metadata(TstContext* suite, const TstCase* item)
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


/**
 * Verify public read-only views over retained dense visual data.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_visual_data_view(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);

    vec3 positions[2] = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
    float sizes[2] = {6.0f, 12.0f};

    AT(dvz_visual_set_attr_mutability(
           visual, "position", DVZ_VISUAL_ATTR_MUTABILITY_STREAMING) == 0);
    AT(dvz_visual_set_data(visual, "position", positions, 2) == 0);
    AT(dvz_visual_set_data(visual, "diameter", sizes, 2) == 0);

    DvzVisualDataView view = {0};
    AT(dvz_visual_data(visual, "position", &view) == 0);
    AT(view.data != NULL);
    AT(view.item_count == 2);
    AT(view.item_size == 3 * sizeof(float));
    AT(view.source == DVZ_VISUAL_ATTR_SOURCE_PER_ITEM);
    AT(view.mutability == DVZ_VISUAL_ATTR_MUTABILITY_STREAMING);
    AT(view.version > 0);
    const float* view_positions = view.data;
    AT(view_positions[3] == 1.0f);

    DvzVisualDataView alias_view = {0};
    AT(dvz_visual_data(visual, "diameter", &alias_view) == 0);
    AT(alias_view.data != NULL);
    AT(alias_view.item_count == 2);
    AT(alias_view.item_size == sizeof(float));
    const float* view_sizes = alias_view.data;
    AT(view_sizes[0] == 6.0f);
    AT(view_sizes[1] == 12.0f);

    AT(dvz_visual_set_attr_source(visual, "color", DVZ_VISUAL_ATTR_SOURCE_CONSTANT) == 0);
    DvzVisualDataView missing_view = {0};
    AT(dvz_visual_data(visual, "color", &missing_view) == -1);
    AT(missing_view.data == NULL);
    AT(dvz_visual_data(visual, "texcoords", &missing_view) == -1);
    AT(dvz_visual_data(NULL, "position", &missing_view) == -1);
    AT(dvz_visual_data(visual, NULL, &missing_view) == -1);
    AT(dvz_visual_data(visual, "position", NULL) == -1);

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Verify scalar color attribute format metadata and retained dense views.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_visual_scalar_color_attr_format(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* point = dvz_point(scene, 0);
    ANN(point);
    DvzVisual* mesh = dvz_mesh(scene, 0);
    ANN(mesh);

    AT(dvz_visual_attr_format(point, "color") == DVZ_VISUAL_ATTR_FORMAT_RGBA_U8);
    AT(dvz_visual_attr_format(point, "position") == DVZ_VISUAL_ATTR_FORMAT_DEFAULT);
    AT(dvz_visual_set_attr_format(
           point, "color", DVZ_VISUAL_ATTR_FORMAT_SCALAR_F32) == 0);
    AT(dvz_visual_attr_format(point, "color") == DVZ_VISUAL_ATTR_FORMAT_SCALAR_F32);

    vec3 positions[2] = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
    float scalars[2] = {0.25f, 0.75f};
    float sizes[2] = {6.0f, 12.0f};
    AT(dvz_visual_set_data(point, "position", positions, 2) == 0);
    AT(dvz_visual_set_data(point, "color", scalars, 2) == 0);
    AT(dvz_visual_set_data(point, "diameter", sizes, 2) == 0);

    DvzVisualDataView view = {0};
    AT(dvz_visual_data(point, "color", &view) == 0);
    AT(view.item_count == 2);
    AT(view.item_size == sizeof(float));
    const float* retained = view.data;
    AT(retained[0] == 0.25f);
    AT(retained[1] == 0.75f);

    tst_log_capture_begin(suite);
    AT_EXPECTED_ERROR_STRICT(
        suite,
        dvz_visual_set_attr_format(point, "color", DVZ_VISUAL_ATTR_FORMAT_RGBA_U8) == -1);
    AT(_captured_log_contains(suite, "format cannot change after payload attachment"));

    tst_log_capture_begin(suite);
    AT_EXPECTED_ERROR_STRICT(
        suite,
        dvz_visual_set_attr_format(mesh, "color", DVZ_VISUAL_ATTR_FORMAT_SCALAR_F32) == -1);
    AT(_captured_log_contains(suite, "does not support format"));

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Verify scalar point/pixel color data emits the RGBA buffer expected by current pipelines.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_scalar_color_emits_rgba_upload(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* pixel = dvz_pixel(scene, 0);
    ANN(pixel);

    const uint32_t N = 4;
    vec3 positions[4] = {
        {-0.5f, -0.5f, 0.0f},
        {0.5f, -0.5f, 0.0f},
        {-0.5f, 0.5f, 0.0f},
        {0.5f, 0.5f, 0.0f},
    };
    float values[4] = {0.0f, 0.25f, 0.75f, 1.0f};
    float sizes[4] = {4.0f, 4.0f, 4.0f, 4.0f};

    AT(dvz_visual_set_attr_format(pixel, "color", DVZ_VISUAL_ATTR_FORMAT_SCALAR_F32) == 0);
    AT(dvz_visual_set_data(pixel, "position", positions, N) == 0);
    AT(dvz_visual_set_data(pixel, "color", values, N) == 0);
    AT(dvz_visual_set_data(pixel, "pixel_size", sizes, N) == 0);

    DvzVisualDataView view = {0};
    AT(dvz_visual_data(pixel, "color", &view) == 0);
    AT(view.item_size == sizeof(float));

    DvzColormap* colormap = dvz_colormap_builtin(scene, DVZ_BUILTIN_COLORMAP_GRAY);
    ANN(colormap);
    DvzScale* scale =
        dvz_scale(scene, &(DvzScaleDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleDesc),
                            .kind = DVZ_SCALE_CONTINUOUS});
    ANN(scale);
    dvz_scale_set_domain(scale, 0.0, 1.0);
    dvz_scale_set_colormap(scale, colormap);
    AT(dvz_visual_set_scale(pixel, "color", scale) == 0);
    AT(dvz_panel_add_visual(panel, pixel, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    bool found_color_upload = false;
    DvzColor expected_first = {0};
    DvzColor expected_last = {0};
    AT(dvz_colormap_sample(colormap, 0.0, &expected_first));
    AT(dvz_colormap_sample(colormap, 1.0, &expected_last));
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd == NULL || cmd->type != DVZ_DRP2_COMMAND_WRITE_BUFFER)
            continue;
        const char* label = dvz_drp2_stream_label(stream, cmd->u.write_buffer.buffer_id);
        if (label == NULL || strstr(label, "color") == NULL)
            continue;
        found_color_upload = true;
        AT(cmd->u.write_buffer.offset == 0);
        AT(cmd->u.write_buffer.size == N * sizeof(DvzColor));
        ANN(cmd->u.write_buffer.data_raw);
        const DvzColor* uploaded = (const DvzColor*)cmd->u.write_buffer.data_raw;
        AT(uploaded[0].r == expected_first.r);
        AT(uploaded[0].g == expected_first.g);
        AT(uploaded[0].b == expected_first.b);
        AT(uploaded[0].a == expected_first.a);
        AT(uploaded[N - 1].r == expected_last.r);
        AT(uploaded[N - 1].g == expected_last.g);
        AT(uploaded[N - 1].b == expected_last.b);
        AT(uploaded[N - 1].a == expected_last.a);
        break;
    }
    AT(found_color_upload);
    dvz_drp2_stream_destroy(stream);

    int attr_idx = _attr_index(pixel, "color");
    AT(attr_idx >= 0);
    AT(pixel->attrs[attr_idx].dirty_item_count == 0);
    dvz_scale_set_domain(scale, 0.0, 2.0);
    AT(pixel->attrs[attr_idx].dirty_first_item == 0);
    AT(pixel->attrs[attr_idx].dirty_item_count == N);

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Verify retained visual-space bounds for point data and range mutations.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_visual_bounds_point_and_range_update(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);

    DvzBounds bounds = {0};
    AT(dvz_visual_bounds(visual, &bounds) == -1);
    AT(!bounds.valid);

    vec3 positions[3] = {
        {-2.0f, +1.0f, 0.0f},
        {+4.0f, -3.0f, 2.0f},
        {+1.0f, +5.0f, -1.0f},
    };
    AT(dvz_visual_set_data(visual, "position", positions, 3) == 0);
    AT(dvz_visual_bounds(visual, &bounds) == 0);
    AT(_bounds_expect(&bounds, 3, -2.0, -3.0, -1.0, +4.0, +5.0, +2.0) == 0);

    vec3 update[1] = {{+8.0f, +2.0f, +4.0f}};
    AT(dvz_visual_set_data_range(visual, "position", update, 1, 1) == 0);
    AT(dvz_visual_bounds(visual, &bounds) == 0);
    AT(_bounds_expect(&bounds, 3, -2.0, +1.0, -1.0, +8.0, +5.0, +4.0) == 0);

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Verify family-specific retained visual-space bounds.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_visual_bounds_family_reducers(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzBounds bounds = {0};

    DvzVisual* segment = dvz_segment(scene, 0);
    ANN(segment);
    vec3 starts[2] = {{-1.0f, -2.0f, 0.0f}, {+2.0f, +1.0f, +3.0f}};
    vec3 ends[2] = {{+4.0f, -1.0f, 1.0f}, {-3.0f, +5.0f, -2.0f}};
    AT(dvz_visual_set_data(segment, "position_start", starts, 2) == 0);
    AT(dvz_visual_set_data(segment, "position_end", ends, 2) == 0);
    AT(dvz_visual_bounds(segment, &bounds) == 0);
    AT(_bounds_expect(&bounds, 3, -3.0, -2.0, -2.0, +4.0, +5.0, +3.0) == 0);

    DvzVisual* sphere = dvz_sphere(scene, 0);
    ANN(sphere);
    vec3 sphere_pos[2] = {{0.0f, 0.0f, 0.0f}, {3.0f, -1.0f, 2.0f}};
    float radius[2] = {0.5f, 2.0f};
    AT(dvz_visual_set_data(sphere, "position", sphere_pos, 2) == 0);
    AT(dvz_visual_set_data(sphere, "radius", radius, 2) == 0);
    AT(dvz_visual_bounds(sphere, &bounds) == 0);
    AT(_bounds_expect(&bounds, 3, -0.5, -3.0, -0.5, +5.0, +1.0, +4.0) == 0);

    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);
    vec3 image_pos[2] = {{0.0f, 0.0f, 0.0f}, {4.0f, 2.0f, 1.0f}};
    vec2 image_extent[2] = {{2.0f, 4.0f}, {6.0f, 2.0f}};
    vec2 image_anchor[2] = {{0.0f, 0.0f}, {-1.0f, +1.0f}};
    AT(dvz_visual_set_data(image, "position", image_pos, 2) == 0);
    AT(dvz_visual_set_data(image, "extent", image_extent, 2) == 0);
    AT(dvz_visual_set_data(image, "anchor", image_anchor, 2) == 0);
    AT(dvz_visual_bounds(image, &bounds) == 0);
    AT(_bounds_expect(&bounds, 3, -1.0, -2.0, 0.0, +10.0, +2.0, +1.0) == 0);

    DvzVisual* volume = dvz_volume(scene, 0);
    ANN(volume);
    double volume_min[3] = {-2.0, -3.0, -4.0};
    double volume_max[3] = {+4.0, +5.0, +6.0};
    AT(dvz_volume_set_bounds(volume, volume_min, volume_max) == 0);
    AT(dvz_visual_bounds(volume, &bounds) == 0);
    AT(_bounds_expect(&bounds, 3, -2.0, -3.0, -4.0, +4.0, +5.0, +6.0) == 0);

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Verify mesh bounds include per-instance transforms.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_visual_bounds_mesh_instance_transform(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* mesh = dvz_mesh(scene, 0);
    ANN(mesh);

    vec3 positions[2] = {{0.0f, 0.0f, 0.0f}, {1.0f, 2.0f, 3.0f}};
    float transforms[2][16] = {
        {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f,
        },
        {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            10.0f, -1.0f, 2.0f, 1.0f,
        },
    };
    AT(dvz_visual_set_data(mesh, "position", positions, 2) == 0);
    AT(dvz_visual_set_data(mesh, "instance_transform", transforms, 2) == 0);

    DvzBounds bounds = {0};
    AT(dvz_visual_bounds(mesh, &bounds) == 0);
    AT(_bounds_expect(&bounds, 3, 0.0, -1.0, 0.0, 11.0, 2.0, 5.0) == 0);

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Verify panel-level visual and screen bounds.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_panel_visual_bounds_and_union(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 200, 100, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);

    DvzVisual* left = dvz_point(scene, 0);
    DvzVisual* right = dvz_point(scene, 0);
    ANN(left);
    ANN(right);
    vec3 left_pos[2] = {{-1.0f, -1.0f, 0.0f}, {0.0f, +1.0f, 0.0f}};
    vec3 right_pos[2] = {{+0.5f, -0.5f, 0.0f}, {+1.0f, +0.5f, 0.0f}};
    AT(dvz_visual_set_data(left, "position", left_pos, 2) == 0);
    AT(dvz_visual_set_data(right, "position", right_pos, 2) == 0);
    AT(dvz_panel_add_visual(panel, left, NULL) == 0);
    AT(dvz_panel_add_visual(panel, right, NULL) == 0);

    DvzBounds bounds = {0};
    AT(dvz_panel_visual_bounds(panel, left, DVZ_BOUNDS_SPACE_VISUAL, &bounds) == 0);
    AT(_bounds_expect(&bounds, 2, -1.0, -1.0, 0.0, 0.0, +1.0, 0.0) == 0);

    AT(dvz_panel_bounds(panel, DVZ_BOUNDS_SPACE_VISUAL, &bounds) == 0);
    AT(_bounds_expect(&bounds, 2, -1.0, -1.0, 0.0, +1.0, +1.0, 0.0) == 0);

    dvz_visual_set_visible(right, false);
    AT(dvz_panel_bounds(panel, DVZ_BOUNDS_SPACE_VISUAL, &bounds) == 0);
    AT(_bounds_expect(&bounds, 2, -1.0, -1.0, 0.0, 0.0, +1.0, 0.0) == 0);

    AT(dvz_panel_visual_bounds(panel, left, DVZ_BOUNDS_SPACE_SCREEN, &bounds) == 0);
    AT(_bounds_expect(&bounds, 2, 0.0, 0.0, 0.0, 100.0, 100.0, 0.0) == 0);

    DvzVisual* unattached = dvz_point(scene, 0);
    ANN(unattached);
    AT(dvz_panel_visual_bounds(panel, unattached, DVZ_BOUNDS_SPACE_VISUAL, &bounds) == -1);

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Verify the panel-owned bounds overlay generates front and occluded wireframe visuals.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_panel_bounds_overlay_visual(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 200, 100, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);

    DvzVisual* points = dvz_point(scene, 0);
    ANN(points);
    vec3 positions[2] = {{-1.0f, -1.0f, 0.0f}, {+1.0f, +1.0f, 0.0f}};
    AT(dvz_visual_set_data(points, "position", positions, 2) == 0);
    AT(dvz_panel_add_visual(panel, points, NULL) == 0);

    AT(!dvz_panel_bounds_visible(panel));
    AT(dvz_panel_set_bounds_visible(panel, true) == 0);
    AT(dvz_panel_bounds_visible(panel));
    _scene_prepare_bounds_visuals(figure);

    DvzVisual* overlay = panel->bounds_visual;
    DvzVisual* occluded_overlay = panel->bounds_occluded_visual;
    ANN(overlay);
    ANN(occluded_overlay);
    AT(overlay != occluded_overlay);
    AT(overlay->type == DVZ_VISUAL_TYPE_SEGMENT);
    AT(occluded_overlay->type == DVZ_VISUAL_TYPE_SEGMENT);
    AT(overlay->visible);
    AT(occluded_overlay->visible);
    AT(overlay->depth_test_enabled);
    AT(occluded_overlay->depth_test_enabled);
    AT(overlay->depth_compare_op == VK_COMPARE_OP_LESS_OR_EQUAL);
    AT(occluded_overlay->depth_compare_op == VK_COMPARE_OP_GREATER);
    int start_idx = _attr_index(overlay, "position_start");
    int end_idx = _attr_index(overlay, "position_end");
    int color_idx = _attr_index(overlay, "color");
    int width_idx = _attr_index(overlay, "line_width");
    AT(start_idx >= 0);
    AT(end_idx >= 0);
    AT(color_idx >= 0);
    AT(width_idx >= 0);
    AT(overlay->attrs[start_idx].item_count == 4);
    AT(overlay->attrs[end_idx].item_count == 4);
    AT(overlay->attrs[color_idx].item_count == 4);
    AT(overlay->attrs[width_idx].item_count == 4);

    start_idx = _attr_index(occluded_overlay, "position_start");
    end_idx = _attr_index(occluded_overlay, "position_end");
    color_idx = _attr_index(occluded_overlay, "color");
    width_idx = _attr_index(occluded_overlay, "line_width");
    AT(start_idx >= 0);
    AT(end_idx >= 0);
    AT(color_idx >= 0);
    AT(width_idx >= 0);
    AT(occluded_overlay->attrs[start_idx].item_count == 4);
    AT(occluded_overlay->attrs[end_idx].item_count == 4);
    AT(occluded_overlay->attrs[color_idx].item_count == 4);
    AT(occluded_overlay->attrs[width_idx].item_count == 4);
    DvzColor* hidden_colors = (DvzColor*)occluded_overlay->attrs[color_idx].data;
    ANN(hidden_colors);
    AT(hidden_colors[0].a == 120);

    DvzBounds bounds = {0};
    AT(dvz_panel_bounds(panel, DVZ_BOUNDS_SPACE_VISUAL, &bounds) == 0);
    AT(_bounds_expect(&bounds, 2, -1.0, -1.0, 0.0, +1.0, +1.0, 0.0) == 0);

    AT(dvz_panel_set_bounds_visible(panel, false) == 0);
    _scene_prepare_bounds_visuals(figure);
    AT(!overlay->visible);
    AT(!occluded_overlay->visible);

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Verify sphere overlays use conservative wire bounds while public bounds remain exact.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_panel_bounds_overlay_sphere_wire_padding(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 200, 200, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);

    DvzVisual* sphere = dvz_sphere(scene, 0);
    ANN(sphere);
    vec3 position[1] = {{0.0f, 0.0f, 0.0f}};
    DvzColor color[1] = {{255, 255, 255, 255}};
    float radius[1] = {0.25f};
    AT(dvz_visual_set_data(sphere, "position", position, 1) == 0);
    AT(dvz_visual_set_data(sphere, "color", color, 1) == 0);
    AT(dvz_visual_set_data(sphere, "radius", radius, 1) == 0);
    AT(dvz_panel_add_visual(panel, sphere, NULL) == 0);

    DvzBounds bounds = {0};
    AT(dvz_visual_bounds(sphere, &bounds) == 0);
    AT(_bounds_expect(&bounds, 3, -0.25, -0.25, -0.25, +0.25, +0.25, +0.25) == 0);

    AT(dvz_panel_set_bounds_visible(panel, true) == 0);
    _scene_prepare_bounds_visuals(figure);
    ANN(panel->bounds_visual);
    int start_idx = _attr_index(panel->bounds_visual, "position_start");
    int end_idx = _attr_index(panel->bounds_visual, "position_end");
    AT(start_idx >= 0);
    AT(end_idx >= 0);
    const float* starts = (const float*)panel->bounds_visual->attrs[start_idx].data;
    const float* ends = (const float*)panel->bounds_visual->attrs[end_idx].data;
    ANN(starts);
    ANN(ends);

    float min_x = +FLT_MAX;
    float max_x = -FLT_MAX;
    for (uint32_t i = 0; i < panel->bounds_visual->attrs[start_idx].item_count; i++)
    {
        min_x = fminf(min_x, starts[3 * i + 0]);
        min_x = fminf(min_x, ends[3 * i + 0]);
        max_x = fmaxf(max_x, starts[3 * i + 0]);
        max_x = fmaxf(max_x, ends[3 * i + 0]);
    }
    AT(min_x < -0.40f);
    AT(max_x > +0.40f);

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify bounds overlay pipeline keys survive runtime suffix composition.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_panel_bounds_overlay_emit_runtime(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 200, 100, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);

    DvzMsaaDesc msaa = dvz_msaa_desc();
    msaa.sample_count = 4;
    AT(dvz_panel_set_msaa(panel, &msaa));

    DvzVisual* spheres = dvz_sphere(scene, 0);
    ANN(spheres);
    vec3 positions[1] = {{0.0f, 0.0f, 0.0f}};
    DvzColor colors[1] = {{220, 120, 80, 255}};
    float radii[1] = {0.25f};
    AT(dvz_visual_set_data(spheres, "position", positions, 1) == 0);
    AT(dvz_visual_set_data(spheres, "color", colors, 1) == 0);
    AT(dvz_visual_set_data(spheres, "radius", radii, 1) == 0);
    AT(dvz_panel_add_visual(panel, spheres, NULL) == 0);
    AT(dvz_panel_set_bounds_visible(panel, true) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_glsl = true;

    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);
    AT(_stream_has_render_pipeline_label_part(
        stream, "_pipe_segmentg_coverage_blend_depth_msaa4"));
    AT(_stream_has_render_pipeline_label_part(
        stream, "_pipe_segmentg_coverage_blend_depth_gt_depth_msaa4"));

    bool found_front_pipeline = false;
    bool found_occluded_pipeline = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        ANN(command);
        if (command->type != DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
            continue;
        const char* label = dvz_drp2_stream_label(stream, command->u.create_render_pipeline.id);
        if (label == NULL)
            continue;
        if (strstr(label, "_pipe_segmentg_coverage_blend_depth_gt_depth_msaa4") != NULL)
        {
            found_occluded_pipeline =
                command->u.create_render_pipeline.has_depth_attachment &&
                !command->u.create_render_pipeline.depth_write_enabled &&
                command->u.create_render_pipeline.depth_compare_op == VK_COMPARE_OP_GREATER;
        }
        else if (strstr(label, "_pipe_segmentg_coverage_blend_depth_msaa4") != NULL)
        {
            found_front_pipeline =
                command->u.create_render_pipeline.has_depth_attachment &&
                command->u.create_render_pipeline.depth_write_enabled &&
                command->u.create_render_pipeline.depth_compare_op == VK_COMPARE_OP_LESS_OR_EQUAL;
        }
    }
    AT(found_front_pipeline);
    AT(found_occluded_pipeline);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_point_typed_data_upload(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);

    vec3 positions[2] = {{0.0f, 0.0f, 0.0f}, {1.0f, 2.0f, 0.0f}};
    DvzColor colors[2] = {{255, 0, 0, 255}, {0, 255, 0, 255}};
    float diameters[2] = {4.0f, 8.0f};
    uint32_t item_state[2] = {DVZ_ITEM_STATE_SELECTED, DVZ_ITEM_STATE_NONE};

    DvzVisualDataUpdate point_updates[] = {
        {.attr_name = "position", .data = positions, .item_count = 2},
        {.attr_name = "color", .data = colors, .item_count = 2},
        {.attr_name = "diameter", .data = diameters, .item_count = 2},
    };
    AT(dvz_visual_set_data_many(visual, point_updates, 3) == 0);
    AT(dvz_visual_set_data(visual, "item_state", item_state, 2) == 0);

    DvzVisualDataView view = {0};
    AT(dvz_visual_data(visual, "diameter", &view) == 0);
    const float* stored_diameters = view.data;
    AT(stored_diameters[1] == 8.0f);

    DvzVisualDataView state_view = {0};
    AT(dvz_visual_data(visual, "item_state", &state_view) == 0);
    const uint32_t* stored_state = state_view.data;
    AT(stored_state[0] == DVZ_ITEM_STATE_SELECTED);
    AT(stored_state[1] == DVZ_ITEM_STATE_NONE);

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_mesh_typed_data_upload(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* visual = dvz_mesh(scene, 0);
    ANN(visual);

    vec3 positions[3] = {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
    };
    DvzColor colors[3] = {{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}};
    vec3 normals[3] = {
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
    };
    vec2 texcoords[3] = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {0.0f, 1.0f},
    };
    mat4 transforms[1] = {{{1.0f, 0.0f, 0.0f, 0.0f},
                           {0.0f, 1.0f, 0.0f, 0.0f},
                           {0.0f, 0.0f, 1.0f, 0.0f},
                           {0.0f, 0.0f, 0.0f, 1.0f}}};

    DvzVisualDataUpdate mesh_updates[] = {
        {.attr_name = "position", .data = positions, .item_count = 3},
        {.attr_name = "color", .data = colors, .item_count = 3},
        {.attr_name = "normal", .data = normals, .item_count = 3},
    };
    AT(dvz_visual_set_data_many(visual, mesh_updates, 3) == 0);
    AT(dvz_visual_set_data(visual, "texcoords", texcoords, 3) == 0);
    AT(dvz_visual_set_data(visual, "instance_transform", transforms, 1) == 0);

    DvzVisualDataView normal_view = {0};
    AT(dvz_visual_data(visual, "normal", &normal_view) == 0);
    const float* stored_normals = normal_view.data;
    AT(stored_normals[2] == 1.0f);

    DvzVisualDataView transform_view = {0};
    AT(dvz_visual_data(visual, "instance_transform", &transform_view) == 0);
    AT(transform_view.item_count == 1);

    DvzVisualDataView texcoord_view = {0};
    AT(dvz_visual_data(visual, "texcoords", &texcoord_view) == 0);
    AT(texcoord_view.item_count == 3);
    AT(texcoord_view.item_size == 2 * sizeof(float));
    const float* stored_texcoords = texcoord_view.data;
    AT(stored_texcoords[3] == 0.0f);
    AT(stored_texcoords[5] == 1.0f);

    DvzVisual* default_color_mesh = dvz_mesh(scene, 0);
    ANN(default_color_mesh);
    AT(dvz_visual_set_data(default_color_mesh, "position", positions, 3) == 0);
    DvzVisualDataView color_view = {0};
    AT(dvz_visual_data(default_color_mesh, "color", &color_view) == 0);
    AT(color_view.item_count == 3);

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Check copied index data convenience upload and helper-owned buffer replacement.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_visual_index_data_upload(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* mesh = dvz_mesh(scene, 0);
    DvzVisual* primitive = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    DvzVisual* point = dvz_point(scene, 0);
    ANN(mesh);
    ANN(primitive);
    ANN(point);

    DvzIndex indices[6] = {0, 1, 2, 2, 1, 3};
    AT(dvz_visual_set_index_data(mesh, indices, 6) == 0);
    AT(_visual_family_state(mesh)->buffer != NULL);
    AT(_visual_family_state(mesh)->buffer->desc.usage & DVZ_SCENE_BUFFER_USAGE_INDEX);
    AT(_visual_family_state(mesh)->buffer->desc.stride == sizeof(DvzIndex));
    AT(_visual_family_state(mesh)->buffer->desc.byte_size == sizeof(indices));
    const DvzIndex* stored = _visual_family_state(mesh)->buffer->data;
    AT(stored[5] == 3);

    DvzSceneBuffer* old_buffer = _visual_family_state(mesh)->buffer;
    DvzIndex updated[3] = {0, 2, 1};
    AT(dvz_visual_set_index_data(mesh, updated, 3) == 0);
    AT(_visual_family_state(mesh)->buffer != old_buffer);
    AT(old_buffer->scene == NULL);
    AT(_visual_family_state(mesh)->buffer->desc.byte_size == sizeof(updated));
    stored = _visual_family_state(mesh)->buffer->data;
    AT(stored[1] == 2);

    AT(dvz_visual_set_index_data(primitive, indices, 3) == 0);
    AT(_visual_family_state(primitive)->buffer != NULL);
    AT(_visual_family_state(primitive)->buffer->desc.byte_size == 3 * sizeof(DvzIndex));

    AT(dvz_visual_set_index_data(point, indices, 3) == -1);
    AT(dvz_visual_set_index_data(mesh, NULL, 3) == -1);
    AT(dvz_visual_set_index_data(mesh, indices, 0) == -1);

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_mesh_geometry_upload(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* visual = dvz_mesh(scene, 0);
    ANN(visual);

    DvzColor colors[4] = {
        {255, 0, 0, 255},
        {0, 255, 0, 255},
        {0, 0, 255, 255},
        {255, 255, 255, 255},
    };
    DvzGeometrySurfaceGridDesc desc = {
        DVZ_STRUCT_INIT_FIELDS(DvzGeometrySurfaceGridDesc),
        .rows = 2,
        .cols = 2,
        .colors = colors,
    };
    DvzGeometry* geometry = dvz_geom_surface_grid(&desc);
    ANN(geometry);

    AT(dvz_mesh_set_geometry(visual, geometry) == 0);

    DvzVisualDataView position_view = {0};
    AT(dvz_visual_data(visual, "position", &position_view) == 0);
    AT(position_view.item_count == 4);
    AT(position_view.item_size == 3 * sizeof(float));
    const float* positions = position_view.data;
    AT(positions[0] == 0.0f);
    AT(positions[3] == 1.0f);

    DvzVisualDataView normal_view = {0};
    AT(dvz_visual_data(visual, "normal", &normal_view) == 0);
    const float* normals = normal_view.data;
    AT(normals[2] == 1.0f);

    DvzVisualDataView color_view = {0};
    AT(dvz_visual_data(visual, "color", &color_view) == 0);
    const uint8_t* stored_colors = color_view.data;
    AT(stored_colors[0] == 255);
    AT(stored_colors[5] == 255);

    DvzVisualDataView texcoord_view = {0};
    AT(dvz_visual_data(visual, "texcoords", &texcoord_view) == 0);
    AT(texcoord_view.item_count == 4);
    AT(texcoord_view.item_size == 2 * sizeof(float));
    const float* texcoords = texcoord_view.data;
    AT(texcoords[0] == 0.0f);
    AT(texcoords[2] == 1.0f);
    AT(texcoords[7] == 1.0f);

    AT(_visual_family_state(visual)->buffer != NULL);
    AT(_visual_family_state(visual)->buffer->desc.usage & DVZ_SCENE_BUFFER_USAGE_INDEX);
    AT(_visual_family_state(visual)->buffer->desc.stride == sizeof(DvzIndex));
    AT(_visual_family_state(visual)->buffer->desc.byte_size == 6 * sizeof(DvzIndex));

    DvzVisual* point = dvz_point(scene, 0);
    ANN(point);
    AT(dvz_mesh_set_geometry(point, geometry) == -1);

    dvz_geometry_destroy(geometry);
    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_polygon_composite(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);

    DvzPolygon* polygon = dvz_polygon(scene, 0);
    ANN(polygon);
    const dvec2 outer[4] = {
        {0.0, 0.0},
        {1.0, 0.0},
        {1.0, 1.0},
        {0.0, 1.0},
    };
    AT(dvz_polygon_geometry(
           polygon,
           &(DvzPolygonDesc){
               DVZ_STRUCT_INIT_FIELDS(DvzPolygonDesc),
               .outer = {.xy = outer, .count = 4},
           }) == 0);

    const DvzColor fill_color = {20, 40, 200, 255};
    const DvzColor stroke_color = {240, 220, 40, 255};
    AT(dvz_polygon_fill_color(polygon, fill_color) == 0);
    AT(dvz_polygon_stroke_color(polygon, stroke_color) == 0);
    AT(dvz_polygon_stroke_width(polygon, 3.0f) == 0);
    AT(dvz_polygon_id(polygon, 42) == 0);
    AT(polygon->user_id == 42);
    AT(dvz_polygon_stroke_caps(polygon, DVZ_SEGMENT_CAP_BUTT, DVZ_SEGMENT_CAP_TRIANGLE_OUT) == 0);
    AT(dvz_polygon_stroke_join(polygon, DVZ_PATH_JOIN_BEVEL, 3.0f) == 0);
    DvzPolygonStyle style = dvz_polygon_style();
    style.fill_color = fill_color;
    style.stroke_color = stroke_color;
    style.stroke_width = 3.0f;
    style.stroke_start_cap = DVZ_SEGMENT_CAP_BUTT;
    style.stroke_end_cap = DVZ_SEGMENT_CAP_TRIANGLE_OUT;
    style.stroke_join = DVZ_PATH_JOIN_BEVEL;
    style.stroke_miter_limit = 3.0f;
    AT(dvz_polygon_set_style(polygon, &style) == 0);

    DvzComposite* composite = dvz_polygon_composite(polygon, 0);
    ANN(composite);
    AT(dvz_composite_visual_count(composite) == 2);
    DvzVisual* fill = dvz_composite_visual(composite, "fill");
    DvzVisual* stroke = dvz_composite_visual(composite, "stroke");
    ANN(fill);
    ANN(stroke);
    AT(dvz_composite_visual_at(composite, 0) == fill);

    AT(dvz_panel_add_composite(panel, composite, &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .z_layer = 5}) == 0);
    AT(panel->visual_count == 2);
    AT(panel->visuals[0].visual == fill);
    AT(panel->visuals[0].z_layer == 5);
    AT(panel->visuals[1].visual == stroke);
    AT(panel->visuals[1].z_layer == 6);
    AT(dvz_panel_add_composite(panel, composite, NULL) == 0);
    AT(panel->visual_count == 2);

    DvzVisualDataView fill_position_view = {0};
    AT(dvz_visual_data(fill, "position", &fill_position_view) == 0);
    AT(fill_position_view.item_count == 4);
    AT(_visual_family_state(fill)->buffer != NULL);
    AT(_visual_family_state(fill)->buffer->desc.byte_size == 6 * sizeof(DvzIndex));

    DvzVisualDataView fill_color_view = {0};
    AT(dvz_visual_data(fill, "color", &fill_color_view) == 0);
    const uint8_t* fill_colors = fill_color_view.data;
    AT(fill_colors[0] == fill_color.r);
    AT(fill_colors[1] == fill_color.g);
    AT(fill_colors[2] == fill_color.b);
    AT(fill_colors[3] == fill_color.a);

    DvzVisualDataView stroke_position_view = {0};
    AT(dvz_visual_data(stroke, "position", &stroke_position_view) == 0);
    AT(stroke_position_view.item_count == 5);
    AT(_visual_family_state(stroke)->path.subpath_count == 1);
    AT(_visual_family_state(stroke)->path.subpath_lengths[0] == 5);
    DvzVisualDataView stroke_width_view = {0};
    AT(dvz_visual_data(stroke, "stroke_width", &stroke_width_view) == 0);
    const float* widths = stroke_width_view.data;
    AC(widths[0], 3.0f, EPS);
    AT(_visual_family_state(stroke)->path.cap_start == DVZ_SEGMENT_CAP_BUTT);
    AT(_visual_family_state(stroke)->path.cap_end == DVZ_SEGMENT_CAP_TRIANGLE_OUT);
    AT(_visual_family_state(stroke)->path.join == DVZ_PATH_JOIN_BEVEL);
    AC(_visual_family_state(stroke)->path.miter_limit, 3.0f, EPS);

    uint64_t fill_position_version = 0;
    uint64_t fill_color_version = 0;
    for (uint32_t ai = 0; ai < fill->attr_count; ai++)
    {
        if (strcmp(fill->attrs[ai].name, "position") == 0)
            fill_position_version = fill->attrs[ai].version;
        else if (strcmp(fill->attrs[ai].name, "color") == 0)
            fill_color_version = fill->attrs[ai].version;
    }
    AT(fill_position_version > 0);
    AT(fill_color_version > 0);
    AT(dvz_polygon_stroke_width(polygon, 7.0f) == 0);
    _scene_prepare_composite_visuals(figure);
    AT(dvz_visual_data(stroke, "stroke_width", &stroke_width_view) == 0);
    widths = stroke_width_view.data;
    AC(widths[0], 7.0f, EPS);
    for (uint32_t ai = 0; ai < fill->attr_count; ai++)
    {
        if (strcmp(fill->attrs[ai].name, "position") == 0)
        {
            AT(fill->attrs[ai].version == fill_position_version);
        }
        else if (strcmp(fill->attrs[ai].name, "color") == 0)
        {
            AT(fill->attrs[ai].version == fill_color_version);
        }
    }

    const DvzColor fill_update = {200, 30, 40, 255};
    AT(dvz_polygon_fill_color(polygon, fill_update) == 0);
    _scene_prepare_composite_visuals(figure);
    AT(dvz_visual_data(fill, "color", &fill_color_view) == 0);
    fill_colors = fill_color_view.data;
    AT(fill_colors[0] == fill_update.r);
    AT(fill_colors[1] == fill_update.g);
    AT(fill_colors[2] == fill_update.b);

    const dvec2 hole[4] = {
        {0.25, 0.25},
        {0.75, 0.25},
        {0.75, 0.75},
        {0.25, 0.75},
    };
    AT(dvz_polygon_hole(polygon, 0, 4, hole) == 0);
    _scene_prepare_composite_visuals(figure);
    AT(dvz_visual_data(fill, "position", &fill_position_view) == 0);
    AT(fill_position_view.item_count == 8);
    AT(dvz_visual_data(stroke, "position", &stroke_position_view) == 0);
    AT(stroke_position_view.item_count == 10);
    AT(_visual_family_state(stroke)->path.subpath_count == 2);

    AT(dvz_polygon_visible(polygon, false) == 0);
    _scene_prepare_composite_visuals(figure);
    AT(!fill->visible);
    AT(!stroke->visible);
    AT(dvz_polygon_visible(polygon, true) == 0);
    _scene_prepare_composite_visuals(figure);
    AT(fill->visible);
    AT(stroke->visible);

    dvz_composite_destroy(composite);
    AT(dvz_composite_visual_count(composite) == 0);
    AT(!fill->visible);
    AT(!stroke->visible);

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_polygon_set_composite(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);

    DvzPolygonSet* set = dvz_polygon_set(scene, 0);
    ANN(set);
    const dvec2 left[4] = {
        {0.0, 0.0},
        {1.0, 0.0},
        {1.0, 1.0},
        {0.0, 1.0},
    };
    const dvec2 right[4] = {
        {2.0, 0.0},
        {3.0, 0.0},
        {3.0, 1.0},
        {2.0, 1.0},
    };
    const uint32_t left_index = dvz_polygon_set_add(
        set,
        &(DvzPolygonDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzPolygonDesc),
            .outer = {.xy = left, .count = 4},
        });
    const uint32_t right_index = dvz_polygon_set_add(
        set,
        &(DvzPolygonDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzPolygonDesc),
            .outer = {.xy = right, .count = 4},
        });
    AT(left_index == 0);
    AT(right_index == 1);

    const DvzColor red = {255, 0, 0, 255};
    const DvzColor green = {0, 255, 0, 255};
    AT(dvz_polygon_set_region_fill_color(set, left_index, red) == 0);
    AT(dvz_polygon_set_region_fill_color(set, right_index, green) == 0);
    AT(dvz_polygon_set_region_stroke_width(set, left_index, 2.0f) == 0);
    AT(dvz_polygon_set_region_stroke_width(set, right_index, 4.0f) == 0);
    const uint64_t ids[2] = {101, 102};
    AT(dvz_polygon_set_region_ids(set, 0, 2, ids) == 0);
    AT(set->polygons[left_index].user_id == 101);
    AT(set->polygons[right_index].user_id == 102);
    AT(dvz_polygon_set_stroke_caps(set, DVZ_SEGMENT_CAP_BUTT, DVZ_SEGMENT_CAP_TRIANGLE_OUT) == 0);
    AT(dvz_polygon_set_stroke_join(set, DVZ_PATH_JOIN_MITER, 5.0f) == 0);

    DvzComposite* composite = dvz_polygon_set_composite(set, 0);
    ANN(composite);
    AT(dvz_panel_add_composite(panel, composite, NULL) == 0);
    AT(panel->visual_count == 2);

    DvzVisual* fill = dvz_composite_visual(composite, "fill");
    DvzVisual* stroke = dvz_composite_visual(composite, "stroke");
    ANN(fill);
    ANN(stroke);

    DvzVisualDataView position_view = {0};
    AT(dvz_visual_data(fill, "position", &position_view) == 0);
    AT(position_view.item_count == 8);
    AT(_visual_family_state(fill)->buffer != NULL);
    AT(_visual_family_state(fill)->buffer->desc.byte_size == 12 * sizeof(DvzIndex));

    DvzVisualDataView color_view = {0};
    AT(dvz_visual_data(fill, "color", &color_view) == 0);
    const uint8_t* colors = color_view.data;
    AT(colors[0] == red.r);
    AT(colors[1] == red.g);
    AT(colors[16] == green.r);
    AT(colors[17] == green.g);

    DvzVisualDataView stroke_position_view = {0};
    AT(dvz_visual_data(stroke, "position", &stroke_position_view) == 0);
    AT(stroke_position_view.item_count == 10);
    AT(_visual_family_state(stroke)->path.subpath_count == 2);
    DvzVisualDataView stroke_width_view = {0};
    AT(dvz_visual_data(stroke, "stroke_width", &stroke_width_view) == 0);
    const float* widths = stroke_width_view.data;
    AC(widths[0], 2.0f, EPS);
    AC(widths[5], 4.0f, EPS);
    AT(_visual_family_state(stroke)->path.cap_start == DVZ_SEGMENT_CAP_BUTT);
    AT(_visual_family_state(stroke)->path.cap_end == DVZ_SEGMENT_CAP_TRIANGLE_OUT);
    AT(_visual_family_state(stroke)->path.join == DVZ_PATH_JOIN_MITER);
    AC(_visual_family_state(stroke)->path.miter_limit, 5.0f, EPS);

    uint64_t fill_position_version = 0;
    uint64_t fill_color_version = 0;
    for (uint32_t ai = 0; ai < fill->attr_count; ai++)
    {
        if (strcmp(fill->attrs[ai].name, "position") == 0)
            fill_position_version = fill->attrs[ai].version;
        else if (strcmp(fill->attrs[ai].name, "color") == 0)
            fill_color_version = fill->attrs[ai].version;
    }
    AT(fill_position_version > 0);
    AT(fill_color_version > 0);
    AT(dvz_polygon_set_region_stroke_width(set, right_index, 7.0f) == 0);
    _scene_prepare_composite_visuals(figure);
    AT(dvz_visual_data(stroke, "stroke_width", &stroke_width_view) == 0);
    widths = stroke_width_view.data;
    AC(widths[0], 2.0f, EPS);
    AC(widths[5], 7.0f, EPS);
    for (uint32_t ai = 0; ai < fill->attr_count; ai++)
    {
        if (strcmp(fill->attrs[ai].name, "position") == 0)
        {
            AT(fill->attrs[ai].version == fill_position_version);
        }
        else if (strcmp(fill->attrs[ai].name, "color") == 0)
        {
            AT(fill->attrs[ai].version == fill_color_version);
        }
    }

    const DvzColor blue = {0, 0, 255, 255};
    AT(dvz_polygon_set_region_fill_color(set, right_index, blue) == 0);
    _scene_prepare_composite_visuals(figure);
    AT(dvz_visual_data(fill, "color", &color_view) == 0);
    colors = color_view.data;
    AT(colors[16] == blue.r);
    AT(colors[17] == blue.g);
    AT(colors[18] == blue.b);

    const DvzColor bulk_fill[2] = {{11, 22, 33, 255}, {44, 55, 66, 255}};
    const DvzColor bulk_stroke[2] = {{77, 88, 99, 255}, {111, 122, 133, 255}};
    const float bulk_widths[2] = {1.5f, 2.5f};
    AT(dvz_polygon_set_region_fill_colors(set, 0, 2, bulk_fill) == 0);
    AT(dvz_polygon_set_region_stroke_colors(set, 0, 2, bulk_stroke) == 0);
    AT(dvz_polygon_set_region_stroke_widths(set, 0, 2, bulk_widths) == 0);
    _scene_prepare_composite_visuals(figure);
    AT(dvz_visual_data(fill, "color", &color_view) == 0);
    colors = color_view.data;
    AT(colors[0] == bulk_fill[0].r);
    AT(colors[16] == bulk_fill[1].r);
    AT(dvz_visual_data(stroke, "stroke_width", &stroke_width_view) == 0);
    widths = stroke_width_view.data;
    AC(widths[0], bulk_widths[0], EPS);
    AC(widths[5], bulk_widths[1], EPS);

    const bool one_visible[2] = {true, false};
    AT(dvz_polygon_set_region_visibilities(set, 0, 2, one_visible) == 0);
    _scene_prepare_composite_visuals(figure);
    AT(dvz_visual_data(fill, "position", &position_view) == 0);
    AT(position_view.item_count == 4);
    AT(dvz_visual_data(stroke, "position", &stroke_position_view) == 0);
    AT(stroke_position_view.item_count == 5);
    AT(dvz_polygon_set_region_visible(set, left_index, false) == 0);
    _scene_prepare_composite_visuals(figure);
    AT(!fill->visible);
    AT(!stroke->visible);

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_graph_composite(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);

    DvzGraph* graph = dvz_graph(scene, 0);
    ANN(graph);
    const dvec3 positions[3] = {
        {0.0, 0.0, 0.0},
        {1.0, 0.0, 0.0},
        {0.5, 1.0, 0.0},
    };
    AT(dvz_graph_node_count(graph, 3) == 0);
    AT(dvz_graph_node_positions(graph, 0, 3, positions) == 0);
    const uint32_t edges[4] = {0, 1, 1, 2};
    AT(dvz_graph_edge_count(graph, 2) == 0);
    AT(dvz_graph_edges(graph, 0, 2, edges) == 0);
    const uint64_t node_ids[3] = {101, 102, 103};
    const uint64_t edge_ids[2] = {201, 202};
    AT(dvz_graph_node_ids(graph, 0, 3, node_ids) == 0);
    AT(dvz_graph_edge_ids(graph, 0, 2, edge_ids) == 0);
    AT(graph->nodes[0].user_id == 101);
    AT(graph->nodes[2].user_id == 103);
    AT(graph->edges[0].user_id == 201);
    AT(graph->edges[1].user_id == 202);
    const DvzColor node_colors[3] = {
        {255, 80, 80, 255},
        {80, 255, 80, 255},
        {80, 80, 255, 255},
    };
    const float node_sizes[3] = {10.0f, 20.0f, 30.0f};
    const DvzColor edge_colors[2] = {{220, 220, 220, 255}, {255, 180, 80, 255}};
    const float edge_widths[2] = {2.0f, 4.0f};
    AT(dvz_graph_node_colors(graph, 0, 3, node_colors) == 0);
    AT(dvz_graph_node_sizes(graph, 0, 3, node_sizes) == 0);
    AT(dvz_graph_edge_colors(graph, 0, 2, edge_colors) == 0);
    AT(dvz_graph_edge_widths(graph, 0, 2, edge_widths) == 0);

    DvzComposite* composite = dvz_graph_composite(graph, 0);
    ANN(composite);
    AT(dvz_composite_visual_count(composite) == 3);
    AT(dvz_panel_add_composite(
           panel, composite,
           &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .z_layer = 7}) == 0);
    AT(panel->visual_count == 3);
    AT(dvz_panel_add_composite(panel, composite, NULL) == 0);
    AT(panel->visual_count == 3);

    DvzVisual* edge_visual = dvz_composite_visual(composite, "edges");
    DvzVisual* node_visual = dvz_composite_visual(composite, "nodes");
    ANN(edge_visual);
    ANN(node_visual);
    AT(edge_visual->type == DVZ_VISUAL_TYPE_SEGMENT);
    AT(node_visual->type == DVZ_VISUAL_TYPE_MARKER);
    AT(panel->visuals[0].z_layer == 7);
    AT(panel->visuals[2].z_layer == 8);

    DvzVisualDataView node_position_view = {0};
    AT(dvz_visual_data(node_visual, "position", &node_position_view) == 0);
    AT(node_position_view.item_count == 3);
    const float* node_positions = node_position_view.data;
    AC(node_positions[0], 0.0f, EPS);
    AC(node_positions[3], 1.0f, EPS);

    DvzVisualDataView node_size_view = {0};
    AT(dvz_visual_data(node_visual, "diameter", &node_size_view) == 0);
    AT(node_size_view.item_count == 3);
    const float* sizes = node_size_view.data;
    AC(sizes[0], 10.0f, EPS);
    AC(sizes[2], 30.0f, EPS);

    DvzVisualDataView edge_start_view = {0};
    DvzVisualDataView edge_end_view = {0};
    DvzVisualDataView edge_width_view = {0};
    AT(dvz_visual_data(edge_visual, "position_start", &edge_start_view) == 0);
    AT(dvz_visual_data(edge_visual, "position_end", &edge_end_view) == 0);
    AT(dvz_visual_data(edge_visual, "stroke_width", &edge_width_view) == 0);
    AT(edge_start_view.item_count == 2);
    const float* starts = edge_start_view.data;
    const float* ends = edge_end_view.data;
    const float* widths = edge_width_view.data;
    AC(starts[0], 0.0f, EPS);
    AC(ends[0], 1.0f, EPS);
    AC(widths[0], 2.0f, EPS);
    AC(widths[1], 4.0f, EPS);

    DvzGraphEdgeStyle edge_style = dvz_graph_edge_style();
    edge_style.mode = DVZ_GRAPH_EDGE_MODE_BEZIER;
    edge_style.tessellation.segment_count = 4;
    AT(dvz_graph_set_edge_style(graph, &edge_style) == 0);
    _scene_prepare_composite_visuals(figure);
    DvzVisual* path_edges = dvz_composite_visual(composite, "edges");
    ANN(path_edges);
    AT(path_edges->type == DVZ_VISUAL_TYPE_PATH);
    AT(path_edges->visible);
    AT(!edge_visual->visible);
    DvzVisualDataView path_position_view = {0};
    AT(dvz_visual_data(path_edges, "position", &path_position_view) == 0);
    AT(path_position_view.item_count == 10);
    AT(_visual_family_state(path_edges)->path.subpath_count == 2);
    AT(_visual_family_state(path_edges)->path.subpath_lengths[0] == 5);
    AT(_visual_family_state(path_edges)->path.subpath_lengths[1] == 5);

    const dvec3 moved[1] = {{2.0, 0.0, 0.0}};
    AT(dvz_graph_node_positions(graph, 1, 1, moved) == 0);
    _scene_prepare_composite_visuals(figure);
    AT(dvz_visual_data(node_visual, "position", &node_position_view) == 0);
    node_positions = node_position_view.data;
    AC(node_positions[3], 2.0f, EPS);
    AT(dvz_visual_data(path_edges, "position", &path_position_view) == 0);
    const float* path_positions = path_position_view.data;
    AC(path_positions[12], 2.0f, EPS);

    dvz_graph_destroy(graph);
    AT(dvz_composite_visual_count(composite) == 0);
    AT(!node_visual->visible);
    AT(!path_edges->visible);

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_additional_typed_data_uploads(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);

    vec3 positions[3] = {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
    };
    DvzColor colors[3] = {{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}};
    vec3 normals[3] = {
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
    };
    float sizes[3] = {4.0f, 8.0f, 12.0f};

    DvzVisual* pixel = dvz_pixel(scene, 0);
    DvzVisual* primitive = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    DvzVisual* sphere = dvz_sphere(scene, 0);
    ANN(pixel);
    ANN(primitive);
    ANN(sphere);

    DvzVisualDataUpdate pixel_updates[] = {
        {.attr_name = "position", .data = positions, .item_count = 3},
        {.attr_name = "color", .data = colors, .item_count = 3},
        {.attr_name = "pixel_size", .data = sizes, .item_count = 3},
    };
    DvzVisualDataUpdate primitive_updates[] = {
        {.attr_name = "position", .data = positions, .item_count = 3},
        {.attr_name = "color", .data = colors, .item_count = 3},
        {.attr_name = "normal", .data = normals, .item_count = 3},
    };
    DvzVisualDataUpdate sphere_updates[] = {
        {.attr_name = "position", .data = positions, .item_count = 3},
        {.attr_name = "color", .data = colors, .item_count = 3},
        {.attr_name = "radius", .data = sizes, .item_count = 3},
    };
    AT(dvz_visual_set_data_many(pixel, pixel_updates, 3) == 0);
    AT(dvz_visual_set_data_many(primitive, primitive_updates, 3) == 0);
    AT(dvz_visual_set_data_many(sphere, sphere_updates, 3) == 0);

    DvzVisualDataView pixel_size_view = {0};
    AT(dvz_visual_data(pixel, "pixel_size", &pixel_size_view) == 0);
    const float* stored_pixel_sizes = pixel_size_view.data;
    AT(stored_pixel_sizes[2] == 12.0f);

    DvzVisualDataView primitive_normal_view = {0};
    AT(dvz_visual_data(primitive, "normal", &primitive_normal_view) == 0);
    const float* stored_normals = primitive_normal_view.data;
    AT(stored_normals[2] == 1.0f);

    DvzVisualDataView sphere_radius_view = {0};
    AT(dvz_visual_data(sphere, "radius", &sphere_radius_view) == 0);
    const float* stored_radii = sphere_radius_view.data;
    AT(stored_radii[1] == 8.0f);

    DvzVisual* flat_primitive = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    ANN(flat_primitive);
    DvzVisualDataUpdate flat_updates[] = {
        {.attr_name = "position", .data = positions, .item_count = 3},
        {.attr_name = "color", .data = colors, .item_count = 3},
    };
    AT(dvz_visual_set_data_many(flat_primitive, flat_updates, 2) == 0);
    DvzVisualDataView flat_normal_view = {0};
    AT(dvz_visual_data(flat_primitive, "normal", &flat_normal_view) == -1);

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_typed_upload_rejects_wrong_family(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* point = dvz_point(scene, 0);
    DvzVisual* mesh = dvz_mesh(scene, 0);
    DvzVisual* primitive = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    DvzVisual* sphere = dvz_sphere(scene, 0);
    ANN(point);
    ANN(mesh);
    ANN(primitive);
    ANN(sphere);

    vec3 positions[1] = {{0.0f, 0.0f, 0.0f}};
    DvzColor colors[1] = {{255, 255, 255, 255}};
    float diameters[1] = {4.0f};
    uint32_t item_state[1] = {DVZ_ITEM_STATE_SELECTED};

    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_data(mesh, "diameter", diameters, 1) == -1);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_data(point, "normal", positions, 1) == -1);
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_visual_set_data(primitive, "radius", diameters, 1) == -1);
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_visual_set_data(sphere, "pixel_size", diameters, 1) == -1);
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_visual_set_data(point, "item_state", item_state, 0) == -1);

    DvzVisualDataUpdate mismatch[] = {
        {.attr_name = "position", .data = positions, .item_count = 1},
        {.attr_name = "color", .data = colors, .item_count = 2},
    };
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_data_many(point, mismatch, 2) == -1);

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_point_external_position_buffer_emits_no_upload(TstContext* suite, const TstCase* item)
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

    DvzSceneBufferDesc desc = {DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
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

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);

    char position_key[DVZ_SCENE_LABEL_SIZE] = {0};
    AT(dvz_scene_buffer_resource_key(position, position_key, sizeof(position_key)));
    uint64_t position_buffer_id = dvz_drp2_stream_label_id(stream, position_key);
    AT(position_buffer_id != 0);

    uint32_t create_count = 0;
    uint32_t write_count = 0;
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


int test_scene_point_storage_position_buffer_emits_usage(TstContext* suite, const TstCase* item)
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

    vec3 positions[4] = {
        {-0.75f, -0.75f, 0.0f},
        {-0.5f, -0.5f, 0.0f},
        {+0.5f, -0.5f, 0.0f},
        { 0.0f, +0.5f, 0.0f},
    };
    DvzSceneBufferDesc desc = {DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
        .usage = DVZ_SCENE_BUFFER_USAGE_VERTEX | DVZ_SCENE_BUFFER_USAGE_STORAGE,
        .stride = sizeof(vec3),
    };
    DvzSceneBuffer* position = dvz_scene_buffer(scene, &desc);
    ANN(position);
    AT(dvz_scene_buffer_set_data(position, positions, sizeof(positions)));
    AT(dvz_visual_set_attr_buffer(visual, "position", position, sizeof(vec3), 3));

    DvzColor colors[3] = {{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}};
    float sizes[3] = {4.0f, 5.0f, 6.0f};
    AT(dvz_visual_set_data(visual, "color", colors, 3) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 3) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
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
            AT(cmd->u.set_vertex_buffer.offset == 0);
        }
    }
    AT(position_buffer_id != 0);

    bool found_create = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        ANN(cmd);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_BUFFER &&
            cmd->u.create_buffer.id == position_buffer_id)
        {
            found_create = true;
            AT((cmd->u.create_buffer.usage & DVZ_DRP2_BUFFER_USAGE_VERTEX) != 0);
            AT((cmd->u.create_buffer.usage & DVZ_DRP2_BUFFER_USAGE_STORAGE) != 0);
        }
    }
    AT(found_create);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_descriptor_abi_rejects_invalid_structs(TstContext* suite, const TstCase* item)
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
    DvzVisual* marker = dvz_marker(scene, 0);
    ANN(marker);
    DvzVisual* vector = dvz_vector(scene, 0);
    ANN(vector);
    DvzVisual* mesh = dvz_mesh(scene, 0);
    ANN(mesh);

    DvzSceneBufferDesc buffer_desc = dvz_scene_buffer_desc();
    buffer_desc.struct_size = 0;
    buffer_desc.usage = DVZ_SCENE_BUFFER_USAGE_VERTEX;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_scene_buffer(scene, &buffer_desc) == NULL);

    buffer_desc = dvz_scene_buffer_desc();
    buffer_desc.flags = 1;
    buffer_desc.usage = DVZ_SCENE_BUFFER_USAGE_VERTEX;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_scene_buffer(scene, &buffer_desc) == NULL);

    DvzSampledFieldDesc field_desc = dvz_sampled_field_desc();
    field_desc.struct_size = DVZ_STRUCT_SIZE(DvzSampledFieldDesc) - 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_sampled_field(scene, &field_desc) == NULL);

    field_desc = dvz_sampled_field_desc();
    field_desc.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_sampled_field(scene, &field_desc) == NULL);

    DvzSceneComputeDesc compute_desc = dvz_scene_compute_desc();
    compute_desc.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_scene_compute(scene, &compute_desc) == NULL);

    compute_desc = dvz_scene_compute_desc();
    compute_desc.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_scene_compute(scene, &compute_desc) == NULL);

    DvzVisualAttachDesc attach_desc = dvz_visual_attach_desc();
    attach_desc.struct_size = DVZ_STRUCT_SIZE(DvzVisualAttachDesc) - 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_panel_add_visual(panel, visual, &attach_desc) < 0);

    attach_desc = dvz_visual_attach_desc();
    attach_desc.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_panel_add_visual(panel, visual, &attach_desc) < 0);

    DvzPanelBackgroundDesc background_desc = dvz_panel_background_desc();
    background_desc.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(suite, !dvz_panel_set_background(panel, &background_desc));

    background_desc = dvz_panel_background_desc();
    background_desc.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, !dvz_panel_set_background(panel, &background_desc));

    DvzPanelBorderDesc border_desc = dvz_panel_border_desc();
    border_desc.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(suite, !dvz_panel_set_border(panel, &border_desc));

    border_desc = dvz_panel_border_desc();
    border_desc.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, !dvz_panel_set_border(panel, &border_desc));

    DvzQueryRequest request = dvz_query_request();
    request.struct_size = DVZ_STRUCT_SIZE(DvzQueryRequest) - 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_panel_query(panel, 0.0, 0.0, &request) < 0);

    request = dvz_query_request();
    request.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_panel_query(panel, 0.0, 0.0, &request) < 0);

    DvzEdlDesc edl = dvz_edl_desc();
    edl.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(suite, !dvz_panel_set_edl(panel, &edl));

    edl = dvz_edl_desc();
    edl.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, !dvz_panel_set_edl(panel, &edl));

    DvzMsaaDesc msaa = dvz_msaa_desc();
    msaa.struct_size = DVZ_STRUCT_SIZE(DvzMsaaDesc) - 1;
    AT_EXPECTED_ERROR_STRICT(suite, !dvz_panel_set_msaa(panel, &msaa));

    msaa = dvz_msaa_desc();
    msaa.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, !dvz_panel_set_msaa(panel, &msaa));

    DvzSsaoDesc ssao = dvz_ssao_desc();
    ssao.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(suite, !dvz_panel_set_ssao(panel, &ssao));

    ssao = dvz_ssao_desc();
    ssao.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, !dvz_panel_set_ssao(panel, &ssao));

    DvzVolumeOcclusionDesc volume_occlusion = dvz_volume_occlusion_desc();
    volume_occlusion.struct_size = DVZ_STRUCT_SIZE(DvzVolumeOcclusionDesc) - 1;
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_panel_set_volume_occluder(panel, NULL, &volume_occlusion) < 0);

    volume_occlusion = dvz_volume_occlusion_desc();
    volume_occlusion.flags = 1;
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_panel_set_volume_occluder(panel, NULL, &volume_occlusion) < 0);

    DvzSceneOcclusionDesc scene_occlusion = dvz_scene_occlusion_desc();
    scene_occlusion.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_panel_set_scene_occlusion(panel, &scene_occlusion) < 0);

    scene_occlusion = dvz_scene_occlusion_desc();
    scene_occlusion.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_panel_set_scene_occlusion(panel, &scene_occlusion) < 0);

    DvzPointStyleDesc point_style = dvz_point_style_desc();
    point_style.struct_size = DVZ_STRUCT_SIZE(DvzPointStyleDesc) - 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_point_set_style(visual, &point_style) < 0);

    point_style = dvz_point_style_desc();
    point_style.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_point_set_style(visual, &point_style) < 0);

    DvzMarkerStyle marker_style = dvz_marker_style();
    marker_style.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_marker_set_style(marker, &marker_style) < 0);

    marker_style = dvz_marker_style();
    marker_style.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_marker_set_style(marker, &marker_style) < 0);

    DvzVectorStyle vector_style = dvz_vector_style();
    vector_style.struct_size = DVZ_STRUCT_SIZE(DvzVectorStyle) - 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_vector_set_style(vector, &vector_style) < 0);

    vector_style = dvz_vector_style();
    vector_style.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_vector_set_style(vector, &vector_style) < 0);

    DvzMaterialDesc material = dvz_material_desc();
    material.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_material(mesh, &material) < 0);

    material = dvz_material_desc();
    material.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_material(mesh, &material) < 0);

    DvzDepthCueDesc depth_cue = dvz_depth_cue_desc();
    depth_cue.struct_size = DVZ_STRUCT_SIZE(DvzDepthCueDesc) - 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_depth_cue(visual, &depth_cue) < 0);

    depth_cue = dvz_depth_cue_desc();
    depth_cue.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_depth_cue(visual, &depth_cue) < 0);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_compute_point_position_buffer_emits_drp2(
    TstContext* suite, const TstCase* item)
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

    vec3 positions[3] = {
        {-0.5f, -0.5f, 0.0f},
        {+0.5f, -0.5f, 0.0f},
        { 0.0f, +0.5f, 0.0f},
    };
    DvzSceneBuffer* position = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
                   .usage = DVZ_SCENE_BUFFER_USAGE_VERTEX | DVZ_SCENE_BUFFER_USAGE_STORAGE,
                   .stride = sizeof(vec3),
                   .byte_size = sizeof(positions),
               });
    ANN(position);
    AT(dvz_scene_buffer_set_data(position, positions, sizeof(positions)));
    AT(dvz_visual_set_attr_buffer(visual, "position", position, 0, 3));

    vec4 params = {0.0f, 0.0f, 3.0f, 0.0f};
    DvzSceneBuffer* param = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
                   .usage = DVZ_SCENE_BUFFER_USAGE_STORAGE,
                   .stride = sizeof(vec4),
                   .byte_size = sizeof(params),
               });
    ANN(param);
    AT(dvz_scene_buffer_set_data(param, &params, sizeof(params)));

    DvzColor colors[3] = {{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}};
    float sizes[3] = {4.0f, 5.0f, 6.0f};
    AT(dvz_visual_set_data(visual, "color", colors, 3) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 3) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    const char* shader =
        "#version 450\n"
        "layout(local_size_x = 1) in;\n"
        "layout(std430, set = 0, binding = 0) readonly buffer Params { vec4 p; } params;\n"
        "layout(std430, set = 0, binding = 1) buffer Positions { float x[]; } positions;\n"
        "void main() {\n"
        "    uint i = gl_GlobalInvocationID.x;\n"
        "    if (i >= uint(params.p.z)) return;\n"
        "    positions.x[3u * i + 0u] += 0.0;\n"
        "}\n";
    DvzSceneCompute* compute = dvz_scene_compute(
        scene, &(DvzSceneComputeDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneComputeDesc),
                   .label = "test_compute_points",
                   .shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL,
                   .shader_source = shader,
                   .dispatch = {3, 1, 1},
               });
    ANN(compute);
    AT(dvz_scene_compute_set_buffer(
        compute, 0, param, DVZ_SCENE_COMPUTE_ACCESS_READ, 0, sizeof(params)));
    AT(dvz_scene_compute_set_buffer(
        compute, 1, position, DVZ_SCENE_COMPUTE_ACCESS_READ_WRITE, 0, sizeof(positions)));
    AT(dvz_figure_add_compute(figure, compute));

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);
    DvzDrp2ValidationResult validation = dvz_drp2_validate_stream(stream);
    AT(validation.ok);

    uint64_t position_buffer_id = 0;
    bool found_compute_pipeline = false;
    bool found_dispatch = false;
    bool found_barrier = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        ANN(cmd);
        if (cmd->type == DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER &&
            cmd->u.set_vertex_buffer.slot == 0)
            position_buffer_id = cmd->u.set_vertex_buffer.buffer_id;
        found_compute_pipeline =
            found_compute_pipeline || cmd->type == DVZ_DRP2_COMMAND_CREATE_COMPUTE_PIPELINE;
        found_dispatch = found_dispatch || cmd->type == DVZ_DRP2_COMMAND_DISPATCH_WORKGROUPS;
    }
    AT(position_buffer_id != 0);
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        ANN(cmd);
        if (cmd->type == DVZ_DRP2_COMMAND_RESOURCE_BARRIER &&
            cmd->u.resource_barrier.buffer_id == position_buffer_id)
            found_barrier = true;
    }
    AT(found_compute_pipeline);
    AT(found_dispatch);
    AT(found_barrier);

    dvz_drp2_stream_destroy(stream);

    DvzDrp2CommandStream* stream2 = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream2);

    uint32_t create_bind_group_count = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream2); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream2, i);
        ANN(cmd);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_BIND_GROUP)
            create_bind_group_count++;
    }
    AT(create_bind_group_count == 0);
    dvz_drp2_stream_destroy(stream2);

    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_point_external_position_buffer_executes(TstContext* suite, const TstCase* item)
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
        log_warn(
            "test_scene_point_external_position_buffer_executes skipped: GPU context creation "
            "failed");
        tst_skip(suite, "GPU context creation failed");
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

    DvzSceneBufferDesc desc = {DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
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

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    char position_key[DVZ_SCENE_LABEL_SIZE] = {0};
    AT(dvz_scene_buffer_resource_key(scene_position, position_key, sizeof(position_key)));
    uint64_t position_buffer_id = dvz_drp2_stream_label_id(stream, position_key);
    AT(position_buffer_id != 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);
    DvzDrp2ExternalBufferDesc external = {
        DVZ_STRUCT_INIT_FIELDS(DvzDrp2ExternalBufferDesc),
        .buffer = runtime_position,
        .size = position_bytes,
        .usage = DVZ_DRP2_BUFFER_USAGE_VERTEX,
    };
    AT(dvz_drp2_runtime_register_external_buffer(runtime, position_buffer_id, &external));

    DvzSemaphore* ready = dvz_semaphore_create_wrapper();
    ANN(ready);
    dvz_semaphore_timeline(dvz_gpu_ctx_device(ctx), 0, ready, 0);
    dvz_semaphore_signal(ready, 1);
    AT(dvz_interop_buffer_wait_timeline(
        dvz_gpu_ctx_device(ctx), runtime_position, position_bytes, ready, 1));

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    dvz_semaphore_destroy(ready);
    dvz_semaphore_free(ready);
    dvz_drp2_runtime_destroy(runtime);
    dvz_drp2_stream_destroy(stream);
    dvz_buffer_destroy(runtime_position);
    dvz_buffer_free(runtime_position);
    dvz_scene_destroy(scene);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}


int test_scene_point_rejects_texcoords_attribute(TstContext* suite, const TstCase* item)
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


int test_scene_primitive_rejects_size_attribute(TstContext* suite, const TstCase* item)
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


int test_scene_path_rejects_size_attribute(TstContext* suite, const TstCase* item)
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


int test_scene_image_rejects_size_attribute(TstContext* suite, const TstCase* item)
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


int test_scene_emit_warns_visual_with_no_position(TstContext* suite, const TstCase* item)
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
    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    tst_log_capture_begin(suite);
    DvzDrp2CommandStream* stream = NULL;
    AT_EXPECTED_LOG_STRICT(
        suite, LOG_WARN, (stream = dvz_figure_emit(figure, &caps, &report)) != NULL);
    AT(stream != NULL);
    AT(_captured_log_contains(suite, "has no 'position' data"));

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_rejects_mismatched_point_attribute_counts(TstContext* suite, const TstCase* item)
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


int test_scene_point_visual_resizes_existing_attributes(TstContext* suite, const TstCase* item)
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


int test_scene_rejects_range_update_without_full_allocation(TstContext* suite, const TstCase* item)
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


int test_scene_rejects_mutation_while_emitted_stream_is_live(TstContext* suite, const TstCase* item)
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

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
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


int test_scene_rejects_scale_binding_while_emitted_stream_is_live(TstContext* suite, const TstCase* item)
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

    vec3 positions[4] = {
        {-0.5f, -0.5f, 0.0f}, {-0.5f, 0.5f, 0.0f},
        {0.5f, -0.5f, 0.0f}, {0.5f, 0.5f, 0.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };
    uint8_t pixels[4 * 4 * 4] = {0};
    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
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
           field, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView), .data = pixels, .bytes_per_row = 16, .rows_per_image = 4}));
    AT(dvz_visual_set_field(image, "field", field));
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
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


int test_scene_rejects_range_mutation_while_emitted_stream_is_live(TstContext* suite, const TstCase* item)
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

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
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


int test_scene_rejects_destroy_while_emitted_stream_is_live(TstContext* suite, const TstCase* item)
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

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
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


int
test_scene_rejects_visual_destroy_while_emitted_stream_is_live(TstContext* suite, const TstCase* item)
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

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
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


int test_scene_live_stream_count_tracks_multiple_emits(TstContext* suite, const TstCase* item)
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

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
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


int test_scene_point_emit(TstContext* suite, const TstCase* item)
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

    vec3 positions[3] = {
        {-0.5f, -0.5f, 0.0f},
        {0.5f, -0.5f, 0.0f},
        {0.0f, 0.5f, 0.0f},
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
    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
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


int test_scene_path_emit(TstContext* suite, const TstCase* item)
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

    vec3 positions[4] = {
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

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
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


int test_scene_image_emit(TstContext* suite, const TstCase* item)
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

    vec3 positions[4] = {
        {-0.5f, -0.5f, 0.0f}, {-0.5f, 0.5f, 0.0f},
        { 0.5f, -0.5f, 0.0f}, { 0.5f, 0.5f, 0.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };
    uint8_t pixels[4 * 4 * 4];
    dvz_memset(pixels, sizeof(pixels), 128, sizeof(pixels));

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_texture(visual, pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
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


int test_scene_image_multi_item_emit(TstContext* suite, const TstCase* item)
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

    vec3 positions[2] = {{-0.4f, 0.0f, 0.0f}, {+0.4f, 0.0f, 0.0f}};
    vec2 extents[2] = {{0.3f, 0.4f}, {0.2f, 0.5f}};
    vec4 tex_rects[2] = {{0.0f, 0.0f, 0.5f, 1.0f}, {0.5f, 0.0f, 1.0f, 1.0f}};
    uint8_t pixels[4 * 4 * 4];
    dvz_memset(pixels, sizeof(pixels), 128, sizeof(pixels));

    AT(dvz_visual_set_data(visual, "position", positions, 2) == 0);
    AT(dvz_visual_set_data(visual, "extent", extents, 2) == 0);
    AT(dvz_visual_set_data(visual, "tex_rect", tex_rects, 2) == 0);
    AT(dvz_visual_set_texture(visual, pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
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
    bool found_uv_values = false;
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
        {
            found_uv_upload = cmd->u.write_buffer.size == 12 * 2 * sizeof(float);
            if (found_uv_upload && cmd->u.write_buffer.data_raw != NULL)
            {
                const float* uv = (const float*)cmd->u.write_buffer.data_raw;
                const float expected_uv[24] = {
                    0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 0.0f, 0.5f, 0.0f,
                    0.0f, 1.0f, 0.5f, 1.0f, 0.5f, 0.0f, 0.5f, 1.0f,
                    1.0f, 0.0f, 1.0f, 0.0f, 0.5f, 1.0f, 1.0f, 1.0f,
                };
                found_uv_values = true;
                for (uint32_t j = 0; j < 24; j++)
                    found_uv_values = found_uv_values && uv[j] == expected_uv[j];
            }
        }
    }

    AT(found_pipeline);
    AT(found_draw);
    AT(found_position_upload);
    AT(found_uv_upload);
    AT(found_uv_values);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify pixel-anchored image visuals emit fixed-size generated quads.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_image_pixel_anchor_emit_wgsl(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 128, 96, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_image(scene, 0);
    ANN(visual);

    vec3 positions[1] = {{10.0f, 20.0f, 0.5f}};
    vec2 extents[1] = {{80.0f, 40.0f}};
    vec2 anchors[1] = {{-1.0f, -1.0f}};
    vec4 tex_rects[1] = {{0.25f, 0.25f, 0.75f, 0.75f}};
    uint8_t pixels[4 * 4 * 4];
    dvz_memset(pixels, sizeof(pixels), 128, sizeof(pixels));

    AT(dvz_visual_set_data(visual, "position_px", positions, 1) == 0);
    AT(dvz_visual_set_data(visual, "extent_px", extents, 1) == 0);
    AT(dvz_visual_set_data(visual, "anchor", anchors, 1) == 0);
    AT(dvz_visual_set_data(visual, "tex_rect", tex_rects, 1) == 0);
    AT(dvz_visual_set_texture(visual, pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(
           panel, visual,
           &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .z_layer = 1, .controller_mode = DVZ_CONTROLLER_FIXED}) == 0);

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

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    uint64_t position_buffer_id = 0;
    bool found_pipeline = false;
    bool found_draw = false;
    bool found_vertex_shader = false;
    bool found_position_values = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        ANN(cmd);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.create_render_pipeline.id);
            if (label != NULL && strstr(label, "_pipe_img_pxw") == label)
            {
                found_pipeline = true;
                AT(cmd->u.create_render_pipeline.topology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
                AT(cmd->u.create_render_pipeline.vertex_buffer_slots == 2);
                AT(cmd->u.create_render_pipeline.binding_count == 2);
                AT(cmd->u.create_render_pipeline.attr_count == 2);
                AT(cmd->u.create_render_pipeline.bind_group_layout_count == 2);
            }
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE)
        {
            found_vertex_shader =
                found_vertex_shader ||
                (strcmp(cmd->u.create_shader_module.stage, "VERTEX") == 0 &&
                 strcmp(cmd->u.create_shader_module.format, "wgsl") == 0 &&
                 cmd->u.create_shader_module.code != NULL &&
                 strstr(cmd->u.create_shader_module.code, "image_pixel_anchor") != NULL);
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER)
        {
            if (cmd->u.set_vertex_buffer.slot == 0)
                position_buffer_id = cmd->u.set_vertex_buffer.buffer_id;
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_DRAW)
        {
            found_draw = cmd->u.draw.vertex_count == 6 && cmd->u.draw.instance_count == 1;
        }
    }
    AT(position_buffer_id != 0);

    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        ANN(cmd);
        if (cmd->type != DVZ_DRP2_COMMAND_WRITE_BUFFER ||
            cmd->u.write_buffer.buffer_id != position_buffer_id ||
            cmd->u.write_buffer.data_raw == NULL)
            continue;
        const float* pos = (const float*)cmd->u.write_buffer.data_raw;
        const float expected[18] = {
            10.0f, 20.0f, 0.5f, 10.0f, 60.0f, 0.5f, 90.0f, 20.0f, 0.5f,
            90.0f, 20.0f, 0.5f, 10.0f, 60.0f, 0.5f, 90.0f, 60.0f, 0.5f,
        };
        found_position_values = cmd->u.write_buffer.size == sizeof(expected);
        for (uint32_t j = 0; j < 18 && found_position_values; j++)
            found_position_values = pos[j] == expected[j];
    }

    AT(found_pipeline);
    AT(found_draw);
    AT(found_vertex_shader);
    AT(found_position_values);

    char* json = dvz_drp2_stream_json(stream, "scene_image_pixel_anchor_wgsl_from_c");
    ANN(json);
    AT(strstr(json, "\"format\": \"wgsl\"") != NULL);
    AT(strstr(json, "image_pixel_anchor") != NULL);
    AT(strstr(json, "\"topology\": \"triangle-list\"") != NULL);
    AT(strstr(json, "\"bind_group_layout_ids\": [") != NULL);
    dvz_drp2_stream_json_destroy(json);

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
int test_scene_glyph_emit_glsl(TstContext* suite, const TstCase* item)
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

    vec3 positions[6] = {{0.0f, 0.0f, 0.0f}};
    vec4 bounds[6] = {
        {-16.0f, -16.0f, 16.0f, 16.0f}, {-16.0f, -16.0f, 16.0f, 16.0f},
        {-16.0f, -16.0f, 16.0f, 16.0f}, {-16.0f, -16.0f, 16.0f, 16.0f},
        {-16.0f, -16.0f, 16.0f, 16.0f}, {-16.0f, -16.0f, 16.0f, 16.0f},
    };
    vec4 texcoords[6] = {
        {0.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f},
        {0.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f},
        {0.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f},
    };
    DvzColor colors[6] = {
        {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
        {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    };
    float angles[6] = {0};
    uint8_t pixels[4 * 4 * 4];
    dvz_memset(pixels, sizeof(pixels), 255, sizeof(pixels));

    AT(dvz_visual_set_data(visual, "position", positions, 6) == 0);
    AT(dvz_visual_set_data(visual, "bounds", bounds, 6) == 0);
    AT(dvz_visual_set_data(visual, "texcoords", texcoords, 6) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 6) == 0);
    AT(dvz_visual_set_data(visual, "angle", angles, 6) == 0);
    AT(dvz_visual_set_texture(visual, pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);
    AT(_stream_has_render_pipeline_label(stream, "_pipe_glyphg"));

    uint64_t position_buffer_id = 0;
    uint64_t bounds_buffer_id = 0;
    uint64_t uv_buffer_id = 0;
    uint64_t color_buffer_id = 0;
    uint64_t angle_buffer_id = 0;
    bool found_pipeline = false;
    bool found_draw = false;
    bool found_texture_bind = false;
    bool found_glyph_layout = false;
    bool found_glyph_bind_group = false;
    bool found_glyph_params_write = false;
    uint64_t glyph_params_buffer_id = 0;
    bool found_position_upload = false;
    bool found_bounds_upload = false;
    bool found_uv_upload = false;
    bool found_color_upload = false;
    bool found_angle_upload = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        ANN(cmd);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            found_pipeline =
                cmd->u.create_render_pipeline.vertex_buffer_slots == 5 &&
                cmd->u.create_render_pipeline.topology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST &&
                cmd->u.create_render_pipeline.binding_count == 5 &&
                cmd->u.create_render_pipeline.attr_count == 5 &&
                cmd->u.create_render_pipeline.bind_group_layout_count == 2;
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_CREATE_BIND_GROUP_LAYOUT)
        {
            const char* label =
                dvz_drp2_stream_label(stream, cmd->u.create_bind_group_layout.id);
            if (label != NULL && strcmp(label, "_bgl_glyph") == 0)
            {
                found_glyph_layout =
                    cmd->u.create_bind_group_layout.entry_count == 3 &&
                    cmd->u.create_bind_group_layout.entries[0].binding_type ==
                        DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE &&
                    cmd->u.create_bind_group_layout.entries[1].binding_type ==
                        DVZ_DRP2_BINDING_TYPE_SAMPLER &&
                    cmd->u.create_bind_group_layout.entries[2].binding_type ==
                        DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER;
            }
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_CREATE_BIND_GROUP)
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.create_bind_group.id);
            if (label != NULL && strncmp(label, "_bg_glyph_", 10) == 0)
            {
                found_glyph_bind_group =
                    cmd->u.create_bind_group.entry_count == 3 &&
                    cmd->u.create_bind_group.entries[2].binding_type ==
                        DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER &&
                    cmd->u.create_bind_group.entries[2].size == 4 * sizeof(float);
                glyph_params_buffer_id = cmd->u.create_bind_group.entries[2].resource_id;
            }
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER)
        {
            if (cmd->u.set_vertex_buffer.slot == 0)
                position_buffer_id = cmd->u.set_vertex_buffer.buffer_id;
            else if (cmd->u.set_vertex_buffer.slot == 1)
                bounds_buffer_id = cmd->u.set_vertex_buffer.buffer_id;
            else if (cmd->u.set_vertex_buffer.slot == 2)
                uv_buffer_id = cmd->u.set_vertex_buffer.buffer_id;
            else if (cmd->u.set_vertex_buffer.slot == 3)
                color_buffer_id = cmd->u.set_vertex_buffer.buffer_id;
            else if (cmd->u.set_vertex_buffer.slot == 4)
                angle_buffer_id = cmd->u.set_vertex_buffer.buffer_id;
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
    AT(bounds_buffer_id != 0);
    AT(uv_buffer_id != 0);
    AT(color_buffer_id != 0);
    AT(angle_buffer_id != 0);

    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        ANN(cmd);
        if (cmd->type != DVZ_DRP2_COMMAND_WRITE_BUFFER)
            continue;
        if (cmd->u.write_buffer.buffer_id == glyph_params_buffer_id)
            found_glyph_params_write = cmd->u.write_buffer.size == 4 * sizeof(float);
        if (cmd->u.write_buffer.buffer_id == position_buffer_id)
            found_position_upload = cmd->u.write_buffer.size == 6 * 3 * sizeof(float);
        if (cmd->u.write_buffer.buffer_id == bounds_buffer_id)
            found_bounds_upload = cmd->u.write_buffer.size == 6 * 4 * sizeof(float);
        if (cmd->u.write_buffer.buffer_id == uv_buffer_id)
            found_uv_upload = cmd->u.write_buffer.size == 6 * 4 * sizeof(float);
        if (cmd->u.write_buffer.buffer_id == color_buffer_id)
            found_color_upload = cmd->u.write_buffer.size == 6 * sizeof(DvzColor);
        if (cmd->u.write_buffer.buffer_id == angle_buffer_id)
            found_angle_upload = cmd->u.write_buffer.size == 6 * sizeof(float);
    }

    AT(found_pipeline);
    AT(found_draw);
    AT(found_texture_bind);
    AT(found_glyph_layout);
    AT(found_glyph_bind_group);
    AT(found_glyph_params_write);
    AT(found_position_upload);
    AT(found_bounds_upload);
    AT(found_uv_upload);
    AT(found_color_upload);
    AT(found_angle_upload);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_image_emit_wgsl(TstContext* suite, const TstCase* item)
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

    vec3 positions[4] = {
        {-0.5f, -0.5f, 0.0f}, {-0.5f, 0.5f, 0.0f},
        { 0.5f, -0.5f, 0.0f}, { 0.5f, 0.5f, 0.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };
    uint8_t pixels[4 * 4 * 4];
    dvz_memset(pixels, sizeof(pixels), 128, sizeof(pixels));

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_texture(visual, pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
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


/**
 * Return whether a stream creates a texture with the expected format and extent.
 *
 * @param stream the emitted command stream
 * @param format expected Vulkan texture format
 * @param width expected texture width
 * @param height expected texture height
 * @return whether a matching texture command was found
 */
static bool _stream_has_texture_format(
    const DvzDrp2CommandStream* stream, VkFormat format, uint32_t width, uint32_t height)
{
    ANN(stream);
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd == NULL || cmd->type != DVZ_DRP2_COMMAND_CREATE_TEXTURE)
            continue;
        if (cmd->u.create_texture.format == format && cmd->u.create_texture.width == width &&
            cmd->u.create_texture.height == height && cmd->u.create_texture.depth == 1)
            return true;
    }
    return false;
}



/**
 * Return whether a stream uploads one texture region with the expected row layout.
 *
 * @param stream the emitted command stream
 * @param width expected upload width
 * @param height expected upload height
 * @param bytes_per_row expected row byte count
 * @return whether a matching upload command was found
 */
static bool _stream_has_texture_upload(
    const DvzDrp2CommandStream* stream, uint32_t width, uint32_t height, uint64_t bytes_per_row)
{
    ANN(stream);
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd == NULL || cmd->type != DVZ_DRP2_COMMAND_WRITE_TEXTURE)
            continue;
        if (cmd->u.write_texture.width == width && cmd->u.write_texture.height == height &&
            cmd->u.write_texture.depth == 1 && cmd->u.write_texture.bytes_per_row == bytes_per_row)
            return true;
    }
    return false;
}



/**
 * Log diagnostics before a focused labels test fails.
 *
 * @param report diagnostic report
 */
static void _labels_log_diagnostics(const DvzDiagnosticReport* report)
{
    ANN(report);
    uint32_t count = dvz_diagnostic_report_count(report);
    for (uint32_t i = 0; i < count; i++)
    {
        const char* message = dvz_diagnostic_report_get(report, i);
        if (message != NULL)
            log_error("%s", message);
    }
}



/**
 * Log render pipeline labels in a labels stream test.
 *
 * @param stream the emitted command stream
 */
static void _labels_log_pipeline_labels(const DvzDrp2CommandStream* stream)
{
    ANN(stream);
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd == NULL || cmd->type != DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
            continue;
        const char* label = dvz_drp2_stream_label(stream, cmd->u.create_render_pipeline.id);
        log_error("pipeline: %s", label != NULL ? label : "(none)");
    }
}


/**
 * Return whether a stream contains the labels texture/sampler/params bind-group layout.
 *
 * @param stream the emitted command stream
 * @return whether the labels layout is present
 */
static bool _labels_stream_has_params_layout(const DvzDrp2CommandStream* stream)
{
    ANN(stream);
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd == NULL || cmd->type != DVZ_DRP2_COMMAND_CREATE_BIND_GROUP_LAYOUT)
            continue;
        if (cmd->u.create_bind_group_layout.entry_count != 3)
            continue;
        const DvzDrp2BindGroupLayoutEntry* entries = cmd->u.create_bind_group_layout.entries;
        if (entries[0].binding == 0 &&
            entries[0].binding_type == DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE &&
            entries[1].binding == 1 &&
            entries[1].binding_type == DVZ_DRP2_BINDING_TYPE_SAMPLER &&
            entries[2].binding == 2 &&
            entries[2].binding_type == DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER)
        {
            return true;
        }
    }
    return false;
}


/**
 * Return whether a stream writes the expected labels presentation uniform.
 *
 * @param stream the emitted command stream
 * @return whether the labels uniform write is present
 */
static bool _labels_stream_has_params_write(const DvzDrp2CommandStream* stream)
{
    ANN(stream);
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd == NULL || cmd->type != DVZ_DRP2_COMMAND_WRITE_BUFFER ||
            cmd->u.write_buffer.size != sizeof(DvzSceneLabelsUniform))
        {
            continue;
        }
        const DvzSceneLabelsUniform* uniform =
            (const DvzSceneLabelsUniform*)cmd->u.write_buffer.data_raw;
        if (uniform == NULL)
            continue;
        if (uniform->ids[0] == 0u && uniform->ids[1] == 17u &&
            uniform->params[0] ==
                (DVZ_SCENE_LABELS_FLAG_SELECTED | DVZ_SCENE_LABELS_FLAG_BOUNDARY) &&
            uniform->params[1] == 123u && uniform->params[2] == 2u &&
            fabsf(uniform->floats[0] - 0.55f) < 1e-6f &&
            fabsf(uniform->floats[1] - 2.0f) < 1e-6f &&
            uniform->hidden_ids[0][0] == (uint32_t)(int32_t)-7 &&
            uniform->hidden_ids[0][1] == 1009u)
        {
            return true;
        }
    }
    return false;
}



/**
 * Build one retained labels visual bound to a categorical scale.
 *
 * @param scene owning scene
 * @param format integer field format
 * @param values field payload
 * @param bytes_per_row field row byte count
 * @param out_figure output configured figure
 * @return 0 on success
 */
static int _labels_emit_figure(
    DvzScene* scene, DvzFieldFormat format, const void* values, uint64_t bytes_per_row,
    DvzFigure** out_figure)
{
    ANN(scene);
    ANN(values);
    ANN(out_figure);

    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* labels = dvz_labels(scene, 0);
    ANN(labels);
    DvzCategoryId hidden[2] = {-7, 1009};
    DvzColor boundary = {255, 244, 64, 245};
    AT(dvz_labels_set_opacity(labels, 0.55f) == 0);
    AT(dvz_labels_set_selected(labels, 17) == 0);
    AT(dvz_labels_set_hidden(labels, hidden, 2) == 0);
    AT(dvz_labels_set_boundary(labels, true, 2.0f, boundary) == 0);
    AT(dvz_labels_set_fallback_seed(labels, 123) == 0);

    vec3 positions[1] = {{0.0f, 0.0f, 0.0f}};
    vec2 extents[1] = {{2.0f, 2.0f}};
    AT(dvz_visual_set_data(labels, "position", positions, 1) == 0);
    AT(dvz_visual_set_data(labels, "extent", extents, 1) == 0);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = format,
                   .semantic = DVZ_FIELD_SEMANTIC_LABEL,
                   .width = 2,
                   .height = 2,
                   .depth = 1,
               });
    ANN(field);
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView),
                   .data = values,
                   .bytes_per_row = bytes_per_row,
                   .rows_per_image = 2,
               }));
    AT(dvz_visual_set_field(labels, "field", field));

    DvzScale* scale = dvz_scale(scene, &(DvzScaleDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleDesc), .kind = DVZ_SCALE_CATEGORICAL});
    ANN(scale);
    DvzScaleCategory categories[3] = {
        {.category_id = -7, .order = 0, .label = "negative", .color = {200, 40, 40, 180}},
        {.category_id = 17, .order = 1, .label = "cell 17", .color = {40, 180, 80, 180}},
        {.category_id = 4000000000LL, .order = 2, .label = "large", .color = {40, 80, 220, 180}},
    };
    AT(dvz_scale_set_categories(scale, categories, 3));
    AT(dvz_visual_set_scale(labels, "labels", scale) == 0);
    AT(dvz_panel_add_visual(panel, labels, NULL) == 0);

    *out_figure = figure;
    return 0;
}



int test_scene_labels_emit_signed_glsl(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    int32_t values[4] = {0, -7, 17, -100};
    DvzFigure* figure = NULL;
    AT(_labels_emit_figure(
           scene, DVZ_FIELD_FORMAT_R32_SINT, values, 2 * sizeof(int32_t), &figure) == 0);
    ANN(figure);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.supports_color_blending = true;
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    if (dvz_diagnostic_report_count(&report) != 0)
        _labels_log_diagnostics(&report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);
    AT(_stream_has_render_pipeline_label_part(stream, "_pipe_labels_sintg"));
    AT(!_stream_has_render_pipeline_label(stream, "_pipe_imgg"));
    AT(_labels_stream_has_params_layout(stream));
    AT(_labels_stream_has_params_write(stream));
    AT(_stream_has_texture_format(stream, VK_FORMAT_R32_SINT, 2, 2));
    AT(_stream_has_texture_upload(stream, 2, 2, 2 * sizeof(int32_t)));

    char* json = dvz_drp2_stream_json(stream, "scene_labels_signed_glsl_from_c");
    ANN(json);
    AT(strstr(json, "\"format\": \"spirv\"") != NULL ||
       strstr(json, "\"format\": \"glsl\"") != NULL);
    AT(strstr(json, "\"mag_filter\": \"nearest\"") != NULL);
    AT(strstr(json, "\"min_filter\": \"nearest\"") != NULL);
    dvz_drp2_stream_json_destroy(json);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_labels_emit_unsigned_glsl(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    uint32_t values[4] = {0, 17, 1009, 4000000000u};
    DvzFigure* figure = NULL;
    AT(_labels_emit_figure(
           scene, DVZ_FIELD_FORMAT_R32_UINT, values, 2 * sizeof(uint32_t), &figure) == 0);
    ANN(figure);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.supports_color_blending = true;
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    if (dvz_diagnostic_report_count(&report) != 0)
        _labels_log_diagnostics(&report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);
    AT(_stream_has_render_pipeline_label_part(stream, "_pipe_labels_uintg"));
    AT(!_stream_has_render_pipeline_label(stream, "_pipe_imgg"));
    AT(_labels_stream_has_params_layout(stream));
    AT(_labels_stream_has_params_write(stream));
    AT(_stream_has_texture_format(stream, VK_FORMAT_R32_UINT, 2, 2));
    AT(_stream_has_texture_upload(stream, 2, 2, 2 * sizeof(uint32_t)));

    char* json = dvz_drp2_stream_json(stream, "scene_labels_unsigned_glsl_from_c");
    ANN(json);
    AT(strstr(json, "\"format\": \"spirv\"") != NULL ||
       strstr(json, "\"format\": \"glsl\"") != NULL);
    dvz_drp2_stream_json_destroy(json);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_labels_emit_wgsl(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    int32_t values[4] = {0, -7, 17, -100};
    DvzFigure* figure = NULL;
    AT(_labels_emit_figure(
           scene, DVZ_FIELD_FORMAT_R32_SINT, values, 2 * sizeof(int32_t), &figure) == 0);
    ANN(figure);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.supports_color_blending = true;
    caps.shader_format_wgsl = true;
    caps.shader_format_glsl = false;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups = 4;
    caps.max_buffer_size = 256 * 1024 * 1024;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_WGSL;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    if (dvz_diagnostic_report_count(&report) != 0)
        _labels_log_diagnostics(&report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);
    if (!_stream_has_render_pipeline_label_part(stream, "_pipe_labels_sintw"))
        _labels_log_pipeline_labels(stream);
    AT(_stream_has_render_pipeline_label_part(stream, "_pipe_labels_sintw"));
    AT(_labels_stream_has_params_layout(stream));
    AT(_labels_stream_has_params_write(stream));
    AT(_stream_has_texture_format(stream, VK_FORMAT_R32_SINT, 2, 2));

    char* json = dvz_drp2_stream_json(stream, "scene_labels_wgsl_from_c");
    ANN(json);
    AT(strstr(json, "\"format\": \"wgsl\"") != NULL);
    AT(strstr(json, "texture_2d<i32>") != NULL);
    AT(strstr(json, "textureLoad") != NULL);
    AT(strstr(json, "@group(1) @binding(0)") != NULL);
    AT(strstr(json, "@group(1) @binding(2)") != NULL);
    AT(strstr(json, "LabelsParams") != NULL);
    dvz_drp2_stream_json_destroy(json);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_image_emit_uses_common_and_texture_sets(TstContext* suite, const TstCase* item)
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

    vec3 positions[4] = {
        {-0.5f, -0.5f, 0.0f}, {-0.5f, 0.5f, 0.0f},
        { 0.5f, -0.5f, 0.0f}, { 0.5f, 0.5f, 0.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };
    uint8_t pixels[4 * 4 * 4];
    dvz_memset(pixels, sizeof(pixels), 128, sizeof(pixels));

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_texture(visual, pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
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
int test_scene_visual_common_binding_layout_order(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);

    vec3 point_pos[3] = {
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

    vec3 prim_pos[3] = {
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

    vec3 path_pos[4] = {
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

    vec3 image_pos[4] = {
        {0.2f, -0.8f, 0.0f}, {0.2f, -0.6f, 0.0f},
        {0.4f, -0.8f, 0.0f}, {0.4f, -0.6f, 0.0f},
    };
    vec2 image_uv[4] = {
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

    vec3 mesh_pos[4] = {
        {0.55f, -0.8f, 0.0f}, {0.55f, -0.6f, 0.0f},
        {0.75f, -0.8f, 0.0f}, {0.75f, -0.6f, 0.0f},
    };
    vec3 mesh_normal[4] = {
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
    };
    DvzIndex mesh_index[6] = {0, 1, 2, 2, 1, 3};
    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
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

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
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


int test_scene_empty_figure_emit_clear_only(TstContext* suite, const TstCase* item)
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

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
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


int test_scene_point_emit_has_vertex_layout(TstContext* suite, const TstCase* item)
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


int test_scene_indexed_primitive_material_updates_runtime(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_GRAPH_REQUIRE_VKLITE(suite);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
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
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)));

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 4) == 0);
    AT(dvz_visual_set_data(visual, "normal", normals, 4) == 0);
    AT(dvz_visual_set_buffer(visual, "index", index_buffer));
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);
    AT(_test_set_phong_material(
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

    AT(_test_set_phong_material(
           visual, (float[3]){0.0f, 0.0f, 1.0f}, 1.0f, 0.0f, 0.25f, 32.0f) == 0);

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


int test_scene_second_emit_no_uploads_when_not_dirty(TstContext* suite, const TstCase* item)
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
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
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
        field, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView), .data = voxels, .bytes_per_row = 2, .rows_per_image = 2}));

    DvzVisual* volume = dvz_volume(scene, 0);
    AT(volume != NULL);
    AT(dvz_visual_set_field(volume, "field", field));
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
    DvzDrp2CommandStream* stream1 = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream1 != NULL);
    dvz_drp2_stream_destroy(stream1);
    AT(!_scene_figure_has_pending_render_work(figure));

    AT(dvz_volume_set_opacity(volume, 0.35f) == 0);
    AT(_scene_figure_has_pending_render_work(figure));
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream2 = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream2 != NULL);
    dvz_drp2_stream_destroy(stream2);
    AT(!_scene_figure_has_pending_render_work(figure));

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure retained labels parameter mutations keep on-demand scheduling dirty until emitted.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_pending_render_work_tracks_labels_state(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    int32_t values[4] = {0, -7, 17, -100};
    DvzFigure* figure = NULL;
    AT(_labels_emit_figure(
           scene, DVZ_FIELD_FORMAT_R32_SINT, values, 2 * sizeof(int32_t), &figure) == 0);
    AT(figure != NULL);
    AT(figure->panel_count == 1);
    DvzPanel* panel = &figure->panels[0];
    AT(panel->visual_count == 1);
    DvzVisual* labels = panel->visuals[0].visual;
    AT(labels != NULL);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_glsl = true;
    caps.supports_color_blending = true;
    DvzDiagnosticReport report;

    AT(_scene_figure_has_pending_render_work(figure));
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 = dvz_figure_emit(figure, &caps, &report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream1 != NULL);
    dvz_drp2_stream_destroy(stream1);
    AT(!_scene_figure_has_pending_render_work(figure));

    AT(dvz_labels_set_opacity(labels, 0.25f) == 0);
    AT(_scene_figure_has_pending_render_work(figure));
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream2 = dvz_figure_emit(figure, &caps, &report);
    if (dvz_diagnostic_report_count(&report) != 0)
        _labels_log_diagnostics(&report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream2 != NULL);
    dvz_drp2_stream_destroy(stream2);
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
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    dvz_panel_set_background_color(panel, 0.02f, 0.03f, 0.04f, 1.0f);
    AT(panel->background_visual != NULL);
    AT(_scene_figure_has_pending_render_work(figure));

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.shader_format_glsl = true;
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = dvz_figure_emit(figure, &caps, &report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);
    dvz_drp2_stream_destroy(stream);

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
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
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
    dvz_visual_set_visible(visual, false);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
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


int test_scene_hidden_indexed_mesh_first_visible_later_uploads(TstContext* suite, const TstCase* item)
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
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)));
    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "normal", normals, 4) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 4) == 0);
    AT(dvz_visual_set_buffer(visual, "index", index_buffer));
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);
    dvz_visual_set_visible(visual, false);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
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
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
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
        field, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView), .data = voxels, .bytes_per_row = 2, .rows_per_image = 2}));

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


int test_scene_partial_update_uploads_only_range(TstContext* suite, const TstCase* item)
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


int test_scene_repeated_partial_updates_across_frames(TstContext* suite, const TstCase* item)
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


int test_scene_partial_update_merges_ranges_before_emit(TstContext* suite, const TstCase* item)
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


int test_scene_multiple_panels_multiple_point_visuals_emit(TstContext* suite, const TstCase* item)
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

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
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
    uint32_t viewport_count = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream1); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream1, i);
        if (cmd == NULL)
            continue;
        if (cmd->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS)
        {
            AC(cmd->u.begin_render_pass.viewport[0], 0.0f, 1e-6f);
            AC(cmd->u.begin_render_pass.viewport[1], 0.0f, 1e-6f);
            AC(cmd->u.begin_render_pass.viewport[2], 1.0f, 1e-6f);
            AC(cmd->u.begin_render_pass.viewport[3], 1.0f, 1e-6f);
            AT(cmd->u.begin_render_pass.clear);
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
    AT(begin_render_pass_count == 1);
    AT(viewport_count == 2);
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
    AT(dvz_visual_set_data(point, "diameter", diameters, 1) == 0);
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

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);
    dvz_drp2_stream_destroy(stream);

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
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
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
int test_scene_visual_scene_occlusion_emits_drp2(TstContext* suite, const TstCase* item)
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
int test_scene_volume_slice_uses_volume_occlusion(TstContext* suite, const TstCase* item)
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
        field, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView), .data = voxels, .bytes_per_row = 2, .rows_per_image = 2}));

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

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
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
int test_scene_volume_slice_uses_generic_scene_occlusion(TstContext* suite, const TstCase* item)
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
        field, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView), .data = voxels, .bytes_per_row = 2, .rows_per_image = 2}));

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

    AT(_test_set_phong_material(
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
    AT(defaults.light_direction[2] == 1.0f);
    AT(defaults.phong.ambient == 0.2f);
    AT(defaults.phong.diffuse == 0.8f);
    AT(defaults.phong.specular == 0.25f);
    AT(defaults.phong.shininess == 32.0f);
    AT(defaults.standard.roughness == 0.5f);
    AT(defaults.standard.specular == 0.5f);
    DvzMaterialDesc phong_defaults = dvz_phong_material_desc();
    AT(phong_defaults.model == DVZ_MATERIAL_MODEL_PHONG);
    DvzMaterialDesc standard_defaults = dvz_standard_material_desc();
    AT(standard_defaults.model == DVZ_MATERIAL_MODEL_STANDARD);
    AT(standard_defaults.standard.roughness == 0.5f);

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
    AT(_visual_family_state(mesh)->material_params.params[0] == 0.2f);
    AT(_visual_family_state(mesh)->material_params.params[1] == 0.8f);

    AT(_test_set_phong_material(
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
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
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
int test_scene_visual_pass_capabilities(TstContext* suite, const TstCase* item)
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
