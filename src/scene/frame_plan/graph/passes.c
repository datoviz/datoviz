/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene FramePlan graph passes                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "frame_plan/internal.h"
#include "internal.h"
#include "_log.h"
#include "_overflow.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Ensure graph pass storage has room for one more descriptor.
 *
 * @param plan the FramePlan
 * @return whether storage is available
 */
static bool _ensure_graph_pass_capacity(DvzFramePlan* plan)
{
    ANN(plan);
    if (plan->graph_passes == NULL || plan->graph_pass_capacity == 0)
    {
        plan->graph_pass_capacity = DVZ_FRAME_PLAN_INITIAL_GRAPH_PASS_CAPACITY;
        plan->graph_passes = (DvzFrameGraphPass*)dvz_calloc(
            plan->graph_pass_capacity, sizeof(DvzFrameGraphPass));
        return plan->graph_passes != NULL;
    }

    if (plan->graph_pass_count < plan->graph_pass_capacity)
        return true;

    if (plan->graph_pass_capacity > UINT32_MAX / 2)
        return false;
    uint32_t capacity = plan->graph_pass_capacity * 2;
    uint64_t bytes = 0;
    if (_dvz_mul_u64_overflows(capacity, sizeof(DvzFrameGraphPass), &bytes))
        return false;

    DvzFrameGraphPass* passes = (DvzFrameGraphPass*)dvz_realloc(plan->graph_passes, bytes);
    if (passes == NULL)
        return false;

    plan->graph_pass_capacity = capacity;
    plan->graph_passes = passes;
    return plan->graph_passes != NULL;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Append a typed graph pass descriptor.
 *
 * @param plan the FramePlan
 * @param pass the pass descriptor
 * @return whether the pass was appended
 */
bool dvz_frame_plan_graph_pass(DvzFramePlan* plan, const DvzFrameGraphPass* pass)
{
    if (plan == NULL || pass == NULL || pass->id[0] == '\0')
        return false;
    if (!_ensure_graph_pass_capacity(plan))
    {
        log_error("cannot grow FramePlan graph pass list");
        return false;
    }

    DvzFrameGraphPass* dst = &plan->graph_passes[plan->graph_pass_count++];
    dvz_memset(dst, sizeof(DvzFrameGraphPass), 0, sizeof(DvzFrameGraphPass));
    dvz_memcpy(dst, sizeof(DvzFrameGraphPass), pass, sizeof(DvzFrameGraphPass));
    return true;
}



/**
 * Return the graph pass count.
 *
 * @param plan the FramePlan
 * @return the graph pass count
 */
uint32_t dvz_frame_plan_graph_pass_count(const DvzFramePlan* plan)
{
    if (plan == NULL)
        return 0;
    return plan->graph_pass_count;
}



/**
 * Return a graph pass descriptor.
 *
 * @param plan the FramePlan
 * @param index the graph pass index
 * @return the pass descriptor, or NULL when index is out of bounds
 */
const DvzFrameGraphPass* dvz_frame_plan_graph_pass_get(const DvzFramePlan* plan, uint32_t index)
{
    if (plan == NULL || index >= plan->graph_pass_count)
        return NULL;
    return &plan->graph_passes[index];
}



/**
 * Add a declared read access to a graph pass descriptor.
 *
 * @param pass the graph pass descriptor
 * @param resource_id the resource id
 * @param usage the resource access usage
 * @return whether the read access was appended
 */
bool dvz_frame_graph_pass_read(
    DvzFrameGraphPass* pass, const char* resource_id, DvzFrameGraphAccessUsage usage)
{
    if (pass == NULL || resource_id == NULL || resource_id[0] == '\0' ||
        pass->read_count >= DVZ_FRAME_PLAN_MAX_GRAPH_ACCESSES)
        return false;

    DvzFrameGraphAccess* access = &pass->reads[pass->read_count++];
    dvz_memset(access, sizeof(DvzFrameGraphAccess), 0, sizeof(DvzFrameGraphAccess));
    _frame_plan_copy_label(access->resource_id, DVZ_SCENE_LABEL_SIZE, resource_id);
    access->usage = usage;
    return true;
}



/**
 * Add a declared write access to a graph pass descriptor.
 *
 * @param pass the graph pass descriptor
 * @param resource_id the resource id
 * @param usage the resource access usage
 * @return whether the write access was appended
 */
bool dvz_frame_graph_pass_write(
    DvzFrameGraphPass* pass, const char* resource_id, DvzFrameGraphAccessUsage usage)
{
    if (pass == NULL || resource_id == NULL || resource_id[0] == '\0' ||
        pass->write_count >= DVZ_FRAME_PLAN_MAX_GRAPH_ACCESSES)
        return false;

    DvzFrameGraphAccess* access = &pass->writes[pass->write_count++];
    dvz_memset(access, sizeof(DvzFrameGraphAccess), 0, sizeof(DvzFrameGraphAccess));
    _frame_plan_copy_label(access->resource_id, DVZ_SCENE_LABEL_SIZE, resource_id);
    access->usage = usage;
    return true;
}



/**
 * Add a color attachment to a graph pass descriptor.
 *
 * @param pass the graph pass descriptor
 * @param attachment the color attachment descriptor
 * @return whether the color attachment was appended
 */
bool dvz_frame_graph_pass_color_attachment(
    DvzFrameGraphPass* pass, const DvzFrameGraphAttachment* attachment)
{
    if (pass == NULL || attachment == NULL || attachment->resource_id[0] == '\0' ||
        pass->color_attachment_count >= DVZ_FRAME_PLAN_MAX_GRAPH_COLOR_ATTACHMENTS)
        return false;

    DvzFrameGraphAttachment* dst = &pass->color_attachments[pass->color_attachment_count++];
    dvz_memset(dst, sizeof(DvzFrameGraphAttachment), 0, sizeof(DvzFrameGraphAttachment));
    dvz_memcpy(dst, sizeof(DvzFrameGraphAttachment), attachment, sizeof(DvzFrameGraphAttachment));
    return true;
}



/**
 * Set the depth attachment on a graph pass descriptor.
 *
 * @param pass the graph pass descriptor
 * @param attachment the depth attachment descriptor
 * @return whether the depth attachment was set
 */
bool dvz_frame_graph_pass_depth_attachment(
    DvzFrameGraphPass* pass, const DvzFrameGraphAttachment* attachment)
{
    if (pass == NULL || attachment == NULL || attachment->resource_id[0] == '\0')
        return false;

    dvz_memcpy(
        &pass->depth_attachment, sizeof(DvzFrameGraphAttachment), attachment,
        sizeof(DvzFrameGraphAttachment));
    pass->has_depth_attachment = true;
    return true;
}
