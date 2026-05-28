/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene FramePlan runtime render emission */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <vulkan/vulkan_core.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_frame_plan.h"
#include "_frame_plan_emit.h"
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
#include "render_contract.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Select the depth-peeling fragment shader variant.
 *
 * @param lit whether the visual carries normals and uses lit shading.
 * @param back_pass whether the pass writes the back-shell accumulation.
 * @return the built-in shader key.
 */
DvzSceneBuiltinShader _depth_peel_fragment_shader(bool lit, bool back_pass)
{
    if (lit)
    {
        return back_pass ? DVZ_SCENE_BUILTIN_SHADER_DEPTH_PEEL_BACK_LIT
                         : DVZ_SCENE_BUILTIN_SHADER_DEPTH_PEEL_FRONT_LIT;
    }
    return back_pass ? DVZ_SCENE_BUILTIN_SHADER_DEPTH_PEEL_BACK
                     : DVZ_SCENE_BUILTIN_SHADER_DEPTH_PEEL_FRONT;
}


/**
 * Return the SPIR-V resource key for one depth-peeling fragment shader.
 *
 * @param shader the built-in shader key.
 * @return the embedded SPIR-V key, or NULL when unsupported.
 */
const char* _depth_peel_fragment_spirv_key(DvzSceneBuiltinShader shader)
{
    switch (shader)
    {
    case DVZ_SCENE_BUILTIN_SHADER_DEPTH_PEEL_FRONT:
        return "depth_peel_front_frag";
    case DVZ_SCENE_BUILTIN_SHADER_DEPTH_PEEL_BACK:
        return "depth_peel_back_frag";
    case DVZ_SCENE_BUILTIN_SHADER_DEPTH_PEEL_FRONT_LIT:
        return "depth_peel_front_lit_frag";
    case DVZ_SCENE_BUILTIN_SHADER_DEPTH_PEEL_BACK_LIT:
        return "depth_peel_back_lit_frag";
    default:
        return NULL;
    }
}



/**
 * Attach scene/runtime labels to ids in an emitted DRP2 stream.
 *
 * @param emitter frame-plan emitter carrying scene/resource id maps
 * @param stream emitted DRP2 command stream
 * @param cfg optional emission configuration with borrowed target id
 */
void _emitter_label_stream_ids(
    const DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream,
    const DvzFramePlanEmitConfig* cfg)
{
    ANN(emitter);
    ANN(stream);

    for (uint32_t i = 0; i < emitter->resources.count; i++)
    {
        const ResourceId* resource = &emitter->resources.resources[i];
        if (resource->id != 0 && resource->key[0] != '\0')
            dvz_drp2_stream_set_label(stream, resource->id, resource->key);
    }

    for (uint32_t i = 0; i < emitter->objects.count; i++)
    {
        const ResourceId* object = &emitter->objects.resources[i];
        if (object->id != 0 && object->key[0] != '\0')
            dvz_drp2_stream_set_label(stream, object->id, object->key);
    }

    if (cfg != NULL && cfg->color_target_id != 0)
        dvz_drp2_stream_set_label(stream, cfg->color_target_id, "rt");
}



/**
 * Attach the FramePlan pass-contract id to an emitted DRP2 render-pass id.
 *
 * @param stream emitted DRP2 command stream
 * @param pass_id the emitted DRP2 render-pass id
 * @param render the source FramePlan render node
 */
void _label_render_pass_contract(
    DvzDrp2CommandStream* stream, uint64_t pass_id, const DvzFramePlanNode* render)
{
    ANN(stream);
    ANN(render);
    if (pass_id != 0 && render->type == DVZ_FRAME_PLAN_NODE_RENDER &&
        render->u.render.has_pass_contract && render->u.render.pass_contract_id[0] != '\0')
    {
        dvz_drp2_stream_set_label(stream, pass_id, render->u.render.pass_contract_id);
    }
}



/**
 * Append one suffix to a runtime object key, reporting truncation as an emission error.
 *
 * @param key key buffer to append to
 * @param size key buffer size
 * @param suffix suffix to append
 * @param report optional diagnostic report
 * @return whether the suffix was appended without truncation
 */
bool _runtime_key_append(char* key, size_t size, const char* suffix, DvzDiagnosticReport* report)
{
    ANN(key);
    ANN(suffix);
    size_t key_len = strlen(key);
    size_t suffix_len = strlen(suffix);
    if (key_len >= size || suffix_len >= size - key_len)
    {
        _diagnostic(report, "runtime pipeline key suffix would be truncated");
        return false;
    }
    int written = dvz_snprintf(key + key_len, size - key_len, "%s", suffix);
    if (written < 0 || (size_t)written != suffix_len)
    {
        _diagnostic(report, "runtime pipeline key suffix append failed");
        return false;
    }
    return true;
}



/**
 * Append a formatted suffix to a runtime object key, reporting truncation as an emission error.
 *
 * @param key key buffer to append to
 * @param size key buffer size
 * @param report optional diagnostic report
 * @param format suffix format string
 * @return whether the suffix was appended without truncation
 */
bool _runtime_key_appendf(
    char* key, size_t size, DvzDiagnosticReport* report, const char* format, ...)
{
    ANN(key);
    ANN(format);
    char suffix[32];
    va_list args;
    va_start(args, format);
    int written = dvz_vsnprintf(suffix, sizeof(suffix), format, args);
    va_end(args);
    if (written < 0 || (size_t)written >= sizeof(suffix))
    {
        _diagnostic(report, "runtime pipeline key formatted suffix would be truncated");
        return false;
    }
    return _runtime_key_append(key, size, suffix, report);
}



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
    _label_render_pass_contract(stream, render_pass_id, render);
    ok = ok && _stream_apply_graph_color_ops(stream, graph_pass, color_id, NULL);
    ok = ok && _stream_apply_graph_depth(stream, graph_pass, graph_depth_id);
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

    /* Detect point-like visual data (position + color + size attributes). */
    bool is_point = _is_point_visual(&emitter->resources, vertex_buffer_ids, vertex_buffer_count);
    bool is_pixel = is_point && desc_kind == DVZ_SCENE_VISUAL_DESC_PIXEL;
    bool is_marker = is_point && desc_kind == DVZ_SCENE_VISUAL_DESC_MARKER;
    bool is_point_like = is_point;
    bool is_splat = !is_point_like && desc_kind == DVZ_SCENE_VISUAL_DESC_SPLAT &&
                    _is_splat_visual(&emitter->resources, vertex_buffer_ids, vertex_buffer_count);
    uint64_t mesh_pos = 0, mesh_color = 0, mesh_normal = 0, mesh_uv = 0, mesh_tex = 0;
    bool is_textured_mesh =
        !is_point_like && !is_splat &&
        _is_textured_mesh_visual(
            &emitter->resources, vertex_buffer_ids, vertex_buffer_count, &mesh_pos, &mesh_color,
            &mesh_normal, &mesh_uv, &mesh_tex);
    bool is_primitive =
        !is_point_like && !is_splat && !is_textured_mesh &&
        _is_primitive_visual(&emitter->resources, vertex_buffer_ids, vertex_buffer_count);
    uint64_t image_pos = 0, image_uv = 0, image_tex = 0;
    bool is_image = !is_point_like && !is_splat && !is_textured_mesh && !is_primitive &&
                    _is_image_visual(
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

    if (cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL &&
        _plan_has_graph_render_passes(plan))
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

        /* Scene render nodes (visual_count > 0 with named resources) skip flat resolution;
         * _emitter_emit_render dispatches to _emitter_emit_render_multi instead. */
        bool is_scene_node = false;
        if (render->u.render.visual_count > 0 && cfg != NULL &&
            cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
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
        }

        if (ok)
        {
            scene_cache.pipeline_id = 0;
            scene_cache.bg_set0 = 0;
            ok = _emitter_emit_render(
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
