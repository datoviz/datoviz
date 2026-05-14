/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene pick/probe execution                                                                  */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include "datoviz/drp2/runtime.h"
#include "datoviz/math/_cglm.h"
#include "../drp2/_stream.h"
#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_scene.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static bool _scene_execute_readback_plan(
    const DvzScene* scene, DvzDrp2Runtime* runtime, const DvzCapabilitySnapshot* caps,
    DvzFramePlan* plan, DvzFramePlanEmitter* emitter, uint8_t rgba[4]);

static bool _scene_process_point_pick_request(
    DvzFigure* figure, DvzDrp2Runtime* runtime, const DvzCapabilitySnapshot* caps,
    const DvzPendingPickRequest* pending);

static bool _scene_process_image_probe_request(
    DvzFigure* figure, DvzDrp2Runtime* runtime, const DvzCapabilitySnapshot* caps,
    const DvzPendingProbeRequest* pending);



/**
 * Execute queued pick/probe requests for one figure through dedicated DRP2 readback streams.
 *
 * @param figure the figure
 * @param runtime the DRP2 runtime
 * @param caps the capability snapshot, or NULL for defaults
 * @return the number of consumed requests
 */
uint32_t dvz_figure_process_requests(
    DvzFigure* figure, DvzDrp2Runtime* runtime, const DvzCapabilitySnapshot* caps)
{
    ANN(figure);
    ANN(figure->scene);
    ANN(runtime);

    DvzCapabilitySnapshot local_caps = {0};
    if (caps == NULL)
    {
        dvz_capability_snapshot_default(&local_caps);
        local_caps.shader_format_glsl = true;
        caps = &local_caps;
    }

    DvzScene* scene = figure->scene;
    uint32_t processed = 0;
    _scene_coalesce_pending_pick_requests(scene, figure);
    _scene_coalesce_pending_probe_requests(scene, figure);

    if (scene->pending_pick_count == 0 && scene->pending_probe_count == 0)
        return 0;

    DvzDrp2RuntimeConfig runtime_cfg = dvz_drp2_runtime_config(runtime);
    DvzDrp2Runtime* request_runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    if (request_runtime == NULL)
    {
        log_error("scene request runtime creation failed");
        return 0;
    }

    for (uint32_t i = 0; i < scene->pending_pick_count;)
    {
        const DvzPendingPickRequest pending = scene->pending_picks[i];
        if (pending.panel == NULL || pending.panel->figure != figure)
        {
            i++;
            continue;
        }
        (void)_scene_process_point_pick_request(figure, request_runtime, caps, &pending);
        _scene_remove_pending_pick_at(scene, i);
        processed++;
    }

    for (uint32_t i = 0; i < scene->pending_probe_count;)
    {
        const DvzPendingProbeRequest pending = scene->pending_probes[i];
        if (pending.panel == NULL || pending.panel->figure != figure)
        {
            i++;
            continue;
        }
        (void)_scene_process_image_probe_request(figure, request_runtime, caps, &pending);
        _scene_remove_pending_probe_at(scene, i);
        processed++;
    }

    dvz_drp2_runtime_destroy(request_runtime);
    return processed;
}



/**
 * Emit, execute, and download one 4-byte readback request.
 *
 * @param scene the owning scene, used for instance-scoped test controls
 * @param runtime the DRP2 runtime
 * @param caps the capability snapshot
 * @param plan the prepared frame plan
 * @param emitter the frame-plan emitter
 * @param rgba the destination 4-byte readback buffer
 * @return true on successful execution and download
 */
static bool _scene_execute_readback_plan(
    const DvzScene* scene, DvzDrp2Runtime* runtime, const DvzCapabilitySnapshot* caps,
    DvzFramePlan* plan, DvzFramePlanEmitter* emitter, uint8_t rgba[4])
{
    ANN(runtime);
    ANN(caps);
    ANN(rgba);
    if (plan == NULL || emitter == NULL)
    {
        log_error("scene readback requires a prepared frame plan and emitter");
        return false;
    }

    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    DvzDrp2CommandStream* stream =
        dvz_frame_plan_emitter_emit_drp2(emitter, plan, caps, &report, &cfg);
    if (stream == NULL)
    {
        log_error("scene readback DRP2 emission failed");
        return false;
    }

    uint64_t rb_id = dvz_frame_plan_emitter_object_id(emitter, "_rb");
    bool ok = false;
    if (rb_id == 0)
    {
        log_error("scene readback plan did not emit the _rb buffer");
    }
    else
    {
        dvz_drp2_runtime_reset(runtime);
        DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
        if (!result.ok)
        {
            log_error(
                "scene readback runtime execution failed (code=%d command=%u)",
                (int)result.code, result.command_index);
        }
        else
        {
            if (scene != NULL && scene->test.force_readback_download_failure)
            {
                log_error("scene readback buffer download forced to fail");
            }
            else
            {
                ok = dvz_drp2_runtime_download_buffer(runtime, rb_id, 0, 4, rgba);
                if (!ok)
                    log_error("scene readback buffer download failed");
            }
        }
    }
    dvz_drp2_stream_destroy(stream);
    return ok;
}



static bool _scene_process_point_pick_request(
    DvzFigure* figure, DvzDrp2Runtime* runtime, const DvzCapabilitySnapshot* caps,
    const DvzPendingPickRequest* pending)
{
    ANN(figure);
    ANN(runtime);
    ANN(caps);
    ANN(pending);
    ANN(pending->panel);

    DvzScene* scene = figure->scene;
    DvzPanel* panel = pending->panel;
    DvzPickResult miss = {
        .request_id = pending->request.request_id,
        .hit = false,
        .panel_id = _scene_panel_public_id(figure, panel),
        .panel_position = {pending->x, pending->y},
    };

    vec2 request_ndc = {0};
    if (!_scene_pick_request_ndc(figure, panel, pending->x, pending->y, request_ndc))
    {
        _scene_pick_trace(
            "picker_request request=%llu x=%.3f y=%.3f panel=%p figure=%ux%u outside_panel=1\n",
            (unsigned long long)pending->request.request_id, pending->x, pending->y,
            (void*)panel, figure->width, figure->height);
        return _scene_push_pick_result(scene, panel, pending->freshness_serial, &miss);
    }

    _scene_pick_trace(
        "picker_request request=%llu x=%.3f y=%.3f ndc=%.6f,%.6f panel=%p "
        "figure=%ux%u panel_desc=%.3f,%.3f,%.3f,%.3f visual_count=%u\n",
        (unsigned long long)pending->request.request_id, pending->x, pending->y, request_ndc[0],
        request_ndc[1], (void*)panel, figure->width, figure->height, panel->desc.x,
        panel->desc.y, panel->desc.width, panel->desc.height, panel->visual_count);

    uint32_t order[DVZ_SCENE_MAX_VISUALS] = {0};
    _scene_panel_visual_order(panel, order);

    for (int32_t oi = (int32_t)panel->visual_count - 1; oi >= 0; oi--)
    {
        DvzPanelAttach* attach = &panel->visuals[order[oi]];
        DvzVisual* visual = attach->visual;
        if (visual == NULL || !visual->visible || visual->type != DVZ_VISUAL_TYPE_POINT)
            continue;
        if ((visual->pick_capabilities & DVZ_PICK_CAPABILITY_ITEM) == 0)
            continue;
        if (attach->controller_mode == DVZ_CONTROLLER_FIXED)
            continue;

        uint64_t picked_id = 0;
        if (!_scene_point_pick_cpu(figure, panel, visual, pending->x, pending->y, &picked_id))
        {
            _scene_pick_trace(
                "picker_visual_miss request=%llu visual=%p order_index=%d attach_slot=%u\n",
                (unsigned long long)pending->request.request_id, (void*)visual, oi, order[oi]);
            continue;
        }

        DvzPickResult resolved = miss;
        resolved.hit = true;
        resolved.visual_id = _scene_visual_public_id(scene, visual);
        resolved.raw_target = DVZ_SCENE_TARGET_ITEM;
        resolved.resolved_target = DVZ_SCENE_TARGET_ITEM;
        resolved.raw_id = picked_id;
        resolved.resolved_id = picked_id;
        _scene_pick_trace(
            "picker_resolved request=%llu visual=%p visual_id=%llu item=%llu\n",
            (unsigned long long)pending->request.request_id, (void*)visual,
            (unsigned long long)resolved.visual_id, (unsigned long long)picked_id);
        return _scene_push_pick_result(scene, panel, pending->freshness_serial, &resolved);
    }

    _scene_pick_trace(
        "picker_request_miss request=%llu x=%.3f y=%.3f\n",
        (unsigned long long)pending->request.request_id, pending->x, pending->y);
    return _scene_push_pick_result(scene, panel, pending->freshness_serial, &miss);
}



static bool _scene_process_image_probe_request(
    DvzFigure* figure, DvzDrp2Runtime* runtime, const DvzCapabilitySnapshot* caps,
    const DvzPendingProbeRequest* pending)
{
    ANN(figure);
    ANN(runtime);
    ANN(caps);
    ANN(pending);
    ANN(pending->panel);

    DvzScene* scene = figure->scene;
    DvzPanel* panel = pending->panel;
    DvzProbeResult miss = {
        .request_id = pending->request.request_id,
        .hit = false,
        .panel_id = _scene_panel_public_id(figure, panel),
        .source_request_id = pending->request.request_id,
    };

    vec2 request_ndc = {0};
    if (!_scene_pick_request_ndc(figure, panel, pending->x, pending->y, request_ndc))
        return _scene_push_probe_result(scene, panel, pending->freshness_serial, &miss);

    uint32_t order[DVZ_SCENE_MAX_VISUALS] = {0};
    _scene_panel_visual_order(panel, order);

    for (int32_t oi = (int32_t)panel->visual_count - 1; oi >= 0; oi--)
    {
        DvzPanelAttach* attach = &panel->visuals[order[oi]];
        DvzVisual* visual = attach->visual;
        if (visual == NULL || !visual->visible || visual->type != DVZ_VISUAL_TYPE_IMAGE)
            continue;

        int pos_idx = _attr_index(visual, "position");
        int uv_idx = _attr_index(visual, "texcoords");
        if (pos_idx < 0 || uv_idx < 0)
            continue;
        DvzVisualAttr* pos_attr = &visual->attrs[pos_idx];
        DvzVisualAttr* uv_attr = &visual->attrs[uv_idx];
        if (pos_attr->data == NULL || uv_attr->data == NULL || pos_attr->item_count == 0 ||
            uv_attr->item_count != pos_attr->item_count || pos_attr->item_size != sizeof(vec3))
        {
            continue;
        }

        const void* texture_data = NULL;
        uint32_t texture_width = 0;
        uint32_t texture_height = 0;
        if (visual->field != NULL && visual->field->data != NULL &&
            visual->field->desc.format == DVZ_FIELD_FORMAT_RGBA8_UNORM)
        {
            texture_data = visual->field->data;
            texture_width = visual->field->desc.width;
            texture_height = visual->field->desc.height;
        }
        else
        {
            DvzFieldRegion upload_region = {0};
            const void* upload_data = NULL;
            if (!_scene_prepare_image_texture(visual, &upload_region, &upload_data) ||
                visual->texture.rgba == NULL || visual->texture.width == 0 ||
                visual->texture.height == 0)
            {
                continue;
            }
            (void)upload_region;
            (void)upload_data;
            texture_data = visual->texture.rgba;
            texture_width = visual->texture.width;
            texture_height = visual->texture.height;
        }

        if (pos_attr->item_count > UINT64_MAX / sizeof(vec3))
        {
            log_error("image probe request position buffer is too large");
            continue;
        }

        uint64_t position_bytes = pos_attr->item_count * sizeof(vec3);
        vec3* probe_positions = (vec3*)dvz_calloc(pos_attr->item_count, sizeof(vec3));
        if (probe_positions == NULL)
        {
            log_error("image probe request position buffer allocation failed");
            continue;
        }

        vec2 target_ndc = {-0.75f, -0.75f};
        /* Image shaders write positions directly, without the shared Vulkan-NDC Y flip. */
        vec2 image_request_ndc = {request_ndc[0], -request_ndc[1]};
        vec2 delta = {
            image_request_ndc[0] - target_ndc[0], image_request_ndc[1] - target_ndc[1]};
        const vec3* source_positions = (const vec3*)pos_attr->data;
        for (uint64_t j = 0; j < pos_attr->item_count; j++)
        {
            probe_positions[j][0] = source_positions[j][0] - delta[0];
            probe_positions[j][1] = source_positions[j][1] - delta[1];
            probe_positions[j][2] = source_positions[j][2];
        }

        uint64_t texture_bytes = (uint64_t)texture_width * texture_height * 4;
        DvzFramePlan* plan = dvz_frame_plan("figure.probe", pending->request.request_id);
        DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
        bool ok = plan != NULL && emitter != NULL &&
                  dvz_frame_plan_upload_bytes(
                      plan, "probe0_position", 0, position_bytes, "position", probe_positions) &&
                  dvz_frame_plan_upload_bytes(
                      plan, "probe0_texcoords", 0, uv_attr->item_count * uv_attr->item_size,
                      "texcoords", uv_attr->data) &&
                  dvz_frame_plan_upload_bytes(
                      plan, "probe0_texture", 0, texture_bytes, "texture", texture_data) &&
                  dvz_frame_plan_upload_set_texture_extent(
                      plan, texture_width, texture_height) &&
                  dvz_frame_plan_render_panel(
                      plan, "panel.probe", "target.probe", false,
                      (DvzPanelDesc){.x = 0, .y = 0, .width = 1, .height = 1}) &&
                  dvz_frame_plan_render_visual(plan, "probe0") &&
                  dvz_frame_plan_copy(plan, "target.probe", "buf.probe", 4) &&
                  dvz_frame_plan_readback(plan, "buf.probe", "request.probe");
        DvzFramePlanNode* render = plan != NULL ? dvz_frame_plan_last_render_node(plan) : NULL;
        if (render != NULL)
        {
            DvzMVP mvp = {0};
            _scene_request_apply_mvp(panel, request_ndc, &mvp);
            render->u.render.has_mvp = true;
            render->u.render.apply_mvp = mvp;
            render->u.render.controller_modes[0] = DVZ_CONTROLLER_APPLY;
        }
        if (!ok)
        {
            log_error(
                "image probe request %" PRIu64 " failed to assemble the GPU readback plan",
                pending->request.request_id);
        }

        uint8_t rgba[4] = {0};
        bool readback_ok = ok && _scene_execute_readback_plan(scene, runtime, caps, plan, emitter, rgba);
        bool hit = readback_ok && rgba[3] > 0;
        if (readback_ok && rgba[3] == 0)
        {
            log_error(
                "image probe request %" PRIu64 " returned a transparent GPU pixel",
                pending->request.request_id);
        }
        dvz_frame_plan_destroy(plan);
        dvz_frame_plan_emitter_destroy(emitter);
        dvz_free(probe_positions);

        if (!hit)
            continue;

        DvzProbeResult resolved = miss;
        resolved.hit = true;
        resolved.visual_id = _scene_visual_public_id(scene, visual);
        resolved.target = DVZ_SCENE_TARGET_PIXEL;
        resolved.value_kind = DVZ_PROBE_VALUE_VEC4;
        resolved.vector[0] = rgba[0] / 255.0;
        resolved.vector[1] = rgba[1] / 255.0;
        resolved.vector[2] = rgba[2] / 255.0;
        resolved.vector[3] = rgba[3] / 255.0;
        dvz_strlcpy(resolved.label, "rgba", sizeof(resolved.label));
        return _scene_push_probe_result(scene, panel, pending->freshness_serial, &resolved);
    }

    return _scene_push_probe_result(scene, panel, pending->freshness_serial, &miss);
}
