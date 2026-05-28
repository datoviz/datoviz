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
#include "_visual_lowering.h"
#include "_visual_pipeline_internal.h"
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



/**
 * Return whether a vector visual lowers through path-stroke geometry.
 *
 * @param visual the retained visual
 * @return whether the vector has path-style point data
 */
static bool _lowering_vector_uses_path_stroke(const DvzVisual* visual)
{
    ANN(visual);
    return visual->type == DVZ_VISUAL_TYPE_VECTOR &&
           !_lowering_has_attr_data(visual, "vector") &&
           _lowering_has_attr_data(visual, "position") &&
           _lowering_has_attr_data(visual, "color") &&
           _lowering_has_attr_data(visual, "line_width");
}



/**
 * Resolve label texture descriptor kind from the retained field profile.
 *
 * @param visual the retained labels visual
 * @param out the output descriptor kind
 * @return whether the field profile is supported
 */
static bool _lowering_labels_desc_kind(const DvzVisual* visual, DvzSceneVisualDescKind* out)
{
    ANN(visual);
    ANN(out);
    if (visual->field == NULL)
        return false;
    DvzSceneSampleProfile profile = {0};
    if (!_scene_sample_profile_resolve(
            visual->field->desc.format, visual->field->desc.semantic, visual->field->desc.dim,
            &profile))
    {
        return false;
    }
    if (_scene_sample_profile_is_signed_label(&profile))
    {
        *out = DVZ_SCENE_VISUAL_DESC_LABELS_SINT;
        return true;
    }
    if (_scene_sample_profile_is_unsigned_label(&profile))
    {
        *out = DVZ_SCENE_VISUAL_DESC_LABELS_UINT;
        return true;
    }
    return false;
}



/**
 * Resolve volume texture descriptor kind from the retained field profile.
 *
 * @param visual the retained volume visual
 * @param out the output descriptor kind
 * @return whether the descriptor kind was resolved
 */
static bool _lowering_volume_desc_kind(const DvzVisual* visual, DvzSceneVisualDescKind* out)
{
    ANN(visual);
    ANN(out);
    *out = DVZ_SCENE_VISUAL_DESC_VOLUME;
    if (visual->field == NULL)
        return true;

    DvzSceneSampleProfile profile = {0};
    if (!_scene_sample_profile_resolve(
            visual->field->desc.format, visual->field->desc.semantic, DVZ_FIELD_DIM_3D,
            &profile))
    {
        return true;
    }
    if (_scene_sample_profile_is_signed_label(&profile))
        *out = DVZ_SCENE_VISUAL_DESC_VOLUME_LABELS_SINT;
    else if (_scene_sample_profile_is_unsigned_label(&profile))
        *out = DVZ_SCENE_VISUAL_DESC_VOLUME_LABELS_UINT;
    return true;
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
    dvz_memset(out, sizeof(DvzVisualLowering), 0, sizeof(DvzVisualLowering));
    out->draw_position_attr = "position";

    switch (visual->type)
    {
    case DVZ_VISUAL_TYPE_POINT:
        out->renderable_kind = DVZ_RENDERABLE_POINT_LIKE;
        out->desc_kind = DVZ_SCENE_VISUAL_DESC_POINT;
        out->point_like_kind = DVZ_SCENE_POINT_LIKE_POINT;
        out->has_point_like_kind = true;
        out->point_style_enabled = visual->material.point_style_enabled;
        out->needs_material_params =
            visual->material.depth_cue_enabled || visual->material.point_style_enabled;
        return true;
    case DVZ_VISUAL_TYPE_PIXEL:
        out->renderable_kind = DVZ_RENDERABLE_POINT_LIKE;
        out->desc_kind = DVZ_SCENE_VISUAL_DESC_PIXEL;
        out->point_like_kind = DVZ_SCENE_POINT_LIKE_PIXEL;
        out->has_point_like_kind = true;
        out->needs_material_params = visual->material.depth_cue_enabled;
        return true;
    case DVZ_VISUAL_TYPE_MARKER:
        out->renderable_kind = DVZ_RENDERABLE_POINT_LIKE;
        out->desc_kind = DVZ_SCENE_VISUAL_DESC_MARKER;
        out->point_like_kind = DVZ_SCENE_POINT_LIKE_MARKER;
        out->has_point_like_kind = true;
        out->needs_material_params = true;
        return true;
    case DVZ_VISUAL_TYPE_SPLAT:
        out->renderable_kind = DVZ_RENDERABLE_POINT_LIKE;
        out->desc_kind = DVZ_SCENE_VISUAL_DESC_SPLAT;
        return true;
    case DVZ_VISUAL_TYPE_SPHERE:
        out->renderable_kind = DVZ_RENDERABLE_POINT_LIKE;
        out->desc_kind = DVZ_SCENE_VISUAL_DESC_SPHERE;
        out->needs_material_params = true;
        return true;
    case DVZ_VISUAL_TYPE_SEGMENT:
        out->renderable_kind = DVZ_RENDERABLE_STROKE_QUAD;
        out->desc_kind = DVZ_SCENE_VISUAL_DESC_SEGMENT;
        out->needs_material_params = true;
        out->draw_position_attr = "position_start";
        out->stroke_quad_cache = &visual->segment.gpu;
        return true;
    case DVZ_VISUAL_TYPE_VECTOR:
        out->renderable_kind = _lowering_vector_uses_path_stroke(visual)
                                   ? DVZ_RENDERABLE_PATH_STROKE
                                   : DVZ_RENDERABLE_STROKE_QUAD;
        out->desc_kind = out->renderable_kind == DVZ_RENDERABLE_PATH_STROKE
                             ? DVZ_SCENE_VISUAL_DESC_PATH
                             : DVZ_SCENE_VISUAL_DESC_SEGMENT;
        out->needs_material_params = true;
        out->needs_vector_params_sync = true;
        out->stroke_quad_cache = &visual->vector.stroke_gpu;
        out->path_stroke_cache = &visual->vector.path_gpu;
        return true;
    case DVZ_VISUAL_TYPE_PATH:
        out->renderable_kind = _lowering_has_attr_data(visual, "line_width")
                                   ? DVZ_RENDERABLE_PATH_STROKE
                                   : DVZ_RENDERABLE_INDEXED_MESH;
        out->desc_kind = out->renderable_kind == DVZ_RENDERABLE_PATH_STROKE
                             ? DVZ_SCENE_VISUAL_DESC_PATH
                             : DVZ_SCENE_VISUAL_DESC_PRIMITIVE;
        out->needs_material_params = out->renderable_kind == DVZ_RENDERABLE_PATH_STROKE;
        out->path_stroke_cache = &visual->path.gpu;
        return true;
    case DVZ_VISUAL_TYPE_PRIMITIVE:
        out->renderable_kind = DVZ_RENDERABLE_INDEXED_MESH;
        out->desc_kind = DVZ_SCENE_VISUAL_DESC_PRIMITIVE;
        out->needs_material_params = _lowering_has_attr_data(visual, "normal");
        return true;
    case DVZ_VISUAL_TYPE_MESH:
        out->renderable_kind = DVZ_RENDERABLE_INDEXED_MESH;
        out->desc_kind = visual->field != NULL ? DVZ_SCENE_VISUAL_DESC_TEXTURED_MESH
                                               : DVZ_SCENE_VISUAL_DESC_PRIMITIVE;
        out->needs_material_params = _lowering_has_attr_data(visual, "normal");
        return true;
    case DVZ_VISUAL_TYPE_IMAGE:
        out->renderable_kind = DVZ_RENDERABLE_TEXTURED_QUAD;
        out->desc_kind = DVZ_SCENE_VISUAL_DESC_IMAGE;
        if (_lowering_has_attr_data(visual, "position_px") &&
            _lowering_has_attr_data(visual, "extent_px"))
        {
            out->draw_position_attr = "position_px";
        }
        return true;
    case DVZ_VISUAL_TYPE_LABELS:
        out->renderable_kind = DVZ_RENDERABLE_TEXTURED_QUAD;
        return _lowering_labels_desc_kind(visual, &out->desc_kind);
    case DVZ_VISUAL_TYPE_GLYPH:
        out->renderable_kind = DVZ_RENDERABLE_TEXTURED_QUAD;
        out->desc_kind = DVZ_SCENE_VISUAL_DESC_GLYPH;
        return true;
    case DVZ_VISUAL_TYPE_VOLUME:
        out->renderable_kind = DVZ_RENDERABLE_VOLUME_PROXY;
        return _lowering_volume_desc_kind(visual, &out->desc_kind);
    case DVZ_VISUAL_TYPE_NONE:
    default:
        out->renderable_kind = DVZ_RENDERABLE_NONE;
        out->desc_kind = DVZ_SCENE_VISUAL_DESC_NONE;
        return false;
    }
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
