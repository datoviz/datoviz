/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene technique graph wboit */
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

/**
 * Emit graph descriptors for one WBOIT panel plan.
 *
 * @param plan the frame plan
 * @param panel_id the panel id
 * @param opaque_needs_depth whether the opaque pass writes depth
 * @param transparent_needs_depth whether the accumulation pass reads depth
 * @return whether graph descriptors were emitted
 */
bool _scene_technique_emit_wboit_frame_graph(
    DvzFramePlan* plan, const char* panel_id, bool opaque_needs_depth,
    bool transparent_needs_depth)
{
    ANN(plan);
    ANN(panel_id);

    char accum_id[DVZ_SCENE_LABEL_SIZE];
    char weight_id[DVZ_SCENE_LABEL_SIZE];
    char depth_id[DVZ_SCENE_LABEL_SIZE];
    char opaque_pass_id[DVZ_SCENE_LABEL_SIZE];
    char accum_pass_id[DVZ_SCENE_LABEL_SIZE];
    char resolve_pass_id[DVZ_SCENE_LABEL_SIZE];
    dvz_snprintf(accum_id, sizeof(accum_id), "%s.wboit.accum", panel_id);
    dvz_snprintf(weight_id, sizeof(weight_id), "%s.wboit.weight", panel_id);
    dvz_snprintf(depth_id, sizeof(depth_id), "%s.depth", panel_id);
    dvz_snprintf(opaque_pass_id, sizeof(opaque_pass_id), "%s.opaque", panel_id);
    dvz_snprintf(accum_pass_id, sizeof(accum_pass_id), "%s.wboit.accum", panel_id);
    dvz_snprintf(resolve_pass_id, sizeof(resolve_pass_id), "%s.wboit.resolve", panel_id);

    DvzFrameGraphResource rt = {0};
    dvz_strlcpy(rt.id, "rt", sizeof(rt.id));
    rt.kind = DVZ_FRAME_GRAPH_RESOURCE_EXTERNAL_TARGET;
    rt.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
    rt.usage_flags =
        DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT | DVZ_FRAME_GRAPH_RESOURCE_USAGE_COPY_SRC;
    rt.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_BORROWED;
    if (!_scene_frame_graph_resource_once(plan, &rt))
        return false;

    DvzFrameGraphResource accum = {0};
    dvz_strlcpy(accum.id, accum_id, sizeof(accum.id));
    accum.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
    accum.format = DVZ_FORMAT_R16G16B16A16_SFLOAT;
    accum.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
    accum.usage_flags =
        DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT | DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED;
    accum.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
    if (!_scene_frame_graph_resource_once(plan, &accum))
        return false;

    DvzFrameGraphResource weight = {0};
    dvz_strlcpy(weight.id, weight_id, sizeof(weight.id));
    weight.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
    weight.format = DVZ_FORMAT_R16_SFLOAT;
    weight.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
    weight.usage_flags =
        DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT | DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED;
    weight.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
    if (!_scene_frame_graph_resource_once(plan, &weight))
        return false;

    bool depth_required = opaque_needs_depth || transparent_needs_depth;
    if (depth_required)
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

    DvzFrameGraphAttachment color = {0};
    DvzFrameGraphAttachment depth = {0};
    DvzFrameGraphPass opaque = {0};
    dvz_strlcpy(opaque.id, opaque_pass_id, sizeof(opaque.id));
    dvz_strlcpy(opaque.panel_id, panel_id, sizeof(opaque.panel_id));
    dvz_strlcpy(opaque.work_label, "opaque", sizeof(opaque.work_label));
    opaque.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    bool color_written = _scene_frame_graph_color_written(plan, "rt");
    _scene_frame_graph_color_attachment(
        &color, "rt",
        color_written ? DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD
                      : DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR,
        !color_written);
    if (!dvz_frame_graph_pass_color_attachment(&opaque, &color))
        return false;
    if (opaque_needs_depth)
    {
        _scene_frame_graph_depth_attachment(
            &depth, depth_id, DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR,
            DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE);
        if (!dvz_frame_graph_pass_depth_attachment(&opaque, &depth))
            return false;
    }
    if (!dvz_frame_plan_graph_pass(plan, &opaque))
        return false;

    DvzFrameGraphPass accum_pass = {0};
    dvz_strlcpy(accum_pass.id, accum_pass_id, sizeof(accum_pass.id));
    dvz_strlcpy(accum_pass.panel_id, panel_id, sizeof(accum_pass.panel_id));
    dvz_strlcpy(accum_pass.work_label, "wboit_accum", sizeof(accum_pass.work_label));
    accum_pass.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    _scene_frame_graph_color_attachment(
        &color, accum_id, DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR, true);
    if (!dvz_frame_graph_pass_color_attachment(&accum_pass, &color))
        return false;
    _scene_frame_graph_color_attachment(
        &color, weight_id, DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR, true);
    if (!dvz_frame_graph_pass_color_attachment(&accum_pass, &color))
        return false;
    if (transparent_needs_depth)
    {
        _scene_frame_graph_depth_attachment(
            &depth, depth_id,
            opaque_needs_depth ? DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD
                               : DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR,
            opaque_needs_depth ? DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ
                               : DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE);
        if (!dvz_frame_graph_pass_depth_attachment(&accum_pass, &depth))
            return false;
    }
    if (!dvz_frame_plan_graph_pass(plan, &accum_pass))
        return false;

    DvzFrameGraphPass resolve = {0};
    dvz_strlcpy(resolve.id, resolve_pass_id, sizeof(resolve.id));
    dvz_strlcpy(resolve.panel_id, panel_id, sizeof(resolve.panel_id));
    dvz_strlcpy(resolve.work_label, "wboit_resolve", sizeof(resolve.work_label));
    resolve.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    if (!dvz_frame_graph_pass_read(&resolve, accum_id, DVZ_FRAME_GRAPH_ACCESS_SAMPLED) ||
        !dvz_frame_graph_pass_read(&resolve, weight_id, DVZ_FRAME_GRAPH_ACCESS_SAMPLED))
        return false;
    _scene_frame_graph_color_attachment(&color, "rt", DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD, false);
    if (!dvz_frame_graph_pass_color_attachment(&resolve, &color))
        return false;
    return dvz_frame_plan_graph_pass(plan, &resolve);
}



/**
 * Emit graph descriptors for one ordinary blended transparent panel plan.
 *
 * @param plan the frame plan
 * @param panel_id the panel id
 * @param emit_opaque_pass whether to emit the graph opaque pass
 * @param opaque_needs_depth whether the opaque pass writes depth
 * @param prior_depth_written whether previous graph passes produced panel depth
 * @param pass_count number of source-over passes
 * @param pass_needs_depth source-over pass depth attachment flags
 * @param pass_writes_depth source-over pass depth-write flags
 * @param msaa optional MSAA state for the opaque prepass
 * @return whether graph descriptors were emitted
 */
bool _scene_technique_emit_blended_frame_graph(
    DvzFramePlan* plan, const char* panel_id, bool emit_opaque_pass, bool opaque_needs_depth,
    bool prior_depth_written, uint32_t pass_count, const bool* pass_needs_depth,
    const bool* pass_writes_depth, const DvzSceneMsaaTechniqueState* msaa)
{
    ANN(plan);
    ANN(panel_id);
    if (pass_count > 0)
    {
        ANN(pass_needs_depth);
        ANN(pass_writes_depth);
    }

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

    bool depth_required = opaque_needs_depth;
    for (uint32_t i = 0; i < pass_count; i++)
        depth_required = depth_required || pass_needs_depth[i];
    bool blended_needs_depth = false;
    for (uint32_t i = 0; i < pass_count; i++)
        blended_needs_depth = blended_needs_depth || pass_needs_depth[i];

    uint32_t sample_count =
        msaa != NULL && msaa->enabled && msaa->sample_count > 1 ? msaa->sample_count : 1;
    /* Later blended passes remain single-sample; do not create an MSAA depth attachment they would
     * need to attach or sample before a depth-resolve path exists. */
    bool multisample_opaque = emit_opaque_pass && sample_count > 1 && !blended_needs_depth;
    if (multisample_opaque)
    {
        DvzFrameGraphResource msaa_color = {0};
        dvz_strlcpy(msaa_color.id, msaa_color_id, sizeof(msaa_color.id));
        msaa_color.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
        msaa_color.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
        msaa_color.usage_flags =
            DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT | DVZ_FRAME_GRAPH_RESOURCE_USAGE_COPY_SRC;
        msaa_color.sample_count = sample_count;
        msaa_color.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
        if (!_scene_frame_graph_resource_once(plan, &msaa_color))
            return false;
    }

    if (depth_required)
    {
        DvzFrameGraphResource depth = {0};
        dvz_strlcpy(depth.id, depth_id, sizeof(depth.id));
        depth.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
        depth.format = DVZ_FORMAT_D32_SFLOAT;
        depth.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
        depth.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_DEPTH_ATTACHMENT |
                            DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED;
        depth.sample_count = multisample_opaque ? sample_count : 1;
        depth.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
        if (!_scene_frame_graph_resource_once(plan, &depth))
            return false;
    }

    DvzFrameGraphAttachment color = {0};
    DvzFrameGraphAttachment depth = {0};
    bool depth_written = prior_depth_written;
    if (emit_opaque_pass)
    {
        DvzFrameGraphPass opaque = {0};
        dvz_strlcpy(opaque.id, opaque_pass_id, sizeof(opaque.id));
        dvz_strlcpy(opaque.panel_id, panel_id, sizeof(opaque.panel_id));
        dvz_strlcpy(opaque.work_label, "opaque", sizeof(opaque.work_label));
        opaque.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
        opaque.alpha_to_coverage = multisample_opaque && msaa != NULL && msaa->alpha_to_coverage;
        bool color_written = _scene_frame_graph_color_written(plan, "rt");
        _scene_frame_graph_color_attachment(
            &color, multisample_opaque ? msaa_color_id : "rt",
            !multisample_opaque && color_written ? DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD
                                                 : DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR,
            multisample_opaque || !color_written);
        if (multisample_opaque)
        {
            dvz_strlcpy(color.resolve_resource_id, "rt", sizeof(color.resolve_resource_id));
            color.resolve_mode = VK_RESOLVE_MODE_AVERAGE_BIT;
        }
        if (!dvz_frame_graph_pass_color_attachment(&opaque, &color))
            return false;
        if (opaque_needs_depth)
        {
            _scene_frame_graph_depth_attachment(
                &depth, depth_id, DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR,
                DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE);
            if (!dvz_frame_graph_pass_depth_attachment(&opaque, &depth))
                return false;
        }
        if (!dvz_frame_plan_graph_pass(plan, &opaque))
            return false;
        depth_written = depth_written || opaque_needs_depth;
    }

    for (uint32_t i = 0; i < pass_count; i++)
    {
        char blend_pass_id[DVZ_SCENE_LABEL_SIZE];
        if (i == 0)
            dvz_snprintf(blend_pass_id, sizeof(blend_pass_id), "%s.transparent_blend", panel_id);
        else
            dvz_snprintf(
                blend_pass_id, sizeof(blend_pass_id), "%s.transparent_blend_%u", panel_id, i);

        DvzFrameGraphPass blend = {0};
        dvz_strlcpy(blend.id, blend_pass_id, sizeof(blend.id));
        dvz_strlcpy(blend.panel_id, panel_id, sizeof(blend.panel_id));
        dvz_strlcpy(blend.work_label, "transparent_blend", sizeof(blend.work_label));
        blend.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
        _scene_frame_graph_color_attachment(
            &color, "rt", DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD, false);
        if (!dvz_frame_graph_pass_color_attachment(&blend, &color))
            return false;
        if (pass_needs_depth[i])
        {
            _scene_frame_graph_depth_attachment(
                &depth, depth_id,
                depth_written ? DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD
                              : DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR,
                pass_writes_depth[i]
                    ? (depth_written ? DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ_WRITE
                                     : DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE)
                    : (depth_written ? DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ
                                     : DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE));
            if (!pass_writes_depth[i])
                depth.store_op = DVZ_FRAME_GRAPH_ATTACHMENT_STORE_DONT_CARE;
            if (!dvz_frame_graph_pass_depth_attachment(&blend, &depth))
                return false;
            depth_written = depth_written || pass_writes_depth[i];
        }
        if (!dvz_frame_plan_graph_pass(plan, &blend))
            return false;
    }
    return true;
}


/**
 * Emit graph descriptors for one panel volume occlusion prepass.
 *
 * @param plan the frame plan
 * @param panel_id the panel id
 * @return whether graph descriptors were emitted
 */
bool _scene_technique_emit_volume_occlusion_frame_graph(DvzFramePlan* plan, const char* panel_id)
{
    ANN(plan);
    ANN(panel_id);

    char depth_id[DVZ_SCENE_LABEL_SIZE];
    char pass_id[DVZ_SCENE_LABEL_SIZE];
    if (!_scene_resource_key_panel_graph(
            panel_id, "volume_occlusion.depth", depth_id, sizeof(depth_id)))
        return false;
    if (!_scene_resource_key_panel_graph(panel_id, "volume_occlusion", pass_id, sizeof(pass_id)))
        return false;

    DvzFrameGraphResource depth = {0};
    dvz_strlcpy(depth.id, depth_id, sizeof(depth.id));
    depth.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
    depth.format = DVZ_FORMAT_R32_SFLOAT;
    depth.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
    depth.usage_flags =
        DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT | DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED;
    depth.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
    if (!_scene_frame_graph_resource_once(plan, &depth))
        return false;

    DvzFrameGraphAttachment color = {0};
    DvzFrameGraphPass pass = {0};
    dvz_strlcpy(pass.id, pass_id, sizeof(pass.id));
    dvz_strlcpy(pass.panel_id, panel_id, sizeof(pass.panel_id));
    dvz_strlcpy(pass.work_label, "volume_occlusion", sizeof(pass.work_label));
    pass.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    _scene_frame_graph_color_attachment(
        &color, depth_id, DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR, true);
    color.clear_color[0] = 0.0f;
    color.clear_color[1] = 0.0f;
    color.clear_color[2] = 0.0f;
    color.clear_color[3] = 0.0f;
    if (!dvz_frame_graph_pass_color_attachment(&pass, &color))
        return false;
    return dvz_frame_plan_graph_pass(plan, &pass);
}


/**
 * Emit graph descriptors for one panel generic scene occlusion prepass.
 *
 * @param plan the frame plan
 * @param panel_id the panel id
 * @return whether graph descriptors were emitted
 */
bool _scene_technique_emit_scene_occlusion_frame_graph(DvzFramePlan* plan, const char* panel_id)
{
    ANN(plan);
    ANN(panel_id);

    char depth_id[DVZ_SCENE_LABEL_SIZE];
    char z_id[DVZ_SCENE_LABEL_SIZE];
    char pass_id[DVZ_SCENE_LABEL_SIZE];
    if (!_scene_resource_key_panel_graph(
            panel_id, "scene_occlusion.depth", depth_id, sizeof(depth_id)))
        return false;
    if (!_scene_resource_key_panel_graph(panel_id, "scene_occlusion.z", z_id, sizeof(z_id)))
        return false;
    if (!_scene_resource_key_panel_graph(panel_id, "scene_occlusion", pass_id, sizeof(pass_id)))
        return false;

    DvzFrameGraphResource depth = {0};
    dvz_strlcpy(depth.id, depth_id, sizeof(depth.id));
    depth.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
    depth.format = DVZ_FORMAT_R32_SFLOAT;
    depth.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
    depth.usage_flags =
        DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT | DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED;
    depth.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
    if (!_scene_frame_graph_resource_once(plan, &depth))
        return false;

    DvzFrameGraphResource z = {0};
    dvz_strlcpy(z.id, z_id, sizeof(z.id));
    z.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
    z.format = DVZ_FORMAT_D32_SFLOAT;
    z.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
    z.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_DEPTH_ATTACHMENT;
    z.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
    if (!_scene_frame_graph_resource_once(plan, &z))
        return false;

    DvzFrameGraphAttachment color = {0};
    DvzFrameGraphAttachment z_attachment = {0};
    DvzFrameGraphPass pass = {0};
    dvz_strlcpy(pass.id, pass_id, sizeof(pass.id));
    dvz_strlcpy(pass.panel_id, panel_id, sizeof(pass.panel_id));
    dvz_strlcpy(pass.work_label, "scene_occlusion", sizeof(pass.work_label));
    pass.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    _scene_frame_graph_color_attachment(
        &color, depth_id, DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR, true);
    color.clear_color[0] = 1.0f;
    color.clear_color[1] = 1.0f;
    color.clear_color[2] = 1.0f;
    color.clear_color[3] = 1.0f;
    if (!dvz_frame_graph_pass_color_attachment(&pass, &color))
        return false;
    _scene_frame_graph_depth_attachment(
        &z_attachment, z_id, DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR,
        DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE);
    if (!dvz_frame_graph_pass_depth_attachment(&pass, &z_attachment))
        return false;
    return dvz_frame_plan_graph_pass(plan, &pass);
}



/**
 * Emit graph descriptors for one depth-peeling panel plan.
 *
 * @param plan the frame plan
 * @param panel_id the panel id
 * @param opaque_needs_depth whether the opaque pass writes depth
 * @param transparent_needs_depth whether peeling passes read depth
 * @return whether graph descriptors were emitted
 */
