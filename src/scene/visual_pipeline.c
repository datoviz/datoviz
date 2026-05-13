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
#include "_shader_registry.h"
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
 * Resolve draw-relevant state for one encoded render visual id.
 *
 * @param emitter the persistent emitter
 * @param encoded_visual_id the render-node visual id
 * @param out the output visual descriptor
 * @return whether a supported visual descriptor was resolved
 */
bool _scene_visual_desc_from_render(
    DvzFramePlanEmitter* emitter, const char* encoded_visual_id, DvzSceneVisualDesc* out)
{
    ANN(emitter);
    ANN(out);
    dvz_memset(out, sizeof(DvzSceneVisualDesc), 0, sizeof(DvzSceneVisualDesc));

    char visual_id[DVZ_SCENE_LABEL_SIZE];
    char shared_index_id[DVZ_SCENE_LABEL_SIZE];
    _parse_visual_id(
        encoded_visual_id, visual_id, sizeof(visual_id), shared_index_id,
        sizeof(shared_index_id));

    char pos_key[DVZ_SCENE_LABEL_SIZE];
    dvz_snprintf(pos_key, sizeof(pos_key), "%s_position", visual_id);
    uint64_t pos_buf = _resource_lookup_id(&emitter->resources, pos_key);
    if (pos_buf == 0)
        return false;
    out->vbuf_ids[out->vbuf_count++] = pos_buf;

    const char* optionals[] = {
        "color", "size", "texcoords", "texture", "normal", "index", "primitive_shading"};
    for (uint32_t ai = 0; ai < 7; ai++)
    {
        char rid[DVZ_SCENE_LABEL_SIZE];
        dvz_snprintf(rid, sizeof(rid), "%s_%s", visual_id, optionals[ai]);
        uint64_t rid_id = 0;
        if (strcmp(optionals[ai], "index") == 0 && shared_index_id[0] != '\0')
            rid_id = _resource_lookup_id(&emitter->resources, shared_index_id);
        else
            rid_id = _resource_lookup_id(&emitter->resources, rid);
        if (rid_id == 0)
            continue;
        if (strcmp(optionals[ai], "index") == 0)
        {
            out->index_buffer_id = rid_id;
            continue;
        }
        if (strcmp(optionals[ai], "primitive_shading") == 0)
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
            strcmp(_resource_data_tag(&emitter->resources, out->vbuf_ids[j]), "normal") == 0;
    }

    out->topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    if (is_primitive)
        out->topology = _resource_topology(&emitter->resources, pos_buf);
    else if (is_image)
        out->topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

    if (is_point)
        out->kind = DVZ_SCENE_VISUAL_DESC_POINT;
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
    const DvzSceneVisualDesc* visual, bool picking, const char* format_tag,
    DvzSceneVisualShaderDesc* out)
{
    ANN(visual);
    ANN(format_tag);
    ANN(out);
    dvz_memset(out, sizeof(DvzSceneVisualShaderDesc), 0, sizeof(DvzSceneVisualShaderDesc));

    switch (visual->kind)
    {
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
            out->vertex_spirv_key = "point_vert";
            out->fragment_spirv_key = "point_frag";
        }
        return true;

    case DVZ_SCENE_VISUAL_DESC_PRIMITIVE:
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
        out->vertex_spirv_key = "image_vert";
        out->fragment_spirv_key = "image_frag";
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
