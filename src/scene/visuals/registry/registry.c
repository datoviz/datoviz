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

#define VISUAL_OPS_WITH_PASS(                                                                      \
    _type, _name, _lowering, _bounds, _pass, _bind, _pipeline, _shader, _draw, _desc)             \
    .type = (_type), .name = (_name), .resolve_lowering = (_lowering),                          \
     .resolve_bounds = (_bounds), .resolve_pass_caps = (_pass),                                  \
     .resolve_bind_desc = (_bind), .resolve_pipeline_desc = (_pipeline),                         \
     .resolve_shader_desc = (_shader), .resolve_draw_desc = (_draw), .resolve_desc = (_desc)

#define VISUAL_OPS(_type, _name, _lowering, _bounds, _bind, _pipeline, _shader, _draw, _desc)    \
    VISUAL_OPS_WITH_PASS(                                                                         \
        _type, _name, _lowering, _bounds, _scene_visual_default_pass_caps, _bind, _pipeline,      \
        _shader, _draw, _desc)

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


#define SRC_PER_ITEM (1u << DVZ_VISUAL_ATTR_SOURCE_PER_ITEM)
#define SRC_CONSTANT (1u << DVZ_VISUAL_ATTR_SOURCE_CONSTANT)
#define SRC_PER_SPAN (1u << DVZ_VISUAL_ATTR_SOURCE_PER_SPAN)
#define SRC_PER_GROUP (1u << DVZ_VISUAL_ATTR_SOURCE_PER_GROUP)

#define SRC_ITEM_ONLY SRC_PER_ITEM
#define SRC_COLOR_GROUPED (SRC_PER_ITEM | SRC_CONSTANT | SRC_PER_GROUP)
#define SRC_COLOR_SEGMENT (SRC_PER_ITEM | SRC_CONSTANT)
#define SRC_COLOR_PATH (SRC_PER_ITEM | SRC_CONSTANT | SRC_PER_SPAN | SRC_PER_GROUP)
#define SRC_SIZE_GROUPED (SRC_PER_ITEM | SRC_CONSTANT | SRC_PER_GROUP)
#define SRC_LINE_WIDTH (SRC_PER_ITEM | SRC_CONSTANT)


static const DvzVisualFamilyAttrDesc SPLAT_ATTRS[] = {
    {"position", 3 * sizeof(float), SRC_ITEM_ONLY, false},
    {"color", 4 * sizeof(uint8_t), SRC_COLOR_GROUPED, false},
    {"sigma", 2 * sizeof(float), SRC_SIZE_GROUPED, false},
    {"angle", sizeof(float), SRC_ITEM_ONLY, false},
};


static const DvzVisualFamilyAttrDesc POINT_ATTRS[] = {
    {"position", 3 * sizeof(float), SRC_ITEM_ONLY, false},
    {"color", 4 * sizeof(uint8_t), SRC_COLOR_GROUPED, false},
    {"size", sizeof(float), SRC_SIZE_GROUPED, false},
    {"item_state", sizeof(uint32_t), SRC_ITEM_ONLY, false},
};


static const DvzVisualFamilyAttrDesc PIXEL_ATTRS[] = {
    {"position", 3 * sizeof(float), SRC_ITEM_ONLY, false},
    {"color", 4 * sizeof(uint8_t), SRC_COLOR_GROUPED, false},
    {"size", sizeof(float), SRC_SIZE_GROUPED, false},
    {"item_state", sizeof(uint32_t), SRC_ITEM_ONLY, false},
};


static const DvzVisualFamilyAttrDesc MARKER_ATTRS[] = {
    {"position", 3 * sizeof(float), SRC_ITEM_ONLY, false},
    {"color", 4 * sizeof(uint8_t), SRC_COLOR_GROUPED, false},
    {"size", sizeof(float), SRC_SIZE_GROUPED, false},
    {"item_state", sizeof(uint32_t), SRC_ITEM_ONLY, false},
    {"angle", sizeof(float), SRC_ITEM_ONLY, false},
    {"shape", sizeof(uint32_t), SRC_ITEM_ONLY, false},
    {"tex_rect", 4 * sizeof(float), SRC_ITEM_ONLY, false},
};


static const DvzVisualFamilyAttrDesc SPHERE_ATTRS[] = {
    {"position", 3 * sizeof(float), SRC_ITEM_ONLY, false},
    {"color", 4 * sizeof(uint8_t), SRC_COLOR_GROUPED, false},
    {"size", sizeof(float), SRC_SIZE_GROUPED, false},
    {"item_state", sizeof(uint32_t), SRC_ITEM_ONLY, false},
};


static const DvzVisualFamilyAttrDesc PRIMITIVE_ATTRS[] = {
    {"position", 3 * sizeof(float), SRC_ITEM_ONLY, false},
    {"color", 4 * sizeof(uint8_t), SRC_COLOR_GROUPED, false},
    {"normal", 3 * sizeof(float), SRC_ITEM_ONLY, false},
};


static const DvzVisualFamilyAttrDesc MESH_ATTRS[] = {
    {"position", 3 * sizeof(float), SRC_ITEM_ONLY, false},
    {"color", 4 * sizeof(uint8_t), SRC_COLOR_GROUPED, false},
    {"normal", 3 * sizeof(float), SRC_ITEM_ONLY, false},
    {"texcoords", 2 * sizeof(float), SRC_ITEM_ONLY, false},
    {"instance_transform", 16 * sizeof(float), SRC_ITEM_ONLY, true},
    {"item_state", sizeof(uint32_t), SRC_ITEM_ONLY, true},
};


static const DvzVisualFamilyAttrDesc PATH_ATTRS[] = {
    {"position", 3 * sizeof(float), SRC_ITEM_ONLY, false},
    {"color", 4 * sizeof(uint8_t), SRC_COLOR_PATH, false},
    {"line_width", sizeof(float), SRC_LINE_WIDTH, false},
};


static const DvzVisualFamilyAttrDesc SEGMENT_ATTRS[] = {
    {"position_start", 3 * sizeof(float), SRC_ITEM_ONLY, false},
    {"position_end", 3 * sizeof(float), SRC_ITEM_ONLY, false},
    {"color", 4 * sizeof(uint8_t), SRC_COLOR_SEGMENT, false},
    {"line_width", sizeof(float), SRC_LINE_WIDTH, false},
};


static const DvzVisualFamilyAttrDesc VECTOR_ATTRS[] = {
    {"position", 3 * sizeof(float), SRC_ITEM_ONLY, false},
    {"vector", 3 * sizeof(float), SRC_ITEM_ONLY, false},
    {"color", 4 * sizeof(uint8_t), SRC_COLOR_SEGMENT, false},
    {"line_width", sizeof(float), SRC_LINE_WIDTH, false},
};


static const DvzVisualFamilyAttrDesc IMAGE_ATTRS[] = {
    {"position", 3 * sizeof(float), SRC_ITEM_ONLY, false},
    {"extent", 2 * sizeof(float), SRC_ITEM_ONLY, false},
    {"position_px", 3 * sizeof(float), SRC_ITEM_ONLY, false},
    {"extent_px", 2 * sizeof(float), SRC_ITEM_ONLY, false},
    {"anchor", 2 * sizeof(float), SRC_ITEM_ONLY, false},
    {"tex_rect", 4 * sizeof(float), SRC_ITEM_ONLY, false},
    {"texcoords", 2 * sizeof(float), SRC_ITEM_ONLY, false},
};


static const DvzVisualFamilyAttrDesc TEXT_ATTRS[] = {
    {"position", 3 * sizeof(float), SRC_ITEM_ONLY, false},
    {"offset", 2 * sizeof(float), SRC_ITEM_ONLY, false},
    {"anchor", 2 * sizeof(float), SRC_ITEM_ONLY, false},
    {"size", sizeof(float), SRC_SIZE_GROUPED, false},
    {"color", 4 * sizeof(uint8_t), SRC_COLOR_GROUPED, false},
    {"angle", sizeof(float), SRC_ITEM_ONLY, false},
};


static const DvzVisualFamilyAttrDesc GLYPH_ATTRS[] = {
    {"position", 3 * sizeof(float), SRC_ITEM_ONLY, false},
    {"bounds", 4 * sizeof(float), SRC_ITEM_ONLY, false},
    {"texcoords", 4 * sizeof(float), SRC_ITEM_ONLY, false},
    {"color", 4 * sizeof(uint8_t), SRC_COLOR_GROUPED, false},
    {"angle", sizeof(float), SRC_ITEM_ONLY, false},
};


static const DvzVisualFamilyAttrDesc VOLUME_ATTRS[] = {
    {"position", 3 * sizeof(float), SRC_ITEM_ONLY, false},
    {"texcoords", 3 * sizeof(float), SRC_ITEM_ONLY, false},
};

static const DvzVisualFamilyOps VISUAL_FAMILY_OPS[] = {
    {VISUAL_OPS(
         DVZ_VISUAL_TYPE_POINT, "point", _scene_point_visual_lowering,
         _scene_visual_default_bounds, _scene_point_visual_bind_desc,
         _scene_point_visual_pipeline_desc, _scene_point_visual_shader_desc,
         _scene_point_visual_draw_desc, _scene_point_visual_desc_from_metadata),
     .family = DVZ_SCENE_VISUAL_FAMILY_POINT, .renderable_kind = DVZ_RENDERABLE_POINT_LIKE,
     .desc_kind = DVZ_SCENE_VISUAL_DESC_POINT,
     .attrs = POINT_ATTRS, .attr_count = DVZ_ARRAY_COUNT(POINT_ATTRS),
     .expected_attrs = "position, color, diameter_px, item_state",
     .attr_alias_public = "diameter_px", .attr_alias_storage = "size",
     .item_range_attr_name = "position",
     .init_state = _scene_visual_init_point_style, .upload_material_params = true,
     .supports_scalar_color_scale = true, .supports_depth_cue = true,
     .sync_point_style_material = true},
    {VISUAL_OPS(
         DVZ_VISUAL_TYPE_PIXEL, "pixel", _scene_pixel_visual_lowering,
         _scene_visual_default_bounds, _scene_pixel_visual_bind_desc,
         _scene_pixel_visual_pipeline_desc, _scene_pixel_visual_shader_desc,
         _scene_pixel_visual_draw_desc, _scene_pixel_visual_desc_from_metadata),
     .family = DVZ_SCENE_VISUAL_FAMILY_PIXEL, .renderable_kind = DVZ_RENDERABLE_POINT_LIKE,
     .desc_kind = DVZ_SCENE_VISUAL_DESC_PIXEL,
     .attrs = PIXEL_ATTRS, .attr_count = DVZ_ARRAY_COUNT(PIXEL_ATTRS),
     .expected_attrs = "position, color, pixel_size_px, item_state",
     .attr_alias_public = "pixel_size_px", .attr_alias_storage = "size",
     .upload_material_params = true, .supports_scalar_color_scale = true,
     .supports_depth_cue = true},
    {VISUAL_OPS_WITH_PASS(
         DVZ_VISUAL_TYPE_MARKER, "marker", _scene_marker_visual_lowering,
         _scene_visual_default_bounds, _scene_marker_visual_pass_caps,
         _scene_marker_visual_bind_desc, _scene_marker_visual_pipeline_desc,
         _scene_marker_visual_shader_desc, _scene_marker_visual_draw_desc,
         _scene_marker_visual_desc_from_metadata),
     .family = DVZ_SCENE_VISUAL_FAMILY_MARKER, .renderable_kind = DVZ_RENDERABLE_POINT_LIKE,
     .desc_kind = DVZ_SCENE_VISUAL_DESC_MARKER,
     .attrs = MARKER_ATTRS, .attr_count = DVZ_ARRAY_COUNT(MARKER_ATTRS),
     .expected_attrs = "position, color, diameter_px, item_state, angle, shape/symbol, tex_rect",
     .attr_alias_public = "diameter_px", .attr_alias_storage = "size",
     .init_state = _scene_visual_init_point_style, .upload_material_params = true,
     .sync_point_style_material = true, .validate_attr = _scene_marker_visual_validate_attr,
     .after_attr_set = _scene_marker_visual_after_attr_set,
     .attr_storage_name = _scene_marker_visual_attr_storage_name},
    {VISUAL_OPS(
         DVZ_VISUAL_TYPE_SEGMENT, "segment", _scene_segment_visual_lowering,
         _scene_segment_visual_bounds, _scene_segment_visual_bind_desc,
         _scene_segment_visual_pipeline_desc, _scene_segment_visual_shader_desc,
         _scene_segment_visual_draw_desc, _scene_segment_visual_desc_from_metadata),
     .family = DVZ_SCENE_VISUAL_FAMILY_SEGMENT, .renderable_kind = DVZ_RENDERABLE_STROKE_QUAD,
     .desc_kind = DVZ_SCENE_VISUAL_DESC_SEGMENT,
     .attrs = SEGMENT_ATTRS, .attr_count = DVZ_ARRAY_COUNT(SEGMENT_ATTRS),
     .expected_attrs = "position_start, position_end, color, stroke_width_px",
     .attr_alias_public = "stroke_width_px", .attr_alias_storage = "line_width",
     .init_state = _scene_segment_visual_init_state,
     .after_attr_set = _scene_stroke_visual_after_attr_set},
    {VISUAL_OPS(
         DVZ_VISUAL_TYPE_PATH, "path", _scene_path_visual_lowering,
         _scene_visual_default_bounds, _scene_path_visual_bind_desc,
         _scene_path_visual_pipeline_desc, _scene_path_visual_shader_desc,
         _scene_path_visual_draw_desc, _scene_path_visual_desc_from_metadata),
     .family = DVZ_SCENE_VISUAL_FAMILY_PATH, .renderable_kind = DVZ_RENDERABLE_PATH_STROKE,
     .desc_kind = DVZ_SCENE_VISUAL_DESC_PATH,
     .default_material_kind = DVZ_MATERIAL_KIND_LIT,
     .default_material_model = DVZ_MATERIAL_MODEL_PHONG,
     .attrs = PATH_ATTRS, .attr_count = DVZ_ARRAY_COUNT(PATH_ATTRS),
     .expected_attrs = "position, color, stroke_width_px",
     .attr_alias_public = "stroke_width_px", .attr_alias_storage = "line_width",
     .init_state = _scene_path_visual_init_state,
     .after_attr_set = _scene_stroke_visual_after_attr_set,
     .upload_position_topology = true},
    {VISUAL_OPS(
         DVZ_VISUAL_TYPE_IMAGE, "image", _scene_image_visual_lowering,
         _scene_image_visual_bounds, _scene_image_visual_bind_desc,
         _scene_image_visual_pipeline_desc, _scene_image_visual_shader_desc,
         _scene_image_visual_draw_desc, _scene_image_visual_desc_from_metadata),
     .family = DVZ_SCENE_VISUAL_FAMILY_IMAGE, .renderable_kind = DVZ_RENDERABLE_TEXTURED_QUAD,
     .desc_kind = DVZ_SCENE_VISUAL_DESC_IMAGE,
     .attrs = IMAGE_ATTRS, .attr_count = DVZ_ARRAY_COUNT(IMAGE_ATTRS),
     .expected_attrs = "position, extent, position_px, extent_px, anchor, tex_rect, texcoords",
     .fill_metadata = _scene_image_visual_fill_metadata, .supports_scale = true},
    {VISUAL_OPS(
         DVZ_VISUAL_TYPE_MESH, "mesh", _scene_mesh_visual_lowering, _scene_mesh_visual_bounds,
         _scene_mesh_visual_bind_desc, _scene_mesh_visual_pipeline_desc,
         _scene_mesh_visual_shader_desc, _scene_mesh_visual_draw_desc,
         _scene_mesh_visual_desc_from_metadata),
     .family = DVZ_SCENE_VISUAL_FAMILY_MESH, .renderable_kind = DVZ_RENDERABLE_INDEXED_MESH,
     .desc_kind = DVZ_SCENE_VISUAL_DESC_PRIMITIVE,
     .default_material_kind = DVZ_MATERIAL_KIND_LIT,
     .default_material_model = DVZ_MATERIAL_MODEL_PHONG, .supports_material = true,
     .supports_depth_cue = true,
     .attrs = MESH_ATTRS, .attr_count = DVZ_ARRAY_COUNT(MESH_ATTRS),
     .expected_attrs = "position, color, normal, texcoords, instance_transform, item_state",
     .after_attr_set = _scene_mesh_visual_after_attr_set, .upload_position_topology = true,
     .upload_material_params = true, .sampled_field_texture_upload = true},
    {VISUAL_OPS(
         DVZ_VISUAL_TYPE_VOLUME, "volume", _scene_volume_visual_lowering,
         _scene_volume_visual_bounds, _scene_volume_visual_bind_desc,
         _scene_volume_visual_pipeline_desc, _scene_volume_visual_shader_desc,
         _scene_volume_visual_draw_desc, _scene_volume_visual_desc_from_metadata),
     .family = DVZ_SCENE_VISUAL_FAMILY_VOLUME, .renderable_kind = DVZ_RENDERABLE_VOLUME_PROXY,
     .desc_kind = DVZ_SCENE_VISUAL_DESC_VOLUME,
     .default_material_kind = DVZ_MATERIAL_KIND_VOLUME,
     .attrs = VOLUME_ATTRS, .attr_count = DVZ_ARRAY_COUNT(VOLUME_ATTRS),
     .expected_attrs = "position, texcoords, plus a bound 3D field",
     .init_state = _scene_volume_visual_init_state,
     .fill_metadata = _scene_volume_visual_fill_metadata, .supports_scale = true},
    {VISUAL_OPS(
         DVZ_VISUAL_TYPE_PRIMITIVE, "primitive", _scene_primitive_visual_lowering,
         _scene_visual_default_bounds, _scene_primitive_visual_bind_desc,
         _scene_primitive_visual_pipeline_desc, _scene_primitive_visual_shader_desc,
         _scene_primitive_visual_draw_desc, _scene_primitive_visual_desc_from_metadata),
     .family = DVZ_SCENE_VISUAL_FAMILY_PRIMITIVE,
     .renderable_kind = DVZ_RENDERABLE_INDEXED_MESH,
     .desc_kind = DVZ_SCENE_VISUAL_DESC_PRIMITIVE,
     .default_material_kind = DVZ_MATERIAL_KIND_LIT,
     .default_material_model = DVZ_MATERIAL_MODEL_PHONG, .supports_material = true,
     .supports_depth_cue = true, .attrs = PRIMITIVE_ATTRS,
     .attr_count = DVZ_ARRAY_COUNT(PRIMITIVE_ATTRS), .expected_attrs = "position, color, normal",
     .upload_position_topology = true,
     .upload_material_params = true},
    {VISUAL_OPS(
         DVZ_VISUAL_TYPE_SPHERE, "sphere", _scene_sphere_visual_lowering,
         _scene_sphere_visual_bounds, _scene_sphere_visual_bind_desc,
         _scene_sphere_visual_pipeline_desc, _scene_sphere_visual_shader_desc,
         _scene_sphere_visual_draw_desc, _scene_sphere_visual_desc_from_metadata),
     .family = DVZ_SCENE_VISUAL_FAMILY_SPHERE, .renderable_kind = DVZ_RENDERABLE_POINT_LIKE,
     .desc_kind = DVZ_SCENE_VISUAL_DESC_SPHERE,
     .default_material_kind = DVZ_MATERIAL_KIND_LIT,
     .default_material_model = DVZ_MATERIAL_MODEL_PHONG, .supports_material = true,
     .supports_depth_cue = true, .attrs = SPHERE_ATTRS,
     .attr_count = DVZ_ARRAY_COUNT(SPHERE_ATTRS),
     .expected_attrs = "position, color, radius, item_state",
     .attr_alias_public = "radius", .attr_alias_storage = "size", .upload_position_topology = true,
     .upload_material_params = true, .bounds_resolves_local_transform = true,
     .size_attr_is_data_space = true},
    {VISUAL_OPS(
         DVZ_VISUAL_TYPE_GLYPH, "glyph", _scene_glyph_visual_lowering,
         _scene_glyph_visual_bounds, _scene_glyph_visual_bind_desc,
         _scene_glyph_visual_pipeline_desc, _scene_glyph_visual_shader_desc,
         _scene_glyph_visual_draw_desc, _scene_glyph_visual_desc_from_metadata),
     .family = DVZ_SCENE_VISUAL_FAMILY_GLYPH, .renderable_kind = DVZ_RENDERABLE_TEXTURED_QUAD,
     .desc_kind = DVZ_SCENE_VISUAL_DESC_GLYPH,
     .attrs = GLYPH_ATTRS, .attr_count = DVZ_ARRAY_COUNT(GLYPH_ATTRS),
     .expected_attrs = "position, bounds, texcoords, color, angle, plus a bound 2D field",
     .upload_position_topology = true, .panel_clip_rect = true,
     .data_coord_uses_plot_clip_rect = true,
     .sampled_field_texture_upload = true},
    {VISUAL_OPS(
         DVZ_VISUAL_TYPE_TEXT, "text", _scene_text_visual_lowering,
         _scene_visual_default_bounds, _scene_text_visual_bind_desc,
         _scene_text_visual_pipeline_desc, _scene_text_visual_shader_desc,
         _scene_text_visual_draw_desc, NULL),
     .family = DVZ_SCENE_VISUAL_FAMILY_TEXT,
     .attrs = TEXT_ATTRS, .attr_count = DVZ_ARRAY_COUNT(TEXT_ATTRS),
     .expected_attrs = "text strings plus position, anchor, size, color, angle",
     .reset_state = _scene_text_visual_reset_state, .skip_visual_uploads = true},
    {VISUAL_OPS(
         DVZ_VISUAL_TYPE_LABELS, "labels", _scene_labels_visual_lowering,
         _scene_visual_default_bounds, _scene_labels_visual_bind_desc,
         _scene_labels_visual_pipeline_desc, _scene_labels_visual_shader_desc,
         _scene_labels_visual_draw_desc, _scene_labels_visual_desc_from_metadata),
     .family = DVZ_SCENE_VISUAL_FAMILY_LABELS, .renderable_kind = DVZ_RENDERABLE_TEXTURED_QUAD,
     .attrs = IMAGE_ATTRS, .attr_count = DVZ_ARRAY_COUNT(IMAGE_ATTRS),
     .expected_attrs = "position, extent, position_px, extent_px, anchor, tex_rect, texcoords",
     .init_state = _scene_labels_visual_init_state,
     .fill_metadata = _scene_labels_visual_fill_metadata, .sampled_field_texture_upload = true,
     .supports_scale = true, .categorical_scale = true},
    {VISUAL_OPS(
         DVZ_VISUAL_TYPE_SPLAT, "splat", _scene_splat_visual_lowering,
         _scene_visual_default_bounds, _scene_splat_visual_bind_desc,
         _scene_splat_visual_pipeline_desc, _scene_splat_visual_shader_desc,
         _scene_splat_visual_draw_desc, _scene_splat_visual_desc_from_metadata),
     .family = DVZ_SCENE_VISUAL_FAMILY_SPLAT, .renderable_kind = DVZ_RENDERABLE_POINT_LIKE,
     .desc_kind = DVZ_SCENE_VISUAL_DESC_SPLAT,
     .attrs = SPLAT_ATTRS, .attr_count = DVZ_ARRAY_COUNT(SPLAT_ATTRS),
     .expected_attrs = "position, color, sigma, angle",
     .validate_attr = _scene_splat_visual_validate_attr},
    {VISUAL_OPS(
         DVZ_VISUAL_TYPE_VECTOR, "vector", _scene_vector_visual_lowering,
         _scene_vector_visual_bounds, _scene_vector_visual_bind_desc,
         _scene_vector_visual_pipeline_desc, _scene_vector_visual_shader_desc,
         _scene_vector_visual_draw_desc, _scene_vector_visual_desc_from_metadata),
     .family = DVZ_SCENE_VISUAL_FAMILY_VECTOR,
     .attrs = VECTOR_ATTRS, .attr_count = DVZ_ARRAY_COUNT(VECTOR_ATTRS),
     .expected_attrs = "position, optional vector, color, stroke_width_px",
     .attr_alias_public = "stroke_width_px", .attr_alias_storage = "line_width",
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
 * Return the registered visual-family operations for one public visual family.
 *
 * @param family public visual family
 * @return registered operations, or NULL when the family is not active
 */
const DvzVisualFamilyOps* _scene_visual_family_ops_for_family(DvzSceneVisualFamily family)
{
    for (uint32_t i = 0; i < DVZ_ARRAY_COUNT(VISUAL_FAMILY_OPS); i++)
    {
        if (VISUAL_FAMILY_OPS[i].family == family)
            return &VISUAL_FAMILY_OPS[i];
    }
    return NULL;
}



/**
 * Return the public visual family for one retained visual type.
 *
 * @param type retained visual type
 * @return public visual family, or NONE when the type is not active
 */
DvzSceneVisualFamily _scene_visual_family_from_type(DvzVisualType type)
{
    const DvzVisualFamilyOps* ops = _scene_visual_family_ops(type);
    return ops != NULL ? ops->family : DVZ_SCENE_VISUAL_FAMILY_NONE;
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
