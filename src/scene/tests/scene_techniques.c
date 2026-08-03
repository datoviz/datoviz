/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene technique graph tests */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "frame_plan/internal.h"
#include "render_contract/internal.h"
#include "runtime/_frame_plan_runtime_internal.h"
#include "scene_emit/panel_render_plan.h"
#include "scene_graph_utils.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

static const DvzFrameGraphResource* _test_graph_resource(const DvzFramePlan* plan, const char* id)
{
    ANN(plan);
    ANN(id);
    for (uint32_t i = 0; i < dvz_frame_plan_graph_resource_count(plan); i++)
    {
        const DvzFrameGraphResource* resource = dvz_frame_plan_graph_resource_get(plan, i);
        if (resource != NULL && strcmp(resource->id, id) == 0)
            return resource;
    }
    return NULL;
}



static const DvzRenderProductContract* _test_surface_product(
    const DvzFramePlan* plan, DvzRenderProductKind kind, DvzRenderProductSampleDomain samples)
{
    ANN(plan);
    for (uint32_t i = 0; i < dvz_frame_plan_product_count(plan); i++)
    {
        const DvzRenderProductContract* product = dvz_frame_plan_product_get(plan, i);
        if (product != NULL && product->kind == kind && product->sample_domain == samples)
            return product;
    }
    return NULL;
}



static const DvzRenderProductContract*
_test_product_version(const DvzFramePlan* plan, DvzRenderProductKind kind, uint32_t version)
{
    ANN(plan);
    for (uint32_t i = 0; i < dvz_frame_plan_product_count(plan); i++)
    {
        const DvzRenderProductContract* product = dvz_frame_plan_product_get(plan, i);
        if (product != NULL && product->kind == kind && product->version == version)
            return product;
    }
    return NULL;
}



/**
 * Create one indexed opaque mesh panel with either G-buffer or GTAO technique work enabled.
 *
 * @param scene the scene
 * @param figure the parent figure
 * @param desc normalized panel rectangle
 * @param gtao whether to enable GTAO instead of the standalone G-buffer
 * @return the configured panel, or NULL on failure
 */
static DvzPanel*
_test_r4_surface_panel(DvzScene* scene, DvzFigure* figure, const DvzPanelDesc* desc, bool gtao)
{
    ANN(scene);
    ANN(figure);
    ANN(desc);

    DvzPanel* panel = dvz_panel(figure, desc);
    if (panel == NULL)
        return NULL;

    DvzVisual* mesh = dvz_mesh(scene, 0);
    if (mesh == NULL)
        return NULL;
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
    DvzIndex indices[6] = {0, 1, 2, 2, 1, 3};
    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){
                   DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX, .stride = sizeof(DvzIndex)});
    if (index_buffer == NULL ||
        dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)) != DVZ_OK ||
        dvz_visual_set_data(mesh, "position", positions, 4) != DVZ_OK ||
        dvz_visual_set_data(mesh, "normal", normals, 4) != DVZ_OK ||
        dvz_visual_set_buffer(mesh, "index", index_buffer) != DVZ_OK ||
        dvz_panel_add_visual(panel, mesh, NULL) != DVZ_OK)
        return NULL;

    if (gtao && !_scene_technique_state_set_ao(
                    &panel->techniques,
                    &(DvzSceneAoDesc){
                        DVZ_STRUCT_INIT_FIELDS(DvzSceneAoDesc), .radius = 1.0f, .intensity = 1.0f,
                        .thickness = 0.1f, .quality = DVZ_AO_QUALITY_MEDIUM}))
        return NULL;
    if (!gtao)
        _scene_technique_state_enable_gbuffer(&panel->techniques, true);
    return panel;
}



/**
 * Verify asymmetric fractional HiDPI panel geometry uses one outward-rounded physical rectangle.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_panel_composition_hidpi_geometry(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 101, 67, 0);
    ANN(figure);
    DvzPanel* panel =
        _test_r4_surface_panel(scene, figure, &(DvzPanelDesc){0.13f, 0.17f, 0.37f, 0.41f}, false);
    ANN(panel);
    figure->device_scale_x = 1.25f;
    figure->device_scale_y = 1.50f;
    figure->render_scale = 0.80f;

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.max_color_attachments = 3;
    caps.supports_render_target_sampling = true;
    caps.render_target_format_rgba16float = true;
    caps.render_target_format_r16float = true;
    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    DvzPanelRenderPlan render_plan = {0};
    AT(_scene_panel_render_plan_build(figure, 0, "figure_0", &caps, &report, &render_plan));
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(render_plan.origin_x == 13);
    AT(render_plan.origin_y == 13);
    AT(render_plan.width == 38);
    AT(render_plan.height == 34);
    AC(render_plan.render_scale, 0.80f, 1e-6f);
    AC(render_plan.local_to_target[0], 1.0f, 1e-6f);
    AC(render_plan.local_to_target[1], 1.0f, 1e-6f);
    AC(render_plan.local_to_target[2], 13.0f, 1e-6f);
    AC(render_plan.local_to_target[3], 13.0f, 1e-6f);

    DvzPanelCompositionSnapshot composition = {0};
    AT(_scene_panel_composition_resolve(&render_plan, &caps, &composition, &report));
    AT(composition.origin_x == render_plan.origin_x);
    AT(composition.origin_y == render_plan.origin_y);
    AT(composition.width == render_plan.width);
    AT(composition.height == render_plan.height);
    AT(memcmp(
           composition.local_to_target, render_plan.local_to_target,
           sizeof(composition.local_to_target)) == 0);

    DvzPanelRenderPlan dormant = render_plan;
    dormant.width = 0;
    dormant.height = 0;
    DvzPanelCompositionSnapshot dormant_composition = {0};
    AT(_scene_panel_composition_resolve(&dormant, &caps, &dormant_composition, &report));
    AT(dormant_composition.pass_count == 0);
    AT(dormant_composition.scratch_resource_count == 0);
    DvzFramePlan* dormant_graph = dvz_frame_plan("r4.dormant", 0);
    ANN(dormant_graph);
    AT(_scene_panel_composition_lower_graph(dormant_graph, &dormant_composition, &report));
    AT(dormant_graph->graph_pass_count == 0);
    for (uint32_t i = 0; i < dormant_graph->graph_resource_count; i++)
        AT(dormant_graph->graph_resources[i].kind == DVZ_FRAME_GRAPH_RESOURCE_EXTERNAL_TARGET);
    dvz_frame_plan_destroy(dormant_graph);

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Verify panel-local technique intermediates and their passes use the exact local extent.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_panel_local_intermediate_realization(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 203, 137, 0);
    ANN(figure);
    DvzPanel* panel =
        _test_r4_surface_panel(scene, figure, &(DvzPanelDesc){0.13f, 0.17f, 0.37f, 0.41f}, false);
    ANN(panel);

    DvzFramePlan* plan = dvz_frame_plan("r4.panel-local", 0);
    ANN(plan);
    AT(_scene_emit_panel_render(figure, 0, plan, "figure_0"));
    const DvzPanelCompositionSnapshot* composition =
        _frame_plan_composition_get(plan, "figure_0_p0");
    ANN(composition);
    AT(composition->origin_x == 26);
    AT(composition->origin_y == 23);
    AT(composition->width == 76);
    AT(composition->height == 57);
    for (uint32_t i = 0; i < plan->graph_resource_count; i++)
    {
        const DvzFrameGraphResource* resource = &plan->graph_resources[i];
        if (resource->kind == DVZ_FRAME_GRAPH_RESOURCE_EXTERNAL_TARGET)
            continue;
        if (strcmp(resource->id, "figure_0_p0.depth") == 0)
        {
            AT(resource->extent_kind == DVZ_FRAME_GRAPH_EXTENT_FIGURE);
            continue;
        }
        AT(resource->extent_kind == DVZ_FRAME_GRAPH_EXTENT_PANEL);
        AT(resource->width == composition->width);
        AT(resource->height == composition->height);
    }
    dvz_frame_plan_destroy(plan);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.max_color_attachments = 3;
    caps.supports_render_target_sampling = true;
    caps.render_target_format_rgba16float = true;
    caps.render_target_format_r16float = true;
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.target_width = 203;
    cfg.target_height = 137;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);

    uint64_t capture_id = 0;
    uint64_t gbuffer_pass_id = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        ANN(command);
        if (command->type != DVZ_DRP2_COMMAND_CREATE_TEXTURE)
            continue;
        const char* label = dvz_drp2_stream_label(stream, command->u.create_texture.id);
        if (label != NULL && strcmp(label, "fig0_p0.gbuffer.normal") == 0)
        {
            AT(command->u.create_texture.width == 76);
            AT(command->u.create_texture.height == 57);
        }
        if (label != NULL && strcmp(label, "fig0_p0.gbuffer.depth") == 0)
        {
            capture_id = command->u.create_texture.id;
            AT(command->u.create_texture.width == 76);
            AT(command->u.create_texture.height == 57);
        }
    }
    AT(capture_id != 0);
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        ANN(command);
        if (command->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS &&
            command->u.begin_render_pass.texture_id == capture_id)
        {
            gbuffer_pass_id = command->u.begin_render_pass.id;
            AT(command->u.begin_render_pass.render_area_px[0] == 0);
            AT(command->u.begin_render_pass.render_area_px[1] == 0);
            AT(command->u.begin_render_pass.render_area_px[2] == 76);
            AT(command->u.begin_render_pass.render_area_px[3] == 57);
            AC(command->u.begin_render_pass.viewport_px[0], 0.0f, 1e-6f);
            AC(command->u.begin_render_pass.viewport_px[1], 0.0f, 1e-6f);
            AC(command->u.begin_render_pass.viewport_px[2], 76.0f, 1e-6f);
            AC(command->u.begin_render_pass.viewport_px[3], 57.0f, 1e-6f);
            AC(command->u.begin_render_pass.scissor_px[0], 0.0f, 1e-6f);
            AC(command->u.begin_render_pass.scissor_px[1], 0.0f, 1e-6f);
            AC(command->u.begin_render_pass.scissor_px[2], 76.0f, 1e-6f);
            AC(command->u.begin_render_pass.scissor_px[3], 57.0f, 1e-6f);
        }
    }
    AT(gbuffer_pass_id != 0);
    bool found_local_viewport = false;
    bool found_local_scissor = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        ANN(command);
        if (command->type == DVZ_DRP2_COMMAND_SET_VIEWPORT &&
            command->u.set_viewport.pass_id == gbuffer_pass_id)
        {
            AC(command->u.set_viewport.viewport[0], 0.0f, 1e-6f);
            AC(command->u.set_viewport.viewport[1], 0.0f, 1e-6f);
            AC(command->u.set_viewport.viewport[2], 75.11f, 1e-4f);
            AC(command->u.set_viewport.viewport[3], 56.17f, 1e-4f);
            found_local_viewport = true;
        }
        if (command->type == DVZ_DRP2_COMMAND_SET_SCISSOR &&
            command->u.set_scissor.pass_id == gbuffer_pass_id)
        {
            AC(command->u.set_scissor.scissor[0], 0.0f, 1e-6f);
            AC(command->u.set_scissor.scissor[1], 0.0f, 1e-6f);
            AC(command->u.set_scissor.scissor[2], 75.11f, 1e-4f);
            AC(command->u.set_scissor.scissor[3], 56.17f, 1e-4f);
            found_local_scissor = true;
        }
    }
    AT(found_local_viewport);
    AT(found_local_scissor);

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Verify a panel-only resize recreates its targets, refreshes descriptors, and then becomes quiet.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_panel_local_resize_refresh(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 200, 100, 0);
    ANN(figure);
    DvzPanel* panel =
        _test_r4_surface_panel(scene, figure, &(DvzPanelDesc){0.10f, 0.20f, 0.30f, 0.50f}, true);
    ANN(panel);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.supports_color_blending = true;
    caps.max_color_attachments = 3;
    caps.supports_render_target_sampling = true;
    caps.render_target_format_rgba16float = true;
    caps.render_target_format_r16float = true;
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.target_width = 200;
    cfg.target_height = 100;
    cfg.runtime_resource_scope_id = UINT64_C(0x44);
    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* initial = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    ANN(initial);
    AT(dvz_diagnostic_report_count(&report) == 0);

    AT(dvz_panel_set_desc(panel, &(DvzPanelDesc){0.10f, 0.20f, 0.45f, 0.50f}) == DVZ_OK);
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* resized = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    ANN(resized);
    AT(dvz_diagnostic_report_count(&report) == 0);

    uint64_t normal_id = 0;
    uint64_t depth_id = 0;
    bool recreated_normal = false;
    bool recreated_depth = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(resized); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(resized, i);
        ANN(command);
        if (command->type != DVZ_DRP2_COMMAND_CREATE_TEXTURE)
            continue;
        const char* label = dvz_drp2_stream_label(resized, command->u.create_texture.id);
        if (label != NULL && strstr(label, "fig0_p0.gbuffer.normal") != NULL)
        {
            normal_id = command->u.create_texture.id;
            recreated_normal =
                command->u.create_texture.width == 90 && command->u.create_texture.height == 50;
        }
        else if (label != NULL && strstr(label, "fig0_p0.gbuffer.depth") != NULL)
        {
            depth_id = command->u.create_texture.id;
            recreated_depth =
                command->u.create_texture.width == 90 && command->u.create_texture.height == 50;
        }
    }
    AT(recreated_normal);
    AT(recreated_depth);
    bool refreshed_gtao = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(resized); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(resized, i);
        ANN(command);
        if (command->type == DVZ_DRP2_COMMAND_CREATE_BIND_GROUP &&
            command->u.create_bind_group.entry_count == 5)
        {
            refreshed_gtao = refreshed_gtao ||
                             (command->u.create_bind_group.entries[0].resource_id == normal_id &&
                              command->u.create_bind_group.entries[1].resource_id == depth_id);
        }
    }
    AT(refreshed_gtao);

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* steady = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    ANN(steady);
    AT(dvz_diagnostic_report_count(&report) == 0);
    bool recreated_steady = false;
    bool refreshed_steady = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(steady); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(steady, i);
        ANN(command);
        if (command->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE)
        {
            const char* label = dvz_drp2_stream_label(steady, command->u.create_texture.id);
            recreated_steady =
                recreated_steady || (label != NULL && strstr(label, "fig0_p0.gbuffer.") != NULL);
        }
        else if (
            command->type == DVZ_DRP2_COMMAND_CREATE_BIND_GROUP &&
            command->u.create_bind_group.entry_count == 5 &&
            command->u.create_bind_group.entries[0].resource_id == normal_id &&
            command->u.create_bind_group.entries[1].resource_id == depth_id)
            refreshed_steady = true;
    }
    AT(!recreated_steady);
    AT(!refreshed_steady);

    _test_scene_stream_destroy(steady);
    _test_scene_stream_destroy(resized);
    _test_scene_stream_destroy(initial);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Verify runtime graph target growth, replacement, lookup, and idempotent cleanup.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_graph_runtime_targets_grow(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    SceneGraphRuntimeTargets targets = {0};
    const uint32_t count = 2 * DVZ_FRAME_PLAN_INITIAL_GRAPH_RESOURCE_CAPACITY + 1;
    for (uint32_t i = 0; i < count; i++)
    {
        char id[DVZ_SCENE_LABEL_SIZE];
        dvz_snprintf(id, sizeof(id), "panel.runtime.%" PRIu32, i);
        AT(_graph_runtime_targets_add(&targets, id, 1000 + i));
    }
    AT(targets.count == count);
    AT(targets.capacity >= count);
    for (uint32_t i = 0; i < count; i++)
    {
        char id[DVZ_SCENE_LABEL_SIZE];
        dvz_snprintf(id, sizeof(id), "panel.runtime.%" PRIu32, i);
        AT(_graph_runtime_targets_get(&targets, id) == 1000 + i);
    }

    AT(_graph_runtime_targets_add(&targets, "panel.runtime.0", 9999));
    AT(targets.count == count);
    AT(_graph_runtime_targets_get(&targets, "panel.runtime.0") == 9999);

    _graph_runtime_targets_destroy(&targets);
    AT(targets.targets == NULL);
    AT(targets.count == 0);
    AT(targets.capacity == 0);
    _graph_runtime_targets_destroy(&targets);
    return 0;
}



/**
 * Verify deterministic immutable composition, approved phase order, and atomic diagnostics.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_panel_composition_snapshot(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzPanelRenderPlan plan = {0};
    dvz_strlcpy(plan.panel_id, "figure_0_p0", sizeof(plan.panel_id));
    plan.width = 64;
    plan.height = 64;
    plan.drawable_count = 2;
    plan.visual_count = 2;
    plan.visuals[0] = (DvzPanelRenderVisualPlan){
        .visual_index = 3,
        .authored_order = 0,
        .layer = DVZ_SCENE_VISUAL_LAYER_SURFACE_OPAQUE,
        .caps =
            {
                .layer = DVZ_SCENE_VISUAL_LAYER_SURFACE_OPAQUE,
                .phase_participation = DVZ_SCENE_VISUAL_PHASE_OPAQUE_SHADING,
            },
    };
    plan.visuals[1] = (DvzPanelRenderVisualPlan){
        .visual_index = 7,
        .authored_order = 1,
        .layer = DVZ_SCENE_VISUAL_LAYER_TRANSPARENT,
        .caps =
            {
                .layer = DVZ_SCENE_VISUAL_LAYER_TRANSPARENT,
                .phase_participation = DVZ_SCENE_VISUAL_PHASE_TRANSPARENT_SHADING,
            },
    };
    plan.opaque_visuals[0] = plan.visuals[0];
    plan.opaque_visual_count = 1;
    plan.has_transparent = true;
    plan.blended_visuals[0] = plan.visuals[1];
    plan.blended_visuals[0].blend_group = 0;
    plan.blended_visual_count = 1;
    plan.blended_group_count = 1;
    plan.transparent_passes[0] = (DvzPanelRenderTransparentPassPlan){
        .kind = DVZ_PANEL_RENDER_TRANSPARENT_BLENDED,
        .index = 0,
    };
    plan.transparent_pass_count = 1;

    DvzCapabilitySnapshot caps = {DVZ_STRUCT_INIT_FIELDS(DvzCapabilitySnapshot)};
    caps.max_color_sample_count = 4;
    caps.max_depth_sample_count = 4;
    caps.render_target_format_rgba16float = true;
    caps.render_target_format_r16float = true;
    caps.supports_render_target_sampling = true;
    caps.supports_color_blending = true;
    DvzPanelCompositionSnapshot first = {0};
    DvzPanelCompositionSnapshot second = {0};
    AT(_scene_panel_composition_resolve(&plan, &caps, &first, NULL));
    AT(_scene_panel_composition_resolve(&plan, &caps, &second, NULL));
    AT(first.valid);
    AT(memcmp(&first, &second, sizeof(first)) == 0);
    AT(first.pass_count == 2);
    AT(first.passes[0].role == DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE);
    AT(first.passes[0].phase == DVZ_SCENE_PHASE_OPAQUE_SHADING);
    AT(first.passes[0].authored_order_begin == 0);
    AT(first.passes[1].role == DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND);
    AT(first.passes[1].phase == DVZ_SCENE_PHASE_TRANSPARENT_SHADING);
    AT(first.passes[1].authored_order_begin == 1);
    AT(first.passes[0].id.value != first.passes[1].id.value);
    AT((first.required_product_mask & (UINT64_C(1) << DVZ_RENDER_PRODUCT_SCENE_COLOR)) != 0);
    AT((first.required_product_mask & (UINT64_C(1) << DVZ_RENDER_PRODUCT_PRESENTATION_COLOR)) !=
       0);
    AT(first.work_declaration_fingerprint != 0);
    AT(first.passes[0].provider == DVZ_SCENE_WORK_PROVIDER_OPAQUE);
    AT(first.passes[0].coordinate_space == DVZ_RENDER_PRODUCT_COORDINATES_PANEL_LOCAL);
    AT(first.passes[0].viewport_panel_local);
    AT(first.passes[0].scissor_panel_local);
    AT(first.passes[0].binding_count == 1);
    AT(first.passes[0].bindings[0].clear_value_kind == DVZ_SCENE_CLEAR_VALUE_FRAME);
    AT(first.passes[1].provider == DVZ_SCENE_WORK_PROVIDER_TRANSPARENT_BLEND);
    AT(first.passes[1].binding_count == 1);
    AT(first.passes[1].bindings[0].load == DVZ_SCENE_ATTACHMENT_LOAD_LOAD);
    AT(first.passes[1].bindings[0].access == DVZ_SCENE_WORK_ACCESS_READ_WRITE);
    AT(first.passes[1].bindings[0].load_source_product_id.value ==
       first.techniques[1].input_ids[0].value);

    DvzFramePlan* generic_graph = dvz_frame_plan("composition.generic-graph", 0);
    ANN(generic_graph);
    AT(_scene_panel_composition_lower_graph(generic_graph, &first, NULL));
    AT(generic_graph->graph_resource_count == 1);
    AT(generic_graph->graph_pass_count == first.pass_count);
    AT(strcmp(generic_graph->graph_resources[0].id, "rt") == 0);
    AT(generic_graph->graph_resources[0].kind == DVZ_FRAME_GRAPH_RESOURCE_EXTERNAL_TARGET);
    AT(generic_graph->realization_count > 0);
    for (uint32_t i = 0; i < first.pass_count; i++)
    {
        const DvzFrameGraphPass* graph_pass = &generic_graph->graph_passes[i];
        AT(graph_pass->has_composition_pass);
        AT(graph_pass->composition_pass_id.value == first.passes[i].id.value);
        AT(graph_pass->color_attachment_count == 1);
        AT(strcmp(graph_pass->color_attachments[0].resource_id, "rt") == 0);
        for (uint32_t j = 0; j < first.passes[i].binding_count; j++)
        {
            const DvzSceneWorkBinding* binding = &first.passes[i].bindings[j];
            ANN(_frame_plan_realization_get(
                generic_graph, first.panel_id, binding->ref_kind, binding->product_id,
                binding->scratch_id));
        }
    }
    AT(generic_graph->graph_passes[0].color_attachments[0].load_op ==
       DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR);
    AT(generic_graph->graph_passes[1].color_attachments[0].load_op ==
       DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD);
    dvz_frame_plan_destroy(generic_graph);

    DvzSceneMsaaTechniqueState msaa = {
        .enabled = true,
        .sample_count = 4,
        .alpha_to_coverage = true,
    };
    DvzPanelRenderPlan multisample_plan = plan;
    multisample_plan.msaa_state = &msaa;
    DvzPanelCompositionSnapshot multisample = {0};
    AT(_scene_panel_composition_resolve(&multisample_plan, &caps, &multisample, NULL));
    generic_graph = dvz_frame_plan("composition.generic-graph-msaa", 0);
    ANN(generic_graph);
    AT(_scene_panel_composition_lower_graph(generic_graph, &multisample, NULL));
    AT(generic_graph->graph_resource_count == 3);
    AT(generic_graph->graph_resources[2].sample_count == 4);
    AT(generic_graph->graph_passes[0].alpha_to_coverage);
    const DvzSceneWorkBinding* multisample_color = &multisample.passes[0].bindings[0];
    const DvzSceneGraphRealization* multisample_color_realization = _frame_plan_realization_get(
        generic_graph, multisample.panel_id, DVZ_SCENE_RESOURCE_REF_PRODUCT,
        multisample_color->product_id, (DvzSceneScratchResourceId){0});
    ANN(multisample_color_realization);
    AT(multisample_color_realization->graph_resource_index < generic_graph->graph_resource_count);
    AT(strcmp(
           generic_graph->graph_passes[0].color_attachments[0].resolve_resource_id,
           generic_graph->graph_resources[multisample_color_realization->graph_resource_index]
               .id) == 0);
    AT(multisample.passes[2].role == DVZ_FRAME_PLAN_RENDER_PASS_PRESENTATION);
    AT(strcmp(generic_graph->graph_passes[2].color_attachments[0].resource_id, "rt") == 0);
    dvz_frame_plan_destroy(generic_graph);

    generic_graph = dvz_frame_plan("composition.generic-graph-rollback", 0);
    ANN(generic_graph);
    DvzFrameGraphResource incompatible_rt = {0};
    dvz_strlcpy(incompatible_rt.id, "rt", sizeof(incompatible_rt.id));
    incompatible_rt.kind = DVZ_FRAME_GRAPH_RESOURCE_EXTERNAL_TARGET;
    incompatible_rt.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
    incompatible_rt.usage_flags =
        DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT | DVZ_FRAME_GRAPH_RESOURCE_USAGE_COPY_SRC;
    incompatible_rt.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
    AT(dvz_frame_plan_graph_resource(generic_graph, &incompatible_rt));
    AT(!_scene_panel_composition_lower_graph(generic_graph, &first, NULL));
    AT(generic_graph->graph_resource_count == 1);
    AT(generic_graph->graph_pass_count == 0);
    dvz_frame_plan_destroy(generic_graph);

    DvzPanelCompositionSnapshot invalid_graph_work = first;
    invalid_graph_work.passes[0].bindings[0].slot = DVZ_FRAME_PLAN_MAX_GRAPH_COLOR_ATTACHMENTS;
    invalid_graph_work.work_declaration_fingerprint =
        _frame_plan_composition_work_fingerprint(&invalid_graph_work);
    generic_graph = dvz_frame_plan("composition.generic-graph-slot", 0);
    ANN(generic_graph);
    AT(!_scene_panel_composition_lower_graph(generic_graph, &invalid_graph_work, NULL));
    AT(generic_graph->graph_resource_count == 0);
    AT(generic_graph->graph_pass_count == 0);
    dvz_frame_plan_destroy(generic_graph);

    invalid_graph_work = multisample;
    DvzSceneWorkBinding* invalid_multisample_load = &invalid_graph_work.passes[0].bindings[0];
    invalid_multisample_load->access = DVZ_SCENE_WORK_ACCESS_READ_WRITE;
    invalid_multisample_load->load = DVZ_SCENE_ATTACHMENT_LOAD_LOAD;
    invalid_multisample_load->clear = false;
    invalid_multisample_load->clear_value_kind = DVZ_SCENE_CLEAR_VALUE_NONE;
    invalid_multisample_load->load_source_ref_kind = DVZ_SCENE_RESOURCE_REF_PRODUCT;
    invalid_multisample_load->load_source_product_id = invalid_multisample_load->product_id;
    invalid_graph_work.work_declaration_fingerprint =
        _frame_plan_composition_work_fingerprint(&invalid_graph_work);
    generic_graph = dvz_frame_plan("composition.generic-graph-msaa-load", 0);
    ANN(generic_graph);
    AT(!_scene_panel_composition_lower_graph(generic_graph, &invalid_graph_work, NULL));
    AT(generic_graph->graph_resource_count == 0);
    AT(generic_graph->graph_pass_count == 0);
    dvz_frame_plan_destroy(generic_graph);

    DvzPanelCompositionSnapshot work_drift = first;
    work_drift.passes[0].provider = DVZ_SCENE_WORK_PROVIDER_EDL;
    DvzDiagnosticReport work_drift_report;
    dvz_diagnostic_report_init(&work_drift_report);
    AT(!_frame_plan_composition_validate(&work_drift, &work_drift_report));
    AT(strstr(dvz_diagnostic_report_get(&work_drift_report, 0), "declarative work drifts") !=
       NULL);

    work_drift = first;
    work_drift.passes[0].provider = (DvzSceneWorkProviderKey)UINT32_MAX;
    dvz_diagnostic_report_init(&work_drift_report);
    AT(!_frame_plan_composition_validate(&work_drift, &work_drift_report));
    AT(strstr(dvz_diagnostic_report_get(&work_drift_report, 0), "unknown work provider") != NULL);

    work_drift = first;
    work_drift.passes[1].bindings[0].load_source_product_id.value++;
    dvz_diagnostic_report_init(&work_drift_report);
    AT(!_frame_plan_composition_validate(&work_drift, &work_drift_report));
    AT(strstr(dvz_diagnostic_report_get(&work_drift_report, 0), "declarative work drifts") !=
       NULL);

    multisample_plan = plan;
    multisample_plan.visuals[1].layer = DVZ_SCENE_VISUAL_LAYER_VOLUME;
    multisample_plan.visuals[1].caps.layer = DVZ_SCENE_VISUAL_LAYER_VOLUME;
    multisample_plan.visuals[1].caps.phase_participation = DVZ_SCENE_VISUAL_PHASE_VOLUME_SHADING;
    multisample_plan.blended_visuals[0] = multisample_plan.visuals[1];
    multisample_plan.blended_visuals[0].blend_group = 0;
    DvzDiagnosticReport participation_report;
    dvz_diagnostic_report_init(&participation_report);
    memset(&second, 0, sizeof(second));
    AT(_scene_panel_composition_resolve(&multisample_plan, &caps, &second, NULL));
    AT(second.techniques[1].id == DVZ_SCENE_TECHNIQUE_VOLUME_SHADING);
    AT(second.techniques[1].phase == DVZ_SCENE_PHASE_VOLUME_SHADING);

    multisample_plan.visuals[1].caps.phase_participation |=
        DVZ_SCENE_VISUAL_PHASE_TRANSPARENT_SHADING;
    multisample_plan.blended_visuals[1] = plan.visuals[1];
    multisample_plan.blended_visuals[1].authored_order = 2;
    multisample_plan.blended_visuals[1].blend_group = 0;
    multisample_plan.blended_visual_count = 2;
    dvz_diagnostic_report_init(&participation_report);
    AT(!_scene_panel_composition_resolve(
        &multisample_plan, &caps, &second, &participation_report));
    AT(strstr(
           dvz_diagnostic_report_get(&participation_report, 0),
           "incompatible visual-layer participation") != NULL);

    multisample_plan = plan;
    multisample_plan.visuals[1].layer = DVZ_SCENE_VISUAL_LAYER_OVERLAY;
    multisample_plan.visuals[1].caps.layer = DVZ_SCENE_VISUAL_LAYER_OVERLAY;
    multisample_plan.visuals[1].caps.phase_participation =
        DVZ_SCENE_VISUAL_PHASE_TRANSPARENT_SHADING;
    multisample_plan.blended_visuals[0] = multisample_plan.visuals[1];
    multisample_plan.blended_visuals[0].blend_group = 0;
    dvz_diagnostic_report_init(&participation_report);
    AT(!_scene_panel_composition_resolve(
        &multisample_plan, &caps, &second, &participation_report));
    AT(strstr(dvz_diagnostic_report_get(&participation_report, 0), "overlay visual") != NULL);

    DvzPanelRenderPlan chain_plan = {0};
    dvz_snprintf(chain_plan.panel_id, sizeof(chain_plan.panel_id), "chain_p0");
    chain_plan.width = 64;
    chain_plan.height = 64;
    chain_plan.drawable_count = 5;
    chain_plan.visual_count = 5;
    chain_plan.visuals[0] = plan.visuals[0];
    chain_plan.visuals[0].authored_order = 0;
    for (uint32_t i = 1; i < 4; i++)
    {
        chain_plan.visuals[i] = plan.visuals[1];
        chain_plan.visuals[i].authored_order = i;
        chain_plan.visuals[i].visual_index = 10 + i;
    }
    chain_plan.visuals[4] = chain_plan.visuals[3];
    chain_plan.visuals[4].authored_order = 4;
    chain_plan.visuals[4].visual_index = 14;
    chain_plan.visuals[4].layer = DVZ_SCENE_VISUAL_LAYER_OVERLAY;
    chain_plan.visuals[4].caps.layer = DVZ_SCENE_VISUAL_LAYER_OVERLAY;
    chain_plan.visuals[4].caps.phase_participation = DVZ_SCENE_VISUAL_PHASE_OVERLAY;
    chain_plan.visuals[3].layer = DVZ_SCENE_VISUAL_LAYER_VOLUME;
    chain_plan.visuals[3].caps.layer = DVZ_SCENE_VISUAL_LAYER_VOLUME;
    chain_plan.visuals[3].caps.phase_participation = DVZ_SCENE_VISUAL_PHASE_VOLUME_SHADING;
    chain_plan.opaque_visuals[0] = chain_plan.visuals[0];
    chain_plan.opaque_visual_count = 1;
    chain_plan.has_transparent = true;
    chain_plan.blended_visuals[0] = chain_plan.visuals[1];
    chain_plan.blended_visuals[0].blend_group = 0;
    chain_plan.blended_visuals[1] = chain_plan.visuals[3];
    chain_plan.blended_visuals[1].blend_group = 1;
    chain_plan.blended_visuals[2] = chain_plan.visuals[4];
    chain_plan.blended_visuals[2].blend_group = 2;
    chain_plan.blended_visual_count = 3;
    chain_plan.blended_group_count = 3;
    chain_plan.wboit_visuals[0] = chain_plan.visuals[2];
    chain_plan.wboit_visual_count = 1;
    chain_plan.wboit_group_count = 1;
    chain_plan.transparent_passes[0] = (DvzPanelRenderTransparentPassPlan){
        .kind = DVZ_PANEL_RENDER_TRANSPARENT_BLENDED, .index = 0};
    chain_plan.transparent_passes[1] = (DvzPanelRenderTransparentPassPlan){
        .kind = DVZ_PANEL_RENDER_TRANSPARENT_WBOIT, .index = 0};
    chain_plan.transparent_passes[2] = (DvzPanelRenderTransparentPassPlan){
        .kind = DVZ_PANEL_RENDER_TRANSPARENT_BLENDED, .index = 1};
    chain_plan.transparent_passes[3] = (DvzPanelRenderTransparentPassPlan){
        .kind = DVZ_PANEL_RENDER_TRANSPARENT_BLENDED, .index = 2};
    chain_plan.transparent_pass_count = 4;
    caps.max_color_attachments = 2;
    DvzPanelCompositionSnapshot chain = {0};
    AT(_scene_panel_composition_resolve(&chain_plan, &caps, &chain, NULL));
    AT(chain.technique_count == 6);
    AT(chain.techniques[1].id == DVZ_SCENE_TECHNIQUE_TRANSPARENT_BLEND);
    AT(chain.techniques[2].id == DVZ_SCENE_TECHNIQUE_WBOIT);
    AT(chain.techniques[3].id == DVZ_SCENE_TECHNIQUE_VOLUME_SHADING);
    AT(chain.techniques[3].phase == DVZ_SCENE_PHASE_VOLUME_SHADING);
    AT(chain.techniques[1].output_ids[0].value == chain.techniques[2].input_ids[0].value);
    AT(chain.techniques[2].output_ids[0].value == chain.techniques[3].input_ids[0].value);
    AT(chain.techniques[1].instance_id.value != chain.techniques[3].instance_id.value);
    AT(chain.techniques[4].id == DVZ_SCENE_TECHNIQUE_OVERLAY_COMPOSITE);
    AT(chain.techniques[4].phase == DVZ_SCENE_PHASE_OVERLAY);
    AT(chain.techniques[4].input_ids[0].value == chain.techniques[3].output_ids[0].value);
    for (uint32_t i = 1; i <= 4; i++)
    {
        const DvzSceneResolvedTechnique* technique = &chain.techniques[i];
        const DvzSceneResolvedPass* scene_color_pass = NULL;
        const DvzSceneWorkBinding* scene_color_binding = NULL;
        for (uint32_t j = 0; j < chain.pass_count; j++)
        {
            const DvzSceneResolvedPass* pass = &chain.passes[j];
            if (pass->technique_instance_id.value != technique->instance_id.value)
                continue;
            for (uint32_t k = 0; k < pass->binding_count; k++)
            {
                if (pass->bindings[k].ref_kind == DVZ_SCENE_RESOURCE_REF_PRODUCT &&
                    pass->bindings[k].product_id.value == technique->output_ids[0].value &&
                    pass->bindings[k].load == DVZ_SCENE_ATTACHMENT_LOAD_LOAD)
                {
                    scene_color_pass = pass;
                    scene_color_binding = &pass->bindings[k];
                }
            }
        }
        ANN(scene_color_pass);
        ANN(scene_color_binding);
        AT(scene_color_binding->load_source_product_id.value == technique->input_ids[0].value);
    }

    DvzPanelRenderPlan occluded_chain = chain_plan;
    occluded_chain.scene_occlusion_enabled = true;
    occluded_chain.scene_occlusion[0] = occluded_chain.visuals[0];
    occluded_chain.scene_occlusion_count = 1;
    DvzPanelCompositionSnapshot occluded = {0};
    AT(_scene_panel_composition_resolve(&occluded_chain, &caps, &occluded, NULL));
    const uint64_t scene_occlusion_bit = UINT64_C(1) << DVZ_RENDER_PRODUCT_SCENE_OCCLUSION_DEPTH;
    uint32_t occluded_transparent_count = 0;
    for (uint32_t i = 0; i < occluded.technique_count; i++)
    {
        if (occluded.techniques[i].id == DVZ_SCENE_TECHNIQUE_TRANSPARENT_BLEND ||
            occluded.techniques[i].id == DVZ_SCENE_TECHNIQUE_WBOIT ||
            occluded.techniques[i].id == DVZ_SCENE_TECHNIQUE_VOLUME_SHADING)
        {
            AT((occluded.techniques[i].input_product_mask & scene_occlusion_bit) != 0);
            occluded_transparent_count++;
        }
    }
    AT(occluded_transparent_count == 3);

    const char* wboit_requirements[] = {
        "two color attachments", "rgba16float", "r16float", "sampling intermediate",
        "color blending"};
    DvzPanelRenderPlan wboit_plan = chain_plan;
    wboit_plan.drawable_count = 2;
    wboit_plan.visual_count = 2;
    wboit_plan.visuals[1] = chain_plan.visuals[2];
    wboit_plan.visuals[1].authored_order = 1;
    wboit_plan.blended_visual_count = 0;
    wboit_plan.blended_group_count = 0;
    wboit_plan.transparent_passes[0] = (DvzPanelRenderTransparentPassPlan){
        .kind = DVZ_PANEL_RENDER_TRANSPARENT_WBOIT, .index = 0};
    wboit_plan.transparent_pass_count = 1;
    for (uint32_t i = 0; i < 5; i++)
    {
        DvzCapabilitySnapshot missing = caps;
        if (i == 0)
            missing.max_color_attachments = 1;
        else if (i == 1)
            missing.render_target_format_rgba16float = false;
        else if (i == 2)
            missing.render_target_format_r16float = false;
        else if (i == 3)
            missing.supports_render_target_sampling = false;
        else
            missing.supports_color_blending = false;
        DvzDiagnosticReport capability_report;
        dvz_diagnostic_report_init(&capability_report);
        DvzPanelCompositionSnapshot rejected = {0};
        AT(!_scene_panel_composition_resolve(
            &wboit_plan, &missing, &rejected, &capability_report));
        AT(dvz_diagnostic_report_count(&capability_report) == 1);
        AT(strstr(dvz_diagnostic_report_get(&capability_report, 0), "WBOIT requires") != NULL);
        AT(strstr(dvz_diagnostic_report_get(&capability_report, 0), wboit_requirements[i]) !=
           NULL);
    }

    DvzPanelRenderPlan peel_plan = wboit_plan;
    peel_plan.transparent_passes[0].kind = DVZ_PANEL_RENDER_TRANSPARENT_DEPTH_PEEL;
    peel_plan.depth_peel_visuals[0] = peel_plan.wboit_visuals[0];
    peel_plan.depth_peel_visual_count = 1;
    peel_plan.depth_peel_group_count = 1;
    peel_plan.wboit_visual_count = 0;
    peel_plan.wboit_group_count = 0;
    caps.max_color_attachments = 3;
    const char* peel_requirements[] = {
        "three color attachments", "rgba16float", "sampling intermediate", "color blending"};
    for (uint32_t i = 0; i < 4; i++)
    {
        DvzCapabilitySnapshot missing = caps;
        if (i == 0)
            missing.max_color_attachments = 2;
        else if (i == 1)
            missing.render_target_format_rgba16float = false;
        else if (i == 2)
            missing.supports_render_target_sampling = false;
        else
            missing.supports_color_blending = false;
        DvzDiagnosticReport capability_report;
        dvz_diagnostic_report_init(&capability_report);
        DvzPanelCompositionSnapshot rejected = {0};
        AT(!_scene_panel_composition_resolve(&peel_plan, &missing, &rejected, &capability_report));
        AT(strstr(dvz_diagnostic_report_get(&capability_report, 0), "depth peeling requires") !=
           NULL);
        AT(strstr(dvz_diagnostic_report_get(&capability_report, 0), peel_requirements[i]) != NULL);
    }

    DvzPanelRenderPlan occluded_peel = peel_plan;
    occluded_peel.scene_occlusion_enabled = true;
    occluded_peel.scene_occlusion[0] = occluded_peel.visuals[0];
    occluded_peel.scene_occlusion_count = 1;
    DvzPanelCompositionSnapshot occluded_peel_composed = {0};
    AT(_scene_panel_composition_resolve(&occluded_peel, &caps, &occluded_peel_composed, NULL));
    bool found_occluded_peel = false;
    for (uint32_t i = 0; i < occluded_peel_composed.technique_count; i++)
    {
        const DvzSceneResolvedTechnique* technique = &occluded_peel_composed.techniques[i];
        if (technique->id == DVZ_SCENE_TECHNIQUE_DEPTH_PEEL)
        {
            AT((technique->input_product_mask & scene_occlusion_bit) != 0);
            found_occluded_peel = true;
        }
    }
    AT(found_occluded_peel);

    generic_graph = dvz_frame_plan("composition.generic-chain", 0);
    ANN(generic_graph);
    AT(_scene_panel_composition_lower_graph(generic_graph, &chain, NULL));
    AT(generic_graph->graph_pass_count == chain.pass_count);
    dvz_frame_plan_destroy(generic_graph);

    generic_graph = dvz_frame_plan("composition.generic-peel", 0);
    ANN(generic_graph);
    AT(_scene_panel_composition_lower_graph(generic_graph, &occluded_peel_composed, NULL));
    AT(generic_graph->graph_pass_count == occluded_peel_composed.pass_count);
    dvz_frame_plan_destroy(generic_graph);

    DvzCapabilitySnapshot no_blend = caps;
    no_blend.supports_color_blending = false;
    DvzDiagnosticReport blend_report;
    dvz_diagnostic_report_init(&blend_report);
    DvzPanelCompositionSnapshot rejected_blend = {0};
    AT(!_scene_panel_composition_resolve(&plan, &no_blend, &rejected_blend, &blend_report));
    AT(strstr(dvz_diagnostic_report_get(&blend_report, 0), "alpha blending requires") != NULL);

    DvzFramePlan* persisted = dvz_frame_plan("composition.persistence", 0);
    ANN(persisted);
    AT(_frame_plan_composition_append(persisted, &first, NULL));
    AT(persisted->composition_count == 1);
    const DvzPanelCompositionSnapshot* stored =
        _frame_plan_composition_get(persisted, "figure_0_p0");
    ANN(stored);
    AT(stored != &first);
    AT(memcmp(stored, &first, sizeof(first)) == 0);
    DvzDiagnosticReport persistence_report;
    dvz_diagnostic_report_init(&persistence_report);
    AT(!_frame_plan_composition_append(persisted, &first, &persistence_report));
    AT(persisted->composition_count == 1);
    AT(strstr(dvz_diagnostic_report_get(&persistence_report, 0), "duplicate") != NULL);
    dvz_frame_plan_destroy(persisted);

    DvzPanelCompositionSnapshot invalid_snapshot = first;
    invalid_snapshot.techniques[1].output_ids[0] = invalid_snapshot.techniques[0].output_ids[0];
    DvzDiagnosticReport validation_report;
    dvz_diagnostic_report_init(&validation_report);
    AT(!_frame_plan_composition_validate(&invalid_snapshot, &validation_report));
    AT(strstr(dvz_diagnostic_report_get(&validation_report, 0), "ambiguous producers") != NULL);

    invalid_snapshot = first;
    invalid_snapshot.passes[1].id = invalid_snapshot.passes[0].id;
    dvz_diagnostic_report_init(&validation_report);
    AT(!_frame_plan_composition_validate(&invalid_snapshot, &validation_report));
    AT(strstr(dvz_diagnostic_report_get(&validation_report, 0), "duplicate pass identity") !=
       NULL);

    invalid_snapshot = first;
    invalid_snapshot.techniques[1].instance_id = invalid_snapshot.techniques[0].instance_id;
    dvz_diagnostic_report_init(&validation_report);
    AT(!_frame_plan_composition_validate(&invalid_snapshot, &validation_report));
    AT(strstr(dvz_diagnostic_report_get(&validation_report, 0), "duplicate technique instance") !=
       NULL);

    invalid_snapshot = first;
    invalid_snapshot.techniques[0].must_follow_phase_mask =
        1u << (uint32_t)DVZ_SCENE_PHASE_TRANSPARENT_SHADING;
    dvz_diagnostic_report_init(&validation_report);
    AT(!_frame_plan_composition_validate(&invalid_snapshot, &validation_report));
    AT(strstr(dvz_diagnostic_report_get(&validation_report, 0), "must_follow") != NULL);

    DvzFramePlan* contract_rejections = dvz_frame_plan("composition.contract-rejections", 0);
    ANN(contract_rejections);

    invalid_snapshot = first;
    invalid_snapshot.techniques[0].version++;
    dvz_diagnostic_report_init(&validation_report);
    AT(!_frame_plan_composition_validate(&invalid_snapshot, &validation_report));
    AT(strstr(dvz_diagnostic_report_get(&validation_report, 0), "immutable contract") != NULL);
    AT(!_frame_plan_composition_append(contract_rejections, &invalid_snapshot, NULL));
    AT(contract_rejections->composition_count == 0);

    invalid_snapshot = first;
    invalid_snapshot.techniques[0].phase = DVZ_SCENE_PHASE_SURFACE_POSTPROCESS;
    invalid_snapshot.passes[0].phase = DVZ_SCENE_PHASE_SURFACE_POSTPROCESS;
    dvz_diagnostic_report_init(&validation_report);
    AT(!_frame_plan_composition_validate(&invalid_snapshot, &validation_report));
    AT(strstr(dvz_diagnostic_report_get(&validation_report, 0), "immutable contract") != NULL);
    AT(!_frame_plan_composition_append(contract_rejections, &invalid_snapshot, NULL));
    AT(contract_rejections->composition_count == 0);

    invalid_snapshot = first;
    invalid_snapshot.techniques[0].participating_layer_mask ^=
        1u << (uint32_t)DVZ_SCENE_VISUAL_LAYER_OVERLAY;
    dvz_diagnostic_report_init(&validation_report);
    AT(!_frame_plan_composition_validate(&invalid_snapshot, &validation_report));
    AT(strstr(dvz_diagnostic_report_get(&validation_report, 0), "immutable contract") != NULL);
    AT(!_frame_plan_composition_append(contract_rejections, &invalid_snapshot, NULL));
    AT(contract_rejections->composition_count == 0);

    invalid_snapshot = first;
    invalid_snapshot.techniques[1].required_capability_mask = 0;
    dvz_diagnostic_report_init(&validation_report);
    AT(!_frame_plan_composition_validate(&invalid_snapshot, &validation_report));
    AT(strstr(dvz_diagnostic_report_get(&validation_report, 0), "immutable contract") != NULL);
    AT(!_frame_plan_composition_append(contract_rejections, &invalid_snapshot, NULL));
    AT(contract_rejections->composition_count == 0);

    invalid_snapshot = first;
    invalid_snapshot.techniques[1].fallback = DVZ_SCENE_TECHNIQUE_FALLBACK_DISABLE_OPTIONAL;
    dvz_diagnostic_report_init(&validation_report);
    AT(!_frame_plan_composition_validate(&invalid_snapshot, &validation_report));
    AT(strstr(dvz_diagnostic_report_get(&validation_report, 0), "fallback semantics") != NULL);
    AT(!_frame_plan_composition_append(contract_rejections, &invalid_snapshot, NULL));
    AT(contract_rejections->composition_count == 0);

    invalid_snapshot = first;
    invalid_snapshot.techniques[1].expansion_flags = 1;
    dvz_diagnostic_report_init(&validation_report);
    AT(!_frame_plan_composition_validate(&invalid_snapshot, &validation_report));
    AT(strstr(dvz_diagnostic_report_get(&validation_report, 0), "undeclared expansion") != NULL);
    AT(!_frame_plan_composition_append(contract_rejections, &invalid_snapshot, NULL));
    AT(contract_rejections->composition_count == 0);

    invalid_snapshot = first;
    invalid_snapshot.pass_count--;
    dvz_diagnostic_report_init(&validation_report);
    AT(!_frame_plan_composition_validate(&invalid_snapshot, &validation_report));
    AT(strstr(dvz_diagnostic_report_get(&validation_report, 0), "missing a declared pass") !=
       NULL);
    AT(!_frame_plan_composition_append(contract_rejections, &invalid_snapshot, NULL));
    AT(contract_rejections->composition_count == 0);

    invalid_snapshot = first;
    invalid_snapshot.passes[invalid_snapshot.pass_count] =
        invalid_snapshot.passes[invalid_snapshot.pass_count - 1];
    invalid_snapshot.passes[invalid_snapshot.pass_count].id.value =
        invalid_snapshot.pass_count + 1;
    invalid_snapshot.passes[invalid_snapshot.pass_count].ordinal++;
    invalid_snapshot.pass_count++;
    dvz_diagnostic_report_init(&validation_report);
    AT(!_frame_plan_composition_validate(&invalid_snapshot, &validation_report));
    AT(strstr(dvz_diagnostic_report_get(&validation_report, 0), "undeclared extra passes") !=
       NULL);
    AT(!_frame_plan_composition_append(contract_rejections, &invalid_snapshot, NULL));
    AT(contract_rejections->composition_count == 0);

    invalid_snapshot = first;
    invalid_snapshot.passes[0].ordinal++;
    dvz_diagnostic_report_init(&validation_report);
    AT(!_frame_plan_composition_validate(&invalid_snapshot, &validation_report));
    AT(strstr(dvz_diagnostic_report_get(&validation_report, 0), "pass-template drift") != NULL);
    AT(!_frame_plan_composition_append(contract_rejections, &invalid_snapshot, NULL));
    AT(contract_rejections->composition_count == 0);

    dvz_frame_plan_destroy(contract_rejections);

    const uint64_t color_bit = UINT64_C(1) << DVZ_RENDER_PRODUCT_SCENE_COLOR;
    DvzPanelCompositionSnapshot cycle = {
        .valid = true,
        .width = 1,
        .height = 1,
        .render_scale = 1.0f,
        .local_to_target = {1.0f, 1.0f, 0.0f, 0.0f},
        .required_product_mask = color_bit,
        .technique_count = 2,
    };
    dvz_snprintf(cycle.panel_id, sizeof(cycle.panel_id), "cycle_p0");
    cycle.techniques[0] = (DvzSceneResolvedTechnique){
        .instance_id = {.value = 1},
        .id = DVZ_SCENE_TECHNIQUE_OPAQUE_SHADING,
        .version = 1,
        .phase = DVZ_SCENE_PHASE_OPAQUE_SHADING,
        .input_product_mask = color_bit,
        .output_product_mask = color_bit,
        .input_count = 1,
        .inputs = {DVZ_RENDER_PRODUCT_SCENE_COLOR},
        .input_ids = {{.value = 2}},
        .output_count = 1,
        .outputs = {DVZ_RENDER_PRODUCT_SCENE_COLOR},
        .output_ids = {{.value = 1}},
    };
    cycle.techniques[1] = (DvzSceneResolvedTechnique){
        .instance_id = {.value = 2},
        .id = DVZ_SCENE_TECHNIQUE_TRANSPARENT_BLEND,
        .version = 1,
        .phase = DVZ_SCENE_PHASE_TRANSPARENT_SHADING,
        .input_product_mask = color_bit,
        .output_product_mask = color_bit,
        .input_count = 1,
        .inputs = {DVZ_RENDER_PRODUCT_SCENE_COLOR},
        .input_ids = {{.value = 1}},
        .output_count = 1,
        .outputs = {DVZ_RENDER_PRODUCT_SCENE_COLOR},
        .output_ids = {{.value = 2}},
    };
    dvz_diagnostic_report_init(&validation_report);
    AT(!_frame_plan_composition_validate(&cycle, &validation_report));
    AT(strstr(dvz_diagnostic_report_get(&validation_report, 0), "dependency cycle") != NULL);

    DvzPanelCompositionSnapshot repeated = first;
    repeated.techniques[3] = repeated.techniques[2];
    repeated.techniques[3].instance_id.value = 4;
    repeated.techniques[2] = repeated.techniques[1];
    repeated.techniques[2].instance_id.value = 3;
    repeated.techniques[2].input_ids[0] = repeated.techniques[1].output_ids[0];
    repeated.techniques[2].output_ids[0].value = 4;
    repeated.techniques[3].input_ids[0] = repeated.techniques[2].output_ids[0];
    repeated.technique_count = 4;
    repeated.passes[2] = repeated.passes[1];
    repeated.passes[2].id.value = 3;
    repeated.passes[2].technique_instance_id = repeated.techniques[2].instance_id;
    repeated.passes[2].ordinal = 1;
    repeated.passes[2].bindings[0].product_id = repeated.techniques[2].output_ids[0];
    repeated.passes[2].bindings[0].load_source_product_id = repeated.techniques[2].input_ids[0];
    repeated.pass_count = 3;
    repeated.work_declaration_fingerprint = _frame_plan_composition_work_fingerprint(&repeated);
    AT(repeated.techniques[1].id == repeated.techniques[2].id);
    AT(_frame_plan_composition_validate(&repeated, NULL));

    DvzScene* empty_scene = dvz_scene();
    ANN(empty_scene);
    DvzFigure* empty_figure = dvz_figure(empty_scene, 64, 64, 0);
    ANN(empty_figure);
    DvzPanel* empty_panel = dvz_panel(empty_figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(empty_panel);
    DvzFramePlan* empty_plan = dvz_frame_plan("empty", 0);
    ANN(empty_plan);
    AT(_scene_emit_panel_render(empty_figure, 0, empty_plan, "empty"));
    const DvzPanelCompositionSnapshot* empty_snapshot =
        _frame_plan_composition_get(empty_plan, "empty_p0");
    ANN(empty_snapshot);
    AT(empty_snapshot->valid);
    AT(empty_snapshot->technique_count == 0);
    AT(empty_snapshot->pass_count == 0);
    dvz_frame_plan_destroy(empty_plan);
    dvz_scene_destroy(empty_scene);

    caps.max_color_attachments = 4;
    DvzPanelRenderPlan effects = plan;
    DvzSceneAoTechniqueState gtao = {
        .enabled = true, .denoise_enabled = true, .debug_mode = DVZ_AO_DEBUG_VISIBILITY};
    DvzSceneEdlTechniqueState edl = {.enabled = true};
    effects.ao_enabled = true;
    effects.ao_state = &gtao;
    effects.edl_enabled = true;
    effects.edl_state = &edl;
    effects.edl_has_depth_producer = true;
    effects.gbuffer_visuals[0] = effects.visuals[0];
    effects.gbuffer_visual_count = 1;
    DvzPanelCompositionSnapshot composed = {0};
    AT(_scene_panel_composition_resolve(&effects, &caps, &composed, NULL));
    bool found_surface_capture_caps = false;
    bool found_gtao_visibility_presentation_caps = false;
    for (uint32_t i = 0; i < composed.technique_count; i++)
    {
        const DvzSceneResolvedTechnique* technique = &composed.techniques[i];
        if (technique->id == DVZ_SCENE_TECHNIQUE_SURFACE_CAPTURE)
        {
            AT(technique->required_capability_mask != 0);
            AT(technique->missing_capability_mask == 0);
            found_surface_capture_caps = true;
        }
        else if (technique->id == DVZ_SCENE_TECHNIQUE_GTAO_VISIBILITY_PRESENTATION)
        {
            AT(technique->required_capability_mask == 0x01u);
            AT(technique->missing_capability_mask == 0);
            found_gtao_visibility_presentation_caps = true;
        }
    }
    AT(found_surface_capture_caps);
    AT(found_gtao_visibility_presentation_caps);
    bool found_gbuffer_work = false;
    bool found_gtao_denoise_work = false;
    bool found_gtao_visibility_presentation_work = false;
    bool found_edl_work = false;
    for (uint32_t i = 0; i < composed.pass_count; i++)
    {
        const DvzSceneResolvedPass* pass = &composed.passes[i];
        if (pass->role == DVZ_FRAME_PLAN_RENDER_PASS_GBUFFER)
        {
            AT(pass->binding_count == 4);
            found_gbuffer_work = true;
        }
        else if (pass->role == DVZ_FRAME_PLAN_RENDER_PASS_GTAO_DENOISE)
        {
            AT(pass->binding_count == 5);
            AT(pass->bindings[0].binding == 0);
            AT(pass->bindings[1].binding == 1);
            AT(pass->bindings[2].binding == 2);
            AT(pass->bindings[3].binding == 3);
            found_gtao_denoise_work = true;
        }
        else if (pass->role == DVZ_FRAME_PLAN_RENDER_PASS_GTAO_VISIBILITY_PRESENTATION)
        {
            AT(pass->binding_count == 2);
            AT(pass->auxiliary_binding_count == 0);
            AT(pass->bindings[0].usage == DVZ_SCENE_WORK_BINDING_SAMPLED);
            AT(pass->bindings[1].load == DVZ_SCENE_ATTACHMENT_LOAD_LOAD);
            found_gtao_visibility_presentation_work = true;
        }
        else if (pass->role == DVZ_FRAME_PLAN_RENDER_PASS_EDL_RESOLVE)
        {
            AT(pass->binding_count == 3);
            AT(pass->bindings[0].binding == 0);
            AT(pass->bindings[1].binding == 1);
            AT(pass->bindings[2].clear_value_kind == DVZ_SCENE_CLEAR_VALUE_FRAME);
            found_edl_work = true;
        }
    }
    AT(found_gbuffer_work);
    AT(found_gtao_denoise_work);
    AT(found_gtao_visibility_presentation_work);
    AT(found_edl_work);
    bool found_surface_device_depth = false;
    bool found_edl_color_scratch = false;
    for (uint32_t i = 0; i < composed.scratch_resource_count; i++)
    {
        const DvzSceneScratchResource* scratch = &composed.scratch_resources[i];
        AT(scratch->extent_policy == DVZ_RENDER_PRODUCT_EXTENT_PANEL_RELATIVE);
        if (scratch->kind == DVZ_SCENE_SCRATCH_Z_ONLY_DEPTH)
        {
            AT(scratch->format == DVZ_FORMAT_D32_SFLOAT);
            found_surface_device_depth = true;
        }
        else if (scratch->kind == DVZ_SCENE_SCRATCH_EDL_COLOR)
        {
            AT(scratch->format == DVZ_FORMAT_R8G8B8A8_UNORM);
            found_edl_color_scratch = true;
        }
    }
    AT(found_surface_device_depth);
    AT(!found_edl_color_scratch);

    generic_graph = dvz_frame_plan("composition.generic-effects", 0);
    ANN(generic_graph);
    dvz_diagnostic_report_init(&validation_report);
    AT(_scene_panel_composition_lower_graph(generic_graph, &composed, &validation_report));
    AT(dvz_diagnostic_report_count(&validation_report) == 0);
    AT(generic_graph->graph_pass_count == composed.pass_count);
    AT(generic_graph->graph_resource_count > composed.scratch_resource_count);
    for (uint32_t i = 0; i < composed.pass_count; i++)
        AT(generic_graph->graph_passes[i].composition_pass_id.value ==
           composed.passes[i].id.value);
    const uint32_t first_panel_resource_count = generic_graph->graph_resource_count;
    DvzPanelCompositionSnapshot adjacent_composed = composed;
    dvz_strlcpy(adjacent_composed.panel_id, "figure_0_p1", sizeof(adjacent_composed.panel_id));
    AT(_scene_panel_composition_lower_graph(generic_graph, &adjacent_composed, NULL));
    AT(generic_graph->graph_pass_count == 2 * composed.pass_count);
    AT(generic_graph->graph_resource_count == 2 * first_panel_resource_count - 1);
    AT(generic_graph->realization_count > DVZ_FRAME_PLAN_INITIAL_GRAPH_RESOURCE_CAPACITY);
    dvz_frame_plan_destroy(generic_graph);

    DvzCapabilitySnapshot no_surface_rgba = caps;
    no_surface_rgba.render_target_format_rgba16float = false;
    DvzPanelCompositionSnapshot surface_fallback = {0};
    AT(!_scene_panel_composition_resolve(&effects, &no_surface_rgba, &surface_fallback, NULL));

    DvzCapabilitySnapshot no_ambient_sampling = caps;
    no_ambient_sampling.supports_render_target_sampling = false;
    DvzPanelCompositionSnapshot ambient_fallback = {0};
    AT(!_scene_panel_composition_resolve(&effects, &no_ambient_sampling, &ambient_fallback, NULL));
    uint32_t edl_pass = UINT32_MAX;
    uint32_t transparent_pass = UINT32_MAX;
    for (uint32_t i = 0; i < composed.pass_count; i++)
    {
        if (composed.passes[i].role == DVZ_FRAME_PLAN_RENDER_PASS_EDL_RESOLVE)
            edl_pass = i;
        if (composed.passes[i].role == DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND)
            transparent_pass = i;
    }
    AT(edl_pass != UINT32_MAX);
    AT(transparent_pass != UINT32_MAX);
    AT(edl_pass < transparent_pass);
    AT(composed.passes[edl_pass].phase == DVZ_SCENE_PHASE_SURFACE_POSTPROCESS);
    AT(composed.passes[transparent_pass].phase == DVZ_SCENE_PHASE_TRANSPARENT_SHADING);

    DvzCapabilitySnapshot legacy_caps = dvz_capability_snapshot();
    legacy_caps.supports_color_blending = true;
    legacy_caps.render_target_format_rgba16float = true;
    DvzPanelCompositionSnapshot legacy = {0};
    AT(!_scene_panel_composition_resolve(&effects, &legacy_caps, &legacy, NULL));

    DvzPanelCompositionSnapshot unchanged;
    memset(&unchanged, 0xA5, sizeof(unchanged));
    DvzPanelCompositionSnapshot sentinel = unchanged;
    DvzPanelRenderPlan invalid = plan;
    invalid.visuals[1].authored_order = 0;
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    AT(!_scene_panel_composition_resolve(&invalid, &caps, &unchanged, &report));
    AT(memcmp(&unchanged, &sentinel, sizeof(unchanged)) == 0);
    AT(dvz_diagnostic_report_count(&report) == 1);
    AT(strstr(dvz_diagnostic_report_get(&report, 0), "authored order") != NULL);

    invalid = plan;
    invalid.scene_occlusion_enabled = true;
    dvz_diagnostic_report_init(&report);
    AT(!_scene_panel_composition_resolve(&invalid, &caps, &unchanged, &report));
    AT(dvz_diagnostic_report_count(&report) == 1);
    AT(strstr(dvz_diagnostic_report_get(&report, 0), "no product producer") != NULL);
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
    AT(_scene_draw_contract_resolve(&facts, DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE, &contract));
    AT(contract.visual_type == DVZ_VISUAL_TYPE_MESH);
    AT(contract.alpha_mode == DVZ_ALPHA_OPAQUE);
    AT(contract.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE);
    AT(contract.depth_test);
    AT(contract.depth_write);
    AT(!contract.samples_depth);
    AT(contract.depth_policy == (DVZ_SCENE_DEPTH_POLICY_TEST | DVZ_SCENE_DEPTH_POLICY_WRITE));
    AT(contract.blend_policy == DVZ_SCENE_BLEND_POLICY_OPAQUE);
    AT(contract.shader_feature_mask == 0);
    AT(contract.bind_group_layout_mask ==
       (DVZ_SCENE_BIND_GROUP_REQUIREMENT_COMMON | DVZ_SCENE_BIND_GROUP_REQUIREMENT_MATERIAL));
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
    AT(_scene_draw_contract_resolve(&facts, DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE, &contract));
    AT(contract.visual_type == DVZ_VISUAL_TYPE_SEGMENT);
    AT(contract.depth_test);
    AT(contract.depth_write);
    AT(contract.blend_policy == DVZ_SCENE_BLEND_POLICY_SEGMENT_COVERAGE);
    AT(contract.blend_target_count == 1);
    AT(contract.blend_targets[0].blend_enabled);
    AT(contract.blend_targets[0].src_color_blend_factor == DVZ_BLEND_FACTOR_SRC_ALPHA);
    AT(contract.blend_targets[0].dst_color_blend_factor == DVZ_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);

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
    AT(contract.shader_feature_mask ==
       (DVZ_SCENE_SHADER_FEATURE_SAMPLE_DEPTH | DVZ_SCENE_SHADER_FEATURE_SAMPLE_VOLUME_OCCLUSION |
        DVZ_SCENE_SHADER_FEATURE_SAMPLE_SCENE_OCCLUSION));
    AT(contract.bind_group_layout_mask ==
       (DVZ_SCENE_BIND_GROUP_REQUIREMENT_COMMON | DVZ_SCENE_BIND_GROUP_REQUIREMENT_VOLUME |
        DVZ_SCENE_BIND_GROUP_REQUIREMENT_SCENE_OCCLUSION));
    AT(contract.needs_common_set);
    AT(contract.needs_volume_set);
    AT(contract.needs_scene_occlusion_set);

    facts = (DvzSceneDrawFacts){
        .visual_type = DVZ_VISUAL_TYPE_MARKER,
        .alpha_mode = DVZ_ALPHA_OPAQUE,
        .uses_common_set = true,
    };
    AT(_scene_draw_contract_resolve(
        &facts, DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND, &contract));
    AT(!_draw_pass_role_matches(&contract));
    facts.overlay_composite = true;
    AT(_scene_draw_contract_resolve(
        &facts, DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND, &contract));
    AT(contract.overlay_composite);
    AT(_draw_pass_role_matches(&contract));

    facts = (DvzSceneDrawFacts){
        .visual_type = DVZ_VISUAL_TYPE_POINT,
        .alpha_mode = DVZ_ALPHA_BLENDED,
        .blend_mode = DVZ_BLEND_ADDITIVE,
        .can_depth_test = true,
        .uses_common_set = true,
    };
    AT(_scene_draw_contract_resolve(
        &facts, DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND, &contract));
    AT(contract.alpha_mode == DVZ_ALPHA_BLENDED);
    AT(contract.blend_mode == DVZ_BLEND_ADDITIVE);
    AT(contract.blend_policy == DVZ_SCENE_BLEND_POLICY_ADDITIVE);
    AT(contract.blend_target_count == 1);
    AT(contract.blend_targets[0].blend_enabled);
    AT(contract.blend_targets[0].src_color_blend_factor == DVZ_BLEND_FACTOR_SRC_ALPHA);
    AT(contract.blend_targets[0].dst_color_blend_factor == DVZ_BLEND_FACTOR_ONE);
    AT(contract.blend_targets[0].src_alpha_blend_factor == DVZ_BLEND_FACTOR_ONE);
    AT(contract.blend_targets[0].dst_alpha_blend_factor == DVZ_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);

    facts.alpha_mode = DVZ_ALPHA_WBOIT;
    AT(!_scene_draw_contract_resolve(
        &facts, DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION, &contract));

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
    AT(contract.depth_policy == (DVZ_SCENE_DEPTH_POLICY_TEST | DVZ_SCENE_DEPTH_POLICY_WRITE));
    AT(contract.blend_policy == DVZ_SCENE_BLEND_POLICY_NONE);
    AT(contract.shader_feature_mask == DVZ_SCENE_SHADER_FEATURE_WRITE_SCENE_OCCLUSION);
    AT(!contract.samples_scene_occlusion);
    AT(!contract.needs_scene_occlusion_set);

    return 0;
}


/**
 * Verify every render-pass role has one centralized fixed-function policy.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_role_policy_mapping_complete(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    const DvzFramePlanRenderPassRole roles[] = {
        DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE,
        DVZ_FRAME_PLAN_RENDER_PASS_GBUFFER,
        DVZ_FRAME_PLAN_RENDER_PASS_VOLUME_OCCLUSION,
        DVZ_FRAME_PLAN_RENDER_PASS_SCENE_OCCLUSION,
        DVZ_FRAME_PLAN_RENDER_PASS_GTAO,
        DVZ_FRAME_PLAN_RENDER_PASS_GTAO_DENOISE,
        DVZ_FRAME_PLAN_RENDER_PASS_GTAO_VISIBILITY_PRESENTATION,
        DVZ_FRAME_PLAN_RENDER_PASS_EDL_RESOLVE,
        DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION,
        DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND,
        DVZ_FRAME_PLAN_RENDER_PASS_WBOIT_RESOLVE,
        DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT,
        DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER,
        DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_COMPOSITE,
        DVZ_FRAME_PLAN_RENDER_PASS_PRESENTATION,
        DVZ_FRAME_PLAN_RENDER_PASS_PICKING,
    };
    for (uint32_t i = 0; i < sizeof(roles) / sizeof(roles[0]); i++)
    {
        DvzSceneTechniquePassPolicy policy = {0};
        AT(_scene_technique_pass_policy(roles[i], &policy));
        AT(policy.role == roles[i]);
    }
    DvzSceneTechniquePassPolicy policy = {0};
    AT(_scene_technique_pass_policy(DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND, &policy));
    AT(policy.transparent_blend);
    AT(!policy.wboit_accumulation);
    AT(!policy.depth_peel);
    AT(!policy.fullscreen_resolve);
    AT(policy.sampled_texture_binding_count == 0);

    AT(_scene_technique_pass_policy(DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION, &policy));
    AT(!policy.transparent_blend);
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

    AT(_scene_technique_pass_policy(
        DVZ_FRAME_PLAN_RENDER_PASS_GTAO_VISIBILITY_PRESENTATION, &policy));
    AT(policy.fullscreen_resolve);

    AT(_scene_technique_pass_policy(DVZ_FRAME_PLAN_RENDER_PASS_PRESENTATION, &policy));
    AT(policy.fullscreen_resolve);
    AT(policy.sampled_texture_binding_count == 1);


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
    contract.transparent_blend = true;
    contract.draw_count = 1;
    contract.draws[0].alpha_mode = DVZ_ALPHA_BLENDED;
    contract.draws[0].pass_role = DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND;
    contract.draws[0].depth_test = true;
    dvz_diagnostic_report_init(&report);
    AT(!_scene_pass_contract_validate(&contract, &report));
    AT(dvz_diagnostic_report_count(&report) > 0);

    dvz_memset(&contract, sizeof(contract), 0, sizeof(contract));
    contract.role = DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND;
    contract.transparent_blend = true;
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
    contract.transparent_blend = true;
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
    contract.attachment_count = 1;
    contract.sampled_read_count = 1;
    contract.attachments[0].role = DVZ_SCENE_ATTACHMENT_SAMPLED;
    contract.attachments[0].read = true;
    dvz_snprintf(
        contract.attachments[0].resource_id, sizeof(contract.attachments[0].resource_id),
        "legacy.scene_occlusion.depth");
    dvz_diagnostic_report_init(&report);
    AT(!_scene_pass_contract_validate(&contract, &report));
    AT(dvz_diagnostic_report_count(&report) > 0);

    dvz_memset(&contract, sizeof(contract), 0, sizeof(contract));
    contract.role = DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND;
    contract.transparent_blend = true;
    contract.draw_count = 1;
    contract.draws[0].alpha_mode = DVZ_ALPHA_BLENDED;
    contract.draws[0].pass_role = DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND;
    contract.draws[0].samples_volume_occlusion = true;
    contract.attachment_count = 1;
    contract.sampled_read_count = 1;
    contract.attachments[0].role = DVZ_SCENE_ATTACHMENT_SAMPLED;
    contract.attachments[0].read = true;
    dvz_snprintf(
        contract.attachments[0].resource_id, sizeof(contract.attachments[0].resource_id),
        "legacy.volume_occlusion.depth");
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
    contract.draws[0].scene_occlusion_bind_set = DVZ_SCENE_SHADER_SET_VISUAL;
    contract.draws[0].scene_occlusion_bind_binding = 0;
    dvz_snprintf(
        contract.draws[0].scene_occlusion_resource_id,
        sizeof(contract.draws[0].scene_occlusion_resource_id), "typed.scene.depth");
    dvz_snprintf(
        contract.draws[0].scene_occlusion_producer_pass_id,
        sizeof(contract.draws[0].scene_occlusion_producer_pass_id), "typed.scene.producer");
    contract.attachment_count = 1;
    contract.sampled_read_count = 1;
    contract.attachments[0].role = DVZ_SCENE_ATTACHMENT_SAMPLED;
    contract.attachments[0].read = true;
    dvz_snprintf(
        contract.attachments[0].resource_id, sizeof(contract.attachments[0].resource_id),
        "typed.scene.depth");
    dvz_snprintf(
        contract.attachments[0].producer_pass_id, sizeof(contract.attachments[0].producer_pass_id),
        "wrong.scene.producer");
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
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
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

    DvzPanelDesc panel0_desc = panel->desc;
    DvzPanel* panel1 = dvz_panel(figure, &(DvzPanelDesc){0.5f, 0.0f, 0.5f, 1.0f});
    AT(panel1 != NULL);
    plan = dvz_frame_plan("figure_0", 0);
    ANN(plan);
    AT(dvz_frame_plan_render_panel_role(
        plan, "figure_0_p0", "rt.0", false, panel0_desc, DVZ_FRAME_PLAN_RENDER_PASS_GBUFFER));
    AT(dvz_frame_plan_render_panel_role(
        plan, "figure_0_p1", "rt.1", false, panel1->desc, DVZ_FRAME_PLAN_RENDER_PASS_GBUFFER));
    DvzFramePlanNode* render0 = &plan->nodes[0];
    DvzFramePlanNode* render1 = &plan->nodes[1];
    render0->u.render.has_composition_pass = true;
    render0->u.render.composition_pass_id = (DvzFramePlanPassId){1};
    render0->u.render.has_graph_pass_index = true;
    render0->u.render.graph_pass_index = 1;
    render1->u.render.has_composition_pass = true;
    render1->u.render.composition_pass_id = (DvzFramePlanPassId){1};
    render1->u.render.has_graph_pass_index = true;
    render1->u.render.graph_pass_index = 0;

    DvzFrameGraphPass pass0 = {0};
    dvz_strlcpy(pass0.id, "figure_0_p0.gbuffer", sizeof(pass0.id));
    dvz_strlcpy(pass0.panel_id, "figure_0_p0", sizeof(pass0.panel_id));
    pass0.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    pass0.has_composition_pass = true;
    pass0.composition_pass_id = (DvzFramePlanPassId){1};
    AT(dvz_frame_plan_graph_pass(plan, &pass0));
    DvzFrameGraphPass pass1 = pass0;
    dvz_strlcpy(pass1.id, "figure_0_p1.gbuffer", sizeof(pass1.id));
    dvz_strlcpy(pass1.panel_id, "figure_0_p1", sizeof(pass1.panel_id));
    AT(dvz_frame_plan_graph_pass(plan, &pass1));

    dvz_diagnostic_report_init(&report);
    AT(!_scene_frame_plan_contracts_validate(figure, plan, &report));
    AT(dvz_diagnostic_report_count(&report) == 2);
    for (uint32_t i = 0; i < dvz_diagnostic_report_count(&report); i++)
    {
        message = dvz_diagnostic_report_get(&report, i);
        ANN(message);
        AT(strstr(message, "no matching graph pass") != NULL);
    }

    dvz_frame_plan_destroy(plan);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Verify panel composition binding rejects unmatched and duplicate typed graph/render bindings.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_panel_composition_binding_is_one_to_one(TstContext* suite, const TstCase* item)
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
    AT(point != NULL);
    vec3 positions[1] = {{0.0f, 0.0f, 0.0f}};
    DvzColor colors[1] = {{255, 255, 255, 255}};
    float sizes[1] = {4.0f};
    AT(dvz_visual_set_data(point, "position", positions, 1) == 0);
    AT(dvz_visual_set_data(point, "color", colors, 1) == 0);
    AT(dvz_visual_set_data(point, "size", sizes, 1) == 0);
    AT(dvz_panel_add_visual(panel, point, NULL) == 0);

    DvzDiagnosticReport report = {0};
    DvzFrameGraphResource seeded_resource = {0};
    dvz_strlcpy(seeded_resource.id, "figure_0_p0.seeded.color", sizeof(seeded_resource.id));
    seeded_resource.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
    seeded_resource.format = DVZ_FORMAT_R8G8B8A8_UNORM;
    seeded_resource.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
    seeded_resource.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT;
    seeded_resource.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
    DvzFrameGraphPass seeded = {0};
    dvz_strlcpy(seeded.panel_id, "figure_0_p0", sizeof(seeded.panel_id));
    seeded.kind = DVZ_FRAME_GRAPH_PASS_RENDER;

    DvzFramePlan* plan = dvz_frame_plan("figure_0", 0);
    ANN(plan);
    AT(dvz_frame_plan_graph_resource(plan, &seeded_resource));
    dvz_strlcpy(seeded.id, "figure_0_p0.seeded.gbuffer", sizeof(seeded.id));
    DvzFrameGraphAttachment seeded_attachment = {0};
    dvz_strlcpy(
        seeded_attachment.resource_id, seeded_resource.id, sizeof(seeded_attachment.resource_id));
    seeded_attachment.load_op = DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR;
    seeded_attachment.store_op = DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE;
    seeded_attachment.access = DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE;
    AT(dvz_frame_graph_pass_color_attachment(&seeded, &seeded_attachment));
    AT(dvz_frame_plan_graph_pass(plan, &seeded));
    dvz_diagnostic_report_init(&report);
    AT(!_scene_emit_panel_render_ex(figure, 0, plan, "figure_0", &report));
    AT(_frame_plan_composition_get(plan, "figure_0_p0") == NULL);
    bool found = false;
    for (uint32_t i = 0; i < dvz_diagnostic_report_count(&report); i++)
    {
        const char* message = dvz_diagnostic_report_get(&report, i);
        found = found || (message != NULL && strstr(message, "typed composition identity"));
    }
    AT(found);
    dvz_frame_plan_destroy(plan);

    plan = dvz_frame_plan("figure_0", 0);
    ANN(plan);
    AT(_scene_emit_panel_render_ex(figure, 0, plan, "figure_0", &report));
    const DvzPanelCompositionSnapshot* snapshot = _frame_plan_composition_get(plan, "figure_0_p0");
    ANN(snapshot);
    const DvzFrameGraphPass* original = dvz_frame_plan_graph_pass_get(plan, 0);
    ANN(original);
    seeded = *original;
    dvz_strlcpy(seeded.id, "figure_0_p0.seeded.typed.duplicate", sizeof(seeded.id));
    AT(dvz_frame_plan_graph_pass(plan, &seeded));
    dvz_diagnostic_report_init(&report);
    AT(!_scene_bind_panel_composition(plan, "figure_0_p0", snapshot, &report));
    found = false;
    for (uint32_t i = 0; i < dvz_diagnostic_report_count(&report); i++)
    {
        const char* message = dvz_diagnostic_report_get(&report, i);
        found = found || (message != NULL && strstr(message, "duplicate graph bindings"));
    }
    AT(found);
    dvz_frame_plan_destroy(plan);

    plan = dvz_frame_plan("figure_0", 0);
    ANN(plan);
    AT(dvz_frame_plan_render_panel_role(
        plan, "figure_0_p0", "rt.query", false, panel->desc, DVZ_FRAME_PLAN_RENDER_PASS_PICKING));
    plan->nodes[0].u.render.has_composition_pass = true;
    plan->nodes[0].u.render.composition_pass_id = (DvzFramePlanPassId){1};
    dvz_diagnostic_report_init(&report);
    AT(!_scene_emit_panel_render_ex(figure, 0, plan, "figure_0", &report));
    found = false;
    for (uint32_t i = 0; i < dvz_diagnostic_report_count(&report); i++)
    {
        const char* message = dvz_diagnostic_report_get(&report, i);
        found = found || (message != NULL && strstr(message, "duplicate render bindings"));
    }
    AT(found);

    dvz_frame_plan_destroy(plan);

    DvzPanelCompositionSnapshot omitted = {
        .valid = true,
        .pass_count = 1,
        .passes = {{
            .id = {.value = 41},
            .role = DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND,
            .ordinal = 0,
        }},
    };
    dvz_snprintf(omitted.panel_id, sizeof(omitted.panel_id), "figure_0_p0");
    plan = dvz_frame_plan("figure_0", 0);
    ANN(plan);
    dvz_diagnostic_report_init(&report);
    AT_EXPECTED_ERROR_STRICT(
        suite, !_scene_bind_panel_composition(plan, "figure_0_p0", &omitted, &report));
    bool missing_render = false;
    bool missing_graph = false;
    for (uint32_t i = 0; i < dvz_diagnostic_report_count(&report); i++)
    {
        const char* message = dvz_diagnostic_report_get(&report, i);
        missing_render =
            missing_render || (message != NULL && strstr(message, "no render binding") != NULL);
        missing_graph = missing_graph ||
                        (message != NULL && strstr(message, "no required graph binding") != NULL);
    }
    AT(missing_render);
    AT(missing_graph);
    dvz_frame_plan_destroy(plan);

    DvzPanelCompositionSnapshot presentation_only = {
        .valid = true,
        .technique_count = 1,
        .techniques = {{
            .instance_id = {.value = 1},
            .id = DVZ_SCENE_TECHNIQUE_PRESENTATION,
            .version = 1,
            .phase = DVZ_SCENE_PHASE_PRESENTATION,
        }},
    };
    dvz_snprintf(presentation_only.panel_id, sizeof(presentation_only.panel_id), "figure_0_p0");
    plan = dvz_frame_plan("figure_0", 0);
    ANN(plan);
    dvz_diagnostic_report_init(&report);
    AT(_scene_bind_panel_composition(plan, "figure_0_p0", &presentation_only, &report));
    AT(dvz_diagnostic_report_count(&report) == 0);
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
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
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
int test_scene_panel_graph_failure_reports_specific_diagnostic(
    TstContext* suite, const TstCase* item)
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
        scene, &(DvzSampledFieldDesc){
                   DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
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
           field, &(DvzFieldDataView){
                      DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView), .data = voxels, .bytes_per_row = 2,
                      .rows_per_image = 2}) == DVZ_OK);

    DvzVisual* volume = dvz_volume(scene, 0);
    DvzVisual* slice = dvz_volume(scene, 0);
    AT(volume != NULL);
    AT(slice != NULL);
    AT(dvz_visual_set_field(volume, "field", field) == DVZ_OK);
    AT(dvz_visual_set_field(slice, "field", field) == DVZ_OK);
    AT(dvz_volume_set_render_mode(volume, DVZ_VOLUME_RENDER_MIP) == 0);
    AT(dvz_volume_set_render_mode(slice, DVZ_VOLUME_RENDER_SLICE) == 0);
    AT(dvz_visual_set_volume_occluded(slice, true) == 0);
    AT(dvz_panel_add_visual(panel, volume, NULL) == 0);
    AT(dvz_panel_add_visual(panel, slice, NULL) == 0);
    AT(dvz_panel_set_volume_occluder(
           panel, volume,
           &(DvzVolumeOcclusionDesc){
               DVZ_STRUCT_INIT_FIELDS(DvzVolumeOcclusionDesc),
               .enabled = true,
               .alpha_threshold = 0.01f,
               .fade_distance = 0.04f,
               .occluded_alpha = 0.2f,
           }) == 0);

    DvzFramePlan* plan = dvz_frame_plan("figure_0", 0);
    ANN(plan);
    DvzFrameGraphResource incompatible_rt = {0};
    dvz_strlcpy(incompatible_rt.id, "rt", sizeof(incompatible_rt.id));
    incompatible_rt.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
    incompatible_rt.format = DVZ_FORMAT_R8_UNORM;
    incompatible_rt.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
    incompatible_rt.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT;
    incompatible_rt.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
    AT(dvz_frame_plan_graph_resource(plan, &incompatible_rt));

    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    AT(!_scene_emit_panel_render_ex(figure, 0, plan, "figure_0", &report));
    AT(dvz_diagnostic_report_count(&report) > 0);
    bool found_incompatible_resource = false;
    for (uint32_t i = 0; i < dvz_diagnostic_report_count(&report); i++)
    {
        const char* message = dvz_diagnostic_report_get(&report, i);
        found_incompatible_resource =
            found_incompatible_resource ||
            (message != NULL && strstr(message, "product") != NULL &&
             strstr(message, "resource rt is incompatible") != NULL);
    }
    AT(found_incompatible_resource);
    AT(dvz_frame_plan_graph_resource_count(plan) == 1);
    AT(dvz_frame_plan_graph_pass_count(plan) == 0);
    AT(_frame_plan_composition_get(plan, "figure_0_p0") == NULL);

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
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
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
        scene, &(DvzSceneBufferDesc){
                   DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)) == DVZ_OK);

    AT(dvz_visual_set_data(mesh, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(mesh, "normal", normals, 4) == 0);
    AT(dvz_visual_set_buffer(mesh, "index", index_buffer) == DVZ_OK);
    AT(dvz_panel_add_visual(panel, mesh, NULL) == 0);

    AT(!_scene_technique_gbuffer_enabled(scene, panel));
    DvzFramePlan* default_plan = dvz_frame_plan("figure.gbuffer.default", 0);
    ANN(default_plan);
    _scene_emit_panel_render(figure, 0, default_plan, "figure_0");
    AT(dvz_frame_plan_node_count(default_plan) == 1);
    const DvzFramePlanNode* default_node = dvz_frame_plan_node_get(default_plan, 0);
    ANN(default_node);
    AT(_frame_plan_render_pass_role(default_node) == DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE);
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
    AT(_frame_plan_render_pass_role(gbuffer_node) == DVZ_FRAME_PLAN_RENDER_PASS_GBUFFER);
    AT(_frame_plan_render_pass_role(opaque_node) == DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE);
    AT(gbuffer_node->u.render.has_composition_pass);
    AT(gbuffer_node->u.render.has_graph_pass_index);
    AT(opaque_node->u.render.has_composition_pass);
    AT(opaque_node->u.render.has_graph_pass_index);
    AT(gbuffer_node->u.render.visual_count == 1);
    AT(opaque_node->u.render.visual_count == 1);
    AT(dvz_frame_plan_graph_resource_count(plan) == 6);
    AT(dvz_frame_plan_graph_pass_count(plan) == 2);
    const DvzPanelCompositionSnapshot* composition =
        _frame_plan_composition_get(plan, "figure_0_p0");
    ANN(composition);
    AT(composition->valid);
    AT(composition->technique_count > 0);
    const DvzFrameGraphPass* gbuffer_pass = dvz_frame_plan_graph_pass_get(plan, 0);
    ANN(gbuffer_pass);
    AT(gbuffer_pass->has_composition_pass);
    AT(gbuffer_pass->composition_pass_id.value ==
       gbuffer_node->u.render.composition_pass_id.value);
    AT(gbuffer_node->u.render.graph_pass_index == 0);
    const DvzFrameGraphPass* opaque_pass =
        dvz_frame_plan_graph_pass_get(plan, opaque_node->u.render.graph_pass_index);
    ANN(opaque_pass);
    AT(opaque_pass->has_composition_pass);
    AT(opaque_pass->composition_pass_id.value == opaque_node->u.render.composition_pass_id.value);
    AT(_scene_test_graph_pass_provider(plan, gbuffer_pass) ==
       DVZ_SCENE_WORK_PROVIDER_SURFACE_CAPTURE);
    AT(gbuffer_pass->color_attachment_count == 3);
    AT(gbuffer_pass->has_depth_attachment);
    AT(strcmp(gbuffer_pass->color_attachments[0].resource_id, "figure_0_p0.gbuffer.depth") == 0);
    AT(strcmp(gbuffer_pass->color_attachments[1].resource_id, "figure_0_p0.gbuffer.normal") == 0);
    AT(strcmp(gbuffer_pass->color_attachments[2].resource_id, "figure_0_p0.gbuffer.coverage") ==
       0);
    AT(strcmp(gbuffer_pass->depth_attachment.resource_id, "figure_0_p0.scene_occlusion.z") == 0);
    char* json = dvz_frame_plan_json(plan);
    ANN(json);
    AT(strstr(json, "\"composition_pass_id\"") != NULL);
    AT(strstr(json, "\"graph_pass_index\"") != NULL);
    AT(strstr(json, "\"compositions\"") != NULL);
    AT(strstr(json, "\"work_fingerprint\"") != NULL);
    dvz_frame_plan_json_destroy(json);

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.target_width = 64;
    cfg.target_height = 64;
    caps = dvz_capability_snapshot();
    caps.max_color_attachments = 3;
    caps.render_target_format_rgba16float = true;
    caps.supports_render_target_sampling = true;
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    if (stream == NULL)
        for (uint32_t i = 0; i < dvz_diagnostic_report_count(&report); i++)
            log_error("%s", dvz_diagnostic_report_get(&report, i));
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);
    DvzDrp2ValidationResult validation = dvz_drp2_validate_stream(stream);
    AT(validation.ok);

    bool found_normal_texture = false;
    bool found_depth_texture = false;
    bool found_coverage_texture = false;
    bool found_device_depth_texture = false;
    bool found_gbuffer_pass = false;
    bool found_gbuffer_pipeline = false;
    uint64_t normal_id = 0;
    uint64_t depth_id = 0;
    uint64_t coverage_id = 0;
    uint64_t device_depth_id = 0;
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
                    cmd->u.create_texture.format == DVZ_FORMAT_R16G16B16A16_SFLOAT &&
                    (cmd->u.create_texture.usage & DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT) != 0;
            }
            if (label != NULL && strcmp(label, "fig0_p0.gbuffer.depth") == 0)
            {
                depth_id = cmd->u.create_texture.id;
                found_depth_texture =
                    cmd->u.create_texture.format == DVZ_FORMAT_R32_SFLOAT &&
                    (cmd->u.create_texture.usage & DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT) != 0;
            }
            if (label != NULL && strcmp(label, "fig0_p0.gbuffer.coverage") == 0)
            {
                coverage_id = cmd->u.create_texture.id;
                found_coverage_texture =
                    cmd->u.create_texture.format == DVZ_FORMAT_R8_UNORM &&
                    (cmd->u.create_texture.usage & DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT) != 0;
            }
            if (label != NULL && strcmp(label, "fig0_p0.scene_occlusion.z") == 0)
            {
                device_depth_id = cmd->u.create_texture.id;
                found_device_depth_texture =
                    cmd->u.create_texture.format == DVZ_FORMAT_D32_SFLOAT &&
                    (cmd->u.create_texture.usage & DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT) != 0;
            }
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS)
        {
            found_gbuffer_pass =
                found_gbuffer_pass ||
                (normal_id != 0 && depth_id != 0 && coverage_id != 0 && device_depth_id != 0 &&
                 cmd->u.begin_render_pass.color_attachment_count == 3 &&
                 cmd->u.begin_render_pass.color_attachments[0].texture_id == depth_id &&
                 cmd->u.begin_render_pass.color_attachments[1].texture_id == normal_id &&
                 cmd->u.begin_render_pass.color_attachments[2].texture_id == coverage_id &&
                 cmd->u.begin_render_pass.depth_texture_id == device_depth_id);
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.create_render_pipeline.id);
            found_gbuffer_pipeline =
                found_gbuffer_pipeline ||
                (label != NULL && strstr(label, "_pipe_gbuffer") != NULL &&
                 cmd->u.create_render_pipeline.color_target_count == 3 &&
                 cmd->u.create_render_pipeline.color_targets[0].format == DVZ_FORMAT_R32_SFLOAT &&
                 cmd->u.create_render_pipeline.color_targets[1].format ==
                     DVZ_FORMAT_R16G16B16A16_SFLOAT &&
                 cmd->u.create_render_pipeline.color_targets[2].format == DVZ_FORMAT_R8_UNORM &&
                 cmd->u.create_render_pipeline.has_depth_attachment &&
                 cmd->u.create_render_pipeline.depth_write_enabled);
        }
    }
    AT(found_normal_texture);
    AT(found_depth_texture);
    AT(found_coverage_texture);
    AT(found_device_depth_texture);
    AT(found_gbuffer_pass);
    AT(found_gbuffer_pipeline);

    DvzAlphaMode retained_alpha = mesh->alpha_mode;
    mesh->alpha_mode = DVZ_ALPHA_WBOIT;
    dvz_diagnostic_report_init(&report);
    AT(_scene_frame_plan_contracts_validate(figure, plan, &report));
    AT(dvz_diagnostic_report_count(&report) == 0);
    mesh->alpha_mode = retained_alpha;

    _test_scene_stream_destroy(stream);
    dvz_frame_plan_destroy(plan);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Verify a live single-sample surface capture materializes one coherent typed product record.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_surface_products_single_sample_contract(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel =
        _test_r4_surface_panel(scene, figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f}, false);
    ANN(panel);
    DvzFramePlan* plan = dvz_frame_plan("figure.surface.products.1x", 0);
    ANN(plan);
    AT(_scene_emit_panel_render(figure, 0, plan, "figure_0"));

    AT(dvz_frame_plan_product_count(plan) >= 3);
    const DvzRenderProductContract* depth = _test_surface_product(
        plan, DVZ_RENDER_PRODUCT_SURFACE_DEPTH, DVZ_RENDER_PRODUCT_SAMPLES_SINGLE);
    const DvzRenderProductContract* normal = _test_surface_product(
        plan, DVZ_RENDER_PRODUCT_SURFACE_NORMAL, DVZ_RENDER_PRODUCT_SAMPLES_SINGLE);
    const DvzRenderProductContract* coverage = _test_surface_product(
        plan, DVZ_RENDER_PRODUCT_SURFACE_COVERAGE, DVZ_RENDER_PRODUCT_SAMPLES_SINGLE);
    ANN(depth);
    ANN(normal);
    ANN(coverage);
    AT(depth->concrete_format == DVZ_FORMAT_R32_SFLOAT);
    AT(normal->concrete_format == DVZ_FORMAT_R16G16B16A16_SFLOAT);
    AT(coverage->concrete_format == DVZ_FORMAT_R8_UNORM);
    AT(depth->sample_count == 1 && normal->sample_count == 1 && coverage->sample_count == 1);
    AT(depth->surface_record_id.value != 0);
    AT(depth->surface_record_id.value == normal->surface_record_id.value);
    AT(depth->surface_record_id.value == coverage->surface_record_id.value);
    AT(depth->source_product_id.value == 0);
    AT(normal->source_product_id.value == 0);
    AT(coverage->source_product_id.value == 0);
    AT(depth->validity == DVZ_RENDER_PRODUCT_VALIDITY_EXPLICIT_COVERAGE);
    AT(normal->validity == DVZ_RENDER_PRODUCT_VALIDITY_EXPLICIT_COVERAGE);
    AT(depth->validity_product_id.value == coverage->id.value);
    AT(normal->validity_product_id.value == coverage->id.value);
    AT(coverage->validity == DVZ_RENDER_PRODUCT_VALIDITY_BACKGROUND_VALUE);
    AT(coverage->coverage == DVZ_RENDER_PRODUCT_COVERAGE_BINARY);
    AT(coverage->has_background_value);
    AT(plan->product_use_count == 0);
    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    AT(dvz_frame_plan_products_validate(plan, &report));
    AT(dvz_diagnostic_report_count(&report) == 0);

    dvz_frame_plan_destroy(plan);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Verify MSAA surface capture and explicit resolve materialize coherent successor records for
 * GTAO.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_surface_products_msaa_resolve_contract(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel =
        _test_r4_surface_panel(scene, figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f}, true);
    ANN(panel);
    AT(dvz_panel_set_msaa(
           panel, &(DvzMsaaDesc){
                      DVZ_STRUCT_INIT_FIELDS(DvzMsaaDesc), .enabled = true, .sample_count = 4,
                      .alpha_to_coverage = true}) == DVZ_OK);
    DvzFramePlan* plan = dvz_frame_plan("figure.surface.products.4x", 0);
    ANN(plan);
    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    AT(_scene_emit_panel_render_ex(figure, 0, plan, "figure_0", &report));

    AT(dvz_frame_plan_product_count(plan) >= 6);
    const DvzRenderProductKind kinds[3] = {
        DVZ_RENDER_PRODUCT_SURFACE_DEPTH,
        DVZ_RENDER_PRODUCT_SURFACE_NORMAL,
        DVZ_RENDER_PRODUCT_SURFACE_COVERAGE,
    };
    const DvzRenderProductResolvePolicy policies[3] = {
        DVZ_RENDER_PRODUCT_RESOLVE_NEAREST_VALID_DEPTH,
        DVZ_RENDER_PRODUCT_RESOLVE_WINNING_NORMAL,
        DVZ_RENDER_PRODUCT_RESOLVE_COVERAGE_FRACTION,
    };
    const uint32_t formats[3] = {
        DVZ_FORMAT_R32_SFLOAT,
        DVZ_FORMAT_R16G16B16A16_SFLOAT,
        DVZ_FORMAT_R8_UNORM,
    };
    const DvzRenderProductContract* source[3] = {0};
    const DvzRenderProductContract* resolved[3] = {0};
    for (uint32_t i = 0; i < 3; i++)
    {
        source[i] = _test_surface_product(plan, kinds[i], DVZ_RENDER_PRODUCT_SAMPLES_MULTISAMPLE);
        resolved[i] = _test_surface_product(plan, kinds[i], DVZ_RENDER_PRODUCT_SAMPLES_RESOLVED);
        ANN(source[i]);
        ANN(resolved[i]);
        AT(source[i]->sample_count == 4);
        AT(source[i]->concrete_format == formats[i]);
        AT(source[i]->source_product_id.value == 0);
        AT(source[i]->resolve_policy == DVZ_RENDER_PRODUCT_RESOLVE_NONE);
        AT(resolved[i]->sample_count == 1);
        AT(resolved[i]->concrete_format == formats[i]);
        AT(resolved[i]->source_product_id.value == source[i]->id.value);
        AT(resolved[i]->resolve_policy == policies[i]);
    }
    AT(source[0]->surface_record_id.value != 0);
    AT(source[0]->surface_record_id.value == source[1]->surface_record_id.value);
    AT(source[0]->surface_record_id.value == source[2]->surface_record_id.value);
    AT(resolved[0]->surface_record_id.value != source[0]->surface_record_id.value);
    AT(resolved[0]->surface_record_id.value == resolved[1]->surface_record_id.value);
    AT(resolved[0]->surface_record_id.value == resolved[2]->surface_record_id.value);
    AT(source[0]->validity_product_id.value == source[2]->id.value);
    AT(source[1]->validity_product_id.value == source[2]->id.value);
    AT(resolved[0]->validity_product_id.value == resolved[2]->id.value);
    AT(resolved[1]->validity_product_id.value == resolved[2]->id.value);
    AT(source[2]->coverage == DVZ_RENDER_PRODUCT_COVERAGE_BINARY);
    AT(resolved[2]->coverage == DVZ_RENDER_PRODUCT_COVERAGE_SAMPLE_FRACTION);

    uint32_t capture_index = UINT32_MAX;
    uint32_t resolve_index = UINT32_MAX;
    uint32_t gtao_index = UINT32_MAX;
    for (uint32_t i = 0; i < dvz_frame_plan_graph_pass_count(plan); i++)
    {
        const DvzFrameGraphPass* pass = dvz_frame_plan_graph_pass_get(plan, i);
        ANN(pass);
        const DvzSceneWorkProviderKey provider = _scene_test_graph_pass_provider(plan, pass);
        if (provider == DVZ_SCENE_WORK_PROVIDER_SURFACE_CAPTURE)
            capture_index = i;
        else if (provider == DVZ_SCENE_WORK_PROVIDER_SURFACE_RESOLVE)
            resolve_index = i;
        else if (provider == DVZ_SCENE_WORK_PROVIDER_GTAO)
            gtao_index = i;
    }
    AT(capture_index != UINT32_MAX);
    AT(resolve_index != UINT32_MAX);
    AT(gtao_index != UINT32_MAX);
    AT(capture_index < resolve_index && resolve_index < gtao_index);
    const DvzFrameGraphPass* capture = dvz_frame_plan_graph_pass_get(plan, capture_index);
    const DvzFrameGraphPass* resolve = dvz_frame_plan_graph_pass_get(plan, resolve_index);
    ANN(capture);
    ANN(resolve);
    AT(capture->color_attachment_count == 3);
    AT(capture->has_depth_attachment);
    for (uint32_t i = 0; i < 3; i++)
    {
        const DvzFrameGraphResource* source_resource =
            dvz_frame_plan_graph_resource_get(plan, source[i]->resource_index);
        const DvzFrameGraphResource* resolved_resource =
            dvz_frame_plan_graph_resource_get(plan, resolved[i]->resource_index);
        ANN(source_resource);
        ANN(resolved_resource);
        AT(strcmp(capture->color_attachments[i].resource_id, source_resource->id) == 0);
        AT(source_resource->format == formats[i] && source_resource->sample_count == 4);
        AT(resolved_resource->format == formats[i] && resolved_resource->sample_count == 1);
    }
    const DvzFrameGraphResource* capture_depth =
        _test_graph_resource(plan, capture->depth_attachment.resource_id);
    ANN(capture_depth);
    AT(capture_depth->format == DVZ_FORMAT_D32_SFLOAT);
    AT(capture_depth->sample_count == 4);
    AT(resolve->read_count == 3);
    AT(resolve->color_attachment_count == 3);
    AT(!resolve->has_depth_attachment);
    for (uint32_t i = 0; i < 3; i++)
    {
        const DvzFrameGraphResource* resolved_resource =
            dvz_frame_plan_graph_resource_get(plan, resolved[i]->resource_index);
        ANN(resolved_resource);
        AT(strcmp(resolve->color_attachments[i].resource_id, resolved_resource->id) == 0);
    }
    AT(plan->product_use_count >= 6);
    for (uint32_t i = 0; i < plan->product_use_count; i++)
    {
        const DvzRenderProductConsumer* use = &plan->product_uses[i];
        bool found_product = false;
        for (uint32_t j = 0; j < dvz_frame_plan_product_count(plan); j++)
        {
            const DvzRenderProductContract* product = dvz_frame_plan_product_get(plan, j);
            found_product =
                found_product || (product != NULL && product->id.value == use->product_id.value);
        }
        AT(found_product);
        AT(use->pass_index < dvz_frame_plan_graph_pass_count(plan));
        AT(use->validity_requirement != DVZ_RENDER_PRODUCT_VALIDITY_REQUIREMENT_NONE);
    }
    dvz_diagnostic_report_init(&report);
    AT(dvz_frame_plan_products_validate(plan, &report));
    AT(dvz_diagnostic_report_count(&report) == 0);

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
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
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
        scene, &(DvzSceneBufferDesc){
                   DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)) == DVZ_OK);

    for (uint32_t i = 0; i < 3; i++)
    {
        DvzVisual* mesh = dvz_mesh(scene, 0);
        AT(mesh != NULL);
        AT(dvz_visual_set_data(mesh, "position", positions, 4) == 0);
        AT(dvz_visual_set_data(mesh, "normal", normals, 4) == 0);
        AT(dvz_visual_set_buffer(mesh, "index", index_buffer) == DVZ_OK);
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
    AT(dvz_frame_plan_node_count(plan) == DVZ_FRAME_PLAN_INITIAL_NODE_CAPACITY + 5);

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
    AT(_frame_plan_render_pass_role(gbuffer_node) == DVZ_FRAME_PLAN_RENDER_PASS_GBUFFER);
    AT(_frame_plan_render_pass_role(opaque_node) == DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE);
    AT(_frame_plan_render_pass_role(blended_node) == DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND);
    AT(_frame_plan_render_pass_role(wboit_node) ==
       DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION);
    AT(_frame_plan_render_pass_role(resolve_node) == DVZ_FRAME_PLAN_RENDER_PASS_WBOIT_RESOLVE);
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
 * Verify scene emission preserves parallel render metadata across visual storage growth.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_frame_plan_visual_reallocation_safe(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);

    vec3 positions[1] = {{0.0f, 0.0f, 0.0f}};
    DvzColor colors[1] = {{255, 255, 255, 255}};
    float sizes[1] = {10.0f};
    const uint32_t visual_count = DVZ_FRAME_PLAN_INITIAL_VISUAL_CAPACITY + 1;
    for (uint32_t i = 0; i < visual_count; i++)
    {
        DvzVisual* point = dvz_point(scene, 0);
        ANN(point);
        AT(dvz_visual_set_data(point, "position", positions, 1) == 0);
        AT(dvz_visual_set_data(point, "color", colors, 1) == 0);
        AT(dvz_visual_set_data(point, "size", sizes, 1) == 0);
        DvzVisualAttachDesc attach = {
            DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc),
            .controller_mode = i % 2 == 0 ? DVZ_CONTROLLER_APPLY : DVZ_CONTROLLER_FIXED,
        };
        AT(dvz_panel_add_visual(panel, point, &attach) == 0);
    }

    DvzFramePlan* plan = dvz_frame_plan("figure.visual.realloc", 0);
    ANN(plan);
    AT(_scene_emit_panel_render(figure, 0, plan, "figure_0"));

    const DvzFramePlanNode* opaque = NULL;
    for (uint32_t i = 0; i < dvz_frame_plan_node_count(plan); i++)
    {
        const DvzFramePlanNode* node = dvz_frame_plan_node_get(plan, i);
        if (node != NULL && node->type == DVZ_FRAME_PLAN_NODE_RENDER &&
            node->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE)
        {
            opaque = node;
            break;
        }
    }
    ANN(opaque);
    AT(opaque->u.render.visual_count == visual_count);
    AT(opaque->u.render.visual_capacity >= visual_count);
    for (uint32_t i = 0; i < visual_count; i++)
    {
        AT(opaque->u.render.visuals[i][0] != '\0');
        AT(opaque->u.render.visual_metadata[i].has_metadata);
        AT(opaque->u.render.visual_metadata[i].visual_index == i);
        const DvzPanelAttach* attach = &panel->visuals[i];
        AT(opaque->u.render.controller_modes[i] == attach->controller_mode);
        bool expected_mvp = attach->visual->has_local_transform ||
                            attach->coord_space == DVZ_VISUAL_COORD_DATA ||
                            attach->coord_space == DVZ_VISUAL_COORD_PANEL ||
                            attach->coord_space == DVZ_VISUAL_COORD_PANEL_PIXEL ||
                            attach->controller_mode == DVZ_CONTROLLER_APPLY_VIEW_PROJ;
        AT(opaque->u.render.visual_has_mvp[i] == expected_mvp);
    }

    DvzDiagnosticReport report = {0};
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
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    AT(dvz_panel_set_msaa(
           panel, &(DvzMsaaDesc){
                      DVZ_STRUCT_INIT_FIELDS(DvzMsaaDesc), .enabled = true, .sample_count = 4,
                      .alpha_to_coverage = true}) == DVZ_OK);

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
    AT(dvz_frame_plan_node_count(plan) == 2);
    AT(dvz_frame_plan_graph_pass_count(plan) == 2);

    const DvzFrameGraphResource* msaa_color = NULL;
    const DvzFrameGraphResource* scene_color = NULL;
    const DvzFrameGraphResource* depth = NULL;
    for (uint32_t i = 0; i < dvz_frame_plan_graph_resource_count(plan); i++)
    {
        const DvzFrameGraphResource* resource = dvz_frame_plan_graph_resource_get(plan, i);
        ANN(resource);
        if (strcmp(resource->id, "figure_0_p0.msaa.color") == 0)
            msaa_color = resource;
        else if (strcmp(resource->id, "figure_0_p0.scene.color.v1") == 0)
            scene_color = resource;
        else if (strcmp(resource->id, "figure_0_p0.depth") == 0)
            depth = resource;
    }
    ANN(msaa_color);
    ANN(scene_color);
    ANN(depth);
    AT(msaa_color->format == 0);
    AT(msaa_color->sample_count == 4);
    AT(msaa_color->usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_COPY_SRC);
    AT(scene_color->extent_kind == DVZ_FRAME_GRAPH_EXTENT_PANEL);
    AT(scene_color->width == 64);
    AT(scene_color->height == 64);
    AT(depth->sample_count == 4);

    const DvzFrameGraphPass* pass = dvz_frame_plan_graph_pass_get(plan, 0);
    ANN(pass);
    AT(_scene_test_graph_pass_provider(plan, pass) == DVZ_SCENE_WORK_PROVIDER_OPAQUE);
    AT(pass->color_attachment_count == 1);
    AT(strcmp(pass->color_attachments[0].resource_id, "figure_0_p0.msaa.color") == 0);
    AT(pass->color_attachments[0].load_op == DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR);
    AT(strcmp(pass->color_attachments[0].resolve_resource_id, "figure_0_p0.scene.color.v1") == 0);
    AT(pass->color_attachments[0].resolve_mode == VK_RESOLVE_MODE_AVERAGE_BIT);
    AT(pass->has_depth_attachment);
    AT(strcmp(pass->depth_attachment.resource_id, "figure_0_p0.depth") == 0);

    const DvzFrameGraphPass* presentation_pass = dvz_frame_plan_graph_pass_get(plan, 1);
    ANN(presentation_pass);
    AT(_scene_test_graph_pass_provider(plan, presentation_pass) ==
       DVZ_SCENE_WORK_PROVIDER_PRESENTATION);
    AT(presentation_pass->color_attachment_count == 1);
    AT(strcmp(presentation_pass->color_attachments[0].resource_id, "rt") == 0);

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.color_target_format = DVZ_FORMAT_B8G8R8A8_UNORM;
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
                found_msaa_texture =
                    cmd->u.create_texture.sample_count == 4 &&
                    cmd->u.create_texture.format == DVZ_FORMAT_B8G8R8A8_UNORM &&
                    (cmd->u.create_texture.usage & DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT) !=
                        0 &&
                    (cmd->u.create_texture.usage & DVZ_DRP2_TEXTURE_USAGE_COPY_SRC) != 0;
            }
            if (label != NULL && strcmp(label, "fig0_p0.depth") == 0)
            {
                found_depth_texture = cmd->u.create_texture.sample_count == 4 &&
                                      cmd->u.create_texture.format == DVZ_FORMAT_D32_SFLOAT;
            }
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE)
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.create_shader_module.id);
            found_sphere_a2c_shader = found_sphere_a2c_shader ||
                                      (label != NULL && strcmp(label, "_fs_sphereg_a2c") == 0);
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS)
        {
            found_resolve_pass =
                found_resolve_pass ||
                (cmd->u.begin_render_pass.texture_id == msaa_texture_id &&
                 cmd->u.begin_render_pass.color_attachments[0].load_op ==
                     DVZ_DRP2_ATTACHMENT_LOAD_CLEAR &&
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
                found_msaa_pipeline || (label != NULL && strstr(label, "_pipe_sphere") != NULL &&
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

    cfg.external_color_target = true;
    cfg.color_target_id = 4242;
    cfg.color_target_format = DVZ_FORMAT_R16G16B16A16_SFLOAT;
    caps.max_color_sample_count = 4;
    caps.max_depth_sample_count = 4;
    AT(_scene_runtime_emitter_reset(scene));
    dvz_diagnostic_report_init(&report);
    stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    ANN(stream);
    AT(dvz_diagnostic_report_error_count(&report) == 0);
    bool found_external_presentation = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        ANN(cmd);
        if (cmd->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS &&
            cmd->u.begin_render_pass.color_attachment_count > 0)
        {
            const DvzDrp2ColorAttachment* attachment =
                &cmd->u.begin_render_pass.color_attachments[0];
            found_external_presentation =
                found_external_presentation || (cmd->u.begin_render_pass.texture_id == 4242 &&
                                                attachment->resolve_texture_id == 0 &&
                                                attachment->resolve_mode == VK_RESOLVE_MODE_NONE);
        }
    }
    AT(found_external_presentation);
    char* json = dvz_drp2_stream_json(stream, "external_msaa_resolve");
    ANN(json);
    AT(strstr(json, "\"texture_id\": 4242") != NULL);
    dvz_drp2_stream_json_destroy(json);
    _test_scene_stream_destroy(stream);

    dvz_frame_plan_destroy(plan);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Verify an MSAA graph pass resolves only within its panel when mixed with plain panels.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_msaa_mixed_plain_panel_resolve_region(TstContext* suite, const TstCase* item)
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
    AT(dvz_panel_set_msaa(
           right, &(DvzMsaaDesc){
                      DVZ_STRUCT_INIT_FIELDS(DvzMsaaDesc), .enabled = true,
                      .sample_count = 4}) == DVZ_OK);

    DvzVisual* left_sphere = dvz_sphere(scene, 0);
    DvzVisual* right_sphere = dvz_sphere(scene, 0);
    AT(left_sphere != NULL);
    AT(right_sphere != NULL);
    vec3 position[1] = {{0.0f, 0.0f, 0.0f}};
    DvzColor color[1] = {{255, 128, 64, 255}};
    float size[1] = {0.35f};
    AT(dvz_visual_set_data(left_sphere, "position", position, 1) == 0);
    AT(dvz_visual_set_data(left_sphere, "color", color, 1) == 0);
    AT(dvz_visual_set_data(left_sphere, "size", size, 1) == 0);
    AT(dvz_visual_set_data(right_sphere, "position", position, 1) == 0);
    AT(dvz_visual_set_data(right_sphere, "color", color, 1) == 0);
    AT(dvz_visual_set_data(right_sphere, "size", size, 1) == 0);
    AT(dvz_panel_add_visual(left, left_sphere, NULL) == 0);
    AT(dvz_panel_add_visual(right, right_sphere, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.target_width = 128;
    cfg.target_height = 64;
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);
    DvzDrp2ValidationResult validation = dvz_drp2_validate_stream(stream);
    AT(validation.ok);

    uint64_t msaa_texture_id = 0;
    bool found_msaa_texture = false;
    bool found_left_plain_pass = false;
    bool found_right_msaa_pass = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        ANN(cmd);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE)
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.create_texture.id);
            if (label != NULL && strcmp(label, "fig0_p1.msaa.color") == 0)
            {
                msaa_texture_id = cmd->u.create_texture.id;
                found_msaa_texture =
                    (cmd->u.create_texture.usage & DVZ_DRP2_TEXTURE_USAGE_COPY_SRC) != 0;
            }
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS)
        {
            bool left_plain_rect = false;
            bool right_msaa_rect = false;
            if (cmd->u.begin_render_pass.has_explicit_rects)
            {
                left_plain_rect = cmd->u.begin_render_pass.viewport_px[0] == 0.0f &&
                                  cmd->u.begin_render_pass.viewport_px[1] == 0.0f &&
                                  cmd->u.begin_render_pass.viewport_px[2] == 64.0f &&
                                  cmd->u.begin_render_pass.viewport_px[3] == 64.0f;
                right_msaa_rect = cmd->u.begin_render_pass.viewport_px[0] == 0.0f &&
                                  cmd->u.begin_render_pass.viewport_px[1] == 0.0f &&
                                  cmd->u.begin_render_pass.viewport_px[2] == 64.0f &&
                                  cmd->u.begin_render_pass.viewport_px[3] == 64.0f;
            }
            else
            {
                left_plain_rect = cmd->u.begin_render_pass.viewport[0] == 0.0f &&
                                  cmd->u.begin_render_pass.viewport[1] == 0.0f &&
                                  cmd->u.begin_render_pass.viewport[2] == 1.0f &&
                                  cmd->u.begin_render_pass.viewport[3] == 1.0f;
                right_msaa_rect = cmd->u.begin_render_pass.viewport[0] == 0.0f &&
                                  cmd->u.begin_render_pass.viewport[1] == 0.0f &&
                                  cmd->u.begin_render_pass.viewport[2] == 1.0f &&
                                  cmd->u.begin_render_pass.viewport[3] == 1.0f;
            }
            found_left_plain_pass =
                found_left_plain_pass ||
                (cmd->u.begin_render_pass.texture_id != 0 &&
                 cmd->u.begin_render_pass.texture_id != msaa_texture_id && left_plain_rect);
            found_right_msaa_pass =
                found_right_msaa_pass ||
                (msaa_texture_id != 0 && cmd->u.begin_render_pass.texture_id == msaa_texture_id &&
                 cmd->u.begin_render_pass.color_attachments[0].resolve_texture_id != 0 &&
                 right_msaa_rect);
        }
    }
    AT(found_msaa_texture);
    AT(found_left_plain_pass);
    AT(found_right_msaa_pass);

    _test_scene_stream_destroy(stream);
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
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    AT(dvz_panel_set_msaa(
           panel, &(DvzMsaaDesc){
                      DVZ_STRUCT_INIT_FIELDS(DvzMsaaDesc), .enabled = true, .sample_count = 4,
                      .alpha_to_coverage = true}) == DVZ_OK);

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
    AT(dvz_frame_plan_node_count(plan) == 3);
    AT(dvz_frame_plan_graph_pass_count(plan) == 3);

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
    AT(_scene_test_graph_pass_provider(plan, opaque_pass) == DVZ_SCENE_WORK_PROVIDER_OPAQUE);
    AT(strcmp(opaque_pass->color_attachments[0].resource_id, "figure_0_p0.msaa.color") == 0);
    AT(opaque_pass->color_attachments[0].resolve_resource_id[0] != '\0');
    AT(opaque_pass->color_attachments[0].resolve_mode == VK_RESOLVE_MODE_AVERAGE_BIT);
    AT(_scene_test_graph_pass_provider(plan, blended_pass) ==
       DVZ_SCENE_WORK_PROVIDER_TRANSPARENT_BLEND);
    AT(strcmp(
           blended_pass->color_attachments[0].resource_id,
           opaque_pass->color_attachments[0].resolve_resource_id) == 0);
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
            found_msaa_sphere_pipeline = found_msaa_sphere_pipeline ||
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
 * Verify the protein-style MSAA + AO + blended overlay graph integrates AO before overlays.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_msaa_gtao_blended_overlay_runtime_lowering(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    AT(dvz_panel_set_msaa(
           panel, &(DvzMsaaDesc){
                      DVZ_STRUCT_INIT_FIELDS(DvzMsaaDesc), .enabled = true, .sample_count = 16,
                      .alpha_to_coverage = true}) == DVZ_OK);
    AT(_scene_technique_state_set_ao(
        &panel->techniques,
        &(DvzSceneAoDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzSceneAoDesc), .radius = 1.0f, .intensity = 2.5f,
            .thickness = 0.1f, .quality = DVZ_AO_QUALITY_MEDIUM}));

    DvzVisual* sphere = dvz_sphere(scene, DVZ_SPHERE_FLAGS_LIGHTING);
    AT(sphere != NULL);
    AT(dvz_sphere_set_mode(sphere, DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR) == 0);
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
    AT(dvz_visual_set_data(overlay, "stroke_width_px", overlay_widths, 2) == 0);
    AT(dvz_segment_set_caps(overlay, DVZ_SEGMENT_CAP_ROUND, DVZ_SEGMENT_CAP_ROUND) == 0);
    AT(dvz_visual_set_depth_test(overlay, false) == 0);
    AT(dvz_visual_set_alpha_mode(overlay, DVZ_ALPHA_BLENDED) == 0);
    AT(dvz_panel_add_visual(panel, overlay, NULL) == 0);

    DvzFramePlan* plan = dvz_frame_plan("figure.msaa.gtao.blended", 0);
    ANN(plan);
    AT(_scene_emit_panel_render(figure, 0, plan, "figure_0"));
    AT(dvz_frame_plan_graph_validate(plan, NULL));

    bool saw_gtao_visibility_presentation = false;
    bool saw_gtao = false;
    bool saw_opaque_after_gtao = false;
    bool saw_blended_after_opaque = false;
    for (uint32_t i = 0; i < dvz_frame_plan_graph_pass_count(plan); i++)
    {
        const DvzFrameGraphPass* pass = dvz_frame_plan_graph_pass_get(plan, i);
        ANN(pass);
        const DvzSceneWorkProviderKey provider = _scene_test_graph_pass_provider(plan, pass);
        if (provider == DVZ_SCENE_WORK_PROVIDER_GTAO_VISIBILITY_PRESENTATION)
            saw_gtao_visibility_presentation = true;
        if (provider == DVZ_SCENE_WORK_PROVIDER_GTAO)
            saw_gtao = true;
        if (saw_gtao && provider == DVZ_SCENE_WORK_PROVIDER_OPAQUE)
            saw_opaque_after_gtao = true;
        if (saw_opaque_after_gtao && provider == DVZ_SCENE_WORK_PROVIDER_TRANSPARENT_BLEND)
            saw_blended_after_opaque = true;
    }
    AT(!saw_gtao_visibility_presentation);
    AT(saw_gtao);
    AT(saw_opaque_after_gtao);
    AT(saw_blended_after_opaque);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.supports_color_blending = true;
    caps.max_color_attachments = 3;
    caps.supports_render_target_sampling = true;
    caps.render_target_format_rgba16float = true;
    caps.render_target_format_r16float = true;
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
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    AT(dvz_panel_set_msaa(
           panel, &(DvzMsaaDesc){
                      DVZ_STRUCT_INIT_FIELDS(DvzMsaaDesc), .enabled = true, .sample_count = 16,
                      .alpha_to_coverage = true}) == DVZ_OK);

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
    AT(_scene_pass_contract_from_render_ex(
        plan, panel, render_node, graph_pass, &caps, &contract));
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
    AT(dvz_diagnostic_report_count(&report) >= 1);
    AT(dvz_diagnostic_report_error_count(&report) == 0);
    AT(dvz_diagnostic_report_get_severity(&report, 0) == DVZ_DIAGNOSTIC_SEVERITY_WARNING);
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
                found_msaa_texture || (label != NULL && strcmp(label, "fig0_p0.msaa.color") == 0 &&
                                       cmd->u.create_texture.sample_count == 8);
            found_depth_texture =
                found_depth_texture || (label != NULL && strcmp(label, "fig0_p0.depth") == 0 &&
                                        cmd->u.create_texture.sample_count == 8);
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.create_render_pipeline.id);
            found_msaa_pipeline =
                found_msaa_pipeline || (label != NULL && strstr(label, "_pipe_sphere") != NULL &&
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
 * Verify capability lowering to one sample emits the plain single-sample topology.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_msaa_runtime_capability_disable_topology(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    AT(dvz_panel_set_msaa(
           panel, &(DvzMsaaDesc){
                      DVZ_STRUCT_INIT_FIELDS(DvzMsaaDesc), .enabled = true, .sample_count = 4,
                      .alpha_to_coverage = true}) == DVZ_OK);

    DvzVisual* sphere = dvz_sphere(scene, 0);
    AT(sphere != NULL);
    vec3 positions[1] = {{0.0f, 0.0f, 0.0f}};
    DvzColor colors[1] = {{180, 200, 255, 255}};
    float sizes[1] = {0.35f};
    AT(dvz_visual_set_data(sphere, "position", positions, 1) == 0);
    AT(dvz_visual_set_data(sphere, "color", colors, 1) == 0);
    AT(dvz_visual_set_data(sphere, "size", sizes, 1) == 0);
    AT(dvz_panel_add_visual(panel, sphere, NULL) == 0);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.max_color_sample_count = 1;
    caps.max_depth_sample_count = 1;

    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.target_width = 64;
    cfg.target_height = 64;
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) >= 1);
    AT(dvz_diagnostic_report_error_count(&report) == 0);
    AT(dvz_diagnostic_report_get_severity(&report, 0) == DVZ_DIAGNOSTIC_SEVERITY_WARNING);
    const char* message = dvz_diagnostic_report_get(&report, 0);
    ANN(message);
    AT(strstr(message, "MSAA disabled") != NULL);
    DvzDrp2ValidationResult validation = dvz_drp2_validate_stream(stream);
    AT(validation.ok);

    bool found_msaa_texture = false;
    bool found_resolve = false;
    bool found_single_sample_pipeline = false;
    bool found_multisample_pipeline = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        ANN(cmd);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE)
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.create_texture.id);
            found_msaa_texture =
                found_msaa_texture || (label != NULL && strcmp(label, "fig0_p0.msaa.color") == 0);
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS)
        {
            for (uint32_t j = 0; j < cmd->u.begin_render_pass.color_attachment_count; j++)
                found_resolve =
                    found_resolve ||
                    cmd->u.begin_render_pass.color_attachments[j].resolve_texture_id != 0;
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.create_render_pipeline.id);
            if (label != NULL && strstr(label, "_pipe_sphere") != NULL)
            {
                found_single_sample_pipeline = found_single_sample_pipeline ||
                                               cmd->u.create_render_pipeline.sample_count == 1;
                found_multisample_pipeline =
                    found_multisample_pipeline || cmd->u.create_render_pipeline.sample_count > 1;
            }
        }
    }
    AT(!found_msaa_texture);
    AT(!found_resolve);
    AT(found_single_sample_pipeline);
    AT(!found_multisample_pipeline);

    _test_scene_stream_destroy(stream);
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
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
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
    AT(dvz_frame_plan_graph_pass_count(default_plan) == 1);
    dvz_frame_plan_destroy(default_plan);

    AT(dvz_panel_set_edl(
           panel, &(DvzEdlDesc){
                      DVZ_STRUCT_INIT_FIELDS(DvzEdlDesc), .radius = 2.0f, .strength = 55.0f,
                      .depth_scale = 1.0f}) == DVZ_OK);

    DvzFramePlan* plan = dvz_frame_plan("figure.edl", 0);
    ANN(plan);
    DvzDiagnosticReport emit_report = {0};
    dvz_diagnostic_report_init(&emit_report);
    bool emitted = _scene_emit_panel_render_ex(figure, 0, plan, "figure_0", &emit_report);
    if (dvz_diagnostic_report_count(&emit_report) > 0)
        log_error("%s", dvz_diagnostic_report_get(&emit_report, 0));
    AT(emitted);
    AT(dvz_diagnostic_report_count(&emit_report) == 0);
    AT(dvz_frame_plan_node_count(plan) == 4);
    const DvzFramePlanNode* opaque_node = dvz_frame_plan_node_get(plan, 0);
    const DvzFramePlanNode* upload_node = dvz_frame_plan_node_get(plan, 1);
    const DvzFramePlanNode* edl_node = dvz_frame_plan_node_get(plan, 2);
    ANN(opaque_node);
    ANN(upload_node);
    ANN(edl_node);
    AT(_frame_plan_render_pass_role(opaque_node) == DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE);
    AT(dvz_frame_plan_node_type(upload_node) == DVZ_FRAME_PLAN_NODE_UPLOAD);
    AT(_frame_plan_render_pass_role(edl_node) == DVZ_FRAME_PLAN_RENDER_PASS_EDL_RESOLVE);
    AT(strcmp(upload_node->u.upload.resource_id, "figure_0_p0.edl.params") == 0);
    AT(upload_node->u.upload.byte_size == sizeof(DvzSceneEdlUniform));
    AT(dvz_frame_plan_graph_pass_count(plan) == 3);
    const DvzFrameGraphPass* opaque_pass = dvz_frame_plan_graph_pass_get(plan, 0);
    const DvzFrameGraphPass* edl_pass = dvz_frame_plan_graph_pass_get(plan, 1);
    ANN(opaque_pass);
    ANN(edl_pass);
    AT(_scene_test_graph_pass_provider(plan, opaque_pass) == DVZ_SCENE_WORK_PROVIDER_OPAQUE);
    AT(_scene_test_graph_pass_provider(plan, edl_pass) == DVZ_SCENE_WORK_PROVIDER_EDL);
    AT(opaque_pass->has_depth_attachment);
    AT(opaque_pass->color_attachment_count == 2);
    const DvzFrameGraphResource* opaque_color =
        _test_graph_resource(plan, opaque_pass->color_attachments[0].resource_id);
    const DvzFrameGraphResource* opaque_depth_product =
        _test_graph_resource(plan, opaque_pass->color_attachments[1].resource_id);
    ANN(opaque_color);
    ANN(opaque_depth_product);
    AT(opaque_depth_product->format == DVZ_FORMAT_R32_SFLOAT);
    AT(edl_pass->read_count == 2);
    AT(strcmp(edl_pass->reads[0].resource_id, opaque_color->id) == 0);
    AT(strcmp(edl_pass->reads[1].resource_id, opaque_depth_product->id) == 0);
    const DvzSceneResolvedPass* edl_work = _graph_composition_pass(plan, edl_pass);
    ANN(edl_work);
    AT(edl_work->auxiliary_binding_count == 1);
    AT(edl_work->auxiliary_bindings[0].kind == DVZ_SCENE_AUXILIARY_EDL_PARAMS);
    AT(edl_work->auxiliary_bindings[0].upload_node_index == 1);
    AT(edl_work->auxiliary_bindings[0].binding == 3);
    DvzScenePassContract opaque_contract = {0};
    AT(_scene_pass_contract_from_render(plan, panel, opaque_node, opaque_pass, &opaque_contract));
    AT(opaque_contract.draw_count == 1);
    const DvzSceneBlendTargetContract authored_color_target =
        opaque_contract.draws[0].blend_targets[0];

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.target_width = 64;
    cfg.target_height = 64;
    caps = dvz_capability_snapshot();
    caps.max_color_attachments = 2;
    caps.supports_render_target_sampling = true;
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    if (stream == NULL)
    {
        for (uint32_t i = 0; i < dvz_diagnostic_report_count(&report); i++)
            log_error("%s", dvz_diagnostic_report_get(&report, i));
    }
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);
    DvzDrp2ValidationResult validation = dvz_drp2_validate_stream(stream);
    AT(validation.ok);

    bool found_color_texture = false;
    bool found_depth_texture = false;
    bool found_params_upload = false;
    bool found_edl_pipeline = false;
    bool found_opaque_surface_pipeline = false;
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
                (label != NULL && strstr(label, "fig0_p0.scene.color.v") != NULL &&
                 cmd->u.create_texture.format == DVZ_FORMAT_R8G8B8A8_UNORM);
            found_depth_texture =
                found_depth_texture ||
                (label != NULL && strstr(label, "fig0_p0.surface.depth") != NULL &&
                 cmd->u.create_texture.format == DVZ_FORMAT_R32_SFLOAT);
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_WRITE_BUFFER)
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.write_buffer.buffer_id);
            found_params_upload = found_params_upload ||
                                  (label != NULL && strcmp(label, "fig0_p0.edl.params") == 0 &&
                                   cmd->u.write_buffer.size == sizeof(DvzSceneEdlUniform));
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.create_render_pipeline.id);
            found_edl_pipeline = found_edl_pipeline ||
                                 (label != NULL && strstr(label, "_pipe_edl_resolve") != NULL);
            if (label != NULL && strstr(label, "_pipe_point") != NULL &&
                cmd->u.create_render_pipeline.color_target_count == 2)
            {
                const DvzDrp2ColorTarget* color = &cmd->u.create_render_pipeline.color_targets[0];
                const DvzDrp2ColorTarget* depth = &cmd->u.create_render_pipeline.color_targets[1];
                AT(color->format == DVZ_FORMAT_R8G8B8A8_UNORM);
                AT(color->blend_enabled == authored_color_target.blend_enabled);
                AT(depth->format == DVZ_FORMAT_R32_SFLOAT);
                AT(!depth->blend_enabled);
                found_opaque_surface_pipeline = true;
            }
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
    AT(found_opaque_surface_pipeline);
    AT(found_edl_bind_group);

    _test_scene_stream_destroy(stream);
    dvz_frame_plan_destroy(plan);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify blended overlays do not suppress an EDL post-process graph pass.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_edl_blended_overlay_runtime_lowering(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
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

    DvzVisual* overlay = dvz_point(scene, 0);
    AT(overlay != NULL);
    vec3 overlay_positions[1] = {{-0.45f, +0.42f, 0.0f}};
    DvzColor overlay_colors[1] = {{255, 255, 255, 180}};
    float overlay_sizes[1] = {10.0f};
    AT(dvz_visual_set_data(overlay, "position", overlay_positions, 1) == 0);
    AT(dvz_visual_set_data(overlay, "color", overlay_colors, 1) == 0);
    AT(dvz_visual_set_data(overlay, "size", overlay_sizes, 1) == 0);
    AT(dvz_visual_set_alpha_mode(overlay, DVZ_ALPHA_BLENDED) == 0);
    AT(dvz_panel_add_visual(panel, overlay, NULL) == 0);

    AT(dvz_panel_set_edl(
           panel, &(DvzEdlDesc){
                      DVZ_STRUCT_INIT_FIELDS(DvzEdlDesc), .radius = 2.0f, .strength = 55.0f,
                      .depth_scale = 1.0f}) == DVZ_OK);

    DvzFramePlan* plan = dvz_frame_plan("figure.edl.label", 0);
    ANN(plan);
    _scene_emit_panel_render(figure, 0, plan, "figure_0");

    bool found_edl_node = false;
    bool found_blended_node = false;
    for (uint32_t i = 0; i < dvz_frame_plan_node_count(plan); i++)
    {
        const DvzFramePlanNode* node = dvz_frame_plan_node_get(plan, i);
        ANN(node);
        if (node->type == DVZ_FRAME_PLAN_NODE_RENDER)
        {
            found_edl_node = found_edl_node || _frame_plan_render_pass_role(node) ==
                                                   DVZ_FRAME_PLAN_RENDER_PASS_EDL_RESOLVE;
            found_blended_node =
                found_blended_node ||
                _frame_plan_render_pass_role(node) == DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND;
        }
    }
    AT(found_edl_node);
    AT(found_blended_node);

    bool found_edl_pass = false;
    bool found_blended_pass = false;
    for (uint32_t i = 0; i < dvz_frame_plan_graph_pass_count(plan); i++)
    {
        const DvzFrameGraphPass* pass = dvz_frame_plan_graph_pass_get(plan, i);
        ANN(pass);
        const DvzSceneWorkProviderKey provider = _scene_test_graph_pass_provider(plan, pass);
        found_edl_pass = found_edl_pass || provider == DVZ_SCENE_WORK_PROVIDER_EDL;
        found_blended_pass =
            found_blended_pass || provider == DVZ_SCENE_WORK_PROVIDER_TRANSPARENT_BLEND;
    }
    AT(found_edl_pass);
    AT(found_blended_pass);

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
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
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
        scene, &(DvzSceneBufferDesc){
                   DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, mesh_indices, sizeof(mesh_indices)) == DVZ_OK);

    DvzVisual* mesh = dvz_mesh(scene, 0);
    AT(mesh != NULL);
    AT(dvz_visual_set_data(mesh, "position", mesh_positions, 4) == 0);
    AT(dvz_visual_set_data(mesh, "normal", mesh_normals, 4) == 0);
    AT(dvz_visual_set_buffer(mesh, "index", index_buffer) == DVZ_OK);
    AT(dvz_panel_add_visual(panel, mesh, NULL) == 0);

    AT(dvz_panel_set_edl(
           panel, &(DvzEdlDesc){
                      DVZ_STRUCT_INIT_FIELDS(DvzEdlDesc), .radius = 2.0f, .strength = 55.0f,
                      .depth_scale = 1.0f}) == DVZ_OK);

    DvzFramePlan* plan = dvz_frame_plan("figure.edl.depth_producers", 0);
    ANN(plan);
    _scene_emit_panel_render(figure, 0, plan, "figure_0");
    AT(dvz_frame_plan_node_count(plan) == 4);
    const DvzFramePlanNode* opaque_node = dvz_frame_plan_node_get(plan, 0);
    const DvzFramePlanNode* edl_node = dvz_frame_plan_node_get(plan, 2);
    ANN(opaque_node);
    ANN(edl_node);
    AT(_frame_plan_render_pass_role(opaque_node) == DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE);
    AT(opaque_node->u.render.visual_count == 3);
    AT(_frame_plan_render_pass_role(edl_node) == DVZ_FRAME_PLAN_RENDER_PASS_EDL_RESOLVE);
    AT(dvz_frame_plan_graph_pass_count(plan) == 3);
    const DvzFrameGraphPass* opaque_pass = dvz_frame_plan_graph_pass_get(plan, 0);
    ANN(opaque_pass);
    AT(opaque_pass->has_depth_attachment);
    AT(strcmp(opaque_pass->depth_attachment.resource_id, "figure_0_p0.depth") == 0);

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
    DvzPanel* fixed_panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 0.5f, 1.0f});
    DvzPanel* transparent_panel = dvz_panel(figure, &(DvzPanelDesc){0.5f, 0.0f, 0.5f, 1.0f});
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
           &(DvzVisualAttachDesc){
               DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc),
               .controller_mode = DVZ_CONTROLLER_FIXED}) == 0);
    AT(dvz_panel_set_edl(
           fixed_panel, &(DvzEdlDesc){
                            DVZ_STRUCT_INIT_FIELDS(DvzEdlDesc), .radius = 2.0f, .strength = 55.0f,
                            .depth_scale = 1.0f}) == DVZ_OK);

    DvzFramePlan* fixed_plan = dvz_frame_plan("figure.edl.fixed", 0);
    ANN(fixed_plan);
    _scene_emit_panel_render(figure, 0, fixed_plan, "figure_0");
    AT(dvz_frame_plan_node_count(fixed_plan) == 1);
    AT(dvz_frame_plan_graph_pass_count(fixed_plan) == 1);

    float point_sizes[3] = {18.0f, 18.0f, 18.0f};
    DvzVisual* transparent_point = dvz_point(scene, 0);
    AT(transparent_point != NULL);
    AT(dvz_visual_set_data(transparent_point, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(transparent_point, "color", colors, 3) == 0);
    AT(dvz_visual_set_data(transparent_point, "size", point_sizes, 3) == 0);
    AT(dvz_visual_set_alpha_mode(transparent_point, DVZ_ALPHA_WBOIT) == 0);
    AT(dvz_panel_add_visual(transparent_panel, transparent_point, NULL) == 0);
    AT(dvz_panel_set_edl(
           transparent_panel, &(DvzEdlDesc){
                                  DVZ_STRUCT_INIT_FIELDS(DvzEdlDesc), .radius = 2.0f,
                                  .strength = 55.0f, .depth_scale = 1.0f}) == DVZ_OK);

    DvzFramePlan* transparent_plan = dvz_frame_plan("figure.edl.transparent", 0);
    ANN(transparent_plan);
    _scene_emit_panel_render(figure, 1, transparent_plan, "figure_0");
    bool found_edl_resolve = false;
    bool found_wboit_resolve = false;
    for (uint32_t i = 0; i < dvz_frame_plan_graph_pass_count(transparent_plan); i++)
    {
        const DvzFrameGraphPass* pass = dvz_frame_plan_graph_pass_get(transparent_plan, i);
        ANN(pass);
        const DvzSceneWorkProviderKey provider =
            _scene_test_graph_pass_provider(transparent_plan, pass);
        found_edl_resolve = found_edl_resolve || provider == DVZ_SCENE_WORK_PROVIDER_EDL;
        found_wboit_resolve =
            found_wboit_resolve || provider == DVZ_SCENE_WORK_PROVIDER_WBOIT_RESOLVE;
    }
    AT(!found_edl_resolve);
    AT(found_wboit_resolve);

    dvz_frame_plan_destroy(transparent_plan);
    dvz_frame_plan_destroy(fixed_plan);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Verify opt-in GTAO declares a G-buffer-backed graph without changing the default path.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_gtao_graph_foundation(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
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
        scene, &(DvzSceneBufferDesc){
                   DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)) == DVZ_OK);

    AT(dvz_visual_set_data(mesh, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(mesh, "normal", normals, 4) == 0);
    AT(dvz_visual_set_buffer(mesh, "index", index_buffer) == DVZ_OK);
    AT(dvz_panel_add_visual(panel, mesh, NULL) == 0);

    AT(!_scene_technique_state_ao_enabled(&panel->techniques));
    AT(panel->techniques.ao.radius == 1.0f);
    AT(panel->techniques.ao.intensity == 1.0f);
    AT(panel->techniques.ao.thickness == 0.25f);
    AT(panel->techniques.ao.quality == DVZ_AO_QUALITY_MEDIUM);

    DvzFramePlan* default_plan = dvz_frame_plan("figure.gtao.default", 0);
    ANN(default_plan);
    _scene_emit_panel_render(figure, 0, default_plan, "figure_0");
    AT(dvz_frame_plan_node_count(default_plan) == 1);
    AT(dvz_frame_plan_graph_pass_count(default_plan) == 1);
    dvz_frame_plan_destroy(default_plan);

    AT(_scene_technique_state_set_ao(
        &panel->techniques,
        &(DvzSceneAoDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzSceneAoDesc), .radius = 1.25f, .intensity = 2.0f,
            .thickness = 0.2f, .quality = DVZ_AO_QUALITY_HIGH}));
    const DvzSceneAoTechniqueState* gtao = _scene_technique_ao_state(scene, panel);
    ANN(gtao);
    AT(gtao->enabled);
    AT(gtao->radius == 1.25f);
    AT(gtao->intensity == 2.0f);
    AT(gtao->thickness == 0.2f);
    AT(gtao->quality == DVZ_AO_QUALITY_HIGH);
    AT(gtao->denoise_enabled);

    /* Keep direct coverage of the internal no-denoise lowering branch. */
    panel->techniques.ao.denoise_enabled = false;
    DvzFramePlan* plan = dvz_frame_plan("figure.gtao", 0);
    ANN(plan);
    _scene_emit_panel_render(figure, 0, plan, "figure_0");
    AT(dvz_frame_plan_node_count(plan) == 4);
    AT(dvz_frame_plan_graph_pass_count(plan) == 3);
    const DvzFramePlanNode* gbuffer_node = dvz_frame_plan_node_get(plan, 0);
    const DvzFramePlanNode* upload_node = dvz_frame_plan_node_get(plan, 1);
    const DvzFramePlanNode* gtao_node = dvz_frame_plan_node_get(plan, 2);
    const DvzFramePlanNode* opaque_node = dvz_frame_plan_node_get(plan, 3);
    ANN(gbuffer_node);
    ANN(opaque_node);
    ANN(upload_node);
    ANN(gtao_node);
    AT(_frame_plan_render_pass_role(gbuffer_node) == DVZ_FRAME_PLAN_RENDER_PASS_GBUFFER);
    AT(_frame_plan_render_pass_role(opaque_node) == DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE);
    AT(strcmp(upload_node->u.upload.resource_id, "figure_0_p0.gtao.params") == 0);
    AT(_frame_plan_render_pass_role(gtao_node) == DVZ_FRAME_PLAN_RENDER_PASS_GTAO);

    bool found_normal = false;
    bool found_depth = false;
    bool found_coverage = false;
    bool found_raw_visibility = false;
    for (uint32_t i = 0; i < dvz_frame_plan_graph_resource_count(plan); i++)
    {
        const DvzFrameGraphResource* resource = dvz_frame_plan_graph_resource_get(plan, i);
        ANN(resource);
        found_normal = found_normal || strcmp(resource->id, "figure_0_p0.gbuffer.normal") == 0;
        found_depth = found_depth || strcmp(resource->id, "figure_0_p0.gbuffer.depth") == 0;
        found_coverage =
            found_coverage || strcmp(resource->id, "figure_0_p0.gbuffer.coverage") == 0;
        found_raw_visibility = found_raw_visibility ||
                               (strcmp(resource->id, "figure_0_p0.gtao.raw_visibility") == 0 &&
                                resource->format == DVZ_FORMAT_R32_SFLOAT);
    }
    AT(found_normal);
    AT(found_depth);
    AT(found_coverage);
    AT(found_raw_visibility);

    const DvzFrameGraphPass* gbuffer_pass = dvz_frame_plan_graph_pass_get(plan, 0);
    const DvzFrameGraphPass* gtao_pass = dvz_frame_plan_graph_pass_get(plan, 1);
    const DvzFrameGraphPass* opaque_pass = dvz_frame_plan_graph_pass_get(plan, 2);
    ANN(gbuffer_pass);
    ANN(opaque_pass);
    ANN(gtao_pass);
    AT(_scene_test_graph_pass_provider(plan, gbuffer_pass) ==
       DVZ_SCENE_WORK_PROVIDER_SURFACE_CAPTURE);
    AT(_scene_test_graph_pass_provider(plan, opaque_pass) == DVZ_SCENE_WORK_PROVIDER_OPAQUE);
    AT(_scene_test_graph_pass_provider(plan, gtao_pass) == DVZ_SCENE_WORK_PROVIDER_GTAO);
    AT(gbuffer_pass->color_attachment_count == 3);
    AT(strcmp(gbuffer_pass->color_attachments[0].resource_id, "figure_0_p0.gbuffer.depth") == 0);
    AT(strcmp(gbuffer_pass->color_attachments[1].resource_id, "figure_0_p0.gbuffer.normal") == 0);
    AT(strcmp(gbuffer_pass->color_attachments[2].resource_id, "figure_0_p0.gbuffer.coverage") ==
       0);
    AT(gtao_pass->read_count == 3);
    AT(strcmp(gtao_pass->reads[0].resource_id, "figure_0_p0.gbuffer.normal") == 0);
    AT(strcmp(gtao_pass->reads[1].resource_id, "figure_0_p0.gbuffer.depth") == 0);
    AT(strcmp(gtao_pass->reads[2].resource_id, "figure_0_p0.gbuffer.coverage") == 0);
    AT(strcmp(gtao_pass->color_attachments[0].resource_id, "figure_0_p0.gtao.raw_visibility") ==
       0);
    AT(opaque_pass->read_count == 1);
    AT(strcmp(opaque_pass->reads[0].resource_id, "figure_0_p0.gtao.raw_visibility") == 0);
    const DvzSceneResolvedPass* gtao_work = _graph_composition_pass(plan, gtao_pass);
    const DvzSceneResolvedPass* opaque_work = _graph_composition_pass(plan, opaque_pass);
    ANN(gtao_work);
    ANN(opaque_work);
    AT(gtao_work->auxiliary_binding_count == 1);
    AT(gtao_work->auxiliary_bindings[0].kind == DVZ_SCENE_AUXILIARY_GTAO_PARAMS);
    AT(gtao_work->auxiliary_bindings[0].upload_node_index == 1);
    AT(gtao_work->auxiliary_bindings[0].binding == 4);

    dvz_frame_plan_destroy(plan);

    AT(_scene_technique_state_set_ao(
        &panel->techniques,
        &(DvzSceneAoDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzSceneAoDesc), .radius = 1.25f, .intensity = 2.0f,
            .thickness = 0.2f, .quality = DVZ_AO_QUALITY_HIGH}));
    plan = dvz_frame_plan("figure.gtao.denoise", 0);
    ANN(plan);
    DvzDiagnosticReport denoise_report = {0};
    dvz_diagnostic_report_init(&denoise_report);
    AT(_scene_emit_panel_render_ex(figure, 0, plan, "figure_0", &denoise_report));
    AT(dvz_frame_plan_node_count(plan) == 6);
    AT(dvz_frame_plan_graph_pass_count(plan) == 5);
    const DvzFramePlanNode* denoise_x_node = dvz_frame_plan_node_get(plan, 3);
    const DvzFramePlanNode* denoise_y_node = dvz_frame_plan_node_get(plan, 4);
    ANN(denoise_x_node);
    ANN(denoise_y_node);
    AT(_frame_plan_render_pass_role(denoise_x_node) == DVZ_FRAME_PLAN_RENDER_PASS_GTAO_DENOISE);
    AT(_frame_plan_render_pass_role(denoise_y_node) == DVZ_FRAME_PLAN_RENDER_PASS_GTAO_DENOISE);
    const DvzFrameGraphPass* denoise_x_pass = dvz_frame_plan_graph_pass_get(plan, 2);
    const DvzFrameGraphPass* denoise_y_pass = dvz_frame_plan_graph_pass_get(plan, 3);
    ANN(denoise_x_pass);
    ANN(denoise_y_pass);
    AT(_scene_test_graph_pass_provider(plan, denoise_x_pass) ==
       DVZ_SCENE_WORK_PROVIDER_GTAO_DENOISE);
    AT(denoise_x_pass->read_count == 4);
    AT(strcmp(denoise_x_pass->reads[0].resource_id, "figure_0_p0.gtao.raw_visibility") == 0);
    AT(strcmp(denoise_x_pass->reads[1].resource_id, "figure_0_p0.gbuffer.normal") == 0);
    AT(strcmp(denoise_x_pass->reads[2].resource_id, "figure_0_p0.gbuffer.depth") == 0);
    AT(strcmp(denoise_x_pass->reads[3].resource_id, "figure_0_p0.gbuffer.coverage") == 0);
    AT(strcmp(
           denoise_x_pass->color_attachments[0].resource_id,
           "figure_0_p0.gtao.denoise.tmp") == 0);
    AT(strcmp(denoise_y_pass->reads[0].resource_id, "figure_0_p0.gtao.denoise.tmp") == 0);
    AT(strcmp(denoise_y_pass->color_attachments[0].resource_id, "figure_0_p0.gtao.denoise") == 0);

    dvz_frame_plan_destroy(plan);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Verify opt-in GTAO lowers its graph resources and fullscreen passes to DRP2.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_gtao_runtime_lowering(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    AT(dvz_panel_set_msaa(
           panel, &(DvzMsaaDesc){
                      DVZ_STRUCT_INIT_FIELDS(DvzMsaaDesc), .enabled = true, .sample_count = 4,
                      .alpha_to_coverage = true}) == DVZ_OK);

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
        scene, &(DvzSceneBufferDesc){
                   DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)) == DVZ_OK);

    AT(dvz_visual_set_data(mesh, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(mesh, "normal", normals, 4) == 0);
    AT(dvz_visual_set_buffer(mesh, "index", index_buffer) == DVZ_OK);
    AT(dvz_panel_add_visual(panel, mesh, NULL) == 0);
    AT(_scene_technique_state_set_ao(
        &panel->techniques,
        &(DvzSceneAoDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzSceneAoDesc), .radius = 1.25f, .intensity = 2.0f,
            .thickness = 0.2f, .quality = DVZ_AO_QUALITY_MEDIUM,
            .debug_mode = DVZ_AO_DEBUG_VISIBILITY}));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.target_width = 64;
    cfg.target_height = 64;
    caps = dvz_capability_snapshot();
    caps.supports_color_blending = true;
    caps.max_color_attachments = 3;
    caps.supports_render_target_sampling = true;
    caps.render_target_format_rgba16float = true;
    caps.render_target_format_r16float = true;
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    if (stream == NULL)
        for (uint32_t i = 0; i < dvz_diagnostic_report_count(&report); i++)
            log_error("%s", dvz_diagnostic_report_get(&report, i));
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);
    DvzDrp2ValidationResult validation = dvz_drp2_validate_stream(stream);
    if (!validation.ok)
    {
        const DvzDrp2Command* failed = dvz_drp2_stream_get(stream, validation.command_index);
        log_error(
            "AO DRP2 validation code=%u command=%u type=%u", validation.code,
            validation.command_index, failed != NULL ? failed->type : UINT32_MAX);
    }
    AT(validation.ok);

    bool found_raw_visibility_texture = false;
    bool found_params_upload = false;
    bool found_gtao_pipeline = false;
    bool found_debug_pipeline = false;
    bool found_ambient_material_pipeline = false;
    bool found_gtao_bind_group = false;
    bool found_ambient_bind_group = false;
    bool found_msaa_color_texture = false;
    bool found_msaa_render_pipeline = false;
    bool found_single_sample_surface = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        ANN(cmd);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE)
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.create_texture.id);
            found_raw_visibility_texture =
                found_raw_visibility_texture ||
                (label != NULL && strcmp(label, "fig0_p0.gtao.raw_visibility") == 0 &&
                 cmd->u.create_texture.format == DVZ_FORMAT_R32_SFLOAT &&
                 (cmd->u.create_texture.usage & DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT) != 0 &&
                 (cmd->u.create_texture.usage & DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING) != 0);
            found_msaa_color_texture =
                found_msaa_color_texture ||
                (label != NULL && strcmp(label, "fig0_p0.msaa.color") == 0 &&
                 cmd->u.create_texture.sample_count == 4);
            found_single_sample_surface =
                found_single_sample_surface ||
                (label != NULL && strcmp(label, "fig0_p0.surface.normal") == 0 &&
                 cmd->u.create_texture.sample_count == 1);
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_WRITE_BUFFER)
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.write_buffer.buffer_id);
            found_params_upload = found_params_upload ||
                                  (label != NULL && strcmp(label, "fig0_p0.gtao.params") == 0 &&
                                   cmd->u.write_buffer.size == sizeof(DvzSceneAoUniform));
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            const char* label = dvz_drp2_stream_label(stream, cmd->u.create_render_pipeline.id);
            found_gtao_pipeline =
                found_gtao_pipeline ||
                (label != NULL && strstr(label, "_pipe_gtao") != NULL &&
                 strstr(label, "_pipe_gtao_visibility_present") == NULL &&
                 cmd->u.create_render_pipeline.color_targets[0].format == DVZ_FORMAT_R32_SFLOAT);
            found_ambient_material_pipeline =
                found_ambient_material_pipeline ||
                (label != NULL && strstr(label, "ambient_visibility") != NULL &&
                 cmd->u.create_render_pipeline.bind_group_layout_count == 4);
            found_debug_pipeline =
                found_debug_pipeline ||
                (label != NULL && strstr(label, "_pipe_gtao_visibility_present") != NULL &&
                 !cmd->u.create_render_pipeline.color_targets[0].blend_enabled &&
                 cmd->u.create_render_pipeline.bind_group_layout_count == 1);
            found_msaa_render_pipeline =
                found_msaa_render_pipeline || cmd->u.create_render_pipeline.sample_count == 4;
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_CREATE_BIND_GROUP)
        {
            found_gtao_bind_group =
                found_gtao_bind_group || cmd->u.create_bind_group.entry_count == 5;
            found_ambient_bind_group =
                found_ambient_bind_group || cmd->u.create_bind_group.entry_count == 2;
        }
    }
    AT(found_raw_visibility_texture);
    AT(found_params_upload);
    AT(found_gtao_pipeline);
    AT(found_debug_pipeline);
    AT(found_ambient_material_pipeline);
    AT(found_gtao_bind_group);
    AT(found_ambient_bind_group);
    AT(found_msaa_color_texture);
    AT(found_msaa_render_pipeline);
    AT(found_single_sample_surface);

    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Execute the scene AO debug-presentation path through the vklite runtime when a GPU is available.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_gtao_glsl_executes(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_GRAPH_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_testing_gpu_ctx_config(suite);
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
        scene, &(DvzSceneBufferDesc){
                   DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)) == DVZ_OK);

    AT(dvz_visual_set_data(mesh, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(mesh, "normal", normals, 4) == 0);
    AT(dvz_visual_set_buffer(mesh, "index", index_buffer) == DVZ_OK);
    AT(dvz_panel_add_visual(panel, mesh, NULL) == 0);
    AT(_scene_technique_state_set_ao(
        &panel->techniques,
        &(DvzSceneAoDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzSceneAoDesc), .radius = 1.0f, .intensity = 2.5f,
            .thickness = 0.1f, .quality = DVZ_AO_QUALITY_MEDIUM,
            .debug_mode = DVZ_AO_DEBUG_VISIBILITY}));

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.supports_color_blending = true;
    caps.max_color_attachments = 3;
    caps.supports_render_target_sampling = true;
    caps.render_target_format_rgba16float = true;
    caps.render_target_format_r16float = true;
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    emit_cfg.target_width = 64;
    emit_cfg.target_height = 64;

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
    for (uint32_t i = 0; i < dvz_diagnostic_report_count(&report); i++)
        log_error("AO DRP2 diagnostic: %s", dvz_diagnostic_report_get(&report, i));
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
 * Execute GTAO with sphere impostors feeding the G-buffer through the vklite runtime.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_sphere_gtao_glsl_executes(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    TST_SCENE_GRAPH_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_testing_gpu_ctx_config(suite);
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
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    DvzVisual* sphere = dvz_sphere(scene, DVZ_SPHERE_FLAGS_LIGHTING);
    AT(sphere != NULL);
    AT(dvz_sphere_set_mode(sphere, DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR) == 0);

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
    AT(_scene_technique_state_set_ao(
        &panel->techniques,
        &(DvzSceneAoDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzSceneAoDesc), .radius = 1.0f, .intensity = 2.5f,
            .thickness = 0.1f, .quality = DVZ_AO_QUALITY_MEDIUM}));

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.supports_color_blending = true;
    caps.max_color_attachments = 3;
    caps.supports_render_target_sampling = true;
    caps.render_target_format_rgba16float = true;
    caps.render_target_format_r16float = true;
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
 * Verify GTAO opt-in is a no-op when no opaque normal-producing visual is present.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_gtao_ignores_ineligible_visuals(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
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
    AT(_scene_technique_state_set_ao(
        &panel->techniques,
        &(DvzSceneAoDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzSceneAoDesc), .radius = 1.25f, .intensity = 2.0f,
            .thickness = 0.2f, .quality = DVZ_AO_QUALITY_HIGH}));

    DvzFramePlan* plan = dvz_frame_plan("figure.gtao.ineligible", 0);
    ANN(plan);
    _scene_emit_panel_render(figure, 0, plan, "figure_0");
    AT(dvz_frame_plan_node_count(plan) == 1);
    AT(dvz_frame_plan_graph_pass_count(plan) == 1);
    for (uint32_t i = 0; i < dvz_frame_plan_graph_resource_count(plan); i++)
    {
        const DvzFrameGraphResource* resource = dvz_frame_plan_graph_resource_get(plan, i);
        ANN(resource);
        AT(strstr(resource->id, ".gbuffer.") == NULL);
        AT(strstr(resource->id, ".gtao.") == NULL);
    }

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
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
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
    AT(_frame_plan_render_pass_role(opaque_node) == DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE);
    AT(_frame_plan_render_pass_role(transparent_node) ==
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
    AT(blend_contract.draws[0].blend_targets[0].src_color_blend_factor ==
       DVZ_BLEND_FACTOR_SRC_ALPHA);
    AT(blend_contract.draws[0].blend_targets[0].dst_color_blend_factor ==
       DVZ_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
    AT(blend_contract.draws[0].blend_targets[0].src_alpha_blend_factor == DVZ_BLEND_FACTOR_ONE);

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
                     DVZ_BLEND_FACTOR_SRC_ALPHA &&
                 command->u.create_render_pipeline.color_targets[0].dst_color_blend_factor ==
                     DVZ_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
            has_standard_blend_depth_test_pipeline =
                has_standard_blend_depth_test_pipeline ||
                (command->u.create_render_pipeline.color_targets[0].blend_enabled &&
                 command->u.create_render_pipeline.has_depth_attachment &&
                 !command->u.create_render_pipeline.depth_write_enabled &&
                 command->u.create_render_pipeline.depth_compare_op ==
                     DVZ_COMPARE_OP_LESS_OR_EQUAL);
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
 * Verify additive visuals use the ordinary transparent pass with exact additive RGB factors.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_visual_additive_blend(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel_full(figure);
    ANN(panel);
    DvzVisual* point = dvz_point(scene, 0);
    ANN(point);

    vec3 positions[2] = {{-0.1f, 0.0f, 0.0f}, {+0.1f, 0.0f, 0.0f}};
    DvzColor colors[2] = {{255, 128, 32, 96}, {64, 128, 255, 96}};
    float sizes[2] = {24.0f, 24.0f};
    AT(dvz_visual_set_data(point, "position", positions, 2) == DVZ_OK);
    AT(dvz_visual_set_data(point, "color", colors, 2) == DVZ_OK);
    AT(dvz_visual_set_data(point, "size", sizes, 2) == DVZ_OK);
    AT(dvz_visual_set_alpha_mode(point, DVZ_ALPHA_BLENDED) == DVZ_OK);
    AT(dvz_visual_set_blend_mode(point, DVZ_BLEND_ADDITIVE) == DVZ_OK);
    AT(dvz_panel_add_visual(panel, point, NULL) == DVZ_OK);

    DvzFramePlan* plan = dvz_frame_plan("figure.blend.additive", 0);
    ANN(plan);
    _scene_emit_panel_render(figure, 0, plan, "figure_0");
    AT(dvz_frame_plan_node_count(plan) == 2);
    const DvzFramePlanNode* node = dvz_frame_plan_node_get(plan, 1);
    ANN(node);
    AT(_frame_plan_render_pass_role(node) == DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND);
    AT(node->u.render.visual_count == 1);
    AT(node->u.render.visual_metadata[0].draw_blend_policy == DVZ_SCENE_BLEND_POLICY_ADDITIVE);

    DvzScenePassContract pass = {0};
    AT(_scene_pass_contract_from_render(plan, panel, node, NULL, &pass));
    AT(pass.draw_count == 1);
    AT(pass.draws[0].blend_policy == DVZ_SCENE_BLEND_POLICY_ADDITIVE);
    AT(pass.draws[0].blend_targets[0].dst_color_blend_factor == DVZ_BLEND_FACTOR_ONE);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
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
    AT(dvz_drp2_validate_stream(stream).ok);

    bool found = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        ANN(command);
        if (command->type != DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
            continue;
        const DvzDrp2ColorTarget* target = &command->u.create_render_pipeline.color_targets[0];
        found = found || (target->blend_enabled &&
                          target->src_color_blend_factor == DVZ_BLEND_FACTOR_SRC_ALPHA &&
                          target->dst_color_blend_factor == DVZ_BLEND_FACTOR_ONE &&
                          target->src_alpha_blend_factor == DVZ_BLEND_FACTOR_ONE &&
                          target->dst_alpha_blend_factor == DVZ_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
    }
    AT(found);

    _test_scene_stream_destroy(stream);
    dvz_frame_plan_destroy(plan);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Verify source-over geometry and volume visuals retain order within separate semantic phases.
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
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
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
           field, &(DvzFieldDataView){
                      DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView), .data = voxels, .bytes_per_row = 2,
                      .rows_per_image = 2}) == DVZ_OK);

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
        scene, &(DvzSceneBufferDesc){
                   DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)) == DVZ_OK);

    AT(dvz_visual_set_field(volume, "field", field) == DVZ_OK);
    AT(dvz_visual_set_field(slice, "field", field) == DVZ_OK);
    AT(dvz_volume_set_render_mode(volume, DVZ_VOLUME_RENDER_MIP) == 0);
    AT(dvz_volume_set_render_mode(slice, DVZ_VOLUME_RENDER_SLICE) == 0);
    AT(dvz_visual_set_alpha_mode(volume, DVZ_ALPHA_BLENDED) == 0);
    AT(dvz_visual_set_alpha_mode(slice, DVZ_ALPHA_BLENDED) == 0);
    AT(dvz_visual_set_data(mesh, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(mesh, "normal", normals, 4) == 0);
    AT(dvz_visual_set_buffer(mesh, "index", index_buffer) == DVZ_OK);
    AT(dvz_visual_set_alpha_mode(mesh, DVZ_ALPHA_BLENDED) == 0);

    AT(dvz_panel_add_visual(
           panel, volume,
           &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .z_layer = 0}) ==
       0);
    AT(dvz_panel_add_visual(
           panel, slice,
           &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .z_layer = 1}) ==
       0);
    AT(dvz_panel_add_visual(
           panel, mesh,
           &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .z_layer = 2}) ==
       0);

    DvzFramePlan* plan = dvz_frame_plan("figure.alpha.volume_mesh", 0);
    ANN(plan);
    _scene_emit_panel_render(figure, 0, plan, "figure_0");

    const DvzFramePlanNode* transparent_nodes[2] = {0};
    uint32_t transparent_node_count = 0;
    for (uint32_t i = 0; i < dvz_frame_plan_node_count(plan); i++)
    {
        const DvzFramePlanNode* node = dvz_frame_plan_node_get(plan, i);
        ANN(node);
        if (_frame_plan_render_pass_role(node) == DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND)
        {
            AT(transparent_node_count < 2);
            transparent_nodes[transparent_node_count++] = node;
        }
    }
    AT(transparent_node_count == 2);
    const DvzFramePlanNode* transparent_node = transparent_nodes[0];
    const DvzFramePlanNode* volume_node = transparent_nodes[1];
    const DvzPanelCompositionSnapshot* composition =
        _frame_plan_composition_get(plan, "figure_0_p0");
    ANN(composition);
    AT(transparent_node->u.render.composition_pass_id.value > 0);
    AT(transparent_node->u.render.composition_pass_id.value <= composition->pass_count);
    AT(volume_node->u.render.composition_pass_id.value > 0);
    AT(volume_node->u.render.composition_pass_id.value <= composition->pass_count);
    AT(composition->passes[transparent_node->u.render.composition_pass_id.value - 1]
           .technique_id == DVZ_SCENE_TECHNIQUE_TRANSPARENT_BLEND);
    AT(composition->passes[volume_node->u.render.composition_pass_id.value - 1].technique_id ==
       DVZ_SCENE_TECHNIQUE_VOLUME_SHADING);
    AT(transparent_node->u.render.visual_count == 1);
    AT(volume_node->u.render.visual_count == 2);
    AT(transparent_node->u.render.visual_metadata[0].visual_index == 2);
    AT(transparent_node->u.render.visual_metadata[0].visual_type == DVZ_VISUAL_TYPE_MESH);
    AT(volume_node->u.render.visual_metadata[0].visual_index == 0);
    AT(volume_node->u.render.visual_metadata[1].visual_index == 1);
    AT(volume_node->u.render.visual_metadata[0].visual_type == DVZ_VISUAL_TYPE_VOLUME);
    AT(volume_node->u.render.visual_metadata[1].visual_type == DVZ_VISUAL_TYPE_VOLUME);
    AT(transparent_node->u.render.has_pass_contract);
    AT(volume_node->u.render.has_pass_contract);
    AT(transparent_node->u.render.visual_metadata[0].has_draw_contract);
    AT(volume_node->u.render.visual_metadata[0].has_draw_contract);
    AT(volume_node->u.render.visual_metadata[1].has_draw_contract);
    AT(strlen(volume_node->u.render.visual_metadata[0].draw_contract_id) > 0);
    AT(volume_node->u.render.visual_metadata[0].draw_depth_policy ==
       DVZ_SCENE_DEPTH_POLICY_SAMPLE);
    AT(volume_node->u.render.visual_metadata[0].draw_blend_policy ==
       DVZ_SCENE_BLEND_POLICY_SOURCE_OVER);
    AT(volume_node->u.render.visual_metadata[0].draw_bind_group_layout_mask &
       DVZ_SCENE_BIND_GROUP_REQUIREMENT_VOLUME);
    AT(transparent_node->u.render.visual_metadata[0].draw_depth_policy ==
       DVZ_SCENE_DEPTH_POLICY_TEST);

    const DvzFrameGraphPass* blend_pass = NULL;
    const DvzFrameGraphPass* volume_pass = NULL;
    for (uint32_t i = 0; i < dvz_frame_plan_graph_pass_count(plan); i++)
    {
        const DvzFrameGraphPass* pass = dvz_frame_plan_graph_pass_get(plan, i);
        ANN(pass);
        if (_scene_test_graph_pass_provider(plan, pass) ==
            DVZ_SCENE_WORK_PROVIDER_TRANSPARENT_BLEND)
        {
            if (pass->composition_pass_id.value ==
                transparent_node->u.render.composition_pass_id.value)
                blend_pass = pass;
            else if (
                pass->composition_pass_id.value == volume_node->u.render.composition_pass_id.value)
                volume_pass = pass;
        }
    }
    ANN(blend_pass);
    ANN(volume_pass);
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
    AT(contract.transparent_blend);
    AT(contract.draw_count == 1);
    AT(contract.color_attachment_count == 1);
    AT(contract.has_depth_attachment);
    AT(contract.needs_common_set);
    AT(!contract.needs_volume_set);
    AT(contract.draws[0].depth_test);
    AT(!contract.draws[0].depth_write);
    dvz_diagnostic_report_init(&graph_report);
    AT(_scene_pass_contract_validate(&contract, &graph_report));
    AT(dvz_diagnostic_report_count(&graph_report) == 0);

    DvzScenePassContract volume_contract = {0};
    AT(_scene_pass_contract_from_render(plan, panel, volume_node, volume_pass, &volume_contract));
    AT(volume_contract.transparent_blend);
    AT(volume_contract.draw_count == 2);
    AT(volume_contract.needs_volume_set);
    AT(volume_contract.draws[0].samples_depth);
    AT(volume_contract.draws[1].samples_depth);
    dvz_diagnostic_report_init(&graph_report);
    AT(_scene_pass_contract_validate(&volume_contract, &graph_report));
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
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
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
           field, &(DvzFieldDataView){
                      DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView), .data = voxels, .bytes_per_row = 2,
                      .rows_per_image = 2}) == DVZ_OK);

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
        scene, &(DvzSceneBufferDesc){
                   DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc),
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)) == DVZ_OK);

    AT(dvz_visual_set_field(volume, "field", field) == DVZ_OK);
    AT(dvz_visual_set_field(slice, "field", field) == DVZ_OK);
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
    AT(dvz_visual_set_buffer(mesh, "index", index_buffer) == DVZ_OK);
    AT(dvz_visual_set_alpha_mode(mesh, DVZ_ALPHA_BLENDED) == 0);
    AT(dvz_visual_set_depth_test(mesh, true) == 0);
    AT(dvz_visual_set_scene_occluder(mesh, true) == 0);

    AT(dvz_panel_add_visual(
           panel, volume,
           &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .z_layer = 0}) ==
       0);
    AT(dvz_panel_add_visual(
           panel, slice,
           &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .z_layer = 1}) ==
       0);
    AT(dvz_panel_add_visual(
           panel, mesh,
           &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .z_layer = 2}) ==
       0);
    AT(dvz_panel_set_volume_occluder(
           panel, volume,
           &(DvzVolumeOcclusionDesc){
               DVZ_STRUCT_INIT_FIELDS(DvzVolumeOcclusionDesc),
               .enabled = true,
               .alpha_threshold = 0.01f,
               .fade_distance = 0.04f,
               .occluded_alpha = 0.2f,
           }) == 0);
    AT(dvz_panel_set_scene_occlusion(
           panel, &(DvzSceneOcclusionDesc){
                      DVZ_STRUCT_INIT_FIELDS(DvzSceneOcclusionDesc),
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
        if (_frame_plan_render_pass_role(node) == DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND)
            blend_node = node;
    }
    ANN(blend_node);
    AT(blend_node->u.render.visual_count == 2);
    AT(blend_node->u.render.visual_metadata[1].has_volume_occlusion);
    AT(blend_node->u.render.visual_metadata[1].has_scene_occlusion);
    AT(strcmp(
           blend_node->u.render.visual_metadata[1].draw_volume_occlusion_resource_id,
           "figure_0_p0.volume_occlusion.depth") == 0);
    AT(strcmp(
           blend_node->u.render.visual_metadata[1].draw_volume_occlusion_producer_pass_id,
           "figure_0_p0.volume_occlusion") == 0);
    AT(blend_node->u.render.visual_metadata[1].draw_volume_occlusion_bind_set ==
       DVZ_SCENE_SHADER_SET_VISUAL);
    AT(blend_node->u.render.visual_metadata[1].draw_volume_occlusion_bind_binding == 3);
    AT(strcmp(
           blend_node->u.render.visual_metadata[1].draw_scene_occlusion_resource_id,
           "figure_0_p0.scene_occlusion.depth") == 0);
    AT(strcmp(
           blend_node->u.render.visual_metadata[1].draw_scene_occlusion_producer_pass_id,
           "figure_0_p0.scene_occlusion") == 0);
    AT(blend_node->u.render.visual_metadata[1].draw_scene_occlusion_bind_set ==
       DVZ_SCENE_SHADER_SET_SCENE_OCCLUSION);
    AT(blend_node->u.render.visual_metadata[1].draw_scene_occlusion_bind_binding == 0);

    const DvzFrameGraphPass* volume_pass = NULL;
    const DvzFrameGraphPass* scene_pass = NULL;
    const DvzFrameGraphPass* blend_pass = NULL;
    for (uint32_t i = 0; i < dvz_frame_plan_graph_pass_count(plan); i++)
    {
        const DvzFrameGraphPass* pass = dvz_frame_plan_graph_pass_get(plan, i);
        ANN(pass);
        const DvzSceneWorkProviderKey provider = _scene_test_graph_pass_provider(plan, pass);
        if (provider == DVZ_SCENE_WORK_PROVIDER_VOLUME_OCCLUSION)
            volume_pass = pass;
        else if (provider == DVZ_SCENE_WORK_PROVIDER_SCENE_OCCLUSION)
            scene_pass = pass;
        else if (provider == DVZ_SCENE_WORK_PROVIDER_TRANSPARENT_BLEND)
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
            strcmp(blend_pass->reads[i].resource_id, "figure_0_p0.volume_occlusion.depth") == 0;
        reads_scene_occlusion =
            reads_scene_occlusion ||
            strcmp(blend_pass->reads[i].resource_id, "figure_0_p0.scene_occlusion.depth") == 0;
    }
    AT(reads_volume_occlusion);
    AT(reads_scene_occlusion);

    DvzDiagnosticReport graph_report;
    dvz_diagnostic_report_init(&graph_report);
    AT(dvz_frame_plan_graph_validate(plan, &graph_report));

    DvzScenePassContract contract = {0};
    AT(_scene_pass_contract_from_render(plan, panel, blend_node, blend_pass, &contract));
    AT(contract.transparent_blend);
    AT(contract.draw_count == 2);
    AT(contract.draws[1].samples_volume_occlusion);
    AT(contract.draws[1].samples_scene_occlusion);
    AT(contract.draws[1].needs_volume_set);
    AT(contract.draws[1].needs_scene_occlusion_set);
    AT(strcmp(
           contract.draws[1].volume_occlusion_resource_id, "figure_0_p0.volume_occlusion.depth") ==
       0);
    AT(strcmp(
           contract.draws[1].volume_occlusion_producer_pass_id, "figure_0_p0.volume_occlusion") ==
       0);
    AT(contract.draws[1].volume_occlusion_bind_set == 1);
    AT(contract.draws[1].volume_occlusion_bind_binding == 3);
    AT(strcmp(
           contract.draws[1].scene_occlusion_resource_id, "figure_0_p0.scene_occlusion.depth") ==
       0);
    AT(strcmp(contract.draws[1].scene_occlusion_producer_pass_id, "figure_0_p0.scene_occlusion") ==
       0);
    AT(contract.draws[1].scene_occlusion_bind_set == 2);
    AT(contract.draws[1].scene_occlusion_bind_binding == 0);
    dvz_diagnostic_report_init(&graph_report);
    AT(_scene_pass_contract_validate(&contract, &graph_report));
    AT(dvz_diagnostic_report_count(&graph_report) == 0);

    DvzScenePassContract exact_contract = contract;
    for (uint32_t i = 0; i < exact_contract.attachment_count; i++)
    {
        if (exact_contract.attachments[i].role == DVZ_SCENE_ATTACHMENT_SAMPLED &&
            strcmp(
                exact_contract.attachments[i].resource_id, "figure_0_p0.volume_occlusion.depth") ==
                0)
        {
            dvz_strlcpy(
                exact_contract.attachments[i].resource_id, "figure_0_p1.volume_occlusion.depth",
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
                exact_contract.attachments[i].resource_id, "figure_0_p0.volume_occlusion.depth") ==
                0)
        {
            AT(strcmp(
                   exact_contract.attachments[i].producer_pass_id,
                   "figure_0_p0.volume_occlusion") == 0);
            dvz_strlcpy(
                exact_contract.attachments[i].producer_pass_id, "figure_0_p1.volume_occlusion",
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
            if (entry->binding == 3 && label != NULL &&
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
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
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

    AT(dvz_frame_plan_node_count(plan) == 4);
    const DvzFramePlanNode* opaque_node = dvz_frame_plan_node_get(plan, 0);
    const DvzFramePlanNode* accum_node = dvz_frame_plan_node_get(plan, 1);
    const DvzFramePlanNode* resolve_node = dvz_frame_plan_node_get(plan, 2);
    ANN(opaque_node);
    ANN(accum_node);
    ANN(resolve_node);
    AT(_frame_plan_render_pass_role(opaque_node) == DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE);
    AT(_frame_plan_render_pass_role(accum_node) ==
       DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION);
    AT(_frame_plan_render_pass_role(resolve_node) == DVZ_FRAME_PLAN_RENDER_PASS_WBOIT_RESOLVE);
    AT(opaque_node->u.render.visual_count == 1);
    AT(accum_node->u.render.visual_count == 1);
    AT(resolve_node->u.render.visual_count == 0);
    AT(opaque_node->u.render.visual_metadata[0].alpha_mode == DVZ_ALPHA_OPAQUE);
    AT(accum_node->u.render.visual_metadata[0].alpha_mode == DVZ_ALPHA_WBOIT);
    AT(strcmp(opaque_node->u.render.render_target_id, "rt") == 0);
    AT(strcmp(accum_node->u.render.render_target_id, "rt.wboit_accum") == 0);
    AT(strcmp(resolve_node->u.render.render_target_id, "rt") == 0);
    AT(dvz_frame_plan_graph_resource_count(plan) == 5);
    AT(dvz_frame_plan_graph_pass_count(plan) == 4);

    const DvzFrameGraphResource* accum_resource =
        _test_graph_resource(plan, "figure_0_p0.wboit.accum");
    const DvzFrameGraphResource* weight_resource =
        _test_graph_resource(plan, "figure_0_p0.wboit.transmittance");
    const DvzFrameGraphResource* depth_resource = _test_graph_resource(plan, "figure_0_p0.depth");
    ANN(accum_resource);
    ANN(weight_resource);
    ANN(depth_resource);
    AT(strcmp(accum_resource->id, "figure_0_p0.wboit.accum") == 0);
    AT(strcmp(weight_resource->id, "figure_0_p0.wboit.transmittance") == 0);
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
    AT(_scene_test_graph_pass_provider(plan, opaque_pass) == DVZ_SCENE_WORK_PROVIDER_OPAQUE);
    AT(_scene_test_graph_pass_provider(plan, accum_pass) ==
       DVZ_SCENE_WORK_PROVIDER_WBOIT_ACCUMULATION);
    AT(_scene_test_graph_pass_provider(plan, resolve_pass) ==
       DVZ_SCENE_WORK_PROVIDER_WBOIT_RESOLVE);
    AT(opaque_pass->has_depth_attachment);
    AT(accum_pass->color_attachment_count == 2);
    AT(accum_pass->has_depth_attachment);
    AT(resolve_pass->read_count == 2);
    AT(resolve_pass->color_attachment_count == 1);
    AT(dvz_frame_plan_graph_dependency_count(plan) == 5);
    bool has_accum_resolve_dependency = false;
    bool has_depth_dependency = false;
    for (uint32_t i = 0; i < dvz_frame_plan_graph_dependency_count(plan); i++)
    {
        DvzFrameGraphDependency dep = {0};
        AT(dvz_frame_plan_graph_dependency_get(plan, i, &dep));
        if (strcmp(dep.producer_pass_id, "figure_0_p0.wboit.accum") == 0 &&
            strcmp(dep.consumer_pass_id, "figure_0_p0.wboit.resolve") == 0)
            has_accum_resolve_dependency = true;
        if (strcmp(dep.producer_pass_id, "figure_0_p0.opaque") == 0 &&
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
    AT(opaque_contract.draws[0].blend_policy == DVZ_SCENE_BLEND_POLICY_SEGMENT_COVERAGE);
    AT(opaque_contract.draws[0].blend_target_count == 1);
    AT(opaque_contract.draws[0].blend_targets[0].blend_enabled);
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
    AT(accum_contract.draws[0].blend_targets[0].format == DVZ_FORMAT_R16G16B16A16_SFLOAT);
    AT(accum_contract.draws[0].blend_targets[0].blend_enabled);
    AT(accum_contract.draws[0].blend_targets[0].dst_color_blend_factor == DVZ_BLEND_FACTOR_ONE);
    AT(accum_contract.draws[0].blend_targets[1].format == DVZ_FORMAT_R16_SFLOAT);
    AT(accum_contract.draws[0].blend_targets[1].blend_enabled);
    AT(accum_contract.draws[0].blend_targets[1].color_write_mask == DVZ_MASK_COLOR_R);
    AT(accum_contract.color_attachment_count == 2);
    AT(accum_contract.has_depth_attachment);
    AT(accum_contract.attachments[0].format == DVZ_FORMAT_R16G16B16A16_SFLOAT);
    AT(accum_contract.attachments[1].format == DVZ_FORMAT_R16_SFLOAT);
    dvz_diagnostic_report_init(&report);
    AT(_scene_pass_contract_validate(&accum_contract, &report));
    AT(dvz_diagnostic_report_count(&report) == 0);

    DvzScenePassContract resolve_contract = {0};
    AT(_scene_pass_contract_from_render(
        plan, panel, resolve_node, resolve_pass, &resolve_contract));
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
int test_scene_visual_alpha_mode_wboit_transparent_only_depth(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
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

    AT(dvz_frame_plan_node_count(plan) == 4);
    const DvzFramePlanNode* opaque_node = dvz_frame_plan_node_get(plan, 0);
    const DvzFramePlanNode* accum_node = dvz_frame_plan_node_get(plan, 1);
    const DvzFramePlanNode* resolve_node = dvz_frame_plan_node_get(plan, 2);
    ANN(opaque_node);
    ANN(accum_node);
    ANN(resolve_node);
    AT(_frame_plan_render_pass_role(opaque_node) == DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE);
    AT(_frame_plan_render_pass_role(accum_node) ==
       DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION);
    AT(_frame_plan_render_pass_role(resolve_node) == DVZ_FRAME_PLAN_RENDER_PASS_WBOIT_RESOLVE);
    AT(opaque_node->u.render.visual_count == 0);
    AT(accum_node->u.render.visual_count == 1);

    const DvzFrameGraphPass* opaque_pass = dvz_frame_plan_graph_pass_get(plan, 0);
    const DvzFrameGraphPass* accum_pass = dvz_frame_plan_graph_pass_get(plan, 1);
    ANN(opaque_pass);
    ANN(accum_pass);
    AT(_scene_test_graph_pass_provider(plan, opaque_pass) == DVZ_SCENE_WORK_PROVIDER_OPAQUE);
    AT(_scene_test_graph_pass_provider(plan, accum_pass) ==
       DVZ_SCENE_WORK_PROVIDER_WBOIT_ACCUMULATION);
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
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
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
    DvzColor opaque_colors[3] = {{255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255}};
    DvzColor transparent_colors[3] = {{255, 0, 0, 128}, {0, 255, 0, 128}, {0, 0, 255, 128}};

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

    AT(dvz_frame_plan_node_count(plan) == 4 + DVZ_SCENE_DEPTH_PEEL_ITERATIONS);
    const DvzFramePlanNode* opaque_node = dvz_frame_plan_node_get(plan, 0);
    const DvzFramePlanNode* init_node = dvz_frame_plan_node_get(plan, 1);
    const DvzFramePlanNode* iter_node = dvz_frame_plan_node_get(plan, 2);
    const DvzFramePlanNode* composite_node =
        dvz_frame_plan_node_get(plan, 2 + DVZ_SCENE_DEPTH_PEEL_ITERATIONS);
    ANN(opaque_node);
    ANN(init_node);
    ANN(iter_node);
    ANN(composite_node);
    AT(_frame_plan_render_pass_role(opaque_node) == DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE);
    AT(_frame_plan_render_pass_role(init_node) == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT);
    AT(_frame_plan_render_pass_role(iter_node) == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER);
    AT(_frame_plan_render_pass_role(composite_node) ==
       DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_COMPOSITE);
    AT(opaque_node->u.render.visual_count == 1);
    AT(init_node->u.render.visual_count == 1);
    AT(iter_node->u.render.visual_count == 1);
    AT(composite_node->u.render.visual_count == 0);
    AT(init_node->u.render.visual_metadata[0].alpha_mode == DVZ_ALPHA_DEPTH_PEEL);
    AT(iter_node->u.render.visual_metadata[0].alpha_mode == DVZ_ALPHA_DEPTH_PEEL);

    AT(dvz_frame_plan_graph_pass_count(plan) == 4 + DVZ_SCENE_DEPTH_PEEL_ITERATIONS);
    const DvzFrameGraphResource* depth_resource = _test_graph_resource(plan, "figure_0_p0.depth");
    const DvzRenderProductContract* front_init =
        _test_product_version(plan, DVZ_RENDER_PRODUCT_TRANSPARENT_ACCUMULATION, 1);
    const DvzRenderProductContract* back_init =
        _test_product_version(plan, DVZ_RENDER_PRODUCT_TRANSPARENT_ACCUMULATION, 2);
    const DvzRenderProductContract* front_first =
        _test_product_version(plan, DVZ_RENDER_PRODUCT_TRANSPARENT_ACCUMULATION, 3);
    const DvzRenderProductContract* back_first =
        _test_product_version(plan, DVZ_RENDER_PRODUCT_TRANSPARENT_ACCUMULATION, 4);
    const DvzRenderProductContract* front_final = _test_product_version(
        plan, DVZ_RENDER_PRODUCT_TRANSPARENT_ACCUMULATION,
        2 * DVZ_SCENE_DEPTH_PEEL_ITERATIONS + 1);
    const DvzRenderProductContract* back_final = _test_product_version(
        plan, DVZ_RENDER_PRODUCT_TRANSPARENT_ACCUMULATION,
        2 * DVZ_SCENE_DEPTH_PEEL_ITERATIONS + 2);
    const DvzRenderProductContract* depth_init =
        _test_product_version(plan, DVZ_RENDER_PRODUCT_TRANSPARENT_PEEL_DEPTH, 1);
    const DvzRenderProductContract* depth_first =
        _test_product_version(plan, DVZ_RENDER_PRODUCT_TRANSPARENT_PEEL_DEPTH, 2);
    ANN(depth_resource);
    ANN(front_init);
    ANN(back_init);
    ANN(front_first);
    ANN(back_first);
    ANN(front_final);
    ANN(back_final);
    ANN(depth_init);
    ANN(depth_first);
    AT(strcmp(depth_resource->id, "figure_0_p0.depth") == 0);
    AT(depth_resource->format == DVZ_FORMAT_D32_SFLOAT);
    AT(front_init->alpha == DVZ_RENDER_PRODUCT_ALPHA_PREMULTIPLIED);
    AT(front_init->encoding == DVZ_RENDER_PRODUCT_ENCODING_PREMULTIPLIED_ACCUMULATION);
    AT(back_init->alpha == DVZ_RENDER_PRODUCT_ALPHA_PREMULTIPLIED);
    AT(front_first->source_product_id.value == front_init->id.value);
    AT(back_first->source_product_id.value == back_init->id.value);
    AT(front_first->resource_index == front_init->resource_index);
    AT(back_first->resource_index == back_init->resource_index);
    AT(depth_init->encoding == DVZ_RENDER_PRODUCT_ENCODING_LINEAR_VIEW_DEPTH);
    AT(depth_init->has_background_value);
    AT(depth_init->background_value[0] == -1.0f);
    AT(depth_init->background_value[1] == -1.0f);
    AT(depth_first->source_product_id.value == 0);
    const DvzFrameGraphResource* front_resource =
        dvz_frame_plan_graph_resource_get(plan, front_init->resource_index);
    const DvzFrameGraphResource* back_resource =
        dvz_frame_plan_graph_resource_get(plan, back_init->resource_index);
    const DvzFrameGraphResource* depth_init_resource =
        dvz_frame_plan_graph_resource_get(plan, depth_init->resource_index);
    const DvzFrameGraphResource* depth_first_resource =
        dvz_frame_plan_graph_resource_get(plan, depth_first->resource_index);
    ANN(front_resource);
    ANN(back_resource);
    ANN(depth_init_resource);
    ANN(depth_first_resource);
    AT(front_resource->format == DVZ_FORMAT_R16G16B16A16_SFLOAT);
    AT(back_resource->format == DVZ_FORMAT_R16G16B16A16_SFLOAT);
    AT(depth_init_resource->format == DVZ_FORMAT_R32G32_SFLOAT);
    AT(depth_first_resource->format == DVZ_FORMAT_R32G32_SFLOAT);
    AT(strcmp(depth_init_resource->id, depth_first_resource->id) != 0);

    const DvzFrameGraphPass* init_pass = dvz_frame_plan_graph_pass_get(plan, 1);
    const DvzFrameGraphPass* iter_pass = dvz_frame_plan_graph_pass_get(plan, 2);
    const DvzFrameGraphPass* composite_pass =
        dvz_frame_plan_graph_pass_get(plan, 2 + DVZ_SCENE_DEPTH_PEEL_ITERATIONS);
    ANN(init_pass);
    ANN(iter_pass);
    ANN(composite_pass);
    AT(_scene_test_graph_pass_provider(plan, init_pass) ==
       DVZ_SCENE_WORK_PROVIDER_DEPTH_PEEL_INIT);
    AT(_scene_test_graph_pass_provider(plan, iter_pass) ==
       DVZ_SCENE_WORK_PROVIDER_DEPTH_PEEL_ITERATION);
    AT(_scene_test_graph_pass_provider(plan, composite_pass) ==
       DVZ_SCENE_WORK_PROVIDER_DEPTH_PEEL_COMPOSITE);
    AT(init_pass->color_attachment_count == 3);
    AT(strcmp(init_pass->color_attachments[0].resource_id, front_resource->id) == 0);
    AT(strcmp(init_pass->color_attachments[1].resource_id, back_resource->id) == 0);
    AT(strcmp(init_pass->color_attachments[2].resource_id, depth_init_resource->id) == 0);
    AT(init_pass->color_attachments[0].load_op == DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR);
    AT(init_pass->color_attachments[1].load_op == DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR);
    AT(init_pass->color_attachments[2].load_op == DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR);
    AT(iter_pass->read_count == 1);
    AT(strcmp(iter_pass->reads[0].resource_id, depth_init_resource->id) == 0);
    AT(iter_pass->color_attachment_count == 3);
    AT(strcmp(iter_pass->color_attachments[0].resource_id, front_resource->id) == 0);
    AT(strcmp(iter_pass->color_attachments[1].resource_id, back_resource->id) == 0);
    AT(strcmp(iter_pass->color_attachments[2].resource_id, depth_first_resource->id) == 0);
    AT(iter_pass->color_attachments[0].load_op == DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD);
    AT(iter_pass->color_attachments[1].load_op == DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD);
    AT(iter_pass->color_attachments[2].load_op == DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR);
    AT(composite_pass->read_count == 2);
    AT(strcmp(
           composite_pass->reads[0].resource_id,
           dvz_frame_plan_graph_resource_get(plan, front_final->resource_index)->id) == 0);
    AT(strcmp(
           composite_pass->reads[1].resource_id,
           dvz_frame_plan_graph_resource_get(plan, back_final->resource_index)->id) == 0);
    AT(composite_pass->color_attachment_count == 1);
    AT(composite_pass->color_attachments[0].load_op == DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD);

    const char* peel_init_source =
        _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_DEPTH_PEEL_FRONT, true);
    const char* peel_iter_source =
        _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_DEPTH_PEEL_BACK, true);
    ANN(peel_init_source);
    ANN(peel_iter_source);
    AT(strstr(peel_init_source, "positiveLinearViewDepth(gl_FragCoord.z)") != NULL);
    AT(strstr(peel_iter_source, "positiveLinearViewDepth(gl_FragCoord.z)") != NULL);

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
    AT(init_contract.draws[0].blend_targets[0].format == DVZ_FORMAT_R16G16B16A16_SFLOAT);
    AT(init_contract.draws[0].blend_targets[0].blend_enabled);
    AT(init_contract.draws[0].blend_targets[2].format == DVZ_FORMAT_R32G32_SFLOAT);
    AT(init_contract.draws[0].blend_targets[2].blend_enabled);
    AT(init_contract.draws[0].blend_targets[2].color_blend_op == DVZ_BLEND_OP_MAX);
    AT(init_contract.draws[0].blend_targets[2].color_write_mask ==
       (DVZ_MASK_COLOR_R | DVZ_MASK_COLOR_G));
    AT(init_contract.draws[0].has_raster_state);
    AT(init_contract.draws[0].cull_mode == DVZ_CULL_MODE_NONE);
    AT(init_contract.draws[0].front_face == DVZ_FRONT_FACE_COUNTER_CLOCKWISE);
    AT(init_contract.color_attachment_count == 3);
    AT(init_contract.has_depth_attachment);
    AT(init_contract.attachments[0].format == DVZ_FORMAT_R16G16B16A16_SFLOAT);
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
    AT(iter_contract.draws[0].blend_targets[0].format == DVZ_FORMAT_R16G16B16A16_SFLOAT);
    AT(iter_contract.draws[0].blend_targets[0].blend_enabled);
    AT(iter_contract.draws[0].blend_targets[2].format == DVZ_FORMAT_R32G32_SFLOAT);
    AT(iter_contract.draws[0].blend_targets[2].blend_enabled);
    AT(iter_contract.draws[0].blend_targets[2].color_blend_op == DVZ_BLEND_OP_MAX);
    AT(iter_contract.draws[0].blend_targets[2].color_write_mask ==
       (DVZ_MASK_COLOR_R | DVZ_MASK_COLOR_G));
    AT(iter_contract.draws[0].has_raster_state);
    AT(iter_contract.draws[0].cull_mode == DVZ_CULL_MODE_NONE);
    AT(iter_contract.draws[0].front_face == DVZ_FRONT_FACE_COUNTER_CLOCKWISE);
    AT(iter_contract.color_attachment_count == 3);
    AT(iter_contract.sampled_read_count == 1);
    AT(iter_contract.needs_depth_peel_sampled_layout);
    AT(iter_contract.sampled_texture_binding_count == 1);
    AT(iter_contract.has_depth_attachment);
    AT(iter_contract.attachments[0].format == DVZ_FORMAT_R16G16B16A16_SFLOAT);
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
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
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

    AT(dvz_frame_plan_node_count(plan) == 5 + DVZ_SCENE_DEPTH_PEEL_ITERATIONS);
    AT(dvz_frame_plan_graph_pass_count(plan) == 5 + DVZ_SCENE_DEPTH_PEEL_ITERATIONS);
    const DvzFramePlanNode* overlay_node =
        dvz_frame_plan_node_get(plan, 3 + DVZ_SCENE_DEPTH_PEEL_ITERATIONS);
    const DvzFrameGraphPass* overlay_pass =
        dvz_frame_plan_graph_pass_get(plan, 3 + DVZ_SCENE_DEPTH_PEEL_ITERATIONS);
    ANN(overlay_node);
    ANN(overlay_pass);
    AT(_frame_plan_render_pass_role(overlay_node) == DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND);
    AT(_scene_test_graph_pass_provider(plan, overlay_pass) ==
       DVZ_SCENE_WORK_PROVIDER_TRANSPARENT_BLEND);

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
    DvzPanel* left = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 0.5f, 1.0f});
    DvzPanel* right = dvz_panel(figure, &(DvzPanelDesc){0.5f, 0.0f, 0.5f, 1.0f});
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
    DvzDiagnosticReport emit_report = {0};
    dvz_diagnostic_report_init(&emit_report);
    AT(_scene_emit_panel_render_ex(figure, 0, plan, "figure_0", &emit_report));
    AT(_scene_emit_panel_render_ex(figure, 1, plan, "figure_0", &emit_report));
    if (dvz_diagnostic_report_count(&emit_report) > 0)
        log_error("%s", dvz_diagnostic_report_get(&emit_report, 0));
    AT(dvz_diagnostic_report_count(&emit_report) == 0);

    const DvzFrameGraphPass* right_opaque = NULL;
    const DvzFrameGraphPass* right_presentation = NULL;
    for (uint32_t i = 0; i < dvz_frame_plan_graph_pass_count(plan); i++)
    {
        const DvzFrameGraphPass* pass = dvz_frame_plan_graph_pass_get(plan, i);
        if (pass == NULL || strcmp(pass->panel_id, "figure_0_p1") != 0)
            continue;
        const DvzSceneWorkProviderKey provider = _scene_test_graph_pass_provider(plan, pass);
        if (provider == DVZ_SCENE_WORK_PROVIDER_OPAQUE)
            right_opaque = pass;
        else if (provider == DVZ_SCENE_WORK_PROVIDER_PRESENTATION)
            right_presentation = pass;
    }
    ANN(right_opaque);
    ANN(right_presentation);
    AT(right_opaque->color_attachment_count == 1);
    AT(strstr(right_opaque->color_attachments[0].resource_id, "figure_0_p1.scene.color.v") ==
       right_opaque->color_attachments[0].resource_id);
    AT(right_opaque->color_attachments[0].load_op == DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR);
    AT(right_presentation->color_attachment_count == 1);
    AT(strcmp(right_presentation->color_attachments[0].resource_id, "rt") == 0);
    AT(right_presentation->color_attachments[0].load_op == DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD);

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
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
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
 * Verify noncontiguous WBOIT runs are versioned and preserve authored transparent ordering.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_visual_alpha_mode_noncontiguous_wboit_preserves_order(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);

    vec3 positions[3] = {
        {-0.5f, -0.5f, 0.0f},
        {0.5f, -0.5f, 0.0f},
        {0.0f, 0.5f, 0.0f},
    };
    DvzColor colors[3] = {{255, 0, 0, 128}, {0, 255, 0, 128}, {0, 0, 255, 128}};
    DvzAlphaMode modes[3] = {DVZ_ALPHA_WBOIT, DVZ_ALPHA_BLENDED, DVZ_ALPHA_WBOIT};
    for (uint32_t i = 0; i < 3; i++)
    {
        DvzVisual* visual = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
        ANN(visual);
        AT(dvz_visual_set_data(visual, "position", positions, 3) == 0);
        AT(dvz_visual_set_data(visual, "color", colors, 3) == 0);
        AT(dvz_visual_set_alpha_mode(visual, modes[i]) == 0);
        AT(dvz_panel_add_visual(panel, visual, NULL) == 0);
    }

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
    DvzFramePlan* plan = dvz_frame_plan("figure.alpha.noncontiguous_wboit", 0);
    ANN(plan);
    _scene_emit_panel_render(figure, 0, plan, "figure_0");
    AT(dvz_frame_plan_node_count(plan) == 7);
    const DvzFrameGraphResource* accum_v2 =
        _test_graph_resource(plan, "figure_0_p0.wboit.accum.v2");
    const DvzFrameGraphResource* transmittance_v2 =
        _test_graph_resource(plan, "figure_0_p0.wboit.transmittance.v2");
    ANN(accum_v2);
    ANN(transmittance_v2);
    AT(accum_v2->format == DVZ_FORMAT_R16G16B16A16_SFLOAT);
    AT(transmittance_v2->format == DVZ_FORMAT_R16_SFLOAT);
    ANN(_test_product_version(plan, DVZ_RENDER_PRODUCT_TRANSPARENT_ACCUMULATION, 1));
    ANN(_test_product_version(plan, DVZ_RENDER_PRODUCT_TRANSPARENT_ACCUMULATION, 2));
    ANN(_test_product_version(plan, DVZ_RENDER_PRODUCT_TRANSPARENT_TRANSMITTANCE, 1));
    ANN(_test_product_version(plan, DVZ_RENDER_PRODUCT_TRANSPARENT_TRANSMITTANCE, 2));
    const DvzFramePlanRenderPassRole expected[] = {
        DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE,
        DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION,
        DVZ_FRAME_PLAN_RENDER_PASS_WBOIT_RESOLVE,
        DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND,
        DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION,
        DVZ_FRAME_PLAN_RENDER_PASS_WBOIT_RESOLVE,
        DVZ_FRAME_PLAN_RENDER_PASS_PRESENTATION,
    };
    for (uint32_t i = 0; i < 7; i++)
    {
        const DvzFramePlanNode* node = dvz_frame_plan_node_get(plan, i);
        ANN(node);
        AT(_frame_plan_render_pass_role(node) == expected[i]);
    }

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
 * Verify noncontiguous depth-peel runs keep separate typed product lineages and authored order.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_visual_alpha_mode_noncontiguous_depth_peel_preserves_order(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    vec3 positions[3] = {
        {-0.5f, -0.5f, 0.0f},
        {0.5f, -0.5f, 0.0f},
        {0.0f, 0.5f, 0.0f},
    };
    DvzColor colors[3] = {{255, 0, 0, 128}, {0, 255, 0, 128}, {0, 0, 255, 128}};
    DvzAlphaMode modes[3] = {DVZ_ALPHA_DEPTH_PEEL, DVZ_ALPHA_BLENDED, DVZ_ALPHA_DEPTH_PEEL};
    for (uint32_t i = 0; i < 3; i++)
    {
        DvzVisual* visual = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
        ANN(visual);
        AT(dvz_visual_set_data(visual, "position", positions, 3) == 0);
        AT(dvz_visual_set_data(visual, "color", colors, 3) == 0);
        AT(dvz_visual_set_alpha_mode(visual, modes[i]) == 0);
        AT(dvz_panel_add_visual(panel, visual, NULL) == 0);
    }

    DvzFramePlan* plan = dvz_frame_plan("figure.alpha.noncontiguous_depth_peel", 0);
    ANN(plan);
    DvzDiagnosticReport emit_report = {0};
    dvz_diagnostic_report_init(&emit_report);
    bool emitted = _scene_emit_panel_render_ex(figure, 0, plan, "figure_0", &emit_report);
    if (dvz_diagnostic_report_count(&emit_report) > 0)
        log_error("%s", dvz_diagnostic_report_get(&emit_report, 0));
    AT(emitted);
    AT(dvz_diagnostic_report_count(&emit_report) == 0);
    AT(dvz_frame_plan_node_count(plan) == 15);
    const DvzFramePlanRenderPassRole expected[] = {
        DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE,
        DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT,
        DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER,
        DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER,
        DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER,
        DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER,
        DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_COMPOSITE,
        DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND,
        DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT,
        DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER,
        DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER,
        DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER,
        DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER,
        DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_COMPOSITE,
        DVZ_FRAME_PLAN_RENDER_PASS_PRESENTATION,
    };
    for (uint32_t i = 0; i < 15; i++)
    {
        const DvzFramePlanNode* node = dvz_frame_plan_node_get(plan, i);
        ANN(node);
        AT(_frame_plan_render_pass_role(node) == expected[i]);
    }
    const DvzPanelCompositionSnapshot* composition =
        _frame_plan_composition_get(plan, "figure_0_p0");
    ANN(composition);
    uint32_t forward_depth_count = 0;
    uint32_t peel_forward_depth_count = 0;
    for (uint32_t i = 0; i < composition->scratch_resource_count; i++)
    {
        const DvzSceneScratchResource* scratch = &composition->scratch_resources[i];
        forward_depth_count += scratch->kind == DVZ_SCENE_SCRATCH_FORWARD_DEPTH ? 1 : 0;
        peel_forward_depth_count += scratch->kind == DVZ_SCENE_SCRATCH_PEEL_FORWARD_DEPTH ? 1 : 0;
    }
    AT(forward_depth_count == 1);
    AT(peel_forward_depth_count == 0);
    ANN(_test_product_version(plan, DVZ_RENDER_PRODUCT_TRANSPARENT_ACCUMULATION, 1));
    ANN(_test_product_version(plan, DVZ_RENDER_PRODUCT_TRANSPARENT_ACCUMULATION, 20));
    ANN(_test_product_version(plan, DVZ_RENDER_PRODUCT_TRANSPARENT_PEEL_DEPTH, 1));
    ANN(_test_product_version(plan, DVZ_RENDER_PRODUCT_TRANSPARENT_PEEL_DEPTH, 10));

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
    AT(dvz_drp2_validate_stream(stream).ok);

    _test_scene_stream_destroy(stream);
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
int test_scene_visual_alpha_mode_emits_depth_peel_drp2(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
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
    DvzColor opaque_colors[3] = {{255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255}};
    DvzColor transparent_colors[3] = {{255, 0, 0, 128}, {0, 255, 0, 128}, {0, 0, 255, 128}};

    AT(dvz_visual_set_data(opaque, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(opaque, "color", opaque_colors, 3) == 0);
    AT(dvz_visual_set_data(transparent, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(transparent, "color", transparent_colors, 3) == 0);
    AT(dvz_visual_set_alpha_mode(transparent, DVZ_ALPHA_DEPTH_PEEL) == 0);
    AT(dvz_visual_set_scene_occluder(opaque, true) == 0);
    AT(dvz_visual_set_scene_occluded(transparent, true) == 0);
    AT(dvz_panel_add_visual(panel, opaque, NULL) == 0);
    AT(dvz_panel_add_visual(panel, transparent, NULL) == 0);
    AT(dvz_panel_set_scene_occlusion(
           panel, &(DvzSceneOcclusionDesc){
                      DVZ_STRUCT_INIT_FIELDS(DvzSceneOcclusionDesc),
                      .enabled = true,
                      .depth_bias = 0.0005f,
                      .soft_edge = 0.01f,
                      .hidden_alpha = 0.2f,
                  }) == 0);

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
    bool has_depth_minmax_pong_rg32 = false;
    bool has_depth_texture = false;
    bool has_three_target_pipeline = false;
    bool has_depth_bounds_rg32_target = false;
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
            has_front_accum = has_front_accum ||
                              (label != NULL && strstr(label, "fig0_p0.peel.accum.v") != NULL);
            has_depth_minmax_pong =
                has_depth_minmax_pong ||
                (label != NULL && strstr(label, "fig0_p0.peel.depth.v") != NULL);
            has_depth_minmax_pong_rg32 =
                has_depth_minmax_pong_rg32 ||
                (label != NULL && strstr(label, "fig0_p0.peel.depth.v") != NULL &&
                 command->u.create_texture.format == DVZ_FORMAT_R32G32_SFLOAT);
            has_depth_texture =
                has_depth_texture || (label != NULL && strcmp(label, "fig0_p0.depth") == 0 &&
                                      command->u.create_texture.format == DVZ_FORMAT_D32_SFLOAT);
        }
        else if (command->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            has_three_target_pipeline = has_three_target_pipeline ||
                                        command->u.create_render_pipeline.color_target_count == 3;
            if (command->u.create_render_pipeline.color_target_count == 3)
            {
                const DvzDrp2ColorTarget* front =
                    &command->u.create_render_pipeline.color_targets[0];
                const DvzDrp2ColorTarget* back =
                    &command->u.create_render_pipeline.color_targets[1];
                const DvzDrp2ColorTarget* bounds =
                    &command->u.create_render_pipeline.color_targets[2];
                has_depth_bounds_rg32_target =
                    has_depth_bounds_rg32_target || bounds->format == DVZ_FORMAT_R32G32_SFLOAT;
                has_depth_peel_front_under_blend =
                    has_depth_peel_front_under_blend ||
                    (front->blend_enabled &&
                     front->src_color_blend_factor == DVZ_BLEND_FACTOR_ONE_MINUS_DST_ALPHA &&
                     front->dst_color_blend_factor == DVZ_BLEND_FACTOR_ONE);
                has_depth_peel_back_over_blend =
                    has_depth_peel_back_over_blend ||
                    (back->blend_enabled && back->src_color_blend_factor == DVZ_BLEND_FACTOR_ONE &&
                     back->dst_color_blend_factor == DVZ_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
                has_depth_bounds_max_blend =
                    has_depth_bounds_max_blend ||
                    (bounds->blend_enabled && bounds->color_blend_op == DVZ_BLEND_OP_MAX &&
                     bounds->alpha_blend_op == DVZ_BLEND_OP_MAX &&
                     bounds->color_write_mask == (DVZ_MASK_COLOR_R | DVZ_MASK_COLOR_G));
                has_depth_peel_no_cull =
                    has_depth_peel_no_cull ||
                    (command->u.create_render_pipeline.has_raster_state &&
                     command->u.create_render_pipeline.cull_mode == DVZ_CULL_MODE_NONE);
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
                     DVZ_BLEND_FACTOR_SRC_ALPHA);
        }
        else if (command->type == DVZ_DRP2_COMMAND_CREATE_BIND_GROUP)
        {
            const char* label = dvz_drp2_stream_label(stream, command->u.create_bind_group.id);
            if (label != NULL && strstr(label, "_bg_depth_peel_iter_") != NULL)
                AT(command->u.create_bind_group.entry_count == 2);
            if (command->u.create_bind_group.entry_count == 2 ||
                command->u.create_bind_group.entry_count == 3)
                sampled_bind_group_count++;
            has_composite_bind_group =
                has_composite_bind_group || command->u.create_bind_group.entry_count == 3;
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
    AT(has_depth_minmax_pong_rg32);
    AT(has_depth_texture);
    AT(has_three_target_pipeline);
    AT(has_depth_bounds_rg32_target);
    AT(has_depth_bounds_max_blend);
    AT(has_depth_peel_front_under_blend);
    AT(has_depth_peel_back_over_blend);
    AT(has_depth_peel_no_cull);
    AT(has_composite_pipeline);
    AT(has_blended_composite_pipeline);
    AT(has_composite_bind_group);
    AT(sampled_bind_group_count >= 1);
    AT(begin_pass_count == 5 + DVZ_SCENE_DEPTH_PEEL_ITERATIONS);
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
                (label != NULL && strstr(label, "fig0_p0.peel.accum.v") != NULL &&
                 strstr(label, "_scope_000000000000007b") != NULL);
        }
        else if (command->type == DVZ_DRP2_COMMAND_CREATE_BIND_GROUP)
        {
            const char* label =
                dvz_drp2_stream_label(scoped_stream, command->u.create_bind_group.id);
            has_scoped_composite_bind_group =
                has_scoped_composite_bind_group ||
                (label != NULL && strstr(label, "_bg_depth_peel_composite_") != NULL &&
                 strstr(label, "_scope_000000000000007b") != NULL);
        }
    }
    AT(has_scoped_front_accum);
    AT(has_scoped_composite_bind_group);

    dvz_diagnostic_report_init(&report);
    AT(dvz_figure_resize(figure, 96, 48) == DVZ_OK);
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
                (label != NULL && strstr(label, "fig0_p0.peel.accum.v") != NULL &&
                 strstr(label, "_scope_000000000000007b") != NULL &&
                 command->u.create_texture.width == 96 && command->u.create_texture.height == 48);
        }
        else if (command->type == DVZ_DRP2_COMMAND_CREATE_BIND_GROUP)
        {
            const char* label =
                dvz_drp2_stream_label(scoped_resize_stream, command->u.create_bind_group.id);
            rebuilt_composite_bind_group =
                rebuilt_composite_bind_group ||
                (label != NULL && strstr(label, "_bg_depth_peel_composite_") != NULL &&
                 strstr(label, "_scope_000000000000007b") != NULL &&
                 command->u.create_bind_group.id != 0);
        }
    }
    AT(resized_front_accum);
    AT(rebuilt_composite_bind_group);

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
int test_scene_visual_alpha_mode_requires_wboit_capabilities(
    TstContext* suite, const TstCase* item)
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
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
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
    DvzColor opaque_colors[3] = {{255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255}};
    DvzColor transparent_colors[3] = {{255, 0, 0, 128}, {0, 255, 0, 128}, {0, 0, 255, 128}};

    AT(dvz_visual_set_data(opaque, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(opaque, "color", opaque_colors, 3) == 0);
    AT(dvz_visual_set_data(transparent, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(transparent, "color", transparent_colors, 3) == 0);
    AT(dvz_visual_set_alpha_mode(transparent, DVZ_ALPHA_WBOIT) == 0);
    AT(dvz_panel_add_visual(panel, opaque, NULL) == 0);
    AT(dvz_panel_add_visual(panel, transparent, NULL) == 0);
    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.05f, 0.05f, 0.08f, 1.0f));

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
    uint64_t begin_pass_textures[4] = {0};
    uint32_t begin_pass_color_counts[4] = {0};
    bool begin_pass_clears[4] = {0};
    bool begin_pass_depths[4] = {0};
    uint64_t begin_pass_depth_textures[4] = {0};
    DvzDrp2AttachmentLoadOp begin_pass_depth_loads[4] = {0};
    DvzDrp2AttachmentAccess begin_pass_depth_access[4] = {0};
    uint64_t begin_pass_second_attachment_textures[4] = {0};
    bool begin_pass_second_attachment_clears[4] = {0};

    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        ANN(command);
        if (command->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE)
        {
            has_accum_texture = has_accum_texture ||
                                command->u.create_texture.format == DVZ_FORMAT_R16G16B16A16_SFLOAT;
            has_weight_texture =
                has_weight_texture || command->u.create_texture.format == DVZ_FORMAT_R16_SFLOAT;
            const char* label = dvz_drp2_stream_label(stream, command->u.create_texture.id);
            has_named_depth_texture = has_named_depth_texture ||
                                      (label != NULL && strcmp(label, "fig0_p0.depth") == 0 &&
                                       command->u.create_texture.format == DVZ_FORMAT_D32_SFLOAT);
            has_graph_accum_texture = has_graph_accum_texture ||
                                      (label != NULL && strcmp(label, "fig0_p0.wboit.accum") == 0);
            if (label != NULL && strcmp(label, "fig0_p0.wboit.accum") == 0)
                graph_accum_texture_id = command->u.create_texture.id;
            has_graph_weight_texture =
                has_graph_weight_texture ||
                (label != NULL && strcmp(label, "fig0_p0.wboit.transmittance") == 0);
            if (label != NULL && strcmp(label, "fig0_p0.wboit.transmittance") == 0)
                graph_weight_texture_id = command->u.create_texture.id;
            has_graph_accum_usage =
                has_graph_accum_usage ||
                (label != NULL && strcmp(label, "fig0_p0.wboit.accum") == 0 &&
                 (command->u.create_texture.usage & (DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT |
                                                     DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING)) ==
                     (DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT |
                      DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING));
            has_graph_weight_usage =
                has_graph_weight_usage ||
                (label != NULL && strcmp(label, "fig0_p0.wboit.transmittance") == 0 &&
                 (command->u.create_texture.usage & (DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT |
                                                     DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING)) ==
                     (DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT |
                      DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING));
            has_graph_depth_usage =
                has_graph_depth_usage || (label != NULL && strcmp(label, "fig0_p0.depth") == 0 &&
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
                     DVZ_BLEND_FACTOR_SRC_ALPHA);
            has_opaque_depth_pipeline =
                has_opaque_depth_pipeline ||
                (command->u.create_render_pipeline.has_depth_attachment &&
                 command->u.create_render_pipeline.depth_write_enabled &&
                 command->u.create_render_pipeline.depth_compare_op ==
                     DVZ_COMPARE_OP_LESS_OR_EQUAL &&
                 command->u.create_render_pipeline.vertex_buffer_slots == 2 &&
                 command->u.create_render_pipeline.topology ==
                     DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
            has_fixed_background_depth_pipeline =
                has_fixed_background_depth_pipeline ||
                (command->u.create_render_pipeline.has_depth_attachment &&
                 !command->u.create_render_pipeline.depth_write_enabled &&
                 command->u.create_render_pipeline.depth_compare_op == DVZ_COMPARE_OP_ALWAYS &&
                 command->u.create_render_pipeline.vertex_buffer_slots == 2 &&
                 command->u.create_render_pipeline.topology ==
                     DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
        }
        else if (command->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS)
        {
            if (begin_pass_count < 4)
            {
                begin_pass_textures[begin_pass_count] = command->u.begin_render_pass.texture_id;
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
    AT(begin_pass_count == 4);
    AT(begin_pass_color_counts[0] == 1);
    AT(begin_pass_color_counts[1] == 2);
    AT(begin_pass_color_counts[2] == 1);
    AT(begin_pass_color_counts[3] == 1);
    AT(begin_pass_textures[1] == graph_accum_texture_id);
    AT(begin_pass_second_attachment_textures[1] == graph_weight_texture_id);
    AT(begin_pass_textures[1] != begin_pass_textures[0]);
    AT(begin_pass_textures[1] != begin_pass_textures[2]);
    AT(begin_pass_clears[0]);
    AT(begin_pass_clears[1]);
    AT(begin_pass_depths[0]);
    AT(begin_pass_depths[1]);
    AT(!begin_pass_depths[2]);
    AT(!begin_pass_depths[3]);
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
    AT(begin_pass_textures[3] != begin_pass_textures[2]);
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
        if (label != NULL && strcmp(label, "fig0_p0.wboit.accum_scope_000000000000007c") == 0)
            scoped_accum_texture_id = command->u.create_texture.id;
        else if (
            label != NULL &&
            strcmp(label, "fig0_p0.wboit.transmittance_scope_000000000000007c") == 0)
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
    AT(dvz_figure_resize(figure, 96, 48) == DVZ_OK);
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
        if (label != NULL && strcmp(label, "fig0_p0.wboit.accum_scope_000000000000007c") == 0 &&
            command->u.create_texture.width == 96 && command->u.create_texture.height == 48)
            resized_accum_texture_id = command->u.create_texture.id;
        else if (
            label != NULL &&
            strcmp(label, "fig0_p0.wboit.transmittance_scope_000000000000007c") == 0 &&
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
    AT(rebuilt_resolve_bind_group);

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
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
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
                AT(command->u.create_shader_module.builtin_version ==
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
                    AT(command->u.create_render_pipeline.builtin_version ==
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
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
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
    DvzColor opaque_colors[3] = {{255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255}};
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
        DVZ_BLEND_FACTOR_ZERO;
    dvz_diagnostic_report_init(&report);
    AT(!_scene_frame_plan_drp2_contracts_validate(plan, stream, &report));
    AT(dvz_diagnostic_report_count(&report) > 0);

    wboit_pipeline->u.create_render_pipeline = original_pipeline_command.u.create_render_pipeline;
    wboit_pipeline->u.create_render_pipeline.color_targets[1].color_write_mask =
        DVZ_MASK_COLOR_R | DVZ_MASK_COLOR_G;
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
        DVZ_BLEND_FACTOR_ZERO;
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
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
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
    DvzColor opaque_colors[3] = {{255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255}};
    DvzColor transparent_colors[3] = {{255, 0, 0, 128}, {0, 255, 0, 128}, {0, 0, 255, 128}};
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
    caps.supports_color_blending = true;

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
        if (command->type != DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE ||
            command->u.create_render_pipeline.color_target_count != 3 ||
            !command->u.create_render_pipeline.has_raster_state)
            continue;
        if (command->u.create_render_pipeline.cull_mode == DVZ_CULL_MODE_NONE)
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
    init_pipeline->u.create_render_pipeline.cull_mode = DVZ_CULL_MODE_BACK;
    dvz_diagnostic_report_init(&report);
    AT(!_scene_frame_plan_drp2_contracts_validate(plan, stream, &report));
    AT(dvz_diagnostic_report_count(&report) > 0);

    init_pipeline->u.create_render_pipeline = original_init_pipeline.u.create_render_pipeline;
    const DvzDrp2Command original_iter_pipeline = *iter_pipeline;
    iter_pipeline->u.create_render_pipeline.front_face = DVZ_FRONT_FACE_CLOCKWISE;
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
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
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
    DvzColor opaque_colors[3] = {{255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255}};
    DvzColor transparent_colors[3] = {{255, 0, 0, 128}, {0, 255, 0, 128}, {0, 0, 255, 128}};

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
            source_over0_has_wboit || (command->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS &&
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
            wboit_has_accum_pass || (command->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS &&
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
            source_over1_has_wboit || (command->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS &&
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

    DvzGpuCtxConfig gpu_cfg = dvz_testing_gpu_ctx_config(suite);
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
           transparent, (float[3]){0.0f, 0.0f, 1.0f}, 0.25f, 0.75f, 0.25f, 32.0f) == 0);
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
            final_target_id = command->u.begin_render_pass.texture_id;
        }
    }
    AT(begin_render_pass_count == 4);
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
        readback, encoder_id, final_target_id, readback_buffer_id, 0, width, height, width * 4,
        height));
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

    DvzGpuCtxConfig gpu_cfg = dvz_testing_gpu_ctx_config(suite);
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

    DvzVisual* transparent = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    AT(transparent != NULL);
    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.05f, 0.05f, 0.08f, 1.0f));
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
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        ANN(command);
        if (command->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS)
            final_target_id = command->u.begin_render_pass.texture_id;
    }
    AT(final_target_id != 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    if (!result.ok)
    {
        const DvzDrp2Command* failed = dvz_drp2_stream_get(stream, result.command_index);
        log_error(
            "depth peel GLSL stream failed: code=%d command=%" PRIu32 " type=%d", result.code,
            result.command_index, failed != NULL ? (int)failed->type : -1);
    }
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
        readback, encoder_id, final_target_id, readback_buffer_id, 0, width, height, width * 4,
        height));
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
