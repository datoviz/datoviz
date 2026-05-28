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
#include "registry/registry.h"


/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct PipelineFixedAttr PipelineFixedAttr;
struct PipelineFixedAttr
{
    uint32_t binding;
    uint32_t location;
    uint32_t format;
    uint32_t stride;
};


typedef struct PipelineFixedLayout PipelineFixedLayout;
struct PipelineFixedLayout
{
    DvzSceneVisualDescKind kind;
    const PipelineFixedAttr* attrs;
    uint32_t attr_count;
};



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

static const PipelineFixedAttr SEGMENT_PIPELINE_ATTRS[] = {
    {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float)},
    {1, 1, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float)},
    {2, 2, VK_FORMAT_R8G8B8A8_UNORM, 4 * sizeof(uint8_t)},
    {3, 3, VK_FORMAT_R32_SFLOAT, sizeof(float)},
};


static const PipelineFixedAttr PATH_PIPELINE_ATTRS[] = {
    {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float)},
    {1, 1, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float)},
    {2, 2, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float)},
    {3, 3, VK_FORMAT_R8G8B8A8_UNORM, 4 * sizeof(uint8_t)},
    {4, 4, VK_FORMAT_R32_SFLOAT, sizeof(float)},
    {5, 5, VK_FORMAT_R32_UINT, sizeof(uint32_t)},
    {6, 6, VK_FORMAT_R32_SFLOAT, sizeof(float)},
};


static const PipelineFixedAttr IMAGE_PIPELINE_ATTRS[] = {
    {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float)},
    {1, 1, VK_FORMAT_R32G32_SFLOAT, 2 * sizeof(float)},
};


static const PipelineFixedAttr TEXTURED_MESH_PIPELINE_ATTRS[] = {
    {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float)},
    {1, 1, VK_FORMAT_R8G8B8A8_UNORM, 4 * sizeof(uint8_t)},
    {2, 2, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float)},
    {3, 3, VK_FORMAT_R32G32_SFLOAT, 2 * sizeof(float)},
};


static const PipelineFixedAttr GLYPH_PIPELINE_ATTRS[] = {
    {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float)},
    {1, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 4 * sizeof(float)},
    {2, 2, VK_FORMAT_R32G32B32A32_SFLOAT, 4 * sizeof(float)},
    {3, 3, VK_FORMAT_R8G8B8A8_UNORM, 4 * sizeof(uint8_t)},
    {4, 4, VK_FORMAT_R32_SFLOAT, sizeof(float)},
};


static const PipelineFixedAttr VOLUME_PIPELINE_ATTRS[] = {
    {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float)},
    {1, 1, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float)},
};


static const PipelineFixedLayout FIXED_PIPELINE_LAYOUTS[] = {
    {DVZ_SCENE_VISUAL_DESC_SEGMENT, SEGMENT_PIPELINE_ATTRS,
     (uint32_t)DVZ_ARRAY_COUNT(SEGMENT_PIPELINE_ATTRS)},
    {DVZ_SCENE_VISUAL_DESC_PATH, PATH_PIPELINE_ATTRS,
     (uint32_t)DVZ_ARRAY_COUNT(PATH_PIPELINE_ATTRS)},
    {DVZ_SCENE_VISUAL_DESC_IMAGE, IMAGE_PIPELINE_ATTRS,
     (uint32_t)DVZ_ARRAY_COUNT(IMAGE_PIPELINE_ATTRS)},
    {DVZ_SCENE_VISUAL_DESC_TEXTURED_MESH, TEXTURED_MESH_PIPELINE_ATTRS,
     (uint32_t)DVZ_ARRAY_COUNT(TEXTURED_MESH_PIPELINE_ATTRS)},
    {DVZ_SCENE_VISUAL_DESC_LABELS_SINT, IMAGE_PIPELINE_ATTRS,
     (uint32_t)DVZ_ARRAY_COUNT(IMAGE_PIPELINE_ATTRS)},
    {DVZ_SCENE_VISUAL_DESC_LABELS_UINT, IMAGE_PIPELINE_ATTRS,
     (uint32_t)DVZ_ARRAY_COUNT(IMAGE_PIPELINE_ATTRS)},
    {DVZ_SCENE_VISUAL_DESC_GLYPH, GLYPH_PIPELINE_ATTRS,
     (uint32_t)DVZ_ARRAY_COUNT(GLYPH_PIPELINE_ATTRS)},
    {DVZ_SCENE_VISUAL_DESC_VOLUME, VOLUME_PIPELINE_ATTRS,
     (uint32_t)DVZ_ARRAY_COUNT(VOLUME_PIPELINE_ATTRS)},
    {DVZ_SCENE_VISUAL_DESC_VOLUME_LABELS_SINT, VOLUME_PIPELINE_ATTRS,
     (uint32_t)DVZ_ARRAY_COUNT(VOLUME_PIPELINE_ATTRS)},
    {DVZ_SCENE_VISUAL_DESC_VOLUME_LABELS_UINT, VOLUME_PIPELINE_ATTRS,
     (uint32_t)DVZ_ARRAY_COUNT(VOLUME_PIPELINE_ATTRS)},
};



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
 * Return the fixed vertex layout for one visual descriptor kind.
 *
 * @param kind the visual descriptor kind
 * @return the fixed layout descriptor, or NULL when the family is dynamic
 */
static const PipelineFixedLayout* _pipeline_fixed_layout(DvzSceneVisualDescKind kind)
{
    for (uint32_t i = 0; i < (uint32_t)DVZ_ARRAY_COUNT(FIXED_PIPELINE_LAYOUTS); i++)
    {
        if (FIXED_PIPELINE_LAYOUTS[i].kind == kind)
            return &FIXED_PIPELINE_LAYOUTS[i];
    }
    return NULL;
}



/**
 * Apply one fixed vertex layout to a pipeline descriptor.
 *
 * @param layout fixed vertex-layout descriptor
 * @param out pipeline descriptor to update
 */
static void _pipeline_apply_fixed_layout(
    const PipelineFixedLayout* layout, DvzSceneVisualPipelineDesc* out)
{
    ANN(layout);
    ANN(out);

    out->vertex_buffer_count = layout->attr_count;
    out->binding_count = layout->attr_count;
    out->attr_count = layout->attr_count;
    for (uint32_t i = 0; i < layout->attr_count; i++)
    {
        const PipelineFixedAttr* attr = &layout->attrs[i];
        _pipeline_attr(out, i, attr->binding, attr->location, attr->format, attr->stride);
    }
}



/**
 * Resolve a fixed-layout visual pipeline descriptor.
 *
 * @param visual the visual descriptor
 * @param caps resolved pass capabilities for the visual
 * @param pass_needs_depth whether the containing pass has a depth attachment
 * @param wboit_accumulation whether the pass is an order-independent transparency pass
 * @param alpha_mode visual alpha mode
 * @param out pipeline descriptor to update
 * @return whether the visual uses a fixed layout and has been resolved
 */
static bool _pipeline_apply_fixed_visual(
    const DvzSceneVisualDesc* visual, const DvzSceneVisualPassCaps* caps, bool pass_needs_depth,
    bool wboit_accumulation, DvzAlphaMode alpha_mode, DvzSceneVisualPipelineDesc* out)
{
    ANN(visual);
    ANN(caps);
    ANN(out);

    const PipelineFixedLayout* layout = _pipeline_fixed_layout(visual->kind);
    if (layout == NULL)
        return false;

    _pipeline_apply_fixed_layout(layout, out);
    out->needs_common_layout = caps->uses_common_set;

    if (visual->kind == DVZ_SCENE_VISUAL_DESC_SEGMENT ||
        visual->kind == DVZ_SCENE_VISUAL_DESC_PATH)
    {
        out->needs_material_layout = caps->needs_material_layout;
        _pipeline_apply_standard_depth_state(
            caps, pass_needs_depth, wboit_accumulation, alpha_mode, visual->depth_compare_op, out);
    }
    else if (
        visual->kind == DVZ_SCENE_VISUAL_DESC_IMAGE)
    {
        out->needs_image_layout = caps->uses_image_set;
    }
    else if (visual->kind == DVZ_SCENE_VISUAL_DESC_TEXTURED_MESH)
    {
        out->needs_image_layout = caps->uses_image_set;
        _pipeline_apply_standard_depth_state(
            caps, pass_needs_depth, wboit_accumulation, alpha_mode, visual->depth_compare_op, out);
    }
    else if (
        visual->kind == DVZ_SCENE_VISUAL_DESC_LABELS_SINT ||
        visual->kind == DVZ_SCENE_VISUAL_DESC_LABELS_UINT)
    {
        out->needs_labels_layout = caps->uses_image_set;
    }
    else if (visual->kind == DVZ_SCENE_VISUAL_DESC_GLYPH)
    {
        out->needs_glyph_layout = caps->uses_image_set;
    }
    else if (
        visual->kind == DVZ_SCENE_VISUAL_DESC_VOLUME ||
        visual->kind == DVZ_SCENE_VISUAL_DESC_VOLUME_LABELS_SINT ||
        visual->kind == DVZ_SCENE_VISUAL_DESC_VOLUME_LABELS_UINT)
    {
        out->needs_volume_layout = caps->uses_volume_set;
        out->has_raster_state = true;
        out->cull_mode = VK_CULL_MODE_BACK_BIT;
        out->front_face = VK_FRONT_FACE_CLOCKWISE;
    }
    else
    {
        return false;
    }
    return true;
}


/**
 * Resolve vertex-layout and depth-state metadata for one visual descriptor.
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
bool _scene_visual_pipeline_desc_resolve(
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
    case DVZ_SCENE_VISUAL_DESC_SPLAT:
        if (picking)
            return false;
        out->vertex_buffer_count = 4;
        out->binding_count = 4;
        out->attr_count = 4;
        _pipeline_attr(out, 0, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float));
        _pipeline_attr(out, 1, 1, 1, VK_FORMAT_R8G8B8A8_UNORM, 4 * sizeof(uint8_t));
        _pipeline_attr(out, 2, 2, 2, VK_FORMAT_R32G32_SFLOAT, 2 * sizeof(float));
        _pipeline_attr(out, 3, 3, 3, VK_FORMAT_R32_SFLOAT, sizeof(float));
        out->step_modes[0] = DVZ_DRP2_VERTEX_STEP_MODE_INSTANCE;
        out->step_modes[1] = DVZ_DRP2_VERTEX_STEP_MODE_INSTANCE;
        out->step_modes[2] = DVZ_DRP2_VERTEX_STEP_MODE_INSTANCE;
        out->step_modes[3] = DVZ_DRP2_VERTEX_STEP_MODE_INSTANCE;
        out->needs_common_layout = caps.uses_common_set;
        _pipeline_apply_standard_depth_state(
            &caps, pass_needs_depth, wboit_accumulation, alpha_mode, visual->depth_compare_op,
            out);
        return true;

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
                &caps, pass_needs_depth, wboit_accumulation, alpha_mode,
                visual->depth_compare_op, out);
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
        out->needs_material_layout = caps.needs_material_layout && !picking;
        _pipeline_apply_standard_depth_state(
            &caps, pass_needs_depth, wboit_accumulation, alpha_mode, visual->depth_compare_op,
            out);
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
        out->needs_material_layout = caps.needs_material_layout && !picking;
        _pipeline_apply_standard_depth_state(
            &caps, pass_needs_depth, wboit_accumulation, alpha_mode, visual->depth_compare_op,
            out);
        return true;

    case DVZ_SCENE_VISUAL_DESC_SEGMENT:
    case DVZ_SCENE_VISUAL_DESC_PATH:
    case DVZ_SCENE_VISUAL_DESC_TEXTURED_MESH:
    case DVZ_SCENE_VISUAL_DESC_IMAGE:
    case DVZ_SCENE_VISUAL_DESC_LABELS_SINT:
    case DVZ_SCENE_VISUAL_DESC_LABELS_UINT:
    case DVZ_SCENE_VISUAL_DESC_GLYPH:
    case DVZ_SCENE_VISUAL_DESC_VOLUME:
    case DVZ_SCENE_VISUAL_DESC_VOLUME_LABELS_SINT:
    case DVZ_SCENE_VISUAL_DESC_VOLUME_LABELS_UINT:
        return _pipeline_apply_fixed_visual(
            visual, &caps, pass_needs_depth, wboit_accumulation, alpha_mode, out);

    case DVZ_SCENE_VISUAL_DESC_NONE:
    default:
        return false;
    }
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
    DvzAlphaMode alpha_mode, DvzControllerMode controller_mode, DvzSceneVisualPipelineDesc* out)
{
    ANN(visual);
    ANN(out);
    dvz_memset(out, sizeof(DvzSceneVisualPipelineDesc), 0, sizeof(DvzSceneVisualPipelineDesc));

    const DvzVisualFamilyOps* ops = _scene_visual_family_ops((DvzVisualType)visual->visual_type);
    if (ops == NULL || ops->resolve_pipeline_desc == NULL)
        return false;
    return ops->resolve_pipeline_desc(
        visual, picking, pass_needs_depth, wboit_accumulation, alpha_mode, controller_mode, out);
}


/**
 * Resolve fallback vertex-buffer layout metadata for an untyped descriptor kind.
 *
 * @param kind visual descriptor kind
 * @param vertex_buffer_count number of available vertex buffers
 * @param instanced_point_like whether point-like bindings are stepped per instance
 * @param out the output pipeline descriptor
 * @return whether fallback pipeline metadata was resolved
 */
bool _scene_visual_fallback_pipeline_desc(
    DvzSceneVisualDescKind kind, uint32_t vertex_buffer_count, bool instanced_point_like,
    DvzSceneVisualPipelineDesc* out)
{
    ANN(out);
    dvz_memset(out, sizeof(DvzSceneVisualPipelineDesc), 0, sizeof(DvzSceneVisualPipelineDesc));

    uint32_t binding_count =
        kind == DVZ_SCENE_VISUAL_DESC_MARKER ? 5
        : kind == DVZ_SCENE_VISUAL_DESC_TEXTURED_MESH || kind == DVZ_SCENE_VISUAL_DESC_SPLAT ? 4
        : kind == DVZ_SCENE_VISUAL_DESC_POINT || kind == DVZ_SCENE_VISUAL_DESC_PIXEL
            ? 3
            : 2;
    if (binding_count > vertex_buffer_count)
        binding_count = vertex_buffer_count;
    out->binding_count = binding_count;
    for (uint32_t i = 0; i < binding_count; i++)
        out->step_modes[i] = DVZ_DRP2_VERTEX_STEP_MODE_VERTEX;

    if (kind == DVZ_SCENE_VISUAL_DESC_POINT || kind == DVZ_SCENE_VISUAL_DESC_PIXEL ||
        kind == DVZ_SCENE_VISUAL_DESC_MARKER)
    {
        out->strides[0] = 3 * sizeof(float);
        out->strides[1] = 4 * sizeof(uint8_t);
        out->strides[2] = sizeof(float);
        if (kind == DVZ_SCENE_VISUAL_DESC_MARKER && binding_count >= 5)
        {
            out->strides[3] = sizeof(float);
            out->strides[4] = sizeof(uint32_t);
        }
        if (instanced_point_like)
        {
            for (uint32_t i = 0; i < binding_count; i++)
                out->step_modes[i] = DVZ_DRP2_VERTEX_STEP_MODE_INSTANCE;
        }
        return true;
    }
    if (kind == DVZ_SCENE_VISUAL_DESC_SPLAT)
    {
        out->strides[0] = 3 * sizeof(float);
        out->strides[1] = 4 * sizeof(uint8_t);
        out->strides[2] = 2 * sizeof(float);
        if (binding_count >= 4)
            out->strides[3] = sizeof(float);
        if (instanced_point_like)
        {
            for (uint32_t i = 0; i < binding_count; i++)
                out->step_modes[i] = DVZ_DRP2_VERTEX_STEP_MODE_INSTANCE;
        }
        return true;
    }
    if (kind == DVZ_SCENE_VISUAL_DESC_PRIMITIVE)
    {
        out->strides[0] = 3 * sizeof(float);
        out->strides[1] = 4 * sizeof(uint8_t);
        return true;
    }
    if (kind == DVZ_SCENE_VISUAL_DESC_TEXTURED_MESH)
    {
        out->strides[0] = 3 * sizeof(float);
        out->strides[1] = 4 * sizeof(uint8_t);
        out->strides[2] = 3 * sizeof(float);
        out->strides[3] = 2 * sizeof(float);
        return true;
    }

    out->strides[0] = 3 * sizeof(float);
    out->strides[1] = 2 * sizeof(float);
    return true;
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
