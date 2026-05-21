/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene technique graph ssao */
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

bool _scene_technique_emit_ssao_frame_graph(
    DvzFramePlan* plan, const char* panel_id, const DvzSceneGBufferPlan* gbuffer,
    const DvzSceneSsaoTechniqueState* ssao_state)
{
    ANN(plan);
    ANN(panel_id);
    ANN(gbuffer);

    if (!gbuffer->enabled || gbuffer->producer_count == 0)
        return true;
    if (!gbuffer->needs_depth || !gbuffer->needs_normal)
        return false;

    char depth_id[DVZ_SCENE_LABEL_SIZE];
    char normal_id[DVZ_SCENE_LABEL_SIZE];
    char occlusion_id[DVZ_SCENE_LABEL_SIZE];
    char blur_id[DVZ_SCENE_LABEL_SIZE];
    char pass_id[DVZ_SCENE_LABEL_SIZE];
    char blur_pass_id[DVZ_SCENE_LABEL_SIZE];
    char composite_id[DVZ_SCENE_LABEL_SIZE];
    dvz_snprintf(depth_id, sizeof(depth_id), "%s.gbuffer.depth", panel_id);
    dvz_snprintf(normal_id, sizeof(normal_id), "%s.gbuffer.normal", panel_id);
    dvz_snprintf(occlusion_id, sizeof(occlusion_id), "%s.ssao.occlusion", panel_id);
    dvz_snprintf(blur_id, sizeof(blur_id), "%s.ssao.blur", panel_id);
    dvz_snprintf(pass_id, sizeof(pass_id), "%s.ssao", panel_id);
    dvz_snprintf(blur_pass_id, sizeof(blur_pass_id), "%s.ssao.blur", panel_id);
    dvz_snprintf(composite_id, sizeof(composite_id), "%s.ssao.composite", panel_id);
    bool blur_enabled = ssao_state != NULL && ssao_state->blur_enabled;

    DvzFrameGraphResource occlusion = {0};
    dvz_strlcpy(occlusion.id, occlusion_id, sizeof(occlusion.id));
    occlusion.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
    occlusion.format = VK_FORMAT_R8_UNORM;
    occlusion.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
    occlusion.usage_flags =
        DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT | DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED;
    occlusion.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
    if (!_scene_frame_graph_resource_once(plan, &occlusion))
        return false;

    DvzFrameGraphAttachment attachment = {0};
    DvzFrameGraphPass ssao = {0};
    dvz_strlcpy(ssao.id, pass_id, sizeof(ssao.id));
    dvz_strlcpy(ssao.panel_id, panel_id, sizeof(ssao.panel_id));
    dvz_strlcpy(ssao.work_label, "ssao", sizeof(ssao.work_label));
    ssao.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    if (!dvz_frame_graph_pass_read(&ssao, normal_id, DVZ_FRAME_GRAPH_ACCESS_SAMPLED) ||
        !dvz_frame_graph_pass_read(&ssao, depth_id, DVZ_FRAME_GRAPH_ACCESS_SAMPLED))
        return false;
    _scene_frame_graph_color_attachment(
        &attachment, occlusion_id, DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR, true);
    attachment.clear_color[0] = 1.0f;
    attachment.clear_color[1] = 1.0f;
    attachment.clear_color[2] = 1.0f;
    attachment.clear_color[3] = 1.0f;
    if (!dvz_frame_graph_pass_color_attachment(&ssao, &attachment))
        return false;
    if (!dvz_frame_plan_graph_pass(plan, &ssao))
        return false;

    if (blur_enabled)
    {
        DvzFrameGraphResource blur = {0};
        dvz_strlcpy(blur.id, blur_id, sizeof(blur.id));
        blur.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
        blur.format = VK_FORMAT_R8_UNORM;
        blur.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
        blur.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT |
                           DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED;
        blur.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
        if (!_scene_frame_graph_resource_once(plan, &blur))
            return false;

        DvzFrameGraphPass blur_pass = {0};
        dvz_strlcpy(blur_pass.id, blur_pass_id, sizeof(blur_pass.id));
        dvz_strlcpy(blur_pass.panel_id, panel_id, sizeof(blur_pass.panel_id));
        dvz_strlcpy(blur_pass.work_label, "ssao_blur", sizeof(blur_pass.work_label));
        blur_pass.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
        if (!dvz_frame_graph_pass_read(&blur_pass, occlusion_id, DVZ_FRAME_GRAPH_ACCESS_SAMPLED) ||
            !dvz_frame_graph_pass_read(&blur_pass, normal_id, DVZ_FRAME_GRAPH_ACCESS_SAMPLED) ||
            !dvz_frame_graph_pass_read(&blur_pass, depth_id, DVZ_FRAME_GRAPH_ACCESS_SAMPLED))
            return false;
        _scene_frame_graph_color_attachment(
            &attachment, blur_id, DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR, true);
        attachment.clear_color[0] = 1.0f;
        attachment.clear_color[1] = 1.0f;
        attachment.clear_color[2] = 1.0f;
        attachment.clear_color[3] = 1.0f;
        if (!dvz_frame_graph_pass_color_attachment(&blur_pass, &attachment))
            return false;
        if (!dvz_frame_plan_graph_pass(plan, &blur_pass))
            return false;
    }

    DvzFrameGraphPass composite = {0};
    dvz_strlcpy(composite.id, composite_id, sizeof(composite.id));
    dvz_strlcpy(composite.panel_id, panel_id, sizeof(composite.panel_id));
    dvz_strlcpy(composite.work_label, "ssao_composite", sizeof(composite.work_label));
    composite.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    if (!dvz_frame_graph_pass_read(
            &composite, blur_enabled ? blur_id : occlusion_id, DVZ_FRAME_GRAPH_ACCESS_SAMPLED))
        return false;
    _scene_frame_graph_color_attachment(
        &attachment, "rt", DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD, false);
    if (!dvz_frame_graph_pass_color_attachment(&composite, &attachment))
        return false;
    return dvz_frame_plan_graph_pass(plan, &composite);
}
