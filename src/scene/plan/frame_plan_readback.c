/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene FramePlan readback nodes                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "_assertions.h"
#include "_frame_plan_internal.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Append a copy node.
 *
 * @param plan the FramePlan
 * @param src_resource_id the source resource id
 * @param dst_resource_id the destination resource id
 * @param byte_size the copy size in bytes
 * @return whether the node was appended
 */
bool dvz_frame_plan_copy(
    DvzFramePlan* plan, const char* src_resource_id, const char* dst_resource_id,
    uint64_t byte_size)
{
    DvzFramePlanCopyDesc desc = {
        .src_resource_id = src_resource_id,
        .dst_resource_id = dst_resource_id,
        .extent = {1, 1, 1},
        .bytes_per_texel = byte_size > UINT32_MAX ? UINT32_MAX : (uint32_t)byte_size,
        .bytes_per_row = byte_size,
        .rows_per_image = 1,
        .byte_size = byte_size,
    };
    return dvz_frame_plan_copy_ex(plan, &desc);
}



/**
 * Append an explicit texture-to-buffer copy node.
 *
 * @param plan the FramePlan
 * @param desc the copy descriptor
 * @return whether the node was appended
 */
bool dvz_frame_plan_copy_ex(DvzFramePlan* plan, const DvzFramePlanCopyDesc* desc)
{
    ANN(desc);
    DvzFramePlanNode* node = _frame_plan_append_node(plan, DVZ_FRAME_PLAN_NODE_COPY);
    if (node == NULL)
        return false;
    _frame_plan_copy_label(
        node->u.copy.src_resource_id, DVZ_SCENE_LABEL_SIZE,
        desc->src_resource_id ? desc->src_resource_id : "");
    _frame_plan_copy_label(
        node->u.copy.dst_resource_id, DVZ_SCENE_LABEL_SIZE,
        desc->dst_resource_id ? desc->dst_resource_id : "");
    node->u.copy.src_attachment_index = desc->src_attachment_index;
    node->u.copy.src_origin[0] = desc->src_origin[0];
    node->u.copy.src_origin[1] = desc->src_origin[1];
    node->u.copy.src_origin[2] = desc->src_origin[2];
    node->u.copy.extent[0] = desc->extent[0] != 0 ? desc->extent[0] : 1;
    node->u.copy.extent[1] = desc->extent[1] != 0 ? desc->extent[1] : 1;
    node->u.copy.extent[2] = desc->extent[2] != 0 ? desc->extent[2] : 1;
    node->u.copy.format = desc->format;
    node->u.copy.bytes_per_texel = desc->bytes_per_texel;
    node->u.copy.bytes_per_row =
        desc->bytes_per_row != 0 ? desc->bytes_per_row : desc->byte_size;
    node->u.copy.rows_per_image = desc->rows_per_image != 0 ? desc->rows_per_image : 1;
    node->u.copy.dst_offset = desc->dst_offset;
    node->u.copy.byte_size = desc->byte_size;
    node->u.copy.request_id = desc->request_id;
    return true;
}



/**
 * Append a readback node.
 *
 * @param plan the FramePlan
 * @param resource_id the resource id
 * @param request_id the request id
 * @return whether the node was appended
 */
bool dvz_frame_plan_readback(DvzFramePlan* plan, const char* resource_id, const char* request_id)
{
    DvzFramePlanNode* node = _frame_plan_append_node(plan, DVZ_FRAME_PLAN_NODE_READBACK);
    if (node == NULL)
        return false;
    _frame_plan_copy_label(
        node->u.readback.resource_id, DVZ_SCENE_LABEL_SIZE, resource_id ? resource_id : "");
    _frame_plan_copy_label(
        node->u.readback.request_id, DVZ_SCENE_LABEL_SIZE, request_id ? request_id : "");
    return true;
}
