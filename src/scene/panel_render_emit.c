/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene panel render emission                                                                  */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#include <vulkan/vulkan_core.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_overflow.h"
#include "_scene.h"
#include "_scene_emit.h"
#include "_scene_emit_internal.h"
#include "_scene_resource_key.h"
#include "_scene_shader_abi.h"
#include "_technique.h"
#include "_visual_pipeline.h"
#include "datoviz/drp2/runtime.h"
#include "render_contract.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Report a panel graph-emission failure to logs and the optional diagnostic report.
 *
 * @param report optional diagnostic report
 * @param fmt printf-style diagnostic format
 */
static void _scene_emit_graph_report(DvzDiagnosticReport* report, const char* fmt, ...)
{
    ANN(fmt);

    char message[DVZ_SCENE_DIAGNOSTIC_SIZE] = {0};
    va_list args;
    va_start(args, fmt);
    int written = dvz_vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);
    if (written < 0)
        return;

    log_error("%s", message);
    if (report != NULL)
        (void)dvz_diagnostic_report_add(report, message);
}



/**
 * Configure common panel transform metadata on a render node.
 *
 * @param node the render node
 * @param panel_apply_mvp the panel APPLY MVP
 * @param panel_viewport the panel pixel viewport
 * @param plot_desc the normalized plot rectangle
 */
static void _scene_configure_panel_render_node(
    DvzFramePlanNode* node, const DvzMVP* panel_apply_mvp,
    const DvzSceneViewportUniform* panel_viewport, DvzPanelDesc plot_desc)
{
    ANN(node);
    ANN(panel_apply_mvp);
    ANN(panel_viewport);
    node->u.render.has_mvp = true;
    node->u.render.apply_mvp = *panel_apply_mvp;
    node->u.render.has_plot_desc = true;
    node->u.render.plot_desc = plot_desc;
    node->u.render.has_viewport = true;
    node->u.render.viewport = *panel_viewport;
}



/**
 * Return a mutable FramePlan node by index.
 *
 * @param plan the destination frame plan
 * @param node_index the node index
 * @return the mutable node, or NULL when the index is invalid
 */
static DvzFramePlanNode* _scene_frame_plan_node_mut(DvzFramePlan* plan, uint32_t node_index)
{
    ANN(plan);
    if (node_index >= plan->count)
        return NULL;
    return &plan->nodes[node_index];
}



/**
 * Build a stable pass-contract id for one render node.
 *
 * @param panel_id the panel id
 * @param pass_role the render-pass role
 * @param out output contract id
 * @param out_size output buffer size
 * @return whether the id was written without truncation
 */
static bool _scene_pass_contract_id(
    const char* panel_id, DvzFramePlanRenderPassRole pass_role, char* out, size_t out_size)
{
    ANN(panel_id);
    ANN(out);
    int ret = dvz_snprintf(out, out_size, "%s.pass.%u", panel_id, (uint32_t)pass_role);
    return ret >= 0 && (size_t)ret < out_size;
}



/**
 * Build a stable draw-contract id for one visual in one render pass.
 *
 * @param pass_contract_id the owning pass-contract id
 * @param visual_index the visual index within the figure
 * @param out output contract id
 * @param out_size output buffer size
 * @return whether the id was written without truncation
 */
static bool _scene_draw_contract_id(
    const char* pass_contract_id, uint32_t visual_index, char* out, size_t out_size)
{
    ANN(pass_contract_id);
    ANN(out);
    int ret = dvz_snprintf(out, out_size, "%s.draw.%u", pass_contract_id, visual_index);
    return ret >= 0 && (size_t)ret < out_size;
}



/**
 * Append a panel render pass with common panel transform metadata.
 *
 * @param plan the destination frame plan
 * @param panel_id the panel id
 * @param render_target_id the render target id
 * @param desc the normalized panel rectangle
 * @param pass_role the render pass role
 * @param panel_apply_mvp the panel APPLY MVP
 * @param panel_viewport the panel pixel viewport
 * @param plot_desc the normalized plot rectangle
 * @param out_index output node index
 * @return whether the render node was appended
 */
static bool _scene_begin_panel_render_pass(
    DvzFramePlan* plan, const char* panel_id, const char* render_target_id, DvzPanelDesc desc,
    DvzFramePlanRenderPassRole pass_role, const DvzMVP* panel_apply_mvp,
    const DvzSceneViewportUniform* panel_viewport, DvzPanelDesc plot_desc, uint32_t* out_index)
{
    ANN(plan);
    ANN(panel_id);
    ANN(render_target_id);
    ANN(out_index);
    uint32_t node_index = plan->count;
    if (!dvz_frame_plan_render_panel_role(
            plan, panel_id, render_target_id, false, desc, pass_role))
        return false;
    DvzFramePlanNode* node = _scene_frame_plan_node_mut(plan, node_index);
    if (node != NULL)
    {
        _scene_configure_panel_render_node(node, panel_apply_mvp, panel_viewport, plot_desc);
        if (!_scene_pass_contract_id(
                panel_id, pass_role, node->u.render.pass_contract_id,
                sizeof(node->u.render.pass_contract_id)))
            return false;
        node->u.render.has_pass_contract = true;
    }
    *out_index = node_index;
    return node != NULL;
}


/**
 * Return whether a graph pass should sample the panel volume-occlusion texture.
 *
 * @param pass the graph pass
 * @return whether the pass renders ordinary visual fragments
 */
static bool _scene_graph_pass_can_sample_visual_occlusion(const DvzFrameGraphPass* pass)
{
    ANN(pass);
    return strcmp(pass->work_label, "opaque") == 0 ||
           strcmp(pass->work_label, "transparent_blend") == 0 ||
           strcmp(pass->work_label, "wboit_accum") == 0 ||
           strcmp(pass->work_label, "depth_peel_init") == 0 ||
           strcmp(pass->work_label, "depth_peel_iter") == 0;
}


/**
 * Add sampled volume-occlusion reads to panel visual render passes.
 *
 * @param plan the frame plan
 * @param panel_id the panel id
 * @return whether all required reads were added
 */
static bool _scene_add_volume_occlusion_reads(DvzFramePlan* plan, const char* panel_id)
{
    ANN(plan);
    ANN(panel_id);
    char depth_id[DVZ_SCENE_LABEL_SIZE];
    if (!_scene_resource_key_panel_graph(
            panel_id, "volume_occlusion.depth", depth_id, sizeof(depth_id)))
        return false;

    for (uint32_t i = 0; i < plan->graph_pass_count; i++)
    {
        DvzFrameGraphPass* pass = &plan->graph_passes[i];
        if (strcmp(pass->panel_id, panel_id) != 0 ||
            !_scene_graph_pass_can_sample_visual_occlusion(pass))
            continue;
        bool already = false;
        for (uint32_t j = 0; j < pass->read_count; j++)
            already = already || strcmp(pass->reads[j].resource_id, depth_id) == 0;
        if (!already && !dvz_frame_graph_pass_read(pass, depth_id, DVZ_FRAME_GRAPH_ACCESS_SAMPLED))
            return false;
    }
    return true;
}


/**
 * Add sampled scene-occlusion reads to panel visual render passes.
 *
 * @param plan the frame plan
 * @param panel_id the panel id
 * @return whether all required reads were added
 */
static bool _scene_add_scene_occlusion_reads(DvzFramePlan* plan, const char* panel_id)
{
    ANN(plan);
    ANN(panel_id);
    char depth_id[DVZ_SCENE_LABEL_SIZE];
    if (!_scene_resource_key_panel_graph(
            panel_id, "scene_occlusion.depth", depth_id, sizeof(depth_id)))
        return false;

    for (uint32_t i = 0; i < plan->graph_pass_count; i++)
    {
        DvzFrameGraphPass* pass = &plan->graph_passes[i];
        if (strcmp(pass->panel_id, panel_id) != 0 ||
            !_scene_graph_pass_can_sample_visual_occlusion(pass))
            continue;
        bool already = false;
        for (uint32_t j = 0; j < pass->read_count; j++)
            already = already || strcmp(pass->reads[j].resource_id, depth_id) == 0;
        if (!already && !dvz_frame_graph_pass_read(pass, depth_id, DVZ_FRAME_GRAPH_ACCESS_SAMPLED))
            return false;
    }
    return true;
}


/**
 * Return whether the panel has a visible target for volume occlusion.
 *
 * @param panel the panel
 * @return whether a volume occlusion prepass should be emitted
 */
static bool _scene_panel_has_visible_volume_occlusion_target(const DvzPanel* panel)
{
    ANN(panel);
    if (!panel->volume_occlusion_enabled || panel->volume_occluder_visual == NULL ||
        !panel->volume_occlusion.enabled || !panel->volume_occluder_visual->visible)
        return false;

    for (uint32_t i = 0; i < panel->visual_count; i++)
    {
        const DvzVisual* visual = panel->visuals[i].visual;
        if (visual == NULL || !visual->visible || visual->type == DVZ_VISUAL_TYPE_TEXT ||
            !visual->volume_occluded || visual == panel->volume_occluder_visual)
            continue;
        int pos_idx = _attr_index(
            visual, visual->type == DVZ_VISUAL_TYPE_SEGMENT ? "position_start" : "position");
        if (pos_idx >= 0 && visual->attrs[pos_idx].item_count > 0)
            return true;
    }
    return false;
}


/**
 * Return whether one panel visual is visible and drawable.
 *
 * @param visual the visual
 * @return whether the visual has position data
 */
static bool _scene_visual_is_visible_drawable(const DvzVisual* visual)
{
    if (visual == NULL || !visual->visible)
        return false;
    if (visual->type == DVZ_VISUAL_TYPE_TEXT)
        return false;
    int pos_idx = _attr_index(
        visual, visual->type == DVZ_VISUAL_TYPE_SEGMENT ? "position_start" : "position");
    return pos_idx >= 0 && visual->attrs[pos_idx].item_count > 0;
}


/**
 * Return whether a visual is owned by a panel colorbar adornment.
 *
 * @param panel the panel owning colorbar handles
 * @param visual the visual to classify
 * @return whether the visual is colorbar-derived
 */
static bool _scene_visual_is_colorbar_derived(const DvzPanel* panel, const DvzVisual* visual)
{
    ANN(panel);
    ANN(visual);
    for (uint32_t i = 0; i < panel->colorbar_count; i++)
    {
        const DvzColorbar* colorbar = panel->colorbars[i];
        if (colorbar == NULL)
            continue;
        if (visual == colorbar->ramp_visual || visual == colorbar->tick_visual ||
            visual == colorbar->text_visual)
        {
            return true;
        }
        if (colorbar->text_visual != NULL &&
            visual == colorbar->text_visual->text.glyph_visual)
        {
            return true;
        }
    }
    return false;
}


/**
 * Return whether a visual is owned by a panel legend adornment.
 *
 * @param panel the panel owning legend handles
 * @param visual the visual to classify
 * @return whether the visual is legend-derived
 */
static bool _scene_visual_is_legend_derived(const DvzPanel* panel, const DvzVisual* visual)
{
    ANN(panel);
    ANN(visual);
    for (uint32_t i = 0; i < panel->legend_count; i++)
    {
        const DvzLegend* legend = panel->legends[i];
        if (legend == NULL)
            continue;
        if (visual == legend->mark_visual || visual == legend->text_visual)
            return true;
        if (legend->text_visual != NULL && visual == legend->text_visual->text.glyph_visual)
            return true;
    }
    return false;
}



/**
 * Return the clip rectangle one visual should use within its panel render pass.
 *
 * @param panel the panel owning the visual attachment
 * @param visual the visual
 * @return the visual clip rectangle kind
 */
static DvzFramePlanClipRect _scene_visual_clip_rect(const DvzPanel* panel, const DvzVisual* visual)
{
    ANN(panel);
    ANN(visual);
    if (visual == panel->background_visual || visual->type == DVZ_VISUAL_TYPE_GLYPH ||
        _scene_visual_is_colorbar_derived(panel, visual) ||
        _scene_visual_is_legend_derived(panel, visual))
    {
        return DVZ_FRAME_PLAN_CLIP_RECT_PANEL;
    }
    return DVZ_FRAME_PLAN_CLIP_RECT_PLOT;
}



/**
 * Return whether the panel has visible scene occluder and occluded targets.
 *
 * @param panel the panel
 * @return whether a scene occlusion prepass should be emitted
 */
static bool _scene_panel_has_visible_scene_occlusion_target(const DvzPanel* panel)
{
    ANN(panel);
    if (!panel->scene_occlusion_enabled || !panel->scene_occlusion.enabled)
        return false;

    bool has_occluder = false;
    bool has_occluded = false;
    for (uint32_t i = 0; i < panel->visual_count; i++)
    {
        const DvzVisual* visual = panel->visuals[i].visual;
        if (!_scene_visual_is_visible_drawable(visual))
            continue;
        has_occluder = has_occluder || visual->scene_occluder;
        has_occluded = has_occluded || visual->scene_occluded;
    }
    return has_occluder && has_occluded;
}


/**
 * Return whether a draw contract requires any depth resource.
 *
 * @param contract the resolved draw contract
 * @return whether the draw uses fixed-function or sampled depth
 */
static bool _scene_draw_contract_needs_depth(const DvzSceneDrawContract* contract)
{
    ANN(contract);
    return (contract->depth_policy & (DVZ_SCENE_DEPTH_POLICY_TEST | DVZ_SCENE_DEPTH_POLICY_WRITE |
                                      DVZ_SCENE_DEPTH_POLICY_SAMPLE)) != 0;
}



/**
 * Resolve whether a transparent draw contract requires a pass depth attachment.
 *
 * @param visual the retained visual
 * @param attach the panel attachment
 * @param pass_role the transparent render-pass role
 * @param out whether the draw requires depth
 * @return whether the draw contract was resolved
 */
static bool _scene_transparent_contract_needs_depth(
    const DvzVisual* visual, const DvzPanelAttach* attach, DvzFramePlanRenderPassRole pass_role,
    bool* out)
{
    ANN(out);
    *out = false;
    DvzSceneDrawContract contract = {0};
    if (!_scene_draw_contract_from_visual(visual, attach, pass_role, &contract))
        return false;
    *out = _scene_draw_contract_needs_depth(&contract);
    return true;
}



/**
 * Append one visual to the active render pass.
 *
 * @param figure the parent figure
 * @param plan the destination frame plan
 * @param node the active render node
 * @param panel the panel owning the visual attachment
 * @param visual the visual
 * @param attach the panel attachment
 * @param visual_index the visual index within the figure
 * @return whether the visual was appended
 */
static bool _scene_append_visual_to_render_pass(
    const DvzFigure* figure, DvzFramePlan* plan, DvzFramePlanNode* node, const DvzPanel* panel,
    const DvzVisual* visual, const DvzPanelAttach* attach, uint32_t visual_index,
    const DvzSceneOcclusionDesc* scene_occlusion, const DvzVolumeOcclusionDesc* volume_occlusion)
{
    ANN(figure);
    ANN(plan);
    ANN(node);
    ANN(panel);
    ANN(visual);
    ANN(attach);

    char visual_id[64];
    uint32_t buffer_idx = _scene_buffer_index(figure->scene, visual->buffer);
    if (buffer_idx != UINT32_MAX)
    {
        if (!_scene_visual_indexed_resource_key(
                figure, visual, visual_index, buffer_idx, visual_id, sizeof(visual_id)))
            return false;
    }
    else
    {
        if (!_scene_visual_resource_key(figure, visual, visual_index, visual_id, sizeof(visual_id)))
            return false;
    }
    (void)plan;
    if (node->u.render.visual_count >= DVZ_SCENE_MAX_RENDER_VISUALS)
        return false;

    DvzFramePlanVisualMeta metadata = {0};
    bool has_metadata = _scene_visual_frame_plan_metadata(figure, visual, visual_index, &metadata);
    if (has_metadata)
    {
        metadata.clip_rect = _scene_visual_clip_rect(panel, visual);
        if (metadata.has_volume && volume_occlusion == NULL)
        {
            metadata.volume_occluded = false;
            metadata.has_volume_occlusion = false;
        }
        if (metadata.has_volume && volume_occlusion != NULL)
        {
            metadata.has_volume_occlusion = true;
            metadata.volume_occlusion = *volume_occlusion;
        }
        if (scene_occlusion == NULL)
            metadata.scene_occluded = false;
        if (metadata.scene_occluded && scene_occlusion != NULL)
        {
            metadata.has_scene_occlusion = true;
            metadata.scene_occlusion = *scene_occlusion;
        }
        DvzSceneDrawContract draw_contract = {0};
        if (!_scene_draw_contract_from_visual(
                visual, attach, node->u.render.pass_role, &draw_contract))
            return false;
        if (scene_occlusion == NULL)
        {
            draw_contract.samples_scene_occlusion = false;
            draw_contract.needs_scene_occlusion_set = false;
            draw_contract.shader_feature_mask &=
                ~((uint32_t)DVZ_SCENE_SHADER_FEATURE_SAMPLE_SCENE_OCCLUSION);
            draw_contract.bind_group_layout_mask &=
                ~((uint32_t)DVZ_SCENE_BIND_GROUP_REQUIREMENT_SCENE_OCCLUSION);
        }
        if (volume_occlusion == NULL)
        {
            draw_contract.samples_volume_occlusion = false;
            draw_contract.shader_feature_mask &=
                ~((uint32_t)DVZ_SCENE_SHADER_FEATURE_SAMPLE_VOLUME_OCCLUSION);
        }
        if (!_scene_draw_contract_id(
                node->u.render.pass_contract_id, visual_index, metadata.draw_contract_id,
                sizeof(metadata.draw_contract_id)))
            return false;
        metadata.has_draw_contract = true;
        metadata.draw_depth_policy = draw_contract.depth_policy;
        metadata.draw_blend_policy = (uint32_t)draw_contract.blend_policy;
        metadata.draw_shader_feature_mask = draw_contract.shader_feature_mask;
        metadata.draw_bind_group_layout_mask = draw_contract.bind_group_layout_mask;
        if (draw_contract.samples_volume_occlusion)
        {
            if (!_scene_resource_key_panel_graph(
                    node->u.render.panel_id, "volume_occlusion.depth",
                    metadata.draw_volume_occlusion_resource_id,
                    sizeof(metadata.draw_volume_occlusion_resource_id)))
                return false;
            if (!_scene_resource_key_panel_graph(
                    node->u.render.panel_id, "volume_occlusion",
                    metadata.draw_volume_occlusion_producer_pass_id,
                    sizeof(metadata.draw_volume_occlusion_producer_pass_id)))
                return false;
            metadata.draw_volume_occlusion_bind_set = DVZ_SCENE_SHADER_SET_VISUAL;
            metadata.draw_volume_occlusion_bind_binding = 3;
        }
        if (draw_contract.samples_scene_occlusion)
        {
            if (!_scene_resource_key_panel_graph(
                    node->u.render.panel_id, "scene_occlusion.depth",
                    metadata.draw_scene_occlusion_resource_id,
                    sizeof(metadata.draw_scene_occlusion_resource_id)))
                return false;
            if (!_scene_resource_key_panel_graph(
                    node->u.render.panel_id, "scene_occlusion",
                    metadata.draw_scene_occlusion_producer_pass_id,
                    sizeof(metadata.draw_scene_occlusion_producer_pass_id)))
                return false;
            bool scene_occlusion_uses_set2 =
                draw_contract.needs_image_set || draw_contract.needs_labels_set ||
                draw_contract.needs_glyph_set || draw_contract.needs_volume_set ||
                draw_contract.needs_material_set;
            metadata.draw_scene_occlusion_bind_set = scene_occlusion_uses_set2
                                                         ? DVZ_SCENE_SHADER_SET_SCENE_OCCLUSION
                                                         : DVZ_SCENE_SHADER_SET_VISUAL;
            metadata.draw_scene_occlusion_bind_binding = 0;
        }
    }

    uint32_t slot = node->u.render.visual_count++;
    dvz_strlcpy(node->u.render.visuals[slot], visual_id, sizeof(node->u.render.visuals[slot]));
    if (has_metadata)
    {
        dvz_memcpy(
            &node->u.render.visual_metadata[slot], sizeof(DvzFramePlanVisualMeta), &metadata,
            sizeof(DvzFramePlanVisualMeta));
        node->u.render.visual_metadata[slot].has_metadata = true;
    }
    node->u.render.controller_modes[slot] = attach->controller_mode;
    return true;
}



/**
 * Emit one panel render node into a frame plan.
 *
 * @param figure the parent figure
 * @param panel_index the panel index within the figure
 * @param plan the destination frame plan
 * @param figure_id the stable figure identifier
 * @param report optional diagnostic report
 * @return whether the panel render graph was emitted
 */
bool _scene_emit_panel_render_ex(
    DvzFigure* figure, uint32_t panel_index, DvzFramePlan* plan, const char* figure_id,
    DvzDiagnosticReport* report)
{
    ANN(figure);
    ANN(plan);
    ANN(figure_id);
    /* Visual preparation is owned by upload emission before render nodes borrow visual data. */
    ASSERT(panel_index < figure->panel_count);
    DvzPanel* panel = &figure->panels[panel_index];

    char panel_id[64];
    dvz_snprintf(panel_id, sizeof(panel_id), "%s_p%u", figure_id, panel_index);
    uint32_t drawable_count = 0;
    for (uint32_t vi = 0; vi < panel->visual_count; vi++)
    {
        DvzVisual* visual = panel->visuals[vi].visual;
        if (visual == NULL || !visual->visible)
            continue;
        if (visual->type == DVZ_VISUAL_TYPE_TEXT)
            continue;
        uint32_t vidx = 0;
        if (!_figure_visual_index(figure, visual, &vidx))
            continue;
        const char* position_attr =
            visual->type == DVZ_VISUAL_TYPE_SEGMENT ? "position_start" : "position";
        int pos_idx = _attr_index(visual, position_attr);
        if (pos_idx >= 0 && visual->attrs[pos_idx].item_count > 0)
            drawable_count++;
        else
            log_warn(
                "%s visual (index %u) has no '%s' data — it will render nothing",
                _visual_type_name(visual->type), vidx, position_attr);
    }

    if (drawable_count == 0)
    {
        dvz_frame_plan_clear_panel(plan, panel_id, "rt", panel->desc);
        return true;
    }

    uint32_t order[DVZ_SCENE_MAX_VISUALS];
    _scene_panel_visual_order(panel, order);

    DvzMVP panel_apply_mvp;
    _scene_panel_apply_mvp(panel, &panel_apply_mvp);
    DvzSceneViewportUniform panel_viewport = {0};
    _scene_panel_pixel_rect(
        panel, &panel_viewport.x, &panel_viewport.y, &panel_viewport.width,
        &panel_viewport.height);
    float framebuffer_scale_x =
        figure->device_scale_x > 0.0f ? figure->device_scale_x * figure->render_scale : 1.0f;
    float framebuffer_scale_y =
        figure->device_scale_y > 0.0f ? figure->device_scale_y * figure->render_scale : 1.0f;
    if (framebuffer_scale_x <= 0.0f)
        framebuffer_scale_x = 1.0f;
    if (framebuffer_scale_y <= 0.0f)
        framebuffer_scale_y = 1.0f;
    panel_viewport.x *= framebuffer_scale_x;
    panel_viewport.y *= framebuffer_scale_y;
    panel_viewport.width *= framebuffer_scale_x;
    panel_viewport.height *= framebuffer_scale_y;
    DvzPanelDesc plot_desc = _scene_panel_plot_desc(panel);

    const uint32_t invalid_node = UINT32_MAX;
    uint32_t scene_occlusion_node = invalid_node;
    bool scene_occlusion_enabled = _scene_panel_has_visible_scene_occlusion_target(panel);
    if (scene_occlusion_enabled)
    {
        if (_scene_begin_panel_render_pass(
                plan, panel_id, "rt.scene_occlusion.depth", panel->desc,
                DVZ_FRAME_PLAN_RENDER_PASS_SCENE_OCCLUSION, &panel_apply_mvp, &panel_viewport,
                plot_desc, &scene_occlusion_node))
        {
            for (uint32_t k = 0; k < panel->visual_count; k++)
            {
                uint32_t vi = order[k];
                DvzPanelAttach* attach = &panel->visuals[vi];
                DvzVisual* visual = attach->visual;
                if (!_scene_visual_is_visible_drawable(visual) || !visual->scene_occluder)
                    continue;
                uint32_t vidx = 0;
                if (!_figure_visual_index(figure, visual, &vidx))
                    continue;
                const DvzVolumeOcclusionDesc* volume_occlusion =
                    visual == panel->volume_occluder_visual && panel->volume_occlusion_enabled
                        ? &panel->volume_occlusion
                        : NULL;
                DvzFramePlanNode* node = _scene_frame_plan_node_mut(plan, scene_occlusion_node);
                if (node == NULL)
                    continue;
                (void)_scene_append_visual_to_render_pass(
                    figure, plan, node, panel, visual, attach, vidx, &panel->scene_occlusion,
                    volume_occlusion);
            }
        }
    }

    uint32_t volume_occlusion_node = invalid_node;
    bool volume_occlusion_enabled = _scene_panel_has_visible_volume_occlusion_target(panel);
    if (volume_occlusion_enabled)
    {
        uint32_t occluder_index = 0;
        if (_figure_visual_index(figure, panel->volume_occluder_visual, &occluder_index))
        {
            if (_scene_begin_panel_render_pass(
                    plan, panel_id, "rt.volume_occlusion.depth", panel->desc,
                    DVZ_FRAME_PLAN_RENDER_PASS_VOLUME_OCCLUSION, &panel_apply_mvp, &panel_viewport,
                    plot_desc, &volume_occlusion_node))
            {
                DvzPanelAttach attach = {
                    .visual = panel->volume_occluder_visual,
                    .z_layer = 0,
                    .controller_mode = DVZ_CONTROLLER_APPLY,
                };
                DvzFramePlanNode* node = _scene_frame_plan_node_mut(plan, volume_occlusion_node);
                if (node != NULL)
                {
                    (void)_scene_append_visual_to_render_pass(
                        figure, plan, node, panel, panel->volume_occluder_visual, &attach,
                        occluder_index, NULL, &panel->volume_occlusion);
                }
            }
        }
    }

    uint32_t opaque_node = invalid_node;
    uint32_t gbuffer_node = invalid_node;
    uint32_t transparent_node = invalid_node;
    uint32_t depth_peel_init_node = invalid_node;
    uint32_t depth_peel_iter_nodes[DVZ_SCENE_DEPTH_PEEL_ITERATIONS] = {0};
    uint32_t depth_peel_composite_node = invalid_node;
    uint32_t blended_nodes[DVZ_SCENE_MAX_RENDER_VISUALS] = {0};
    bool blended_needs_depth[DVZ_SCENE_MAX_RENDER_VISUALS] = {0};
    bool blended_writes_depth[DVZ_SCENE_MAX_RENDER_VISUALS] = {0};
    uint32_t blended_count = 0;
    uint32_t edl_node = invalid_node;
    uint32_t ssao_node = invalid_node;
    uint32_t ssao_blur_node = invalid_node;
    uint32_t ssao_composite_node = invalid_node;
    DvzSceneGBufferPlan gbuffer = {0};
    _scene_technique_gbuffer_plan_init(&gbuffer);
    bool gbuffer_enabled = _scene_technique_gbuffer_enabled(figure->scene, panel);
    const DvzSceneSsaoTechniqueState* ssao_state =
        _scene_technique_ssao_state(figure->scene, panel);
    const DvzSceneMsaaTechniqueState* msaa_state =
        _scene_technique_msaa_state(figure->scene, panel);
    bool ssao_enabled = ssao_state != NULL && ssao_state->enabled;
    bool gbuffer_required = gbuffer_enabled || ssao_enabled;
    const DvzSceneEdlTechniqueState* edl_state = _scene_technique_edl_state(figure->scene, panel);
    bool edl_enabled = edl_state != NULL && edl_state->enabled;
    bool edl_has_depth_producer = false;
    bool has_transparent = false;
    bool opaque_needs_depth = false;
    bool transparent_needs_depth = false;
    bool graph_ok = true;
    for (uint32_t i = 0; i < DVZ_SCENE_DEPTH_PEEL_ITERATIONS; i++)
        depth_peel_iter_nodes[i] = invalid_node;
    for (uint32_t k = 0; k < panel->visual_count; k++)
    {
        uint32_t vi = order[k];
        DvzPanelAttach* attach = &panel->visuals[vi];
        DvzVisual* visual = attach->visual;
        if (visual == NULL || !visual->visible)
            continue;
        if (visual->type == DVZ_VISUAL_TYPE_TEXT)
            continue;
        uint32_t vidx = 0;
        if (!_figure_visual_index(figure, visual, &vidx))
            continue;
        int pos_idx = _attr_index(
            visual, visual->type == DVZ_VISUAL_TYPE_SEGMENT ? "position_start" : "position");
        if (pos_idx < 0 || visual->attrs[pos_idx].item_count == 0)
            continue;

        DvzSceneVisualPassCaps caps = {0};
        if (!_scene_visual_pass_caps_from_visual(visual, attach, &caps))
            continue;
        if (!caps.draws_in_opaque_pass)
        {
            has_transparent = true;
            DvzFramePlanRenderPassRole pass_role = DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND;
            if (caps.draws_in_transparent_blend_pass)
            {
                pass_role = DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND;
            }
            else if (caps.draws_in_wboit_pass)
            {
                pass_role = DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION;
            }
            else if (caps.draws_in_depth_peel_pass)
            {
                pass_role = DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT;
            }
            bool contract_needs_depth = false;
            if (_scene_transparent_contract_needs_depth(
                    visual, attach, pass_role, &contract_needs_depth))
            {
                transparent_needs_depth = transparent_needs_depth || contract_needs_depth;
            }
            continue;
        }

        if (gbuffer_required && _scene_technique_gbuffer_plan_add_visual(&gbuffer, visual, attach))
        {
            if (gbuffer_node == invalid_node)
            {
                if (!_scene_begin_panel_render_pass(
                        plan, panel_id, "rt.gbuffer.normal", panel->desc,
                        DVZ_FRAME_PLAN_RENDER_PASS_GBUFFER, &panel_apply_mvp, &panel_viewport,
                        plot_desc, &gbuffer_node))
                    continue;
            }
            DvzFramePlanNode* node = _scene_frame_plan_node_mut(plan, gbuffer_node);
            if (node == NULL)
                continue;
            (void)_scene_append_visual_to_render_pass(
                figure, plan, node, panel, visual, attach, vidx,
                scene_occlusion_enabled ? &panel->scene_occlusion : NULL,
                volume_occlusion_enabled ? &panel->volume_occlusion : NULL);
        }

        if (opaque_node == invalid_node)
        {
            if (!_scene_begin_panel_render_pass(
                    plan, panel_id, "rt", panel->desc, DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE,
                    &panel_apply_mvp, &panel_viewport, plot_desc, &opaque_node))
                continue;
        }
        DvzFramePlanNode* node = _scene_frame_plan_node_mut(plan, opaque_node);
        if (node == NULL)
            continue;
        (void)_scene_append_visual_to_render_pass(
            figure, plan, node, panel, visual, attach, vidx,
            scene_occlusion_enabled ? &panel->scene_occlusion : NULL,
            volume_occlusion_enabled ? &panel->volume_occlusion : NULL);
        bool edl_depth_visual = edl_enabled && caps.eligible_for_depth_postprocess;
        opaque_needs_depth = opaque_needs_depth || caps.writes_depth || edl_depth_visual;
        edl_has_depth_producer = edl_has_depth_producer || edl_depth_visual;
    }

    if (opaque_node == invalid_node && has_transparent)
    {
        (void)_scene_begin_panel_render_pass(
            plan, panel_id, "rt", panel->desc, DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE, &panel_apply_mvp,
            &panel_viewport, plot_desc, &opaque_node);
    }

    for (uint32_t k = 0; k < panel->visual_count; k++)
    {
        uint32_t vi = order[k];
        DvzPanelAttach* attach = &panel->visuals[vi];
        DvzVisual* visual = attach->visual;
        if (visual == NULL || !visual->visible)
            continue;
        if (visual->type == DVZ_VISUAL_TYPE_TEXT)
            continue;
        DvzSceneVisualPassCaps caps = {0};
        if (!_scene_visual_pass_caps_from_visual(visual, attach, &caps))
            continue;
        if (caps.draws_in_opaque_pass)
            continue;
        uint32_t vidx = 0;
        if (!_figure_visual_index(figure, visual, &vidx))
            continue;
        int pos_idx = _attr_index(
            visual, visual->type == DVZ_VISUAL_TYPE_SEGMENT ? "position_start" : "position");
        if (pos_idx < 0 || visual->attrs[pos_idx].item_count == 0)
            continue;

        if (caps.draws_in_transparent_blend_pass)
        {
            DvzSceneDrawContract draw_contract = {0};
            if (!_scene_draw_contract_from_visual(
                    visual, attach, DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND, &draw_contract))
                continue;
            bool draw_needs_depth = _scene_draw_contract_needs_depth(&draw_contract);
            bool draw_writes_depth =
                (draw_contract.depth_policy & DVZ_SCENE_DEPTH_POLICY_WRITE) != 0;

            bool start_blended_pass = blended_count == 0;
            if (!start_blended_pass)
            {
                uint32_t prev = blended_count - 1;
                start_blended_pass = blended_writes_depth[prev] != draw_writes_depth;
            }
            if (start_blended_pass)
            {
                if (blended_count >= DVZ_SCENE_MAX_RENDER_VISUALS)
                    continue;
                uint32_t node_index = invalid_node;
                if (!_scene_begin_panel_render_pass(
                        plan, panel_id, "rt", panel->desc,
                        DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND, &panel_apply_mvp,
                        &panel_viewport, plot_desc, &node_index))
                    continue;
                blended_nodes[blended_count] = node_index;
                blended_count++;
            }
            uint32_t blend_idx = blended_count - 1;
            DvzFramePlanNode* node = _scene_frame_plan_node_mut(plan, blended_nodes[blend_idx]);
            if (node == NULL)
                continue;
            (void)_scene_append_visual_to_render_pass(
                figure, plan, node, panel, visual, attach, vidx,
                scene_occlusion_enabled ? &panel->scene_occlusion : NULL,
                volume_occlusion_enabled ? &panel->volume_occlusion : NULL);
            blended_needs_depth[blend_idx] = blended_needs_depth[blend_idx] || draw_needs_depth;
            blended_writes_depth[blend_idx] = blended_writes_depth[blend_idx] || draw_writes_depth;
            transparent_needs_depth = transparent_needs_depth || draw_needs_depth;
            continue;
        }

        if (caps.draws_in_depth_peel_pass)
        {
            if (depth_peel_init_node == invalid_node)
            {
                if (!_scene_begin_panel_render_pass(
                        plan, panel_id, "rt.depth_peel_init", panel->desc,
                        DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT, &panel_apply_mvp,
                        &panel_viewport, plot_desc, &depth_peel_init_node))
                    continue;
                bool iter_nodes_ok = true;
                for (uint32_t iter_idx = 0; iter_idx < DVZ_SCENE_DEPTH_PEEL_ITERATIONS; iter_idx++)
                {
                    char iter_target_id[DVZ_SCENE_LABEL_SIZE];
                    dvz_snprintf(
                        iter_target_id, sizeof(iter_target_id), "rt.depth_peel_iter.%" PRIu32,
                        iter_idx);
                    if (!_scene_begin_panel_render_pass(
                            plan, panel_id, iter_target_id, panel->desc,
                            DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER, &panel_apply_mvp,
                            &panel_viewport, plot_desc, &depth_peel_iter_nodes[iter_idx]))
                    {
                        iter_nodes_ok = false;
                        break;
                    }
                    DvzFramePlanNode* iter_node =
                        _scene_frame_plan_node_mut(plan, depth_peel_iter_nodes[iter_idx]);
                    if (iter_node == NULL)
                    {
                        iter_nodes_ok = false;
                        break;
                    }
                    int ret = dvz_snprintf(
                        iter_node->u.render.pass_contract_id,
                        sizeof(iter_node->u.render.pass_contract_id), "%s.pass.%u.iter.%" PRIu32,
                        panel_id, (uint32_t)DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER, iter_idx);
                    if (ret < 0 || (size_t)ret >= sizeof(iter_node->u.render.pass_contract_id))
                    {
                        iter_nodes_ok = false;
                        break;
                    }
                }
                if (!iter_nodes_ok)
                    continue;
                if (!_scene_begin_panel_render_pass(
                        plan, panel_id, "rt", panel->desc,
                        DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_COMPOSITE, &panel_apply_mvp,
                        &panel_viewport, plot_desc, &depth_peel_composite_node))
                    continue;
            }
            DvzFramePlanNode* init_node = _scene_frame_plan_node_mut(plan, depth_peel_init_node);
            if (init_node == NULL)
                continue;
            (void)_scene_append_visual_to_render_pass(
                figure, plan, init_node, panel, visual, attach, vidx,
                scene_occlusion_enabled ? &panel->scene_occlusion : NULL,
                volume_occlusion_enabled ? &panel->volume_occlusion : NULL);
            for (uint32_t iter_idx = 0; iter_idx < DVZ_SCENE_DEPTH_PEEL_ITERATIONS; iter_idx++)
            {
                DvzFramePlanNode* iter_node =
                    _scene_frame_plan_node_mut(plan, depth_peel_iter_nodes[iter_idx]);
                if (iter_node == NULL)
                    continue;
                (void)_scene_append_visual_to_render_pass(
                    figure, plan, iter_node, panel, visual, attach, vidx,
                    scene_occlusion_enabled ? &panel->scene_occlusion : NULL,
                    volume_occlusion_enabled ? &panel->volume_occlusion : NULL);
            }
            transparent_needs_depth = transparent_needs_depth || caps.needs_depth_attachment;
            continue;
        }

        if (transparent_node == invalid_node)
        {
            if (!_scene_begin_panel_render_pass(
                    plan, panel_id, "rt.wboit_accum", panel->desc,
                    DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION, &panel_apply_mvp,
                    &panel_viewport, plot_desc, &transparent_node))
                continue;
        }
        DvzFramePlanNode* node = _scene_frame_plan_node_mut(plan, transparent_node);
        if (node == NULL)
            continue;
        (void)_scene_append_visual_to_render_pass(
            figure, plan, node, panel, visual, attach, vidx,
            scene_occlusion_enabled ? &panel->scene_occlusion : NULL,
            volume_occlusion_enabled ? &panel->volume_occlusion : NULL);
        transparent_needs_depth = transparent_needs_depth || caps.needs_depth_attachment;
    }

    if (scene_occlusion_node != invalid_node &&
        !_scene_technique_emit_scene_occlusion_frame_graph(plan, panel_id))
    {
        _scene_emit_graph_report(
            report, "failed to emit scene occlusion FramePlan graph for panel %s", panel_id);
        graph_ok = false;
    }

    if (transparent_node != invalid_node)
    {
        if (volume_occlusion_node != invalid_node &&
            !_scene_technique_emit_volume_occlusion_frame_graph(plan, panel_id))
        {
            _scene_emit_graph_report(
                report, "failed to emit volume occlusion FramePlan graph for panel %s", panel_id);
            graph_ok = false;
        }
        uint32_t resolve_node = invalid_node;
        (void)_scene_begin_panel_render_pass(
            plan, panel_id, "rt", panel->desc, DVZ_FRAME_PLAN_RENDER_PASS_WBOIT_RESOLVE,
            &panel_apply_mvp, &panel_viewport, plot_desc, &resolve_node);
        if (gbuffer_required && gbuffer_node != invalid_node &&
            !_scene_technique_emit_gbuffer_frame_graph(plan, panel_id, &gbuffer))
        {
            _scene_emit_graph_report(
                report, "failed to emit G-buffer FramePlan graph for panel %s", panel_id);
            graph_ok = false;
        }
        if (!_scene_technique_emit_wboit_frame_graph(
                plan, panel_id, opaque_needs_depth, transparent_needs_depth))
        {
            _scene_emit_graph_report(
                report, "failed to emit WBOIT FramePlan graph for panel %s", panel_id);
            graph_ok = false;
        }
        if (blended_count > 0 && !_scene_technique_emit_blended_frame_graph(
                                     plan, panel_id, false, opaque_needs_depth,
                                     opaque_needs_depth || transparent_needs_depth, blended_count,
                                     blended_needs_depth, blended_writes_depth))
        {
            _scene_emit_graph_report(
                report, "failed to emit blended FramePlan graph for panel %s", panel_id);
            graph_ok = false;
        }
    }
    else if (depth_peel_init_node != invalid_node)
    {
        if (volume_occlusion_node != invalid_node &&
            !_scene_technique_emit_volume_occlusion_frame_graph(plan, panel_id))
        {
            _scene_emit_graph_report(
                report, "failed to emit volume occlusion FramePlan graph for panel %s", panel_id);
            graph_ok = false;
        }
        if (gbuffer_required && gbuffer_node != invalid_node &&
            !_scene_technique_emit_gbuffer_frame_graph(plan, panel_id, &gbuffer))
        {
            _scene_emit_graph_report(
                report, "failed to emit G-buffer FramePlan graph for panel %s", panel_id);
            graph_ok = false;
        }
        if (!_scene_technique_emit_depth_peel_frame_graph(
                plan, panel_id, opaque_needs_depth, transparent_needs_depth))
        {
            _scene_emit_graph_report(
                report, "failed to emit depth-peeling FramePlan graph for panel %s", panel_id);
            graph_ok = false;
        }
    }
    else if (blended_count > 0)
    {
        if (volume_occlusion_node != invalid_node &&
            !_scene_technique_emit_volume_occlusion_frame_graph(plan, panel_id))
        {
            _scene_emit_graph_report(
                report, "failed to emit volume occlusion FramePlan graph for panel %s", panel_id);
            graph_ok = false;
        }
        if (gbuffer_required && gbuffer_node != invalid_node &&
            !_scene_technique_emit_gbuffer_frame_graph(plan, panel_id, &gbuffer))
        {
            _scene_emit_graph_report(
                report, "failed to emit G-buffer FramePlan graph for panel %s", panel_id);
            graph_ok = false;
        }
        bool blended_depth_producer = opaque_needs_depth || transparent_needs_depth;
        if (!_scene_technique_emit_blended_frame_graph(
                plan, panel_id, true, blended_depth_producer, blended_depth_producer,
                blended_count, blended_needs_depth, blended_writes_depth))
        {
            _scene_emit_graph_report(
                report, "failed to emit blended FramePlan graph for panel %s", panel_id);
            graph_ok = false;
        }
    }
    else if (
        opaque_node != invalid_node &&
        (opaque_needs_depth || volume_occlusion_node != invalid_node ||
         scene_occlusion_node != invalid_node))
    {
        if (volume_occlusion_node != invalid_node &&
            !_scene_technique_emit_volume_occlusion_frame_graph(plan, panel_id))
        {
            _scene_emit_graph_report(
                report, "failed to emit volume occlusion FramePlan graph for panel %s", panel_id);
            graph_ok = false;
        }
        if (gbuffer_required && gbuffer_node != invalid_node &&
            !_scene_technique_emit_gbuffer_frame_graph(plan, panel_id, &gbuffer))
        {
            _scene_emit_graph_report(
                report, "failed to emit G-buffer FramePlan graph for panel %s", panel_id);
            graph_ok = false;
        }
        if (edl_enabled && edl_has_depth_producer)
        {
            char edl_params_key[DVZ_SCENE_LABEL_SIZE];
            if (_scene_edl_params_resource_key(panel_id, edl_params_key, sizeof(edl_params_key)))
            {
                _scene_technique_edl_uniform(edl_state, &panel->techniques.edl.uniform);
                if (dvz_frame_plan_upload_bytes(
                        plan, edl_params_key, 0, sizeof(DvzSceneEdlUniform), "edl_params",
                        &panel->techniques.edl.uniform))
                {
                    DvzFramePlanNode* node = &plan->nodes[plan->count - 1];
                    node->u.upload.buffer_usage = DVZ_DRP2_BUFFER_USAGE_UNIFORM |
                                                  DVZ_DRP2_BUFFER_USAGE_MAP_WRITE |
                                                  DVZ_DRP2_BUFFER_USAGE_COPY_DST;
                }
            }
            if (!_scene_begin_panel_render_pass(
                    plan, panel_id, "rt", panel->desc, DVZ_FRAME_PLAN_RENDER_PASS_EDL_RESOLVE,
                    &panel_apply_mvp, &panel_viewport, plot_desc, &edl_node) ||
                !_scene_technique_emit_edl_frame_graph(plan, panel_id))
            {
                _scene_emit_graph_report(
                    report, "failed to emit EDL FramePlan graph for panel %s", panel_id);
                graph_ok = false;
            }
        }
        else if (
            (gbuffer_node != invalid_node || volume_occlusion_node != invalid_node ||
             scene_occlusion_node != invalid_node || (!ssao_enabled && msaa_state != NULL)) &&
            !_scene_technique_emit_opaque_frame_graph(
                plan, panel_id, opaque_needs_depth, msaa_state))
        {
            _scene_emit_graph_report(
                report, "failed to emit opaque FramePlan graph for panel %s", panel_id);
            graph_ok = false;
        }
    }
    if (ssao_enabled && gbuffer_node != invalid_node && gbuffer.producer_count > 0)
    {
        char ssao_params_key[DVZ_SCENE_LABEL_SIZE];
        if (_scene_ssao_params_resource_key(panel_id, ssao_params_key, sizeof(ssao_params_key)))
        {
            _scene_technique_ssao_uniform(
                ssao_state, &panel_apply_mvp, &panel_viewport, &panel->techniques.ssao.uniform);
            if (dvz_frame_plan_upload_bytes(
                    plan, ssao_params_key, 0, sizeof(DvzSceneSsaoUniform), "ssao_params",
                    &panel->techniques.ssao.uniform))
            {
                DvzFramePlanNode* node = &plan->nodes[plan->count - 1];
                node->u.upload.buffer_usage = DVZ_DRP2_BUFFER_USAGE_UNIFORM |
                                              DVZ_DRP2_BUFFER_USAGE_MAP_WRITE |
                                              DVZ_DRP2_BUFFER_USAGE_COPY_DST;
            }
        }
        if (!_scene_begin_panel_render_pass(
                plan, panel_id, "rt.ssao.occlusion", panel->desc, DVZ_FRAME_PLAN_RENDER_PASS_SSAO,
                &panel_apply_mvp, &panel_viewport, plot_desc, &ssao_node))
            ssao_node = invalid_node;
        if (ssao_state->blur_enabled)
        {
            if (!_scene_begin_panel_render_pass(
                    plan, panel_id, "rt.ssao.blur", panel->desc,
                    DVZ_FRAME_PLAN_RENDER_PASS_SSAO_BLUR, &panel_apply_mvp, &panel_viewport,
                    plot_desc, &ssao_blur_node))
                ssao_blur_node = invalid_node;
        }
        if (!_scene_begin_panel_render_pass(
                plan, panel_id, "rt", panel->desc, DVZ_FRAME_PLAN_RENDER_PASS_SSAO_COMPOSITE,
                &panel_apply_mvp, &panel_viewport, plot_desc, &ssao_composite_node))
            ssao_composite_node = invalid_node;
        if (ssao_node == invalid_node ||
            (ssao_state->blur_enabled && ssao_blur_node == invalid_node) ||
            ssao_composite_node == invalid_node ||
            !_scene_technique_emit_ssao_frame_graph(plan, panel_id, &gbuffer, ssao_state))
        {
            _scene_emit_graph_report(
                report, "failed to emit SSAO FramePlan graph for panel %s", panel_id);
            graph_ok = false;
        }
    }
    if (volume_occlusion_node != invalid_node &&
        !_scene_add_volume_occlusion_reads(plan, panel_id))
    {
        _scene_emit_graph_report(
            report, "failed to add volume occlusion FramePlan reads for panel %s", panel_id);
        graph_ok = false;
    }
    if (scene_occlusion_node != invalid_node && !_scene_add_scene_occlusion_reads(plan, panel_id))
    {
        _scene_emit_graph_report(
            report, "failed to add scene occlusion FramePlan reads for panel %s", panel_id);
        graph_ok = false;
    }
    return graph_ok;
}



/**
 * Emit one panel render node into a frame plan.
 *
 * @param figure the parent figure
 * @param panel_index the panel index within the figure
 * @param plan the destination frame plan
 * @param figure_id the stable figure identifier
 * @return whether the panel render graph was emitted
 */
bool _scene_emit_panel_render(
    DvzFigure* figure, uint32_t panel_index, DvzFramePlan* plan, const char* figure_id)
{
    return _scene_emit_panel_render_ex(figure, panel_index, plan, figure_id, NULL);
}
