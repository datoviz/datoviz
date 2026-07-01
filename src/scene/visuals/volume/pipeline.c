/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Volume visual pipeline descriptors                                                           */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "volume/internal.h"

#include <vulkan/vulkan_core.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_visual_pipeline_internal.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve volume visual pipeline metadata.
 *
 * @param visual the visual descriptor
 * @param picking whether the render pass is a picking pass
 * @param pass_needs_depth whether the containing render pass has a depth attachment
 * @param wboit_accumulation whether the pass is an order-independent transparency pass
 * @param alpha_mode visual alpha mode
 * @param controller_mode controller attachment mode for the visual
 * @param out the output pipeline descriptor
 * @return whether a pipeline descriptor was resolved
 */
bool _scene_volume_visual_pipeline_desc(
    const DvzSceneVisualDesc* visual, bool picking, bool pass_needs_depth,
    bool wboit_accumulation, DvzAlphaMode alpha_mode, DvzControllerMode controller_mode,
    DvzSceneShaderFormat shader_format, DvzSceneVisualPipelineDesc* out)
{
    ANN(visual);
    ANN(out);
    dvz_memset(out, sizeof(DvzSceneVisualPipelineDesc), 0, sizeof(DvzSceneVisualPipelineDesc));
    (void)picking;
    (void)wboit_accumulation;

    bool volume = visual->kind == DVZ_SCENE_VISUAL_DESC_VOLUME ||
                  visual->kind == DVZ_SCENE_VISUAL_DESC_VOLUME_LABELS_SINT ||
                  visual->kind == DVZ_SCENE_VISUAL_DESC_VOLUME_LABELS_UINT;
    if (!volume)
        return false;

    DvzSceneVisualPassCaps caps = {0};
    if (!_scene_visual_pass_caps_from_desc(visual, alpha_mode, controller_mode, &caps))
        return false;

    out->topology = visual->topology;
    out->has_depth_state = pass_needs_depth;
    out->needs_scene_occlusion_layout = visual->scene_occluded;
    if (pass_needs_depth)
    {
        out->depth_write_enabled = false;
        out->depth_compare_op = DVZ_COMPARE_OP_ALWAYS;
    }
    out->vertex_buffer_count = 2;
    out->binding_count = 2;
    out->attr_count = 2;
    _scene_visual_pipeline_attr(out, 0, 0, 0, DVZ_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float));
    _scene_visual_pipeline_attr(out, 1, 1, 1, DVZ_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float));
    out->needs_common_layout = caps.uses_common_set;
    out->needs_volume_layout = caps.uses_volume_set;
    out->has_raster_state = true;
    out->cull_mode = DVZ_CULL_MODE_BACK;
    out->front_face = DVZ_FRONT_FACE_CLOCKWISE;
    return true;
}
