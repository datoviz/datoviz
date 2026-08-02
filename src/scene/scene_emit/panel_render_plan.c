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

#include <math.h>
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
    uint32_t visual_index, uint32_t authored_order, const DvzSceneVisualPassCaps* caps)
{
    ANN(dst);
    ANN(count);
    ANN(visual);
    ANN(attach);
    ANN(caps);
    if (*count >= DVZ_SCENE_MAX_VISUALS)
        return false;
    dst[*count] = (DvzPanelRenderVisualPlan){
        .visual = visual,
        .attach = attach,
        .visual_index = visual_index,
        .blend_group = DVZ_PANEL_RENDER_INVALID_INDEX,
        .authored_order = authored_order,
        .layer = caps->layer,
        .caps = *caps,
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
    uint32_t authored_order, const DvzSceneVisualPassCaps* caps)
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
        if (!start_blended_pass && out->transparent_pass_count > 0)
            start_blended_pass = out->transparent_passes[out->transparent_pass_count - 1].kind !=
                                 DVZ_PANEL_RENDER_TRANSPARENT_BLENDED;
        if (!start_blended_pass)
        {
            uint32_t prev = out->blended_group_count - 1;
            start_blended_pass = out->blended_writes_depth[prev] != draw_writes_depth;
            if (!start_blended_pass && out->blended_visual_count > 0)
            {
                DvzSceneVisualLayer previous_layer =
                    out->blended_visuals[out->blended_visual_count - 1].layer;
                bool previous_overlay = previous_layer == DVZ_SCENE_VISUAL_LAYER_OVERLAY;
                bool current_overlay = caps->layer == DVZ_SCENE_VISUAL_LAYER_OVERLAY;
                start_blended_pass = previous_overlay != current_overlay;
            }
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
                out->blended_visuals, &out->blended_visual_count, visual, attach, visual_index,
                authored_order, caps))
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
        if (out->depth_peel_visual_count > 0 && out->transparent_pass_count > 0 &&
            out->transparent_passes[out->transparent_pass_count - 1].kind !=
                DVZ_PANEL_RENDER_TRANSPARENT_DEPTH_PEEL)
            out->unsupported_noncontiguous_oit = true;
        if (out->depth_peel_visual_count == 0 &&
            !_scene_panel_render_plan_append_transparent_pass(
                out, DVZ_PANEL_RENDER_TRANSPARENT_DEPTH_PEEL, 0))
            return false;
        if (!_scene_panel_render_plan_append(
                out->depth_peel_visuals, &out->depth_peel_visual_count, visual, attach,
                visual_index, authored_order, caps))
            return false;
        out->transparent_needs_depth =
            out->transparent_needs_depth || caps->needs_depth_attachment;
        return true;
    }

    if (out->wboit_visual_count > 0 && out->transparent_pass_count > 0 &&
        out->transparent_passes[out->transparent_pass_count - 1].kind !=
            DVZ_PANEL_RENDER_TRANSPARENT_WBOIT)
        out->unsupported_noncontiguous_oit = true;
    if (out->wboit_visual_count == 0 && !_scene_panel_render_plan_append_transparent_pass(
                                            out, DVZ_PANEL_RENDER_TRANSPARENT_WBOIT, 0))
        return false;
    if (!_scene_panel_render_plan_append(
            out->wboit_visuals, &out->wboit_visual_count, visual, attach, visual_index,
            authored_order, caps))
        return false;
    out->transparent_needs_depth = out->transparent_needs_depth || caps->needs_depth_attachment;
    return true;
}



static uint32_t _scene_panel_render_plan_phase_bucket(
    const DvzPanelRenderPlan* plan, const DvzPanelRenderTransparentPassPlan* pass)
{
    ANN(plan);
    ANN(pass);
    if (pass->kind != DVZ_PANEL_RENDER_TRANSPARENT_BLENDED)
        return 0;
    uint32_t layers = 0;
    for (uint32_t i = 0; i < plan->blended_visual_count; i++)
    {
        const DvzPanelRenderVisualPlan* visual = &plan->blended_visuals[i];
        if (visual->blend_group == pass->index && visual->layer < 32)
            layers |= 1u << (uint32_t)visual->layer;
    }
    const uint32_t overlay = 1u << (uint32_t)DVZ_SCENE_VISUAL_LAYER_OVERLAY;
    if (layers == overlay)
        return 1;
    return 0;
}



static void _scene_panel_render_plan_order_phases(DvzPanelRenderPlan* plan)
{
    ANN(plan);
    DvzPanelRenderTransparentPassPlan ordered[DVZ_SCENE_MAX_RENDER_VISUALS] = {0};
    uint32_t count = 0;
    for (uint32_t bucket = 0; bucket < 2; bucket++)
    {
        for (uint32_t i = 0; i < plan->transparent_pass_count; i++)
        {
            if (_scene_panel_render_plan_phase_bucket(plan, &plan->transparent_passes[i]) ==
                bucket)
                ordered[count++] = plan->transparent_passes[i];
        }
    }
    ASSERT(count == plan->transparent_pass_count);
    dvz_memcpy(
        plan->transparent_passes, sizeof(plan->transparent_passes), ordered,
        sizeof(plan->transparent_passes));
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
static bool _scene_panel_render_plan_build_mutable(
    DvzFigure* figure, uint32_t panel_index, const char* figure_id, DvzPanelRenderPlan* out)
{
    ANN(figure);
    ANN(figure_id);
    ANN(out);
    ASSERT(panel_index < figure->panel_count);
    DvzPanel* panel = &figure->panels[panel_index];
    dvz_snprintf(out->panel_id, sizeof(out->panel_id), "%s_p%u", figure_id, panel_index);
    float panel_x = 0.0f;
    float panel_y = 0.0f;
    float panel_width = 0.0f;
    float panel_height = 0.0f;
    _scene_panel_pixel_rect(panel, &panel_x, &panel_y, &panel_width, &panel_height);
    const float scale_x =
        figure->device_scale_x > 0.0f ? figure->device_scale_x * figure->render_scale : 1.0f;
    const float scale_y =
        figure->device_scale_y > 0.0f ? figure->device_scale_y * figure->render_scale : 1.0f;
    const float x0 = panel_x * scale_x;
    const float y0 = panel_y * scale_y;
    const float x1 = (panel_x + panel_width) * scale_x;
    const float y1 = (panel_y + panel_height) * scale_y;
    out->origin_x = (int32_t)floorf(x0);
    out->origin_y = (int32_t)floorf(y0);
    const int32_t outer_x = (int32_t)ceilf(x1);
    const int32_t outer_y = (int32_t)ceilf(y1);
    out->width = outer_x > out->origin_x ? (uint32_t)(outer_x - out->origin_x) : 0;
    out->height = outer_y > out->origin_y ? (uint32_t)(outer_y - out->origin_y) : 0;
    out->render_scale = figure->render_scale > 0.0f ? figure->render_scale : 1.0f;
    out->local_to_target[0] = 1.0f;
    out->local_to_target[1] = 1.0f;
    out->local_to_target[2] = (float)out->origin_x;
    out->local_to_target[3] = (float)out->origin_y;
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

        DvzSceneVisualPassCaps caps = {0};
        if (!_scene_visual_pass_caps_from_visual(visual, attach, &caps))
            continue;
        if (!_scene_panel_render_plan_append(
                out->visuals, &out->visual_count, visual, attach, vidx, k, &caps))
            return false;

        if (out->scene_occlusion_enabled && visual->scene_occluder)
        {
            if (!_scene_panel_render_plan_append(
                    out->scene_occlusion, &out->scene_occlusion_count, visual, attach, vidx, k,
                    &caps))
                return false;
        }

        if (!caps.draws_in_opaque_pass)
        {
            if (!_scene_panel_render_plan_classify_transparent(
                    out, visual, attach, vidx, k, &caps))
                return false;
            continue;
        }

        if (out->gbuffer_required &&
            _scene_technique_gbuffer_plan_add_visual(&out->gbuffer, visual, attach))
        {
            if (!_scene_panel_render_plan_append(
                    out->gbuffer_visuals, &out->gbuffer_visual_count, visual, attach, vidx, k,
                    &caps))
                return false;
        }

        if (!_scene_panel_render_plan_append(
                out->opaque_visuals, &out->opaque_visual_count, visual, attach, vidx, k, &caps))
            return false;
        bool edl_depth_visual = out->edl_enabled && caps.eligible_for_depth_postprocess;
        out->opaque_needs_depth = out->opaque_needs_depth || caps.writes_depth || edl_depth_visual;
        out->edl_has_depth_producer = out->edl_has_depth_producer || edl_depth_visual;
    }

    _scene_panel_render_plan_order_phases(out);

    return true;
}



/**
 * Build and atomically publish one immutable panel composition snapshot.
 *
 * @param figure the parent figure
 * @param panel_index the panel index within the figure
 * @param figure_id the stable figure identifier
 * @param caps the active capability snapshot
 * @param report optional diagnostic report
 * @param out output panel render plan, unchanged on failure
 * @return whether planning and composition succeeded
 */
bool _scene_panel_render_plan_build(
    DvzFigure* figure, uint32_t panel_index, const char* figure_id,
    const DvzCapabilitySnapshot* caps, DvzDiagnosticReport* report, DvzPanelRenderPlan* out)
{
    ANN(figure);
    ANN(figure_id);
    ANN(out);

    DvzPanelRenderPlan draft = {0};
    if (!_scene_panel_render_plan_build_mutable(figure, panel_index, figure_id, &draft))
        return false;
    if (!_scene_panel_composition_resolve(&draft, caps, &draft.composition, report))
        return false;
    *out = draft;
    return true;
}
