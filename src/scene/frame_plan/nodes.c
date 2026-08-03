/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene FramePlan nodes                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "internal.h"
#include "_log.h"
#include "_overflow.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Ensure FramePlan node storage has room for one more node.
 *
 * @param plan the FramePlan
 * @return whether storage is available
 */
static bool _ensure_node_capacity(DvzFramePlan* plan)
{
    ANN(plan);
    if (plan->nodes == NULL || plan->capacity == 0)
    {
        plan->capacity = DVZ_FRAME_PLAN_INITIAL_NODE_CAPACITY;
        plan->nodes = (DvzFramePlanNode*)dvz_calloc(plan->capacity, sizeof(DvzFramePlanNode));
        return plan->nodes != NULL;
    }

    if (plan->count < plan->capacity)
        return true;

    if (plan->capacity > UINT32_MAX / 2)
        return false;
    uint32_t capacity = plan->capacity * 2;
    uint64_t bytes = 0;
    if (_dvz_mul_u64_overflows(capacity, sizeof(DvzFramePlanNode), &bytes))
        return false;

    DvzFramePlanNode* nodes = (DvzFramePlanNode*)dvz_realloc(plan->nodes, bytes);
    if (nodes == NULL)
        return false;

    plan->capacity = capacity;
    plan->nodes = nodes;
    return plan->nodes != NULL;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Append a zero-initialized node to a FramePlan.
 *
 * @param plan the FramePlan
 * @param type the node type
 * @return the appended node, or NULL on failure
 */
DvzFramePlanNode* _frame_plan_append_node(DvzFramePlan* plan, DvzFramePlanNodeType type)
{
    if (plan == NULL)
    {
        log_error("cannot append FramePlan node to a null plan");
        return NULL;
    }
    if (!_ensure_node_capacity(plan))
    {
        log_error("cannot grow FramePlan node list");
        return NULL;
    }

    DvzFramePlanNode* node = &plan->nodes[plan->count++];
    dvz_memset(node, sizeof(DvzFramePlanNode), 0, sizeof(DvzFramePlanNode));
    node->type = type;
    return node;
}



/**
 * Return the most recently appended node when it has the expected type.
 *
 * @param plan the FramePlan
 * @param type the expected node type
 * @return the last node, or NULL when absent or of another type
 */
DvzFramePlanNode* _frame_plan_last_node(DvzFramePlan* plan, DvzFramePlanNodeType type)
{
    if (plan == NULL || plan->count == 0)
        return NULL;

    DvzFramePlanNode* node = &plan->nodes[plan->count - 1];
    if (node->type != type)
        return NULL;
    return node;
}



/**
 * Return a FramePlan node count.
 *
 * @param plan the FramePlan
 * @return the node count
 */
uint32_t dvz_frame_plan_node_count(const DvzFramePlan* plan)
{
    if (plan == NULL)
        return 0;
    return plan->count;
}



/**
 * Return a FramePlan node.
 *
 * @param plan the FramePlan
 * @param index the node index
 * @return the node, or NULL when index is out of bounds
 */
const DvzFramePlanNode* dvz_frame_plan_node_get(const DvzFramePlan* plan, uint32_t index)
{
    if (plan == NULL || index >= plan->count)
        return NULL;
    return &plan->nodes[index];
}



/**
 * Return a FramePlan node type.
 *
 * @param node the FramePlan node
 * @return the node type
 */
DvzFramePlanNodeType dvz_frame_plan_node_type(const DvzFramePlanNode* node)
{
    if (node == NULL)
        return DVZ_FRAME_PLAN_NODE_NONE;
    return node->type;
}



/**
 * Return a FramePlan render node pass role.
 *
 * @param node the FramePlan node
 * @return the render pass role, or opaque for non-render nodes
 */
DvzFramePlanRenderPassRole _frame_plan_render_pass_role(const DvzFramePlanNode* node)
{
    if (node == NULL || node->type != DVZ_FRAME_PLAN_NODE_RENDER)
        return DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE;
    return node->u.render.pass_role;
}
