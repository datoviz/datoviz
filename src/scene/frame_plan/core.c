/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene FramePlan                                                                              */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "frame_plan/frame_plan.h"
#include "internal.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Copy a FramePlan label into a fixed-size destination buffer.
 *
 * @param dst destination label buffer
 * @param dst_size destination buffer size
 * @param src source label string
 */
void _frame_plan_copy_label(char* dst, uint64_t dst_size, const char* src)
{
    ANN(dst);
    ANN(src);
    if (dst_size == 0)
        return;
    size_t len = strlen(src);
    if (len >= dst_size)
        len = (size_t)dst_size - 1;
    memcpy(dst, src, len);
    dst[len] = '\0';
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create an empty FramePlan.
 *
 * @param figure_id the figure id
 * @param frame_index the frame index
 * @return the FramePlan
 */
DvzFramePlan* dvz_frame_plan(const char* figure_id, uint64_t frame_index)
{
    DvzFramePlan* plan = (DvzFramePlan*)dvz_calloc(1, sizeof(DvzFramePlan));
    if (plan == NULL)
        return NULL;
    _frame_plan_copy_label(plan->figure_id, DVZ_SCENE_LABEL_SIZE, figure_id ? figure_id : "");
    plan->frame_index = frame_index;
    plan->capacity = DVZ_FRAME_PLAN_INITIAL_NODE_CAPACITY;
    plan->nodes = (DvzFramePlanNode*)dvz_calloc(plan->capacity, sizeof(DvzFramePlanNode));
    if (plan->nodes == NULL)
    {
        dvz_free(plan);
        return NULL;
    }
    return plan;
}



/**
 * Destroy a FramePlan.
 *
 * @param plan the FramePlan
 */
void dvz_frame_plan_destroy(DvzFramePlan* plan)
{
    if (plan == NULL)
        return;
    for (uint32_t i = 0; i < plan->count; i++)
    {
        if (plan->nodes[i].type == DVZ_FRAME_PLAN_NODE_UPLOAD)
        {
            dvz_free(plan->nodes[i].u.upload.owned_data);
            plan->nodes[i].u.upload.owned_data = NULL;
        }
    }
    dvz_free(plan->nodes);
    dvz_free(plan->graph_resources);
    dvz_free(plan->graph_passes);
    dvz_free(plan);
}
