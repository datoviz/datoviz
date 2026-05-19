/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene graph — DvzScene / DvzFigure / DvzPanel / DvzVisual                                  */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_overflow.h"
#include "_scene_emit.h"
#include "render_contract.h"
#include "datoviz/drp2/runtime.h"
#include "datoviz/math/_cglm.h"
#include "../drp2/_stream.h"
#include "_scene.h"
#include "_technique.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static void _scene_stream_release(void* owner);


/**
 * Return whether a panel layout reservation is finite and leaves non-empty plot space.
 *
 * @param reserve the reservation descriptor
 * @return whether the reservation is valid
 */
static bool _panel_layout_reserve_valid(const DvzPanelLayoutReserve* reserve)
{
    ANN(reserve);
    if (!isfinite(reserve->left) || !isfinite(reserve->right) || !isfinite(reserve->bottom) ||
        !isfinite(reserve->top))
        return false;
    if (reserve->left < 0.0f || reserve->right < 0.0f || reserve->bottom < 0.0f ||
        reserve->top < 0.0f)
        return false;
    return reserve->left + reserve->right < 2.0f && reserve->bottom + reserve->top < 2.0f;
}

static bool _scene_stream_register(DvzScene* scene, DvzDrp2CommandStream* stream);

static bool _scene_has_live_streams(const DvzScene* scene);

static void _scene_figure_id(const DvzFigure* figure, char* out, uint32_t size);

static void _scene_panel_pixel_size(const DvzPanel* panel, float* out_width, float* out_height);

static bool _scene_pick_request_supersedes(
    const DvzPendingPickRequest* pending, const DvzPanel* panel, uint64_t request_id);

static bool _scene_probe_request_supersedes(
    const DvzPendingProbeRequest* pending, const DvzPanel* panel, uint64_t request_id);

static void _scene_drop_superseded_pick_requests(
    DvzScene* scene, const DvzPanel* panel, uint64_t request_id);

static void _scene_drop_superseded_probe_requests(
    DvzScene* scene, const DvzPanel* panel, uint64_t request_id);

static void _scene_drop_superseded_pick_results(
    DvzScene* scene, const DvzPanel* panel, uint64_t request_id);

static void _scene_drop_superseded_probe_results(
    DvzScene* scene, const DvzPanel* panel, uint64_t request_id);

static bool _scene_request_ids_share_freshness_scope(uint64_t lhs_request_id, uint64_t rhs_request_id);

static DvzRequestFreshnessScope* _scene_touch_request_scope(
    DvzRequestFreshnessScope* scopes, uint32_t* scope_count, DvzPanel* panel, uint64_t request_id,
    uint64_t freshness_serial);

static uint64_t _scene_latest_request_scope_serial(
    const DvzRequestFreshnessScope* scopes, uint32_t scope_count, const DvzPanel* panel,
    uint64_t request_id);

static void _scene_emit_defaults(
    const DvzCapabilitySnapshot** caps, DvzCapabilitySnapshot* default_caps,
    DvzDiagnosticReport** report, DvzDiagnosticReport* local_report,
    const DvzFramePlanEmitConfig** cfg, DvzFramePlanEmitConfig* default_cfg);

static bool _scene_figure_validate_transparency_modes(
    const DvzFigure* figure, const char* figure_id, DvzDiagnosticReport* report);

static void _scene_report_capability_fallbacks(
    const DvzFramePlan* plan, const DvzCapabilitySnapshot* caps, DvzDiagnosticReport* report);

static void _scene_commit_emit_success(DvzFigure* figure);

static bool _scene_visual_has_pending_render_work(const DvzVisual* visual);

static bool _scene_panel_has_pending_adornment_work(const DvzPanel* panel);

static bool _scene_panel_has_pending_visual_work(const DvzPanel* panel);

static bool _scene_visual_dirty_material_emits_upload(const DvzVisual* visual);

/**
 * Resolve the stable emitted figure identifier for one scene figure.
 *
 * @param figure the figure
 * @param out the destination string buffer
 * @param size the destination buffer size in bytes
 */
static void _scene_figure_id(const DvzFigure* figure, char* out, uint32_t size)
{
    ANN(figure);
    ANN(out);
    ASSERT(size > 0);
    dvz_strlcpy(out, "fig0", size);
    if (figure->scene == NULL)
        return;
    for (uint32_t i = 0; i < figure->scene->figure_count; i++)
    {
        if (&figure->scene->figures[i] == figure)
        {
            dvz_snprintf(out, size, "fig%u", i);
            return;
        }
    }
}



/**
 * Build the per-panel apply MVP from the active controller state.
 *
 * @param panel the panel
 * @param out the destination MVP
 */
void _scene_panel_apply_mvp(const DvzPanel* panel, DvzMVP* out)
{
    ANN(panel);
    ANN(out);
    glm_mat4_identity(out->model);
    glm_mat4_identity(out->view);
    glm_mat4_identity(out->proj);
    out->time  = 0.0f;
    out->flags = 0;
    if (panel->camera != NULL)
        dvz_camera_mvp(panel->camera, out);
    else if (panel->panzoom != NULL)
        dvz_panzoom_mvp(panel->panzoom, out);
    if (panel->arcball != NULL)
        dvz_arcball_mvp(panel->arcball, out);
}



/**
 * Return a panel's pixel size, falling back to a conventional viewport size.
 *
 * @param panel the panel
 * @param out_width output width in pixels
 * @param out_height output height in pixels
 */
static void _scene_panel_pixel_size(const DvzPanel* panel, float* out_width, float* out_height)
{
    ANN(panel);
    ANN(out_width);
    ANN(out_height);
    float figure_width = panel->figure != NULL && panel->figure->width > 0 ?
                             (float)panel->figure->width :
                             800.0f;
    float figure_height = panel->figure != NULL && panel->figure->height > 0 ?
                              (float)panel->figure->height :
                              600.0f;
    float width = panel->desc.width * figure_width;
    float height = panel->desc.height * figure_height;
    *out_width = width > 0.0f ? width : 800.0f;
    *out_height = height > 0.0f ? height : 600.0f;
}



/**
 * Return a panel's pixel rectangle, falling back to a conventional viewport size.
 *
 * @param panel the panel
 * @param out_x output x origin in pixels
 * @param out_y output y origin in pixels
 * @param out_width output width in pixels
 * @param out_height output height in pixels
 */
void _scene_panel_pixel_rect(
    const DvzPanel* panel, float* out_x, float* out_y, float* out_width, float* out_height)
{
    ANN(panel);
    ANN(out_x);
    ANN(out_y);
    ANN(out_width);
    ANN(out_height);
    float figure_width = panel->figure != NULL && panel->figure->width > 0 ?
                             (float)panel->figure->width :
                             800.0f;
    float figure_height = panel->figure != NULL && panel->figure->height > 0 ?
                              (float)panel->figure->height :
                              600.0f;
    *out_x = panel->desc.x * figure_width;
    *out_y = panel->desc.y * figure_height;
    _scene_panel_pixel_size(panel, out_width, out_height);
}



/**
 * Return whether two request ids belong to the same freshness/supersession scope.
 *
 * Freshness is currently tracked per panel and request kind. Explicit non-zero request ids only
 * supersede older work with the same id, while anonymous zero-id requests use latest-request-wins
 * semantics within their panel/kind stream.
 *
 * @param lhs_request_id the first request id
 * @param rhs_request_id the second request id
 * @return true when the ids belong to the same supersession scope
 */
static bool _scene_request_ids_share_freshness_scope(uint64_t lhs_request_id, uint64_t rhs_request_id)
{
    if (lhs_request_id == 0 || rhs_request_id == 0)
        return lhs_request_id == 0 && rhs_request_id == 0;
    return lhs_request_id == rhs_request_id;
}



/**
 * Find or allocate one panel-local request freshness scope entry.
 *
 * When the scope table is full, the least-recently touched entry is recycled. Freshness serials are
 * monotonically increasing, so `touched_serial` also provides a stable eviction order.
 *
 * @param scopes the scope table
 * @param scope_count the number of initialized entries
 * @param panel the owning panel
 * @param request_id the panel-local request id
 * @param freshness_serial the serial touching the scope
 * @return the updated scope entry
 */
static DvzRequestFreshnessScope* _scene_touch_request_scope(
    DvzRequestFreshnessScope* scopes, uint32_t* scope_count, DvzPanel* panel, uint64_t request_id,
    uint64_t freshness_serial)
{
    ANN(scopes);
    ANN(scope_count);
    ANN(panel);
    ASSERT(freshness_serial != 0);

    for (uint32_t i = 0; i < *scope_count; i++)
    {
        if (scopes[i].panel != panel)
            continue;
        if (!_scene_request_ids_share_freshness_scope(scopes[i].request_id, request_id))
            continue;
        scopes[i].request_id = request_id;
        scopes[i].freshness_serial = freshness_serial;
        scopes[i].touched_serial = freshness_serial;
        return &scopes[i];
    }

    if (*scope_count < DVZ_SCENE_MAX_REQUEST_SCOPES)
    {
        DvzRequestFreshnessScope* scope = &scopes[*scope_count];
        (*scope_count)++;
        scope->panel = panel;
        scope->request_id = request_id;
        scope->freshness_serial = freshness_serial;
        scope->touched_serial = freshness_serial;
        return scope;
    }

    uint32_t oldest_index = 0;
    uint64_t oldest_touch = scopes[0].touched_serial;
    for (uint32_t i = 1; i < DVZ_SCENE_MAX_REQUEST_SCOPES; i++)
    {
        if (scopes[i].touched_serial >= oldest_touch)
            continue;
        oldest_index = i;
        oldest_touch = scopes[i].touched_serial;
    }

    DvzRequestFreshnessScope* scope = &scopes[oldest_index];
    scope->panel = panel;
    scope->request_id = request_id;
    scope->freshness_serial = freshness_serial;
    scope->touched_serial = freshness_serial;
    return scope;
}



/**
 * Return the newest known freshness serial for one panel-local request scope.
 *
 * @param scopes the scope table
 * @param scope_count the number of initialized entries
 * @param panel the owning panel
 * @param request_id the panel-local request id
 * @return the newest known serial, or 0 when the scope is unknown
 */
static uint64_t _scene_latest_request_scope_serial(
    const DvzRequestFreshnessScope* scopes, uint32_t scope_count, const DvzPanel* panel,
    uint64_t request_id)
{
    ANN(scopes);
    ANN(panel);
    for (uint32_t i = 0; i < scope_count; i++)
    {
        if (scopes[i].panel != panel)
            continue;
        if (!_scene_request_ids_share_freshness_scope(scopes[i].request_id, request_id))
            continue;
        return scopes[i].freshness_serial;
    }
    return 0;
}



/**
 * Return whether one queued pick request is superseded by a newer request.
 *
 * @param pending the queued request
 * @param panel the panel receiving the new request
 * @param request_id the new request id
 * @return true when the old request should be dropped
 */
static bool _scene_pick_request_supersedes(
    const DvzPendingPickRequest* pending, const DvzPanel* panel, uint64_t request_id)
{
    ANN(pending);
    if (pending->panel != panel)
        return false;
    return _scene_request_ids_share_freshness_scope(pending->request.request_id, request_id);
}



/**
 * Return whether one queued probe request is superseded by a newer request.
 *
 * @param pending the queued request
 * @param panel the panel receiving the new request
 * @param request_id the new request id
 * @return true when the old request should be dropped
 */
static bool _scene_probe_request_supersedes(
    const DvzPendingProbeRequest* pending, const DvzPanel* panel, uint64_t request_id)
{
    ANN(pending);
    if (pending->panel != panel)
        return false;
    return _scene_request_ids_share_freshness_scope(pending->request.request_id, request_id);
}



/**
 * Drop unresolved pick requests superseded by a newer panel request.
 *
 * @param scene the scene
 * @param panel the panel receiving the new request
 * @param request_id the new request id
 */
static void _scene_drop_superseded_pick_requests(
    DvzScene* scene, const DvzPanel* panel, uint64_t request_id)
{
    ANN(scene);
    ANN(panel);
    uint32_t old_count = scene->pending_pick_count;
    uint32_t write = 0;
    for (uint32_t read = 0; read < scene->pending_pick_count; read++)
    {
        DvzPendingPickRequest pending = scene->pending_picks[read];
        if (_scene_pick_request_supersedes(&pending, panel, request_id))
            continue;
        if (write != read)
            scene->pending_picks[write] = pending;
        write++;
    }
    for (uint32_t i = write; i < old_count; i++)
    {
        dvz_memset(
            &scene->pending_picks[i], sizeof(DvzPendingPickRequest), 0,
            sizeof(DvzPendingPickRequest));
    }
    scene->pending_pick_count = write;
}



/**
 * Drop unresolved probe requests superseded by a newer panel request.
 *
 * @param scene the scene
 * @param panel the panel receiving the new request
 * @param request_id the new request id
 */
static void _scene_drop_superseded_probe_requests(
    DvzScene* scene, const DvzPanel* panel, uint64_t request_id)
{
    ANN(scene);
    ANN(panel);
    uint32_t old_count = scene->pending_probe_count;
    uint32_t write = 0;
    for (uint32_t read = 0; read < scene->pending_probe_count; read++)
    {
        DvzPendingProbeRequest pending = scene->pending_probes[read];
        if (_scene_probe_request_supersedes(&pending, panel, request_id))
            continue;
        if (write != read)
            scene->pending_probes[write] = pending;
        write++;
    }
    for (uint32_t i = write; i < old_count; i++)
    {
        dvz_memset(
            &scene->pending_probes[i], sizeof(DvzPendingProbeRequest), 0,
            sizeof(DvzPendingProbeRequest));
    }
    scene->pending_probe_count = write;
}



/**
 * Drop queued pick results superseded by a newer panel request.
 *
 * @param scene the scene
 * @param panel the panel receiving the new request
 * @param request_id the new request id
 */
static void _scene_drop_superseded_pick_results(
    DvzScene* scene, const DvzPanel* panel, uint64_t request_id)
{
    ANN(scene);
    ANN(panel);
    DvzQueuedPickResult kept[DVZ_SCENE_MAX_PICK_RESULTS] = {0};
    uint32_t kept_count = 0;
    for (uint32_t i = 0; i < scene->pick_result_count; i++)
    {
        uint32_t index = (scene->pick_result_head + i) % DVZ_SCENE_MAX_PICK_RESULTS;
        DvzQueuedPickResult queued = scene->pick_results[index];
        if (queued.panel == panel &&
            _scene_request_ids_share_freshness_scope(queued.result.request_id, request_id))
        {
            continue;
        }
        kept[kept_count++] = queued;
    }
    dvz_memset(scene->pick_results, sizeof(scene->pick_results), 0, sizeof(scene->pick_results));
    for (uint32_t i = 0; i < kept_count; i++)
        scene->pick_results[i] = kept[i];
    scene->pick_result_head = 0;
    scene->pick_result_count = kept_count;
}



/**
 * Drop queued probe results superseded by a newer panel request.
 *
 * @param scene the scene
 * @param panel the panel receiving the new request
 * @param request_id the new request id
 */
static void _scene_drop_superseded_probe_results(
    DvzScene* scene, const DvzPanel* panel, uint64_t request_id)
{
    ANN(scene);
    ANN(panel);
    DvzQueuedProbeResult kept[DVZ_SCENE_MAX_PROBE_RESULTS] = {0};
    uint32_t kept_count = 0;
    for (uint32_t i = 0; i < scene->probe_result_count; i++)
    {
        uint32_t index = (scene->probe_result_head + i) % DVZ_SCENE_MAX_PROBE_RESULTS;
        DvzQueuedProbeResult queued = scene->probe_results[index];
        if (queued.panel == panel &&
            _scene_request_ids_share_freshness_scope(queued.result.request_id, request_id))
        {
            continue;
        }
        kept[kept_count++] = queued;
    }
    dvz_memset(
        scene->probe_results, sizeof(scene->probe_results), 0, sizeof(scene->probe_results));
    for (uint32_t i = 0; i < kept_count; i++)
        scene->probe_results[i] = kept[i];
    scene->probe_result_head = 0;
    scene->probe_result_count = kept_count;
}



/**
 * Allocate the next monotonically increasing request freshness serial.
 *
 * Serial 0 stays reserved as the "no freshness tracking" sentinel used by legacy synthetic tests.
 *
 * @param scene the owning scene
 * @return the next non-zero request freshness serial
 */
uint64_t _scene_next_request_serial(DvzScene* scene)
{
    ANN(scene);
    scene->next_request_serial++;
    if (scene->next_request_serial == 0)
        scene->next_request_serial++;
    return scene->next_request_serial;
}



/**
 * Record the newest pick freshness serial for one panel-local request scope.
 *
 * @param scene the owning scene
 * @param panel the panel
 * @param request_id the panel-local request id
 * @param freshness_serial the newest request serial
 */
void _scene_track_pick_request_serial(
    DvzScene* scene, DvzPanel* panel, uint64_t request_id, uint64_t freshness_serial)
{
    ANN(scene);
    ANN(panel);
    if (freshness_serial == 0)
        return;
    (void)_scene_touch_request_scope(
        scene->pick_scopes, &scene->pick_scope_count, panel, request_id, freshness_serial);
}



/**
 * Record the newest probe freshness serial for one panel-local request scope.
 *
 * @param scene the owning scene
 * @param panel the panel
 * @param request_id the panel-local request id
 * @param freshness_serial the newest request serial
 */
void _scene_track_probe_request_serial(
    DvzScene* scene, DvzPanel* panel, uint64_t request_id, uint64_t freshness_serial)
{
    ANN(scene);
    ANN(panel);
    if (freshness_serial == 0)
        return;
    (void)_scene_touch_request_scope(
        scene->probe_scopes, &scene->probe_scope_count, panel, request_id, freshness_serial);
}



/**
 * Return whether one pick request/result scope is still current.
 *
 * @param scene the scene
 * @param panel the panel
 * @param request_id the request id
 * @param freshness_serial the request freshness serial
 * @return true when the serial still matches the newest known panel-local request scope
 */
bool _scene_pick_request_is_current(
    const DvzScene* scene, const DvzPanel* panel, uint64_t request_id, uint64_t freshness_serial)
{
    ANN(scene);
    ANN(panel);
    if (freshness_serial == 0)
        return true;
    uint64_t latest_serial = _scene_latest_request_scope_serial(
        scene->pick_scopes, scene->pick_scope_count, panel, request_id);
    return latest_serial == 0 || latest_serial == freshness_serial;
}



/**
 * Return whether one probe request/result scope is still current.
 *
 * @param scene the scene
 * @param panel the panel
 * @param request_id the request id
 * @param freshness_serial the request freshness serial
 * @return true when the serial still matches the newest known panel-local request scope
 */
bool _scene_probe_request_is_current(
    const DvzScene* scene, const DvzPanel* panel, uint64_t request_id, uint64_t freshness_serial)
{
    ANN(scene);
    ANN(panel);
    if (freshness_serial == 0)
        return true;
    uint64_t latest_serial = _scene_latest_request_scope_serial(
        scene->probe_scopes, scene->probe_scope_count, panel, request_id);
    return latest_serial == 0 || latest_serial == freshness_serial;
}



/**
 * Normalize optional emit inputs to concrete stack-backed defaults.
 *
 * @param caps the optional capabilities pointer to normalize
 * @param default_caps the stack storage for default capabilities
 * @param report the optional diagnostic report pointer to normalize
 * @param local_report the stack storage for a local report
 * @param cfg the optional emit config pointer to normalize
 * @param default_cfg the stack storage for the default emit config
 */
static void _scene_emit_defaults(
    const DvzCapabilitySnapshot** caps, DvzCapabilitySnapshot* default_caps,
    DvzDiagnosticReport** report, DvzDiagnosticReport* local_report,
    const DvzFramePlanEmitConfig** cfg, DvzFramePlanEmitConfig* default_cfg)
{
    ANN(caps);
    ANN(default_caps);
    ANN(report);
    ANN(local_report);
    ANN(cfg);
    ANN(default_cfg);
    if (*caps == NULL)
    {
        dvz_capability_snapshot_default(default_caps);
        *caps = default_caps;
    }
    *default_cfg = dvz_frame_plan_emit_config();
    if (*cfg == NULL)
        *cfg = default_cfg;
    if (*report == NULL)
    {
        dvz_diagnostic_report_init(local_report);
        *report = local_report;
    }
}



/**
 * Return whether one visual can contribute a drawable panel item.
 *
 * @param visual the visual
 * @return whether the visual is visible and has position data
 */
static bool _scene_emit_visual_drawable(const DvzVisual* visual)
{
    if (visual == NULL || !visual->visible || visual->type == DVZ_VISUAL_TYPE_TEXT)
        return false;
    const char* position_attr =
        visual->type == DVZ_VISUAL_TYPE_SEGMENT ? "position_start" : "position";
    int pos_idx = _attr_index(visual, position_attr);
    return pos_idx >= 0 && visual->attrs[pos_idx].item_count > 0;
}



/**
 * Validate per-panel transparency mode combinations before FramePlan emission.
 *
 * @param figure the figure
 * @param figure_id the stable emitted figure identifier
 * @param report the diagnostic report
 * @return whether transparency mode combinations are supported
 */
static bool _scene_figure_validate_transparency_modes(
    const DvzFigure* figure, const char* figure_id, DvzDiagnosticReport* report)
{
    ANN(figure);
    ANN(figure_id);
    bool ok = true;
    char panel_id[DVZ_SCENE_LABEL_SIZE];
    char message[DVZ_SCENE_DIAGNOSTIC_SIZE];
    for (uint32_t pi = 0; pi < figure->panel_count; pi++)
    {
        const DvzPanel* panel = &figure->panels[pi];
        bool has_wboit = false;
        bool has_depth_peel = false;
        uint32_t wboit_visual = UINT32_MAX;
        uint32_t depth_peel_visual = UINT32_MAX;
        for (uint32_t vi = 0; vi < panel->visual_count; vi++)
        {
            const DvzVisual* visual = panel->visuals[vi].visual;
            if (!_scene_emit_visual_drawable(visual))
                continue;
            uint32_t visual_index = UINT32_MAX;
            (void)_figure_visual_index(figure, visual, &visual_index);
            if (_scene_alpha_mode_is_wboit(visual->alpha_mode))
            {
                has_wboit = true;
                if (wboit_visual == UINT32_MAX)
                    wboit_visual = visual_index;
            }
            if (_scene_alpha_mode_is_depth_peel(visual->alpha_mode))
            {
                has_depth_peel = true;
                if (depth_peel_visual == UINT32_MAX)
                    depth_peel_visual = visual_index;
            }
        }
        if (has_wboit && has_depth_peel)
        {
            dvz_snprintf(panel_id, sizeof(panel_id), "%s_p%u", figure_id, pi);
            dvz_snprintf(
                message, sizeof(message),
                "panel %s mixes WBOIT visual %u and depth-peel visual %u; mixed OIT "
                "composition is not specified",
                panel_id, wboit_visual, depth_peel_visual);
            (void)dvz_diagnostic_report_add(report, message);
            ok = false;
        }
    }
    return ok;
}



/**
 * Clamp a requested sample count to a supported power-of-two sample count.
 *
 * @param sample_count requested sample count
 * @param max_sample_count maximum supported sample count
 * @return supported sample count
 */
static uint32_t _scene_lowered_sample_count(uint32_t sample_count, uint32_t max_sample_count)
{
    if (sample_count <= 1 || max_sample_count <= 1)
        return 1;
    if (sample_count >= 16 && max_sample_count >= 16)
        return 16;
    if (sample_count >= 8 && max_sample_count >= 8)
        return 8;
    if (sample_count >= 4 && max_sample_count >= 4)
        return 4;
    if (sample_count >= 2 && max_sample_count >= 2)
        return 2;
    return 1;
}



/**
 * Return the sample-count limit for one graph resource.
 *
 * @param resource the graph resource
 * @param caps the active capability snapshot
 * @return maximum supported sample count for the resource
 */
static uint32_t _scene_resource_sample_limit(
    const DvzFrameGraphResource* resource, const DvzCapabilitySnapshot* caps)
{
    ANN(resource);
    ANN(caps);
    uint32_t max_sample_count = 16;
    bool color = (resource->usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT) != 0;
    bool depth = (resource->usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_DEPTH_ATTACHMENT) != 0;
    if (color || depth)
    {
        uint32_t color_max = caps->max_color_sample_count != 0 ? caps->max_color_sample_count : 1;
        uint32_t depth_max = caps->max_depth_sample_count != 0 ? caps->max_depth_sample_count : 1;
        max_sample_count = color_max < depth_max ? color_max : depth_max;
    }
    return max_sample_count != 0 ? max_sample_count : 1;
}



/**
 * Report capability fallbacks that the runtime emitter will apply.
 *
 * @param plan the emitted FramePlan
 * @param caps the active capability snapshot
 * @param report optional diagnostic report
 */
static void _scene_report_capability_fallbacks(
    const DvzFramePlan* plan, const DvzCapabilitySnapshot* caps, DvzDiagnosticReport* report)
{
    if (plan == NULL || caps == NULL || report == NULL)
        return;

    for (uint32_t i = 0; i < dvz_frame_plan_graph_resource_count(plan); i++)
    {
        const DvzFrameGraphResource* resource = dvz_frame_plan_graph_resource_get(plan, i);
        if (resource == NULL)
            continue;
        uint32_t sample_count = resource->sample_count != 0 ? resource->sample_count : 1;
        if (sample_count <= 1)
            continue;
        uint32_t max_sample_count = _scene_resource_sample_limit(resource, caps);
        uint32_t lowered = _scene_lowered_sample_count(sample_count, max_sample_count);
        if (lowered >= sample_count)
            continue;

        char message[DVZ_SCENE_DIAGNOSTIC_SIZE];
        int ret = dvz_snprintf(
            message, sizeof(message),
            "scene capability fallback: graph resource '%s' sample count lowered from %u to %u",
            resource->id, sample_count, lowered);
        if (ret >= 0 && (size_t)ret < sizeof(message))
            (void)dvz_diagnostic_report_add(report, message);
        else
            (void)dvz_diagnostic_report_add(
                report, "scene capability fallback: sample count lowered");
    }
}



/**
 * Return whether one visual carries dirty state that should trigger a new emitted frame.
 *
 * @param visual the visual to inspect
 * @return whether rendering work is pending for this visual
 */
static bool _scene_visual_has_pending_render_work(const DvzVisual* visual)
{
    if (visual == NULL || !visual->visible)
        return false;

    if (_scene_visual_dirty_material_emits_upload(visual) || visual->texture.dirty)
        return true;
    if (
        visual->type == DVZ_VISUAL_TYPE_VOLUME &&
        visual->volume_realized_version != visual->volume.version)
    {
        return true;
    }
    if (visual->field != NULL && visual->field->dirty)
        return true;
    if (visual->buffer != NULL && visual->buffer->dirty)
        return true;
    if (visual->segment.gpu.dirty || visual->path.gpu.dirty || visual->image_gpu.dirty)
        return true;

    for (uint32_t i = 0; i < visual->attr_count; i++)
    {
        const DvzVisualAttr* attr = &visual->attrs[i];
        if (attr->dirty_item_count > 0)
            return true;
        if (attr->buffer != NULL && attr->buffer->dirty)
            return true;
    }
    return false;
}



/**
 * Return whether a dirty material payload is emitted for one visual family.
 *
 * @param visual the visual to inspect
 * @return whether material_params_dirty should trigger app rendering
 */
static bool _scene_visual_dirty_material_emits_upload(const DvzVisual* visual)
{
    if (visual == NULL || !visual->material_params_dirty)
        return false;

    switch (visual->type)
    {
    case DVZ_VISUAL_TYPE_POINT:
    case DVZ_VISUAL_TYPE_PIXEL:
    case DVZ_VISUAL_TYPE_MARKER:
    case DVZ_VISUAL_TYPE_SEGMENT:
    case DVZ_VISUAL_TYPE_PATH:
    case DVZ_VISUAL_TYPE_PRIMITIVE:
    case DVZ_VISUAL_TYPE_MESH:
    case DVZ_VISUAL_TYPE_SPHERE:
        return true;
    default:
        return false;
    }
}



/**
 * Return whether panel-owned adornments need realization before the next emitted frame.
 *
 * @param panel the panel to inspect
 * @return whether axis, text, or annotation work is pending
 */
static bool _scene_panel_has_pending_adornment_work(const DvzPanel* panel)
{
    if (panel == NULL)
        return false;

    for (uint32_t dim = 0; dim < 2; dim++)
    {
        const DvzAxis* axis = &panel->axes[dim];
        if (axis->panel == panel && axis->dirty)
            return true;
    }

    const DvzScene* scene = panel->figure != NULL ? panel->figure->scene : NULL;
    if (scene == NULL)
        return false;

    for (uint32_t i = 0; i < scene->annotation_count; i++)
    {
        const DvzAnnotation* annotation = &scene->annotations[i];
        if (annotation->panel == panel && annotation->dirty_flags != DVZ_TEXT_DIRTY_NONE)
            return true;
    }
    return false;
}



/**
 * Return whether one panel has visible dirty visuals attached.
 *
 * @param panel the panel to inspect
 * @return whether render work is pending for attached visuals
 */
static bool _scene_panel_has_pending_visual_work(const DvzPanel* panel)
{
    if (panel == NULL)
        return false;
    for (uint32_t i = 0; i < panel->visual_count; i++)
    {
        const DvzVisual* visual = panel->visuals[i].visual;
        if (_scene_visual_has_pending_render_work(visual))
            return true;
        if (visual != NULL && visual->type == DVZ_VISUAL_TYPE_TEXT)
        {
            const DvzFigure* figure = panel->figure;
            uint64_t version = visual->text.strings_version + visual->text.renderer_version;
            for (uint32_t ai = 0; ai < visual->attr_count; ai++)
                version += visual->attrs[ai].version;
            if (
                visual->text.string_count > 0 && visual->text.strings != NULL &&
                (visual->text.realized_version != version ||
                 (figure != NULL &&
                  (visual->text.visual_figure_width != figure->width ||
                   visual->text.visual_figure_height != figure->height))))
            {
                return true;
            }
        }
    }
    return false;
}



/**
 * Clear dirty scene state after one successful figure emit.
 *
 * @param figure the emitted figure
 */
static void _scene_commit_emit_success(DvzFigure* figure)
{
    ANN(figure);
    ANN(figure->scene);
    for (uint32_t pi = 0; pi < figure->panel_count; pi++)
    {
        DvzPanel* panel = &figure->panels[pi];
        for (uint32_t vi = 0; vi < panel->visual_count; vi++)
        {
            DvzVisual* visual = panel->visuals[vi].visual;
            if (visual == NULL || !visual->visible)
                continue;
            for (uint32_t ai = 0; ai < visual->attr_count; ai++)
            {
                visual->attrs[ai].dirty_item_count = 0;
                if (visual->attrs[ai].buffer != NULL)
                    visual->attrs[ai].buffer->dirty = false;
            }
            if (visual->buffer != NULL)
                visual->buffer->dirty = false;
            if (
                visual->type == DVZ_VISUAL_TYPE_POINT || visual->type == DVZ_VISUAL_TYPE_PIXEL ||
                visual->type == DVZ_VISUAL_TYPE_MARKER ||
                visual->type == DVZ_VISUAL_TYPE_SEGMENT ||
                visual->type == DVZ_VISUAL_TYPE_PATH ||
                visual->type == DVZ_VISUAL_TYPE_PRIMITIVE ||
                visual->type == DVZ_VISUAL_TYPE_MESH ||
                visual->type == DVZ_VISUAL_TYPE_SPHERE)
            {
                int normal_idx = _attr_index(visual, "normal");
                bool has_normals =
                    normal_idx >= 0 && visual->attrs[normal_idx].data != NULL &&
                    visual->attrs[normal_idx].item_count > 0;
                bool point_like = visual->type == DVZ_VISUAL_TYPE_POINT ||
                                  visual->type == DVZ_VISUAL_TYPE_PIXEL ||
                                  visual->type == DVZ_VISUAL_TYPE_MARKER;
                if (point_like || has_normals || visual->type == DVZ_VISUAL_TYPE_SEGMENT ||
                    visual->type == DVZ_VISUAL_TYPE_PATH || visual->type == DVZ_VISUAL_TYPE_SPHERE)
                    visual->material_params_dirty = false;
            }
            if (visual->type == DVZ_VISUAL_TYPE_IMAGE || visual->type == DVZ_VISUAL_TYPE_VOLUME)
                _scene_visual_texture_mark_clean(visual);
            if (visual->type == DVZ_VISUAL_TYPE_VOLUME)
                visual->volume_realized_version = visual->volume.version;
        }
    }
    for (uint32_t i = 0; i < figure->scene->field_count; i++)
        _scene_refresh_field_dirty_state(figure->scene, &figure->scene->fields[i]);
}



/**
 * Return whether a figure has retained scene work waiting for another emitted frame.
 *
 * @param figure the figure to inspect
 * @return whether dirty retained state is pending for this figure
 */
bool _scene_figure_has_pending_render_work(const DvzFigure* figure)
{
    if (figure == NULL)
        return false;

    for (uint32_t i = 0; i < figure->panel_count; i++)
    {
        const DvzPanel* panel = &figure->panels[i];
        if (
            _scene_panel_has_pending_visual_work(panel) ||
            _scene_panel_has_pending_adornment_work(panel))
        {
            return true;
        }
    }
    return false;
}



static void _scene_stream_release(void* owner)
{
    DvzScene* scene = (DvzScene*)owner;
    if (scene == NULL)
        return;
    if (scene->outstanding_emitted_streams == 0)
    {
        log_error("scene emitted stream release underflow");
        return;
    }
    scene->outstanding_emitted_streams--;
}



static bool _scene_stream_register(DvzScene* scene, DvzDrp2CommandStream* stream)
{
    ANN(scene);
    ANN(stream);
    if (scene->outstanding_emitted_streams == UINT32_MAX)
    {
        log_error("scene emitted stream count overflow");
        return false;
    }
    scene->outstanding_emitted_streams++;
    stream->owner = scene;
    stream->owner_release = _scene_stream_release;
    stream->owner_released = false;
    return true;
}



static bool _scene_has_live_streams(const DvzScene* scene)
{
    return scene != NULL && scene->outstanding_emitted_streams > 0;
}



bool _scene_visual_mutation_allowed(const DvzScene* scene, const char* action)
{
    ANN(action);
    if (!_scene_has_live_streams(scene))
        return true;
    log_error(
        "cannot %s while an emitted stream is still live; destroy the stream first", action);
    return false;
}


/**
 * Copy optional public formatting state into retained scene storage.
 *
 * @param dst the destination format state
 * @param src the source descriptor, or NULL to clear the destination
 */
void _scene_format_state_copy(DvzSceneFormatState* dst, const DvzFormatDesc* src)
{
    ANN(dst);
    dvz_memset(dst, sizeof(DvzSceneFormatState), 0, sizeof(DvzSceneFormatState));
    if (src == NULL)
        return;
    dst->precision = src->precision;
    dst->scientific = src->scientific;
    dst->trim_trailing_zeros = src->trim_trailing_zeros;
    dst->show_unit = src->show_unit;
    if (src->unit != NULL)
        dvz_strlcpy(dst->unit, src->unit, sizeof(dst->unit));
    if (src->prefix != NULL)
        dvz_strlcpy(dst->prefix, src->prefix, sizeof(dst->prefix));
    if (src->suffix != NULL)
        dvz_strlcpy(dst->suffix, src->suffix, sizeof(dst->suffix));
}


/*************************************************************************************************/
/*  Scene                                                                                        */
/*************************************************************************************************/

DvzScene* dvz_scene(void)
{
    DvzScene* scene = (DvzScene*)dvz_calloc(1, sizeof(DvzScene));
    if (scene == NULL)
        return NULL;
    dvz_capability_snapshot_default(&scene->caps);
    _scene_technique_state_init(&scene->techniques);
    scene->clock.mode = DVZ_CLOCK_REALTIME;
    scene->clock.fps = 60.0;
    scene->emitter = dvz_frame_plan_emitter();
    if (scene->emitter == NULL)
    {
        dvz_free(scene);
        return NULL;
    }
    return scene;
}


void dvz_scene_set_capabilities(DvzScene* scene, const DvzCapabilitySnapshot* caps)
{
    ANN(scene);
    ANN(caps);
    dvz_capability_snapshot_copy(&scene->caps, caps);
}


void dvz_scene_destroy(DvzScene* scene)
{
    if (scene == NULL)
        return;
    if (!_scene_visual_mutation_allowed(scene, "destroy scene-owned visual data"))
        return;
    for (uint32_t i = 0; i < scene->visual_count; i++)
        _scene_visual_reset(&scene->visuals[i], true);
    for (uint32_t i = 0; i < scene->font_count; i++)
        _scene_font_release(&scene->fonts[i]);
    for (uint32_t i = 0; i < DVZ_SCENE_MAX_FIELDS; i++)
        _scene_field_reset(&scene->fields[i]);
    for (uint32_t i = 0; i < DVZ_SCENE_MAX_BUFFERS; i++)
        _scene_buffer_reset(&scene->buffers[i]);
    for (uint32_t i = 0; i < scene->selection_count; i++)
        scene->selections[i].scene = NULL;
    for (uint32_t i = 0; i < scene->interaction_count; i++)
        scene->interactions[i].scene = NULL;
    for (uint32_t i = 0; i < scene->link_channel_count; i++)
        scene->link_channels[i].scene = NULL;
    for (uint32_t i = 0; i < scene->pinned_readout_count; i++)
        scene->pinned_readouts[i].scene = NULL;
    if (scene->emitter != NULL)
    {
        dvz_frame_plan_emitter_destroy(scene->emitter);
        scene->emitter = NULL;
    }
    dvz_free(scene);
}



/*************************************************************************************************/
/*  Figure                                                                                       */
/*************************************************************************************************/

DvzFigure* dvz_figure(DvzScene* scene, uint32_t width, uint32_t height, uint32_t flags)
{
    ANN(scene);
    if (scene->figure_count >= DVZ_SCENE_MAX_FIGURES)
        return NULL;
    DvzFigure* fig = &scene->figures[scene->figure_count++];
    fig->scene  = scene;
    fig->width  = width;
    fig->height = height;
    fig->flags  = flags;
    return fig;
}




/**
 * Update a figure logical size.
 *
 * @param figure the figure
 * @param width width in logical pixels
 * @param height height in logical pixels
 */
void dvz_figure_resize(DvzFigure* figure, uint32_t width, uint32_t height)
{
    ANN(figure);
    figure->width = width;
    figure->height = height;
    for (uint32_t i = 0; i < figure->panel_count; i++)
    {
        DvzPanel* panel = &figure->panels[i];
        float panel_width = 0.0f;
        float panel_height = 0.0f;
        float panel_x = 0.0f;
        float panel_y = 0.0f;
        _scene_panel_pixel_rect(panel, &panel_x, &panel_y, &panel_width, &panel_height);
        if (panel->panzoom != NULL)
            dvz_panzoom_viewport(panel->panzoom, panel_x, panel_y, panel_width, panel_height);
        if (panel->arcball != NULL)
            dvz_arcball_resize(panel->arcball, panel_width, panel_height);
        if (panel->camera != NULL)
            dvz_camera_resize(panel->camera, panel_width, panel_height);
        if (panel->fly != NULL)
            dvz_fly_viewport(panel->fly, panel_x, panel_y, panel_width, panel_height);
        if (panel->turntable != NULL)
            dvz_turntable_viewport(
                panel->turntable, panel_x, panel_y, panel_width, panel_height);
    }
}



/**
 * Return a figure logical size.
 *
 * @param figure the figure
 * @param out_width output width in logical pixels, may be NULL
 * @param out_height output height in logical pixels, may be NULL
 */
void dvz_figure_size(const DvzFigure* figure, uint32_t* out_width, uint32_t* out_height)
{
    ANN(figure);
    if (out_width != NULL)
        *out_width = figure->width;
    if (out_height != NULL)
        *out_height = figure->height;
}




void dvz_figure_destroy(DvzFigure* figure)
{
    if (figure == NULL)
        return;
    /* Mark slot as empty */
    figure->scene = NULL;
}


DvzDrp2CommandStream* dvz_figure_emit_ex(
    DvzFigure* figure, const DvzCapabilitySnapshot* caps, DvzDiagnosticReport* report,
    const DvzFramePlanEmitConfig* cfg)
{
    ANN(figure);
    ANN(figure->scene);
    ANN(figure->scene->emitter);
    DvzFramePlanEmitter* emitter = figure->scene->emitter;

    DvzCapabilitySnapshot default_caps;
    DvzDiagnosticReport local_report;
    DvzFramePlanEmitConfig default_cfg;
    _scene_emit_defaults(&caps, &default_caps, &report, &local_report, &cfg, &default_cfg);

    char figure_id[64];
    _scene_figure_id(figure, figure_id, sizeof(figure_id));

    if (!_scene_figure_validate_transparency_modes(figure, figure_id, report))
        return NULL;

    DvzFramePlan* plan = dvz_frame_plan(figure_id, 0);
    if (plan == NULL)
        return NULL;

    _scene_emit_visual_uploads(figure, plan);

    bool panels_ok = true;
    uint32_t graph_report_start = dvz_diagnostic_report_count(report);
    for (uint32_t pi = 0; pi < figure->panel_count; pi++)
        panels_ok = _scene_emit_panel_render_ex(figure, pi, plan, figure_id, report) && panels_ok;
    if (!panels_ok)
    {
        if (dvz_diagnostic_report_count(report) == graph_report_start)
            (void)dvz_diagnostic_report_add(report, "scene FramePlan graph emission failed");
        dvz_frame_plan_destroy(plan);
        return NULL;
    }

    DvzDiagnosticReport contract_report;
    dvz_diagnostic_report_init(&contract_report);
    bool contracts_ok =
        _scene_frame_plan_contracts_validate_ex(figure, plan, caps, &contract_report);
    if (!contracts_ok)
    {
        for (uint32_t i = 0; i < dvz_diagnostic_report_count(&contract_report); i++)
        {
            const char* message = dvz_diagnostic_report_get(&contract_report, i);
            if (message != NULL)
            {
                log_error("scene render contract validation failed: %s", message);
                (void)dvz_diagnostic_report_add(report, message);
            }
        }
        dvz_frame_plan_destroy(plan);
        return NULL;
    }

    _scene_report_capability_fallbacks(plan, caps, report);

    DvzDrp2CommandStream* stream =
        dvz_frame_plan_emitter_emit_drp2(emitter, plan, caps, report, cfg);
    if (stream != NULL && !_scene_stream_register(figure->scene, stream))
    {
        dvz_drp2_stream_destroy(stream);
        stream = NULL;
    }

    if (stream != NULL)
        _scene_commit_emit_success(figure);

    dvz_frame_plan_destroy(plan);
    return stream;
}



DvzDrp2CommandStream* dvz_figure_emit(
    DvzFigure* figure, const DvzCapabilitySnapshot* caps, DvzDiagnosticReport* report)
{
    return dvz_figure_emit_ex(figure, caps, report, NULL);
}


/**
 * Advance fly controllers attached to a figure.
 *
 * @param figure the figure
 * @param dt elapsed time in seconds
 * @return whether any fly controller still needs animation frames
 */
bool _dvz_figure_fly_update(DvzFigure* figure, double dt)
{
    if (figure == NULL)
        return false;

    bool active = false;
    for (uint32_t i = 0; i < figure->panel_count; i++)
    {
        DvzFly* fly = figure->panels[i].fly;
        if (fly == NULL)
            continue;

        if (fly->key_forward || fly->key_backward || fly->key_left || fly->key_right ||
            fly->key_up || fly->key_down || fly->interacting || fly->pivot_marker_time_left > 0.0)
        {
            active = true;
        }
        dvz_fly_update(fly, dt);
    }
    return active;
}


/*************************************************************************************************/
/*  Panel                                                                                        */
/*************************************************************************************************/

/**
 * Return default panel MSAA options.
 *
 * @return MSAA descriptor with 4x samples and alpha-to-coverage enabled
 */
DvzMsaaDesc dvz_msaa_desc(void)
{
    return (DvzMsaaDesc){
        .enabled = true,
        .sample_count = 4,
        .alpha_to_coverage = true,
    };
}


DvzPanel* dvz_panel(DvzFigure* figure, DvzPanelDesc desc)
{
    ANN(figure);
    if (figure->panel_count >= DVZ_SCENE_MAX_PANELS)
        return NULL;
    DvzPanel* panel       = &figure->panels[figure->panel_count++];
    panel->figure         = figure;
    panel->desc           = desc;
    panel->layout_reserve = dvz_panel_layout_reserve();
    _scene_technique_state_init(&panel->techniques);
    panel->visual_count = 0;
    return panel;
}


/**
 * Return the default panel layout reservation.
 *
 * @return default panel layout reservation
 */
DvzPanelLayoutReserve dvz_panel_layout_reserve(void)
{
    return (DvzPanelLayoutReserve){0};
}


/**
 * Reserve visual-space room around one panel's plot area for future adornments.
 *
 * @param panel the panel
 * @param reserve reservation descriptor, or NULL for defaults
 * @return whether the reservation was accepted
 */
bool dvz_panel_set_layout_reserve(DvzPanel* panel, const DvzPanelLayoutReserve* reserve)
{
    if (panel == NULL)
        return false;
    DvzPanelLayoutReserve next = reserve != NULL ? *reserve : dvz_panel_layout_reserve();
    if (!_panel_layout_reserve_valid(&next))
        return false;
    panel->layout_reserve = next;
    for (uint32_t dim = 0; dim < 2; dim++)
    {
        DvzAxis* axis = &panel->axes[dim];
        if (axis->panel == NULL)
            continue;
        axis->tick_cache_valid = false;
        axis->dirty = true;
        axis->version++;
    }
    return true;
}


/**
 * Return one panel's layout reservation.
 *
 * @param panel the panel
 * @param out output reservation
 * @return whether the reservation was written
 */
bool dvz_panel_get_layout_reserve(DvzPanel* panel, DvzPanelLayoutReserve* out)
{
    if (panel == NULL || out == NULL)
        return false;
    *out = panel->layout_reserve;
    return true;
}


/**
 * Configure Eye-Dome Lighting for one panel.
 *
 * @param panel the panel
 * @param desc EDL descriptor, or NULL to disable
 * @return whether the panel EDL state was updated
 */
bool dvz_panel_set_edl(DvzPanel* panel, const DvzEdlDesc* desc)
{
    ANN(panel);
    return _scene_technique_state_set_edl(&panel->techniques, desc);
}


/**
 * Configure internal multisample antialiasing for one panel.
 *
 * @param panel the panel
 * @param desc MSAA descriptor, or NULL to disable
 * @return whether the panel MSAA state was updated
 */
bool dvz_panel_set_msaa(DvzPanel* panel, const DvzMsaaDesc* desc)
{
    ANN(panel);
    return _scene_technique_state_set_msaa(&panel->techniques, desc);
}


/**
 * Configure screen-space ambient occlusion for one panel.
 *
 * @param panel the panel
 * @param desc SSAO descriptor, or NULL to disable
 * @return whether the panel SSAO state was updated
 */
bool dvz_panel_set_ssao(DvzPanel* panel, const DvzSsaoDesc* desc)
{
    ANN(panel);
    return _scene_technique_state_set_ssao(&panel->techniques, desc);
}


/**
 * Configure generic screen-space scene occlusion for one panel.
 *
 * @param panel the panel
 * @param desc scene occlusion descriptor, or NULL to disable
 * @return 0 on success, -1 on validation error
 */
int dvz_panel_set_scene_occlusion(DvzPanel* panel, const DvzSceneOcclusionDesc* desc)
{
    ANN(panel);
    if (desc == NULL || !desc->enabled)
    {
        panel->scene_occlusion_enabled = false;
        dvz_memset(
            &panel->scene_occlusion, sizeof(DvzSceneOcclusionDesc), 0,
            sizeof(DvzSceneOcclusionDesc));
        return 0;
    }

    panel->scene_occlusion = *desc;
    panel->scene_occlusion.enabled = true;
    if (panel->scene_occlusion.soft_edge <= 0.0f)
        panel->scene_occlusion.soft_edge = 0.002f;
    if (panel->scene_occlusion.hidden_alpha < 0.0f)
        panel->scene_occlusion.hidden_alpha = 0.0f;
    if (panel->scene_occlusion.hidden_alpha > 1.0f)
        panel->scene_occlusion.hidden_alpha = 1.0f;
    panel->scene_occlusion_enabled = true;
    return 0;
}


/**
 * Configure a panel volume visual as the screen-space occluder for embedded visuals.
 *
 * @param panel the panel
 * @param volume the volume visual attached to the same panel, or NULL to disable
 * @param desc volume occlusion descriptor, or NULL to disable
 * @return 0 on success, -1 on validation error
 */
int dvz_panel_set_volume_occluder(
    DvzPanel* panel, DvzVisual* volume, const DvzVolumeOcclusionDesc* desc)
{
    ANN(panel);
    if (volume == NULL || desc == NULL || !desc->enabled)
    {
        panel->volume_occluder_visual = NULL;
        panel->volume_occlusion_enabled = false;
        dvz_memset(
            &panel->volume_occlusion, sizeof(DvzVolumeOcclusionDesc), 0,
            sizeof(DvzVolumeOcclusionDesc));
        return 0;
    }
    if (volume->type != DVZ_VISUAL_TYPE_VOLUME)
    {
        log_error("dvz_panel_set_volume_occluder requires a volume visual");
        return -1;
    }

    bool attached = false;
    for (uint32_t i = 0; i < panel->visual_count; i++)
        attached = attached || panel->visuals[i].visual == volume;
    if (!attached)
    {
        log_error("volume occluder must be attached to the panel");
        return -1;
    }

    panel->volume_occluder_visual = volume;
    panel->volume_occlusion = *desc;
    panel->volume_occlusion.enabled = true;
    if (panel->volume_occlusion.alpha_threshold <= 0.0f)
        panel->volume_occlusion.alpha_threshold = 0.08f;
    if (panel->volume_occlusion.fade_distance <= 0.0f)
        panel->volume_occlusion.fade_distance = 0.08f;
    if (panel->volume_occlusion.occluded_alpha <= 0.0f)
        panel->volume_occlusion.occluded_alpha = 0.20f;
    panel->volume_occlusion_enabled = true;
    return 0;
}



void dvz_panel_destroy(DvzPanel* panel)
{
    if (panel == NULL)
        return;
    if (panel->panzoom != NULL)
    {
        dvz_panzoom_destroy(panel->panzoom);
        panel->panzoom = NULL;
    }
    if (panel->arcball != NULL)
    {
        dvz_arcball_destroy(panel->arcball);
        panel->arcball = NULL;
    }
    if (panel->fly != NULL)
    {
        dvz_fly_destroy(panel->fly);
        panel->fly = NULL;
    }
    if (panel->turntable != NULL)
    {
        dvz_turntable_destroy(panel->turntable);
        panel->turntable = NULL;
    }
    if (panel->camera != NULL)
    {
        dvz_camera_destroy(panel->camera);
        panel->camera = NULL;
    }
    panel->figure       = NULL;
    panel->visual_count = 0;
    panel->colorbar_count = 0;
    panel->interaction = NULL;
    panel->pinned_readout_count = 0;
    dvz_memset(&panel->hover, sizeof(DvzHoverState), 0, sizeof(DvzHoverState));
}


void dvz_panel_set_panzoom(DvzPanel* panel, DvzInputRouter* router, int flags)
{
    ANN(panel);
    if (panel->panzoom != NULL)
        dvz_panzoom_destroy(panel->panzoom);
    float w = 0.0f;
    float h = 0.0f;
    float x = 0.0f;
    float y = 0.0f;
    _scene_panel_pixel_rect(panel, &x, &y, &w, &h);
    panel->panzoom = dvz_panzoom(w, h, flags);
    dvz_panzoom_viewport(panel->panzoom, x, y, w, h);
    if (router != NULL)
        dvz_panzoom_connect(panel->panzoom, router);
}


DvzPanzoom* dvz_panel_panzoom(DvzPanel* panel)
{
    ANN(panel);
    return panel->panzoom;
}


void dvz_panel_set_arcball(DvzPanel* panel, DvzInputRouter* router, int flags)
{
    ANN(panel);
    if (panel->arcball != NULL)
        dvz_arcball_destroy(panel->arcball);
    float w = 0.0f;
    float h = 0.0f;
    _scene_panel_pixel_size(panel, &w, &h);
    panel->arcball = dvz_arcball(w, h, flags);
    if (router != NULL)
        dvz_arcball_connect(panel->arcball, router);
}


/**
 * Return the arcball controller attached to a panel.
 *
 * @param panel the panel
 * @return the panel-owned arcball, or NULL
 */
DvzArcball* dvz_panel_arcball(DvzPanel* panel)
{
    ANN(panel);
    return panel->arcball;
}


/**
 * Set or replace the camera attached to a panel.
 *
 * @param panel the panel
 * @param desc the camera descriptor, or NULL for defaults
 * @return the panel-owned camera
 */
DvzCamera* dvz_panel_set_camera(DvzPanel* panel, const DvzCameraDesc* desc)
{
    ANN(panel);
    if (panel->camera != NULL)
        dvz_camera_destroy(panel->camera);
    panel->camera = _dvz_camera(desc);
    if (panel->camera != NULL)
    {
        float w = 0.0f;
        float h = 0.0f;
        _scene_panel_pixel_size(panel, &w, &h);
        dvz_camera_resize(panel->camera, w, h);
    }
    return panel->camera;
}


/**
 * Return the camera attached to a panel.
 *
 * @param panel the panel
 * @return the panel-owned camera, or NULL
 */
DvzCamera* dvz_panel_camera(DvzPanel* panel)
{
    ANN(panel);
    return panel->camera;
}


/**
 * Attach a fly camera controller to a panel and connect it to an input router.
 *
 * @param panel the panel
 * @param router input router to subscribe to
 * @param desc fly descriptor, or NULL for defaults
 * @return the panel-owned fly controller
 */
DvzFly* dvz_panel_set_fly(DvzPanel* panel, DvzInputRouter* router, const DvzFlyDesc* desc)
{
    ANN(panel);
    if (panel->fly != NULL)
        dvz_fly_destroy(panel->fly);
    if (panel->camera == NULL)
    {
        DvzCameraDesc camera_desc = dvz_camera_desc();
        panel->camera = _dvz_camera(&camera_desc);
    }
    if (panel->camera == NULL)
        return NULL;

    panel->fly = dvz_fly(desc);
    if (panel->fly == NULL)
        return NULL;

    float w = 0.0f;
    float h = 0.0f;
    float x = 0.0f;
    float y = 0.0f;
    _scene_panel_pixel_rect(panel, &x, &y, &w, &h);
    dvz_fly_viewport(panel->fly, x, y, w, h);
    dvz_fly_set_camera(panel->fly, panel->camera);
    dvz_camera_resize(panel->camera, w, h);
    if (router != NULL)
        dvz_fly_connect(panel->fly, router);
    return panel->fly;
}


/**
 * Return the fly controller attached to a panel.
 *
 * @param panel the panel
 * @return the panel-owned fly controller, or NULL
 */
DvzFly* dvz_panel_fly(DvzPanel* panel)
{
    ANN(panel);
    return panel->fly;
}


/**
 * Attach a turntable camera controller to a panel and connect it to an input router.
 *
 * @param panel the panel
 * @param router input router to subscribe to
 * @param desc turntable descriptor, or NULL for defaults
 * @return the panel-owned turntable controller
 */
DvzTurntable* dvz_panel_set_turntable(
    DvzPanel* panel, DvzInputRouter* router, const DvzTurntableDesc* desc)
{
    ANN(panel);
    if (panel->turntable != NULL)
        dvz_turntable_destroy(panel->turntable);
    if (panel->camera == NULL)
    {
        DvzCameraDesc camera_desc = dvz_camera_desc();
        panel->camera = _dvz_camera(&camera_desc);
    }
    if (panel->camera == NULL)
        return NULL;

    panel->turntable = dvz_turntable(desc);
    if (panel->turntable == NULL)
        return NULL;

    float w = 0.0f;
    float h = 0.0f;
    float x = 0.0f;
    float y = 0.0f;
    _scene_panel_pixel_rect(panel, &x, &y, &w, &h);
    dvz_turntable_viewport(panel->turntable, x, y, w, h);
    dvz_turntable_set_camera(panel->turntable, panel->camera);
    dvz_camera_resize(panel->camera, w, h);
    if (router != NULL)
        dvz_turntable_connect(panel->turntable, router);
    return panel->turntable;
}


/**
 * Return the turntable controller attached to a panel.
 *
 * @param panel the panel
 * @return the panel-owned turntable controller, or NULL
 */
DvzTurntable* dvz_panel_turntable(DvzPanel* panel)
{
    ANN(panel);
    return panel->turntable;
}


/*************************************************************************************************/
/*  Pick / probe request queues                                                                  */
/*************************************************************************************************/


/**
 * Register one scene-level callback used to request a host frame.
 *
 * @param scene the scene
 * @param callback callback pointer
 * @param user_data opaque pointer forwarded to the callback
 * @return true on success, false when the subscription table is full or input is invalid
 */
bool _scene_add_request_frame_callback(
    DvzScene* scene, DvzSceneRequestFrameCallback callback, void* user_data)
{
    if (scene == NULL || callback == NULL)
        return false;

    DvzSceneRequestFrameSubscription* free_slot = NULL;
    for (uint32_t i = 0; i < DVZ_SCENE_MAX_REQUEST_FRAME_SUBSCRIPTIONS; i++)
    {
        DvzSceneRequestFrameSubscription* sub = &scene->request_frame_subscriptions[i];
        if (sub->active && sub->callback == callback && sub->user_data == user_data)
            return true;
        if (!sub->active && free_slot == NULL)
            free_slot = sub;
    }

    if (free_slot == NULL)
    {
        log_error("scene request-frame subscription table is full");
        return false;
    }

    free_slot->callback = callback;
    free_slot->user_data = user_data;
    free_slot->active = true;
    return true;
}


/**
 * Remove one scene-level host frame request callback.
 *
 * @param scene the scene
 * @param callback callback pointer
 * @param user_data opaque pointer previously registered with the callback
 */
void _scene_remove_request_frame_callback(
    DvzScene* scene, DvzSceneRequestFrameCallback callback, void* user_data)
{
    if (scene == NULL || callback == NULL)
        return;

    for (uint32_t i = 0; i < DVZ_SCENE_MAX_REQUEST_FRAME_SUBSCRIPTIONS; i++)
    {
        DvzSceneRequestFrameSubscription* sub = &scene->request_frame_subscriptions[i];
        if (sub->active && sub->callback == callback && sub->user_data == user_data)
        {
            dvz_memset(sub, sizeof(DvzSceneRequestFrameSubscription), 0,
                       sizeof(DvzSceneRequestFrameSubscription));
            return;
        }
    }
}


/**
 * Notify all scene hosts that one figure needs another frame.
 *
 * @param figure figure requesting a frame
 */
void _scene_notify_request_frame(DvzFigure* figure)
{
    if (figure == NULL || figure->scene == NULL)
        return;
    DvzScene* scene = figure->scene;
    for (uint32_t i = 0; i < DVZ_SCENE_MAX_REQUEST_FRAME_SUBSCRIPTIONS; i++)
    {
        const DvzSceneRequestFrameSubscription* sub = &scene->request_frame_subscriptions[i];
        DvzSceneRequestFrameCallback callback = sub->callback;
        void* user_data = sub->user_data;
        if (sub->active && callback != NULL)
            callback(figure, user_data);
    }
}


/**
 * Queue one explicit pick request on a panel.
 *
 * @param panel the panel
 * @param x the logical panel x coordinate
 * @param y the logical panel y coordinate
 * @param request the request descriptor, or NULL for defaults
 * @return 0 on success, -1 on error
 */
int dvz_panel_pick(DvzPanel* panel, double x, double y, const DvzPickRequest* request)
{
    ANN(panel);
    if (panel->figure == NULL || panel->figure->scene == NULL)
        return -1;
    DvzScene* scene = panel->figure->scene;
    uint64_t request_id = request != NULL ? request->request_id : 0;
    _scene_drop_superseded_pick_requests(scene, panel, request_id);
    _scene_drop_superseded_pick_results(scene, panel, request_id);
    if (scene->pending_pick_count >= DVZ_SCENE_MAX_PENDING_REQUESTS)
    {
        log_error("pick request queue is full");
        return -1;
    }
    DvzPendingPickRequest* pending = &scene->pending_picks[scene->pending_pick_count++];
    dvz_memset(pending, sizeof(DvzPendingPickRequest), 0, sizeof(DvzPendingPickRequest));
    pending->panel = panel;
    pending->x = x;
    pending->y = y;
    pending->freshness_serial = _scene_next_request_serial(scene);
    _scene_track_pick_request_serial(
        scene, panel, request_id, pending->freshness_serial);
    if (request != NULL)
        pending->request = *request;
    else if (panel->interaction != NULL)
        pending->request.hit_policy = panel->interaction->pick_hit_policy;
    _scene_notify_request_frame(panel->figure);
    return 0;
}


/**
 * Queue one explicit probe request on a panel.
 *
 * @param panel the panel
 * @param x the logical panel x coordinate
 * @param y the logical panel y coordinate
 * @param request the request descriptor, or NULL for defaults
 * @return 0 on success, -1 on error
 */
int dvz_panel_probe(DvzPanel* panel, double x, double y, const DvzProbeRequest* request)
{
    ANN(panel);
    if (panel->figure == NULL || panel->figure->scene == NULL)
        return -1;
    DvzScene* scene = panel->figure->scene;
    uint64_t request_id = request != NULL ? request->request_id : 0;
    _scene_drop_superseded_probe_requests(scene, panel, request_id);
    _scene_drop_superseded_probe_results(scene, panel, request_id);
    if (scene->pending_probe_count >= DVZ_SCENE_MAX_PENDING_REQUESTS)
    {
        log_error("probe request queue is full");
        return -1;
    }
    DvzPendingProbeRequest* pending = &scene->pending_probes[scene->pending_probe_count++];
    dvz_memset(pending, sizeof(DvzPendingProbeRequest), 0, sizeof(DvzPendingProbeRequest));
    pending->panel = panel;
    pending->x = x;
    pending->y = y;
    pending->freshness_serial = _scene_next_request_serial(scene);
    _scene_track_probe_request_serial(
        scene, panel, request_id, pending->freshness_serial);
    if (request != NULL)
        pending->request = *request;
    _scene_notify_request_frame(panel->figure);
    return 0;
}


/**
 * Poll one resolved pick result from the scene queue.
 *
 * @param scene the scene
 * @param out_result the destination result
 * @return true when a result was written
 */
bool dvz_scene_poll_pick(DvzScene* scene, DvzPickResult* out_result)
{
    ANN(scene);
    ANN(out_result);
    if (scene->pick_result_count == 0)
        return false;
    uint32_t index = scene->pick_result_head;
    *out_result = scene->pick_results[index].result;
    dvz_memset(
        &scene->pick_results[index], sizeof(DvzQueuedPickResult), 0,
        sizeof(DvzQueuedPickResult));
    scene->pick_result_head = (index + 1) % DVZ_SCENE_MAX_PICK_RESULTS;
    scene->pick_result_count--;
    return true;
}


/**
 * Poll one resolved probe result from the scene queue.
 *
 * @param scene the scene
 * @param out_result the destination result
 * @return true when a result was written
 */
bool dvz_scene_poll_probe(DvzScene* scene, DvzProbeResult* out_result)
{
    ANN(scene);
    ANN(out_result);
    if (scene->probe_result_count == 0)
        return false;
    uint32_t index = scene->probe_result_head;
    *out_result = scene->probe_results[index].result;
    dvz_memset(
        &scene->probe_results[index], sizeof(DvzQueuedProbeResult), 0,
        sizeof(DvzQueuedProbeResult));
    scene->probe_result_head = (index + 1) % DVZ_SCENE_MAX_PROBE_RESULTS;
    scene->probe_result_count--;
    return true;
}
