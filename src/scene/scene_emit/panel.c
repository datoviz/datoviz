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
#include "_scene_resource_key.h"
#include "_scene_shader_abi.h"
#include "_technique.h"
#include "_visual_internal.h"
#include "_visual_pipeline.h"
#include "core/generated_visual_policy.h"
#include "datoviz/drp2/runtime.h"
#include "domain/buffer_internal.h"
#include "registry/registry.h"
#include "render_contract/render_contract.h"
#include "scene_emit/internal.h"
#include "scene_emit/panel_render_plan.h"
#include "scene_emit/scene_emit.h"
#include "scene_emit/visual_lowering.h"


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
 * Resolve generated-role policy carried by an attachment.
 *
 * @param attach the panel visual attachment
 * @param out_policy resolved policy
 * @return whether a generated-role policy was resolved
 */
static bool
_scene_visual_generated_policy(const DvzPanelAttach* attach, DvzGeneratedVisualPolicy* out_policy)
{
    ANN(attach);
    ANN(out_policy);
    if (!attach->has_generated_role)
        return false;
    *out_policy = _scene_generated_visual_policy(attach->generated_role);
    return true;
}


static bool
_scene_visual_explicit_clip_rect(const DvzPanelAttach* attach, DvzFramePlanClipRect* out_clip_rect)
{
    ANN(attach);
    ANN(out_clip_rect);
    switch (attach->clip_rect)
    {
    case DVZ_VISUAL_CLIP_AUTO:
        return false;
    case DVZ_VISUAL_CLIP_PANEL:
        *out_clip_rect = DVZ_FRAME_PLAN_CLIP_RECT_PANEL;
        return true;
    case DVZ_VISUAL_CLIP_PLOT:
        *out_clip_rect = DVZ_FRAME_PLAN_CLIP_RECT_PLOT;
        return true;
    default:
        return false;
    }
}


static bool _scene_visual_explicit_viewport_rect(
    const DvzPanelAttach* attach, DvzFramePlanViewportRect* out_viewport_rect)
{
    ANN(attach);
    ANN(out_viewport_rect);
    switch (attach->viewport_rect)
    {
    case DVZ_VISUAL_VIEWPORT_AUTO:
        return false;
    case DVZ_VISUAL_VIEWPORT_PANEL:
        *out_viewport_rect = DVZ_FRAME_PLAN_VIEWPORT_PANEL;
        return true;
    case DVZ_VISUAL_VIEWPORT_PLOT:
        *out_viewport_rect = DVZ_FRAME_PLAN_VIEWPORT_PLOT;
        return true;
    case DVZ_VISUAL_VIEWPORT_TARGET:
        *out_viewport_rect = DVZ_FRAME_PLAN_VIEWPORT_TARGET;
        return true;
    default:
        return false;
    }
}



/**
 * Return the clip rectangle one visual should use within its panel render pass.
 *
 * @param panel the panel owning the visual attachment
 * @param visual the visual
 * @return the visual clip rectangle kind
 */
static DvzFramePlanClipRect _scene_visual_clip_rect(
    const DvzPanel* panel, const DvzVisual* visual, const DvzPanelAttach* attach)
{
    ANN(panel);
    ANN(visual);
    ANN(attach);
    DvzGeneratedVisualPolicy policy = {0};
    if (_scene_visual_generated_policy(attach, &policy))
        return policy.clip_rect;
    DvzFramePlanClipRect explicit_clip_rect = DVZ_FRAME_PLAN_CLIP_RECT_PLOT;
    if (_scene_visual_explicit_clip_rect(attach, &explicit_clip_rect))
        return explicit_clip_rect;
    if (
        visual->ops != NULL && visual->ops->data_coord_uses_plot_clip_rect &&
        attach->coord_space == DVZ_VISUAL_COORD_DATA)
        return DVZ_FRAME_PLAN_CLIP_RECT_PLOT;
    if (visual->ops != NULL && visual->ops->panel_clip_rect)
    {
        return DVZ_FRAME_PLAN_CLIP_RECT_PANEL;
    }
    return DVZ_FRAME_PLAN_CLIP_RECT_PLOT;
}


/**
 * Return the viewport rectangle one visual should use within its panel render pass.
 *
 * @param panel the panel owning the visual attachment
 * @param visual the visual
 * @param attach the visual attachment
 * @return the visual viewport rectangle kind
 */
static DvzFramePlanViewportRect _scene_visual_viewport_rect(
    const DvzPanel* panel, const DvzVisual* visual, const DvzPanelAttach* attach)
{
    ANN(panel);
    ANN(visual);
    ANN(attach);
    DvzGeneratedVisualPolicy policy = {0};
    if (_scene_visual_generated_policy(attach, &policy))
        return policy.viewport_rect;
    DvzFramePlanViewportRect explicit_viewport_rect = DVZ_FRAME_PLAN_VIEWPORT_PLOT;
    if (_scene_visual_explicit_viewport_rect(attach, &explicit_viewport_rect))
        return explicit_viewport_rect;
    if (attach->coord_space == DVZ_VISUAL_COORD_DATA || attach->coord_space == DVZ_VISUAL_COORD_VIEW)
    {
        return DVZ_FRAME_PLAN_VIEWPORT_PLOT;
    }
    if (visual->ops != NULL && visual->ops->panel_clip_rect)
    {
        return DVZ_FRAME_PLAN_VIEWPORT_PANEL;
    }
    return DVZ_FRAME_PLAN_VIEWPORT_PANEL;
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
 * @param report optional diagnostic report
 * @return whether the visual was appended
 */
static bool _scene_append_visual_to_render_pass(
    const DvzFigure* figure, DvzFramePlan* plan, DvzFramePlanNode* node, const DvzPanel* panel,
    const DvzVisual* visual, const DvzPanelAttach* attach, uint32_t visual_index,
    const DvzSceneOcclusionDesc* scene_occlusion, const DvzVolumeOcclusionDesc* volume_occlusion,
    DvzDiagnosticReport* report)
{
    ANN(figure);
    ANN(plan);
    ANN(node);
    ANN(panel);
    ANN(visual);
    ANN(attach);

    char visual_id[64];
    uint32_t buffer_idx = _scene_buffer_index(figure->scene, _visual_family_state(visual)->buffer);
    if (buffer_idx != UINT32_MAX)
    {
        if (!_scene_visual_indexed_resource_key(
                figure, visual, visual_index, buffer_idx, visual_id, sizeof(visual_id)))
            return false;
    }
    else
    {
        if (!_scene_visual_resource_key(
                figure, visual, visual_index, visual_id, sizeof(visual_id)))
            return false;
    }
    (void)plan;
    if (node->u.render.visual_count >= DVZ_SCENE_MAX_RENDER_VISUALS)
        return false;

    DvzFramePlanVisualMeta metadata = {0};
    bool has_metadata = _scene_visual_frame_plan_metadata(figure, visual, visual_index, &metadata);
    if (!has_metadata)
    {
        _scene_emit_graph_report(report, "visual %s has no typed FramePlan metadata", visual_id);
        return false;
    }
    metadata.clip_rect = _scene_visual_clip_rect(panel, visual, attach);
    metadata.viewport_rect = _scene_visual_viewport_rect(panel, visual, attach);
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

    uint32_t slot = node->u.render.visual_count++;
    dvz_strlcpy(node->u.render.visuals[slot], visual_id, sizeof(node->u.render.visuals[slot]));
    dvz_memcpy(
        &node->u.render.visual_metadata[slot], sizeof(DvzFramePlanVisualMeta), &metadata,
        sizeof(DvzFramePlanVisualMeta));
    node->u.render.visual_metadata[slot].has_metadata = true;
    node->u.render.controller_modes[slot] = attach->controller_mode;

    DvzMVP visual_mvp = {0};
    if (!_scene_panel_attachment_mvp(
            panel, visual, attach, &node->u.render.apply_mvp, &visual_mvp))
        return false;
    node->u.render.visual_mvp[slot] = visual_mvp;
    node->u.render.visual_has_mvp[slot] =
        visual->has_local_transform || attach->coord_space == DVZ_VISUAL_COORD_DATA ||
        attach->coord_space == DVZ_VISUAL_COORD_PANEL ||
        attach->coord_space == DVZ_VISUAL_COORD_PANEL_PIXEL ||
        attach->controller_mode == DVZ_CONTROLLER_APPLY_VIEW_PROJ;
    return true;
}



/**
 * Resolve the framebuffer-scaled panel viewport uniform.
 *
 * @param figure the parent figure
 * @param panel the panel
 * @param out output viewport uniform
 */
static void _scene_panel_viewport_uniform(
    const DvzFigure* figure, const DvzPanel* panel, DvzSceneViewportUniform* out)
{
    ANN(figure);
    ANN(panel);
    ANN(out);
    _scene_panel_pixel_rect(panel, &out->x, &out->y, &out->width, &out->height);
    float framebuffer_scale_x =
        figure->device_scale_x > 0.0f ? figure->device_scale_x * figure->render_scale : 1.0f;
    float framebuffer_scale_y =
        figure->device_scale_y > 0.0f ? figure->device_scale_y * figure->render_scale : 1.0f;
    if (framebuffer_scale_x <= 0.0f)
        framebuffer_scale_x = 1.0f;
    if (framebuffer_scale_y <= 0.0f)
        framebuffer_scale_y = 1.0f;
    out->x *= framebuffer_scale_x;
    out->y *= framebuffer_scale_y;
    out->width *= framebuffer_scale_x;
    out->height *= framebuffer_scale_y;
}


static bool _scene_append_planned_visual_to_render_pass(
    const DvzFigure* figure, DvzFramePlan* plan, DvzFramePlanNode* node, const DvzPanel* panel,
    const DvzPanelRenderVisualPlan* visual_plan, const DvzSceneOcclusionDesc* scene_occlusion,
    const DvzVolumeOcclusionDesc* volume_occlusion, DvzDiagnosticReport* report)
{
    ANN(visual_plan);
    return _scene_append_visual_to_render_pass(
        figure, plan, node, panel, visual_plan->visual, visual_plan->attach,
        visual_plan->visual_index, scene_occlusion, volume_occlusion, report);
}



static bool _scene_emit_edl_params_upload(
    DvzFramePlan* plan, DvzPanel* panel, const char* panel_id,
    const DvzSceneEdlTechniqueState* edl_state, const DvzMVP* panel_apply_mvp,
    const DvzSceneViewportUniform* panel_viewport)
{
    ANN(plan);
    ANN(panel);
    ANN(panel_id);
    ANN(edl_state);
    ANN(panel_apply_mvp);
    ANN(panel_viewport);
    char edl_params_key[DVZ_SCENE_LABEL_SIZE];
    if (!_scene_edl_params_resource_key(panel_id, edl_params_key, sizeof(edl_params_key)))
        return false;
    _scene_technique_edl_uniform(
        edl_state, panel_apply_mvp, panel_viewport, &panel->techniques.edl.uniform);
    if (!dvz_frame_plan_upload_bytes(
            plan, edl_params_key, 0, sizeof(DvzSceneEdlUniform), "edl_params",
            &panel->techniques.edl.uniform))
        return false;
    DvzFramePlanNode* node = &plan->nodes[plan->count - 1];
    node->u.upload.buffer_usage = DVZ_DRP2_BUFFER_USAGE_UNIFORM | DVZ_DRP2_BUFFER_USAGE_MAP_WRITE |
                                  DVZ_DRP2_BUFFER_USAGE_COPY_DST;
    return true;
}



static bool _scene_emit_ssao_params_upload(
    DvzFramePlan* plan, DvzPanel* panel, const char* panel_id,
    const DvzSceneSsaoTechniqueState* ssao_state, const DvzMVP* panel_apply_mvp,
    const DvzSceneViewportUniform* panel_viewport)
{
    ANN(plan);
    ANN(panel);
    ANN(panel_id);
    ANN(ssao_state);
    ANN(panel_apply_mvp);
    ANN(panel_viewport);
    char ssao_params_key[DVZ_SCENE_LABEL_SIZE];
    if (!_scene_ssao_params_resource_key(panel_id, ssao_params_key, sizeof(ssao_params_key)))
        return false;
    _scene_technique_ssao_uniform(
        ssao_state, panel_apply_mvp, panel_viewport, &panel->techniques.ssao.uniform);
    if (!dvz_frame_plan_upload_bytes(
            plan, ssao_params_key, 0, sizeof(DvzSceneSsaoUniform), "ssao_params",
            &panel->techniques.ssao.uniform))
        return false;
    DvzFramePlanNode* node = &plan->nodes[plan->count - 1];
    node->u.upload.buffer_usage = DVZ_DRP2_BUFFER_USAGE_UNIFORM | DVZ_DRP2_BUFFER_USAGE_MAP_WRITE |
                                  DVZ_DRP2_BUFFER_USAGE_COPY_DST;
    return true;
}


static bool _scene_emit_blended_group_node(
    const DvzFigure* figure, DvzFramePlan* plan, const DvzPanel* panel,
    const DvzPanelRenderPlan* render_plan, const DvzMVP* panel_apply_mvp,
    const DvzSceneViewportUniform* panel_viewport, DvzPanelDesc plot_desc, uint32_t group,
    uint32_t* blended_nodes, DvzDiagnosticReport* report)
{
    ANN(figure);
    ANN(plan);
    ANN(panel);
    ANN(render_plan);
    ANN(panel_apply_mvp);
    ANN(panel_viewport);
    ANN(blended_nodes);
    if (group >= render_plan->blended_group_count)
        return true;
    const char* panel_id = render_plan->panel_id;
    if (!_scene_begin_panel_render_pass(
            plan, panel_id, "rt", panel->desc, DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND,
            panel_apply_mvp, panel_viewport, plot_desc, &blended_nodes[group]))
        return true;
    DvzFramePlanNode* node = _scene_frame_plan_node_mut(plan, blended_nodes[group]);
    if (node == NULL)
        return true;
    bool graph_ok = true;
    for (uint32_t i = 0; i < render_plan->blended_visual_count; i++)
    {
        if (render_plan->blended_visuals[i].blend_group != group)
            continue;
        if (!_scene_append_planned_visual_to_render_pass(
                figure, plan, node, panel, &render_plan->blended_visuals[i],
                render_plan->scene_occlusion_enabled ? &panel->scene_occlusion : NULL,
                render_plan->volume_occlusion_enabled ? &panel->volume_occlusion : NULL, report))
            graph_ok = false;
    }
    return graph_ok;
}


/**
 * Return the effective MSAA state after applying runtime sample-count capabilities.
 *
 * @param requested configured panel MSAA state
 * @param caps active runtime capabilities, or NULL to preserve the requested state
 * @param panel_id stable panel id used for diagnostics
 * @param report optional diagnostic report
 * @param storage output storage for the effective state
 * @return effective MSAA state, or NULL when MSAA is disabled or lowered to single-sample
 */
static const DvzSceneMsaaTechniqueState* _scene_effective_msaa_state(
    const DvzSceneMsaaTechniqueState* requested, const DvzCapabilitySnapshot* caps,
    const char* panel_id, DvzDiagnosticReport* report, DvzSceneMsaaTechniqueState* storage)
{
    ANN(storage);
    if (requested == NULL || !requested->enabled || requested->sample_count <= 1)
        return NULL;
    *storage = *requested;
    if (caps == NULL)
        return storage;

    uint32_t color_max = caps->max_color_sample_count != 0 ? caps->max_color_sample_count : 1;
    uint32_t depth_max = caps->max_depth_sample_count != 0 ? caps->max_depth_sample_count : 1;
    uint32_t max_sample_count = color_max < depth_max ? color_max : depth_max;
    uint32_t effective = 1;
    if (requested->sample_count >= 16 && max_sample_count >= 16)
        effective = 16;
    else if (requested->sample_count >= 8 && max_sample_count >= 8)
        effective = 8;
    else if (requested->sample_count >= 4 && max_sample_count >= 4)
        effective = 4;
    else if (requested->sample_count >= 2 && max_sample_count >= 2)
        effective = 2;

    if (effective == requested->sample_count)
        return storage;

    if (report != NULL)
    {
        char message[DVZ_SCENE_DIAGNOSTIC_SIZE] = {0};
        if (effective > 1)
        {
            dvz_snprintf(
                message, sizeof(message),
                "panel %s MSAA sample count lowered from %" PRIu32 " to %" PRIu32,
                panel_id != NULL ? panel_id : "?", requested->sample_count, effective);
        }
        else
        {
            dvz_snprintf(
                message, sizeof(message),
                "panel %s MSAA disabled because runtime supports only single-sample color/depth "
                "attachments",
                panel_id != NULL ? panel_id : "?");
        }
        (void)dvz_diagnostic_report_add(report, message);
    }
    if (effective <= 1)
        return NULL;

    storage->sample_count = effective;
    return storage;
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
bool _scene_emit_panel_render_caps(
    DvzFigure* figure, uint32_t panel_index, DvzFramePlan* plan, const char* figure_id,
    const DvzCapabilitySnapshot* caps, DvzDiagnosticReport* report)
{
    ANN(figure);
    ANN(plan);
    ANN(figure_id);
    /* Visual preparation is owned by upload emission before render nodes borrow visual data. */
    ASSERT(panel_index < figure->panel_count);
    DvzPanel* panel = &figure->panels[panel_index];

    DvzPanelRenderPlan render_plan = {0};
    if (!_scene_panel_render_plan_build(figure, panel_index, figure_id, &render_plan))
        return false;
    const char* panel_id = render_plan.panel_id;
    DvzSceneMsaaTechniqueState effective_msaa_storage = {0};
    const DvzSceneMsaaTechniqueState* effective_msaa = _scene_effective_msaa_state(
        render_plan.msaa_state, caps, panel_id, report, &effective_msaa_storage);

    if (render_plan.drawable_count == 0)
    {
        dvz_frame_plan_clear_panel(plan, panel_id, "rt", panel->desc);
        return true;
    }

    DvzMVP panel_apply_mvp;
    _scene_panel_apply_mvp(panel, &panel_apply_mvp);
    DvzSceneViewportUniform panel_viewport = {0};
    _scene_panel_viewport_uniform(figure, panel, &panel_viewport);
    DvzPanelDesc plot_desc = _scene_panel_plot_desc(panel);

    const uint32_t invalid_node = DVZ_PANEL_RENDER_INVALID_INDEX;
    uint32_t scene_occlusion_node = invalid_node;
    bool graph_ok = true;
    if (render_plan.scene_occlusion_enabled)
    {
        if (_scene_begin_panel_render_pass(
                plan, panel_id, "rt.scene_occlusion.depth", panel->desc,
                DVZ_FRAME_PLAN_RENDER_PASS_SCENE_OCCLUSION, &panel_apply_mvp, &panel_viewport,
                plot_desc, &scene_occlusion_node))
        {
            for (uint32_t k = 0; k < render_plan.scene_occlusion_count; k++)
            {
                const DvzPanelRenderVisualPlan* visual_plan = &render_plan.scene_occlusion[k];
                const DvzVolumeOcclusionDesc* volume_occlusion =
                    visual_plan->visual == panel->volume_occluder_visual &&
                            panel->volume_occlusion_enabled
                        ? &panel->volume_occlusion
                        : NULL;
                DvzFramePlanNode* node = _scene_frame_plan_node_mut(plan, scene_occlusion_node);
                if (node == NULL)
                    continue;
                if (!_scene_append_planned_visual_to_render_pass(
                        figure, plan, node, panel, visual_plan, &panel->scene_occlusion,
                        volume_occlusion, report))
                    graph_ok = false;
            }
        }
    }

    uint32_t volume_occlusion_node = invalid_node;
    if (render_plan.volume_occlusion_enabled && render_plan.has_volume_occluder)
    {
        if (_scene_begin_panel_render_pass(
                plan, panel_id, "rt.volume_occlusion.depth", panel->desc,
                DVZ_FRAME_PLAN_RENDER_PASS_VOLUME_OCCLUSION, &panel_apply_mvp, &panel_viewport,
                plot_desc, &volume_occlusion_node))
        {
            DvzFramePlanNode* node = _scene_frame_plan_node_mut(plan, volume_occlusion_node);
            if (node != NULL)
            {
                DvzPanelRenderVisualPlan visual_plan = {
                    .visual = panel->volume_occluder_visual,
                    .attach = &render_plan.volume_occluder_attach,
                    .visual_index = render_plan.volume_occluder_visual_index,
                };
                if (!_scene_append_planned_visual_to_render_pass(
                        figure, plan, node, panel, &visual_plan, NULL, &panel->volume_occlusion,
                        report))
                    graph_ok = false;
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
    bool blended_group_emitted[DVZ_SCENE_MAX_RENDER_VISUALS] = {0};
    uint32_t edl_node = invalid_node;
    uint32_t ssao_node = invalid_node;
    uint32_t ssao_blur_node = invalid_node;
    uint32_t ssao_composite_node = invalid_node;
    for (uint32_t i = 0; i < DVZ_SCENE_DEPTH_PEEL_ITERATIONS; i++)
        depth_peel_iter_nodes[i] = invalid_node;
    if (render_plan.gbuffer_visual_count > 0)
    {
        if (_scene_begin_panel_render_pass(
                plan, panel_id, "rt.gbuffer.normal", panel->desc,
                DVZ_FRAME_PLAN_RENDER_PASS_GBUFFER, &panel_apply_mvp, &panel_viewport, plot_desc,
                &gbuffer_node))
        {
            DvzFramePlanNode* node = _scene_frame_plan_node_mut(plan, gbuffer_node);
            if (node != NULL)
            {
                for (uint32_t i = 0; i < render_plan.gbuffer_visual_count; i++)
                {
                    if (!_scene_append_planned_visual_to_render_pass(
                            figure, plan, node, panel, &render_plan.gbuffer_visuals[i],
                            render_plan.scene_occlusion_enabled ? &panel->scene_occlusion : NULL,
                            render_plan.volume_occlusion_enabled ? &panel->volume_occlusion : NULL,
                            report))
                        graph_ok = false;
                }
            }
        }
    }

    if (render_plan.opaque_visual_count > 0 || render_plan.has_transparent)
    {
        if (_scene_begin_panel_render_pass(
                plan, panel_id, "rt", panel->desc, DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE,
                &panel_apply_mvp, &panel_viewport, plot_desc, &opaque_node))
        {
            DvzFramePlanNode* node = _scene_frame_plan_node_mut(plan, opaque_node);
            if (node != NULL)
            {
                for (uint32_t i = 0; i < render_plan.opaque_visual_count; i++)
                {
                    if (!_scene_append_planned_visual_to_render_pass(
                            figure, plan, node, panel, &render_plan.opaque_visuals[i],
                            render_plan.scene_occlusion_enabled ? &panel->scene_occlusion : NULL,
                            render_plan.volume_occlusion_enabled ? &panel->volume_occlusion : NULL,
                            report))
                        graph_ok = false;
                }
            }
        }
    }

    for (uint32_t pass_idx = 0; pass_idx < render_plan.transparent_pass_count; pass_idx++)
    {
        const DvzPanelRenderTransparentPassPlan* transparent_pass =
            &render_plan.transparent_passes[pass_idx];
        if (transparent_pass->kind != DVZ_PANEL_RENDER_TRANSPARENT_BLENDED)
            break;
        uint32_t group = transparent_pass->index;
        if (group >= render_plan.blended_group_count || blended_group_emitted[group])
            continue;
        if (!_scene_emit_blended_group_node(
                figure, plan, panel, &render_plan, &panel_apply_mvp, &panel_viewport, plot_desc,
                group, blended_nodes, report))
            graph_ok = false;
        blended_group_emitted[group] = true;
    }

    if (render_plan.depth_peel_visual_count > 0)
    {
        if (_scene_begin_panel_render_pass(
                plan, panel_id, "rt.depth_peel_init", panel->desc,
                DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT, &panel_apply_mvp, &panel_viewport,
                plot_desc, &depth_peel_init_node))
        {
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
            if (!iter_nodes_ok ||
                !_scene_begin_panel_render_pass(
                    plan, panel_id, "rt", panel->desc,
                    DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_COMPOSITE, &panel_apply_mvp,
                    &panel_viewport, plot_desc, &depth_peel_composite_node))
                depth_peel_init_node = invalid_node;
        }
        DvzFramePlanNode* init_node = _scene_frame_plan_node_mut(plan, depth_peel_init_node);
        if (init_node != NULL)
        {
            for (uint32_t i = 0; i < render_plan.depth_peel_visual_count; i++)
            {
                if (!_scene_append_planned_visual_to_render_pass(
                        figure, plan, init_node, panel, &render_plan.depth_peel_visuals[i],
                        render_plan.scene_occlusion_enabled ? &panel->scene_occlusion : NULL,
                        render_plan.volume_occlusion_enabled ? &panel->volume_occlusion : NULL,
                        report))
                    graph_ok = false;
            }
        }
        for (uint32_t iter_idx = 0; iter_idx < DVZ_SCENE_DEPTH_PEEL_ITERATIONS; iter_idx++)
        {
            DvzFramePlanNode* iter_node =
                _scene_frame_plan_node_mut(plan, depth_peel_iter_nodes[iter_idx]);
            if (iter_node == NULL)
                continue;
            for (uint32_t i = 0; i < render_plan.depth_peel_visual_count; i++)
            {
                if (!_scene_append_planned_visual_to_render_pass(
                        figure, plan, iter_node, panel, &render_plan.depth_peel_visuals[i],
                        render_plan.scene_occlusion_enabled ? &panel->scene_occlusion : NULL,
                        render_plan.volume_occlusion_enabled ? &panel->volume_occlusion : NULL,
                        report))
                    graph_ok = false;
            }
        }
    }

    if (render_plan.wboit_visual_count > 0)
    {
        if (_scene_begin_panel_render_pass(
                plan, panel_id, "rt.wboit_accum", panel->desc,
                DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION, &panel_apply_mvp,
                &panel_viewport, plot_desc, &transparent_node))
        {
            DvzFramePlanNode* node = _scene_frame_plan_node_mut(plan, transparent_node);
            if (node != NULL)
            {
                for (uint32_t i = 0; i < render_plan.wboit_visual_count; i++)
                {
                    if (!_scene_append_planned_visual_to_render_pass(
                            figure, plan, node, panel, &render_plan.wboit_visuals[i],
                            render_plan.scene_occlusion_enabled ? &panel->scene_occlusion : NULL,
                            render_plan.volume_occlusion_enabled ? &panel->volume_occlusion : NULL,
                            report))
                        graph_ok = false;
                }
            }
        }
    }

    for (uint32_t group = 0; group < render_plan.blended_group_count; group++)
    {
        if (blended_group_emitted[group])
            continue;
        if (!_scene_emit_blended_group_node(
                figure, plan, panel, &render_plan, &panel_apply_mvp, &panel_viewport, plot_desc,
                group, blended_nodes, report))
            graph_ok = false;
        blended_group_emitted[group] = true;
    }

    if (scene_occlusion_node != invalid_node &&
        !_scene_technique_emit_scene_occlusion_frame_graph(plan, panel_id))
    {
        _scene_emit_graph_report(
            report, "failed to emit scene occlusion FramePlan graph for panel %s", panel_id);
        graph_ok = false;
    }

    bool emit_blended_after_ssao = false;
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
        if (render_plan.gbuffer_required && gbuffer_node != invalid_node &&
            !_scene_technique_emit_gbuffer_frame_graph(plan, panel_id, &render_plan.gbuffer))
        {
            _scene_emit_graph_report(
                report, "failed to emit G-buffer FramePlan graph for panel %s", panel_id);
            graph_ok = false;
        }
        if (!_scene_technique_emit_wboit_frame_graph(
                plan, panel_id, render_plan.opaque_needs_depth,
                render_plan.transparent_needs_depth))
        {
            _scene_emit_graph_report(
                report, "failed to emit WBOIT FramePlan graph for panel %s", panel_id);
            graph_ok = false;
        }
        if (render_plan.blended_group_count > 0 &&
            !_scene_technique_emit_blended_frame_graph(
                plan, panel_id, false, render_plan.opaque_needs_depth,
                render_plan.opaque_needs_depth || render_plan.transparent_needs_depth,
                render_plan.blended_group_count, render_plan.blended_needs_depth,
                render_plan.blended_writes_depth, NULL))
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
        if (render_plan.gbuffer_required && gbuffer_node != invalid_node &&
            !_scene_technique_emit_gbuffer_frame_graph(plan, panel_id, &render_plan.gbuffer))
        {
            _scene_emit_graph_report(
                report, "failed to emit G-buffer FramePlan graph for panel %s", panel_id);
            graph_ok = false;
        }
        if (!_scene_technique_emit_depth_peel_frame_graph(
                plan, panel_id, render_plan.opaque_needs_depth,
                render_plan.transparent_needs_depth))
        {
            _scene_emit_graph_report(
                report, "failed to emit depth-peeling FramePlan graph for panel %s", panel_id);
            graph_ok = false;
        }
        if (render_plan.blended_group_count > 0 &&
            !_scene_technique_emit_blended_frame_graph(
                plan, panel_id, false, false, false, render_plan.blended_group_count,
                render_plan.blended_needs_depth, render_plan.blended_writes_depth, NULL))
        {
            _scene_emit_graph_report(
                report, "failed to emit blended FramePlan graph for panel %s", panel_id);
            graph_ok = false;
        }
    }
    else if (render_plan.blended_group_count > 0)
    {
        if (volume_occlusion_node != invalid_node &&
            !_scene_technique_emit_volume_occlusion_frame_graph(plan, panel_id))
        {
            _scene_emit_graph_report(
                report, "failed to emit volume occlusion FramePlan graph for panel %s", panel_id);
            graph_ok = false;
        }
        if (render_plan.gbuffer_required && gbuffer_node != invalid_node &&
            !_scene_technique_emit_gbuffer_frame_graph(plan, panel_id, &render_plan.gbuffer))
        {
            _scene_emit_graph_report(
                report, "failed to emit G-buffer FramePlan graph for panel %s", panel_id);
            graph_ok = false;
        }
        if (render_plan.edl_enabled && render_plan.edl_has_depth_producer)
        {
            (void)_scene_emit_edl_params_upload(
                plan, panel, panel_id, render_plan.edl_state, &panel_apply_mvp, &panel_viewport);
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
        bool blended_depth_producer =
            render_plan.opaque_needs_depth || render_plan.transparent_needs_depth;
        if (render_plan.ssao_enabled && gbuffer_node != invalid_node &&
            render_plan.gbuffer.producer_count > 0)
        {
            bool any_blended_needs_depth = false;
            for (uint32_t i = 0; i < render_plan.blended_group_count; i++)
                any_blended_needs_depth =
                    any_blended_needs_depth || render_plan.blended_needs_depth[i];
            const DvzSceneMsaaTechniqueState* pre_ssao_msaa =
                any_blended_needs_depth ? NULL : effective_msaa;
            if (!_scene_technique_emit_blended_frame_graph(
                    plan, panel_id, true, blended_depth_producer, blended_depth_producer, 0, NULL,
                    NULL, pre_ssao_msaa))
            {
                _scene_emit_graph_report(
                    report, "failed to emit blended FramePlan graph for panel %s", panel_id);
                graph_ok = false;
            }
            else
            {
                emit_blended_after_ssao = true;
            }
        }
        else if (!_scene_technique_emit_blended_frame_graph(
                     plan, panel_id, true, blended_depth_producer, blended_depth_producer,
                     render_plan.blended_group_count, render_plan.blended_needs_depth,
                     render_plan.blended_writes_depth, effective_msaa))
        {
            _scene_emit_graph_report(
                report, "failed to emit blended FramePlan graph for panel %s", panel_id);
            graph_ok = false;
        }
    }
    else if (
        opaque_node != invalid_node &&
        (render_plan.opaque_needs_depth || volume_occlusion_node != invalid_node ||
         scene_occlusion_node != invalid_node))
    {
        if (volume_occlusion_node != invalid_node &&
            !_scene_technique_emit_volume_occlusion_frame_graph(plan, panel_id))
        {
            _scene_emit_graph_report(
                report, "failed to emit volume occlusion FramePlan graph for panel %s", panel_id);
            graph_ok = false;
        }
        if (render_plan.gbuffer_required && gbuffer_node != invalid_node &&
            !_scene_technique_emit_gbuffer_frame_graph(plan, panel_id, &render_plan.gbuffer))
        {
            _scene_emit_graph_report(
                report, "failed to emit G-buffer FramePlan graph for panel %s", panel_id);
            graph_ok = false;
        }
        if (render_plan.edl_enabled && render_plan.edl_has_depth_producer)
        {
            (void)_scene_emit_edl_params_upload(
                plan, panel, panel_id, render_plan.edl_state, &panel_apply_mvp, &panel_viewport);
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
             scene_occlusion_node != invalid_node ||
             (!render_plan.ssao_enabled && effective_msaa != NULL)) &&
            !_scene_technique_emit_opaque_frame_graph(
                plan, panel_id, render_plan.opaque_needs_depth, effective_msaa))
        {
            _scene_emit_graph_report(
                report, "failed to emit opaque FramePlan graph for panel %s", panel_id);
            graph_ok = false;
        }
    }
    if (render_plan.ssao_enabled && gbuffer_node != invalid_node &&
        render_plan.gbuffer.producer_count > 0)
    {
        (void)_scene_emit_ssao_params_upload(
            plan, panel, panel_id, render_plan.ssao_state, &panel_apply_mvp, &panel_viewport);
        if (!_scene_begin_panel_render_pass(
                plan, panel_id, "rt.ssao.occlusion", panel->desc, DVZ_FRAME_PLAN_RENDER_PASS_SSAO,
                &panel_apply_mvp, &panel_viewport, plot_desc, &ssao_node))
            ssao_node = invalid_node;
        if (render_plan.ssao_state->blur_enabled)
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
            (render_plan.ssao_state->blur_enabled && ssao_blur_node == invalid_node) ||
            ssao_composite_node == invalid_node ||
            !_scene_technique_emit_ssao_frame_graph(
                plan, panel_id, &render_plan.gbuffer, render_plan.ssao_state))
        {
            _scene_emit_graph_report(
                report, "failed to emit SSAO FramePlan graph for panel %s", panel_id);
            graph_ok = false;
        }
    }
    if (emit_blended_after_ssao)
    {
        bool blended_depth_producer =
            render_plan.opaque_needs_depth || render_plan.transparent_needs_depth;
        if (!_scene_technique_emit_blended_frame_graph(
                plan, panel_id, false, false, blended_depth_producer,
                render_plan.blended_group_count, render_plan.blended_needs_depth,
                render_plan.blended_writes_depth, NULL))
        {
            _scene_emit_graph_report(
                report, "failed to emit blended FramePlan graph for panel %s", panel_id);
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


bool _scene_emit_panel_render_ex(
    DvzFigure* figure, uint32_t panel_index, DvzFramePlan* plan, const char* figure_id,
    DvzDiagnosticReport* report)
{
    return _scene_emit_panel_render_caps(figure, panel_index, plan, figure_id, NULL, report);
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
