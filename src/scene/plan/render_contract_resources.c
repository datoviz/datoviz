/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene render contract resources                                                              */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "render_contract_internal.h"

#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_scene_resource_key.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return whether a graph attachment access includes reads.
 *
 * @param access the attachment access mode
 * @return whether reads are allowed
 */
static bool _attachment_access_reads(DvzFrameGraphAttachmentAccess access)
{
    return access == DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ ||
           access == DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ_WRITE;
}



/**
 * Return whether a graph attachment access includes writes.
 *
 * @param access the attachment access mode
 * @return whether writes are allowed
 */
static bool _attachment_access_writes(DvzFrameGraphAttachmentAccess access)
{
    return access == DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE ||
           access == DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ_WRITE;
}



/**
 * Copy the producer pass id for one graph read into an attachment use.
 *
 * @param plan the FramePlan
 * @param consumer_pass_id the graph pass consuming the read
 * @param use the sampled attachment use to update
 */
static void _contract_apply_read_dependency(
    const DvzFramePlan* plan, const char* consumer_pass_id, DvzSceneAttachmentUse* use)
{
    ANN(plan);
    ANN(consumer_pass_id);
    ANN(use);
    for (uint32_t i = 0; i < dvz_frame_plan_graph_dependency_count(plan); i++)
    {
        DvzFrameGraphDependency dependency = {0};
        if (!dvz_frame_plan_graph_dependency_get(plan, i, &dependency))
            continue;
        if (
            strcmp(dependency.consumer_pass_id, consumer_pass_id) == 0 &&
            strcmp(dependency.resource_id, use->resource_id) == 0)
        {
            dvz_strlcpy(
                use->producer_pass_id, dependency.producer_pass_id,
                sizeof(use->producer_pass_id));
            return;
        }
    }
}



/**
 * Return the graph resource with a given id.
 *
 * @param plan the FramePlan
 * @param resource_id the graph resource id
 * @return the graph resource, or NULL when absent
 */
static const DvzFrameGraphResource* _contract_resource_by_id(
    const DvzFramePlan* plan, const char* resource_id)
{
    ANN(plan);
    ANN(resource_id);
    for (uint32_t i = 0; i < dvz_frame_plan_graph_resource_count(plan); i++)
    {
        const DvzFrameGraphResource* resource = dvz_frame_plan_graph_resource_get(plan, i);
        if (resource != NULL && strcmp(resource->id, resource_id) == 0)
            return resource;
    }
    return NULL;
}



/**
 * Clamp a requested sample count to a supported power-of-two sample count.
 *
 * @param sample_count requested sample count
 * @param max_sample_count maximum supported sample count
 * @return supported sample count
 */
static uint32_t _contract_lowered_sample_count(uint32_t sample_count, uint32_t max_sample_count)
{
    if (sample_count <= 1 || max_sample_count <= 1)
        return 1;
    if (sample_count >= 16 && max_sample_count >= 16)
        return 16;
    if (sample_count >= 8 && max_sample_count >= 8)
        return 8;
    if (sample_count >= 4 && max_sample_count >= 4)
        return 4;
    if (sample_count >= 2 && max_sample_count >= 2)
        return 2;
    return 1;
}



/**
 * Return the maximum supported sample count for a graph resource under active capabilities.
 *
 * @param resource the graph resource descriptor
 * @param caps the active capability snapshot
 * @return maximum supported sample count for the resource
 */
static uint32_t _contract_resource_sample_limit(
    const DvzFrameGraphResource* resource, const DvzCapabilitySnapshot* caps)
{
    ANN(resource);
    if (caps == NULL)
        return 16;

    uint32_t max_sample_count = 16;
    const bool color =
        (resource->usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT) != 0;
    const bool depth =
        (resource->usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_DEPTH_ATTACHMENT) != 0;
    if (color || depth)
    {
        uint32_t color_max = caps->max_color_sample_count;
        uint32_t depth_max = caps->max_depth_sample_count;
        color_max = color_max != 0 ? color_max : 1;
        depth_max = depth_max != 0 ? depth_max : 1;
        max_sample_count = color_max < depth_max ? color_max : depth_max;
    }
    return max_sample_count != 0 ? max_sample_count : 1;
}



/**
 * Return a graph resource's capability-resolved sample count.
 *
 * @param resource the graph resource descriptor
 * @param caps the active capability snapshot, or NULL to preserve the requested count
 * @return resolved sample count
 */
static uint32_t _contract_resolved_resource_sample_count(
    const DvzFrameGraphResource* resource, const DvzCapabilitySnapshot* caps)
{
    if (resource == NULL)
        return 1;
    uint32_t requested = resource->sample_count != 0 ? resource->sample_count : 1;
    if (caps == NULL)
        return requested;
    return _contract_lowered_sample_count(
        requested, _contract_resource_sample_limit(resource, caps));
}



/**
 * Copy graph resource facts into an attachment use.
 *
 * @param plan the FramePlan
 * @param caps the active capability snapshot, or NULL to preserve requested sample counts
 * @param use the attachment use
 */
static void _contract_apply_resource_facts(
    const DvzFramePlan* plan, const DvzCapabilitySnapshot* caps, DvzSceneAttachmentUse* use)
{
    ANN(plan);
    ANN(use);
    const DvzFrameGraphResource* resource = _contract_resource_by_id(plan, use->resource_id);
    if (resource == NULL)
        return;
    use->format = resource->format;
    use->requested_sample_count = resource->sample_count == 0 ? 1 : resource->sample_count;
    use->resolved_sample_count = _contract_resolved_resource_sample_count(resource, caps);
    use->sample_count = use->resolved_sample_count;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Append one attachment use to a pass contract.
 *
 * @param contract the pass contract
 * @param resource_id the graph resource id
 * @param role the attachment role
 * @return the appended attachment use, or NULL if the contract is full
 */
DvzSceneAttachmentUse* _contract_append_attachment(
    DvzScenePassContract* contract, const char* resource_id, DvzSceneAttachmentRole role)
{
    ANN(contract);
    ANN(resource_id);
    if (contract->attachment_count >= DVZ_SCENE_MAX_CONTRACT_ATTACHMENTS)
        return NULL;

    DvzSceneAttachmentUse* use = &contract->attachments[contract->attachment_count++];
    dvz_memset(use, sizeof(DvzSceneAttachmentUse), 0, sizeof(DvzSceneAttachmentUse));
    dvz_strlcpy(use->resource_id, resource_id, sizeof(use->resource_id));
    use->role = role;
    return use;
}



/**
 * Append one color attachment to a pass contract.
 *
 * @param plan the FramePlan
 * @param contract the pass contract
 * @param attachment the graph attachment
 * @param caps the active capability snapshot, or NULL to preserve requested sample counts
 * @return whether the attachment was appended
 */
bool _contract_append_color_attachment(
    const DvzFramePlan* plan, DvzScenePassContract* contract,
    const DvzFrameGraphAttachment* attachment, const DvzCapabilitySnapshot* caps)
{
    ANN(plan);
    ANN(contract);
    ANN(attachment);
    DvzSceneAttachmentUse* use = _contract_append_attachment(
        contract, attachment->resource_id, DVZ_SCENE_ATTACHMENT_COLOR);
    if (use == NULL)
        return false;
    use->load_op = attachment->load_op;
    use->store_op = attachment->store_op;
    use->access = attachment->access;
    use->read = _attachment_access_reads(attachment->access);
    use->write = _attachment_access_writes(attachment->access);
    use->clear = attachment->load_op == DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR;
    use->preserve = attachment->load_op == DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD ||
                    attachment->store_op == DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE;
    _contract_apply_resource_facts(plan, caps, use);
    contract->color_attachment_count++;
    return true;
}



/**
 * Append one depth attachment to a pass contract.
 *
 * @param plan the FramePlan
 * @param contract the pass contract
 * @param attachment the graph attachment
 * @param caps the active capability snapshot, or NULL to preserve requested sample counts
 * @return whether the attachment was appended
 */
bool _contract_append_depth_attachment(
    const DvzFramePlan* plan, DvzScenePassContract* contract,
    const DvzFrameGraphAttachment* attachment, const DvzCapabilitySnapshot* caps)
{
    ANN(plan);
    ANN(contract);
    ANN(attachment);
    DvzSceneAttachmentUse* use = _contract_append_attachment(
        contract, attachment->resource_id, DVZ_SCENE_ATTACHMENT_DEPTH);
    if (use == NULL)
        return false;
    use->load_op = attachment->load_op;
    use->store_op = attachment->store_op;
    use->access = attachment->access;
    use->read = _attachment_access_reads(attachment->access);
    use->write = _attachment_access_writes(attachment->access);
    use->clear = attachment->load_op == DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR;
    use->preserve = attachment->load_op == DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD ||
                    attachment->store_op == DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE;
    _contract_apply_resource_facts(plan, caps, use);
    contract->has_depth_attachment = true;
    return true;
}



/**
 * Append one sampled read edge to a pass contract.
 *
 * @param plan the FramePlan
 * @param contract the pass contract
 * @param consumer_pass_id graph pass id that owns the read
 * @param read the graph read edge
 * @param caps the active capability snapshot, or NULL to preserve requested sample counts
 * @return whether the read was appended
 */
bool _contract_append_read(
    const DvzFramePlan* plan, DvzScenePassContract* contract, const char* consumer_pass_id,
    const DvzFrameGraphAccess* read, const DvzCapabilitySnapshot* caps)
{
    ANN(plan);
    ANN(contract);
    ANN(consumer_pass_id);
    ANN(read);
    DvzSceneAttachmentUse* use = _contract_append_attachment(
        contract, read->resource_id, DVZ_SCENE_ATTACHMENT_SAMPLED);
    if (use == NULL)
        return false;
    use->read = true;
    _contract_apply_resource_facts(plan, caps, use);
    _contract_apply_read_dependency(plan, consumer_pass_id, use);
    contract->sampled_read_count++;
    if (_scene_resource_id_has_depth_marker(read->resource_id))
        contract->sampled_depth_read_count++;
    return true;
}



/**
 * Return whether a pass contract has a depth attachment.
 *
 * @param contract the pass contract
 * @return whether the contract includes a depth attachment
 */
bool _contract_has_depth_attachment(const DvzScenePassContract* contract)
{
    ANN(contract);
    for (uint32_t i = 0; i < contract->attachment_count; i++)
    {
        if (contract->attachments[i].role == DVZ_SCENE_ATTACHMENT_DEPTH)
            return true;
    }
    return false;
}



/**
 * Return whether one pass has a sampled or producer-backed depth resource.
 *
 * @param contract the pass contract
 * @return whether sampled-depth draws can resolve a produced depth resource
 */
bool _contract_has_sampled_depth_resource(const DvzScenePassContract* contract)
{
    ANN(contract);
    if (contract->sampled_depth_read_count > 0)
        return true;
    for (uint32_t i = 0; i < contract->attachment_count; i++)
    {
        const DvzSceneAttachmentUse* attachment = &contract->attachments[i];
        if (attachment->role != DVZ_SCENE_ATTACHMENT_DEPTH)
            continue;
        if (
            attachment->load_op == DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD &&
            (attachment->access == DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ ||
             attachment->access == DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ_WRITE))
            return true;
    }
    return false;
}



/**
 * Count attachment uses with a given role.
 *
 * @param contract the pass contract
 * @param role the attachment role
 * @return number of matching attachments
 */
uint32_t _contract_attachment_count(
    const DvzScenePassContract* contract, DvzSceneAttachmentRole role)
{
    ANN(contract);
    uint32_t count = 0;
    for (uint32_t i = 0; i < contract->attachment_count; i++)
    {
        if (contract->attachments[i].role == role)
            count++;
    }
    return count;
}



/**
 * Return the first attachment matching a role and resource suffix.
 *
 * @param contract the pass contract
 * @param role the attachment role
 * @param suffix the expected resource id suffix
 * @return the matching attachment, or NULL
 */
const DvzSceneAttachmentUse* _contract_attachment_suffix(
    const DvzScenePassContract* contract, DvzSceneAttachmentRole role, const char* suffix)
{
    ANN(contract);
    ANN(suffix);
    for (uint32_t i = 0; i < contract->attachment_count; i++)
    {
        const DvzSceneAttachmentUse* use = &contract->attachments[i];
        if (use->role == role && _scene_resource_id_has_suffix(use->resource_id, suffix))
            return use;
    }
    return NULL;
}



/**
 * Return whether a pass contract reads a resource containing a suffix.
 *
 * @param contract the pass contract
 * @param suffix the expected resource id suffix
 * @return whether a sampled attachment matches
 */
bool _contract_reads_resource_suffix(const DvzScenePassContract* contract, const char* suffix)
{
    ANN(contract);
    ANN(suffix);
    for (uint32_t i = 0; i < contract->attachment_count; i++)
    {
        const DvzSceneAttachmentUse* use = &contract->attachments[i];
        if (use->role != DVZ_SCENE_ATTACHMENT_SAMPLED || !use->read)
            continue;
        if (_scene_resource_id_has_suffix(use->resource_id, suffix))
            return true;
    }
    return false;
}



/**
 * Return the sampled attachment for an exact resource id.
 *
 * @param contract the pass contract
 * @param resource_id the expected graph resource id
 * @return the sampled attachment use, or NULL
 */
const DvzSceneAttachmentUse* _contract_sampled_resource_use(
    const DvzScenePassContract* contract, const char* resource_id)
{
    ANN(contract);
    ANN(resource_id);
    for (uint32_t i = 0; i < contract->attachment_count; i++)
    {
        const DvzSceneAttachmentUse* use = &contract->attachments[i];
        if (use->role != DVZ_SCENE_ATTACHMENT_SAMPLED || !use->read)
            continue;
        if (strcmp(use->resource_id, resource_id) == 0)
            return use;
    }
    return NULL;
}



/**
 * Return whether any draw in a contract tests or writes fixed-function depth.
 *
 * @param contract the pass contract
 * @return whether a depth attachment is required
 */
bool _contract_needs_depth(const DvzScenePassContract* contract)
{
    ANN(contract);
    for (uint32_t i = 0; i < contract->draw_count; i++)
    {
        const DvzSceneDrawContract* draw = &contract->draws[i];
        if (draw->depth_test || draw->depth_write)
            return true;
    }
    return false;
}
