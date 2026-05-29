/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene query scratch storage                                                                  */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_alloc.h"
#include "internal.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Destroy a synthetic query frame plan wrapper.
 *
 * @param plan the plan wrapper
 */
void _scene_query_scratch_destroy(DvzSceneQueryScratch* plan)
{
    if (plan == NULL)
        return;
    dvz_frame_plan_destroy(plan->plan);
    plan->plan = NULL;
    dvz_free(plan->query_positions);
    plan->query_positions = NULL;
    dvz_free(plan->query_texcoords);
    plan->query_texcoords = NULL;
    dvz_free(plan->query_colors);
    plan->query_colors = NULL;
    dvz_free(plan->query_ids);
    plan->query_ids = NULL;
    dvz_free(plan->query_position_start);
    plan->query_position_start = NULL;
    dvz_free(plan->query_position_curr);
    plan->query_position_curr = NULL;
    dvz_free(plan->query_position_end);
    plan->query_position_end = NULL;
    dvz_free(plan->query_line_width);
    plan->query_line_width = NULL;
    dvz_free(plan->query_path_flags);
    plan->query_path_flags = NULL;
    dvz_free(plan->query_path_distance);
    plan->query_path_distance = NULL;
    dvz_free(plan->query_indices);
    plan->query_indices = NULL;
}
