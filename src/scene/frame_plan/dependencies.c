/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene FramePlan dependencies                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "internal.h"
#include "graph/internal.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return the number of graph pass dependencies inferred from resource accesses.
 *
 * @param plan the FramePlan
 * @return the dependency count
 */
uint32_t dvz_frame_plan_graph_dependency_count(const DvzFramePlan* plan)
{
    if (plan == NULL)
        return 0;
    uint32_t count = 0;
    for (uint32_t i = 0; i < plan->graph_pass_count; i++)
        count += _frame_plan_graph_pass_dependency_count(plan, &plan->graph_passes[i], i, UINT32_MAX, NULL);
    return count;
}



/**
 * Return one graph pass dependency inferred from resource accesses.
 *
 * @param plan the FramePlan
 * @param index the dependency index
 * @param out output dependency descriptor
 * @return whether the dependency exists
 */
bool dvz_frame_plan_graph_dependency_get(
    const DvzFramePlan* plan, uint32_t index, DvzFrameGraphDependency* out)
{
    if (plan == NULL || out == NULL)
        return false;
    uint32_t base = 0;
    for (uint32_t i = 0; i < plan->graph_pass_count; i++)
    {
        uint32_t count =
            _frame_plan_graph_pass_dependency_count(plan, &plan->graph_passes[i], i, UINT32_MAX, NULL);
        if (index < base + count)
        {
            (void)_frame_plan_graph_pass_dependency_count(
                plan, &plan->graph_passes[i], i, index - base, out);
            return true;
        }
        base += count;
    }
    return false;
}
