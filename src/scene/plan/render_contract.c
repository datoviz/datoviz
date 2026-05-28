/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "render_contract_internal.h"

#include <stdlib.h>
#include <string.h>

#include <vulkan/vulkan_core.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_scene_resource_key.h"
#include "_technique.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Validate technique-specific attachment facts for one pass contract.
 *
 * @param contract the pass contract
 * @param report optional diagnostic report
 * @return whether technique-specific facts are internally consistent
 */
static bool _scene_pass_contract_validate_technique(
    const DvzScenePassContract* contract, DvzDiagnosticReport* report)
{
    ANN(contract);
    bool ok = true;
    const DvzSceneAttachmentUse* attachment = NULL;

    switch (contract->role)
    {
    case DVZ_FRAME_PLAN_RENDER_PASS_VOLUME_OCCLUSION:
        attachment = _contract_attachment_suffix(
            contract, DVZ_SCENE_ATTACHMENT_COLOR, ".volume_occlusion.depth");
        if (contract->color_attachment_count != 1 || attachment == NULL ||
            attachment->format != VK_FORMAT_R32_SFLOAT || !attachment->write ||
            !attachment->clear)
        {
            _contract_report(report, "volume occlusion pass has invalid output attachment");
            ok = false;
        }
        break;

    case DVZ_FRAME_PLAN_RENDER_PASS_SCENE_OCCLUSION:
        attachment = _contract_attachment_suffix(
            contract, DVZ_SCENE_ATTACHMENT_COLOR, ".scene_occlusion.depth");
        if (contract->color_attachment_count != 1 || attachment == NULL ||
            attachment->format != VK_FORMAT_R32_SFLOAT || !attachment->write ||
            !attachment->clear)
        {
            _contract_report(report, "scene occlusion pass has invalid color attachment");
            ok = false;
        }
        attachment = _contract_attachment_suffix(
            contract, DVZ_SCENE_ATTACHMENT_DEPTH, ".scene_occlusion.z");
        if (attachment == NULL || attachment->format != VK_FORMAT_D32_SFLOAT ||
            !attachment->write || !attachment->clear)
        {
            _contract_report(report, "scene occlusion pass has invalid depth attachment");
            ok = false;
        }
        break;

    case DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION:
        attachment = _contract_attachment_suffix(
            contract, DVZ_SCENE_ATTACHMENT_COLOR, ".wboit.accum");
        if (attachment == NULL || attachment->format != VK_FORMAT_R16G16B16A16_SFLOAT ||
            attachment->sample_count != 1)
        {
            _contract_report(report, "WBOIT accumulation pass has invalid accumulation target");
            ok = false;
        }
        attachment = _contract_attachment_suffix(
            contract, DVZ_SCENE_ATTACHMENT_COLOR, ".wboit.weight");
        if (attachment == NULL || attachment->format != VK_FORMAT_R16_SFLOAT ||
            attachment->sample_count != 1)
        {
            _contract_report(report, "WBOIT accumulation pass has invalid weight target");
            ok = false;
        }
        if (contract->color_attachment_count != 2)
        {
            _contract_report(report, "WBOIT accumulation pass must have two color attachments");
            ok = false;
        }
        if (_contract_needs_depth(contract) && !contract->has_depth_attachment)
        {
            _contract_report(report, "WBOIT accumulation pass is missing required depth");
            ok = false;
        }
        break;

    case DVZ_FRAME_PLAN_RENDER_PASS_WBOIT_RESOLVE:
        if (contract->draw_count != 0 || contract->color_attachment_count != 1 ||
            contract->sampled_read_count != 2 || !contract->needs_wboit_resolve_layout ||
            contract->sampled_texture_binding_count != 2)
        {
            _contract_report(report, "WBOIT resolve pass has invalid attachment shape");
            ok = false;
        }
        break;

    case DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT:
    case DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER:
        for (uint32_t i = 0; i < contract->attachment_count; i++)
        {
            attachment = &contract->attachments[i];
            if (attachment->role == DVZ_SCENE_ATTACHMENT_COLOR &&
                attachment->format != VK_FORMAT_R16G16B16A16_SFLOAT)
            {
                _contract_report(report, "depth peel color attachment has invalid format");
                ok = false;
            }
        }
        if (contract->color_attachment_count != 3)
        {
            _contract_report(report, "depth peel raster pass must have three color attachments");
            ok = false;
        }
        if (contract->role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER &&
            contract->sampled_read_count != 1)
        {
            _contract_report(report, "depth peel iteration pass must sample previous bounds");
            ok = false;
        }
        if (_contract_needs_depth(contract) && !contract->has_depth_attachment)
        {
            _contract_report(report, "depth peel raster pass is missing required depth");
            ok = false;
        }
        break;

    case DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_COMPOSITE:
        if (contract->draw_count != 0 || contract->color_attachment_count != 1 ||
            contract->sampled_read_count != 2 || !contract->needs_depth_peel_sampled_layout ||
            contract->sampled_texture_binding_count != 2)
        {
            _contract_report(report, "depth peel composite pass has invalid attachment shape");
            ok = false;
        }
        break;

    default:
        break;
    }

    return ok;
}



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
 * Return the graph pass matching one render node.
 *
 * @param plan the FramePlan
 * @param render the render node
 * @return the graph pass, or NULL when the render node has no graph pass
 */
const DvzFrameGraphPass* _contract_graph_pass_for_render(
    const DvzFramePlan* plan, const DvzFramePlanNode* render)
{
    ANN(plan);
    ANN(render);
    if (render->type != DVZ_FRAME_PLAN_NODE_RENDER)
        return NULL;
    const char* work_label = _scene_render_role_work_label(render->u.render.pass_role);
    if (work_label[0] == '\0')
        return NULL;

    uint32_t ordinal = 0;
    for (uint32_t i = 0; i < plan->count; i++)
    {
        const DvzFramePlanNode* candidate = &plan->nodes[i];
        if (candidate == render)
            break;
        if (candidate->type != DVZ_FRAME_PLAN_NODE_RENDER)
            continue;
        const char* candidate_label =
            _scene_render_role_work_label(candidate->u.render.pass_role);
        if (candidate_label[0] != '\0' &&
            strcmp(candidate->u.render.panel_id, render->u.render.panel_id) == 0 &&
            strcmp(candidate_label, work_label) == 0)
            ordinal++;
    }

    uint32_t seen = 0;
    for (uint32_t i = 0; i < dvz_frame_plan_graph_pass_count(plan); i++)
    {
        const DvzFrameGraphPass* pass = dvz_frame_plan_graph_pass_get(plan, i);
        if (pass == NULL || strcmp(pass->panel_id, render->u.render.panel_id) != 0 ||
            strcmp(pass->work_label, work_label) != 0)
            continue;
        if (seen == ordinal)
            return pass;
        seen++;
    }
    return NULL;
}



/**
 * Validate that every graph-backed render node has a matching graph pass.
 *
 * @param plan the FramePlan to inspect
 * @param report optional diagnostic report
 * @return whether every graph-backed render role has a graph pass
 */
bool _contract_validate_graph_backed_render_nodes(
    const DvzFramePlan* plan, DvzDiagnosticReport* report)
{
    ANN(plan);
    bool ok = true;
    for (uint32_t i = 0; i < plan->count; i++)
    {
        const DvzFramePlanNode* render = &plan->nodes[i];
        if (render->type != DVZ_FRAME_PLAN_NODE_RENDER)
            continue;
        if (!_scene_render_role_requires_graph_pass(render->u.render.pass_role))
            continue;
        if (_contract_graph_pass_for_render(plan, render) != NULL)
            continue;
        _contract_report(report, "graph-backed render node has no matching graph pass");
        ok = false;
    }
    return ok;
}



/**
 * Return the figure panel that owns one render node.
 *
 * @param figure the figure
 * @param plan the FramePlan
 * @param render the render node
 * @return the panel, or NULL when no panel id matches
 */
static const DvzPanel* _contract_panel_for_render(
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



/**
 * Validate generic invariants for a passive scene pass contract.
 *
 * @param contract the pass contract
 * @param report optional diagnostic report
 * @return whether the contract is internally consistent
 */
bool _scene_pass_contract_validate(
    const DvzScenePassContract* contract, DvzDiagnosticReport* report)
{
    ANN(contract);
    bool ok = true;
    bool needs_depth = false;
    bool samples_depth = false;
    bool samples_volume_occlusion = false;
    bool samples_scene_occlusion = false;

    for (uint32_t i = 0; i < contract->draw_count; i++)
    {
        const DvzSceneDrawContract* draw = &contract->draws[i];
        if (!_draw_pass_role_matches(draw))
        {
            _contract_report(report, "draw alpha mode does not match render pass role");
            ok = false;
        }
        if (contract->source_over_blend && draw->depth_write)
        {
            _contract_report(report, "source-over draw must not write depth");
            ok = false;
        }
        needs_depth = needs_depth || draw->depth_test || draw->depth_write;
        samples_depth = samples_depth || draw->samples_depth;
        if (draw->samples_volume_occlusion)
        {
            const DvzSceneAttachmentUse* use = NULL;
            if (draw->volume_occlusion_resource_id[0] != '\0')
                use = _contract_sampled_resource_use(
                    contract, draw->volume_occlusion_resource_id);
            if (draw->volume_occlusion_resource_id[0] != '\0' && use == NULL)
            {
                _contract_report(
                    report, "volume-occluded draw has no exact volume occlusion read edge");
                ok = false;
            }
            else if (
                use != NULL && draw->volume_occlusion_producer_pass_id[0] != '\0' &&
                strcmp(use->producer_pass_id, draw->volume_occlusion_producer_pass_id) != 0)
            {
                _contract_report(
                    report, "volume-occluded draw producer pass mismatches contract");
                ok = false;
            }
            else if (draw->volume_occlusion_resource_id[0] == '\0')
            {
                samples_volume_occlusion = true;
            }
        }
        if (draw->samples_scene_occlusion)
        {
            const DvzSceneAttachmentUse* use = NULL;
            if (draw->scene_occlusion_resource_id[0] != '\0')
                use = _contract_sampled_resource_use(contract, draw->scene_occlusion_resource_id);
            if (draw->scene_occlusion_resource_id[0] != '\0' && use == NULL)
            {
                _contract_report(
                    report, "scene-occluded draw has no exact scene occlusion read edge");
                ok = false;
            }
            else if (
                use != NULL && draw->scene_occlusion_producer_pass_id[0] != '\0' &&
                strcmp(use->producer_pass_id, draw->scene_occlusion_producer_pass_id) != 0)
            {
                _contract_report(
                    report, "scene-occluded draw producer pass mismatches contract");
                ok = false;
            }
            else if (draw->scene_occlusion_resource_id[0] == '\0')
            {
                samples_scene_occlusion = true;
            }
        }
    }

    if (needs_depth && !_contract_has_depth_attachment(contract))
    {
        _contract_report(report, "depth-capable draw is in a pass without depth attachment");
        ok = false;
    }
    if (samples_depth && !_contract_has_sampled_depth_resource(contract))
    {
        _contract_report(report, "sampled-depth draw has no produced depth resource");
        ok = false;
    }
    if (samples_volume_occlusion &&
        !_contract_reads_resource_suffix(contract, ".volume_occlusion.depth"))
    {
        _contract_report(report, "volume-occluded draw has no volume occlusion read edge");
        ok = false;
    }
    if (samples_scene_occlusion &&
        !_contract_reads_resource_suffix(contract, ".scene_occlusion.depth"))
    {
        _contract_report(report, "scene-occluded draw has no scene occlusion read edge");
        ok = false;
    }
    ok = _scene_pass_contract_validate_technique(contract, report) && ok;
    return ok;
}



/**
 * Validate all graph-backed render contracts in one FramePlan.
 *
 * @param figure the figure that produced the FramePlan
 * @param plan the completed FramePlan
 * @param caps the active capability snapshot, or NULL to preserve requested sample counts
 * @param report optional diagnostic report
 * @return whether all graph-backed render contracts are valid
 */
bool _scene_frame_plan_contracts_validate_ex(
    const DvzFigure* figure, const DvzFramePlan* plan, const DvzCapabilitySnapshot* caps,
    DvzDiagnosticReport* report)
{
    ANN(figure);
    ANN(plan);
    bool ok = _contract_validate_graph_backed_render_nodes(plan, report);
    for (uint32_t i = 0; i < plan->count; i++)
    {
        const DvzFramePlanNode* render = &plan->nodes[i];
        if (render->type != DVZ_FRAME_PLAN_NODE_RENDER)
            continue;

        const DvzFrameGraphPass* graph_pass = _contract_graph_pass_for_render(plan, render);
        if (graph_pass == NULL)
            continue;

        const DvzPanel* panel = _contract_panel_for_render(figure, plan, render);
        if (panel == NULL)
        {
            _contract_report(report, "render contract has no matching panel");
            ok = false;
            continue;
        }

        DvzScenePassContract contract = {0};
        if (!_scene_pass_contract_from_render_ex(plan, panel, render, graph_pass, caps, &contract))
        {
            _contract_report(report, "render contract resolution failed");
            ok = false;
            continue;
        }
        if (!_scene_pass_contract_validate(&contract, report))
            ok = false;
    }
    return ok;
}



/**
 * Validate all graph-backed render contracts in one FramePlan.
 *
 * @param figure the figure that produced the FramePlan
 * @param plan the completed FramePlan
 * @param report optional diagnostic report
 * @return whether all graph-backed render contracts are valid
 */
bool _scene_frame_plan_contracts_validate(
    const DvzFigure* figure, const DvzFramePlan* plan, DvzDiagnosticReport* report)
{
    const DvzCapabilitySnapshot* caps =
        figure != NULL && figure->scene != NULL ? &figure->scene->caps : NULL;
    return _scene_frame_plan_contracts_validate_ex(figure, plan, caps, report);
}
