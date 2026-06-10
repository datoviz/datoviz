/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene technique graph tests                                                                   */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene_graph_utils.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

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



int test_scene_draw_contract_resolver_matrix(TstContext* suite, const TstCase* item)
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
        .visual_type = DVZ_VISUAL_TYPE_SEGMENT,
        .alpha_mode = DVZ_ALPHA_OPAQUE,
        .can_depth_test = true,
        .can_write_depth = true,
        .writes_depth = true,
        .uses_segment_pipeline = true,
        .uses_common_set = true,
        .uses_material_set = true,
    };
    AT(_scene_draw_contract_resolve(
        &facts, DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE, &contract));
    AT(contract.visual_type == DVZ_VISUAL_TYPE_SEGMENT);
    AT(contract.depth_test);
    AT(contract.depth_write);
    AT(contract.blend_policy == DVZ_SCENE_BLEND_POLICY_SEGMENT_COVERAGE);
    AT(contract.blend_target_count == 1);
    AT(contract.blend_targets[0].blend_enabled);
    AT(
        contract.blend_targets[0].src_color_blend_factor ==
        VK_BLEND_FACTOR_SRC_ALPHA);
    AT(
        contract.blend_targets[0].dst_color_blend_factor ==
        VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);

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
int test_scene_role_work_label_mapping_complete(TstContext* suite, const TstCase* item)
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
    AT(policy.sampled_texture_binding_count == 2);

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
int test_scene_render_contract_validation_errors(TstContext* suite, const TstCase* item)
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
int test_scene_frame_plan_missing_graph_pass_fails_contract(TstContext* suite, const TstCase* item)
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

    _test_scene_stream_destroy(stream);
    dvz_frame_plan_destroy(plan);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify render contracts reject missing typed visual metadata.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_render_contract_rejects_untyped_visual_metadata(
    TstContext* suite, const TstCase* item)
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
        plan, "figure_0_p0", "target.panel.0.color", false, panel->desc,
        DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE));
    AT(dvz_frame_plan_render_visual(plan, "visual.compat.0"));
    const DvzFramePlanNode* render = dvz_frame_plan_node_get(plan, 0);
    ANN(render);

    DvzScenePassContract contract = {0};
    AT(!_scene_pass_contract_from_render(plan, panel, render, NULL, &contract));

    dvz_frame_plan_destroy(plan);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify panel graph-emission failures are threaded into diagnostics.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_panel_graph_failure_reports_specific_diagnostic(TstContext* suite, const TstCase* item)
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

    DvzFramePlan* plan = dvz_frame_plan("figure_0", 0);
    ANN(plan);
    DvzFrameGraphPass saturated = {0};
    dvz_strlcpy(saturated.id, "figure_0_p0.synthetic_full_reads", sizeof(saturated.id));
    dvz_strlcpy(saturated.panel_id, "figure_0_p0", sizeof(saturated.panel_id));
    dvz_strlcpy(saturated.work_label, "opaque", sizeof(saturated.work_label));
    saturated.kind = DVZ_FRAME_GRAPH_PASS_COMPUTE;
    saturated.read_count = DVZ_FRAME_PLAN_MAX_GRAPH_ACCESSES;
    for (uint32_t i = 0; i < DVZ_FRAME_PLAN_MAX_GRAPH_ACCESSES; i++)
    {
        dvz_snprintf(
            saturated.reads[i].resource_id, sizeof(saturated.reads[i].resource_id),
            "synthetic.read.%u", i);
        saturated.reads[i].usage = DVZ_FRAME_GRAPH_ACCESS_SAMPLED;
    }
    AT(dvz_frame_plan_graph_pass(plan, &saturated));

    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    AT_EXPECTED_ERROR_STRICT(
        suite, !_scene_emit_panel_render_ex(figure, 0, plan, "figure_0", &report));
    AT(dvz_diagnostic_report_count(&report) == 1);
    const char* message = dvz_diagnostic_report_get(&report, 0);
    ANN(message);
    AT(strcmp(message, "failed to add volume occlusion FramePlan reads for panel figure_0_p0") == 0);

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
int test_scene_gbuffer_runtime_lowering(TstContext* suite, const TstCase* item)
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

    vec3 positions[4] = {
        {-0.5f, -0.5f, 0.0f},
        {0.5f, -0.5f, 0.0f},
        {-0.5f, 0.5f, 0.0f},
        {0.5f, 0.5f, 0.0f},
    };
    vec3 normals[4] = {
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
    };
    DvzIndex indices[6] = {0, 1, 2, 2, 1, 3};
    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
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
    caps = dvz_capability_snapshot();
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
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

    _test_scene_stream_destroy(stream);
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
int test_scene_frame_plan_node_reallocation_safe(TstContext* suite, const TstCase* item)
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

    vec3 positions[4] = {
        {-0.5f, -0.5f, 0.0f},
        {0.5f, -0.5f, 0.0f},
        {-0.5f, 0.5f, 0.0f},
        {0.5f, 0.5f, 0.0f},
    };
    vec3 normals[4] = {
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
    };
    DvzIndex indices[6] = {0, 1, 2, 2, 1, 3};
    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
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

    vec3 point_positions[3] = {
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
int test_scene_msaa_runtime_lowering(TstContext* suite, const TstCase* item)
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
        panel, &(DvzMsaaDesc){DVZ_STRUCT_INIT_FIELDS(DvzMsaaDesc), .enabled = true, .sample_count = 4, .alpha_to_coverage = true}));

    DvzVisual* sphere = dvz_sphere(scene, 0);
    AT(sphere != NULL);
    vec3 positions[1] = {{0.0f, 0.0f, 0.0f}};
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
    caps = dvz_capability_snapshot();
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
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

    _test_scene_stream_destroy(stream);
    dvz_frame_plan_destroy(plan);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Verify ordinary blended overlays do not disable MSAA on the preceding opaque pass.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_msaa_blended_overlay_runtime_lowering(TstContext* suite, const TstCase* item)
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
        panel,
        &(DvzMsaaDesc){DVZ_STRUCT_INIT_FIELDS(DvzMsaaDesc),
                       .enabled = true,
                       .sample_count = 4,
                       .alpha_to_coverage = true}));

    DvzVisual* sphere = dvz_sphere(scene, 0);
    AT(sphere != NULL);
    vec3 sphere_positions[1] = {{0.0f, 0.0f, 0.0f}};
    DvzColor sphere_colors[1] = {{255, 128, 64, 255}};
    float sphere_sizes[1] = {0.35f};
    AT(dvz_visual_set_data(sphere, "position", sphere_positions, 1) == 0);
    AT(dvz_visual_set_data(sphere, "color", sphere_colors, 1) == 0);
    AT(dvz_visual_set_data(sphere, "size", sphere_sizes, 1) == 0);
    AT(dvz_panel_add_visual(panel, sphere, NULL) == 0);

    DvzVisual* overlay = dvz_point(scene, 0);
    AT(overlay != NULL);
    vec3 overlay_positions[2] = {{-0.25f, 0.0f, 0.2f}, {0.25f, 0.0f, 0.2f}};
    DvzColor overlay_colors[2] = {{0, 255, 255, 160}, {0, 255, 255, 160}};
    float overlay_sizes[2] = {8.0f, 8.0f};
    AT(dvz_visual_set_data(overlay, "position", overlay_positions, 2) == 0);
    AT(dvz_visual_set_data(overlay, "color", overlay_colors, 2) == 0);
    AT(dvz_visual_set_data(overlay, "size", overlay_sizes, 2) == 0);
    AT(dvz_visual_set_depth_test(overlay, false) == 0);
    AT(dvz_visual_set_alpha_mode(overlay, DVZ_ALPHA_BLENDED) == 0);
    AT(dvz_panel_add_visual(panel, overlay, NULL) == 0);

    DvzFramePlan* plan = dvz_frame_plan("figure.msaa.blended", 0);
    ANN(plan);
    AT(_scene_emit_panel_render(figure, 0, plan, "figure_0"));
    AT(dvz_frame_plan_node_count(plan) == 2);
    AT(dvz_frame_plan_graph_pass_count(plan) == 2);

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

    const DvzFrameGraphPass* opaque_pass = dvz_frame_plan_graph_pass_get(plan, 0);
    const DvzFrameGraphPass* blended_pass = dvz_frame_plan_graph_pass_get(plan, 1);
    ANN(opaque_pass);
    ANN(blended_pass);
    AT(strcmp(opaque_pass->work_label, "opaque") == 0);
    AT(strcmp(opaque_pass->color_attachments[0].resource_id, "figure_0_p0.msaa.color") == 0);
    AT(strcmp(opaque_pass->color_attachments[0].resolve_resource_id, "rt") == 0);
    AT(opaque_pass->color_attachments[0].resolve_mode == VK_RESOLVE_MODE_AVERAGE_BIT);
    AT(strcmp(blended_pass->work_label, "transparent_blend") == 0);
    AT(strcmp(blended_pass->color_attachments[0].resource_id, "rt") == 0);
    AT(!blended_pass->has_depth_attachment);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.supports_color_blending = true;
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.target_width = 64;
    cfg.target_height = 64;
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);
    DvzDrp2ValidationResult validation = dvz_drp2_validate_stream(stream);
    AT(validation.ok);

    uint64_t msaa_texture_id = 0;
    bool found_msaa_texture = false;
    bool found_resolve_pass = false;
    bool found_msaa_sphere_pipeline = false;
    bool found_single_sample_blended_pipeline = false;
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
                found_msaa_texture = cmd->u.create_texture.sample_count == 4;
            }
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS)
        {
            found_resolve_pass =
                found_resolve_pass ||
                (cmd->u.begin_render_pass.texture_id == msaa_texture_id &&
                 cmd->u.begin_render_pass.color_attachments[0].resolve_texture_id != 0);
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.create_render_pipeline.id);
            found_msaa_sphere_pipeline =
                found_msaa_sphere_pipeline ||
                (label != NULL && strstr(label, "_pipe_sphere") != NULL &&
                 cmd->u.create_render_pipeline.sample_count == 4 &&
                 cmd->u.create_render_pipeline.alpha_to_coverage_enabled);
            found_single_sample_blended_pipeline =
                found_single_sample_blended_pipeline ||
                (label != NULL && strstr(label, "_pipe_point") != NULL &&
                 cmd->u.create_render_pipeline.sample_count == 1 &&
                 cmd->u.create_render_pipeline.color_targets[0].blend_enabled);
        }
    }
    AT(found_msaa_texture);
    AT(found_resolve_pass);
    AT(found_msaa_sphere_pipeline);
    AT(found_single_sample_blended_pipeline);

    _test_scene_stream_destroy(stream);
    dvz_frame_plan_destroy(plan);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Verify the protein-style MSAA + SSAO + blended overlay graph leaves overlays last.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_msaa_ssao_blended_overlay_runtime_lowering(TstContext* suite, const TstCase* item)
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
        panel,
        &(DvzMsaaDesc){DVZ_STRUCT_INIT_FIELDS(DvzMsaaDesc),
                       .enabled = true,
                       .sample_count = 16,
                       .alpha_to_coverage = true}));
    AT(_scene_technique_state_set_ssao(
        &panel->techniques,
        &(DvzSceneSsaoDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneSsaoDesc),
                            .radius = 1.0f,
                            .strength = 2.5f,
                            .bias = 0.02f,
                            .sample_count = 16,
                            .blur_enabled = true}));

    DvzVisual* sphere = dvz_sphere(scene, DVZ_SPHERE_FLAGS_LIGHTING);
    AT(sphere != NULL);
    AT(dvz_sphere_mode(sphere, DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR) == 0);
    vec3 sphere_positions[1] = {{0.0f, 0.0f, 0.0f}};
    DvzColor sphere_colors[1] = {{255, 128, 64, 255}};
    float sphere_radii[1] = {0.35f};
    AT(dvz_visual_set_data(sphere, "position", sphere_positions, 1) == 0);
    AT(dvz_visual_set_data(sphere, "color", sphere_colors, 1) == 0);
    AT(dvz_visual_set_data(sphere, "radius", sphere_radii, 1) == 0);
    AT(dvz_panel_add_visual(panel, sphere, NULL) == 0);

    DvzVisual* overlay = dvz_segment(scene, 0);
    AT(overlay != NULL);
    vec3 overlay_starts[2] = {{-0.35f, 0.0f, 0.2f}, {0.0f, -0.35f, 0.2f}};
    vec3 overlay_ends[2] = {{+0.35f, 0.0f, 0.2f}, {0.0f, +0.35f, 0.2f}};
    DvzColor overlay_colors[2] = {{0, 255, 255, 160}, {0, 255, 255, 160}};
    float overlay_widths[2] = {8.0f, 8.0f};
    AT(dvz_visual_set_data(overlay, "position_start", overlay_starts, 2) == 0);
    AT(dvz_visual_set_data(overlay, "position_end", overlay_ends, 2) == 0);
    AT(dvz_visual_set_data(overlay, "color", overlay_colors, 2) == 0);
    AT(dvz_visual_set_data(overlay, "stroke_width", overlay_widths, 2) == 0);
    AT(dvz_segment_set_caps(overlay, DVZ_SEGMENT_CAP_ROUND, DVZ_SEGMENT_CAP_ROUND) == 0);
    AT(dvz_visual_set_depth_test(overlay, false) == 0);
    AT(dvz_visual_set_alpha_mode(overlay, DVZ_ALPHA_BLENDED) == 0);
    AT(dvz_panel_add_visual(panel, overlay, NULL) == 0);

    DvzFramePlan* plan = dvz_frame_plan("figure.msaa.ssao.blended", 0);
    ANN(plan);
    AT(_scene_emit_panel_render(figure, 0, plan, "figure_0"));
    AT(dvz_frame_plan_graph_validate(plan, NULL));

    bool saw_ssao_composite = false;
    bool saw_blended_after_ssao = false;
    for (uint32_t i = 0; i < dvz_frame_plan_graph_pass_count(plan); i++)
    {
        const DvzFrameGraphPass* pass = dvz_frame_plan_graph_pass_get(plan, i);
        ANN(pass);
        if (strcmp(pass->work_label, "ssao_composite") == 0)
            saw_ssao_composite = true;
        if (saw_ssao_composite && strcmp(pass->work_label, "transparent_blend") == 0)
            saw_blended_after_ssao = true;
    }
    AT(saw_ssao_composite);
    AT(saw_blended_after_ssao);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.supports_color_blending = true;
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.target_width = 64;
    cfg.target_height = 64;
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);
    DvzDrp2ValidationResult validation = dvz_drp2_validate_stream(stream);
    AT(validation.ok);

    _test_scene_stream_destroy(stream);
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
int test_scene_msaa_runtime_capability_lowering(TstContext* suite, const TstCase* item)
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
        panel, &(DvzMsaaDesc){DVZ_STRUCT_INIT_FIELDS(DvzMsaaDesc), .enabled = true, .sample_count = 16,
                              .alpha_to_coverage = true}));

    DvzVisual* sphere = dvz_sphere(scene, 0);
    AT(sphere != NULL);
    vec3 positions[1] = {{0.0f, 0.0f, 0.0f}};
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

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.max_color_sample_count = 16;
    caps.max_depth_sample_count = 8;

    const DvzFramePlanNode* render_node = dvz_frame_plan_node_get(plan, 0);
    const DvzFrameGraphPass* graph_pass = dvz_frame_plan_graph_pass_get(plan, 0);
    ANN(render_node);
    ANN(graph_pass);
    DvzScenePassContract contract = {0};
    AT(_scene_pass_contract_from_render_ex(plan, panel, render_node, graph_pass, &caps, &contract));
    const DvzSceneAttachmentUse* contract_msaa_color = NULL;
    const DvzSceneAttachmentUse* contract_depth = NULL;
    for (uint32_t i = 0; i < contract.attachment_count; i++)
    {
        const DvzSceneAttachmentUse* use = &contract.attachments[i];
        if (strcmp(use->resource_id, "figure_0_p0.msaa.color") == 0)
            contract_msaa_color = use;
        else if (strcmp(use->resource_id, "figure_0_p0.depth") == 0)
            contract_depth = use;
    }
    ANN(contract_msaa_color);
    ANN(contract_depth);
    AT(contract_msaa_color->requested_sample_count == 16);
    AT(contract_msaa_color->resolved_sample_count == 8);
    AT(contract_msaa_color->sample_count == 8);
    AT(contract_depth->requested_sample_count == 16);
    AT(contract_depth->resolved_sample_count == 8);
    AT(contract_depth->sample_count == 8);

    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.target_width = 64;
    cfg.target_height = 64;
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
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

    _test_scene_stream_destroy(stream);
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
int test_scene_edl_runtime_lowering(TstContext* suite, const TstCase* item)
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
    vec3 positions[3] = {
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
        panel, &(DvzEdlDesc){DVZ_STRUCT_INIT_FIELDS(DvzEdlDesc), .radius = 2.0f, .strength = 55.0f, .depth_scale = 1.0f}));

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
    caps = dvz_capability_snapshot();
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
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

    _test_scene_stream_destroy(stream);
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
int test_scene_edl_depth_producer_capabilities(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    vec3 point_positions[3] = {
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

    vec3 primitive_positions[3] = {
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

    vec3 mesh_positions[4] = {
        {0.10f, -0.50f, 0.2f},
        {0.70f, -0.50f, 0.2f},
        {0.10f, +0.30f, 0.2f},
        {0.70f, +0.30f, 0.2f},
    };
    vec3 mesh_normals[4] = {
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
    };
    DvzIndex mesh_indices[6] = {0, 1, 2, 2, 1, 3};
    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
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
        panel, &(DvzEdlDesc){DVZ_STRUCT_INIT_FIELDS(DvzEdlDesc), .radius = 2.0f, .strength = 55.0f, .depth_scale = 1.0f}));

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
int test_scene_edl_ignores_ineligible_passes(TstContext* suite, const TstCase* item)
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

    vec3 positions[3] = {
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
           &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .controller_mode = DVZ_CONTROLLER_FIXED}) == 0);
    AT(dvz_panel_set_edl(
        fixed_panel, &(DvzEdlDesc){DVZ_STRUCT_INIT_FIELDS(DvzEdlDesc), .radius = 2.0f, .strength = 55.0f, .depth_scale = 1.0f}));

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
        &(DvzEdlDesc){DVZ_STRUCT_INIT_FIELDS(DvzEdlDesc), .radius = 2.0f, .strength = 55.0f, .depth_scale = 1.0f}));

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
int test_scene_ssao_graph_foundation(TstContext* suite, const TstCase* item)
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

    vec3 positions[4] = {
        {-0.5f, -0.5f, 0.0f},
        {0.5f, -0.5f, 0.0f},
        {-0.5f, 0.5f, 0.0f},
        {0.5f, 0.5f, 0.0f},
    };
    vec3 normals[4] = {
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
    };
    DvzIndex indices[6] = {0, 1, 2, 2, 1, 3};
    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
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
        &(DvzSceneSsaoDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneSsaoDesc), .radius = 1.25f, .strength = 2.0f, .bias = 0.05f,
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
        &(DvzSceneSsaoDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneSsaoDesc), .radius = 1.25f, .strength = 2.0f, .bias = 0.05f,
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
int test_scene_ssao_runtime_lowering(TstContext* suite, const TstCase* item)
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
        panel, &(DvzMsaaDesc){DVZ_STRUCT_INIT_FIELDS(DvzMsaaDesc), .enabled = true, .sample_count = 4, .alpha_to_coverage = true}));

    DvzVisual* mesh = dvz_mesh(scene, 0);
    AT(mesh != NULL);

    vec3 positions[4] = {
        {-0.5f, -0.5f, 0.0f},
        {0.5f, -0.5f, 0.0f},
        {-0.5f, 0.5f, 0.0f},
        {0.5f, 0.5f, 0.0f},
    };
    vec3 normals[4] = {
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
    };
    DvzIndex indices[6] = {0, 1, 2, 2, 1, 3};
    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
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
        &(DvzSceneSsaoDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneSsaoDesc), .radius = 1.25f, .strength = 2.0f, .bias = 0.05f,
                            .sample_count = 16}));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.target_width = 64;
    cfg.target_height = 64;
    caps = dvz_capability_snapshot();
    caps.supports_color_blending = true;
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
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

    _test_scene_stream_destroy(stream);
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
int test_scene_ssao_glsl_executes(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_GRAPH_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
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
    DvzVisual* mesh = dvz_mesh(scene, 0);
    AT(mesh != NULL);

    vec3 positions[4] = {
        {-0.75f, -0.75f, 0.20f},
        {+0.75f, -0.75f, 0.20f},
        {-0.75f, +0.75f, 0.55f},
        {+0.75f, +0.75f, 0.55f},
    };
    vec3 normals[4] = {
        {0.0f, -0.35f, 0.94f},
        {0.0f, -0.35f, 0.94f},
        {0.0f, +0.35f, 0.94f},
        {0.0f, +0.35f, 0.94f},
    };
    DvzIndex indices[6] = {0, 1, 2, 2, 1, 3};
    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
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
        &(DvzSceneSsaoDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneSsaoDesc), .radius = 1.0f, .strength = 2.5f, .bias = 0.02f,
                            .sample_count = 16}));

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.supports_color_blending = true;
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    emit_cfg.target_width = 64;
    emit_cfg.target_height = 64;

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

    dvz_drp2_runtime_destroy(runtime);
    _test_scene_stream_destroy(stream);
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
int test_scene_sphere_ssao_glsl_executes(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_GRAPH_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
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
    DvzFigure* figure = dvz_figure(scene, 96, 96, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    DvzVisual* sphere = dvz_sphere(scene, DVZ_SPHERE_FLAGS_LIGHTING);
    AT(sphere != NULL);
    AT(dvz_sphere_mode(sphere, DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR) == 0);

    vec3 positions[4] = {
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

    AT(dvz_visual_set_data(sphere, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(sphere, "color", colors, 4) == 0);
    AT(dvz_visual_set_data(sphere, "radius", sizes, 4) == 0);
    AT(dvz_panel_add_visual(panel, sphere, NULL) == 0);
    AT(_scene_technique_state_set_ssao(
        &panel->techniques,
        &(DvzSceneSsaoDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneSsaoDesc), .radius = 1.0f, .strength = 2.5f, .bias = 0.02f,
                            .sample_count = 16}));

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

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    dvz_drp2_runtime_destroy(runtime);
    _test_scene_stream_destroy(stream);
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
int test_scene_ssao_ignores_ineligible_visuals(TstContext* suite, const TstCase* item)
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
    vec3 positions[3] = {
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
        &(DvzSceneSsaoDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneSsaoDesc), .radius = 1.25f, .strength = 2.0f, .bias = 0.05f,
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
int test_scene_visual_alpha_mode_standard_blend(TstContext* suite, const TstCase* item)
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

    vec3 positions[3] = {
        {-0.5f, -0.5f, 0.0f},
        {0.5f, -0.5f, 0.0f},
        {0.0f, 0.5f, 0.0f},
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

    DvzScenePassContract blend_contract = {0};
    AT(_scene_pass_contract_from_render(plan, panel, transparent_node, NULL, &blend_contract));
    AT(blend_contract.draw_count == 1);
    AT(blend_contract.draws[0].blend_policy == DVZ_SCENE_BLEND_POLICY_SOURCE_OVER);
    AT(blend_contract.draws[0].blend_target_count == 1);
    AT(blend_contract.draws[0].blend_targets[0].blend_enabled);
    AT(
        blend_contract.draws[0].blend_targets[0].src_color_blend_factor ==
        VK_BLEND_FACTOR_SRC_ALPHA);
    AT(
        blend_contract.draws[0].blend_targets[0].dst_color_blend_factor ==
        VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
    AT(
        blend_contract.draws[0].blend_targets[0].src_alpha_blend_factor ==
        VK_BLEND_FACTOR_ONE);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = _test_scene_emit_stream(figure, &caps, &report);
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

    stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
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

    _test_scene_stream_destroy(stream);
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
int test_scene_blended_mesh_orders_after_volume_slice(TstContext* suite, const TstCase* item)
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
    DvzVisual* mesh = dvz_mesh(scene, 0);
    AT(volume != NULL);
    AT(slice != NULL);
    AT(mesh != NULL);

    vec3 positions[4] = {
        {-0.5f, -0.5f, 0.0f},
        {0.5f, -0.5f, 0.0f},
        {-0.5f, 0.5f, 0.0f},
        {0.5f, 0.5f, 0.0f},
    };
    vec3 normals[4] = {
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
    };
    DvzIndex indices[6] = {0, 1, 2, 2, 1, 3};
    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
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
           panel, volume, &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .z_layer = 0}) == 0);
    AT(dvz_panel_add_visual(
           panel, slice, &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .z_layer = 1}) == 0);
    AT(dvz_panel_add_visual(
           panel, mesh, &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .z_layer = 2}) == 0);

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

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.supports_render_target_sampling = true;
    caps.supports_color_blending = true;
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
    _test_scene_stream_destroy(stream);

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
int test_scene_blended_mesh_occlusion_contracts(TstContext* suite, const TstCase* item)
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
    DvzVisual* mesh = dvz_mesh(scene, 0);
    AT(volume != NULL);
    AT(slice != NULL);
    AT(mesh != NULL);

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
        scene, &(DvzSceneBufferDesc){DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
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
           panel, volume, &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .z_layer = 0}) == 0);
    AT(dvz_panel_add_visual(
           panel, slice, &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .z_layer = 1}) == 0);
    AT(dvz_panel_add_visual(
           panel, mesh, &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .z_layer = 2}) == 0);
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

    DvzFramePlan* plan = dvz_frame_plan("figure.alpha.volume_mesh_occlusion", 0);
    ANN(plan);
    _scene_emit_visual_uploads(figure, plan, NULL);
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
    AT(strcmp(
           blend_node->u.render.visual_metadata[1].draw_volume_occlusion_resource_id,
           "figure_0_p0.volume_occlusion.depth") == 0);
    AT(strcmp(
           blend_node->u.render.visual_metadata[1].draw_volume_occlusion_producer_pass_id,
           "figure_0_p0.volume_occlusion") == 0);
    AT(blend_node->u.render.visual_metadata[1].draw_volume_occlusion_bind_set == DVZ_SCENE_SHADER_SET_VISUAL);
    AT(blend_node->u.render.visual_metadata[1].draw_volume_occlusion_bind_binding == 3);
    AT(strcmp(
           blend_node->u.render.visual_metadata[1].draw_scene_occlusion_resource_id,
           "figure_0_p0.scene_occlusion.depth") == 0);
    AT(strcmp(
           blend_node->u.render.visual_metadata[1].draw_scene_occlusion_producer_pass_id,
           "figure_0_p0.scene_occlusion") == 0);
    AT(blend_node->u.render.visual_metadata[1].draw_scene_occlusion_bind_set == DVZ_SCENE_SHADER_SET_SCENE_OCCLUSION);
    AT(blend_node->u.render.visual_metadata[1].draw_scene_occlusion_bind_binding == 0);

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
    AT(strcmp(
           contract.draws[1].volume_occlusion_resource_id,
           "figure_0_p0.volume_occlusion.depth") == 0);
    AT(strcmp(
           contract.draws[1].volume_occlusion_producer_pass_id,
           "figure_0_p0.volume_occlusion") == 0);
    AT(contract.draws[1].volume_occlusion_bind_set == 1);
    AT(contract.draws[1].volume_occlusion_bind_binding == 3);
    AT(strcmp(
           contract.draws[1].scene_occlusion_resource_id,
           "figure_0_p0.scene_occlusion.depth") == 0);
    AT(strcmp(
           contract.draws[1].scene_occlusion_producer_pass_id,
           "figure_0_p0.scene_occlusion") == 0);
    AT(contract.draws[1].scene_occlusion_bind_set == 2);
    AT(contract.draws[1].scene_occlusion_bind_binding == 0);
    AT(contract.draws[2].depth_test);
    AT(!contract.draws[2].depth_write);
    dvz_diagnostic_report_init(&graph_report);
    AT(_scene_pass_contract_validate(&contract, &graph_report));
    AT(dvz_diagnostic_report_count(&graph_report) == 0);

    DvzScenePassContract exact_contract = contract;
    for (uint32_t i = 0; i < exact_contract.attachment_count; i++)
    {
        if (exact_contract.attachments[i].role == DVZ_SCENE_ATTACHMENT_SAMPLED &&
            strcmp(
                exact_contract.attachments[i].resource_id,
                "figure_0_p0.volume_occlusion.depth") == 0)
        {
            dvz_strlcpy(
                exact_contract.attachments[i].resource_id,
                "figure_0_p1.volume_occlusion.depth",
                sizeof(exact_contract.attachments[i].resource_id));
            break;
        }
    }
    dvz_diagnostic_report_init(&graph_report);
    AT(!_scene_pass_contract_validate(&exact_contract, &graph_report));
    AT(dvz_diagnostic_report_count(&graph_report) > 0);

    exact_contract = contract;
    for (uint32_t i = 0; i < exact_contract.attachment_count; i++)
    {
        if (exact_contract.attachments[i].role == DVZ_SCENE_ATTACHMENT_SAMPLED &&
            strcmp(
                exact_contract.attachments[i].resource_id,
                "figure_0_p0.volume_occlusion.depth") == 0)
        {
            AT(strcmp(
                   exact_contract.attachments[i].producer_pass_id,
                   "figure_0_p0.volume_occlusion") == 0);
            dvz_strlcpy(
                exact_contract.attachments[i].producer_pass_id,
                "figure_0_p1.volume_occlusion",
                sizeof(exact_contract.attachments[i].producer_pass_id));
            break;
        }
    }
    dvz_diagnostic_report_init(&graph_report);
    AT(!_scene_pass_contract_validate(&exact_contract, &graph_report));
    AT(dvz_diagnostic_report_count(&graph_report) > 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
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
    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);
    DvzDrp2CommandStream* stream =
        dvz_frame_plan_emitter_emit_drp2(emitter, plan, &caps, &report, &cfg);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);
    DvzDrp2ValidationResult validation = dvz_drp2_validate_stream(stream);
    AT(validation.ok);
    dvz_diagnostic_report_init(&report);
    AT(_scene_frame_plan_drp2_contracts_validate(plan, stream, &report));
    AT(dvz_diagnostic_report_count(&report) == 0);

    DvzDrp2Command* volume_bind_group = NULL;
    uint32_t volume_binding_index = UINT32_MAX;
    for (uint32_t i = 0; i < stream->count; i++)
    {
        DvzDrp2Command* command = &stream->commands[i];
        if (command->type != DVZ_DRP2_COMMAND_CREATE_BIND_GROUP)
            continue;
        for (uint32_t j = 0; j < command->u.create_bind_group.entry_count; j++)
        {
            DvzDrp2BindGroupEntry* entry = &command->u.create_bind_group.entries[j];
            const char* label = dvz_drp2_stream_label(stream, entry->resource_id);
            if (
                entry->binding == 3 && label != NULL &&
                strcmp(label, "figure_0_p0.volume_occlusion.depth") == 0)
            {
                volume_bind_group = command;
                volume_binding_index = j;
                break;
            }
        }
        if (volume_bind_group != NULL)
            break;
    }
    ANN(volume_bind_group);
    AT(volume_binding_index != UINT32_MAX);
    uint32_t original_binding =
        volume_bind_group->u.create_bind_group.entries[volume_binding_index].binding;
    volume_bind_group->u.create_bind_group.entries[volume_binding_index].binding = 4;
    dvz_diagnostic_report_init(&report);
    AT(!_scene_frame_plan_drp2_contracts_validate(plan, stream, &report));
    AT(dvz_diagnostic_report_count(&report) > 0);
    volume_bind_group->u.create_bind_group.entries[volume_binding_index].binding =
        original_binding;

    _test_scene_stream_destroy(stream);
    dvz_frame_plan_emitter_destroy(emitter);
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
int test_scene_visual_alpha_mode_splits_frame_plan_passes(TstContext* suite, const TstCase* item)
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

    vec3 positions[3] = {
        {-0.5f, -0.5f, 0.0f},
        {0.5f, -0.5f, 0.0f},
        {0.0f, 0.5f, 0.0f},
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
    AT(opaque_contract.draws[0].blend_target_count == 1);
    AT(!opaque_contract.draws[0].blend_targets[0].blend_enabled);
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
    AT(accum_contract.draws[0].blend_target_count == 2);
    AT(accum_contract.draws[0].blend_targets[0].format == VK_FORMAT_R16G16B16A16_SFLOAT);
    AT(accum_contract.draws[0].blend_targets[0].blend_enabled);
    AT(
        accum_contract.draws[0].blend_targets[0].dst_color_blend_factor ==
        VK_BLEND_FACTOR_ONE);
    AT(accum_contract.draws[0].blend_targets[1].format == VK_FORMAT_R16_SFLOAT);
    AT(accum_contract.draws[0].blend_targets[1].blend_enabled);
    AT(
        accum_contract.draws[0].blend_targets[1].color_write_mask ==
        VK_COLOR_COMPONENT_R_BIT);
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
int test_scene_visual_alpha_mode_wboit_transparent_only_depth(TstContext* suite, const TstCase* item)
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

    vec3 positions[3] = {
        {-0.5f, -0.5f, 0.0f},
        {0.5f, -0.5f, 0.0f},
        {0.0f, 0.5f, 0.0f},
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

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
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

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);
    DvzDrp2ValidationResult validation = dvz_drp2_validate_stream(stream);
    AT(validation.ok);

    _test_scene_stream_destroy(stream);
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
int test_scene_visual_alpha_mode_depth_peel_frame_plan(TstContext* suite, const TstCase* item)
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

    vec3 positions[3] = {
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

    AT(dvz_frame_plan_node_count(plan) == 3 + DVZ_SCENE_DEPTH_PEEL_ITERATIONS);
    const DvzFramePlanNode* opaque_node = dvz_frame_plan_node_get(plan, 0);
    const DvzFramePlanNode* init_node = dvz_frame_plan_node_get(plan, 1);
    const DvzFramePlanNode* iter_node = dvz_frame_plan_node_get(plan, 2);
    const DvzFramePlanNode* composite_node =
        dvz_frame_plan_node_get(plan, 2 + DVZ_SCENE_DEPTH_PEEL_ITERATIONS);
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

    AT(dvz_frame_plan_graph_resource_count(plan) == 6);
    AT(dvz_frame_plan_graph_pass_count(plan) == 3 + DVZ_SCENE_DEPTH_PEEL_ITERATIONS);
    const DvzFrameGraphResource* depth_resource = dvz_frame_plan_graph_resource_get(plan, 1);
    const DvzFrameGraphResource* front_accum = dvz_frame_plan_graph_resource_get(plan, 2);
    ANN(depth_resource);
    ANN(front_accum);
    AT(strcmp(depth_resource->id, "figure_0_p0.depth.opaque") == 0);
    AT(depth_resource->format == VK_FORMAT_D32_SFLOAT);
    AT(strcmp(front_accum->id, "figure_0_p0.peel.front_accum") == 0);
    AT(front_accum->format == VK_FORMAT_R16G16B16A16_SFLOAT);

    const DvzFrameGraphPass* init_pass = dvz_frame_plan_graph_pass_get(plan, 1);
    const DvzFrameGraphPass* iter_pass = dvz_frame_plan_graph_pass_get(plan, 2);
    const DvzFrameGraphPass* composite_pass =
        dvz_frame_plan_graph_pass_get(plan, 2 + DVZ_SCENE_DEPTH_PEEL_ITERATIONS);
    ANN(init_pass);
    ANN(iter_pass);
    ANN(composite_pass);
    AT(strcmp(init_pass->work_label, "depth_peel_init") == 0);
    AT(strcmp(iter_pass->work_label, "depth_peel_iter") == 0);
    AT(strcmp(composite_pass->work_label, "depth_peel_composite") == 0);
    AT(init_pass->color_attachment_count == 3);
    AT(iter_pass->read_count == 1);
    AT(strcmp(iter_pass->reads[0].resource_id, "figure_0_p0.peel.depth_minmax_ping") == 0);
    AT(iter_pass->color_attachment_count == 3);
    AT(composite_pass->read_count == 2);
    AT(strcmp(composite_pass->reads[0].resource_id, "figure_0_p0.peel.front_accum") == 0);
    AT(strcmp(composite_pass->reads[1].resource_id, "figure_0_p0.peel.back_accum") == 0);
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
    AT(init_contract.draws[0].blend_target_count == 3);
    AT(init_contract.draws[0].blend_targets[0].format == VK_FORMAT_R16G16B16A16_SFLOAT);
    AT(init_contract.draws[0].blend_targets[0].blend_enabled);
    AT(init_contract.draws[0].blend_targets[2].format == VK_FORMAT_R16G16B16A16_SFLOAT);
    AT(init_contract.draws[0].blend_targets[2].blend_enabled);
    AT(init_contract.draws[0].blend_targets[2].color_blend_op == VK_BLEND_OP_MAX);
    AT(
        init_contract.draws[0].blend_targets[2].color_write_mask ==
        (VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT));
    AT(init_contract.draws[0].has_raster_state);
    AT(init_contract.draws[0].cull_mode == VK_CULL_MODE_NONE);
    AT(init_contract.draws[0].front_face == VK_FRONT_FACE_COUNTER_CLOCKWISE);
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
    AT(iter_contract.draws[0].blend_target_count == 3);
    AT(iter_contract.draws[0].blend_targets[0].format == VK_FORMAT_R16G16B16A16_SFLOAT);
    AT(iter_contract.draws[0].blend_targets[0].blend_enabled);
    AT(iter_contract.draws[0].blend_targets[2].format == VK_FORMAT_R16G16B16A16_SFLOAT);
    AT(iter_contract.draws[0].blend_targets[2].blend_enabled);
    AT(iter_contract.draws[0].blend_targets[2].color_blend_op == VK_BLEND_OP_MAX);
    AT(
        iter_contract.draws[0].blend_targets[2].color_write_mask ==
        (VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT));
    AT(iter_contract.draws[0].has_raster_state);
    AT(iter_contract.draws[0].cull_mode == VK_CULL_MODE_NONE);
    AT(iter_contract.draws[0].front_face == VK_FRONT_FACE_COUNTER_CLOCKWISE);
    AT(iter_contract.color_attachment_count == 3);
    AT(iter_contract.sampled_read_count == 1);
    AT(iter_contract.needs_depth_peel_sampled_layout);
    AT(iter_contract.sampled_texture_binding_count == 1);
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
    AT(composite_contract.attachment_count == 3);
    AT(composite_contract.sampled_read_count == 2);
    AT(composite_contract.needs_depth_peel_sampled_layout);
    AT(composite_contract.sampled_texture_binding_count == 2);
    dvz_diagnostic_report_init(&report);
    AT(_scene_pass_contract_validate(&composite_contract, &report));
    AT(dvz_diagnostic_report_count(&report) == 0);

    dvz_frame_plan_destroy(plan);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify source-over overlays on a depth-peel panel get matching graph passes.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_visual_alpha_mode_depth_peel_blended_overlay(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    DvzVisual* peel = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    DvzVisual* overlay = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    AT(peel != NULL);
    AT(overlay != NULL);

    vec3 positions[3] = {
        {-0.5f, -0.5f, 0.0f},
        {0.5f, -0.5f, 0.0f},
        {0.0f, 0.5f, 0.0f},
    };
    DvzColor peel_colors[3] = {{255, 0, 0, 128}, {0, 255, 0, 128}, {0, 0, 255, 128}};
    DvzColor overlay_colors[3] = {
        {255, 255, 255, 192}, {255, 255, 255, 192}, {255, 255, 255, 192}};

    AT(dvz_visual_set_data(peel, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(peel, "color", peel_colors, 3) == 0);
    AT(dvz_visual_set_alpha_mode(peel, DVZ_ALPHA_DEPTH_PEEL) == 0);
    AT(dvz_panel_add_visual(panel, peel, NULL) == 0);

    AT(dvz_visual_set_data(overlay, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(overlay, "color", overlay_colors, 3) == 0);
    AT(dvz_visual_set_alpha_mode(overlay, DVZ_ALPHA_BLENDED) == 0);
    AT(dvz_panel_add_visual(panel, overlay, NULL) == 0);

    DvzFramePlan* plan = dvz_frame_plan("figure.alpha.depth_peel.overlay", 0);
    ANN(plan);
    _scene_emit_panel_render(figure, 0, plan, "figure_0");

    AT(dvz_frame_plan_node_count(plan) == 4 + DVZ_SCENE_DEPTH_PEEL_ITERATIONS);
    AT(dvz_frame_plan_graph_pass_count(plan) == 4 + DVZ_SCENE_DEPTH_PEEL_ITERATIONS);
    const DvzFramePlanNode* overlay_node =
        dvz_frame_plan_node_get(plan, 3 + DVZ_SCENE_DEPTH_PEEL_ITERATIONS);
    const DvzFrameGraphPass* overlay_pass =
        dvz_frame_plan_graph_pass_get(plan, 3 + DVZ_SCENE_DEPTH_PEEL_ITERATIONS);
    ANN(overlay_node);
    ANN(overlay_pass);
    AT(dvz_frame_plan_render_pass_role(overlay_node) == DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND);
    AT(strcmp(overlay_pass->work_label, "transparent_blend") == 0);

    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    AT(_scene_frame_plan_contracts_validate(figure, plan, &report));
    AT(dvz_diagnostic_report_count(&report) == 0);

    dvz_frame_plan_destroy(plan);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify a depth-peel panel preserves earlier graph-backed panel output.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_visual_alpha_mode_depth_peel_loads_prior_panel(
    TstContext* suite, const TstCase* item)
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

    DvzVisual* blended = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    DvzVisual* peel = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    AT(blended != NULL);
    AT(peel != NULL);

    vec3 positions[3] = {
        {-0.5f, -0.5f, 0.0f},
        {0.5f, -0.5f, 0.0f},
        {0.0f, 0.5f, 0.0f},
    };
    DvzColor colors[3] = {{255, 0, 0, 128}, {0, 255, 0, 128}, {0, 0, 255, 128}};
    AT(dvz_visual_set_data(blended, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(blended, "color", colors, 3) == 0);
    AT(dvz_visual_set_alpha_mode(blended, DVZ_ALPHA_BLENDED) == 0);
    AT(dvz_panel_add_visual(left, blended, NULL) == 0);
    AT(dvz_visual_set_data(peel, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(peel, "color", colors, 3) == 0);
    AT(dvz_visual_set_alpha_mode(peel, DVZ_ALPHA_DEPTH_PEEL) == 0);
    AT(dvz_panel_add_visual(right, peel, NULL) == 0);

    DvzFramePlan* plan = dvz_frame_plan("figure.alpha.depth_peel.load_prior", 0);
    ANN(plan);
    AT(_scene_emit_panel_render(figure, 0, plan, "figure_0"));
    AT(_scene_emit_panel_render(figure, 1, plan, "figure_0"));

    const DvzFrameGraphPass* right_opaque = NULL;
    for (uint32_t i = 0; i < dvz_frame_plan_graph_pass_count(plan); i++)
    {
        const DvzFrameGraphPass* pass = dvz_frame_plan_graph_pass_get(plan, i);
        if (
            pass != NULL && strcmp(pass->panel_id, "figure_0_p1") == 0 &&
            strcmp(pass->work_label, "opaque") == 0)
        {
            right_opaque = pass;
            break;
        }
    }
    ANN(right_opaque);
    AT(right_opaque->color_attachment_count == 1);
    AT(right_opaque->color_attachments[0].load_op == DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD);

    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    AT(_scene_frame_plan_contracts_validate(figure, plan, &report));
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
int test_scene_visual_alpha_mode_mixed_oit_rejected(TstContext* suite, const TstCase* item)
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

    vec3 positions[3] = {
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

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
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

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
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
int test_scene_visual_alpha_mode_emits_depth_peel_drp2(TstContext* suite, const TstCase* item)
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

    vec3 positions[3] = {
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

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.max_color_attachments = 3;
    caps.render_target_format_rgba16float = true;
    caps.supports_render_target_sampling = true;
    caps.supports_color_blending = true;

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

    bool has_front_accum = false;
    bool has_depth_minmax_pong = false;
    bool has_depth_texture = false;
    bool has_three_target_pipeline = false;
    bool has_depth_bounds_max_blend = false;
    bool has_depth_peel_front_under_blend = false;
    bool has_depth_peel_back_over_blend = false;
    bool has_depth_peel_no_cull = false;
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
            has_front_accum =
                has_front_accum || (label != NULL &&
                                    strcmp(label, "fig0_p0.peel.front_accum") == 0);
            has_depth_minmax_pong =
                has_depth_minmax_pong ||
                (label != NULL && strcmp(label, "fig0_p0.peel.depth_minmax_pong") == 0);
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
            if (command->u.create_render_pipeline.color_target_count == 3)
            {
                const DvzDrp2ColorTarget* front =
                    &command->u.create_render_pipeline.color_targets[0];
                const DvzDrp2ColorTarget* back =
                    &command->u.create_render_pipeline.color_targets[1];
                const DvzDrp2ColorTarget* bounds =
                    &command->u.create_render_pipeline.color_targets[2];
                has_depth_peel_front_under_blend =
                    has_depth_peel_front_under_blend ||
                    (front->blend_enabled &&
                     front->src_color_blend_factor == VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA &&
                     front->dst_color_blend_factor == VK_BLEND_FACTOR_ONE);
                has_depth_peel_back_over_blend =
                    has_depth_peel_back_over_blend ||
                    (back->blend_enabled && back->src_color_blend_factor == VK_BLEND_FACTOR_ONE &&
                     back->dst_color_blend_factor == VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
                has_depth_bounds_max_blend =
                    has_depth_bounds_max_blend ||
                    (bounds->blend_enabled && bounds->color_blend_op == VK_BLEND_OP_MAX &&
                     bounds->alpha_blend_op == VK_BLEND_OP_MAX &&
                     bounds->color_write_mask ==
                         (VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT));
                has_depth_peel_no_cull =
                    has_depth_peel_no_cull ||
                    (command->u.create_render_pipeline.has_raster_state &&
                     command->u.create_render_pipeline.cull_mode == VK_CULL_MODE_NONE);
            }
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
            if (command->u.create_bind_group.entry_count == 2 ||
                command->u.create_bind_group.entry_count == 3)
                sampled_bind_group_count++;
            has_composite_bind_group = has_composite_bind_group ||
                                       command->u.create_bind_group.entry_count == 3;
        }
        else if (command->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS)
        {
            begin_pass_count++;
            if (command->u.begin_render_pass.color_attachment_count == 3)
                triple_attachment_passes++;
        }
    }

    AT(has_front_accum);
    AT(has_depth_minmax_pong);
    AT(has_depth_texture);
    AT(has_three_target_pipeline);
    AT(has_depth_bounds_max_blend);
    AT(has_depth_peel_front_under_blend);
    AT(has_depth_peel_back_over_blend);
    AT(has_depth_peel_no_cull);
    AT(has_composite_pipeline);
    AT(has_blended_composite_pipeline);
    AT(has_composite_bind_group);
    AT(sampled_bind_group_count >= 1);
    AT(begin_pass_count == 3 + DVZ_SCENE_DEPTH_PEEL_ITERATIONS);
    AT(triple_attachment_passes == 1 + DVZ_SCENE_DEPTH_PEEL_ITERATIONS);

    dvz_diagnostic_report_init(&report);
    cfg.runtime_resource_scope_id = UINT64_C(0x7b);
    DvzDrp2CommandStream* scoped_stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    ANN(scoped_stream);
    AT(dvz_diagnostic_report_count(&report) == 0);

    bool has_scoped_front_accum = false;
    bool has_scoped_composite_bind_group = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(scoped_stream); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(scoped_stream, i);
        ANN(command);
        if (command->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE)
        {
            const char* label = dvz_drp2_stream_label(scoped_stream, command->u.create_texture.id);
            has_scoped_front_accum =
                has_scoped_front_accum ||
                (label != NULL &&
                 strcmp(label, "fig0_p0.peel.front_accum_scope_000000000000007b") == 0);
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
    AT(has_scoped_front_accum);
    AT(has_scoped_composite_bind_group);

    dvz_diagnostic_report_init(&report);
    cfg.target_width = 96;
    cfg.target_height = 48;
    DvzDrp2CommandStream* scoped_resize_stream =
        _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    ANN(scoped_resize_stream);
    AT(dvz_diagnostic_report_count(&report) == 0);

    bool resized_front_accum = false;
    bool rebuilt_composite_bind_group = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(scoped_resize_stream); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(scoped_resize_stream, i);
        ANN(command);
        if (command->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE)
        {
            const char* label =
                dvz_drp2_stream_label(scoped_resize_stream, command->u.create_texture.id);
            resized_front_accum =
                resized_front_accum ||
                (label != NULL &&
                 strcmp(label, "fig0_p0.peel.front_accum_scope_000000000000007b") == 0 &&
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
    AT(resized_front_accum);
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

    _test_scene_stream_destroy(scoped_resize_stream);
    _test_scene_stream_destroy(scoped_stream);
    _test_scene_stream_destroy(stream);
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
int test_scene_visual_alpha_mode_requires_wboit_capabilities(TstContext* suite, const TstCase* item)
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

    vec3 positions[3] = {
        {-0.5f, -0.5f, 0.0f},
        {0.5f, -0.5f, 0.0f},
        {0.0f, 0.5f, 0.0f},
    };
    DvzColor colors[3] = {{255, 0, 0, 128}, {0, 255, 0, 128}, {0, 0, 255, 128}};
    float sizes[3] = {10.0f, 12.0f, 14.0f};

    AT(dvz_visual_set_data(visual, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 3) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 3) == 0);
    AT(dvz_visual_set_alpha_mode(visual, DVZ_ALPHA_WBOIT) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = _test_scene_emit_stream(figure, &caps, &report);
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

    stream = _test_scene_emit_stream(figure, &caps, &report);
    AT(stream != NULL);
    AT(dvz_diagnostic_report_count(&report) == 0);

    _test_scene_stream_destroy(stream);
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
int test_scene_visual_alpha_mode_emits_wboit_drp2(TstContext* suite, const TstCase* item)
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

    vec3 positions[3] = {
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

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
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

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
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
    DvzDrp2CommandStream* scoped_stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
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
        _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
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

    _test_scene_stream_destroy(scoped_resize_stream);
    _test_scene_stream_destroy(scoped_stream);
    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify WBOIT splat visuals lower to a two-target accumulation pipeline.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_splat_alpha_mode_emits_wboit_drp2(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    DvzVisual* splat = dvz_splat(scene, 0);
    AT(splat != NULL);

    vec3 positions[2] = {{-0.25f, 0.0f, 0.0f}, {+0.25f, 0.0f, 0.0f}};
    DvzColor colors[2] = {{255, 128, 64, 32}, {64, 128, 255, 32}};
    vec2 sigma[2] = {{6.0f, 3.0f}, {4.0f, 7.0f}};
    float angles[2] = {0.3f, -0.5f};
    AT(dvz_visual_set_data(splat, "position", positions, 2) == 0);
    AT(dvz_visual_set_data(splat, "color", colors, 2) == 0);
    AT(dvz_visual_set_data(splat, "sigma", sigma, 2) == 0);
    AT(dvz_visual_set_data(splat, "angle", angles, 2) == 0);
    AT(dvz_visual_set_alpha_mode(splat, DVZ_ALPHA_WBOIT) == 0);
    AT(dvz_panel_add_visual(panel, splat, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.max_color_attachments = 2;
    caps.render_target_format_rgba16float = true;
    caps.render_target_format_r16float = true;
    caps.supports_render_target_sampling = true;
    caps.supports_color_blending = true;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);
    DvzDrp2ValidationResult validation = dvz_drp2_validate_stream(stream);
    AT(validation.ok);

    bool has_splat_wboit_shader = false;
    bool has_splat_wboit_pipeline = false;
    bool has_splat_wboit_pipeline_identity = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        ANN(command);
        if (command->type == DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE)
        {
            bool splat_wboit =
                strcmp(command->u.create_shader_module.builtin_family, "scene.splat") == 0 &&
                strcmp(command->u.create_shader_module.builtin_variant, "wboit") == 0;
            if (splat_wboit)
            {
                has_splat_wboit_shader = true;
                AT(
                    command->u.create_shader_module.builtin_version ==
                    DVZ_SCENE_SHADER_BUILTIN_CONTRACT_VERSION);
            }
        }
        else if (command->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            bool splat_wboit_pipeline =
                command->u.create_render_pipeline.binding_count == 4 &&
                command->u.create_render_pipeline.color_target_count == 2 &&
                command->u.create_render_pipeline.color_targets[0].blend_enabled &&
                command->u.create_render_pipeline.color_targets[1].blend_enabled;
            if (splat_wboit_pipeline)
            {
                has_splat_wboit_pipeline = true;
                if (strcmp(command->u.create_render_pipeline.builtin_pipeline, "scene.splat") == 0)
                {
                    has_splat_wboit_pipeline_identity = true;
                    AT(
                        command->u.create_render_pipeline.builtin_version ==
                        DVZ_SCENE_SHADER_BUILTIN_CONTRACT_VERSION);
                }
            }
        }
    }
    AT(has_splat_wboit_shader);
    AT(has_splat_wboit_pipeline);
    AT(has_splat_wboit_pipeline_identity);

    _test_scene_stream_destroy(stream);
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
int test_scene_drp2_contract_checker_rejects_pipeline_drift(TstContext* suite, const TstCase* item)
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
    vec3 positions[3] = {
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
    _scene_emit_visual_uploads(figure, plan, NULL);
    AT(_scene_emit_panel_render(figure, 0, plan, "figure_0"));

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    AT(_scene_frame_plan_contracts_validate(figure, plan, &report));
    AT(dvz_diagnostic_report_count(&report) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
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

    DvzDrp2Command* resolve_pipeline = NULL;
    for (uint32_t i = 0; i < stream->count; i++)
    {
        DvzDrp2Command* command = &stream->commands[i];
        if (command->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE &&
            command->u.create_render_pipeline.color_target_count == 1 &&
            command->u.create_render_pipeline.vertex_buffer_slots == 0 &&
            command->u.create_render_pipeline.bind_group_layout_count == 1 &&
            command->u.create_render_pipeline.color_targets[0].blend_enabled)
        {
            resolve_pipeline = command;
            break;
        }
    }
    ANN(resolve_pipeline);

    const DvzDrp2Command original_resolve_pipeline_command = *resolve_pipeline;

    wboit_pipeline->u.create_render_pipeline.color_targets[0].blend_enabled = false;
    dvz_diagnostic_report_init(&report);
    AT(!_scene_frame_plan_drp2_contracts_validate(plan, stream, &report));
    AT(dvz_diagnostic_report_count(&report) > 0);

    wboit_pipeline->u.create_render_pipeline = original_pipeline_command.u.create_render_pipeline;
    wboit_pipeline->u.create_render_pipeline.color_targets[0].dst_color_blend_factor =
        VK_BLEND_FACTOR_ZERO;
    dvz_diagnostic_report_init(&report);
    AT(!_scene_frame_plan_drp2_contracts_validate(plan, stream, &report));
    AT(dvz_diagnostic_report_count(&report) > 0);

    wboit_pipeline->u.create_render_pipeline = original_pipeline_command.u.create_render_pipeline;
    wboit_pipeline->u.create_render_pipeline.color_targets[1].color_write_mask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT;
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

    resolve_pipeline->u.create_render_pipeline.color_targets[0].dst_color_blend_factor =
        VK_BLEND_FACTOR_ZERO;
    dvz_diagnostic_report_init(&report);
    AT(!_scene_frame_plan_drp2_contracts_validate(plan, stream, &report));
    AT(dvz_diagnostic_report_count(&report) > 0);

    resolve_pipeline->u.create_render_pipeline =
        original_resolve_pipeline_command.u.create_render_pipeline;
    resolve_pipeline->u.create_render_pipeline.bind_group_layout_count = 0;
    dvz_diagnostic_report_init(&report);
    AT(!_scene_frame_plan_drp2_contracts_validate(plan, stream, &report));
    AT(dvz_diagnostic_report_count(&report) > 0);

    resolve_pipeline->u.create_render_pipeline =
        original_resolve_pipeline_command.u.create_render_pipeline;
    resolve_pipeline->u.create_render_pipeline.vertex_buffer_slots = 1;
    dvz_diagnostic_report_init(&report);
    AT(!_scene_frame_plan_drp2_contracts_validate(plan, stream, &report));
    AT(dvz_diagnostic_report_count(&report) > 0);

    resolve_pipeline->u.create_render_pipeline =
        original_resolve_pipeline_command.u.create_render_pipeline;

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

    _test_scene_stream_destroy(stream);
    dvz_frame_plan_emitter_destroy(emitter);
    dvz_frame_plan_destroy(plan);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify scene DRP2 contract validation catches emitted raster-state policy drift.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_drp2_contract_checker_rejects_raster_drift(TstContext* suite, const TstCase* item)
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
    vec3 positions[3] = {
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

    DvzFramePlan* plan = dvz_frame_plan("figure.contract.raster", 0);
    ANN(plan);
    _scene_emit_visual_uploads(figure, plan, NULL);
    AT(_scene_emit_panel_render(figure, 0, plan, "figure_0"));

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.max_color_attachments = 3;
    caps.render_target_format_rgba16float = true;
    caps.supports_render_target_sampling = true;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
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

    DvzDrp2Command* init_pipeline = NULL;
    DvzDrp2Command* iter_pipeline = NULL;
    for (uint32_t i = 0; i < stream->count; i++)
    {
        DvzDrp2Command* command = &stream->commands[i];
        if (
            command->type != DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE ||
            command->u.create_render_pipeline.color_target_count != 3 ||
            !command->u.create_render_pipeline.has_raster_state)
            continue;
        if (command->u.create_render_pipeline.cull_mode == VK_CULL_MODE_NONE)
        {
            if (init_pipeline == NULL)
                init_pipeline = command;
            else
                iter_pipeline = command;
        }
        if (init_pipeline != NULL && iter_pipeline != NULL)
            break;
    }
    ANN(init_pipeline);
    ANN(iter_pipeline);

    const DvzDrp2Command original_init_pipeline = *init_pipeline;
    init_pipeline->u.create_render_pipeline.cull_mode = VK_CULL_MODE_BACK_BIT;
    dvz_diagnostic_report_init(&report);
    AT(!_scene_frame_plan_drp2_contracts_validate(plan, stream, &report));
    AT(dvz_diagnostic_report_count(&report) > 0);

    init_pipeline->u.create_render_pipeline = original_init_pipeline.u.create_render_pipeline;
    const DvzDrp2Command original_iter_pipeline = *iter_pipeline;
    iter_pipeline->u.create_render_pipeline.front_face = VK_FRONT_FACE_CLOCKWISE;
    dvz_diagnostic_report_init(&report);
    AT(!_scene_frame_plan_drp2_contracts_validate(plan, stream, &report));
    AT(dvz_diagnostic_report_count(&report) > 0);

    iter_pipeline->u.create_render_pipeline = original_iter_pipeline.u.create_render_pipeline;

    _test_scene_stream_destroy(stream);
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
int test_scene_alpha_mode_toggle_refreshes_drp2_contracts(TstContext* suite, const TstCase* item)
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

    vec3 positions[3] = {
        {-0.5f, -0.5f, 0.0f},
        {0.5f, -0.5f, 0.0f},
        {0.0f, 0.5f, 0.0f},
    };
    vec3 shifted[3] = {
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

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
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

    DvzDrp2CommandStream* source_over0 = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
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
    _test_scene_stream_destroy(source_over0);

    AT(dvz_visual_set_alpha_mode(transparent, DVZ_ALPHA_WBOIT) == 0);
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* wboit = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
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
    _test_scene_stream_destroy(wboit);

    AT(dvz_visual_set_alpha_mode(transparent, DVZ_ALPHA_BLENDED) == 0);
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* source_over1 = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
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
    _test_scene_stream_destroy(source_over1);
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
int test_scene_visual_alpha_mode_wboit_glsl_executes(TstContext* suite, const TstCase* item)
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

    DvzVisual* transparent = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    AT(transparent != NULL);
    vec3 positions[3] = {
        {-0.6f, -0.6f, 0.0f},
        {0.6f, -0.6f, 0.0f},
        {0.0f, 0.6f, 0.0f},
    };
    vec3 normals[3] = {
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
    };
    DvzColor colors[3] = {{255, 0, 0, 128}, {0, 255, 0, 128}, {0, 0, 255, 128}};
    AT(dvz_visual_set_data(transparent, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(transparent, "color", colors, 3) == 0);
    AT(dvz_visual_set_data(transparent, "normal", normals, 3) == 0);
    AT(_test_set_phong_material(
           transparent, (float[3]){0.0f, 0.0f, 1.0f}, 0.25f, 0.75f, 0.25f, 32.0f) ==
       0);
    AT(dvz_visual_set_alpha_mode(transparent, DVZ_ALPHA_WBOIT) == 0);
    AT(dvz_panel_add_visual(panel, transparent, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
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

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
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

    _test_scene_stream_destroy(readback);
    dvz_drp2_runtime_destroy(runtime);
    _test_scene_stream_destroy(stream);
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
int test_scene_visual_alpha_mode_depth_peel_glsl_executes(TstContext* suite, const TstCase* item)
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

    DvzVisual* transparent = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    AT(transparent != NULL);
    dvz_panel_set_background_color(panel, 0.05f, 0.05f, 0.08f, 1.0f);
    vec3 positions[3] = {
        {-0.6f, -0.6f, 0.0f},
        {0.6f, -0.6f, 0.0f},
        {0.0f, 0.6f, 0.0f},
    };
    DvzColor colors[3] = {{255, 0, 0, 192}, {0, 255, 0, 192}, {0, 0, 255, 192}};
    AT(dvz_visual_set_data(transparent, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(transparent, "color", colors, 3) == 0);
    AT(dvz_visual_set_alpha_mode(transparent, DVZ_ALPHA_DEPTH_PEEL) == 0);
    AT(dvz_panel_add_visual(panel, transparent, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.max_color_attachments = 3;
    caps.render_target_format_rgba16float = true;
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

    _test_scene_stream_destroy(readback);
    dvz_drp2_runtime_destroy(runtime);
    _test_scene_stream_destroy(stream);
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
