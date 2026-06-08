/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Point query policy                                                                           */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "../../query/internal.h"
#include "_assertions.h"
#include "query_point_like.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static const DvzScenePointLikeQueryDesc POINT_QUERY_DESC = {
    .label = "point",
    .plan_id = "figure.query.point",
    .metadata_visual_type = DVZ_VISUAL_TYPE_POINT,
    .desc_kind = DVZ_SCENE_VISUAL_DESC_POINT,
    .point_like_kind = DVZ_SCENE_POINT_LIKE_POINT,
};



/**
 * Return whether a point visual can answer one query request.
 *
 * @param panel the panel
 * @param visual the visual
 * @param request query request
 * @return true when the family should try the request
 */
static bool _point_query_eligible(
    const DvzPanel* panel, const DvzVisual* visual, const DvzQueryRequest* request)
{
    ANN(panel);
    return _dvz_scene_query_item_target_eligible(visual, request, DVZ_VISUAL_TYPE_POINT);
}



/**
 * Build a point-family r32uint item query plan.
 *
 * @param ctx build context
 * @param out_plan output query plan
 * @return true when the plan was assembled
 */
bool _point_query_build(
    const DvzSceneQueryBuildContext* ctx, DvzSceneQueryPlan* out_plan)
{
    return _scene_query_point_like_build(ctx, &POINT_QUERY_DESC, out_plan);
}



/**
 * Decode a point-family r32uint item query payload.
 *
 * @param ctx decode context
 * @param out_result output query result
 * @return true when a terminal result was produced
 */
bool _point_query_decode(
    const DvzSceneQueryDecodeContext* ctx, DvzQueryResult* out_result)
{
    return _dvz_scene_query_decode_item_id(ctx, DVZ_SCENE_VISUAL_FAMILY_POINT, out_result);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return point visual query operations.
 *
 * @return query operation table
 */
const DvzSceneQueryFamilyOps* _dvz_scene_query_point_ops(void)
{
    static const DvzSceneQueryFamilyOps ops = {
        .name = "point",
        .family = DVZ_SCENE_VISUAL_FAMILY_POINT,
        .eligible = _point_query_eligible,
        .build = _point_query_build,
        .decode = _point_query_decode,
        .readout = _scene_query_point_like_readout,
    };
    return &ops;
}
