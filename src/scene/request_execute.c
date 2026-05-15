/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene request execution                                                                      */
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

static bool _scene_image_probe_sample_cpu(
    const DvzPanel* panel, DvzVisual* visual, const vec2 request_ndc, uint8_t rgba[4]);

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


/**
 * Sample the retained image texture at one panel-local request coordinate.
 *
 * @param panel the panel owning the probed visual
 * @param visual the image visual
 * @param request_ndc the request coordinate in panel-local NDC
 * @param rgba the output RGBA8 value
 * @return true when a retained texture value was sampled
 */
static bool _scene_image_probe_sample_cpu(
    const DvzPanel* panel, DvzVisual* visual, const vec2 request_ndc, uint8_t rgba[4])
{
    ANN(panel);
    ANN(visual);
    ANN(request_ndc);
    ANN(rgba);

    const uint8_t* data = NULL;
    uint32_t width = 0;
    uint32_t height = 0;
    if (visual->field != NULL && visual->field->data != NULL &&
        visual->field->desc.format == DVZ_FIELD_FORMAT_RGBA8_UNORM)
    {
        data = (const uint8_t*)visual->field->data;
        width = visual->field->desc.width;
        height = visual->field->desc.height;
    }
    else
    {
        DvzFieldRegion upload_region = {0};
        const void* upload_data = NULL;
        if (!_scene_prepare_image_texture(visual, &upload_region, &upload_data) ||
            visual->texture.rgba == NULL)
            return false;
        (void)upload_region;
        (void)upload_data;
        data = (const uint8_t*)visual->texture.rgba;
        width = visual->texture.width;
        height = visual->texture.height;
    }
    if (data == NULL || width == 0 || height == 0)
        return false;

    double u = 0.5 * ((double)request_ndc[0] + 1.0);
    double v = 0.5 * (1.0 - (double)request_ndc[1]);
    int pos_idx = _attr_index(visual, "position");
    int uv_idx = _attr_index(visual, "texcoords");
    if (pos_idx >= 0 && uv_idx >= 0)
    {
        DvzVisualAttr* pos_attr = &visual->attrs[pos_idx];
        DvzVisualAttr* uv_attr = &visual->attrs[uv_idx];
        if (pos_attr->data != NULL && uv_attr->data != NULL && pos_attr->item_count > 0 &&
            uv_attr->item_count == pos_attr->item_count && pos_attr->item_size == sizeof(vec3) &&
            uv_attr->item_size == sizeof(vec2))
        {
            const vec3* positions = (const vec3*)pos_attr->data;
            const vec2* texcoords = (const vec2*)uv_attr->data;
            float min_x = positions[0][0];
            float max_x = positions[0][0];
            float min_y = positions[0][1];
            float max_y = positions[0][1];
            float min_u = texcoords[0][0];
            float max_u = texcoords[0][0];
            float min_v = texcoords[0][1];
            float max_v = texcoords[0][1];
            for (uint64_t j = 1; j < pos_attr->item_count; j++)
            {
                if (positions[j][0] < min_x)
                    min_x = positions[j][0];
                if (positions[j][0] > max_x)
                    max_x = positions[j][0];
                if (positions[j][1] < min_y)
                    min_y = positions[j][1];
                if (positions[j][1] > max_y)
                    max_y = positions[j][1];
                if (texcoords[j][0] < min_u)
                    min_u = texcoords[j][0];
                if (texcoords[j][0] > max_u)
                    max_u = texcoords[j][0];
                if (texcoords[j][1] < min_v)
                    min_v = texcoords[j][1];
                if (texcoords[j][1] > max_v)
                    max_v = texcoords[j][1];
            }
            if (max_x != min_x && max_y != min_y)
            {
                DvzMVP mvp = {0};
                _scene_panel_apply_mvp(panel, &mvp);
                mat4 proj_view = GLM_MAT4_IDENTITY_INIT;
                mat4 proj_view_model = GLM_MAT4_IDENTITY_INIT;
                mat4 inv_proj_view_model = GLM_MAT4_IDENTITY_INIT;
                glm_mat4_mul(mvp.proj, mvp.view, proj_view);
                glm_mat4_mul(proj_view, mvp.model, proj_view_model);
                glm_mat4_inv(proj_view_model, inv_proj_view_model);

                vec4 clip = {request_ndc[0], -request_ndc[1], 0.0f, 1.0f};
                vec4 local = {0};
                glm_mat4_mulv(inv_proj_view_model, clip, local);
                if (local[3] != 0.0f)
                {
                    double x = (double)(local[0] / local[3]);
                    double y = (double)(local[1] / local[3]);
                    double su = (x - min_x) / (double)(max_x - min_x);
                    double sv = (y - min_y) / (double)(max_y - min_y);
                    u = min_u + su * (double)(max_u - min_u);
                    v = min_v + sv * (double)(max_v - min_v);
                }
            }
        }
    }
    if (u < 0.0)
        u = 0.0;
    if (v < 0.0)
        v = 0.0;
    if (u >= 1.0)
        u = 1.0 - 1e-12;
    if (v >= 1.0)
        v = 1.0 - 1e-12;

    uint32_t x = (uint32_t)(u * (double)width);
    uint32_t y = (uint32_t)(v * (double)height);
    if (x >= width)
        x = width - 1;
    if (y >= height)
        y = height - 1;

    uint64_t index = 4 * ((uint64_t)y * width + x);
    rgba[0] = data[index + 0];
    rgba[1] = data[index + 1];
    rgba[2] = data[index + 2];
    rgba[3] = data[index + 3];
    return true;
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
    bool segment_probe = pending->request.target == DVZ_SCENE_TARGET_SEGMENT;

    for (int32_t oi = (int32_t)panel->visual_count - 1; oi >= 0; oi--)
    {
        DvzPanelAttach* attach = &panel->visuals[order[oi]];
        DvzVisual* visual = attach->visual;
        if (visual == NULL || visual->type != DVZ_VISUAL_TYPE_IMAGE)
            continue;
        if (segment_probe)
        {
            if ((visual->pick_capabilities & DVZ_PICK_CAPABILITY_GROUP) == 0)
                continue;
        }
        else if (!visual->visible)
        {
            continue;
        }

        DvzSceneProbePlan probe_plan = {0};
        if (!_scene_image_probe_plan(panel, visual, pending, request_ndc, &probe_plan))
            continue;

        uint8_t rgba[4] = {0};
        bool readback_ok = _scene_execute_readback_plan(
            scene, runtime, caps, probe_plan.plan, probe_plan.emitter, rgba);
        if (readback_ok)
            (void)_scene_image_probe_sample_cpu(panel, visual, request_ndc, rgba);
        bool hit = readback_ok && rgba[3] > 0;
        if (!segment_probe && readback_ok && rgba[3] == 0)
        {
            log_error(
                "image probe request %" PRIu64 " returned a transparent GPU pixel",
                pending->request.request_id);
        }
        _scene_probe_plan_destroy(&probe_plan);

        if (!hit)
            continue;

        DvzProbeResult resolved = miss;
        resolved.hit = true;
        resolved.visual_id = _scene_visual_public_id(scene, visual);
        if (segment_probe)
        {
            uint64_t label_id = (uint64_t)rgba[0] | ((uint64_t)rgba[1] << 8) |
                                ((uint64_t)rgba[2] << 16);
            if (label_id == 0)
                continue;
            resolved.target = DVZ_SCENE_TARGET_SEGMENT;
            resolved.target_id = label_id;
            resolved.value_kind = DVZ_PROBE_VALUE_LABEL;
            resolved.category_id = label_id;
            dvz_snprintf(resolved.label, sizeof(resolved.label), "label %" PRIu64, label_id);
        }
        else
        {
            resolved.target = DVZ_SCENE_TARGET_PIXEL;
            resolved.value_kind = DVZ_PROBE_VALUE_VEC4;
            resolved.vector[0] = rgba[0] / 255.0;
            resolved.vector[1] = rgba[1] / 255.0;
            resolved.vector[2] = rgba[2] / 255.0;
            resolved.vector[3] = rgba[3] / 255.0;
            dvz_strlcpy(resolved.label, "rgba", sizeof(resolved.label));
        }
        return _scene_push_probe_result(scene, panel, pending->freshness_serial, &resolved);
    }

    return _scene_push_probe_result(scene, panel, pending->freshness_serial, &miss);
}
