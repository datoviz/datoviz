/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene FramePlan runtime untyped compatibility render emission                                  */
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
#include "frame_plan/frame_plan.h"
#include "frame_plan/emit.h"
#include "_frame_plan_runtime_internal.h"
#include "_frame_plan_runtime_upload.h"
#include "_render_pass.h"
#include "_scene.h"
#include "_scene_common_bindings.h"
#include "_scene_resource_key.h"
#include "_scene_shader_abi.h"
#include "_shader_registry.h"
#include "_technique.h"
#include "_visual_pipeline.h"
#include "_visual_pipeline_internal.h"
#include "datoviz/drp2.h"
#include "datoviz/drp2/stream.h"
#include "datoviz/scene.h"
#include "render_contract/render_contract.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

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
bool _emitter_emit_render(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanNode* render, const uint64_t* vertex_buffer_ids,
    uint32_t vertex_buffer_count, const DvzFramePlanNode* readback, bool clear,
    const DvzFramePlanEmitConfig* cfg, SceneRenderStateCache* cache, DvzDiagnosticReport* report)
{
    ANN(emitter);
    ANN(stream);
    ANN(plan);
    ANN(render);

    /* Scene render node: per-visual multi-draw in a single pass. */
    if (vertex_buffer_count == 0 && render->u.render.visual_count > 0 && cfg != NULL &&
        cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
        return _emitter_emit_render_multi(
            emitter, stream, plan, render, readback, clear, cfg, cache, report);

    /* Generic single-draw path (non-scene nodes, WGSL, or fallback). */
    ANN(vertex_buffer_ids);
    if (vertex_buffer_count == 0)
        return false;

    bool ok = true;
    bool is_new = false;
    const char* fmt = _shader_format_tag(cfg);

    const DvzFramePlanVisualMeta* visual_meta = NULL;
    DvzSceneVisualDescKind desc_kind = DVZ_SCENE_VISUAL_DESC_NONE;
    if (render->u.render.visual_count == 1 && render->u.render.visual_metadata[0].has_metadata)
    {
        visual_meta = &render->u.render.visual_metadata[0];
        desc_kind = _scene_visual_meta_desc_kind(&emitter->resources, visual_meta);
    }
    if (render->u.render.visual_count > 0 && !render->u.render.allow_untyped_visuals)
    {
        for (uint32_t i = 0; i < render->u.render.visual_count; i++)
        {
            if (render->u.render.visual_metadata[i].has_metadata)
                continue;
            _diagnostic(report, "render visual missing typed metadata");
            return false;
        }
    }

    /* Detect point-like visual data (position + color + size attributes). */
    bool is_point = _scene_untyped_compat_is_point_visual(
        &emitter->resources, vertex_buffer_ids, vertex_buffer_count);
    bool is_pixel = is_point && desc_kind == DVZ_SCENE_VISUAL_DESC_PIXEL;
    bool is_marker = is_point && desc_kind == DVZ_SCENE_VISUAL_DESC_MARKER;
    bool is_point_like = is_point;
    bool is_splat = !is_point_like && desc_kind == DVZ_SCENE_VISUAL_DESC_SPLAT &&
                    _scene_untyped_compat_is_splat_visual(
                        &emitter->resources, vertex_buffer_ids, vertex_buffer_count);
    uint64_t mesh_pos = 0, mesh_color = 0, mesh_normal = 0, mesh_uv = 0, mesh_tex = 0;
    bool is_textured_mesh =
        !is_point_like && !is_splat &&
        _scene_untyped_compat_is_textured_mesh_visual(
            &emitter->resources, vertex_buffer_ids, vertex_buffer_count, &mesh_pos, &mesh_color,
            &mesh_normal, &mesh_uv, &mesh_tex);
    bool is_primitive =
        !is_point_like && !is_splat && !is_textured_mesh &&
        _scene_untyped_compat_is_primitive_visual(
            &emitter->resources, vertex_buffer_ids, vertex_buffer_count);
    uint64_t image_pos = 0, image_uv = 0, image_tex = 0;
    bool is_image = !is_point_like && !is_splat && !is_textured_mesh && !is_primitive &&
                    _scene_untyped_compat_is_image_visual(
                        &emitter->resources, vertex_buffer_ids, vertex_buffer_count, &image_pos,
                        &image_uv, &image_tex);
    bool is_labels = is_image && (desc_kind == DVZ_SCENE_VISUAL_DESC_LABELS_SINT ||
                                  desc_kind == DVZ_SCENE_VISUAL_DESC_LABELS_UINT);
    bool is_labels_sint = is_labels && desc_kind == DVZ_SCENE_VISUAL_DESC_LABELS_SINT;
    bool is_labels_uint = is_labels && desc_kind == DVZ_SCENE_VISUAL_DESC_LABELS_UINT;
    bool labels_query_u32 = render->u.render.picking && is_labels &&
                            cfg != NULL && cfg->color_target_format == VK_FORMAT_R32_UINT;
    bool image_pixel_space =
        is_image && !is_labels_sint && !is_labels_uint && visual_meta != NULL &&
        visual_meta->image_pixel_space;

    const char* vs_glsl = NULL;
    const char* fs_glsl = NULL;
    const char* vs_wgsl = NULL;
    const char* fs_wgsl = NULL;
    uint32_t topology = 0;
    uint32_t vertex_count = 3; /* default for stub / non-point path */
    uint32_t instance_count = 1;
    uint64_t bgl_id = 0;
    uint64_t bg_id = 0;
    DvzScenePointLikeLoweringDesc point_like_lowering = {0};
    bool has_point_like_lowering = false;

    /* Common bind group IDs used for GLSL/WGSL point, primitive, and image paths. */
    uint64_t common_bgl_id = 0;
    uint64_t common_bg_id = 0;
    bool uses_common = (is_point || is_splat || is_textured_mesh || is_primitive || is_image) &&
                       cfg != NULL &&
                       (cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL ||
                        (cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_WGSL &&
                         (is_point || is_splat || is_textured_mesh || is_primitive || is_image)));

    uint64_t splat_vertex_ids[4];
    if (is_splat)
    {
        uint64_t splat_pos = _scene_visual_resource_by_role(
            &emitter->resources, vertex_buffer_ids, vertex_buffer_count,
            DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION);
        uint64_t splat_color = _scene_visual_resource_by_role(
            &emitter->resources, vertex_buffer_ids, vertex_buffer_count,
            DVZ_FRAME_PLAN_RESOURCE_ROLE_COLOR);
        uint64_t splat_sigma = _scene_visual_resource_by_role(
            &emitter->resources, vertex_buffer_ids, vertex_buffer_count,
            DVZ_FRAME_PLAN_RESOURCE_ROLE_SIGMA);
        uint64_t splat_angle = _scene_visual_resource_by_role(
            &emitter->resources, vertex_buffer_ids, vertex_buffer_count,
            DVZ_FRAME_PLAN_RESOURCE_ROLE_ANGLE);
        if (splat_pos == 0 || splat_color == 0 || splat_sigma == 0 || splat_angle == 0)
            return false;
        splat_vertex_ids[0] = splat_pos;
        splat_vertex_ids[1] = splat_color;
        splat_vertex_ids[2] = splat_sigma;
        splat_vertex_ids[3] = splat_angle;
        vertex_buffer_ids = splat_vertex_ids;
        vertex_buffer_count = 4;
    }

    /* When IMAGE: re-narrow vertex_buffer_ids to (position, texcoords) only — the texture
     * is bound through a bind group, not as a vertex buffer. */
    uint64_t image_vertex_ids[2];
    if (is_image)
    {
        image_vertex_ids[0] = image_pos;
        image_vertex_ids[1] = image_uv;
        vertex_buffer_ids = image_vertex_ids;
        vertex_buffer_count = 2;
    }
    uint64_t textured_mesh_vertex_ids[4];
    if (is_textured_mesh)
    {
        textured_mesh_vertex_ids[0] = mesh_pos;
        textured_mesh_vertex_ids[1] = mesh_color;
        textured_mesh_vertex_ids[2] = mesh_normal;
        textured_mesh_vertex_ids[3] = mesh_uv;
        vertex_buffer_ids = textured_mesh_vertex_ids;
        vertex_buffer_count = 4;
    }

    char vs_key[32];
    char fs_key[16];
    char pipe_key[48];

    if (is_point_like)
    {
        /* Point-like visuals: native points for GLSL, instanced quads for WGSL. */
        bool picking = render->u.render.picking;
        bool query_u32 =
            picking && cfg != NULL && cfg->color_target_format == VK_FORMAT_R32_UINT;
        bool depth_cue = visual_meta != NULL && visual_meta->depth_cue_enabled && !picking;
        bool point_style = visual_meta != NULL && visual_meta->point_style_enabled && !is_pixel &&
                           !is_marker && !picking;
        const char* suffix = query_u32                  ? "_query_u32"
                             : picking                  ? "_pick"
                             : point_style && depth_cue ? "_cue_style"
                             : point_style              ? "_style"
                             : depth_cue                ? "_cue"
                                                        : "";

        DvzSceneBuiltinShader shader = DVZ_SCENE_BUILTIN_SHADER_POINT;
        if (query_u32)
            shader = (is_pixel || is_marker) ? DVZ_SCENE_BUILTIN_SHADER_PIXEL_QUERY_U32
                                             : DVZ_SCENE_BUILTIN_SHADER_POINT_QUERY_U32;
        else if (is_marker)
            shader =
                picking ? DVZ_SCENE_BUILTIN_SHADER_PIXEL_PICK : DVZ_SCENE_BUILTIN_SHADER_MARKER;
        else if (is_pixel)
            shader = picking     ? DVZ_SCENE_BUILTIN_SHADER_PIXEL_PICK
                     : depth_cue ? DVZ_SCENE_BUILTIN_SHADER_PIXEL_DEPTH_CUE
                                 : DVZ_SCENE_BUILTIN_SHADER_PIXEL;
        else if (picking)
            shader = DVZ_SCENE_BUILTIN_SHADER_POINT_PICK;
        else if (point_style)
            shader = depth_cue ? DVZ_SCENE_BUILTIN_SHADER_POINT_STYLE_DEPTH_CUE
                               : DVZ_SCENE_BUILTIN_SHADER_POINT_STYLE;
        else
            shader = depth_cue ? DVZ_SCENE_BUILTIN_SHADER_POINT_DEPTH_CUE
                               : DVZ_SCENE_BUILTIN_SHADER_POINT;

        const char* key = is_marker ? "marker" : is_pixel ? "pixel" : "point";
        dvz_snprintf(vs_key, sizeof(vs_key), "_vs_%s%s%s", key, suffix, fmt);
        dvz_snprintf(fs_key, sizeof(fs_key), "_fs_%s%s%s", key, suffix, fmt);
        vs_glsl = _builtin_shader_glsl(shader, false);
        fs_glsl = _builtin_shader_glsl(shader, true);
        vs_wgsl = _builtin_shader_wgsl(shader, false);
        fs_wgsl = _builtin_shader_wgsl(shader, true);

        uint64_t pos_id = _scene_visual_resource_by_role(
            &emitter->resources, vertex_buffer_ids, vertex_buffer_count,
            DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION);
        if (pos_id != 0)
        {
            uint64_t sz = _resource_byte_size(&emitter->resources, pos_id);
            if (sz > 0)
                vertex_count = (uint32_t)(sz / (3 * sizeof(float)));
        }
        if (visual_meta != NULL && visual_meta->vertex_count > 0)
            vertex_count = visual_meta->vertex_count;
        DvzSceneShaderFormat shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
        if (cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_WGSL)
            shader_format = DVZ_SCENE_SHADER_FORMAT_WGSL;
        has_point_like_lowering = _scene_point_like_lowering_desc(
            is_marker  ? DVZ_SCENE_POINT_LIKE_MARKER
            : is_pixel ? DVZ_SCENE_POINT_LIKE_PIXEL
                       : DVZ_SCENE_POINT_LIKE_POINT,
            shader_format, vertex_count, &point_like_lowering);
        if (!has_point_like_lowering)
            return false;
        topology = point_like_lowering.topology;
    }
    else if (is_splat)
    {
        uint64_t pos_size = _resource_byte_size(&emitter->resources, vertex_buffer_ids[0]);
        if (pos_size > 0)
            instance_count = (uint32_t)(pos_size / (3 * sizeof(float)));
        if (visual_meta != NULL && visual_meta->vertex_count > 0)
            instance_count = visual_meta->vertex_count;
        vertex_count = 6;
        topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        dvz_snprintf(vs_key, sizeof(vs_key), "_vs_splat%s", fmt);
        dvz_snprintf(fs_key, sizeof(fs_key), "_fs_splat%s", fmt);
        vs_glsl = _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_SPLAT, false);
        fs_glsl = _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_SPLAT, true);
        vs_wgsl = _builtin_shader_wgsl(DVZ_SCENE_BUILTIN_SHADER_SPLAT, false);
        fs_wgsl = _builtin_shader_wgsl(DVZ_SCENE_BUILTIN_SHADER_SPLAT, true);
    }
    else if (is_textured_mesh)
    {
        uint64_t pos_size = _resource_byte_size(&emitter->resources, mesh_pos);
        if (pos_size > 0)
            vertex_count = (uint32_t)(pos_size / (3 * sizeof(float)));
        if (visual_meta != NULL && visual_meta->vertex_count > 0)
            vertex_count = visual_meta->vertex_count;
        topology = _resource_topology(&emitter->resources, mesh_pos);
        if (topology == UINT32_MAX)
            topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        dvz_snprintf(vs_key, sizeof(vs_key), "_vs_mesh_textured%s", fmt);
        dvz_snprintf(fs_key, sizeof(fs_key), "_fs_mesh_textured%s", fmt);
        vs_glsl = _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_MESH_TEXTURED, false);
        fs_glsl = _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_MESH_TEXTURED, true);
        vs_wgsl = _builtin_shader_wgsl(DVZ_SCENE_BUILTIN_SHADER_MESH_TEXTURED, false);
        fs_wgsl = _builtin_shader_wgsl(DVZ_SCENE_BUILTIN_SHADER_MESH_TEXTURED, true);

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
            ok = ok && dvz_drp2_stream_create_sampler_filter(
                           stream, sampler_id, DVZ_DRP2_FILTER_LINEAR, DVZ_DRP2_FILTER_LINEAR);

        char bg_key[48];
        dvz_snprintf(bg_key, sizeof(bg_key), "_bg_img_%" PRIu64, mesh_tex);
        bool bg_new = false;
        bg_id = _obj_id(emitter, bg_key, &bg_new);
        if (bg_id == 0)
            return false;
        if (ok && bg_new)
            ok = ok && dvz_drp2_stream_create_texture_sampler_bind_group(
                           stream, bg_id, bgl_id, mesh_tex, sampler_id);
    }
    else if (is_primitive)
    {
        uint64_t pos_id = _scene_visual_resource_by_role(
            &emitter->resources, vertex_buffer_ids, vertex_buffer_count,
            DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION);
        if (pos_id != 0)
        {
            uint64_t sz = _resource_byte_size(&emitter->resources, pos_id);
            if (sz > 0)
                vertex_count = (uint32_t)(sz / (3 * sizeof(float)));
            topology = _resource_topology(&emitter->resources, pos_id);
        }
        if (visual_meta != NULL && visual_meta->vertex_count > 0)
            vertex_count = visual_meta->vertex_count;
        /* Primitive visual: pass-through shaders with visual-selected topology. */
        dvz_snprintf(vs_key, sizeof(vs_key), "_vs_prim%s", fmt);
        dvz_snprintf(fs_key, sizeof(fs_key), "_fs_prim%s", fmt);
        vs_glsl = _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE, false);
        fs_glsl = _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE, true);
        vs_wgsl = _builtin_shader_wgsl(DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE, false);
        fs_wgsl = _builtin_shader_wgsl(DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE, true);
    }
    else if (is_image)
    {
        /* Image visual: textured-quad shaders, TRIANGLE_STRIP topology, 4 vertices. */
        uint64_t pos_size = _resource_byte_size(&emitter->resources, image_pos);
        if (pos_size > 0)
            vertex_count = (uint32_t)(pos_size / (3 * sizeof(float)));
        if (visual_meta != NULL && visual_meta->vertex_count > 0)
            vertex_count = visual_meta->vertex_count;
        topology = _resource_topology(&emitter->resources, image_pos);
        if (topology == UINT32_MAX)
            topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        DvzSceneBuiltinShader image_shader =
            image_pixel_space ? DVZ_SCENE_BUILTIN_SHADER_IMAGE_PIXEL
                              : DVZ_SCENE_BUILTIN_SHADER_IMAGE;
        const char* shader_name = image_pixel_space ? "img_px" : "img";
        if (is_labels_sint)
        {
            image_shader = labels_query_u32 ? DVZ_SCENE_BUILTIN_SHADER_LABELS_SINT_QUERY_U32
                                            : DVZ_SCENE_BUILTIN_SHADER_LABELS_SINT;
            shader_name = labels_query_u32 ? "labels_sint_query_u32" : "labels_sint";
        }
        else if (is_labels_uint)
        {
            image_shader = labels_query_u32 ? DVZ_SCENE_BUILTIN_SHADER_LABELS_UINT_QUERY_U32
                                            : DVZ_SCENE_BUILTIN_SHADER_LABELS_UINT;
            shader_name = labels_query_u32 ? "labels_uint_query_u32" : "labels_uint";
        }
        dvz_snprintf(vs_key, sizeof(vs_key), "_vs_%s%s", shader_name, fmt);
        dvz_snprintf(fs_key, sizeof(fs_key), "_fs_%s%s", shader_name, fmt);
        vs_glsl = _builtin_shader_glsl(image_shader, false);
        fs_glsl = _builtin_shader_glsl(image_shader, true);
        vs_wgsl = _builtin_shader_wgsl(image_shader, false);
        fs_wgsl = _builtin_shader_wgsl(image_shader, true);

        bool labels_nearest = is_labels_sint || is_labels_uint;
        if (labels_nearest)
        {
            bool bgl_new = false;
            bgl_id = _obj_id(emitter, "_bgl_labels", &bgl_new);
            if (bgl_id == 0)
                return false;
            if (bgl_new)
                ok = ok && _create_labels_bind_group_layout(stream, bgl_id);

            bool sampler_new = false;
            uint64_t sampler_id = _obj_id(emitter, "_sampler_labels_nearest", &sampler_new);
            if (sampler_id == 0)
                return false;
            if (ok && sampler_new)
                ok = ok && dvz_drp2_stream_create_sampler_filter(
                               stream, sampler_id, DVZ_DRP2_FILTER_NEAREST,
                               DVZ_DRP2_FILTER_NEAREST);

            DvzLabelsState labels_state = {0};
            labels_state.opacity = 1.0f;
            labels_state.boundary_width_px = 1.0f;
            labels_state.boundary_color = (DvzColor){255, 255, 255, 255};
            if (visual_meta != NULL && visual_meta->has_labels)
                labels_state = visual_meta->labels_state;
            DvzSceneVisualBindDesc bind = {
                .labels_texture_id = image_tex,
                .labels_visual_index = visual_meta != NULL ? visual_meta->visual_index : 0,
                .labels_state = labels_state,
            };
            ok = ok && _resolve_labels_bind_group(emitter, stream, bgl_id, sampler_id, &bind, &bg_id);
        }
        else
        {
            bool bgl_new = false;
            bgl_id = _obj_id(emitter, "_bgl_img", &bgl_new);
            if (bgl_id == 0)
                return false;
            if (bgl_new)
                ok = ok && dvz_drp2_stream_create_texture_sampler_bind_group_layout(stream, bgl_id);

            bool sampler_new = false;
            bool nearest_image_sampler =
                visual_meta != NULL && visual_meta->image_nearest_sampler;
            uint64_t sampler_id = _obj_id(
                emitter, nearest_image_sampler ? "_sampler_img_nearest" : "_sampler_img",
                &sampler_new);
            if (sampler_id == 0)
                return false;
            if (ok && sampler_new)
            {
                DvzDrp2FilterMode filter =
                    nearest_image_sampler ? DVZ_DRP2_FILTER_NEAREST : DVZ_DRP2_FILTER_LINEAR;
                ok = ok && dvz_drp2_stream_create_sampler_filter(
                               stream, sampler_id, filter, filter);
            }

            char bg_key[48];
            dvz_snprintf(
                bg_key, sizeof(bg_key),
                nearest_image_sampler ? "_bg_img_nearest_%" PRIu64 : "_bg_img_%" PRIu64,
                image_tex);
            bool bg_new = false;
            bg_id = _obj_id(emitter, bg_key, &bg_new);
            if (bg_id == 0)
                return false;
            if (ok && bg_new)
                ok = ok && dvz_drp2_stream_create_texture_sampler_bind_group(
                               stream, bg_id, bgl_id, image_tex, sampler_id);
        }
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

    /* Common bind group infrastructure. */
    if (uses_common)
    {
        ok = ok && _scene_common_bindings_resolve_single_set(
                       emitter, stream, render, &common_bgl_id, &common_bg_id);
        if (!ok)
            return false;
    }

    /* SPIR-V resource names (stem of .vert.spv / .frag.spv after embed_resources key mangling). */
    const char* vs_spirv_key = NULL;
    const char* fs_spirv_key = NULL;
    if (is_point_like)
    {
        bool picking = render->u.render.picking;
        bool query_u32 =
            picking && cfg != NULL && cfg->color_target_format == VK_FORMAT_R32_UINT;
        bool depth_cue = visual_meta != NULL && visual_meta->depth_cue_enabled && !picking;
        bool point_style = visual_meta != NULL && visual_meta->point_style_enabled && !is_pixel &&
                           !is_marker && !picking;
        if (query_u32)
        {
            vs_spirv_key = (is_pixel || is_marker) ? "pixel_pick_vert" : "point_pick_vert";
            fs_spirv_key =
                (is_pixel || is_marker) ? "pixel_query_u32_frag" : "point_query_u32_frag";
        }
        else if (picking)
        {
            vs_spirv_key = (is_pixel || is_marker) ? "pixel_pick_vert" : "point_pick_vert";
            fs_spirv_key = (is_pixel || is_marker) ? "pixel_pick_frag" : "point_pick_frag";
        }
        else if (is_marker)
        {
            vs_spirv_key = "marker_vert";
            fs_spirv_key = "marker_frag";
        }
        else if (is_pixel)
        {
            vs_spirv_key = depth_cue ? "pixel_cue_vert" : "pixel_vert";
            fs_spirv_key = depth_cue ? "pixel_cue_frag" : "pixel_frag";
        }
        else if (point_style)
        {
            vs_spirv_key = depth_cue ? "point_cue_style_vert" : "point_style_vert";
            fs_spirv_key = depth_cue ? "point_cue_style_frag" : "point_style_frag";
        }
        else
        {
            vs_spirv_key = depth_cue ? "point_cue_vert" : "point_vert";
            fs_spirv_key = depth_cue ? "point_cue_frag" : "point_frag";
        }
    }
    else if (is_textured_mesh)
    {
        vs_spirv_key = "mesh_textured_vert";
        fs_spirv_key = "mesh_textured_frag";
    }
    else if (is_splat)
    {
        vs_spirv_key = "splat_vert";
        fs_spirv_key = "splat_frag";
    }
    else if (is_primitive)
    {
        vs_spirv_key = "primitive_vert";
        fs_spirv_key = "primitive_frag";
    }
    else if (is_image)
    {
        vs_spirv_key = image_pixel_space ? "image_pixel_vert" : "image_vert";
        fs_spirv_key = is_labels_sint && labels_query_u32  ? "labels_sint_query_u32_frag"
                       : is_labels_uint && labels_query_u32 ? "labels_uint_query_u32_frag"
                       : is_labels_sint                     ? "labels_sint_frag"
                       : is_labels_uint                     ? "labels_uint_frag"
                                                            : "image_frag";
    }

    uint64_t vs_id = _obj_id(emitter, vs_key, &is_new);
    if (vs_id == 0)
        return false;
    if (is_new)
    {
        if (cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_WGSL && vs_wgsl != NULL)
        {
            ok = ok && _emit_shader(stream, vs_id, "VERTEX", vs_wgsl, vs_glsl, cfg);
        }
        else if (
            vs_glsl != NULL && vs_spirv_key != NULL && cfg != NULL &&
            cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
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
        if (cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_WGSL && fs_wgsl != NULL)
        {
            ok = ok && _emit_shader(stream, fs_id, "FRAGMENT", fs_wgsl, fs_glsl, cfg);
        }
        else if (
            fs_glsl != NULL && fs_spirv_key != NULL && cfg != NULL &&
            cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
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
                           stream, fs_id, "FRAGMENT", _fixture_fragment_wgsl(),
                           _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_FIXTURE, true), cfg);
        }
    }

    if (is_point_like)
    {
        bool picking = render->u.render.picking;
        bool query_u32 =
            picking && cfg != NULL && cfg->color_target_format == VK_FORMAT_R32_UINT;
        bool depth_cue = visual_meta != NULL && visual_meta->depth_cue_enabled && !picking;
        bool point_style = visual_meta != NULL && visual_meta->point_style_enabled && !is_pixel &&
                           !is_marker && !picking;
        const char* suffix = query_u32                  ? "_query_u32"
                             : picking                  ? "_pick"
                             : point_style && depth_cue ? "_cue_style"
                             : point_style              ? "_style"
                             : depth_cue                ? "_cue"
                                                        : "";
        dvz_snprintf(
            pipe_key, sizeof(pipe_key), "_pipe_%s%s%s",
            is_marker  ? "marker"
            : is_pixel ? "pixel"
                       : "point",
            suffix, fmt);
    }
    else if (is_splat)
        dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe_splat%s", fmt);
    else if (is_textured_mesh)
        dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe_mesh_textured_t%u%s", topology, fmt);
    else if (is_primitive)
        dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe_prim_t%u%s", topology, fmt);
    else if (is_image)
    {
        const char* pipe_name = is_labels_sint  ? "labels_sint"
                                : is_labels_uint ? "labels_uint"
                                : image_pixel_space ? "img_px"
                                                 : "img";
        dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe_%s%s", pipe_name, fmt);
    }
    else
        dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe%u%s", vertex_buffer_count, fmt);

    uint64_t pipe_id = _obj_id(emitter, pipe_key, &is_new);
    if (pipe_id == 0)
        return false;
    if (ok && is_new)
    {
        if (is_point_like)
        {
            uint32_t strides[5] = {
                3 * sizeof(float), 4 * sizeof(uint8_t), sizeof(float), sizeof(float),
                sizeof(uint32_t)};
            uint32_t bindings[5] = {0, 1, 2, 3, 4};
            uint32_t locations[5] = {0, 1, 2, 3, 4};
            uint32_t formats[5] = {
                VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R32_SFLOAT,
                VK_FORMAT_R32_SFLOAT, VK_FORMAT_R32_UINT};
            uint32_t offsets[5] = {0, 0, 0, 0, 0};
            uint32_t point_like_attr_count = is_marker && !render->u.render.picking ? 5 : 3;
            if (point_like_lowering.lowering == DVZ_SCENE_POINT_LIKE_LOWERING_INSTANCED_QUADS)
            {
                uint32_t step_modes[5] = {
                    point_like_lowering.vertex_step_mode, point_like_lowering.vertex_step_mode,
                    point_like_lowering.vertex_step_mode, point_like_lowering.vertex_step_mode,
                    point_like_lowering.vertex_step_mode,
                };
                ok = ok && dvz_drp2_stream_create_render_pipeline_ex2(
                               stream, pipe_id, vs_id, fs_id, vertex_buffer_count, topology,
                               point_like_attr_count, strides, step_modes, point_like_attr_count,
                               bindings, locations, formats, offsets);
            }
            else
            {
                ok = ok && dvz_drp2_stream_create_render_pipeline_ex(
                               stream, pipe_id, vs_id, fs_id, vertex_buffer_count, topology,
                               point_like_attr_count, strides, point_like_attr_count, bindings,
                               locations, formats, offsets);
            }
            if (ok && uses_common && common_bgl_id != 0)
                ok = dvz_drp2_stream_pipeline_set_bind_group_layout(stream, common_bgl_id);
        }
        else if (is_splat)
        {
            uint32_t strides[4] = {
                3 * sizeof(float), 4 * sizeof(uint8_t), 2 * sizeof(float), sizeof(float)};
            uint32_t step_modes[4] = {
                DVZ_DRP2_VERTEX_STEP_MODE_INSTANCE, DVZ_DRP2_VERTEX_STEP_MODE_INSTANCE,
                DVZ_DRP2_VERTEX_STEP_MODE_INSTANCE,
                DVZ_DRP2_VERTEX_STEP_MODE_INSTANCE};
            uint32_t bindings[4] = {0, 1, 2, 3};
            uint32_t locations[4] = {0, 1, 2, 3};
            uint32_t formats[4] = {
                VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R8G8B8A8_UNORM,
                VK_FORMAT_R32G32_SFLOAT, VK_FORMAT_R32_SFLOAT};
            uint32_t offsets[4] = {0, 0, 0, 0};
            ok = ok && dvz_drp2_stream_create_render_pipeline_ex2(
                           stream, pipe_id, vs_id, fs_id, vertex_buffer_count, topology, 4,
                           strides, step_modes, 4, bindings, locations, formats, offsets);
            if (ok && uses_common && common_bgl_id != 0)
                ok = dvz_drp2_stream_pipeline_set_bind_group_layout(stream, common_bgl_id);
        }
        else if (is_textured_mesh)
        {
            uint32_t strides[4] = {
                3 * sizeof(float), 4 * sizeof(uint8_t), 3 * sizeof(float),
                2 * sizeof(float)};
            uint32_t bindings[4] = {0, 1, 2, 3};
            uint32_t locations[4] = {0, 1, 2, 3};
            uint32_t formats[4] = {
                VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R8G8B8A8_UNORM,
                VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32_SFLOAT};
            uint32_t offsets[4] = {0, 0, 0, 0};
            ok = ok && dvz_drp2_stream_create_render_pipeline_ex(
                           stream, pipe_id, vs_id, fs_id, vertex_buffer_count, topology, 4,
                           strides, 4, bindings, locations, formats, offsets);
            if (ok && uses_common && common_bgl_id != 0)
                ok = dvz_drp2_stream_pipeline_set_bind_group_layout(stream, common_bgl_id);
            if (ok && bgl_id != 0)
                ok = dvz_drp2_stream_pipeline_set_bind_group_layout2(stream, bgl_id);
        }
        else if (is_primitive)
        {
            /* binding0=position(vec3), binding1=color(u8vec4) */
            uint32_t strides[2] = {3 * sizeof(float), 4 * sizeof(uint8_t)};
            uint32_t bindings[2] = {0, 1};
            uint32_t locations[2] = {0, 1};
            uint32_t formats[2] = {VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R8G8B8A8_UNORM};
            uint32_t offsets[2] = {0, 0};
            ok = ok && dvz_drp2_stream_create_render_pipeline_ex(
                           stream, pipe_id, vs_id, fs_id, vertex_buffer_count, topology, 2,
                           strides, 2, bindings, locations, formats, offsets);
            if (ok && uses_common && common_bgl_id != 0)
                ok = dvz_drp2_stream_pipeline_set_bind_group_layout(stream, common_bgl_id);
        }
        else if (is_image)
        {
            /* binding0=position(vec3), binding1=texcoords(vec2); set0=common, set1=image */
            uint32_t strides[2] = {3 * sizeof(float), 2 * sizeof(float)};
            uint32_t bindings[2] = {0, 1};
            uint32_t locations[2] = {0, 1};
            uint32_t formats[2] = {VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32_SFLOAT};
            uint32_t offsets[2] = {0, 0};
            ok = ok && dvz_drp2_stream_create_render_pipeline_ex(
                           stream, pipe_id, vs_id, fs_id, vertex_buffer_count, topology, 2,
                           strides, 2, bindings, locations, formats, offsets);
            if (ok && uses_common && common_bgl_id != 0)
                ok = dvz_drp2_stream_pipeline_set_bind_group_layout(stream, common_bgl_id);
            if (ok && bgl_id != 0)
                ok = dvz_drp2_stream_pipeline_set_bind_group_layout2(stream, bgl_id);
        }
        else
        {
            ok = ok && dvz_drp2_stream_create_render_pipeline(
                           stream, pipe_id, vs_id, fs_id, vertex_buffer_count);
        }
        if (ok && cfg != NULL && cfg->color_target_format != 0)
            ok = dvz_drp2_stream_pipeline_set_color_target(stream, 0, cfg->color_target_format);
    }

    uint64_t color_id = 0;
    ok = ok && _render_pass_resolve_color_target(emitter, stream, cfg, &color_id);

    uint64_t rb_id = 0;
    ok = ok && _render_pass_resolve_readback_buffer(emitter, stream, readback, &rb_id);

    if (!ok)
        return false;

    uint64_t encoder_id = 0;
    uint64_t render_pass_id = 0;
    uint64_t command_buffer_id = 0;
    uint64_t submission_id = 0;
    _render_pass_next_ids(
        emitter, &encoder_id, &render_pass_id, &command_buffer_id, &submission_id);

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
    if (ok && uses_common && common_bg_id != 0)
        ok = dvz_drp2_stream_set_bind_group(stream, render_pass_id, 0, common_bg_id);
    if (ok && (is_textured_mesh || is_image) && bg_id != 0)
        ok = dvz_drp2_stream_set_bind_group(stream, render_pass_id, 1, bg_id);
    uint32_t draw_vertex_count = vertex_count;
    uint32_t draw_instance_count = instance_count;
    if (is_point_like && has_point_like_lowering)
    {
        draw_vertex_count = point_like_lowering.draw_vertex_count;
        draw_instance_count = point_like_lowering.draw_instance_count;
    }
    if (is_point_like || is_splat || is_textured_mesh || is_primitive || is_image)
    {
        DvzSceneVisualDescKind kind = is_image          ? DVZ_SCENE_VISUAL_DESC_IMAGE
                                      : is_textured_mesh ? DVZ_SCENE_VISUAL_DESC_TEXTURED_MESH
                                      : is_primitive     ? DVZ_SCENE_VISUAL_DESC_PRIMITIVE
                                      : is_splat         ? DVZ_SCENE_VISUAL_DESC_SPLAT
                                      : is_marker        ? DVZ_SCENE_VISUAL_DESC_MARKER
                                      : is_pixel         ? DVZ_SCENE_VISUAL_DESC_PIXEL
                                                         : DVZ_SCENE_VISUAL_DESC_POINT;
        if (is_labels_sint)
            kind = DVZ_SCENE_VISUAL_DESC_LABELS_SINT;
        else if (is_labels_uint)
            kind = DVZ_SCENE_VISUAL_DESC_LABELS_UINT;
        bool instanced_point_like =
            is_splat || (has_point_like_lowering &&
                         point_like_lowering.lowering ==
                             DVZ_SCENE_POINT_LIKE_LOWERING_INSTANCED_QUADS);
        SceneDrawPacket packet = {0};
        if (!_scene_draw_packet_init_fallback(
                &emitter->resources, kind, pipe_id, common_bg_id,
                (is_textured_mesh || is_image) ? bg_id : 0,
                vertex_buffer_ids, vertex_buffer_count, draw_vertex_count, draw_instance_count,
                instanced_point_like, report, &packet))
            return false;
        ok = ok && _scene_draw_packet_emit(stream, render_pass_id, &packet);
    }
    else
    {
        for (uint32_t i = 0; ok && i < vertex_buffer_count; i++)
            ok = dvz_drp2_stream_set_vertex_buffer(
                stream, render_pass_id, i, vertex_buffer_ids[i], 0);
        ok = ok && dvz_drp2_stream_draw(
                       stream, render_pass_id, draw_vertex_count, draw_instance_count, 0, 0);
    }
    ok = ok && dvz_drp2_stream_end_render_pass(stream, render_pass_id);
    ok =
        ok && _render_pass_copy_finish_submit(
                  stream, encoder_id, command_buffer_id, submission_id, color_id, rb_id, readback);
    return ok;
}
