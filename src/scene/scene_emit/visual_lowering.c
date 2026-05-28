/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene visual lowering                                                                        */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_alloc.h"
#include "_assertions.h"
#include "_scene.h"
#include "_visual_internal.h"
#include "scene_emit/visual_lowering.h"
#include "_visual_pipeline_internal.h"
#include "registry/registry.h"
#include "sample_profile.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return whether one retained visual has CPU-side data for an attribute.
 *
 * @param visual the retained visual
 * @param attr_name the attribute name
 * @return whether the attribute exists and has data
 */
static bool _lowering_has_attr_data(const DvzVisual* visual, const char* attr_name)
{
    ANN(visual);
    ANN(attr_name);
    int attr_idx = _attr_index(visual, attr_name);
    return attr_idx >= 0 && visual->attrs[attr_idx].data != NULL &&
           visual->attrs[attr_idx].item_count > 0;
}



/**
 * Return whether one image-like visual lowers per-item rectangles to textured quads.
 *
 * @param visual the retained visual
 * @return whether generated quads are needed
 */
static bool _lowering_image_uses_generated_quads(const DvzVisual* visual)
{
    ANN(visual);
    DvzSceneVisualDescKind desc_kind = _scene_visual_lowering_desc_kind(visual);
    bool image_like = desc_kind == DVZ_SCENE_VISUAL_DESC_IMAGE ||
                      desc_kind == DVZ_SCENE_VISUAL_DESC_LABELS_SINT ||
                      desc_kind == DVZ_SCENE_VISUAL_DESC_LABELS_UINT;
    return image_like && (_lowering_has_attr_data(visual, "extent") ||
                          _lowering_has_attr_data(visual, "extent_px"));
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve retained visual family state into reusable renderable lowering facts.
 *
 * @param visual the retained visual
 * @param out the output lowering facts
 * @return whether lowering facts were resolved
 */
bool _scene_visual_lowering_resolve(const DvzVisual* visual, DvzVisualLowering* out)
{
    ANN(visual);
    ANN(out);
    const DvzVisualFamilyOps* ops = _scene_visual_family_ops(visual->type);
    if (ops == NULL || ops->resolve_lowering == NULL)
        return false;
    return ops->resolve_lowering(visual, out);
}



/**
 * Return the reusable renderable primitive kind emitted by one retained visual.
 *
 * @param visual the retained visual
 * @return renderable primitive kind
 */
DvzRenderableKind _scene_visual_lowering_renderable_kind(const DvzVisual* visual)
{
    ANN(visual);
    DvzVisualLowering lowering = {0};
    if (!_scene_visual_lowering_resolve(visual, &lowering))
        return DVZ_RENDERABLE_NONE;
    return lowering.renderable_kind;
}



/**
 * Return the descriptor kind emitted by one retained visual.
 *
 * @param visual the retained visual
 * @return descriptor kind
 */
DvzSceneVisualDescKind _scene_visual_lowering_desc_kind(const DvzVisual* visual)
{
    ANN(visual);
    DvzVisualLowering lowering = {0};
    if (!_scene_visual_lowering_resolve(visual, &lowering))
        return DVZ_SCENE_VISUAL_DESC_NONE;
    return lowering.desc_kind;
}



/**
 * Fill family-owned FramePlan metadata fields from one retained visual.
 *
 * @param visual the retained visual
 * @param metadata the metadata being built
 * @return whether the family-owned metadata was valid
 */
bool _scene_visual_lowering_fill_metadata(
    const DvzVisual* visual, DvzFramePlanVisualMeta* metadata)
{
    ANN(visual);
    ANN(metadata);
    DvzVisualLowering lowering = {0};
    if (!_scene_visual_lowering_resolve(visual, &lowering))
        return false;

    if (_scene_visual_desc_is_volume(lowering.desc_kind))
    {
        metadata->has_volume = true;
        metadata->volume_state = visual->volume;
        metadata->volume_occluded = visual->volume_occluded;
        DvzSceneSampleProfile profile = {0};
        metadata->volume_transfer_rgba =
            visual->field != NULL &&
            _scene_sample_profile_resolve(
                visual->field->desc.format, visual->field->desc.semantic, visual->field->desc.dim,
                &profile) &&
            _scene_sample_profile_is_direct_rgba(&profile);
    }

    if (lowering.desc_kind == DVZ_SCENE_VISUAL_DESC_LABELS_SINT ||
        lowering.desc_kind == DVZ_SCENE_VISUAL_DESC_LABELS_UINT)
    {
        metadata->has_labels = true;
        metadata->labels_state = visual->labels;
    }

    if (_lowering_image_uses_generated_quads(visual))
    {
        if (visual->image_gpu.vertex_count > UINT32_MAX)
            return false;
        if (visual->image_gpu.vertex_count > 0)
            metadata->vertex_count = (uint32_t)visual->image_gpu.vertex_count;
        metadata->image_pixel_space = visual->image_gpu.pixel_space;
    }
    return true;
}



/**
 * Return whether one visual samples the panel volume-occlusion target.
 *
 * @param visual the retained visual
 * @return whether volume occlusion is enabled for the visual
 */
bool _scene_visual_lowering_volume_occluded(const DvzVisual* visual)
{
    ANN(visual);
    return visual->volume_occluded;
}
