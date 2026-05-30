/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene FramePlan runtime render pass emission                                                 */
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
 * Return G-buffer targets associated with a panel id.
 *
 * @param targets target array
 * @param renders render-node array parallel to targets
 * @param count target count
 * @param panel_id panel id to find
 * @return target entry, or NULL when absent
 */
const SceneGBufferTargets* _gbuffer_targets_for_panel(
    const SceneGBufferTargets* targets, const DvzFramePlanNode* const* renders, uint32_t count,
    const char* panel_id)
{
    ANN(targets);
    ANN(renders);
    ANN(panel_id);

    for (uint32_t i = 0; i < count; i++)
    {
        if (renders[i] != NULL && strcmp(renders[i]->u.render.panel_id, panel_id) == 0)
            return &targets[i];
    }
    return NULL;
}



/**
 * Return EDL targets associated with a panel id.
 *
 * @param targets target array
 * @param renders render-node array parallel to targets
 * @param count target count
 * @param panel_id panel id to find
 * @return target entry, or NULL when absent
 */
const SceneEdlTargets* _edl_targets_for_panel(
    const SceneEdlTargets* targets, const DvzFramePlanNode* const* renders, uint32_t count,
    const char* panel_id)
{
    ANN(targets);
    ANN(renders);
    ANN(panel_id);

    for (uint32_t i = 0; i < count; i++)
    {
        if (renders[i] != NULL && strcmp(renders[i]->u.render.panel_id, panel_id) == 0)
            return &targets[i];
    }
    return NULL;
}



/**
 * Return SSAO targets associated with a panel id.
 *
 * @param targets target array
 * @param renders render-node array parallel to targets
 * @param count target count
 * @param panel_id panel id to find
 * @return target entry, or NULL when absent
 */
const SceneSsaoTargets* _ssao_targets_for_panel(
    const SceneSsaoTargets* targets, const DvzFramePlanNode* const* renders, uint32_t count,
    const char* panel_id)
{
    ANN(targets);
    ANN(renders);
    ANN(panel_id);

    for (uint32_t i = 0; i < count; i++)
    {
        if (renders[i] != NULL && strcmp(renders[i]->u.render.panel_id, panel_id) == 0)
            return &targets[i];
    }
    return NULL;
}



/**
 * Return WBOIT targets associated with a panel id.
 *
 * @param targets target array.
 * @param renders render-node array parallel to targets.
 * @param count target count.
 * @param panel_id panel id to find.
 * @return target entry, or NULL when absent.
 */
const SceneWboitTargets* _wboit_targets_for_panel(
    const SceneWboitTargets* targets, const DvzFramePlanNode* const* renders, uint32_t count,
    const char* panel_id)
{
    ANN(targets);
    ANN(renders);
    ANN(panel_id);

    for (uint32_t i = 0; i < count; i++)
    {
        if (renders[i] != NULL && strcmp(renders[i]->u.render.panel_id, panel_id) == 0)
            return &targets[i];
    }
    return NULL;
}


/**
 * Return depth-peeling targets associated with a panel id.
 *
 * @param targets target array.
 * @param renders render-node array parallel to targets.
 * @param count target count.
 * @param panel_id panel id to find.
 * @return target entry, or NULL when absent.
 */
const SceneDepthPeelTargets* _depth_peel_targets_for_panel(
    const SceneDepthPeelTargets* targets, const DvzFramePlanNode* const* renders, uint32_t count,
    const char* panel_id)
{
    ANN(targets);
    ANN(renders);
    ANN(panel_id);

    for (uint32_t i = 0; i < count; i++)
    {
        if (renders[i] != NULL && strcmp(renders[i]->u.render.panel_id, panel_id) == 0)
            return &targets[i];
    }
    return NULL;
}



/**
 * Return the prepared draw batch for a render node.
 *
 * @param batches prepared render batches.
 * @param count number of prepared render batches.
 * @param render render node.
 * @return the matching batch, or NULL when no draws were prepared.
 */
const SceneRenderBatch* _render_batch_for_node(
    const SceneRenderBatch* batches, uint32_t count, const DvzFramePlanNode* render)
{
    ANN(render);
    for (uint32_t i = 0; i < count; i++)
    {
        if (batches[i].render == render)
            return &batches[i];
    }
    return NULL;
}



/**
 * Emit one graph-unordered plain render node with the requested role into the final color target.
 *
 * @param emitter persistent frame-plan emitter.
 * @param stream destination DRP2 command stream.
 * @param render render node not referenced by a frame-graph pass.
 * @param cfg frame-plan emission configuration.
 * @param encoder_id active command encoder id.
 * @param color_id final color target id.
 * @param clear_color final target clear color.
 * @param batch prepared draw batch for the render node.
 * @param pass_role render-pass role to emit.
 * @param clear_final whether the final target still needs its first clear.
 * @return whether the render commands were emitted.
 */
static bool _emitter_emit_graph_unordered_plain_render(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlanNode* render,
    const DvzFramePlanEmitConfig* cfg, uint64_t encoder_id, uint64_t color_id,
    const float clear_color[4], const SceneRenderBatch* batch,
    DvzFramePlanRenderPassRole pass_role, bool* clear_final)
{
    ANN(emitter);
    ANN(stream);
    ANN(render);
    ANN(batch);
    ANN(clear_final);

    if (pass_role != DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE &&
        pass_role != DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND)
    {
        return true;
    }
    if (render->u.render.pass_role != pass_role)
    {
        return true;
    }

    bool clear = *clear_final;
    uint64_t pass_id = _emitter_next_transient_id(emitter);
    _label_render_pass_contract(stream, pass_id, render);
    bool ok = true;

    if (render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE)
    {
        ok = dvz_drp2_stream_begin_render_pass_region_clear(
            stream, pass_id, encoder_id, color_id, clear_color[0], clear_color[1], clear_color[2],
            clear_color[3], 0.0f, 0.0f, 1.0f, 1.0f, clear);
    }
    else
    {
        clear = false;
        ok = dvz_drp2_stream_begin_render_pass_region_clear(
            stream, pass_id, encoder_id, color_id, clear_color[0], clear_color[1], clear_color[2],
            clear_color[3], render->u.render.desc.x, render->u.render.desc.y,
            render->u.render.desc.width, render->u.render.desc.height, false);
    }

    bool needs_depth = _scene_render_needs_depth(emitter, render);
    if (ok && needs_depth)
    {
        ok = dvz_drp2_stream_begin_render_pass_set_depth(stream, 1.0f);
        if (ok && !clear)
            ok = dvz_drp2_stream_begin_render_pass_set_depth_ops(
                stream, DVZ_DRP2_ATTACHMENT_LOAD_CLEAR, DVZ_DRP2_ATTACHMENT_STORE_STORE);
    }

    SceneRenderStateCache scene_cache = {0};
    ok = ok &&
         _emitter_emit_render_multi_draws(
             stream, render, cfg, pass_id, batch->draws, batch->draw_count, &scene_cache) &&
         dvz_drp2_stream_end_render_pass(stream, pass_id);

    if (ok)
        *clear_final = false;
    return ok;
}



/**
 * Emit graph-unordered plain render nodes with one pass role into the final color target.
 *
 * @param emitter persistent frame-plan emitter.
 * @param stream destination DRP2 command stream.
 * @param plan frame plan containing plain render nodes.
 * @param cfg frame-plan emission configuration.
 * @param encoder_id active command encoder id.
 * @param color_id final color target id.
 * @param clear_color final target clear color.
 * @param batches prepared draw batches.
 * @param batch_count number of prepared draw batches.
 * @param pass_role render-pass role to emit.
 * @param clear_final whether the final target still needs its first clear.
 * @return whether the render commands were emitted.
 */
static bool _emitter_emit_graph_unordered_plain_renders_for_role(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanEmitConfig* cfg, uint64_t encoder_id, uint64_t color_id,
    const float clear_color[4], const SceneRenderBatch* batches, uint32_t batch_count,
    DvzFramePlanRenderPassRole pass_role, bool* clear_final)
{
    ANN(emitter);
    ANN(stream);
    ANN(plan);
    ANN(batches);
    ANN(clear_final);

    bool ok = true;
    for (uint32_t i = 0; ok && i < plan->count; i++)
    {
        const DvzFramePlanNode* render = &plan->nodes[i];
        if (render->type != DVZ_FRAME_PLAN_NODE_RENDER || render->u.render.visual_count == 0)
            continue;
        if (_graph_pass_for_render(plan, render) != NULL)
            continue;

        const SceneRenderBatch* batch = _render_batch_for_node(batches, batch_count, render);
        if (batch == NULL)
            continue;
        ok = _emitter_emit_graph_unordered_plain_render(
            emitter, stream, render, cfg, encoder_id, color_id, clear_color, batch, pass_role,
            clear_final);
    }
    return ok;
}



/* Scene render path: one BeginRenderPass per panel, one Draw per visual inside it. */
bool _emitter_emit_render_multi(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanNode* render, const DvzFramePlanNode* readback, bool clear,
    const DvzFramePlanEmitConfig* cfg, SceneRenderStateCache* cache, DvzDiagnosticReport* report)
{
    ANN(emitter);
    ANN(stream);
    ANN(plan);
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
    const DvzFrameGraphPass* graph_pass = NULL;
    uint64_t graph_depth_id = 0;
    if (!_graph_resolve_render_depth(
            emitter, stream, plan, render, cfg, &graph_pass, &graph_depth_id))
        return false;
    uint64_t sampled_depth_id =
        graph_pass != NULL && graph_pass->has_depth_attachment &&
                graph_pass->depth_attachment.access == DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ
            ? graph_depth_id
            : 0;
    bool pass_has_depth_attachment =
        graph_pass != NULL && graph_pass->has_depth_attachment && graph_depth_id != 0;
    if (!pass_has_depth_attachment && needs_depth && graph_depth_id == 0)
        pass_has_depth_attachment = true;

    SceneDrawPacket draws[DVZ_SCENE_MAX_RENDER_VISUALS] = {0};
    uint32_t draw_count = 0;
    uint32_t pass_sample_count = _graph_render_pass_sample_count(emitter, plan, graph_pass);
    ok = _emitter_prepare_render_multi(
        emitter, stream, render, cfg, pass_has_depth_attachment, false, sampled_depth_id, false, 0,
        0, 0, 0, pass_sample_count, graph_pass != NULL && graph_pass->alpha_to_coverage, report,
        draws, &draw_count);
    if (!ok)
        return false;

    ok = dvz_drp2_stream_begin_command_encoder(stream, encoder_id) &&
         dvz_drp2_stream_begin_render_pass_region_clear(
             stream, render_pass_id, encoder_id, color_id, cr, cg, cb, ca, 0.0f, 0.0f, 1.0f, 1.0f,
             clear);
    ok = ok && _stream_apply_graph_color_ops(stream, graph_pass, color_id, NULL);
    ok = ok && _stream_apply_graph_depth(stream, graph_pass, graph_depth_id);
    _label_render_pass_contract(stream, render_pass_id, render);
    if (ok && needs_depth && graph_depth_id == 0)
    {
        ok = dvz_drp2_stream_begin_render_pass_set_depth(stream, 1.0f);
        if (ok && !clear)
            ok = dvz_drp2_stream_begin_render_pass_set_depth_ops(
                stream, DVZ_DRP2_ATTACHMENT_LOAD_CLEAR, DVZ_DRP2_ATTACHMENT_STORE_STORE);
    }
    ok = ok &&
         _emitter_emit_render_multi_draws(
             stream, render, cfg, render_pass_id, draws, draw_count, cache) &&
         dvz_drp2_stream_end_render_pass(stream, render_pass_id);
    ok =
        ok && _render_pass_copy_finish_submit(
                  stream, encoder_id, command_buffer_id, submission_id, color_id, rb_id, readback);
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
 * @param needs_depth whether the figure pass needs a transient depth attachment
 * @return whether the commands were emitted
 */
bool _emitter_emit_scene_figure_renders(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanNode* readback, const DvzFramePlanEmitConfig* cfg, bool needs_depth,
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
            emitter, stream, render, cfg, needs_depth, false, 0, false, 0, 0, 0, 0, 1, false,
            report, batch->draws, &batch->draw_count);
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
             stream, render_pass_id, encoder_id, color_id, cr, cg, cb, ca, 0.0f, 0.0f, 1.0f, 1.0f,
             true);
    if (ok && needs_depth)
        ok = dvz_drp2_stream_begin_render_pass_set_depth(stream, 1.0f);

    SceneRenderStateCache scene_cache = {0};
    for (uint32_t i = 0; ok && i < batch_count; i++)
    {
        ok = _emitter_emit_render_multi_draws(
            stream, batches[i].render, cfg, render_pass_id, batches[i].draws,
            batches[i].draw_count, &scene_cache);
    }

    ok = ok && dvz_drp2_stream_end_render_pass(stream, render_pass_id);
    ok =
        ok && _render_pass_copy_finish_submit(
                  stream, encoder_id, command_buffer_id, submission_id, color_id, rb_id, readback);
    dvz_free(batches);
    return ok;
}


/**
 * Return whether the plan contains graph-backed render passes.
 *
 * @param plan the FramePlan.
 * @return whether graph-backed scene render nodes are present.
 */
bool _plan_has_graph_render_passes(const DvzFramePlan* plan)
{
    ANN(plan);
    if (dvz_frame_plan_graph_pass_count(plan) > 0)
        return true;
    for (uint32_t i = 0; i < plan->count; i++)
    {
        const DvzFramePlanNode* node = &plan->nodes[i];
        if (node->type != DVZ_FRAME_PLAN_NODE_RENDER)
            continue;
        if (node->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_GBUFFER ||
            node->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_VOLUME_OCCLUSION ||
            node->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_SCENE_OCCLUSION ||
            node->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_SSAO ||
            node->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_SSAO_BLUR ||
            node->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_SSAO_COMPOSITE ||
            node->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_EDL_RESOLVE ||
            node->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION ||
            node->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND ||
            node->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_WBOIT_RESOLVE ||
            node->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT ||
            node->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER ||
            node->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_COMPOSITE)
            return true;
    }
    return false;
}



/**
 * Emit scene render nodes with graph-backed technique passes.
 *
 * @param emitter the persistent emitter.
 * @param stream destination DRP2 command stream.
 * @param plan the FramePlan.
 * @param readback optional readback copy node.
 * @param cfg frame-plan emit configuration.
 * @param report diagnostic report receiving recoverable emission errors.
 * @return whether the commands were emitted.
 */
bool _emitter_emit_scene_graph_renders(
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

    SceneRenderBatch* batches =
        (SceneRenderBatch*)dvz_calloc(plan->count, sizeof(SceneRenderBatch));
    SceneGBufferTargets* gbuffer_targets =
        (SceneGBufferTargets*)dvz_calloc(plan->count, sizeof(SceneGBufferTargets));
    const DvzFramePlanNode** gbuffer_renders =
        (const DvzFramePlanNode**)dvz_calloc(plan->count, sizeof(DvzFramePlanNode*));
    SceneEdlTargets* edl_targets =
        (SceneEdlTargets*)dvz_calloc(plan->count, sizeof(SceneEdlTargets));
    const DvzFramePlanNode** edl_renders =
        (const DvzFramePlanNode**)dvz_calloc(plan->count, sizeof(DvzFramePlanNode*));
    SceneSsaoTargets* ssao_targets =
        (SceneSsaoTargets*)dvz_calloc(plan->count, sizeof(SceneSsaoTargets));
    const DvzFramePlanNode** ssao_renders =
        (const DvzFramePlanNode**)dvz_calloc(plan->count, sizeof(DvzFramePlanNode*));
    SceneWboitTargets* wboit_targets =
        (SceneWboitTargets*)dvz_calloc(plan->count, sizeof(SceneWboitTargets));
    const DvzFramePlanNode** wboit_renders =
        (const DvzFramePlanNode**)dvz_calloc(plan->count, sizeof(DvzFramePlanNode*));
    SceneDepthPeelTargets* depth_peel_targets =
        (SceneDepthPeelTargets*)dvz_calloc(plan->count, sizeof(SceneDepthPeelTargets));
    const DvzFramePlanNode** depth_peel_renders =
        (const DvzFramePlanNode**)dvz_calloc(plan->count, sizeof(DvzFramePlanNode*));
    if (batches == NULL || gbuffer_targets == NULL || gbuffer_renders == NULL ||
        edl_targets == NULL || edl_renders == NULL || wboit_targets == NULL ||
        wboit_renders == NULL || ssao_targets == NULL || ssao_renders == NULL ||
        depth_peel_targets == NULL || depth_peel_renders == NULL)
    {
        dvz_free(depth_peel_renders);
        dvz_free(depth_peel_targets);
        dvz_free(ssao_renders);
        dvz_free(ssao_targets);
        dvz_free(wboit_renders);
        dvz_free(wboit_targets);
        dvz_free(edl_renders);
        dvz_free(edl_targets);
        dvz_free(gbuffer_renders);
        dvz_free(gbuffer_targets);
        dvz_free(batches);
        return false;
    }

    bool ok = true;
    uint32_t batch_count = 0;
    uint32_t gbuffer_target_count = 0;
    uint32_t edl_target_count = 0;
    uint32_t ssao_target_count = 0;
    uint32_t target_count = 0;
    uint32_t depth_target_count = 0;
    for (uint32_t i = 0; ok && i < plan->count; i++)
    {
        const DvzFramePlanNode* render = &plan->nodes[i];
        if (render->type != DVZ_FRAME_PLAN_NODE_RENDER)
            continue;
        if (render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_SSAO ||
            render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_SSAO_BLUR ||
            render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_SSAO_COMPOSITE ||
            render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_EDL_RESOLVE ||
            render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_WBOIT_RESOLVE ||
            render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_COMPOSITE)
        {
            if (render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_SSAO)
            {
                ok = _emitter_prepare_ssao_targets(
                    emitter, stream, plan, render, cfg, &ssao_targets[ssao_target_count]);
                if (ok)
                    ssao_renders[ssao_target_count++] = render;
            }
            else if (render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_EDL_RESOLVE)
            {
                ok = _emitter_prepare_edl_targets(
                    emitter, stream, plan, render, cfg, &edl_targets[edl_target_count]);
                if (ok)
                    edl_renders[edl_target_count++] = render;
            }
            continue;
        }

        const DvzFrameGraphPass* render_graph_pass = NULL;
        uint64_t render_graph_depth_id = 0;
        ok = _graph_resolve_render_depth(
            emitter, stream, plan, render, cfg, &render_graph_pass, &render_graph_depth_id);
        if (!ok)
            break;
        bool pass_has_depth_attachment = render_graph_pass != NULL &&
                                         render_graph_pass->has_depth_attachment &&
                                         render_graph_depth_id != 0;
        bool depth_peel_render =
            render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT ||
            render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER;
        bool transient_depth_allowed =
            render->u.render.pass_role != DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION;
        if (!depth_peel_render && transient_depth_allowed && !pass_has_depth_attachment &&
            _scene_render_needs_depth(emitter, render))
            pass_has_depth_attachment = true;
        uint64_t sampled_depth_id = 0;
        if (render_graph_pass != NULL && render_graph_pass->has_depth_attachment &&
            render_graph_pass->depth_attachment.access == DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ)
            sampled_depth_id = render_graph_depth_id;
        uint64_t depth_peel_sampled_bgl_id = 0;
        uint64_t depth_peel_sampled_bg_id = 0;
        uint64_t depth_peel_dummy_bg_id = 0;
        if (render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_GBUFFER)
        {
            ok = _emitter_prepare_gbuffer_targets(
                emitter, stream, plan, render, cfg, &gbuffer_targets[gbuffer_target_count]);
            if (ok)
                gbuffer_renders[gbuffer_target_count++] = render;
        }
        else if (render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION)
        {
            ok = _emitter_prepare_wboit_targets(
                emitter, stream, plan, render, color_id, cfg, &wboit_targets[target_count]);
            if (ok)
            {
                sampled_depth_id = wboit_targets[target_count].depth_id;
                wboit_renders[target_count++] = render;
            }
        }
        else if (render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT)
        {
            ok = _emitter_prepare_depth_peel_targets(
                emitter, stream, plan, render, color_id, cfg,
                &depth_peel_targets[depth_target_count]);
            if (ok)
            {
                if (render_graph_pass != NULL && render_graph_pass->has_depth_attachment &&
                    render_graph_pass->depth_attachment.access ==
                        DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ)
                    sampled_depth_id = depth_peel_targets[depth_target_count].depth_id;
                depth_peel_renders[depth_target_count++] = render;
            }
        }
        else if (render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER)
        {
            const SceneDepthPeelTargets* targets = _depth_peel_targets_for_panel(
                depth_peel_targets, depth_peel_renders, depth_target_count,
                render->u.render.panel_id);
            if (targets != NULL && render_graph_pass != NULL &&
                render_graph_pass->has_depth_attachment &&
                render_graph_pass->depth_attachment.access ==
                    DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ)
                sampled_depth_id = targets->depth_id;
            if (targets != NULL && render_graph_pass != NULL)
            {
                depth_peel_sampled_bgl_id = targets->iter_bgl_id;
                depth_peel_dummy_bg_id = targets->dummy_bg_id;
                for (uint32_t iter_idx = 0; iter_idx < DVZ_SCENE_DEPTH_PEEL_ITERATIONS; iter_idx++)
                {
                    char iter_pass_id[DVZ_SCENE_LABEL_SIZE];
                    dvz_snprintf(
                        iter_pass_id, sizeof(iter_pass_id), "%s.peel.iter.%" PRIu32,
                        render->u.render.panel_id, iter_idx);
                    if (strcmp(render_graph_pass->id, iter_pass_id) == 0)
                    {
                        depth_peel_sampled_bg_id = targets->iter_bg_ids[iter_idx];
                        break;
                    }
                }
            }
        }
        bool sampled_depth_is_volume_occlusion = false;
        uint64_t volume_occlusion_depth_id = 0;
        ok = _graph_resolve_volume_occlusion_read(
            emitter, stream, plan, cfg, render_graph_pass, &volume_occlusion_depth_id);
        if (!ok)
            break;
        if (volume_occlusion_depth_id != 0)
        {
            sampled_depth_id = volume_occlusion_depth_id;
            sampled_depth_is_volume_occlusion = true;
        }
        uint64_t scene_occlusion_depth_id = 0;
        ok = _graph_resolve_scene_occlusion_read(
            emitter, stream, plan, cfg, render_graph_pass, &scene_occlusion_depth_id);
        if (!ok)
            break;

        if (render->u.render.visual_count > 0)
        {
            SceneRenderBatch* batch = &batches[batch_count];
            batch->render = render;
            bool force_point_depth =
                render_graph_pass != NULL && render_graph_pass->has_depth_attachment &&
                _scene_resource_id_has_suffix(
                    render_graph_pass->depth_attachment.resource_id, ".edl.depth");
            ok = _emitter_prepare_render_multi(
                emitter, stream, render, cfg, pass_has_depth_attachment, force_point_depth,
                sampled_depth_id, sampled_depth_is_volume_occlusion, scene_occlusion_depth_id,
                depth_peel_sampled_bgl_id, depth_peel_sampled_bg_id, depth_peel_dummy_bg_id,
                _graph_render_pass_sample_count(emitter, plan, render_graph_pass),
                render_graph_pass != NULL && render_graph_pass->alpha_to_coverage, report,
                batch->draws, &batch->draw_count);
            if (ok)
                batch_count++;
            else
            {
                char message[96];
                dvz_snprintf(
                    message, sizeof(message), "failed to prepare scene render batch for role %d",
                    (int)render->u.render.pass_role);
                _diagnostic(report, message);
            }
        }
    }
    if (!ok)
    {
        dvz_free(depth_peel_renders);
        dvz_free(depth_peel_targets);
        dvz_free(ssao_renders);
        dvz_free(ssao_targets);
        dvz_free(wboit_renders);
        dvz_free(wboit_targets);
        dvz_free(edl_renders);
        dvz_free(edl_targets);
        dvz_free(gbuffer_renders);
        dvz_free(gbuffer_targets);
        dvz_free(batches);
        return false;
    }

    uint64_t encoder_id = _emitter_next_transient_id(emitter);

    float cr = cfg ? cfg->clear_color[0] : 0.0f;
    float cg = cfg ? cfg->clear_color[1] : 0.0f;
    float cb = cfg ? cfg->clear_color[2] : 0.0f;
    float ca = cfg ? cfg->clear_color[3] : 1.0f;
    bool clear_final = true;
    SceneRenderStateCache scene_cache = {0};

    ok = dvz_drp2_stream_begin_command_encoder(stream, encoder_id);
    bool use_graph_order = dvz_frame_plan_graph_pass_count(plan) > 0;
    uint32_t order_count = use_graph_order ? dvz_frame_plan_graph_pass_count(plan) : plan->count;
    float clear_color[4] = {cr, cg, cb, ca};
    bool unordered_plain_opaque_emitted = false;
    for (uint32_t i = 0; ok && i < order_count; i++)
    {
        const DvzFrameGraphPass* ordered_graph_pass =
            use_graph_order ? dvz_frame_plan_graph_pass_get(plan, i) : NULL;
        const DvzFramePlanNode* render =
            use_graph_order ? _graph_render_for_pass(plan, ordered_graph_pass) : &plan->nodes[i];
        if (render == NULL || render->type != DVZ_FRAME_PLAN_NODE_RENDER)
            continue;

        if (use_graph_order && !unordered_plain_opaque_emitted &&
            (render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION ||
             render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND ||
             render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_WBOIT_RESOLVE ||
             render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT ||
             render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER ||
             render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_COMPOSITE))
        {
            ok = _emitter_emit_graph_unordered_plain_renders_for_role(
                emitter, stream, plan, cfg, encoder_id, color_id, clear_color, batches,
                batch_count, DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE, &clear_final);
            unordered_plain_opaque_emitted = true;
            if (!ok)
                break;
        }

        if (render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_GBUFFER)
        {
            const SceneGBufferTargets* targets = _gbuffer_targets_for_panel(
                gbuffer_targets, gbuffer_renders, gbuffer_target_count, render->u.render.panel_id);
            if (targets == NULL)
            {
                ok = false;
                break;
            }
            const DvzFrameGraphPass* graph_pass = ordered_graph_pass != NULL
                                                      ? ordered_graph_pass
                                                      : _graph_pass_for_render(plan, render);
            uint64_t target_id = _graph_color_attachment_texture_id(
                graph_pass, 0, color_id, &targets->graph, targets->normal_id);
            uint64_t pass_id = _emitter_next_transient_id(emitter);
            _label_render_pass_contract(stream, pass_id, render);
            const SceneRenderBatch* batch = _render_batch_for_node(batches, batch_count, render);
            bool has_draws = batch != NULL;
            ok = dvz_drp2_stream_begin_render_pass_region_clear(
                stream, pass_id, encoder_id, target_id, 0.5f, 0.5f, 1.0f, 0.0f,
                render->u.render.desc.x, render->u.render.desc.y, render->u.render.desc.width,
                render->u.render.desc.height, true);
            if (ok && graph_pass != NULL && graph_pass->color_attachment_count > 1)
            {
                uint64_t object_id = _graph_color_attachment_texture_id(
                    graph_pass, 1, color_id, &targets->graph, targets->object_id);
                ok = dvz_drp2_stream_begin_render_pass_add_color_attachment(
                    stream, object_id, 0.0f, 0.0f, 0.0f, 0.0f, true);
            }
            ok =
                ok && _stream_apply_graph_color_ops(stream, graph_pass, color_id, &targets->graph);
            ok = ok && _stream_apply_graph_depth(stream, graph_pass, targets->depth_id);
            if (ok && has_draws)
            {
                scene_cache.pipeline_id = 0;
                scene_cache.bg_set0 = 0;
                ok = _emitter_emit_render_multi_draws(
                    stream, render, cfg, pass_id, batch->draws, batch->draw_count, &scene_cache);
            }
            ok = ok && dvz_drp2_stream_end_render_pass(stream, pass_id);
            scene_cache.pipeline_id = 0;
            scene_cache.bg_set0 = 0;
        }
        else if (
            render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_VOLUME_OCCLUSION ||
            render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_SCENE_OCCLUSION)
        {
            const DvzFrameGraphPass* graph_pass = ordered_graph_pass != NULL
                                                      ? ordered_graph_pass
                                                      : _graph_pass_for_render(plan, render);
            SceneGraphRuntimeTargets graph_targets = {0};
            ok = _graph_prepare_render_color_targets(
                emitter, stream, plan, graph_pass, cfg, &graph_targets);
            if (!ok)
                break;
            uint64_t target_id =
                _graph_color_attachment_texture_id(graph_pass, 0, color_id, &graph_targets, 0);
            if (target_id == 0)
            {
                ok = false;
                break;
            }
            uint64_t graph_depth_id = 0;
            if (graph_pass != NULL && graph_pass->has_depth_attachment)
            {
                if (!_graph_resolve_render_depth(
                        emitter, stream, plan, render, cfg, &graph_pass, &graph_depth_id))
                {
                    ok = false;
                    break;
                }
            }
            uint64_t pass_id = _emitter_next_transient_id(emitter);
            _label_render_pass_contract(stream, pass_id, render);
            const SceneRenderBatch* batch = _render_batch_for_node(batches, batch_count, render);
            bool has_draws = batch != NULL;
            bool scene_depth =
                render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_SCENE_OCCLUSION;
            ok = dvz_drp2_stream_begin_render_pass_region_clear(
                stream, pass_id, encoder_id, target_id, scene_depth ? 1.0f : 0.0f,
                scene_depth ? 1.0f : 0.0f, scene_depth ? 1.0f : 0.0f, scene_depth ? 1.0f : 0.0f,
                render->u.render.desc.x, render->u.render.desc.y, render->u.render.desc.width,
                render->u.render.desc.height, true);
            ok = ok && _stream_apply_graph_color_ops(stream, graph_pass, color_id, &graph_targets);
            ok = ok && _stream_apply_graph_depth(stream, graph_pass, graph_depth_id);
            if (ok && has_draws)
            {
                scene_cache.pipeline_id = 0;
                scene_cache.bg_set0 = 0;
                ok = _emitter_emit_render_multi_draws(
                    stream, render, cfg, pass_id, batch->draws, batch->draw_count, &scene_cache);
            }
            ok = ok && dvz_drp2_stream_end_render_pass(stream, pass_id);
            scene_cache.pipeline_id = 0;
            scene_cache.bg_set0 = 0;
        }
        else if (render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE)
        {
            const SceneEdlTargets* edl = _edl_targets_for_panel(
                edl_targets, edl_renders, edl_target_count, render->u.render.panel_id);
            const SceneWboitTargets* targets = _wboit_targets_for_panel(
                wboit_targets, wboit_renders, target_count, render->u.render.panel_id);
            const SceneDepthPeelTargets* depth_targets = _depth_peel_targets_for_panel(
                depth_peel_targets, depth_peel_renders, depth_target_count,
                render->u.render.panel_id);
            const SceneGraphRuntimeTargets* graph_targets = edl != NULL       ? &edl->graph
                                                            : targets != NULL ? &targets->graph
                                                            : depth_targets != NULL
                                                                ? &depth_targets->graph
                                                                : NULL;
            uint64_t graph_depth_id = edl != NULL             ? edl->depth_id
                                      : targets != NULL       ? targets->depth_id
                                      : depth_targets != NULL ? depth_targets->depth_id
                                                              : 0;
            const DvzFrameGraphPass* graph_pass = ordered_graph_pass != NULL
                                                      ? ordered_graph_pass
                                                      : _graph_pass_for_render(plan, render);
            SceneGraphRuntimeTargets local_graph_targets = {0};
            if (ok && graph_targets == NULL)
            {
                ok = _graph_prepare_render_color_targets(
                    emitter, stream, plan, graph_pass, cfg, &local_graph_targets);
                if (!ok)
                    break;
                if (local_graph_targets.count > 0)
                    graph_targets = &local_graph_targets;
            }
            if (ok && graph_depth_id == 0 && graph_pass != NULL &&
                graph_pass->has_depth_attachment)
            {
                if (!_graph_resolve_render_depth(
                        emitter, stream, plan, render, cfg, &graph_pass, &graph_depth_id))
                {
                    ok = false;
                    break;
                }
            }
            uint64_t target_id = _graph_color_attachment_texture_id(
                graph_pass, 0, color_id, graph_targets, color_id);
            uint64_t pass_id = _emitter_next_transient_id(emitter);
            _label_render_pass_contract(stream, pass_id, render);
            const SceneRenderBatch* batch = _render_batch_for_node(batches, batch_count, render);
            bool has_draws = batch != NULL;
            ok = ok && dvz_drp2_stream_begin_render_pass_region_clear(
                           stream, pass_id, encoder_id, target_id, cr, cg, cb, ca, 0.0f, 0.0f,
                           1.0f, 1.0f, clear_final);
            ok = ok && _stream_apply_graph_color_ops(stream, graph_pass, color_id, graph_targets);
            if (ok && graph_depth_id != 0)
                ok = _stream_apply_graph_depth(stream, graph_pass, graph_depth_id);
            if (ok && has_draws && graph_depth_id == 0 &&
                _scene_render_needs_depth(emitter, render))
            {
                ok = dvz_drp2_stream_begin_render_pass_set_depth(stream, 1.0f);
                if (ok && !clear_final)
                    ok = dvz_drp2_stream_begin_render_pass_set_depth_ops(
                        stream, DVZ_DRP2_ATTACHMENT_LOAD_CLEAR,
                        DVZ_DRP2_ATTACHMENT_STORE_STORE);
            }
            if (ok && has_draws)
            {
                scene_cache.pipeline_id = 0;
                scene_cache.bg_set0 = 0;
                ok = _emitter_emit_render_multi_draws(
                    stream, render, cfg, pass_id, batch->draws, batch->draw_count, &scene_cache);
            }
            ok = ok && dvz_drp2_stream_end_render_pass(stream, pass_id);
            scene_cache.pipeline_id = 0;
            scene_cache.bg_set0 = 0;
            clear_final = false;
        }
        else if (render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION)
        {
            const SceneWboitTargets* targets = _wboit_targets_for_panel(
                wboit_targets, wboit_renders, target_count, render->u.render.panel_id);
            if (targets == NULL)
            {
                ok = false;
                break;
            }
            uint64_t pass_id = _emitter_next_transient_id(emitter);
            _label_render_pass_contract(stream, pass_id, render);
            const DvzFrameGraphPass* graph_pass = ordered_graph_pass != NULL
                                                      ? ordered_graph_pass
                                                      : _graph_pass_for_render(plan, render);
            uint64_t accum_id = _graph_color_attachment_texture_id(
                graph_pass, 0, color_id, &targets->graph, targets->accum_id);
            uint64_t weight_id = _graph_color_attachment_texture_id(
                graph_pass, 1, color_id, &targets->graph, targets->weight_id);
            ok = dvz_drp2_stream_begin_render_pass_region_clear(
                     stream, pass_id, encoder_id, accum_id, 0.0f, 0.0f, 0.0f, 0.0f,
                     render->u.render.desc.x, render->u.render.desc.y, render->u.render.desc.width,
                     render->u.render.desc.height, true) &&
                 dvz_drp2_stream_begin_render_pass_add_color_attachment(
                     stream, weight_id, 0.0f, 0.0f, 0.0f, 0.0f, true);
            ok =
                ok && _stream_apply_graph_color_ops(stream, graph_pass, color_id, &targets->graph);
            ok = ok && _stream_apply_graph_depth(stream, graph_pass, targets->depth_id);
            const SceneRenderBatch* batch = _render_batch_for_node(batches, batch_count, render);
            bool has_draws = batch != NULL;
            if (ok && has_draws)
            {
                scene_cache.pipeline_id = 0;
                scene_cache.bg_set0 = 0;
                ok = _emitter_emit_render_multi_draws(
                    stream, render, cfg, pass_id, batch->draws, batch->draw_count, &scene_cache);
            }
            ok = ok && dvz_drp2_stream_end_render_pass(stream, pass_id);
        }
        else if (render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND)
        {
            uint64_t pass_id = _emitter_next_transient_id(emitter);
            _label_render_pass_contract(stream, pass_id, render);
            const DvzFrameGraphPass* graph_pass = ordered_graph_pass != NULL
                                                      ? ordered_graph_pass
                                                      : _graph_pass_for_render(plan, render);
            SceneGraphRuntimeTargets graph_targets = {0};
            ok = _graph_prepare_render_color_targets(
                emitter, stream, plan, graph_pass, cfg, &graph_targets);
            if (!ok)
                break;
            uint64_t graph_depth_id = 0;
            if (graph_pass != NULL && graph_pass->has_depth_attachment)
            {
                const DvzFrameGraphPass* depth_graph_pass = graph_pass;
                ok = _graph_resolve_render_depth(
                    emitter, stream, plan, render, cfg, &depth_graph_pass, &graph_depth_id);
                if (!ok)
                    break;
                graph_pass = depth_graph_pass;
            }
            uint64_t target_id = _graph_color_attachment_texture_id(
                graph_pass, 0, color_id, &graph_targets, color_id);
            const SceneRenderBatch* batch = _render_batch_for_node(batches, batch_count, render);
            bool has_draws = batch != NULL;
            ok = dvz_drp2_stream_begin_render_pass_region_clear(
                stream, pass_id, encoder_id, target_id, cr, cg, cb, ca, render->u.render.desc.x,
                render->u.render.desc.y, render->u.render.desc.width, render->u.render.desc.height,
                false);
            ok = ok && _stream_apply_graph_color_ops(stream, graph_pass, color_id, &graph_targets);
            if (ok && graph_depth_id != 0)
                ok = _stream_apply_graph_depth(stream, graph_pass, graph_depth_id);
            else if (ok && has_draws && _scene_render_needs_depth(emitter, render))
                ok = dvz_drp2_stream_begin_render_pass_set_depth(stream, 1.0f);
            if (ok && has_draws)
            {
                scene_cache.pipeline_id = 0;
                scene_cache.bg_set0 = 0;
                ok = _emitter_emit_render_multi_draws(
                    stream, render, cfg, pass_id, batch->draws, batch->draw_count, &scene_cache);
            }
            ok = ok && dvz_drp2_stream_end_render_pass(stream, pass_id);
            clear_final = false;
        }
        else if (
            render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT ||
            render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER)
        {
            const SceneDepthPeelTargets* targets = _depth_peel_targets_for_panel(
                depth_peel_targets, depth_peel_renders, depth_target_count,
                render->u.render.panel_id);
            if (targets == NULL)
            {
                ok = false;
                break;
            }
            uint64_t pass_id = _emitter_next_transient_id(emitter);
            _label_render_pass_contract(stream, pass_id, render);
            const DvzFrameGraphPass* graph_pass = ordered_graph_pass != NULL
                                                      ? ordered_graph_pass
                                                      : _graph_pass_for_render(plan, render);
            uint64_t first_id = _graph_color_attachment_texture_id(
                graph_pass, 0, color_id, &targets->graph, color_id);
            uint64_t second_id = _graph_color_attachment_texture_id(
                graph_pass, 1, color_id, &targets->graph, color_id);
            uint64_t third_id = _graph_color_attachment_texture_id(
                graph_pass, 2, color_id, &targets->graph, color_id);
            ok = dvz_drp2_stream_begin_render_pass_region_clear(
                     stream, pass_id, encoder_id, first_id, 0.0f, 0.0f, 0.0f, 0.0f,
                     render->u.render.desc.x, render->u.render.desc.y, render->u.render.desc.width,
                     render->u.render.desc.height, true) &&
                 dvz_drp2_stream_begin_render_pass_add_color_attachment(
                     stream, second_id, 0.0f, 0.0f, 0.0f, 0.0f, true) &&
                 dvz_drp2_stream_begin_render_pass_add_color_attachment(
                     stream, third_id, 1.0f, 0.0f, 0.0f, 0.0f, true);
            ok =
                ok && _stream_apply_graph_color_ops(stream, graph_pass, color_id, &targets->graph);
            ok = ok && _stream_apply_graph_depth(stream, graph_pass, targets->depth_id);
            const SceneRenderBatch* batch = _render_batch_for_node(batches, batch_count, render);
            bool has_draws = batch != NULL;
            if (ok && has_draws)
            {
                scene_cache.pipeline_id = 0;
                scene_cache.bg_set0 = 0;
                ok = _emitter_emit_render_multi_draws(
                    stream, render, cfg, pass_id, batch->draws, batch->draw_count, &scene_cache);
            }
            ok = ok && dvz_drp2_stream_end_render_pass(stream, pass_id);
        }
        else if (render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_COMPOSITE)
        {
            const SceneDepthPeelTargets* targets = _depth_peel_targets_for_panel(
                depth_peel_targets, depth_peel_renders, depth_target_count,
                render->u.render.panel_id);
            if (targets == NULL)
            {
                ok = false;
                break;
            }
            uint64_t pass_id = _emitter_next_transient_id(emitter);
            _label_render_pass_contract(stream, pass_id, render);
            const DvzFrameGraphPass* graph_pass = ordered_graph_pass != NULL
                                                      ? ordered_graph_pass
                                                      : _graph_pass_for_render(plan, render);
            uint64_t target_id = _graph_color_attachment_texture_id(
                graph_pass, 0, color_id, &targets->graph, color_id);
            DvzPanelDesc viewport = _render_desc_framebuffer_rect(&render->u.render.desc, cfg);
            ok = dvz_drp2_stream_begin_render_pass_region_clear(
                     stream, pass_id, encoder_id, target_id, 0.0f, 0.0f, 0.0f, 0.0f,
                     render->u.render.desc.x, render->u.render.desc.y, render->u.render.desc.width,
                     render->u.render.desc.height, false) &&
                 _stream_apply_graph_color_ops(stream, graph_pass, color_id, &targets->graph) &&
                 dvz_drp2_stream_set_viewport(
                     stream, pass_id, viewport.x, viewport.y, viewport.width, viewport.height) &&
                 dvz_drp2_stream_set_scissor(
                     stream, pass_id, viewport.x, viewport.y, viewport.width, viewport.height) &&
                 dvz_drp2_stream_set_pipeline(stream, pass_id, targets->composite_pipeline_id) &&
                 dvz_drp2_stream_set_bind_group(stream, pass_id, 0, targets->composite_bg_id) &&
                 dvz_drp2_stream_draw(stream, pass_id, 3, 1, 0, 0) &&
                 dvz_drp2_stream_end_render_pass(stream, pass_id);
            clear_final = false;
        }
        else if (render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_SSAO)
        {
            const SceneSsaoTargets* targets = _ssao_targets_for_panel(
                ssao_targets, ssao_renders, ssao_target_count, render->u.render.panel_id);
            if (targets == NULL)
            {
                ok = false;
                break;
            }
            uint64_t pass_id = _emitter_next_transient_id(emitter);
            _label_render_pass_contract(stream, pass_id, render);
            const DvzFrameGraphPass* graph_pass = ordered_graph_pass != NULL
                                                      ? ordered_graph_pass
                                                      : _graph_pass_for_render(plan, render);
            uint64_t target_id = _graph_color_attachment_texture_id(
                graph_pass, 0, color_id, &targets->graph, targets->occlusion_id);
            DvzPanelDesc viewport = _render_desc_framebuffer_rect(&render->u.render.desc, cfg);
            ok = dvz_drp2_stream_begin_render_pass_region_clear(
                     stream, pass_id, encoder_id, target_id, 1.0f, 1.0f, 1.0f, 1.0f,
                     render->u.render.desc.x, render->u.render.desc.y, render->u.render.desc.width,
                     render->u.render.desc.height, true) &&
                 _stream_apply_graph_color_ops(stream, graph_pass, color_id, &targets->graph) &&
                 dvz_drp2_stream_set_viewport(
                     stream, pass_id, viewport.x, viewport.y, viewport.width, viewport.height) &&
                 dvz_drp2_stream_set_scissor(
                     stream, pass_id, viewport.x, viewport.y, viewport.width, viewport.height) &&
                 dvz_drp2_stream_set_pipeline(stream, pass_id, targets->ssao_pipeline_id) &&
                 dvz_drp2_stream_set_bind_group(stream, pass_id, 0, targets->ssao_bg_id) &&
                 dvz_drp2_stream_draw(stream, pass_id, 3, 1, 0, 0) &&
                 dvz_drp2_stream_end_render_pass(stream, pass_id);
        }
        else if (render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_SSAO_BLUR)
        {
            const SceneSsaoTargets* targets = _ssao_targets_for_panel(
                ssao_targets, ssao_renders, ssao_target_count, render->u.render.panel_id);
            if (targets == NULL || targets->blur_id == 0 || targets->blur_pipeline_id == 0)
            {
                ok = false;
                break;
            }
            uint64_t pass_id = _emitter_next_transient_id(emitter);
            _label_render_pass_contract(stream, pass_id, render);
            const DvzFrameGraphPass* graph_pass = ordered_graph_pass != NULL
                                                      ? ordered_graph_pass
                                                      : _graph_pass_for_render(plan, render);
            uint64_t target_id = _graph_color_attachment_texture_id(
                graph_pass, 0, color_id, &targets->graph, targets->blur_id);
            DvzPanelDesc viewport = _render_desc_framebuffer_rect(&render->u.render.desc, cfg);
            ok = dvz_drp2_stream_begin_render_pass_region_clear(
                     stream, pass_id, encoder_id, target_id, 1.0f, 1.0f, 1.0f, 1.0f,
                     render->u.render.desc.x, render->u.render.desc.y, render->u.render.desc.width,
                     render->u.render.desc.height, true) &&
                 _stream_apply_graph_color_ops(stream, graph_pass, color_id, &targets->graph) &&
                 dvz_drp2_stream_set_viewport(
                     stream, pass_id, viewport.x, viewport.y, viewport.width, viewport.height) &&
                 dvz_drp2_stream_set_scissor(
                     stream, pass_id, viewport.x, viewport.y, viewport.width, viewport.height) &&
                 dvz_drp2_stream_set_pipeline(stream, pass_id, targets->blur_pipeline_id) &&
                 dvz_drp2_stream_set_bind_group(stream, pass_id, 0, targets->blur_bg_id) &&
                 dvz_drp2_stream_draw(stream, pass_id, 3, 1, 0, 0) &&
                 dvz_drp2_stream_end_render_pass(stream, pass_id);
        }
        else if (render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_SSAO_COMPOSITE)
        {
            const SceneSsaoTargets* targets = _ssao_targets_for_panel(
                ssao_targets, ssao_renders, ssao_target_count, render->u.render.panel_id);
            if (targets == NULL)
            {
                ok = false;
                break;
            }
            uint64_t pass_id = _emitter_next_transient_id(emitter);
            _label_render_pass_contract(stream, pass_id, render);
            const DvzFrameGraphPass* graph_pass = ordered_graph_pass != NULL
                                                      ? ordered_graph_pass
                                                      : _graph_pass_for_render(plan, render);
            uint64_t target_id = _graph_color_attachment_texture_id(
                graph_pass, 0, color_id, &targets->graph, color_id);
            DvzPanelDesc viewport = _render_desc_framebuffer_rect(&render->u.render.desc, cfg);
            ok = dvz_drp2_stream_begin_render_pass_region_clear(
                     stream, pass_id, encoder_id, target_id, 0.0f, 0.0f, 0.0f, 0.0f,
                     render->u.render.desc.x, render->u.render.desc.y, render->u.render.desc.width,
                     render->u.render.desc.height, false) &&
                 _stream_apply_graph_color_ops(stream, graph_pass, color_id, &targets->graph) &&
                 dvz_drp2_stream_set_viewport(
                     stream, pass_id, viewport.x, viewport.y, viewport.width, viewport.height) &&
                 dvz_drp2_stream_set_scissor(
                     stream, pass_id, viewport.x, viewport.y, viewport.width, viewport.height) &&
                 dvz_drp2_stream_set_pipeline(stream, pass_id, targets->composite_pipeline_id) &&
                 dvz_drp2_stream_set_bind_group(stream, pass_id, 0, targets->composite_bg_id) &&
                 dvz_drp2_stream_draw(stream, pass_id, 3, 1, 0, 0) &&
                 dvz_drp2_stream_end_render_pass(stream, pass_id);
            clear_final = false;
        }
        else if (render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_EDL_RESOLVE)
        {
            const SceneEdlTargets* targets = _edl_targets_for_panel(
                edl_targets, edl_renders, edl_target_count, render->u.render.panel_id);
            if (targets == NULL)
            {
                ok = false;
                break;
            }
            uint64_t pass_id = _emitter_next_transient_id(emitter);
            _label_render_pass_contract(stream, pass_id, render);
            const DvzFrameGraphPass* graph_pass = ordered_graph_pass != NULL
                                                      ? ordered_graph_pass
                                                      : _graph_pass_for_render(plan, render);
            uint64_t target_id = _graph_color_attachment_texture_id(
                graph_pass, 0, color_id, &targets->graph, color_id);
            DvzPanelDesc viewport = _render_desc_framebuffer_rect(&render->u.render.desc, cfg);
            ok = dvz_drp2_stream_begin_render_pass_region_clear(
                     stream, pass_id, encoder_id, target_id, cr, cg, cb, ca,
                     render->u.render.desc.x, render->u.render.desc.y, render->u.render.desc.width,
                     render->u.render.desc.height, true) &&
                 _stream_apply_graph_color_ops(stream, graph_pass, color_id, &targets->graph) &&
                 dvz_drp2_stream_set_viewport(
                     stream, pass_id, viewport.x, viewport.y, viewport.width, viewport.height) &&
                 dvz_drp2_stream_set_scissor(
                     stream, pass_id, viewport.x, viewport.y, viewport.width, viewport.height) &&
                 dvz_drp2_stream_set_pipeline(stream, pass_id, targets->resolve_pipeline_id) &&
                 dvz_drp2_stream_set_bind_group(stream, pass_id, 0, targets->resolve_bg_id) &&
                 dvz_drp2_stream_draw(stream, pass_id, 3, 1, 0, 0) &&
                 dvz_drp2_stream_end_render_pass(stream, pass_id);
            clear_final = false;
        }
        else if (render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_WBOIT_RESOLVE)
        {
            const SceneWboitTargets* targets = _wboit_targets_for_panel(
                wboit_targets, wboit_renders, target_count, render->u.render.panel_id);
            if (targets == NULL)
            {
                ok = false;
                break;
            }
            uint64_t pass_id = _emitter_next_transient_id(emitter);
            _label_render_pass_contract(stream, pass_id, render);
            const DvzFrameGraphPass* graph_pass = ordered_graph_pass != NULL
                                                      ? ordered_graph_pass
                                                      : _graph_pass_for_render(plan, render);
            uint64_t target_id = _graph_color_attachment_texture_id(
                graph_pass, 0, color_id, &targets->graph, color_id);
            ok = dvz_drp2_stream_begin_render_pass_region_clear(
                     stream, pass_id, encoder_id, target_id, 0.0f, 0.0f, 0.0f, 0.0f,
                     render->u.render.desc.x, render->u.render.desc.y, render->u.render.desc.width,
                     render->u.render.desc.height, false) &&
                 _stream_apply_graph_color_ops(stream, graph_pass, color_id, &targets->graph) &&
                 _emitter_emit_wboit_resolve(stream, render, cfg, pass_id, targets) &&
                 dvz_drp2_stream_end_render_pass(stream, pass_id);
        }
    }

    if (ok && use_graph_order && !unordered_plain_opaque_emitted)
    {
        ok = _emitter_emit_graph_unordered_plain_renders_for_role(
            emitter, stream, plan, cfg, encoder_id, color_id, clear_color, batches, batch_count,
            DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE, &clear_final);
        unordered_plain_opaque_emitted = true;
    }
    if (ok && use_graph_order)
        ok = _emitter_emit_graph_unordered_plain_renders_for_role(
            emitter, stream, plan, cfg, encoder_id, color_id, clear_color, batches, batch_count,
            DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND, &clear_final);

    uint64_t command_buffer_id = _emitter_next_transient_id(emitter);
    uint64_t submission_id = _emitter_next_transient_id(emitter);
    ok =
        ok && _render_pass_copy_finish_submit(
                  stream, encoder_id, command_buffer_id, submission_id, color_id, rb_id, readback);
    dvz_free(depth_peel_renders);
    dvz_free(depth_peel_targets);
    dvz_free(ssao_renders);
    dvz_free(ssao_targets);
    dvz_free(wboit_renders);
    dvz_free(wboit_targets);
    dvz_free(edl_renders);
    dvz_free(edl_targets);
    dvz_free(gbuffer_renders);
    dvz_free(gbuffer_targets);
    dvz_free(batches);
    return ok;
}



/**
 * Return whether two normalized panel rectangles overlap with positive area.
 *
 * @param a first panel rectangle.
 * @param b second panel rectangle.
 * @return whether the two rectangles overlap.
 */
static bool _panel_desc_overlaps(DvzPanelDesc a, DvzPanelDesc b)
{
    if (a.width <= 0.0f || a.height <= 0.0f || b.width <= 0.0f || b.height <= 0.0f)
        return false;

    const float ax1 = a.x + a.width;
    const float ay1 = a.y + a.height;
    const float bx1 = b.x + b.width;
    const float by1 = b.y + b.height;
    return a.x < bx1 && b.x < ax1 && a.y < by1 && b.y < ay1;
}



/**
 * Return whether plain scene render batching would mix overlapping depth-tested panels.
 *
 * @param emitter persistent frame-plan emitter.
 * @param plan FramePlan containing plain render nodes.
 * @param cfg frame-plan emit configuration.
 * @return whether overlapping depth-tested panels require separate render passes.
 */
static bool _plain_scene_depth_panels_overlap(
    DvzFramePlanEmitter* emitter, const DvzFramePlan* plan, const DvzFramePlanEmitConfig* cfg)
{
    ANN(emitter);
    ANN(plan);

    if (cfg == NULL || cfg->shader_format != DVZ_SCENE_SHADER_FORMAT_GLSL)
        return false;

    DvzPanelDesc depth_descs[DVZ_SCENE_MAX_PANELS] = {0};
    uint32_t depth_desc_count = 0;
    for (uint32_t i = 0; i < plan->count; i++)
    {
        const DvzFramePlanNode* render = &plan->nodes[i];
        if (render->type != DVZ_FRAME_PLAN_NODE_RENDER || render->u.render.visual_count == 0)
            continue;
        if (!_scene_render_visual_has_position_resource(emitter, render, 0))
            continue;
        if (!_scene_render_needs_depth(emitter, render))
            continue;

        for (uint32_t j = 0; j < depth_desc_count; j++)
        {
            if (_panel_desc_overlaps(depth_descs[j], render->u.render.desc))
                return true;
        }
        if (depth_desc_count < DVZ_SCENE_MAX_PANELS)
            depth_descs[depth_desc_count++] = render->u.render.desc;
    }
    return false;
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
bool _emitter_emit_plain_renders(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const uint64_t* fallback_vertex_buffer_ids, uint32_t fallback_vertex_buffer_count,
    const DvzFramePlanNode* readback, const DvzFramePlanEmitConfig* cfg,
    DvzDiagnosticReport* report)
{
    ANN(emitter);
    ANN(stream);
    ANN(plan);

    if (_plan_has_graph_render_passes(plan))
        return _emitter_emit_scene_graph_renders(emitter, stream, plan, readback, cfg, report);

    uint32_t render_node_count = 0;
    uint32_t scene_render_node_count = 0;
    bool any_scene_render_needs_depth = false;
    for (uint32_t i = 0; i < plan->count; i++)
    {
        const DvzFramePlanNode* render = &plan->nodes[i];
        if (render->type != DVZ_FRAME_PLAN_NODE_RENDER)
            continue;
        render_node_count++;
        if (render->u.render.visual_count > 0 && cfg != NULL &&
            cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
        {
            if (_scene_render_visual_has_position_resource(emitter, render, 0))
            {
                scene_render_node_count++;
                any_scene_render_needs_depth =
                    any_scene_render_needs_depth || _scene_render_needs_depth(emitter, render);
            }
        }
    }
    bool depth_panels_overlap = _plain_scene_depth_panels_overlap(emitter, plan, cfg);
    if (dvz_frame_plan_graph_pass_count(plan) == 0 && render_node_count > 0 &&
        render_node_count == scene_render_node_count && !depth_panels_overlap)
        return _emitter_emit_scene_figure_renders(
            emitter, stream, plan, readback, cfg, any_scene_render_needs_depth, report);

    bool ok = true;
    uint32_t render_count = 0;
    SceneRenderStateCache scene_cache = {0};
    bool use_graph_order = dvz_frame_plan_graph_pass_count(plan) > 0;
    uint32_t order_count = use_graph_order ? dvz_frame_plan_graph_pass_count(plan) : plan->count;
    for (uint32_t i = 0; ok && i < order_count; i++)
    {
        const DvzFrameGraphPass* graph_pass =
            use_graph_order ? dvz_frame_plan_graph_pass_get(plan, i) : NULL;
        const DvzFramePlanNode* render =
            use_graph_order ? _graph_render_for_pass(plan, graph_pass) : &plan->nodes[i];
        if (render == NULL || render->type != DVZ_FRAME_PLAN_NODE_RENDER)
            continue;

        uint64_t vertex_buffer_ids[DVZ_SCENE_MAX_NODE_RESOURCES] = {0};
        uint32_t vertex_buffer_count = 0;

        /* Retained scene render nodes skip flat resolution; fixture visuals may also carry
         * position metadata, but they do not identify a visual family. */
        bool is_scene_node = false;
        if (render->u.render.visual_count > 0)
        {
            const DvzFramePlanVisualMeta* meta = &render->u.render.visual_metadata[0];
            is_scene_node =
                meta->has_metadata && (meta->visual_type != 0 || meta->desc_kind != 0 ||
                                       meta->has_draw_contract) &&
                _scene_render_visual_has_position_resource(emitter, render, 0);
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
        }

        if (ok)
        {
            scene_cache.pipeline_id = 0;
            scene_cache.bg_set0 = 0;
            ok = _emitter_emit_render_compat(
                emitter, stream, plan, render, vertex_buffer_ids, vertex_buffer_count,
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
bool _emitter_emit_clear_only(
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
             clear_node->u.clear.desc.x, clear_node->u.clear.desc.y,
             clear_node->u.clear.desc.width, clear_node->u.clear.desc.height, clear) &&
         dvz_drp2_stream_end_render_pass(stream, render_pass_id);
    ok =
        ok && _render_pass_copy_finish_submit(
                  stream, encoder_id, command_buffer_id, submission_id, color_id, rb_id, readback);
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
bool _emitter_emit_texture_render(
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
    ok =
        ok && _render_pass_copy_finish_submit(
                  stream, encoder_id, command_buffer_id, submission_id, color_id, rb_id, readback);
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
bool _emitter_emit_compute_assisted_render(
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
        ok = ok && dvz_drp2_stream_create_storage_bind_group_layout(stream, bgl_stor_id);

    char cs_key[16];
    dvz_snprintf(cs_key, sizeof(cs_key), "_cs%s", fmt);
    uint64_t cs_id = _obj_id(emitter, cs_key, &is_new);
    if (cs_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && _emit_shader(
                       stream, cs_id, "COMPUTE", _compute_copy_wgsl(),
                       _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_COMPUTE_COPY, false), cfg);

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
        emitter->resources.first_compute_input_id, emitter->resources.first_compute_output_id);
    uint64_t bg_stor_id = _obj_id(emitter, bg_stor_key, &is_new);
    if (bg_stor_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && dvz_drp2_stream_create_storage_bind_group(
                       stream, bg_stor_id, bgl_stor_id, emitter->resources.first_compute_input_id,
                       emitter->resources.first_compute_output_id,
                       emitter->resources.compute_buffer_size);

    char vs_key[16];
    dvz_snprintf(vs_key, sizeof(vs_key), "_vs%s", fmt);
    uint64_t vs_id = _obj_id(emitter, vs_key, &is_new);
    if (vs_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && _emit_shader(
                       stream, vs_id, "VERTEX", _fixture_vertex_wgsl(),
                       _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_FIXTURE, false), cfg);

    char fs_key[16];
    dvz_snprintf(fs_key, sizeof(fs_key), "_fs%s", fmt);
    uint64_t fs_id = _obj_id(emitter, fs_key, &is_new);
    if (fs_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && _emit_shader(
                       stream, fs_id, "FRAGMENT", _fixture_fragment_wgsl(),
                       _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_FIXTURE, true), cfg);

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
             stream, render_pass_id, encoder_id, color_id, cfg ? cfg->clear_color[0] : 0.0f,
             cfg ? cfg->clear_color[1] : 0.0f, cfg ? cfg->clear_color[2] : 0.0f,
             cfg ? cfg->clear_color[3] : 1.0f) &&
         dvz_drp2_stream_set_pipeline(stream, render_pass_id, pipe_id) &&
         dvz_drp2_stream_set_vertex_buffer(
             stream, render_pass_id, 0, emitter->resources.first_compute_output_id, 0) &&
         dvz_drp2_stream_draw(stream, render_pass_id, 3, 1, 0, 0) &&
         dvz_drp2_stream_end_render_pass(stream, render_pass_id);
    ok =
        ok && _render_pass_copy_finish_submit(
                  stream, encoder_id, command_buffer_id, submission_id, color_id, rb_id, readback);
    return ok;
}
