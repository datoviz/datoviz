/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Visual point-like query helpers                                                              */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "../query/internal.h"
#include "_visual_pipeline.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct
{
    const char* label;
    const char* plan_id;
    DvzVisualType metadata_visual_type;
    DvzSceneVisualDescKind desc_kind;
    DvzScenePointLikeKind point_like_kind;
} DvzScenePointLikeQueryDesc;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

bool _scene_query_point_like_build(
    const DvzSceneQueryBuildContext* ctx, const DvzScenePointLikeQueryDesc* desc,
    DvzSceneQueryPlan* out_plan);

bool _scene_query_point_like_build_ex(
    const DvzSceneQueryBuildContext* ctx, const char* label, const char* plan_id,
    DvzVisualType metadata_visual_type, DvzSceneVisualDescKind desc_kind,
    DvzScenePointLikeKind point_like_kind, DvzSceneQueryPlan* out_plan);

bool _scene_query_point_like_readout(
    const DvzSceneQueryReadoutContext* ctx, DvzQueryResult* result);
