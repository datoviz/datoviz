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

static bool _scene_push_pick_result(
    DvzScene* scene, DvzPanel* panel, const DvzPickResult* result);

static bool _scene_push_probe_result(
    DvzScene* scene, DvzPanel* panel, const DvzProbeResult* result);

static bool _scene_pick_request_ndc(
    const DvzFigure* figure, const DvzPanel* panel, double x, double y, vec2 out_ndc);

static uint64_t _scene_panel_public_id(const DvzFigure* figure, const DvzPanel* panel);

static void _scene_center_apply_mvp(DvzMVP* mvp, const vec2 ndc);

static bool _scene_decode_pick_id(const uint8_t rgba[4], uint64_t* out_id);

static bool _scene_execute_readback_plan(
    const DvzScene* scene, DvzDrp2Runtime* runtime, const DvzCapabilitySnapshot* caps,
    DvzFramePlan* plan, DvzFramePlanEmitter* emitter, uint8_t rgba[4]);

static bool _scene_process_point_pick_request(
    DvzFigure* figure, DvzDrp2Runtime* runtime, const DvzCapabilitySnapshot* caps,
    const DvzPendingPickRequest* pending);

static bool _scene_process_image_probe_request(
    DvzFigure* figure, DvzDrp2Runtime* runtime, const DvzCapabilitySnapshot* caps,
    const DvzPendingProbeRequest* pending);

static void _scene_remove_pending_pick_at(DvzScene* scene, uint32_t index);

static void _scene_remove_pending_probe_at(DvzScene* scene, uint32_t index);



/**
 * Append one resolved pick result to the scene queue.
 *
 * @param scene the scene
 * @param panel the owning panel, or NULL for synthetic test injection
 * @param result the result payload
 * @return true on success, false when the queue is full
 */
static bool _scene_push_pick_result(
    DvzScene* scene, DvzPanel* panel, const DvzPickResult* result)
{
    ANN(scene);
    ANN(result);
    if (scene->pick_result_count >= DVZ_SCENE_MAX_PICK_RESULTS)
    {
        log_error("pick result queue is full");
        return false;
    }
    uint32_t index = (scene->pick_result_head + scene->pick_result_count) % DVZ_SCENE_MAX_PICK_RESULTS;
    scene->pick_results[index].panel = panel;
    scene->pick_results[index].result = *result;
    scene->pick_result_count++;
    return true;
}



/**
 * Append one resolved probe result to the scene queue.
 *
 * @param scene the scene
 * @param panel the owning panel, or NULL for synthetic test injection
 * @param result the result payload
 * @return true on success, false when the queue is full
 */
static bool _scene_push_probe_result(
    DvzScene* scene, DvzPanel* panel, const DvzProbeResult* result)
{
    ANN(scene);
    ANN(result);
    if (scene->probe_result_count >= DVZ_SCENE_MAX_PROBE_RESULTS)
    {
        log_error("probe result queue is full");
        return false;
    }
    uint32_t index =
        (scene->probe_result_head + scene->probe_result_count) % DVZ_SCENE_MAX_PROBE_RESULTS;
    scene->probe_results[index].panel = panel;
    scene->probe_results[index].result = *result;
    scene->probe_result_count++;
    return true;
}



static bool _scene_pick_request_ndc(
    const DvzFigure* figure, const DvzPanel* panel, double x, double y, vec2 out_ndc)
{
    ANN(figure);
    ANN(panel);
    ANN(out_ndc);
    if (figure->width == 0 || figure->height == 0)
        return false;

    double panel_width = panel->desc.width * (double)figure->width;
    double panel_height = panel->desc.height * (double)figure->height;
    if (panel_width <= 0.0 || panel_height <= 0.0)
        return false;

    double px = x / panel_width;
    double py = y / panel_height;
    if (px < 0.0 || px > 1.0 || py < 0.0 || py > 1.0)
        return false;

    out_ndc[0] = (float)(2.0 * px - 1.0);
    out_ndc[1] = (float)(1.0 - 2.0 * py);
    return true;
}



static void _scene_center_apply_mvp(DvzMVP* mvp, const vec2 ndc)
{
    ANN(mvp);
    mvp->proj[3][0] -= ndc[0];
    mvp->proj[3][1] -= ndc[1];
}



static bool _scene_decode_pick_id(const uint8_t rgba[4], uint64_t* out_id)
{
    ANN(rgba);
    ANN(out_id);
    *out_id = (uint64_t)rgba[0] | ((uint64_t)rgba[1] << 8) | ((uint64_t)rgba[2] << 16) |
              ((uint64_t)rgba[3] << 24);
    return *out_id != 0;
}



static void _scene_remove_pending_pick_at(DvzScene* scene, uint32_t index)
{
    ANN(scene);
    ASSERT(index < scene->pending_pick_count);
    for (uint32_t i = index + 1; i < scene->pending_pick_count; i++)
        scene->pending_picks[i - 1] = scene->pending_picks[i];
    scene->pending_pick_count--;
    dvz_memset(
        &scene->pending_picks[scene->pending_pick_count], sizeof(DvzPendingPickRequest), 0,
        sizeof(DvzPendingPickRequest));
}



static void _scene_remove_pending_probe_at(DvzScene* scene, uint32_t index)
{
    ANN(scene);
    ASSERT(index < scene->pending_probe_count);
    for (uint32_t i = index + 1; i < scene->pending_probe_count; i++)
        scene->pending_probes[i - 1] = scene->pending_probes[i];
    scene->pending_probe_count--;
    dvz_memset(
        &scene->pending_probes[scene->pending_probe_count], sizeof(DvzPendingProbeRequest), 0,
        sizeof(DvzPendingProbeRequest));
}



/**
 * Resolve the stable public panel id within one figure.
 *
 * @param figure the figure
 * @param panel the panel
 * @return the 1-based public panel id, or 1 when not found
 */
static uint64_t _scene_panel_public_id(const DvzFigure* figure, const DvzPanel* panel)
{
    ANN(figure);
    ANN(panel);
    for (uint32_t pi = 0; pi < figure->panel_count; pi++)
    {
        if (&figure->panels[pi] == panel)
            return (uint64_t)pi + 1;
    }
    return 1;
}



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

    for (uint32_t i = 0; i < scene->pending_pick_count;)
    {
        const DvzPendingPickRequest pending = scene->pending_picks[i];
        if (pending.panel == NULL || pending.panel->figure != figure)
        {
            i++;
            continue;
        }
        (void)_scene_process_point_pick_request(figure, runtime, caps, &pending);
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
        (void)_scene_process_image_probe_request(figure, runtime, caps, &pending);
        _scene_remove_pending_probe_at(scene, i);
        processed++;
    }

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
        return _scene_push_pick_result(scene, panel, &miss);

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

        int pos_idx = _attr_index(visual, "position");
        int color_idx = _attr_index(visual, "color");
        int size_idx = _attr_index(visual, "size");
        if (pos_idx < 0 || color_idx < 0 || size_idx < 0)
            continue;
        DvzVisualAttr* pos_attr = &visual->attrs[pos_idx];
        DvzVisualAttr* color_attr = &visual->attrs[color_idx];
        DvzVisualAttr* size_attr = &visual->attrs[size_idx];
        if (pos_attr->data == NULL || color_attr->data == NULL || size_attr->data == NULL ||
            pos_attr->item_count == 0 || color_attr->item_count != pos_attr->item_count ||
            size_attr->item_count != pos_attr->item_count)
        {
            continue;
        }

        DvzFramePlan* plan = dvz_frame_plan("figure.pick", pending->request.request_id);
        DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();

        DvzMVP mvp = {0};
        glm_mat4_identity(mvp.model);
        glm_mat4_identity(mvp.view);
        glm_mat4_identity(mvp.proj);
        if (panel->panzoom != NULL)
            dvz_panzoom_mvp(panel->panzoom, &mvp);
        if (panel->arcball != NULL)
            dvz_arcball_mvp(panel->arcball, &mvp);
        vec2 target_ndc = {-0.75f, -0.75f};
        vec2 delta = {request_ndc[0] - target_ndc[0], request_ndc[1] - target_ndc[1]};
        _scene_center_apply_mvp(&mvp, delta);

        bool ok = plan != NULL && emitter != NULL &&
                  dvz_frame_plan_upload_bytes(
                      plan, "pick0_position", 0, pos_attr->item_count * pos_attr->item_size,
                      "position", pos_attr->data) &&
                  dvz_frame_plan_upload_bytes(
                      plan, "pick0_color", 0, color_attr->item_count * color_attr->item_size,
                      "color", color_attr->data) &&
                  dvz_frame_plan_upload_bytes(
                      plan, "pick0_size", 0, size_attr->item_count * size_attr->item_size, "size",
                      size_attr->data) &&
                  dvz_frame_plan_render_panel(
                      plan, "panel.pick", "target.pick", true,
                      (DvzPanelDesc){.x = 0, .y = 0, .width = 1, .height = 1}) &&
                  dvz_frame_plan_render_visual(plan, "pick0") &&
                  dvz_frame_plan_copy(plan, "target.pick", "buf.pick", 4) &&
                  dvz_frame_plan_readback(plan, "buf.pick", "request.pick");
        DvzFramePlanNode* render = plan != NULL ? dvz_frame_plan_last_render_node(plan) : NULL;
        if (render != NULL)
        {
            render->u.render.has_mvp = true;
            render->u.render.apply_mvp = mvp;
            render->u.render.controller_modes[0] = DVZ_CONTROLLER_APPLY;
        }

        uint8_t rgba[4] = {0};
        uint64_t picked_id = 0;
        bool hit =
            ok && _scene_execute_readback_plan(scene, runtime, caps, plan, emitter, rgba) &&
            _scene_decode_pick_id(rgba, &picked_id);
        dvz_frame_plan_destroy(plan);
        dvz_frame_plan_emitter_destroy(emitter);

        if (!hit)
            continue;

        DvzPickResult resolved = miss;
        resolved.hit = true;
        resolved.visual_id = _scene_visual_public_id(scene, visual);
        resolved.raw_target = DVZ_SCENE_TARGET_ITEM;
        resolved.resolved_target = DVZ_SCENE_TARGET_ITEM;
        resolved.raw_id = picked_id - 1;
        resolved.resolved_id = picked_id - 1;
        return _scene_push_pick_result(scene, panel, &resolved);
    }

    return _scene_push_pick_result(scene, panel, &miss);
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
        return _scene_push_probe_result(scene, panel, &miss);

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
            uv_attr->item_count != pos_attr->item_count)
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

        uint64_t position_bytes = pos_attr->item_count * pos_attr->item_size;
        uint64_t texture_bytes = (uint64_t)texture_width * texture_height * 4;
        DvzFramePlan* plan = dvz_frame_plan("figure.probe", pending->request.request_id);
        DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
        bool ok = plan != NULL && emitter != NULL &&
                  dvz_frame_plan_upload_bytes(
                      plan, "probe0_position", 0, position_bytes, "position", pos_attr->data) &&
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
        return _scene_push_probe_result(scene, panel, &resolved);
    }

    return _scene_push_probe_result(scene, panel, &miss);
}



/**
 * Push one resolved pick result into the internal scene queue.
 *
 * @param scene the scene
 * @param result the resolved result
 * @return true on success
 */
bool _dvz_scene_enqueue_pick_result(DvzScene* scene, const DvzPickResult* result)
{
    return _scene_push_pick_result(scene, NULL, result);
}



/**
 * Push one resolved probe result into the internal scene queue.
 *
 * @param scene the scene
 * @param result the resolved result
 * @return true on success
 */
bool _dvz_scene_enqueue_probe_result(DvzScene* scene, const DvzProbeResult* result)
{
    return _scene_push_probe_result(scene, NULL, result);
}
