/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene FramePlan graph resources                                                              */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_frame_plan_internal.h"
#include "_log.h"
#include "_overflow.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Ensure graph resource storage has room for one more descriptor.
 *
 * @param plan the FramePlan
 * @return whether storage is available
 */
static bool _ensure_graph_resource_capacity(DvzFramePlan* plan)
{
    ANN(plan);
    if (plan->graph_resources == NULL || plan->graph_resource_capacity == 0)
    {
        plan->graph_resource_capacity = DVZ_FRAME_PLAN_INITIAL_GRAPH_RESOURCE_CAPACITY;
        plan->graph_resources = (DvzFrameGraphResource*)dvz_calloc(
            plan->graph_resource_capacity, sizeof(DvzFrameGraphResource));
        return plan->graph_resources != NULL;
    }

    if (plan->graph_resource_count < plan->graph_resource_capacity)
        return true;

    if (plan->graph_resource_capacity > UINT32_MAX / 2)
        return false;
    uint32_t capacity = plan->graph_resource_capacity * 2;
    uint64_t bytes = 0;
    if (_dvz_mul_u64_overflows(capacity, sizeof(DvzFrameGraphResource), &bytes))
        return false;

    DvzFrameGraphResource* resources =
        (DvzFrameGraphResource*)dvz_realloc(plan->graph_resources, bytes);
    if (resources == NULL)
        return false;

    plan->graph_resource_capacity = capacity;
    plan->graph_resources = resources;
    return plan->graph_resources != NULL;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Append a typed graph resource descriptor.
 *
 * @param plan the FramePlan
 * @param resource the resource descriptor
 * @return whether the resource was appended
 */
bool dvz_frame_plan_graph_resource(DvzFramePlan* plan, const DvzFrameGraphResource* resource)
{
    if (plan == NULL || resource == NULL || resource->id[0] == '\0')
        return false;
    if (!_ensure_graph_resource_capacity(plan))
    {
        log_error("cannot grow FramePlan graph resource list");
        return false;
    }

    DvzFrameGraphResource* dst = &plan->graph_resources[plan->graph_resource_count++];
    dvz_memset(dst, sizeof(DvzFrameGraphResource), 0, sizeof(DvzFrameGraphResource));
    dvz_memcpy(dst, sizeof(DvzFrameGraphResource), resource, sizeof(DvzFrameGraphResource));
    return true;
}



/**
 * Return the graph resource count.
 *
 * @param plan the FramePlan
 * @return the graph resource count
 */
uint32_t dvz_frame_plan_graph_resource_count(const DvzFramePlan* plan)
{
    if (plan == NULL)
        return 0;
    return plan->graph_resource_count;
}



/**
 * Return a graph resource descriptor.
 *
 * @param plan the FramePlan
 * @param index the graph resource index
 * @return the resource descriptor, or NULL when index is out of bounds
 */
const DvzFrameGraphResource*
dvz_frame_plan_graph_resource_get(const DvzFramePlan* plan, uint32_t index)
{
    if (plan == NULL || index >= plan->graph_resource_count)
        return NULL;
    return &plan->graph_resources[index];
}
