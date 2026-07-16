/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene render DRP2 contract checker                                                          */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "internal.h"

#include <stdlib.h>
#include <string.h>

#include <vulkan/vulkan_core.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "../../drp2/_stream.h"



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
 * @param format emitted texture format token, where zero means DRP2's default color format
 * @return effective texture format token
 */
static uint32_t _contract_effective_color_format(uint32_t format)
{
    return format != 0 ? format : DVZ_FORMAT_R8G8B8A8_UNORM;
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
 * Validate one emitted DRP2 color target against an explicit scene blend target contract.
 *
 * @param expected the expected color-target contract
 * @param actual the emitted color-target state
 * @param report optional diagnostic report
 * @return whether the emitted target matches the contract
 */
static bool _contract_validate_drp2_blend_target(
    const DvzSceneBlendTargetContract* expected, const DvzDrp2ColorTarget* actual,
    DvzDiagnosticReport* report)
{
    ANN(expected);
    ANN(actual);
    bool ok = true;
    if (expected->format != 0 &&
        _contract_effective_color_format(actual->format) != expected->format)
    {
        _contract_report(report, "DRP2 pipeline color target format mismatches blend contract");
        ok = false;
    }
    if (actual->blend_enabled != expected->blend_enabled)
    {
        _contract_report(report, "DRP2 pipeline blend enable mismatches contract");
        ok = false;
    }
    if (actual->color_write_mask != expected->color_write_mask)
    {
        _contract_report(report, "DRP2 pipeline color write mask mismatches contract");
        ok = false;
    }
    if (!expected->blend_enabled)
        return ok;

    if (
        actual->src_color_blend_factor != expected->src_color_blend_factor ||
        actual->dst_color_blend_factor != expected->dst_color_blend_factor ||
        actual->color_blend_op != expected->color_blend_op ||
        actual->src_alpha_blend_factor != expected->src_alpha_blend_factor ||
        actual->dst_alpha_blend_factor != expected->dst_alpha_blend_factor ||
        actual->alpha_blend_op != expected->alpha_blend_op)
    {
        char message[256];
        dvz_snprintf(
            message, sizeof(message),
            "DRP2 pipeline blend equation mismatches contract for target %u "
            "(actual color %u/%u/%u alpha %u/%u/%u, expected color %u/%u/%u alpha %u/%u/%u)",
            expected->target_index, actual->src_color_blend_factor,
            actual->dst_color_blend_factor, actual->color_blend_op,
            actual->src_alpha_blend_factor, actual->dst_alpha_blend_factor,
            actual->alpha_blend_op, expected->src_color_blend_factor,
            expected->dst_color_blend_factor, expected->color_blend_op,
            expected->src_alpha_blend_factor, expected->dst_alpha_blend_factor,
            expected->alpha_blend_op);
        _contract_report(report, message);
        ok = false;
    }
    return ok;
}



/**
 * Validate all target blend state for a pipeline against a draw blend policy.
 *
 * @param command the CreateRenderPipeline command
 * @param blend_policy the resolved draw blend policy
 * @param report optional diagnostic report
 * @return whether target blend state matches the resolved contract
 */
static bool _contract_validate_drp2_blend_targets(
    const DvzDrp2Command* command, DvzSceneBlendPolicy blend_policy, DvzDiagnosticReport* report)
{
    ANN(command);
    bool ok = true;
    DvzSceneBlendTargetContract targets[DVZ_DRP2_MAX_COLOR_ATTACHMENTS] = {0};
    uint32_t target_count = 0;
    _draw_blend_target_contracts(blend_policy, targets, &target_count);
    if (target_count == 0)
        return true;

    uint32_t pipeline_target_count = command->u.create_render_pipeline.color_target_count != 0 ?
                                         command->u.create_render_pipeline.color_target_count :
                                         1;
    if (pipeline_target_count < target_count)
    {
        _contract_report(report, "DRP2 pipeline color target count mismatches blend contract");
        return false;
    }

    for (uint32_t i = 0; i < target_count; i++)
    {
        uint32_t target_index = targets[i].target_index;
        if (target_index >= pipeline_target_count ||
            target_index >= DVZ_DRP2_MAX_COLOR_ATTACHMENTS)
        {
            _contract_report(report, "DRP2 pipeline blend target index is out of range");
            ok = false;
            continue;
        }
        ok = _contract_validate_drp2_blend_target(
                 &targets[i], &command->u.create_render_pipeline.color_targets[target_index],
                 report) &&
             ok;
    }
    return ok;
}



/**
 * Validate emitted DRP2 raster state against the resolved draw contract.
 *
 * @param command the CreateRenderPipeline command
 * @param meta stored visual metadata snapshot
 * @param pass_role the active render pass role
 * @param report optional diagnostic report
 * @return whether emitted raster state matches the draw contract
 */
static bool _contract_validate_drp2_raster_state(
    const DvzDrp2Command* command, const DvzFramePlanVisualMeta* meta,
    DvzFramePlanRenderPassRole pass_role, DvzDiagnosticReport* report)
{
    ANN(command);
    ANN(meta);
    bool has_raster_state = false;
    uint32_t cull_mode = 0;
    uint32_t front_face = 0;
    DvzSceneDrawFacts facts = {
        .visual_type = meta->visual_type,
        .desc_kind = meta->desc_kind,
    };
    _draw_raster_state_contract(
        &facts, pass_role, &has_raster_state, &cull_mode, &front_face);

    if (!has_raster_state)
    {
        if (command->u.create_render_pipeline.has_raster_state)
        {
            _contract_report(report, "DRP2 pipeline unexpectedly sets raster state");
            return false;
        }
        return true;
    }

    bool ok = true;
    if (!command->u.create_render_pipeline.has_raster_state)
    {
        _contract_report(report, "DRP2 pipeline missing contracted raster state");
        return false;
    }
    if (command->u.create_render_pipeline.cull_mode != cull_mode)
    {
        _contract_report(report, "DRP2 pipeline cull mode mismatches contract");
        ok = false;
    }
    if (command->u.create_render_pipeline.front_face != front_face)
    {
        _contract_report(report, "DRP2 pipeline front face mismatches contract");
        ok = false;
    }
    return ok;
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
 * Return whether a pipeline has a material-compatible bind-group layout.
 *
 * Item-state point-like shaders use a combined set-1 layout containing both material params and
 * item-state style params, so that layout satisfies the material-set contract.
 *
 * @param stream the DRP2 command stream
 * @param command the CreateRenderPipeline command
 * @return whether the pipeline references a material-compatible layout
 */
static bool _contract_pipeline_has_material_layout(
    const DvzDrp2CommandStream* stream, const DvzDrp2Command* command)
{
    return _contract_pipeline_has_layout_label(stream, command, "_bgl_material_params") ||
           _contract_pipeline_has_layout_label(stream, command, "_bgl_item_state_style");
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
        !_contract_pipeline_has_material_layout(stream, command))
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
    if ((mask & DVZ_SCENE_BIND_GROUP_REQUIREMENT_LABELS) != 0 &&
        !_contract_pipeline_has_layout_label(stream, command, "_bgl_labels"))
    {
        _contract_report(report, "DRP2 pipeline missing labels bind-group layout");
        ok = false;
    }
    if ((mask & DVZ_SCENE_BIND_GROUP_REQUIREMENT_GLYPH) != 0 &&
        !_contract_pipeline_has_layout_label(stream, command, "_bgl_glyph"))
    {
        _contract_report(report, "DRP2 pipeline missing glyph bind-group layout");
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
 * Resolve the pipeline blend policy for a graph-backed fullscreen technique pass.
 *
 * @param role the active render-pass role
 * @param out_policy output blend policy
 * @return whether the pass role owns a fullscreen pipeline contract
 */
static bool _contract_fullscreen_pipeline_blend_policy(
    DvzFramePlanRenderPassRole role, DvzSceneBlendPolicy* out_policy)
{
    ANN(out_policy);
    *out_policy = DVZ_SCENE_BLEND_POLICY_NONE;

    switch (role)
    {
    case DVZ_FRAME_PLAN_RENDER_PASS_SSAO:
    case DVZ_FRAME_PLAN_RENDER_PASS_SSAO_BLUR:
    case DVZ_FRAME_PLAN_RENDER_PASS_EDL_RESOLVE:
        *out_policy = DVZ_SCENE_BLEND_POLICY_OPAQUE;
        return true;
    case DVZ_FRAME_PLAN_RENDER_PASS_SSAO_COMPOSITE:
    case DVZ_FRAME_PLAN_RENDER_PASS_WBOIT_RESOLVE:
    case DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_COMPOSITE:
        *out_policy = DVZ_SCENE_BLEND_POLICY_SOURCE_OVER;
        return true;
    default:
        return false;
    }
}



/**
 * Validate one graph-backed fullscreen pipeline draw against its pass-role contract.
 *
 * @param command the Draw command using the fullscreen pipeline
 * @param pipeline the active CreateRenderPipeline command, or NULL for a persistent pipeline
 * @param role the active render-pass role
 * @param pass_state resolved DRP2 render-pass state
 * @param report optional diagnostic report
 * @return whether the fullscreen draw and pipeline match the pass contract
 */
static bool _contract_validate_drp2_fullscreen_pipeline(
    const DvzDrp2Command* command, const DvzDrp2Command* pipeline,
    DvzFramePlanRenderPassRole role, const ContractDrp2PassState* pass_state,
    DvzDiagnosticReport* report)
{
    ANN(command);
    ANN(pass_state);
    DvzSceneBlendPolicy blend_policy = DVZ_SCENE_BLEND_POLICY_NONE;
    if (!_contract_fullscreen_pipeline_blend_policy(role, &blend_policy))
        return true;

    if (pipeline == NULL)
        return true;

    bool ok = true;
    if (command->type != DVZ_DRP2_COMMAND_DRAW || command->u.draw.vertex_count != 3 ||
        command->u.draw.instance_count != 1 || command->u.draw.first_vertex != 0 ||
        command->u.draw.first_instance != 0)
    {
        _contract_report(report, "DRP2 fullscreen pass must draw one fullscreen triangle");
        ok = false;
    }
    if (pipeline->u.create_render_pipeline.vertex_buffer_slots != 0 ||
        pipeline->u.create_render_pipeline.binding_count != 0 ||
        pipeline->u.create_render_pipeline.attr_count != 0)
    {
        _contract_report(report, "DRP2 fullscreen pipeline must not use vertex buffers");
        ok = false;
    }
    if (pipeline->u.create_render_pipeline.bind_group_layout_count != 1)
    {
        _contract_report(report, "DRP2 fullscreen pipeline must use one bind-group layout");
        ok = false;
    }
    if (pipeline->u.create_render_pipeline.has_depth_attachment ||
        pipeline->u.create_render_pipeline.depth_write_enabled)
    {
        _contract_report(report, "DRP2 fullscreen pipeline must not use depth state");
        ok = false;
    }
    if (pipeline->u.create_render_pipeline.topology != 0 &&
        pipeline->u.create_render_pipeline.topology != DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
    {
        _contract_report(report, "DRP2 fullscreen pipeline topology mismatches contract");
        ok = false;
    }

    ok = _contract_validate_drp2_pipeline_color_targets(pipeline, pass_state, report) && ok;
    if (pass_state->has_sample_count &&
        _contract_effective_sample_count(pipeline->u.create_render_pipeline.sample_count) !=
            pass_state->sample_count)
    {
        _contract_report(report, "DRP2 fullscreen pipeline sample count mismatches render pass");
        ok = false;
    }
    ok = _contract_validate_drp2_blend_targets(pipeline, blend_policy, report) && ok;
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
 * Record graph sampled reads inferred from a persistent bind-group label.
 *
 * @param stream the DRP2 command stream
 * @param graph_pass active graph pass
 * @param bind_group_label emitted bind-group label containing dependency ids
 * @param state active render-pass checker state
 */
static void _contract_mark_drp2_sampled_reads_from_label(
    const DvzDrp2CommandStream* stream, const DvzFrameGraphPass* graph_pass,
    const char* bind_group_label, ContractDrp2PassState* state)
{
    ANN(stream);
    ANN(graph_pass);
    ANN(state);
    if (bind_group_label == NULL)
        return;

    const char* p = bind_group_label;
    while (*p != '\0')
    {
        while (*p != '\0' && (*p < '0' || *p > '9'))
            p++;
        if (*p == '\0')
            break;

        char* end = NULL;
        unsigned long long parsed = strtoull(p, &end, 10);
        if (end == p)
        {
            p++;
            continue;
        }

        const char* label = dvz_drp2_stream_label(stream, (uint64_t)parsed);
        for (uint32_t j = 0; j < graph_pass->read_count; j++)
        {
            if (graph_pass->reads[j].usage != DVZ_FRAME_GRAPH_ACCESS_SAMPLED)
                continue;
            if (_contract_resource_label_matches(label, graph_pass->reads[j].resource_id))
                state->sampled_reads_matched[j] = true;
        }
        p = end;
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
 * Return whether a persistent bind-group label references an expected resource id.
 *
 * @param stream the DRP2 command stream
 * @param bind_group_label emitted bind-group label containing dependency ids
 * @param resource_id expected graph resource id
 * @return whether one referenced object label matches the resource
 */
static bool _contract_bind_group_label_references_resource(
    const DvzDrp2CommandStream* stream, const char* bind_group_label, const char* resource_id)
{
    ANN(stream);
    ANN(resource_id);
    if (bind_group_label == NULL)
        return false;

    const char* p = bind_group_label;
    while (*p != '\0')
    {
        while (*p != '\0' && (*p < '0' || *p > '9'))
            p++;
        if (*p == '\0')
            break;

        char* end = NULL;
        unsigned long long parsed = strtoull(p, &end, 10);
        if (end == p)
        {
            p++;
            continue;
        }

        const char* label = dvz_drp2_stream_label(stream, (uint64_t)parsed);
        if (_contract_resource_label_matches(label, resource_id))
            return true;
        p = end;
    }
    return false;
}


/**
 * Validate one draw-owned sampled-resource binding against active DRP2 bind groups.
 *
 * @param stream the DRP2 command stream
 * @param active_bind_groups bind-group ids currently active by set index
 * @param set expected shader set
 * @param binding expected binding within the set
 * @param resource_id expected graph resource id
 * @param report optional diagnostic report
 * @return whether the active bind group satisfies the draw-owned resource contract
 */
static bool _contract_validate_drp2_sampled_binding(
    const DvzDrp2CommandStream* stream, const uint64_t* active_bind_groups, uint32_t set,
    uint32_t binding, const char* resource_id, DvzDiagnosticReport* report)
{
    ANN(stream);
    ANN(active_bind_groups);
    ANN(resource_id);
    if (resource_id[0] == '\0')
        return true;
    if (set >= DVZ_DRP2_MAX_BIND_GROUPS)
    {
        _contract_report(report, "DRP2 sampled resource bind set is out of range");
        return false;
    }

    uint64_t bind_group_id = active_bind_groups[set];
    if (bind_group_id == 0)
    {
        _contract_report(report, "DRP2 draw is missing a contracted sampled bind group");
        return false;
    }

    const DvzDrp2Command* bind_group = _contract_drp2_bind_group_for_id(stream, bind_group_id);
    if (bind_group == NULL)
    {
        const char* label = dvz_drp2_stream_label(stream, bind_group_id);
        if (_contract_bind_group_label_references_resource(stream, label, resource_id))
            return true;
        _contract_report(report, "DRP2 persistent bind group misses sampled resource");
        return false;
    }

    for (uint32_t i = 0; i < bind_group->u.create_bind_group.entry_count; i++)
    {
        const DvzDrp2BindGroupEntry* entry = &bind_group->u.create_bind_group.entries[i];
        if (entry->binding != binding)
            continue;
        if (entry->binding_type != DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE ||
            (entry->resource_kind != DVZ_DRP2_BINDING_RESOURCE_TEXTURE &&
             entry->resource_kind != DVZ_DRP2_BINDING_RESOURCE_TEXTURE_VIEW))
        {
            _contract_report(report, "DRP2 sampled binding has wrong resource type");
            return false;
        }
        const char* label = dvz_drp2_stream_label(stream, entry->resource_id);
        if (!_contract_resource_label_matches(label, resource_id))
        {
            _contract_report(report, "DRP2 sampled binding resource mismatches contract");
            return false;
        }
        return true;
    }

    _contract_report(report, "DRP2 bind group misses contracted sampled binding");
    return false;
}


/**
 * Validate draw-owned occlusion sampled-resource bindings against active DRP2 state.
 *
 * @param stream the DRP2 command stream
 * @param active_bind_groups bind-group ids currently active by set index
 * @param meta stored visual metadata snapshot
 * @param report optional diagnostic report
 * @return whether all draw-owned occlusion bindings are satisfied
 */
static bool _contract_validate_drp2_draw_occlusion_bindings(
    const DvzDrp2CommandStream* stream, const uint64_t* active_bind_groups,
    const DvzFramePlanVisualMeta* meta, DvzDiagnosticReport* report)
{
    ANN(stream);
    ANN(active_bind_groups);
    ANN(meta);
    bool ok = true;
    ok = _contract_validate_drp2_sampled_binding(
             stream, active_bind_groups, meta->draw_volume_occlusion_bind_set,
             meta->draw_volume_occlusion_bind_binding, meta->draw_volume_occlusion_resource_id,
             report) &&
         ok;
    ok = _contract_validate_drp2_sampled_binding(
             stream, active_bind_groups, meta->draw_scene_occlusion_bind_set,
             meta->draw_scene_occlusion_bind_binding, meta->draw_scene_occlusion_resource_id,
             report) &&
         ok;
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
    DvzFramePlanRenderPassRole pass_role, const ContractDrp2PassState* pass_state,
    DvzDiagnosticReport* report)
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
         blend_policy == DVZ_SCENE_BLEND_POLICY_ADDITIVE ||
         blend_policy == DVZ_SCENE_BLEND_POLICY_WBOIT ||
         blend_policy == DVZ_SCENE_BLEND_POLICY_DEPTH_PEEL) &&
        command->u.create_render_pipeline.depth_write_enabled)
    {
        _contract_report(report, "transparent DRP2 pipeline writes depth");
        ok = false;
    }

    ok = _contract_validate_drp2_blend_targets(command, blend_policy, report) && ok;
    ok = _contract_validate_drp2_raster_state(command, meta, pass_role, report) && ok;

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

bool _scene_frame_plan_drp2_contracts_validate(
    const DvzFramePlan* plan, const DvzDrp2CommandStream* stream, DvzDiagnosticReport* report)
{
    ANN(plan);
    ANN(stream);
    bool ok = _contract_validate_graph_topology(plan, report);
    ok = _contract_validate_graph_backed_render_nodes(plan, report) && ok;
    const DvzFramePlanNode* active_render = NULL;
    const DvzFrameGraphPass* active_graph_pass = NULL;
    ContractDrp2PassState active_pass_state = {0};
    uint32_t active_draw_index = 0;
    const DvzDrp2Command* active_pipeline = NULL;
    uint64_t active_bind_groups[DVZ_DRP2_MAX_BIND_GROUPS] = {0};

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
            dvz_memset(
                active_bind_groups, sizeof(active_bind_groups), 0, sizeof(active_bind_groups));
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
                if (command->u.set_bind_group.slot < DVZ_DRP2_MAX_BIND_GROUPS)
                    active_bind_groups[command->u.set_bind_group.slot] =
                        command->u.set_bind_group.bind_group_id;
                uint64_t bind_group_id = command->u.set_bind_group.bind_group_id;
                const DvzDrp2Command* bind_group =
                    _contract_drp2_bind_group_for_id(stream, bind_group_id);
                if (bind_group != NULL)
                {
                    _contract_mark_drp2_sampled_reads(
                        stream, active_graph_pass, bind_group, &active_pass_state);
                }
                else
                {
                    const char* label = dvz_drp2_stream_label(stream, bind_group_id);
                    _contract_mark_drp2_sampled_reads_from_label(
                        stream, active_graph_pass, label, &active_pass_state);
                }
            }
            break;
        case DVZ_DRP2_COMMAND_DRAW:
        case DVZ_DRP2_COMMAND_DRAW_INDEXED:
            if (active_render == NULL)
                break;
            if (active_render->u.render.visual_count == 0)
            {
                if (!_contract_validate_drp2_fullscreen_pipeline(
                        command, active_pipeline, active_render->u.render.pass_role,
                        &active_pass_state, report))
                    ok = false;
                break;
            }
            if (active_draw_index >= active_render->u.render.visual_count)
            {
                _contract_report(report, "DRP2 render pass has more draws than its contract");
                ok = false;
                break;
            }
            const DvzFramePlanVisualMeta* meta =
                &active_render->u.render.visual_metadata[active_draw_index];
            if (meta->has_draw_contract &&
                !_contract_validate_drp2_draw_occlusion_bindings(
                    stream, active_bind_groups, meta, report))
                ok = false;
            if (active_pipeline != NULL)
            {
                if (meta->has_draw_contract &&
                    !_contract_validate_drp2_pipeline(
                        stream, active_pipeline, meta, active_graph_pass,
                        active_render->u.render.pass_role, &active_pass_state, report))
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
            dvz_memset(
                active_bind_groups, sizeof(active_bind_groups), 0, sizeof(active_bind_groups));
            break;
        default:
            break;
        }
    }
    return ok;
}
