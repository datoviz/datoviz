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
#include <string.h>

#include <vulkan/vulkan_core.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_scene_resource_key.h"
#include "_shader_registry.h"
#include "_technique.h"
#include "_visual_pipeline.h"
#include "_visual_pipeline_internal.h"
#include "datoviz/drp2/enums.h"


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
static void _pipeline_attr(
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
static void _pipeline_instance_transform(
    DvzSceneVisualPipelineDesc* out, uint32_t first_attr, uint32_t binding)
{
    ANN(out);
    for (uint32_t i = 0; i < 4; i++)
    {
        _pipeline_attr(
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
static void _pipeline_apply_standard_depth_state(
    const DvzSceneVisualPassCaps* caps, bool pass_needs_depth, bool wboit_accumulation,
    DvzAlphaMode alpha_mode, DvzSceneVisualPipelineDesc* out)
{
    ANN(caps);
    ANN(out);

    if (!pass_needs_depth)
        return;

    out->depth_write_enabled =
        caps->can_write_depth && !wboit_accumulation && alpha_mode != DVZ_ALPHA_BLENDED;
    out->depth_compare_op =
        caps->can_depth_test ? VK_COMPARE_OP_LESS_OR_EQUAL : VK_COMPARE_OP_ALWAYS;
}


/**
 * Resolve vertex-layout and depth-state metadata for one visual descriptor.
 *
 * @param visual the visual descriptor
 * @param picking whether the render pass is a picking pass
 * @param pass_needs_depth whether the containing render pass has a depth attachment
 * @param controller_mode controller attachment mode for the visual
 * @param out the output pipeline descriptor
 * @return whether a pipeline descriptor was resolved
 */
bool _scene_visual_pipeline_desc(
    const DvzSceneVisualDesc* visual, bool picking, bool pass_needs_depth, bool wboit_accumulation,
    DvzAlphaMode alpha_mode, DvzControllerMode controller_mode, DvzSceneVisualPipelineDesc* out)
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

    switch (visual->kind)
    {
    case DVZ_SCENE_VISUAL_DESC_PIXEL:
    case DVZ_SCENE_VISUAL_DESC_POINT:
    case DVZ_SCENE_VISUAL_DESC_SPHERE:
    case DVZ_SCENE_VISUAL_DESC_MARKER:
        if (visual->kind == DVZ_SCENE_VISUAL_DESC_MARKER)
        {
            uint32_t attr_count = picking ? 2 : 5;
            if (visual->has_selection_mask && !picking)
                attr_count++;
            out->vertex_buffer_count = visual->has_selection_mask ? 6 : 5;
            out->binding_count = out->vertex_buffer_count;
            out->attr_count = attr_count;
            _pipeline_attr(out, 0, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float));
            _pipeline_attr(
                out, 1, picking ? 2 : 1, picking ? 2 : 1,
                picking ? VK_FORMAT_R32_SFLOAT : VK_FORMAT_R8G8B8A8_UNORM, 4 * sizeof(uint8_t));
            _pipeline_attr(out, 2, 2, 2, VK_FORMAT_R32_SFLOAT, sizeof(float));
            _pipeline_attr(out, 3, 3, 3, VK_FORMAT_R32_SFLOAT, sizeof(float));
            _pipeline_attr(out, 4, 4, 4, VK_FORMAT_R32_UINT, sizeof(uint32_t));
            if (visual->has_selection_mask && !picking)
                _pipeline_attr(out, 5, 5, 5, VK_FORMAT_R8_UINT, sizeof(uint8_t));
            out->needs_common_layout = caps.uses_common_set;
            out->needs_material_layout = caps.needs_material_layout && !picking;
            _pipeline_apply_standard_depth_state(
                &caps, pass_needs_depth, wboit_accumulation, alpha_mode, out);
            return true;
        }

        out->vertex_buffer_count = visual->has_selection_mask ? 4 : 3;
        out->binding_count = out->vertex_buffer_count;
        out->attr_count = visual->kind == DVZ_SCENE_VISUAL_DESC_SPHERE ? 3
                          : picking                                    ? 2
                          : visual->has_selection_mask                 ? 4
                                                                       : 3;
        uint32_t color_binding = picking && visual->kind != DVZ_SCENE_VISUAL_DESC_SPHERE ? 2 : 1;
        uint32_t color_format = picking && visual->kind != DVZ_SCENE_VISUAL_DESC_SPHERE
                                    ? VK_FORMAT_R32_SFLOAT
                                    : VK_FORMAT_R8G8B8A8_UNORM;
        _pipeline_attr(out, 0, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float));
        _pipeline_attr(out, 1, color_binding, color_binding, color_format, 4 * sizeof(uint8_t));
        _pipeline_attr(out, 2, 2, 2, VK_FORMAT_R32_SFLOAT, sizeof(float));
        if (visual->has_selection_mask && !picking)
            _pipeline_attr(out, 3, 3, 5, VK_FORMAT_R8_UINT, sizeof(uint8_t));
        out->needs_common_layout = caps.uses_common_set;
        out->needs_material_layout = caps.needs_material_layout;
        _pipeline_apply_standard_depth_state(
            &caps, pass_needs_depth, wboit_accumulation, alpha_mode, out);
        return true;

    case DVZ_SCENE_VISUAL_DESC_PRIMITIVE:
        out->vertex_buffer_count =
            (visual->has_normal ? 3u : 2u) + (visual->has_instance_transform ? 1u : 0u);
        out->binding_count = out->vertex_buffer_count;
        out->attr_count =
            (visual->has_normal ? 3u : 2u) + (visual->has_instance_transform ? 4u : 0u);
        _pipeline_attr(out, 0, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float));
        _pipeline_attr(out, 1, 1, 1, VK_FORMAT_R8G8B8A8_UNORM, 4 * sizeof(uint8_t));
        if (visual->has_normal)
            _pipeline_attr(out, 2, 2, 2, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float));
        if (visual->has_instance_transform)
        {
            uint32_t transform_binding = visual->has_normal ? 3 : 2;
            _pipeline_instance_transform(out, transform_binding, transform_binding);
        }
        out->needs_common_layout = caps.uses_common_set;
        out->needs_material_layout = caps.needs_material_layout;
        _pipeline_apply_standard_depth_state(
            &caps, pass_needs_depth, wboit_accumulation, alpha_mode, out);
        return true;

    case DVZ_SCENE_VISUAL_DESC_SEGMENT:
    case DVZ_SCENE_VISUAL_DESC_PATH:
        out->vertex_buffer_count = 4;
        out->binding_count = 4;
        out->attr_count = 4;
        _pipeline_attr(out, 0, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float));
        _pipeline_attr(out, 1, 1, 1, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float));
        _pipeline_attr(out, 2, 2, 2, VK_FORMAT_R8G8B8A8_UNORM, 4 * sizeof(uint8_t));
        _pipeline_attr(out, 3, 3, 3, VK_FORMAT_R32_SFLOAT, sizeof(float));
        out->needs_common_layout = caps.uses_common_set;
        out->needs_material_layout = caps.needs_material_layout;
        _pipeline_apply_standard_depth_state(
            &caps, pass_needs_depth, wboit_accumulation, alpha_mode, out);
        return true;

    case DVZ_SCENE_VISUAL_DESC_IMAGE:
        out->vertex_buffer_count = 2;
        out->binding_count = 2;
        out->attr_count = 2;
        _pipeline_attr(out, 0, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float));
        _pipeline_attr(out, 1, 1, 1, VK_FORMAT_R32G32_SFLOAT, 2 * sizeof(float));
        out->needs_common_layout = caps.uses_common_set;
        out->needs_image_layout = caps.uses_image_set;
        return true;

    case DVZ_SCENE_VISUAL_DESC_GLYPH:
        out->vertex_buffer_count = 5;
        out->binding_count = 5;
        out->attr_count = 5;
        _pipeline_attr(out, 0, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float));
        _pipeline_attr(out, 1, 1, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 4 * sizeof(float));
        _pipeline_attr(out, 2, 2, 2, VK_FORMAT_R32G32B32A32_SFLOAT, 4 * sizeof(float));
        _pipeline_attr(out, 3, 3, 3, VK_FORMAT_R8G8B8A8_UNORM, 4 * sizeof(uint8_t));
        _pipeline_attr(out, 4, 4, 4, VK_FORMAT_R32_SFLOAT, sizeof(float));
        out->needs_common_layout = caps.uses_common_set;
        out->needs_glyph_layout = caps.uses_image_set;
        return true;

    case DVZ_SCENE_VISUAL_DESC_VOLUME:
        out->vertex_buffer_count = 2;
        out->binding_count = 2;
        out->attr_count = 2;
        _pipeline_attr(out, 0, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float));
        _pipeline_attr(out, 1, 1, 1, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float));
        out->needs_common_layout = caps.uses_common_set;
        out->needs_volume_layout = caps.uses_volume_set;
        out->has_raster_state = true;
        out->cull_mode = VK_CULL_MODE_BACK_BIT;
        out->front_face = VK_FRONT_FACE_CLOCKWISE;
        return true;

    case DVZ_SCENE_VISUAL_DESC_NONE:
    default:
        return false;
    }
}
