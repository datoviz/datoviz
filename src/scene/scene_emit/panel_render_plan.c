/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene panel render planning                                                                  */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <string.h>

#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_visual_internal.h"
#include "_visual_pipeline.h"
#include "registry/registry.h"
#include "render_contract/render_contract.h"
#include "scene_emit/panel_render_plan.h"
#include "scene_emit/visual_lowering.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return the draw-position attribute used to decide whether one visual can render.
 *
 * @param visual the visual
 * @return the position-like attribute name
 */
const char* _scene_panel_render_visual_draw_position_attr(const DvzVisual* visual)
{
    ANN(visual);
    DvzVisualLowering lowering = {0};
    if (!_scene_visual_lowering_resolve(visual, &lowering) || lowering.draw_position_attr == NULL)
        return "position";
    return lowering.draw_position_attr;
}



/**
 * Return whether one panel visual is visible and drawable.
 *
 * @param visual the visual
 * @return whether the visual has position data
 */
bool _scene_panel_render_visual_is_visible_drawable(const DvzVisual* visual)
{
    if (visual == NULL || !visual->visible)
        return false;
    if (visual->ops != NULL && visual->ops->skip_visual_uploads)
        return false;
    int pos_idx = _attr_index(visual, _scene_panel_render_visual_draw_position_attr(visual));
    return pos_idx >= 0 && visual->attrs[pos_idx].item_count > 0;
}



/**
 * Count visible drawable visuals attached to one panel.
 *
 * @param figure the parent figure
 * @param panel the panel
 * @return number of visuals with drawable position data
 */
static uint32_t _scene_panel_drawable_visual_count(const DvzFigure* figure, const DvzPanel* panel)
{
    ANN(figure);
    ANN(panel);
    uint32_t count = 0;
    for (uint32_t vi = 0; vi < panel->visual_count; vi++)
    {
        const DvzVisual* visual = panel->visuals[vi].visual;
        if (visual == NULL || !visual->visible)
            continue;
        if (visual->ops != NULL && visual->ops->skip_visual_uploads)
            continue;
        uint32_t visual_index = 0;
        if (!_figure_visual_index(figure, visual, &visual_index))
            continue;
        const char* position_attr = _scene_panel_render_visual_draw_position_attr(visual);
        int pos_idx = _attr_index(visual, position_attr);
        if (pos_idx >= 0 && visual->attrs[pos_idx].item_count > 0)
        {
            count++;
            continue;
        }
        log_warn(
            "%s visual (index %u) has no '%s' data — it will render nothing",
            _visual_type_name(visual->type), visual_index, position_attr);
    }
    return count;
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
        if (!_scene_panel_render_visual_is_visible_drawable(visual) ||
            !_scene_visual_lowering_volume_occluded(visual) ||
            visual == panel->volume_occluder_visual)
            continue;
        return true;
    }
    return false;
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
        if (!_scene_panel_render_visual_is_visible_drawable(visual))
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



static bool _scene_panel_render_plan_append(
    DvzPanelRenderVisualPlan* dst, uint32_t* count, DvzVisual* visual, DvzPanelAttach* attach,
    uint32_t visual_index)
{
    ANN(dst);
    ANN(count);
    ANN(visual);
    ANN(attach);
    if (*count >= DVZ_SCENE_MAX_VISUALS)
        return false;
    dst[*count] = (DvzPanelRenderVisualPlan){
        .visual = visual,
        .attach = attach,
        .visual_index = visual_index,
        .blend_group = DVZ_PANEL_RENDER_INVALID_INDEX,
    };
    (*count)++;
    return true;
}


static bool _scene_panel_render_plan_append_transparent_pass(
    DvzPanelRenderPlan* out, DvzPanelRenderTransparentKind kind, uint32_t index)
{
    ANN(out);
    if (out->transparent_pass_count >= DVZ_SCENE_MAX_RENDER_VISUALS)
        return false;
    out->transparent_passes[out->transparent_pass_count++] =
        (DvzPanelRenderTransparentPassPlan){.kind = kind, .index = index};
    return true;
}



static bool _scene_panel_render_plan_classify_transparent(
    DvzPanelRenderPlan* out, DvzVisual* visual, DvzPanelAttach* attach, uint32_t visual_index,
    const DvzSceneVisualPassCaps* caps)
{
    ANN(out);
    ANN(visual);
    ANN(attach);
    ANN(caps);

    out->has_transparent = true;
    DvzFramePlanRenderPassRole pass_role = DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND;
    if (caps->draws_in_wboit_pass)
        pass_role = DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION;
    else if (caps->draws_in_depth_peel_pass)
        pass_role = DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT;

    DvzSceneDrawContract draw_contract = {0};
    bool has_contract =
        _scene_draw_contract_from_visual(visual, attach, pass_role, &draw_contract);
    bool draw_needs_depth = false;
    bool draw_writes_depth = false;
    if (has_contract)
    {
        draw_needs_depth = _scene_draw_contract_needs_depth(&draw_contract);
        draw_writes_depth = (draw_contract.depth_policy & DVZ_SCENE_DEPTH_POLICY_WRITE) != 0;
        out->transparent_needs_depth = out->transparent_needs_depth || draw_needs_depth;
    }
    else
    {
        out->transparent_needs_depth =
            out->transparent_needs_depth || caps->needs_depth_attachment;
    }

    if (caps->draws_in_transparent_blend_pass)
    {
        bool start_blended_pass = out->blended_group_count == 0;
        if (!start_blended_pass)
        {
            uint32_t prev = out->blended_group_count - 1;
            start_blended_pass = out->blended_writes_depth[prev] != draw_writes_depth;
        }
        if (start_blended_pass)
        {
            if (out->blended_group_count >= DVZ_SCENE_MAX_RENDER_VISUALS)
                return false;
            if (!_scene_panel_render_plan_append_transparent_pass(
                    out, DVZ_PANEL_RENDER_TRANSPARENT_BLENDED, out->blended_group_count))
                return false;
            out->blended_group_count++;
        }
        if (!_scene_panel_render_plan_append(
                out->blended_visuals, &out->blended_visual_count, visual, attach, visual_index))
            return false;
        uint32_t group = out->blended_group_count - 1;
        DvzPanelRenderVisualPlan* planned = &out->blended_visuals[out->blended_visual_count - 1];
        planned->blend_group = group;
        planned->needs_depth = draw_needs_depth;
        planned->writes_depth = draw_writes_depth;
        out->blended_needs_depth[group] = out->blended_needs_depth[group] || draw_needs_depth;
        out->blended_writes_depth[group] = out->blended_writes_depth[group] || draw_writes_depth;
        return true;
    }

    if (caps->draws_in_depth_peel_pass)
    {
        if (out->depth_peel_visual_count == 0 &&
            !_scene_panel_render_plan_append_transparent_pass(
                out, DVZ_PANEL_RENDER_TRANSPARENT_DEPTH_PEEL, 0))
            return false;
        if (!_scene_panel_render_plan_append(
                out->depth_peel_visuals, &out->depth_peel_visual_count, visual, attach,
                visual_index))
            return false;
        out->transparent_needs_depth =
            out->transparent_needs_depth || caps->needs_depth_attachment;
        return true;
    }

    if (out->wboit_visual_count == 0 && !_scene_panel_render_plan_append_transparent_pass(
                                            out, DVZ_PANEL_RENDER_TRANSPARENT_WBOIT, 0))
        return false;
    if (!_scene_panel_render_plan_append(
            out->wboit_visuals, &out->wboit_visual_count, visual, attach, visual_index))
        return false;
    out->transparent_needs_depth = out->transparent_needs_depth || caps->needs_depth_attachment;
    return true;
}



/**
 * Build the render-policy plan consumed by panel emission.
 *
 * @param figure the parent figure
 * @param panel_index the panel index within the figure
 * @param figure_id the stable figure identifier
 * @param out output panel render plan
 * @return whether the plan was built
 */
bool _scene_panel_render_plan_build(
    DvzFigure* figure, uint32_t panel_index, const char* figure_id, DvzPanelRenderPlan* out)
{
    ANN(figure);
    ANN(figure_id);
    ANN(out);
    ASSERT(panel_index < figure->panel_count);
    memset(out, 0, sizeof(*out));

    DvzPanel* panel = &figure->panels[panel_index];
    dvz_snprintf(out->panel_id, sizeof(out->panel_id), "%s_p%u", figure_id, panel_index);
    out->drawable_count = _scene_panel_drawable_visual_count(figure, panel);
    _scene_panel_visual_order(panel, out->order);

    _scene_technique_gbuffer_plan_init(&out->gbuffer);
    out->scene_occlusion_enabled = _scene_panel_has_visible_scene_occlusion_target(panel);
    out->volume_occlusion_enabled = _scene_panel_has_visible_volume_occlusion_target(panel);
    out->gbuffer_enabled = _scene_technique_gbuffer_enabled(figure->scene, panel);
    out->ssao_state = _scene_technique_ssao_state(figure->scene, panel);
    out->msaa_state = _scene_technique_msaa_state(figure->scene, panel);
    out->edl_state = _scene_technique_edl_state(figure->scene, panel);
    out->ssao_enabled = out->ssao_state != NULL && out->ssao_state->enabled;
    out->gbuffer_required = out->gbuffer_enabled || out->ssao_enabled;
    out->edl_enabled = out->edl_state != NULL && out->edl_state->enabled;

    if (out->volume_occlusion_enabled)
    {
        uint32_t occluder_index = 0;
        if (_figure_visual_index(figure, panel->volume_occluder_visual, &occluder_index))
        {
            out->has_volume_occluder = true;
            out->volume_occluder_visual_index = occluder_index;
            out->volume_occluder_attach = (DvzPanelAttach){
                .visual = panel->volume_occluder_visual,
                .z_layer = 0,
                .controller_mode = DVZ_CONTROLLER_APPLY,
            };
        }
    }

    for (uint32_t k = 0; k < panel->visual_count; k++)
    {
        uint32_t vi = out->order[k];
        DvzPanelAttach* attach = &panel->visuals[vi];
        DvzVisual* visual = attach->visual;
        if (!_scene_panel_render_visual_is_visible_drawable(visual))
            continue;
        uint32_t vidx = 0;
        if (!_figure_visual_index(figure, visual, &vidx))
            continue;

        if (out->scene_occlusion_enabled && visual->scene_occluder)
        {
            if (!_scene_panel_render_plan_append(
                    out->scene_occlusion, &out->scene_occlusion_count, visual, attach, vidx))
                return false;
        }

        DvzSceneVisualPassCaps caps = {0};
        if (!_scene_visual_pass_caps_from_visual(visual, attach, &caps))
            continue;
        if (!caps.draws_in_opaque_pass)
        {
            if (!_scene_panel_render_plan_classify_transparent(out, visual, attach, vidx, &caps))
                return false;
            continue;
        }

        if (out->gbuffer_required &&
            _scene_technique_gbuffer_plan_add_visual(&out->gbuffer, visual, attach))
        {
            if (!_scene_panel_render_plan_append(
                    out->gbuffer_visuals, &out->gbuffer_visual_count, visual, attach, vidx))
                return false;
        }

        if (!_scene_panel_render_plan_append(
                out->opaque_visuals, &out->opaque_visual_count, visual, attach, vidx))
            return false;
        bool edl_depth_visual = out->edl_enabled && caps.eligible_for_depth_postprocess;
        out->opaque_needs_depth = out->opaque_needs_depth || caps.writes_depth || edl_depth_visual;
        out->edl_has_depth_producer = out->edl_has_depth_producer || edl_depth_visual;
    }

    return true;
}
