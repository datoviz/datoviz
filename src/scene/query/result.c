/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene query result helpers                                                                   */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "internal.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Convert a pick status to a query status.
 *
 * @param status pick status
 * @return query status
 */
static DvzQueryStatus _query_status_from_pick(DvzPickStatus status)
{
    switch (status)
    {
    case DVZ_PICK_STATUS_HIT:
        return DVZ_QUERY_STATUS_HIT;
    case DVZ_PICK_STATUS_MISS:
        return DVZ_QUERY_STATUS_MISS;
    case DVZ_PICK_STATUS_OUTSIDE_PANEL:
        return DVZ_QUERY_STATUS_OUTSIDE_PANEL;
    case DVZ_PICK_STATUS_UNSUPPORTED_TARGET:
        return DVZ_QUERY_STATUS_UNSUPPORTED_TARGET;
    case DVZ_PICK_STATUS_NO_CAPABLE_VISUAL:
        return DVZ_QUERY_STATUS_NO_CAPABLE_VISUAL;
    case DVZ_PICK_STATUS_GPU_EXEC_FAILED:
        return DVZ_QUERY_STATUS_GPU_EXEC_FAILED;
    case DVZ_PICK_STATUS_READBACK_FAILED:
        return DVZ_QUERY_STATUS_READBACK_FAILED;
    case DVZ_PICK_STATUS_STALE_DROPPED:
        return DVZ_QUERY_STATUS_STALE_DROPPED;
    case DVZ_PICK_STATUS_INVALID_RESULT:
        return DVZ_QUERY_STATUS_DECODE_FAILED;
    case DVZ_PICK_STATUS_UNKNOWN:
    default:
        return DVZ_QUERY_STATUS_UNKNOWN;
    }
}



/**
 * Convert a probe status to a query status.
 *
 * @param status probe status
 * @return query status
 */
static DvzQueryStatus _query_status_from_probe(DvzProbeStatus status)
{
    switch (status)
    {
    case DVZ_PROBE_STATUS_HIT:
        return DVZ_QUERY_STATUS_HIT;
    case DVZ_PROBE_STATUS_MISS:
        return DVZ_QUERY_STATUS_MISS;
    case DVZ_PROBE_STATUS_OUTSIDE_PANEL:
        return DVZ_QUERY_STATUS_OUTSIDE_PANEL;
    case DVZ_PROBE_STATUS_UNSUPPORTED_TARGET:
        return DVZ_QUERY_STATUS_UNSUPPORTED_TARGET;
    case DVZ_PROBE_STATUS_NO_CAPABLE_VISUAL:
        return DVZ_QUERY_STATUS_NO_CAPABLE_VISUAL;
    case DVZ_PROBE_STATUS_GPU_EXEC_FAILED:
        return DVZ_QUERY_STATUS_GPU_EXEC_FAILED;
    case DVZ_PROBE_STATUS_READBACK_FAILED:
        return DVZ_QUERY_STATUS_READBACK_FAILED;
    case DVZ_PROBE_STATUS_STALE_DROPPED:
        return DVZ_QUERY_STATUS_STALE_DROPPED;
    case DVZ_PROBE_STATUS_INVALID_RESULT:
        return DVZ_QUERY_STATUS_DECODE_FAILED;
    case DVZ_PROBE_STATUS_UNKNOWN:
    default:
        return DVZ_QUERY_STATUS_UNKNOWN;
    }
}



/**
 * Convert a probe value kind to a query value kind.
 *
 * @param kind probe value kind
 * @return query value kind
 */
static DvzQueryValueKind _query_value_kind_from_probe(DvzProbeValueKind kind)
{
    switch (kind)
    {
    case DVZ_PROBE_VALUE_SCALAR:
        return DVZ_QUERY_VALUE_SCALAR;
    case DVZ_PROBE_VALUE_VEC2:
        return DVZ_QUERY_VALUE_VEC2;
    case DVZ_PROBE_VALUE_VEC3:
        return DVZ_QUERY_VALUE_VEC3;
    case DVZ_PROBE_VALUE_VEC4:
        return DVZ_QUERY_VALUE_VEC4;
    case DVZ_PROBE_VALUE_LABEL:
        return DVZ_QUERY_VALUE_CATEGORY;
    case DVZ_PROBE_VALUE_NONE:
    default:
        return DVZ_QUERY_VALUE_NONE;
    }
}



/**
 * Return whether two request ids belong to the same query freshness scope.
 *
 * @param lhs_request_id the first request id
 * @param rhs_request_id the second request id
 * @return true when the ids share one latest-wins scope
 */
static bool _query_result_request_ids_share_scope(
    uint64_t lhs_request_id, uint64_t rhs_request_id)
{
    if (lhs_request_id == 0 || rhs_request_id == 0)
        return lhs_request_id == 0 && rhs_request_id == 0;
    return lhs_request_id == rhs_request_id;
}



/**
 * Return the latest query freshness serial for one request scope.
 *
 * @param scene the scene
 * @param panel the panel
 * @param request_id the request id
 * @return latest freshness serial, or zero when absent
 */
static uint64_t _query_result_latest_request_serial(
    const DvzScene* scene, const DvzPanel* panel, uint64_t request_id)
{
    ANN(scene);
    ANN(panel);
    for (uint32_t i = 0; i < scene->query_scope_count; i++)
    {
        const DvzRequestFreshnessScope* scope = &scene->query_scopes[i];
        if (scope->panel == panel &&
            _query_result_request_ids_share_scope(scope->request_id, request_id))
        {
            return scope->freshness_serial;
        }
    }
    return 0;
}



/**
 * Return whether a query result scope is still current.
 *
 * @param scene the scene
 * @param panel the panel
 * @param request_id the request id
 * @param freshness_serial the originating freshness serial
 * @return true when the result is current
 */
static bool _query_result_is_current(
    const DvzScene* scene, const DvzPanel* panel, uint64_t request_id, uint64_t freshness_serial)
{
    ANN(scene);
    ANN(panel);
    if (freshness_serial == 0)
        return true;
    uint64_t latest_serial = _query_result_latest_request_serial(scene, panel, request_id);
    return latest_serial == 0 || latest_serial == freshness_serial;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Drop queued query results superseded by a newer panel query.
 *
 * @param scene the scene
 * @param panel the panel
 * @param request_id the new request id
 */
void _dvz_scene_query_drop_superseded_results(
    DvzScene* scene, const DvzPanel* panel, uint64_t request_id)
{
    ANN(scene);
    ANN(panel);
    DvzQueuedQueryResult kept[DVZ_SCENE_MAX_QUERY_RESULTS] = {0};
    uint32_t kept_count = 0;
    for (uint32_t i = 0; i < scene->query_result_count; i++)
    {
        uint32_t index = (scene->query_result_head + i) % DVZ_SCENE_MAX_QUERY_RESULTS;
        DvzQueuedQueryResult queued = scene->query_results[index];
        if (queued.panel == panel &&
            _query_result_request_ids_share_scope(queued.result.request_id, request_id))
        {
            continue;
        }
        kept[kept_count++] = queued;
    }
    dvz_memset(scene->query_results, sizeof(scene->query_results), 0, sizeof(scene->query_results));
    for (uint32_t i = 0; i < kept_count; i++)
        scene->query_results[i] = kept[i];
    scene->query_result_head = 0;
    scene->query_result_count = kept_count;
}



/**
 * Push one native query result.
 *
 * @param scene the scene
 * @param panel the panel
 * @param freshness_serial the originating freshness serial
 * @param result the result
 * @return true on success
 */
bool _dvz_scene_query_push_result(
    DvzScene* scene, DvzPanel* panel, uint64_t freshness_serial, const DvzQueryResult* result)
{
    ANN(scene);
    ANN(result);
    if (panel != NULL &&
        !_query_result_is_current(scene, panel, result->request_id, freshness_serial))
    {
        return true;
    }
    if (scene->query_result_count >= DVZ_SCENE_MAX_QUERY_RESULTS)
    {
        log_error("query result queue is full");
        return false;
    }
    uint32_t index =
        (scene->query_result_head + scene->query_result_count) % DVZ_SCENE_MAX_QUERY_RESULTS;
    scene->query_results[index].panel = panel;
    scene->query_results[index].freshness_serial = freshness_serial;
    scene->query_results[index].result = *result;
    scene->query_result_count++;
    return true;
}



/**
 * Poll one resolved query result.
 *
 * @param scene the scene
 * @param out_result output query result
 * @return true when a result was written
 */
bool dvz_scene_poll_query(DvzScene* scene, DvzQueryResult* out_result)
{
    ANN(scene);
    ANN(out_result);

    if (scene->query_result_count > 0)
    {
        uint32_t index = scene->query_result_head;
        *out_result = scene->query_results[index].result;
        dvz_memset(
            &scene->query_results[index], sizeof(DvzQueuedQueryResult), 0,
            sizeof(DvzQueuedQueryResult));
        scene->query_result_head = (index + 1) % DVZ_SCENE_MAX_QUERY_RESULTS;
        scene->query_result_count--;
        return true;
    }

    DvzPickResult pick = {0};
    if (dvz_scene_poll_pick(scene, &pick))
    {
        _dvz_scene_query_from_pick(&pick, out_result);
        return true;
    }

    DvzProbeResult probe = {0};
    if (dvz_scene_poll_probe(scene, &probe))
    {
        _dvz_scene_query_from_probe(&probe, out_result);
        return true;
    }
    return false;
}



/**
 * Fill a query result from one pick result.
 *
 * @param pick the pick result
 * @param out_result output query result
 */
void _dvz_scene_query_from_pick(const DvzPickResult* pick, DvzQueryResult* out_result)
{
    ANN(pick);
    ANN(out_result);
    *out_result = (DvzQueryResult){0};
    out_result->request_id = pick->request_id;
    out_result->status = _query_status_from_pick(pick->status);
    out_result->hit = pick->hit;
    out_result->panel_id = pick->panel_id;
    out_result->panel_position[0] = pick->panel_position[0];
    out_result->panel_position[1] = pick->panel_position[1];
    out_result->visual_id = pick->visual_id;
    out_result->visual_family = pick->visual_family;
    out_result->raw_parent_target = pick->raw_parent_target;
    out_result->raw_parent_id = pick->raw_parent_id;
    out_result->raw_target = pick->raw_target;
    out_result->raw_id = pick->raw_id;
    out_result->resolved_parent_target = pick->resolved_parent_target;
    out_result->resolved_parent_id = pick->resolved_parent_id;
    out_result->resolved_target = pick->resolved_target;
    out_result->resolved_id = pick->resolved_id;
    out_result->item_id = pick->item_id;
    out_result->group_id = pick->group_id;
    out_result->auxiliary_id = pick->auxiliary_id;
    out_result->instance_id = pick->instance_id;
    out_result->link_key = pick->link_key;
    out_result->has_data_position = pick->has_data_position;
    out_result->data_position[0] = pick->data_position[0];
    out_result->data_position[1] = pick->data_position[1];
    out_result->data_position[2] = pick->data_position[2];
    out_result->value_kind = DVZ_QUERY_VALUE_NONE;
}



/**
 * Fill a query result from one probe result.
 *
 * @param probe the probe result
 * @param out_result output query result
 */
void _dvz_scene_query_from_probe(const DvzProbeResult* probe, DvzQueryResult* out_result)
{
    ANN(probe);
    ANN(out_result);
    *out_result = (DvzQueryResult){0};
    out_result->request_id = probe->request_id;
    out_result->status = _query_status_from_probe(probe->status);
    out_result->hit = probe->hit;
    out_result->panel_id = probe->panel_id;
    out_result->panel_position[0] = probe->panel_position[0];
    out_result->panel_position[1] = probe->panel_position[1];
    out_result->visual_id = probe->visual_id;
    out_result->visual_family = probe->visual_family;
    out_result->raw_target = probe->target;
    out_result->raw_id = probe->target_id;
    out_result->resolved_target = probe->target;
    out_result->resolved_id = probe->target_id;
    out_result->item_id = probe->item_id;
    out_result->group_id = probe->group_id;
    out_result->auxiliary_id = probe->auxiliary_id;
    out_result->has_data_position = probe->has_coordinate;
    out_result->data_position[0] = probe->coordinate[0];
    out_result->data_position[1] = probe->coordinate[1];
    out_result->data_position[2] = probe->coordinate[2];
    out_result->has_uvw = probe->has_uvw;
    out_result->uvw[0] = probe->uvw[0];
    out_result->uvw[1] = probe->uvw[1];
    out_result->uvw[2] = probe->uvw[2];
    out_result->value_kind = _query_value_kind_from_probe(probe->value_kind);
    out_result->scalar = probe->scalar;
    out_result->vector[0] = probe->vector[0];
    out_result->vector[1] = probe->vector[1];
    out_result->vector[2] = probe->vector[2];
    out_result->vector[3] = probe->vector[3];
    out_result->category_id = probe->category_id;
    out_result->scale = probe->scale;
    dvz_snprintf(out_result->label, sizeof(out_result->label), "%s", probe->label);
    dvz_snprintf(out_result->unit, sizeof(out_result->unit), "%s", probe->unit);
}
