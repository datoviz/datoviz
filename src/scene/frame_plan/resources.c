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
 * Return the default FramePlan upload descriptor.
 *
 * @return default upload descriptor
 */
DvzFramePlanUploadDesc dvz_frame_plan_upload_desc(void)
{
    return (DvzFramePlanUploadDesc){
        .struct_size = sizeof(DvzFramePlanUploadDesc),
        .topology = (DvzPrimitiveTopology)UINT32_MAX,
        .texture_format = DVZ_FORMAT_NONE,
    };
}



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
    DvzFramePlanUploadDesc desc = dvz_frame_plan_upload_desc();
    desc.resource_id = resource_id;
    desc.byte_offset = byte_offset;
    desc.byte_size = byte_size;
    desc.data_tag = data_tag;
    desc.data = data;
    return dvz_frame_plan_upload_ex(plan, &desc);
}



/**
 * Append an upload node from a descriptor.
 *
 * @param plan the FramePlan
 * @param desc upload descriptor
 * @return whether the node was appended
 */
bool dvz_frame_plan_upload_ex(DvzFramePlan* plan, const DvzFramePlanUploadDesc* desc)
{
    if (desc == NULL)
        return false;
    if (desc->struct_size != 0 && desc->struct_size < sizeof(DvzFramePlanUploadDesc))
        return false;
    DvzFramePlanNode* node = _frame_plan_append_node(plan, DVZ_FRAME_PLAN_NODE_UPLOAD);
    if (node == NULL)
        return false;
    _frame_plan_copy_label(
        node->u.upload.resource_id, DVZ_SCENE_LABEL_SIZE,
        desc->resource_id ? desc->resource_id : "");
    node->u.upload.byte_offset = desc->byte_offset;
    node->u.upload.byte_size = desc->byte_size;
    _frame_plan_copy_label(
        node->u.upload.data_tag, DVZ_SCENE_LABEL_SIZE, desc->data_tag ? desc->data_tag : "");
    node->u.upload.data = desc->data;
    node->u.upload.topology = (uint32_t)desc->topology;
    node->u.upload.texture_width = desc->texture_width;
    node->u.upload.texture_height = desc->texture_height;
    node->u.upload.texture_depth = desc->texture_depth;
    node->u.upload.texture_format = (uint32_t)desc->texture_format;
    node->u.upload.texture_bytes_per_texel = desc->texture_bytes_per_texel;
    node->u.upload.texture_alloc_width = desc->texture_alloc_width;
    node->u.upload.texture_alloc_height = desc->texture_alloc_height;
    node->u.upload.texture_alloc_depth = desc->texture_alloc_depth;
    node->u.upload.texture_origin_x = desc->texture_origin_x;
    node->u.upload.texture_origin_y = desc->texture_origin_y;
    node->u.upload.texture_origin_z = desc->texture_origin_z;
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
 * Append one explicit persistent-resource retirement.
 *
 * @param plan the FramePlan
 * @param resource_id the immutable semantic resource key
 * @param kind the resource kind
 * @param lifecycle_revision the retirement lifecycle revision
 * @return whether the retirement was appended
 */
bool _frame_plan_retire_resource(
    DvzFramePlan* plan, const char* resource_id, DvzFramePlanResourceKind kind,
    uint64_t lifecycle_revision)
{
    if (plan == NULL || resource_id == NULL || resource_id[0] == '\0' ||
        plan->retirement_count >= DVZ_FRAME_PLAN_MAX_RETIREMENTS)
        return false;
    DvzFramePlanRetirement* retirement = &plan->retirements[plan->retirement_count++];
    _frame_plan_copy_label(
        retirement->resource_id, sizeof(retirement->resource_id), resource_id);
    retirement->kind = kind;
    retirement->lifecycle_revision = lifecycle_revision;
    return true;
}
