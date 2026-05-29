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
    .type = (_type), .name = (_name), .resolve_lowering = (_lowering),                          \
     .resolve_bounds = (_bounds), .resolve_pass_caps = _scene_visual_default_pass_caps,          \
     .resolve_bind_desc = (_bind), .resolve_pipeline_desc = (_pipeline),                         \
     .resolve_shader_desc = (_shader), .resolve_draw_desc = (_draw)

static const DvzVisualFamilyOps VISUAL_FAMILY_OPS[] = {
    {VISUAL_OPS(
         DVZ_VISUAL_TYPE_POINT, "point", _scene_point_visual_lowering,
         _scene_visual_default_bounds, _scene_point_visual_bind_desc,
         _scene_point_visual_pipeline_desc, _scene_point_visual_shader_desc,
         _scene_point_visual_draw_desc),
     .init_state = _scene_visual_init_point_style, .upload_material_params = true},
    {VISUAL_OPS(
         DVZ_VISUAL_TYPE_PIXEL, "pixel", _scene_pixel_visual_lowering,
         _scene_visual_default_bounds, _scene_pixel_visual_bind_desc,
         _scene_pixel_visual_pipeline_desc, _scene_pixel_visual_shader_desc,
         _scene_pixel_visual_draw_desc),
     .upload_material_params = true},
    {VISUAL_OPS(
         DVZ_VISUAL_TYPE_MARKER, "marker", _scene_marker_visual_lowering,
         _scene_visual_default_bounds, _scene_marker_visual_bind_desc,
         _scene_marker_visual_pipeline_desc, _scene_marker_visual_shader_desc,
         _scene_marker_visual_draw_desc),
     .init_state = _scene_visual_init_point_style, .upload_material_params = true},
    {VISUAL_OPS(
         DVZ_VISUAL_TYPE_SEGMENT, "segment", _scene_segment_visual_lowering,
         _scene_segment_visual_bounds, _scene_segment_visual_bind_desc,
         _scene_segment_visual_pipeline_desc, _scene_segment_visual_shader_desc,
         _scene_segment_visual_draw_desc),
     .init_state = _scene_segment_visual_init_state,
     .after_attr_set = _scene_stroke_visual_after_attr_set},
    {VISUAL_OPS(
         DVZ_VISUAL_TYPE_PATH, "path", _scene_path_visual_lowering,
         _scene_visual_default_bounds, _scene_path_visual_bind_desc,
         _scene_path_visual_pipeline_desc, _scene_path_visual_shader_desc,
         _scene_path_visual_draw_desc),
     .init_state = _scene_path_visual_init_state,
     .after_attr_set = _scene_stroke_visual_after_attr_set,
     .upload_position_topology = true},
    {VISUAL_OPS(
         DVZ_VISUAL_TYPE_IMAGE, "image", _scene_image_visual_lowering,
         _scene_image_visual_bounds, _scene_image_visual_bind_desc,
         _scene_image_visual_pipeline_desc, _scene_image_visual_shader_desc,
         _scene_image_visual_draw_desc),
     .fill_metadata = _scene_image_visual_fill_metadata},
    {VISUAL_OPS(
         DVZ_VISUAL_TYPE_MESH, "mesh", _scene_mesh_visual_lowering, _scene_mesh_visual_bounds,
         _scene_mesh_visual_bind_desc, _scene_mesh_visual_pipeline_desc,
         _scene_mesh_visual_shader_desc, _scene_mesh_visual_draw_desc),
     .after_attr_set = _scene_mesh_visual_after_attr_set, .upload_position_topology = true,
     .upload_material_params = true},
    {VISUAL_OPS(
         DVZ_VISUAL_TYPE_VOLUME, "volume", _scene_volume_visual_lowering,
         _scene_volume_visual_bounds, _scene_volume_visual_bind_desc,
         _scene_volume_visual_pipeline_desc, _scene_volume_visual_shader_desc,
         _scene_volume_visual_draw_desc),
     .init_state = _scene_volume_visual_init_state,
     .fill_metadata = _scene_volume_visual_fill_metadata},
    {VISUAL_OPS(
         DVZ_VISUAL_TYPE_PRIMITIVE, "primitive", _scene_primitive_visual_lowering,
         _scene_visual_default_bounds, _scene_primitive_visual_bind_desc,
         _scene_primitive_visual_pipeline_desc, _scene_primitive_visual_shader_desc,
         _scene_primitive_visual_draw_desc),
     .upload_position_topology = true, .upload_material_params = true},
    {VISUAL_OPS(
         DVZ_VISUAL_TYPE_SPHERE, "sphere", _scene_sphere_visual_lowering,
         _scene_sphere_visual_bounds, _scene_sphere_visual_bind_desc,
         _scene_sphere_visual_pipeline_desc, _scene_sphere_visual_shader_desc,
         _scene_sphere_visual_draw_desc),
     .upload_position_topology = true, .upload_material_params = true},
    {VISUAL_OPS(
         DVZ_VISUAL_TYPE_GLYPH, "glyph", _scene_glyph_visual_lowering,
         _scene_glyph_visual_bounds, _scene_glyph_visual_bind_desc,
         _scene_glyph_visual_pipeline_desc, _scene_glyph_visual_shader_desc,
         _scene_glyph_visual_draw_desc),
     .upload_position_topology = true},
    {VISUAL_OPS(
         DVZ_VISUAL_TYPE_TEXT, "text", _scene_text_visual_lowering,
         _scene_visual_default_bounds, _scene_text_visual_bind_desc,
         _scene_text_visual_pipeline_desc, _scene_text_visual_shader_desc,
         _scene_text_visual_draw_desc),
     .reset_state = _scene_text_visual_reset_state},
    {VISUAL_OPS(
         DVZ_VISUAL_TYPE_LABELS, "labels", _scene_labels_visual_lowering,
         _scene_visual_default_bounds, _scene_labels_visual_bind_desc,
         _scene_labels_visual_pipeline_desc, _scene_labels_visual_shader_desc,
         _scene_labels_visual_draw_desc),
     .init_state = _scene_labels_visual_init_state,
     .fill_metadata = _scene_labels_visual_fill_metadata},
    {VISUAL_OPS(
         DVZ_VISUAL_TYPE_SPLAT, "splat", _scene_splat_visual_lowering,
         _scene_visual_default_bounds, _scene_splat_visual_bind_desc,
         _scene_splat_visual_pipeline_desc, _scene_splat_visual_shader_desc,
         _scene_splat_visual_draw_desc),
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
