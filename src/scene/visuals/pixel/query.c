/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Pixel query policy                                                                           */
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

/**
 * Return whether a pixel visual can answer one query request.
 *
 * @param panel the panel
 * @param visual the visual
 * @param request query request
 * @return true when the family should try the request
 */
static bool _pixel_query_eligible(
    const DvzPanel* panel, const DvzVisual* visual, const DvzQueryRequest* request)
{
    ANN(panel);
    return _dvz_scene_query_item_target_eligible(visual, request, DVZ_VISUAL_TYPE_PIXEL);
}



/**
 * Build a pixel-family r32uint item query plan.
 *
 * @param ctx build context
 * @param out_plan output query plan
 * @return true when the plan was assembled
 */
static bool _pixel_query_build(
    const DvzSceneQueryBuildContext* ctx, DvzSceneQueryPlan* out_plan)
{
    static const DvzScenePointLikeQueryDesc desc = {
        .label = "pixel",
        .plan_id = "figure.query.pixel",
        .metadata_visual_type = DVZ_VISUAL_TYPE_PIXEL,
        .desc_kind = DVZ_SCENE_VISUAL_DESC_PIXEL,
        .point_like_kind = DVZ_SCENE_POINT_LIKE_PIXEL,
    };
    return _scene_query_point_like_build(ctx, &desc, out_plan);
}



/**
 * Decode a pixel-family r32uint item query payload.
 *
 * @param ctx decode context
 * @param out_result output query result
 * @return true when a terminal result was produced
 */
static bool _pixel_query_decode(
    const DvzSceneQueryDecodeContext* ctx, DvzQueryResult* out_result)
{
    return _dvz_scene_query_decode_item_id(ctx, DVZ_SCENE_VISUAL_FAMILY_PIXEL, out_result);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return pixel visual query operations.
 *
 * @return query operation table
 */
const DvzSceneQueryFamilyOps* _dvz_scene_query_pixel_ops(void)
{
    static const DvzSceneQueryFamilyOps ops = {
        .name = "pixel",
        .family = DVZ_SCENE_VISUAL_FAMILY_PIXEL,
        .eligible = _pixel_query_eligible,
        .build = _pixel_query_build,
        .decode = _pixel_query_decode,
        .readout = _scene_query_point_like_readout,
    };
    return &ops;
}
