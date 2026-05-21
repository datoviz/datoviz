/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene visual pipeline helpers                                                                */
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
#include "datoviz/drp2/enums.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef enum
{
    DVZ_SCENE_SHADER_FEATURE_NONE             = 0,
    DVZ_SCENE_SHADER_FEATURE_PICKING          = 1u << 0,
    DVZ_SCENE_SHADER_FEATURE_DEPTH_CUE        = 1u << 1,
    DVZ_SCENE_SHADER_FEATURE_LIGHTING         = 1u << 2,
    DVZ_SCENE_SHADER_FEATURE_WBOIT_ACCUM      = 1u << 3,
    DVZ_SCENE_SHADER_FEATURE_VOLUME_MIP       = 1u << 4,
    DVZ_SCENE_SHADER_FEATURE_VOLUME_COMPOSITE = 1u << 5,
    DVZ_SCENE_SHADER_FEATURE_POINT_STYLE      = 1u << 6,
    DVZ_SCENE_SHADER_FEATURE_INSTANCING       = 1u << 7,
    DVZ_SCENE_SHADER_FEATURE_SELECTION_MASK   = 1u << 8,
} DvzSceneShaderFeatureFlag;


typedef struct DvzSceneShaderFeatures
{
    DvzSceneVisualDescKind kind;
    uint32_t topology;
    uint32_t flags;
} DvzSceneShaderFeatures;



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
static uint64_t _resource_lookup_label(const ConverterState* state, const char* key)
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
    uint64_t id = _resource_lookup_label(state, key);
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
static bool _visual_meta_is_primitive(uint32_t visual_type)
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
static bool _visual_meta_is_stroked_path(
    const ConverterState* state, const DvzFramePlanVisualMeta* meta)
{
    ANN(state);
    ANN(meta);
    if (meta->visual_type != DVZ_VISUAL_TYPE_PATH)
        return false;
    return _resource_lookup_label(state, meta->position_start_id) != 0 &&
           _resource_lookup_label(state, meta->position_end_id) != 0 &&
           _resource_lookup_label(state, meta->line_width_id) != 0 &&
           _resource_lookup_label(state, meta->index_id) != 0;
}


/**
 * Return whether one retained visual has dense data for an attribute.
 *
 * @param visual the retained visual
 * @param name the attribute name
 * @return whether dense data exists
 */
static bool _visual_has_dense_attr(const DvzVisual* visual, const char* name)
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
static bool _visual_desc_is_primitive(DvzSceneVisualDescKind kind)
{
    return kind == DVZ_SCENE_VISUAL_DESC_PRIMITIVE;
}



/**
 * Return whether a visual descriptor uses a sampled-image bind group.
 *
 * @param kind the visual descriptor kind
 * @return whether the descriptor is image-like
 */
static bool _visual_desc_is_image(DvzSceneVisualDescKind kind)
{
    return kind == DVZ_SCENE_VISUAL_DESC_IMAGE || kind == DVZ_SCENE_VISUAL_DESC_GLYPH;
}



/**
 * Return whether a visual descriptor uses a sampled-volume bind group.
 *
 * @param kind the visual descriptor kind
 * @return whether the descriptor is volume-like
 */
static bool _visual_desc_is_volume(DvzSceneVisualDescKind kind)
{
    return kind == DVZ_SCENE_VISUAL_DESC_VOLUME;
}


/**
 * Return whether a visual descriptor uses the sphere impostor pipeline family.
 *
 * @param kind the visual descriptor kind
 * @return whether the descriptor is sphere-like
 */
static bool _visual_desc_is_sphere(DvzSceneVisualDescKind kind)
{
    return kind == DVZ_SCENE_VISUAL_DESC_SPHERE;
}


/**
 * Return whether a visual descriptor uses the segment analytic stroke pipeline family.
 *
 * @param kind the visual descriptor kind
 * @return whether the descriptor is segment-like
 */
static bool _visual_desc_is_segment(DvzSceneVisualDescKind kind)
{
    return kind == DVZ_SCENE_VISUAL_DESC_SEGMENT;
}



/**
 * Return the point-like family represented by a retained visual type.
 *
 * @param visual_type the retained visual type
 * @param out the output point-like family
 * @return whether the visual type is point-like
 */
static bool _visual_meta_point_like_kind(uint32_t visual_type, DvzScenePointLikeKind* out)
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
        encoded_visual_id, visual_id, sizeof(visual_id), shared_index_id,
        sizeof(shared_index_id));
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
static uint64_t _render_visual_resource_id(
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
    out->depth_cue_enabled = meta->depth_cue_enabled;
    out->point_style_enabled = meta->point_style_enabled;
    out->scene_occluded = meta->scene_occluded;
    out->scene_occlusion = meta->scene_occlusion;
    out->instance_count = 1;

    if (error != NULL)
        *error = NULL;

    bool stroked_path = _visual_meta_is_stroked_path(&emitter->resources, meta);
    bool segment_like = meta->visual_type == DVZ_VISUAL_TYPE_SEGMENT || stroked_path;
    const char* primary_position_id = segment_like ? meta->position_start_id : meta->position_id;
    uint64_t pos_buf = _resource_lookup_label(&emitter->resources, primary_position_id);
    if (pos_buf == 0)
    {
        if (error != NULL)
            *error = segment_like
                         ? "typed segment metadata missing position_start resource"
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
    if (_visual_meta_point_like_kind(meta->visual_type, &point_like_kind))
    {
        uint64_t color_id = _resource_lookup_label(&emitter->resources, meta->color_id);
        uint64_t size_id = _resource_lookup_label(&emitter->resources, meta->size_id);
        uint64_t angle_id = _resource_lookup_label(&emitter->resources, meta->angle_id);
        uint64_t shape_id = _resource_lookup_label(&emitter->resources, meta->shape_id);
        uint64_t selection_id = _resource_lookup_label(&emitter->resources, meta->selection_id);
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
        out->kind = point_like_kind == DVZ_SCENE_POINT_LIKE_PIXEL ?
                        DVZ_SCENE_VISUAL_DESC_PIXEL :
                    point_like_kind == DVZ_SCENE_POINT_LIKE_MARKER ?
                        DVZ_SCENE_VISUAL_DESC_MARKER :
                        DVZ_SCENE_VISUAL_DESC_POINT;
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
        out->material_buffer_id = _resource_lookup_label(&emitter->resources, meta->material_id);
        return true;
    }

    if (segment_like)
    {
        uint64_t end_id = _resource_lookup_label(&emitter->resources, meta->position_end_id);
        uint64_t color_id = _resource_lookup_label(&emitter->resources, meta->color_id);
        uint64_t line_width_id = _resource_lookup_label(&emitter->resources, meta->line_width_id);
        uint64_t index_id = _resource_lookup_label(&emitter->resources, meta->index_id);
        uint64_t material_id = _resource_lookup_label(&emitter->resources, meta->material_id);
        if (end_id == 0 || color_id == 0 || line_width_id == 0 || index_id == 0 ||
            material_id == 0)
        {
            if (error != NULL)
                *error = stroked_path ?
                             "typed stroked path metadata missing endpoint/color/width/index "
                             "resource" :
                             "typed segment metadata missing endpoint/color/width/index/cap "
                             "resource";
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
            uint64_t index_count =
                _resource_byte_size(&emitter->resources, index_id) /
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
            _resource_item_stride(&emitter->resources, index_id) == sizeof(uint16_t)
                ? "uint16"
                : "uint32";
        return true;
    }

    if (meta->visual_type == DVZ_VISUAL_TYPE_SPHERE)
    {
        uint64_t color_id = _resource_lookup_label(&emitter->resources, meta->color_id);
        uint64_t size_id = _resource_lookup_label(&emitter->resources, meta->size_id);
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
        out->material_buffer_id = _resource_lookup_label(&emitter->resources, meta->material_id);
        return true;
    }

    if (_visual_meta_is_primitive(meta->visual_type))
    {
        uint64_t color_id = _resource_lookup_label(&emitter->resources, meta->color_id);
        if (color_id == 0)
        {
            if (error != NULL)
                *error = "typed primitive metadata missing color resource";
            return false;
        }
        out->kind = DVZ_SCENE_VISUAL_DESC_PRIMITIVE;
        out->vbuf_ids[out->vbuf_count++] = color_id;
        uint64_t normal_id = _resource_lookup_label(&emitter->resources, meta->normal_id);
        if (normal_id != 0)
        {
            out->vbuf_ids[out->vbuf_count++] = normal_id;
            out->has_normal = true;
        }
        uint64_t instance_transform_id =
            _resource_lookup_label(&emitter->resources, meta->instance_transform_id);
        if (meta->visual_type == DVZ_VISUAL_TYPE_MESH && instance_transform_id != 0)
        {
            out->vbuf_ids[out->vbuf_count++] = instance_transform_id;
            out->has_instance_transform = true;
            uint64_t transform_bytes = _resource_byte_size(&emitter->resources, instance_transform_id);
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
        out->index_buffer_id = _resource_lookup_label(&emitter->resources, meta->index_id);
        out->material_buffer_id = _resource_lookup_label(&emitter->resources, meta->material_id);
    }
    else if (meta->visual_type == DVZ_VISUAL_TYPE_IMAGE ||
             meta->visual_type == DVZ_VISUAL_TYPE_GLYPH)
    {
        uint64_t uv_id = _resource_lookup_label(&emitter->resources, meta->texcoords_id);
        uint64_t tex_id = _resource_lookup_label(&emitter->resources, meta->texture_id);
        if (uv_id == 0 || tex_id == 0)
        {
            if (error != NULL)
                *error = "typed image metadata missing texcoords/texture resource";
            return false;
        }
        out->kind = meta->visual_type == DVZ_VISUAL_TYPE_GLYPH ? DVZ_SCENE_VISUAL_DESC_GLYPH :
                                                                 DVZ_SCENE_VISUAL_DESC_IMAGE;
        if (meta->visual_type == DVZ_VISUAL_TYPE_GLYPH)
        {
            uint64_t bounds_id = _resource_lookup_label(&emitter->resources, meta->bounds_id);
            uint64_t color_id = _resource_lookup_label(&emitter->resources, meta->color_id);
            uint64_t angle_id = _resource_lookup_label(&emitter->resources, meta->angle_id);
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
            out->topology = meta->visual_type == DVZ_VISUAL_TYPE_GLYPH ?
                                VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST :
                                VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    }
    else if (meta->visual_type == DVZ_VISUAL_TYPE_VOLUME)
    {
        uint64_t uvw_id = _resource_lookup_label(&emitter->resources, meta->texcoords_id);
        uint64_t tex_id = _resource_lookup_label(&emitter->resources, meta->volume_texture_id);
        uint64_t transfer_tex_id =
            _resource_lookup_label(&emitter->resources, meta->volume_transfer_texture_id);
        if (tex_id == 0)
            tex_id = _resource_lookup_label(&emitter->resources, meta->texture_id);
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
        uint64_t index_count =
            _resource_byte_size(&emitter->resources, out->index_buffer_id) /
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
        if (role == DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION) has_pos = true;
        if (role == DVZ_FRAME_PLAN_RESOURCE_ROLE_COLOR)    has_col = true;
        if (role == DVZ_FRAME_PLAN_RESOURCE_ROLE_SIZE)     has_sz  = true;
        if (role != DVZ_FRAME_PLAN_RESOURCE_ROLE_NONE)
            continue;

        const char* tag = _resource_data_tag(state, ids[i]);
        if (strcmp(tag, "position") == 0) has_pos = true;
        if (strcmp(tag, "color") == 0)    has_col = true;
        if (strcmp(tag, "size") == 0)     has_sz  = true;
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
    const ConverterState* state, const uint64_t* ids, uint32_t n,
    uint64_t* out_pos, uint64_t* out_uv, uint64_t* out_tex)
{
    if (n != 3)
        return false;
    uint64_t pos = 0, uv = 0, tex = 0;
    for (uint32_t i = 0; i < n; i++)
    {
        DvzFramePlanResourceRole role = _resource_role(state, ids[i]);
        if (role == DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION) { pos = ids[i]; continue; }
        if (role == DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXCOORDS) { uv = ids[i]; continue; }
        if (role == DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXTURE) { tex = ids[i]; continue; }
        if (role != DVZ_FRAME_PLAN_RESOURCE_ROLE_NONE)
            continue;

        const char* tag = _resource_data_tag(state, ids[i]);
        if (strcmp(tag, "position") == 0)  pos = ids[i];
        else if (strcmp(tag, "texcoords") == 0) uv = ids[i];
        else if (strcmp(tag, "texture") == 0)   tex = ids[i];
    }
    if (pos == 0 || uv == 0 || tex == 0)
        return false;
    if (out_pos) *out_pos = pos;
    if (out_uv)  *out_uv  = uv;
    if (out_tex) *out_tex = tex;
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
    const ConverterState* state, const uint64_t* ids, uint32_t n,
    DvzFramePlanResourceRole role)
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
            bool segment_like = meta->visual_type == DVZ_VISUAL_TYPE_SEGMENT ||
                                _visual_meta_is_stroked_path(&emitter->resources, meta);
            if (segment_like)
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
                    &emitter->resources, meta->texcoords_id, out_ids, out_count, false))
                return false;
            if (!_append_resource_key(
                    &emitter->resources, meta->texture_id, out_ids, out_count, false))
                return false;
            continue;
        }

        /* "position" is always required. Other attrs are family-dependent and optional. */
        uint64_t pos = _render_visual_resource_id(
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
            uint64_t id =
                _render_visual_resource_id(emitter, render->u.render.visuals[i], optional[ai]);
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
    if (render->type != DVZ_FRAME_PLAN_NODE_RENDER || visual_index >= render->u.render.visual_count)
        return false;

    const DvzFramePlanVisualMeta* meta = &render->u.render.visual_metadata[visual_index];
    if (meta->has_metadata)
    {
        bool segment_like = meta->visual_type == DVZ_VISUAL_TYPE_SEGMENT ||
                            _visual_meta_is_stroked_path(&emitter->resources, meta);
        return _resource_lookup_label(
                   &emitter->resources, segment_like ? meta->position_start_id :
                                                       meta->position_id) != 0;
    }

    return _render_visual_resource_id(
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
    if (render->type != DVZ_FRAME_PLAN_NODE_RENDER || visual_index >= render->u.render.visual_count)
        return false;

    out->depth_test_enabled = true;
    out->instance_count = 1;
    const DvzFramePlanVisualMeta* meta = &render->u.render.visual_metadata[visual_index];
    if (meta->has_metadata)
        return _scene_visual_desc_from_metadata(emitter, meta, out, error);

    uint64_t pos_buf = _render_visual_resource_id(
        emitter, render->u.render.visuals[visual_index], DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION);
    if (pos_buf == 0)
        return false;
    out->vbuf_ids[out->vbuf_count++] = pos_buf;

    const DvzFramePlanResourceRole optionals[] = {
        DVZ_FRAME_PLAN_RESOURCE_ROLE_COLOR, DVZ_FRAME_PLAN_RESOURCE_ROLE_SIZE,
        DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXCOORDS, DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXTURE,
        DVZ_FRAME_PLAN_RESOURCE_ROLE_NORMAL, DVZ_FRAME_PLAN_RESOURCE_ROLE_INDEX,
        DVZ_FRAME_PLAN_RESOURCE_ROLE_MATERIAL_PARAMS, DVZ_FRAME_PLAN_RESOURCE_ROLE_SELECTION};
    for (uint32_t ai = 0; ai < 8; ai++)
    {
        uint64_t rid_id = _render_visual_resource_id(
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
        out->has_normal =
            out->has_normal ||
            _scene_visual_resource_by_role(
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
        uint64_t index_count =
            _resource_byte_size(&emitter->resources, out->index_buffer_id) /
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


/**
 * Resolve common pass capabilities from normalized visual facts.
 *
 * @param kind the visual descriptor kind
 * @param alpha_mode the visual alpha mode
 * @param controller_mode the panel controller attachment mode
 * @param has_normals whether the visual has normals
 * @param has_material_resource whether a material uniform resource exists
 * @param depth_cue_enabled whether retained material state enables depth cueing
 * @param out the output pass capabilities
 */
static void _scene_visual_pass_caps_resolve(
    DvzSceneVisualDescKind kind, DvzAlphaMode alpha_mode, DvzControllerMode controller_mode,
    bool has_normals, bool has_material_resource, bool depth_cue_enabled, bool depth_test_enabled,
    DvzSceneVisualPassCaps* out)
{
    ANN(out);
    dvz_memset(out, sizeof(DvzSceneVisualPassCaps), 0, sizeof(DvzSceneVisualPassCaps));

    bool primitive = _visual_desc_is_primitive(kind);
    bool segment = _visual_desc_is_segment(kind);
    bool sphere = _visual_desc_is_sphere(kind);
    bool point_like = kind == DVZ_SCENE_VISUAL_DESC_POINT ||
                      kind == DVZ_SCENE_VISUAL_DESC_PIXEL ||
                      kind == DVZ_SCENE_VISUAL_DESC_MARKER;
    bool image = _visual_desc_is_image(kind);
    bool volume = _visual_desc_is_volume(kind);
    bool fixed = controller_mode == DVZ_CONTROLLER_FIXED;
    bool wboit = _scene_alpha_mode_is_wboit(alpha_mode);
    bool depth_peel = _scene_alpha_mode_is_depth_peel(alpha_mode);
    bool transparent_blend = _scene_alpha_mode_is_blended(alpha_mode);

    out->kind = kind;
    out->alpha_mode = alpha_mode;
    out->controller_mode = controller_mode;
    out->fixed_controller = fixed;
    out->has_normals = has_normals;
    out->depth_test_enabled = depth_test_enabled;
    out->draws_in_wboit_pass = wboit;
    out->draws_in_depth_peel_pass = depth_peel;
    out->draws_in_transparent_blend_pass = transparent_blend;
    out->draws_in_opaque_pass = !wboit && !depth_peel && !transparent_blend;
    out->uses_source_over_blend = _scene_alpha_mode_is_blended(alpha_mode);
    out->writes_color = kind != DVZ_SCENE_VISUAL_DESC_NONE;
    out->writes_depth =
        out->draws_in_opaque_pass && (primitive || segment || point_like || sphere) && !fixed &&
        depth_test_enabled;
    out->can_write_depth =
        (primitive || segment || point_like || sphere) && !fixed && depth_test_enabled;
    out->can_depth_test =
        (primitive || segment || point_like || sphere) && !fixed && depth_test_enabled;
    out->samples_depth = volume && !fixed;
    out->needs_depth_attachment = out->can_depth_test || out->samples_depth;
    out->eligible_for_depth_postprocess = out->draws_in_opaque_pass && out->writes_depth;
    out->eligible_for_gbuffer = out->draws_in_opaque_pass &&
                                ((primitive && has_normals) || sphere) && out->writes_depth;
    out->uses_common_set = kind != DVZ_SCENE_VISUAL_DESC_NONE;
    out->needs_material_layout = (primitive && has_normals) || segment || sphere ||
                                 (point_like && has_material_resource);
    out->uses_material_set = out->needs_material_layout && has_material_resource;
    out->uses_image_set = image;
    out->uses_volume_set = volume;
    out->supports_depth_cue = (primitive && has_normals) || point_like || sphere;
    out->depth_cue_enabled = out->supports_depth_cue && depth_cue_enabled;
}



/**
 * Resolve pass capabilities from one retained visual attachment.
 *
 * @param visual the retained visual
 * @param attach the panel attachment
 * @param out the output pass capabilities
 * @return whether capabilities were resolved
 */
bool _scene_visual_pass_caps_from_visual(
    const DvzVisual* visual, const DvzPanelAttach* attach, DvzSceneVisualPassCaps* out)
{
    ANN(visual);
    ANN(attach);
    ANN(out);

    DvzSceneVisualDescKind kind = DVZ_SCENE_VISUAL_DESC_NONE;
    switch (visual->type)
    {
    case DVZ_VISUAL_TYPE_POINT:
    case DVZ_VISUAL_TYPE_PIXEL:
    case DVZ_VISUAL_TYPE_MARKER:
        kind = visual->type == DVZ_VISUAL_TYPE_PIXEL ?
                   DVZ_SCENE_VISUAL_DESC_PIXEL :
               visual->type == DVZ_VISUAL_TYPE_MARKER ?
                   DVZ_SCENE_VISUAL_DESC_MARKER :
                   DVZ_SCENE_VISUAL_DESC_POINT;
        break;
    case DVZ_VISUAL_TYPE_SPHERE:
        kind = DVZ_SCENE_VISUAL_DESC_SPHERE;
        break;
    case DVZ_VISUAL_TYPE_SEGMENT:
        kind = DVZ_SCENE_VISUAL_DESC_SEGMENT;
        break;
    case DVZ_VISUAL_TYPE_PRIMITIVE:
    case DVZ_VISUAL_TYPE_MESH:
        kind = DVZ_SCENE_VISUAL_DESC_PRIMITIVE;
        break;
    case DVZ_VISUAL_TYPE_PATH:
        kind = _visual_has_dense_attr(visual, "line_width") ? DVZ_SCENE_VISUAL_DESC_SEGMENT :
                                                              DVZ_SCENE_VISUAL_DESC_PRIMITIVE;
        break;
    case DVZ_VISUAL_TYPE_IMAGE:
        kind = DVZ_SCENE_VISUAL_DESC_IMAGE;
        break;
    case DVZ_VISUAL_TYPE_GLYPH:
        kind = DVZ_SCENE_VISUAL_DESC_GLYPH;
        break;
    case DVZ_VISUAL_TYPE_VOLUME:
        kind = DVZ_SCENE_VISUAL_DESC_VOLUME;
        break;
    case DVZ_VISUAL_TYPE_NONE:
    default:
        return false;
    }

    bool has_normals = false;
    if (_visual_desc_is_primitive(kind))
    {
        int normal_idx = _attr_index(visual, "normal");
        has_normals = normal_idx >= 0 && visual->attrs[normal_idx].data != NULL &&
                      visual->attrs[normal_idx].item_count > 0;
    }

    bool point_like = kind == DVZ_SCENE_VISUAL_DESC_POINT ||
                      kind == DVZ_SCENE_VISUAL_DESC_PIXEL ||
                      kind == DVZ_SCENE_VISUAL_DESC_MARKER;
    bool stroked_path = visual->type == DVZ_VISUAL_TYPE_PATH &&
                        _visual_has_dense_attr(visual, "line_width");
    bool has_material_resource = has_normals || visual->type == DVZ_VISUAL_TYPE_SEGMENT ||
                                 stroked_path ||
                                 visual->type == DVZ_VISUAL_TYPE_SPHERE ||
                                 (point_like && visual->material.depth_cue_enabled) ||
                                 (visual->type == DVZ_VISUAL_TYPE_POINT &&
                                  visual->material.point_style_enabled) ||
                                 visual->type == DVZ_VISUAL_TYPE_MARKER;
    _scene_visual_pass_caps_resolve(
        kind, visual->alpha_mode, attach->controller_mode, has_normals, has_material_resource,
        visual->material.depth_cue_enabled, visual->depth_test_enabled, out);
    return true;
}



/**
 * Resolve pass capabilities from one FramePlan visual descriptor.
 *
 * @param visual the visual descriptor
 * @param alpha_mode the visual alpha mode
 * @param controller_mode the panel controller attachment mode
 * @param out the output pass capabilities
 * @return whether capabilities were resolved
 */
bool _scene_visual_pass_caps_from_desc(
    const DvzSceneVisualDesc* visual, DvzAlphaMode alpha_mode,
    DvzControllerMode controller_mode, DvzSceneVisualPassCaps* out)
{
    ANN(visual);
    ANN(out);
    if (visual->kind == DVZ_SCENE_VISUAL_DESC_NONE)
        return false;

    _scene_visual_pass_caps_resolve(
        visual->kind, alpha_mode, controller_mode, visual->has_normal,
        visual->material_buffer_id != 0, visual->depth_cue_enabled,
        visual->depth_test_enabled, out);
    return true;
}



/**
 * Return whether one shader-feature flag is set.
 *
 * @param features the shader feature descriptor
 * @param flag the feature flag
 * @return whether the flag is present
 */
static bool _shader_features_has(
    const DvzSceneShaderFeatures* features, DvzSceneShaderFeatureFlag flag)
{
    ANN(features);
    return (features->flags & (uint32_t)flag) != 0;
}



/**
 * Resolve shader feature flags from a visual descriptor and pass mode.
 *
 * @param visual the visual descriptor
 * @param picking whether the pass writes pick ids
 * @param wboit_accumulation whether the pass is the WBOIT accumulation pass
 * @param out the output feature descriptor
 */
static void _scene_shader_features_resolve(
    const DvzSceneVisualDesc* visual, bool picking, bool wboit_accumulation,
    DvzSceneShaderFeatures* out)
{
    ANN(visual);
    ANN(out);
    dvz_memset(out, sizeof(DvzSceneShaderFeatures), 0, sizeof(DvzSceneShaderFeatures));

    out->kind = visual->kind;
    out->topology = visual->topology;
    if (picking)
        out->flags |= DVZ_SCENE_SHADER_FEATURE_PICKING;
    if (wboit_accumulation)
        out->flags |= DVZ_SCENE_SHADER_FEATURE_WBOIT_ACCUM;
    if (
        visual->depth_cue_enabled &&
        (visual->kind == DVZ_SCENE_VISUAL_DESC_POINT ||
         visual->kind == DVZ_SCENE_VISUAL_DESC_PIXEL))
        out->flags |= DVZ_SCENE_SHADER_FEATURE_DEPTH_CUE;
    if (visual->point_style_enabled && visual->kind == DVZ_SCENE_VISUAL_DESC_POINT)
        out->flags |= DVZ_SCENE_SHADER_FEATURE_POINT_STYLE;
    if (visual->has_selection_mask && !picking &&
        (visual->kind == DVZ_SCENE_VISUAL_DESC_POINT ||
         visual->kind == DVZ_SCENE_VISUAL_DESC_MARKER))
    {
        out->flags |= DVZ_SCENE_SHADER_FEATURE_SELECTION_MASK;
    }
    if (visual->kind == DVZ_SCENE_VISUAL_DESC_SPHERE && visual->material_buffer_id != 0)
        out->flags |= DVZ_SCENE_SHADER_FEATURE_LIGHTING;
    if (visual->has_normal)
        out->flags |= DVZ_SCENE_SHADER_FEATURE_LIGHTING;
    if (visual->has_instance_transform)
        out->flags |= DVZ_SCENE_SHADER_FEATURE_INSTANCING;
    if (visual->volume_state.render_mode == DVZ_VOLUME_RENDER_MIP)
        out->flags |= DVZ_SCENE_SHADER_FEATURE_VOLUME_MIP;
    if (visual->volume_state.render_mode == DVZ_VOLUME_RENDER_COMPOSITE)
        out->flags |= DVZ_SCENE_SHADER_FEATURE_VOLUME_COMPOSITE;
}



/**
 * Attach built-in shader source pointers to a shader descriptor.
 *
 * @param out the output shader descriptor
 * @param shader the built-in shader id
 */
static void _scene_shader_desc_set_builtin(
    DvzSceneVisualShaderDesc* out, DvzSceneBuiltinShader shader)
{
    ANN(out);
    out->vertex_glsl = _builtin_shader_glsl(shader, false);
    out->fragment_glsl = _builtin_shader_glsl(shader, true);
    out->vertex_wgsl = _builtin_shader_wgsl(shader, false);
    out->fragment_wgsl = _builtin_shader_wgsl(shader, true);
}


/**
 * Attach stable built-in identity metadata to a shader descriptor.
 *
 * @param out the output shader descriptor
 * @param family the shader family id
 * @param variant the shader variant id
 */
static void _scene_shader_desc_set_identity(
    DvzSceneVisualShaderDesc* out, const char* family, const char* variant)
{
    ANN(out);
    out->builtin_family = family;
    out->builtin_variant = variant != NULL ? variant : "default";
    out->builtin_pipeline = family;
}



/**
 * Resolve point-like shader metadata from feature flags.
 *
 * @param visual_name the visual key stem
 * @param features the shader features
 * @param format_tag the shader-format cache-key suffix
 * @param out the output shader descriptor
 * @return whether a shader descriptor was resolved
 */
static bool _scene_shader_desc_point_like(
    const char* visual_name, const DvzSceneShaderFeatures* features, const char* format_tag,
    DvzSceneVisualShaderDesc* out)
{
    ANN(visual_name);
    ANN(features);
    ANN(format_tag);
    ANN(out);

    bool picking = _shader_features_has(features, DVZ_SCENE_SHADER_FEATURE_PICKING);
    bool depth_cue = _shader_features_has(features, DVZ_SCENE_SHADER_FEATURE_DEPTH_CUE);
    bool point_style = _shader_features_has(features, DVZ_SCENE_SHADER_FEATURE_POINT_STYLE);
    bool selection = _shader_features_has(features, DVZ_SCENE_SHADER_FEATURE_SELECTION_MASK) &&
                     !depth_cue && !point_style;

    const char* suffix = picking ? "_pick" : selection ? "_select" :
                         point_style && depth_cue ? "_cue_style" :
                         point_style ? "_style" : depth_cue ? "_cue" : "";
    dvz_snprintf(
        out->vertex_key, sizeof(out->vertex_key), "_vs_%s%s%s", visual_name, suffix, format_tag);
    dvz_snprintf(
        out->fragment_key, sizeof(out->fragment_key), "_fs_%s%s%s", visual_name, suffix,
        format_tag);
    dvz_snprintf(
        out->pipeline_key, sizeof(out->pipeline_key), "_pipe_%s%s%s", visual_name, suffix,
        format_tag);

    DvzSceneBuiltinShader shader = DVZ_SCENE_BUILTIN_SHADER_POINT;
    if (features->kind == DVZ_SCENE_VISUAL_DESC_MARKER)
        shader = picking ? DVZ_SCENE_BUILTIN_SHADER_PIXEL_PICK :
                 selection ? DVZ_SCENE_BUILTIN_SHADER_MARKER_SELECTION :
                             DVZ_SCENE_BUILTIN_SHADER_MARKER;
    else if (features->kind == DVZ_SCENE_VISUAL_DESC_PIXEL)
        shader = picking ? DVZ_SCENE_BUILTIN_SHADER_PIXEL_PICK :
                 depth_cue ? DVZ_SCENE_BUILTIN_SHADER_PIXEL_DEPTH_CUE :
                             DVZ_SCENE_BUILTIN_SHADER_PIXEL;
    else if (picking)
        shader = DVZ_SCENE_BUILTIN_SHADER_POINT_PICK;
    else if (selection)
        shader = DVZ_SCENE_BUILTIN_SHADER_POINT_SELECTION;
    else if (point_style)
        shader = depth_cue ? DVZ_SCENE_BUILTIN_SHADER_POINT_STYLE_DEPTH_CUE :
                             DVZ_SCENE_BUILTIN_SHADER_POINT_STYLE;
    else
        shader = depth_cue ? DVZ_SCENE_BUILTIN_SHADER_POINT_DEPTH_CUE :
                             DVZ_SCENE_BUILTIN_SHADER_POINT;

    _scene_shader_desc_set_builtin(out, shader);
    _scene_shader_desc_set_identity(out, features->kind == DVZ_SCENE_VISUAL_DESC_PIXEL ?
                                             "scene.pixel" :
                                         features->kind == DVZ_SCENE_VISUAL_DESC_MARKER ?
                                             "scene.marker" :
                                             "scene.point",
                                    picking ? "pick" :
                                    selection ? "selection" :
                                    point_style && depth_cue ? "style_depth_cue" :
                                    point_style ? "style" :
                                    depth_cue ? "depth_cue" : "default");
    if (!picking)
    {
        if (features->kind == DVZ_SCENE_VISUAL_DESC_MARKER)
        {
            out->vertex_spirv_key = selection ? "marker_select_vert" : "marker_vert";
            out->fragment_spirv_key = selection ? "marker_select_frag" : "marker_frag";
        }
        else if (features->kind == DVZ_SCENE_VISUAL_DESC_PIXEL)
        {
            out->vertex_spirv_key = depth_cue ? "pixel_cue_vert" : "pixel_vert";
            out->fragment_spirv_key = depth_cue ? "pixel_cue_frag" : "pixel_frag";
        }
        else if (point_style)
        {
            out->vertex_spirv_key = depth_cue ? "point_cue_style_vert" : "point_style_vert";
            out->fragment_spirv_key = depth_cue ? "point_cue_style_frag" : "point_style_frag";
        }
        else
        {
            out->vertex_spirv_key = selection ? "point_select_vert" :
                                    depth_cue ? "point_cue_vert" : "point_vert";
            out->fragment_spirv_key = selection ? "point_select_frag" :
                                      depth_cue ? "point_cue_frag" : "point_frag";
        }
    }
    else
    {
        out->vertex_spirv_key =
            features->kind == DVZ_SCENE_VISUAL_DESC_PIXEL ||
                    features->kind == DVZ_SCENE_VISUAL_DESC_MARKER ?
                "pixel_pick_vert" :
                "point_pick_vert";
        out->fragment_spirv_key =
            features->kind == DVZ_SCENE_VISUAL_DESC_PIXEL ||
                    features->kind == DVZ_SCENE_VISUAL_DESC_MARKER ?
                "pixel_pick_frag" :
                "point_pick_frag";
    }
    return true;
}



/**
 * Resolve primitive shader metadata from feature flags.
 *
 * @param features the shader features
 * @param format_tag the shader-format cache-key suffix
 * @param out the output shader descriptor
 * @return whether a shader descriptor was resolved
 */
static bool _scene_shader_desc_primitive(
    const DvzSceneShaderFeatures* features, const char* format_tag,
    DvzSceneVisualShaderDesc* out)
{
    ANN(features);
    ANN(format_tag);
    ANN(out);

    bool lit = _shader_features_has(features, DVZ_SCENE_SHADER_FEATURE_LIGHTING);
    bool instanced = _shader_features_has(features, DVZ_SCENE_SHADER_FEATURE_INSTANCING);
    if (_shader_features_has(features, DVZ_SCENE_SHADER_FEATURE_WBOIT_ACCUM))
    {
        DvzSceneBuiltinShader shader = lit ? DVZ_SCENE_BUILTIN_SHADER_WBOIT_ACCUM_LIT :
                                             DVZ_SCENE_BUILTIN_SHADER_WBOIT_ACCUM;
        dvz_snprintf(
            out->vertex_key, sizeof(out->vertex_key), "_vs_wboit_accum_n%u%s", lit ? 1u : 0u,
            format_tag);
        dvz_snprintf(
            out->fragment_key, sizeof(out->fragment_key), "_fs_wboit_accum_n%u%s", lit ? 1u : 0u,
            format_tag);
        dvz_snprintf(
            out->pipeline_key, sizeof(out->pipeline_key), "_pipe_wboit_accum_t%u_n%u%s",
            features->topology, lit ? 1u : 0u, format_tag);
        _scene_shader_desc_set_builtin(out, shader);
        if (instanced)
        {
            DvzSceneBuiltinShader vertex_shader =
                lit ? DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE_LIT_INSTANCED :
                      DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE_INSTANCED;
            out->vertex_glsl = _builtin_shader_glsl(vertex_shader, false);
            out->vertex_wgsl = _builtin_shader_wgsl(vertex_shader, false);
            out->vertex_spirv_key = lit ? "primitive_lit_instanced_vert" :
                                          "primitive_instanced_vert";
        }
        _scene_shader_desc_set_identity(
            out, "scene.primitive",
            instanced ? (lit ? "wboit_lit_instanced" : "wboit_instanced") :
                        (lit ? "wboit_lit" : "wboit"));
        return true;
    }

    if (lit)
    {
        dvz_snprintf(
            out->vertex_key, sizeof(out->vertex_key), "_vs_prim_lit%s%s",
            instanced ? "_inst" : "", format_tag);
        dvz_snprintf(out->fragment_key, sizeof(out->fragment_key), "_fs_prim_lit%s", format_tag);
        dvz_snprintf(
            out->pipeline_key, sizeof(out->pipeline_key), "_pipe_prim_lit_t%u%s%s",
            features->topology, instanced ? "_inst" : "", format_tag);
        _scene_shader_desc_set_builtin(
            out, instanced ? DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE_LIT_INSTANCED :
                             DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE_LIT);
        _scene_shader_desc_set_identity(
            out, "scene.primitive", instanced ? "lit_instanced" : "lit");
        return true;
    }

    dvz_snprintf(
        out->vertex_key, sizeof(out->vertex_key), "_vs_prim%s%s", instanced ? "_inst" : "",
        format_tag);
    dvz_snprintf(out->fragment_key, sizeof(out->fragment_key), "_fs_prim%s", format_tag);
    dvz_snprintf(
        out->pipeline_key, sizeof(out->pipeline_key), "_pipe_prim_t%u%s%s", features->topology,
        instanced ? "_inst" : "", format_tag);
    _scene_shader_desc_set_builtin(
        out, instanced ? DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE_INSTANCED :
                         DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE);
    _scene_shader_desc_set_identity(
        out, "scene.primitive", instanced ? "instanced" : "default");
    out->vertex_spirv_key = instanced ? "primitive_instanced_vert" : "primitive_vert";
    out->fragment_spirv_key = "primitive_frag";
    return true;
}


/**
 * Resolve segment shader metadata.
 *
 * @param format_tag the shader-format cache-key suffix
 * @param out the output shader descriptor
 * @return whether a shader descriptor was resolved
 */
static bool
_scene_shader_desc_segment(const char* format_tag, DvzSceneVisualShaderDesc* out)
{
    ANN(format_tag);
    ANN(out);

    dvz_snprintf(out->vertex_key, sizeof(out->vertex_key), "_vs_segment%s", format_tag);
    dvz_snprintf(out->fragment_key, sizeof(out->fragment_key), "_fs_segment%s", format_tag);
    dvz_snprintf(out->pipeline_key, sizeof(out->pipeline_key), "_pipe_segment%s", format_tag);
    _scene_shader_desc_set_builtin(out, DVZ_SCENE_BUILTIN_SHADER_SEGMENT);
    _scene_shader_desc_set_identity(out, "scene.segment", "default");
    out->vertex_spirv_key = "segment_vert";
    out->fragment_spirv_key = "segment_frag";
    return true;
}


/**
 * Resolve sphere shader metadata from feature flags.
 *
 * @param features the shader features
 * @param format_tag the shader-format cache-key suffix
 * @param out the output shader descriptor
 * @return whether a shader descriptor was resolved
 */
static bool _scene_shader_desc_sphere(
    const DvzSceneShaderFeatures* features, const char* format_tag,
    DvzSceneVisualShaderDesc* out)
{
    ANN(features);
    ANN(format_tag);
    ANN(out);

    (void)features;
    dvz_snprintf(out->vertex_key, sizeof(out->vertex_key), "_vs_sphere%s", format_tag);
    dvz_snprintf(out->fragment_key, sizeof(out->fragment_key), "_fs_sphere%s", format_tag);
    dvz_snprintf(out->pipeline_key, sizeof(out->pipeline_key), "_pipe_sphere%s", format_tag);
    _scene_shader_desc_set_builtin(out, DVZ_SCENE_BUILTIN_SHADER_SPHERE);
    _scene_shader_desc_set_identity(out, "scene.sphere", "default");
    out->vertex_spirv_key = "sphere_vert";
    out->fragment_spirv_key = "sphere_frag";
    return true;
}



/**
 * Resolve shader and pipeline cache-key metadata for one visual descriptor.
 *
 * @param visual the visual descriptor
 * @param picking whether the render pass is a picking pass
 * @param format_tag the shader-format cache-key suffix
 * @param out the output shader descriptor
 * @return whether a shader descriptor was resolved
 */
bool _scene_visual_shader_desc(
    const DvzSceneVisualDesc* visual, bool picking, bool wboit_accumulation,
    const char* format_tag,
    DvzSceneVisualShaderDesc* out)
{
    ANN(visual);
    ANN(format_tag);
    ANN(out);
    dvz_memset(out, sizeof(DvzSceneVisualShaderDesc), 0, sizeof(DvzSceneVisualShaderDesc));

    DvzSceneShaderFeatures features = {0};
    _scene_shader_features_resolve(visual, picking, wboit_accumulation, &features);

    switch (visual->kind)
    {
    case DVZ_SCENE_VISUAL_DESC_PIXEL:
        return _scene_shader_desc_point_like("pixel", &features, format_tag, out);

    case DVZ_SCENE_VISUAL_DESC_POINT:
        return _scene_shader_desc_point_like("point", &features, format_tag, out);

    case DVZ_SCENE_VISUAL_DESC_MARKER:
        return _scene_shader_desc_point_like("marker", &features, format_tag, out);

    case DVZ_SCENE_VISUAL_DESC_SPHERE:
        return _scene_shader_desc_sphere(&features, format_tag, out);

    case DVZ_SCENE_VISUAL_DESC_SEGMENT:
        return _scene_shader_desc_segment(format_tag, out);

    case DVZ_SCENE_VISUAL_DESC_PRIMITIVE:
        return _scene_shader_desc_primitive(&features, format_tag, out);

    case DVZ_SCENE_VISUAL_DESC_IMAGE:
        dvz_snprintf(out->vertex_key, sizeof(out->vertex_key), "_vs_img%s", format_tag);
        dvz_snprintf(out->fragment_key, sizeof(out->fragment_key), "_fs_img%s", format_tag);
        dvz_snprintf(out->pipeline_key, sizeof(out->pipeline_key), "_pipe_img%s", format_tag);
        _scene_shader_desc_set_builtin(out, DVZ_SCENE_BUILTIN_SHADER_IMAGE);
        _scene_shader_desc_set_identity(out, "scene.image", "default");
        out->vertex_spirv_key = "image_vert";
        out->fragment_spirv_key = "image_frag";
        return true;

    case DVZ_SCENE_VISUAL_DESC_GLYPH:
        dvz_snprintf(out->vertex_key, sizeof(out->vertex_key), "_vs_glyph%s", format_tag);
        dvz_snprintf(out->fragment_key, sizeof(out->fragment_key), "_fs_glyph%s", format_tag);
        dvz_snprintf(out->pipeline_key, sizeof(out->pipeline_key), "_pipe_glyph%s", format_tag);
        _scene_shader_desc_set_builtin(out, DVZ_SCENE_BUILTIN_SHADER_GLYPH);
        _scene_shader_desc_set_identity(out, "scene.glyph", "msdf");
        out->vertex_spirv_key = "glyph_vert";
        out->fragment_spirv_key = "glyph_frag";
        return true;

    case DVZ_SCENE_VISUAL_DESC_VOLUME:
    {
        bool mip = _shader_features_has(&features, DVZ_SCENE_SHADER_FEATURE_VOLUME_MIP);
        bool composite =
            _shader_features_has(&features, DVZ_SCENE_SHADER_FEATURE_VOLUME_COMPOSITE);
        dvz_snprintf(out->vertex_key, sizeof(out->vertex_key), "_vs_vol_slice%s", format_tag);
        dvz_snprintf(
            out->fragment_key, sizeof(out->fragment_key),
            composite ? "_fs_vol_composite%s" : mip ? "_fs_vol_mip%s" : "_fs_vol_slice%s",
            format_tag);
        dvz_snprintf(
            out->pipeline_key, sizeof(out->pipeline_key),
            composite ? "_pipe_vol_composite%s" : mip ? "_pipe_vol_mip%s" : "_pipe_vol_slice%s",
            format_tag);
        DvzSceneBuiltinShader shader = composite ? DVZ_SCENE_BUILTIN_SHADER_VOLUME_COMPOSITE :
                                     mip       ? DVZ_SCENE_BUILTIN_SHADER_VOLUME_MIP :
                                                 DVZ_SCENE_BUILTIN_SHADER_VOLUME_SLICE;
        _scene_shader_desc_set_builtin(out, shader);
        _scene_shader_desc_set_identity(
            out, "scene.volume", composite ? "composite" : mip ? "mip" : "slice");
        out->vertex_spirv_key = "volume_slice_vert";
        out->fragment_spirv_key =
            composite ? "volume_composite_frag" : mip ? "volume_mip_frag" : "volume_slice_frag";
        return true;
    }

    case DVZ_SCENE_VISUAL_DESC_NONE:
    default:
        return false;
    }
}





/**
 * Set one vertex attribute slot in a visual pipeline descriptor.
 *
 * @param out pipeline descriptor to update
 * @param index descriptor attribute index
 * @param binding vertex buffer binding index
 * @param location shader input location
 * @param format vertex input format
 * @param stride vertex buffer stride in bytes
 */
static void _pipeline_attr(
    DvzSceneVisualPipelineDesc* out, uint32_t index, uint32_t binding, uint32_t location,
    uint32_t format, uint32_t stride)
{
    ANN(out);
    ASSERT(index < DVZ_SCENE_MAX_NODE_RESOURCES);

    out->bindings[index] = binding;
    out->locations[index] = location;
    out->formats[index] = format;
    out->strides[index] = stride;
    out->strides[binding] = stride;
    if (out->step_modes[binding] == 0)
        out->step_modes[binding] = DVZ_DRP2_VERTEX_STEP_MODE_VERTEX;
}


/**
 * Set one mat4 instance transform vertex input.
 *
 * @param out pipeline descriptor to update
 * @param first_attr first descriptor attribute index
 * @param binding vertex buffer binding index
 */
static void _pipeline_instance_transform(
    DvzSceneVisualPipelineDesc* out, uint32_t first_attr, uint32_t binding)
{
    ANN(out);
    for (uint32_t i = 0; i < 4; i++)
    {
        _pipeline_attr(
            out, first_attr + i, binding, 3 + i, VK_FORMAT_R32G32B32A32_SFLOAT,
            16 * sizeof(float));
        out->offsets[first_attr + i] = i * 4 * sizeof(float);
    }
    out->step_modes[binding] = DVZ_DRP2_VERTEX_STEP_MODE_INSTANCE;
}



/**
 * Apply the standard depth-state rules shared by raster visual descriptors.
 *
 * @param caps resolved pass capabilities for the visual
 * @param pass_needs_depth whether the containing pass has a depth attachment
 * @param wboit_accumulation whether the pass is an order-independent transparency pass
 * @param alpha_mode visual alpha mode
 * @param out pipeline descriptor to update
 */
static void _pipeline_apply_standard_depth_state(
    const DvzSceneVisualPassCaps* caps, bool pass_needs_depth, bool wboit_accumulation,
    DvzAlphaMode alpha_mode, DvzSceneVisualPipelineDesc* out)
{
    ANN(caps);
    ANN(out);

    if (!pass_needs_depth)
        return;

    out->depth_write_enabled =
        caps->can_write_depth && !wboit_accumulation && alpha_mode != DVZ_ALPHA_BLENDED;
    out->depth_compare_op =
        caps->can_depth_test ? VK_COMPARE_OP_LESS_OR_EQUAL : VK_COMPARE_OP_ALWAYS;
}


/**
 * Resolve vertex-layout and depth-state metadata for one visual descriptor.
 *
 * @param visual the visual descriptor
 * @param picking whether the render pass is a picking pass
 * @param pass_needs_depth whether the containing render pass has a depth attachment
 * @param controller_mode controller attachment mode for the visual
 * @param out the output pipeline descriptor
 * @return whether a pipeline descriptor was resolved
 */
bool _scene_visual_pipeline_desc(
    const DvzSceneVisualDesc* visual, bool picking, bool pass_needs_depth,
    bool wboit_accumulation, DvzAlphaMode alpha_mode, DvzControllerMode controller_mode,
    DvzSceneVisualPipelineDesc* out)
{
    ANN(visual);
    ANN(out);
    dvz_memset(out, sizeof(DvzSceneVisualPipelineDesc), 0, sizeof(DvzSceneVisualPipelineDesc));

    DvzSceneVisualPassCaps caps = {0};
    if (!_scene_visual_pass_caps_from_desc(visual, alpha_mode, controller_mode, &caps))
        return false;

    out->topology = visual->topology;
    out->has_depth_state = pass_needs_depth;
    out->needs_scene_occlusion_layout = visual->scene_occluded;
    if (pass_needs_depth)
    {
        out->depth_write_enabled = false;
        out->depth_compare_op = VK_COMPARE_OP_ALWAYS;
    }

    switch (visual->kind)
    {
    case DVZ_SCENE_VISUAL_DESC_PIXEL:
    case DVZ_SCENE_VISUAL_DESC_POINT:
    case DVZ_SCENE_VISUAL_DESC_SPHERE:
    case DVZ_SCENE_VISUAL_DESC_MARKER:
        if (visual->kind == DVZ_SCENE_VISUAL_DESC_MARKER)
        {
            uint32_t attr_count = picking ? 2 : 5;
            if (visual->has_selection_mask && !picking)
                attr_count++;
            out->vertex_buffer_count = visual->has_selection_mask ? 6 : 5;
            out->binding_count = out->vertex_buffer_count;
            out->attr_count = attr_count;
            _pipeline_attr(out, 0, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float));
            _pipeline_attr(
                out, 1, picking ? 2 : 1, picking ? 2 : 1,
                picking ? VK_FORMAT_R32_SFLOAT : VK_FORMAT_R8G8B8A8_UNORM,
                4 * sizeof(uint8_t));
            _pipeline_attr(out, 2, 2, 2, VK_FORMAT_R32_SFLOAT, sizeof(float));
            _pipeline_attr(out, 3, 3, 3, VK_FORMAT_R32_SFLOAT, sizeof(float));
            _pipeline_attr(out, 4, 4, 4, VK_FORMAT_R32_UINT, sizeof(uint32_t));
            if (visual->has_selection_mask && !picking)
                _pipeline_attr(out, 5, 5, 5, VK_FORMAT_R8_UINT, sizeof(uint8_t));
            out->needs_common_layout = caps.uses_common_set;
            out->needs_material_layout = caps.needs_material_layout && !picking;
            _pipeline_apply_standard_depth_state(
                &caps, pass_needs_depth, wboit_accumulation, alpha_mode, out);
            return true;
        }

        out->vertex_buffer_count = visual->has_selection_mask ? 4 : 3;
        out->binding_count = out->vertex_buffer_count;
        out->attr_count =
            visual->kind == DVZ_SCENE_VISUAL_DESC_SPHERE ? 3 :
            picking ? 2 : visual->has_selection_mask ? 4 : 3;
        uint32_t color_binding = picking && visual->kind != DVZ_SCENE_VISUAL_DESC_SPHERE ? 2 : 1;
        uint32_t color_format = picking && visual->kind != DVZ_SCENE_VISUAL_DESC_SPHERE ?
                                    VK_FORMAT_R32_SFLOAT :
                                    VK_FORMAT_R8G8B8A8_UNORM;
        _pipeline_attr(out, 0, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float));
        _pipeline_attr(out, 1, color_binding, color_binding, color_format, 4 * sizeof(uint8_t));
        _pipeline_attr(out, 2, 2, 2, VK_FORMAT_R32_SFLOAT, sizeof(float));
        if (visual->has_selection_mask && !picking)
            _pipeline_attr(out, 3, 3, 5, VK_FORMAT_R8_UINT, sizeof(uint8_t));
        out->needs_common_layout = caps.uses_common_set;
        out->needs_material_layout = caps.needs_material_layout;
        _pipeline_apply_standard_depth_state(
            &caps, pass_needs_depth, wboit_accumulation, alpha_mode, out);
        return true;

    case DVZ_SCENE_VISUAL_DESC_PRIMITIVE:
        out->vertex_buffer_count =
            (visual->has_normal ? 3u : 2u) + (visual->has_instance_transform ? 1u : 0u);
        out->binding_count = out->vertex_buffer_count;
        out->attr_count = (visual->has_normal ? 3u : 2u) +
                          (visual->has_instance_transform ? 4u : 0u);
        _pipeline_attr(out, 0, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float));
        _pipeline_attr(out, 1, 1, 1, VK_FORMAT_R8G8B8A8_UNORM, 4 * sizeof(uint8_t));
        if (visual->has_normal)
            _pipeline_attr(out, 2, 2, 2, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float));
        if (visual->has_instance_transform)
        {
            uint32_t transform_binding = visual->has_normal ? 3 : 2;
            _pipeline_instance_transform(out, transform_binding, transform_binding);
        }
        out->needs_common_layout = caps.uses_common_set;
        out->needs_material_layout = caps.needs_material_layout;
        _pipeline_apply_standard_depth_state(
            &caps, pass_needs_depth, wboit_accumulation, alpha_mode, out);
        return true;

    case DVZ_SCENE_VISUAL_DESC_SEGMENT:
        out->vertex_buffer_count = 4;
        out->binding_count = 4;
        out->attr_count = 4;
        _pipeline_attr(out, 0, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float));
        _pipeline_attr(out, 1, 1, 1, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float));
        _pipeline_attr(out, 2, 2, 2, VK_FORMAT_R8G8B8A8_UNORM, 4 * sizeof(uint8_t));
        _pipeline_attr(out, 3, 3, 3, VK_FORMAT_R32_SFLOAT, sizeof(float));
        out->needs_common_layout = caps.uses_common_set;
        out->needs_material_layout = caps.needs_material_layout;
        _pipeline_apply_standard_depth_state(
            &caps, pass_needs_depth, wboit_accumulation, alpha_mode, out);
        return true;

    case DVZ_SCENE_VISUAL_DESC_IMAGE:
        out->vertex_buffer_count = 2;
        out->binding_count = 2;
        out->attr_count = 2;
        _pipeline_attr(out, 0, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float));
        _pipeline_attr(out, 1, 1, 1, VK_FORMAT_R32G32_SFLOAT, 2 * sizeof(float));
        out->needs_common_layout = caps.uses_common_set;
        out->needs_image_layout = caps.uses_image_set;
        return true;

    case DVZ_SCENE_VISUAL_DESC_GLYPH:
        out->vertex_buffer_count = 5;
        out->binding_count = 5;
        out->attr_count = 5;
        _pipeline_attr(out, 0, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float));
        _pipeline_attr(out, 1, 1, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 4 * sizeof(float));
        _pipeline_attr(out, 2, 2, 2, VK_FORMAT_R32G32B32A32_SFLOAT, 4 * sizeof(float));
        _pipeline_attr(out, 3, 3, 3, VK_FORMAT_R8G8B8A8_UNORM, 4 * sizeof(uint8_t));
        _pipeline_attr(out, 4, 4, 4, VK_FORMAT_R32_SFLOAT, sizeof(float));
        out->needs_common_layout = caps.uses_common_set;
        out->needs_glyph_layout = caps.uses_image_set;
        return true;

    case DVZ_SCENE_VISUAL_DESC_VOLUME:
        out->vertex_buffer_count = 2;
        out->binding_count = 2;
        out->attr_count = 2;
        _pipeline_attr(out, 0, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float));
        _pipeline_attr(out, 1, 1, 1, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float));
        out->needs_common_layout = caps.uses_common_set;
        out->needs_volume_layout = caps.uses_volume_set;
        out->has_raster_state = true;
        out->cull_mode = VK_CULL_MODE_BACK_BIT;
        out->front_face = VK_FRONT_FACE_CLOCKWISE;
        return true;

    case DVZ_SCENE_VISUAL_DESC_NONE:
    default:
        return false;
    }
}



/**
 * Resolve bind-group role metadata for one visual descriptor.
 *
 * @param visual the visual descriptor
 * @param controller_mode the visual's panel controller attachment mode
 * @param out the output bind descriptor
 * @return whether a bind descriptor was resolved
 */
bool _scene_visual_bind_desc(
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
    if (!_scene_visual_pass_caps_from_desc(
            visual, DVZ_ALPHA_OPAQUE, controller_mode, &caps))
        return false;

    switch (visual->kind)
    {
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

    case DVZ_SCENE_VISUAL_DESC_SEGMENT:
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
        out->uses_common_set0 = caps.uses_common_set;
        out->uses_fixed_common = caps.fixed_controller;
        out->uses_volume_set1 = caps.uses_volume_set;
        out->volume_texture_id = visual->volume_texture_id;
        out->volume_transfer_texture_id = visual->volume_transfer_texture_id;
        out->volume_visual_index = visual->volume_visual_index;
        out->volume_transfer_rgba = visual->volume_transfer_rgba;
        out->volume_occluded = visual->volume_occluded;
        out->volume_occlusion = visual->volume_occlusion;
        out->volume_state = visual->volume_state;
        return true;

    case DVZ_SCENE_VISUAL_DESC_NONE:
    default:
        return false;
    }
}



/**
 * Return whether a scene render node needs a depth attachment for fixed-function depth testing.
 *
 * @param emitter the persistent emitter
 * @param render the render node
 * @return whether the render node contains depth-tested geometry
 */
bool _scene_render_needs_depth(DvzFramePlanEmitter* emitter, const DvzFramePlanNode* render)
{
    ANN(emitter);
    ANN(render);
    if (render->type != DVZ_FRAME_PLAN_NODE_RENDER || render->u.render.visual_count == 0)
        return false;

    for (uint32_t i = 0; i < render->u.render.visual_count; i++)
    {
        if (render->u.render.controller_modes[i] == DVZ_CONTROLLER_FIXED)
            continue;

        const DvzFramePlanVisualMeta* meta = &render->u.render.visual_metadata[i];
        if (meta->has_metadata)
        {
            bool segment_like = meta->visual_type == DVZ_VISUAL_TYPE_SEGMENT ||
                                _visual_meta_is_stroked_path(&emitter->resources, meta);
            uint64_t pos_buf = _resource_lookup_label(
                &emitter->resources, segment_like ? meta->position_start_id : meta->position_id);
            if (pos_buf == 0)
                continue;
            bool has_color = _resource_lookup_label(&emitter->resources, meta->color_id) != 0;
            DvzScenePointLikeKind point_like_kind = DVZ_SCENE_POINT_LIKE_POINT;
            if (_visual_meta_point_like_kind(meta->visual_type, &point_like_kind))
            {
                if (has_color)
                    return true;
                continue;
            }
            if (meta->visual_type == DVZ_VISUAL_TYPE_SPHERE)
            {
                bool has_size = _resource_lookup_label(&emitter->resources, meta->size_id) != 0;
                if (has_color && has_size)
                    return true;
                continue;
            }
            if (segment_like)
            {
                bool has_end =
                    _resource_lookup_label(&emitter->resources, meta->position_end_id) != 0;
                bool has_line_width =
                    _resource_lookup_label(&emitter->resources, meta->line_width_id) != 0;
                bool has_index =
                    _resource_lookup_label(&emitter->resources, meta->index_id) != 0;
                if (has_end && has_color && has_line_width && has_index)
                    return true;
                continue;
            }
            if (!_visual_meta_is_primitive(meta->visual_type))
                continue;
            bool has_topology = _resource_topology(&emitter->resources, pos_buf) != UINT32_MAX ||
                                meta->topology != UINT32_MAX;
            if (has_color && has_topology)
                return true;
            continue;
        }

        uint64_t pos_buf = _render_visual_resource_id(
            emitter, render->u.render.visuals[i], DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION);
        if (pos_buf == 0)
            continue;

        bool has_topology = _resource_topology(&emitter->resources, pos_buf) != UINT32_MAX;
        bool has_color =
            _render_visual_resource_id(
                emitter, render->u.render.visuals[i], DVZ_FRAME_PLAN_RESOURCE_ROLE_COLOR) != 0;
        if (has_color && has_topology)
            return true;
    }

    return false;
}
