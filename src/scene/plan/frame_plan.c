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

#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_frame_plan.h"
#include "_frame_plan_internal.h"
#include "_json.h"
#include "_log.h"
#include "_overflow.h"



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
    dvz_strlcpy(dst, src, (size_t)dst_size);
}



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



static const char* _node_type_name(DvzFramePlanNodeType type)
{
    switch (type)
    {
    case DVZ_FRAME_PLAN_NODE_UPLOAD:
        return "upload";
    case DVZ_FRAME_PLAN_NODE_COMPUTE:
        return "compute";
    case DVZ_FRAME_PLAN_NODE_RENDER:
        return "render";
    case DVZ_FRAME_PLAN_NODE_CLEAR:
        return "clear";
    case DVZ_FRAME_PLAN_NODE_COPY:
        return "copy";
    case DVZ_FRAME_PLAN_NODE_READBACK:
        return "readback";
    case DVZ_FRAME_PLAN_NODE_NONE:
        return "none";
    default:
        return "none";
    }
}



static const char* _render_pass_role_name(DvzFramePlanRenderPassRole role)
{
    switch (role)
    {
    case DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE:
        return "opaque";
    case DVZ_FRAME_PLAN_RENDER_PASS_GBUFFER:
        return "gbuffer";
    case DVZ_FRAME_PLAN_RENDER_PASS_VOLUME_OCCLUSION:
        return "volume_occlusion";
    case DVZ_FRAME_PLAN_RENDER_PASS_SCENE_OCCLUSION:
        return "scene_occlusion";
    case DVZ_FRAME_PLAN_RENDER_PASS_SSAO:
        return "ssao";
    case DVZ_FRAME_PLAN_RENDER_PASS_SSAO_BLUR:
        return "ssao_blur";
    case DVZ_FRAME_PLAN_RENDER_PASS_SSAO_COMPOSITE:
        return "ssao_composite";
    case DVZ_FRAME_PLAN_RENDER_PASS_EDL_RESOLVE:
        return "edl_resolve";
    case DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION:
        return "transparent_accumulation";
    case DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND:
        return "transparent_blend";
    case DVZ_FRAME_PLAN_RENDER_PASS_WBOIT_RESOLVE:
        return "wboit_resolve";
    case DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT:
        return "depth_peel_init";
    case DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER:
        return "depth_peel_iter";
    case DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_COMPOSITE:
        return "depth_peel_composite";
    case DVZ_FRAME_PLAN_RENDER_PASS_PICKING:
        return "picking";
    default:
        return "opaque";
    }
}



static void _json_append_string_array(
    JsonBuilder* builder, uint32_t count, const char values[][DVZ_SCENE_LABEL_SIZE])
{
    ANN(builder);
    _json_append(builder, "[");
    for (uint32_t i = 0; i < count; i++)
    {
        if (i > 0)
            _json_append(builder, ", ");
        _json_append_escaped_string(builder, values[i]);
    }
    _json_append(builder, "]");
}



static const char* _graph_resource_kind_name(DvzFrameGraphResourceKind kind)
{
    switch (kind)
    {
    case DVZ_FRAME_GRAPH_RESOURCE_BUFFER:
        return "buffer";
    case DVZ_FRAME_GRAPH_RESOURCE_TEXTURE:
        return "texture";
    case DVZ_FRAME_GRAPH_RESOURCE_EXTERNAL_TARGET:
        return "external_target";
    case DVZ_FRAME_GRAPH_RESOURCE_NONE:
        return "none";
    default:
        return "none";
    }
}



static const char* _graph_extent_kind_name(DvzFrameGraphExtentKind kind)
{
    switch (kind)
    {
    case DVZ_FRAME_GRAPH_EXTENT_FIGURE:
        return "figure";
    case DVZ_FRAME_GRAPH_EXTENT_PANEL:
        return "panel";
    case DVZ_FRAME_GRAPH_EXTENT_FIXED:
        return "fixed";
    case DVZ_FRAME_GRAPH_EXTENT_RESOURCE_REF:
        return "resource_ref";
    case DVZ_FRAME_GRAPH_EXTENT_NONE:
        return "none";
    default:
        return "none";
    }
}



static const char* _graph_lifetime_name(DvzFrameGraphResourceLifetime lifetime)
{
    switch (lifetime)
    {
    case DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_BORROWED:
        return "borrowed";
    case DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME:
        return "per_frame";
    case DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PERSISTENT:
        return "persistent";
    case DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_NONE:
        return "none";
    default:
        return "none";
    }
}



static const char* _graph_pass_kind_name(DvzFrameGraphPassKind kind)
{
    switch (kind)
    {
    case DVZ_FRAME_GRAPH_PASS_RENDER:
        return "render";
    case DVZ_FRAME_GRAPH_PASS_COMPUTE:
        return "compute";
    case DVZ_FRAME_GRAPH_PASS_COPY:
        return "copy";
    case DVZ_FRAME_GRAPH_PASS_READBACK:
        return "readback";
    case DVZ_FRAME_GRAPH_PASS_CLEAR:
        return "clear";
    case DVZ_FRAME_GRAPH_PASS_NONE:
        return "none";
    default:
        return "none";
    }
}



static const char* _graph_access_usage_name(DvzFrameGraphAccessUsage usage)
{
    switch (usage)
    {
    case DVZ_FRAME_GRAPH_ACCESS_SAMPLED:
        return "sampled";
    case DVZ_FRAME_GRAPH_ACCESS_STORAGE_READ:
        return "storage_read";
    case DVZ_FRAME_GRAPH_ACCESS_STORAGE_WRITE:
        return "storage_write";
    case DVZ_FRAME_GRAPH_ACCESS_COLOR_ATTACHMENT:
        return "color_attachment";
    case DVZ_FRAME_GRAPH_ACCESS_DEPTH_ATTACHMENT_READ:
        return "depth_attachment_read";
    case DVZ_FRAME_GRAPH_ACCESS_DEPTH_ATTACHMENT_WRITE:
        return "depth_attachment_write";
    case DVZ_FRAME_GRAPH_ACCESS_COPY_SRC:
        return "copy_src";
    case DVZ_FRAME_GRAPH_ACCESS_COPY_DST:
        return "copy_dst";
    case DVZ_FRAME_GRAPH_ACCESS_NONE:
        return "none";
    default:
        return "none";
    }
}



static const char* _graph_attachment_load_name(DvzFrameGraphAttachmentLoadOp op)
{
    switch (op)
    {
    case DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR:
        return "clear";
    case DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD:
        return "load";
    case DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_DONT_CARE:
        return "dont_care";
    default:
        return "dont_care";
    }
}



static const char* _graph_attachment_store_name(DvzFrameGraphAttachmentStoreOp op)
{
    switch (op)
    {
    case DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE:
        return "store";
    case DVZ_FRAME_GRAPH_ATTACHMENT_STORE_DONT_CARE:
        return "dont_care";
    default:
        return "dont_care";
    }
}



static const char* _graph_attachment_access_name(DvzFrameGraphAttachmentAccess access)
{
    switch (access)
    {
    case DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ:
        return "read";
    case DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE:
        return "write";
    case DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ_WRITE:
        return "read_write";
    case DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_NONE:
        return "none";
    default:
        return "none";
    }
}



static bool _graph_resource_index(
    const DvzFramePlan* plan, const char* resource_id, uint32_t* index)
{
    if (plan == NULL || resource_id == NULL || resource_id[0] == '\0')
        return false;

    for (uint32_t i = 0; i < plan->graph_resource_count; i++)
    {
        if (strcmp(plan->graph_resources[i].id, resource_id) == 0)
        {
            if (index != NULL)
                *index = i;
            return true;
        }
    }
    return false;
}



static bool _graph_pass_id_exists_before(const DvzFramePlan* plan, const char* pass_id, uint32_t end)
{
    if (plan == NULL || pass_id == NULL || pass_id[0] == '\0')
        return false;

    uint32_t n = end < plan->graph_pass_count ? end : plan->graph_pass_count;
    for (uint32_t i = 0; i < n; i++)
    {
        if (strcmp(plan->graph_passes[i].id, pass_id) == 0)
            return true;
    }
    return false;
}



static uint32_t _graph_usage_flag(DvzFrameGraphAccessUsage usage)
{
    switch (usage)
    {
    case DVZ_FRAME_GRAPH_ACCESS_SAMPLED:
        return DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED;
    case DVZ_FRAME_GRAPH_ACCESS_STORAGE_READ:
    case DVZ_FRAME_GRAPH_ACCESS_STORAGE_WRITE:
        return DVZ_FRAME_GRAPH_RESOURCE_USAGE_STORAGE;
    case DVZ_FRAME_GRAPH_ACCESS_COLOR_ATTACHMENT:
        return DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT;
    case DVZ_FRAME_GRAPH_ACCESS_DEPTH_ATTACHMENT_READ:
    case DVZ_FRAME_GRAPH_ACCESS_DEPTH_ATTACHMENT_WRITE:
        return DVZ_FRAME_GRAPH_RESOURCE_USAGE_DEPTH_ATTACHMENT;
    case DVZ_FRAME_GRAPH_ACCESS_COPY_SRC:
        return DVZ_FRAME_GRAPH_RESOURCE_USAGE_COPY_SRC;
    case DVZ_FRAME_GRAPH_ACCESS_COPY_DST:
        return DVZ_FRAME_GRAPH_RESOURCE_USAGE_COPY_DST;
    case DVZ_FRAME_GRAPH_ACCESS_NONE:
        return DVZ_FRAME_GRAPH_RESOURCE_USAGE_NONE;
    default:
        return DVZ_FRAME_GRAPH_RESOURCE_USAGE_NONE;
    }
}



static bool _graph_access_reads(DvzFrameGraphAccessUsage usage)
{
    return usage == DVZ_FRAME_GRAPH_ACCESS_SAMPLED ||
           usage == DVZ_FRAME_GRAPH_ACCESS_STORAGE_READ ||
           usage == DVZ_FRAME_GRAPH_ACCESS_DEPTH_ATTACHMENT_READ ||
           usage == DVZ_FRAME_GRAPH_ACCESS_COPY_SRC;
}



static bool _graph_access_writes(DvzFrameGraphAccessUsage usage)
{
    return usage == DVZ_FRAME_GRAPH_ACCESS_STORAGE_WRITE ||
           usage == DVZ_FRAME_GRAPH_ACCESS_COLOR_ATTACHMENT ||
           usage == DVZ_FRAME_GRAPH_ACCESS_DEPTH_ATTACHMENT_WRITE ||
           usage == DVZ_FRAME_GRAPH_ACCESS_COPY_DST;
}



static bool _graph_attachment_reads(const DvzFrameGraphAttachment* attachment)
{
    ANN(attachment);
    return attachment->access == DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ ||
           attachment->access == DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ_WRITE ||
           attachment->load_op == DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD;
}



static bool _graph_attachment_writes(const DvzFrameGraphAttachment* attachment)
{
    ANN(attachment);
    return attachment->access == DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE ||
           attachment->access == DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ_WRITE;
}



static bool _graph_pass_writes_resource(const DvzFrameGraphPass* pass, const char* resource_id)
{
    ANN(pass);
    ANN(resource_id);
    for (uint32_t i = 0; i < pass->write_count; i++)
    {
        if (_graph_access_writes(pass->writes[i].usage) &&
            strcmp(pass->writes[i].resource_id, resource_id) == 0)
            return true;
    }
    for (uint32_t i = 0; i < pass->color_attachment_count; i++)
    {
        if (_graph_attachment_writes(&pass->color_attachments[i]) &&
            strcmp(pass->color_attachments[i].resource_id, resource_id) == 0)
            return true;
    }
    if (pass->has_depth_attachment && _graph_attachment_writes(&pass->depth_attachment) &&
        strcmp(pass->depth_attachment.resource_id, resource_id) == 0)
        return true;
    if (pass->has_stencil_attachment && _graph_attachment_writes(&pass->stencil_attachment) &&
        strcmp(pass->stencil_attachment.resource_id, resource_id) == 0)
        return true;
    return false;
}



/**
 * Return whether a graph resource is per-frame.
 *
 * @param plan the FramePlan.
 * @param resource_id the graph resource id.
 * @return whether the resource has per-frame lifetime.
 */
static bool _graph_resource_is_per_frame(const DvzFramePlan* plan, const char* resource_id)
{
    ANN(plan);
    ANN(resource_id);
    uint32_t resource_index = 0;
    if (!_graph_resource_index(plan, resource_id, &resource_index))
        return false;
    return plan->graph_resources[resource_index].lifetime ==
           DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
}



/**
 * Return the graph access represented by a color attachment.
 *
 * @param attachment the graph attachment descriptor.
 * @return graph color attachment access.
 */
static DvzFrameGraphAccessUsage
_graph_color_attachment_usage(const DvzFrameGraphAttachment* attachment)
{
    ANN(attachment);
    (void)attachment;
    return DVZ_FRAME_GRAPH_ACCESS_COLOR_ATTACHMENT;
}



/**
 * Return the graph access represented by a depth attachment.
 *
 * @param attachment the graph attachment descriptor.
 * @return graph depth attachment access.
 */
static DvzFrameGraphAccessUsage
_graph_depth_attachment_usage(const DvzFrameGraphAttachment* attachment)
{
    ANN(attachment);
    if (attachment->access == DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ)
        return DVZ_FRAME_GRAPH_ACCESS_DEPTH_ATTACHMENT_READ;
    if (attachment->access == DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_NONE)
        return DVZ_FRAME_GRAPH_ACCESS_NONE;
    return DVZ_FRAME_GRAPH_ACCESS_DEPTH_ATTACHMENT_WRITE;
}



/**
 * Count producer declarations for a resource in one graph pass.
 *
 * @param pass the graph pass descriptor.
 * @param resource_id the graph resource id.
 * @param count output producer declaration count.
 * @param usage optional output last producer usage.
 * @return whether at least one producer declaration exists.
 */
static bool _graph_pass_write_count_resource(
    const DvzFrameGraphPass* pass, const char* resource_id, uint32_t* count,
    DvzFrameGraphAccessUsage* usage)
{
    ANN(pass);
    ANN(resource_id);
    ANN(count);
    *count = 0;
    for (uint32_t i = 0; i < pass->write_count; i++)
    {
        if (_graph_access_writes(pass->writes[i].usage) &&
            strcmp(pass->writes[i].resource_id, resource_id) == 0)
        {
            *count += 1;
            if (usage != NULL)
                *usage = pass->writes[i].usage;
        }
    }
    for (uint32_t i = 0; i < pass->color_attachment_count; i++)
    {
        if (_graph_attachment_writes(&pass->color_attachments[i]) &&
            strcmp(pass->color_attachments[i].resource_id, resource_id) == 0)
        {
            *count += 1;
            if (usage != NULL)
                *usage = _graph_color_attachment_usage(&pass->color_attachments[i]);
        }
    }
    if (pass->has_depth_attachment && _graph_attachment_writes(&pass->depth_attachment) &&
        strcmp(pass->depth_attachment.resource_id, resource_id) == 0)
    {
        *count += 1;
        if (usage != NULL)
            *usage = _graph_depth_attachment_usage(&pass->depth_attachment);
    }
    if (pass->has_stencil_attachment && _graph_attachment_writes(&pass->stencil_attachment) &&
        strcmp(pass->stencil_attachment.resource_id, resource_id) == 0)
    {
        *count += 1;
        if (usage != NULL)
            *usage = _graph_depth_attachment_usage(&pass->stencil_attachment);
    }
    return *count > 0;
}



/**
 * Find the latest graph pass that writes a resource before a consumer pass.
 *
 * @param plan the FramePlan.
 * @param resource_id the graph resource id.
 * @param pass_index the consumer pass index.
 * @param producer_index output producer pass index.
 * @param producer_usage optional output producer access usage.
 * @return whether a producer was found.
 */
static bool _graph_find_last_writer_before(
    const DvzFramePlan* plan, const char* resource_id, uint32_t pass_index,
    uint32_t* producer_index, DvzFrameGraphAccessUsage* producer_usage)
{
    ANN(plan);
    ANN(resource_id);
    ANN(producer_index);
    for (uint32_t i = pass_index; i > 0; i--)
    {
        uint32_t count = 0;
        DvzFrameGraphAccessUsage usage = DVZ_FRAME_GRAPH_ACCESS_NONE;
        if (_graph_pass_write_count_resource(&plan->graph_passes[i - 1], resource_id, &count, &usage))
        {
            *producer_index = i - 1;
            if (producer_usage != NULL)
                *producer_usage = usage;
            return true;
        }
    }
    return false;
}



/**
 * Find the first graph pass that writes a resource after a consumer pass.
 *
 * @param plan the FramePlan.
 * @param resource_id the graph resource id.
 * @param pass_index the consumer pass index.
 * @param producer_index output producer pass index.
 * @param producer_usage optional output producer access usage.
 * @return whether a producer was found after the consumer.
 */
static bool _graph_find_first_writer_after(
    const DvzFramePlan* plan, const char* resource_id, uint32_t pass_index,
    uint32_t* producer_index, DvzFrameGraphAccessUsage* producer_usage)
{
    ANN(plan);
    ANN(resource_id);
    ANN(producer_index);
    for (uint32_t i = pass_index + 1; i < plan->graph_pass_count; i++)
    {
        uint32_t count = 0;
        DvzFrameGraphAccessUsage usage = DVZ_FRAME_GRAPH_ACCESS_NONE;
        if (_graph_pass_write_count_resource(&plan->graph_passes[i], resource_id, &count, &usage))
        {
            *producer_index = i;
            if (producer_usage != NULL)
                *producer_usage = usage;
            return true;
        }
    }
    return false;
}



/**
 * Build a dependency edge for a consumer access when a prior producer exists.
 *
 * @param plan the FramePlan.
 * @param resource_id the graph resource id.
 * @param consumer_usage the consumer access usage.
 * @param consumer_index the consumer pass index.
 * @param out optional output dependency descriptor.
 * @return whether a dependency edge exists.
 */
static bool _graph_dependency_from_access(
    const DvzFramePlan* plan, const char* resource_id, DvzFrameGraphAccessUsage consumer_usage,
    uint32_t consumer_index, DvzFrameGraphDependency* out)
{
    ANN(plan);
    ANN(resource_id);
    uint32_t producer_index = 0;
    DvzFrameGraphAccessUsage producer_usage = DVZ_FRAME_GRAPH_ACCESS_NONE;
    if (!_graph_find_last_writer_before(
            plan, resource_id, consumer_index, &producer_index, &producer_usage))
        return false;
    if (out != NULL)
    {
        dvz_memset(out, sizeof(DvzFrameGraphDependency), 0, sizeof(DvzFrameGraphDependency));
        _frame_plan_copy_label(out->resource_id, DVZ_SCENE_LABEL_SIZE, resource_id);
        out->producer_pass_index = producer_index;
        out->consumer_pass_index = consumer_index;
        _frame_plan_copy_label(
            out->producer_pass_id, DVZ_SCENE_LABEL_SIZE, plan->graph_passes[producer_index].id);
        _frame_plan_copy_label(
            out->consumer_pass_id, DVZ_SCENE_LABEL_SIZE, plan->graph_passes[consumer_index].id);
        out->producer_usage = producer_usage;
        out->consumer_usage = consumer_usage;
    }
    return true;
}



/**
 * Count dependencies consumed by one graph pass.
 *
 * @param plan the FramePlan.
 * @param pass the graph pass descriptor.
 * @param pass_index the graph pass index.
 * @param target_index dependency index to materialize.
 * @param out optional output dependency descriptor.
 * @return dependency count for the pass.
 */
static uint32_t _graph_pass_dependency_count(
    const DvzFramePlan* plan, const DvzFrameGraphPass* pass, uint32_t pass_index,
    uint32_t target_index, DvzFrameGraphDependency* out)
{
    ANN(plan);
    ANN(pass);
    uint32_t count = 0;
#define COUNT_DEP(resource, usage)                                                               \
    do                                                                                            \
    {                                                                                             \
        if (_graph_dependency_from_access(plan, (resource), (usage), pass_index,                  \
                                          count == target_index ? out : NULL))                    \
            count++;                                                                              \
    } while (0)

    for (uint32_t i = 0; i < pass->read_count; i++)
    {
        if (_graph_access_reads(pass->reads[i].usage))
            COUNT_DEP(pass->reads[i].resource_id, pass->reads[i].usage);
    }
    for (uint32_t i = 0; i < pass->color_attachment_count; i++)
    {
        const DvzFrameGraphAttachment* attachment = &pass->color_attachments[i];
        if (_graph_attachment_reads(attachment))
            COUNT_DEP(attachment->resource_id, _graph_color_attachment_usage(attachment));
    }
    if (pass->has_depth_attachment && _graph_attachment_reads(&pass->depth_attachment))
    {
        COUNT_DEP(
            pass->depth_attachment.resource_id,
            _graph_depth_attachment_usage(&pass->depth_attachment));
    }
    if (pass->has_stencil_attachment && _graph_attachment_reads(&pass->stencil_attachment))
    {
        COUNT_DEP(
            pass->stencil_attachment.resource_id,
            _graph_depth_attachment_usage(&pass->stencil_attachment));
    }
#undef COUNT_DEP
    return count;
}



static bool _graph_resource_written_before(
    const DvzFramePlan* plan, const char* resource_id, uint32_t pass_index)
{
    ANN(plan);
    ANN(resource_id);
    for (uint32_t i = 0; i < pass_index && i < plan->graph_pass_count; i++)
    {
        if (_graph_pass_writes_resource(&plan->graph_passes[i], resource_id))
            return true;
    }
    return false;
}



static bool _graph_resource_is_color_attachment_compatible(const DvzFrameGraphResource* resource)
{
    ANN(resource);
    return resource->kind == DVZ_FRAME_GRAPH_RESOURCE_TEXTURE ||
           resource->kind == DVZ_FRAME_GRAPH_RESOURCE_EXTERNAL_TARGET;
}



static bool _graph_resource_is_depth_attachment_compatible(const DvzFrameGraphResource* resource)
{
    ANN(resource);
    return resource->kind == DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
}


/**
 * Return a graph resource's effective sample count.
 *
 * @param resource the graph resource
 * @return sample count, defaulting to 1 when unset
 */
static uint32_t _graph_resource_sample_count(const DvzFrameGraphResource* resource)
{
    return resource != NULL && resource->sample_count != 0 ? resource->sample_count : 1;
}


/**
 * Return whether a graph resource sample count is supported.
 *
 * @param sample_count the effective sample count
 * @return whether the value is valid
 */
static bool _graph_resource_sample_count_valid(uint32_t sample_count)
{
    return sample_count == 1 || sample_count == 2 || sample_count == 4 || sample_count == 8 ||
           sample_count == 16;
}



static bool _graph_resource_extent_matches(
    const DvzFrameGraphResource* a, const DvzFrameGraphResource* b)
{
    ANN(a);
    ANN(b);

    if (a->extent_kind != b->extent_kind)
        return false;
    if (a->extent_kind == DVZ_FRAME_GRAPH_EXTENT_FIXED)
        return a->width == b->width && a->height == b->height;
    if (a->extent_kind == DVZ_FRAME_GRAPH_EXTENT_RESOURCE_REF)
        return strcmp(a->extent_resource_id, b->extent_resource_id) == 0;
    return a->extent_kind != DVZ_FRAME_GRAPH_EXTENT_NONE;
}



static bool _graph_report(DvzDiagnosticReport* report, const char* fmt, ...)
{
    if (report == NULL)
        return false;

    char message[DVZ_SCENE_DIAGNOSTIC_SIZE] = {0};
    va_list args;
    va_start(args, fmt);
    int written = dvz_vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);
    if (written < 0)
        return false;
    return dvz_diagnostic_report_add(report, message);
}



static bool _graph_validate_access(
    const DvzFramePlan* plan, const DvzFrameGraphAccess* access, uint32_t pass_index,
    DvzDiagnosticReport* report)
{
    ANN(plan);
    ANN(access);
    if (access->resource_id[0] == '\0' || access->usage == DVZ_FRAME_GRAPH_ACCESS_NONE)
    {
        _graph_report(report, "FramePlan graph pass access is incomplete");
        return false;
    }

    uint32_t resource_index = 0;
    if (!_graph_resource_index(plan, access->resource_id, &resource_index))
    {
        _graph_report(report, "FramePlan graph access references unknown resource '%s'",
                      access->resource_id);
        return false;
    }

    const DvzFrameGraphResource* resource = &plan->graph_resources[resource_index];
    uint32_t required = _graph_usage_flag(access->usage);
    if (required != 0 && (resource->usage_flags & required) == 0)
    {
        _graph_report(
            report, "FramePlan graph resource '%s' is missing usage for %s", resource->id,
            _graph_access_usage_name(access->usage));
        return false;
    }

    if (_graph_access_reads(access->usage) &&
        resource->lifetime == DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME &&
        !_graph_resource_written_before(plan, access->resource_id, pass_index))
    {
        _graph_report(
            report, "FramePlan graph pass '%s' reads resource '%s' before any producer",
            plan->graph_passes[pass_index].id, access->resource_id);
        return false;
    }
    return true;
}



static bool _graph_validate_attachment(
    const DvzFramePlan* plan, const DvzFrameGraphAttachment* attachment,
    DvzFrameGraphAccessUsage usage, uint32_t pass_index, DvzDiagnosticReport* report)
{
    ANN(plan);
    ANN(attachment);
    if (attachment->resource_id[0] == '\0' ||
        attachment->access == DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_NONE)
    {
        _graph_report(report, "FramePlan graph attachment is incomplete");
        return false;
    }

    DvzFrameGraphAccess access = {0};
    _frame_plan_copy_label(access.resource_id, DVZ_SCENE_LABEL_SIZE, attachment->resource_id);
    access.usage = usage;
    if (!_graph_validate_access(plan, &access, pass_index, report))
        return false;

    uint32_t resource_index = 0;
    if (!_graph_resource_index(plan, attachment->resource_id, &resource_index))
        return false;

    const DvzFrameGraphResource* resource = &plan->graph_resources[resource_index];
    uint32_t sample_count = _graph_resource_sample_count(resource);
    if (!_graph_resource_sample_count_valid(sample_count))
    {
        _graph_report(
            report, "FramePlan graph resource '%s' has invalid sample count %" PRIu32,
            resource->id, sample_count);
        return false;
    }
    if (usage == DVZ_FRAME_GRAPH_ACCESS_COLOR_ATTACHMENT &&
        !_graph_resource_is_color_attachment_compatible(resource))
    {
        _graph_report(
            report, "FramePlan graph color attachment resource '%s' is not renderable",
            resource->id);
        return false;
    }
    if ((usage == DVZ_FRAME_GRAPH_ACCESS_DEPTH_ATTACHMENT_READ ||
         usage == DVZ_FRAME_GRAPH_ACCESS_DEPTH_ATTACHMENT_WRITE) &&
        !_graph_resource_is_depth_attachment_compatible(resource))
    {
        _graph_report(
            report, "FramePlan graph depth attachment resource '%s' is not a texture",
            resource->id);
        return false;
    }
    if (_graph_attachment_reads(attachment) &&
        resource->lifetime == DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME &&
        !_graph_resource_written_before(plan, attachment->resource_id, pass_index))
    {
        _graph_report(
            report, "FramePlan graph pass '%s' loads attachment resource '%s' before any producer",
            plan->graph_passes[pass_index].id, attachment->resource_id);
        return false;
    }
    if (usage == DVZ_FRAME_GRAPH_ACCESS_COLOR_ATTACHMENT &&
        attachment->resolve_resource_id[0] != '\0')
    {
        uint32_t resolve_index = 0;
        if (!_graph_resource_index(plan, attachment->resolve_resource_id, &resolve_index))
        {
            _graph_report(
                report, "FramePlan graph resolve attachment references unknown resource '%s'",
                attachment->resolve_resource_id);
            return false;
        }
        const DvzFrameGraphResource* resolve = &plan->graph_resources[resolve_index];
        if (!_graph_resource_is_color_attachment_compatible(resolve))
        {
            _graph_report(
                report, "FramePlan graph resolve resource '%s' is not renderable",
                resolve->id);
            return false;
        }
        if (_graph_resource_sample_count(resource) <= 1 ||
            _graph_resource_sample_count(resolve) != 1)
        {
            _graph_report(
                report,
                "FramePlan graph resolve from '%s' to '%s' requires multisample color and "
                "single-sample resolve",
                resource->id, resolve->id);
            return false;
        }
        if (!_graph_resource_extent_matches(resource, resolve))
        {
            _graph_report(
                report,
                "FramePlan graph resolve resource '%s' extent does not match color attachment "
                "'%s'",
                resolve->id, resource->id);
            return false;
        }
        if (resource->format != 0 && resolve->format != 0 && resource->format != resolve->format)
        {
            _graph_report(
                report,
                "FramePlan graph resolve resource '%s' format does not match color attachment "
                "'%s'",
                resolve->id, resource->id);
            return false;
        }
    }
    return true;
}



static bool _graph_validate_render_pass_attachment_extents(
    const DvzFramePlan* plan, const DvzFrameGraphPass* pass, DvzDiagnosticReport* report)
{
    ANN(plan);
    ANN(pass);

    if (pass->color_attachment_count == 0)
        return true;

    uint32_t first_color_index = 0;
    if (!_graph_resource_index(
            plan, pass->color_attachments[0].resource_id, &first_color_index))
        return true;

    bool ok = true;
    const DvzFrameGraphResource* first_color = &plan->graph_resources[first_color_index];
    uint32_t first_sample_count = _graph_resource_sample_count(first_color);
    for (uint32_t i = 1; i < pass->color_attachment_count; i++)
    {
        uint32_t color_index = 0;
        if (!_graph_resource_index(plan, pass->color_attachments[i].resource_id, &color_index))
            continue;
        const DvzFrameGraphResource* color = &plan->graph_resources[color_index];
        if (!_graph_resource_extent_matches(first_color, color))
        {
            _graph_report(
                report,
                "FramePlan graph color attachment resource '%s' extent does not match '%s'",
                color->id, first_color->id);
            ok = false;
        }
        if (_graph_resource_sample_count(color) != first_sample_count)
        {
            _graph_report(
                report,
                "FramePlan graph color attachment resource '%s' sample count does not match "
                "'%s'",
                color->id, first_color->id);
            ok = false;
        }
    }

    if (pass->has_depth_attachment)
    {
        uint32_t depth_index = 0;
        if (_graph_resource_index(plan, pass->depth_attachment.resource_id, &depth_index))
        {
            const DvzFrameGraphResource* depth = &plan->graph_resources[depth_index];
            if (!_graph_resource_extent_matches(first_color, depth))
            {
                _graph_report(
                    report,
                    "FramePlan graph depth attachment resource '%s' extent does not match color "
                    "attachment '%s'",
                    depth->id, first_color->id);
                ok = false;
            }
            if (_graph_resource_sample_count(depth) != first_sample_count)
            {
                _graph_report(
                    report,
                    "FramePlan graph depth attachment resource '%s' sample count does not "
                    "match color attachment '%s'",
                    depth->id, first_color->id);
                ok = false;
            }
        }
    }
    return ok;
}



static bool _graph_validate_pass_kind(const DvzFrameGraphPass* pass, DvzDiagnosticReport* report)
{
    ANN(pass);

    bool ok = true;
    bool has_attachment = pass->color_attachment_count > 0 || pass->has_depth_attachment ||
                          pass->has_stencil_attachment;
    if (has_attachment && pass->kind != DVZ_FRAME_GRAPH_PASS_RENDER)
    {
        _graph_report(
            report, "FramePlan graph pass '%s' has render attachments but is not a render pass",
            pass->id);
        ok = false;
    }

    if (pass->kind == DVZ_FRAME_GRAPH_PASS_RENDER && pass->color_attachment_count == 0)
    {
        _graph_report(
            report, "FramePlan graph render pass '%s' has no color attachments", pass->id);
        ok = false;
    }

    for (uint32_t i = 0; i < pass->read_count; i++)
    {
        if (pass->kind != DVZ_FRAME_GRAPH_PASS_RENDER &&
            (pass->reads[i].usage == DVZ_FRAME_GRAPH_ACCESS_COLOR_ATTACHMENT ||
             pass->reads[i].usage == DVZ_FRAME_GRAPH_ACCESS_DEPTH_ATTACHMENT_READ))
        {
            _graph_report(
                report, "FramePlan graph pass '%s' has render-only read access", pass->id);
            ok = false;
        }
    }
    for (uint32_t i = 0; i < pass->write_count; i++)
    {
        if (pass->kind != DVZ_FRAME_GRAPH_PASS_RENDER &&
            (pass->writes[i].usage == DVZ_FRAME_GRAPH_ACCESS_COLOR_ATTACHMENT ||
             pass->writes[i].usage == DVZ_FRAME_GRAPH_ACCESS_DEPTH_ATTACHMENT_WRITE))
        {
            _graph_report(
                report, "FramePlan graph pass '%s' has render-only write access", pass->id);
            ok = false;
        }
    }
    return ok;
}



/**
 * Validate producer declarations and producer availability for a graph pass.
 *
 * @param plan the FramePlan.
 * @param pass the graph pass descriptor.
 * @param pass_index the graph pass index.
 * @param report optional diagnostic report.
 * @return whether producer declarations are valid.
 */
static bool _graph_validate_pass_producers(
    const DvzFramePlan* plan, const DvzFrameGraphPass* pass, uint32_t pass_index,
    DvzDiagnosticReport* report)
{
    ANN(plan);
    ANN(pass);
    bool ok = true;
    for (uint32_t i = 0; i < plan->graph_resource_count; i++)
    {
        uint32_t write_count = 0;
        DvzFrameGraphAccessUsage usage = DVZ_FRAME_GRAPH_ACCESS_NONE;
        if (_graph_pass_write_count_resource(pass, plan->graph_resources[i].id, &write_count, &usage) &&
            write_count > 1)
        {
            _graph_report(
                report,
                "FramePlan graph pass '%s' has ambiguous producer declarations for resource '%s'",
                pass->id, plan->graph_resources[i].id);
            ok = false;
        }
    }

    for (uint32_t i = 0; i < pass->read_count; i++)
    {
        if (!_graph_access_reads(pass->reads[i].usage) ||
            !_graph_resource_is_per_frame(plan, pass->reads[i].resource_id))
            continue;
        if (!_graph_dependency_from_access(
                plan, pass->reads[i].resource_id, pass->reads[i].usage, pass_index, NULL))
        {
            uint32_t producer_index = 0;
            if (_graph_find_first_writer_after(
                    plan, pass->reads[i].resource_id, pass_index, &producer_index, NULL))
            {
                _graph_report(
                    report,
                    "FramePlan graph pass '%s' reads resource '%s' before producer pass '%s'; "
                    "graph passes must be topological",
                    pass->id, pass->reads[i].resource_id, plan->graph_passes[producer_index].id);
            }
            else
            {
                _graph_report(
                    report,
                    "FramePlan graph pass '%s' has no producer for resource '%s'",
                    pass->id, pass->reads[i].resource_id);
            }
            ok = false;
        }
    }
    return ok;
}



static void _json_append_graph_resource_usage(JsonBuilder* builder, uint32_t usage_flags)
{
    ANN(builder);
    const struct
    {
        uint32_t flag;
        const char* name;
    } items[] = {
        {DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT, "color_attachment"},
        {DVZ_FRAME_GRAPH_RESOURCE_USAGE_DEPTH_ATTACHMENT, "depth_attachment"},
        {DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED, "sampled"},
        {DVZ_FRAME_GRAPH_RESOURCE_USAGE_STORAGE, "storage"},
        {DVZ_FRAME_GRAPH_RESOURCE_USAGE_COPY_SRC, "copy_src"},
        {DVZ_FRAME_GRAPH_RESOURCE_USAGE_COPY_DST, "copy_dst"},
    };

    _json_append(builder, "[");
    bool first = true;
    for (uint32_t i = 0; i < sizeof(items) / sizeof(items[0]); i++)
    {
        if ((usage_flags & items[i].flag) == 0)
            continue;
        _json_append(builder, "%s", first ? "" : ", ");
        _json_append_escaped_string(builder, items[i].name);
        first = false;
    }
    _json_append(builder, "]");
}



static void _json_append_graph_resource(JsonBuilder* builder, const DvzFrameGraphResource* resource)
{
    ANN(builder);
    ANN(resource);
    _json_append(builder, "{ \"id\": ");
    _json_append_escaped_string(builder, resource->id);
    _json_append(builder, ", \"kind\": ");
    _json_append_escaped_string(builder, _graph_resource_kind_name(resource->kind));
    _json_append(builder, ", \"format\": %" PRIu32 ", \"extent\": { \"kind\": ", resource->format);
    _json_append_escaped_string(builder, _graph_extent_kind_name(resource->extent_kind));
    _json_append(
        builder, ", \"width\": %" PRIu32 ", \"height\": %" PRIu32 ", \"depth\": %" PRIu32,
        resource->width, resource->height, resource->depth);
    if (resource->extent_resource_id[0] != '\0')
    {
        _json_append(builder, ", \"resource_id\": ");
        _json_append_escaped_string(builder, resource->extent_resource_id);
    }
    _json_append(
        builder, " }, \"sample_count\": %" PRIu32 ", \"usage\": ",
        resource->sample_count != 0 ? resource->sample_count : 1);
    _json_append_graph_resource_usage(builder, resource->usage_flags);
    _json_append(builder, ", \"lifetime\": ");
    _json_append_escaped_string(builder, _graph_lifetime_name(resource->lifetime));
    _json_append(builder, " }");
}



static void _json_append_graph_access(JsonBuilder* builder, const DvzFrameGraphAccess* access)
{
    ANN(builder);
    ANN(access);
    _json_append(builder, "{ \"resource_id\": ");
    _json_append_escaped_string(builder, access->resource_id);
    _json_append(builder, ", \"usage\": ");
    _json_append_escaped_string(builder, _graph_access_usage_name(access->usage));
    _json_append(builder, " }");
}



static void _json_append_graph_access_array(
    JsonBuilder* builder, uint32_t count, const DvzFrameGraphAccess* accesses)
{
    ANN(builder);
    _json_append(builder, "[");
    for (uint32_t i = 0; i < count; i++)
    {
        if (i > 0)
            _json_append(builder, ", ");
        _json_append_graph_access(builder, &accesses[i]);
    }
    _json_append(builder, "]");
}



static void _json_append_graph_rect(JsonBuilder* builder, const DvzFrameGraphRect* rect)
{
    ANN(builder);
    ANN(rect);
    _json_append(
        builder, "{ \"x\": %.9g, \"y\": %.9g, \"width\": %.9g, \"height\": %.9g }",
        (double)rect->x, (double)rect->y, (double)rect->width, (double)rect->height);
}



static void
_json_append_graph_attachment(JsonBuilder* builder, const DvzFrameGraphAttachment* attachment)
{
    ANN(builder);
    ANN(attachment);
    _json_append(builder, "{ \"resource_id\": ");
    _json_append_escaped_string(builder, attachment->resource_id);
    _json_append(builder, ", \"load_op\": ");
    _json_append_escaped_string(builder, _graph_attachment_load_name(attachment->load_op));
    _json_append(builder, ", \"store_op\": ");
    _json_append_escaped_string(builder, _graph_attachment_store_name(attachment->store_op));
    _json_append(builder, ", \"access\": ");
    _json_append_escaped_string(builder, _graph_attachment_access_name(attachment->access));
    if (attachment->resolve_resource_id[0] != '\0')
    {
        _json_append(builder, ", \"resolve_resource_id\": ");
        _json_append_escaped_string(builder, attachment->resolve_resource_id);
        _json_append(builder, ", \"resolve_mode\": %" PRIu32, attachment->resolve_mode);
    }
    _json_append(
        builder,
        ", \"clear_color\": [%.9g, %.9g, %.9g, %.9g], \"clear_depth\": %.9g,"
        " \"clear_stencil\": %" PRIu32 " }",
        (double)attachment->clear_color[0], (double)attachment->clear_color[1],
        (double)attachment->clear_color[2], (double)attachment->clear_color[3],
        (double)attachment->clear_depth, attachment->clear_stencil);
}



static void _json_append_graph_pass(JsonBuilder* builder, const DvzFrameGraphPass* pass)
{
    ANN(builder);
    ANN(pass);
    _json_append(builder, "{ \"id\": ");
    _json_append_escaped_string(builder, pass->id);
    _json_append(builder, ", \"kind\": ");
    _json_append_escaped_string(builder, _graph_pass_kind_name(pass->kind));
    _json_append(builder, ", \"panel_id\": ");
    _json_append_escaped_string(builder, pass->panel_id);
    if (pass->has_viewport)
    {
        _json_append(builder, ", \"viewport\": ");
        _json_append_graph_rect(builder, &pass->viewport);
    }
    if (pass->has_scissor)
    {
        _json_append(builder, ", \"scissor\": ");
        _json_append_graph_rect(builder, &pass->scissor);
    }
    _json_append(builder, ", \"reads\": ");
    _json_append_graph_access_array(builder, pass->read_count, pass->reads);
    _json_append(builder, ", \"writes\": ");
    _json_append_graph_access_array(builder, pass->write_count, pass->writes);
    _json_append(builder, ", \"color_attachments\": [");
    for (uint32_t i = 0; i < pass->color_attachment_count; i++)
    {
        if (i > 0)
            _json_append(builder, ", ");
        _json_append_graph_attachment(builder, &pass->color_attachments[i]);
    }
    _json_append(builder, "]");
    if (pass->has_depth_attachment)
    {
        _json_append(builder, ", \"depth_attachment\": ");
        _json_append_graph_attachment(builder, &pass->depth_attachment);
    }
    if (pass->has_stencil_attachment)
    {
        _json_append(builder, ", \"stencil_attachment\": ");
        _json_append_graph_attachment(builder, &pass->stencil_attachment);
    }
    if (pass->alpha_to_coverage)
        _json_append(builder, ", \"alpha_to_coverage\": true");
    if (pass->work_label[0] != '\0')
    {
        _json_append(builder, ", \"work\": ");
        _json_append_escaped_string(builder, pass->work_label);
    }
    _json_append(builder, " }");
}



static void _json_append_node(JsonBuilder* builder, const DvzFramePlanNode* node)
{
    ANN(builder);
    ANN(node);

    switch (node->type)
    {
    case DVZ_FRAME_PLAN_NODE_UPLOAD:
        _json_append(builder, "{ \"type\": \"%s\", \"resource_id\": ", _node_type_name(node->type));
        _json_append_escaped_string(builder, node->u.upload.resource_id);
        _json_append(
            builder, ", \"byte_offset\": %" PRIu64 ", \"byte_size\": %" PRIu64 ", \"data_tag\": ",
            node->u.upload.byte_offset, node->u.upload.byte_size);
        _json_append_escaped_string(builder, node->u.upload.data_tag);
        if (node->u.upload.texture_width > 0 && node->u.upload.texture_height > 0)
        {
            uint32_t texture_depth =
                node->u.upload.texture_depth > 0 ? node->u.upload.texture_depth : 1;
            _json_append(
                builder,
                ", \"texture\": { \"origin_x\": %" PRIu32 ", \"origin_y\": %" PRIu32
                ", \"origin_z\": %" PRIu32 ", \"width\": %" PRIu32
                ", \"height\": %" PRIu32 ", \"depth\": %" PRIu32,
                node->u.upload.texture_origin_x, node->u.upload.texture_origin_y,
                node->u.upload.texture_origin_z, node->u.upload.texture_width,
                node->u.upload.texture_height, texture_depth);
            if (node->u.upload.texture_format != 0)
            {
                _json_append(
                    builder, ", \"format\": %" PRIu32 ", \"bytes_per_texel\": %" PRIu32,
                    node->u.upload.texture_format, node->u.upload.texture_bytes_per_texel);
            }
            if (node->u.upload.texture_alloc_width > 0 &&
                node->u.upload.texture_alloc_height > 0)
            {
                _json_append(
                    builder,
                    ", \"alloc_width\": %" PRIu32 ", \"alloc_height\": %" PRIu32
                    ", \"alloc_depth\": %" PRIu32,
                    node->u.upload.texture_alloc_width, node->u.upload.texture_alloc_height,
                    node->u.upload.texture_alloc_depth > 0 ? node->u.upload.texture_alloc_depth
                                                           : 1);
            }
            _json_append(builder, " }");
        }
        _json_append(builder, " }");
        break;
    case DVZ_FRAME_PLAN_NODE_COMPUTE:
        _json_append(builder, "{ \"type\": \"%s\", \"shader_key\": ", _node_type_name(node->type));
        _json_append_escaped_string(builder, node->u.compute.shader_key);
        _json_append(
            builder, ", \"dispatch\": { \"x\": %" PRIu32 ", \"y\": %" PRIu32
                     ", \"z\": %" PRIu32 " }, \"reads\": ",
            node->u.compute.dispatch[0], node->u.compute.dispatch[1], node->u.compute.dispatch[2]);
        _json_append_string_array(builder, node->u.compute.read_count, node->u.compute.reads);
        _json_append(builder, ", \"writes\": ");
        _json_append_string_array(builder, node->u.compute.write_count, node->u.compute.writes);
        _json_append(builder, " }");
        break;
    case DVZ_FRAME_PLAN_NODE_RENDER:
        _json_append(builder, "{ \"type\": \"%s\", \"panel_id\": ", _node_type_name(node->type));
        _json_append_escaped_string(builder, node->u.render.panel_id);
        _json_append(builder, ", \"render_target_id\": ");
        _json_append_escaped_string(builder, node->u.render.render_target_id);
        _json_append(builder, ", \"pass_role\": ");
        _json_append_escaped_string(builder, _render_pass_role_name(node->u.render.pass_role));
        _json_append(builder, ", \"visuals\": ");
        _json_append_string_array(builder, node->u.render.visual_count, node->u.render.visuals);
        _json_append(builder, ", \"picking\": %s }", node->u.render.picking ? "true" : "false");
        break;
    case DVZ_FRAME_PLAN_NODE_CLEAR:
        _json_append(builder, "{ \"type\": \"%s\", \"panel_id\": ", _node_type_name(node->type));
        _json_append_escaped_string(builder, node->u.clear.panel_id);
        _json_append(builder, ", \"render_target_id\": ");
        _json_append_escaped_string(builder, node->u.clear.render_target_id);
        _json_append(builder, " }");
        break;
    case DVZ_FRAME_PLAN_NODE_COPY:
        _json_append(builder, "{ \"type\": \"%s\", \"src_resource_id\": ", _node_type_name(node->type));
        _json_append_escaped_string(builder, node->u.copy.src_resource_id);
        _json_append(builder, ", \"dst_resource_id\": ");
        _json_append_escaped_string(builder, node->u.copy.dst_resource_id);
        _json_append(
            builder,
            ", \"src_attachment_index\": %" PRIu32
            ", \"src_origin\": { \"x\": %" PRIu32 ", \"y\": %" PRIu32
            ", \"z\": %" PRIu32 " }, \"extent\": { \"width\": %" PRIu32
            ", \"height\": %" PRIu32 ", \"depth\": %" PRIu32 " }, \"format\": %" PRIu32
            ", \"bytes_per_texel\": %" PRIu32 ", \"bytes_per_row\": %" PRIu64
            ", \"rows_per_image\": %" PRIu32 ", \"dst_offset\": %" PRIu64
            ", \"byte_size\": %" PRIu64 ", \"request_id\": %" PRIu64 " }",
            node->u.copy.src_attachment_index, node->u.copy.src_origin[0],
            node->u.copy.src_origin[1], node->u.copy.src_origin[2], node->u.copy.extent[0],
            node->u.copy.extent[1], node->u.copy.extent[2], node->u.copy.format,
            node->u.copy.bytes_per_texel, node->u.copy.bytes_per_row,
            node->u.copy.rows_per_image, node->u.copy.dst_offset, node->u.copy.byte_size,
            node->u.copy.request_id);
        break;
    case DVZ_FRAME_PLAN_NODE_READBACK:
        _json_append(builder, "{ \"type\": \"%s\", \"resource_id\": ", _node_type_name(node->type));
        _json_append_escaped_string(builder, node->u.readback.resource_id);
        _json_append(builder, ", \"request_id\": ");
        _json_append_escaped_string(builder, node->u.readback.request_id);
        _json_append(builder, " }");
        break;
    case DVZ_FRAME_PLAN_NODE_NONE:
        _json_append(builder, "{ \"type\": \"none\" }");
        break;
    default:
        _json_append(builder, "{ \"type\": \"none\" }");
        break;
    }
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Initialize a capability snapshot.
 *
 * @param snapshot the capability snapshot
 */
void dvz_capability_snapshot_default(DvzCapabilitySnapshot* snapshot)
{
    ANN(snapshot);
    dvz_memset(snapshot, sizeof(DvzCapabilitySnapshot), 0, sizeof(DvzCapabilitySnapshot));
    snapshot->max_buffer_size = 256 * 1024 * 1024;
    snapshot->max_texture_dimension_2d = 4096;
    snapshot->max_bind_groups = 4;
    snapshot->max_vertex_buffers = 8;
    snapshot->max_color_attachments = 1;
    snapshot->max_color_sample_count = 16;
    snapshot->max_depth_sample_count = 16;
    snapshot->shader_format_wgsl = true;
    snapshot->shader_format_glsl = true;
    snapshot->render_target_format_rgba16float = false;
    snapshot->render_target_format_r16float = false;
    snapshot->supports_render_target_sampling = false;
    snapshot->supports_color_blending = false;
    snapshot->supports_readback = true;
    snapshot->min_texture_copy_bytes_per_row_alignment = 4;
    snapshot->max_readback_size = snapshot->max_buffer_size;
    snapshot->texture_format_r32uint = true;
    snapshot->texture_format_rg32uint = true;
    snapshot->render_target_format_r32uint = true;
    snapshot->render_target_format_rg32uint = true;
    snapshot->query_profile_u32_r32 = true;
    snapshot->query_profile_u64_rg32 = true;
    snapshot->query_profile_u64_2xr32 = true;
}



/**
 * Copy a capability snapshot.
 *
 * @param dst the destination snapshot
 * @param src the source snapshot
 */
void dvz_capability_snapshot_copy(DvzCapabilitySnapshot* dst, const DvzCapabilitySnapshot* src)
{
    ANN(dst);
    ANN(src);
    dvz_memcpy(dst, sizeof(DvzCapabilitySnapshot), src, sizeof(DvzCapabilitySnapshot));
}



/**
 * Initialize a diagnostic report.
 *
 * @param report the diagnostic report
 */
void dvz_diagnostic_report_init(DvzDiagnosticReport* report)
{
    ANN(report);
    dvz_memset(report, sizeof(DvzDiagnosticReport), 0, sizeof(DvzDiagnosticReport));
}



/**
 * Add a diagnostic message.
 *
 * @param report the diagnostic report
 * @param message the diagnostic message
 * @return whether the message was added
 */
bool dvz_diagnostic_report_add(DvzDiagnosticReport* report, const char* message)
{
    ANN(report);
    ANN(message);
    if (report->count >= DVZ_SCENE_MAX_DIAGNOSTICS)
        return false;
    _frame_plan_copy_label(
        report->messages[report->count], DVZ_SCENE_DIAGNOSTIC_SIZE, message);
    report->count++;
    return true;
}



/**
 * Return a diagnostic count.
 *
 * @param report the diagnostic report
 * @return the number of diagnostic messages
 */
uint32_t dvz_diagnostic_report_count(const DvzDiagnosticReport* report)
{
    if (report == NULL)
        return 0;
    return report->count;
}



/**
 * Return a diagnostic message.
 *
 * @param report the diagnostic report
 * @param index the diagnostic index
 * @return the diagnostic message, or NULL when index is out of bounds
 */
const char* dvz_diagnostic_report_get(const DvzDiagnosticReport* report, uint32_t index)
{
    if (report == NULL || index >= report->count)
        return NULL;
    return report->messages[index];
}



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
DvzFramePlanRenderPassRole dvz_frame_plan_render_pass_role(const DvzFramePlanNode* node)
{
    if (node == NULL || node->type != DVZ_FRAME_PLAN_NODE_RENDER)
        return DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE;
    return node->u.render.pass_role;
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



bool dvz_frame_plan_upload_bytes(
    DvzFramePlan* plan, const char* resource_id, uint64_t byte_offset, uint64_t byte_size,
    const char* data_tag, const void* data)
{
    DvzFramePlanNode* node = _frame_plan_append_node(plan, DVZ_FRAME_PLAN_NODE_UPLOAD);
    if (node == NULL)
        return false;
    _frame_plan_copy_label(node->u.upload.resource_id, DVZ_SCENE_LABEL_SIZE, resource_id ? resource_id : "");
    node->u.upload.byte_offset = byte_offset;
    node->u.upload.byte_size = byte_size;
    _frame_plan_copy_label(node->u.upload.data_tag, DVZ_SCENE_LABEL_SIZE, data_tag ? data_tag : "");
    node->u.upload.data = data;
    node->u.upload.topology = UINT32_MAX;
    return true;
}



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
 * Append a compute node.
 *
 * @param plan the FramePlan
 * @param shader_key the shader key
 * @param x dispatch workgroup count in X
 * @param y dispatch workgroup count in Y
 * @param z dispatch workgroup count in Z
 * @return whether the node was appended
 */
bool dvz_frame_plan_compute(
    DvzFramePlan* plan, const char* shader_key, uint32_t x, uint32_t y, uint32_t z)
{
    DvzFramePlanNode* node = _frame_plan_append_node(plan, DVZ_FRAME_PLAN_NODE_COMPUTE);
    if (node == NULL)
        return false;
    _frame_plan_copy_label(node->u.compute.shader_key, DVZ_SCENE_LABEL_SIZE, shader_key ? shader_key : "");
    node->u.compute.dispatch[0] = x;
    node->u.compute.dispatch[1] = y;
    node->u.compute.dispatch[2] = z;
    return true;
}



/**
 * Add a resource read to the most recent compute node.
 *
 * @param plan the FramePlan
 * @param resource_id the resource id
 * @return whether the resource was appended
 */
bool dvz_frame_plan_compute_read(DvzFramePlan* plan, const char* resource_id)
{
    DvzFramePlanNode* node = _frame_plan_last_node(plan, DVZ_FRAME_PLAN_NODE_COMPUTE);
    if (node == NULL || node->u.compute.read_count >= DVZ_SCENE_MAX_NODE_RESOURCES)
        return false;
    _frame_plan_copy_label(
        node->u.compute.reads[node->u.compute.read_count], DVZ_SCENE_LABEL_SIZE,
        resource_id ? resource_id : "");
    node->u.compute.read_count++;
    return true;
}



/**
 * Add a resource write to the most recent compute node.
 *
 * @param plan the FramePlan
 * @param resource_id the resource id
 * @return whether the resource was appended
 */
bool dvz_frame_plan_compute_write(DvzFramePlan* plan, const char* resource_id)
{
    DvzFramePlanNode* node = _frame_plan_last_node(plan, DVZ_FRAME_PLAN_NODE_COMPUTE);
    if (node == NULL || node->u.compute.write_count >= DVZ_SCENE_MAX_NODE_RESOURCES)
        return false;
    _frame_plan_copy_label(
        node->u.compute.writes[node->u.compute.write_count], DVZ_SCENE_LABEL_SIZE,
        resource_id ? resource_id : "");
    node->u.compute.write_count++;
    return true;
}



/**
 * Append a render node.
 *
 * @param plan the FramePlan
 * @param panel_id the panel id
 * @param render_target_id the render target id
 * @param picking whether the node renders picking output
 * @return whether the node was appended
 */
bool dvz_frame_plan_render(
    DvzFramePlan* plan, const char* panel_id, const char* render_target_id, bool picking)
{
    return dvz_frame_plan_render_panel(
        plan, panel_id, render_target_id, picking, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
}



bool dvz_frame_plan_render_panel(
    DvzFramePlan* plan, const char* panel_id, const char* render_target_id, bool picking,
    DvzPanelDesc desc)
{
    return dvz_frame_plan_render_panel_role(
        plan, panel_id, render_target_id, picking, desc,
        picking ? DVZ_FRAME_PLAN_RENDER_PASS_PICKING : DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE);
}



bool dvz_frame_plan_render_panel_role(
    DvzFramePlan* plan, const char* panel_id, const char* render_target_id, bool picking,
    DvzPanelDesc desc, DvzFramePlanRenderPassRole pass_role)
{
    DvzFramePlanNode* node = _frame_plan_append_node(plan, DVZ_FRAME_PLAN_NODE_RENDER);
    if (node == NULL)
        return false;
    _frame_plan_copy_label(node->u.render.panel_id, DVZ_SCENE_LABEL_SIZE, panel_id ? panel_id : "");
    _frame_plan_copy_label(
        node->u.render.render_target_id, DVZ_SCENE_LABEL_SIZE,
        render_target_id ? render_target_id : "");
    node->u.render.picking = picking;
    node->u.render.pass_role = picking ? DVZ_FRAME_PLAN_RENDER_PASS_PICKING : pass_role;
    node->u.render.desc = desc;
    return true;
}



DvzFramePlanNode* dvz_frame_plan_last_render_node(DvzFramePlan* plan)
{
    return _frame_plan_last_node(plan, DVZ_FRAME_PLAN_NODE_RENDER);
}



/**
 * Append a clear-only render node.
 *
 * @param plan the FramePlan
 * @param panel_id the panel id
 * @param render_target_id the render target id
 * @return whether the node was appended
 */
bool dvz_frame_plan_clear(DvzFramePlan* plan, const char* panel_id, const char* render_target_id)
{
    return dvz_frame_plan_clear_panel(
        plan, panel_id, render_target_id, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
}



bool dvz_frame_plan_clear_panel(
    DvzFramePlan* plan, const char* panel_id, const char* render_target_id, DvzPanelDesc desc)
{
    DvzFramePlanNode* node = _frame_plan_append_node(plan, DVZ_FRAME_PLAN_NODE_CLEAR);
    if (node == NULL)
        return false;
    _frame_plan_copy_label(node->u.clear.panel_id, DVZ_SCENE_LABEL_SIZE, panel_id ? panel_id : "");
    _frame_plan_copy_label(
        node->u.clear.render_target_id, DVZ_SCENE_LABEL_SIZE,
        render_target_id ? render_target_id : "");
    node->u.clear.desc = desc;
    return true;
}



/**
 * Add a visual to the most recent render node.
 *
 * @param plan the FramePlan
 * @param visual_id the visual id
 * @return whether the visual was appended
 */
bool dvz_frame_plan_render_visual(DvzFramePlan* plan, const char* visual_id)
{
    DvzFramePlanNode* node = _frame_plan_last_node(plan, DVZ_FRAME_PLAN_NODE_RENDER);
    if (node == NULL || node->u.render.visual_count >= DVZ_SCENE_MAX_RENDER_VISUALS)
        return false;
    _frame_plan_copy_label(
        node->u.render.visuals[node->u.render.visual_count], DVZ_SCENE_LABEL_SIZE,
        visual_id ? visual_id : "");
    node->u.render.visual_count++;
    return true;
}



/**
 * Attach typed metadata to the most recently appended render visual.
 *
 * @param plan the FramePlan
 * @param metadata the visual metadata
 * @return whether the metadata was attached
 */
bool dvz_frame_plan_render_visual_metadata(
    DvzFramePlan* plan, const DvzFramePlanVisualMeta* metadata)
{
    DvzFramePlanNode* node = _frame_plan_last_node(plan, DVZ_FRAME_PLAN_NODE_RENDER);
    if (node == NULL || metadata == NULL || node->u.render.visual_count == 0)
        return false;
    uint32_t index = node->u.render.visual_count - 1;
    dvz_memcpy(
        &node->u.render.visual_metadata[index], sizeof(DvzFramePlanVisualMeta), metadata,
        sizeof(DvzFramePlanVisualMeta));
    node->u.render.visual_metadata[index].has_metadata = true;
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
        count += _graph_pass_dependency_count(plan, &plan->graph_passes[i], i, UINT32_MAX, NULL);
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
            _graph_pass_dependency_count(plan, &plan->graph_passes[i], i, UINT32_MAX, NULL);
        if (index < base + count)
        {
            (void)_graph_pass_dependency_count(
                plan, &plan->graph_passes[i], i, index - base, out);
            return true;
        }
        base += count;
    }
    return false;
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



/**
 * Validate the typed FramePlan graph descriptors.
 *
 * @param plan the FramePlan
 * @param report the diagnostic report
 * @return whether the graph descriptors are valid
 */
bool dvz_frame_plan_graph_validate(const DvzFramePlan* plan, DvzDiagnosticReport* report)
{
    if (plan == NULL)
    {
        _graph_report(report, "FramePlan graph validation requires a plan");
        return false;
    }

    bool ok = true;
    for (uint32_t i = 0; i < plan->graph_resource_count; i++)
    {
        const DvzFrameGraphResource* resource = &plan->graph_resources[i];
        if (resource->id[0] == '\0' || resource->kind == DVZ_FRAME_GRAPH_RESOURCE_NONE ||
            resource->lifetime == DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_NONE)
        {
            _graph_report(report, "FramePlan graph resource at index %" PRIu32 " is incomplete", i);
            ok = false;
        }
        for (uint32_t j = i + 1; j < plan->graph_resource_count; j++)
        {
            if (strcmp(resource->id, plan->graph_resources[j].id) == 0)
            {
                _graph_report(report, "FramePlan graph resource id '%s' is duplicated", resource->id);
                ok = false;
            }
        }
    }

    for (uint32_t i = 0; i < plan->graph_pass_count; i++)
    {
        const DvzFrameGraphPass* pass = &plan->graph_passes[i];
        if (pass->id[0] == '\0' || pass->kind == DVZ_FRAME_GRAPH_PASS_NONE)
        {
            _graph_report(report, "FramePlan graph pass at index %" PRIu32 " is incomplete", i);
            ok = false;
        }
        ok = _graph_validate_pass_kind(pass, report) && ok;
        ok = _graph_validate_pass_producers(plan, pass, i, report) && ok;
        if (_graph_pass_id_exists_before(plan, pass->id, i))
        {
            _graph_report(report, "FramePlan graph pass id '%s' is duplicated", pass->id);
            ok = false;
        }

        for (uint32_t j = 0; j < pass->read_count; j++)
        {
            if (!_graph_access_reads(pass->reads[j].usage))
            {
                _graph_report(
                    report, "FramePlan graph read access for resource '%s' is not a read usage",
                    pass->reads[j].resource_id);
                ok = false;
                continue;
            }
            ok = _graph_validate_access(plan, &pass->reads[j], i, report) && ok;
        }
        for (uint32_t j = 0; j < pass->write_count; j++)
        {
            if (!_graph_access_writes(pass->writes[j].usage))
            {
                _graph_report(
                    report, "FramePlan graph write access for resource '%s' is not a write usage",
                    pass->writes[j].resource_id);
                ok = false;
                continue;
            }
            ok = _graph_validate_access(plan, &pass->writes[j], i, report) && ok;
        }
        for (uint32_t j = 0; j < pass->color_attachment_count; j++)
            ok = _graph_validate_attachment(
                     plan, &pass->color_attachments[j], DVZ_FRAME_GRAPH_ACCESS_COLOR_ATTACHMENT,
                     i, report) &&
                 ok;
        if (pass->has_depth_attachment)
        {
            DvzFrameGraphAccessUsage usage = pass->depth_attachment.access ==
                                                     DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ
                                                 ? DVZ_FRAME_GRAPH_ACCESS_DEPTH_ATTACHMENT_READ
                                                 : DVZ_FRAME_GRAPH_ACCESS_DEPTH_ATTACHMENT_WRITE;
            ok = _graph_validate_attachment(plan, &pass->depth_attachment, usage, i, report) && ok;
        }
        if (pass->has_stencil_attachment)
        {
            DvzFrameGraphAccessUsage usage = pass->stencil_attachment.access ==
                                                     DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ
                                                 ? DVZ_FRAME_GRAPH_ACCESS_DEPTH_ATTACHMENT_READ
                                                 : DVZ_FRAME_GRAPH_ACCESS_DEPTH_ATTACHMENT_WRITE;
            ok = _graph_validate_attachment(plan, &pass->stencil_attachment, usage, i, report) &&
                 ok;
        }
        if (pass->kind == DVZ_FRAME_GRAPH_PASS_RENDER)
            ok = _graph_validate_render_pass_attachment_extents(plan, pass, report) && ok;
    }
    return ok;
}



/**
 * Serialize a FramePlan as deterministic debug JSON.
 *
 * @param plan the FramePlan
 * @return an owned NUL-terminated JSON string
 */
char* dvz_frame_plan_json(const DvzFramePlan* plan)
{
    if (plan == NULL)
        return NULL;

    JsonBuilder builder = {0};
    if (!_json_init(&builder))
        return NULL;

    _json_append(
        &builder,
        "{\n"
        "  \"frame_plan_schema\": \"0.1\",\n"
        "  \"frame_plan\": {\n"
        "    \"figure_id\": ");
    _json_append_escaped_string(&builder, plan->figure_id);
    _json_append(
        &builder,
        ",\n"
        "    \"frame_index\": %" PRIu64 ",\n"
        "    \"nodes\": [\n",
        plan->frame_index);

    for (uint32_t i = 0; i < plan->count; i++)
    {
        _json_append(&builder, "      ");
        _json_append_node(&builder, &plan->nodes[i]);
        _json_append(&builder, "%s\n", i + 1 < plan->count ? "," : "");
    }

    _json_append(
        &builder,
        "    ],\n"
        "    \"graph\": {\n"
        "      \"resources\": [\n");
    for (uint32_t i = 0; i < plan->graph_resource_count; i++)
    {
        _json_append(&builder, "        ");
        _json_append_graph_resource(&builder, &plan->graph_resources[i]);
        _json_append(&builder, "%s\n", i + 1 < plan->graph_resource_count ? "," : "");
    }

    _json_append(
        &builder,
        "      ],\n"
        "      \"passes\": [\n");
    for (uint32_t i = 0; i < plan->graph_pass_count; i++)
    {
        _json_append(&builder, "        ");
        _json_append_graph_pass(&builder, &plan->graph_passes[i]);
        _json_append(&builder, "%s\n", i + 1 < plan->graph_pass_count ? "," : "");
    }

    _json_append(
        &builder,
        "      ]\n"
        "    }\n"
        "  }\n"
        "}\n");
    if (builder.failed)
    {
        dvz_free(builder.data);
        return NULL;
    }
    return builder.data;
}



/**
 * Serialize graph pass order and inferred dependencies as deterministic debug JSON.
 *
 * @param plan the FramePlan
 * @return an owned NUL-terminated JSON string
 */
char* dvz_frame_plan_graph_dump(const DvzFramePlan* plan)
{
    if (plan == NULL)
        return NULL;

    JsonBuilder builder = {0};
    if (!_json_init(&builder))
        return NULL;

    _json_append(
        &builder,
        "{\n"
        "  \"graph_debug\": {\n"
        "    \"passes\": [\n");
    for (uint32_t i = 0; i < plan->graph_pass_count; i++)
    {
        const DvzFrameGraphPass* pass = &plan->graph_passes[i];
        _json_append(
            &builder,
            "      { \"index\": %" PRIu32 ", \"id\": ",
            i);
        _json_append_escaped_string(&builder, pass->id);
        _json_append(&builder, ", \"kind\": ");
        _json_append_escaped_string(&builder, _graph_pass_kind_name(pass->kind));
        _json_append(&builder, ", \"reads\": ");
        _json_append_graph_access_array(&builder, pass->read_count, pass->reads);
        _json_append(&builder, ", \"writes\": ");
        _json_append_graph_access_array(&builder, pass->write_count, pass->writes);
        _json_append(&builder, ", \"color_attachments\": [");
        for (uint32_t j = 0; j < pass->color_attachment_count; j++)
        {
            if (j > 0)
                _json_append(&builder, ", ");
            _json_append_graph_attachment(&builder, &pass->color_attachments[j]);
        }
        _json_append(&builder, "]");
        if (pass->has_depth_attachment)
        {
            _json_append(&builder, ", \"depth_attachment\": ");
            _json_append_graph_attachment(&builder, &pass->depth_attachment);
        }
        _json_append(&builder, " }%s\n", i + 1 < plan->graph_pass_count ? "," : "");
    }

    uint32_t dependency_count = dvz_frame_plan_graph_dependency_count(plan);
    _json_append(
        &builder,
        "    ],\n"
        "    \"dependencies\": [\n");
    for (uint32_t i = 0; i < dependency_count; i++)
    {
        DvzFrameGraphDependency dep = {0};
        if (!dvz_frame_plan_graph_dependency_get(plan, i, &dep))
            continue;
        _json_append(
            &builder,
            "      { \"resource_id\": ");
        _json_append_escaped_string(&builder, dep.resource_id);
        _json_append(&builder, ", \"producer\": ");
        _json_append_escaped_string(&builder, dep.producer_pass_id);
        _json_append(&builder, ", \"consumer\": ");
        _json_append_escaped_string(&builder, dep.consumer_pass_id);
        _json_append(&builder, ", \"producer_usage\": ");
        _json_append_escaped_string(&builder, _graph_access_usage_name(dep.producer_usage));
        _json_append(&builder, ", \"consumer_usage\": ");
        _json_append_escaped_string(&builder, _graph_access_usage_name(dep.consumer_usage));
        _json_append(&builder, " }%s\n", i + 1 < dependency_count ? "," : "");
    }
    _json_append(
        &builder,
        "    ]\n"
        "  }\n"
        "}\n");
    if (builder.failed)
    {
        dvz_free(builder.data);
        return NULL;
    }
    return builder.data;
}



/**
 * Destroy a JSON string returned by dvz_frame_plan_json().
 *
 * @param json the JSON string
 */
void dvz_frame_plan_json_destroy(char* json) { dvz_free(json); }
