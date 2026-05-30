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

#include <stdbool.h>
#include <stdint.h>

#include "_assertions.h"
#include "_compat.h"
#include "frame_plan/frame_plan.h"
#include "frame_plan/emit.h"
#include "_frame_plan_runtime_internal.h"
#include "_render_pass.h"
#include "_scene.h"
#include "_scene_shader_abi.h"
#include "_shader_registry.h"
#include "datoviz/drp2.h"
#include "datoviz/drp2/stream.h"
#include "datoviz/scene.h"


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
bool _emitter_emit_render_compat(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanNode* render, const uint64_t* vertex_buffer_ids,
    uint32_t vertex_buffer_count, const DvzFramePlanNode* readback, bool clear,
    const DvzFramePlanEmitConfig* cfg, SceneRenderStateCache* cache, DvzDiagnosticReport* report)
{
    ANN(emitter);
    ANN(stream);
    ANN(plan);
    ANN(render);

    if (cache != NULL && render->u.render.visual_count > 0)
        return _emitter_emit_render_multi(
            emitter, stream, plan, render, readback, clear, cfg, cache, report);

    if (render->u.render.visual_count > 0)
    {
        for (uint32_t i = 0; i < render->u.render.visual_count; i++)
        {
            if (render->u.render.visual_metadata[i].has_metadata)
                continue;
            _diagnostic(report, "render visual missing typed metadata");
            return false;
        }
    }

    /* Generic single-draw path (non-scene nodes, WGSL, or fallback). */
    ANN(vertex_buffer_ids);
    if (vertex_buffer_count == 0)
        return false;

    bool ok = true;
    bool is_new = false;
    const char* fmt = _shader_format_tag(cfg);

    char vs_key[32];
    char fs_key[16];
    char pipe_key[48];

    if (cfg != NULL && cfg->fullscreen_triangle)
    {
        dvz_snprintf(vs_key, sizeof(vs_key), "_vs_full%s", fmt);
        dvz_snprintf(fs_key, sizeof(fs_key), "_fs%s", fmt);
    }
    else
    {
        dvz_snprintf(vs_key, sizeof(vs_key), "_vs%s", fmt);
        dvz_snprintf(fs_key, sizeof(fs_key), "_fs%s", fmt);
    }

    uint64_t vs_id = _obj_id(emitter, vs_key, &is_new);
    if (vs_id == 0)
        return false;
    if (is_new)
    {
        const char* vertex_wgsl = NULL;
        const char* vertex_glsl_src = NULL;
        _render_vertex_shader_source(cfg, &vertex_wgsl, &vertex_glsl_src);
        ok = ok && _emit_shader(stream, vs_id, "VERTEX", vertex_wgsl, vertex_glsl_src, cfg);
    }

    uint64_t fs_id = _obj_id(emitter, fs_key, &is_new);
    if (fs_id == 0)
        return false;
    if (ok && is_new)
    {
        ok = ok && _emit_shader(
                       stream, fs_id, "FRAGMENT", _fixture_fragment_wgsl(),
                       _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_FIXTURE, true), cfg);
    }

    dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe%u%s", vertex_buffer_count, fmt);

    uint64_t pipe_id = _obj_id(emitter, pipe_key, &is_new);
    if (pipe_id == 0)
        return false;
    if (ok && is_new)
    {
        ok = ok && dvz_drp2_stream_create_render_pipeline(
                       stream, pipe_id, vs_id, fs_id, vertex_buffer_count);
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
    for (uint32_t i = 0; ok && i < vertex_buffer_count; i++)
        ok = dvz_drp2_stream_set_vertex_buffer(stream, render_pass_id, i, vertex_buffer_ids[i], 0);
    ok = ok && dvz_drp2_stream_draw(stream, render_pass_id, 3, 1, 0, 0);
    ok = ok && dvz_drp2_stream_end_render_pass(stream, render_pass_id);
    ok =
        ok && _render_pass_copy_finish_submit(
                  stream, encoder_id, command_buffer_id, submission_id, color_id, rb_id, readback);
    return ok;
}
