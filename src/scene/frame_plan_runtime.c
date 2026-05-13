/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene FramePlan runtime emission                                                             */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <vulkan/vulkan_core.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_frame_plan_emit.h"
#include "_frame_plan.h"
#include "_overflow.h"
#include "_shader_registry.h"
#include "datoviz/drp2.h"
#include "datoviz/drp2/stream.h"
#include "datoviz/math/_cglm.h"
#include "datoviz/scene.h"
#include "datoviz/scene/panzoom.h"
#include "_scene.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/*
 * Return true when vertex_buffer_ids[0..n-1] carry data_tags "position", "color", "size"
 * (in any order), which identifies a DvzPoint visual.
 */
static bool _is_point_visual(
    const ConverterState* state, const uint64_t* ids, uint32_t n)
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
static bool _is_primitive_visual(
    const ConverterState* state, const uint64_t* ids, uint32_t n)
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
static bool _is_image_visual(
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
 * Emit runtime-mode upload commands.
 *
 * @param emitter the persistent emitter
 * @param stream the DRP2 command stream
 * @param node the upload node
 * @return whether the commands were emitted
 */
static bool _emitter_emit_upload(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlanNode* node,
    uint64_t* out_id)
{
    ANN(emitter);
    ANN(stream);
    ANN(node);
    ANN(out_id);

    /* Texture upload: routed when texture_width > 0 (RGBA8 2D). */
    if (node->u.upload.texture_width > 0 && node->u.upload.texture_height > 0)
    {
        bool is_new = false;
        ResourceId* resource =
            _resource_entry(&emitter->resources, node->u.upload.resource_id, &is_new);
        if (resource == NULL)
            return false;
        dvz_strlcpy(resource->data_tag, node->u.upload.data_tag, sizeof(resource->data_tag));
        resource->byte_size = node->u.upload.byte_size;
        uint64_t id = resource->id;
        uint32_t w  = node->u.upload.texture_width;
        uint32_t h  = node->u.upload.texture_height;
        uint32_t bpr = w * 4;
        if (is_new)
        {
            uint32_t usage =
                DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING | DVZ_DRP2_TEXTURE_USAGE_COPY_DST;
            if (!dvz_drp2_stream_create_texture_2d_usage(stream, id, w, h, usage))
                return false;
        }
        if (emitter->resources.first_texture_id == 0)
            emitter->resources.first_texture_id = id;
        *out_id = id;
        if (node->u.upload.data == NULL)
            return false;
        if (node->u.upload.texture_origin_x == 0 && node->u.upload.texture_origin_y == 0)
            return dvz_drp2_stream_write_texture_2d_bytes(
                stream, id, 0, w, h, bpr, h, node->u.upload.data);
        return dvz_drp2_stream_write_texture_2d_region_bytes(
            stream, id, 0, node->u.upload.texture_origin_x, node->u.upload.texture_origin_y, w, h,
            bpr, h, node->u.upload.data);
    }

    uint64_t buffer_size = 0;
    if (_dvz_add_u64_overflows(node->u.upload.byte_offset, node->u.upload.byte_size, &buffer_size))
        return false;

    bool is_new = false;
    ResourceId* resource =
        _resource_entry(&emitter->resources, node->u.upload.resource_id, &is_new);
    if (resource == NULL)
        return false;
    if (!_resource_ensure_byte_size(&emitter->resources, resource, buffer_size, &is_new))
        return false;

    dvz_strlcpy(resource->data_tag, node->u.upload.data_tag, sizeof(resource->data_tag));
    resource->usage = node->u.upload.buffer_usage;
    resource->item_stride = node->u.upload.item_stride;
    if (node->u.upload.topology != UINT32_MAX)
        resource->topology = node->u.upload.topology;
    uint64_t id = resource->id;
    uint32_t usage = node->u.upload.buffer_usage != 0
                         ? node->u.upload.buffer_usage
                         : (DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_VERTEX);
    if (is_new && !dvz_drp2_stream_create_buffer(stream, id, buffer_size, usage))
        return false;
    if (emitter->resources.first_vertex_buffer_id == 0)
        emitter->resources.first_vertex_buffer_id = id;
    *out_id = id;

    if (node->u.upload.data != NULL)
    {
        /* Real vertex data provided — encode directly into the stream. */
        return dvz_drp2_stream_write_buffer_bytes(
            stream, id, node->u.upload.byte_offset, node->u.upload.byte_size,
            node->u.upload.data);
    }
    else
    {
        /* No data: write zeros (placeholder / test path). */
        char* zero_data = _zero_base64_alloc(node->u.upload.byte_size);
        if (zero_data == NULL)
            return false;
        bool ok = dvz_drp2_stream_write_buffer(
            stream, id, node->u.upload.byte_offset, node->u.upload.byte_size, zero_data);
        dvz_free(zero_data);
        return ok;
    }
}


/**
 * Emit runtime-mode texture upload commands.
 *
 * @param emitter the persistent emitter
 * @param stream the DRP2 command stream
 * @param node the upload node
 * @param out_id the emitted texture id
 * @return whether the commands were emitted
 */
static bool _emitter_emit_texture_upload(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlanNode* node,
    uint64_t* out_id)
{
    ANN(emitter);
    ANN(stream);
    ANN(node);
    ANN(out_id);

    bool exists = false;
    for (uint32_t i = 0; i < emitter->resources.count; i++)
    {
        if (strcmp(emitter->resources.resources[i].key, node->u.upload.resource_id) == 0)
        {
            exists = true;
            break;
        }
    }

    uint64_t id = _resource_id(&emitter->resources, node->u.upload.resource_id);
    if (id == 0)
        return false;

    char* data = _zero_base64_alloc(node->u.upload.byte_size);
    if (data == NULL)
        return false;

    uint32_t usage = DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING | DVZ_DRP2_TEXTURE_USAGE_COPY_DST;
    if (!exists && !dvz_drp2_stream_create_texture_2d_usage(stream, id, 2, 2, usage))
    {
        dvz_free(data);
        return false;
    }
    if (emitter->resources.first_texture_id == 0)
        emitter->resources.first_texture_id = id;
    *out_id = id;
    bool ok = dvz_drp2_stream_write_texture_2d(stream, id, 0, 2, 2, 8, 2, data);
    dvz_free(data);
    return ok;
}



/**
 * Emit runtime-mode compute input/output buffer commands.
 *
 * @param emitter the persistent emitter
 * @param stream the DRP2 command stream
 * @param upload the upload node backing the compute input
 * @param compute the compute node naming the output resource
 * @return whether the commands were emitted
 */
static bool _emitter_emit_compute_buffers(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlanNode* upload,
    const DvzFramePlanNode* compute)
{
    ANN(emitter);
    ANN(stream);
    ANN(upload);
    ANN(compute);
    if (compute->u.compute.write_count == 0)
        return false;

    uint64_t input_size = 0;
    if (_dvz_add_u64_overflows(
            upload->u.upload.byte_offset, upload->u.upload.byte_size, &input_size))
        return false;

    bool input_create = false;
    bool output_create = false;
    ResourceId* input =
        _resource_entry(&emitter->resources, upload->u.upload.resource_id, &input_create);
    ResourceId* output =
        _resource_entry(&emitter->resources, compute->u.compute.writes[0], &output_create);
    if (input == NULL || output == NULL)
        return false;
    if (!_resource_ensure_byte_size(&emitter->resources, input, input_size, &input_create))
        return false;
    if (!_resource_ensure_byte_size(&emitter->resources, output, input_size, &output_create))
        return false;

    uint64_t input_id = input->id;
    uint64_t output_id = output->id;
    emitter->resources.first_compute_input_id = input_id;
    emitter->resources.first_compute_output_id = output_id;
    emitter->resources.first_vertex_buffer_id = output_id;
    emitter->resources.compute_buffer_size = input_size;

    char* data = _zero_base64_alloc(upload->u.upload.byte_size);
    if (data == NULL)
        return false;

    bool ok = true;
    if (input_create)
    {
        ok = ok && dvz_drp2_stream_create_buffer(
                       stream, input_id, input_size,
                       DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_STORAGE);
    }
    ok = ok && dvz_drp2_stream_write_buffer(
                   stream, input_id, upload->u.upload.byte_offset, upload->u.upload.byte_size,
                   data);
    if (output_create)
    {
        ok = ok && dvz_drp2_stream_create_buffer(
                       stream, output_id, input_size,
                       DVZ_DRP2_BUFFER_USAGE_STORAGE | DVZ_DRP2_BUFFER_USAGE_VERTEX);
    }
    dvz_free(data);
    return ok;
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
static bool _emitter_resolve_render_vertex_buffers(
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

        /* Optional attrs — collect any that exist. Order matches family pipeline expectations:
         * POINT      = position, color, size
         * PRIMITIVE  = position, color
         * IMAGE      = position, texcoords (+ texture, registered alongside).
         */
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
static bool _scene_render_needs_depth(
    DvzFramePlanEmitter* emitter, const DvzFramePlanNode* render)
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



/* Scene render path: one panel's draws emitted inside an already-open render pass. */
static bool _emitter_emit_render_multi_in_pass(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlanNode* render,
    uint64_t render_pass_id, const DvzFramePlanEmitConfig* cfg, SceneRenderStateCache* cache)
{
    ANN(emitter);
    ANN(stream);
    ANN(render);

    bool ok = true;
    bool is_new = false;
    const char* fmt = _shader_format_tag(cfg);
    bool pass_needs_depth = _scene_render_needs_depth(emitter, render);

    /* --- MVP UBO infrastructure (one BGL shared across all panels) --- */
    uint64_t mvp_bgl_id = _obj_id(emitter, "_bgl_mvp", &is_new);
    if (mvp_bgl_id == 0)
        return false;
    if (is_new)
        ok = ok && dvz_drp2_stream_create_uniform_bind_group_layout(stream, mvp_bgl_id);

    /* Determine which modes are needed for this panel. */
    bool needs_apply = false, needs_fixed = false;
    for (uint32_t i = 0; i < render->u.render.visual_count; i++)
    {
        if (render->u.render.controller_modes[i] == DVZ_CONTROLLER_FIXED)
            needs_fixed = true;
        else
            needs_apply = true;
    }

    uint64_t apply_bg_id = 0, fixed_bg_id = 0;

    if (needs_apply && ok)
    {
        char buf_key[128], bg_key[128], slot_key[128];
        dvz_snprintf(buf_key, sizeof(buf_key), "_mvp_buf_%s_apply", render->u.render.panel_id);
        dvz_snprintf(bg_key, sizeof(bg_key), "_mvp_bg_%s_apply", render->u.render.panel_id);
        dvz_snprintf(slot_key, sizeof(slot_key), "%s_apply", render->u.render.panel_id);

        uint64_t buf_id = _obj_id(emitter, buf_key, &is_new);
        if (buf_id == 0)
            return false;
        if (is_new)
        {
            uint32_t usage = DVZ_DRP2_BUFFER_USAGE_UNIFORM | DVZ_DRP2_BUFFER_USAGE_MAP_WRITE |
                             DVZ_DRP2_BUFFER_USAGE_COPY_DST;
            ok = ok && dvz_drp2_stream_create_buffer(stream, buf_id, sizeof(DvzMVP), usage);
        }
        uint64_t bg_id = _obj_id(emitter, bg_key, &is_new);
        if (bg_id == 0)
            return false;
        if (ok && is_new)
            ok = ok && dvz_drp2_stream_create_uniform_bind_group(
                           stream, bg_id, mvp_bgl_id, buf_id, 0, sizeof(DvzMVP));

        DvzMVP* slot = _emitter_mvp_slot(emitter, slot_key);
        if (slot != NULL)
            *slot = render->u.render.apply_mvp;
        ok = ok && dvz_drp2_stream_write_buffer_bytes(
                       stream, buf_id, 0, sizeof(DvzMVP),
                       slot ? slot : &render->u.render.apply_mvp);
        apply_bg_id = bg_id;
    }

    if (needs_fixed && ok)
    {
        const char* buf_key = "_mvp_buf_fixed";
        const char* bg_key = "_mvp_bg_fixed";
        const char* slot_key = "_fixed";

        uint64_t buf_id = _obj_id(emitter, buf_key, &is_new);
        if (buf_id == 0)
            return false;
        if (is_new)
        {
            uint32_t usage = DVZ_DRP2_BUFFER_USAGE_UNIFORM | DVZ_DRP2_BUFFER_USAGE_MAP_WRITE |
                             DVZ_DRP2_BUFFER_USAGE_COPY_DST;
            ok = ok && dvz_drp2_stream_create_buffer(stream, buf_id, sizeof(DvzMVP), usage);
        }
        uint64_t bg_id = _obj_id(emitter, bg_key, &is_new);
        if (bg_id == 0)
            return false;
        if (ok && is_new)
            ok = ok && dvz_drp2_stream_create_uniform_bind_group(
                           stream, bg_id, mvp_bgl_id, buf_id, 0, sizeof(DvzMVP));

        DvzMVP* slot = _emitter_mvp_slot(emitter, slot_key);
        if (slot != NULL)
        {
            glm_mat4_identity(slot->model);
            glm_mat4_identity(slot->view);
            glm_mat4_identity(slot->proj);
            slot->time  = 0.0f;
            slot->flags = 0;
        }
        DvzMVP local_identity = {0};
        glm_mat4_identity(local_identity.model);
        glm_mat4_identity(local_identity.view);
        glm_mat4_identity(local_identity.proj);
        ok = ok && dvz_drp2_stream_write_buffer_bytes(
                       stream, buf_id, 0, sizeof(DvzMVP),
                       slot ? slot : &local_identity);
        fixed_bg_id = bg_id;
    }

    /* Image BGL + sampler (shared, created lazily on first image visual). */
    uint64_t img_bgl_id = 0, img_sampler_id = 0;

    /* Per-visual draw descriptors. */
    struct {
        uint64_t pipeline_id;
        uint64_t bg_set0;  /* MVP bg (point/prim) or texture bg (image); 0 = none */
        uint64_t bg_set1;  /* primitive shading bg; 0 = none */
        uint64_t vbuf_ids[DVZ_SCENE_MAX_NODE_RESOURCES];
        uint32_t vbuf_count;
        uint64_t index_buffer_id;
        uint32_t index_count;
        const char* index_format;
        uint32_t vertex_count;
    } draws[DVZ_SCENE_MAX_RENDER_VISUALS];
    uint32_t draw_count = 0;

    for (uint32_t i = 0; ok && i < render->u.render.visual_count; i++)
    {
        char visual_id[DVZ_SCENE_LABEL_SIZE];
        char shared_index_id[DVZ_SCENE_LABEL_SIZE];
        _parse_visual_id(
            render->u.render.visuals[i], visual_id, sizeof(visual_id), shared_index_id,
            sizeof(shared_index_id));

        /* Resolve vertex buffers for this visual. */
        uint64_t vbuf_ids[DVZ_SCENE_MAX_NODE_RESOURCES] = {0};
        uint32_t vbuf_count = 0;
        uint64_t index_buf = 0;
        uint64_t shading_buf = 0;

        char pos_key[DVZ_SCENE_LABEL_SIZE];
        dvz_snprintf(pos_key, sizeof(pos_key), "%s_position", visual_id);
        uint64_t pos_buf = _resource_lookup_id(&emitter->resources, pos_key);
        if (pos_buf == 0)
            continue;
        vbuf_ids[vbuf_count++] = pos_buf;

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
                index_buf = rid_id;
                continue;
            }
            if (strcmp(optionals[ai], "primitive_shading") == 0)
            {
                shading_buf = rid_id;
                continue;
            }
            if (vbuf_count >= DVZ_SCENE_MAX_NODE_RESOURCES)
                continue;
            vbuf_ids[vbuf_count++] = rid_id;
        }

        /* Detect visual family. */
        bool vis_is_point = _is_point_visual(&emitter->resources, vbuf_ids, vbuf_count);
        bool vis_is_prim =
            !vis_is_point && _is_primitive_visual(&emitter->resources, vbuf_ids, vbuf_count);
        uint64_t img_pos = 0, img_uv = 0, img_tex = 0;
        bool vis_is_image =
            !vis_is_point && !vis_is_prim &&
            _is_image_visual(&emitter->resources, vbuf_ids, vbuf_count, &img_pos, &img_uv, &img_tex);

        if (!vis_is_point && !vis_is_prim && !vis_is_image)
            continue;

        /* Vertex count from position buffer. */
        uint64_t pos_sz = _resource_byte_size(&emitter->resources, pos_buf);
        uint32_t vertex_count = (pos_sz > 0) ? (uint32_t)(pos_sz / (3 * sizeof(float))) : 3;

        bool has_normal = false;
        for (uint32_t j = 0; j < vbuf_count; j++)
            has_normal = has_normal ||
                         strcmp(_resource_data_tag(&emitter->resources, vbuf_ids[j]), "normal") == 0;

        /* Topology. */
        uint32_t topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        if (vis_is_prim)
            topology = _resource_topology(&emitter->resources, pos_buf);
        else if (vis_is_image)
            topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

        /* Shader keys. */
        char vs_key[32], fs_key[16], pipe_key[48];
        const char* vs_glsl      = NULL;
        const char* fs_glsl      = NULL;
        const char* vs_spirv_key = NULL;
        const char* fs_spirv_key = NULL;

        if (vis_is_point)
        {
            if (render->u.render.picking)
            {
                dvz_snprintf(vs_key, sizeof(vs_key), "_vs_point_pick%s", fmt);
                dvz_snprintf(fs_key, sizeof(fs_key), "_fs_point_pick%s", fmt);
                vs_glsl = _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_POINT_PICK, false);
                fs_glsl = _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_POINT_PICK, true);
            }
            else
            {
                dvz_snprintf(vs_key, sizeof(vs_key), "_vs_point%s", fmt);
                dvz_snprintf(fs_key, sizeof(fs_key), "_fs_point%s", fmt);
                vs_glsl      = _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_POINT, false);
                fs_glsl      = _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_POINT, true);
                vs_spirv_key = "point_vert";
                fs_spirv_key = "point_frag";
            }
        }
        else if (vis_is_prim)
        {
            if (has_normal)
            {
                dvz_snprintf(vs_key, sizeof(vs_key), "_vs_prim_lit%s", fmt);
                dvz_snprintf(fs_key, sizeof(fs_key), "_fs_prim_lit%s", fmt);
                vs_glsl = _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE_LIT, false);
                fs_glsl = _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE_LIT, true);
            }
            else
            {
                dvz_snprintf(vs_key, sizeof(vs_key), "_vs_prim%s", fmt);
                dvz_snprintf(fs_key, sizeof(fs_key), "_fs_prim%s", fmt);
                vs_glsl      = _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE, false);
                fs_glsl      = _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE, true);
                vs_spirv_key = "primitive_vert";
                fs_spirv_key = "primitive_frag";
            }
        }
        else /* vis_is_image */
        {
            dvz_snprintf(vs_key, sizeof(vs_key), "_vs_img%s", fmt);
            dvz_snprintf(fs_key, sizeof(fs_key), "_fs_img%s", fmt);
            vs_glsl      = _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_IMAGE, false);
            fs_glsl      = _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_IMAGE, true);
            vs_spirv_key = "image_vert";
            fs_spirv_key = "image_frag";
        }

        /* Shaders (cached). */
        uint64_t vs_id = _obj_id(emitter, vs_key, &is_new);
        if (vs_id == 0) { ok = false; break; }
        if (is_new)
        {
            if (vs_spirv_key != NULL)
                ok = ok && _emit_shader_spirv(stream, vs_id, "VERTEX", vs_spirv_key, vs_glsl, cfg);
            else
                ok = ok && _emit_shader(stream, vs_id, "VERTEX", NULL, vs_glsl, cfg);
        }

        uint64_t fs_id = _obj_id(emitter, fs_key, &is_new);
        if (fs_id == 0) { ok = false; break; }
        if (ok && is_new)
        {
            if (fs_spirv_key != NULL)
                ok = ok && _emit_shader_spirv(stream, fs_id, "FRAGMENT", fs_spirv_key, fs_glsl, cfg);
            else
                ok = ok && _emit_shader(stream, fs_id, "FRAGMENT", NULL, fs_glsl, cfg);
        }

        /* Pipeline (cached by family + topology). */
        if (vis_is_point)
        {
            dvz_snprintf(
                pipe_key, sizeof(pipe_key), render->u.render.picking ? "_pipe_point_pick%s" : "_pipe_point%s",
                fmt);
        }
        else if (vis_is_prim)
            dvz_snprintf(
                pipe_key, sizeof(pipe_key), has_normal ? "_pipe_prim_lit_t%u%s" : "_pipe_prim_t%u%s",
                topology, fmt);
        else
            dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe_img%s", fmt);

        uint64_t pipe_id = _obj_id(emitter, pipe_key, &is_new);
        if (pipe_id == 0) { ok = false; break; }
        if (ok && is_new)
        {
            if (vis_is_point)
            {
                uint32_t strides[3]   = {3 * sizeof(float), 4 * sizeof(uint8_t), sizeof(float)};
                uint32_t bindings[3]  = {0, 1, 2};
                uint32_t locations[3] = {0, 1, 2};
                uint32_t formats[3]   = {VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R8G8B8A8_UNORM,
                                         VK_FORMAT_R32_SFLOAT};
                uint32_t offsets[3]   = {0, 0, 0};
                uint32_t binding_count = 3;
                uint32_t attr_count = render->u.render.picking ? 2 : 3;
                if (render->u.render.picking)
                {
                    bindings[1] = 2;
                    locations[1] = 2;
                    formats[1] = VK_FORMAT_R32_SFLOAT;
                }
                ok = ok && dvz_drp2_stream_create_render_pipeline_ex(
                               stream, pipe_id, vs_id, fs_id, 3, topology,
                               binding_count, strides, attr_count, bindings, locations, formats,
                               offsets);
                if (ok && mvp_bgl_id != 0)
                    ok = dvz_drp2_stream_pipeline_set_bind_group_layout(stream, mvp_bgl_id);
                if (ok && pass_needs_depth)
                    ok = dvz_drp2_stream_pipeline_set_depth_state(
                        stream, false, VK_COMPARE_OP_ALWAYS);
            }
            else if (vis_is_prim)
            {
                uint32_t strides[3]   = {3 * sizeof(float), 4 * sizeof(uint8_t), 3 * sizeof(float)};
                uint32_t bindings[3]  = {0, 1, 2};
                uint32_t locations[3] = {0, 1, 2};
                uint32_t formats[3]   = {
                    VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R8G8B8A8_UNORM,
                    VK_FORMAT_R32G32B32_SFLOAT};
                uint32_t offsets[3]   = {0, 0, 0};
                uint32_t attr_count = has_normal ? 3 : 2;
                uint64_t shading_bgl_id = 0;
                bool shading_bgl_new = false;
                if (has_normal)
                {
                    shading_bgl_id = _obj_id(emitter, "_bgl_prim_shading", &shading_bgl_new);
                    if (shading_bgl_id == 0) { ok = false; break; }
                    if (shading_bgl_new)
                        ok = ok &&
                             dvz_drp2_stream_create_uniform_bind_group_layout(stream, shading_bgl_id);
                }
                ok = ok && dvz_drp2_stream_create_render_pipeline_ex(
                               stream, pipe_id, vs_id, fs_id, attr_count, topology,
                               attr_count, strides, attr_count, bindings, locations, formats, offsets);
                if (ok && mvp_bgl_id != 0)
                    ok = dvz_drp2_stream_pipeline_set_bind_group_layout(stream, mvp_bgl_id);
                if (ok && has_normal)
                    if (ok)
                        ok = dvz_drp2_stream_pipeline_set_bind_group_layout2(stream, shading_bgl_id);
                if (ok && pass_needs_depth)
                    ok = dvz_drp2_stream_pipeline_set_depth_state(
                        stream, has_normal,
                        has_normal ? VK_COMPARE_OP_LESS_OR_EQUAL : VK_COMPARE_OP_ALWAYS);
            }
            else /* vis_is_image */
            {
                uint32_t strides[2]   = {3 * sizeof(float), 2 * sizeof(float)};
                uint32_t bindings[2]  = {0, 1};
                uint32_t locations[2] = {0, 1};
                uint32_t formats[2]   = {VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32_SFLOAT};
                uint32_t offsets[2]   = {0, 0};

                /* Image BGL (lazy). */
                if (img_bgl_id == 0)
                {
                    img_bgl_id = _obj_id(emitter, "_bgl_img", &is_new);
                    if (img_bgl_id == 0) { ok = false; break; }
                    if (is_new)
                        ok = ok &&
                             dvz_drp2_stream_create_texture_sampler_bind_group_layout(
                                 stream, img_bgl_id);
                }
                ok = ok && dvz_drp2_stream_create_render_pipeline_ex(
                               stream, pipe_id, vs_id, fs_id, 2,
                               VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
                               2, strides, 2, bindings, locations, formats, offsets);
                if (ok && img_bgl_id != 0)
                    ok = dvz_drp2_stream_pipeline_set_bind_group_layout(stream, img_bgl_id);
                if (ok && pass_needs_depth)
                    ok = dvz_drp2_stream_pipeline_set_depth_state(
                        stream, false, VK_COMPARE_OP_ALWAYS);
            }
        }

        /* Bind group at set 0. */
        uint64_t vis_bg_set0 = 0;
        uint64_t vis_bg_set1 = 0;
        if (vis_is_point || vis_is_prim)
        {
            vis_bg_set0 = (render->u.render.controller_modes[i] == DVZ_CONTROLLER_FIXED)
                              ? fixed_bg_id
                              : apply_bg_id;
            if (vis_is_prim && has_normal && shading_buf != 0)
            {
                bool shading_bgl_new = false;
                uint64_t shading_bgl_id = _obj_id(emitter, "_bgl_prim_shading", &shading_bgl_new);
                if (shading_bgl_id == 0) { ok = false; break; }
                if (shading_bgl_new)
                    ok = ok && dvz_drp2_stream_create_uniform_bind_group_layout(stream, shading_bgl_id);
                char shading_bg_key[64];
                dvz_snprintf(shading_bg_key, sizeof(shading_bg_key), "_bg_prim_shading_%" PRIu64, shading_buf);
                uint64_t shading_bg_id = _obj_id(emitter, shading_bg_key, &is_new);
                if (shading_bg_id == 0) { ok = false; break; }
                if (ok && is_new)
                    ok = ok && dvz_drp2_stream_create_uniform_bind_group(
                                   stream, shading_bg_id, shading_bgl_id, shading_buf, 0,
                                   sizeof(DvzPrimitiveShadingState));
                vis_bg_set1 = shading_bg_id;
            }
        }
        else /* vis_is_image */
        {
            /* Image BGL + sampler (lazy). */
            if (img_bgl_id == 0)
            {
                img_bgl_id = _obj_id(emitter, "_bgl_img", &is_new);
                if (img_bgl_id == 0) { ok = false; break; }
                if (is_new)
                    ok = ok && dvz_drp2_stream_create_texture_sampler_bind_group_layout(
                                   stream, img_bgl_id);
            }
            if (img_sampler_id == 0)
            {
                img_sampler_id = _obj_id(emitter, "_sampler_img", &is_new);
                if (img_sampler_id == 0) { ok = false; break; }
                if (ok && is_new)
                    ok = ok && dvz_drp2_stream_create_sampler(stream, img_sampler_id);
            }
            char img_bg_key[64];
            dvz_snprintf(img_bg_key, sizeof(img_bg_key), "_bg_img_%" PRIu64, img_tex);
            uint64_t img_bg_id = _obj_id(emitter, img_bg_key, &is_new);
            if (img_bg_id == 0) { ok = false; break; }
            if (ok && is_new)
                ok = ok && dvz_drp2_stream_create_texture_sampler_bind_group(
                               stream, img_bg_id, img_bgl_id, img_tex, img_sampler_id);
            vis_bg_set0 = img_bg_id;

            /* Narrow vertex buffers to (position, texcoords) for image draw. */
            vbuf_ids[0] = img_pos;
            vbuf_ids[1] = img_uv;
            vbuf_count  = 2;
        }

        if (!ok)
            break;

        draws[draw_count].pipeline_id = pipe_id;
        draws[draw_count].bg_set0     = vis_bg_set0;
        draws[draw_count].bg_set1     = vis_bg_set1;
        draws[draw_count].vertex_count = vertex_count;
        draws[draw_count].vbuf_count  = vbuf_count;
        draws[draw_count].index_buffer_id = index_buf;
        draws[draw_count].index_count =
            (index_buf != 0 && _resource_item_stride(&emitter->resources, index_buf) != 0)
                ? (uint32_t)(_resource_byte_size(&emitter->resources, index_buf) /
                             _resource_item_stride(&emitter->resources, index_buf))
                : 0;
        draws[draw_count].index_format =
            _resource_item_stride(&emitter->resources, index_buf) == sizeof(uint16_t) ? "uint16"
                                                                                       : "uint32";
        for (uint32_t j = 0; j < vbuf_count; j++)
            draws[draw_count].vbuf_ids[j] = vbuf_ids[j];
        draw_count++;
    }

    if (!ok || draw_count == 0)
        return false;

    if (!ok)
        return false;

    ok = ok && dvz_drp2_stream_set_viewport(
                   stream, render_pass_id, render->u.render.desc.x, render->u.render.desc.y,
                   render->u.render.desc.width, render->u.render.desc.height) &&
         dvz_drp2_stream_set_scissor(
             stream, render_pass_id, render->u.render.desc.x, render->u.render.desc.y,
             render->u.render.desc.width, render->u.render.desc.height);

    uint64_t last_pipeline = (cache != NULL) ? cache->pipeline_id : 0;
    uint64_t last_bg_set0 = (cache != NULL) ? cache->bg_set0 : 0;
    uint64_t last_bg_set1 = 0;
    for (uint32_t d = 0; ok && d < draw_count; d++)
    {
        if (draws[d].pipeline_id != last_pipeline)
        {
            ok = ok && dvz_drp2_stream_set_pipeline(stream, render_pass_id, draws[d].pipeline_id);
            last_pipeline = draws[d].pipeline_id;
            last_bg_set0  = 0;
        }
        if (draws[d].bg_set0 != 0 && draws[d].bg_set0 != last_bg_set0)
        {
            ok = ok &&
                 dvz_drp2_stream_set_bind_group(stream, render_pass_id, 0, draws[d].bg_set0);
            last_bg_set0 = draws[d].bg_set0;
        }
        if (draws[d].bg_set1 != 0 && draws[d].bg_set1 != last_bg_set1)
        {
            ok = ok &&
                 dvz_drp2_stream_set_bind_group(stream, render_pass_id, 1, draws[d].bg_set1);
            last_bg_set1 = draws[d].bg_set1;
        }
        for (uint32_t j = 0; ok && j < draws[d].vbuf_count; j++)
            ok = ok && dvz_drp2_stream_set_vertex_buffer(
                           stream, render_pass_id, j, draws[d].vbuf_ids[j], 0);
        if (ok && draws[d].index_buffer_id != 0)
        {
            ok = ok &&
                 dvz_drp2_stream_set_index_buffer(
                     stream, render_pass_id, draws[d].index_buffer_id, draws[d].index_format, 0) &&
                 dvz_drp2_stream_draw_indexed(
                     stream, render_pass_id, draws[d].index_count, 1, 0, 0, 0);
        }
        else
        {
            ok = ok &&
                 dvz_drp2_stream_draw(stream, render_pass_id, draws[d].vertex_count, 1, 0, 0);
        }
    }

    if (cache != NULL)
    {
        cache->pipeline_id = last_pipeline;
        cache->bg_set0 = last_bg_set0;
    }

    return ok;
}



/* Scene render path: one BeginRenderPass per panel, one Draw per visual inside it. */
static bool _emitter_emit_render_multi(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlanNode* render,
    const DvzFramePlanNode* readback, bool clear, const DvzFramePlanEmitConfig* cfg,
    SceneRenderStateCache* cache)
{
    ANN(emitter);
    ANN(stream);
    ANN(render);

    bool ok = true;
    bool is_new = false;

    /* Color target. */
    uint64_t color_id = 0;
    if (cfg != NULL && cfg->external_color_target)
    {
        color_id = _color_target_id(cfg);
    }
    else
    {
        color_id = _obj_id(emitter, "_ct", &is_new);
        if (color_id == 0)
            return false;
        if (is_new)
        {
            uint32_t usage =
                DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC;
            ok = ok && dvz_drp2_stream_create_texture_2d_usage(stream, color_id, 4, 4, usage);
        }
    }

    uint64_t rb_id = 0;
    if (readback != NULL)
    {
        rb_id = _obj_buffer_id(emitter, "_rb", readback->u.copy.byte_size, &is_new);
        if (rb_id == 0)
            return false;
        if (ok && is_new)
        {
            uint32_t usage = DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ;
            ok = ok &&
                 dvz_drp2_stream_create_buffer(stream, rb_id, readback->u.copy.byte_size, usage);
        }
    }
    if (!ok)
        return false;

    uint64_t encoder_id = _emitter_next_transient_id(emitter);
    uint64_t render_pass_id = _emitter_next_transient_id(emitter);
    uint64_t command_buffer_id = _emitter_next_transient_id(emitter);
    uint64_t submission_id = _emitter_next_transient_id(emitter);

    float cr = cfg ? cfg->clear_color[0] : 0.0f;
    float cg = cfg ? cfg->clear_color[1] : 0.0f;
    float cb = cfg ? cfg->clear_color[2] : 0.0f;
    float ca = cfg ? cfg->clear_color[3] : 1.0f;
    bool needs_depth = _scene_render_needs_depth(emitter, render);

    ok = dvz_drp2_stream_begin_command_encoder(stream, encoder_id) &&
         dvz_drp2_stream_begin_render_pass_region_clear(
             stream, render_pass_id, encoder_id, color_id, cr, cg, cb, ca, 0.0f, 0.0f, 1.0f,
             1.0f, clear);
    if (ok && needs_depth)
        ok = dvz_drp2_stream_begin_render_pass_set_depth(stream, 1.0f);
    ok = ok &&
         _emitter_emit_render_multi_in_pass(
             emitter, stream, render, render_pass_id, cfg, cache) &&
         dvz_drp2_stream_end_render_pass(stream, render_pass_id);
    if (ok && readback != NULL)
        ok = ok && dvz_drp2_stream_copy_texture_to_buffer(
                       stream, encoder_id, color_id, rb_id, 0, 1, 1, 4, 1);
    ok = ok && dvz_drp2_stream_finish_command_encoder(stream, encoder_id, command_buffer_id);
    if (readback != NULL)
        ok = ok && dvz_drp2_stream_queue_submit_readback(
                       stream, command_buffer_id, submission_id, rb_id, 0,
                       readback->u.copy.byte_size);
    else
        ok = ok && dvz_drp2_stream_queue_submit(stream, command_buffer_id, submission_id);
    return ok;
}



/**
 * Emit all scene render nodes inside one figure-wide render pass.
 *
 * @param emitter the persistent emitter
 * @param stream the DRP2 command stream
 * @param plan the FramePlan
 * @param readback the optional readback copy node
 * @param cfg the emission config
 * @return whether the commands were emitted
 */
static bool _emitter_emit_scene_figure_renders(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanNode* readback, const DvzFramePlanEmitConfig* cfg)
{
    ANN(emitter);
    ANN(stream);
    ANN(plan);

    bool ok = true;
    bool is_new = false;

    uint64_t color_id = 0;
    if (cfg != NULL && cfg->external_color_target)
    {
        color_id = _color_target_id(cfg);
    }
    else
    {
        color_id = _obj_id(emitter, "_ct", &is_new);
        if (color_id == 0)
            return false;
        if (is_new)
        {
            uint32_t usage =
                DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC;
            ok = ok && dvz_drp2_stream_create_texture_2d_usage(stream, color_id, 4, 4, usage);
        }
    }

    uint64_t rb_id = 0;
    if (readback != NULL)
    {
        rb_id = _obj_buffer_id(emitter, "_rb", readback->u.copy.byte_size, &is_new);
        if (rb_id == 0)
            return false;
        if (ok && is_new)
        {
            uint32_t usage = DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ;
            ok = ok &&
                 dvz_drp2_stream_create_buffer(stream, rb_id, readback->u.copy.byte_size, usage);
        }
    }
    if (!ok)
        return false;

    uint64_t encoder_id = _emitter_next_transient_id(emitter);
    uint64_t render_pass_id = _emitter_next_transient_id(emitter);
    uint64_t command_buffer_id = _emitter_next_transient_id(emitter);
    uint64_t submission_id = _emitter_next_transient_id(emitter);

    float cr = cfg ? cfg->clear_color[0] : 0.0f;
    float cg = cfg ? cfg->clear_color[1] : 0.0f;
    float cb = cfg ? cfg->clear_color[2] : 0.0f;
    float ca = cfg ? cfg->clear_color[3] : 1.0f;

    ok = dvz_drp2_stream_begin_command_encoder(stream, encoder_id) &&
         dvz_drp2_stream_begin_render_pass_region_clear(
             stream, render_pass_id, encoder_id, color_id, cr, cg, cb, ca, 0.0f, 0.0f, 1.0f,
             1.0f, true);

    SceneRenderStateCache scene_cache = {0};
    for (uint32_t i = 0; ok && i < plan->count; i++)
    {
        const DvzFramePlanNode* render = &plan->nodes[i];
        if (render->type != DVZ_FRAME_PLAN_NODE_RENDER || render->u.render.visual_count == 0)
            continue;
        ok = _emitter_emit_render_multi_in_pass(
            emitter, stream, render, render_pass_id, cfg, &scene_cache);
    }

    ok = ok && dvz_drp2_stream_end_render_pass(stream, render_pass_id);
    if (ok && readback != NULL)
        ok = ok && dvz_drp2_stream_copy_texture_to_buffer(
                       stream, encoder_id, color_id, rb_id, 0, 1, 1, 4, 1);
    ok = ok && dvz_drp2_stream_finish_command_encoder(stream, encoder_id, command_buffer_id);
    if (readback != NULL)
        ok = ok && dvz_drp2_stream_queue_submit_readback(
                       stream, command_buffer_id, submission_id, rb_id, 0,
                       readback->u.copy.byte_size);
    else
        ok = ok && dvz_drp2_stream_queue_submit(stream, command_buffer_id, submission_id);
    return ok;
}



/**
 * Emit runtime-mode static render commands.
 *
 * @param emitter the persistent emitter
 * @param stream the DRP2 command stream
 * @param vertex_buffer_ids the vertex buffer ids
 * @param vertex_buffer_count the vertex buffer count
 * @param readback the optional readback copy node
 * @param cfg the emission config
 * @return whether the commands were emitted
 */
static bool _emitter_emit_render(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlanNode* render,
    const uint64_t* vertex_buffer_ids, uint32_t vertex_buffer_count,
    const DvzFramePlanNode* readback, bool clear, const DvzFramePlanEmitConfig* cfg,
    SceneRenderStateCache* cache)
{
    ANN(emitter);
    ANN(stream);
    ANN(render);

    /* Scene render node: per-visual multi-draw in a single pass. */
    if (vertex_buffer_count == 0 && render->u.render.visual_count > 0 &&
        cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
        return _emitter_emit_render_multi(emitter, stream, render, readback, clear, cfg, cache);

    /* Generic single-draw path (non-scene nodes, WGSL, or fallback). */
    ANN(vertex_buffer_ids);
    if (vertex_buffer_count == 0)
        return false;

    bool ok = true;
    bool is_new = false;
    const char* fmt = _shader_format_tag(cfg);

    /* Detect DvzPoint visual (position + color + size attributes). */
    bool is_point = _is_point_visual(&emitter->resources, vertex_buffer_ids, vertex_buffer_count);
    bool is_primitive =
        !is_point && _is_primitive_visual(&emitter->resources, vertex_buffer_ids, vertex_buffer_count);
    uint64_t image_pos = 0, image_uv = 0, image_tex = 0;
    bool is_image = !is_point && !is_primitive &&
                    _is_image_visual(&emitter->resources, vertex_buffer_ids, vertex_buffer_count,
                                     &image_pos, &image_uv, &image_tex);

    const char* vs_glsl = NULL;
    const char* fs_glsl = NULL;
    uint32_t topology = 0;
    uint32_t vertex_count = 3; /* default for stub / non-point path */
    uint64_t bgl_id = 0;
    uint64_t bg_id  = 0;

    /* MVP UBO bind group IDs — used for GLSL point/primitive path. */
    uint64_t mvp_bgl_id = 0;
    uint64_t mvp_buf_id = 0;
    uint64_t mvp_bg_id  = 0;
    bool uses_mvp =
        (is_point || is_primitive) &&
        cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL;

    /* When IMAGE: re-narrow vertex_buffer_ids to (position, texcoords) only — the texture
     * is bound through a bind group, not as a vertex buffer. */
    uint64_t image_vertex_ids[2];
    if (is_image)
    {
        image_vertex_ids[0] = image_pos;
        image_vertex_ids[1] = image_uv;
        vertex_buffer_ids   = image_vertex_ids;
        vertex_buffer_count = 2;
    }

    char vs_key[32];
    char fs_key[16];
    char pipe_key[48];

    if (is_point && cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
    {
        /* Point visual: use type-specific shaders and POINT_LIST topology. */
        dvz_snprintf(vs_key, sizeof(vs_key), "_vs_point%s", fmt);
        dvz_snprintf(fs_key, sizeof(fs_key), "_fs_point%s", fmt);
        vs_glsl = _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_POINT, false);
        fs_glsl = _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_POINT, true);
        topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;

        /* Infer vertex count from position buffer byte_size / sizeof(vec3). */
        for (uint32_t i = 0; i < vertex_buffer_count; i++)
        {
            if (strcmp(_resource_data_tag(&emitter->resources, vertex_buffer_ids[i]),
                       "position") == 0)
            {
                uint64_t sz = _resource_byte_size(&emitter->resources, vertex_buffer_ids[i]);
                if (sz > 0)
                    vertex_count = (uint32_t)(sz / (3 * sizeof(float)));
                break;
            }
        }
    }
    else if (is_primitive && cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
    {
        /* Primitive visual: pass-through shaders with visual-selected topology. */
        for (uint32_t i = 0; i < vertex_buffer_count; i++)
        {
            if (strcmp(_resource_data_tag(&emitter->resources, vertex_buffer_ids[i]),
                       "position") == 0)
            {
                uint64_t sz = _resource_byte_size(&emitter->resources, vertex_buffer_ids[i]);
                if (sz > 0)
                    vertex_count = (uint32_t)(sz / (3 * sizeof(float)));
                topology = _resource_topology(&emitter->resources, vertex_buffer_ids[i]);
                break;
            }
        }
        dvz_snprintf(vs_key, sizeof(vs_key), "_vs_prim%s", fmt);
        dvz_snprintf(fs_key, sizeof(fs_key), "_fs_prim%s", fmt);
        vs_glsl = _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE, false);
        fs_glsl = _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE, true);
    }
    else if (is_image && cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
    {
        /* Image visual: textured-quad shaders, TRIANGLE_STRIP topology, 4 vertices. */
        uint64_t pos_size = _resource_byte_size(&emitter->resources, image_pos);
        if (pos_size > 0)
            vertex_count = (uint32_t)(pos_size / (3 * sizeof(float)));
        topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        dvz_snprintf(vs_key, sizeof(vs_key), "_vs_img%s", fmt);
        dvz_snprintf(fs_key, sizeof(fs_key), "_fs_img%s", fmt);
        vs_glsl = _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_IMAGE, false);
        fs_glsl = _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_IMAGE, true);

        /* Sampler + texture-sampler bind-group layout + bind-group, all persistent. */
        bool bgl_new = false;
        bgl_id = _obj_id(emitter, "_bgl_img", &bgl_new);
        if (bgl_id == 0)
            return false;
        if (bgl_new)
            ok = ok && dvz_drp2_stream_create_texture_sampler_bind_group_layout(stream, bgl_id);

        bool sampler_new = false;
        uint64_t sampler_id = _obj_id(emitter, "_sampler_img", &sampler_new);
        if (sampler_id == 0)
            return false;
        if (ok && sampler_new)
            ok = ok && dvz_drp2_stream_create_sampler(stream, sampler_id);

        char bg_key[48];
        dvz_snprintf(bg_key, sizeof(bg_key), "_bg_img_%" PRIu64, image_tex);
        bool bg_new = false;
        bg_id = _obj_id(emitter, bg_key, &bg_new);
        if (bg_id == 0)
            return false;
        if (ok && bg_new)
            ok = ok && dvz_drp2_stream_create_texture_sampler_bind_group(
                           stream, bg_id, bgl_id, image_tex, sampler_id);
    }
    else if (cfg != NULL && cfg->fullscreen_triangle)
    {
        dvz_snprintf(vs_key, sizeof(vs_key), "_vs_full%s", fmt);
        dvz_snprintf(fs_key, sizeof(fs_key), "_fs%s", fmt);
    }
    else
    {
        dvz_snprintf(vs_key, sizeof(vs_key), "_vs%s", fmt);
        dvz_snprintf(fs_key, sizeof(fs_key), "_fs%s", fmt);
    }

    /* MVP UBO infrastructure (GLSL point/primitive path only). */
    if (uses_mvp)
    {
        bool mvp_bgl_new = false;
        mvp_bgl_id = _obj_id(emitter, "_bgl_mvp", &mvp_bgl_new);
        if (mvp_bgl_id == 0)
            return false;
        if (mvp_bgl_new)
            ok = ok && dvz_drp2_stream_create_uniform_bind_group_layout(stream, mvp_bgl_id);

        const char* mode_tag = (render->u.render.controller_modes[0] == DVZ_CONTROLLER_FIXED)
                                   ? "fixed"
                                   : "apply";
        char mvp_buf_key[128], mvp_bg_key[128];
        dvz_snprintf(
            mvp_buf_key, sizeof(mvp_buf_key), "_mvp_buf_%s_%s", render->u.render.panel_id,
            mode_tag);
        dvz_snprintf(
            mvp_bg_key, sizeof(mvp_bg_key), "_mvp_bg_%s_%s", render->u.render.panel_id, mode_tag);

        bool mvp_buf_new = false;
        mvp_buf_id = _obj_id(emitter, mvp_buf_key, &mvp_buf_new);
        if (mvp_buf_id == 0)
            return false;
        if (mvp_buf_new)
        {
            uint32_t usage =
                DVZ_DRP2_BUFFER_USAGE_UNIFORM |
                DVZ_DRP2_BUFFER_USAGE_MAP_WRITE |
                DVZ_DRP2_BUFFER_USAGE_COPY_DST;
            ok = ok && dvz_drp2_stream_create_buffer(stream, mvp_buf_id, sizeof(DvzMVP), usage);
        }

        bool mvp_bg_new = false;
        mvp_bg_id = _obj_id(emitter, mvp_bg_key, &mvp_bg_new);
        if (mvp_bg_id == 0)
            return false;
        if (mvp_bg_new)
            ok = ok && dvz_drp2_stream_create_uniform_bind_group(
                           stream, mvp_bg_id, mvp_bgl_id, mvp_buf_id, 0, sizeof(DvzMVP));

        /* Copy MVP into the emitter's per-(panel, controller_mode) cache (persists past
         * frame plan destruction so write_buffer_bytes' borrowed pointer stays valid). */
        char mvp_slot_key[128];
        dvz_snprintf(
            mvp_slot_key, sizeof(mvp_slot_key), "%s_%s", render->u.render.panel_id, mode_tag);
        DvzMVP* mvp_slot = _emitter_mvp_slot(emitter, mvp_slot_key);
        if (mvp_slot != NULL)
            *mvp_slot = render->u.render.apply_mvp;
        ok = ok && dvz_drp2_stream_write_buffer_bytes(
                       stream, mvp_buf_id, 0, sizeof(DvzMVP),
                       mvp_slot ? mvp_slot : &render->u.render.apply_mvp);
    }

    /* SPIR-V resource names (stem of .vert.spv / .frag.spv after embed_resources key mangling). */
    const char* vs_spirv_key = NULL;
    const char* fs_spirv_key = NULL;
    if (is_point)
    {
        vs_spirv_key = "point_vert";
        fs_spirv_key = "point_frag";
    }
    else if (is_primitive)
    {
        vs_spirv_key = "primitive_vert";
        fs_spirv_key = "primitive_frag";
    }
    else if (is_image)
    {
        vs_spirv_key = "image_vert";
        fs_spirv_key = "image_frag";
    }

    uint64_t vs_id = _obj_id(emitter, vs_key, &is_new);
    if (vs_id == 0)
        return false;
    if (is_new)
    {
        if (vs_glsl != NULL && vs_spirv_key != NULL &&
            cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
        {
            ok = ok && _emit_shader_spirv(stream, vs_id, "VERTEX", vs_spirv_key, vs_glsl, cfg);
        }
        else if (vs_glsl != NULL)
        {
            ok = ok && _emit_shader(stream, vs_id, "VERTEX", NULL, vs_glsl, cfg);
        }
        else
        {
            const char* vertex_wgsl = NULL;
            const char* vertex_glsl_src = NULL;
            _render_vertex_shader_source(cfg, &vertex_wgsl, &vertex_glsl_src);
            ok = ok && _emit_shader(stream, vs_id, "VERTEX", vertex_wgsl, vertex_glsl_src, cfg);
        }
    }

    uint64_t fs_id = _obj_id(emitter, fs_key, &is_new);
    if (fs_id == 0)
        return false;
    if (ok && is_new)
    {
        if (fs_glsl != NULL && fs_spirv_key != NULL &&
            cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
        {
            ok = ok && _emit_shader_spirv(stream, fs_id, "FRAGMENT", fs_spirv_key, fs_glsl, cfg);
        }
        else if (fs_glsl != NULL)
        {
            ok = ok && _emit_shader(stream, fs_id, "FRAGMENT", NULL, fs_glsl, cfg);
        }
        else
        {
            ok = ok && _emit_shader(
                           stream, fs_id, "FRAGMENT", _fixture_fragment_wgsl(), _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_FIXTURE, true), cfg);
        }
    }

    if (is_point && cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
        dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe_point%s", fmt);
    else if (is_primitive && cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
        dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe_prim_t%u%s", topology, fmt);
    else if (is_image && cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
        dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe_img%s", fmt);
    else
        dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe%u%s", vertex_buffer_count, fmt);

    uint64_t pipe_id = _obj_id(emitter, pipe_key, &is_new);
    if (pipe_id == 0)
        return false;
    if (ok && is_new)
    {
        if (is_point && cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
        {
            /* Explicit vertex layout: binding0=position(vec3), binding1=color(u8vec4), binding2=size(float) */
            uint32_t strides[3]   = {3*sizeof(float), 4*sizeof(uint8_t), sizeof(float)};
            uint32_t bindings[3]  = {0, 1, 2};
            uint32_t locations[3] = {0, 1, 2};
            uint32_t formats[3]   = {VK_FORMAT_R32G32B32_SFLOAT,
                                     VK_FORMAT_R8G8B8A8_UNORM,
                                     VK_FORMAT_R32_SFLOAT};
            uint32_t offsets[3]   = {0, 0, 0};
            ok = ok && dvz_drp2_stream_create_render_pipeline_ex(
                           stream, pipe_id, vs_id, fs_id, vertex_buffer_count,
                           topology,
                           3, strides,
                           3, bindings, locations, formats, offsets);
            if (ok && uses_mvp && mvp_bgl_id != 0)
                ok = dvz_drp2_stream_pipeline_set_bind_group_layout(stream, mvp_bgl_id);
        }
        else if (is_primitive && cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
        {
            /* binding0=position(vec3), binding1=color(u8vec4) */
            uint32_t strides[2]   = {3*sizeof(float), 4*sizeof(uint8_t)};
            uint32_t bindings[2]  = {0, 1};
            uint32_t locations[2] = {0, 1};
            uint32_t formats[2]   = {VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R8G8B8A8_UNORM};
            uint32_t offsets[2]   = {0, 0};
            ok = ok && dvz_drp2_stream_create_render_pipeline_ex(
                           stream, pipe_id, vs_id, fs_id, vertex_buffer_count,
                           topology,
                           2, strides,
                           2, bindings, locations, formats, offsets);
            if (ok && uses_mvp && mvp_bgl_id != 0)
                ok = dvz_drp2_stream_pipeline_set_bind_group_layout(stream, mvp_bgl_id);
        }
        else if (is_image && cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
        {
            /* binding0=position(vec3), binding1=texcoords(vec2); bgl=img */
            uint32_t strides[2]   = {3*sizeof(float), 2*sizeof(float)};
            uint32_t bindings[2]  = {0, 1};
            uint32_t locations[2] = {0, 1};
            uint32_t formats[2]   = {VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32_SFLOAT};
            uint32_t offsets[2]   = {0, 0};
            ok = ok && dvz_drp2_stream_create_render_pipeline_ex(
                           stream, pipe_id, vs_id, fs_id, vertex_buffer_count,
                           topology,
                           2, strides,
                           2, bindings, locations, formats, offsets);
            ok = ok && dvz_drp2_stream_pipeline_set_bind_group_layout(stream, bgl_id);
        }
        else
        {
            ok = ok && dvz_drp2_stream_create_render_pipeline(
                           stream, pipe_id, vs_id, fs_id, vertex_buffer_count);
        }
    }

    uint64_t color_id = 0;
    if (cfg != NULL && cfg->external_color_target)
    {
        color_id = _color_target_id(cfg);
    }
    else
    {
        color_id = _obj_id(emitter, "_ct", &is_new);
        if (color_id == 0)
            return false;
        if (ok && is_new)
        {
            uint32_t usage =
                DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC;
            ok = ok && dvz_drp2_stream_create_texture_2d_usage(stream, color_id, 4, 4, usage);
        }
    }

    uint64_t rb_id = 0;
    if (readback != NULL)
    {
        rb_id = _obj_buffer_id(emitter, "_rb", readback->u.copy.byte_size, &is_new);
        if (rb_id == 0)
            return false;
        if (ok && is_new)
        {
            uint32_t usage = DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ;
            ok = ok &&
                 dvz_drp2_stream_create_buffer(stream, rb_id, readback->u.copy.byte_size, usage);
        }
    }

    if (!ok)
        return false;

    uint64_t encoder_id = _emitter_next_transient_id(emitter);
    uint64_t render_pass_id = _emitter_next_transient_id(emitter);
    uint64_t command_buffer_id = _emitter_next_transient_id(emitter);
    uint64_t submission_id = _emitter_next_transient_id(emitter);

    float cr = cfg ? cfg->clear_color[0] : 0.0f;
    float cg = cfg ? cfg->clear_color[1] : 0.0f;
    float cb = cfg ? cfg->clear_color[2] : 0.0f;
    float ca = cfg ? cfg->clear_color[3] : 1.0f;
    ok = dvz_drp2_stream_begin_command_encoder(stream, encoder_id) &&
         dvz_drp2_stream_begin_render_pass_region_clear(
             stream, render_pass_id, encoder_id, color_id, cr, cg, cb, ca, render->u.render.desc.x,
             render->u.render.desc.y, render->u.render.desc.width, render->u.render.desc.height,
             clear) &&
         dvz_drp2_stream_set_pipeline(stream, render_pass_id, pipe_id);
    if (ok && is_image && bg_id != 0)
        ok = dvz_drp2_stream_set_bind_group(stream, render_pass_id, 0, bg_id);
    if (ok && uses_mvp && mvp_bg_id != 0)
        ok = dvz_drp2_stream_set_bind_group(stream, render_pass_id, 0, mvp_bg_id);
    for (uint32_t i = 0; ok && i < vertex_buffer_count; i++)
        ok = dvz_drp2_stream_set_vertex_buffer(stream, render_pass_id, i, vertex_buffer_ids[i], 0);
    ok = ok && dvz_drp2_stream_draw(stream, render_pass_id, vertex_count, 1, 0, 0) &&
         dvz_drp2_stream_end_render_pass(stream, render_pass_id);
    if (ok && readback != NULL)
    {
        ok = ok && dvz_drp2_stream_copy_texture_to_buffer(
                       stream, encoder_id, color_id, rb_id, 0, 1, 1, 4, 1);
    }
    ok = ok && dvz_drp2_stream_finish_command_encoder(stream, encoder_id, command_buffer_id);
    if (readback != NULL)
    {
        ok = ok && dvz_drp2_stream_queue_submit_readback(
                       stream, command_buffer_id, submission_id, rb_id, 0,
                       readback->u.copy.byte_size);
    }
    else
    {
        ok = ok && dvz_drp2_stream_queue_submit(stream, command_buffer_id, submission_id);
    }
    return ok;
}



/**
 * Emit all plain render nodes in a runtime-mode FramePlan.
 *
 * @param emitter the persistent emitter
 * @param stream the DRP2 command stream
 * @param plan the FramePlan
 * @param fallback_vertex_buffer_ids uploaded vertex buffer ids used when visual ids are generic
 * @param fallback_vertex_buffer_count number of fallback vertex buffer ids
 * @param readback the optional readback copy node
 * @param cfg the emission config
 * @return whether all render commands were emitted
 */
static bool _emitter_emit_plain_renders(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const uint64_t* fallback_vertex_buffer_ids, uint32_t fallback_vertex_buffer_count,
    const DvzFramePlanNode* readback, const DvzFramePlanEmitConfig* cfg)
{
    ANN(emitter);
    ANN(stream);
    ANN(plan);

    uint32_t render_node_count = 0;
    uint32_t scene_render_node_count = 0;
    bool any_scene_render_needs_depth = false;
    for (uint32_t i = 0; i < plan->count; i++)
    {
        const DvzFramePlanNode* render = &plan->nodes[i];
        if (render->type != DVZ_FRAME_PLAN_NODE_RENDER)
            continue;
        render_node_count++;
        if (render->u.render.visual_count > 0 &&
            cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
        {
            char visual_id[DVZ_SCENE_LABEL_SIZE];
            char shared_index_id[DVZ_SCENE_LABEL_SIZE];
            char probe[DVZ_SCENE_LABEL_SIZE];
            _parse_visual_id(
                render->u.render.visuals[0], visual_id, sizeof(visual_id), shared_index_id,
                sizeof(shared_index_id));
            dvz_snprintf(probe, sizeof(probe), "%s_position", visual_id);
            if (_resource_lookup_id(&emitter->resources, probe) != 0)
            {
                scene_render_node_count++;
                any_scene_render_needs_depth =
                    any_scene_render_needs_depth || _scene_render_needs_depth(emitter, render);
            }
        }
    }
    if (render_node_count > 0 && render_node_count == scene_render_node_count &&
        !any_scene_render_needs_depth)
        return _emitter_emit_scene_figure_renders(emitter, stream, plan, readback, cfg);

    bool ok = true;
    uint32_t render_count = 0;
    SceneRenderStateCache scene_cache = {0};
    for (uint32_t i = 0; ok && i < plan->count; i++)
    {
        const DvzFramePlanNode* render = &plan->nodes[i];
        if (render->type != DVZ_FRAME_PLAN_NODE_RENDER)
            continue;

        uint64_t vertex_buffer_ids[DVZ_SCENE_MAX_NODE_RESOURCES] = {0};
        uint32_t vertex_buffer_count = 0;

        /* Scene render nodes (visual_count > 0 with named resources) skip flat resolution;
         * _emitter_emit_render dispatches to _emitter_emit_render_multi instead. */
        bool is_scene_node = false;
        if (render->u.render.visual_count > 0 &&
            cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
        {
            char visual_id[DVZ_SCENE_LABEL_SIZE];
            char shared_index_id[DVZ_SCENE_LABEL_SIZE];
            char probe[DVZ_SCENE_LABEL_SIZE];
            _parse_visual_id(
                render->u.render.visuals[0], visual_id, sizeof(visual_id), shared_index_id,
                sizeof(shared_index_id));
            dvz_snprintf(probe, sizeof(probe), "%s_position", visual_id);
            is_scene_node = _resource_lookup_id(&emitter->resources, probe) != 0;
        }

        if (!is_scene_node)
        {
            ok = _emitter_resolve_render_vertex_buffers(
                emitter, render, vertex_buffer_ids, &vertex_buffer_count);
            if (!ok && fallback_vertex_buffer_ids != NULL && fallback_vertex_buffer_count > 0)
            {
                ok = true;
                vertex_buffer_count = fallback_vertex_buffer_count;
                for (uint32_t j = 0; j < vertex_buffer_count; j++)
                    vertex_buffer_ids[j] = fallback_vertex_buffer_ids[j];
            }
            scene_cache.pipeline_id = 0;
            scene_cache.bg_set0 = 0;
        }

        if (ok)
        {
            ok = _emitter_emit_render(
                emitter, stream, render, vertex_buffer_ids, vertex_buffer_count,
                render_count == 0 ? readback : NULL, render_count == 0, cfg,
                is_scene_node ? &scene_cache : NULL);
        }
        render_count++;
    }
    return ok && render_count > 0;
}



/**
 * Emit runtime-mode clear-only render commands.
 *
 * @param emitter the persistent emitter
 * @param stream the DRP2 command stream
 * @param readback the optional readback copy node
 * @param cfg the emission config
 * @return whether the commands were emitted
 */
static bool _emitter_emit_clear_only(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlanNode* clear_node,
    const DvzFramePlanNode* readback, bool clear, const DvzFramePlanEmitConfig* cfg)
{
    ANN(emitter);
    ANN(stream);
    ANN(clear_node);

    bool ok = true;
    bool is_new = false;

    uint64_t color_id = 0;
    if (cfg != NULL && cfg->external_color_target)
    {
        color_id = _color_target_id(cfg);
    }
    else
    {
        color_id = _obj_id(emitter, "_ct", &is_new);
        if (color_id == 0)
            return false;
        if (is_new)
        {
            uint32_t usage =
                DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC;
            ok = ok && dvz_drp2_stream_create_texture_2d_usage(stream, color_id, 4, 4, usage);
        }
    }

    uint64_t rb_id = 0;
    if (readback != NULL)
    {
        rb_id = _obj_buffer_id(emitter, "_rb", readback->u.copy.byte_size, &is_new);
        if (rb_id == 0)
            return false;
        if (ok && is_new)
        {
            uint32_t usage = DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ;
            ok = ok &&
                 dvz_drp2_stream_create_buffer(stream, rb_id, readback->u.copy.byte_size, usage);
        }
    }
    if (!ok)
        return false;

    uint64_t encoder_id = _emitter_next_transient_id(emitter);
    uint64_t render_pass_id = _emitter_next_transient_id(emitter);
    uint64_t command_buffer_id = _emitter_next_transient_id(emitter);
    uint64_t submission_id = _emitter_next_transient_id(emitter);

    float cr = cfg ? cfg->clear_color[0] : 0.0f;
    float cg = cfg ? cfg->clear_color[1] : 0.0f;
    float cb = cfg ? cfg->clear_color[2] : 0.0f;
    float ca = cfg ? cfg->clear_color[3] : 1.0f;
    ok = dvz_drp2_stream_begin_command_encoder(stream, encoder_id) &&
         dvz_drp2_stream_begin_render_pass_region_clear(
             stream, render_pass_id, encoder_id, color_id, cr, cg, cb, ca,
             clear_node->u.clear.desc.x, clear_node->u.clear.desc.y, clear_node->u.clear.desc.width,
             clear_node->u.clear.desc.height, clear) &&
         dvz_drp2_stream_end_render_pass(stream, render_pass_id);
    if (ok && readback != NULL)
    {
        ok = ok && dvz_drp2_stream_copy_texture_to_buffer(
                       stream, encoder_id, color_id, rb_id, 0, 1, 1, 4, 1);
    }
    ok = ok && dvz_drp2_stream_finish_command_encoder(stream, encoder_id, command_buffer_id);
    if (readback != NULL)
    {
        ok = ok && dvz_drp2_stream_queue_submit_readback(
                       stream, command_buffer_id, submission_id, rb_id, 0,
                       readback->u.copy.byte_size);
    }
    else
    {
        ok = ok && dvz_drp2_stream_queue_submit(stream, command_buffer_id, submission_id);
    }
    return ok;
}


/**
 * Emit runtime-mode texture render commands.
 *
 * @param emitter the persistent emitter
 * @param stream the DRP2 command stream
 * @param texture_id the sampled texture id
 * @param readback the optional readback copy node
 * @param cfg the emission config
 * @return whether the commands were emitted
 */
static bool _emitter_emit_texture_render(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, uint64_t texture_id,
    const DvzFramePlanNode* readback, const DvzFramePlanEmitConfig* cfg)
{
    ANN(emitter);
    ANN(stream);
    if (texture_id == 0)
        return false;

    bool ok = true;
    bool is_new = false;
    const char* fmt = _shader_format_tag(cfg);

    uint64_t sampler_id = _obj_id(emitter, "_sampler", &is_new);
    if (sampler_id == 0)
        return false;
    if (is_new)
        ok = ok && dvz_drp2_stream_create_sampler(stream, sampler_id);

    uint64_t bgl_id = _obj_id(emitter, "_bgl_tex", &is_new);
    if (bgl_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && dvz_drp2_stream_create_texture_sampler_bind_group_layout(stream, bgl_id);

    char vs_key[16];
    dvz_snprintf(vs_key, sizeof(vs_key), "_vs_tex%s", fmt);
    uint64_t vs_id = _obj_id(emitter, vs_key, &is_new);
    if (vs_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && _emit_shader(
                       stream, vs_id, "VERTEX", _texture_vertex_wgsl(),
                       _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_TEXTURE, false), cfg);

    char fs_key[16];
    dvz_snprintf(fs_key, sizeof(fs_key), "_fs_tex%s", fmt);
    uint64_t fs_id = _obj_id(emitter, fs_key, &is_new);
    if (fs_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && _emit_shader(
                       stream, fs_id, "FRAGMENT", _texture_fragment_wgsl(),
                       _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_TEXTURE, true), cfg);

    char pipe_key[32];
    dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe_tex%s_%" PRIu64, fmt, bgl_id);
    uint64_t pipe_id = _obj_id(emitter, pipe_key, &is_new);
    if (pipe_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && dvz_drp2_stream_create_render_pipeline_with_bind_group_layout(
                       stream, pipe_id, vs_id, fs_id, 0, bgl_id);

    char bg_key[32];
    dvz_snprintf(bg_key, sizeof(bg_key), "_bg_tex_%" PRIu64, texture_id);
    uint64_t bg_id = _obj_id(emitter, bg_key, &is_new);
    if (bg_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && dvz_drp2_stream_create_texture_sampler_bind_group(
                       stream, bg_id, bgl_id, texture_id, sampler_id);

    uint64_t color_id = 0;
    if (cfg != NULL && cfg->external_color_target)
    {
        color_id = _color_target_id(cfg);
    }
    else
    {
        color_id = _obj_id(emitter, "_ct", &is_new);
        if (color_id == 0)
            return false;
        if (ok && is_new)
        {
            uint32_t usage =
                DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC;
            ok = ok && dvz_drp2_stream_create_texture_2d_usage(stream, color_id, 4, 4, usage);
        }
    }

    uint64_t rb_id = 0;
    if (readback != NULL)
    {
        rb_id = _obj_buffer_id(emitter, "_rb", readback->u.copy.byte_size, &is_new);
        if (rb_id == 0)
            return false;
        if (ok && is_new)
        {
            uint32_t usage = DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ;
            ok = ok &&
                 dvz_drp2_stream_create_buffer(stream, rb_id, readback->u.copy.byte_size, usage);
        }
    }

    if (!ok)
        return false;

    uint64_t encoder_id = _emitter_next_transient_id(emitter);
    uint64_t render_pass_id = _emitter_next_transient_id(emitter);
    uint64_t command_buffer_id = _emitter_next_transient_id(emitter);
    uint64_t submission_id = _emitter_next_transient_id(emitter);

    float cr2 = cfg ? cfg->clear_color[0] : 0.0f;
    float cg2 = cfg ? cfg->clear_color[1] : 0.0f;
    float cb2 = cfg ? cfg->clear_color[2] : 0.0f;
    float ca2 = cfg ? cfg->clear_color[3] : 1.0f;
    ok = dvz_drp2_stream_begin_command_encoder(stream, encoder_id) &&
         dvz_drp2_stream_begin_render_pass_clear(
             stream, render_pass_id, encoder_id, color_id, cr2, cg2, cb2, ca2) &&
         dvz_drp2_stream_set_pipeline(stream, render_pass_id, pipe_id) &&
         dvz_drp2_stream_set_bind_group(stream, render_pass_id, 0, bg_id) &&
         dvz_drp2_stream_draw(stream, render_pass_id, 3, 1, 0, 0) &&
         dvz_drp2_stream_end_render_pass(stream, render_pass_id);
    if (ok && readback != NULL)
    {
        ok = ok && dvz_drp2_stream_copy_texture_to_buffer(
                       stream, encoder_id, color_id, rb_id, 0, 1, 1, 4, 1);
    }
    ok = ok && dvz_drp2_stream_finish_command_encoder(stream, encoder_id, command_buffer_id);
    if (readback != NULL)
    {
        ok = ok && dvz_drp2_stream_queue_submit_readback(
                       stream, command_buffer_id, submission_id, rb_id, 0,
                       readback->u.copy.byte_size);
    }
    else
    {
        ok = ok && dvz_drp2_stream_queue_submit(stream, command_buffer_id, submission_id);
    }
    return ok;
}


/**
 * Emit runtime-mode compute pass followed by render commands.
 *
 * @param emitter the persistent emitter
 * @param stream the DRP2 command stream
 * @param compute the compute node
 * @param readback the optional readback copy node
 * @param cfg the emission config
 * @return whether the commands were emitted
 */
static bool _emitter_emit_compute_assisted_render(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlanNode* compute,
    const DvzFramePlanNode* readback, const DvzFramePlanEmitConfig* cfg)
{
    ANN(emitter);
    ANN(stream);
    ANN(compute);
    if (emitter->resources.first_compute_input_id == 0 ||
        emitter->resources.first_compute_output_id == 0 ||
        emitter->resources.compute_buffer_size == 0)
        return false;

    bool ok = true;
    bool is_new = false;
    const char* fmt = _shader_format_tag(cfg);

    uint64_t bgl_stor_id = _obj_id(emitter, "_bgl_stor", &is_new);
    if (bgl_stor_id == 0)
        return false;
    if (is_new)
        ok = ok &&
             dvz_drp2_stream_create_storage_bind_group_layout(stream, bgl_stor_id);

    char cs_key[16];
    dvz_snprintf(cs_key, sizeof(cs_key), "_cs%s", fmt);
    uint64_t cs_id = _obj_id(emitter, cs_key, &is_new);
    if (cs_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && _emit_shader(
                       stream, cs_id, "COMPUTE", _compute_copy_wgsl(), _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_COMPUTE_COPY, false), cfg);

    char cpipe_key[32];
    dvz_snprintf(cpipe_key, sizeof(cpipe_key), "_cpipe%s_%" PRIu64, fmt, bgl_stor_id);
    uint64_t cpipe_id = _obj_id(emitter, cpipe_key, &is_new);
    if (cpipe_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && dvz_drp2_stream_create_compute_pipeline_with_bind_group_layout(
                       stream, cpipe_id, cs_id, bgl_stor_id);

    char bg_stor_key[64];
    dvz_snprintf(
        bg_stor_key, sizeof(bg_stor_key), "_bg_stor_%" PRIu64 "_%" PRIu64,
        emitter->resources.first_compute_input_id,
        emitter->resources.first_compute_output_id);
    uint64_t bg_stor_id = _obj_id(emitter, bg_stor_key, &is_new);
    if (bg_stor_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && dvz_drp2_stream_create_storage_bind_group(
                       stream, bg_stor_id, bgl_stor_id,
                       emitter->resources.first_compute_input_id,
                       emitter->resources.first_compute_output_id,
                       emitter->resources.compute_buffer_size);

    char vs_key[16];
    dvz_snprintf(vs_key, sizeof(vs_key), "_vs%s", fmt);
    uint64_t vs_id = _obj_id(emitter, vs_key, &is_new);
    if (vs_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && _emit_shader(
                       stream, vs_id, "VERTEX", _fixture_vertex_wgsl(), _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_FIXTURE, false), cfg);

    char fs_key[16];
    dvz_snprintf(fs_key, sizeof(fs_key), "_fs%s", fmt);
    uint64_t fs_id = _obj_id(emitter, fs_key, &is_new);
    if (fs_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && _emit_shader(
                       stream, fs_id, "FRAGMENT", _fixture_fragment_wgsl(), _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_FIXTURE, true), cfg);

    char pipe_key[32];
    dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe1%s", fmt);
    uint64_t pipe_id = _obj_id(emitter, pipe_key, &is_new);
    if (pipe_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && dvz_drp2_stream_create_render_pipeline(stream, pipe_id, vs_id, fs_id, 1);

    uint64_t color_id = 0;
    if (cfg != NULL && cfg->external_color_target)
    {
        color_id = _color_target_id(cfg);
    }
    else
    {
        color_id = _obj_id(emitter, "_ct", &is_new);
        if (color_id == 0)
            return false;
        if (ok && is_new)
        {
            uint32_t usage =
                DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC;
            ok = ok && dvz_drp2_stream_create_texture_2d_usage(stream, color_id, 4, 4, usage);
        }
    }

    uint64_t rb_id = 0;
    if (readback != NULL)
    {
        rb_id = _obj_buffer_id(emitter, "_rb", readback->u.copy.byte_size, &is_new);
        if (rb_id == 0)
            return false;
        if (ok && is_new)
        {
            uint32_t usage = DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ;
            ok = ok &&
                 dvz_drp2_stream_create_buffer(stream, rb_id, readback->u.copy.byte_size, usage);
        }
    }

    if (!ok)
        return false;

    uint64_t encoder_id = _emitter_next_transient_id(emitter);
    uint64_t compute_pass_id = _emitter_next_transient_id(emitter);
    uint64_t render_pass_id = _emitter_next_transient_id(emitter);
    uint64_t command_buffer_id = _emitter_next_transient_id(emitter);
    uint64_t submission_id = _emitter_next_transient_id(emitter);

    ok = dvz_drp2_stream_begin_command_encoder(stream, encoder_id) &&
         dvz_drp2_stream_begin_compute_pass(stream, compute_pass_id, encoder_id) &&
         dvz_drp2_stream_set_pipeline(stream, compute_pass_id, cpipe_id) &&
         dvz_drp2_stream_set_bind_group(stream, compute_pass_id, 0, bg_stor_id) &&
         dvz_drp2_stream_dispatch_workgroups(
             stream, compute_pass_id, compute->u.compute.dispatch[0],
             compute->u.compute.dispatch[1], compute->u.compute.dispatch[2]) &&
         dvz_drp2_stream_end_compute_pass(stream, compute_pass_id) &&
         dvz_drp2_stream_begin_render_pass_clear(
             stream, render_pass_id, encoder_id, color_id,
             cfg ? cfg->clear_color[0] : 0.0f, cfg ? cfg->clear_color[1] : 0.0f,
             cfg ? cfg->clear_color[2] : 0.0f, cfg ? cfg->clear_color[3] : 1.0f) &&
         dvz_drp2_stream_set_pipeline(stream, render_pass_id, pipe_id) &&
         dvz_drp2_stream_set_vertex_buffer(
             stream, render_pass_id, 0, emitter->resources.first_compute_output_id, 0) &&
         dvz_drp2_stream_draw(stream, render_pass_id, 3, 1, 0, 0) &&
         dvz_drp2_stream_end_render_pass(stream, render_pass_id);
    if (ok && readback != NULL)
    {
        ok = ok && dvz_drp2_stream_copy_texture_to_buffer(
                       stream, encoder_id, color_id, rb_id, 0, 1, 1, 4, 1);
    }
    ok = ok && dvz_drp2_stream_finish_command_encoder(stream, encoder_id, command_buffer_id);
    if (readback != NULL)
    {
        ok = ok && dvz_drp2_stream_queue_submit_readback(
                       stream, command_buffer_id, submission_id, rb_id, 0,
                       readback->u.copy.byte_size);
    }
    else
    {
        ok = ok && dvz_drp2_stream_queue_submit(stream, command_buffer_id, submission_id);
    }
    return ok;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Emit a runtime-mode DRP2 command stream from a FramePlan.
 *
 * @param emitter the persistent emitter
 * @param plan the FramePlan
 * @param caps the capability snapshot
 * @param report the diagnostic report
 * @param cfg the emission configuration
 * @return an owned DRP2 command stream, or NULL on failure
 */
DvzDrp2CommandStream* dvz_frame_plan_emitter_emit_drp2(
    DvzFramePlanEmitter* emitter, const DvzFramePlan* plan, const DvzCapabilitySnapshot* caps,
    DvzDiagnosticReport* report, const DvzFramePlanEmitConfig* cfg)
{
    ANN(emitter);
    ANN(plan);

    const DvzFramePlanNode* upload = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_UPLOAD);
    const DvzFramePlanNode* compute = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_COMPUTE);
    const DvzFramePlanNode* render = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_RENDER);
    const DvzFramePlanNode* clear = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_CLEAR);
    const DvzFramePlanNode* copy = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_COPY);
    const DvzFramePlanNode* readback = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_READBACK);
    bool clear_only = upload == NULL && compute == NULL && clear != NULL;
    bool retained_render = upload == NULL && compute == NULL && render != NULL &&
                           render->u.render.visual_count > 0;

    if ((!clear_only && !retained_render && upload == NULL) || (clear_only ? clear == NULL : render == NULL))
    {
        _diagnostic(report, "runtime converter requires upload+render");
        return NULL;
    }
    bool texture_render = !clear_only && _render_uses_texture(render);
    if (compute != NULL)
    {
        if (compute->u.compute.write_count == 0)
        {
            _diagnostic(report, "runtime converter requires compute output");
            return NULL;
        }
    }
    if (readback != NULL && copy == NULL)
    {
        _diagnostic(report, "runtime converter requires copy before readback");
        return NULL;
    }
    if (caps != NULL && !_validate_capabilities(plan, caps, cfg, report))
        return NULL;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    bool ok = true;
    uint64_t fallback_vertex_buffer_ids[DVZ_SCENE_MAX_NODE_RESOURCES] = {0};
    uint32_t fallback_vertex_buffer_count = 0;
    uint64_t texture_id = 0;
    if (!emitter->handshake_sent)
    {
        ok = dvz_drp2_stream_hello_renderer(stream, "scene-runtime") &&
             dvz_drp2_stream_renderer_hello_reply(stream, "datoviz-drp2-runtime");
        emitter->handshake_sent = ok;
    }

    for (uint32_t i = 0; ok && i < plan->count; i++)
    {
        if (plan->nodes[i].type == DVZ_FRAME_PLAN_NODE_UPLOAD)
        {
            if (compute != NULL)
            {
                ok = _emitter_emit_compute_buffers(emitter, stream, &plan->nodes[i], compute);
            }
            else if (texture_render)
            {
                ok = _emitter_emit_texture_upload(emitter, stream, &plan->nodes[i], &texture_id);
            }
            else
            {
                uint64_t uploaded_id = 0;
                ok = _emitter_emit_upload(
                    emitter, stream, &plan->nodes[i], &uploaded_id);
                if (ok && fallback_vertex_buffer_count < DVZ_SCENE_MAX_NODE_RESOURCES)
                    fallback_vertex_buffer_ids[fallback_vertex_buffer_count++] = uploaded_id;
            }
        }
    }

    ok = ok && (clear_only
                    ? _emitter_emit_clear_only(emitter, stream, clear, copy, true, cfg)
                    : compute != NULL
                    ? _emitter_emit_compute_assisted_render(emitter, stream, compute, copy, cfg)
                    : texture_render
                    ? _emitter_emit_texture_render(emitter, stream, texture_id, copy, cfg)
                    : _emitter_emit_plain_renders(
                          emitter, stream, plan, fallback_vertex_buffer_ids,
                          fallback_vertex_buffer_count, copy, cfg));
    if (!ok)
    {
        _diagnostic(report, "failed to emit runtime DRP2 stream");
        dvz_drp2_stream_destroy(stream);
        return NULL;
    }
    return stream;
}
