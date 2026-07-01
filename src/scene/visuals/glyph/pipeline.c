/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Glyph visual pipeline descriptors                                                            */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "glyph/internal.h"

#include <vulkan/vulkan_core.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_visual_pipeline_internal.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve glyph visual pipeline metadata.
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
bool _scene_glyph_visual_pipeline_desc(
    const DvzSceneVisualDesc* visual, bool picking, bool pass_needs_depth,
    bool wboit_accumulation, DvzAlphaMode alpha_mode, DvzControllerMode controller_mode,
    DvzSceneShaderFormat shader_format, DvzSceneVisualPipelineDesc* out)
{
    ANN(visual);
    ANN(out);
    dvz_memset(out, sizeof(DvzSceneVisualPipelineDesc), 0, sizeof(DvzSceneVisualPipelineDesc));
    (void)picking;
    (void)wboit_accumulation;

    if (visual->kind != DVZ_SCENE_VISUAL_DESC_GLYPH)
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
    out->vertex_buffer_count = 5;
    out->binding_count = 5;
    out->attr_count = 5;
    _scene_visual_pipeline_attr(out, 0, 0, 0, DVZ_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float));
    _scene_visual_pipeline_attr(out, 1, 1, 1, DVZ_FORMAT_R32G32B32A32_SFLOAT, 4 * sizeof(float));
    _scene_visual_pipeline_attr(out, 2, 2, 2, DVZ_FORMAT_R32G32B32A32_SFLOAT, 4 * sizeof(float));
    _scene_visual_pipeline_attr(out, 3, 3, 3, DVZ_FORMAT_R8G8B8A8_UNORM, 4 * sizeof(uint8_t));
    _scene_visual_pipeline_attr(out, 4, 4, 4, DVZ_FORMAT_R32_SFLOAT, sizeof(float));
    out->needs_common_layout = caps.uses_common_set;
    out->needs_glyph_layout = caps.uses_image_set;
    return true;
}
