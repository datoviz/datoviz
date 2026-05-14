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

#include <vulkan/vulkan_core.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_frame_plan_emit.h"
#include "_frame_plan.h"
#include "_frame_plan_runtime_upload.h"
#include "_mvp_bindings.h"
#include "_render_pass.h"
#include "_shader_registry.h"
#include "_visual_pipeline.h"
#include "datoviz/drp2.h"
#include "datoviz/drp2/stream.h"
#include "datoviz/scene.h"
#include "_scene.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct SceneRenderDraw SceneRenderDraw;
typedef struct SceneRenderBatch SceneRenderBatch;

struct SceneRenderDraw
{
    uint64_t pipeline_id;
    uint64_t bg_set0;  /* MVP bg; 0 = none */
    uint64_t bg_set1;  /* image texture or primitive shading bg; 0 = none */
    DvzSceneVisualDesc visual;
};


struct SceneRenderBatch
{
    const DvzFramePlanNode* render;
    SceneRenderDraw draws[DVZ_SCENE_MAX_RENDER_VISUALS];
    uint32_t draw_count;
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Prepare resources for one panel's draws before opening the render pass.
 *
 * @param emitter frame-plan emitter carrying scene/runtime state.
 * @param stream destination DRP2 command stream.
 * @param render render node to prepare.
 * @param cfg optional frame-plan emit configuration.
 * @param report diagnostic report receiving recoverable emission errors.
 * @param draws output draw descriptors filled from prepared visuals.
 * @param draw_count_out output number of prepared draw descriptors.
 * @return true when the render node has drawable prepared visuals, false otherwise.
 */
static bool _emitter_prepare_render_multi(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlanNode* render,
    const DvzFramePlanEmitConfig* cfg, DvzDiagnosticReport* report, SceneRenderDraw* draws,
    uint32_t* draw_count_out)
{
    ANN(emitter);
    ANN(stream);
    ANN(render);
    ANN(draws);
    ANN(draw_count_out);

    bool ok = true;
    bool is_new = false;
    const char* fmt = _shader_format_tag(cfg);
    bool pass_needs_depth = _scene_render_needs_depth(emitter, render);

    uint64_t mvp_bgl_id = 0;
    uint64_t apply_bg_id = 0;
    uint64_t fixed_bg_id = 0;
    if (!_mvp_bindings_resolve_panel_sets(
            emitter, stream, render, &mvp_bgl_id, &apply_bg_id, &fixed_bg_id))
        return false;

    /* Image BGL + sampler (shared, created lazily on first image visual). */
    uint64_t img_bgl_id = 0, img_sampler_id = 0;

    uint32_t draw_count = 0;

    for (uint32_t i = 0; ok && i < render->u.render.visual_count; i++)
    {
        DvzSceneVisualDesc desc = {0};
        const char* visual_error = NULL;
        if (!_scene_visual_desc_from_render(emitter, render, i, &desc, &visual_error))
        {
            if (render->u.render.visual_metadata[i].has_metadata)
            {
                _diagnostic(
                    report, visual_error != NULL ? visual_error :
                                                  "invalid typed visual metadata");
                ok = false;
                break;
            }
            continue;
        }

        bool vis_is_point = desc.kind == DVZ_SCENE_VISUAL_DESC_POINT;
        bool vis_is_prim = desc.kind == DVZ_SCENE_VISUAL_DESC_PRIMITIVE;
        bool vis_is_image = desc.kind == DVZ_SCENE_VISUAL_DESC_IMAGE;

        DvzSceneVisualShaderDesc shader = {0};
        if (!_scene_visual_shader_desc(&desc, render->u.render.picking, fmt, &shader))
            continue;

        /* Shaders (cached). */
        uint64_t vs_id = _obj_id(emitter, shader.vertex_key, &is_new);
        if (vs_id == 0) { ok = false; break; }
        if (is_new)
        {
            if (cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_WGSL)
            {
                if (shader.vertex_wgsl == NULL)
                    ok = false;
                else
                    ok = ok && _emit_shader(
                                     stream, vs_id, "VERTEX", shader.vertex_wgsl,
                                     shader.vertex_glsl, cfg);
            }
            else if (shader.vertex_spirv_key != NULL)
                ok = ok && _emit_shader_spirv(
                               stream, vs_id, "VERTEX", shader.vertex_spirv_key,
                               shader.vertex_glsl, cfg);
            else
                ok = ok && _emit_shader(stream, vs_id, "VERTEX", NULL, shader.vertex_glsl, cfg);
        }

        uint64_t fs_id = _obj_id(emitter, shader.fragment_key, &is_new);
        if (fs_id == 0) { ok = false; break; }
        if (ok && is_new)
        {
            if (cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_WGSL)
            {
                if (shader.fragment_wgsl == NULL)
                    ok = false;
                else
                    ok = ok &&
                         _emit_shader(
                             stream, fs_id, "FRAGMENT", shader.fragment_wgsl,
                             shader.fragment_glsl, cfg);
            }
            else if (shader.fragment_spirv_key != NULL)
                ok = ok && _emit_shader_spirv(
                               stream, fs_id, "FRAGMENT", shader.fragment_spirv_key,
                               shader.fragment_glsl, cfg);
            else
                ok = ok &&
                     _emit_shader(stream, fs_id, "FRAGMENT", NULL, shader.fragment_glsl, cfg);
        }

        uint64_t pipe_id = _obj_id(emitter, shader.pipeline_key, &is_new);
        if (pipe_id == 0) { ok = false; break; }
        if (ok && is_new)
        {
            DvzSceneVisualPipelineDesc pipeline = {0};
            if (!_scene_visual_pipeline_desc(
                    &desc, render->u.render.picking, pass_needs_depth, &pipeline))
            {
                ok = false;
                break;
            }
            uint64_t shading_bgl_id = 0;
            if (pipeline.needs_shading_layout)
            {
                bool shading_bgl_new = false;
                shading_bgl_id = _obj_id(emitter, "_bgl_prim_shading", &shading_bgl_new);
                if (shading_bgl_id == 0) { ok = false; break; }
                if (shading_bgl_new)
                    ok = ok &&
                         dvz_drp2_stream_create_uniform_bind_group_layout(stream, shading_bgl_id);
            }
            if (pipeline.needs_image_layout && img_bgl_id == 0)
            {
                img_bgl_id = _obj_id(emitter, "_bgl_img", &is_new);
                if (img_bgl_id == 0) { ok = false; break; }
                if (is_new)
                    ok = ok && dvz_drp2_stream_create_texture_sampler_bind_group_layout(
                                   stream, img_bgl_id);
            }
            ok = ok && dvz_drp2_stream_create_render_pipeline_ex(
                           stream, pipe_id, vs_id, fs_id, pipeline.vertex_buffer_count,
                           pipeline.topology, pipeline.binding_count, pipeline.strides,
                           pipeline.attr_count, pipeline.bindings, pipeline.locations,
                           pipeline.formats, pipeline.offsets);
            if (ok && pipeline.needs_mvp_layout && mvp_bgl_id != 0)
                ok = dvz_drp2_stream_pipeline_set_bind_group_layout(stream, mvp_bgl_id);
            if (ok && pipeline.needs_image_layout && img_bgl_id != 0 &&
                !pipeline.needs_mvp_layout)
                ok = dvz_drp2_stream_pipeline_set_bind_group_layout(stream, img_bgl_id);
            if (ok && pipeline.needs_image_layout && img_bgl_id != 0 &&
                pipeline.needs_mvp_layout)
                ok = dvz_drp2_stream_pipeline_set_bind_group_layout2(stream, img_bgl_id);
            if (ok && pipeline.needs_shading_layout)
                ok = dvz_drp2_stream_pipeline_set_bind_group_layout2(stream, shading_bgl_id);
            if (ok && pipeline.has_depth_state)
                ok = dvz_drp2_stream_pipeline_set_depth_state(
                    stream, pipeline.depth_write_enabled, pipeline.depth_compare_op);
        }

        /* Bind group at set 0. */
        uint64_t vis_bg_set0 = 0;
        uint64_t vis_bg_set1 = 0;
        DvzSceneVisualBindDesc bind = {0};
        if (!_scene_visual_bind_desc(&desc, render->u.render.controller_modes[i], &bind))
        {
            ok = false;
            break;
        }
        if (bind.uses_mvp_set0)
            vis_bg_set0 = bind.uses_fixed_mvp ? fixed_bg_id : apply_bg_id;
        if (bind.uses_shading_set1)
        {
            bool shading_bgl_new = false;
            uint64_t shading_bgl_id = _obj_id(emitter, "_bgl_prim_shading", &shading_bgl_new);
            if (shading_bgl_id == 0) { ok = false; break; }
            if (shading_bgl_new)
                ok = ok && dvz_drp2_stream_create_uniform_bind_group_layout(
                               stream, shading_bgl_id);
            char shading_bg_key[64];
            dvz_snprintf(
                shading_bg_key, sizeof(shading_bg_key), "_bg_prim_shading_%" PRIu64,
                bind.shading_buffer_id);
            uint64_t shading_bg_id = _obj_id(emitter, shading_bg_key, &is_new);
            if (shading_bg_id == 0) { ok = false; break; }
            if (ok && is_new)
                ok = ok && dvz_drp2_stream_create_uniform_bind_group(
                               stream, shading_bg_id, shading_bgl_id,
                               bind.shading_buffer_id, 0,
                               sizeof(DvzPrimitiveShadingState));
            vis_bg_set1 = shading_bg_id;
        }
        if (bind.uses_image_set1)
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
            dvz_snprintf(
                img_bg_key, sizeof(img_bg_key), "_bg_img_%" PRIu64, bind.image_texture_id);
            uint64_t img_bg_id = _obj_id(emitter, img_bg_key, &is_new);
            if (img_bg_id == 0) { ok = false; break; }
            if (ok && is_new)
                ok = ok && dvz_drp2_stream_create_texture_sampler_bind_group(
                               stream, img_bg_id, img_bgl_id, bind.image_texture_id,
                               img_sampler_id);
            vis_bg_set1 = img_bg_id;
        }

        if (!ok)
            break;

        draws[draw_count].pipeline_id = pipe_id;
        draws[draw_count].bg_set0     = vis_bg_set0;
        draws[draw_count].bg_set1     = vis_bg_set1;
        draws[draw_count].visual      = desc;
        draw_count++;
    }

    if (!ok || draw_count == 0)
        return false;

    *draw_count_out = draw_count;
    return true;
}



/**
 * Emit one panel's already-prepared draws inside an open render pass.
 *
 * @param stream destination DRP2 command stream.
 * @param render render node whose viewport/scissor and visuals are emitted.
 * @param render_pass_id active render-pass id.
 * @param draws prepared draw descriptors.
 * @param draw_count number of prepared draw descriptors.
 * @param cache optional state cache shared across panels in the same render pass.
 * @return true when all draw commands were emitted successfully, false otherwise.
 */
static bool _emitter_emit_render_multi_draws(
    DvzDrp2CommandStream* stream, const DvzFramePlanNode* render, uint64_t render_pass_id,
    const SceneRenderDraw* draws, uint32_t draw_count, SceneRenderStateCache* cache)
{
    ANN(stream);
    ANN(render);
    ANN(draws);

    bool ok = dvz_drp2_stream_set_viewport(
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
        for (uint32_t j = 0; ok && j < draws[d].visual.vbuf_count; j++)
            ok = ok && dvz_drp2_stream_set_vertex_buffer(
                           stream, render_pass_id, j, draws[d].visual.vbuf_ids[j], 0);
        if (ok && draws[d].visual.index_buffer_id != 0)
        {
            ok = ok &&
                 dvz_drp2_stream_set_index_buffer(
                     stream, render_pass_id, draws[d].visual.index_buffer_id,
                     draws[d].visual.index_format, 0) &&
                 dvz_drp2_stream_draw_indexed(
                     stream, render_pass_id, draws[d].visual.index_count, 1, 0, 0, 0);
        }
        else
        {
            ok = ok &&
                 dvz_drp2_stream_draw(
                     stream, render_pass_id, draws[d].visual.vertex_count, 1, 0, 0);
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
    SceneRenderStateCache* cache, DvzDiagnosticReport* report)
{
    ANN(emitter);
    ANN(stream);
    ANN(render);

    uint64_t color_id = 0;
    if (!_render_pass_resolve_color_target(emitter, stream, cfg, &color_id))
        return false;

    uint64_t rb_id = 0;
    if (!_render_pass_resolve_readback_buffer(emitter, stream, readback, &rb_id))
        return false;

    bool ok = true;

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
    bool needs_depth = _scene_render_needs_depth(emitter, render);

    SceneRenderDraw draws[DVZ_SCENE_MAX_RENDER_VISUALS] = {0};
    uint32_t draw_count = 0;
    ok = _emitter_prepare_render_multi(
        emitter, stream, render, cfg, report, draws, &draw_count);
    if (!ok)
        return false;

    ok = dvz_drp2_stream_begin_command_encoder(stream, encoder_id) &&
         dvz_drp2_stream_begin_render_pass_region_clear(
             stream, render_pass_id, encoder_id, color_id, cr, cg, cb, ca, 0.0f, 0.0f, 1.0f,
             1.0f, clear);
    if (ok && needs_depth)
        ok = dvz_drp2_stream_begin_render_pass_set_depth(stream, 1.0f);
    ok = ok &&
         _emitter_emit_render_multi_draws(
             stream, render, render_pass_id, draws, draw_count, cache) &&
         dvz_drp2_stream_end_render_pass(stream, render_pass_id);
    ok = ok && _render_pass_copy_finish_submit(
                   stream, encoder_id, command_buffer_id, submission_id, color_id, rb_id,
                   readback);
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
    const DvzFramePlanNode* readback, const DvzFramePlanEmitConfig* cfg,
    DvzDiagnosticReport* report)
{
    ANN(emitter);
    ANN(stream);
    ANN(plan);

    uint64_t color_id = 0;
    if (!_render_pass_resolve_color_target(emitter, stream, cfg, &color_id))
        return false;

    uint64_t rb_id = 0;
    if (!_render_pass_resolve_readback_buffer(emitter, stream, readback, &rb_id))
        return false;

    bool ok = true;

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

    SceneRenderBatch* batches =
        (SceneRenderBatch*)dvz_calloc(plan->count, sizeof(SceneRenderBatch));
    if (batches == NULL)
        return false;
    uint32_t batch_count = 0;
    for (uint32_t i = 0; ok && i < plan->count; i++)
    {
        const DvzFramePlanNode* render = &plan->nodes[i];
        if (render->type != DVZ_FRAME_PLAN_NODE_RENDER || render->u.render.visual_count == 0)
            continue;
        SceneRenderBatch* batch = &batches[batch_count];
        batch->render = render;
        ok = _emitter_prepare_render_multi(
            emitter, stream, render, cfg, report, batch->draws, &batch->draw_count);
        if (ok)
            batch_count++;
    }
    if (!ok || batch_count == 0)
    {
        dvz_free(batches);
        return false;
    }

    ok = dvz_drp2_stream_begin_command_encoder(stream, encoder_id) &&
         dvz_drp2_stream_begin_render_pass_region_clear(
             stream, render_pass_id, encoder_id, color_id, cr, cg, cb, ca, 0.0f, 0.0f, 1.0f,
             1.0f, true);

    SceneRenderStateCache scene_cache = {0};
    for (uint32_t i = 0; ok && i < batch_count; i++)
    {
        ok = _emitter_emit_render_multi_draws(
            stream, batches[i].render, render_pass_id, batches[i].draws, batches[i].draw_count,
            &scene_cache);
    }

    ok = ok && dvz_drp2_stream_end_render_pass(stream, render_pass_id);
    ok = ok && _render_pass_copy_finish_submit(
                   stream, encoder_id, command_buffer_id, submission_id, color_id, rb_id,
                   readback);
    dvz_free(batches);
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
    SceneRenderStateCache* cache, DvzDiagnosticReport* report)
{
    ANN(emitter);
    ANN(stream);
    ANN(render);

    /* Scene render node: per-visual multi-draw in a single pass. */
    if (vertex_buffer_count == 0 && render->u.render.visual_count > 0 &&
        cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
        return _emitter_emit_render_multi(
            emitter, stream, render, readback, clear, cfg, cache, report);

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
    const char* vs_wgsl = NULL;
    const char* fs_wgsl = NULL;
    uint32_t topology = 0;
    uint32_t vertex_count = 3; /* default for stub / non-point path */
    uint64_t bgl_id = 0;
    uint64_t bg_id  = 0;

    /* MVP UBO bind group IDs — used for GLSL point/primitive/image paths. */
    uint64_t mvp_bgl_id = 0;
    uint64_t mvp_bg_id  = 0;
    bool uses_mvp =
        (is_point || is_primitive || is_image) &&
        cfg != NULL &&
        (cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL ||
         (cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_WGSL && is_primitive));

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

        uint64_t pos_id = _scene_visual_resource_by_role(
            &emitter->resources, vertex_buffer_ids, vertex_buffer_count,
            DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION);
        if (pos_id != 0)
        {
            uint64_t sz = _resource_byte_size(&emitter->resources, pos_id);
            if (sz > 0)
                vertex_count = (uint32_t)(sz / (3 * sizeof(float)));
        }
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
        /* Primitive visual: pass-through shaders with visual-selected topology. */
        dvz_snprintf(vs_key, sizeof(vs_key), "_vs_prim%s", fmt);
        dvz_snprintf(fs_key, sizeof(fs_key), "_fs_prim%s", fmt);
        vs_glsl = _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE, false);
        fs_glsl = _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE, true);
        vs_wgsl = _builtin_shader_wgsl(DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE, false);
        fs_wgsl = _builtin_shader_wgsl(DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE, true);
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
        ok = ok && _mvp_bindings_resolve_single_set(
                       emitter, stream, render, &mvp_bgl_id, &mvp_bg_id);
        if (!ok)
            return false;
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
        if (cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_WGSL && vs_wgsl != NULL)
        {
            ok = ok && _emit_shader(stream, vs_id, "VERTEX", vs_wgsl, vs_glsl, cfg);
        }
        else if (vs_glsl != NULL && vs_spirv_key != NULL &&
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
        if (cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_WGSL && fs_wgsl != NULL)
        {
            ok = ok && _emit_shader(stream, fs_id, "FRAGMENT", fs_wgsl, fs_glsl, cfg);
        }
        else if (fs_glsl != NULL && fs_spirv_key != NULL &&
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
    else if (is_primitive)
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
        else if (is_primitive)
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
            /* binding0=position(vec3), binding1=texcoords(vec2); set0=MVP, set1=image */
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
            if (ok && uses_mvp && mvp_bgl_id != 0)
                ok = dvz_drp2_stream_pipeline_set_bind_group_layout(stream, mvp_bgl_id);
            if (ok && bgl_id != 0)
                ok = dvz_drp2_stream_pipeline_set_bind_group_layout2(stream, bgl_id);
        }
        else
        {
            ok = ok && dvz_drp2_stream_create_render_pipeline(
                           stream, pipe_id, vs_id, fs_id, vertex_buffer_count);
        }
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
    if (ok && uses_mvp && mvp_bg_id != 0)
        ok = dvz_drp2_stream_set_bind_group(stream, render_pass_id, 0, mvp_bg_id);
    if (ok && is_image && bg_id != 0)
        ok = dvz_drp2_stream_set_bind_group(stream, render_pass_id, 1, bg_id);
    for (uint32_t i = 0; ok && i < vertex_buffer_count; i++)
        ok = dvz_drp2_stream_set_vertex_buffer(stream, render_pass_id, i, vertex_buffer_ids[i], 0);
    ok = ok && dvz_drp2_stream_draw(stream, render_pass_id, vertex_count, 1, 0, 0) &&
         dvz_drp2_stream_end_render_pass(stream, render_pass_id);
    ok = ok && _render_pass_copy_finish_submit(
                   stream, encoder_id, command_buffer_id, submission_id, color_id, rb_id,
                   readback);
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
    const DvzFramePlanNode* readback, const DvzFramePlanEmitConfig* cfg,
    DvzDiagnosticReport* report)
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
            if (_scene_render_visual_has_position_resource(emitter, render, 0))
            {
                scene_render_node_count++;
                any_scene_render_needs_depth =
                    any_scene_render_needs_depth || _scene_render_needs_depth(emitter, render);
            }
        }
    }
    if (render_node_count > 0 && render_node_count == scene_render_node_count &&
        !any_scene_render_needs_depth)
        return _emitter_emit_scene_figure_renders(emitter, stream, plan, readback, cfg, report);

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
            is_scene_node = _scene_render_visual_has_position_resource(emitter, render, 0);
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
                is_scene_node ? &scene_cache : NULL, report);
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

    uint64_t color_id = 0;
    if (!_render_pass_resolve_color_target(emitter, stream, cfg, &color_id))
        return false;

    uint64_t rb_id = 0;
    if (!_render_pass_resolve_readback_buffer(emitter, stream, readback, &rb_id))
        return false;

    bool ok = true;

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
             stream, render_pass_id, encoder_id, color_id, cr, cg, cb, ca,
             clear_node->u.clear.desc.x, clear_node->u.clear.desc.y, clear_node->u.clear.desc.width,
             clear_node->u.clear.desc.height, clear) &&
         dvz_drp2_stream_end_render_pass(stream, render_pass_id);
    ok = ok && _render_pass_copy_finish_submit(
                   stream, encoder_id, command_buffer_id, submission_id, color_id, rb_id,
                   readback);
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
    ok = ok && _render_pass_copy_finish_submit(
                   stream, encoder_id, command_buffer_id, submission_id, color_id, rb_id,
                   readback);
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
    ok = ok && _render_pass_resolve_color_target(emitter, stream, cfg, &color_id);

    uint64_t rb_id = 0;
    ok = ok && _render_pass_resolve_readback_buffer(emitter, stream, readback, &rb_id);

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
    ok = ok && _render_pass_copy_finish_submit(
                   stream, encoder_id, command_buffer_id, submission_id, color_id, rb_id,
                   readback);
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
                          fallback_vertex_buffer_count, copy, cfg, report));
    if (!ok)
    {
        _diagnostic(report, "failed to emit runtime DRP2 stream");
        dvz_drp2_stream_destroy(stream);
        return NULL;
    }
    return stream;
}
