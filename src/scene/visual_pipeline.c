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
#include "_visual_pipeline.h"
#include "datoviz/drp2/enums.h"



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
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_COLOR:
        return "color";
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_SIZE:
        return "size";
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXCOORDS:
        return "texcoords";
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXTURE:
        return "texture";
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_NORMAL:
        return "normal";
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_INDEX:
        return "index";
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_PRIMITIVE_SHADING:
        return "primitive_shading";
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

    if (error != NULL)
        *error = NULL;

    uint64_t pos_buf = _resource_lookup_label(&emitter->resources, meta->position_id);
    if (pos_buf == 0)
    {
        if (error != NULL)
            *error = "typed visual metadata missing position resource";
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

    DvzScenePointLikeKind point_like_kind = DVZ_SCENE_POINT_LIKE_POINT;
    if (_visual_meta_point_like_kind(meta->visual_type, &point_like_kind))
    {
        uint64_t color_id = _resource_lookup_label(&emitter->resources, meta->color_id);
        uint64_t size_id = _resource_lookup_label(&emitter->resources, meta->size_id);
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
        out->kind = point_like_kind == DVZ_SCENE_POINT_LIKE_PIXEL
                        ? DVZ_SCENE_VISUAL_DESC_PIXEL
                        : DVZ_SCENE_VISUAL_DESC_POINT;
        out->point_like_kind = point_like_kind;
        out->vbuf_ids[out->vbuf_count++] = color_id;
        out->vbuf_ids[out->vbuf_count++] = size_id;
        out->topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
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
        out->shading_buffer_id = _resource_lookup_label(&emitter->resources, meta->shading_id);
    }
    else if (meta->visual_type == DVZ_VISUAL_TYPE_IMAGE)
    {
        uint64_t uv_id = _resource_lookup_label(&emitter->resources, meta->texcoords_id);
        uint64_t tex_id = _resource_lookup_label(&emitter->resources, meta->texture_id);
        if (uv_id == 0 || tex_id == 0)
        {
            if (error != NULL)
                *error = "typed image metadata missing texcoords/texture resource";
            return false;
        }
        out->kind = DVZ_SCENE_VISUAL_DESC_IMAGE;
        out->vbuf_ids[out->vbuf_count++] = uv_id;
        out->image_texture_id = tex_id;
        out->topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    }
    else if (meta->visual_type == DVZ_VISUAL_TYPE_VOLUME)
    {
        uint64_t uvw_id = _resource_lookup_label(&emitter->resources, meta->texcoords_id);
        uint64_t tex_id = _resource_lookup_label(&emitter->resources, meta->volume_texture_id);
        if (tex_id == 0)
            tex_id = _resource_lookup_label(&emitter->resources, meta->texture_id);
        if (uvw_id == 0 || tex_id == 0)
        {
            if (error != NULL)
                *error = "typed volume metadata missing texcoords/texture resource";
            return false;
        }
        out->kind = DVZ_SCENE_VISUAL_DESC_VOLUME;
        out->vbuf_ids[out->vbuf_count++] = uvw_id;
        out->volume_texture_id = tex_id;
        out->volume_state = meta->volume_state;
        out->topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
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
            if (!_append_resource_key(
                    &emitter->resources, meta->position_id, out_ids, out_count, true))
                return false;
            if (!_append_resource_key(
                    &emitter->resources, meta->color_id, out_ids, out_count, false))
                return false;
            if (!_append_resource_key(
                    &emitter->resources, meta->size_id, out_ids, out_count, false))
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
         * POINT      = position, color, size
         * PRIMITIVE  = position, color
         * IMAGE      = position, texcoords (+ texture, registered alongside). */
        const DvzFramePlanResourceRole optional[] = {
            DVZ_FRAME_PLAN_RESOURCE_ROLE_COLOR, DVZ_FRAME_PLAN_RESOURCE_ROLE_SIZE,
            DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXCOORDS, DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXTURE};
        for (uint32_t ai = 0; ai < 4; ai++)
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
        return _resource_lookup_label(&emitter->resources, meta->position_id) != 0;

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
        DVZ_FRAME_PLAN_RESOURCE_ROLE_PRIMITIVE_SHADING};
    for (uint32_t ai = 0; ai < 7; ai++)
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
        if (optionals[ai] == DVZ_FRAME_PLAN_RESOURCE_ROLE_PRIMITIVE_SHADING)
        {
            out->shading_buffer_id = rid_id;
            continue;
        }
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

    switch (visual->kind)
    {
    case DVZ_SCENE_VISUAL_DESC_PIXEL:
        dvz_snprintf(out->vertex_key, sizeof(out->vertex_key), "_vs_pixel%s", format_tag);
        dvz_snprintf(out->fragment_key, sizeof(out->fragment_key), "_fs_pixel%s", format_tag);
        dvz_snprintf(out->pipeline_key, sizeof(out->pipeline_key), "_pipe_pixel%s", format_tag);
        out->vertex_glsl = _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_PIXEL, false);
        out->fragment_glsl = _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_PIXEL, true);
        out->vertex_wgsl = _builtin_shader_wgsl(DVZ_SCENE_BUILTIN_SHADER_PIXEL, false);
        out->fragment_wgsl = _builtin_shader_wgsl(DVZ_SCENE_BUILTIN_SHADER_PIXEL, true);
        out->vertex_spirv_key = "pixel_vert";
        out->fragment_spirv_key = "pixel_frag";
        return true;

    case DVZ_SCENE_VISUAL_DESC_POINT:
        if (picking)
        {
            dvz_snprintf(out->vertex_key, sizeof(out->vertex_key), "_vs_point_pick%s", format_tag);
            dvz_snprintf(
                out->fragment_key, sizeof(out->fragment_key), "_fs_point_pick%s", format_tag);
            dvz_snprintf(
                out->pipeline_key, sizeof(out->pipeline_key), "_pipe_point_pick%s", format_tag);
            out->vertex_glsl =
                _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_POINT_PICK, false);
            out->fragment_glsl =
                _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_POINT_PICK, true);
        }
        else
        {
            dvz_snprintf(out->vertex_key, sizeof(out->vertex_key), "_vs_point%s", format_tag);
            dvz_snprintf(out->fragment_key, sizeof(out->fragment_key), "_fs_point%s", format_tag);
            dvz_snprintf(out->pipeline_key, sizeof(out->pipeline_key), "_pipe_point%s", format_tag);
            out->vertex_glsl = _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_POINT, false);
            out->fragment_glsl = _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_POINT, true);
            out->vertex_wgsl = _builtin_shader_wgsl(DVZ_SCENE_BUILTIN_SHADER_POINT, false);
            out->fragment_wgsl = _builtin_shader_wgsl(DVZ_SCENE_BUILTIN_SHADER_POINT, true);
            out->vertex_spirv_key = "point_vert";
            out->fragment_spirv_key = "point_frag";
        }
        return true;

    case DVZ_SCENE_VISUAL_DESC_PRIMITIVE:
        if (wboit_accumulation)
        {
            DvzSceneBuiltinShader shader = visual->has_normal
                                               ? DVZ_SCENE_BUILTIN_SHADER_WBOIT_ACCUM_LIT
                                               : DVZ_SCENE_BUILTIN_SHADER_WBOIT_ACCUM;
            dvz_snprintf(
                out->vertex_key, sizeof(out->vertex_key), "_vs_wboit_accum_n%u%s",
                visual->has_normal ? 1u : 0u, format_tag);
            dvz_snprintf(
                out->fragment_key, sizeof(out->fragment_key), "_fs_wboit_accum_n%u%s",
                visual->has_normal ? 1u : 0u, format_tag);
            dvz_snprintf(
                out->pipeline_key, sizeof(out->pipeline_key), "_pipe_wboit_accum_t%u_n%u%s",
                visual->topology, visual->has_normal ? 1u : 0u, format_tag);
            out->vertex_glsl = _builtin_shader_glsl(shader, false);
            out->fragment_glsl = _builtin_shader_glsl(shader, true);
            return true;
        }
        if (visual->has_normal)
        {
            dvz_snprintf(out->vertex_key, sizeof(out->vertex_key), "_vs_prim_lit%s", format_tag);
            dvz_snprintf(
                out->fragment_key, sizeof(out->fragment_key), "_fs_prim_lit%s", format_tag);
            dvz_snprintf(
                out->pipeline_key, sizeof(out->pipeline_key), "_pipe_prim_lit_t%u%s",
                visual->topology, format_tag);
            out->vertex_glsl =
                _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE_LIT, false);
            out->fragment_glsl =
                _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE_LIT, true);
        }
        else
        {
            dvz_snprintf(out->vertex_key, sizeof(out->vertex_key), "_vs_prim%s", format_tag);
            dvz_snprintf(out->fragment_key, sizeof(out->fragment_key), "_fs_prim%s", format_tag);
            dvz_snprintf(
                out->pipeline_key, sizeof(out->pipeline_key), "_pipe_prim_t%u%s",
                visual->topology, format_tag);
            out->vertex_glsl = _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE, false);
            out->fragment_glsl = _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE, true);
            out->vertex_wgsl = _builtin_shader_wgsl(DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE, false);
            out->fragment_wgsl = _builtin_shader_wgsl(DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE, true);
            out->vertex_spirv_key = "primitive_vert";
            out->fragment_spirv_key = "primitive_frag";
        }
        return true;

    case DVZ_SCENE_VISUAL_DESC_IMAGE:
        dvz_snprintf(out->vertex_key, sizeof(out->vertex_key), "_vs_img%s", format_tag);
        dvz_snprintf(out->fragment_key, sizeof(out->fragment_key), "_fs_img%s", format_tag);
        dvz_snprintf(out->pipeline_key, sizeof(out->pipeline_key), "_pipe_img%s", format_tag);
        out->vertex_glsl = _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_IMAGE, false);
        out->fragment_glsl = _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_IMAGE, true);
        out->vertex_wgsl = _builtin_shader_wgsl(DVZ_SCENE_BUILTIN_SHADER_IMAGE, false);
        out->fragment_wgsl = _builtin_shader_wgsl(DVZ_SCENE_BUILTIN_SHADER_IMAGE, true);
        out->vertex_spirv_key = "image_vert";
        out->fragment_spirv_key = "image_frag";
        return true;

    case DVZ_SCENE_VISUAL_DESC_VOLUME:
        dvz_snprintf(out->vertex_key, sizeof(out->vertex_key), "_vs_vol_slice%s", format_tag);
        dvz_snprintf(out->fragment_key, sizeof(out->fragment_key), "_fs_vol_slice%s", format_tag);
        dvz_snprintf(out->pipeline_key, sizeof(out->pipeline_key), "_pipe_vol_slice%s", format_tag);
        out->vertex_glsl = _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_VOLUME_SLICE, false);
        out->fragment_glsl = _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_VOLUME_SLICE, true);
        out->vertex_spirv_key = "volume_slice_vert";
        out->fragment_spirv_key = "volume_slice_frag";
        return true;

    case DVZ_SCENE_VISUAL_DESC_NONE:
    default:
        return false;
    }
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

    out->topology = visual->topology;
    out->has_depth_state = pass_needs_depth;
    if (pass_needs_depth)
    {
        out->depth_write_enabled = false;
        out->depth_compare_op = VK_COMPARE_OP_ALWAYS;
    }

    switch (visual->kind)
    {
    case DVZ_SCENE_VISUAL_DESC_PIXEL:
    case DVZ_SCENE_VISUAL_DESC_POINT:
        out->vertex_buffer_count = 3;
        out->binding_count = 3;
        out->attr_count = picking ? 2 : 3;
        out->strides[0] = 3 * sizeof(float);
        out->strides[1] = 4 * sizeof(uint8_t);
        out->strides[2] = sizeof(float);
        out->bindings[0] = 0;
        out->bindings[1] = picking ? 2 : 1;
        out->bindings[2] = 2;
        out->locations[0] = 0;
        out->locations[1] = picking ? 2 : 1;
        out->locations[2] = 2;
        out->formats[0] = VK_FORMAT_R32G32B32_SFLOAT;
        out->formats[1] = picking ? VK_FORMAT_R32_SFLOAT : VK_FORMAT_R8G8B8A8_UNORM;
        out->formats[2] = VK_FORMAT_R32_SFLOAT;
        out->needs_common_layout = true;
        return true;

    case DVZ_SCENE_VISUAL_DESC_PRIMITIVE:
        out->vertex_buffer_count = visual->has_normal ? 3 : 2;
        out->binding_count = out->vertex_buffer_count;
        out->attr_count = out->vertex_buffer_count;
        out->strides[0] = 3 * sizeof(float);
        out->strides[1] = 4 * sizeof(uint8_t);
        out->strides[2] = 3 * sizeof(float);
        out->bindings[0] = 0;
        out->bindings[1] = 1;
        out->bindings[2] = 2;
        out->locations[0] = 0;
        out->locations[1] = 1;
        out->locations[2] = 2;
        out->formats[0] = VK_FORMAT_R32G32B32_SFLOAT;
        out->formats[1] = VK_FORMAT_R8G8B8A8_UNORM;
        out->formats[2] = VK_FORMAT_R32G32B32_SFLOAT;
        out->needs_common_layout = true;
        out->needs_shading_layout = visual->has_normal;
        if (pass_needs_depth)
        {
            bool fixed = controller_mode == DVZ_CONTROLLER_FIXED;
            out->depth_write_enabled =
                !fixed && !wboit_accumulation && alpha_mode != DVZ_ALPHA_BLENDED;
            out->depth_compare_op = fixed ? VK_COMPARE_OP_ALWAYS : VK_COMPARE_OP_LESS_OR_EQUAL;
        }
        return true;

    case DVZ_SCENE_VISUAL_DESC_IMAGE:
        out->vertex_buffer_count = 2;
        out->binding_count = 2;
        out->attr_count = 2;
        out->strides[0] = 3 * sizeof(float);
        out->strides[1] = 2 * sizeof(float);
        out->bindings[0] = 0;
        out->bindings[1] = 1;
        out->locations[0] = 0;
        out->locations[1] = 1;
        out->formats[0] = VK_FORMAT_R32G32B32_SFLOAT;
        out->formats[1] = VK_FORMAT_R32G32_SFLOAT;
        out->needs_common_layout = true;
        out->needs_image_layout = true;
        return true;

    case DVZ_SCENE_VISUAL_DESC_VOLUME:
        out->vertex_buffer_count = 2;
        out->binding_count = 2;
        out->attr_count = 2;
        out->strides[0] = 3 * sizeof(float);
        out->strides[1] = 3 * sizeof(float);
        out->bindings[0] = 0;
        out->bindings[1] = 1;
        out->locations[0] = 0;
        out->locations[1] = 1;
        out->formats[0] = VK_FORMAT_R32G32B32_SFLOAT;
        out->formats[1] = VK_FORMAT_R32G32B32_SFLOAT;
        out->needs_common_layout = true;
        out->needs_volume_layout = true;
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

    switch (visual->kind)
    {
    case DVZ_SCENE_VISUAL_DESC_PIXEL:
    case DVZ_SCENE_VISUAL_DESC_POINT:
        out->uses_common_set0 = true;
        out->uses_fixed_common = controller_mode == DVZ_CONTROLLER_FIXED;
        return true;

    case DVZ_SCENE_VISUAL_DESC_PRIMITIVE:
        out->uses_common_set0 = true;
        out->uses_fixed_common = controller_mode == DVZ_CONTROLLER_FIXED;
        out->uses_shading_set1 = visual->has_normal && visual->shading_buffer_id != 0;
        out->shading_buffer_id = visual->shading_buffer_id;
        return true;

    case DVZ_SCENE_VISUAL_DESC_IMAGE:
        out->uses_common_set0 = true;
        out->uses_fixed_common = controller_mode == DVZ_CONTROLLER_FIXED;
        out->uses_image_set1 = true;
        out->image_texture_id = visual->image_texture_id;
        return true;

    case DVZ_SCENE_VISUAL_DESC_VOLUME:
        out->uses_common_set0 = true;
        out->uses_fixed_common = controller_mode == DVZ_CONTROLLER_FIXED;
        out->uses_volume_set1 = true;
        out->volume_texture_id = visual->volume_texture_id;
        out->volume_state = visual->volume_state;
        return true;

    case DVZ_SCENE_VISUAL_DESC_NONE:
    default:
        return false;
    }
}



/**
 * Return whether a scene render node needs a depth attachment.
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
            if (!_visual_meta_is_primitive(meta->visual_type))
                continue;
            uint64_t pos_buf = _resource_lookup_label(&emitter->resources, meta->position_id);
            if (pos_buf == 0)
                continue;
            bool has_topology = _resource_topology(&emitter->resources, pos_buf) != UINT32_MAX ||
                                meta->topology != UINT32_MAX;
            bool has_color = _resource_lookup_label(&emitter->resources, meta->color_id) != 0;
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
