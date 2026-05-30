/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene visual render pipeline descriptors */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>
#include <vulkan/vulkan_core.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_technique.h"
#include "_visual_pipeline.h"
#include "_visual_pipeline_internal.h"
#include "datoviz/drp2/enums.h"
#include "registry/registry.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Set one vertex attribute slot in a visual pipeline descriptor.
 *
 * @param out pipeline descriptor to update
 * @param index descriptor attribute index
 * @param binding vertex buffer binding index
 * @param location shader input location
 * @param format vertex input format
 * @param stride vertex buffer stride in bytes
 */
void _scene_visual_pipeline_attr(
    DvzSceneVisualPipelineDesc* out, uint32_t index, uint32_t binding, uint32_t location,
    uint32_t format, uint32_t stride)
{
    ANN(out);
    ASSERT(index < DVZ_SCENE_MAX_NODE_RESOURCES);

    out->bindings[index] = binding;
    out->locations[index] = location;
    out->formats[index] = format;
    out->strides[index] = stride;
    out->strides[binding] = stride;
    if (out->step_modes[binding] == 0)
        out->step_modes[binding] = DVZ_DRP2_VERTEX_STEP_MODE_VERTEX;
}


/**
 * Set one mat4 instance transform vertex input.
 *
 * @param out pipeline descriptor to update
 * @param first_attr first descriptor attribute index
 * @param binding vertex buffer binding index
 */
void _scene_visual_pipeline_instance_transform(
    DvzSceneVisualPipelineDesc* out, uint32_t first_attr, uint32_t binding)
{
    ANN(out);
    for (uint32_t i = 0; i < 4; i++)
    {
        _scene_visual_pipeline_attr(
            out, first_attr + i, binding, 3 + i, VK_FORMAT_R32G32B32A32_SFLOAT,
            16 * sizeof(float));
        out->offsets[first_attr + i] = i * 4 * sizeof(float);
    }
    out->step_modes[binding] = DVZ_DRP2_VERTEX_STEP_MODE_INSTANCE;
}



/**
 * Apply the standard depth-state rules shared by raster visual descriptors.
 *
 * @param caps resolved pass capabilities for the visual
 * @param pass_needs_depth whether the containing pass has a depth attachment
 * @param wboit_accumulation whether the pass is an order-independent transparency pass
 * @param alpha_mode visual alpha mode
 * @param out pipeline descriptor to update
 */
void _scene_visual_pipeline_apply_standard_depth_state(
    const DvzSceneVisualPassCaps* caps, bool pass_needs_depth, bool wboit_accumulation,
    DvzAlphaMode alpha_mode, uint32_t depth_compare_op, DvzSceneVisualPipelineDesc* out)
{
    ANN(caps);
    ANN(out);

    if (!pass_needs_depth)
        return;

    bool forward_compare = depth_compare_op == VK_COMPARE_OP_LESS ||
                           depth_compare_op == VK_COMPARE_OP_LESS_OR_EQUAL;
    out->depth_write_enabled =
        caps->can_write_depth && forward_compare && !wboit_accumulation &&
        alpha_mode != DVZ_ALPHA_BLENDED;
    out->depth_compare_op = caps->can_depth_test ? depth_compare_op : VK_COMPARE_OP_ALWAYS;
}



/**
 * Resolve vertex-layout and depth-state metadata through the visual-family registry.
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
bool _scene_visual_pipeline_desc(
    const DvzSceneVisualDesc* visual, bool picking, bool pass_needs_depth, bool wboit_accumulation,
    DvzAlphaMode alpha_mode, DvzControllerMode controller_mode,
    DvzSceneShaderFormat shader_format, DvzSceneVisualPipelineDesc* out)
{
    ANN(visual);
    ANN(out);
    dvz_memset(out, sizeof(DvzSceneVisualPipelineDesc), 0, sizeof(DvzSceneVisualPipelineDesc));

    const DvzVisualFamilyOps* ops = _scene_visual_family_ops((DvzVisualType)visual->visual_type);
    if (ops == NULL || ops->resolve_pipeline_desc == NULL)
        return false;
    return ops->resolve_pipeline_desc(
        visual, picking, pass_needs_depth, wboit_accumulation, alpha_mode, controller_mode,
        shader_format, out);
}


/**
 * Apply picking query vertex-format overrides to one pipeline descriptor when needed.
 *
 * @param visual the visual descriptor
 * @param color_target_format picking color target format
 * @param pipeline pipeline descriptor to update
 */
void _scene_visual_pipeline_desc_apply_query_pick(
    const DvzSceneVisualDesc* visual, uint32_t color_target_format,
    DvzSceneVisualPipelineDesc* pipeline)
{
    ANN(visual);
    ANN(pipeline);
    if (color_target_format != VK_FORMAT_R32_UINT)
        return;

    if (visual->kind == DVZ_SCENE_VISUAL_DESC_SEGMENT && pipeline->attr_count > 2)
    {
        pipeline->strides[2] = sizeof(uint32_t);
        pipeline->formats[2] = VK_FORMAT_R32_UINT;
    }
    else if (visual->kind == DVZ_SCENE_VISUAL_DESC_PATH && pipeline->attr_count > 3)
    {
        pipeline->strides[3] = sizeof(uint32_t);
        pipeline->formats[3] = VK_FORMAT_R32_UINT;
    }
    else if (visual->kind == DVZ_SCENE_VISUAL_DESC_PRIMITIVE && pipeline->attr_count > 1)
    {
        pipeline->strides[1] = sizeof(uint32_t);
        pipeline->formats[1] = VK_FORMAT_R32_UINT;
    }
}


/**
 * Apply render-pass-specific policy to one pipeline descriptor.
 *
 * @param visual the visual descriptor
 * @param pass_role render pass role being prepared
 * @param force_point_depth whether point-like visuals must write depth
 * @param pass_sample_count multisample count for the render pass
 * @param pass_alpha_to_coverage whether alpha-to-coverage is enabled for the pass
 * @param pipeline pipeline descriptor to update
 */
void _scene_visual_pipeline_desc_apply_pass_policy(
    const DvzSceneVisualDesc* visual, DvzFramePlanRenderPassRole pass_role, bool force_point_depth,
    uint32_t pass_sample_count, bool pass_alpha_to_coverage,
    DvzSceneVisualPipelineDesc* pipeline)
{
    ANN(visual);
    ANN(pipeline);

    bool point_like = visual->kind == DVZ_SCENE_VISUAL_DESC_POINT ||
                      visual->kind == DVZ_SCENE_VISUAL_DESC_PIXEL ||
                      visual->kind == DVZ_SCENE_VISUAL_DESC_MARKER;

    if (
        pass_role == DVZ_FRAME_PLAN_RENDER_PASS_GBUFFER &&
        visual->kind != DVZ_SCENE_VISUAL_DESC_SPHERE)
    {
        pipeline->needs_material_layout = false;
    }

    if (pass_role == DVZ_FRAME_PLAN_RENDER_PASS_SCENE_OCCLUSION)
    {
        pipeline->needs_image_layout = false;
        pipeline->needs_glyph_layout = false;
        pipeline->needs_material_layout = false;
        pipeline->needs_scene_occlusion_layout = false;
        pipeline->has_depth_state = true;
        pipeline->depth_write_enabled = true;
        pipeline->depth_compare_op = VK_COMPARE_OP_LESS_OR_EQUAL;
    }

    if (force_point_depth && point_like)
    {
        pipeline->has_depth_state = true;
        pipeline->depth_write_enabled = true;
        pipeline->depth_compare_op = VK_COMPARE_OP_LESS_OR_EQUAL;
    }

    if (
        pass_sample_count > 1 &&
        (visual->kind == DVZ_SCENE_VISUAL_DESC_SPHERE || point_like) && pass_alpha_to_coverage)
    {
        pipeline->alpha_to_coverage = true;
    }
}
