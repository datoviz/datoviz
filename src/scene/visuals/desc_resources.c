/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene visual resource resolution                                                           */
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
 * Return the untyped compatibility data tag for a typed resource role.
 *
 * @param role the typed resource role
 * @return the compatibility data tag, or NULL when the role has no tag fallback
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
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION_NEXT:
        return "position_next";
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_COLOR:
        return "color";
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_SIZE:
        return "size";
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_SIGMA:
        return "sigma";
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
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_ITEM_STATE:
        return "item_state";
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_ITEM_STATE_STYLE:
        return "item_state_style";
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
 * Resolve the untyped compatibility resource key for one encoded render visual and role.
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
 * Resolve one untyped compatibility render-visual resource id by role.
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
 * Find the first visual resource with a typed role, falling back to compatibility tags.
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
            bool stroked_path = _scene_visual_meta_is_stroked_path(&emitter->resources, meta);
            bool segment_like = _scene_visual_meta_renderable_kind(&emitter->resources, meta) ==
                                DVZ_RENDERABLE_STROKE_QUAD;
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
                if (!_append_resource_key(
                        &emitter->resources, meta->position_next_id, out_ids, out_count, true))
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
                    &emitter->resources, meta->sigma_id, out_ids, out_count, false))
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

        return false;
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
        if (
            _scene_visual_meta_desc_kind(&emitter->resources, meta) ==
                DVZ_SCENE_VISUAL_DESC_NONE &&
            _scene_visual_meta_renderable_kind(&emitter->resources, meta) == DVZ_RENDERABLE_NONE)
        {
            return false;
        }
        bool stroked_path = _scene_visual_meta_is_stroked_path(&emitter->resources, meta);
        bool segment_like = _scene_visual_meta_renderable_kind(&emitter->resources, meta) ==
                            DVZ_RENDERABLE_STROKE_QUAD;
        return _scene_visual_resource_lookup_label(
                   &emitter->resources,
                   (segment_like || stroked_path) ? meta->position_start_id : meta->position_id) !=
               0;
    }
    return false;
}
