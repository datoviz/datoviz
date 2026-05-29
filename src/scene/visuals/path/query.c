/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Path query policy                                                                            */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
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
 * Return whether a path visual can answer one query request.
 *
 * @param panel the panel
 * @param visual the visual
 * @param request query request
 * @return true when the family should try the request
 */
static bool _path_query_eligible(
    const DvzPanel* panel, const DvzVisual* visual, const DvzQueryRequest* request)
{
    ANN(panel);
    ANN(visual);
    ANN(request);
    if (visual->type != DVZ_VISUAL_TYPE_PATH)
    {
        if (visual->type != DVZ_VISUAL_TYPE_VECTOR)
            return false;
        int vector_idx = _attr_index(visual, "vector");
        if (vector_idx >= 0 && visual->attrs[vector_idx].data != NULL &&
            visual->attrs[vector_idx].item_count > 0)
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
    int width_idx = _attr_index(visual, "line_width");
    if (width_idx < 0)
        return false;
    const DvzVisualAttr* width = &visual->attrs[width_idx];
    if (width->data == NULL || width->item_count == 0 || width->item_size != sizeof(float))
        return false;
    return (visual->query_capabilities & DVZ_QUERY_CAPABILITY_ITEM) != 0;
}



/**
 * Build a path-family r32uint item query plan.
 *
 * @param ctx build context
 * @param out_plan output query plan
 * @return true when the plan was assembled
 */
static bool _path_query_build(const DvzSceneQueryBuildContext* ctx, DvzSceneQueryPlan* out_plan)
{
    static const DvzStrokeQueryDesc desc = {
        .label = "path",
        .plan_id = "figure.query.path",
        .metadata_visual_type = DVZ_VISUAL_TYPE_PATH,
        .renderable_kind = DVZ_RENDERABLE_PATH_STROKE,
        .desc_kind = DVZ_SCENE_VISUAL_DESC_PATH,
        .path_stroke = true,
        .geometry = _path_stroke_query_geometry,
    };
    return _stroke_query_build(ctx, &desc, out_plan);
}



/**
 * Decode a path-family r32uint item query payload.
 *
 * @param ctx decode context
 * @param out_result output query result
 * @return true when a terminal result was produced
 */
static bool _path_query_decode(const DvzSceneQueryDecodeContext* ctx, DvzQueryResult* out_result)
{
    ANN(ctx);
    ANN(ctx->build);
    ANN(ctx->build->visual);
    return _dvz_scene_query_decode_item_id(ctx, DVZ_SCENE_VISUAL_FAMILY_PATH, out_result);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return path visual query operations.
 *
 * @return query operation table
 */
const DvzSceneQueryFamilyOps* _dvz_scene_query_path_ops(void)
{
    static const DvzSceneQueryFamilyOps ops = {
        .name = "path",
        .family = DVZ_SCENE_VISUAL_FAMILY_PATH,
        .eligible = _path_query_eligible,
        .build = _path_query_build,
        .decode = _path_query_decode,
        .readout = _stroke_query_readout,
    };
    return &ops;
}
