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

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "_assertions.h"
#include "frame_plan/frame_plan.h"
#include "frame_plan/emit.h"
#include "_frame_plan_runtime_internal.h"
#include "_frame_plan_runtime_upload.h"
#include "_render_pass.h"
#include "datoviz/drp2.h"
#include "datoviz/drp2/stream.h"
#include "datoviz/scene.h"
#include "render_contract/render_contract.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

#define DVZ_FRAME_PLAN_EMIT_CONFIG_KNOWN_FLAGS 0u



static bool _runtime_emit_config_validate(
    const DvzFramePlanEmitConfig* cfg, DvzDiagnosticReport* report)
{
    if (cfg == NULL)
    {
        _diagnostic(report, "missing DvzFramePlanEmitConfig");
        return false;
    }
    if (!DVZ_STRUCT_VALID(cfg, DvzFramePlanEmitConfig, DVZ_FRAME_PLAN_EMIT_CONFIG_KNOWN_FLAGS))
    {
        _diagnostic(report, "invalid DvzFramePlanEmitConfig ABI prologue");
        return false;
    }
    return true;
}



static bool _runtime_upload_is_texture(const DvzFramePlanNode* node)
{
    ANN(node);
    return node->type == DVZ_FRAME_PLAN_NODE_UPLOAD && node->u.upload.texture_width > 0 &&
           node->u.upload.texture_height > 0;
}



static bool _runtime_plan_uploads_resource(const DvzFramePlan* plan, const char* resource_id)
{
    ANN(plan);
    if (resource_id == NULL || resource_id[0] == '\0')
        return false;
    for (uint32_t i = 0; i < plan->count; i++)
    {
        const DvzFramePlanNode* node = &plan->nodes[i];
        if (node->type != DVZ_FRAME_PLAN_NODE_UPLOAD)
            continue;
        if (strcmp(node->u.upload.resource_id, resource_id) == 0)
            return true;
    }
    return false;
}



static bool _runtime_render_has_typed_scene_visual(
    const DvzFramePlan* plan, const DvzFramePlanNode* render)
{
    ANN(plan);
    ANN(render);
    if (render->type != DVZ_FRAME_PLAN_NODE_RENDER || render->u.render.visual_count == 0)
        return false;

    bool has_uploads = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_UPLOAD) != NULL;
    for (uint32_t i = 0; i < render->u.render.visual_count; i++)
    {
        const DvzFramePlanVisualMeta* meta = &render->u.render.visual_metadata[i];
        if (!meta->has_metadata ||
            (meta->visual_type == 0 && meta->desc_kind == 0 && !meta->has_draw_contract))
        {
            continue;
        }
        if (!has_uploads || _runtime_plan_uploads_resource(plan, meta->position_id) ||
            _runtime_plan_uploads_resource(plan, meta->position_start_id))
        {
            return true;
        }
    }
    return false;
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
    if (!_runtime_emit_config_validate(cfg, report))
        return NULL;

    const DvzFramePlanNode* upload = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_UPLOAD);
    const DvzFramePlanNode* compute = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_COMPUTE);
    const DvzFramePlanNode* render = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_RENDER);
    const DvzFramePlanNode* clear = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_CLEAR);
    const DvzFramePlanNode* copy = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_COPY);
    const DvzFramePlanNode* readback = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_READBACK);
    bool scene_compute = compute != NULL && compute->u.compute.binding_count > 0;
    bool clear_only = compute == NULL && clear != NULL && render == NULL;
    bool retained_render =
        upload == NULL && (compute == NULL || scene_compute) && render != NULL &&
        render->u.render.visual_count > 0;

    if ((!clear_only && !retained_render && upload == NULL) || (!clear_only && render == NULL))
    {
        _diagnostic(report, "runtime converter requires upload+render");
        return NULL;
    }
    bool texture_render = !clear_only && _render_uses_texture(render);
    bool scene_render =
        !clear_only && !render->u.render.picking &&
        _runtime_render_has_typed_scene_visual(plan, render);
    bool legacy_texture_render = texture_render && !scene_render;
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
    emitter->max_color_sample_count =
        caps != NULL && caps->max_color_sample_count != 0 ? caps->max_color_sample_count : 16;
    emitter->max_depth_sample_count =
        caps != NULL && caps->max_depth_sample_count != 0 ? caps->max_depth_sample_count : 16;

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

    if (compute != NULL && !scene_compute)
    {
        ok = _emitter_emit_compute_buffers(emitter, stream, upload, compute);
    }
    for (uint32_t i = 0; ok && (compute == NULL || scene_compute) && i < plan->count; i++)
    {
        if (plan->nodes[i].type == DVZ_FRAME_PLAN_NODE_UPLOAD)
        {
            if (texture_render && _runtime_upload_is_texture(&plan->nodes[i]))
            {
                ok = _emitter_emit_texture_upload(emitter, stream, &plan->nodes[i], &texture_id);
            }
            else
            {
                uint64_t uploaded_id = 0;
                ok = _emitter_emit_upload(emitter, stream, &plan->nodes[i], &uploaded_id);
                if (ok && fallback_vertex_buffer_count < DVZ_SCENE_MAX_NODE_RESOURCES)
                    fallback_vertex_buffer_ids[fallback_vertex_buffer_count++] = uploaded_id;
            }
        }
    }

    if (ok && scene_compute)
        ok = _emitter_emit_compute_passes(emitter, stream, plan, cfg, report);

    ok = ok &&
         (clear_only ? _emitter_emit_clear_only(emitter, stream, clear, copy, true, cfg)
          : compute != NULL && !scene_compute
              ? _emitter_emit_compute_assisted_render(emitter, stream, compute, copy, cfg)
          : legacy_texture_render
              ? _emitter_emit_texture_render(emitter, stream, texture_id, copy, cfg)
              : _emitter_emit_plain_renders(
                    emitter, stream, plan, fallback_vertex_buffer_ids, fallback_vertex_buffer_count,
                    copy, cfg, report));
    if (!ok)
    {
        _diagnostic(report, "failed to emit runtime DRP2 stream");
        dvz_drp2_stream_destroy(stream);
        return NULL;
    }
    _emitter_label_stream_ids(emitter, stream, cfg);
    if (!_scene_frame_plan_drp2_contracts_validate(plan, stream, report))
    {
        _diagnostic(report, "emitted runtime DRP2 stream failed scene contract validation");
        dvz_drp2_stream_destroy(stream);
        return NULL;
    }
    return stream;
}
