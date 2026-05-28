/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene FramePlan resources                                                                    */
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
 * @param format texture format, using VkFormat values
 * @param bytes_per_texel bytes in one texel
 * @return whether the format was applied
 */
bool dvz_frame_plan_upload_set_texture_format(
    DvzFramePlan* plan, uint32_t format, uint32_t bytes_per_texel)
{
    if (plan == NULL || plan->count == 0)
        return false;
    DvzFramePlanNode* node = &plan->nodes[plan->count - 1];
    if (node->type != DVZ_FRAME_PLAN_NODE_UPLOAD)
        return false;
    node->u.upload.texture_format = format;
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
