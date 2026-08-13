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
#include "frame_plan/internal.h"
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



static bool _scene_composition_pass_for_role(
    const DvzPanelCompositionSnapshot* snapshot, DvzFramePlanRenderPassRole role, uint32_t ordinal,
    const DvzSceneResolvedPass** out)
{
    ANN(snapshot);
    ANN(out);
    for (uint32_t i = 0; i < snapshot->pass_count; i++)
    {
        if (snapshot->passes[i].role == role && snapshot->passes[i].ordinal == ordinal)
        {
            *out = &snapshot->passes[i];
            return true;
        }
    }
    return false;
}



/**
 * Persist typed composition identity and direct graph-pass indices.
 *
 * @param plan the destination frame plan
 * @param panel_id the panel id
 * @param snapshot the immutable composition snapshot
 * @param report optional diagnostic report
 * @return whether every graph-backed render pass was linked
 */
bool _scene_bind_panel_composition(
    DvzFramePlan* plan, const char* panel_id, const DvzPanelCompositionSnapshot* snapshot,
    DvzDiagnosticReport* report)
{
    ANN(plan);
    ANN(panel_id);
    ANN(snapshot);
    bool ok = true;

    for (uint32_t i = 0; i < plan->count; i++)
    {
        DvzFramePlanNode* render = &plan->nodes[i];
        if (render->type != DVZ_FRAME_PLAN_NODE_RENDER ||
            strcmp(render->u.render.panel_id, panel_id) != 0)
            continue;
        uint32_t ordinal = 0;
        for (uint32_t j = 0; j < i; j++)
        {
            const DvzFramePlanNode* previous = &plan->nodes[j];
            if (previous->type == DVZ_FRAME_PLAN_NODE_RENDER &&
                strcmp(previous->u.render.panel_id, panel_id) == 0 &&
                previous->u.render.pass_role == render->u.render.pass_role)
                ordinal++;
        }
        const DvzSceneResolvedPass* resolved = NULL;
        if (!_scene_composition_pass_for_role(
                snapshot, render->u.render.pass_role, ordinal, &resolved))
        {
            _scene_emit_graph_report(
                report, "panel %s render role %u ordinal %u is absent from composition snapshot",
                panel_id, (uint32_t)render->u.render.pass_role, ordinal);
            ok = false;
            continue;
        }
        bool duplicate = false;
        for (uint32_t j = 0; j < i; j++)
        {
            const DvzFramePlanNode* previous = &plan->nodes[j];
            if (previous->type == DVZ_FRAME_PLAN_NODE_RENDER &&
                strcmp(previous->u.render.panel_id, panel_id) == 0 &&
                previous->u.render.has_composition_pass &&
                previous->u.render.composition_pass_id.value == resolved->id.value)
            {
                _scene_emit_graph_report(
                    report, "panel %s composition pass %u has duplicate render bindings", panel_id,
                    resolved->id.value);
                ok = false;
                duplicate = true;
                break;
            }
        }
        if (duplicate)
            continue;
        render->u.render.has_composition_pass = true;
        render->u.render.composition_pass_id = resolved->id;
        int written = dvz_snprintf(
            render->u.render.pass_contract_id, sizeof(render->u.render.pass_contract_id),
            "%s.composition.%u", panel_id, resolved->id.value);
        if (written < 0 || (size_t)written >= sizeof(render->u.render.pass_contract_id))
        {
            _scene_emit_graph_report(
                report, "panel %s composition pass contract id is truncated", panel_id);
            ok = false;
            continue;
        }
        render->u.render.has_pass_contract = true;
        for (uint32_t j = 0; j < render->u.render.visual_count; j++)
        {
            DvzFramePlanVisualMeta* metadata = &render->u.render.visual_metadata[j];
            written = dvz_snprintf(
                metadata->draw_contract_id, sizeof(metadata->draw_contract_id), "%s.draw.%u",
                render->u.render.pass_contract_id, metadata->visual_index);
            if (written < 0 || (size_t)written >= sizeof(metadata->draw_contract_id))
            {
                _scene_emit_graph_report(
                    report, "panel %s composition draw contract id is truncated", panel_id);
                ok = false;
                break;
            }
            metadata->has_draw_contract = true;
        }
    }

    for (uint32_t i = 0; i < plan->graph_pass_count; i++)
    {
        const DvzFrameGraphPass* pass = &plan->graph_passes[i];
        if (strcmp(pass->panel_id, panel_id) == 0 && !pass->has_composition_pass)
        {
            _scene_emit_graph_report(
                report, "panel %s graph pass %s has no typed composition identity", panel_id,
                pass->id);
            ok = false;
        }
    }

    for (uint32_t i = 0; i < plan->count; i++)
    {
        DvzFramePlanNode* render = &plan->nodes[i];
        if (render->type != DVZ_FRAME_PLAN_NODE_RENDER ||
            strcmp(render->u.render.panel_id, panel_id) != 0 ||
            !render->u.render.has_composition_pass)
            continue;
        uint32_t match_count = 0;
        uint32_t match_index = UINT32_MAX;
        for (uint32_t j = 0; j < plan->graph_pass_count; j++)
        {
            const DvzFrameGraphPass* pass = &plan->graph_passes[j];
            if (strcmp(pass->panel_id, panel_id) == 0 && pass->has_composition_pass &&
                pass->composition_pass_id.value == render->u.render.composition_pass_id.value)
            {
                match_count++;
                match_index = j;
            }
        }
        if (match_count > 1)
        {
            _scene_emit_graph_report(
                report, "panel %s composition pass %u has duplicate graph bindings", panel_id,
                render->u.render.composition_pass_id.value);
            ok = false;
            continue;
        }
        if (match_count == 1)
        {
            bool duplicate = false;
            for (uint32_t j = 0; j < i; j++)
            {
                const DvzFramePlanNode* previous = &plan->nodes[j];
                if (previous->type == DVZ_FRAME_PLAN_NODE_RENDER &&
                    strcmp(previous->u.render.panel_id, panel_id) == 0 &&
                    previous->u.render.has_graph_pass_index &&
                    previous->u.render.graph_pass_index == match_index)
                {
                    _scene_emit_graph_report(
                        report, "panel %s graph pass %u has duplicate render bindings", panel_id,
                        match_index);
                    ok = false;
                    duplicate = true;
                    break;
                }
            }
            if (!duplicate)
            {
                render->u.render.has_graph_pass_index = true;
                render->u.render.graph_pass_index = match_index;
            }
        }
        if (render->u.render.has_composition_pass && !render->u.render.has_graph_pass_index)
        {
            _scene_emit_graph_report(
                report, "panel %s composition pass %u has no resolved graph pass", panel_id,
                render->u.render.composition_pass_id.value);
            ok = false;
        }
    }

    /* The forward scans above reject physical work absent from the snapshot. This reverse scan
     * rejects snapshot work omitted by the typed lowerer. Presentation is represented by a
     * technique without a resolved pass, and an empty panel has no resolved passes, so both
     * intentionally require no physical binding here. */
    for (uint32_t i = 0; i < snapshot->pass_count; i++)
    {
        const DvzSceneResolvedPass* resolved = &snapshot->passes[i];
        uint32_t render_count = 0;
        uint32_t graph_count = 0;
        for (uint32_t j = 0; j < plan->count; j++)
        {
            const DvzFramePlanNode* render = &plan->nodes[j];
            if (render->type == DVZ_FRAME_PLAN_NODE_RENDER &&
                strcmp(render->u.render.panel_id, panel_id) == 0 &&
                render->u.render.has_composition_pass &&
                render->u.render.composition_pass_id.value == resolved->id.value)
                render_count++;
        }
        for (uint32_t j = 0; j < plan->graph_pass_count; j++)
        {
            const DvzFrameGraphPass* pass = &plan->graph_passes[j];
            if (strcmp(pass->panel_id, panel_id) == 0 && pass->has_composition_pass &&
                pass->composition_pass_id.value == resolved->id.value)
                graph_count++;
        }
        if (render_count == 0)
        {
            _scene_emit_graph_report(
                report, "panel %s composition pass %u has no render binding", panel_id,
                resolved->id.value);
            ok = false;
        }
        else if (render_count > 1)
        {
            _scene_emit_graph_report(
                report, "panel %s composition pass %u has duplicate render bindings", panel_id,
                resolved->id.value);
            ok = false;
        }

        if (graph_count == 0)
        {
            _scene_emit_graph_report(
                report, "panel %s composition pass %u has no required graph binding", panel_id,
                resolved->id.value);
            ok = false;
        }
        else if (graph_count > 1)
        {
            _scene_emit_graph_report(
                report, "panel %s composition pass %u has duplicate graph bindings", panel_id,
                resolved->id.value);
            ok = false;
        }
    }
    return ok;
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
    if (visual->ops != NULL && visual->ops->data_coord_uses_plot_clip_rect &&
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
    if (attach->coord_space == DVZ_VISUAL_COORD_DATA ||
        attach->coord_space == DVZ_VISUAL_COORD_VIEW)
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
                figure, visual, visual_index, _visual_family_state(visual)->buffer->id, visual_id,
                sizeof(visual_id)))
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
    metadata.draw_blend_mode = (uint32_t)draw_contract.blend_mode;
    metadata.draw_shader_feature_mask = draw_contract.shader_feature_mask;
    metadata.draw_bind_group_layout_mask = draw_contract.bind_group_layout_mask;
    metadata.draw_overlay_composite = draw_contract.overlay_composite;
    metadata.draw_has_raster_state = draw_contract.has_raster_state;
    metadata.draw_cull_mode = draw_contract.cull_mode;
    metadata.draw_front_face = draw_contract.front_face;
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

    DvzMVP visual_mvp = {0};
    if (!_scene_panel_attachment_mvp(
            panel, visual, attach, &node->u.render.apply_mvp, &visual_mvp))
        return false;
    if (!_frame_plan_render_visual_reserve(node, node->u.render.visual_count + 1))
        return false;
    uint32_t slot = node->u.render.visual_count;
    dvz_strlcpy(node->u.render.visuals[slot], visual_id, sizeof(node->u.render.visuals[slot]));
    dvz_memcpy(
        &node->u.render.visual_metadata[slot], sizeof(DvzFramePlanVisualMeta), &metadata,
        sizeof(DvzFramePlanVisualMeta));
    node->u.render.visual_metadata[slot].has_metadata = true;
    node->u.render.controller_modes[slot] = attach->controller_mode;

    node->u.render.visual_mvp[slot] = visual_mvp;
    node->u.render.visual_has_mvp[slot] =
        visual->has_local_transform || attach->coord_space == DVZ_VISUAL_COORD_DATA ||
        attach->coord_space == DVZ_VISUAL_COORD_PANEL ||
        attach->coord_space == DVZ_VISUAL_COORD_PANEL_PIXEL ||
        attach->controller_mode == DVZ_CONTROLLER_APPLY_VIEW_PROJ;
    node->u.render.visual_count++;
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
    const DvzSceneViewportUniform* panel_viewport, uint32_t* out_node_index)
{
    ANN(plan);
    ANN(panel);
    ANN(panel_id);
    ANN(edl_state);
    ANN(panel_apply_mvp);
    ANN(panel_viewport);
    ANN(out_node_index);
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
    *out_node_index = plan->count - 1;
    node->u.upload.buffer_usage = DVZ_DRP2_BUFFER_USAGE_UNIFORM | DVZ_DRP2_BUFFER_USAGE_MAP_WRITE |
                                  DVZ_DRP2_BUFFER_USAGE_COPY_DST;
    return true;
}



static bool _scene_emit_gtao_params_upload(
    DvzFramePlan* plan, DvzPanel* panel, const char* panel_id,
    const DvzSceneAoTechniqueState* ao_state, const DvzMVP* panel_apply_mvp,
    const DvzSceneViewportUniform* panel_viewport, uint32_t* out_node_index)
{
    ANN(plan);
    ANN(panel);
    ANN(panel_id);
    ANN(ao_state);
    ANN(panel_apply_mvp);
    ANN(panel_viewport);
    ANN(out_node_index);
    char gtao_params_key[DVZ_SCENE_LABEL_SIZE];
    if (!_scene_gtao_params_resource_key(panel_id, gtao_params_key, sizeof(gtao_params_key)))
        return false;
    _scene_technique_ao_uniform(
        ao_state, panel_apply_mvp, panel_viewport, &panel->techniques.ao.uniform);
    if (!dvz_frame_plan_upload_bytes(
            plan, gtao_params_key, 0, sizeof(DvzSceneAoUniform), "gtao_params",
            &panel->techniques.ao.uniform))
        return false;
    DvzFramePlanNode* node = &plan->nodes[plan->count - 1];
    *out_node_index = plan->count - 1;
    node->u.upload.buffer_usage = DVZ_DRP2_BUFFER_USAGE_UNIFORM | DVZ_DRP2_BUFFER_USAGE_MAP_WRITE |
                                  DVZ_DRP2_BUFFER_USAGE_COPY_DST;
    return true;
}



/**
 * Bind one typed auxiliary resource to its emitted upload node.
 *
 * @param snapshot mutable composition snapshot
 * @param kind auxiliary resource kind
 * @param node_index FramePlan upload-node index
 * @return whether at least one provider binding was resolved
 */
static bool _scene_bind_auxiliary_upload(
    DvzPanelCompositionSnapshot* snapshot, DvzSceneAuxiliaryKind kind, uint32_t node_index)
{
    ANN(snapshot);
    if (node_index == UINT32_MAX)
        return false;
    bool found = false;
    for (uint32_t i = 0; i < snapshot->pass_count; i++)
    {
        DvzSceneResolvedPass* pass = &snapshot->passes[i];
        for (uint32_t j = 0; j < pass->auxiliary_binding_count; j++)
        {
            DvzSceneAuxiliaryBinding* binding = &pass->auxiliary_bindings[j];
            if (binding->kind == kind)
            {
                binding->upload_node_index = node_index;
                found = true;
            }
        }
    }
    return found;
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
 * Emit one authored WBOIT run and its immediate source-over resolve.
 *
 * @param figure source figure
 * @param plan destination FramePlan
 * @param panel source panel
 * @param render_plan resolved panel render plan
 * @param panel_apply_mvp panel transform uniform
 * @param panel_viewport panel viewport uniform
 * @param plot_desc panel-local plot rectangle
 * @param group WBOIT run index
 * @param report optional diagnostic report
 * @return whether both nodes and all visuals were emitted
 */
static bool _scene_emit_wboit_group_nodes(
    const DvzFigure* figure, DvzFramePlan* plan, const DvzPanel* panel,
    const DvzPanelRenderPlan* render_plan, const DvzMVP* panel_apply_mvp,
    const DvzSceneViewportUniform* panel_viewport, DvzPanelDesc plot_desc, uint32_t group,
    DvzDiagnosticReport* report)
{
    ANN(figure);
    ANN(plan);
    ANN(panel);
    ANN(render_plan);
    ANN(panel_apply_mvp);
    ANN(panel_viewport);
    if (group >= render_plan->wboit_group_count)
        return false;

    char target_id[DVZ_SCENE_LABEL_SIZE] = {0};
    if (group == 0)
        dvz_strlcpy(target_id, "rt.wboit_accum", sizeof(target_id));
    else
        dvz_snprintf(target_id, sizeof(target_id), "rt.wboit_accum.%" PRIu32, group);

    uint32_t accum_node = DVZ_PANEL_RENDER_INVALID_INDEX;
    if (!_scene_begin_panel_render_pass(
            plan, render_plan->panel_id, target_id, panel->desc,
            DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION, panel_apply_mvp, panel_viewport,
            plot_desc, &accum_node))
        return false;
    DvzFramePlanNode* node = _scene_frame_plan_node_mut(plan, accum_node);
    if (node == NULL)
        return false;

    for (uint32_t i = 0; i < render_plan->wboit_visual_count; i++)
    {
        if (render_plan->wboit_visuals[i].blend_group != group)
            continue;
        if (!_scene_append_planned_visual_to_render_pass(
                figure, plan, node, panel, &render_plan->wboit_visuals[i],
                render_plan->scene_occlusion_enabled ? &panel->scene_occlusion : NULL,
                render_plan->volume_occlusion_enabled ? &panel->volume_occlusion : NULL, report))
            return false;
    }

    uint32_t resolve_node = DVZ_PANEL_RENDER_INVALID_INDEX;
    return _scene_begin_panel_render_pass(
        plan, render_plan->panel_id, "rt", panel->desc, DVZ_FRAME_PLAN_RENDER_PASS_WBOIT_RESOLVE,
        panel_apply_mvp, panel_viewport, plot_desc, &resolve_node);
}



static bool _scene_emit_depth_peel_group_nodes(
    const DvzFigure* figure, DvzFramePlan* plan, const DvzPanel* panel,
    const DvzPanelRenderPlan* render_plan, const DvzMVP* panel_apply_mvp,
    const DvzSceneViewportUniform* panel_viewport, DvzPanelDesc plot_desc, uint32_t group,
    DvzDiagnosticReport* report)
{
    ANN(figure);
    ANN(plan);
    ANN(panel);
    ANN(render_plan);
    ANN(panel_apply_mvp);
    ANN(panel_viewport);
    if (group >= render_plan->depth_peel_group_count)
        return false;

    char target_id[DVZ_SCENE_LABEL_SIZE] = {0};
    dvz_snprintf(target_id, sizeof(target_id), "rt.depth_peel.%" PRIu32 ".init", group);
    uint32_t init_node_index = DVZ_PANEL_RENDER_INVALID_INDEX;
    if (!_scene_begin_panel_render_pass(
            plan, render_plan->panel_id, target_id, panel->desc,
            DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT, panel_apply_mvp, panel_viewport, plot_desc,
            &init_node_index))
        return false;
    DvzFramePlanNode* init_node = _scene_frame_plan_node_mut(plan, init_node_index);
    if (init_node == NULL)
        return false;
    for (uint32_t i = 0; i < render_plan->depth_peel_visual_count; i++)
    {
        const DvzPanelRenderVisualPlan* visual = &render_plan->depth_peel_visuals[i];
        if (visual->blend_group != group)
            continue;
        if (!_scene_append_planned_visual_to_render_pass(
                figure, plan, init_node, panel, visual,
                render_plan->scene_occlusion_enabled ? &panel->scene_occlusion : NULL,
                render_plan->volume_occlusion_enabled ? &panel->volume_occlusion : NULL, report))
            return false;
    }

    for (uint32_t iter = 0; iter < DVZ_SCENE_DEPTH_PEEL_ITERATIONS; iter++)
    {
        dvz_snprintf(
            target_id, sizeof(target_id), "rt.depth_peel.%" PRIu32 ".iter.%" PRIu32, group, iter);
        uint32_t iter_node_index = DVZ_PANEL_RENDER_INVALID_INDEX;
        if (!_scene_begin_panel_render_pass(
                plan, render_plan->panel_id, target_id, panel->desc,
                DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER, panel_apply_mvp, panel_viewport,
                plot_desc, &iter_node_index))
            return false;
        DvzFramePlanNode* iter_node = _scene_frame_plan_node_mut(plan, iter_node_index);
        if (iter_node == NULL)
            return false;
        for (uint32_t i = 0; i < render_plan->depth_peel_visual_count; i++)
        {
            const DvzPanelRenderVisualPlan* visual = &render_plan->depth_peel_visuals[i];
            if (visual->blend_group != group)
                continue;
            if (!_scene_append_planned_visual_to_render_pass(
                    figure, plan, iter_node, panel, visual,
                    render_plan->scene_occlusion_enabled ? &panel->scene_occlusion : NULL,
                    render_plan->volume_occlusion_enabled ? &panel->volume_occlusion : NULL,
                    report))
                return false;
        }
    }

    dvz_snprintf(target_id, sizeof(target_id), "rt.depth_peel.%" PRIu32 ".composite", group);
    uint32_t composite_node_index = DVZ_PANEL_RENDER_INVALID_INDEX;
    return _scene_begin_panel_render_pass(
        plan, render_plan->panel_id, target_id, panel->desc,
        DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_COMPOSITE, panel_apply_mvp, panel_viewport,
        plot_desc, &composite_node_index);
}



/**
 * Reconstruct the effective MSAA state from the immutable composition snapshot.
 *
 * @param snapshot immutable composition snapshot
 * @param panel_id stable panel id used for diagnostics
 * @param report optional diagnostic report
 * @param storage output storage for the effective state
 * @return effective MSAA state, or NULL when MSAA is disabled or lowered to single-sample
 */
static const DvzSceneMsaaTechniqueState* _scene_composition_msaa_state(
    const DvzPanelCompositionSnapshot* snapshot, const char* panel_id, DvzDiagnosticReport* report,
    DvzSceneMsaaTechniqueState* storage)
{
    ANN(snapshot);
    ANN(storage);
    if (snapshot->requested_sample_count <= 1)
        return NULL;
    uint32_t effective = snapshot->effective_sample_count;
    if (effective != snapshot->requested_sample_count && report != NULL)
    {
        char message[DVZ_SCENE_DIAGNOSTIC_SIZE] = {0};
        if (effective > 1)
        {
            dvz_snprintf(
                message, sizeof(message),
                "panel %s MSAA sample count lowered from %" PRIu32 " to %" PRIu32,
                panel_id != NULL ? panel_id : "?", snapshot->requested_sample_count, effective);
        }
        else
        {
            dvz_snprintf(
                message, sizeof(message),
                "panel %s MSAA disabled because runtime supports only single-sample color/depth "
                "attachments",
                panel_id != NULL ? panel_id : "?");
        }
        (void)dvz_diagnostic_report_add_with_severity(
            report, DVZ_DIAGNOSTIC_SEVERITY_WARNING, message);
    }
    if (effective <= 1)
        return NULL;
    *storage = (DvzSceneMsaaTechniqueState){
        .enabled = true,
        .sample_count = effective,
        .alpha_to_coverage = snapshot->alpha_to_coverage,
    };
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
    if (!_scene_panel_render_plan_build(
            figure, panel_index, figure_id, caps, report, &render_plan))
        return false;
    const char* panel_id = render_plan.panel_id;
    DvzSceneMsaaTechniqueState effective_msaa_storage = {0};
    const DvzSceneMsaaTechniqueState* effective_msaa = _scene_composition_msaa_state(
        &render_plan.composition, panel_id, report, &effective_msaa_storage);

    if (render_plan.drawable_count == 0)
    {
        return dvz_frame_plan_clear_panel(plan, panel_id, "rt", panel->desc) &&
               _frame_plan_composition_append(plan, &render_plan.composition, report);
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
    uint32_t surface_resolve_node = invalid_node;
    uint32_t blended_nodes[DVZ_SCENE_MAX_RENDER_VISUALS] = {0};
    bool blended_group_emitted[DVZ_SCENE_MAX_RENDER_VISUALS] = {0};
    uint32_t edl_node = invalid_node;
    uint32_t gtao_node = invalid_node;
    uint32_t gtao_denoise_node = invalid_node;
    uint32_t gtao_denoise_y_node = invalid_node;
    uint32_t gtao_visibility_presentation_node = invalid_node;
    uint32_t presentation_node = invalid_node;
    uint32_t edl_params_node = invalid_node;
    uint32_t gtao_params_node = invalid_node;
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
    const DvzSceneResolvedPass* surface_resolve_pass = NULL;
    if (_scene_composition_pass_for_role(
            &render_plan.composition, DVZ_FRAME_PLAN_RENDER_PASS_SURFACE_RESOLVE, 0,
            &surface_resolve_pass) &&
        !_scene_begin_panel_render_pass(
            plan, panel_id, "rt.surface.resolve", panel->desc,
            DVZ_FRAME_PLAN_RENDER_PASS_SURFACE_RESOLVE, &panel_apply_mvp, &panel_viewport,
            plot_desc, &surface_resolve_node))
        graph_ok = false;

    if (render_plan.ao_enabled && gbuffer_node != invalid_node &&
        render_plan.gbuffer.producer_count > 0)
    {
        if (!_scene_emit_gtao_params_upload(
                plan, panel, panel_id, render_plan.ao_state, &panel_apply_mvp, &panel_viewport,
                &gtao_params_node))
            graph_ok = false;
        if (!_scene_begin_panel_render_pass(
                plan, panel_id, "rt.gtao.raw_visibility", panel->desc,
                DVZ_FRAME_PLAN_RENDER_PASS_GTAO, &panel_apply_mvp, &panel_viewport, plot_desc,
                &gtao_node))
            gtao_node = invalid_node;
        if (render_plan.ao_state->denoise_enabled)
        {
            if (!_scene_begin_panel_render_pass(
                    plan, panel_id, "rt.gtao.denoise.x", panel->desc,
                    DVZ_FRAME_PLAN_RENDER_PASS_GTAO_DENOISE, &panel_apply_mvp, &panel_viewport,
                    plot_desc, &gtao_denoise_node))
                gtao_denoise_node = invalid_node;
            if (!_scene_begin_panel_render_pass(
                    plan, panel_id, "rt.gtao.denoise.y", panel->desc,
                    DVZ_FRAME_PLAN_RENDER_PASS_GTAO_DENOISE, &panel_apply_mvp, &panel_viewport,
                    plot_desc, &gtao_denoise_y_node))
                gtao_denoise_y_node = invalid_node;
        }
        if (gtao_node == invalid_node ||
            (render_plan.ao_state->denoise_enabled &&
             (gtao_denoise_node == invalid_node || gtao_denoise_y_node == invalid_node)))
            graph_ok = false;
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

    if (render_plan.ao_enabled && render_plan.ao_state != NULL &&
        render_plan.ao_state->debug_mode == DVZ_AO_DEBUG_VISIBILITY)
    {
        if (!_scene_begin_panel_render_pass(
                plan, panel_id, "rt", panel->desc,
                DVZ_FRAME_PLAN_RENDER_PASS_GTAO_VISIBILITY_PRESENTATION, &panel_apply_mvp,
                &panel_viewport, plot_desc, &gtao_visibility_presentation_node))
            graph_ok = false;
    }

    for (uint32_t pass_idx = 0; pass_idx < render_plan.transparent_pass_count; pass_idx++)
    {
        const DvzPanelRenderTransparentPassPlan* transparent_pass =
            &render_plan.transparent_passes[pass_idx];
        const uint32_t group = transparent_pass->index;
        if (transparent_pass->kind == DVZ_PANEL_RENDER_TRANSPARENT_DEPTH_PEEL)
        {
            if (!_scene_emit_depth_peel_group_nodes(
                    figure, plan, panel, &render_plan, &panel_apply_mvp, &panel_viewport,
                    plot_desc, group, report))
                graph_ok = false;
            continue;
        }
        if (transparent_pass->kind == DVZ_PANEL_RENDER_TRANSPARENT_WBOIT)
        {
            if (!_scene_emit_wboit_group_nodes(
                    figure, plan, panel, &render_plan, &panel_apply_mvp, &panel_viewport,
                    plot_desc, group, report))
                graph_ok = false;
            continue;
        }
        if (group >= render_plan.blended_group_count || blended_group_emitted[group])
            continue;
        if (!_scene_emit_blended_group_node(
                figure, plan, panel, &render_plan, &panel_apply_mvp, &panel_viewport, plot_desc,
                group, blended_nodes, report))
            graph_ok = false;
        blended_group_emitted[group] = true;
    }

    if (render_plan.edl_enabled && render_plan.edl_has_depth_producer)
    {
        if (!_scene_emit_edl_params_upload(
                plan, panel, panel_id, render_plan.edl_state, &panel_apply_mvp, &panel_viewport,
                &edl_params_node))
            graph_ok = false;
        if (!_scene_begin_panel_render_pass(
                plan, panel_id, "rt", panel->desc, DVZ_FRAME_PLAN_RENDER_PASS_EDL_RESOLVE,
                &panel_apply_mvp, &panel_viewport, plot_desc, &edl_node))
            graph_ok = false;
    }

    const DvzSceneResolvedPass* presentation_pass = NULL;
    if (_scene_composition_pass_for_role(
            &render_plan.composition, DVZ_FRAME_PLAN_RENDER_PASS_PRESENTATION, 0,
            &presentation_pass) &&
        !_scene_begin_panel_render_pass(
            plan, panel_id, "rt", panel->desc, DVZ_FRAME_PLAN_RENDER_PASS_PRESENTATION,
            &panel_apply_mvp, &panel_viewport, plot_desc, &presentation_node))
        graph_ok = false;
    if (graph_ok && edl_params_node != invalid_node &&
        !_scene_bind_auxiliary_upload(
            &render_plan.composition, DVZ_SCENE_AUXILIARY_EDL_PARAMS, edl_params_node))
        graph_ok = false;
    if (graph_ok && gtao_params_node != invalid_node &&
        !_scene_bind_auxiliary_upload(
            &render_plan.composition, DVZ_SCENE_AUXILIARY_GTAO_PARAMS, gtao_params_node))
        graph_ok = false;
    if (graph_ok && (edl_params_node != invalid_node || gtao_params_node != invalid_node))
    {
        render_plan.composition.work_declaration_fingerprint =
            _frame_plan_composition_work_fingerprint(&render_plan.composition);
        graph_ok = _frame_plan_composition_validate(&render_plan.composition, report);
    }
    if (graph_ok && !_scene_panel_composition_lower_graph(plan, &render_plan.composition, report))
        graph_ok = false;
    if (graph_ok &&
        !_scene_bind_panel_composition(plan, panel_id, &render_plan.composition, report))
        graph_ok = false;
    if (graph_ok && !_frame_plan_composition_append(plan, &render_plan.composition, report))
        graph_ok = false;
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
