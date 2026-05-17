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
#include "_overflow.h"
#include "_scene.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static bool _scene_execute_readback_plan(
    const DvzScene* scene, DvzSceneRequestExecutor* executor, const DvzCapabilitySnapshot* caps,
    DvzFramePlan* plan, uint32_t target_width, uint32_t target_height, uint8_t rgba[4],
    bool* out_executed);

static bool _scene_runtime_config_matches(
    const DvzDrp2RuntimeConfig* a, const DvzDrp2RuntimeConfig* b);

static bool _scene_request_executor_prepare(
    DvzSceneRequestExecutor* executor, DvzDrp2Runtime* source_runtime);

static bool _scene_image_probe_static_versions(
    const DvzVisual* visual, uint64_t* out_position_version, uint64_t* out_texcoord_version,
    uint64_t* out_texture_version);

static bool _scene_image_probe_needs_static_upload(
    const DvzSceneRequestExecutor* executor, const DvzVisual* visual, uint64_t position_version,
    uint64_t texcoord_version, uint64_t texture_version);

static void _scene_image_probe_mark_static_uploaded(
    DvzSceneRequestExecutor* executor, DvzVisual* visual, uint64_t position_version,
    uint64_t texcoord_version, uint64_t texture_version);

static bool _scene_probe_request_has_image_candidate(
    const DvzFigure* figure, const DvzPendingProbeRequest* pending);

static bool _scene_pick_request_has_point_like_candidate(
    const DvzFigure* figure, const DvzPendingPickRequest* pending);

static bool _scene_point_like_pick_plan(
    const DvzFigure* figure, const DvzPanel* panel, DvzVisual* visual,
    const DvzPendingPickRequest* pending, const vec2 request_ndc, DvzSceneProbePlan* out_plan,
    uint32_t* out_target_width, uint32_t* out_target_height);

static bool _scene_process_point_pick_request(
    DvzFigure* figure, DvzSceneRequestExecutor* executor, const DvzCapabilitySnapshot* caps,
    const DvzPendingPickRequest* pending);

static bool _scene_process_image_probe_request(
    DvzFigure* figure, DvzSceneRequestExecutor* executor, const DvzCapabilitySnapshot* caps,
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
    DvzSceneRequestExecutor executor = {0};
    _scene_request_executor_init(&executor);
    uint32_t processed =
        _dvz_figure_process_requests_with_executor(figure, runtime, &executor, caps);
    _scene_request_executor_destroy(&executor);
    return processed;
}



/**
 * Execute queued pick/probe requests with a caller-owned retained request executor.
 *
 * @param figure the figure
 * @param runtime the caller's main DRP2 runtime
 * @param executor the retained request executor
 * @param caps the capability snapshot, or NULL for defaults
 * @return the number of consumed requests
 */
uint32_t _dvz_figure_process_requests_with_executor(
    DvzFigure* figure, DvzDrp2Runtime* runtime, DvzSceneRequestExecutor* executor,
    const DvzCapabilitySnapshot* caps)
{
    ANN(figure);
    ANN(figure->scene);
    ANN(runtime);
    ANN(executor);

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

    for (uint32_t i = 0; i < scene->pending_pick_count;)
    {
        const DvzPendingPickRequest pending = scene->pending_picks[i];
        if (pending.panel == NULL || pending.panel->figure != figure)
        {
            i++;
            continue;
        }
        if (_scene_pick_request_has_point_like_candidate(figure, &pending))
            (void)_scene_request_executor_prepare(executor, runtime);
        (void)_scene_process_point_pick_request(figure, executor, caps, &pending);
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
        if (_scene_probe_request_has_image_candidate(figure, &pending))
            (void)_scene_request_executor_prepare(executor, runtime);
        (void)_scene_process_image_probe_request(figure, executor, caps, &pending);
        _scene_remove_pending_probe_at(scene, i);
        processed++;
    }

    return processed;
}



/**
 * Initialize a retained scene request executor.
 *
 * @param executor the request executor
 */
void _scene_request_executor_init(DvzSceneRequestExecutor* executor)
{
    ANN(executor);
    dvz_memset(executor, sizeof(DvzSceneRequestExecutor), 0, sizeof(DvzSceneRequestExecutor));
}



/**
 * Destroy a retained scene request executor.
 *
 * @param executor the request executor
 */
void _scene_request_executor_destroy(DvzSceneRequestExecutor* executor)
{
    if (executor == NULL)
        return;
    if (executor->runtime != NULL)
        dvz_drp2_runtime_destroy(executor->runtime);
    if (executor->emitter != NULL)
        dvz_frame_plan_emitter_destroy(executor->emitter);
    dvz_memset(executor, sizeof(DvzSceneRequestExecutor), 0, sizeof(DvzSceneRequestExecutor));
}



/**
 * Emit, execute, and download one 4-byte readback request.
 *
 * @param scene the owning scene, used for instance-scoped test controls
 * @param runtime the DRP2 runtime
 * @param caps the capability snapshot
 * @param plan the prepared frame plan
 * @param rgba the destination 4-byte readback buffer
 * @param out_executed whether the stream executed successfully before download
 * @return true on successful execution and download
 */
static bool _scene_execute_readback_plan(
    const DvzScene* scene, DvzSceneRequestExecutor* executor, const DvzCapabilitySnapshot* caps,
    DvzFramePlan* plan, uint32_t target_width, uint32_t target_height, uint8_t rgba[4],
    bool* out_executed)
{
    ANN(executor);
    ANN(caps);
    ANN(rgba);
    ANN(out_executed);
    *out_executed = false;
    if (plan == NULL || executor->runtime == NULL || executor->emitter == NULL)
    {
        log_error("scene readback requires a prepared frame plan and emitter");
        return false;
    }

    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.target_width = target_width > 0 ? target_width : 1;
    cfg.target_height = target_height > 0 ? target_height : 1;
    DvzDrp2CommandStream* stream =
        dvz_frame_plan_emitter_emit_drp2(executor->emitter, plan, caps, &report, &cfg);
    if (stream == NULL)
    {
        log_error("scene readback DRP2 emission failed");
        return false;
    }
    uint64_t rb_id = dvz_frame_plan_emitter_object_id(executor->emitter, "_rb");
    bool ok = false;
    if (rb_id == 0)
    {
        log_error("scene readback plan did not emit the _rb buffer");
    }
    else
    {
        DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(executor->runtime, stream);
        if (!result.ok)
        {
            log_error(
                "scene readback runtime execution failed (code=%d command=%u)",
                (int)result.code, result.command_index);
        }
        else
        {
            *out_executed = true;
            if (scene != NULL && scene->test.force_readback_download_failure)
            {
                log_error("scene readback buffer download forced to fail");
            }
            else
            {
                ok = dvz_drp2_runtime_download_buffer(executor->runtime, rb_id, 0, 4, rgba);
                if (!ok)
                    log_error("scene readback buffer download failed");
            }
        }
    }
    dvz_drp2_stream_destroy(stream);
    return ok;
}


/**
 * Return whether two DRP2 runtime configurations borrow the same backend.
 *
 * @param a first runtime configuration
 * @param b second runtime configuration
 * @return true when the configurations match
 */
static bool _scene_runtime_config_matches(
    const DvzDrp2RuntimeConfig* a, const DvzDrp2RuntimeConfig* b)
{
    ANN(a);
    ANN(b);
    return a->device == b->device && a->allocator == b->allocator &&
           a->semantic_only == b->semantic_only;
}



/**
 * Ensure a retained request executor is ready for the caller runtime's backend.
 *
 * @param executor the retained request executor
 * @param source_runtime the caller's main DRP2 runtime
 * @return true when the executor is ready
 */
static bool _scene_request_executor_prepare(
    DvzSceneRequestExecutor* executor, DvzDrp2Runtime* source_runtime)
{
    ANN(executor);
    ANN(source_runtime);

    DvzDrp2RuntimeConfig runtime_cfg = dvz_drp2_runtime_config(source_runtime);
    if (executor->runtime != NULL && executor->emitter != NULL &&
        _scene_runtime_config_matches(&executor->runtime_cfg, &runtime_cfg))
    {
        return true;
    }

    _scene_request_executor_destroy(executor);
    executor->emitter = dvz_frame_plan_emitter();
    if (executor->emitter == NULL)
    {
        log_error("scene request emitter creation failed");
        return false;
    }
    executor->emitter_create_count++;

    executor->runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    if (executor->runtime == NULL)
    {
        log_error("scene request runtime creation failed");
        _scene_request_executor_destroy(executor);
        return false;
    }
    executor->runtime_cfg = runtime_cfg;
    executor->runtime_create_count++;
    return true;
}



/**
 * Return image-probe static resource versions for one visual.
 *
 * @param visual the image visual
 * @param out_position_version position attribute version
 * @param out_texcoord_version texcoord attribute version
 * @param out_texture_version texture payload version
 * @return true when required static resources exist
 */
static bool _scene_image_probe_static_versions(
    const DvzVisual* visual, uint64_t* out_position_version, uint64_t* out_texcoord_version,
    uint64_t* out_texture_version)
{
    ANN(visual);
    ANN(out_position_version);
    ANN(out_texcoord_version);
    ANN(out_texture_version);

    int pos_idx = _attr_index(visual, "position");
    int uv_idx = _attr_index(visual, "texcoords");
    if (pos_idx < 0 || uv_idx < 0)
        return false;
    const DvzVisualAttr* pos_attr = &visual->attrs[pos_idx];
    const DvzVisualAttr* uv_attr = &visual->attrs[uv_idx];
    if (pos_attr->data == NULL || uv_attr->data == NULL || pos_attr->item_count == 0 ||
        uv_attr->item_count != pos_attr->item_count || pos_attr->item_size != sizeof(vec3) ||
        uv_attr->item_size != sizeof(vec2))
    {
        return false;
    }

    *out_position_version = pos_attr->version;
    *out_texcoord_version = uv_attr->version;
    *out_texture_version = visual->texture.version;
    return true;
}



/**
 * Return whether the retained image-probe static resources must be refreshed.
 *
 * @param executor the retained request executor
 * @param visual the image visual
 * @param position_version position attribute version
 * @param texcoord_version texcoord attribute version
 * @param texture_version texture payload version
 * @return true when static uploads are required
 */
static bool _scene_image_probe_needs_static_upload(
    const DvzSceneRequestExecutor* executor, const DvzVisual* visual, uint64_t position_version,
    uint64_t texcoord_version, uint64_t texture_version)
{
    ANN(executor);
    ANN(visual);
    return executor->image_probe_visual != visual ||
           executor->image_probe_position_version != position_version ||
           executor->image_probe_texcoord_version != texcoord_version ||
           executor->image_probe_texture_version != texture_version;
}



/**
 * Mark the retained image-probe static resources as current.
 *
 * @param executor the retained request executor
 * @param visual the image visual
 * @param position_version position attribute version
 * @param texcoord_version texcoord attribute version
 * @param texture_version texture payload version
 */
static void _scene_image_probe_mark_static_uploaded(
    DvzSceneRequestExecutor* executor, DvzVisual* visual, uint64_t position_version,
    uint64_t texcoord_version, uint64_t texture_version)
{
    ANN(executor);
    ANN(visual);
    executor->image_probe_visual = visual;
    executor->image_probe_position_version = position_version;
    executor->image_probe_texcoord_version = texcoord_version;
    executor->image_probe_texture_version = texture_version;
}



/**
 * Return whether one pending probe has a visible image candidate that may need GPU readback.
 *
 * @param figure figure whose request queue is being processed
 * @param pending pending probe request
 * @return true when a matching image visual exists
 */
static bool _scene_probe_request_has_image_candidate(
    const DvzFigure* figure, const DvzPendingProbeRequest* pending)
{
    ANN(figure);
    ANN(pending);
    if (pending->panel == NULL || pending->panel->figure != figure)
        return false;

    const DvzPanel* panel = pending->panel;
    bool segment_probe = pending->request.target == DVZ_SCENE_TARGET_SEGMENT;
    for (uint32_t i = 0; i < panel->visual_count; i++)
    {
        const DvzVisual* visual = panel->visuals[i].visual;
        if (visual == NULL || visual->type != DVZ_VISUAL_TYPE_IMAGE)
            continue;
        if (segment_probe)
        {
            if ((visual->pick_capabilities & DVZ_PICK_CAPABILITY_GROUP) != 0)
                return true;
        }
        else if (visual->visible)
        {
            return true;
        }
    }
    return false;
}


/**
 * Return whether one pending pick has a visible point-like GPU candidate.
 *
 * @param figure figure whose request queue is being processed
 * @param pending pending pick request
 * @return true when a matching point, pixel, or marker visual exists
 */
static bool _scene_pick_request_has_point_like_candidate(
    const DvzFigure* figure, const DvzPendingPickRequest* pending)
{
    ANN(figure);
    ANN(pending);
    if (pending->panel == NULL || pending->panel->figure != figure)
        return false;

    const DvzPanel* panel = pending->panel;
    for (uint32_t i = 0; i < panel->visual_count; i++)
    {
        const DvzVisual* visual = panel->visuals[i].visual;
        if (visual == NULL || !visual->visible)
            continue;
        if (
            visual->type != DVZ_VISUAL_TYPE_POINT && visual->type != DVZ_VISUAL_TYPE_PIXEL &&
            visual->type != DVZ_VISUAL_TYPE_MARKER)
            continue;
        if ((visual->pick_capabilities & DVZ_PICK_CAPABILITY_ITEM) == 0)
            continue;
        if (panel->visuals[i].controller_mode == DVZ_CONTROLLER_FIXED)
            continue;
        return true;
    }
    return false;
}


/**
 * Decode a little-endian RGBA8 item id payload.
 *
 * @param rgba encoded pick pixel
 * @param out_item_id decoded zero-based item id
 * @return true when the pixel contains a non-zero id
 */
static bool _scene_decode_pick_rgba(const uint8_t rgba[4], uint64_t* out_item_id)
{
    ANN(rgba);
    ANN(out_item_id);
    uint32_t encoded =
        (uint32_t)rgba[0] | ((uint32_t)rgba[1] << 8) | ((uint32_t)rgba[2] << 16);
    if (encoded == 0)
        return false;
    *out_item_id = (uint64_t)encoded - 1;
    return true;
}


/**
 * Build a synthetic GPU readback frame plan for one point-like pick request.
 *
 * @param figure the parent figure
 * @param panel the panel receiving the request
 * @param visual the point or pixel visual to pick
 * @param pending the pending pick request
 * @param request_ndc the request coordinate in panel-local NDC
 * @param out_plan the output plan wrapper
 * @param out_target_width output offscreen target width
 * @param out_target_height output offscreen target height
 * @return true when the plan was assembled
 */
static bool _scene_point_like_pick_plan(
    const DvzFigure* figure, const DvzPanel* panel, DvzVisual* visual,
    const DvzPendingPickRequest* pending, const vec2 request_ndc, DvzSceneProbePlan* out_plan,
    uint32_t* out_target_width, uint32_t* out_target_height)
{
    ANN(figure);
    ANN(panel);
    ANN(visual);
    ANN(pending);
    ANN(request_ndc);
    ANN(out_plan);
    ANN(out_target_width);
    ANN(out_target_height);

    int pos_idx = _attr_index(visual, "position");
    int color_idx = _attr_index(visual, "color");
    int size_idx = _attr_index(visual, "size");
    if (pos_idx < 0 || color_idx < 0 || size_idx < 0)
        return false;

    DvzVisualAttr* pos_attr = &visual->attrs[pos_idx];
    DvzVisualAttr* color_attr = &visual->attrs[color_idx];
    DvzVisualAttr* size_attr = &visual->attrs[size_idx];
    if (pos_attr->data == NULL || color_attr->data == NULL || size_attr->data == NULL ||
        pos_attr->item_count == 0 || color_attr->item_count != pos_attr->item_count ||
        size_attr->item_count != pos_attr->item_count || pos_attr->item_size != sizeof(vec3) ||
        color_attr->item_size != sizeof(DvzColor) || size_attr->item_size != sizeof(float))
    {
        return false;
    }

    uint64_t position_bytes = 0;
    uint64_t color_bytes = 0;
    uint64_t size_bytes = 0;
    if (_dvz_mul_u64_overflows(pos_attr->item_count, pos_attr->item_size, &position_bytes) ||
        _dvz_mul_u64_overflows(color_attr->item_count, color_attr->item_size, &color_bytes) ||
        _dvz_mul_u64_overflows(size_attr->item_count, size_attr->item_size, &size_bytes))
    {
        log_error("point-like pick request buffer size overflow");
        return false;
    }

    double panel_width = panel->desc.width * (double)figure->width;
    double panel_height = panel->desc.height * (double)figure->height;
    if (panel_width <= 0.0 || panel_height <= 0.0)
        return false;
    uint32_t target_width = (uint32_t)(panel_width + 0.5);
    uint32_t target_height = (uint32_t)(panel_height + 0.5);
    if (target_width == 0)
        target_width = 1;
    if (target_height == 0)
        target_height = 1;

    DvzColor* pick_colors = (DvzColor*)dvz_calloc(pos_attr->item_count, sizeof(DvzColor));
    if (pick_colors == NULL)
        return false;
    for (uint64_t i = 0; i < pos_attr->item_count; i++)
    {
        uint32_t encoded = (uint32_t)i + 1u;
        pick_colors[i][0] = (uint8_t)(encoded & 0xFFu);
        pick_colors[i][1] = (uint8_t)((encoded >> 8u) & 0xFFu);
        pick_colors[i][2] = (uint8_t)((encoded >> 16u) & 0xFFu);
        pick_colors[i][3] = 255;
    }

    DvzFramePlan* plan = dvz_frame_plan("figure.pick", pending->request.request_id);
    bool ok = plan != NULL;
    ok = ok && dvz_frame_plan_upload_bytes(
                   plan, "pick0_position", 0, position_bytes, "position", pos_attr->data) &&
         dvz_frame_plan_upload_bytes(
             plan, "pick0_color", 0, color_bytes, "color", pick_colors) &&
         dvz_frame_plan_upload_bytes(
             plan, "pick0_size", 0, size_bytes, "size", size_attr->data);

    DvzFramePlanVisualMeta metadata = {0};
    metadata.has_metadata = true;
    metadata.visual_type =
        visual->type == DVZ_VISUAL_TYPE_MARKER ? (uint32_t)DVZ_VISUAL_TYPE_PIXEL :
                                                 (uint32_t)visual->type;
    metadata.alpha_mode = DVZ_ALPHA_OPAQUE;
    metadata.depth_test_enabled = visual->depth_test_enabled;
    dvz_strlcpy(metadata.position_id, "pick0_position", sizeof(metadata.position_id));
    dvz_strlcpy(metadata.color_id, "pick0_color", sizeof(metadata.color_id));
    dvz_strlcpy(metadata.size_id, "pick0_size", sizeof(metadata.size_id));

    ok = ok && dvz_frame_plan_render_panel(
                   plan, "panel.pick", "target.pick", true,
                   (DvzPanelDesc){.x = 0, .y = 0, .width = 1, .height = 1}) &&
         dvz_frame_plan_render_visual(plan, "pick0") &&
         dvz_frame_plan_render_visual_metadata(plan, &metadata);

    DvzFramePlanNode* render = plan != NULL ? dvz_frame_plan_last_render_node(plan) : NULL;
    if (render != NULL)
    {
        DvzMVP mvp = {0};
        _scene_panel_apply_mvp(panel, &mvp);
        vec2 target_ndc = {
            -1.0f + 1.0f / (float)target_width,
            1.0f - 1.0f / (float)target_height,
        };
        vec2 delta = {request_ndc[0] - target_ndc[0], request_ndc[1] - target_ndc[1]};
        mvp.proj[3][0] -= delta[0];
        mvp.proj[3][1] -= delta[1];
        render->u.render.has_mvp = true;
        render->u.render.apply_mvp = mvp;
        render->u.render.has_viewport = true;
        render->u.render.viewport =
            (DvzSceneViewportUniform){0.0f, 0.0f, (float)target_width, (float)target_height};
        render->u.render.controller_modes[0] = DVZ_CONTROLLER_APPLY;
    }

    ok = ok && dvz_frame_plan_copy(plan, "target.pick", "buf.pick", 4) &&
         dvz_frame_plan_readback(plan, "buf.pick", "request.pick");
    if (!ok)
    {
        log_error(
            "point-like pick request %" PRIu64 " failed to assemble the GPU readback plan",
            pending->request.request_id);
        dvz_frame_plan_destroy(plan);
        dvz_free(pick_colors);
        return false;
    }

    out_plan->plan = plan;
    out_plan->pick_colors = pick_colors;
    *out_target_width = target_width;
    *out_target_height = target_height;
    return true;
}



static bool _scene_process_point_pick_request(
    DvzFigure* figure, DvzSceneRequestExecutor* executor, const DvzCapabilitySnapshot* caps,
    const DvzPendingPickRequest* pending)
{
    ANN(figure);
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
        if (visual == NULL || !visual->visible)
            continue;
        if (
            visual->type != DVZ_VISUAL_TYPE_POINT && visual->type != DVZ_VISUAL_TYPE_PIXEL &&
            visual->type != DVZ_VISUAL_TYPE_MARKER)
            continue;
        if ((visual->pick_capabilities & DVZ_PICK_CAPABILITY_ITEM) == 0)
            continue;
        if (attach->controller_mode == DVZ_CONTROLLER_FIXED)
            continue;

        if (executor == NULL || executor->runtime == NULL || executor->emitter == NULL)
        {
            log_error("point-like pick request requires a DRP2 runtime");
            continue;
        }

        DvzSceneProbePlan pick_plan = {0};
        uint32_t target_width = 0;
        uint32_t target_height = 0;
        if (!_scene_point_like_pick_plan(
                figure, panel, visual, pending, request_ndc, &pick_plan, &target_width,
                &target_height))
        {
            _scene_pick_trace(
                "picker_visual_miss request=%llu visual=%p order_index=%d attach_slot=%u\n",
                (unsigned long long)pending->request.request_id, (void*)visual, oi, order[oi]);
            continue;
        }

        uint8_t rgba[4] = {0};
        bool executed = false;
        bool readback_ok = _scene_execute_readback_plan(
            scene, executor, caps, pick_plan.plan, target_width, target_height, rgba, &executed);
        _scene_probe_plan_destroy(&pick_plan);

        uint64_t picked_id = 0;
        if (!readback_ok || !_scene_decode_pick_rgba(rgba, &picked_id))
        {
            _scene_pick_trace(
                "picker_visual_miss request=%llu visual=%p order_index=%d attach_slot=%u "
                "readback_ok=%d executed=%d rgba=%u,%u,%u,%u\n",
                (unsigned long long)pending->request.request_id, (void*)visual, oi, order[oi],
                readback_ok ? 1 : 0, executed ? 1 : 0, rgba[0], rgba[1], rgba[2], rgba[3]);
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
    DvzFigure* figure, DvzSceneRequestExecutor* executor, const DvzCapabilitySnapshot* caps,
    const DvzPendingProbeRequest* pending)
{
    ANN(figure);
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

        if (executor == NULL || executor->runtime == NULL || executor->emitter == NULL)
        {
            log_error("image probe request requires a DRP2 runtime");
            continue;
        }

        uint64_t position_version = 0;
        uint64_t texcoord_version = 0;
        uint64_t texture_version = 0;
        if (!_scene_image_probe_static_versions(
                visual, &position_version, &texcoord_version, &texture_version))
        {
            continue;
        }

        bool include_static_uploads = _scene_image_probe_needs_static_upload(
            executor, visual, position_version, texcoord_version, texture_version);

        DvzSceneProbePlan probe_plan = {0};
        if (!_scene_image_probe_plan(
                panel, visual, pending, request_ndc, include_static_uploads, &probe_plan))
            continue;

        uint8_t rgba[4] = {0};
        bool executed = false;
        bool readback_ok = _scene_execute_readback_plan(
            scene, executor, caps, probe_plan.plan, 1, 1, rgba, &executed);
        if (executed && include_static_uploads)
        {
            _scene_image_probe_mark_static_uploaded(
                executor, visual, position_version, texcoord_version, texture_version);
            executor->image_probe_static_upload_count++;
        }
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
