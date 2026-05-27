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



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

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
