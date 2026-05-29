/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Segment query policy                                                                         */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_visual_internal.h"
#include "stroke/internal.h"
#include "../../query/internal.h"
#include "_assertions.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return whether a segment visual can answer one query request.
 *
 * @param panel the panel
 * @param visual the visual
 * @param request query request
 * @return true when the family should try the request
 */
static bool _segment_query_eligible(
    const DvzPanel* panel, const DvzVisual* visual, const DvzQueryRequest* request)
{
    ANN(panel);
    ANN(visual);
    ANN(request);
    if (visual->type != DVZ_VISUAL_TYPE_SEGMENT)
    {
        if (visual->type != DVZ_VISUAL_TYPE_VECTOR)
            return false;
        int vector_idx = _attr_index(visual, "vector");
        if (vector_idx < 0 || visual->attrs[vector_idx].data == NULL ||
            visual->attrs[vector_idx].item_count == 0)
        {
            return false;
        }
    }
    if (request->target != DVZ_SCENE_TARGET_NONE && request->target != DVZ_SCENE_TARGET_ITEM &&
        request->target != DVZ_SCENE_TARGET_OBJECT &&
        request->target != DVZ_SCENE_TARGET_SEGMENT)
    {
        return false;
    }
    return (visual->query_capabilities & DVZ_QUERY_CAPABILITY_ITEM) != 0;
}



/**
 * Build a segment-family r32uint item query plan.
 *
 * @param ctx build context
 * @param out_plan output query plan
 * @return true when the plan was assembled
 */
static bool _segment_query_build(
    const DvzSceneQueryBuildContext* ctx, DvzSceneQueryPlan* out_plan)
{
    static const DvzStrokeQueryDesc desc = {
        .label = "segment",
        .plan_id = "figure.query.segment",
        .metadata_visual_type = DVZ_VISUAL_TYPE_SEGMENT,
        .renderable_kind = DVZ_RENDERABLE_STROKE_QUAD,
        .desc_kind = DVZ_SCENE_VISUAL_DESC_SEGMENT,
        .path_stroke = false,
        .geometry = _stroke_quad_query_geometry,
    };
    return _stroke_query_build(ctx, &desc, out_plan);
}



/**
 * Decode a segment-family r32uint item query payload.
 *
 * @param ctx decode context
 * @param out_result output query result
 * @return true when a terminal result was produced
 */
static bool _segment_query_decode(
    const DvzSceneQueryDecodeContext* ctx, DvzQueryResult* out_result)
{
    ANN(ctx);
    ANN(ctx->build);
    ANN(ctx->build->visual);
    return _dvz_scene_query_decode_item_id(ctx, DVZ_SCENE_VISUAL_FAMILY_SEGMENT, out_result);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return segment visual query operations.
 *
 * @return query operation table
 */
const DvzSceneQueryFamilyOps* _dvz_scene_query_segment_ops(void)
{
    static const DvzSceneQueryFamilyOps ops = {
        .name = "segment",
        .family = DVZ_SCENE_VISUAL_FAMILY_SEGMENT,
        .eligible = _segment_query_eligible,
        .build = _segment_query_build,
        .decode = _segment_query_decode,
        .readout = _stroke_query_readout,
    };
    return &ops;
}
