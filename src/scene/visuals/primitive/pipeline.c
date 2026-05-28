/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Primitive visual pipeline descriptors                                                        */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "primitive/internal.h"

#include <vulkan/vulkan_core.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_visual_pipeline_internal.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve primitive visual pipeline metadata.
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
bool _scene_primitive_visual_pipeline_desc(
    const DvzSceneVisualDesc* visual, bool picking, bool pass_needs_depth,
    bool wboit_accumulation, DvzAlphaMode alpha_mode, DvzControllerMode controller_mode,
    DvzSceneVisualPipelineDesc* out)
{
    ANN(visual);
    ANN(out);
    dvz_memset(out, sizeof(DvzSceneVisualPipelineDesc), 0, sizeof(DvzSceneVisualPipelineDesc));

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

    out->vertex_buffer_count =
        (visual->has_normal ? 3u : 2u) + (visual->has_instance_transform ? 1u : 0u);
    out->binding_count = out->vertex_buffer_count;
    out->attr_count = (visual->has_normal ? 3u : 2u) +
                      (visual->has_instance_transform ? 4u : 0u);
    _scene_visual_pipeline_attr(out, 0, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float));
    _scene_visual_pipeline_attr(out, 1, 1, 1, VK_FORMAT_R8G8B8A8_UNORM, 4 * sizeof(uint8_t));
    if (visual->has_normal)
    {
        _scene_visual_pipeline_attr(
            out, 2, 2, 2, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float));
    }
    if (visual->has_instance_transform)
    {
        uint32_t transform_binding = visual->has_normal ? 3 : 2;
        _scene_visual_pipeline_instance_transform(out, transform_binding, transform_binding);
    }

    out->needs_common_layout = caps.uses_common_set;
    out->needs_material_layout = caps.needs_material_layout && !picking;
    _scene_visual_pipeline_apply_standard_depth_state(
        &caps, pass_needs_depth, wboit_accumulation, alpha_mode, visual->depth_compare_op, out);
    return true;
}
