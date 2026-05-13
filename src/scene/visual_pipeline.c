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

#include "_assertions.h"
#include "_compat.h"
#include "_visual_pipeline.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

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
        char visual_id[DVZ_SCENE_LABEL_SIZE];
        char shared_index_id[DVZ_SCENE_LABEL_SIZE];
        _parse_visual_id(
            render->u.render.visuals[i], visual_id, sizeof(visual_id), shared_index_id,
            sizeof(shared_index_id));
        /* "position" is always required. Other attrs are family-dependent and optional. */
        char pos_id[DVZ_SCENE_LABEL_SIZE];
        dvz_snprintf(pos_id, sizeof(pos_id), "%s_position", visual_id);
        uint64_t pos = _resource_lookup_id(&emitter->resources, pos_id);
        if (pos == 0)
            return false;
        if (*out_count >= DVZ_SCENE_MAX_NODE_RESOURCES)
            return false;
        out_ids[(*out_count)++] = pos;

        /* Optional attrs - collect any that exist. Order matches family pipeline expectations:
         * POINT      = position, color, size
         * PRIMITIVE  = position, color
         * IMAGE      = position, texcoords (+ texture, registered alongside). */
        const char* optional[] = {"color", "size", "texcoords", "texture"};
        for (uint32_t ai = 0; ai < 4; ai++)
        {
            char rid[DVZ_SCENE_LABEL_SIZE];
            dvz_snprintf(rid, sizeof(rid), "%s_%s", visual_id, optional[ai]);
            uint64_t id = _resource_lookup_id(&emitter->resources, rid);
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
        char visual_id[DVZ_SCENE_LABEL_SIZE];
        char shared_index_id[DVZ_SCENE_LABEL_SIZE];
        _parse_visual_id(
            render->u.render.visuals[i], visual_id, sizeof(visual_id), shared_index_id,
            sizeof(shared_index_id));

        char pos_key[DVZ_SCENE_LABEL_SIZE];
        dvz_snprintf(pos_key, sizeof(pos_key), "%s_position", visual_id);
        uint64_t pos_buf = _resource_lookup_id(&emitter->resources, pos_key);
        if (pos_buf == 0)
            continue;

        bool has_color = false;
        bool has_topology = _resource_topology(&emitter->resources, pos_buf) != UINT32_MAX;
        bool has_normal = false;
        for (uint32_t ai = 0; ai < 2; ai++)
        {
            const char* tag = ai == 0 ? "color" : "normal";
            char rid[DVZ_SCENE_LABEL_SIZE];
            dvz_snprintf(rid, sizeof(rid), "%s_%s", visual_id, tag);
            uint64_t attr_id = _resource_lookup_id(&emitter->resources, rid);
            if (attr_id == 0)
                continue;
            has_color = has_color || strcmp(tag, "color") == 0;
            has_normal = has_normal || strcmp(tag, "normal") == 0;
        }
        if (has_color && has_topology && has_normal)
            return true;
    }

    return false;
}
