/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene visual frame-plan metadata                                                             */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#include <vulkan/vulkan_core.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_overflow.h"
#include "_scene.h"
#include "scene_emit/scene_emit.h"
#include "scene_emit/internal.h"
#include "_scene_resource_key.h"
#include "_scene_shader_abi.h"
#include "_technique.h"
#include "_visual_pipeline.h"
#include "_visual_pipeline_internal.h"
#include "_visual_internal.h"
#include "annotation/scale_internal.h"
#include "domain/buffer_internal.h"
#include "scene_emit/visual_lowering.h"
#include "datoviz/drp2/runtime.h"
#include "render_contract/render_contract.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Build typed FramePlan metadata for one retained visual.
 *
 * @param figure the parent figure
 * @param visual the retained visual
 * @param visual_index the visual index within the figure
 * @param metadata the output metadata
 * @return whether metadata was built
 */
bool _scene_visual_frame_plan_metadata(
    const DvzFigure* figure, const DvzVisual* visual, uint32_t visual_index,
    DvzFramePlanVisualMeta* metadata)
{
    ANN(figure);
    ANN(figure->scene);
    ANN(visual);
    ANN(metadata);

    dvz_memset(metadata, sizeof(DvzFramePlanVisualMeta), 0, sizeof(DvzFramePlanVisualMeta));
    metadata->has_metadata = true;
    metadata->visual_type = (uint32_t)visual->type;
    DvzVisualLowering lowering = {0};
    if (!_scene_visual_lowering_resolve(visual, &lowering))
        return false;
    DvzRenderableKind renderable_kind = lowering.renderable_kind;
    metadata->renderable_kind = (uint32_t)renderable_kind;
    metadata->desc_kind = (uint32_t)lowering.desc_kind;
    metadata->point_like_kind = lowering.has_point_like_kind ? (uint32_t)lowering.point_like_kind
                                                            : UINT32_MAX;
    metadata->visual_index = visual_index;
    metadata->buffer_index = UINT32_MAX;
    metadata->topology = (uint32_t)_visual_family_state(visual)->topology;
    metadata->instance_count = 1;
    metadata->has_item_range = visual->has_item_range;
    metadata->item_range_first = visual->item_range_first;
    metadata->item_range_count = visual->item_range_count;
    metadata->alpha_mode = visual->alpha_mode;
    metadata->depth_test_enabled = visual->depth_test_enabled;
    metadata->depth_compare_op = visual->depth_compare_op;
    metadata->depth_cue_enabled = visual->material.depth_cue_enabled;
    metadata->point_style_enabled = lowering.point_style_enabled;
    metadata->glyph_atlas_encoding = (uint32_t)_visual_family_state(visual)->glyph_atlas_encoding;
    metadata->glyph_distance_range_px = _visual_family_state(visual)->glyph_distance_range_px;
    metadata->scale_index = _scene_scale_index(figure->scene, _visual_family_state(visual)->scale);
    if (_visual_family_state(visual)->field != NULL)
        metadata->image_color_role = _visual_family_state(visual)->field->desc.color_role;
    metadata->scene_occluder = visual->scene_occluder;
    metadata->scene_occluded = visual->scene_occluded;
    if (_visual_family_state(visual)->field != NULL)
    {
        metadata->field_format = (uint32_t)_visual_family_state(visual)->field->desc.format;
        metadata->field_semantic = (uint32_t)_visual_family_state(visual)->field->desc.semantic;
        metadata->field_width = _visual_family_state(visual)->field->desc.width;
        metadata->field_height = _visual_family_state(visual)->field->desc.height;
        metadata->field_depth = _visual_family_state(visual)->field->desc.depth;
    }

    const char* vertex_count_attr =
        renderable_kind == DVZ_RENDERABLE_STROKE_QUAD ? "position_start" : "position";
    int vertex_count_idx = _attr_index(visual, vertex_count_attr);
    if (vertex_count_idx >= 0)
    {
        if (visual->attrs[vertex_count_idx].item_count > UINT32_MAX)
            return false;
        metadata->vertex_count = (uint32_t)visual->attrs[vertex_count_idx].item_count;
    }

    if (!_scene_attr_resource_key(
            figure, visual, visual_index, "position", metadata->position_id,
            sizeof(metadata->position_id)))
        return false;
    if (!_scene_attr_resource_key(
            figure, visual, visual_index, "position_start", metadata->position_start_id,
            sizeof(metadata->position_start_id)))
        return false;
    if (!_scene_attr_resource_key(
            figure, visual, visual_index, "position_end", metadata->position_end_id,
            sizeof(metadata->position_end_id)))
        return false;
    if (!_scene_attr_resource_key(
            figure, visual, visual_index, "position_next", metadata->position_next_id,
            sizeof(metadata->position_next_id)))
        return false;
    if (!_scene_attr_resource_key(
            figure, visual, visual_index, "color", metadata->color_id, sizeof(metadata->color_id)))
        return false;
    if (!_scene_attr_resource_key(
            figure, visual, visual_index, "size", metadata->size_id, sizeof(metadata->size_id)))
        return false;
    if (!_scene_attr_resource_key(
            figure, visual, visual_index, "sigma", metadata->sigma_id,
            sizeof(metadata->sigma_id)))
        return false;
    if (!_scene_attr_resource_key(
            figure, visual, visual_index, "angle", metadata->angle_id, sizeof(metadata->angle_id)))
        return false;
    if (!_scene_attr_resource_key(
            figure, visual, visual_index, "bounds", metadata->bounds_id,
            sizeof(metadata->bounds_id)))
        return false;
    if (!_scene_attr_resource_key(
            figure, visual, visual_index, "shape", metadata->shape_id, sizeof(metadata->shape_id)))
        return false;
    if (!_scene_attr_resource_key(
            figure, visual, visual_index, "line_width", metadata->line_width_id,
            sizeof(metadata->line_width_id)))
        return false;
    if (!_scene_attr_resource_key(
            figure, visual, visual_index, "tex_rect", metadata->tex_rect_id,
            sizeof(metadata->tex_rect_id)))
        return false;
    if (!_scene_attr_resource_key(
            figure, visual, visual_index, "texcoords", metadata->texcoords_id,
            sizeof(metadata->texcoords_id)))
        return false;
    if (!_scene_attr_resource_key(
            figure, visual, visual_index, "instance_transform", metadata->instance_transform_id,
            sizeof(metadata->instance_transform_id)))
        return false;
    if (!_scene_visual_texture_resource_key(
            figure, visual, visual_index, metadata->texture_id, sizeof(metadata->texture_id)))
        return false;
    metadata->image_nearest_sampler = _visual_family_state(visual)->image_nearest_sampler;
    if (!_scene_visual_lowering_fill_metadata(visual, metadata))
        return false;
    if (_scene_visual_desc_is_volume(lowering.desc_kind))
    {
        dvz_strlcpy(
            metadata->volume_texture_id, metadata->texture_id,
            sizeof(metadata->volume_texture_id));
        if (!_scene_resource_key_volume_transfer(
                visual_index, metadata->volume_transfer_texture_id,
                sizeof(metadata->volume_transfer_texture_id)))
            return false;
        if (!_scene_resource_key_volume_label_lookup(
                visual_index, metadata->volume_label_lookup_id,
                sizeof(metadata->volume_label_lookup_id)))
            return false;
    }
    if (!_scene_attr_resource_key(
            figure, visual, visual_index, "normal", metadata->normal_id,
            sizeof(metadata->normal_id)))
        return false;
    if (!_scene_attr_resource_key(
            figure, visual, visual_index, "item_state", metadata->selection_id,
            sizeof(metadata->selection_id)))
        return false;
    if (_scene_visual_has_attr_data(visual, "item_state"))
    {
        if (!_scene_visual_attr_resource_key(
                figure, visual, visual_index, "item_state_style", metadata->item_state_style_id,
                sizeof(metadata->item_state_style_id)))
            return false;
    }
    bool path_stroke = renderable_kind == DVZ_RENDERABLE_PATH_STROKE;
    bool stroke_quad = renderable_kind == DVZ_RENDERABLE_STROKE_QUAD;
    bool stroke = path_stroke || stroke_quad;
    if (path_stroke)
    {
        if (!_scene_visual_attr_resource_key(
                figure, visual, visual_index, "path_flags", metadata->path_flags_id,
                sizeof(metadata->path_flags_id)))
            return false;
        if (!_scene_visual_attr_resource_key(
                figure, visual, visual_index, "path_distance", metadata->path_distance_id,
                sizeof(metadata->path_distance_id)))
            return false;
        if (!_scene_visual_attr_resource_key(
                figure, visual, visual_index, "position_next", metadata->position_next_id,
                sizeof(metadata->position_next_id)))
            return false;
    }
    if (stroke || _scene_visual_needs_material_params(visual))
    {
        if (!_scene_visual_attr_resource_key(
                figure, visual, visual_index, "material_params", metadata->material_id,
                sizeof(metadata->material_id)))
            return false;
    }

    uint32_t buffer_index = _scene_buffer_index(figure->scene, _visual_family_state(visual)->buffer);
    if (buffer_index != UINT32_MAX)
    {
        metadata->buffer_index = buffer_index;
        if (_visual_family_state(visual)->buffer->desc.stride > 0)
        {
            uint64_t index_count = _visual_family_state(visual)->buffer->desc.byte_size /
                                   _visual_family_state(visual)->buffer->desc.stride;
            if (index_count > UINT32_MAX)
                return false;
            metadata->index_count = (uint32_t)index_count;
        }
        if (!_scene_resource_key_buffer(
                _visual_family_state(visual)->buffer->id, metadata->index_id,
                sizeof(metadata->index_id)))
            return false;
    }
    if (stroke)
    {
        if (!_scene_visual_attr_resource_key(
                figure, visual, visual_index, "index", metadata->index_id,
                sizeof(metadata->index_id)))
            return false;
        const DvzPathStrokeGpuCache* path_cache = lowering.path_stroke_cache;
        const DvzStrokeQuadGpuCache* stroke_cache = lowering.stroke_quad_cache;
        uint64_t vertex_count = path_stroke && path_cache != NULL ? path_cache->vertex_count
                                : stroke_cache != NULL            ? stroke_cache->vertex_count
                                                                  : 0;
        uint64_t index_count = path_stroke && path_cache != NULL ? path_cache->index_count
                               : stroke_cache != NULL            ? stroke_cache->index_count
                                                                 : 0;
        if (vertex_count > UINT32_MAX || index_count > UINT32_MAX)
            return false;
        metadata->vertex_count = (uint32_t)vertex_count;
        metadata->index_count = (uint32_t)index_count;
    }
    return true;
}
