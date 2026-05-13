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

#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_json.h"
#include "_log.h"
#include "_overflow.h"
#include "_scene_emit.h"
#include "datoviz/drp2/runtime.h"
#include "datoviz/math/_cglm.h"
#include "../drp2/_stream.h"
#include "_scene.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static void _scene_stream_release(void* owner);

static bool _scene_stream_register(DvzScene* scene, DvzDrp2CommandStream* stream);

static bool _scene_has_live_streams(const DvzScene* scene);

bool _scene_visual_mutation_allowed(const DvzScene* scene, const char* action);

static void _format_state_copy(DvzSceneFormatState* dst, const DvzFormatDesc* src);

static void _scene_mark_scale_dirty(DvzScale* scale);

static void _scene_mark_colormap_dirty(DvzColormap* colormap);

bool _field_region_byte_size(
    DvzFieldFormat format, const DvzFieldRegion* region, uint64_t* out_size);

bool _scene_color_from_colormap(
    const DvzColormap* colormap, double t, uint8_t out_rgba[4]);

static bool _selection_matches_pick(
    const DvzSelection* selection, const DvzPickResult* pick, DvzSelectionItem* out_item);

static bool _selection_item_equals(const DvzSelectionItem* a, const DvzSelectionItem* b);

static bool _scene_remove_selection_item(DvzSelection* selection, const DvzSelectionItem* item);

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

void _scene_field_reset(DvzSampledField* field);

void _scene_buffer_reset(DvzSceneBuffer* buffer);

static void _json_append_field(
    JsonBuilder* b, const DvzScene* scene, uint32_t field_idx, bool* first);

static void _json_append_buffer(
    JsonBuilder* b, const DvzScene* scene, uint32_t buffer_idx, bool* first);

static void _json_append_visual(
    JsonBuilder* b, const DvzScene* scene, const DvzVisual* visual, bool* first);

static void _json_append_panel(
    JsonBuilder* b, const DvzScene* scene, uint32_t figure_idx, uint32_t panel_idx,
    const DvzPanel* panel, bool* first);

static void _json_append_figure(
    JsonBuilder* b, const DvzScene* scene, uint32_t figure_idx, bool* first);

static void _scene_emit_defaults(
    const DvzCapabilitySnapshot** caps, DvzCapabilitySnapshot* default_caps,
    DvzDiagnosticReport** report, DvzDiagnosticReport* local_report,
    const DvzFramePlanEmitConfig** cfg, DvzFramePlanEmitConfig* default_cfg);

static void _scene_commit_emit_success(DvzFigure* figure);

static bool _selection_matches_pick(
    const DvzSelection* selection, const DvzPickResult* pick, DvzSelectionItem* out_item)
{
    ANN(selection);
    ANN(pick);
    ANN(out_item);
    if (!pick->hit)
        return false;
    if (pick->resolved_target == DVZ_SCENE_TARGET_NONE || pick->resolved_id == 0)
        return false;
    if (selection->desc.target != DVZ_SCENE_TARGET_NONE &&
        selection->desc.target != pick->resolved_target)
        return false;
    out_item->visual_id = pick->visual_id;
    out_item->target = pick->resolved_target;
    out_item->target_id = pick->resolved_id;
    out_item->link_key = 0;
    return true;
}


static bool _selection_item_equals(const DvzSelectionItem* a, const DvzSelectionItem* b)
{
    ANN(a);
    ANN(b);
    return a->visual_id == b->visual_id && a->target == b->target && a->target_id == b->target_id &&
           a->link_key == b->link_key;
}


static bool _scene_remove_selection_item(DvzSelection* selection, const DvzSelectionItem* item)
{
    ANN(selection);
    ANN(item);
    for (uint32_t i = 0; i < selection->item_count; i++)
    {
        if (!_selection_item_equals(&selection->items[i], item))
            continue;
        for (uint32_t j = i + 1; j < selection->item_count; j++)
            selection->items[j - 1] = selection->items[j];
        selection->item_count--;
        dvz_memset(
            &selection->items[selection->item_count], sizeof(DvzSelectionItem), 0,
            sizeof(DvzSelectionItem));
        return true;
    }
    return false;
}


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
            if (visual == NULL)
                continue;
            for (uint32_t ai = 0; ai < visual->attr_count; ai++)
                visual->attrs[ai].dirty_item_count = 0;
            if (visual->type == DVZ_VISUAL_TYPE_PRIMITIVE ||
                visual->type == DVZ_VISUAL_TYPE_MESH)
            {
                int normal_idx = _attr_index(visual, "normal");
                bool has_normals =
                    normal_idx >= 0 && visual->attrs[normal_idx].data != NULL &&
                    visual->attrs[normal_idx].item_count > 0;
                if (has_normals)
                    visual->primitive_shading_dirty = false;
            }
            if (visual->type == DVZ_VISUAL_TYPE_IMAGE)
                _scene_visual_texture_mark_clean(visual);
        }
    }
    for (uint32_t i = 0; i < figure->scene->field_count; i++)
        _scene_refresh_field_dirty_state(figure->scene, &figure->scene->fields[i]);
    for (uint32_t i = 0; i < figure->scene->buffer_count; i++)
        figure->scene->buffers[i].dirty = false;
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


static void _format_state_copy(DvzSceneFormatState* dst, const DvzFormatDesc* src)
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


static void _scene_mark_scale_dirty(DvzScale* scale)
{
    if (scale == NULL || scale->scene == NULL)
        return;
    DvzScene* scene = scale->scene;
    for (uint32_t i = 0; i < scene->visual_count; i++)
    {
        DvzVisual* visual = &scene->visuals[i];
        if (visual->scene != scene || visual->scale != scale)
            continue;
        if (visual->type == DVZ_VISUAL_TYPE_IMAGE && visual->field != NULL &&
            _field_format_is_scalar(visual->field->desc.format))
        {
            _scene_visual_texture_mark_clean(visual);
            visual->texture.dirty = true;
        }
    }
}


static void _scene_mark_colormap_dirty(DvzColormap* colormap)
{
    if (colormap == NULL || colormap->scene == NULL)
        return;
    DvzScene* scene = colormap->scene;
    for (uint32_t i = 0; i < scene->scale_count; i++)
    {
        DvzScale* scale = &scene->scales[i];
        if (scale->scene == scene && scale->colormap == colormap)
            _scene_mark_scale_dirty(scale);
    }
}


bool _scene_color_from_colormap(
    const DvzColormap* colormap, double t, uint8_t out_rgba[4])
{
    ANN(out_rgba);
    if (t < 0.0)
        t = 0.0;
    if (t > 1.0)
        t = 1.0;

    if (colormap != NULL && colormap->stop_count >= 2)
    {
        const DvzColormapStop* lo = &colormap->stops[0];
        const DvzColormapStop* hi = &colormap->stops[colormap->stop_count - 1];
        for (uint32_t i = 1; i < colormap->stop_count; i++)
        {
            if (t <= colormap->stops[i].position)
            {
                lo = &colormap->stops[i - 1];
                hi = &colormap->stops[i];
                break;
            }
        }
        double span = hi->position - lo->position;
        double u = span > 0.0 ? (t - lo->position) / span : 0.0;
        if (u < 0.0)
            u = 0.0;
        if (u > 1.0)
            u = 1.0;
        for (uint32_t c = 0; c < 4; c++)
        {
            double value = (1.0 - u) * lo->rgba[c] + u * hi->rgba[c];
            out_rgba[c] = (uint8_t)(value + 0.5);
        }
        return true;
    }

    uint8_t gray = (uint8_t)(255.0 * t + 0.5);
    out_rgba[0] = gray;
    out_rgba[1] = gray;
    out_rgba[2] = gray;
    out_rgba[3] = 255;
    return true;
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
        _scene_panel_pixel_size(panel, &panel_width, &panel_height);
        if (panel->panzoom != NULL)
            dvz_panzoom_resize(panel->panzoom, panel_width, panel_height);
        if (panel->arcball != NULL)
            dvz_arcball_resize(panel->arcball, panel_width, panel_height);
        if (panel->camera != NULL)
            dvz_camera_resize(panel->camera, panel_width, panel_height);
    }
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

    char figure_id[64];
    _scene_figure_id(figure, figure_id, sizeof(figure_id));

    DvzFramePlan* plan = dvz_frame_plan(figure_id, 0);
    if (plan == NULL)
        return NULL;

    _scene_emit_visual_uploads(figure, plan);

    for (uint32_t pi = 0; pi < figure->panel_count; pi++)
        _scene_emit_panel_render(figure, pi, plan, figure_id);

    DvzCapabilitySnapshot default_caps;
    DvzDiagnosticReport local_report;
    DvzFramePlanEmitConfig default_cfg;
    _scene_emit_defaults(&caps, &default_caps, &report, &local_report, &cfg, &default_cfg);

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


/*************************************************************************************************/
/*  Panel                                                                                        */
/*************************************************************************************************/

DvzPanel* dvz_panel(DvzFigure* figure, DvzPanelDesc desc)
{
    ANN(figure);
    if (figure->panel_count >= DVZ_SCENE_MAX_PANELS)
        return NULL;
    DvzPanel* panel     = &figure->panels[figure->panel_count++];
    panel->figure       = figure;
    panel->desc         = desc;
    panel->visual_count = 0;
    return panel;
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
    _scene_panel_pixel_size(panel, &w, &h);
    panel->panzoom = dvz_panzoom(w, h, flags);
    if (router != NULL)
        dvz_panzoom_connect(panel->panzoom, router);
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


/*************************************************************************************************/
/*  Interaction / selection / readout                                                           */
/*************************************************************************************************/

/**
 * Create a scene-owned interaction policy object.
 *
 * @param scene the scene
 * @return the interaction policy, or NULL on allocation failure
 */
DvzInteractionPolicy* dvz_interaction(DvzScene* scene)
{
    ANN(scene);
    if (scene->interaction_count >= DVZ_SCENE_MAX_INTERACTIONS)
    {
        log_error("maximum interaction policy count reached");
        return NULL;
    }
    DvzInteractionPolicy* interaction = &scene->interactions[scene->interaction_count++];
    dvz_memset(interaction, sizeof(DvzInteractionPolicy), 0, sizeof(DvzInteractionPolicy));
    interaction->scene = scene;
    interaction->pick_hit_policy = DVZ_PICK_HIT_FRONTMOST;
    return interaction;
}


/**
 * Destroy a scene-owned interaction policy object.
 *
 * @param interaction the interaction policy
 */
void dvz_interaction_destroy(DvzInteractionPolicy* interaction)
{
    if (interaction == NULL)
        return;
    if (interaction->panel != NULL && interaction->panel->interaction == interaction)
        interaction->panel->interaction = NULL;
    interaction->scene = NULL;
    interaction->panel = NULL;
    interaction->selection = NULL;
    interaction->link_channel = NULL;
    interaction->auto_pin_readout = false;
}


/**
 * Bind an interaction policy to a panel.
 *
 * @param interaction the interaction policy
 * @param panel the panel
 */
void dvz_interaction_bind_panel(DvzInteractionPolicy* interaction, DvzPanel* panel)
{
    ANN(interaction);
    ANN(panel);
    if (panel->figure == NULL || panel->figure->scene == NULL || interaction->scene != panel->figure->scene)
    {
        log_error("cannot bind an interaction policy across scenes");
        return;
    }
    if (interaction->panel != NULL && interaction->panel->interaction == interaction)
        interaction->panel->interaction = NULL;
    panel->interaction = interaction;
    interaction->panel = panel;
}


/**
 * Attach a retained selection object to an interaction policy.
 *
 * @param interaction the interaction policy
 * @param selection the selection
 */
void dvz_interaction_set_selection(DvzInteractionPolicy* interaction, DvzSelection* selection)
{
    ANN(interaction);
    if (selection != NULL && selection->scene != interaction->scene)
    {
        log_error("cannot bind a selection from a different scene");
        return;
    }
    interaction->selection = selection;
}


/**
 * Set the active link channel used by an interaction policy.
 *
 * @param interaction the interaction policy
 * @param channel the link channel
 */
void dvz_interaction_set_link_channel(
    DvzInteractionPolicy* interaction, DvzLinkChannel* channel)
{
    ANN(interaction);
    if (channel != NULL && channel->scene != interaction->scene)
    {
        log_error("cannot bind a link channel from a different scene");
        return;
    }
    interaction->link_channel = channel;
}


/**
 * Set the hit-selection policy used for picking.
 *
 * @param interaction the interaction policy
 * @param policy the hit-selection policy
 */
void dvz_interaction_set_pick_hit_policy(
    DvzInteractionPolicy* interaction, DvzPickHitPolicy policy)
{
    ANN(interaction);
    interaction->pick_hit_policy = policy;
}


/**
 * Enable or disable automatic probe pinning.
 *
 * @param interaction the interaction policy
 * @param enabled whether auto pinning is enabled
 */
void dvz_interaction_set_auto_pin_readout(DvzInteractionPolicy* interaction, bool enabled)
{
    ANN(interaction);
    interaction->auto_pin_readout = enabled;
}


/**
 * Create a scene-owned link channel.
 *
 * @param scene the scene
 * @param name the stable channel name, or NULL
 * @return the link channel, or NULL on allocation failure
 */
DvzLinkChannel* dvz_link_channel(DvzScene* scene, const char* name)
{
    ANN(scene);
    if (scene->link_channel_count >= DVZ_SCENE_MAX_LINK_CHANNELS)
    {
        log_error("maximum link channel count reached");
        return NULL;
    }
    DvzLinkChannel* channel = &scene->link_channels[scene->link_channel_count++];
    dvz_memset(channel, sizeof(DvzLinkChannel), 0, sizeof(DvzLinkChannel));
    channel->scene = scene;
    if (name != NULL)
        dvz_strlcpy(channel->name, name, sizeof(channel->name));
    return channel;
}


/**
 * Destroy a scene-owned link channel.
 *
 * @param channel the link channel
 */
void dvz_link_channel_destroy(DvzLinkChannel* channel)
{
    if (channel == NULL)
        return;
    if (channel->scene != NULL)
    {
        DvzScene* scene = channel->scene;
        for (uint32_t i = 0; i < scene->visual_count; i++)
        {
            if (scene->visuals[i].link_channel == channel)
            {
                scene->visuals[i].link_channel = NULL;
                if (scene->visuals[i].link_keys != NULL)
                {
                    dvz_free(scene->visuals[i].link_keys);
                    scene->visuals[i].link_keys = NULL;
                }
                scene->visuals[i].link_key_count = 0;
            }
        }
        for (uint32_t i = 0; i < scene->interaction_count; i++)
        {
            if (scene->interactions[i].link_channel == channel)
                scene->interactions[i].link_channel = NULL;
        }
        for (uint32_t i = 0; i < scene->figure_count; i++)
        {
            DvzFigure* figure = &scene->figures[i];
            for (uint32_t j = 0; j < figure->panel_count; j++)
            {
                DvzPanel* panel = &figure->panels[j];
                if (panel->hover.link_channel == channel)
                    panel->hover.link_channel = NULL;
            }
        }
    }
    channel->scene = NULL;
}


/**
 * Create a retained scene-owned selection object.
 *
 * @param scene the scene
 * @param desc the selection descriptor, or NULL for defaults
 * @return the selection, or NULL on allocation failure
 */
DvzSelection* dvz_selection(DvzScene* scene, const DvzSelectionDesc* desc)
{
    ANN(scene);
    if (scene->selection_count >= DVZ_SCENE_MAX_SELECTIONS)
    {
        log_error("maximum selection count reached");
        return NULL;
    }
    DvzSelection* selection = &scene->selections[scene->selection_count++];
    dvz_memset(selection, sizeof(DvzSelection), 0, sizeof(DvzSelection));
    selection->scene = scene;
    if (desc != NULL)
        selection->desc = *desc;
    else
        selection->desc.mode = DVZ_SELECT_REPLACE;
    return selection;
}


/**
 * Destroy a retained selection object.
 *
 * @param selection the selection
 */
void dvz_selection_destroy(DvzSelection* selection)
{
    if (selection == NULL)
        return;
    if (selection->scene != NULL)
    {
        DvzScene* scene = selection->scene;
        for (uint32_t i = 0; i < scene->interaction_count; i++)
        {
            if (scene->interactions[i].selection == selection)
                scene->interactions[i].selection = NULL;
        }
    }
    selection->scene = NULL;
    selection->item_count = 0;
}


/**
 * Clear the contents of a selection object.
 *
 * @param selection the selection
 */
void dvz_selection_clear(DvzSelection* selection)
{
    ANN(selection);
    selection->item_count = 0;
    dvz_memset(selection->items, sizeof(selection->items), 0, sizeof(selection->items));
}


/**
 * Apply one resolved pick result to a selection.
 *
 * @param selection the selection
 * @param pick the pick result
 * @return 0 on success, -1 on error
 */
int dvz_selection_apply_pick(DvzSelection* selection, const DvzPickResult* pick)
{
    ANN(selection);
    ANN(pick);
    DvzSelectionItem item = {0};
    if (!_selection_matches_pick(selection, pick, &item))
        return -1;
    bool present = false;
    for (uint32_t i = 0; i < selection->item_count; i++)
    {
        if (_selection_item_equals(&selection->items[i], &item))
        {
            present = true;
            break;
        }
    }
    switch (selection->desc.mode)
    {
    case DVZ_SELECT_REPLACE:
        dvz_selection_clear(selection);
        selection->items[0] = item;
        selection->item_count = 1;
        return 0;
    case DVZ_SELECT_ADDITIVE:
        if (present)
            return 0;
        break;
    case DVZ_SELECT_SUBTRACT:
        if (present)
            _scene_remove_selection_item(selection, &item);
        return 0;
    case DVZ_SELECT_TOGGLE:
        if (present)
        {
            _scene_remove_selection_item(selection, &item);
            return 0;
        }
        break;
    default:
        break;
    }
    if (selection->item_count >= DVZ_SCENE_MAX_SELECTION_ITEMS)
    {
        log_error("selection item capacity reached");
        return -1;
    }
    selection->items[selection->item_count++] = item;
    return 0;
}


/**
 * Return the number of stored selection items.
 *
 * @param selection the selection
 * @return the item count
 */
uint32_t dvz_selection_count(const DvzSelection* selection)
{
    ANN(selection);
    return selection->item_count;
}


/**
 * Copy selection contents into caller-owned storage.
 *
 * @param selection the selection
 * @param items the destination item array
 * @param max_items the maximum number of items to write
 */
void dvz_selection_copy(
    const DvzSelection* selection, DvzSelectionItem* items, uint32_t max_items)
{
    ANN(selection);
    if (items == NULL || max_items == 0)
        return;
    uint32_t count = selection->item_count < max_items ? selection->item_count : max_items;
    dvz_memcpy(items, max_items * sizeof(DvzSelectionItem), selection->items, count * sizeof(DvzSelectionItem));
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
    *out_result = scene->pick_results[scene->pick_result_head].result;
    scene->pick_result_head = (scene->pick_result_head + 1) % DVZ_SCENE_MAX_PICK_RESULTS;
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
    *out_result = scene->probe_results[scene->probe_result_head].result;
    scene->probe_result_head = (scene->probe_result_head + 1) % DVZ_SCENE_MAX_PROBE_RESULTS;
    scene->probe_result_count--;
    return true;
}


/**
 * Return the retained hover state for one panel.
 *
 * @param scene the scene
 * @param panel the panel
 * @return the hover state, or NULL when the panel is foreign
 */
const DvzHoverState* dvz_scene_hover(const DvzScene* scene, const DvzPanel* panel)
{
    ANN(scene);
    ANN(panel);
    if (panel->figure == NULL || panel->figure->scene != scene)
        return NULL;
    return &panel->hover;
}


/**
 * Create a pinned readout from a resolved probe result.
 *
 * @param panel the panel
 * @param probe the probe result
 * @return the pinned readout, or NULL on allocation failure
 */
DvzPinnedReadout* dvz_pinned_readout(DvzPanel* panel, const DvzProbeResult* probe)
{
    ANN(panel);
    ANN(probe);
    if (panel->figure == NULL || panel->figure->scene == NULL)
        return NULL;
    DvzScene* scene = panel->figure->scene;
    if (scene->pinned_readout_count >= DVZ_SCENE_MAX_PINNED_READOUTS)
    {
        log_error("maximum pinned readout count reached");
        return NULL;
    }
    if (panel->pinned_readout_count >= DVZ_SCENE_MAX_PINNED_READOUTS)
    {
        log_error("maximum panel pinned readout count reached");
        return NULL;
    }
    DvzPinnedReadout* readout = &scene->pinned_readouts[scene->pinned_readout_count++];
    dvz_memset(readout, sizeof(DvzPinnedReadout), 0, sizeof(DvzPinnedReadout));
    readout->scene = scene;
    readout->panel = panel;
    readout->probe = *probe;
    panel->pinned_readouts[panel->pinned_readout_count++] = readout;
    return readout;
}


/**
 * Destroy a pinned readout object.
 *
 * @param readout the pinned readout
 */
void dvz_pinned_readout_destroy(DvzPinnedReadout* readout)
{
    if (readout == NULL)
        return;
    if (readout->panel != NULL)
    {
        DvzPanel* panel = readout->panel;
        for (uint32_t i = 0; i < panel->pinned_readout_count; i++)
        {
            if (panel->pinned_readouts[i] != readout)
                continue;
            for (uint32_t j = i + 1; j < panel->pinned_readout_count; j++)
                panel->pinned_readouts[j - 1] = panel->pinned_readouts[j];
            panel->pinned_readouts[panel->pinned_readout_count - 1] = NULL;
            panel->pinned_readout_count--;
            break;
        }
    }
    readout->scene = NULL;
    readout->panel = NULL;
    readout->has_format = false;
}


/**
 * Override formatting on a pinned readout.
 *
 * @param readout the pinned readout
 * @param format the format descriptor, or NULL to clear the override
 */
void dvz_pinned_readout_set_format(DvzPinnedReadout* readout, const DvzFormatDesc* format)
{
    ANN(readout);
    readout->has_format = format != NULL;
    _format_state_copy(&readout->format, format);
}


/*************************************************************************************************/
/*  Text / annotation                                                                           */
/*************************************************************************************************/

/**
 * Create a scene-owned font resource.
 *
 * @param scene the scene
 * @param desc the font descriptor
 * @return the font, or NULL on allocation failure
 */
DvzFont* dvz_font(DvzScene* scene, const DvzFontDesc* desc)
{
    ANN(scene);
    ANN(desc);
    if (scene->font_count >= DVZ_SCENE_MAX_FONTS)
    {
        log_error("maximum font count reached");
        return NULL;
    }
    DvzFont* font = &scene->fonts[scene->font_count++];
    dvz_memset(font, sizeof(DvzFont), 0, sizeof(DvzFont));
    font->scene = scene;
    font->size_pts = desc->size_pts;
    font->flags = desc->flags;
    if (desc->path != NULL)
        dvz_strlcpy(font->path, desc->path, sizeof(font->path));
    return font;
}


/**
 * Destroy a scene-owned font resource.
 *
 * @param font the font
 */
void dvz_font_destroy(DvzFont* font)
{
    if (font == NULL)
        return;
    font->scene = NULL;
}


/**
 * Create a retained text object attached to a panel.
 *
 * @param panel the panel
 * @param desc the text descriptor
 * @return the text object, or NULL on allocation failure
 */
DvzText* dvz_text(DvzPanel* panel, const DvzTextDesc* desc)
{
    ANN(panel);
    ANN(desc);
    if (panel->figure == NULL || panel->figure->scene == NULL)
        return NULL;
    DvzScene* scene = panel->figure->scene;
    if (scene->text_count >= DVZ_SCENE_MAX_TEXTS)
    {
        log_error("maximum text count reached");
        return NULL;
    }
    if (desc->style.font != NULL && desc->style.font->scene != scene)
    {
        log_error("cannot bind a font from a different scene");
        return NULL;
    }
    DvzText* text = &scene->texts[scene->text_count++];
    dvz_memset(text, sizeof(DvzText), 0, sizeof(DvzText));
    text->scene = scene;
    text->panel = panel;
    text->style = desc->style;
    text->placement = desc->placement;
    text->flags = desc->flags;
    if (desc->string != NULL)
        dvz_strlcpy(text->string, desc->string, sizeof(text->string));
    return text;
}


/**
 * Destroy a retained text object.
 *
 * @param text the text
 */
void dvz_text_destroy(DvzText* text)
{
    if (text == NULL)
        return;
    text->scene = NULL;
    text->panel = NULL;
}


/**
 * Update the content string on a retained text object.
 *
 * @param text the text
 * @param string the new string
 */
void dvz_text_set_string(DvzText* text, const char* string)
{
    ANN(text);
    text->string[0] = '\0';
    if (string != NULL)
        dvz_strlcpy(text->string, string, sizeof(text->string));
}


/**
 * Update the style on a retained text object.
 *
 * @param text the text
 * @param style the new style
 */
void dvz_text_set_style(DvzText* text, const DvzTextStyle* style)
{
    ANN(text);
    ANN(style);
    if (style->font != NULL && (text->scene == NULL || style->font->scene != text->scene))
    {
        log_error("cannot bind a font from a different scene");
        return;
    }
    text->style = *style;
}


/**
 * Update the placement on a retained text object.
 *
 * @param text the text
 * @param placement the new placement
 */
void dvz_text_set_placement(DvzText* text, const DvzTextPlacement* placement)
{
    ANN(text);
    ANN(placement);
    text->placement = *placement;
}


/**
 * Create a retained annotation object attached to a panel.
 *
 * @param panel the panel
 * @param desc the annotation descriptor
 * @return the annotation, or NULL on allocation failure
 */
DvzAnnotation* dvz_annotation(DvzPanel* panel, const DvzAnnotationDesc* desc)
{
    ANN(panel);
    ANN(desc);
    if (panel->figure == NULL || panel->figure->scene == NULL)
        return NULL;
    DvzScene* scene = panel->figure->scene;
    if (scene->annotation_count >= DVZ_SCENE_MAX_ANNOTATIONS)
    {
        log_error("maximum annotation count reached");
        return NULL;
    }
    if (desc->style.font != NULL && desc->style.font->scene != scene)
    {
        log_error("cannot bind a font from a different scene");
        return NULL;
    }
    DvzAnnotation* annotation = &scene->annotations[scene->annotation_count++];
    dvz_memset(annotation, sizeof(DvzAnnotation), 0, sizeof(DvzAnnotation));
    annotation->scene = scene;
    annotation->panel = panel;
    annotation->kind = desc->kind;
    annotation->style = desc->style;
    annotation->placement = desc->placement;
    annotation->flags = desc->flags;
    if (desc->text != NULL)
        dvz_strlcpy(annotation->text, desc->text, sizeof(annotation->text));
    return annotation;
}


/**
 * Create a retained label annotation.
 *
 * @param panel the panel
 * @param desc the label descriptor
 * @return the annotation, or NULL on allocation failure
 */
DvzAnnotation* dvz_annotation_label(DvzPanel* panel, const DvzLabelDesc* desc)
{
    ANN(desc);
    return dvz_annotation(
        panel, &(DvzAnnotationDesc){
                   .kind = DVZ_ANNOTATION_LABEL,
                   .text = desc->text,
                   .style = desc->style,
                   .placement = desc->placement,
                   .flags = desc->flags});
}


/**
 * Destroy a retained annotation object.
 *
 * @param annotation the annotation
 */
void dvz_annotation_destroy(DvzAnnotation* annotation)
{
    if (annotation == NULL)
        return;
    annotation->scene = NULL;
    annotation->panel = NULL;
    annotation->has_format = false;
}


/**
 * Override formatting policy on an annotation.
 *
 * @param annotation the annotation
 * @param format the format descriptor, or NULL to clear the override
 */
void dvz_annotation_set_format(DvzAnnotation* annotation, const DvzFormatDesc* format)
{
    ANN(annotation);
    annotation->has_format = format != NULL;
    _format_state_copy(&annotation->format, format);
}



/*************************************************************************************************/
/*  Scale / colormap / colorbar                                                                  */
/*************************************************************************************************/

/**
 * Create a scene-owned scale object.
 *
 * @param scene the scene
 * @param desc the scale descriptor, or NULL for defaults
 * @return the scale, or NULL on allocation failure
 */
DvzScale* dvz_scale(DvzScene* scene, const DvzScaleDesc* desc)
{
    ANN(scene);
    if (scene->scale_count >= DVZ_SCENE_MAX_SCALES)
    {
        log_error("maximum scale count reached");
        return NULL;
    }
    DvzScale* scale = &scene->scales[scene->scale_count++];
    dvz_memset(scale, sizeof(DvzScale), 0, sizeof(DvzScale));
    scale->scene = scene;
    scale->kind = desc != NULL ? desc->kind : DVZ_SCALE_CONTINUOUS;
    if (desc != NULL)
    {
        if (desc->label != NULL)
            dvz_strlcpy(scale->label, desc->label, sizeof(scale->label));
        if (desc->unit != NULL)
            dvz_strlcpy(scale->unit, desc->unit, sizeof(scale->unit));
        _format_state_copy(&scale->format, &desc->format);
    }
    return scale;
}


/**
 * Destroy a scale object.
 *
 * @param scale the scale
 */
void dvz_scale_destroy(DvzScale* scale)
{
    if (scale == NULL)
        return;
    scale->scene = NULL;
    scale->colormap = NULL;
    scale->has_domain = false;
    scale->has_view_range = false;
}


/**
 * Set the semantic domain on a scale.
 *
 * @param scale the scale
 * @param min the domain minimum
 * @param max the domain maximum
 */
void dvz_scale_set_domain(DvzScale* scale, double min, double max)
{
    ANN(scale);
    scale->domain_min = min;
    scale->domain_max = max;
    scale->has_domain = true;
    _scene_mark_scale_dirty(scale);
}


/**
 * Set the current visible range on a scale.
 *
 * @param scale the scale
 * @param min the view-range minimum
 * @param max the view-range maximum
 */
void dvz_scale_set_view_range(DvzScale* scale, double min, double max)
{
    ANN(scale);
    scale->view_min = min;
    scale->view_max = max;
    scale->has_view_range = true;
    _scene_mark_scale_dirty(scale);
}


/**
 * Bind a colormap to a scale.
 *
 * @param scale the scale
 * @param colormap the colormap
 */
void dvz_scale_set_colormap(DvzScale* scale, DvzColormap* colormap)
{
    ANN(scale);
    if (colormap != NULL && colormap->scene != scale->scene)
    {
        log_error("cannot bind a colormap from a different scene");
        return;
    }
    scale->colormap = colormap;
    _scene_mark_scale_dirty(scale);
}


/**
 * Override shared formatting policy on a scale.
 *
 * @param scale the scale
 * @param format the format descriptor, or NULL to clear the override
 */
void dvz_scale_set_format(DvzScale* scale, const DvzFormatDesc* format)
{
    ANN(scale);
    _format_state_copy(&scale->format, format);
    _scene_mark_scale_dirty(scale);
}


/**
 * Create a scene-owned colormap object.
 *
 * @param scene the scene
 * @param desc the colormap descriptor, or NULL for defaults
 * @return the colormap, or NULL on allocation failure
 */
DvzColormap* dvz_colormap(DvzScene* scene, const DvzColormapDesc* desc)
{
    ANN(scene);
    if (scene->colormap_count >= DVZ_SCENE_MAX_COLORMAPS)
    {
        log_error("maximum colormap count reached");
        return NULL;
    }
    DvzColormap* colormap = &scene->colormaps[scene->colormap_count++];
    dvz_memset(colormap, sizeof(DvzColormap), 0, sizeof(DvzColormap));
    colormap->scene = scene;
    colormap->kind = desc != NULL ? desc->kind : DVZ_COLORMAP_CONTINUOUS;
    colormap->builtin = desc != NULL ? desc->builtin : DVZ_BUILTIN_COLORMAP_NONE;
    if (desc != NULL)
    {
        colormap->center = desc->center;
        colormap->has_center = desc->center != 0.0;
        if (desc->label != NULL)
            dvz_strlcpy(colormap->label, desc->label, sizeof(colormap->label));
    }
    return colormap;
}


/**
 * Create a scene-owned built-in colormap object.
 *
 * @param scene the scene
 * @param builtin the built-in colormap selector
 * @return the colormap, or NULL on allocation failure
 */
DvzColormap* dvz_colormap_builtin(DvzScene* scene, DvzBuiltinColormap builtin)
{
    DvzColormapDesc desc = {
        .kind = DVZ_COLORMAP_CONTINUOUS,
        .builtin = builtin,
    };
    return dvz_colormap(scene, &desc);
}


/**
 * Destroy a colormap object.
 *
 * @param colormap the colormap
 */
void dvz_colormap_destroy(DvzColormap* colormap)
{
    if (colormap == NULL)
        return;
    colormap->scene = NULL;
    colormap->stop_count = 0;
    colormap->has_center = false;
}


/**
 * Set custom color stops on a colormap.
 *
 * @param colormap the colormap
 * @param stops the color stops
 * @param count the number of stops
 */
void dvz_colormap_set_stops(DvzColormap* colormap, const DvzColormapStop* stops, uint32_t count)
{
    ANN(colormap);
    if (count > DVZ_SCENE_MAX_COLOR_STOPS)
    {
        log_error("too many color stops: %u > %u", count, DVZ_SCENE_MAX_COLOR_STOPS);
        return;
    }
    if (count > 0)
        ANN(stops);
    colormap->stop_count = count;
    if (count > 0)
        dvz_memcpy(colormap->stops, sizeof(colormap->stops), stops, count * sizeof(DvzColormapStop));
    _scene_mark_colormap_dirty(colormap);
}


/**
 * Set the diverging center on a colormap.
 *
 * @param colormap the colormap
 * @param center the semantic center value
 */
void dvz_colormap_set_center(DvzColormap* colormap, double center)
{
    ANN(colormap);
    colormap->center = center;
    colormap->has_center = true;
    _scene_mark_colormap_dirty(colormap);
}


/**
 * Create a panel-attached colorbar bound to a scale.
 *
 * @param panel the panel
 * @param scale the scale
 * @param desc the colorbar descriptor, or NULL for defaults
 * @return the colorbar, or NULL on allocation failure
 */
DvzColorbar* dvz_colorbar(DvzPanel* panel, DvzScale* scale, const DvzColorbarDesc* desc)
{
    ANN(panel);
    ANN(scale);
    if (panel->figure == NULL || panel->figure->scene == NULL)
    {
        log_error("cannot create a colorbar on a detached panel");
        return NULL;
    }
    DvzScene* scene = panel->figure->scene;
    if (scale->scene != scene)
    {
        log_error("cannot attach a scale from a different scene to a panel colorbar");
        return NULL;
    }
    if (scene->colorbar_count >= DVZ_SCENE_MAX_COLORBARS)
    {
        log_error("maximum colorbar count reached");
        return NULL;
    }
    if (panel->colorbar_count >= DVZ_SCENE_MAX_PANEL_COLORBARS)
    {
        log_error("maximum panel colorbar count reached");
        return NULL;
    }
    DvzColorbar* colorbar = &scene->colorbars[scene->colorbar_count++];
    dvz_memset(colorbar, sizeof(DvzColorbar), 0, sizeof(DvzColorbar));
    colorbar->scene = scene;
    colorbar->panel = panel;
    colorbar->scale = scale;
    colorbar->orientation =
        desc != NULL ? desc->orientation : DVZ_COLORBAR_ORIENTATION_VERTICAL;
    colorbar->anchor = desc != NULL ? desc->anchor : DVZ_SCENE_ANCHOR_PANEL_RIGHT;
    colorbar->flags = desc != NULL ? desc->flags : 0;
    if (desc != NULL && desc->title != NULL)
        dvz_strlcpy(colorbar->title, desc->title, sizeof(colorbar->title));
    panel->colorbars[panel->colorbar_count++] = colorbar;
    return colorbar;
}


/**
 * Destroy a colorbar.
 *
 * @param colorbar the colorbar
 */
void dvz_colorbar_destroy(DvzColorbar* colorbar)
{
    if (colorbar == NULL)
        return;
    if (colorbar->panel != NULL)
    {
        DvzPanel* panel = colorbar->panel;
        for (uint32_t i = 0; i < panel->colorbar_count; i++)
        {
            if (panel->colorbars[i] != colorbar)
                continue;
            for (uint32_t j = i + 1; j < panel->colorbar_count; j++)
                panel->colorbars[j - 1] = panel->colorbars[j];
            panel->colorbars[panel->colorbar_count - 1] = NULL;
            panel->colorbar_count--;
            break;
        }
    }
    colorbar->scene = NULL;
    colorbar->panel = NULL;
    colorbar->scale = NULL;
    colorbar->has_format = false;
}


/**
 * Override formatting policy on a colorbar.
 *
 * @param colorbar the colorbar
 * @param format the format descriptor, or NULL to clear the override
 */
void dvz_colorbar_set_format(DvzColorbar* colorbar, const DvzFormatDesc* format)
{
    ANN(colorbar);
    colorbar->has_format = format != NULL;
    _format_state_copy(&colorbar->format, format);
}



/*************************************************************************************************/
/*  Scene JSON serialization                                                                     */
/*************************************************************************************************/

/* Return the scene-global index of a visual, or UINT32_MAX if not found. */
static uint32_t _visual_index(const DvzScene* scene, const DvzVisual* visual)
{
    for (uint32_t i = 0; i < scene->visual_count; i++)
        if (&scene->visuals[i] == visual)
            return i;
    return UINT32_MAX;
}


static void _json_append_visual_binding(
    JsonBuilder* b, const DvzVisual* visual, DvzVisualBindingKind kind)
{
    ANN(b);
    if (visual == NULL || visual->scene == NULL)
    {
        _json_append(b, "null");
        return;
    }

    const DvzVisualBinding* binding = _visual_binding_const(visual, kind);
    if (binding == NULL || binding->resource == NULL)
    {
        _json_append(b, "null");
        return;
    }

    switch (kind)
    {
    case DVZ_VISUAL_BINDING_SCALE:
        for (uint32_t si = 0; si < visual->scene->scale_count; si++)
        {
            if (&visual->scene->scales[si] != (DvzScale*)binding->resource)
                continue;
            _json_append(b, "{\"id\":\"s%u\",\"slot\":", si);
            _json_append_escaped_string(b, binding->slot);
            _json_append(b, "}");
            return;
        }
        break;
    case DVZ_VISUAL_BINDING_FIELD:
    {
        uint32_t field_idx = _scene_field_index(visual->scene, (DvzSampledField*)binding->resource);
        if (field_idx != UINT32_MAX)
        {
            _json_append(b, "{\"id\":\"f%u\",\"slot\":", field_idx);
            _json_append_escaped_string(b, binding->slot);
            _json_append(b, "}");
            return;
        }
        break;
    }
    case DVZ_VISUAL_BINDING_BUFFER:
    {
        uint32_t buffer_idx = _scene_buffer_index(visual->scene, (DvzSceneBuffer*)binding->resource);
        if (buffer_idx != UINT32_MAX)
        {
            _json_append(b, "{\"id\":\"b%u\",\"slot\":", buffer_idx);
            _json_append_escaped_string(b, binding->slot);
            _json_append(b, "}");
            return;
        }
        break;
    }
    default:
        break;
    }

    _json_append(b, "null");
}



/**
 * Append one sampled-field JSON object.
 *
 * @param b the JSON builder
 * @param scene the owning scene
 * @param field_idx the field index
 * @param first whether this is the first array item
 */
static void _json_append_field(
    JsonBuilder* b, const DvzScene* scene, uint32_t field_idx, bool* first)
{
    ANN(b);
    ANN(scene);
    ANN(first);
    const DvzSampledField* field = &scene->fields[field_idx];
    if (field->scene != scene)
        return;
    _json_append(
        b,
        "%s{\"id\":\"f%u\",\"dim\":%u,\"format\":%u,\"semantic\":%u,"
        "\"width\":%u,\"height\":%u,\"depth\":%u,\"data\":",
        *first ? "" : ",", field_idx, (uint32_t)field->desc.dim, (uint32_t)field->desc.format,
        (uint32_t)field->desc.semantic, field->desc.width, field->desc.height, field->desc.depth);
    if (field->data != NULL && field->data_size > 0)
        _json_append_base64(b, (const uint8_t*)field->data, field->data_size);
    else
        _json_append(b, "null");
    _json_append(
        b,
        ",\"geometry\":{\"axis_order\":[%u,%u,%u],\"axis_flip\":[%s,%s,%s],"
        "\"origin\":[%.6g,%.6g,%.6g],\"spacing\":[%.6g,%.6g,%.6g],\"unit\":",
        field->geometry.axis_order[0], field->geometry.axis_order[1], field->geometry.axis_order[2],
        field->geometry.axis_flip[0] ? "true" : "false",
        field->geometry.axis_flip[1] ? "true" : "false",
        field->geometry.axis_flip[2] ? "true" : "false", field->geometry.origin[0],
        field->geometry.origin[1], field->geometry.origin[2], field->geometry.spacing[0],
        field->geometry.spacing[1], field->geometry.spacing[2]);
    _json_append_escaped_string(b, field->geometry.unit);
    _json_append(
        b,
        "},\"dirty\":{\"pending\":%s,\"full\":%s,\"region\":{\"x\":%u,\"y\":%u,\"z\":%u,"
        "\"width\":%u,\"height\":%u,\"depth\":%u}}}",
        field->dirty ? "true" : "false", field->dirty_full ? "true" : "false",
        field->dirty_region.x, field->dirty_region.y, field->dirty_region.z,
        field->dirty_region.width, field->dirty_region.height, field->dirty_region.depth);
    *first = false;
}



/**
 * Append one scene-buffer JSON object.
 *
 * @param b the JSON builder
 * @param scene the owning scene
 * @param buffer_idx the buffer index
 * @param first whether this is the first array item
 */
static void _json_append_buffer(
    JsonBuilder* b, const DvzScene* scene, uint32_t buffer_idx, bool* first)
{
    ANN(b);
    ANN(scene);
    ANN(first);
    const DvzSceneBuffer* buffer = &scene->buffers[buffer_idx];
    if (buffer->scene != scene)
        return;
    _json_append(
        b, "%s{\"id\":\"b%u\",\"usage\":%u,\"stride\":%u,\"byte_size\":%" PRIu64 ",\"data\":",
        *first ? "" : ",", buffer_idx, buffer->desc.usage, buffer->desc.stride,
        buffer->desc.byte_size);
    if (buffer->data != NULL && buffer->desc.byte_size > 0)
        _json_append_base64(b, (const uint8_t*)buffer->data, buffer->desc.byte_size);
    else
        _json_append(b, "null");
    _json_append(b, ",\"dirty\":{\"pending\":%s}}", buffer->dirty ? "true" : "false");
    *first = false;
}



/**
 * Append one visual attribute JSON object.
 *
 * @param b the JSON builder
 * @param attr the attribute
 * @param first whether this is the first array item
 */
static void _json_append_visual_attr(
    JsonBuilder* b, const DvzVisualAttr* attr, bool* first)
{
    ANN(b);
    ANN(attr);
    ANN(first);
    uint64_t byte_size = (uint64_t)attr->item_count * attr->item_size;
    _json_append(
        b, "%s{\"name\":\"%s\",\"item_count\":%u,\"item_size\":%u,\"data\":",
        *first ? "" : ",", attr->name, attr->item_count, attr->item_size);
    if (attr->data != NULL && byte_size > 0)
        _json_append_base64(b, (const uint8_t*)attr->data, byte_size);
    else
        _json_append(b, "null");
    _json_append(b, "}");
    *first = false;
}



/**
 * Append one panel visual JSON object.
 *
 * @param b the JSON builder
 * @param scene the owning scene
 * @param visual the visual
 * @param first whether this is the first array item
 */
static void _json_append_visual(
    JsonBuilder* b, const DvzScene* scene, const DvzVisual* visual, bool* first)
{
    ANN(b);
    ANN(scene);
    ANN(visual);
    ANN(first);
    uint32_t visual_idx = _visual_index(scene, visual);
    _json_append(
        b, "%s{\"id\":\"v%u\",\"type\":\"%s\",\"visible\":%s,\"attrs\":[", *first ? "" : ",",
        visual_idx, _visual_type_name(visual->type), visual->visible ? "true" : "false");
    bool first_attr = true;
    for (uint32_t ai = 0; ai < visual->attr_count; ai++)
        _json_append_visual_attr(b, &visual->attrs[ai], &first_attr);
    _json_append(b, "],\"scale\":");
    _json_append_visual_binding(b, visual, DVZ_VISUAL_BINDING_SCALE);
    _json_append(b, ",\"field\":");
    _json_append_visual_binding(b, visual, DVZ_VISUAL_BINDING_FIELD);
    _json_append(b, ",\"buffer\":");
    _json_append_visual_binding(b, visual, DVZ_VISUAL_BINDING_BUFFER);
    _json_append(b, ",\"field_state\":");
    if (visual->field != NULL)
    {
        _json_append(
            b,
            "{\"pending\":%s,\"full\":%s,\"region\":{\"x\":%u,\"y\":%u,\"z\":%u,"
            "\"width\":%u,\"height\":%u,\"depth\":%u}}",
            visual->texture.field_dirty ? "true" : "false",
            visual->texture.field_dirty_full ? "true" : "false",
            visual->texture.field_dirty_region.x, visual->texture.field_dirty_region.y,
            visual->texture.field_dirty_region.z, visual->texture.field_dirty_region.width,
            visual->texture.field_dirty_region.height, visual->texture.field_dirty_region.depth);
    }
    else
    {
        _json_append(b, "null");
    }
    _json_append(b, "}");
    *first = false;
}



/**
 * Append one panel JSON object.
 *
 * @param b the JSON builder
 * @param scene the owning scene
 * @param figure_idx the parent figure index
 * @param panel_idx the panel index
 * @param panel the panel
 * @param first whether this is the first array item
 */
static void _json_append_panel(
    JsonBuilder* b, const DvzScene* scene, uint32_t figure_idx, uint32_t panel_idx,
    const DvzPanel* panel, bool* first)
{
    ANN(b);
    ANN(scene);
    ANN(panel);
    ANN(first);
    _json_append(
        b,
        "%s{\"id\":\"fig%u_p%u\","
        "\"desc\":{\"x\":%.6g,\"y\":%.6g,\"width\":%.6g,\"height\":%.6g},"
        "\"visuals\":[",
        *first ? "" : ",", figure_idx, panel_idx, (double)panel->desc.x, (double)panel->desc.y,
        (double)panel->desc.width, (double)panel->desc.height);
    bool first_visual = true;
    for (uint32_t vi = 0; vi < panel->visual_count; vi++)
    {
        const DvzVisual* visual = panel->visuals[vi].visual;
        if (visual == NULL)
            continue;
        _json_append_visual(b, scene, visual, &first_visual);
    }
    _json_append(b, "]}");
    *first = false;
}



/**
 * Append one figure JSON object.
 *
 * @param b the JSON builder
 * @param scene the owning scene
 * @param figure_idx the figure index
 * @param first whether this is the first array item
 */
static void _json_append_figure(
    JsonBuilder* b, const DvzScene* scene, uint32_t figure_idx, bool* first)
{
    ANN(b);
    ANN(scene);
    ANN(first);
    const DvzFigure* figure = &scene->figures[figure_idx];
    if (figure->scene == NULL)
        return;
    _json_append(
        b, "%s{\"id\":\"fig%u\",\"width\":%u,\"height\":%u,\"panels\":[", *first ? "" : ",",
        figure_idx, figure->width, figure->height);
    bool first_panel = true;
    for (uint32_t pi = 0; pi < figure->panel_count; pi++)
        _json_append_panel(b, scene, figure_idx, pi, &figure->panels[pi], &first_panel);
    _json_append(b, "]}");
    *first = false;
}



char* dvz_scene_json(const DvzScene* scene)
{
    ANN(scene);

    JsonBuilder b = {0};
    if (!_json_init(&b))
        return NULL;

    _json_append(&b, "{\"fields\":[");
    bool first_field = true;
    for (uint32_t i = 0; i < scene->field_count; i++)
        _json_append_field(&b, scene, i, &first_field);

    _json_append(&b, "],\"buffers\":[");
    bool first_buffer = true;
    for (uint32_t i = 0; i < scene->buffer_count; i++)
        _json_append_buffer(&b, scene, i, &first_buffer);

    _json_append(&b, "],\"figures\":[");
    bool first_figure = true;
    for (uint32_t fi = 0; fi < scene->figure_count; fi++)
        _json_append_figure(&b, scene, fi, &first_figure);
    _json_append(&b, "]}");

    return _json_finish(&b);
}



void dvz_scene_json_destroy(char* json)
{
    dvz_free(json);
}
