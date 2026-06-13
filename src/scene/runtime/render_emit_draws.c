/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene FramePlan runtime render draw emission                                                 */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_assertions.h"
#include "_frame_plan_runtime_internal.h"
#include "_scene_shader_abi.h"
#include "datoviz/drp2/stream.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Convert one normalized render descriptor to framebuffer coordinates.
 *
 * @param desc normalized render descriptor
 * @param cfg emission configuration carrying target extent
 * @return framebuffer-coordinate rectangle
 */
DvzPanelDesc _render_desc_framebuffer_rect(
    const DvzPanelDesc* desc, const DvzFramePlanEmitConfig* cfg)
{
    ANN(desc);
    float width = cfg != NULL && cfg->target_width > 0 ? (float)cfg->target_width : 1.0f;
    float height = cfg != NULL && cfg->target_height > 0 ? (float)cfg->target_height : 1.0f;
    return (DvzPanelDesc){
        .x = desc->x * width,
        .y = desc->y * height,
        .width = desc->width * width,
        .height = desc->height * height,
    };
}



/**
 * Emit one panel's already-prepared draws inside an open render pass.
 *
 * @param stream destination DRP2 command stream.
 * @param render render node whose viewport/scissor and visuals are emitted.
 * @param cfg emission configuration carrying target extent
 * @param render_pass_id active render-pass id.
 * @param draws prepared draw descriptors.
 * @param draw_count number of prepared draw descriptors.
 * @param cache optional state cache shared across panels in the same render pass.
 * @return true when all draw commands were emitted successfully, false otherwise.
 */
bool _emitter_emit_render_multi_draws(
    DvzDrp2CommandStream* stream, const DvzFramePlanNode* render,
    const DvzFramePlanEmitConfig* cfg, uint64_t render_pass_id, const SceneDrawPacket* draws,
    uint32_t draw_count, SceneRenderStateCache* cache)
{
    ANN(stream);
    ANN(render);
    ANN(draws);

    DvzPanelDesc viewport = _render_desc_framebuffer_rect(&render->u.render.desc, cfg);
    bool ok = dvz_drp2_stream_set_viewport(
                  stream, render_pass_id, viewport.x, viewport.y, viewport.width,
                  viewport.height) &&
              dvz_drp2_stream_set_scissor(
                  stream, render_pass_id, viewport.x, viewport.y, viewport.width,
                  viewport.height);

    DvzPanelDesc active_scissor = viewport;
    DvzPanelDesc active_viewport = viewport;
    uint64_t last_pipeline = (cache != NULL) ? cache->pipeline_id : 0;
    uint64_t last_bg_set0 = (cache != NULL) ? cache->bg_set0 : 0;
    uint64_t last_bg_set1 = 0;
    uint64_t last_bg_set2 = 0;
    uint64_t last_bg_set3 = 0;
    for (uint32_t d = 0; ok && d < draw_count; d++)
    {
        DvzPanelDesc draw_scissor = viewport;
        if (draws[d].clip_rect == DVZ_FRAME_PLAN_CLIP_RECT_PLOT && render->u.render.has_plot_desc)
            draw_scissor = _render_desc_framebuffer_rect(&render->u.render.plot_desc, cfg);
        if (draw_scissor.x != active_viewport.x || draw_scissor.y != active_viewport.y ||
            draw_scissor.width != active_viewport.width ||
            draw_scissor.height != active_viewport.height)
        {
            ok = ok && dvz_drp2_stream_set_viewport(
                           stream, render_pass_id, draw_scissor.x, draw_scissor.y,
                           draw_scissor.width, draw_scissor.height);
            active_viewport = draw_scissor;
        }
        if (draw_scissor.x != active_scissor.x || draw_scissor.y != active_scissor.y ||
            draw_scissor.width != active_scissor.width ||
            draw_scissor.height != active_scissor.height)
        {
            ok = ok && dvz_drp2_stream_set_scissor(
                           stream, render_pass_id, draw_scissor.x, draw_scissor.y,
                           draw_scissor.width, draw_scissor.height);
            active_scissor = draw_scissor;
        }
        if (draws[d].pipeline_id != last_pipeline)
        {
            ok = ok && dvz_drp2_stream_set_pipeline(stream, render_pass_id, draws[d].pipeline_id);
            last_pipeline = draws[d].pipeline_id;
            last_bg_set0 = 0;
            last_bg_set1 = 0;
            last_bg_set2 = 0;
            last_bg_set3 = 0;
        }
        if (draws[d].bg_set0 != 0 && draws[d].bg_set0 != last_bg_set0)
        {
            ok = ok && dvz_drp2_stream_set_bind_group(stream, render_pass_id, 0, draws[d].bg_set0);
            last_bg_set0 = draws[d].bg_set0;
        }
        if (draws[d].bg_set1 != 0 && draws[d].bg_set1 != last_bg_set1)
        {
            ok = ok && dvz_drp2_stream_set_bind_group(stream, render_pass_id, 1, draws[d].bg_set1);
            last_bg_set1 = draws[d].bg_set1;
        }
        if (draws[d].bg_set2 != 0 && draws[d].bg_set2 != last_bg_set2)
        {
            ok = ok && dvz_drp2_stream_set_bind_group(stream, render_pass_id, 2, draws[d].bg_set2);
            last_bg_set2 = draws[d].bg_set2;
        }
        if (draws[d].bg_set3 != 0 && draws[d].bg_set3 != last_bg_set3)
        {
            ok =
                ok && dvz_drp2_stream_set_bind_group(
                          stream, render_pass_id, DVZ_SCENE_DEPTH_PEEL_BIND_SET, draws[d].bg_set3);
            last_bg_set3 = draws[d].bg_set3;
        }
        ok = ok && _scene_draw_packet_emit(stream, render_pass_id, &draws[d]);
    }

    if (cache != NULL)
    {
        cache->pipeline_id = last_pipeline;
        cache->bg_set0 = last_bg_set0;
    }

    return ok;
}



/**
 * Emit a WBOIT resolve pass into the final color target.
 *
 * @param stream destination DRP2 command stream.
 * @param render resolve render node.
 * @param cfg emission configuration carrying target extent
 * @param render_pass_id active render-pass id.
 * @param targets WBOIT target ids.
 * @return whether all commands were emitted.
 */
bool _emitter_emit_wboit_resolve(
    DvzDrp2CommandStream* stream, const DvzFramePlanNode* render,
    const DvzFramePlanEmitConfig* cfg, uint64_t render_pass_id, const SceneWboitTargets* targets)
{
    ANN(stream);
    ANN(render);
    ANN(targets);

    DvzPanelDesc viewport = _render_desc_framebuffer_rect(&render->u.render.desc, cfg);
    return dvz_drp2_stream_set_viewport(
               stream, render_pass_id, viewport.x, viewport.y, viewport.width, viewport.height) &&
           dvz_drp2_stream_set_scissor(
               stream, render_pass_id, viewport.x, viewport.y, viewport.width, viewport.height) &&
           dvz_drp2_stream_set_pipeline(stream, render_pass_id, targets->resolve_pipeline_id) &&
           dvz_drp2_stream_set_bind_group(stream, render_pass_id, 0, targets->resolve_bg_id) &&
           dvz_drp2_stream_draw(stream, render_pass_id, 3, 1, 0, 0);
}
