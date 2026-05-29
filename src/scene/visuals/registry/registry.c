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

#include "bounds_internal.h"
#include "glyph/internal.h"
#include "image/internal.h"
#include "labels/internal.h"
#include "marker/internal.h"
#include "mesh/internal.h"
#include "pixel/internal.h"
#include "point/internal.h"
#include "primitive/internal.h"
#include "path/internal.h"
#include "scene_emit/visual_lowering.h"
#include "segment/internal.h"
#include "splat/internal.h"
#include "sphere/internal.h"
#include "text/internal.h"
#include "vector/internal.h"
#include "volume/internal.h"

/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define VISUAL_OPS(_type, _name, _lowering, _bounds, _bind, _pipeline, _shader, _draw)           \
    .type = (_type), .name = (_name), .default_material_kind = DVZ_MATERIAL_KIND_UNLIT,          \
     .default_material_model = DVZ_MATERIAL_MODEL_UNLIT, .resolve_lowering = (_lowering),        \
     .resolve_bounds = (_bounds), .resolve_pass_caps = _scene_visual_default_pass_caps,          \
     .resolve_bind_desc = (_bind), .resolve_pipeline_desc = (_pipeline),                         \
     .resolve_shader_desc = (_shader), .resolve_draw_desc = (_draw)

typedef struct
{
    DvzSceneVisualDescKind desc_kind;
    DvzVisualType type;
} DvzVisualDescDefaultType;


static const DvzVisualDescDefaultType VISUAL_DESC_DEFAULT_TYPES[] = {
    {DVZ_SCENE_VISUAL_DESC_POINT, DVZ_VISUAL_TYPE_POINT},
    {DVZ_SCENE_VISUAL_DESC_PIXEL, DVZ_VISUAL_TYPE_PIXEL},
    {DVZ_SCENE_VISUAL_DESC_SPLAT, DVZ_VISUAL_TYPE_SPLAT},
    {DVZ_SCENE_VISUAL_DESC_MARKER, DVZ_VISUAL_TYPE_MARKER},
    {DVZ_SCENE_VISUAL_DESC_SPHERE, DVZ_VISUAL_TYPE_SPHERE},
    {DVZ_SCENE_VISUAL_DESC_SEGMENT, DVZ_VISUAL_TYPE_SEGMENT},
    {DVZ_SCENE_VISUAL_DESC_PATH, DVZ_VISUAL_TYPE_PATH},
    {DVZ_SCENE_VISUAL_DESC_PRIMITIVE, DVZ_VISUAL_TYPE_PRIMITIVE},
    {DVZ_SCENE_VISUAL_DESC_TEXTURED_MESH, DVZ_VISUAL_TYPE_MESH},
    {DVZ_SCENE_VISUAL_DESC_IMAGE, DVZ_VISUAL_TYPE_IMAGE},
    {DVZ_SCENE_VISUAL_DESC_LABELS_SINT, DVZ_VISUAL_TYPE_LABELS},
    {DVZ_SCENE_VISUAL_DESC_LABELS_UINT, DVZ_VISUAL_TYPE_LABELS},
    {DVZ_SCENE_VISUAL_DESC_GLYPH, DVZ_VISUAL_TYPE_GLYPH},
    {DVZ_SCENE_VISUAL_DESC_VOLUME, DVZ_VISUAL_TYPE_VOLUME},
    {DVZ_SCENE_VISUAL_DESC_VOLUME_LABELS_SINT, DVZ_VISUAL_TYPE_VOLUME},
    {DVZ_SCENE_VISUAL_DESC_VOLUME_LABELS_UINT, DVZ_VISUAL_TYPE_VOLUME},
};

static const DvzVisualFamilyOps VISUAL_FAMILY_OPS[] = {
    {VISUAL_OPS(
         DVZ_VISUAL_TYPE_POINT, "point", _scene_point_visual_lowering,
         _scene_visual_default_bounds, _scene_point_visual_bind_desc,
         _scene_point_visual_pipeline_desc, _scene_point_visual_shader_desc,
         _scene_point_visual_draw_desc),
     .renderable_kind = DVZ_RENDERABLE_POINT_LIKE, .desc_kind = DVZ_SCENE_VISUAL_DESC_POINT,
     .init_state = _scene_visual_init_point_style, .upload_material_params = true,
     .supports_depth_cue = true, .sync_point_style_material = true},
    {VISUAL_OPS(
         DVZ_VISUAL_TYPE_PIXEL, "pixel", _scene_pixel_visual_lowering,
         _scene_visual_default_bounds, _scene_pixel_visual_bind_desc,
         _scene_pixel_visual_pipeline_desc, _scene_pixel_visual_shader_desc,
         _scene_pixel_visual_draw_desc),
     .renderable_kind = DVZ_RENDERABLE_POINT_LIKE, .desc_kind = DVZ_SCENE_VISUAL_DESC_PIXEL,
     .upload_material_params = true, .supports_depth_cue = true},
    {VISUAL_OPS(
         DVZ_VISUAL_TYPE_MARKER, "marker", _scene_marker_visual_lowering,
         _scene_visual_default_bounds, _scene_marker_visual_bind_desc,
         _scene_marker_visual_pipeline_desc, _scene_marker_visual_shader_desc,
         _scene_marker_visual_draw_desc),
     .renderable_kind = DVZ_RENDERABLE_POINT_LIKE, .desc_kind = DVZ_SCENE_VISUAL_DESC_MARKER,
     .init_state = _scene_visual_init_point_style, .upload_material_params = true,
     .sync_point_style_material = true},
    {VISUAL_OPS(
         DVZ_VISUAL_TYPE_SEGMENT, "segment", _scene_segment_visual_lowering,
         _scene_segment_visual_bounds, _scene_segment_visual_bind_desc,
         _scene_segment_visual_pipeline_desc, _scene_segment_visual_shader_desc,
         _scene_segment_visual_draw_desc),
     .renderable_kind = DVZ_RENDERABLE_STROKE_QUAD, .desc_kind = DVZ_SCENE_VISUAL_DESC_SEGMENT,
     .init_state = _scene_segment_visual_init_state,
     .after_attr_set = _scene_stroke_visual_after_attr_set},
    {VISUAL_OPS(
         DVZ_VISUAL_TYPE_PATH, "path", _scene_path_visual_lowering,
         _scene_visual_default_bounds, _scene_path_visual_bind_desc,
         _scene_path_visual_pipeline_desc, _scene_path_visual_shader_desc,
         _scene_path_visual_draw_desc),
     .renderable_kind = DVZ_RENDERABLE_PATH_STROKE, .desc_kind = DVZ_SCENE_VISUAL_DESC_PATH,
     .default_material_kind = DVZ_MATERIAL_KIND_LIT,
     .default_material_model = DVZ_MATERIAL_MODEL_PHONG,
     .init_state = _scene_path_visual_init_state,
     .after_attr_set = _scene_stroke_visual_after_attr_set,
     .upload_position_topology = true},
    {VISUAL_OPS(
         DVZ_VISUAL_TYPE_IMAGE, "image", _scene_image_visual_lowering,
         _scene_image_visual_bounds, _scene_image_visual_bind_desc,
         _scene_image_visual_pipeline_desc, _scene_image_visual_shader_desc,
         _scene_image_visual_draw_desc),
     .renderable_kind = DVZ_RENDERABLE_TEXTURED_QUAD, .desc_kind = DVZ_SCENE_VISUAL_DESC_IMAGE,
     .fill_metadata = _scene_image_visual_fill_metadata, .supports_scale = true},
    {VISUAL_OPS(
         DVZ_VISUAL_TYPE_MESH, "mesh", _scene_mesh_visual_lowering, _scene_mesh_visual_bounds,
         _scene_mesh_visual_bind_desc, _scene_mesh_visual_pipeline_desc,
         _scene_mesh_visual_shader_desc, _scene_mesh_visual_draw_desc),
     .renderable_kind = DVZ_RENDERABLE_INDEXED_MESH, .desc_kind = DVZ_SCENE_VISUAL_DESC_PRIMITIVE,
     .default_material_kind = DVZ_MATERIAL_KIND_LIT,
     .default_material_model = DVZ_MATERIAL_MODEL_PHONG, .supports_material = true,
     .supports_depth_cue = true,
     .after_attr_set = _scene_mesh_visual_after_attr_set, .upload_position_topology = true,
     .upload_material_params = true, .sampled_field_texture_upload = true},
    {VISUAL_OPS(
         DVZ_VISUAL_TYPE_VOLUME, "volume", _scene_volume_visual_lowering,
         _scene_volume_visual_bounds, _scene_volume_visual_bind_desc,
         _scene_volume_visual_pipeline_desc, _scene_volume_visual_shader_desc,
         _scene_volume_visual_draw_desc),
     .renderable_kind = DVZ_RENDERABLE_VOLUME_PROXY, .desc_kind = DVZ_SCENE_VISUAL_DESC_VOLUME,
     .default_material_kind = DVZ_MATERIAL_KIND_VOLUME,
     .init_state = _scene_volume_visual_init_state,
     .fill_metadata = _scene_volume_visual_fill_metadata, .supports_scale = true},
    {VISUAL_OPS(
         DVZ_VISUAL_TYPE_PRIMITIVE, "primitive", _scene_primitive_visual_lowering,
         _scene_visual_default_bounds, _scene_primitive_visual_bind_desc,
         _scene_primitive_visual_pipeline_desc, _scene_primitive_visual_shader_desc,
         _scene_primitive_visual_draw_desc),
     .renderable_kind = DVZ_RENDERABLE_INDEXED_MESH, .desc_kind = DVZ_SCENE_VISUAL_DESC_PRIMITIVE,
     .default_material_kind = DVZ_MATERIAL_KIND_LIT,
     .default_material_model = DVZ_MATERIAL_MODEL_PHONG, .supports_material = true,
     .supports_depth_cue = true, .upload_position_topology = true,
     .upload_material_params = true},
    {VISUAL_OPS(
         DVZ_VISUAL_TYPE_SPHERE, "sphere", _scene_sphere_visual_lowering,
         _scene_sphere_visual_bounds, _scene_sphere_visual_bind_desc,
         _scene_sphere_visual_pipeline_desc, _scene_sphere_visual_shader_desc,
         _scene_sphere_visual_draw_desc),
     .renderable_kind = DVZ_RENDERABLE_POINT_LIKE, .desc_kind = DVZ_SCENE_VISUAL_DESC_SPHERE,
     .default_material_kind = DVZ_MATERIAL_KIND_LIT,
     .default_material_model = DVZ_MATERIAL_MODEL_PHONG, .supports_material = true,
     .supports_depth_cue = true, .upload_position_topology = true,
     .upload_material_params = true},
    {VISUAL_OPS(
         DVZ_VISUAL_TYPE_GLYPH, "glyph", _scene_glyph_visual_lowering,
         _scene_glyph_visual_bounds, _scene_glyph_visual_bind_desc,
         _scene_glyph_visual_pipeline_desc, _scene_glyph_visual_shader_desc,
         _scene_glyph_visual_draw_desc),
     .renderable_kind = DVZ_RENDERABLE_TEXTURED_QUAD, .desc_kind = DVZ_SCENE_VISUAL_DESC_GLYPH,
     .upload_position_topology = true, .panel_clip_rect = true},
    {VISUAL_OPS(
         DVZ_VISUAL_TYPE_TEXT, "text", _scene_text_visual_lowering,
         _scene_visual_default_bounds, _scene_text_visual_bind_desc,
         _scene_text_visual_pipeline_desc, _scene_text_visual_shader_desc,
         _scene_text_visual_draw_desc),
     .reset_state = _scene_text_visual_reset_state, .skip_visual_uploads = true},
    {VISUAL_OPS(
         DVZ_VISUAL_TYPE_LABELS, "labels", _scene_labels_visual_lowering,
         _scene_visual_default_bounds, _scene_labels_visual_bind_desc,
         _scene_labels_visual_pipeline_desc, _scene_labels_visual_shader_desc,
         _scene_labels_visual_draw_desc),
     .renderable_kind = DVZ_RENDERABLE_TEXTURED_QUAD,
     .init_state = _scene_labels_visual_init_state,
     .fill_metadata = _scene_labels_visual_fill_metadata, .sampled_field_texture_upload = true,
     .supports_scale = true, .categorical_scale = true},
    {VISUAL_OPS(
         DVZ_VISUAL_TYPE_SPLAT, "splat", _scene_splat_visual_lowering,
         _scene_visual_default_bounds, _scene_splat_visual_bind_desc,
         _scene_splat_visual_pipeline_desc, _scene_splat_visual_shader_desc,
         _scene_splat_visual_draw_desc),
     .renderable_kind = DVZ_RENDERABLE_POINT_LIKE, .desc_kind = DVZ_SCENE_VISUAL_DESC_SPLAT,
     .validate_attr = _scene_splat_visual_validate_attr},
    {VISUAL_OPS(
         DVZ_VISUAL_TYPE_VECTOR, "vector", _scene_vector_visual_lowering,
         _scene_vector_visual_bounds, _scene_vector_visual_bind_desc,
         _scene_vector_visual_pipeline_desc, _scene_vector_visual_shader_desc,
         _scene_vector_visual_draw_desc),
     .init_state = _scene_vector_visual_init_state,
     .after_attr_set = _scene_stroke_visual_after_attr_set},
};

#undef VISUAL_OPS



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



/**
 * Return the default renderable kind for one retained visual family.
 *
 * @param type retained visual type
 * @return default renderable kind, or NONE when the type is not renderable
 */
DvzRenderableKind _scene_visual_family_renderable_kind(DvzVisualType type)
{
    const DvzVisualFamilyOps* ops = _scene_visual_family_ops(type);
    return ops != NULL ? ops->renderable_kind : DVZ_RENDERABLE_NONE;
}



/**
 * Return the default descriptor kind for one retained visual family.
 *
 * @param type retained visual type
 * @return default descriptor kind, or NONE when there is no default descriptor
 */
DvzSceneVisualDescKind _scene_visual_family_desc_kind(DvzVisualType type)
{
    const DvzVisualFamilyOps* ops = _scene_visual_family_ops(type);
    return ops != NULL ? ops->desc_kind : DVZ_SCENE_VISUAL_DESC_NONE;
}



/**
 * Return the default retained visual family for one descriptor kind.
 *
 * @param kind descriptor kind
 * @return retained visual type, or NONE when the descriptor has no default family
 */
DvzVisualType _scene_visual_family_desc_default_type(DvzSceneVisualDescKind kind)
{
    for (uint32_t i = 0; i < DVZ_ARRAY_COUNT(VISUAL_DESC_DEFAULT_TYPES); i++)
    {
        if (VISUAL_DESC_DEFAULT_TYPES[i].desc_kind == kind)
            return VISUAL_DESC_DEFAULT_TYPES[i].type;
    }
    return DVZ_VISUAL_TYPE_NONE;
}
