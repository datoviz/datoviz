/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Marker query policy                                                                          */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "../../query/internal.h"
#include "_assertions.h"
#include "query_point_like.h"
#include "registry/registry.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return whether a marker visual can answer one query request.
 *
 * @param panel the panel
 * @param visual the visual
 * @param request query request
 * @return true when the family should try the request
 */
static bool _marker_query_eligible(
    const DvzPanel* panel, const DvzVisual* visual, const DvzQueryRequest* request)
{
    ANN(panel);
    return _dvz_scene_query_item_target_eligible(visual, request, DVZ_VISUAL_TYPE_MARKER);
}



/**
 * Build a marker-family r32uint item query plan.
 *
 * @param ctx build context
 * @param out_plan output query plan
 * @return true when the plan was assembled
 */
static bool _marker_query_build(
    const DvzSceneQueryBuildContext* ctx, DvzSceneQueryPlan* out_plan)
{
    DvzScenePointLikeQueryDesc desc = {
        .label = "marker",
        .plan_id = "figure.query.marker",
        .metadata_visual_type = DVZ_VISUAL_TYPE_PIXEL,
        .point_like_kind = DVZ_SCENE_POINT_LIKE_PIXEL,
    };
    desc.desc_kind = _scene_visual_family_desc_kind(DVZ_VISUAL_TYPE_PIXEL);
    return _scene_query_point_like_build(ctx, &desc, out_plan);
}



/**
 * Decode a marker-family r32uint item query payload.
 *
 * @param ctx decode context
 * @param out_result output query result
 * @return true when a terminal result was produced
 */
static bool _marker_query_decode(
    const DvzSceneQueryDecodeContext* ctx, DvzQueryResult* out_result)
{
    return _dvz_scene_query_decode_item_id(ctx, DVZ_SCENE_VISUAL_FAMILY_MARKER, out_result);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return marker visual query operations.
 *
 * @return query operation table
 */
const DvzSceneQueryFamilyOps* _dvz_scene_query_marker_ops(void)
{
    static const DvzSceneQueryFamilyOps ops = {
        .name = "marker",
        .family = DVZ_SCENE_VISUAL_FAMILY_MARKER,
        .eligible = _marker_query_eligible,
        .build = _marker_query_build,
        .decode = _marker_query_decode,
        .readout = _scene_query_point_like_readout,
    };
    return &ops;
}
