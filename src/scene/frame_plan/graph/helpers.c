/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene FramePlan graph helpers                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "frame_plan/internal.h"
#include "internal.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return a stable graph access usage name.
 *
 * @param usage graph access usage
 * @return graph access usage name
 */
const char* _frame_graph_access_usage_name(DvzFrameGraphAccessUsage usage)
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



/**
 * Resolve a graph resource index by resource id.
 *
 * @param plan the FramePlan
 * @param resource_id the graph resource id
 * @param index optional output resource index
 * @return whether the resource exists
 */
bool _frame_plan_graph_resource_index(
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



/**
 * Return whether a graph pass id appears before a bounded pass index.
 *
 * @param plan the FramePlan
 * @param pass_id the graph pass id
 * @param end exclusive upper pass index
 * @return whether the id was found
 */
bool _frame_plan_graph_pass_id_exists_before(
    const DvzFramePlan* plan, const char* pass_id, uint32_t end)
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



/**
 * Return the graph resource usage flag required by one access usage.
 *
 * @param usage graph access usage
 * @return required graph resource usage flag
 */
uint32_t _frame_plan_graph_usage_flag(DvzFrameGraphAccessUsage usage)
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



/**
 * Return whether one graph access usage reads a resource.
 *
 * @param usage graph access usage
 * @return whether the usage reads
 */
bool _frame_plan_graph_access_reads(DvzFrameGraphAccessUsage usage)
{
    return usage == DVZ_FRAME_GRAPH_ACCESS_SAMPLED ||
           usage == DVZ_FRAME_GRAPH_ACCESS_STORAGE_READ ||
           usage == DVZ_FRAME_GRAPH_ACCESS_DEPTH_ATTACHMENT_READ ||
           usage == DVZ_FRAME_GRAPH_ACCESS_COPY_SRC;
}



/**
 * Return whether one graph access usage writes a resource.
 *
 * @param usage graph access usage
 * @return whether the usage writes
 */
bool _frame_plan_graph_access_writes(DvzFrameGraphAccessUsage usage)
{
    return usage == DVZ_FRAME_GRAPH_ACCESS_STORAGE_WRITE ||
           usage == DVZ_FRAME_GRAPH_ACCESS_COLOR_ATTACHMENT ||
           usage == DVZ_FRAME_GRAPH_ACCESS_DEPTH_ATTACHMENT_WRITE ||
           usage == DVZ_FRAME_GRAPH_ACCESS_COPY_DST;
}



/**
 * Return whether one graph attachment reads its resource.
 *
 * @param attachment graph attachment descriptor
 * @return whether the attachment reads
 */
bool _frame_plan_graph_attachment_reads(const DvzFrameGraphAttachment* attachment)
{
    ANN(attachment);
    return attachment->access == DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ ||
           attachment->access == DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ_WRITE ||
           attachment->load_op == DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD;
}



/**
 * Return whether one graph attachment writes its resource.
 *
 * @param attachment graph attachment descriptor
 * @return whether the attachment writes
 */
bool _frame_plan_graph_attachment_writes(const DvzFrameGraphAttachment* attachment)
{
    ANN(attachment);
    return attachment->access == DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE ||
           attachment->access == DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ_WRITE;
}



/**
 * Return whether one graph pass writes a resource.
 *
 * @param pass graph pass descriptor
 * @param resource_id graph resource id
 * @return whether the pass writes the resource
 */
bool _frame_plan_graph_pass_writes_resource(const DvzFrameGraphPass* pass, const char* resource_id)
{
    ANN(pass);
    ANN(resource_id);
    for (uint32_t i = 0; i < pass->write_count; i++)
    {
        if (_frame_plan_graph_access_writes(pass->writes[i].usage) &&
            strcmp(pass->writes[i].resource_id, resource_id) == 0)
            return true;
    }
    for (uint32_t i = 0; i < pass->color_attachment_count; i++)
    {
        if (_frame_plan_graph_attachment_writes(&pass->color_attachments[i]) &&
            strcmp(pass->color_attachments[i].resource_id, resource_id) == 0)
            return true;
        if (pass->color_attachments[i].resolve_resource_id[0] != '\0' &&
            strcmp(pass->color_attachments[i].resolve_resource_id, resource_id) == 0)
            return true;
    }
    if (pass->has_depth_attachment &&
        _frame_plan_graph_attachment_writes(&pass->depth_attachment) &&
        strcmp(pass->depth_attachment.resource_id, resource_id) == 0)
        return true;
    if (pass->has_stencil_attachment &&
        _frame_plan_graph_attachment_writes(&pass->stencil_attachment) &&
        strcmp(pass->stencil_attachment.resource_id, resource_id) == 0)
        return true;
    return false;
}



/**
 * Return whether a graph resource is per-frame.
 *
 * @param plan the FramePlan
 * @param resource_id the graph resource id
 * @return whether the resource has per-frame lifetime
 */
bool _frame_plan_graph_resource_is_per_frame(const DvzFramePlan* plan, const char* resource_id)
{
    ANN(plan);
    ANN(resource_id);
    uint32_t resource_index = 0;
    if (!_frame_plan_graph_resource_index(plan, resource_id, &resource_index))
        return false;
    return plan->graph_resources[resource_index].lifetime ==
           DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
}



/**
 * Return the graph access represented by a color attachment.
 *
 * @param attachment the graph attachment descriptor
 * @return graph color attachment access
 */
DvzFrameGraphAccessUsage
_frame_plan_graph_color_attachment_usage(const DvzFrameGraphAttachment* attachment)
{
    ANN(attachment);
    (void)attachment;
    return DVZ_FRAME_GRAPH_ACCESS_COLOR_ATTACHMENT;
}



/**
 * Return the graph access represented by a depth attachment.
 *
 * @param attachment the graph attachment descriptor
 * @return graph depth attachment access
 */
DvzFrameGraphAccessUsage
_frame_plan_graph_depth_attachment_usage(const DvzFrameGraphAttachment* attachment)
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
 * @param pass the graph pass descriptor
 * @param resource_id the graph resource id
 * @param count output producer declaration count
 * @param usage optional output last producer usage
 * @return whether at least one producer declaration exists
 */
bool _frame_plan_graph_pass_write_count_resource(
    const DvzFrameGraphPass* pass, const char* resource_id, uint32_t* count,
    DvzFrameGraphAccessUsage* usage)
{
    ANN(pass);
    ANN(resource_id);
    ANN(count);
    *count = 0;
    for (uint32_t i = 0; i < pass->write_count; i++)
    {
        if (_frame_plan_graph_access_writes(pass->writes[i].usage) &&
            strcmp(pass->writes[i].resource_id, resource_id) == 0)
        {
            *count += 1;
            if (usage != NULL)
                *usage = pass->writes[i].usage;
        }
    }
    for (uint32_t i = 0; i < pass->color_attachment_count; i++)
    {
        if (_frame_plan_graph_attachment_writes(&pass->color_attachments[i]) &&
            strcmp(pass->color_attachments[i].resource_id, resource_id) == 0)
        {
            *count += 1;
            if (usage != NULL)
                *usage = _frame_plan_graph_color_attachment_usage(&pass->color_attachments[i]);
        }
        if (pass->color_attachments[i].resolve_resource_id[0] != '\0' &&
            strcmp(pass->color_attachments[i].resolve_resource_id, resource_id) == 0)
        {
            *count += 1;
            if (usage != NULL)
                *usage = DVZ_FRAME_GRAPH_ACCESS_COLOR_ATTACHMENT;
        }
    }
    if (pass->has_depth_attachment &&
        _frame_plan_graph_attachment_writes(&pass->depth_attachment) &&
        strcmp(pass->depth_attachment.resource_id, resource_id) == 0)
    {
        *count += 1;
        if (usage != NULL)
            *usage = _frame_plan_graph_depth_attachment_usage(&pass->depth_attachment);
    }
    if (pass->has_stencil_attachment &&
        _frame_plan_graph_attachment_writes(&pass->stencil_attachment) &&
        strcmp(pass->stencil_attachment.resource_id, resource_id) == 0)
    {
        *count += 1;
        if (usage != NULL)
            *usage = _frame_plan_graph_depth_attachment_usage(&pass->stencil_attachment);
    }
    return *count > 0;
}



/**
 * Find the latest graph pass that writes a resource before a consumer pass.
 *
 * @param plan the FramePlan
 * @param resource_id the graph resource id
 * @param pass_index the consumer pass index
 * @param producer_index output producer pass index
 * @param producer_usage optional output producer access usage
 * @return whether a producer was found
 */
bool _frame_plan_graph_find_last_writer_before(
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
        const DvzFrameGraphPass* pass = &plan->graph_passes[i - 1];
        bool has_writer =
            _frame_plan_graph_pass_write_count_resource(pass, resource_id, &count, &usage);
        if (has_writer)
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
 * @param plan the FramePlan
 * @param resource_id the graph resource id
 * @param pass_index the consumer pass index
 * @param producer_index output producer pass index
 * @param producer_usage optional output producer access usage
 * @return whether a producer was found after the consumer
 */
bool _frame_plan_graph_find_first_writer_after(
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
        if (_frame_plan_graph_pass_write_count_resource(
                &plan->graph_passes[i], resource_id, &count, &usage))
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
 * @param plan the FramePlan
 * @param resource_id the graph resource id
 * @param consumer_usage the consumer access usage
 * @param consumer_index the consumer pass index
 * @param out optional output dependency descriptor
 * @return whether a dependency edge exists
 */
bool _frame_plan_graph_dependency_from_access(
    const DvzFramePlan* plan, const char* resource_id, DvzFrameGraphAccessUsage consumer_usage,
    uint32_t consumer_index, DvzFrameGraphDependency* out)
{
    ANN(plan);
    ANN(resource_id);
    uint32_t producer_index = 0;
    DvzFrameGraphAccessUsage producer_usage = DVZ_FRAME_GRAPH_ACCESS_NONE;
    if (!_frame_plan_graph_find_last_writer_before(
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
 * @param plan the FramePlan
 * @param pass the graph pass descriptor
 * @param pass_index the graph pass index
 * @param target_index dependency index to materialize
 * @param out optional output dependency descriptor
 * @return dependency count for the pass
 */
uint32_t _frame_plan_graph_pass_dependency_count(
    const DvzFramePlan* plan, const DvzFrameGraphPass* pass, uint32_t pass_index,
    uint32_t target_index, DvzFrameGraphDependency* out)
{
    ANN(plan);
    ANN(pass);
    uint32_t count = 0;
#define COUNT_DEP(resource, usage)                                                                \
    do                                                                                            \
    {                                                                                             \
        if (_frame_plan_graph_dependency_from_access(                                             \
                plan, (resource), (usage), pass_index, count == target_index ? out : NULL))       \
            count++;                                                                              \
    } while (0)

    for (uint32_t i = 0; i < pass->read_count; i++)
    {
        if (_frame_plan_graph_access_reads(pass->reads[i].usage))
            COUNT_DEP(pass->reads[i].resource_id, pass->reads[i].usage);
    }
    for (uint32_t i = 0; i < pass->color_attachment_count; i++)
    {
        const DvzFrameGraphAttachment* attachment = &pass->color_attachments[i];
        if (_frame_plan_graph_attachment_reads(attachment))
            COUNT_DEP(
                attachment->resource_id, _frame_plan_graph_color_attachment_usage(attachment));
    }
    if (pass->has_depth_attachment && _frame_plan_graph_attachment_reads(&pass->depth_attachment))
    {
        COUNT_DEP(
            pass->depth_attachment.resource_id,
            _frame_plan_graph_depth_attachment_usage(&pass->depth_attachment));
    }
    if (pass->has_stencil_attachment &&
        _frame_plan_graph_attachment_reads(&pass->stencil_attachment))
    {
        COUNT_DEP(
            pass->stencil_attachment.resource_id,
            _frame_plan_graph_depth_attachment_usage(&pass->stencil_attachment));
    }
#undef COUNT_DEP
    return count;
}



/**
 * Return whether a resource has been written before a graph pass.
 *
 * @param plan the FramePlan
 * @param resource_id the graph resource id
 * @param pass_index the graph pass index
 * @return whether a prior write exists
 */
bool _frame_plan_graph_resource_written_before(
    const DvzFramePlan* plan, const char* resource_id, uint32_t pass_index)
{
    ANN(plan);
    ANN(resource_id);
    for (uint32_t i = 0; i < pass_index && i < plan->graph_pass_count; i++)
    {
        if (_frame_plan_graph_pass_writes_resource(&plan->graph_passes[i], resource_id))
            return true;
    }
    return false;
}



/**
 * Return whether a graph resource can be used as a color attachment.
 *
 * @param resource graph resource descriptor
 * @return whether the resource is color-attachment compatible
 */
bool _frame_plan_graph_resource_is_color_attachment_compatible(
    const DvzFrameGraphResource* resource)
{
    ANN(resource);
    return resource->kind == DVZ_FRAME_GRAPH_RESOURCE_TEXTURE ||
           resource->kind == DVZ_FRAME_GRAPH_RESOURCE_EXTERNAL_TARGET;
}



/**
 * Return whether a graph resource can be used as a depth attachment.
 *
 * @param resource graph resource descriptor
 * @return whether the resource is depth-attachment compatible
 */
bool _frame_plan_graph_resource_is_depth_attachment_compatible(
    const DvzFrameGraphResource* resource)
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
uint32_t _frame_plan_graph_resource_sample_count(const DvzFrameGraphResource* resource)
{
    return resource != NULL && resource->sample_count != 0 ? resource->sample_count : 1;
}



/**
 * Return whether a graph resource sample count is supported.
 *
 * @param sample_count the effective sample count
 * @return whether the value is valid
 */
bool _frame_plan_graph_resource_sample_count_valid(uint32_t sample_count)
{
    return sample_count == 1 || sample_count == 2 || sample_count == 4 || sample_count == 8 ||
           sample_count == 16;
}



/**
 * Return whether two graph resource extents match.
 *
 * @param a first graph resource descriptor
 * @param b second graph resource descriptor
 * @return whether the extents match
 */
bool _frame_plan_graph_resource_extent_matches(
    const DvzFrameGraphResource* a, const DvzFrameGraphResource* b)
{
    ANN(a);
    ANN(b);

    if (a->extent_kind != b->extent_kind)
        return false;
    if (a->extent_kind == DVZ_FRAME_GRAPH_EXTENT_FIXED ||
        a->extent_kind == DVZ_FRAME_GRAPH_EXTENT_PANEL)
        return a->width == b->width && a->height == b->height;
    if (a->extent_kind == DVZ_FRAME_GRAPH_EXTENT_RESOURCE_REF)
        return strcmp(a->extent_resource_id, b->extent_resource_id) == 0;
    return a->extent_kind != DVZ_FRAME_GRAPH_EXTENT_NONE;
}



/**
 * Append a formatted graph diagnostic.
 *
 * @param report optional diagnostic report
 * @param fmt printf-style format string
 * @return whether the diagnostic was appended
 */
bool _frame_plan_graph_report(DvzDiagnosticReport* report, const char* fmt, ...)
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
