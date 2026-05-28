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
#include "_visual_pipeline_internal.h"
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
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Resolve generic pass capabilities from retained visual and family lowering facts.
 *
 * @param visual the retained visual
 * @param attach the panel attachment
 * @param lowering resolved visual lowering facts
 * @param out output pass capabilities
 * @return whether capabilities were resolved
 */
static bool _resolve_pass_caps(
    const DvzVisual* visual, const DvzPanelAttach* attach, const DvzVisualLowering* lowering,
    DvzSceneVisualPassCaps* out)
{
    ANN(visual);
    ANN(attach);
    ANN(lowering);
    ANN(out);

    DvzSceneVisualDescKind kind = lowering->desc_kind;
    bool has_normals = false;
    if (_scene_visual_desc_is_primitive(kind) || kind == DVZ_SCENE_VISUAL_DESC_TEXTURED_MESH)
    {
        int normal_idx = _attr_index(visual, "normal");
        has_normals = normal_idx >= 0 && visual->attrs[normal_idx].data != NULL &&
                      visual->attrs[normal_idx].item_count > 0;
    }

    bool point_like = kind == DVZ_SCENE_VISUAL_DESC_POINT || kind == DVZ_SCENE_VISUAL_DESC_PIXEL ||
                      kind == DVZ_SCENE_VISUAL_DESC_MARKER;
    bool stroke = lowering->renderable_kind == DVZ_RENDERABLE_STROKE_QUAD ||
                  lowering->renderable_kind == DVZ_RENDERABLE_PATH_STROKE;
    bool has_material_resource =
        has_normals || stroke || _scene_visual_desc_is_sphere(kind) ||
        (point_like && lowering->needs_material_params);
    _scene_visual_pass_caps_resolve(
        kind, visual->alpha_mode, attach->controller_mode, has_normals, has_material_resource,
        visual->material.depth_cue_enabled, visual->depth_test_enabled, out);
    return true;
}


/**
 * Resolve bind-group role metadata for one visual descriptor.
 *
 * @param visual the visual descriptor
 * @param controller_mode the visual's panel controller attachment mode
 * @param out the output bind descriptor
 * @return whether a bind descriptor was resolved
 */
static bool _resolve_bind_desc(
    const DvzSceneVisualDesc* visual, DvzControllerMode controller_mode,
    DvzSceneVisualBindDesc* out)
{
    ANN(visual);
    ANN(out);
    dvz_memset(out, sizeof(DvzSceneVisualBindDesc), 0, sizeof(DvzSceneVisualBindDesc));
    out->uses_scene_occlusion_set2 = visual->scene_occluded;
    out->scene_occlusion = visual->scene_occlusion;
    out->controller_mode = controller_mode;

    DvzSceneVisualPassCaps caps = {0};
    if (!_scene_visual_pass_caps_from_desc(visual, DVZ_ALPHA_OPAQUE, controller_mode, &caps))
        return false;

    switch (visual->kind)
    {
    case DVZ_SCENE_VISUAL_DESC_SPLAT:
        out->uses_common_set0 = caps.uses_common_set;
        out->uses_fixed_common = caps.fixed_controller;
        return true;

    case DVZ_SCENE_VISUAL_DESC_PIXEL:
    case DVZ_SCENE_VISUAL_DESC_POINT:
    case DVZ_SCENE_VISUAL_DESC_MARKER:
    case DVZ_SCENE_VISUAL_DESC_SPHERE:
        out->uses_common_set0 = caps.uses_common_set;
        out->uses_fixed_common = caps.fixed_controller;
        out->uses_material_set1 = caps.uses_material_set;
        out->material_buffer_id = visual->material_buffer_id;
        return true;

    case DVZ_SCENE_VISUAL_DESC_PRIMITIVE:
        out->uses_common_set0 = caps.uses_common_set;
        out->uses_fixed_common = caps.fixed_controller;
        out->uses_material_set1 = caps.uses_material_set;
        out->material_buffer_id = visual->material_buffer_id;
        return true;

    case DVZ_SCENE_VISUAL_DESC_TEXTURED_MESH:
        out->uses_common_set0 = caps.uses_common_set;
        out->uses_fixed_common = caps.fixed_controller;
        out->uses_image_set1 = caps.uses_image_set;
        out->uses_textured_mesh_set1 = caps.uses_image_set;
        out->image_texture_id = visual->image_texture_id;
        out->image_nearest_sampler = visual->image_nearest_sampler;
        out->material_buffer_id = visual->material_buffer_id;
        return true;

    case DVZ_SCENE_VISUAL_DESC_SEGMENT:
    case DVZ_SCENE_VISUAL_DESC_PATH:
        out->uses_common_set0 = caps.uses_common_set;
        out->uses_fixed_common = caps.fixed_controller;
        out->uses_material_set1 = caps.uses_material_set;
        out->material_buffer_id = visual->material_buffer_id;
        return true;

    case DVZ_SCENE_VISUAL_DESC_IMAGE:
        out->uses_common_set0 = caps.uses_common_set;
        out->uses_fixed_common = caps.fixed_controller;
        out->uses_image_set1 = caps.uses_image_set;
        out->image_texture_id = visual->image_texture_id;
        out->image_nearest_sampler = visual->image_nearest_sampler;
        return true;

    case DVZ_SCENE_VISUAL_DESC_LABELS_SINT:
    case DVZ_SCENE_VISUAL_DESC_LABELS_UINT:
        out->uses_common_set0 = caps.uses_common_set;
        out->uses_fixed_common = caps.fixed_controller;
        out->uses_labels_set1 = caps.uses_image_set;
        out->labels_texture_id = visual->image_texture_id;
        out->labels_visual_index = visual->labels_visual_index;
        out->labels_state = visual->labels_state;
        return true;

    case DVZ_SCENE_VISUAL_DESC_GLYPH:
        out->uses_common_set0 = caps.uses_common_set;
        out->uses_fixed_common = caps.fixed_controller;
        out->uses_glyph_set1 = caps.uses_image_set;
        out->glyph_texture_id = visual->image_texture_id;
        out->glyph_atlas_encoding = visual->glyph_atlas_encoding;
        out->glyph_distance_range_px =
            visual->glyph_distance_range_px > 0.0f ? visual->glyph_distance_range_px : 4.0f;
        return true;

    case DVZ_SCENE_VISUAL_DESC_VOLUME:
    case DVZ_SCENE_VISUAL_DESC_VOLUME_LABELS_SINT:
    case DVZ_SCENE_VISUAL_DESC_VOLUME_LABELS_UINT:
        out->uses_common_set0 = caps.uses_common_set;
        out->uses_fixed_common = caps.fixed_controller;
        out->uses_volume_set1 = caps.uses_volume_set;
        out->volume_texture_id = visual->volume_texture_id;
        out->volume_transfer_texture_id = visual->volume_transfer_texture_id;
        out->volume_label_lookup_buffer_id = visual->volume_label_lookup_buffer_id;
        out->volume_label_lookup_buffer_size = visual->volume_label_lookup_buffer_size;
        out->volume_visual_index = visual->volume_visual_index;
        out->volume_transfer_rgba = visual->volume_transfer_rgba;
        out->volume_occluded = visual->volume_occluded;
        out->volume_occlusion = visual->volume_occlusion;
        out->volume_state = visual->volume_state;
        if (
            visual->kind == DVZ_SCENE_VISUAL_DESC_VOLUME_LABELS_SINT ||
            visual->kind == DVZ_SCENE_VISUAL_DESC_VOLUME_LABELS_UINT)
            out->volume_state.sampling = DVZ_VOLUME_SAMPLING_NEAREST;
        return true;

    case DVZ_SCENE_VISUAL_DESC_NONE:
    default:
        return false;
    }
}



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

static const DvzVisualFamilyOps VISUAL_FAMILY_OPS[] = {
    {DVZ_VISUAL_TYPE_POINT, "point", _scene_point_visual_lowering, _resolve_pass_caps,
     _resolve_bind_desc, _scene_visual_pipeline_desc_resolve, _scene_visual_shader_desc_resolve,
     _scene_visual_draw_desc_resolve, NULL},
    {DVZ_VISUAL_TYPE_PIXEL, "pixel", _scene_pixel_visual_lowering, _resolve_pass_caps,
     _resolve_bind_desc, _scene_visual_pipeline_desc_resolve, _scene_visual_shader_desc_resolve,
     _scene_visual_draw_desc_resolve, NULL},
    {DVZ_VISUAL_TYPE_MARKER, "marker", _scene_marker_visual_lowering, _resolve_pass_caps,
     _resolve_bind_desc, _scene_visual_pipeline_desc_resolve, _scene_visual_shader_desc_resolve,
     _scene_visual_draw_desc_resolve, NULL},
    {DVZ_VISUAL_TYPE_SEGMENT, "segment", _scene_segment_visual_lowering, _resolve_pass_caps,
     _resolve_bind_desc, _scene_visual_pipeline_desc_resolve, _scene_visual_shader_desc_resolve,
     _scene_visual_draw_desc_resolve, NULL},
    {DVZ_VISUAL_TYPE_PATH, "path", _scene_path_visual_lowering, _resolve_pass_caps,
     _resolve_bind_desc, _scene_visual_pipeline_desc_resolve, _scene_visual_shader_desc_resolve,
     _scene_visual_draw_desc_resolve, NULL},
    {DVZ_VISUAL_TYPE_IMAGE, "image", _scene_image_visual_lowering, _resolve_pass_caps,
     _resolve_bind_desc, _scene_visual_pipeline_desc_resolve, _scene_visual_shader_desc_resolve,
     _scene_visual_draw_desc_resolve, _scene_image_visual_fill_metadata},
    {DVZ_VISUAL_TYPE_MESH, "mesh", _scene_mesh_visual_lowering, _resolve_pass_caps,
     _resolve_bind_desc, _scene_visual_pipeline_desc_resolve, _scene_visual_shader_desc_resolve,
     _scene_visual_draw_desc_resolve, NULL},
    {DVZ_VISUAL_TYPE_VOLUME, "volume", _scene_volume_visual_lowering, _resolve_pass_caps,
     _resolve_bind_desc, _scene_visual_pipeline_desc_resolve, _scene_visual_shader_desc_resolve,
     _scene_visual_draw_desc_resolve, _scene_volume_visual_fill_metadata},
    {DVZ_VISUAL_TYPE_PRIMITIVE, "primitive", _scene_primitive_visual_lowering, _resolve_pass_caps,
     _resolve_bind_desc, _scene_visual_pipeline_desc_resolve, _scene_visual_shader_desc_resolve,
     _scene_visual_draw_desc_resolve, NULL},
    {DVZ_VISUAL_TYPE_SPHERE, "sphere", _scene_sphere_visual_lowering, _resolve_pass_caps,
     _resolve_bind_desc, _scene_visual_pipeline_desc_resolve, _scene_visual_shader_desc_resolve,
     _scene_visual_draw_desc_resolve, NULL},
    {DVZ_VISUAL_TYPE_GLYPH, "glyph", _scene_glyph_visual_lowering, _resolve_pass_caps,
     _resolve_bind_desc, _scene_visual_pipeline_desc_resolve, _scene_visual_shader_desc_resolve,
     _scene_visual_draw_desc_resolve, NULL},
    {DVZ_VISUAL_TYPE_TEXT, "text", _scene_text_visual_lowering, _resolve_pass_caps,
     _resolve_bind_desc, _scene_visual_pipeline_desc_resolve, _scene_visual_shader_desc_resolve,
     _scene_visual_draw_desc_resolve, NULL},
    {DVZ_VISUAL_TYPE_LABELS, "labels", _scene_labels_visual_lowering, _resolve_pass_caps,
     _resolve_bind_desc, _scene_visual_pipeline_desc_resolve, _scene_visual_shader_desc_resolve,
     _scene_visual_draw_desc_resolve, _scene_labels_visual_fill_metadata},
    {DVZ_VISUAL_TYPE_SPLAT, "splat", _scene_splat_visual_lowering, _resolve_pass_caps,
     _resolve_bind_desc, _scene_visual_pipeline_desc_resolve, _scene_visual_shader_desc_resolve,
     _scene_visual_draw_desc_resolve, NULL},
    {DVZ_VISUAL_TYPE_VECTOR, "vector", _scene_vector_visual_lowering, _resolve_pass_caps,
     _resolve_bind_desc, _scene_visual_pipeline_desc_resolve, _scene_visual_shader_desc_resolve,
     _scene_visual_draw_desc_resolve, NULL},
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
