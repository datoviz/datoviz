/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Vector visual pipeline descriptors                                                           */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "vector/internal.h"

#include <vulkan/vulkan_core.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_visual_pipeline_internal.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Apply segment-style vector pipeline metadata.
 *
 * @param visual the visual descriptor
 * @param caps resolved pass capabilities for the visual
 * @param pass_needs_depth whether the containing render pass has a depth attachment
 * @param wboit_accumulation whether the pass is an order-independent transparency pass
 * @param alpha_mode visual alpha mode
 * @param out the output pipeline descriptor
 */
static void _vector_segment_pipeline_desc(
    const DvzSceneVisualDesc* visual, const DvzSceneVisualPassCaps* caps, bool pass_needs_depth,
    bool wboit_accumulation, DvzAlphaMode alpha_mode, DvzSceneVisualPipelineDesc* out)
{
    ANN(visual);
    ANN(caps);
    ANN(out);

    out->vertex_buffer_count = 4;
    out->binding_count = 4;
    out->attr_count = 4;
    _scene_visual_pipeline_attr(out, 0, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float));
    _scene_visual_pipeline_attr(out, 1, 1, 1, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float));
    _scene_visual_pipeline_attr(out, 2, 2, 2, VK_FORMAT_R8G8B8A8_UNORM, 4 * sizeof(uint8_t));
    _scene_visual_pipeline_attr(out, 3, 3, 3, VK_FORMAT_R32_SFLOAT, sizeof(float));
    out->needs_common_layout = caps->uses_common_set;
    out->needs_material_layout = caps->needs_material_layout;
    _scene_visual_pipeline_apply_standard_depth_state(
        caps, pass_needs_depth, wboit_accumulation, alpha_mode, visual->depth_compare_op, out);
}



/**
 * Apply path-style vector pipeline metadata.
 *
 * @param visual the visual descriptor
 * @param caps resolved pass capabilities for the visual
 * @param pass_needs_depth whether the containing render pass has a depth attachment
 * @param wboit_accumulation whether the pass is an order-independent transparency pass
 * @param alpha_mode visual alpha mode
 * @param out the output pipeline descriptor
 */
static void _vector_path_pipeline_desc(
    const DvzSceneVisualDesc* visual, const DvzSceneVisualPassCaps* caps, bool pass_needs_depth,
    bool wboit_accumulation, DvzAlphaMode alpha_mode, DvzSceneVisualPipelineDesc* out)
{
    ANN(visual);
    ANN(caps);
    ANN(out);

    out->vertex_buffer_count = 7;
    out->binding_count = 7;
    out->attr_count = 7;
    _scene_visual_pipeline_attr(out, 0, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float));
    _scene_visual_pipeline_attr(out, 1, 1, 1, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float));
    _scene_visual_pipeline_attr(out, 2, 2, 2, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float));
    _scene_visual_pipeline_attr(out, 3, 3, 3, VK_FORMAT_R8G8B8A8_UNORM, 4 * sizeof(uint8_t));
    _scene_visual_pipeline_attr(out, 4, 4, 4, VK_FORMAT_R32_SFLOAT, sizeof(float));
    _scene_visual_pipeline_attr(out, 5, 5, 5, VK_FORMAT_R32_UINT, sizeof(uint32_t));
    _scene_visual_pipeline_attr(out, 6, 6, 6, VK_FORMAT_R32_SFLOAT, sizeof(float));
    out->needs_common_layout = caps->uses_common_set;
    out->needs_material_layout = caps->needs_material_layout;
    _scene_visual_pipeline_apply_standard_depth_state(
        caps, pass_needs_depth, wboit_accumulation, alpha_mode, visual->depth_compare_op, out);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve vector visual pipeline metadata.
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
bool _scene_vector_visual_pipeline_desc(
    const DvzSceneVisualDesc* visual, bool picking, bool pass_needs_depth,
    bool wboit_accumulation, DvzAlphaMode alpha_mode, DvzControllerMode controller_mode,
    DvzSceneVisualPipelineDesc* out)
{
    ANN(visual);
    ANN(out);
    dvz_memset(out, sizeof(DvzSceneVisualPipelineDesc), 0, sizeof(DvzSceneVisualPipelineDesc));
    (void)picking;

    DvzSceneVisualPassCaps caps = {0};
    if (!_scene_visual_pass_caps_from_desc(visual, alpha_mode, controller_mode, &caps))
        return false;

    out->topology = visual->topology;
    out->has_depth_state = pass_needs_depth;
    out->needs_scene_occlusion_layout = visual->scene_occluded;
    if (pass_needs_depth)
    {
        out->depth_write_enabled = false;
        out->depth_compare_op = VK_COMPARE_OP_ALWAYS;
    }

    if (visual->kind == DVZ_SCENE_VISUAL_DESC_SEGMENT)
    {
        _vector_segment_pipeline_desc(
            visual, &caps, pass_needs_depth, wboit_accumulation, alpha_mode, out);
        return true;
    }
    if (visual->kind == DVZ_SCENE_VISUAL_DESC_PATH)
    {
        _vector_path_pipeline_desc(
            visual, &caps, pass_needs_depth, wboit_accumulation, alpha_mode, out);
        return true;
    }
    return false;
}
