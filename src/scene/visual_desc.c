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
#include "sample_profile.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve a resource id from a non-empty FramePlan resource key.
 *
 * @param state the resource state
 * @param key the resource key
 * @return the resource id, or zero when absent
 */
uint64_t _scene_visual_resource_lookup_label(const ConverterState* state, const char* key)
{
    ANN(state);
    if (key == NULL || key[0] == '\0')
        return 0;
    return _resource_lookup_id(state, key);
}



/**
 * Append a resolved resource key to a small id list.
 *
 * @param state the resource state
 * @param key the resource key
 * @param out_ids the output id list
 * @param out_count the output id count
 * @param required whether absence is an error
 * @return whether the append or optional skip succeeded
 */
static bool _append_resource_key(
    const ConverterState* state, const char* key, uint64_t* out_ids, uint32_t* out_count,
    bool required)
{
    ANN(state);
    ANN(out_ids);
    ANN(out_count);
    uint64_t id = _scene_visual_resource_lookup_label(state, key);
    if (id == 0)
        return !required;
    if (*out_count >= DVZ_SCENE_MAX_NODE_RESOURCES)
        return false;
    out_ids[(*out_count)++] = id;
    return true;
}



/**
 * Return whether a retained visual type uses the primitive pipeline family.
 *
 * @param visual_type the retained visual type
 * @return whether the visual type is primitive-like
 */
bool _scene_visual_meta_is_primitive(uint32_t visual_type)
{
    return visual_type == DVZ_VISUAL_TYPE_PRIMITIVE || visual_type == DVZ_VISUAL_TYPE_MESH ||
           visual_type == DVZ_VISUAL_TYPE_PATH;
}


/**
 * Return whether typed path metadata has derived stroke resources.
 *
 * @param state the resource state
 * @param meta the typed visual metadata
 * @return whether the path should use the segment stroke pipeline
 */
bool _scene_visual_meta_is_stroked_path(
    const ConverterState* state, const DvzFramePlanVisualMeta* meta)
{
    ANN(state);
    ANN(meta);
    if (meta->visual_type != DVZ_VISUAL_TYPE_PATH)
        return false;
    return _scene_visual_resource_lookup_label(state, meta->position_start_id) != 0 &&
           _scene_visual_resource_lookup_label(state, meta->position_id) != 0 &&
           _scene_visual_resource_lookup_label(state, meta->position_end_id) != 0 &&
           _scene_visual_resource_lookup_label(state, meta->line_width_id) != 0 &&
           _scene_visual_resource_lookup_label(state, meta->path_flags_id) != 0 &&
           _scene_visual_resource_lookup_label(state, meta->path_distance_id) != 0 &&
           _scene_visual_resource_lookup_label(state, meta->index_id) != 0;
}


/**
 * Return whether one retained visual has dense data for an attribute.
 *
 * @param visual the retained visual
 * @param name the attribute name
 * @return whether dense data exists
 */
bool _scene_visual_has_dense_attr(const DvzVisual* visual, const char* name)
{
    ANN(visual);
    ANN(name);
    int attr_idx = _attr_index(visual, name);
    return attr_idx >= 0 && visual->attrs[attr_idx].data != NULL &&
           visual->attrs[attr_idx].item_count > 0;
}


/**
 * Return whether a visual descriptor uses the primitive pipeline family.
 *
 * @param kind the visual descriptor kind
 * @return whether the descriptor is primitive-like
 */
bool _scene_visual_desc_is_primitive(DvzSceneVisualDescKind kind)
{
    return kind == DVZ_SCENE_VISUAL_DESC_PRIMITIVE;
}



/**
 * Return whether a visual descriptor uses a sampled-image bind group.
 *
 * @param kind the visual descriptor kind
 * @return whether the descriptor is image-like
 */
bool _scene_visual_desc_is_image(DvzSceneVisualDescKind kind)
{
    return kind == DVZ_SCENE_VISUAL_DESC_IMAGE || kind == DVZ_SCENE_VISUAL_DESC_LABELS_SINT ||
           kind == DVZ_SCENE_VISUAL_DESC_LABELS_UINT || kind == DVZ_SCENE_VISUAL_DESC_GLYPH;
}


/**
 * Return whether a visual descriptor uses a sampled-volume bind group.
 *
 * @param kind the visual descriptor kind
 * @return whether the descriptor is volume-like
 */
bool _scene_visual_desc_is_volume(DvzSceneVisualDescKind kind)
{
    return kind == DVZ_SCENE_VISUAL_DESC_VOLUME ||
           kind == DVZ_SCENE_VISUAL_DESC_VOLUME_LABELS_SINT ||
           kind == DVZ_SCENE_VISUAL_DESC_VOLUME_LABELS_UINT;
}


/**
 * Return whether a visual descriptor uses the sphere impostor pipeline family.
 *
 * @param kind the visual descriptor kind
 * @return whether the descriptor is sphere-like
 */
bool _scene_visual_desc_is_sphere(DvzSceneVisualDescKind kind)
{
    return kind == DVZ_SCENE_VISUAL_DESC_SPHERE;
}


/**
 * Return whether a visual descriptor uses the segment analytic stroke pipeline family.
 *
 * @param kind the visual descriptor kind
 * @return whether the descriptor is segment-like
 */
bool _scene_visual_desc_is_segment(DvzSceneVisualDescKind kind)
{
    return kind == DVZ_SCENE_VISUAL_DESC_SEGMENT;
}


/**
 * Return whether a visual descriptor uses the path analytic stroke pipeline family.
 *
 * @param kind the visual descriptor kind
 * @return whether the descriptor is path-like
 */
bool _scene_visual_desc_is_path(DvzSceneVisualDescKind kind)
{
    return kind == DVZ_SCENE_VISUAL_DESC_PATH;
}


/**
 * Return whether a visual descriptor uses an analytic stroke pipeline family.
 *
 * @param kind the visual descriptor kind
 * @return whether the descriptor is stroke-like
 */
bool _scene_visual_desc_is_stroke(DvzSceneVisualDescKind kind)
{
    return _scene_visual_desc_is_segment(kind) || _scene_visual_desc_is_path(kind);
}



/**
 * Return the point-like family represented by a retained visual type.
 *
 * @param visual_type the retained visual type
 * @param out the output point-like family
 * @return whether the visual type is point-like
 */
bool _scene_visual_meta_point_like_kind(uint32_t visual_type, DvzScenePointLikeKind* out)
{
    ANN(out);
    switch (visual_type)
    {
    case DVZ_VISUAL_TYPE_POINT:
        *out = DVZ_SCENE_POINT_LIKE_POINT;
        return true;
    case DVZ_VISUAL_TYPE_PIXEL:
        *out = DVZ_SCENE_POINT_LIKE_PIXEL;
        return true;
    case DVZ_VISUAL_TYPE_MARKER:
        *out = DVZ_SCENE_POINT_LIKE_MARKER;
        return true;
    default:
        return false;
    }
}



/**
 * Resolve the backend-specific point-like visual lowering policy.
 *
 * @param kind the point-like visual family
 * @param shader_format the target shader format
 * @param item_count number of logical items in the visual
 * @param out the output lowering descriptor
 * @return whether the lowering descriptor was resolved
 */
bool _scene_point_like_lowering_desc(
    DvzScenePointLikeKind kind, DvzSceneShaderFormat shader_format, uint32_t item_count,
    DvzScenePointLikeLoweringDesc* out)
{
    ANN(out);
    dvz_memset(
        out, sizeof(DvzScenePointLikeLoweringDesc), 0, sizeof(DvzScenePointLikeLoweringDesc));
    out->kind = kind;

    if (shader_format == DVZ_SCENE_SHADER_FORMAT_WGSL)
    {
        out->lowering = DVZ_SCENE_POINT_LIKE_LOWERING_INSTANCED_QUADS;
        out->topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        out->vertex_step_mode = DVZ_DRP2_VERTEX_STEP_MODE_INSTANCE;
        out->draw_vertex_count = 6;
        out->draw_instance_count = item_count;
        return true;
    }

    out->lowering = DVZ_SCENE_POINT_LIKE_LOWERING_NATIVE_POINTS;
    out->topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    out->vertex_step_mode = DVZ_DRP2_VERTEX_STEP_MODE_VERTEX;
    out->draw_vertex_count = item_count;
    out->draw_instance_count = 1;
    return true;
}



/**
 * Return the legacy data tag for a typed resource role.
 *
 * @param role the typed resource role
 * @return the legacy data tag, or NULL when the role has no tag fallback
 */
static const char* _resource_role_tag(DvzFramePlanResourceRole role)
{
    switch (role)
    {
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION:
        return "position";
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION_START:
        return "position_start";
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION_END:
        return "position_end";
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_COLOR:
        return "color";
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_SIZE:
        return "size";
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_ANGLE:
        return "angle";
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_SHAPE:
        return "shape";
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_LINE_WIDTH:
        return "line_width";
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXCOORDS:
        return "texcoords";
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXTURE:
        return "texture";
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_NORMAL:
        return "normal";
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_INDEX:
        return "index";
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_PRIMITIVE_SHADING:
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_MATERIAL_PARAMS:
        return "material_params";
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_SELECTION:
        return "selection";
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_PATH_FLAGS:
        return "path_flags";
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_PATH_DISTANCE:
        return "path_distance";
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_NONE:
    default:
        return NULL;
    }
}



/**
 * Resolve the legacy resource key for one encoded render visual and role.
 *
 * @param encoded_visual_id the render-node visual debug id
 * @param role the typed resource role
 * @param out_key the output resource key
 * @param out_size the output key buffer size
 * @return whether the key was resolved
 */
static bool _render_visual_resource_key(
    const char* encoded_visual_id, DvzFramePlanResourceRole role, char* out_key, uint64_t out_size)
{
    ANN(encoded_visual_id);
    ANN(out_key);
    out_key[0] = '\0';

    char visual_id[DVZ_SCENE_LABEL_SIZE];
    char shared_index_id[DVZ_SCENE_LABEL_SIZE];
    _scene_resource_key_split_visual(
        encoded_visual_id, visual_id, sizeof(visual_id), shared_index_id, sizeof(shared_index_id));
    if (role == DVZ_FRAME_PLAN_RESOURCE_ROLE_INDEX && shared_index_id[0] != '\0')
    {
        dvz_strlcpy(out_key, shared_index_id, (size_t)out_size);
        return out_key[0] != '\0';
    }

    const char* tag = _resource_role_tag(role);
    if (tag == NULL)
        return false;
    return _scene_resource_key_visual_data(visual_id, tag, out_key, out_size);
}



/**
 * Resolve one legacy render-visual resource id by role.
 *
 * @param emitter the persistent emitter
 * @param encoded_visual_id the render-node visual debug id
 * @param role the typed resource role
 * @return the resource id, or zero when absent
 */
uint64_t _scene_render_visual_resource_id(
    const DvzFramePlanEmitter* emitter, const char* encoded_visual_id,
    DvzFramePlanResourceRole role)
{
    ANN(emitter);
    char key[DVZ_SCENE_LABEL_SIZE];
    if (!_render_visual_resource_key(encoded_visual_id, role, key, sizeof(key)))
        return 0;
    return _resource_lookup_id(&emitter->resources, key);
}



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
    bool segment_like = meta->visual_type == DVZ_VISUAL_TYPE_SEGMENT;
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

    DvzScenePointLikeKind point_like_kind = DVZ_SCENE_POINT_LIKE_POINT;
    if (_scene_visual_meta_point_like_kind(meta->visual_type, &point_like_kind))
    {
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
        out->kind = point_like_kind == DVZ_SCENE_POINT_LIKE_PIXEL    ? DVZ_SCENE_VISUAL_DESC_PIXEL
                    : point_like_kind == DVZ_SCENE_POINT_LIKE_MARKER ? DVZ_SCENE_VISUAL_DESC_MARKER
                                                                     : DVZ_SCENE_VISUAL_DESC_POINT;
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

    if (meta->visual_type == DVZ_VISUAL_TYPE_SPHERE)
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

    if (_scene_visual_meta_is_primitive(meta->visual_type))
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
        uint64_t instance_transform_id =
            _scene_visual_resource_lookup_label(&emitter->resources, meta->instance_transform_id);
        if (meta->visual_type == DVZ_VISUAL_TYPE_MESH && instance_transform_id != 0)
        {
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
    else if (
        meta->visual_type == DVZ_VISUAL_TYPE_IMAGE || meta->visual_type == DVZ_VISUAL_TYPE_GLYPH ||
        meta->visual_type == DVZ_VISUAL_TYPE_LABELS)
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
        if (meta->visual_type == DVZ_VISUAL_TYPE_GLYPH)
            out->kind = DVZ_SCENE_VISUAL_DESC_GLYPH;
        else if (meta->visual_type == DVZ_VISUAL_TYPE_LABELS)
        {
            DvzSceneSampleProfile profile = {0};
            bool ok = _scene_sample_profile_resolve(
                (DvzFieldFormat)meta->field_format, DVZ_FIELD_SEMANTIC_LABEL, DVZ_FIELD_DIM_2D,
                &profile);
            if (ok && _scene_sample_profile_is_signed_label(&profile))
                out->kind = DVZ_SCENE_VISUAL_DESC_LABELS_SINT;
            else if (ok && _scene_sample_profile_is_unsigned_label(&profile))
                out->kind = DVZ_SCENE_VISUAL_DESC_LABELS_UINT;
            else
            {
                if (error != NULL)
                    *error = "labels metadata has unsupported integer texture format";
                return false;
            }
            out->labels_visual_index = meta->visual_index;
            out->labels_state = meta->labels_state;
        }
        else
        {
            out->kind = DVZ_SCENE_VISUAL_DESC_IMAGE;
            out->image_pixel_space = meta->image_pixel_space;
        }
        if (meta->visual_type == DVZ_VISUAL_TYPE_GLYPH)
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
        if (meta->visual_type == DVZ_VISUAL_TYPE_GLYPH)
        {
            out->glyph_atlas_encoding = meta->glyph_atlas_encoding;
            out->glyph_distance_range_px =
                meta->glyph_distance_range_px > 0.0f ? meta->glyph_distance_range_px : 4.0f;
        }
        out->topology = _resource_topology(&emitter->resources, pos_buf);
        if (out->topology == UINT32_MAX)
            out->topology = meta->visual_type == DVZ_VISUAL_TYPE_GLYPH
                                ? VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
                                : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    }
    else if (meta->visual_type == DVZ_VISUAL_TYPE_VOLUME)
    {
        uint64_t uvw_id =
            _scene_visual_resource_lookup_label(&emitter->resources, meta->texcoords_id);
        uint64_t tex_id =
            _scene_visual_resource_lookup_label(&emitter->resources, meta->volume_texture_id);
        uint64_t transfer_tex_id = _scene_visual_resource_lookup_label(
            &emitter->resources, meta->volume_transfer_texture_id);
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
        DvzSceneSampleProfile profile = {0};
        bool has_profile =
            _scene_sample_profile_resolve(
                (DvzFieldFormat)meta->field_format, (DvzFieldSemantic)meta->field_semantic,
                DVZ_FIELD_DIM_3D, &profile);
        if (has_profile && _scene_sample_profile_is_signed_label(&profile))
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
        else if (has_profile && _scene_sample_profile_is_unsigned_label(&profile))
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



/*
 * Return true when vertex_buffer_ids[0..n-1] carry data_tags "position", "color", "size"
 * (in any order), which identifies a DvzPoint visual.
 */
bool _is_point_visual(const ConverterState* state, const uint64_t* ids, uint32_t n)
{
    if (n < 3)
        return false;
    bool has_pos = false, has_col = false, has_sz = false;
    for (uint32_t i = 0; i < n; i++)
    {
        DvzFramePlanResourceRole role = _resource_role(state, ids[i]);
        if (role == DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION)
            has_pos = true;
        if (role == DVZ_FRAME_PLAN_RESOURCE_ROLE_COLOR)
            has_col = true;
        if (role == DVZ_FRAME_PLAN_RESOURCE_ROLE_SIZE)
            has_sz = true;
        if (role != DVZ_FRAME_PLAN_RESOURCE_ROLE_NONE)
            continue;

        const char* tag = _resource_data_tag(state, ids[i]);
        if (strcmp(tag, "position") == 0)
            has_pos = true;
        if (strcmp(tag, "color") == 0)
            has_col = true;
        if (strcmp(tag, "size") == 0)
            has_sz = true;
    }
    return has_pos && has_col && has_sz;
}



/*
 * Return true when ids carry "position" + "color" with an optional "normal" attribute and
 * a topology hint on the position resource, identifying a DvzPrimitive visual.
 */
bool _is_primitive_visual(const ConverterState* state, const uint64_t* ids, uint32_t n)
{
    if (n < 2 || n > 3)
        return false;
    bool has_pos = false, has_col = false, has_topo = false, has_normal = false;
    for (uint32_t i = 0; i < n; i++)
    {
        DvzFramePlanResourceRole role = _resource_role(state, ids[i]);
        if (role == DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION)
        {
            has_pos = true;
            if (_resource_topology(state, ids[i]) != UINT32_MAX)
                has_topo = true;
            continue;
        }
        if (role == DVZ_FRAME_PLAN_RESOURCE_ROLE_COLOR)
        {
            has_col = true;
            continue;
        }
        if (role == DVZ_FRAME_PLAN_RESOURCE_ROLE_NORMAL)
        {
            has_normal = true;
            continue;
        }
        if (role != DVZ_FRAME_PLAN_RESOURCE_ROLE_NONE)
            continue;

        const char* tag = _resource_data_tag(state, ids[i]);
        if (strcmp(tag, "position") == 0)
        {
            has_pos = true;
            if (_resource_topology(state, ids[i]) != UINT32_MAX)
                has_topo = true;
        }
        if (strcmp(tag, "color") == 0)
            has_col = true;
        if (strcmp(tag, "normal") == 0)
            has_normal = true;
    }
    return has_pos && has_col && has_topo && (n == 2 || has_normal);
}



/*
 * Return true when ids carry exactly "position" + "texcoords" + "texture", identifying
 * a DvzImage visual. Outputs the position id, texcoords id, and texture id.
 */
bool _is_image_visual(
    const ConverterState* state, const uint64_t* ids, uint32_t n, uint64_t* out_pos,
    uint64_t* out_uv, uint64_t* out_tex)
{
    if (n != 3)
        return false;
    uint64_t pos = 0, uv = 0, tex = 0;
    for (uint32_t i = 0; i < n; i++)
    {
        DvzFramePlanResourceRole role = _resource_role(state, ids[i]);
        if (role == DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION)
        {
            pos = ids[i];
            continue;
        }
        if (role == DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXCOORDS)
        {
            uv = ids[i];
            continue;
        }
        if (role == DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXTURE)
        {
            tex = ids[i];
            continue;
        }
        if (role != DVZ_FRAME_PLAN_RESOURCE_ROLE_NONE)
            continue;

        const char* tag = _resource_data_tag(state, ids[i]);
        if (strcmp(tag, "position") == 0)
            pos = ids[i];
        else if (strcmp(tag, "texcoords") == 0)
            uv = ids[i];
        else if (strcmp(tag, "texture") == 0)
            tex = ids[i];
    }
    if (pos == 0 || uv == 0 || tex == 0)
        return false;
    if (out_pos)
        *out_pos = pos;
    if (out_uv)
        *out_uv = uv;
    if (out_tex)
        *out_tex = tex;
    return true;
}



/**
 * Find the first visual resource with a typed role, falling back to legacy tags.
 *
 * @param state the resource state
 * @param ids the resource ids
 * @param n the resource id count
 * @param role the typed role to find
 * @return the resource id, or zero when absent
 */
uint64_t _scene_visual_resource_by_role(
    const ConverterState* state, const uint64_t* ids, uint32_t n, DvzFramePlanResourceRole role)
{
    ANN(state);
    ANN(ids);

    for (uint32_t i = 0; i < n; i++)
    {
        if (_resource_role(state, ids[i]) == role)
            return ids[i];
    }

    const char* tag = _resource_role_tag(role);
    if (tag == NULL)
        return 0;
    for (uint32_t i = 0; i < n; i++)
    {
        if (_resource_role(state, ids[i]) != DVZ_FRAME_PLAN_RESOURCE_ROLE_NONE)
            continue;
        if (strcmp(_resource_data_tag(state, ids[i]), tag) == 0)
            return ids[i];
    }
    return 0;
}



/**
 * Resolve persistent vertex-buffer ids for a render node with no new uploads.
 *
 * @param emitter the persistent emitter
 * @param render the render node
 * @param out_ids the output vertex buffer ids
 * @param out_count the output vertex buffer count
 * @return whether all ids were resolved
 */
bool _emitter_resolve_render_vertex_buffers(
    DvzFramePlanEmitter* emitter, const DvzFramePlanNode* render, uint64_t* out_ids,
    uint32_t* out_count)
{
    ANN(emitter);
    ANN(render);
    ANN(out_ids);
    ANN(out_count);

    *out_count = 0;
    for (uint32_t i = 0; i < render->u.render.visual_count; i++)
    {
        const DvzFramePlanVisualMeta* meta = &render->u.render.visual_metadata[i];
        if (meta->has_metadata)
        {
            bool segment_like = meta->visual_type == DVZ_VISUAL_TYPE_SEGMENT;
            bool stroked_path = _scene_visual_meta_is_stroked_path(&emitter->resources, meta);
            if (stroked_path)
            {
                if (!_append_resource_key(
                        &emitter->resources, meta->position_start_id, out_ids, out_count, true))
                    return false;
                if (!_append_resource_key(
                        &emitter->resources, meta->position_id, out_ids, out_count, true))
                    return false;
                if (!_append_resource_key(
                        &emitter->resources, meta->position_end_id, out_ids, out_count, true))
                    return false;
            }
            else if (segment_like)
            {
                if (!_append_resource_key(
                        &emitter->resources, meta->position_start_id, out_ids, out_count, true))
                    return false;
                if (!_append_resource_key(
                        &emitter->resources, meta->position_end_id, out_ids, out_count, true))
                    return false;
            }
            else if (!_append_resource_key(
                         &emitter->resources, meta->position_id, out_ids, out_count, true))
                return false;
            if (!_append_resource_key(
                    &emitter->resources, meta->color_id, out_ids, out_count, false))
                return false;
            if (!_append_resource_key(
                    &emitter->resources, meta->size_id, out_ids, out_count, false))
                return false;
            if (!_append_resource_key(
                    &emitter->resources, meta->angle_id, out_ids, out_count, false))
                return false;
            if (!_append_resource_key(
                    &emitter->resources, meta->shape_id, out_ids, out_count, false))
                return false;
            if (!_append_resource_key(
                    &emitter->resources, meta->selection_id, out_ids, out_count, false))
                return false;
            if (!_append_resource_key(
                    &emitter->resources, meta->line_width_id, out_ids, out_count, false))
                return false;
            if (!_append_resource_key(
                    &emitter->resources, meta->path_flags_id, out_ids, out_count, false))
                return false;
            if (!_append_resource_key(
                    &emitter->resources, meta->path_distance_id, out_ids, out_count, false))
                return false;
            if (!_append_resource_key(
                    &emitter->resources, meta->texcoords_id, out_ids, out_count, false))
                return false;
            if (!_append_resource_key(
                    &emitter->resources, meta->texture_id, out_ids, out_count, false))
                return false;
            continue;
        }

        /* "position" is always required. Other attrs are family-dependent and optional. */
        uint64_t pos = _scene_render_visual_resource_id(
            emitter, render->u.render.visuals[i], DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION);
        if (pos == 0)
            return false;
        if (*out_count >= DVZ_SCENE_MAX_NODE_RESOURCES)
            return false;
        out_ids[(*out_count)++] = pos;

        /* Optional attrs - collect any that exist. Order matches family pipeline expectations:
         * POINT      = position, color, size, optional selection
         * PRIMITIVE  = position, color
         * IMAGE      = position, texcoords (+ texture, registered alongside). */
        const DvzFramePlanResourceRole optional[] = {
            DVZ_FRAME_PLAN_RESOURCE_ROLE_COLOR, DVZ_FRAME_PLAN_RESOURCE_ROLE_SIZE,
            DVZ_FRAME_PLAN_RESOURCE_ROLE_SELECTION, DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXCOORDS,
            DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXTURE};
        for (uint32_t ai = 0; ai < 5; ai++)
        {
            uint64_t id = _scene_render_visual_resource_id(
                emitter, render->u.render.visuals[i], optional[ai]);
            if (id == 0)
                continue;
            if (*out_count >= DVZ_SCENE_MAX_NODE_RESOURCES)
                return false;
            out_ids[(*out_count)++] = id;
        }
    }
    return *out_count > 0;
}



/**
 * Return whether one render visual has a registered position resource.
 *
 * @param emitter the persistent emitter
 * @param render the render node
 * @param visual_index the visual index within the render node
 * @return whether the visual's position resource exists
 */
bool _scene_render_visual_has_position_resource(
    DvzFramePlanEmitter* emitter, const DvzFramePlanNode* render, uint32_t visual_index)
{
    ANN(emitter);
    ANN(render);
    if (render->type != DVZ_FRAME_PLAN_NODE_RENDER ||
        visual_index >= render->u.render.visual_count)
        return false;

    const DvzFramePlanVisualMeta* meta = &render->u.render.visual_metadata[visual_index];
    if (meta->has_metadata)
    {
        bool segment_like = meta->visual_type == DVZ_VISUAL_TYPE_SEGMENT;
        bool stroked_path = _scene_visual_meta_is_stroked_path(&emitter->resources, meta);
        return _scene_visual_resource_lookup_label(
                   &emitter->resources,
                   (segment_like || stroked_path) ? meta->position_start_id : meta->position_id) !=
               0;
    }

    return _scene_render_visual_resource_id(
               emitter, render->u.render.visuals[visual_index],
               DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION) != 0;
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
        DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXCOORDS,       DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXTURE,
        DVZ_FRAME_PLAN_RESOURCE_ROLE_NORMAL,          DVZ_FRAME_PLAN_RESOURCE_ROLE_INDEX,
        DVZ_FRAME_PLAN_RESOURCE_ROLE_MATERIAL_PARAMS, DVZ_FRAME_PLAN_RESOURCE_ROLE_SELECTION};
    for (uint32_t ai = 0; ai < 8; ai++)
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
    bool is_primitive =
        !is_point && _is_primitive_visual(&emitter->resources, out->vbuf_ids, out->vbuf_count);
    uint64_t img_pos = 0, img_uv = 0, img_tex = 0;
    bool is_image =
        !is_point && !is_primitive &&
        _is_image_visual(
            &emitter->resources, out->vbuf_ids, out->vbuf_count, &img_pos, &img_uv, &img_tex);

    if (!is_point && !is_primitive && !is_image)
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
    if (is_primitive)
        out->topology = _resource_topology(&emitter->resources, pos_buf);
    else if (is_image)
        out->topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

    if (is_point)
    {
        out->kind = DVZ_SCENE_VISUAL_DESC_POINT;
        out->point_like_kind = DVZ_SCENE_POINT_LIKE_POINT;
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
