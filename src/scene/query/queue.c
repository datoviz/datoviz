/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene query queue bridge                                                                     */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "../_scene.h"
#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "datoviz/scene.h"
#include "internal.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return whether two request ids belong to the same query freshness scope.
 *
 * @param lhs_request_id the first request id
 * @param rhs_request_id the second request id
 * @return true when the ids share one latest-wins scope
 */
static bool _query_request_ids_share_scope(uint64_t lhs_request_id, uint64_t rhs_request_id)
{
    if (lhs_request_id == 0 || rhs_request_id == 0)
        return lhs_request_id == 0 && rhs_request_id == 0;
    return lhs_request_id == rhs_request_id;
}



/**
 * Touch one query request freshness scope.
 *
 * @param scene the scene
 * @param panel the panel
 * @param request_id the request id
 * @param freshness_serial the newest request serial
 */
static void _query_track_request_serial(
    DvzScene* scene, DvzPanel* panel, uint64_t request_id, uint64_t freshness_serial)
{
    ANN(scene);
    ANN(panel);
    if (freshness_serial == 0)
        return;

    for (uint32_t i = 0; i < scene->query_scope_count; i++)
    {
        DvzRequestFreshnessScope* scope = &scene->query_scopes[i];
        if (scope->panel == panel && _query_request_ids_share_scope(scope->request_id, request_id))
        {
            scope->request_id = request_id;
            scope->freshness_serial = freshness_serial;
            scope->touched_serial = freshness_serial;
            return;
        }
    }

    if (scene->query_scope_count >= DVZ_SCENE_MAX_REQUEST_SCOPES)
    {
        log_error("query request freshness scope table is full");
        return;
    }

    DvzRequestFreshnessScope* scope = &scene->query_scopes[scene->query_scope_count++];
    *scope = (DvzRequestFreshnessScope){
        .panel = panel,
        .request_id = request_id,
        .freshness_serial = freshness_serial,
        .touched_serial = freshness_serial,
    };
}



/**
 * Return whether a pending query is superseded by a newer request.
 *
 * @param pending the pending query
 * @param panel the panel
 * @param request_id the newer request id
 * @return true when the pending query is stale
 */
static bool _query_pending_superseded(
    const DvzPendingQueryRequest* pending, const DvzPanel* panel, uint64_t request_id)
{
    ANN(pending);
    return pending->panel == panel &&
           _query_request_ids_share_scope(pending->request.request_id, request_id);
}



/**
 * Drop unresolved query requests superseded by a newer panel query.
 *
 * @param scene the scene
 * @param panel the panel
 * @param request_id the new request id
 */
static void _query_drop_superseded_requests(
    DvzScene* scene, const DvzPanel* panel, uint64_t request_id)
{
    ANN(scene);
    ANN(panel);
    uint32_t old_count = scene->pending_query_count;
    uint32_t write = 0;
    for (uint32_t read = 0; read < scene->pending_query_count; read++)
    {
        DvzPendingQueryRequest pending = scene->pending_queries[read];
        if (_query_pending_superseded(&pending, panel, request_id))
            continue;
        if (write != read)
            scene->pending_queries[write] = pending;
        write++;
    }
    for (uint32_t i = write; i < old_count; i++)
    {
        dvz_memset(
            &scene->pending_queries[i], sizeof(DvzPendingQueryRequest), 0,
            sizeof(DvzPendingQueryRequest));
    }
    scene->pending_query_count = write;
}



/**
 * Coalesce pending query requests for one figure.
 *
 * @param scene the scene
 * @param figure the figure being processed
 */
static void _query_coalesce_pending_requests(DvzScene* scene, const DvzFigure* figure)
{
    ANN(scene);
    ANN(figure);
    bool keep[DVZ_SCENE_MAX_PENDING_REQUESTS] = {0};
    uint32_t write = 0;
    uint32_t old_count = scene->pending_query_count;

    for (int32_t i = (int32_t)scene->pending_query_count - 1; i >= 0; i--)
    {
        const DvzPendingQueryRequest* pending = &scene->pending_queries[i];
        bool keep_pending = true;
        if (pending->panel != NULL && pending->panel->figure == figure)
        {
            for (uint32_t j = (uint32_t)i + 1; j < scene->pending_query_count; j++)
            {
                const DvzPendingQueryRequest* newer = &scene->pending_queries[j];
                if (!keep[j])
                    continue;
                if (newer->panel != pending->panel)
                    continue;
                if (!_query_request_ids_share_scope(
                        newer->request.request_id, pending->request.request_id))
                {
                    continue;
                }
                keep_pending = false;
                break;
            }
        }
        keep[i] = keep_pending;
    }

    for (uint32_t read = 0; read < old_count; read++)
    {
        if (!keep[read])
            continue;
        if (write != read)
            scene->pending_queries[write] = scene->pending_queries[read];
        write++;
    }
    for (uint32_t i = write; i < old_count; i++)
    {
        dvz_memset(
            &scene->pending_queries[i], sizeof(DvzPendingQueryRequest), 0,
            sizeof(DvzPendingQueryRequest));
    }
    scene->pending_query_count = write;
}



/**
 * Remove one pending query by queue index.
 *
 * @param scene the scene
 * @param index the queue index
 */
static void _query_remove_pending_at(DvzScene* scene, uint32_t index)
{
    ANN(scene);
    ASSERT(index < scene->pending_query_count);
    for (uint32_t i = index + 1; i < scene->pending_query_count; i++)
        scene->pending_queries[i - 1] = scene->pending_queries[i];
    scene->pending_query_count--;
    dvz_memset(
        &scene->pending_queries[scene->pending_query_count], sizeof(DvzPendingQueryRequest), 0,
        sizeof(DvzPendingQueryRequest));
}



/**
 * Return the capability bit required by one query target.
 *
 * @param target the requested scene target
 * @return required capability bit, or zero for unsupported targets
 */
static uint32_t _query_target_capability(DvzSceneTargetKind target)
{
    switch (target)
    {
    case DVZ_SCENE_TARGET_NONE:
        return DVZ_PICK_CAPABILITY_ITEM;
    case DVZ_SCENE_TARGET_OBJECT:
        return DVZ_PICK_CAPABILITY_OBJECT;
    case DVZ_SCENE_TARGET_ITEM:
        return DVZ_PICK_CAPABILITY_ITEM;
    case DVZ_SCENE_TARGET_VERTEX:
        return DVZ_PICK_CAPABILITY_VERTEX;
    case DVZ_SCENE_TARGET_FACE:
    case DVZ_SCENE_TARGET_TRIANGLE:
        return DVZ_PICK_CAPABILITY_FACE;
    case DVZ_SCENE_TARGET_PIXEL:
        return DVZ_PICK_CAPABILITY_PIXEL;
    case DVZ_SCENE_TARGET_SAMPLE:
        return DVZ_PICK_CAPABILITY_SAMPLE;
    case DVZ_SCENE_TARGET_STRIP:
        return DVZ_PICK_CAPABILITY_GROUP;
    case DVZ_SCENE_TARGET_SEGMENT:
        return DVZ_PICK_CAPABILITY_ITEM;
    case DVZ_SCENE_TARGET_TEXT:
    case DVZ_SCENE_TARGET_ANNOTATION:
        return DVZ_PICK_CAPABILITY_OBJECT;
    default:
        return 0;
    }
}



/**
 * Return whether a target uses the value/probe legacy adapter.
 *
 * @param target the query target
 * @return true for probe-style targets
 */
static bool _query_target_uses_probe(DvzSceneTargetKind target)
{
    return target == DVZ_SCENE_TARGET_SAMPLE || target == DVZ_SCENE_TARGET_SEGMENT;
}



/**
 * Return whether a query profile is supported by the capability snapshot.
 *
 * @param profile the query profile
 * @param caps the capability snapshot
 * @return true when supported
 */
static bool _query_profile_supported(DvzQueryProfile profile, const DvzCapabilitySnapshot* caps)
{
    ANN(caps);
    if (!caps->supports_readback)
        return false;

    switch (profile)
    {
    case DVZ_QUERY_PROFILE_U32_R32:
        return caps->query_profile_u32_r32;
    case DVZ_QUERY_PROFILE_U64_RG32:
        return caps->query_profile_u64_rg32;
    case DVZ_QUERY_PROFILE_U64_2XR32:
        return caps->query_profile_u64_2xr32;
    case DVZ_QUERY_PROFILE_PACKED_RGBA8:
        return caps->query_profile_packed_rgba8;
    case DVZ_QUERY_PROFILE_UNSUPPORTED:
    default:
        return false;
    }
}



/**
 * Resolve the effective profile for one query request.
 *
 * @param request the query request
 * @param caps the capability snapshot
 * @return a supported profile, or unsupported
 */
static DvzQueryProfile
_query_select_profile(const DvzQueryRequest* request, const DvzCapabilitySnapshot* caps)
{
    ANN(request);
    ANN(caps);
    if (request->profile != DVZ_QUERY_PROFILE_UNSUPPORTED)
    {
        if (_query_profile_supported(request->profile, caps))
            return request->profile;
        return DVZ_QUERY_PROFILE_UNSUPPORTED;
    }
    if (_query_profile_supported(DVZ_QUERY_PROFILE_U32_R32, caps))
        return DVZ_QUERY_PROFILE_U32_R32;
    if (_query_profile_supported(DVZ_QUERY_PROFILE_U64_RG32, caps))
        return DVZ_QUERY_PROFILE_U64_RG32;
    if (_query_profile_supported(DVZ_QUERY_PROFILE_U64_2XR32, caps))
        return DVZ_QUERY_PROFILE_U64_2XR32;
    if (_query_profile_supported(DVZ_QUERY_PROFILE_PACKED_RGBA8, caps))
        return DVZ_QUERY_PROFILE_PACKED_RGBA8;
    return DVZ_QUERY_PROFILE_UNSUPPORTED;
}



/**
 * Return one currently drawable query candidate for a panel request.
 *
 * @param panel the panel
 * @param capability required query capability
 * @return the topmost matching visual, or NULL
 */
static DvzVisual* _query_candidate_visual(const DvzPanel* panel, uint32_t capability)
{
    ANN(panel);
    if (capability == 0)
        return NULL;
    for (int32_t i = (int32_t)panel->visual_count - 1; i >= 0; i--)
    {
        DvzVisual* visual = panel->visuals[i].visual;
        if (visual == NULL || !visual->visible)
            continue;
        if ((visual->pick_capabilities & capability) != 0)
            return visual;
    }
    return NULL;
}


/**
 * Return the family query operation table eligible for one visual.
 *
 * @param panel the panel
 * @param visual the visual
 * @param request the query request
 * @return operation table, or NULL when no native family path is eligible
 */
static const DvzSceneQueryFamilyOps* _query_family_ops_for_visual(
    const DvzPanel* panel, const DvzVisual* visual, const DvzQueryRequest* request)
{
    ANN(panel);
    ANN(visual);
    ANN(request);
    for (uint32_t i = 0; i < _dvz_scene_query_registry_count(); i++)
    {
        const DvzSceneQueryFamilyOps* ops = _dvz_scene_query_registry_get(i);
        if (ops == NULL || ops->eligible == NULL)
            continue;
        if (ops->eligible(panel, visual, request))
            return ops;
    }
    return NULL;
}



/**
 * Initialize a query result from one pending request.
 *
 * @param figure the figure
 * @param pending the pending query
 * @param out_result output result
 */
static void _query_result_init(
    const DvzFigure* figure, const DvzPendingQueryRequest* pending, DvzQueryResult* out_result)
{
    ANN(figure);
    ANN(pending);
    ANN(out_result);
    *out_result = (DvzQueryResult){0};
    out_result->request_id = pending->request.request_id;
    out_result->freshness_serial = pending->freshness_serial;
    out_result->status = DVZ_QUERY_STATUS_UNKNOWN;
    out_result->panel_id = _scene_panel_public_id(figure, pending->panel);
    out_result->panel_position[0] = pending->x;
    out_result->panel_position[1] = pending->y;
    out_result->raw_target = pending->request.target;
    out_result->resolved_target = pending->request.target;
    out_result->value_kind = DVZ_QUERY_VALUE_NONE;
}



/**
 * Resolve one native query request without falling back to pick/probe queues.
 *
 * @param figure the figure
 * @param caps the capability snapshot
 * @param pending the pending query
 * @param out_result output result
 * @return true when a result was produced
 */
static bool _query_process_pending(
    DvzFigure* figure, DvzDrp2Runtime* runtime, DvzSceneRequestExecutor* executor,
    const DvzCapabilitySnapshot* caps,
    const DvzPendingQueryRequest* pending, DvzQueryResult* out_result)
{
    ANN(figure);
    ANN(executor);
    ANN(caps);
    ANN(pending);
    ANN(out_result);
    _query_result_init(figure, pending, out_result);

    vec2 request_ndc = {0};
    if (!_scene_pick_request_ndc(figure, pending->panel, pending->x, pending->y, request_ndc))
    {
        out_result->status = DVZ_QUERY_STATUS_OUTSIDE_PANEL;
        return true;
    }

    uint32_t capability = _query_target_capability(pending->request.target);
    if (capability == 0)
    {
        out_result->status = DVZ_QUERY_STATUS_UNSUPPORTED_TARGET;
        return true;
    }

    out_result->profile = _query_select_profile(&pending->request, caps);
    if (out_result->profile == DVZ_QUERY_PROFILE_UNSUPPORTED)
    {
        out_result->status = caps->supports_readback ? DVZ_QUERY_STATUS_UNSUPPORTED_QUERY_PROFILE
                                                     : DVZ_QUERY_STATUS_READBACK_FAILED;
        return true;
    }

    for (int32_t i = (int32_t)pending->panel->visual_count - 1; i >= 0; i--)
    {
        DvzVisual* visual = pending->panel->visuals[i].visual;
        if (visual == NULL || !visual->visible)
            continue;
        if ((visual->pick_capabilities & capability) == 0)
            continue;
        const DvzSceneQueryFamilyOps* ops =
            _query_family_ops_for_visual(pending->panel, visual, &pending->request);
        if (ops == NULL || ops->build == NULL || ops->decode == NULL)
            continue;
        if (_dvz_scene_query_execute_family(
                figure, runtime, executor, caps, pending, request_ndc, out_result->profile,
                visual, ops, out_result))
        {
            out_result->freshness_serial = pending->freshness_serial;
            out_result->profile = _query_select_profile(&pending->request, caps);
            return true;
        }
    }

    DvzVisual* visual = _query_candidate_visual(pending->panel, capability);
    if (visual == NULL)
    {
        out_result->status = DVZ_QUERY_STATUS_NO_CAPABLE_VISUAL;
        return true;
    }

    if (pending->request.target == DVZ_SCENE_TARGET_PIXEL)
    {
        out_result->visual_id = _scene_visual_public_id(figure->scene, visual);
        out_result->status = DVZ_QUERY_STATUS_UNSUPPORTED_VISUAL_FAMILY;
        return true;
    }

    if (_query_target_uses_probe(pending->request.target))
    {
        DvzProbeResult probe = {0};
        if (_scene_query_execute_probe_legacy(figure, runtime, executor, caps, pending, &probe))
        {
            _dvz_scene_query_from_probe(&probe, out_result);
            out_result->freshness_serial = pending->freshness_serial;
            out_result->profile = _query_select_profile(&pending->request, caps);
            return true;
        }
    }
    else
    {
        DvzPickResult pick = {0};
        if (_scene_query_execute_pick_legacy(figure, runtime, executor, caps, pending, &pick))
        {
            _dvz_scene_query_from_pick(&pick, out_result);
            out_result->freshness_serial = pending->freshness_serial;
            out_result->profile = _query_select_profile(&pending->request, caps);
            return true;
        }
    }

    out_result->visual_id = _scene_visual_public_id(figure->scene, visual);
    out_result->status = runtime == NULL ? DVZ_QUERY_STATUS_GPU_EXEC_FAILED
                                         : DVZ_QUERY_STATUS_UNSUPPORTED_VISUAL_FAMILY;
    return true;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Queue one panel query.
 *
 * @param panel the panel
 * @param x panel-local logical x coordinate
 * @param y panel-local logical y coordinate
 * @param request query request, or NULL for defaults
 * @return 0 on success, -1 on failure
 */
int dvz_panel_query(DvzPanel* panel, double x, double y, const DvzQueryRequest* request)
{
    ANN(panel);
    if (panel->figure == NULL || panel->figure->scene == NULL)
        return -1;
    DvzScene* scene = panel->figure->scene;
    DvzQueryRequest local = {0};
    if (request != NULL)
        local = *request;

    _query_drop_superseded_requests(scene, panel, local.request_id);
    _dvz_scene_query_drop_superseded_results(scene, panel, local.request_id);
    if (scene->pending_query_count >= DVZ_SCENE_MAX_PENDING_REQUESTS)
    {
        log_error("query request queue is full");
        return -1;
    }

    DvzPendingQueryRequest* pending = &scene->pending_queries[scene->pending_query_count++];
    dvz_memset(pending, sizeof(DvzPendingQueryRequest), 0, sizeof(DvzPendingQueryRequest));
    pending->panel = panel;
    pending->x = x;
    pending->y = y;
    pending->freshness_serial = _scene_next_request_serial(scene);
    pending->request = local;
    _query_track_request_serial(scene, panel, local.request_id, pending->freshness_serial);
    _scene_notify_request_frame(panel->figure);
    return 0;
}



/**
 * Execute queued panel queries for one figure.
 *
 * @param figure the figure
 * @param runtime the DRP2 runtime
 * @param caps the capability snapshot, or NULL for defaults
 * @return the number of consumed requests
 */
uint32_t dvz_figure_process_queries(
    DvzFigure* figure, DvzDrp2Runtime* runtime, const DvzCapabilitySnapshot* caps)
{
    ANN(figure);
    ANN(figure->scene);

    DvzCapabilitySnapshot local_caps = {0};
    if (caps == NULL)
    {
        dvz_capability_snapshot_default(&local_caps);
        caps = &local_caps;
    }

    if (!_scene_figure_resolve_layouts(figure))
        return 0;

    DvzScene* scene = figure->scene;
    uint32_t processed = 0;
    _query_coalesce_pending_requests(scene, figure);
    DvzSceneRequestExecutor executor = {0};
    _scene_request_executor_init(&executor);

    for (uint32_t i = 0; i < scene->pending_query_count;)
    {
        const DvzPendingQueryRequest pending = scene->pending_queries[i];
        if (pending.panel == NULL || pending.panel->figure != figure)
        {
            i++;
            continue;
        }

        DvzQueryResult result = {0};
        if (_query_process_pending(figure, runtime, &executor, caps, &pending, &result))
            (void)_dvz_scene_query_push_result(
                scene, pending.panel, pending.freshness_serial, &result);

        _query_remove_pending_at(scene, i);
        processed++;
    }

    _scene_request_executor_destroy(&executor);
    return processed;
}



/**
 * Queue and synchronously resolve one panel query.
 *
 * @param panel the panel
 * @param runtime the DRP2 runtime
 * @param x panel-local logical x coordinate
 * @param y panel-local logical y coordinate
 * @param request query request, or NULL for defaults
 * @param out_result output query result
 * @return 0 on success, -1 on failure
 */
int dvz_panel_query_now(
    DvzPanel* panel, DvzDrp2Runtime* runtime, double x, double y, const DvzQueryRequest* request,
    DvzQueryResult* out_result)
{
    ANN(panel);
    ANN(runtime);
    ANN(out_result);
    if (panel->figure == NULL || dvz_panel_query(panel, x, y, request) != 0)
        return -1;

    (void)dvz_figure_process_queries(panel->figure, runtime, NULL);
    const uint64_t request_id = request != NULL ? request->request_id : 0;
    DvzQueryResult result = {0};
    while (dvz_scene_poll_query(panel->figure->scene, &result))
    {
        if (result.request_id == request_id)
        {
            *out_result = result;
            return 0;
        }
    }
    return -1;
}
