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

#include "_scene.h"
#include "core/scene_notify_internal.h"
#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "datoviz/scene.h"
#include "interaction/internal.h"
#include "internal.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

#define DVZ_QUERY_REQUEST_KNOWN_FLAGS 0u



static bool _query_request_validate(const DvzQueryRequest* request)
{
    if (request == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(request, DvzQueryRequest, DVZ_QUERY_REQUEST_KNOWN_FLAGS))
    {
        log_error("invalid DvzQueryRequest ABI prologue");
        return false;
    }
    return true;
}



DvzQueryRequest dvz_query_request(void)
{
    DvzQueryRequest request = {DVZ_STRUCT_INIT_FIELDS(DvzQueryRequest)};
    return request;
}


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
    if (!_query_request_validate(request))
        return -1;
    if (panel->figure == NULL || panel->figure->scene == NULL)
        return -1;
    DvzScene* scene = panel->figure->scene;
    DvzQueryRequest local = dvz_query_request();
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
 * Queue one panel query at a DATA-coordinate point.
 *
 * @param panel the panel
 * @param x data x coordinate
 * @param y data y coordinate
 * @param request query request, or NULL for defaults
 * @return 0 on success, -1 on failure
 */
int dvz_panel_query_data(DvzPanel* panel, double x, double y, const DvzQueryRequest* request)
{
    double panel_px[2] = {0};
    if (!dvz_panel_data_to_position(
            panel, DVZ_PANEL_COORD_PANEL_PX, (const double[2]){x, y}, panel_px))
    {
        return -1;
    }
    return dvz_panel_query(panel, panel_px[0], panel_px[1], request);
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
        local_caps = dvz_capability_snapshot();
        caps = &local_caps;
    }
    if (!dvz_capability_snapshot_valid(caps))
        return 0;

    if (!_scene_figure_resolve_layouts(figure))
        return 0;

    DvzScene* scene = figure->scene;
    uint32_t processed = 0;
    _query_coalesce_pending_requests(scene, figure);
    DvzSceneRequestExecutor* executor = &scene->query_executor;

    for (uint32_t i = 0; i < scene->pending_query_count;)
    {
        const DvzPendingQueryRequest pending = scene->pending_queries[i];
        if (pending.panel == NULL || pending.panel->figure != figure)
        {
            i++;
            continue;
        }

        DvzQueryResult result = {0};
        if (_dvz_scene_query_process_pending(figure, runtime, executor, caps, &pending, &result))
        {
            _scene_item_interaction_apply_query_result(
                pending.item_interaction, pending.item_interaction_kind, &result);
            (void)_dvz_scene_query_push_result(
                scene, pending.panel, pending.freshness_serial, &result);
        }

        _query_remove_pending_at(scene, i);
        processed++;
    }

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
