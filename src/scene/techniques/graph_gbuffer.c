/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene technique graph gbuffer */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <float.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <vulkan/vulkan_core.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_scene.h"
#include "_scene_resource_key.h"
#include "_technique.h"
#include "_technique_internal.h"
#include "_visual_pipeline.h"
#include "datoviz/scene.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

bool _scene_technique_emit_opaque_frame_graph(
    DvzFramePlan* plan, const char* panel_id, bool needs_depth,
    const DvzSceneMsaaTechniqueState* msaa)
{
    ANN(plan);
    ANN(panel_id);

    uint32_t sample_count =
        msaa != NULL && msaa->enabled && msaa->sample_count > 1 ? msaa->sample_count : 1;
    bool multisample = sample_count > 1;
    char depth_id[DVZ_SCENE_LABEL_SIZE];
    char msaa_color_id[DVZ_SCENE_LABEL_SIZE];
    char opaque_pass_id[DVZ_SCENE_LABEL_SIZE];
    dvz_snprintf(depth_id, sizeof(depth_id), "%s.depth", panel_id);
    dvz_snprintf(msaa_color_id, sizeof(msaa_color_id), "%s.msaa.color", panel_id);
    dvz_snprintf(opaque_pass_id, sizeof(opaque_pass_id), "%s.opaque", panel_id);

    DvzFrameGraphResource rt = {0};
    dvz_strlcpy(rt.id, "rt", sizeof(rt.id));
    rt.kind = DVZ_FRAME_GRAPH_RESOURCE_EXTERNAL_TARGET;
    rt.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
    rt.usage_flags =
        DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT | DVZ_FRAME_GRAPH_RESOURCE_USAGE_COPY_SRC;
    rt.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_BORROWED;
    if (!_scene_frame_graph_resource_once(plan, &rt))
        return false;

    if (multisample)
    {
        DvzFrameGraphResource msaa_color = {0};
        dvz_strlcpy(msaa_color.id, msaa_color_id, sizeof(msaa_color.id));
        msaa_color.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
        msaa_color.format = DVZ_FORMAT_R8G8B8A8_UNORM;
        msaa_color.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
        msaa_color.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT;
        msaa_color.sample_count = sample_count;
        msaa_color.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
        if (!_scene_frame_graph_resource_once(plan, &msaa_color))
            return false;
    }

    if (needs_depth)
    {
        DvzFrameGraphResource depth = {0};
        dvz_strlcpy(depth.id, depth_id, sizeof(depth.id));
        depth.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
        depth.format = DVZ_FORMAT_D32_SFLOAT;
        depth.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
        depth.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_DEPTH_ATTACHMENT;
        depth.sample_count = sample_count;
        depth.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
        if (!_scene_frame_graph_resource_once(plan, &depth))
            return false;
    }

    DvzFrameGraphAttachment color = {0};
    DvzFrameGraphAttachment depth = {0};
    DvzFrameGraphPass opaque = {0};
    dvz_strlcpy(opaque.id, opaque_pass_id, sizeof(opaque.id));
    dvz_strlcpy(opaque.panel_id, panel_id, sizeof(opaque.panel_id));
    dvz_strlcpy(opaque.work_label, "opaque", sizeof(opaque.work_label));
    opaque.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    opaque.alpha_to_coverage = multisample && msaa != NULL && msaa->alpha_to_coverage;
    _scene_frame_graph_color_attachment(
        &color, multisample ? msaa_color_id : "rt", DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR, true);
    if (multisample)
    {
        dvz_strlcpy(color.resolve_resource_id, "rt", sizeof(color.resolve_resource_id));
        color.resolve_mode = VK_RESOLVE_MODE_AVERAGE_BIT;
    }
    if (!dvz_frame_graph_pass_color_attachment(&opaque, &color))
        return false;
    if (needs_depth)
    {
        _scene_frame_graph_depth_attachment(
            &depth, depth_id, DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR,
            DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE);
        if (!dvz_frame_graph_pass_depth_attachment(&opaque, &depth))
            return false;
    }
    return dvz_frame_plan_graph_pass(plan, &opaque);
}



/**
 * Emit graph descriptors for one depth-based post-processing panel plan.
 *
 * @param plan the frame plan
 * @param panel_id the panel id
 * @param desc the depth post-process graph descriptor
 * @return whether graph descriptors were emitted
 */
bool _scene_technique_emit_depth_postprocess_frame_graph(
    DvzFramePlan* plan, const char* panel_id, const DvzSceneDepthPostProcessGraphDesc* desc)
{
    ANN(plan);
    ANN(panel_id);
    ANN(desc);
    ANN(desc->color_suffix);
    ANN(desc->depth_suffix);
    ANN(desc->resolve_suffix);
    ANN(desc->resolve_work_label);

    char color_id[DVZ_SCENE_LABEL_SIZE];
    char depth_id[DVZ_SCENE_LABEL_SIZE];
    char opaque_pass_id[DVZ_SCENE_LABEL_SIZE];
    char resolve_pass_id[DVZ_SCENE_LABEL_SIZE];
    dvz_snprintf(color_id, sizeof(color_id), "%s.%s", panel_id, desc->color_suffix);
    dvz_snprintf(depth_id, sizeof(depth_id), "%s.%s", panel_id, desc->depth_suffix);
    dvz_snprintf(opaque_pass_id, sizeof(opaque_pass_id), "%s.opaque", panel_id);
    dvz_snprintf(
        resolve_pass_id, sizeof(resolve_pass_id), "%s.%s", panel_id, desc->resolve_suffix);

    DvzFrameGraphResource rt = {0};
    dvz_strlcpy(rt.id, "rt", sizeof(rt.id));
    rt.kind = DVZ_FRAME_GRAPH_RESOURCE_EXTERNAL_TARGET;
    rt.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
    rt.usage_flags =
        DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT | DVZ_FRAME_GRAPH_RESOURCE_USAGE_COPY_SRC;
    rt.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_BORROWED;
    if (!_scene_frame_graph_resource_once(plan, &rt))
        return false;

    DvzFrameGraphResource color = {0};
    dvz_strlcpy(color.id, color_id, sizeof(color.id));
    color.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
    color.format = desc->color_format;
    color.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
    color.usage_flags =
        DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT | DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED;
    color.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
    if (!_scene_frame_graph_resource_once(plan, &color))
        return false;

    DvzFrameGraphResource depth = {0};
    dvz_strlcpy(depth.id, depth_id, sizeof(depth.id));
    depth.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
    depth.format = DVZ_FORMAT_D32_SFLOAT;
    depth.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
    depth.usage_flags =
        DVZ_FRAME_GRAPH_RESOURCE_USAGE_DEPTH_ATTACHMENT | DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED;
    depth.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
    if (!_scene_frame_graph_resource_once(plan, &depth))
        return false;

    DvzFrameGraphAttachment attachment = {0};
    DvzFrameGraphAttachment depth_attachment = {0};
    DvzFrameGraphPass opaque = {0};
    dvz_strlcpy(opaque.id, opaque_pass_id, sizeof(opaque.id));
    dvz_strlcpy(opaque.panel_id, panel_id, sizeof(opaque.panel_id));
    dvz_strlcpy(opaque.work_label, "opaque", sizeof(opaque.work_label));
    opaque.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    _scene_frame_graph_color_attachment(
        &attachment, color_id, DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR, true);
    if (!dvz_frame_graph_pass_color_attachment(&opaque, &attachment))
        return false;
    _scene_frame_graph_depth_attachment(
        &depth_attachment, depth_id, DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR,
        DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE);
    if (!dvz_frame_graph_pass_depth_attachment(&opaque, &depth_attachment))
        return false;
    if (!dvz_frame_plan_graph_pass(plan, &opaque))
        return false;

    DvzFrameGraphPass resolve = {0};
    dvz_strlcpy(resolve.id, resolve_pass_id, sizeof(resolve.id));
    dvz_strlcpy(resolve.panel_id, panel_id, sizeof(resolve.panel_id));
    dvz_strlcpy(resolve.work_label, desc->resolve_work_label, sizeof(resolve.work_label));
    resolve.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    if (!dvz_frame_graph_pass_read(&resolve, color_id, DVZ_FRAME_GRAPH_ACCESS_SAMPLED) ||
        !dvz_frame_graph_pass_read(&resolve, depth_id, DVZ_FRAME_GRAPH_ACCESS_SAMPLED))
        return false;
    _scene_frame_graph_color_attachment(
        &attachment, "rt", desc->resolve_load_op, desc->resolve_clear);
    if (!dvz_frame_graph_pass_color_attachment(&resolve, &attachment))
        return false;
    return dvz_frame_plan_graph_pass(plan, &resolve);
}



/**
 * Emit graph descriptors for one EDL post-processing panel plan.
 *
 * @param plan the frame plan
 * @param panel_id the panel id
 * @return whether graph descriptors were emitted
 */
bool _scene_technique_emit_edl_frame_graph(DvzFramePlan* plan, const char* panel_id)
{
    const DvzSceneDepthPostProcessGraphDesc desc = {
        .color_suffix = "edl.color",
        .depth_suffix = "edl.depth",
        .resolve_suffix = "edl.resolve",
        .resolve_work_label = "edl_resolve",
        .color_format = DVZ_FORMAT_R8G8B8A8_UNORM,
        .resolve_load_op = DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR,
        .resolve_clear = true,
    };
    return _scene_technique_emit_depth_postprocess_frame_graph(plan, panel_id, &desc);
}



/**
 * Reset an internal G-buffer plan.
 *
 * @param plan the G-buffer plan
 */
void _scene_technique_gbuffer_plan_init(DvzSceneGBufferPlan* plan)
{
    ANN(plan);
    dvz_memset(plan, sizeof(DvzSceneGBufferPlan), 0, sizeof(DvzSceneGBufferPlan));
}



/**
 * Add one retained visual to an internal G-buffer plan when it is eligible.
 *
 * @param plan the G-buffer plan
 * @param visual the retained visual
 * @param attach the panel attachment
 * @return whether the visual was accepted as a G-buffer producer
 */
bool _scene_technique_gbuffer_plan_add_visual(
    DvzSceneGBufferPlan* plan, const DvzVisual* visual, const DvzPanelAttach* attach)
{
    ANN(plan);
    ANN(visual);
    ANN(attach);

    DvzSceneVisualPassCaps caps = {0};
    if (!_scene_visual_pass_caps_from_visual(visual, attach, &caps))
        return false;
    if (!_scene_caps_support_gbuffer(&caps))
        return false;

    plan->enabled = true;
    plan->needs_depth = true;
    plan->needs_normal = true;
    plan->producer_count++;
    return true;
}



/**
 * Emit graph descriptors for one panel G-buffer plan.
 *
 * @param plan the frame plan
 * @param panel_id the panel id
 * @param gbuffer the G-buffer plan
 * @return whether graph descriptors were emitted
 */
bool _scene_technique_emit_gbuffer_frame_graph(
    DvzFramePlan* plan, const char* panel_id, const DvzSceneGBufferPlan* gbuffer)
{
    ANN(plan);
    ANN(panel_id);
    ANN(gbuffer);

    if (!gbuffer->enabled || gbuffer->producer_count == 0)
        return true;
    if (!gbuffer->needs_depth && !gbuffer->needs_normal && !gbuffer->needs_object_id)
        return false;

    char depth_id[DVZ_SCENE_LABEL_SIZE];
    char normal_id[DVZ_SCENE_LABEL_SIZE];
    char object_id[DVZ_SCENE_LABEL_SIZE];
    char pass_id[DVZ_SCENE_LABEL_SIZE];
    dvz_snprintf(depth_id, sizeof(depth_id), "%s.gbuffer.depth", panel_id);
    dvz_snprintf(normal_id, sizeof(normal_id), "%s.gbuffer.normal", panel_id);
    dvz_snprintf(object_id, sizeof(object_id), "%s.gbuffer.object_id", panel_id);
    dvz_snprintf(pass_id, sizeof(pass_id), "%s.gbuffer", panel_id);

    if (gbuffer->needs_depth)
    {
        DvzFrameGraphResource depth = {0};
        dvz_strlcpy(depth.id, depth_id, sizeof(depth.id));
        depth.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
        depth.format = DVZ_FORMAT_D32_SFLOAT;
        depth.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
        depth.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_DEPTH_ATTACHMENT |
                            DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED;
        depth.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
        if (!_scene_frame_graph_resource_once(plan, &depth))
            return false;
    }

    if (gbuffer->needs_normal)
    {
        DvzFrameGraphResource normal = {0};
        dvz_strlcpy(normal.id, normal_id, sizeof(normal.id));
        normal.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
        normal.format = DVZ_FORMAT_R16G16B16A16_SFLOAT;
        normal.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
        normal.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT |
                             DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED;
        normal.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
        if (!_scene_frame_graph_resource_once(plan, &normal))
            return false;
    }

    if (gbuffer->needs_object_id)
    {
        DvzFrameGraphResource object = {0};
        dvz_strlcpy(object.id, object_id, sizeof(object.id));
        object.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
        object.format = DVZ_FORMAT_R32_UINT;
        object.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
        object.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT |
                             DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED;
        object.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
        if (!_scene_frame_graph_resource_once(plan, &object))
            return false;
    }

    DvzFrameGraphPass pass = {0};
    DvzFrameGraphAttachment attachment = {0};
    dvz_strlcpy(pass.id, pass_id, sizeof(pass.id));
    dvz_strlcpy(pass.panel_id, panel_id, sizeof(pass.panel_id));
    dvz_strlcpy(pass.work_label, "gbuffer", sizeof(pass.work_label));
    pass.kind = DVZ_FRAME_GRAPH_PASS_RENDER;

    if (gbuffer->needs_normal)
    {
        _scene_frame_graph_color_attachment(
            &attachment, normal_id, DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR, true);
        attachment.clear_color[0] = 0.5f;
        attachment.clear_color[1] = 0.5f;
        attachment.clear_color[2] = 1.0f;
        attachment.clear_color[3] = 0.0f;
        if (!dvz_frame_graph_pass_color_attachment(&pass, &attachment))
            return false;
    }

    if (gbuffer->needs_object_id)
    {
        _scene_frame_graph_color_attachment(
            &attachment, object_id, DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR, true);
        attachment.clear_color[0] = 0.0f;
        attachment.clear_color[1] = 0.0f;
        attachment.clear_color[2] = 0.0f;
        attachment.clear_color[3] = 0.0f;
        if (!dvz_frame_graph_pass_color_attachment(&pass, &attachment))
            return false;
    }

    if (gbuffer->needs_depth)
    {
        _scene_frame_graph_depth_attachment(
            &attachment, depth_id, DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR,
            DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE);
        if (!dvz_frame_graph_pass_depth_attachment(&pass, &attachment))
            return false;
    }

    return dvz_frame_plan_graph_pass(plan, &pass);
}



/**
 * Emit graph descriptors for one SSAO graph-only panel plan.
 *
 * @param plan the frame plan
 * @param panel_id the panel id
 * @param gbuffer the G-buffer plan providing normal and depth inputs
 * @param ssao_state the effective SSAO state
 * @return whether graph descriptors were emitted
 */
