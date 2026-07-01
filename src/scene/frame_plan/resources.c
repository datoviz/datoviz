/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene FramePlan upload resources                                                             */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "_alloc.h"
#include "internal.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Append an upload node.
 *
 * @param plan the FramePlan
 * @param resource_id the resource id
 * @param byte_offset the byte offset
 * @param byte_size the byte size
 * @param data_tag the debug data tag
 * @return whether the node was appended
 */
bool dvz_frame_plan_upload(
    DvzFramePlan* plan, const char* resource_id, uint64_t byte_offset, uint64_t byte_size,
    const char* data_tag)
{
    return dvz_frame_plan_upload_bytes(plan, resource_id, byte_offset, byte_size, data_tag, NULL);
}



/**
 * Append an upload node with actual data.
 *
 * @param plan the FramePlan
 * @param resource_id the resource id
 * @param byte_offset the byte offset
 * @param byte_size the byte size
 * @param data_tag the debug data tag
 * @param data pointer to upload data
 * @return whether the node was appended
 */
bool dvz_frame_plan_upload_bytes(
    DvzFramePlan* plan, const char* resource_id, uint64_t byte_offset, uint64_t byte_size,
    const char* data_tag, const void* data)
{
    DvzFramePlanNode* node = _frame_plan_append_node(plan, DVZ_FRAME_PLAN_NODE_UPLOAD);
    if (node == NULL)
        return false;
    _frame_plan_copy_label(
        node->u.upload.resource_id, DVZ_SCENE_LABEL_SIZE, resource_id ? resource_id : "");
    node->u.upload.byte_offset = byte_offset;
    node->u.upload.byte_size = byte_size;
    _frame_plan_copy_label(
        node->u.upload.data_tag, DVZ_SCENE_LABEL_SIZE, data_tag ? data_tag : "");
    node->u.upload.data = data;
    node->u.upload.topology = UINT32_MAX;
    return true;
}



/**
 * Tag the most recent upload with primitive topology.
 *
 * @param plan the FramePlan
 * @param topology the primitive topology, or UINT32_MAX
 * @return whether the topology was applied
 */
bool dvz_frame_plan_upload_set_topology(DvzFramePlan* plan, uint32_t topology)
{
    if (plan == NULL || plan->count == 0)
        return false;
    DvzFramePlanNode* node = &plan->nodes[plan->count - 1];
    if (node->type != DVZ_FRAME_PLAN_NODE_UPLOAD)
        return false;
    node->u.upload.topology = topology;
    return true;
}



/**
 * Set the 2D texture write extent on the most recent upload.
 *
 * @param plan the FramePlan
 * @param width written texture-region width
 * @param height written texture-region height
 * @return whether the extent was applied
 */
bool dvz_frame_plan_upload_set_texture_extent(
    DvzFramePlan* plan, uint32_t width, uint32_t height)
{
    if (plan == NULL || plan->count == 0)
        return false;
    DvzFramePlanNode* node = &plan->nodes[plan->count - 1];
    if (node->type != DVZ_FRAME_PLAN_NODE_UPLOAD)
        return false;
    node->u.upload.texture_width  = width;
    node->u.upload.texture_height = height;
    node->u.upload.texture_depth  = 1;
    return true;
}



/**
 * Mark the most recently appended upload node as a 3D texture write.
 *
 * @param plan the FramePlan
 * @param width written texture-region width in texels
 * @param height written texture-region height in texels
 * @param depth written texture-region depth in texels
 * @return whether the hint was applied
 */
bool dvz_frame_plan_upload_set_texture_3d_extent(
    DvzFramePlan* plan, uint32_t width, uint32_t height, uint32_t depth)
{
    if (plan == NULL || plan->count == 0)
        return false;
    DvzFramePlanNode* node = &plan->nodes[plan->count - 1];
    if (node->type != DVZ_FRAME_PLAN_NODE_UPLOAD)
        return false;
    node->u.upload.texture_width  = width;
    node->u.upload.texture_height = height;
    node->u.upload.texture_depth  = depth;
    return true;
}



/**
 * Set the texture format on the most recently appended texture upload.
 *
 * @param plan the FramePlan
 * @param format texture format token
 * @param bytes_per_texel bytes in one texel
 * @return whether the format was applied
 */
bool dvz_frame_plan_upload_set_texture_format(
    DvzFramePlan* plan, DvzFormat format, uint32_t bytes_per_texel)
{
    if (plan == NULL || plan->count == 0)
        return false;
    DvzFramePlanNode* node = &plan->nodes[plan->count - 1];
    if (node->type != DVZ_FRAME_PLAN_NODE_UPLOAD)
        return false;
    node->u.upload.texture_format = (uint32_t)format;
    node->u.upload.texture_bytes_per_texel = bytes_per_texel;
    return true;
}



/**
 * Set the allocation extent on the most recently appended texture upload.
 *
 * @param plan the FramePlan
 * @param width full texture allocation width in pixels
 * @param height full texture allocation height in pixels
 * @return whether the allocation extent was applied
 */
bool dvz_frame_plan_upload_set_texture_allocation_extent(
    DvzFramePlan* plan, uint32_t width, uint32_t height)
{
    if (plan == NULL || plan->count == 0)
        return false;
    DvzFramePlanNode* node = &plan->nodes[plan->count - 1];
    if (node->type != DVZ_FRAME_PLAN_NODE_UPLOAD)
        return false;
    node->u.upload.texture_alloc_width  = width;
    node->u.upload.texture_alloc_height = height;
    node->u.upload.texture_alloc_depth  = 1;
    return true;
}



/**
 * Set the 3D allocation extent on the most recently appended texture upload.
 *
 * @param plan the FramePlan
 * @param width full texture allocation width in texels
 * @param height full texture allocation height in texels
 * @param depth full texture allocation depth in texels
 * @return whether the allocation extent was applied
 */
bool dvz_frame_plan_upload_set_texture_3d_allocation_extent(
    DvzFramePlan* plan, uint32_t width, uint32_t height, uint32_t depth)
{
    if (plan == NULL || plan->count == 0)
        return false;
    DvzFramePlanNode* node = &plan->nodes[plan->count - 1];
    if (node->type != DVZ_FRAME_PLAN_NODE_UPLOAD)
        return false;
    node->u.upload.texture_alloc_width  = width;
    node->u.upload.texture_alloc_height = height;
    node->u.upload.texture_alloc_depth  = depth;
    return true;
}



/**
 * Set the 2D subregion origin on the most recently appended texture upload.
 *
 * @param plan the FramePlan
 * @param origin_x destination x offset in pixels
 * @param origin_y destination y offset in pixels
 * @return whether the origin was applied
 */
bool dvz_frame_plan_upload_set_texture_region(
    DvzFramePlan* plan, uint32_t origin_x, uint32_t origin_y)
{
    if (plan == NULL || plan->count == 0)
        return false;
    DvzFramePlanNode* node = &plan->nodes[plan->count - 1];
    if (node->type != DVZ_FRAME_PLAN_NODE_UPLOAD)
        return false;
    node->u.upload.texture_origin_x = origin_x;
    node->u.upload.texture_origin_y = origin_y;
    node->u.upload.texture_origin_z = 0;
    return true;
}



/**
 * Set the 3D subregion origin on the most recently appended texture upload.
 *
 * @param plan the FramePlan
 * @param origin_x destination x offset in texels
 * @param origin_y destination y offset in texels
 * @param origin_z destination z offset in texels
 * @return whether the origin was applied
 */
bool dvz_frame_plan_upload_set_texture_3d_region(
    DvzFramePlan* plan, uint32_t origin_x, uint32_t origin_y, uint32_t origin_z)
{
    if (plan == NULL || plan->count == 0)
        return false;
    DvzFramePlanNode* node = &plan->nodes[plan->count - 1];
    if (node->type != DVZ_FRAME_PLAN_NODE_UPLOAD)
        return false;
    node->u.upload.texture_origin_x = origin_x;
    node->u.upload.texture_origin_y = origin_y;
    node->u.upload.texture_origin_z = origin_z;
    return true;
}



/**
 * Attach typed metadata to the most recently appended upload node.
 *
 * @param plan the FramePlan
 * @param metadata the upload metadata
 * @return whether the metadata was attached
 */
bool dvz_frame_plan_upload_metadata(DvzFramePlan* plan, const DvzFramePlanUploadMeta* metadata)
{
    DvzFramePlanNode* node = _frame_plan_last_node(plan, DVZ_FRAME_PLAN_NODE_UPLOAD);
    if (node == NULL || metadata == NULL)
        return false;
    dvz_memcpy(
        &node->u.upload.metadata, sizeof(DvzFramePlanUploadMeta), metadata,
        sizeof(DvzFramePlanUploadMeta));
    node->u.upload.metadata.has_metadata = true;
    return true;
}
