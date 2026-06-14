/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene technique graph depth peel */
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

bool _scene_technique_emit_depth_peel_frame_graph(
    DvzFramePlan* plan, const char* panel_id, bool opaque_needs_depth,
    bool transparent_needs_depth)
{
    ANN(plan);
    ANN(panel_id);

    char opaque_depth_id[DVZ_SCENE_LABEL_SIZE];
    char front_accum_id[DVZ_SCENE_LABEL_SIZE];
    char back_accum_id[DVZ_SCENE_LABEL_SIZE];
    char depth_minmax_ping_id[DVZ_SCENE_LABEL_SIZE];
    char depth_minmax_pong_id[DVZ_SCENE_LABEL_SIZE];
    char opaque_pass_id[DVZ_SCENE_LABEL_SIZE];
    char init_pass_id[DVZ_SCENE_LABEL_SIZE];
    char composite_pass_id[DVZ_SCENE_LABEL_SIZE];
    dvz_snprintf(opaque_depth_id, sizeof(opaque_depth_id), "%s.depth.opaque", panel_id);
    dvz_snprintf(front_accum_id, sizeof(front_accum_id), "%s.peel.front_accum", panel_id);
    dvz_snprintf(back_accum_id, sizeof(back_accum_id), "%s.peel.back_accum", panel_id);
    dvz_snprintf(
        depth_minmax_ping_id, sizeof(depth_minmax_ping_id), "%s.peel.depth_minmax_ping", panel_id);
    dvz_snprintf(
        depth_minmax_pong_id, sizeof(depth_minmax_pong_id), "%s.peel.depth_minmax_pong", panel_id);
    dvz_snprintf(opaque_pass_id, sizeof(opaque_pass_id), "%s.opaque", panel_id);
    dvz_snprintf(init_pass_id, sizeof(init_pass_id), "%s.peel.init", panel_id);
    dvz_snprintf(composite_pass_id, sizeof(composite_pass_id), "%s.peel.composite", panel_id);

    DvzFrameGraphResource rt = {0};
    dvz_strlcpy(rt.id, "rt", sizeof(rt.id));
    rt.kind = DVZ_FRAME_GRAPH_RESOURCE_EXTERNAL_TARGET;
    rt.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
    rt.usage_flags =
        DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT | DVZ_FRAME_GRAPH_RESOURCE_USAGE_COPY_SRC;
    rt.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_BORROWED;
    if (!_scene_frame_graph_resource_once(plan, &rt))
        return false;

    bool depth_required = opaque_needs_depth || transparent_needs_depth;
    if (depth_required)
    {
        DvzFrameGraphResource depth = {0};
        dvz_strlcpy(depth.id, opaque_depth_id, sizeof(depth.id));
        depth.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
        depth.format = VK_FORMAT_D32_SFLOAT;
        depth.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
        depth.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_DEPTH_ATTACHMENT;
        depth.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
        if (!_scene_frame_graph_resource_once(plan, &depth))
            return false;
    }

    const char* peel_ids[4] = {
        front_accum_id, back_accum_id, depth_minmax_ping_id, depth_minmax_pong_id};
    for (uint32_t i = 0; i < 4; i++)
    {
        DvzFrameGraphResource resource = {0};
        dvz_strlcpy(resource.id, peel_ids[i], sizeof(resource.id));
        resource.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
        resource.format = i < 2 ? VK_FORMAT_R16G16B16A16_SFLOAT : VK_FORMAT_R32G32_SFLOAT;
        resource.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
        resource.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT |
                               DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED;
        resource.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
        if (!_scene_frame_graph_resource_once(plan, &resource))
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
            &depth, opaque_depth_id, DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR,
            DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE);
        if (!dvz_frame_graph_pass_depth_attachment(&opaque, &depth))
            return false;
    }
    if (!dvz_frame_plan_graph_pass(plan, &opaque))
        return false;

    DvzFrameGraphPass init = {0};
    dvz_strlcpy(init.id, init_pass_id, sizeof(init.id));
    dvz_strlcpy(init.panel_id, panel_id, sizeof(init.panel_id));
    dvz_strlcpy(init.work_label, "depth_peel_init", sizeof(init.work_label));
    init.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    for (uint32_t i = 0; i < 3; i++)
    {
        _scene_frame_graph_color_attachment(
            &color, peel_ids[i], DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR, true);
        if (i == 2)
        {
            color.clear_color[0] = -1.0f;
            color.clear_color[1] = -1.0f;
            color.clear_color[2] = 0.0f;
            color.clear_color[3] = 0.0f;
        }
        if (!dvz_frame_graph_pass_color_attachment(&init, &color))
            return false;
    }
    if (transparent_needs_depth)
    {
        _scene_frame_graph_depth_attachment(
            &depth, opaque_depth_id,
            opaque_needs_depth ? DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD
                               : DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR,
            opaque_needs_depth ? DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ
                               : DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE);
        depth.store_op = DVZ_FRAME_GRAPH_ATTACHMENT_STORE_DONT_CARE;
        if (!dvz_frame_graph_pass_depth_attachment(&init, &depth))
            return false;
    }
    if (!dvz_frame_plan_graph_pass(plan, &init))
        return false;

    for (uint32_t iter_idx = 0; iter_idx < DVZ_SCENE_DEPTH_PEEL_ITERATIONS; iter_idx++)
    {
        char iter_pass_id[DVZ_SCENE_LABEL_SIZE];
        const char* prev_depth = (iter_idx % 2 == 0) ? depth_minmax_ping_id : depth_minmax_pong_id;
        const char* next_depth = (iter_idx % 2 == 0) ? depth_minmax_pong_id : depth_minmax_ping_id;
        dvz_snprintf(
            iter_pass_id, sizeof(iter_pass_id), "%s.peel.iter.%" PRIu32, panel_id, iter_idx);

        DvzFrameGraphPass iter = {0};
        dvz_strlcpy(iter.id, iter_pass_id, sizeof(iter.id));
        dvz_strlcpy(iter.panel_id, panel_id, sizeof(iter.panel_id));
        dvz_strlcpy(iter.work_label, "depth_peel_iter", sizeof(iter.work_label));
        iter.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
        if (!dvz_frame_graph_pass_read(&iter, prev_depth, DVZ_FRAME_GRAPH_ACCESS_SAMPLED))
            return false;
        _scene_frame_graph_color_attachment(
            &color, front_accum_id, DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD, true);
        if (!dvz_frame_graph_pass_color_attachment(&iter, &color))
            return false;
        _scene_frame_graph_color_attachment(
            &color, back_accum_id, DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD, true);
        if (!dvz_frame_graph_pass_color_attachment(&iter, &color))
            return false;
        _scene_frame_graph_color_attachment(
            &color, next_depth, DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR, true);
        color.clear_color[0] = -1.0f;
        color.clear_color[1] = -1.0f;
        color.clear_color[2] = 0.0f;
        color.clear_color[3] = 0.0f;
        if (!dvz_frame_graph_pass_color_attachment(&iter, &color))
            return false;
        if (transparent_needs_depth)
        {
            _scene_frame_graph_depth_attachment(
                &depth, opaque_depth_id,
                opaque_needs_depth ? DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD
                                   : DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR,
                opaque_needs_depth ? DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ
                                   : DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE);
            depth.store_op = DVZ_FRAME_GRAPH_ATTACHMENT_STORE_DONT_CARE;
            if (!dvz_frame_graph_pass_depth_attachment(&iter, &depth))
                return false;
        }
        if (!dvz_frame_plan_graph_pass(plan, &iter))
            return false;
    }

    DvzFrameGraphPass composite = {0};
    dvz_strlcpy(composite.id, composite_pass_id, sizeof(composite.id));
    dvz_strlcpy(composite.panel_id, panel_id, sizeof(composite.panel_id));
    dvz_strlcpy(composite.work_label, "depth_peel_composite", sizeof(composite.work_label));
    composite.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    if (!dvz_frame_graph_pass_read(&composite, front_accum_id, DVZ_FRAME_GRAPH_ACCESS_SAMPLED) ||
        !dvz_frame_graph_pass_read(&composite, back_accum_id, DVZ_FRAME_GRAPH_ACCESS_SAMPLED))
        return false;
    _scene_frame_graph_color_attachment(&color, "rt", DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD, false);
    if (!dvz_frame_graph_pass_color_attachment(&composite, &color))
        return false;
    return dvz_frame_plan_graph_pass(plan, &composite);
}



/**
 * Emit graph descriptors for one ordinary opaque panel render pass.
 *
 * @param plan the frame plan
 * @param panel_id the panel id
 * @param needs_depth whether the opaque pass writes depth
 * @return whether graph descriptors were emitted
 */
