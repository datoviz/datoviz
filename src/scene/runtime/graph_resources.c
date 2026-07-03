/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene FramePlan runtime graph resources */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <vulkan/vulkan_core.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "frame_plan/frame_plan.h"
#include "frame_plan/emit.h"
#include "_frame_plan_runtime_internal.h"
#include "_frame_plan_runtime_upload.h"
#include "_render_pass.h"
#include "_scene.h"
#include "_scene_common_bindings.h"
#include "_scene_resource_key.h"
#include "_scene_shader_abi.h"
#include "_shader_registry.h"
#include "_technique.h"
#include "_visual_pipeline.h"
#include "datoviz/drp2.h"
#include "datoviz/drp2/stream.h"
#include "datoviz/scene.h"
#include "render_contract/render_contract.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return the configured render-target extent, falling back to fixture dimensions.
 *
 * @param cfg optional frame-plan emit configuration.
 * @param width output width in pixels.
 * @param height output height in pixels.
 */
void _emit_target_extent(const DvzFramePlanEmitConfig* cfg, uint32_t* width, uint32_t* height)
{
    ANN(width);
    ANN(height);
    *width = (cfg != NULL && cfg->target_width > 0) ? cfg->target_width : 4;
    *height = (cfg != NULL && cfg->target_height > 0) ? cfg->target_height : 4;
}



/**
 * Return a graph resource descriptor by id.
 *
 * @param plan the FramePlan.
 * @param resource_id the graph resource id.
 * @return the resource descriptor, or NULL when absent.
 */
const DvzFrameGraphResource*
_graph_resource_by_id(const DvzFramePlan* plan, const char* resource_id)
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
 * Return a graph pass descriptor by panel and work label.
 *
 * @param plan the FramePlan.
 * @param panel_id the panel id.
 * @param work_label the graph pass work label.
 * @return the graph pass descriptor, or NULL when absent.
 */
const DvzFrameGraphPass*
_graph_pass_by_panel_work(const DvzFramePlan* plan, const char* panel_id, const char* work_label)
{
    ANN(plan);
    ANN(panel_id);
    ANN(work_label);
    for (uint32_t i = 0; i < dvz_frame_plan_graph_pass_count(plan); i++)
    {
        const DvzFrameGraphPass* pass = dvz_frame_plan_graph_pass_get(plan, i);
        if (pass != NULL && strcmp(pass->panel_id, panel_id) == 0 &&
            strcmp(pass->work_label, work_label) == 0)
            return pass;
    }
    return NULL;
}


/**
 * Return a graph pass descriptor by id.
 *
 * @param plan the FramePlan.
 * @param pass_id graph pass id.
 * @return the graph pass descriptor, or NULL when absent.
 */
const DvzFrameGraphPass* _graph_pass_by_id(const DvzFramePlan* plan, const char* pass_id)
{
    ANN(plan);
    ANN(pass_id);
    for (uint32_t i = 0; i < dvz_frame_plan_graph_pass_count(plan); i++)
    {
        const DvzFrameGraphPass* pass = dvz_frame_plan_graph_pass_get(plan, i);
        if (pass != NULL && strcmp(pass->id, pass_id) == 0)
            return pass;
    }
    return NULL;
}



/**
 * Return the graph pass associated with a render node.
 *
 * @param plan the FramePlan.
 * @param render render node.
 * @return the graph pass descriptor, or NULL when absent.
 */
const DvzFrameGraphPass*
_graph_pass_for_render(const DvzFramePlan* plan, const DvzFramePlanNode* render)
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
        const char* candidate_label = _scene_render_role_work_label(candidate->u.render.pass_role);
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
 * Return the render node associated with a graph pass descriptor.
 *
 * @param plan the FramePlan.
 * @param pass graph pass descriptor.
 * @return the matching render node, or NULL when absent.
 */
const DvzFramePlanNode*
_graph_render_for_pass(const DvzFramePlan* plan, const DvzFrameGraphPass* pass)
{
    ANN(plan);
    ANN(pass);
    uint32_t ordinal = 0;
    for (uint32_t i = 0; i < dvz_frame_plan_graph_pass_count(plan); i++)
    {
        const DvzFrameGraphPass* candidate = dvz_frame_plan_graph_pass_get(plan, i);
        if (candidate == pass)
            break;
        if (candidate != NULL && strcmp(candidate->panel_id, pass->panel_id) == 0 &&
            strcmp(candidate->work_label, pass->work_label) == 0)
            ordinal++;
    }

    uint32_t seen = 0;
    for (uint32_t i = 0; i < plan->count; i++)
    {
        const DvzFramePlanNode* render = &plan->nodes[i];
        if (render->type != DVZ_FRAME_PLAN_NODE_RENDER)
            continue;
        const char* work_label = _scene_render_role_work_label(render->u.render.pass_role);
        if (work_label[0] != '\0' && strcmp(render->u.render.panel_id, pass->panel_id) == 0 &&
            strcmp(work_label, pass->work_label) == 0)
        {
            if (seen == ordinal)
                return render;
            seen++;
        }
    }
    return NULL;
}



/**
 * Convert graph texture usage flags to DRP2 texture usage flags.
 *
 * @param usage_flags graph resource usage flags.
 * @return DRP2 texture usage flags.
 */
uint32_t _graph_texture_usage_to_drp2(uint32_t usage_flags)
{
    uint32_t out = 0;
    if ((usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT) != 0 ||
        (usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_DEPTH_ATTACHMENT) != 0)
        out |= DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT;
    if ((usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED) != 0)
        out |= DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING;
    if ((usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_STORAGE) != 0)
        out |= DVZ_DRP2_TEXTURE_USAGE_STORAGE_BINDING;
    if ((usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_COPY_SRC) != 0)
        out |= DVZ_DRP2_TEXTURE_USAGE_COPY_SRC;
    if ((usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_COPY_DST) != 0)
        out |= DVZ_DRP2_TEXTURE_USAGE_COPY_DST;
    return out != 0 ? out : DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT;
}



/**
 * Convert one declared graph access into DRP2 texture usage flags.
 *
 * @param usage graph pass access usage.
 * @return DRP2 texture usage flags.
 */
uint32_t _graph_access_usage_to_drp2(DvzFrameGraphAccessUsage usage)
{
    switch (usage)
    {
    case DVZ_FRAME_GRAPH_ACCESS_SAMPLED:
        return DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING;
    case DVZ_FRAME_GRAPH_ACCESS_STORAGE_READ:
    case DVZ_FRAME_GRAPH_ACCESS_STORAGE_WRITE:
        return DVZ_DRP2_TEXTURE_USAGE_STORAGE_BINDING;
    case DVZ_FRAME_GRAPH_ACCESS_COLOR_ATTACHMENT:
    case DVZ_FRAME_GRAPH_ACCESS_DEPTH_ATTACHMENT_READ:
    case DVZ_FRAME_GRAPH_ACCESS_DEPTH_ATTACHMENT_WRITE:
        return DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT;
    case DVZ_FRAME_GRAPH_ACCESS_COPY_SRC:
        return DVZ_DRP2_TEXTURE_USAGE_COPY_SRC;
    case DVZ_FRAME_GRAPH_ACCESS_COPY_DST:
        return DVZ_DRP2_TEXTURE_USAGE_COPY_DST;
    default:
        return 0;
    }
}



/**
 * Return the graph access implied by a depth attachment declaration.
 *
 * @param attachment graph attachment descriptor.
 * @return graph access usage.
 */
DvzFrameGraphAccessUsage _graph_depth_attachment_usage(const DvzFrameGraphAttachment* attachment)
{
    ANN(attachment);
    if (attachment->access == DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ)
        return DVZ_FRAME_GRAPH_ACCESS_DEPTH_ATTACHMENT_READ;
    if (attachment->access == DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_NONE)
        return DVZ_FRAME_GRAPH_ACCESS_NONE;
    return DVZ_FRAME_GRAPH_ACCESS_DEPTH_ATTACHMENT_WRITE;
}


/**
 * Compute DRP2 texture usage from all graph pass access declarations for a resource.
 *
 * @param plan the FramePlan.
 * @param resource_id graph resource id.
 * @return DRP2 texture usage flags implied by graph passes.
 */
uint32_t _graph_declared_texture_usage_to_drp2(const DvzFramePlan* plan, const char* resource_id)
{
    ANN(plan);
    ANN(resource_id);
    uint32_t usage = 0;
    for (uint32_t i = 0; i < dvz_frame_plan_graph_pass_count(plan); i++)
    {
        const DvzFrameGraphPass* pass = dvz_frame_plan_graph_pass_get(plan, i);
        if (pass == NULL)
            continue;
        for (uint32_t j = 0; j < pass->read_count; j++)
        {
            if (strcmp(pass->reads[j].resource_id, resource_id) == 0)
                usage |= _graph_access_usage_to_drp2(pass->reads[j].usage);
        }
        for (uint32_t j = 0; j < pass->write_count; j++)
        {
            if (strcmp(pass->writes[j].resource_id, resource_id) == 0)
                usage |= _graph_access_usage_to_drp2(pass->writes[j].usage);
        }
        for (uint32_t j = 0; j < pass->color_attachment_count; j++)
        {
            if (strcmp(pass->color_attachments[j].resource_id, resource_id) == 0)
                usage |= _graph_access_usage_to_drp2(DVZ_FRAME_GRAPH_ACCESS_COLOR_ATTACHMENT);
            if (strcmp(pass->color_attachments[j].resolve_resource_id, resource_id) == 0)
                usage |= _graph_access_usage_to_drp2(DVZ_FRAME_GRAPH_ACCESS_COLOR_ATTACHMENT);
        }
        if (pass->has_depth_attachment &&
            strcmp(pass->depth_attachment.resource_id, resource_id) == 0)
        {
            usage |= _graph_access_usage_to_drp2(
                _graph_depth_attachment_usage(&pass->depth_attachment));
        }
    }
    return usage;
}



/**
 * Convert a graph attachment load operation to a DRP2 attachment load operation.
 *
 * @param op graph attachment load operation.
 * @return DRP2 attachment load operation.
 */
DvzDrp2AttachmentLoadOp _graph_load_op_to_drp2(DvzFrameGraphAttachmentLoadOp op)
{
    switch (op)
    {
    case DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR:
        return DVZ_DRP2_ATTACHMENT_LOAD_CLEAR;
    case DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD:
        return DVZ_DRP2_ATTACHMENT_LOAD_LOAD;
    case DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_DONT_CARE:
        return DVZ_DRP2_ATTACHMENT_LOAD_DONT_CARE;
    default:
        return DVZ_DRP2_ATTACHMENT_LOAD_LOAD;
    }
}



/**
 * Convert a graph attachment store operation to a DRP2 attachment store operation.
 *
 * @param op graph attachment store operation.
 * @return DRP2 attachment store operation.
 */
DvzDrp2AttachmentStoreOp _graph_store_op_to_drp2(DvzFrameGraphAttachmentStoreOp op)
{
    switch (op)
    {
    case DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE:
        return DVZ_DRP2_ATTACHMENT_STORE_STORE;
    case DVZ_FRAME_GRAPH_ATTACHMENT_STORE_DONT_CARE:
        return DVZ_DRP2_ATTACHMENT_STORE_DONT_CARE;
    default:
        return DVZ_DRP2_ATTACHMENT_STORE_STORE;
    }
}



/**
 * Convert graph attachment access to DRP2 attachment access.
 *
 * @param access graph attachment access.
 * @return DRP2 attachment access.
 */
DvzDrp2AttachmentAccess _graph_attachment_access_to_drp2(DvzFrameGraphAttachmentAccess access)
{
    switch (access)
    {
    case DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ:
        return DVZ_DRP2_ATTACHMENT_ACCESS_READ;
    case DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ_WRITE:
        return DVZ_DRP2_ATTACHMENT_ACCESS_READ_WRITE;
    case DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_NONE:
    case DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE:
    default:
        return DVZ_DRP2_ATTACHMENT_ACCESS_WRITE;
    }
}


/**
 * Return a graph resource's effective sample count.
 *
 * @param resource the graph resource
 * @return sample count, defaulting to 1 when unset
 */
uint32_t _graph_resource_sample_count(const DvzFrameGraphResource* resource)
{
    return resource != NULL && resource->sample_count != 0 ? resource->sample_count : 1;
}


/**
 * Clamp a requested sample count to a supported power-of-two sample count.
 *
 * @param sample_count requested sample count.
 * @param max_sample_count maximum supported sample count.
 * @return supported sample count.
 */
uint32_t _sample_count_lowered(uint32_t sample_count, uint32_t max_sample_count)
{
    if (sample_count <= 1 || max_sample_count <= 1)
        return 1;
    if (sample_count >= 16 && max_sample_count >= 16)
        return 16;
    if (sample_count >= 8 && max_sample_count >= 8)
        return 8;
    if (sample_count >= 4 && max_sample_count >= 4)
        return 4;
    if (sample_count >= 2 && max_sample_count >= 2)
        return 2;
    return 1;
}


/**
 * Return a graph resource's capability-lowered sample count.
 *
 * @param emitter runtime emitter carrying current device limits.
 * @param resource the graph resource.
 * @return supported sample count, defaulting to one when unset.
 */
uint32_t _graph_resource_lowered_sample_count(
    const DvzFramePlanEmitter* emitter, const DvzFrameGraphResource* resource)
{
    uint32_t sample_count = _graph_resource_sample_count(resource);
    if (sample_count <= 1 || emitter == NULL || resource == NULL)
        return sample_count;

    uint32_t max_sample_count = 16;
    bool color = (resource->usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT) != 0;
    bool depth = (resource->usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_DEPTH_ATTACHMENT) != 0;
    if (color || depth)
    {
        uint32_t color_max =
            emitter->max_color_sample_count != 0 ? emitter->max_color_sample_count : 1;
        uint32_t depth_max =
            emitter->max_depth_sample_count != 0 ? emitter->max_depth_sample_count : 1;
        max_sample_count = color_max < depth_max ? color_max : depth_max;
    }
    if (max_sample_count == 0)
        max_sample_count = 1;
    return _sample_count_lowered(sample_count, max_sample_count);
}


/**
 * Return the raster sample count implied by a render graph pass.
 *
 * @param emitter runtime emitter carrying current device limits.
 * @param plan the FramePlan carrying graph resources
 * @param pass graph pass descriptor
 * @return raster sample count, defaulting to 1
 */
uint32_t _graph_render_pass_sample_count(
    const DvzFramePlanEmitter* emitter, const DvzFramePlan* plan, const DvzFrameGraphPass* pass)
{
    if (plan == NULL || pass == NULL || pass->color_attachment_count == 0)
        return 1;
    const DvzFrameGraphResource* resource =
        _graph_resource_by_id(plan, pass->color_attachments[0].resource_id);
    uint32_t sample_count = _graph_resource_lowered_sample_count(emitter, resource);
    if (pass->has_depth_attachment)
    {
        const DvzFrameGraphResource* depth =
            _graph_resource_by_id(plan, pass->depth_attachment.resource_id);
        uint32_t depth_sample_count = _graph_resource_lowered_sample_count(emitter, depth);
        if (depth_sample_count < sample_count)
            sample_count = depth_sample_count;
    }
    return sample_count;
}



/**
 * Apply graph color attachment load/store operations to the current DRP2 render pass command.
 *
 * @param stream destination DRP2 command stream.
 * @param pass graph pass descriptor.
 * @return whether the command was updated.
 */
bool _stream_apply_graph_color_ops(
    DvzDrp2CommandStream* stream, const DvzFrameGraphPass* pass, uint64_t final_color_id,
    const SceneGraphRuntimeTargets* targets)
{
    ANN(stream);
    if (pass == NULL)
        return true;
    bool ok = true;
    for (uint32_t i = 0; ok && i < pass->color_attachment_count; i++)
    {
        const DvzFrameGraphAttachment* attachment = &pass->color_attachments[i];
        ok = dvz_drp2_stream_begin_render_pass_set_color_attachment_ops(
                 stream, i, _graph_load_op_to_drp2(attachment->load_op),
                 _graph_store_op_to_drp2(attachment->store_op)) &&
             dvz_drp2_stream_begin_render_pass_set_color_attachment_access(
                 stream, i, _graph_attachment_access_to_drp2(attachment->access));
        if (ok && attachment->resolve_resource_id[0] != '\0')
        {
            uint64_t resolve_id = _graph_runtime_texture_id_for_resource(
                attachment->resolve_resource_id, final_color_id, targets, 0);
            ok = resolve_id != 0 && dvz_drp2_stream_begin_render_pass_set_color_attachment_resolve(
                                        stream, i, resolve_id, attachment->resolve_mode);
        }
    }
    return ok;
}



/**
 * Apply graph depth attachment state to the current DRP2 render pass command.
 *
 * @param stream destination DRP2 command stream.
 * @param pass graph pass descriptor.
 * @param depth_id named depth texture id, or zero for no graph depth.
 * @return whether the command was updated.
 */
bool _stream_apply_graph_depth(
    DvzDrp2CommandStream* stream, const DvzFrameGraphPass* pass, uint64_t depth_id)
{
    ANN(stream);
    if (pass == NULL || !pass->has_depth_attachment || depth_id == 0)
        return true;
    const DvzFrameGraphAttachment* attachment = &pass->depth_attachment;
    return dvz_drp2_stream_begin_render_pass_set_depth_texture(
               stream, depth_id, attachment->clear_depth) &&
           dvz_drp2_stream_begin_render_pass_set_depth_ops(
               stream, _graph_load_op_to_drp2(attachment->load_op),
               _graph_store_op_to_drp2(attachment->store_op)) &&
           dvz_drp2_stream_begin_render_pass_set_depth_access(
               stream, _graph_attachment_access_to_drp2(attachment->access));
}



/**
 * Register one graph resource id to runtime texture id mapping.
 *
 * @param targets runtime target map.
 * @param resource_id graph resource id.
 * @param texture_id runtime texture id.
 * @return whether the mapping was registered.
 */
bool _graph_runtime_targets_add(
    SceneGraphRuntimeTargets* targets, const char* resource_id, uint64_t texture_id)
{
    ANN(targets);
    ANN(resource_id);

    if (resource_id[0] == '\0' || texture_id == 0)
        return true;
    for (uint32_t i = 0; i < targets->count; i++)
    {
        if (strcmp(targets->targets[i].resource_id, resource_id) == 0)
        {
            targets->targets[i].texture_id = texture_id;
            return true;
        }
    }
    if (targets->count >= DVZ_FRAME_PLAN_INITIAL_GRAPH_RESOURCE_CAPACITY)
        return false;

    SceneGraphRuntimeTarget* target = &targets->targets[targets->count++];
    dvz_strlcpy(target->resource_id, resource_id, sizeof(target->resource_id));
    target->texture_id = texture_id;
    return true;
}



/**
 * Return one registered runtime texture id by graph resource id.
 *
 * @param targets runtime target map.
 * @param resource_id graph resource id.
 * @return runtime texture id, or zero when no mapping exists.
 */
uint64_t
_graph_runtime_targets_get(const SceneGraphRuntimeTargets* targets, const char* resource_id)
{
    if (targets == NULL || resource_id == NULL || resource_id[0] == '\0')
        return 0;
    for (uint32_t i = 0; i < targets->count; i++)
    {
        if (strcmp(targets->targets[i].resource_id, resource_id) == 0)
            return targets->targets[i].texture_id;
    }
    return 0;
}



/**
 * Return the runtime texture id for one graph resource id.
 *
 * @param resource_id graph resource id.
 * @param final_color_id runtime id of the final render target.
 * @param targets optional runtime targets for the panel.
 * @param fallback_id fallback runtime id when no graph declaration is available.
 * @return runtime texture id for the graph resource.
 */
uint64_t _graph_runtime_texture_id_for_resource(
    const char* resource_id, uint64_t final_color_id, const SceneGraphRuntimeTargets* targets,
    uint64_t fallback_id)
{
    if (resource_id == NULL || resource_id[0] == '\0')
        return fallback_id;
    if (strcmp(resource_id, "rt") == 0)
        return final_color_id;
    if (targets != NULL)
    {
        uint64_t texture_id = _graph_runtime_targets_get(targets, resource_id);
        if (texture_id != 0)
            return texture_id;
    }
    return fallback_id;
}



/**
 * Return the runtime texture id declared by one graph color attachment.
 *
 * @param pass graph pass descriptor, or NULL.
 * @param attachment_index color attachment index.
 * @param final_color_id runtime id of the final render target.
 * @param targets optional runtime targets for the panel.
 * @param fallback_id fallback runtime id when no graph declaration is available.
 * @return runtime texture id for the graph color attachment.
 */
uint64_t _graph_color_attachment_texture_id(
    const DvzFrameGraphPass* pass, uint32_t attachment_index, uint64_t final_color_id,
    const SceneGraphRuntimeTargets* targets, uint64_t fallback_id)
{
    if (pass == NULL || attachment_index >= pass->color_attachment_count)
        return fallback_id;

    return _graph_runtime_texture_id_for_resource(
        pass->color_attachments[attachment_index].resource_id, final_color_id, targets,
        fallback_id);
}



/**
 * Return the runtime texture id declared by one graph sampled read.
 *
 * @param pass graph pass descriptor, or NULL.
 * @param read_index sampled read index.
 * @param final_color_id runtime id of the final render target.
 * @param targets optional runtime targets for the panel.
 * @param fallback_id fallback runtime id when no graph declaration is available.
 * @return runtime texture id for the graph sampled read.
 */
uint64_t _graph_sampled_read_texture_id(
    const DvzFrameGraphPass* pass, uint32_t read_index, uint64_t final_color_id,
    const SceneGraphRuntimeTargets* targets, uint64_t fallback_id)
{
    if (pass == NULL || read_index >= pass->read_count)
        return fallback_id;

    return _graph_runtime_texture_id_for_resource(
        pass->reads[read_index].resource_id, final_color_id, targets, fallback_id);
}


/**
 * Return the index of a sampled volume-occlusion read in a graph pass.
 *
 * @param pass graph pass descriptor, or NULL
 * @param out_read_index output read index
 * @return whether a volume occlusion read was found
 */
bool _graph_volume_occlusion_read_index(const DvzFrameGraphPass* pass, uint32_t* out_read_index)
{
    ANN(out_read_index);
    if (pass == NULL)
        return false;
    for (uint32_t i = 0; i < pass->read_count; i++)
    {
        if (pass->reads[i].usage == DVZ_FRAME_GRAPH_ACCESS_SAMPLED &&
            _scene_resource_id_has_suffix(pass->reads[i].resource_id, ".volume_occlusion.depth"))
        {
            *out_read_index = i;
            return true;
        }
    }
    return false;
}


/**
 * Resolve the sampled volume-occlusion texture read by a graph pass.
 *
 * @param emitter the persistent emitter
 * @param stream destination DRP2 command stream
 * @param plan the FramePlan
 * @param cfg optional frame-plan emit configuration
 * @param pass graph pass descriptor, or NULL
 * @param out_id output texture id
 * @return whether the lookup succeeded
 */
bool _graph_resolve_volume_occlusion_read(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanEmitConfig* cfg, const DvzFrameGraphPass* pass, uint64_t* out_id)
{
    ANN(out_id);
    *out_id = 0;
    uint32_t read_index = 0;
    if (!_graph_volume_occlusion_read_index(pass, &read_index))
        return true;

    const DvzFrameGraphResource* resource =
        _graph_resource_by_id(plan, pass->reads[read_index].resource_id);
    if (resource == NULL)
        return false;
    uint32_t width = 0;
    uint32_t height = 0;
    _emit_target_extent(cfg, &width, &height);
    return _graph_resolve_texture_2d(
        emitter, stream, plan, cfg, resource, width, height, DVZ_FORMAT_R32_SFLOAT, out_id);
}


/**
 * Return the index of a sampled scene-occlusion read in a graph pass.
 *
 * @param pass graph pass descriptor, or NULL
 * @param out_read_index output read index
 * @return whether a scene occlusion read was found
 */
bool _graph_scene_occlusion_read_index(const DvzFrameGraphPass* pass, uint32_t* out_read_index)
{
    ANN(out_read_index);
    if (pass == NULL)
        return false;
    for (uint32_t i = 0; i < pass->read_count; i++)
    {
        if (pass->reads[i].usage == DVZ_FRAME_GRAPH_ACCESS_SAMPLED &&
            _scene_resource_id_has_suffix(pass->reads[i].resource_id, ".scene_occlusion.depth"))
        {
            *out_read_index = i;
            return true;
        }
    }
    return false;
}


/**
 * Resolve the sampled scene-occlusion texture read by a graph pass.
 *
 * @param emitter the persistent emitter
 * @param stream destination DRP2 command stream
 * @param plan the FramePlan
 * @param cfg optional frame-plan emit configuration
 * @param pass graph pass descriptor, or NULL
 * @param out_id output texture id
 * @return whether the lookup succeeded
 */
bool _graph_resolve_scene_occlusion_read(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanEmitConfig* cfg, const DvzFrameGraphPass* pass, uint64_t* out_id)
{
    ANN(out_id);
    *out_id = 0;
    uint32_t read_index = 0;
    if (!_graph_scene_occlusion_read_index(pass, &read_index))
        return true;

    const DvzFrameGraphResource* resource =
        _graph_resource_by_id(plan, pass->reads[read_index].resource_id);
    if (resource == NULL)
        return false;
    uint32_t width = 0;
    uint32_t height = 0;
    _emit_target_extent(cfg, &width, &height);
    return _graph_resolve_texture_2d(
        emitter, stream, plan, cfg, resource, width, height, DVZ_FORMAT_R32_SFLOAT, out_id);
}


/**
 * Prepare graph-declared color targets for one render pass.
 *
 * @param emitter the persistent emitter
 * @param stream destination DRP2 command stream
 * @param plan the FramePlan
 * @param pass graph pass descriptor
 * @param cfg optional frame-plan emit configuration
 * @param out output runtime target map
 * @return whether all non-external color targets were prepared
 */
bool _graph_prepare_render_color_targets(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFrameGraphPass* pass, const DvzFramePlanEmitConfig* cfg,
    SceneGraphRuntimeTargets* out)
{
    ANN(emitter);
    ANN(stream);
    ANN(plan);
    ANN(out);
    if (pass == NULL)
        return true;

    uint32_t width = 0;
    uint32_t height = 0;
    _emit_target_extent(cfg, &width, &height);

    bool ok = true;
    for (uint32_t i = 0; ok && i < pass->color_attachment_count; i++)
    {
        const DvzFrameGraphResource* resource =
            _graph_resource_by_id(plan, pass->color_attachments[i].resource_id);
        if (resource == NULL || resource->kind == DVZ_FRAME_GRAPH_RESOURCE_EXTERNAL_TARGET)
            continue;
        uint64_t texture_id = 0;
        ok = _graph_resolve_texture_2d(
            emitter, stream, plan, cfg, resource, width, height, DVZ_FORMAT_R8G8B8A8_UNORM,
            &texture_id);
        ok = ok && _graph_runtime_targets_add(out, resource->id, texture_id);
    }
    return ok;
}



/**
 * Resolve or create one runtime 2D texture.
 *
 * @param emitter the persistent emitter.
 * @param stream destination DRP2 command stream.
 * @param key persistent resource key.
 * @param width texture width in pixels.
 * @param height texture height in pixels.
 * @param format texture format token.
 * @param usage DRP2 texture usage flags.
 * @param out_id output texture id.
 * @return whether the texture id is available.
 */
bool _runtime_resolve_texture_2d(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const char* key, uint32_t width,
    uint32_t height, uint32_t format, uint32_t usage, uint32_t sample_count, uint64_t* out_id)
{
    ANN(emitter);
    ANN(stream);
    ANN(key);
    ANN(out_id);

    bool is_new = false;
    ResourceId* resource = _resource_entry(&emitter->resources, key, &is_new);
    if (resource == NULL)
        return false;
    if (width == 0 || height == 0)
        return false;
    if (is_new || resource->texture_width == 0 || resource->texture_height == 0 ||
        resource->texture_depth == 0)
    {
        resource->texture_width = width;
        resource->texture_height = height;
        resource->texture_depth = 1;
        resource->texture_format = format;
        resource->texture_sample_count = sample_count;
        is_new = true;
    }
    else if (
        width != resource->texture_width || height != resource->texture_height ||
        resource->texture_depth != 1 || format != resource->texture_format ||
        sample_count != resource->texture_sample_count)
    {
        resource->texture_width = width;
        resource->texture_height = height;
        resource->texture_depth = 1;
        resource->texture_format = format;
        resource->texture_sample_count = sample_count;
        is_new = true;
    }

    if (is_new)
    {
        DvzDrp2TextureDesc desc = dvz_drp2_texture_desc();
        desc.id = resource->id;
        desc.width = width;
        desc.height = height;
        desc.depth = 1;
        desc.format = format;
        desc.usage = usage;
        desc.sample_count = sample_count;
        if (!dvz_drp2_stream_create_texture(stream, &desc))
            return false;
    }
    *out_id = resource->id;
    return true;
}



/**
 * Build a runtime resource key scoped to the current frame target when requested.
 *
 * @param cfg optional emission configuration.
 * @param base_key unscoped runtime resource key.
 * @param out_key output key buffer.
 * @param out_size output key buffer size.
 */
void _runtime_scope_key(
    const DvzFramePlanEmitConfig* cfg, const char* base_key, char* out_key, size_t out_size)
{
    ANN(base_key);
    ANN(out_key);

    if (out_size == 0)
        return;
    if (cfg != NULL && cfg->runtime_resource_scope_id != 0)
    {
        dvz_snprintf(
            out_key, out_size, "%s_scope_%016" PRIx64, base_key, cfg->runtime_resource_scope_id);
        return;
    }
    dvz_strlcpy(out_key, base_key, out_size);
}



/**
 * Resolve or create one graph-declared 2D texture resource.
 *
 * @param emitter the persistent emitter.
 * @param stream destination DRP2 command stream.
 * @param plan the FramePlan carrying access declarations.
 * @param cfg optional emission configuration with runtime resource scope.
 * @param resource graph resource descriptor.
 * @param width texture width in pixels.
 * @param height texture height in pixels.
 * @param fallback_format fallback texture format token when the graph format is zero.
 * @param out_id output texture id.
 * @return whether the texture id is available.
 */
bool _graph_resolve_texture_2d(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanEmitConfig* cfg, const DvzFrameGraphResource* resource, uint32_t width,
    uint32_t height, uint32_t fallback_format, uint64_t* out_id)
{
    ANN(resource);
    uint32_t format = resource->format != 0 ? resource->format : fallback_format;
    uint32_t usage = _graph_texture_usage_to_drp2(resource->usage_flags);
    if (plan != NULL)
        usage |= _graph_declared_texture_usage_to_drp2(plan, resource->id);

    char key[DVZ_SCENE_LABEL_SIZE];
    _runtime_scope_key(cfg, resource->id, key, sizeof(key));
    return _runtime_resolve_texture_2d(
        emitter, stream, key, width, height, format, usage,
        _graph_resource_lowered_sample_count(emitter, resource), out_id);
}



/**
 * Resolve the named graph depth texture for a render node.
 *
 * @param emitter the persistent emitter.
 * @param stream destination DRP2 command stream.
 * @param plan the FramePlan.
 * @param render render node.
 * @param cfg optional frame-plan emit configuration.
 * @param graph_pass output graph pass descriptor, or NULL.
 * @param out_depth_id output depth texture id, or zero when no graph depth exists.
 * @return whether graph depth resolution succeeded.
 */
bool _graph_resolve_render_depth(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanNode* render, const DvzFramePlanEmitConfig* cfg,
    const DvzFrameGraphPass** graph_pass, uint64_t* out_depth_id)
{
    ANN(emitter);
    ANN(stream);
    ANN(plan);
    ANN(render);
    ANN(graph_pass);
    ANN(out_depth_id);

    *graph_pass = _graph_pass_for_render(plan, render);
    *out_depth_id = 0;
    if (*graph_pass == NULL || !(*graph_pass)->has_depth_attachment)
        return true;

    const DvzFrameGraphResource* depth_resource =
        _graph_resource_by_id(plan, (*graph_pass)->depth_attachment.resource_id);
    if (depth_resource == NULL)
        return true;

    uint32_t width = 0;
    uint32_t height = 0;
    _emit_target_extent(cfg, &width, &height);
    return _graph_resolve_texture_2d(
        emitter, stream, plan, cfg, depth_resource, width, height, DVZ_FORMAT_D32_SFLOAT,
        out_depth_id);
}



/**
 * Prepare graph-declared G-buffer targets for one panel.
 *
 * @param emitter the persistent emitter
 * @param stream destination DRP2 command stream
 * @param plan the FramePlan
 * @param render the G-buffer render node
 * @param cfg optional frame-plan emit configuration
 * @param out output G-buffer target ids
 * @return whether all declared targets were prepared
 */
