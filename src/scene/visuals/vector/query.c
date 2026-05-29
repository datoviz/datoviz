/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Vector visual query support                                                                  */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_assertions.h"
#include "_visual_internal.h"
#include "../../query/internal.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return whether a vector visual has a retained dense displacement attribute.
 *
 * @param visual the vector visual
 * @return true when straight-vector lowering should be used
 */
static bool _vector_query_has_dense_vector(const DvzVisual* visual)
{
    ANN(visual);
    int idx = _attr_index(visual, "vector");
    if (idx < 0)
        return false;
    const DvzVisualAttr* attr = &visual->attrs[idx];
    return attr->data != NULL && attr->item_count > 0;
}



/**
 * Return the query operation table that owns the current vector lowering mode.
 *
 * @param visual the vector visual
 * @return segment or path query operations
 */
static const DvzSceneQueryFamilyOps* _vector_query_delegate(const DvzVisual* visual)
{
    ANN(visual);
    return _vector_query_has_dense_vector(visual) ? _dvz_scene_query_segment_ops()
                                                 : _dvz_scene_query_path_ops();
}



/**
 * Return whether a vector visual can answer one query request.
 *
 * @param panel the panel
 * @param visual the visual
 * @param request query request
 * @return true when the vector family should try the request
 */
static bool _vector_query_eligible(
    const DvzPanel* panel, const DvzVisual* visual, const DvzQueryRequest* request)
{
    ANN(panel);
    ANN(visual);
    ANN(request);
    if (visual->type != DVZ_VISUAL_TYPE_VECTOR)
        return false;
    const DvzSceneQueryFamilyOps* delegate = _vector_query_delegate(visual);
    return delegate != NULL && delegate->eligible != NULL &&
           delegate->eligible(panel, visual, request);
}



/**
 * Build a vector item query plan through the active vector lowering mode.
 *
 * @param ctx build context
 * @param out_plan output query plan
 * @return true when the delegated plan was assembled
 */
static bool _vector_query_build(
    const DvzSceneQueryBuildContext* ctx, DvzSceneQueryPlan* out_plan)
{
    ANN(ctx);
    ANN(ctx->visual);
    ANN(out_plan);
    const DvzSceneQueryFamilyOps* delegate = _vector_query_delegate(ctx->visual);
    return delegate != NULL && delegate->build != NULL && delegate->build(ctx, out_plan);
}



/**
 * Decode a vector item query payload.
 *
 * @param ctx decode context
 * @param out_result output query result
 * @return true when a terminal result was produced
 */
static bool _vector_query_decode(
    const DvzSceneQueryDecodeContext* ctx, DvzQueryResult* out_result)
{
    ANN(ctx);
    ANN(ctx->build);
    ANN(ctx->build->visual);
    ANN(out_result);
    return _dvz_scene_query_decode_item_id(ctx, DVZ_SCENE_VISUAL_FAMILY_VECTOR, out_result);
}



/**
 * Complete vector readout fields through the active vector lowering mode.
 *
 * @param ctx readout context
 * @param result query result
 * @return true when readout succeeded
 */
static bool _vector_query_readout(
    const DvzSceneQueryReadoutContext* ctx, DvzQueryResult* result)
{
    ANN(ctx);
    ANN(ctx->build);
    ANN(ctx->build->visual);
    ANN(result);
    const DvzSceneQueryFamilyOps* delegate = _vector_query_delegate(ctx->build->visual);
    if (delegate == NULL || delegate->readout == NULL)
        return true;
    return delegate->readout(ctx, result);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return vector visual query operations.
 *
 * @return query operation table
 */
const DvzSceneQueryFamilyOps* _dvz_scene_query_vector_ops(void)
{
    static const DvzSceneQueryFamilyOps ops = {
        .name = "vector",
        .family = DVZ_SCENE_VISUAL_FAMILY_VECTOR,
        .eligible = _vector_query_eligible,
        .build = _vector_query_build,
        .decode = _vector_query_decode,
        .readout = _vector_query_readout,
    };
    return &ops;
}
