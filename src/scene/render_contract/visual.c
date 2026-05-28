/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene render visual contracts                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "internal.h"

#include <stdlib.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_technique.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Find the panel attachment for a scene-global visual index.
 *
 * @param panel the panel
 * @param visual_index the scene-global visual index
 * @return the panel attachment, or NULL when absent
 */
static const DvzPanelAttach* _panel_attach_from_visual_index(
    const DvzPanel* panel, uint32_t visual_index)
{
    ANN(panel);
    if (panel->figure == NULL || panel->figure->scene == NULL)
        return NULL;

    for (uint32_t i = 0; i < panel->visual_count; i++)
    {
        uint32_t index = 0;
        if (_figure_visual_index(panel->figure, panel->visuals[i].visual, &index) &&
            index == visual_index)
            return &panel->visuals[i];
    }
    return NULL;
}



/**
 * Apply a stored FramePlan draw-contract snapshot to a resolved draw contract.
 *
 * @param meta stored visual metadata snapshot
 * @param draw draw contract to update
 */
static void _contract_apply_draw_metadata(
    const DvzFramePlanVisualMeta* meta, DvzSceneDrawContract* draw)
{
    ANN(meta);
    ANN(draw);
    if (!meta->has_draw_contract)
        return;

    draw->depth_policy = meta->draw_depth_policy;
    draw->blend_policy = (DvzSceneBlendPolicy)meta->draw_blend_policy;
    _draw_blend_target_contracts(
        draw->blend_policy, draw->blend_targets, &draw->blend_target_count);
    DvzSceneDrawFacts facts = {.visual_type = draw->visual_type};
    _draw_raster_state_contract(
        &facts, draw->pass_role, &draw->has_raster_state, &draw->cull_mode,
        &draw->front_face);
    draw->shader_feature_mask = meta->draw_shader_feature_mask;
    draw->bind_group_layout_mask = meta->draw_bind_group_layout_mask;

    draw->depth_test = (draw->depth_policy & DVZ_SCENE_DEPTH_POLICY_TEST) != 0;
    draw->depth_write = (draw->depth_policy & DVZ_SCENE_DEPTH_POLICY_WRITE) != 0;
    draw->samples_depth = (draw->depth_policy & DVZ_SCENE_DEPTH_POLICY_SAMPLE) != 0;
    draw->samples_volume_occlusion =
        (draw->shader_feature_mask & DVZ_SCENE_SHADER_FEATURE_SAMPLE_VOLUME_OCCLUSION) != 0;
    draw->samples_scene_occlusion =
        (draw->shader_feature_mask & DVZ_SCENE_SHADER_FEATURE_SAMPLE_SCENE_OCCLUSION) != 0;
    draw->writes_volume_occlusion_depth =
        (draw->shader_feature_mask & DVZ_SCENE_SHADER_FEATURE_WRITE_VOLUME_OCCLUSION) != 0;
    draw->writes_scene_occlusion_depth =
        (draw->shader_feature_mask & DVZ_SCENE_SHADER_FEATURE_WRITE_SCENE_OCCLUSION) != 0;

    draw->needs_common_set =
        (draw->bind_group_layout_mask & DVZ_SCENE_BIND_GROUP_REQUIREMENT_COMMON) != 0;
    draw->needs_material_set =
        (draw->bind_group_layout_mask & DVZ_SCENE_BIND_GROUP_REQUIREMENT_MATERIAL) != 0;
    draw->needs_image_set =
        (draw->bind_group_layout_mask & DVZ_SCENE_BIND_GROUP_REQUIREMENT_IMAGE) != 0;
    draw->needs_labels_set =
        (draw->bind_group_layout_mask & DVZ_SCENE_BIND_GROUP_REQUIREMENT_LABELS) != 0;
    draw->needs_glyph_set =
        (draw->bind_group_layout_mask & DVZ_SCENE_BIND_GROUP_REQUIREMENT_GLYPH) != 0;
    draw->needs_volume_set =
        (draw->bind_group_layout_mask & DVZ_SCENE_BIND_GROUP_REQUIREMENT_VOLUME) != 0;
    draw->needs_scene_occlusion_set =
        (draw->bind_group_layout_mask & DVZ_SCENE_BIND_GROUP_REQUIREMENT_SCENE_OCCLUSION) != 0;
    dvz_strlcpy(
        draw->volume_occlusion_resource_id, meta->draw_volume_occlusion_resource_id,
        sizeof(draw->volume_occlusion_resource_id));
    dvz_strlcpy(
        draw->volume_occlusion_producer_pass_id,
        meta->draw_volume_occlusion_producer_pass_id,
        sizeof(draw->volume_occlusion_producer_pass_id));
    draw->volume_occlusion_bind_set = meta->draw_volume_occlusion_bind_set;
    draw->volume_occlusion_bind_binding = meta->draw_volume_occlusion_bind_binding;
    dvz_strlcpy(
        draw->scene_occlusion_resource_id, meta->draw_scene_occlusion_resource_id,
        sizeof(draw->scene_occlusion_resource_id));
    dvz_strlcpy(
        draw->scene_occlusion_producer_pass_id, meta->draw_scene_occlusion_producer_pass_id,
        sizeof(draw->scene_occlusion_producer_pass_id));
    draw->scene_occlusion_bind_set = meta->draw_scene_occlusion_bind_set;
    draw->scene_occlusion_bind_binding = meta->draw_scene_occlusion_bind_binding;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return the figure panel that owns one render node.
 *
 * @param figure the figure
 * @param plan the FramePlan
 * @param render the render node
 * @return the panel, or NULL when no panel id matches
 */
const DvzPanel* _contract_panel_for_render(
    const DvzFigure* figure, const DvzFramePlan* plan, const DvzFramePlanNode* render)
{
    ANN(figure);
    ANN(plan);
    ANN(render);
    char panel_id[DVZ_SCENE_LABEL_SIZE];
    for (uint32_t i = 0; i < figure->panel_count; i++)
    {
        dvz_snprintf(panel_id, sizeof(panel_id), "%s_p%u", plan->figure_id, i);
        if (strcmp(panel_id, render->u.render.panel_id) == 0)
            return &figure->panels[i];
    }

    const char* suffix = strrchr(render->u.render.panel_id, '_');
    if (suffix != NULL && suffix[1] == 'p')
    {
        char* end = NULL;
        unsigned long index = strtoul(&suffix[2], &end, 10);
        if (end != &suffix[2] && *end == '\0' && index < figure->panel_count)
            return &figure->panels[index];
    }
    return NULL;
}



/**
 * Resolve one FramePlan render pass into a passive pass contract.
 *
 * @param panel the panel that owns the render pass
 * @param render the FramePlan render node
 * @param graph_pass the matching graph pass, or NULL when none was emitted
 * @param caps the active capability snapshot, or NULL to preserve requested sample counts
 * @param out the output pass contract
 * @return whether the pass contract was resolved
 */
bool _scene_pass_contract_from_render_ex(
    const DvzFramePlan* plan, const DvzPanel* panel, const DvzFramePlanNode* render,
    const DvzFrameGraphPass* graph_pass, const DvzCapabilitySnapshot* caps,
    DvzScenePassContract* out)
{
    ANN(plan);
    ANN(panel);
    ANN(render);
    ANN(out);
    if (render->type != DVZ_FRAME_PLAN_NODE_RENDER)
        return false;
    dvz_memset(out, sizeof(DvzScenePassContract), 0, sizeof(DvzScenePassContract));

    out->kind = DVZ_SCENE_PASS_KIND_RASTER;
    out->role = render->u.render.pass_role;
    dvz_strlcpy(out->panel_id, render->u.render.panel_id, sizeof(out->panel_id));
    if (graph_pass != NULL)
        dvz_strlcpy(out->id, graph_pass->id, sizeof(out->id));
    else
        dvz_strlcpy(out->id, render->u.render.render_target_id, sizeof(out->id));

    DvzSceneTechniquePassPolicy policy = {0};
    if (!_scene_technique_pass_policy(out->role, &policy))
        return false;
    out->source_over_blend = policy.source_over_blend;
    out->wboit_accumulation = policy.wboit_accumulation;
    out->depth_peel = policy.depth_peel;
    out->fullscreen_resolve = policy.fullscreen_resolve;
    out->needs_wboit_resolve_layout = policy.needs_wboit_resolve_layout;
    out->needs_depth_peel_sampled_layout = policy.needs_depth_peel_sampled_layout;
    out->sampled_texture_binding_count = policy.sampled_texture_binding_count;

    for (uint32_t i = 0; i < render->u.render.visual_count; i++)
    {
        if (out->draw_count >= DVZ_SCENE_MAX_RENDER_VISUALS)
            return false;

        const DvzFramePlanVisualMeta* meta = &render->u.render.visual_metadata[i];
        if (!meta->has_metadata)
            continue;
        const DvzPanelAttach* attach = _panel_attach_from_visual_index(panel, meta->visual_index);
        if (attach == NULL || attach->visual == NULL)
            return false;
        if (!_scene_draw_contract_from_visual(
                attach->visual, attach, out->role, &out->draws[out->draw_count]))
            return false;
        _contract_apply_draw_metadata(meta, &out->draws[out->draw_count]);
        const DvzSceneDrawContract* draw = &out->draws[out->draw_count];
        out->needs_common_set = out->needs_common_set || draw->needs_common_set;
        out->needs_material_set = out->needs_material_set || draw->needs_material_set;
        out->needs_image_set = out->needs_image_set || draw->needs_image_set;
        out->needs_labels_set = out->needs_labels_set || draw->needs_labels_set;
        out->needs_glyph_set = out->needs_glyph_set || draw->needs_glyph_set;
        out->needs_volume_set = out->needs_volume_set || draw->needs_volume_set;
        out->needs_scene_occlusion_set =
            out->needs_scene_occlusion_set || draw->needs_scene_occlusion_set;
        out->draw_count++;
    }

    if (graph_pass != NULL)
    {
        for (uint32_t i = 0; i < graph_pass->color_attachment_count; i++)
        {
            if (!_contract_append_color_attachment(
                    plan, out, &graph_pass->color_attachments[i], caps))
                return false;
        }
        if (graph_pass->has_depth_attachment &&
            !_contract_append_depth_attachment(plan, out, &graph_pass->depth_attachment, caps))
            return false;
        for (uint32_t i = 0; i < graph_pass->read_count; i++)
        {
            if (!_contract_append_read(plan, out, graph_pass->id, &graph_pass->reads[i], caps))
                return false;
        }
    }
    return true;
}



/**
 * Resolve one FramePlan render pass into a passive pass contract.
 *
 * @param panel the panel that owns the render pass
 * @param render the FramePlan render node
 * @param graph_pass the matching graph pass, or NULL when none was emitted
 * @param out the output pass contract
 * @return whether the pass contract was resolved
 */
bool _scene_pass_contract_from_render(
    const DvzFramePlan* plan, const DvzPanel* panel, const DvzFramePlanNode* render,
    const DvzFrameGraphPass* graph_pass, DvzScenePassContract* out)
{
    return _scene_pass_contract_from_render_ex(plan, panel, render, graph_pass, NULL, out);
}
