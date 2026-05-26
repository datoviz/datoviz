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
#include "_scene_emit.h"
#include "_scene_emit_internal.h"
#include "_scene_resource_key.h"
#include "_scene_shader_abi.h"
#include "_technique.h"
#include "_visual_pipeline.h"
#include "datoviz/drp2/runtime.h"
#include "render_contract.h"


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
    metadata->visual_index = visual_index;
    metadata->buffer_index = UINT32_MAX;
    metadata->topology = (uint32_t)visual->topology;
    metadata->instance_count = 1;
    metadata->alpha_mode = visual->alpha_mode;
    metadata->depth_test_enabled = visual->depth_test_enabled;
    metadata->depth_compare_op = visual->depth_compare_op;
    metadata->depth_cue_enabled = visual->material.depth_cue_enabled;
    metadata->point_style_enabled =
        visual->type == DVZ_VISUAL_TYPE_POINT && visual->material.point_style_enabled;
    metadata->glyph_atlas_encoding = (uint32_t)visual->glyph_atlas_encoding;
    metadata->glyph_distance_range_px = visual->glyph_distance_range_px;
    metadata->scale_index = _scene_scale_index(figure->scene, visual->scale);
    metadata->scene_occluder = visual->scene_occluder;
    metadata->scene_occluded = visual->scene_occluded;
    if (visual->field != NULL)
    {
        metadata->field_format = (uint32_t)visual->field->desc.format;
        metadata->field_width = visual->field->desc.width;
        metadata->field_height = visual->field->desc.height;
        metadata->field_depth = visual->field->desc.depth;
    }

    const char* vertex_count_attr =
        visual->type == DVZ_VISUAL_TYPE_SEGMENT ? "position_start" : "position";
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
            figure, visual, visual_index, "color", metadata->color_id, sizeof(metadata->color_id)))
        return false;
    if (!_scene_attr_resource_key(
            figure, visual, visual_index, "size", metadata->size_id, sizeof(metadata->size_id)))
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
    if (visual->type == DVZ_VISUAL_TYPE_VOLUME)
    {
        metadata->has_volume = true;
        metadata->volume_state = visual->volume;
        metadata->volume_occluded = visual->volume_occluded;
        metadata->volume_transfer_rgba =
            visual->field != NULL && visual->field->desc.format == DVZ_FIELD_FORMAT_RGBA8_UNORM;
        if (visual->field != NULL)
        {
            metadata->field_format = (uint32_t)visual->field->desc.format;
            metadata->field_width = visual->field->desc.width;
            metadata->field_height = visual->field->desc.height;
            metadata->field_depth = visual->field->desc.depth;
        }
        dvz_strlcpy(
            metadata->volume_texture_id, metadata->texture_id,
            sizeof(metadata->volume_texture_id));
        if (!_scene_resource_key_volume_transfer(
                visual_index, metadata->volume_transfer_texture_id,
                sizeof(metadata->volume_transfer_texture_id)))
            return false;
    }
    if (!_scene_attr_resource_key(
            figure, visual, visual_index, "normal", metadata->normal_id,
            sizeof(metadata->normal_id)))
        return false;
    if (!_scene_attr_resource_key(
            figure, visual, visual_index, "selection", metadata->selection_id,
            sizeof(metadata->selection_id)))
        return false;
    if (visual->type == DVZ_VISUAL_TYPE_PATH &&
        _scene_visual_has_attr_data(visual, "line_width"))
    {
        if (!_scene_visual_attr_resource_key(
                figure, visual, visual_index, "path_flags", metadata->path_flags_id,
                sizeof(metadata->path_flags_id)))
            return false;
        if (!_scene_visual_attr_resource_key(
                figure, visual, visual_index, "path_distance", metadata->path_distance_id,
                sizeof(metadata->path_distance_id)))
            return false;
    }
    if (visual->type == DVZ_VISUAL_TYPE_SEGMENT || _scene_visual_needs_material_params(visual))
    {
        if (!_scene_visual_attr_resource_key(
                figure, visual, visual_index, "material_params", metadata->material_id,
                sizeof(metadata->material_id)))
            return false;
    }

    uint32_t buffer_index = _scene_buffer_index(figure->scene, visual->buffer);
    if (buffer_index != UINT32_MAX)
    {
        metadata->buffer_index = buffer_index;
        if (!_scene_resource_key_buffer(
                buffer_index, metadata->index_id, sizeof(metadata->index_id)))
            return false;
    }
    if (visual->type == DVZ_VISUAL_TYPE_SEGMENT ||
        (visual->type == DVZ_VISUAL_TYPE_PATH &&
         _scene_visual_has_attr_data(visual, "line_width")))
    {
        if (!_scene_visual_attr_resource_key(
                figure, visual, visual_index, "index", metadata->index_id,
                sizeof(metadata->index_id)))
            return false;
        if (visual->type == DVZ_VISUAL_TYPE_SEGMENT)
        {
            if (visual->segment.gpu.vertex_count > UINT32_MAX ||
                visual->segment.gpu.index_count > UINT32_MAX)
                return false;
            metadata->vertex_count = (uint32_t)visual->segment.gpu.vertex_count;
            metadata->index_count = (uint32_t)visual->segment.gpu.index_count;
        }
        else if (visual->type == DVZ_VISUAL_TYPE_PATH)
        {
            if (visual->path.gpu.vertex_count > UINT32_MAX ||
                visual->path.gpu.index_count > UINT32_MAX)
                return false;
            metadata->vertex_count = (uint32_t)visual->path.gpu.vertex_count;
            metadata->index_count = (uint32_t)visual->path.gpu.index_count;
        }
    }
    if ((visual->type == DVZ_VISUAL_TYPE_IMAGE || visual->type == DVZ_VISUAL_TYPE_LABELS) &&
        _scene_image_uses_generated_quads(visual))
    {
        if (visual->image_gpu.vertex_count > UINT32_MAX)
            return false;
        if (visual->image_gpu.vertex_count > 0)
            metadata->vertex_count = (uint32_t)visual->image_gpu.vertex_count;
    }
    return true;
}
