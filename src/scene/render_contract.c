/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "render_contract.h"

#include <stdlib.h>
#include <string.h>

#include <vulkan/vulkan_core.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_visual_pipeline.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static void _contract_report(DvzDiagnosticReport* report, const char* message);



/**
 * Return whether one render-pass role carries retained scene visual draws.
 *
 * @param role the render-pass role
 * @return whether the role may contain ordinary visual draws
 */
static bool _role_is_visual_pass(DvzFramePlanRenderPassRole role)
{
    return role == DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE ||
           role == DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND ||
           role == DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION ||
           role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT ||
           role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER;
}



/**
 * Return whether one draw contract belongs in one pass role.
 *
 * @param draw the resolved draw contract
 * @return whether the alpha mode and pass role match
 */
static bool _draw_pass_role_matches(const DvzSceneDrawContract* draw)
{
    ANN(draw);
    switch (draw->pass_role)
    {
    case DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE:
        return draw->alpha_mode == DVZ_ALPHA_OPAQUE || draw->alpha_mode == DVZ_ALPHA_MASK;
    case DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND:
        return draw->alpha_mode == DVZ_ALPHA_BLENDED;
    case DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION:
        return draw->alpha_mode == DVZ_ALPHA_WBOIT;
    case DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT:
    case DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER:
        return draw->alpha_mode == DVZ_ALPHA_DEPTH_PEEL;
    case DVZ_FRAME_PLAN_RENDER_PASS_VOLUME_OCCLUSION:
        return draw->writes_volume_occlusion_depth;
    case DVZ_FRAME_PLAN_RENDER_PASS_SCENE_OCCLUSION:
        return draw->writes_scene_occlusion_depth;
    default:
        return true;
    }
}



/**
 * Resolve a draw depth policy from its component depth requirements.
 *
 * @param depth_test whether fixed-function depth testing is required
 * @param depth_write whether fixed-function depth writes are required
 * @param samples_depth whether the shader samples a produced depth resource
 * @return depth-policy bit mask
 */
static uint32_t _draw_depth_policy(bool depth_test, bool depth_write, bool samples_depth)
{
    uint32_t policy = DVZ_SCENE_DEPTH_POLICY_NONE;
    if (depth_test)
        policy |= DVZ_SCENE_DEPTH_POLICY_TEST;
    if (depth_write)
        policy |= DVZ_SCENE_DEPTH_POLICY_WRITE;
    if (samples_depth)
        policy |= DVZ_SCENE_DEPTH_POLICY_SAMPLE;
    return policy;
}



/**
 * Resolve a draw blend policy from alpha mode and render-pass role.
 *
 * @param alpha_mode the visual alpha mode
 * @param pass_role the render-pass role carrying the draw
 * @return resolved blend policy
 */
static DvzSceneBlendPolicy _draw_blend_policy(
    DvzAlphaMode alpha_mode, DvzFramePlanRenderPassRole pass_role)
{
    switch (pass_role)
    {
    case DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND:
        return DVZ_SCENE_BLEND_POLICY_SOURCE_OVER;
    case DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION:
        return DVZ_SCENE_BLEND_POLICY_WBOIT;
    case DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT:
    case DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER:
        return DVZ_SCENE_BLEND_POLICY_DEPTH_PEEL;
    case DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE:
        return alpha_mode == DVZ_ALPHA_OPAQUE || alpha_mode == DVZ_ALPHA_MASK
                   ? DVZ_SCENE_BLEND_POLICY_OPAQUE
                   : DVZ_SCENE_BLEND_POLICY_NONE;
    default:
        return DVZ_SCENE_BLEND_POLICY_NONE;
    }
}



/**
 * Return whether a graph attachment access includes reads.
 *
 * @param access the attachment access mode
 * @return whether reads are allowed
 */
static bool _attachment_access_reads(DvzFrameGraphAttachmentAccess access)
{
    return access == DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ ||
           access == DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ_WRITE;
}



/**
 * Return whether a graph attachment access includes writes.
 *
 * @param access the attachment access mode
 * @return whether writes are allowed
 */
static bool _attachment_access_writes(DvzFrameGraphAttachmentAccess access)
{
    return access == DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE ||
           access == DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ_WRITE;
}



/**
 * Append one attachment use to a pass contract.
 *
 * @param contract the pass contract
 * @param resource_id the graph resource id
 * @param role the attachment role
 * @return the appended attachment use, or NULL if the contract is full
 */
static DvzSceneAttachmentUse* _contract_append_attachment(
    DvzScenePassContract* contract, const char* resource_id, DvzSceneAttachmentRole role)
{
    ANN(contract);
    ANN(resource_id);
    if (contract->attachment_count >= DVZ_SCENE_MAX_CONTRACT_ATTACHMENTS)
        return NULL;

    DvzSceneAttachmentUse* use = &contract->attachments[contract->attachment_count++];
    dvz_memset(use, sizeof(DvzSceneAttachmentUse), 0, sizeof(DvzSceneAttachmentUse));
    dvz_strlcpy(use->resource_id, resource_id, sizeof(use->resource_id));
    use->role = role;
    return use;
}


/**
 * Return the graph resource with a given id.
 *
 * @param plan the FramePlan
 * @param resource_id the graph resource id
 * @return the graph resource, or NULL when absent
 */
static const DvzFrameGraphResource* _contract_resource_by_id(
    const DvzFramePlan* plan, const char* resource_id)
{
    ANN(plan);
    ANN(resource_id);
    for (uint32_t i = 0; i < dvz_frame_plan_graph_resource_count(plan); i++)
    {
        const DvzFrameGraphResource* resource = dvz_frame_plan_graph_resource_get(plan, i);
        if (resource != NULL && strcmp(resource->id, resource_id) == 0)
            return resource;
    }
    return NULL;
}



/**
 * Copy graph resource facts into an attachment use.
 *
 * @param plan the FramePlan
 * @param use the attachment use
 */
static void _contract_apply_resource_facts(
    const DvzFramePlan* plan, DvzSceneAttachmentUse* use)
{
    ANN(plan);
    ANN(use);
    const DvzFrameGraphResource* resource = _contract_resource_by_id(plan, use->resource_id);
    if (resource == NULL)
        return;
    use->format = resource->format;
    use->sample_count = resource->sample_count == 0 ? 1 : resource->sample_count;
}



/**
 * Append one color attachment to a pass contract.
 *
 * @param plan the FramePlan
 * @param contract the pass contract
 * @param attachment the graph attachment
 * @return whether the attachment was appended
 */
static bool _contract_append_color_attachment(
    const DvzFramePlan* plan, DvzScenePassContract* contract,
    const DvzFrameGraphAttachment* attachment)
{
    ANN(plan);
    ANN(contract);
    ANN(attachment);
    DvzSceneAttachmentUse* use = _contract_append_attachment(
        contract, attachment->resource_id, DVZ_SCENE_ATTACHMENT_COLOR);
    if (use == NULL)
        return false;
    use->load_op = attachment->load_op;
    use->store_op = attachment->store_op;
    use->access = attachment->access;
    use->read = _attachment_access_reads(attachment->access);
    use->write = _attachment_access_writes(attachment->access);
    use->clear = attachment->load_op == DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR;
    use->preserve = attachment->load_op == DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD ||
                    attachment->store_op == DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE;
    _contract_apply_resource_facts(plan, use);
    contract->color_attachment_count++;
    return true;
}



/**
 * Append one depth attachment to a pass contract.
 *
 * @param plan the FramePlan
 * @param contract the pass contract
 * @param attachment the graph attachment
 * @return whether the attachment was appended
 */
static bool _contract_append_depth_attachment(
    const DvzFramePlan* plan, DvzScenePassContract* contract,
    const DvzFrameGraphAttachment* attachment)
{
    ANN(plan);
    ANN(contract);
    ANN(attachment);
    DvzSceneAttachmentUse* use = _contract_append_attachment(
        contract, attachment->resource_id, DVZ_SCENE_ATTACHMENT_DEPTH);
    if (use == NULL)
        return false;
    use->load_op = attachment->load_op;
    use->store_op = attachment->store_op;
    use->access = attachment->access;
    use->read = _attachment_access_reads(attachment->access);
    use->write = _attachment_access_writes(attachment->access);
    use->clear = attachment->load_op == DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR;
    use->preserve = attachment->load_op == DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD ||
                    attachment->store_op == DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE;
    _contract_apply_resource_facts(plan, use);
    contract->has_depth_attachment = true;
    return true;
}



/**
 * Append one sampled read edge to a pass contract.
 *
 * @param plan the FramePlan
 * @param contract the pass contract
 * @param read the graph read edge
 * @return whether the read was appended
 */
static bool _contract_append_read(
    const DvzFramePlan* plan, DvzScenePassContract* contract, const DvzFrameGraphAccess* read)
{
    ANN(plan);
    ANN(contract);
    ANN(read);
    DvzSceneAttachmentUse* use = _contract_append_attachment(
        contract, read->resource_id, DVZ_SCENE_ATTACHMENT_SAMPLED);
    if (use == NULL)
        return false;
    use->read = true;
    _contract_apply_resource_facts(plan, use);
    contract->sampled_read_count++;
    if (strstr(read->resource_id, ".depth") != NULL)
        contract->sampled_depth_read_count++;
    return true;
}



/**
 * Return whether a pass contract has a depth attachment.
 *
 * @param contract the pass contract
 * @return whether the contract includes a depth attachment
 */
static bool _contract_has_depth_attachment(const DvzScenePassContract* contract)
{
    ANN(contract);
    for (uint32_t i = 0; i < contract->attachment_count; i++)
    {
        if (contract->attachments[i].role == DVZ_SCENE_ATTACHMENT_DEPTH)
            return true;
    }
    return false;
}



/**
 * Return whether one pass has a sampled or producer-backed depth resource.
 *
 * @param contract the pass contract
 * @return whether sampled-depth draws can resolve a produced depth resource
 */
static bool _contract_has_sampled_depth_resource(const DvzScenePassContract* contract)
{
    ANN(contract);
    if (contract->sampled_depth_read_count > 0)
        return true;
    for (uint32_t i = 0; i < contract->attachment_count; i++)
    {
        const DvzSceneAttachmentUse* attachment = &contract->attachments[i];
        if (attachment->role != DVZ_SCENE_ATTACHMENT_DEPTH)
            continue;
        if (
            attachment->load_op == DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD &&
            (attachment->access == DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ ||
             attachment->access == DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ_WRITE))
            return true;
    }
    return false;
}


/**
 * Count attachment uses with a given role.
 *
 * @param contract the pass contract
 * @param role the attachment role
 * @return number of matching attachments
 */
static uint32_t _contract_attachment_count(
    const DvzScenePassContract* contract, DvzSceneAttachmentRole role)
{
    ANN(contract);
    uint32_t count = 0;
    for (uint32_t i = 0; i < contract->attachment_count; i++)
    {
        if (contract->attachments[i].role == role)
            count++;
    }
    return count;
}



/**
 * Return the first attachment matching a role and resource suffix.
 *
 * @param contract the pass contract
 * @param role the attachment role
 * @param suffix the expected resource id suffix
 * @return the matching attachment, or NULL
 */
static const DvzSceneAttachmentUse* _contract_attachment_suffix(
    const DvzScenePassContract* contract, DvzSceneAttachmentRole role, const char* suffix)
{
    ANN(contract);
    ANN(suffix);
    for (uint32_t i = 0; i < contract->attachment_count; i++)
    {
        const DvzSceneAttachmentUse* use = &contract->attachments[i];
        if (use->role == role && strstr(use->resource_id, suffix) != NULL)
            return use;
    }
    return NULL;
}



/**
 * Return whether a pass contract reads a resource containing a suffix.
 *
 * @param contract the pass contract
 * @param suffix the expected resource id suffix
 * @return whether a sampled attachment matches
 */
static bool _contract_reads_resource_suffix(
    const DvzScenePassContract* contract, const char* suffix)
{
    ANN(contract);
    ANN(suffix);
    for (uint32_t i = 0; i < contract->attachment_count; i++)
    {
        const DvzSceneAttachmentUse* use = &contract->attachments[i];
        if (use->role != DVZ_SCENE_ATTACHMENT_SAMPLED || !use->read)
            continue;
        if (strstr(use->resource_id, suffix) != NULL)
            return true;
    }
    return false;
}


/**
 * Return whether any draw in a contract tests or writes fixed-function depth.
 *
 * @param contract the pass contract
 * @return whether a depth attachment is required
 */
static bool _contract_needs_depth(const DvzScenePassContract* contract)
{
    ANN(contract);
    for (uint32_t i = 0; i < contract->draw_count; i++)
    {
        const DvzSceneDrawContract* draw = &contract->draws[i];
        if (draw->depth_test || draw->depth_write)
            return true;
    }
    return false;
}



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
        if (_contract_needs_depth(contract) && !contract->has_depth_attachment)
        {
            _contract_report(report, "depth peel raster pass is missing required depth");
            ok = false;
        }
        break;

    case DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_COMPOSITE:
        if (contract->draw_count != 0 || contract->color_attachment_count != 1 ||
            contract->sampled_read_count != 3 || !contract->needs_depth_peel_sampled_layout ||
            contract->sampled_texture_binding_count != 3)
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
 * Add a diagnostic message if a report was provided.
 *
 * @param report the optional diagnostic report
 * @param message the diagnostic message
 */
static void _contract_report(DvzDiagnosticReport* report, const char* message)
{
    if (report != NULL)
        (void)dvz_diagnostic_report_add(report, message);
}



/**
 * Return the graph work label used by one render-pass role.
 *
 * @param role the FramePlan render-pass role
 * @return the graph work label, or an empty string when none is expected
 */
static const char* _contract_work_label_for_render_role(DvzFramePlanRenderPassRole role)
{
    switch (role)
    {
    case DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE:
        return "opaque";
    case DVZ_FRAME_PLAN_RENDER_PASS_GBUFFER:
        return "gbuffer";
    case DVZ_FRAME_PLAN_RENDER_PASS_VOLUME_OCCLUSION:
        return "volume_occlusion";
    case DVZ_FRAME_PLAN_RENDER_PASS_SCENE_OCCLUSION:
        return "scene_occlusion";
    case DVZ_FRAME_PLAN_RENDER_PASS_SSAO:
        return "ssao";
    case DVZ_FRAME_PLAN_RENDER_PASS_SSAO_BLUR:
        return "ssao_blur";
    case DVZ_FRAME_PLAN_RENDER_PASS_SSAO_COMPOSITE:
        return "ssao_composite";
    case DVZ_FRAME_PLAN_RENDER_PASS_EDL_RESOLVE:
        return "edl_resolve";
    case DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION:
        return "wboit_accum";
    case DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND:
        return "transparent_blend";
    case DVZ_FRAME_PLAN_RENDER_PASS_WBOIT_RESOLVE:
        return "wboit_resolve";
    case DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT:
        return "depth_peel_init";
    case DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER:
        return "depth_peel_iter";
    case DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_COMPOSITE:
        return "depth_peel_composite";
    case DVZ_FRAME_PLAN_RENDER_PASS_PICKING:
        return "picking";
    default:
        return "";
    }
}



/**
 * Return whether one render-pass role must have a matching graph pass.
 *
 * @param role the FramePlan render-pass role
 * @return whether the role is graph-backed
 */
static bool _contract_render_role_requires_graph_pass(DvzFramePlanRenderPassRole role)
{
    switch (role)
    {
    case DVZ_FRAME_PLAN_RENDER_PASS_GBUFFER:
    case DVZ_FRAME_PLAN_RENDER_PASS_VOLUME_OCCLUSION:
    case DVZ_FRAME_PLAN_RENDER_PASS_SCENE_OCCLUSION:
    case DVZ_FRAME_PLAN_RENDER_PASS_SSAO:
    case DVZ_FRAME_PLAN_RENDER_PASS_SSAO_BLUR:
    case DVZ_FRAME_PLAN_RENDER_PASS_SSAO_COMPOSITE:
    case DVZ_FRAME_PLAN_RENDER_PASS_EDL_RESOLVE:
    case DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION:
    case DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND:
    case DVZ_FRAME_PLAN_RENDER_PASS_WBOIT_RESOLVE:
    case DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT:
    case DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER:
    case DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_COMPOSITE:
        return true;
    default:
        return false;
    }
}



/**
 * Return the graph pass matching one render node.
 *
 * @param plan the FramePlan
 * @param render the render node
 * @return the graph pass, or NULL when the render node has no graph pass
 */
static const DvzFrameGraphPass* _contract_graph_pass_for_render(
    const DvzFramePlan* plan, const DvzFramePlanNode* render)
{
    ANN(plan);
    ANN(render);
    if (render->type != DVZ_FRAME_PLAN_NODE_RENDER)
        return NULL;
    const char* work_label =
        _contract_work_label_for_render_role(render->u.render.pass_role);
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
            _contract_work_label_for_render_role(candidate->u.render.pass_role);
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



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve one visual-facts row and pass role into an explicit draw contract.
 *
 * @param facts visual facts used by the resolver matrix
 * @param pass_role the render pass role that will carry the draw
 * @param out the output draw contract
 * @return whether the draw contract was resolved
 */
bool _scene_draw_contract_resolve(
    const DvzSceneDrawFacts* facts, DvzFramePlanRenderPassRole pass_role,
    DvzSceneDrawContract* out)
{
    ANN(facts);
    ANN(out);
    dvz_memset(out, sizeof(DvzSceneDrawContract), 0, sizeof(DvzSceneDrawContract));

    bool scene_depth_pass = pass_role == DVZ_FRAME_PLAN_RENDER_PASS_SCENE_OCCLUSION;
    bool ordinary_visual_pass = _role_is_visual_pass(pass_role);
    out->visual_type = facts->visual_type;
    out->alpha_mode = facts->alpha_mode;
    out->pass_role = pass_role;
    out->depth_test = facts->can_depth_test && (ordinary_visual_pass || scene_depth_pass);
    out->depth_write = facts->writes_depth || (scene_depth_pass && facts->can_write_depth);
    out->samples_depth =
        facts->samples_depth && ordinary_visual_pass &&
        pass_role != DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE;
    out->samples_volume_occlusion = facts->volume_occluded && ordinary_visual_pass;
    out->samples_scene_occlusion = facts->scene_occluded && ordinary_visual_pass;
    out->writes_volume_occlusion_depth =
        pass_role == DVZ_FRAME_PLAN_RENDER_PASS_VOLUME_OCCLUSION &&
        facts->visual_type == DVZ_VISUAL_TYPE_VOLUME;
    out->writes_scene_occlusion_depth =
        pass_role == DVZ_FRAME_PLAN_RENDER_PASS_SCENE_OCCLUSION && facts->scene_occluder;
    out->needs_common_set = facts->uses_common_set;
    out->needs_material_set = facts->uses_material_set;
    out->needs_image_set = facts->uses_image_set;
    out->needs_volume_set = facts->uses_volume_set;
    out->needs_scene_occlusion_set = out->samples_scene_occlusion;
    out->depth_policy =
        _draw_depth_policy(out->depth_test, out->depth_write, out->samples_depth);
    out->blend_policy = _draw_blend_policy(facts->alpha_mode, pass_role);
    if (out->samples_depth)
        out->shader_feature_mask |= DVZ_SCENE_SHADER_FEATURE_SAMPLE_DEPTH;
    if (out->samples_volume_occlusion)
        out->shader_feature_mask |= DVZ_SCENE_SHADER_FEATURE_SAMPLE_VOLUME_OCCLUSION;
    if (out->samples_scene_occlusion)
        out->shader_feature_mask |= DVZ_SCENE_SHADER_FEATURE_SAMPLE_SCENE_OCCLUSION;
    if (out->writes_volume_occlusion_depth)
        out->shader_feature_mask |= DVZ_SCENE_SHADER_FEATURE_WRITE_VOLUME_OCCLUSION;
    if (out->writes_scene_occlusion_depth)
        out->shader_feature_mask |= DVZ_SCENE_SHADER_FEATURE_WRITE_SCENE_OCCLUSION;
    if (out->needs_common_set)
        out->bind_group_layout_mask |= DVZ_SCENE_BIND_GROUP_REQUIREMENT_COMMON;
    if (out->needs_material_set)
        out->bind_group_layout_mask |= DVZ_SCENE_BIND_GROUP_REQUIREMENT_MATERIAL;
    if (out->needs_image_set)
        out->bind_group_layout_mask |= DVZ_SCENE_BIND_GROUP_REQUIREMENT_IMAGE;
    if (out->needs_volume_set)
        out->bind_group_layout_mask |= DVZ_SCENE_BIND_GROUP_REQUIREMENT_VOLUME;
    if (out->needs_scene_occlusion_set)
        out->bind_group_layout_mask |= DVZ_SCENE_BIND_GROUP_REQUIREMENT_SCENE_OCCLUSION;
    return true;
}



/**
 * Resolve one retained visual draw into a passive render contract.
 *
 * @param visual the retained visual
 * @param attach the panel attachment
 * @param pass_role the render pass role that will carry the draw
 * @param out the output draw contract
 * @return whether the draw contract was resolved
 */
bool _scene_draw_contract_from_visual(
    const DvzVisual* visual, const DvzPanelAttach* attach, DvzFramePlanRenderPassRole pass_role,
    DvzSceneDrawContract* out)
{
    ANN(visual);
    ANN(attach);
    ANN(out);

    DvzSceneVisualPassCaps caps = {0};
    if (!_scene_visual_pass_caps_from_visual(visual, attach, &caps))
        return false;

    DvzSceneDrawFacts facts = {
        .visual_type = (uint32_t)visual->type,
        .alpha_mode = visual->alpha_mode,
        .can_depth_test = caps.can_depth_test,
        .can_write_depth = caps.can_write_depth,
        .writes_depth = caps.writes_depth,
        .samples_depth = caps.samples_depth,
        .volume_occluded = visual->volume_occluded,
        .scene_occluded = visual->scene_occluded,
        .scene_occluder = visual->scene_occluder,
        .uses_common_set = caps.uses_common_set,
        .uses_material_set = caps.uses_material_set,
        .uses_image_set = caps.uses_image_set,
        .uses_volume_set = caps.uses_volume_set,
    };
    return _scene_draw_contract_resolve(&facts, pass_role, out);
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

    out->source_over_blend = out->role == DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND;
    out->wboit_accumulation =
        out->role == DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION;
    out->depth_peel = out->role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT ||
                      out->role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER;
    out->fullscreen_resolve = out->role == DVZ_FRAME_PLAN_RENDER_PASS_WBOIT_RESOLVE ||
                              out->role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_COMPOSITE ||
                              out->role == DVZ_FRAME_PLAN_RENDER_PASS_EDL_RESOLVE ||
                              out->role == DVZ_FRAME_PLAN_RENDER_PASS_SSAO_COMPOSITE;
    out->needs_wboit_resolve_layout =
        out->role == DVZ_FRAME_PLAN_RENDER_PASS_WBOIT_RESOLVE;
    out->needs_depth_peel_sampled_layout =
        out->role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_COMPOSITE;
    if (out->needs_wboit_resolve_layout)
        out->sampled_texture_binding_count = 2;
    else if (out->needs_depth_peel_sampled_layout)
        out->sampled_texture_binding_count = 3;

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
        const DvzSceneDrawContract* draw = &out->draws[out->draw_count];
        out->needs_common_set = out->needs_common_set || draw->needs_common_set;
        out->needs_material_set = out->needs_material_set || draw->needs_material_set;
        out->needs_image_set = out->needs_image_set || draw->needs_image_set;
        out->needs_volume_set = out->needs_volume_set || draw->needs_volume_set;
        out->needs_scene_occlusion_set =
            out->needs_scene_occlusion_set || draw->needs_scene_occlusion_set;
        out->draw_count++;
    }

    if (graph_pass != NULL)
    {
        for (uint32_t i = 0; i < graph_pass->color_attachment_count; i++)
        {
            if (!_contract_append_color_attachment(plan, out, &graph_pass->color_attachments[i]))
                return false;
        }
        if (graph_pass->has_depth_attachment &&
            !_contract_append_depth_attachment(plan, out, &graph_pass->depth_attachment))
            return false;
        for (uint32_t i = 0; i < graph_pass->read_count; i++)
        {
            if (!_contract_append_read(plan, out, &graph_pass->reads[i]))
                return false;
        }
    }
    return true;
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
        samples_volume_occlusion = samples_volume_occlusion || draw->samples_volume_occlusion;
        samples_scene_occlusion = samples_scene_occlusion || draw->samples_scene_occlusion;
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
 * @param report optional diagnostic report
 * @return whether all graph-backed render contracts are valid
 */
bool _scene_frame_plan_contracts_validate(
    const DvzFigure* figure, const DvzFramePlan* plan, DvzDiagnosticReport* report)
{
    ANN(figure);
    ANN(plan);
    bool ok = true;
    for (uint32_t i = 0; i < plan->count; i++)
    {
        const DvzFramePlanNode* render = &plan->nodes[i];
        if (render->type != DVZ_FRAME_PLAN_NODE_RENDER)
            continue;

        const DvzFrameGraphPass* graph_pass = _contract_graph_pass_for_render(plan, render);
        if (graph_pass == NULL)
        {
            if (_contract_render_role_requires_graph_pass(render->u.render.pass_role))
            {
                _contract_report(report, "graph-backed render node has no matching graph pass");
                ok = false;
            }
            continue;
        }

        const DvzPanel* panel = _contract_panel_for_render(figure, plan, render);
        if (panel == NULL)
        {
            _contract_report(report, "render contract has no matching panel");
            ok = false;
            continue;
        }

        DvzScenePassContract contract = {0};
        if (!_scene_pass_contract_from_render(plan, panel, render, graph_pass, &contract))
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
