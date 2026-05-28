/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene visual descriptor lowering                                                             */
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


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve draw-relevant state from typed FramePlan visual metadata.
 *
 * @param emitter the persistent emitter
 * @param meta the typed visual metadata
 * @param out the output visual descriptor
 * @return whether a supported visual descriptor was resolved
 */
static bool _scene_visual_desc_from_metadata(
    DvzFramePlanEmitter* emitter, const DvzFramePlanVisualMeta* meta, DvzSceneVisualDesc* out,
    const char** error)
{
    ANN(emitter);
    ANN(meta);
    ANN(out);

    out->depth_test_enabled = meta->depth_test_enabled;
    out->depth_compare_op =
        meta->depth_compare_op != 0 ? meta->depth_compare_op : VK_COMPARE_OP_LESS_OR_EQUAL;
    out->depth_cue_enabled = meta->depth_cue_enabled;
    out->point_style_enabled = meta->point_style_enabled;
    out->scene_occluded = meta->scene_occluded;
    out->scene_occlusion = meta->scene_occlusion;
    out->instance_count = 1;

    if (error != NULL)
        *error = NULL;

    bool stroked_path = _scene_visual_meta_is_stroked_path(&emitter->resources, meta);
    DvzRenderableKind renderable_kind =
        _scene_visual_meta_renderable_kind(&emitter->resources, meta);
    DvzSceneVisualDescKind desc_kind = _scene_visual_meta_desc_kind(&emitter->resources, meta);
    bool segment_like = renderable_kind == DVZ_RENDERABLE_STROKE_QUAD;
    bool stroke_like = segment_like || stroked_path;
    const char* primary_position_id = stroke_like ? meta->position_start_id : meta->position_id;
    uint64_t pos_buf =
        _scene_visual_resource_lookup_label(&emitter->resources, primary_position_id);
    if (pos_buf == 0)
    {
        if (error != NULL)
            *error = stroke_like ? "typed stroke metadata missing position_start resource"
                                 : "typed visual metadata missing position resource";
        return false;
    }
    out->vbuf_ids[out->vbuf_count++] = pos_buf;

    uint64_t pos_size = _resource_byte_size(&emitter->resources, pos_buf);
    uint64_t vertex_count = (pos_size > 0) ? pos_size / (3 * sizeof(float)) : 3;
    if (vertex_count > UINT32_MAX)
    {
        if (error != NULL)
            *error = "typed visual metadata vertex count exceeds uint32";
        return false;
    }
    out->vertex_count = (uint32_t)vertex_count;
    if (meta->vertex_count > 0)
        out->vertex_count = meta->vertex_count;

    bool point_like_desc = desc_kind == DVZ_SCENE_VISUAL_DESC_POINT ||
                           desc_kind == DVZ_SCENE_VISUAL_DESC_PIXEL ||
                           desc_kind == DVZ_SCENE_VISUAL_DESC_MARKER;
    if (point_like_desc)
    {
        DvzScenePointLikeKind point_like_kind = DVZ_SCENE_POINT_LIKE_POINT;
        if (meta->point_like_kind != UINT32_MAX)
            point_like_kind = (DvzScenePointLikeKind)meta->point_like_kind;
        else if (desc_kind == DVZ_SCENE_VISUAL_DESC_PIXEL)
            point_like_kind = DVZ_SCENE_POINT_LIKE_PIXEL;
        else if (desc_kind == DVZ_SCENE_VISUAL_DESC_MARKER)
            point_like_kind = DVZ_SCENE_POINT_LIKE_MARKER;
        uint64_t color_id =
            _scene_visual_resource_lookup_label(&emitter->resources, meta->color_id);
        uint64_t size_id = _scene_visual_resource_lookup_label(&emitter->resources, meta->size_id);
        uint64_t angle_id =
            _scene_visual_resource_lookup_label(&emitter->resources, meta->angle_id);
        uint64_t shape_id =
            _scene_visual_resource_lookup_label(&emitter->resources, meta->shape_id);
        uint64_t selection_id =
            _scene_visual_resource_lookup_label(&emitter->resources, meta->selection_id);
        if (color_id == 0 || size_id == 0)
        {
            if (error != NULL)
            {
                *error = point_like_kind == DVZ_SCENE_POINT_LIKE_POINT
                             ? "typed point metadata missing color/size resource"
                             : "typed point-like metadata missing color/size resource";
            }
            return false;
        }
        if (point_like_kind == DVZ_SCENE_POINT_LIKE_MARKER && (angle_id == 0 || shape_id == 0))
        {
            if (error != NULL)
                *error = "typed marker metadata missing angle/shape resource";
            return false;
        }
        out->kind = desc_kind;
        out->point_like_kind = point_like_kind;
        out->vbuf_ids[out->vbuf_count++] = color_id;
        out->vbuf_ids[out->vbuf_count++] = size_id;
        if (point_like_kind == DVZ_SCENE_POINT_LIKE_MARKER)
        {
            out->vbuf_ids[out->vbuf_count++] = angle_id;
            out->vbuf_ids[out->vbuf_count++] = shape_id;
        }
        if (selection_id != 0)
        {
            out->vbuf_ids[out->vbuf_count++] = selection_id;
            out->has_selection_mask = true;
        }
        out->topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        out->material_buffer_id =
            _scene_visual_resource_lookup_label(&emitter->resources, meta->material_id);
        return true;
    }

    if (desc_kind == DVZ_SCENE_VISUAL_DESC_SPLAT)
    {
        uint64_t color_id =
            _scene_visual_resource_lookup_label(&emitter->resources, meta->color_id);
        uint64_t sigma_id =
            _scene_visual_resource_lookup_label(&emitter->resources, meta->sigma_id);
        uint64_t angle_id =
            _scene_visual_resource_lookup_label(&emitter->resources, meta->angle_id);
        if (color_id == 0 || sigma_id == 0 || angle_id == 0)
        {
            if (error != NULL)
                *error = "typed splat metadata missing color/sigma/angle resource";
            return false;
        }
        uint32_t item_count = out->vertex_count;
        out->kind = DVZ_SCENE_VISUAL_DESC_SPLAT;
        out->vbuf_ids[out->vbuf_count++] = color_id;
        out->vbuf_ids[out->vbuf_count++] = sigma_id;
        out->vbuf_ids[out->vbuf_count++] = angle_id;
        out->topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        out->vertex_count = 6;
        out->instance_count = item_count;
        return true;
    }

    if (stroked_path)
    {
        uint64_t curr_id =
            _scene_visual_resource_lookup_label(&emitter->resources, meta->position_id);
        uint64_t next_id =
            _scene_visual_resource_lookup_label(&emitter->resources, meta->position_end_id);
        uint64_t color_id =
            _scene_visual_resource_lookup_label(&emitter->resources, meta->color_id);
        uint64_t line_width_id =
            _scene_visual_resource_lookup_label(&emitter->resources, meta->line_width_id);
        uint64_t flags_id =
            _scene_visual_resource_lookup_label(&emitter->resources, meta->path_flags_id);
        uint64_t distance_id =
            _scene_visual_resource_lookup_label(&emitter->resources, meta->path_distance_id);
        uint64_t index_id =
            _scene_visual_resource_lookup_label(&emitter->resources, meta->index_id);
        uint64_t material_id =
            _scene_visual_resource_lookup_label(&emitter->resources, meta->material_id);
        if (curr_id == 0 || next_id == 0 || color_id == 0 || line_width_id == 0 ||
            flags_id == 0 || distance_id == 0 || index_id == 0 || material_id == 0)
        {
            if (error != NULL)
                *error = "typed stroked path metadata missing adjacency/color/width/flags/"
                         "distance/index resource";
            return false;
        }
        out->kind = DVZ_SCENE_VISUAL_DESC_PATH;
        out->vbuf_ids[out->vbuf_count++] = curr_id;
        out->vbuf_ids[out->vbuf_count++] = next_id;
        out->vbuf_ids[out->vbuf_count++] = color_id;
        out->vbuf_ids[out->vbuf_count++] = line_width_id;
        out->vbuf_ids[out->vbuf_count++] = flags_id;
        out->vbuf_ids[out->vbuf_count++] = distance_id;
        out->topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        out->index_buffer_id = index_id;
        out->material_buffer_id = material_id;
        if (meta->vertex_count > 0)
            out->vertex_count = meta->vertex_count;
        if (_resource_item_stride(&emitter->resources, index_id) != 0)
        {
            uint64_t index_count = _resource_byte_size(&emitter->resources, index_id) /
                                   _resource_item_stride(&emitter->resources, index_id);
            if (index_count > UINT32_MAX)
            {
                if (error != NULL)
                    *error = "typed path index count exceeds uint32";
                return false;
            }
            out->index_count = (uint32_t)index_count;
        }
        if (meta->index_count > 0)
            out->index_count = meta->index_count;
        out->index_format =
            _resource_item_stride(&emitter->resources, index_id) == sizeof(uint16_t) ? "uint16"
                                                                                     : "uint32";
        return true;
    }

    if (segment_like)
    {
        uint64_t end_id =
            _scene_visual_resource_lookup_label(&emitter->resources, meta->position_end_id);
        uint64_t color_id =
            _scene_visual_resource_lookup_label(&emitter->resources, meta->color_id);
        uint64_t line_width_id =
            _scene_visual_resource_lookup_label(&emitter->resources, meta->line_width_id);
        uint64_t index_id =
            _scene_visual_resource_lookup_label(&emitter->resources, meta->index_id);
        uint64_t material_id =
            _scene_visual_resource_lookup_label(&emitter->resources, meta->material_id);
        if (end_id == 0 || color_id == 0 || line_width_id == 0 || index_id == 0 ||
            material_id == 0)
        {
            if (error != NULL)
                *error = "typed segment metadata missing endpoint/color/width/index/cap resource";
            return false;
        }
        out->kind = DVZ_SCENE_VISUAL_DESC_SEGMENT;
        out->vbuf_ids[out->vbuf_count++] = end_id;
        out->vbuf_ids[out->vbuf_count++] = color_id;
        out->vbuf_ids[out->vbuf_count++] = line_width_id;
        out->topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        out->index_buffer_id = index_id;
        out->material_buffer_id = material_id;
        if (meta->vertex_count > 0)
            out->vertex_count = meta->vertex_count;
        if (_resource_item_stride(&emitter->resources, index_id) != 0)
        {
            uint64_t index_count = _resource_byte_size(&emitter->resources, index_id) /
                                   _resource_item_stride(&emitter->resources, index_id);
            if (index_count > UINT32_MAX)
            {
                if (error != NULL)
                    *error = "typed segment index count exceeds uint32";
                return false;
            }
            out->index_count = (uint32_t)index_count;
        }
        if (meta->index_count > 0)
            out->index_count = meta->index_count;
        out->index_format =
            _resource_item_stride(&emitter->resources, index_id) == sizeof(uint16_t) ? "uint16"
                                                                                     : "uint32";
        return true;
    }

    if (desc_kind == DVZ_SCENE_VISUAL_DESC_SPHERE)
    {
        uint64_t color_id =
            _scene_visual_resource_lookup_label(&emitter->resources, meta->color_id);
        uint64_t size_id = _scene_visual_resource_lookup_label(&emitter->resources, meta->size_id);
        if (color_id == 0 || size_id == 0)
        {
            if (error != NULL)
                *error = "typed sphere metadata missing color/size resource";
            return false;
        }
        out->kind = DVZ_SCENE_VISUAL_DESC_SPHERE;
        out->vbuf_ids[out->vbuf_count++] = color_id;
        out->vbuf_ids[out->vbuf_count++] = size_id;
        out->topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        out->material_buffer_id =
            _scene_visual_resource_lookup_label(&emitter->resources, meta->material_id);
        return true;
    }

    if (desc_kind == DVZ_SCENE_VISUAL_DESC_PRIMITIVE ||
        desc_kind == DVZ_SCENE_VISUAL_DESC_TEXTURED_MESH)
    {
        uint64_t color_id =
            _scene_visual_resource_lookup_label(&emitter->resources, meta->color_id);
        if (color_id == 0)
        {
            if (error != NULL)
                *error = "typed primitive metadata missing color resource";
            return false;
        }
        out->kind = DVZ_SCENE_VISUAL_DESC_PRIMITIVE;
        out->vbuf_ids[out->vbuf_count++] = color_id;
        uint64_t normal_id =
            _scene_visual_resource_lookup_label(&emitter->resources, meta->normal_id);
        if (normal_id != 0)
        {
            out->vbuf_ids[out->vbuf_count++] = normal_id;
            out->has_normal = true;
        }
        uint64_t texture_id = desc_kind == DVZ_SCENE_VISUAL_DESC_TEXTURED_MESH
                                  ? _scene_visual_resource_lookup_label(
                                        &emitter->resources, meta->texture_id)
                                  : 0;
        if (texture_id != 0)
        {
            uint64_t texcoords_id =
                _scene_visual_resource_lookup_label(&emitter->resources, meta->texcoords_id);
            if (normal_id == 0 || texcoords_id == 0)
            {
                if (error != NULL)
                    *error = "typed textured mesh metadata missing normal/texcoords resource";
                return false;
            }
            out->kind = DVZ_SCENE_VISUAL_DESC_TEXTURED_MESH;
            out->vbuf_ids[out->vbuf_count++] = texcoords_id;
            out->image_texture_id = texture_id;
        }
        else
        {
            out->kind = DVZ_SCENE_VISUAL_DESC_PRIMITIVE;
        }
        uint64_t instance_transform_id =
            _scene_visual_resource_lookup_label(&emitter->resources, meta->instance_transform_id);
        if (instance_transform_id != 0)
        {
            if (out->kind == DVZ_SCENE_VISUAL_DESC_TEXTURED_MESH)
            {
                if (error != NULL)
                    *error = "typed textured mesh instancing is not supported";
                return false;
            }
            out->vbuf_ids[out->vbuf_count++] = instance_transform_id;
            out->has_instance_transform = true;
            uint64_t transform_bytes =
                _resource_byte_size(&emitter->resources, instance_transform_id);
            uint64_t instance_count = transform_bytes / (16 * sizeof(float));
            if (instance_count == 0 || instance_count > UINT32_MAX)
            {
                if (error != NULL)
                    *error = "typed mesh instance_transform count is invalid";
                return false;
            }
            out->instance_count = (uint32_t)instance_count;
        }
        out->topology = _resource_topology(&emitter->resources, pos_buf);
        if (out->topology == UINT32_MAX)
            out->topology = meta->topology;
        if (out->topology == UINT32_MAX)
        {
            if (error != NULL)
                *error = "typed primitive metadata missing topology resource";
            return false;
        }
        out->index_buffer_id =
            _scene_visual_resource_lookup_label(&emitter->resources, meta->index_id);
        out->material_buffer_id =
            _scene_visual_resource_lookup_label(&emitter->resources, meta->material_id);
    }
    else if (_scene_visual_desc_is_image(desc_kind))
    {
        uint64_t uv_id =
            _scene_visual_resource_lookup_label(&emitter->resources, meta->texcoords_id);
        uint64_t tex_id =
            _scene_visual_resource_lookup_label(&emitter->resources, meta->texture_id);
        if (uv_id == 0 || tex_id == 0)
        {
            if (error != NULL)
                *error = "typed image metadata missing texcoords/texture resource";
            return false;
        }
        out->kind = desc_kind;
        if (desc_kind == DVZ_SCENE_VISUAL_DESC_LABELS_SINT ||
            desc_kind == DVZ_SCENE_VISUAL_DESC_LABELS_UINT)
        {
            out->labels_visual_index = meta->visual_index;
            out->labels_state = meta->labels_state;
        }
        else if (desc_kind == DVZ_SCENE_VISUAL_DESC_IMAGE)
        {
            out->image_pixel_space = meta->image_pixel_space;
            out->image_nearest_sampler = meta->image_nearest_sampler;
        }
        if (desc_kind == DVZ_SCENE_VISUAL_DESC_GLYPH)
        {
            uint64_t bounds_id =
                _scene_visual_resource_lookup_label(&emitter->resources, meta->bounds_id);
            uint64_t color_id =
                _scene_visual_resource_lookup_label(&emitter->resources, meta->color_id);
            uint64_t angle_id =
                _scene_visual_resource_lookup_label(&emitter->resources, meta->angle_id);
            if (bounds_id == 0 || color_id == 0 || angle_id == 0)
            {
                if (error != NULL)
                    *error = "typed glyph metadata missing bounds/color/angle resource";
                return false;
            }
            out->vbuf_ids[out->vbuf_count++] = bounds_id;
            out->vbuf_ids[out->vbuf_count++] = uv_id;
            out->vbuf_ids[out->vbuf_count++] = color_id;
            out->vbuf_ids[out->vbuf_count++] = angle_id;
        }
        else
        {
            out->vbuf_ids[out->vbuf_count++] = uv_id;
        }
        out->image_texture_id = tex_id;
        if (desc_kind == DVZ_SCENE_VISUAL_DESC_GLYPH)
        {
            out->glyph_atlas_encoding = meta->glyph_atlas_encoding;
            out->glyph_distance_range_px =
                meta->glyph_distance_range_px > 0.0f ? meta->glyph_distance_range_px : 4.0f;
        }
        out->topology = _resource_topology(&emitter->resources, pos_buf);
        if (out->topology == UINT32_MAX)
            out->topology = desc_kind == DVZ_SCENE_VISUAL_DESC_GLYPH
                                ? VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
                                : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    }
    else if (_scene_visual_desc_is_volume(desc_kind))
    {
        uint64_t uvw_id =
            _scene_visual_resource_lookup_label(&emitter->resources, meta->texcoords_id);
        uint64_t tex_id =
            _scene_visual_resource_lookup_label(&emitter->resources, meta->volume_texture_id);
        uint64_t transfer_tex_id = _scene_visual_resource_lookup_label(
            &emitter->resources, meta->volume_transfer_texture_id);
        uint64_t label_lookup_id = _scene_visual_resource_lookup_label(
            &emitter->resources, meta->volume_label_lookup_id);
        if (tex_id == 0)
            tex_id = _scene_visual_resource_lookup_label(&emitter->resources, meta->texture_id);
        if (uvw_id == 0 || tex_id == 0)
        {
            if (error != NULL)
                *error = "typed volume metadata missing texcoords/texture resource";
            return false;
        }
        if (!meta->volume_transfer_rgba && transfer_tex_id == 0)
        {
            if (error != NULL)
                *error = "typed scalar volume metadata missing transfer texture resource";
            return false;
        }
        if (desc_kind == DVZ_SCENE_VISUAL_DESC_VOLUME_LABELS_SINT)
        {
            if (
                meta->volume_state.render_mode != DVZ_VOLUME_RENDER_SLICE &&
                meta->volume_state.render_mode != DVZ_VOLUME_RENDER_COMPOSITE)
            {
                if (error != NULL)
                    *error = "label volumes only support slice and composite render modes";
                return false;
            }
            out->kind = DVZ_SCENE_VISUAL_DESC_VOLUME_LABELS_SINT;
        }
        else if (desc_kind == DVZ_SCENE_VISUAL_DESC_VOLUME_LABELS_UINT)
        {
            if (
                meta->volume_state.render_mode != DVZ_VOLUME_RENDER_SLICE &&
                meta->volume_state.render_mode != DVZ_VOLUME_RENDER_COMPOSITE)
            {
                if (error != NULL)
                    *error = "label volumes only support slice and composite render modes";
                return false;
            }
            out->kind = DVZ_SCENE_VISUAL_DESC_VOLUME_LABELS_UINT;
        }
        else
            out->kind = DVZ_SCENE_VISUAL_DESC_VOLUME;
        out->vbuf_ids[out->vbuf_count++] = uvw_id;
        out->volume_texture_id = tex_id;
        out->volume_transfer_texture_id = transfer_tex_id;
        out->volume_label_lookup_buffer_id = label_lookup_id;
        out->volume_label_lookup_buffer_size =
            _resource_byte_size(&emitter->resources, label_lookup_id);
        out->volume_visual_index = meta->visual_index;
        out->volume_transfer_rgba = meta->volume_transfer_rgba;
        out->volume_occluded = meta->volume_occluded;
        out->volume_occlusion = meta->volume_occlusion;
        out->volume_state = meta->volume_state;
        out->topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    }
    else
    {
        if (error != NULL)
            *error = "unsupported typed visual metadata";
        return false;
    }

    if (out->index_buffer_id != 0 &&
        _resource_item_stride(&emitter->resources, out->index_buffer_id) != 0)
    {
        uint64_t index_count = _resource_byte_size(&emitter->resources, out->index_buffer_id) /
                               _resource_item_stride(&emitter->resources, out->index_buffer_id);
        if (index_count > UINT32_MAX)
        {
            if (error != NULL)
                *error = "typed index metadata count exceeds uint32";
            return false;
        }
        out->index_count = (uint32_t)index_count;
    }
    out->index_format =
        _resource_item_stride(&emitter->resources, out->index_buffer_id) == sizeof(uint16_t)
            ? "uint16"
            : "uint32";

    return true;
}



/**
 * Resolve draw-relevant state for one encoded render visual id.
 *
 * @param emitter the persistent emitter
 * @param render the render node
 * @param visual_index the visual index within the render node
 * @param out the output visual descriptor
 * @return whether a supported visual descriptor was resolved
 */
bool _scene_visual_desc_from_render(
    DvzFramePlanEmitter* emitter, const DvzFramePlanNode* render, uint32_t visual_index,
    DvzSceneVisualDesc* out, const char** error)
{
    ANN(emitter);
    ANN(render);
    ANN(out);
    if (error != NULL)
        *error = NULL;
    dvz_memset(out, sizeof(DvzSceneVisualDesc), 0, sizeof(DvzSceneVisualDesc));
    if (render->type != DVZ_FRAME_PLAN_NODE_RENDER ||
        visual_index >= render->u.render.visual_count)
        return false;

    out->depth_test_enabled = true;
    out->depth_compare_op = VK_COMPARE_OP_LESS_OR_EQUAL;
    out->instance_count = 1;
    const DvzFramePlanVisualMeta* meta = &render->u.render.visual_metadata[visual_index];
    if (meta->has_metadata)
        return _scene_visual_desc_from_metadata(emitter, meta, out, error);

    uint64_t pos_buf = _scene_render_visual_resource_id(
        emitter, render->u.render.visuals[visual_index], DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION);
    if (pos_buf == 0)
        return false;
    out->vbuf_ids[out->vbuf_count++] = pos_buf;

    const DvzFramePlanResourceRole optionals[] = {
        DVZ_FRAME_PLAN_RESOURCE_ROLE_COLOR,           DVZ_FRAME_PLAN_RESOURCE_ROLE_SIZE,
        DVZ_FRAME_PLAN_RESOURCE_ROLE_SIGMA,           DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXCOORDS,
        DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXTURE,         DVZ_FRAME_PLAN_RESOURCE_ROLE_NORMAL,
        DVZ_FRAME_PLAN_RESOURCE_ROLE_INDEX,           DVZ_FRAME_PLAN_RESOURCE_ROLE_MATERIAL_PARAMS,
        DVZ_FRAME_PLAN_RESOURCE_ROLE_SELECTION};
    for (uint32_t ai = 0; ai < 9; ai++)
    {
        uint64_t rid_id = _scene_render_visual_resource_id(
            emitter, render->u.render.visuals[visual_index], optionals[ai]);
        if (rid_id == 0)
            continue;
        if (optionals[ai] == DVZ_FRAME_PLAN_RESOURCE_ROLE_INDEX)
        {
            out->index_buffer_id = rid_id;
            continue;
        }
        if (optionals[ai] == DVZ_FRAME_PLAN_RESOURCE_ROLE_MATERIAL_PARAMS)
        {
            out->material_buffer_id = rid_id;
            continue;
        }
        if (optionals[ai] == DVZ_FRAME_PLAN_RESOURCE_ROLE_SELECTION)
            out->has_selection_mask = true;
        if (out->vbuf_count >= DVZ_SCENE_MAX_NODE_RESOURCES)
            return false;
        out->vbuf_ids[out->vbuf_count++] = rid_id;
    }

    bool is_point = _is_point_visual(&emitter->resources, out->vbuf_ids, out->vbuf_count);
    bool is_splat =
        !is_point && _is_splat_visual(&emitter->resources, out->vbuf_ids, out->vbuf_count);
    uint64_t mesh_pos = 0, mesh_color = 0, mesh_normal = 0, mesh_uv = 0, mesh_tex = 0;
    bool is_textured_mesh =
        !is_point && !is_splat &&
        _is_textured_mesh_visual(
            &emitter->resources, out->vbuf_ids, out->vbuf_count, &mesh_pos, &mesh_color,
            &mesh_normal, &mesh_uv, &mesh_tex);
    bool is_primitive =
        !is_point && !is_splat && !is_textured_mesh &&
        _is_primitive_visual(&emitter->resources, out->vbuf_ids, out->vbuf_count);
    uint64_t img_pos = 0, img_uv = 0, img_tex = 0;
    bool is_image =
        !is_point && !is_splat && !is_primitive &&
        _is_image_visual(
            &emitter->resources, out->vbuf_ids, out->vbuf_count, &img_pos, &img_uv, &img_tex);

    if (!is_point && !is_splat && !is_textured_mesh && !is_primitive && !is_image)
        return false;

    uint64_t pos_size = _resource_byte_size(&emitter->resources, pos_buf);
    uint64_t vertex_count = (pos_size > 0) ? pos_size / (3 * sizeof(float)) : 3;
    if (vertex_count > UINT32_MAX)
        return false;
    out->vertex_count = (uint32_t)vertex_count;

    for (uint32_t j = 0; j < out->vbuf_count; j++)
    {
        out->has_normal = out->has_normal || _scene_visual_resource_by_role(
                                                 &emitter->resources, &out->vbuf_ids[j], 1,
                                                 DVZ_FRAME_PLAN_RESOURCE_ROLE_NORMAL) != 0;
    }

    out->topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    if (is_primitive || is_textured_mesh)
        out->topology = _resource_topology(&emitter->resources, pos_buf);
    else if (is_image)
        out->topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

    if (is_point)
    {
        out->kind = DVZ_SCENE_VISUAL_DESC_POINT;
        out->point_like_kind = DVZ_SCENE_POINT_LIKE_POINT;
    }
    else if (is_splat)
    {
        uint64_t color_id = _scene_visual_resource_by_role(
            &emitter->resources, out->vbuf_ids, out->vbuf_count,
            DVZ_FRAME_PLAN_RESOURCE_ROLE_COLOR);
        uint64_t sigma_id = _scene_visual_resource_by_role(
            &emitter->resources, out->vbuf_ids, out->vbuf_count,
            DVZ_FRAME_PLAN_RESOURCE_ROLE_SIGMA);
        uint64_t angle_id = _scene_visual_resource_by_role(
            &emitter->resources, out->vbuf_ids, out->vbuf_count,
            DVZ_FRAME_PLAN_RESOURCE_ROLE_ANGLE);
        if (color_id == 0 || sigma_id == 0 || angle_id == 0)
            return false;
        out->kind = DVZ_SCENE_VISUAL_DESC_SPLAT;
        out->vbuf_ids[0] = pos_buf;
        out->vbuf_ids[1] = color_id;
        out->vbuf_ids[2] = sigma_id;
        out->vbuf_ids[3] = angle_id;
        out->vbuf_count = 4;
        out->topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        out->instance_count = out->vertex_count;
        out->vertex_count = 6;
    }
    else if (is_textured_mesh)
    {
        out->kind = DVZ_SCENE_VISUAL_DESC_TEXTURED_MESH;
        out->image_texture_id = mesh_tex;
        out->vbuf_ids[0] = mesh_pos;
        out->vbuf_ids[1] = mesh_color;
        out->vbuf_ids[2] = mesh_normal;
        out->vbuf_ids[3] = mesh_uv;
        out->vbuf_count = 4;
        out->has_normal = true;
    }
    else if (is_primitive)
        out->kind = DVZ_SCENE_VISUAL_DESC_PRIMITIVE;
    else
    {
        out->kind = DVZ_SCENE_VISUAL_DESC_IMAGE;
        out->image_texture_id = img_tex;
        out->vbuf_ids[0] = img_pos;
        out->vbuf_ids[1] = img_uv;
        out->vbuf_count = 2;
    }

    if (out->index_buffer_id != 0 &&
        _resource_item_stride(&emitter->resources, out->index_buffer_id) != 0)
    {
        uint64_t index_count = _resource_byte_size(&emitter->resources, out->index_buffer_id) /
                               _resource_item_stride(&emitter->resources, out->index_buffer_id);
        if (index_count > UINT32_MAX)
            return false;
        out->index_count = (uint32_t)index_count;
    }
    out->index_format =
        _resource_item_stride(&emitter->resources, out->index_buffer_id) == sizeof(uint16_t)
            ? "uint16"
            : "uint32";

    return true;
}
