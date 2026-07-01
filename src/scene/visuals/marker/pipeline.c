/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Marker visual pipeline descriptors                                                           */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "marker/internal.h"

#include <vulkan/vulkan_core.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_visual_pipeline_internal.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve marker visual pipeline metadata.
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
bool _scene_marker_visual_pipeline_desc(
    const DvzSceneVisualDesc* visual, bool picking, bool pass_needs_depth,
    bool wboit_accumulation, DvzAlphaMode alpha_mode, DvzControllerMode controller_mode,
    DvzSceneShaderFormat shader_format, DvzSceneVisualPipelineDesc* out)
{
    ANN(visual);
    ANN(out);
    dvz_memset(out, sizeof(DvzSceneVisualPipelineDesc), 0, sizeof(DvzSceneVisualPipelineDesc));

    DvzSceneVisualPassCaps caps = {0};
    if (!_scene_visual_pass_caps_from_desc(visual, alpha_mode, controller_mode, &caps))
        return false;

    DvzScenePointLikeLoweringDesc lowering = {0};
    if (!_scene_point_like_lowering_desc(
            visual->point_like_kind, shader_format, visual->vertex_count, &lowering))
        return false;

    out->topology = lowering.topology;
    out->has_depth_state = pass_needs_depth;
    out->needs_scene_occlusion_layout = visual->scene_occluded;
    if (pass_needs_depth)
    {
        out->depth_write_enabled = false;
        out->depth_compare_op = DVZ_COMPARE_OP_ALWAYS;
    }

    uint32_t attr_count = picking ? 2 : 5;
    bool atlas_marker = visual->image_texture_id != 0 && !picking;
    bool distance_marker =
        atlas_marker &&
        (visual->glyph_atlas_encoding == DVZ_TEXT_ATLAS_ENCODING_SDF_ALPHA ||
         visual->glyph_atlas_encoding == DVZ_TEXT_ATLAS_ENCODING_MSDF_RGB);
    if (visual->has_item_state && !picking)
        attr_count++;
    if (atlas_marker)
        attr_count++;
    out->vertex_buffer_count = visual->has_item_state ? 6 : 5;
    if (atlas_marker)
        out->vertex_buffer_count++;
    out->binding_count = out->vertex_buffer_count;
    out->attr_count = attr_count;

    _scene_visual_pipeline_attr(out, 0, 0, 0, DVZ_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float));
    _scene_visual_pipeline_attr(
        out, 1, picking ? 2 : 1, picking ? 2 : 1,
        picking ? DVZ_FORMAT_R32_SFLOAT : DVZ_FORMAT_R8G8B8A8_UNORM, 4 * sizeof(uint8_t));
    _scene_visual_pipeline_attr(out, 2, 2, 2, DVZ_FORMAT_R32_SFLOAT, sizeof(float));
    _scene_visual_pipeline_attr(out, 3, 3, 3, DVZ_FORMAT_R32_SFLOAT, sizeof(float));
    _scene_visual_pipeline_attr(out, 4, 4, 4, DVZ_FORMAT_R32_UINT, sizeof(uint32_t));
    uint32_t next_attr = 5;
    if (visual->has_item_state && !picking)
        _scene_visual_pipeline_attr(
            out, next_attr++, 5, 5, DVZ_FORMAT_R32_UINT, sizeof(uint32_t));
    if (atlas_marker)
    {
        uint32_t binding = visual->has_item_state ? 6 : 5;
        _scene_visual_pipeline_attr(
            out, next_attr++, binding, 6, DVZ_FORMAT_R32G32B32A32_SFLOAT, 4 * sizeof(float));
    }
    for (uint32_t i = 0; i < out->binding_count; i++)
        out->step_modes[i] = lowering.vertex_step_mode;

    out->needs_common_layout = caps.uses_common_set;
    out->needs_material_layout = caps.needs_material_layout && !picking && !atlas_marker;
    out->needs_item_state_style_layout = visual->has_item_state && !picking;
    out->needs_image_layout = atlas_marker && !distance_marker;
    out->needs_glyph_layout = distance_marker;
    _scene_visual_pipeline_apply_standard_depth_state(
        &caps, pass_needs_depth, wboit_accumulation, alpha_mode, visual->depth_compare_op, out);
    return true;
}
