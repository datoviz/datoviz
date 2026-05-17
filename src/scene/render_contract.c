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
#include "_technique.h"
#include "_visual_pipeline.h"
#include "../drp2/_stream.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

typedef struct ContractDrp2PassState
{
    uint32_t color_attachment_count;
    uint32_t color_formats[DVZ_DRP2_MAX_COLOR_ATTACHMENTS];
    bool color_format_known[DVZ_DRP2_MAX_COLOR_ATTACHMENTS];
    bool has_sample_count;
    uint32_t sample_count;
    bool saw_sampled_bind_group;
    bool sampled_reads_matched[DVZ_FRAME_PLAN_MAX_GRAPH_ACCESSES];
} ContractDrp2PassState;



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
 * Return the FramePlan render node tagged with one pass-contract id.
 *
 * @param plan the FramePlan
 * @param pass_contract_id the pass-contract id
 * @return the render node, or NULL when no node matches
 */
static const DvzFramePlanNode* _contract_render_for_pass_id(
    const DvzFramePlan* plan, const char* pass_contract_id)
{
    ANN(plan);
    ANN(pass_contract_id);
    if (pass_contract_id[0] == '\0')
        return NULL;
    for (uint32_t i = 0; i < plan->count; i++)
    {
        const DvzFramePlanNode* node = &plan->nodes[i];
        if (node->type == DVZ_FRAME_PLAN_NODE_RENDER && node->u.render.has_pass_contract &&
            strcmp(node->u.render.pass_contract_id, pass_contract_id) == 0)
            return node;
    }
    return NULL;
}



/**
 * Return the DRP2 CreateRenderPipeline command for a pipeline id.
 *
 * @param stream the DRP2 command stream
 * @param pipeline_id the render pipeline id
 * @return the pipeline creation command, or NULL when it was created by an earlier stream
 */
static const DvzDrp2Command* _contract_drp2_pipeline_for_id(
    const DvzDrp2CommandStream* stream, uint64_t pipeline_id)
{
    ANN(stream);
    if (pipeline_id == 0)
        return NULL;
    for (uint32_t i = 0; i < stream->count; i++)
    {
        const DvzDrp2Command* command = &stream->commands[i];
        if (command->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE &&
            command->u.create_render_pipeline.id == pipeline_id)
            return command;
    }
    return NULL;
}



/**
 * Return the DRP2 CreateTexture command for a texture id.
 *
 * @param stream the DRP2 command stream
 * @param texture_id the texture id
 * @return the texture creation command, or NULL when it was created by an earlier stream
 */
static const DvzDrp2Command* _contract_drp2_texture_for_id(
    const DvzDrp2CommandStream* stream, uint64_t texture_id)
{
    ANN(stream);
    if (texture_id == 0)
        return NULL;
    for (uint32_t i = 0; i < stream->count; i++)
    {
        const DvzDrp2Command* command = &stream->commands[i];
        if (command->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE &&
            command->u.create_texture.id == texture_id)
            return command;
    }
    return NULL;
}



/**
 * Return the DRP2 CreateBindGroup command for a bind-group id.
 *
 * @param stream the DRP2 command stream
 * @param bind_group_id the bind-group id
 * @return the bind-group creation command, or NULL when it was created by an earlier stream
 */
static const DvzDrp2Command* _contract_drp2_bind_group_for_id(
    const DvzDrp2CommandStream* stream, uint64_t bind_group_id)
{
    ANN(stream);
    if (bind_group_id == 0)
        return NULL;
    for (uint32_t i = 0; i < stream->count; i++)
    {
        const DvzDrp2Command* command = &stream->commands[i];
        if (command->type == DVZ_DRP2_COMMAND_CREATE_BIND_GROUP &&
            command->u.create_bind_group.id == bind_group_id)
            return command;
    }
    return NULL;
}



/**
 * Return whether a DRP2 label names one graph resource, with optional runtime scope suffix.
 *
 * @param label emitted runtime object label
 * @param resource_id graph resource id
 * @return whether the label matches the graph resource
 */
static bool _contract_resource_label_matches(const char* label, const char* resource_id)
{
    if (label == NULL || resource_id == NULL || resource_id[0] == '\0')
        return false;
    size_t len = strlen(resource_id);
    return strncmp(label, resource_id, len) == 0 &&
           (label[len] == '\0' || strncmp(&label[len], "_scope_", 7) == 0);
}



/**
 * Return the effective DRP2 color format when a command omits it.
 *
 * @param format emitted VkFormat value, where zero means DRP2's default color format
 * @return effective VkFormat value
 */
static uint32_t _contract_effective_color_format(uint32_t format)
{
    return format != 0 ? format : VK_FORMAT_R8G8B8A8_UNORM;
}



/**
 * Return the effective DRP2 raster sample count when a command omits it.
 *
 * @param sample_count emitted sample count, where zero means one sample
 * @return effective sample count
 */
static uint32_t _contract_effective_sample_count(uint32_t sample_count)
{
    return sample_count != 0 ? sample_count : 1;
}



/**
 * Return one render-pass color attachment texture id.
 *
 * @param command the BeginRenderPass command
 * @param index color attachment index
 * @return color texture id, or zero when absent
 */
static uint64_t _contract_render_pass_color_texture_id(
    const DvzDrp2Command* command, uint32_t index)
{
    ANN(command);
    if (index >= DVZ_DRP2_MAX_COLOR_ATTACHMENTS)
        return 0;
    if (command->u.begin_render_pass.color_attachment_count > 0)
        return command->u.begin_render_pass.color_attachments[index].texture_id;
    return index == 0 ? command->u.begin_render_pass.texture_id : 0;
}



/**
 * Resolve emitted attachment formats and sample count for one DRP2 render pass.
 *
 * @param stream the DRP2 command stream
 * @param command the BeginRenderPass command
 * @param out output pass state
 * @param report optional diagnostic report
 * @return whether known attachment sample counts agree
 */
static bool _contract_resolve_drp2_pass_state(
    const DvzDrp2CommandStream* stream, const DvzDrp2Command* command,
    ContractDrp2PassState* out, DvzDiagnosticReport* report)
{
    ANN(stream);
    ANN(command);
    ANN(out);
    dvz_memset(out, sizeof(ContractDrp2PassState), 0, sizeof(ContractDrp2PassState));

    bool ok = true;
    out->color_attachment_count = command->u.begin_render_pass.color_attachment_count != 0 ?
                                      command->u.begin_render_pass.color_attachment_count :
                                      1;
    if (out->color_attachment_count > DVZ_DRP2_MAX_COLOR_ATTACHMENTS)
    {
        _contract_report(report, "DRP2 render pass has too many color attachments");
        return false;
    }

    for (uint32_t i = 0; i < out->color_attachment_count; i++)
    {
        uint64_t texture_id = _contract_render_pass_color_texture_id(command, i);
        const DvzDrp2Command* texture = _contract_drp2_texture_for_id(stream, texture_id);
        if (texture == NULL)
            continue;

        out->color_formats[i] =
            _contract_effective_color_format(texture->u.create_texture.format);
        out->color_format_known[i] = true;

        uint32_t sample_count =
            _contract_effective_sample_count(texture->u.create_texture.sample_count);
        if (!out->has_sample_count)
        {
            out->sample_count = sample_count;
            out->has_sample_count = true;
        }
        else if (out->sample_count != sample_count)
        {
            _contract_report(report, "DRP2 render pass attachment sample-count mismatch");
            ok = false;
        }
    }

    if (command->u.begin_render_pass.has_depth_attachment &&
        command->u.begin_render_pass.depth_texture_id != 0)
    {
        const DvzDrp2Command* depth =
            _contract_drp2_texture_for_id(stream, command->u.begin_render_pass.depth_texture_id);
        if (depth != NULL)
        {
            uint32_t sample_count =
                _contract_effective_sample_count(depth->u.create_texture.sample_count);
            if (!out->has_sample_count)
            {
                out->sample_count = sample_count;
                out->has_sample_count = true;
            }
            else if (out->sample_count != sample_count)
            {
                _contract_report(report, "DRP2 depth attachment sample-count mismatch");
                ok = false;
            }
        }
    }
    return ok;
}



/**
 * Validate a source-over DRP2 color target.
 *
 * @param target the emitted color-target state
 * @return whether the target uses source-over blending
 */
static bool _contract_drp2_source_over_blend(const DvzDrp2ColorTarget* target)
{
    ANN(target);
    return target->blend_enabled &&
           target->src_color_blend_factor == VK_BLEND_FACTOR_SRC_ALPHA &&
           target->dst_color_blend_factor == VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA &&
           target->color_blend_op == VK_BLEND_OP_ADD &&
           target->src_alpha_blend_factor == VK_BLEND_FACTOR_ONE &&
           target->dst_alpha_blend_factor == VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA &&
           target->alpha_blend_op == VK_BLEND_OP_ADD;
}



/**
 * Return whether a pipeline has a bind-group layout with a given scene runtime label.
 *
 * @param stream the DRP2 command stream
 * @param command the CreateRenderPipeline command
 * @param label expected bind-group-layout label
 * @return whether the pipeline references the labeled layout
 */
static bool _contract_pipeline_has_layout_label(
    const DvzDrp2CommandStream* stream, const DvzDrp2Command* command, const char* label)
{
    ANN(stream);
    ANN(command);
    ANN(label);
    for (uint32_t i = 0; i < command->u.create_render_pipeline.bind_group_layout_count; i++)
    {
        uint64_t layout_id = command->u.create_render_pipeline.bind_group_layout_ids[i];
        const char* actual = dvz_drp2_stream_label(stream, layout_id);
        if (actual != NULL && strcmp(actual, label) == 0)
            return true;
    }
    return false;
}



/**
 * Validate a pipeline's bind-group layouts against one draw-contract mask.
 *
 * @param stream the DRP2 command stream
 * @param command the CreateRenderPipeline command
 * @param mask required scene bind-group layout mask
 * @param report optional diagnostic report
 * @return whether required layouts are present
 */
static bool _contract_validate_drp2_pipeline_layouts(
    const DvzDrp2CommandStream* stream, const DvzDrp2Command* command, uint32_t mask,
    DvzDiagnosticReport* report)
{
    ANN(stream);
    ANN(command);
    bool ok = true;
    if ((mask & DVZ_SCENE_BIND_GROUP_REQUIREMENT_COMMON) != 0 &&
        !_contract_pipeline_has_layout_label(stream, command, "_bgl_scene_common"))
    {
        _contract_report(report, "DRP2 pipeline missing common bind-group layout");
        ok = false;
    }
    if ((mask & DVZ_SCENE_BIND_GROUP_REQUIREMENT_MATERIAL) != 0 &&
        !_contract_pipeline_has_layout_label(stream, command, "_bgl_material_params"))
    {
        _contract_report(report, "DRP2 pipeline missing material bind-group layout");
        ok = false;
    }
    if ((mask & DVZ_SCENE_BIND_GROUP_REQUIREMENT_IMAGE) != 0 &&
        !_contract_pipeline_has_layout_label(stream, command, "_bgl_img"))
    {
        _contract_report(report, "DRP2 pipeline missing image bind-group layout");
        ok = false;
    }
    if ((mask & DVZ_SCENE_BIND_GROUP_REQUIREMENT_VOLUME) != 0 &&
        !_contract_pipeline_has_layout_label(stream, command, "_bgl_volume"))
    {
        _contract_report(report, "DRP2 pipeline missing volume bind-group layout");
        ok = false;
    }
    if ((mask & DVZ_SCENE_BIND_GROUP_REQUIREMENT_SCENE_OCCLUSION) != 0 &&
        !_contract_pipeline_has_layout_label(stream, command, "_bgl_scene_occ"))
    {
        _contract_report(report, "DRP2 pipeline missing scene-occlusion bind-group layout");
        ok = false;
    }
    return ok;
}



/**
 * Validate a pipeline's color target formats against known render-pass attachment formats.
 *
 * @param command the CreateRenderPipeline command
 * @param pass_state resolved DRP2 render-pass state
 * @param report optional diagnostic report
 * @return whether known color formats match
 */
static bool _contract_validate_drp2_pipeline_color_targets(
    const DvzDrp2Command* command, const ContractDrp2PassState* pass_state,
    DvzDiagnosticReport* report)
{
    ANN(command);
    ANN(pass_state);
    bool ok = true;
    uint32_t target_count = command->u.create_render_pipeline.color_target_count != 0 ?
                                command->u.create_render_pipeline.color_target_count :
                                1;
    if (
        pass_state->color_attachment_count != 0 &&
        target_count != pass_state->color_attachment_count)
    {
        _contract_report(report, "DRP2 pipeline color target count mismatches render pass");
        ok = false;
    }

    uint32_t count = target_count < pass_state->color_attachment_count ?
                         target_count :
                         pass_state->color_attachment_count;
    for (uint32_t i = 0; i < count; i++)
    {
        if (!pass_state->color_format_known[i])
            continue;
        uint32_t pipeline_format = _contract_effective_color_format(
            command->u.create_render_pipeline.color_targets[i].format);
        if (pipeline_format != pass_state->color_formats[i])
        {
            _contract_report(report, "DRP2 pipeline color target format mismatches render pass");
            ok = false;
        }
    }
    return ok;
}



/**
 * Record graph sampled reads satisfied by one emitted bind group.
 *
 * @param stream the DRP2 command stream
 * @param graph_pass active graph pass
 * @param bind_group the CreateBindGroup command
 * @param state active render-pass checker state
 */
static void _contract_mark_drp2_sampled_reads(
    const DvzDrp2CommandStream* stream, const DvzFrameGraphPass* graph_pass,
    const DvzDrp2Command* bind_group, ContractDrp2PassState* state)
{
    ANN(stream);
    ANN(graph_pass);
    ANN(bind_group);
    ANN(state);

    for (uint32_t i = 0; i < bind_group->u.create_bind_group.entry_count; i++)
    {
        const DvzDrp2BindGroupEntry* entry = &bind_group->u.create_bind_group.entries[i];
        if (entry->binding_type != DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE &&
            entry->binding_type != DVZ_DRP2_BINDING_TYPE_STORAGE_TEXTURE)
            continue;
        if (entry->resource_kind != DVZ_DRP2_BINDING_RESOURCE_TEXTURE &&
            entry->resource_kind != DVZ_DRP2_BINDING_RESOURCE_TEXTURE_VIEW)
            continue;

        state->saw_sampled_bind_group = true;
        const char* label = dvz_drp2_stream_label(stream, entry->resource_id);
        for (uint32_t j = 0; j < graph_pass->read_count; j++)
        {
            if (graph_pass->reads[j].usage != DVZ_FRAME_GRAPH_ACCESS_SAMPLED)
                continue;
            if (_contract_resource_label_matches(label, graph_pass->reads[j].resource_id))
                state->sampled_reads_matched[j] = true;
        }
    }
}



/**
 * Validate that observed sampled bind groups cover the active graph pass sampled reads.
 *
 * @param graph_pass active graph pass
 * @param state active render-pass checker state
 * @param report optional diagnostic report
 * @return whether all observed sampled-read contracts were satisfied
 */
static bool _contract_validate_drp2_sampled_reads(
    const DvzFrameGraphPass* graph_pass, const ContractDrp2PassState* state,
    DvzDiagnosticReport* report)
{
    if (graph_pass == NULL || state == NULL || !state->saw_sampled_bind_group)
        return true;

    bool ok = true;
    for (uint32_t i = 0; i < graph_pass->read_count; i++)
    {
        if (graph_pass->reads[i].usage != DVZ_FRAME_GRAPH_ACCESS_SAMPLED)
            continue;
        if (!state->sampled_reads_matched[i])
        {
            _contract_report(report, "DRP2 sampled bind group misses graph read resource");
            ok = false;
        }
    }
    return ok;
}



/**
 * Validate one DRP2 pipeline against one draw-contract metadata snapshot.
 *
 * @param stream the DRP2 command stream
 * @param command the CreateRenderPipeline command
 * @param meta the draw metadata snapshot
 * @param graph_pass the matching graph pass, or NULL for non-graph rendering
 * @param pass_state resolved DRP2 render-pass state
 * @param report optional diagnostic report
 * @return whether the emitted pipeline matches the stored draw contract
 */
static bool _contract_validate_drp2_pipeline(
    const DvzDrp2CommandStream* stream, const DvzDrp2Command* command,
    const DvzFramePlanVisualMeta* meta, const DvzFrameGraphPass* graph_pass,
    const ContractDrp2PassState* pass_state, DvzDiagnosticReport* report)
{
    ANN(stream);
    ANN(command);
    ANN(meta);
    ANN(pass_state);
    bool ok = true;
    uint32_t depth_policy = meta->draw_depth_policy;
    DvzSceneBlendPolicy blend_policy = (DvzSceneBlendPolicy)meta->draw_blend_policy;

    ok = _contract_validate_drp2_pipeline_color_targets(command, pass_state, report) && ok;
    if (pass_state->has_sample_count &&
        _contract_effective_sample_count(command->u.create_render_pipeline.sample_count) !=
            pass_state->sample_count)
    {
        _contract_report(report, "DRP2 pipeline sample count mismatches render pass");
        ok = false;
    }
    ok = _contract_validate_drp2_pipeline_layouts(
             stream, command, meta->draw_bind_group_layout_mask, report) &&
         ok;

    if ((depth_policy & (DVZ_SCENE_DEPTH_POLICY_TEST | DVZ_SCENE_DEPTH_POLICY_WRITE)) != 0 &&
        !command->u.create_render_pipeline.has_depth_attachment)
    {
        _contract_report(report, "DRP2 pipeline missing contracted depth attachment state");
        ok = false;
    }
    if ((depth_policy & DVZ_SCENE_DEPTH_POLICY_WRITE) != 0 &&
        !command->u.create_render_pipeline.depth_write_enabled)
    {
        _contract_report(report, "DRP2 pipeline missing contracted depth write");
        ok = false;
    }
    if ((depth_policy & DVZ_SCENE_DEPTH_POLICY_WRITE) == 0 &&
        (blend_policy == DVZ_SCENE_BLEND_POLICY_SOURCE_OVER ||
         blend_policy == DVZ_SCENE_BLEND_POLICY_WBOIT ||
         blend_policy == DVZ_SCENE_BLEND_POLICY_DEPTH_PEEL) &&
        command->u.create_render_pipeline.depth_write_enabled)
    {
        _contract_report(report, "transparent DRP2 pipeline writes depth");
        ok = false;
    }

    if (blend_policy == DVZ_SCENE_BLEND_POLICY_SOURCE_OVER)
    {
        if (command->u.create_render_pipeline.color_target_count < 1 ||
            !_contract_drp2_source_over_blend(&command->u.create_render_pipeline.color_targets[0]))
        {
            _contract_report(report, "DRP2 pipeline missing source-over blend state");
            ok = false;
        }
    }
    else if (blend_policy == DVZ_SCENE_BLEND_POLICY_WBOIT)
    {
        if (command->u.create_render_pipeline.color_target_count < 2 ||
            command->u.create_render_pipeline.color_targets[0].format !=
                VK_FORMAT_R16G16B16A16_SFLOAT ||
            command->u.create_render_pipeline.color_targets[1].format != VK_FORMAT_R16_SFLOAT ||
            !command->u.create_render_pipeline.color_targets[0].blend_enabled ||
            !command->u.create_render_pipeline.color_targets[1].blend_enabled)
        {
            _contract_report(report, "DRP2 pipeline missing WBOIT accumulation targets");
            ok = false;
        }
    }
    else if (blend_policy == DVZ_SCENE_BLEND_POLICY_DEPTH_PEEL)
    {
        if (command->u.create_render_pipeline.color_target_count < 3 ||
            command->u.create_render_pipeline.color_targets[0].format !=
                VK_FORMAT_R16G16B16A16_SFLOAT ||
            command->u.create_render_pipeline.color_targets[1].format !=
                VK_FORMAT_R16G16B16A16_SFLOAT ||
            command->u.create_render_pipeline.color_targets[2].format !=
                VK_FORMAT_R16G16B16A16_SFLOAT)
        {
            _contract_report(report, "DRP2 pipeline missing depth-peel color targets");
            ok = false;
        }
    }
    else if (
        blend_policy == DVZ_SCENE_BLEND_POLICY_OPAQUE &&
        command->u.create_render_pipeline.color_target_count > 0 &&
        command->u.create_render_pipeline.color_targets[0].blend_enabled)
    {
        _contract_report(report, "opaque DRP2 pipeline unexpectedly enables blending");
        ok = false;
    }

    (void)graph_pass;
    return ok;
}



/**
 * Validate an emitted BeginRenderPass command against a FramePlan render node.
 *
 * @param plan the FramePlan
 * @param render the matching render node
 * @param command the BeginRenderPass command
 * @param report optional diagnostic report
 * @return whether the emitted render-pass attachment shape matches the contract
 */
static bool _contract_validate_drp2_begin_render_pass(
    const DvzFramePlan* plan, const DvzFramePlanNode* render, const DvzDrp2Command* command,
    DvzDiagnosticReport* report)
{
    ANN(plan);
    ANN(render);
    ANN(command);
    bool ok = true;
    const DvzFrameGraphPass* graph_pass = _contract_graph_pass_for_render(plan, render);
    if (graph_pass != NULL)
    {
        if (command->u.begin_render_pass.color_attachment_count !=
            graph_pass->color_attachment_count)
        {
            _contract_report(report, "DRP2 render pass color attachment count mismatch");
            ok = false;
        }
        if (command->u.begin_render_pass.has_depth_attachment != graph_pass->has_depth_attachment)
        {
            _contract_report(report, "DRP2 render pass depth attachment mismatch");
            ok = false;
        }
    }

    bool needs_fixed_depth = false;
    for (uint32_t i = 0; i < render->u.render.visual_count; i++)
    {
        const DvzFramePlanVisualMeta* meta = &render->u.render.visual_metadata[i];
        if (meta->has_draw_contract &&
            (meta->draw_depth_policy &
             (DVZ_SCENE_DEPTH_POLICY_TEST | DVZ_SCENE_DEPTH_POLICY_WRITE)) != 0)
            needs_fixed_depth = true;
    }
    if (needs_fixed_depth && !command->u.begin_render_pass.has_depth_attachment)
    {
        _contract_report(report, "DRP2 render pass missing contracted depth attachment");
        ok = false;
    }
    return ok;
}



/**
 * Validate that graph dependencies are already in topological pass order.
 *
 * @param plan the FramePlan
 * @param report optional diagnostic report
 * @return whether every producer appears before its consumer
 */
static bool _contract_validate_graph_topology(
    const DvzFramePlan* plan, DvzDiagnosticReport* report)
{
    ANN(plan);
    bool ok = true;
    uint32_t count = dvz_frame_plan_graph_dependency_count(plan);
    for (uint32_t i = 0; i < count; i++)
    {
        DvzFrameGraphDependency dependency = {0};
        if (!dvz_frame_plan_graph_dependency_get(plan, i, &dependency))
        {
            _contract_report(report, "FramePlan graph dependency lookup failed");
            ok = false;
            continue;
        }
        if (dependency.producer_pass_index >= dependency.consumer_pass_index)
        {
            _contract_report(report, "FramePlan graph pass order is not topological");
            ok = false;
        }
    }
    return ok;
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
    if (pass_role == DVZ_FRAME_PLAN_RENDER_PASS_GBUFFER &&
        facts->visual_type != DVZ_VISUAL_TYPE_SPHERE)
        out->needs_material_set = false;
    if (scene_depth_pass)
    {
        out->needs_material_set = false;
        out->needs_image_set = false;
    }
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
            if (_scene_render_role_requires_graph_pass(render->u.render.pass_role))
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



/**
 * Validate an emitted DRP2 stream against stored FramePlan render contracts.
 *
 * @param plan the completed FramePlan
 * @param stream the emitted DRP2 command stream
 * @param report optional diagnostic report
 * @return whether labeled DRP2 render passes and draws match their stored contracts
 */
bool _scene_frame_plan_drp2_contracts_validate(
    const DvzFramePlan* plan, const DvzDrp2CommandStream* stream, DvzDiagnosticReport* report)
{
    ANN(plan);
    ANN(stream);
    bool ok = _contract_validate_graph_topology(plan, report);
    const DvzFramePlanNode* active_render = NULL;
    const DvzFrameGraphPass* active_graph_pass = NULL;
    ContractDrp2PassState active_pass_state = {0};
    uint32_t active_draw_index = 0;
    const DvzDrp2Command* active_pipeline = NULL;

    for (uint32_t i = 0; i < stream->count; i++)
    {
        const DvzDrp2Command* command = &stream->commands[i];
        switch (command->type)
        {
        case DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS:
        {
            active_render = NULL;
            active_graph_pass = NULL;
            dvz_memset(
                &active_pass_state, sizeof(ContractDrp2PassState), 0,
                sizeof(ContractDrp2PassState));
            active_draw_index = 0;
            active_pipeline = NULL;
            const char* label = dvz_drp2_stream_label(stream, command->u.begin_render_pass.id);
            if (label == NULL)
                break;
            active_render = _contract_render_for_pass_id(plan, label);
            if (active_render == NULL)
                break;
            active_graph_pass = _contract_graph_pass_for_render(plan, active_render);
            if (!_contract_validate_drp2_begin_render_pass(plan, active_render, command, report))
                ok = false;
            if (!_contract_resolve_drp2_pass_state(stream, command, &active_pass_state, report))
                ok = false;
            break;
        }
        case DVZ_DRP2_COMMAND_SET_PIPELINE:
            if (active_render != NULL)
            {
                active_pipeline =
                    _contract_drp2_pipeline_for_id(stream, command->u.set_pipeline.pipeline_id);
            }
            break;
        case DVZ_DRP2_COMMAND_SET_BIND_GROUP:
            if (active_render != NULL && active_graph_pass != NULL)
            {
                uint64_t bind_group_id = command->u.set_bind_group.bind_group_id;
                const DvzDrp2Command* bind_group =
                    _contract_drp2_bind_group_for_id(stream, bind_group_id);
                if (bind_group != NULL)
                    _contract_mark_drp2_sampled_reads(
                        stream, active_graph_pass, bind_group, &active_pass_state);
            }
            break;
        case DVZ_DRP2_COMMAND_DRAW:
        case DVZ_DRP2_COMMAND_DRAW_INDEXED:
            if (active_render == NULL)
                break;
            if (active_render->u.render.visual_count == 0)
                break;
            if (active_draw_index >= active_render->u.render.visual_count)
            {
                _contract_report(report, "DRP2 render pass has more draws than its contract");
                ok = false;
                break;
            }
            if (active_pipeline != NULL)
            {
                const DvzFramePlanVisualMeta* meta =
                    &active_render->u.render.visual_metadata[active_draw_index];
                if (meta->has_draw_contract &&
                    !_contract_validate_drp2_pipeline(
                        stream, active_pipeline, meta, active_graph_pass, &active_pass_state,
                        report))
                    ok = false;
            }
            active_draw_index++;
            break;
        case DVZ_DRP2_COMMAND_END_RENDER_PASS:
            if (!_contract_validate_drp2_sampled_reads(
                    active_graph_pass, &active_pass_state, report))
                ok = false;
            if (active_render != NULL && active_render->u.render.visual_count > 0 &&
                active_draw_index != active_render->u.render.visual_count)
            {
                _contract_report(report, "DRP2 render pass draw count does not match contract");
                ok = false;
            }
            active_render = NULL;
            active_graph_pass = NULL;
            dvz_memset(
                &active_pass_state, sizeof(ContractDrp2PassState), 0,
                sizeof(ContractDrp2PassState));
            active_draw_index = 0;
            active_pipeline = NULL;
            break;
        default:
            break;
        }
    }
    return ok;
}
