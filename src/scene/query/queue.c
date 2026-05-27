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
#include "_assertions.h"
#include "_compat.h"
#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return whether a query target is value-oriented.
 *
 * @param target the requested scene target
 * @return whether the transitional bridge should use the probe path
 */
static bool _query_target_uses_probe(DvzSceneTargetKind target)
{
    return target == DVZ_SCENE_TARGET_PIXEL || target == DVZ_SCENE_TARGET_SAMPLE ||
           target == DVZ_SCENE_TARGET_SEGMENT;
}



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
 * Fill a query result from one pick result.
 *
 * @param pick the pick result
 * @param out_result output query result
 */
static void _query_from_pick(const DvzPickResult* pick, DvzQueryResult* out_result)
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
static void _query_from_probe(const DvzProbeResult* probe, DvzQueryResult* out_result)
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
    DvzQueryRequest local = {0};
    if (request != NULL)
        local = *request;

    if (_query_target_uses_probe(local.target))
    {
        DvzProbeRequest probe = {
            .request_id = local.request_id,
            .target = local.target,
            .flags = local.flags,
        };
        return dvz_panel_probe(panel, x, y, &probe);
    }

    DvzPickRequest pick = {
        .request_id = local.request_id,
        .target = local.target,
        .hit_policy = local.hit_policy,
        .flags = local.flags,
    };
    return dvz_panel_pick(panel, x, y, &pick);
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

    DvzPickResult pick = {0};
    if (dvz_scene_poll_pick(scene, &pick))
    {
        _query_from_pick(&pick, out_result);
        return true;
    }

    DvzProbeResult probe = {0};
    if (dvz_scene_poll_probe(scene, &probe))
    {
        _query_from_probe(&probe, out_result);
        return true;
    }
    return false;
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
    return dvz_figure_process_requests(figure, runtime, caps);
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
