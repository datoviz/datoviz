/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene visual family registry                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "registry/registry.h"

#include "_alloc.h"
#include "_assertions.h"
#include "scene_emit/visual_lowering.h"
#include "sample_profile.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Initialize reusable lowering facts with the default position attribute.
 *
 * @param out output lowering facts
 */
static void _lowering_init(DvzVisualLowering* out)
{
    ANN(out);
    dvz_memset(out, sizeof(DvzVisualLowering), 0, sizeof(DvzVisualLowering));
    out->draw_position_attr = "position";
}



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



/**
 * Resolve point visual lowering facts.
 *
 * @param visual the retained visual
 * @param out output lowering facts
 * @return whether lowering facts were resolved
 */
static bool _lower_point(const DvzVisual* visual, DvzVisualLowering* out)
{
    ANN(visual);
    _lowering_init(out);
    out->renderable_kind = DVZ_RENDERABLE_POINT_LIKE;
    out->desc_kind = DVZ_SCENE_VISUAL_DESC_POINT;
    out->point_like_kind = DVZ_SCENE_POINT_LIKE_POINT;
    out->has_point_like_kind = true;
    out->point_style_enabled = visual->material.point_style_enabled;
    out->needs_material_params =
        visual->material.depth_cue_enabled || visual->material.point_style_enabled;
    return true;
}



/**
 * Resolve pixel visual lowering facts.
 *
 * @param visual the retained visual
 * @param out output lowering facts
 * @return whether lowering facts were resolved
 */
static bool _lower_pixel(const DvzVisual* visual, DvzVisualLowering* out)
{
    ANN(visual);
    _lowering_init(out);
    out->renderable_kind = DVZ_RENDERABLE_POINT_LIKE;
    out->desc_kind = DVZ_SCENE_VISUAL_DESC_PIXEL;
    out->point_like_kind = DVZ_SCENE_POINT_LIKE_PIXEL;
    out->has_point_like_kind = true;
    out->needs_material_params = visual->material.depth_cue_enabled;
    return true;
}



/**
 * Resolve marker visual lowering facts.
 *
 * @param visual the retained visual
 * @param out output lowering facts
 * @return whether lowering facts were resolved
 */
static bool _lower_marker(const DvzVisual* visual, DvzVisualLowering* out)
{
    ANN(visual);
    (void)visual;
    _lowering_init(out);
    out->renderable_kind = DVZ_RENDERABLE_POINT_LIKE;
    out->desc_kind = DVZ_SCENE_VISUAL_DESC_MARKER;
    out->point_like_kind = DVZ_SCENE_POINT_LIKE_MARKER;
    out->has_point_like_kind = true;
    out->needs_material_params = true;
    return true;
}



/**
 * Resolve splat visual lowering facts.
 *
 * @param visual the retained visual
 * @param out output lowering facts
 * @return whether lowering facts were resolved
 */
static bool _lower_splat(const DvzVisual* visual, DvzVisualLowering* out)
{
    ANN(visual);
    (void)visual;
    _lowering_init(out);
    out->renderable_kind = DVZ_RENDERABLE_POINT_LIKE;
    out->desc_kind = DVZ_SCENE_VISUAL_DESC_SPLAT;
    return true;
}



/**
 * Resolve sphere visual lowering facts.
 *
 * @param visual the retained visual
 * @param out output lowering facts
 * @return whether lowering facts were resolved
 */
static bool _lower_sphere(const DvzVisual* visual, DvzVisualLowering* out)
{
    ANN(visual);
    (void)visual;
    _lowering_init(out);
    out->renderable_kind = DVZ_RENDERABLE_POINT_LIKE;
    out->desc_kind = DVZ_SCENE_VISUAL_DESC_SPHERE;
    out->needs_material_params = true;
    return true;
}



/**
 * Resolve segment visual lowering facts.
 *
 * @param visual the retained visual
 * @param out output lowering facts
 * @return whether lowering facts were resolved
 */
static bool _lower_segment(const DvzVisual* visual, DvzVisualLowering* out)
{
    ANN(visual);
    _lowering_init(out);
    out->renderable_kind = DVZ_RENDERABLE_STROKE_QUAD;
    out->desc_kind = DVZ_SCENE_VISUAL_DESC_SEGMENT;
    out->needs_material_params = true;
    out->draw_position_attr = "position_start";
    out->stroke_quad_cache = &visual->segment.gpu;
    return true;
}



/**
 * Resolve vector visual lowering facts.
 *
 * @param visual the retained visual
 * @param out output lowering facts
 * @return whether lowering facts were resolved
 */
static bool _lower_vector(const DvzVisual* visual, DvzVisualLowering* out)
{
    ANN(visual);
    _lowering_init(out);
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
}



/**
 * Resolve path visual lowering facts.
 *
 * @param visual the retained visual
 * @param out output lowering facts
 * @return whether lowering facts were resolved
 */
static bool _lower_path(const DvzVisual* visual, DvzVisualLowering* out)
{
    ANN(visual);
    _lowering_init(out);
    out->renderable_kind = _lowering_has_attr_data(visual, "line_width")
                               ? DVZ_RENDERABLE_PATH_STROKE
                               : DVZ_RENDERABLE_INDEXED_MESH;
    out->desc_kind = out->renderable_kind == DVZ_RENDERABLE_PATH_STROKE
                         ? DVZ_SCENE_VISUAL_DESC_PATH
                         : DVZ_SCENE_VISUAL_DESC_PRIMITIVE;
    out->needs_material_params = out->renderable_kind == DVZ_RENDERABLE_PATH_STROKE;
    out->path_stroke_cache = &visual->path.gpu;
    return true;
}



/**
 * Resolve primitive visual lowering facts.
 *
 * @param visual the retained visual
 * @param out output lowering facts
 * @return whether lowering facts were resolved
 */
static bool _lower_primitive(const DvzVisual* visual, DvzVisualLowering* out)
{
    ANN(visual);
    _lowering_init(out);
    out->renderable_kind = DVZ_RENDERABLE_INDEXED_MESH;
    out->desc_kind = DVZ_SCENE_VISUAL_DESC_PRIMITIVE;
    out->needs_material_params = _lowering_has_attr_data(visual, "normal");
    return true;
}



/**
 * Resolve mesh visual lowering facts.
 *
 * @param visual the retained visual
 * @param out output lowering facts
 * @return whether lowering facts were resolved
 */
static bool _lower_mesh(const DvzVisual* visual, DvzVisualLowering* out)
{
    ANN(visual);
    _lowering_init(out);
    out->renderable_kind = DVZ_RENDERABLE_INDEXED_MESH;
    out->desc_kind = visual->field != NULL ? DVZ_SCENE_VISUAL_DESC_TEXTURED_MESH
                                           : DVZ_SCENE_VISUAL_DESC_PRIMITIVE;
    out->needs_material_params = _lowering_has_attr_data(visual, "normal");
    return true;
}



/**
 * Resolve image visual lowering facts.
 *
 * @param visual the retained visual
 * @param out output lowering facts
 * @return whether lowering facts were resolved
 */
static bool _lower_image(const DvzVisual* visual, DvzVisualLowering* out)
{
    ANN(visual);
    _lowering_init(out);
    out->renderable_kind = DVZ_RENDERABLE_TEXTURED_QUAD;
    out->desc_kind = DVZ_SCENE_VISUAL_DESC_IMAGE;
    if (_lowering_has_attr_data(visual, "position_px") &&
        _lowering_has_attr_data(visual, "extent_px"))
    {
        out->draw_position_attr = "position_px";
    }
    return true;
}



/**
 * Resolve labels visual lowering facts.
 *
 * @param visual the retained visual
 * @param out output lowering facts
 * @return whether lowering facts were resolved
 */
static bool _lower_labels(const DvzVisual* visual, DvzVisualLowering* out)
{
    ANN(visual);
    _lowering_init(out);
    out->renderable_kind = DVZ_RENDERABLE_TEXTURED_QUAD;
    return _lowering_labels_desc_kind(visual, &out->desc_kind);
}



/**
 * Resolve glyph visual lowering facts.
 *
 * @param visual the retained visual
 * @param out output lowering facts
 * @return whether lowering facts were resolved
 */
static bool _lower_glyph(const DvzVisual* visual, DvzVisualLowering* out)
{
    ANN(visual);
    (void)visual;
    _lowering_init(out);
    out->renderable_kind = DVZ_RENDERABLE_TEXTURED_QUAD;
    out->desc_kind = DVZ_SCENE_VISUAL_DESC_GLYPH;
    return true;
}



/**
 * Resolve volume visual lowering facts.
 *
 * @param visual the retained visual
 * @param out output lowering facts
 * @return whether lowering facts were resolved
 */
static bool _lower_volume(const DvzVisual* visual, DvzVisualLowering* out)
{
    ANN(visual);
    _lowering_init(out);
    out->renderable_kind = DVZ_RENDERABLE_VOLUME_PROXY;
    return _lowering_volume_desc_kind(visual, &out->desc_kind);
}



/**
 * Reject retained text visual lowering; text synchronizes glyph visuals before render emission.
 *
 * @param visual the retained visual
 * @param out output lowering facts
 * @return false because text visuals are semantic parents, not renderable draws
 */
static bool _lower_text(const DvzVisual* visual, DvzVisualLowering* out)
{
    ANN(visual);
    (void)visual;
    _lowering_init(out);
    out->renderable_kind = DVZ_RENDERABLE_NONE;
    out->desc_kind = DVZ_SCENE_VISUAL_DESC_NONE;
    return false;
}



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

static const DvzVisualFamilyOps VISUAL_FAMILY_OPS[] = {
    {DVZ_VISUAL_TYPE_POINT, "point", _lower_point},
    {DVZ_VISUAL_TYPE_PIXEL, "pixel", _lower_pixel},
    {DVZ_VISUAL_TYPE_MARKER, "marker", _lower_marker},
    {DVZ_VISUAL_TYPE_SEGMENT, "segment", _lower_segment},
    {DVZ_VISUAL_TYPE_PATH, "path", _lower_path},
    {DVZ_VISUAL_TYPE_IMAGE, "image", _lower_image},
    {DVZ_VISUAL_TYPE_MESH, "mesh", _lower_mesh},
    {DVZ_VISUAL_TYPE_VOLUME, "volume", _lower_volume},
    {DVZ_VISUAL_TYPE_PRIMITIVE, "primitive", _lower_primitive},
    {DVZ_VISUAL_TYPE_SPHERE, "sphere", _lower_sphere},
    {DVZ_VISUAL_TYPE_GLYPH, "glyph", _lower_glyph},
    {DVZ_VISUAL_TYPE_TEXT, "text", _lower_text},
    {DVZ_VISUAL_TYPE_LABELS, "labels", _lower_labels},
    {DVZ_VISUAL_TYPE_SPLAT, "splat", _lower_splat},
    {DVZ_VISUAL_TYPE_VECTOR, "vector", _lower_vector},
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return the registered visual-family operations for one visual type.
 *
 * @param type visual type
 * @return registered operations, or NULL when the type is not active
 */
const DvzVisualFamilyOps* _scene_visual_family_ops(DvzVisualType type)
{
    for (uint32_t i = 0; i < DVZ_ARRAY_COUNT(VISUAL_FAMILY_OPS); i++)
    {
        if (VISUAL_FAMILY_OPS[i].type == type)
            return &VISUAL_FAMILY_OPS[i];
    }
    return NULL;
}



/**
 * Return the number of registered visual-family operation records.
 *
 * @return registered operation count
 */
uint32_t _scene_visual_family_ops_count(void)
{
    return DVZ_ARRAY_COUNT(VISUAL_FAMILY_OPS);
}



/**
 * Return one registered visual-family operation record by registry index.
 *
 * @param index registry index
 * @return registered operations, or NULL when the index is invalid
 */
const DvzVisualFamilyOps* _scene_visual_family_ops_at(uint32_t index)
{
    if (index >= DVZ_ARRAY_COUNT(VISUAL_FAMILY_OPS))
        return NULL;
    return &VISUAL_FAMILY_OPS[index];
}



/**
 * Return whether one visual type has registered visual-family operations.
 *
 * @param type visual type
 * @return whether the type has registered operations
 */
bool _scene_visual_family_ops_registered(DvzVisualType type)
{
    return _scene_visual_family_ops(type) != NULL;
}
