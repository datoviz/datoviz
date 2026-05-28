/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene FramePlan terminal graph                                                               */
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



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_FRAME_PLAN_ASCII_INITIAL_CAPACITY 4096
#define DVZ_FRAME_PLAN_ASCII_LINE_CAPACITY 1024



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct AsciiBuilder
{
    char* data;
    size_t length;
    size_t capacity;
    bool failed;
} AsciiBuilder;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return a graph pass kind name.
 *
 * @param kind the graph pass kind
 * @return the graph pass kind name
 */
static const char* _ascii_graph_pass_kind_name(DvzFrameGraphPassKind kind)
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



/**
 * Return a graph resource kind name.
 *
 * @param kind the graph resource kind
 * @return the graph resource kind name
 */
static const char* _ascii_graph_resource_kind_name(DvzFrameGraphResourceKind kind)
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



/**
 * Return a graph resource extent kind name.
 *
 * @param kind the graph resource extent kind
 * @return the graph resource extent kind name
 */
static const char* _ascii_graph_extent_kind_name(DvzFrameGraphExtentKind kind)
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



/**
 * Return a graph resource lifetime name.
 *
 * @param lifetime the graph resource lifetime
 * @return the graph resource lifetime name
 */
static const char* _ascii_graph_lifetime_name(DvzFrameGraphResourceLifetime lifetime)
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



/**
 * Return a graph access usage name.
 *
 * @param usage the graph access usage
 * @return the graph access usage name
 */
static const char* _ascii_graph_access_usage_name(DvzFrameGraphAccessUsage usage)
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
 * Return a graph attachment load operation name.
 *
 * @param op the graph attachment load operation
 * @return the graph attachment load operation name
 */
static const char* _ascii_graph_attachment_load_name(DvzFrameGraphAttachmentLoadOp op)
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



/**
 * Return a graph attachment store operation name.
 *
 * @param op the graph attachment store operation
 * @return the graph attachment store operation name
 */
static const char* _ascii_graph_attachment_store_name(DvzFrameGraphAttachmentStoreOp op)
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



/**
 * Initialize an ASCII text builder.
 *
 * @param builder the text builder
 * @return whether the builder was initialized
 */
static bool _ascii_builder_init(AsciiBuilder* builder)
{
    ANN(builder);
    dvz_memset(builder, sizeof(AsciiBuilder), 0, sizeof(AsciiBuilder));
    builder->capacity = DVZ_FRAME_PLAN_ASCII_INITIAL_CAPACITY;
    builder->data = (char*)dvz_calloc(builder->capacity, sizeof(char));
    if (builder->data == NULL)
    {
        builder->failed = true;
        return false;
    }
    return true;
}



/**
 * Ensure the text builder has room for additional bytes.
 *
 * @param builder the text builder
 * @param extra additional bytes excluding the NUL terminator
 * @return whether the builder has enough capacity
 */
static bool _ascii_builder_reserve(AsciiBuilder* builder, size_t extra)
{
    ANN(builder);
    if (builder->failed)
        return false;
    if (extra > SIZE_MAX - builder->length - 1)
    {
        builder->failed = true;
        return false;
    }

    size_t needed = builder->length + extra + 1;
    if (needed <= builder->capacity)
        return true;

    size_t capacity = builder->capacity;
    while (capacity < needed)
    {
        if (capacity > SIZE_MAX / 2)
        {
            builder->failed = true;
            return false;
        }
        capacity *= 2;
    }

    char* data = (char*)dvz_realloc(builder->data, capacity);
    if (data == NULL)
    {
        builder->failed = true;
        return false;
    }
    builder->data = data;
    builder->capacity = capacity;
    return true;
}



/**
 * Append formatted text to a builder.
 *
 * @param builder the text builder
 * @param fmt the printf-style format string
 * @return whether the formatted text was appended
 */
static bool _ascii_append(AsciiBuilder* builder, const char* fmt, ...)
{
    ANN(builder);
    ANN(fmt);
    if (builder->failed)
        return false;

    char line[DVZ_FRAME_PLAN_ASCII_LINE_CAPACITY] = {0};
    va_list args;
    va_start(args, fmt);
    int written = dvz_vsnprintf(line, sizeof(line), fmt, args);
    va_end(args);
    if (written < 0 || (size_t)written >= sizeof(line))
    {
        builder->failed = true;
        return false;
    }

    size_t size = (size_t)written;
    if (!_ascii_builder_reserve(builder, size))
        return false;

    dvz_memcpy(builder->data + builder->length, builder->capacity - builder->length, line, size);
    builder->length += size;
    builder->data[builder->length] = '\0';
    return true;
}



/**
 * Find a graph resource by id.
 *
 * @param plan the FramePlan
 * @param resource_id the graph resource id
 * @return the graph resource, or NULL if absent
 */
static const DvzFrameGraphResource*
_ascii_resource_by_id(const DvzFramePlan* plan, const char* resource_id)
{
    ANN(plan);
    ANN(resource_id);
    for (uint32_t i = 0; i < plan->graph_resource_count; i++)
    {
        const DvzFrameGraphResource* resource = &plan->graph_resources[i];
        if (strcmp(resource->id, resource_id) == 0)
            return resource;
    }
    return NULL;
}



/**
 * Append one resource usage token.
 *
 * @param dst destination buffer
 * @param dst_size destination buffer size
 * @param first whether this is the first token
 * @param token the usage token
 */
static void
_ascii_append_usage_token(char* dst, size_t dst_size, bool* first, const char* token)
{
    ANN(dst);
    ANN(first);
    ANN(token);
    if (!*first)
        strlcat(dst, ",", dst_size);
    strlcat(dst, token, dst_size);
    *first = false;
}



/**
 * Format graph resource usage flags as a compact comma-separated string.
 *
 * @param usage_flags the usage flags
 * @param dst destination buffer
 * @param dst_size destination buffer size
 */
static void _ascii_resource_usage(uint32_t usage_flags, char* dst, size_t dst_size)
{
    ANN(dst);
    if (dst_size == 0)
        return;
    dst[0] = '\0';

    bool first = true;
    if ((usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT) != 0)
        _ascii_append_usage_token(dst, dst_size, &first, "color");
    if ((usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_DEPTH_ATTACHMENT) != 0)
        _ascii_append_usage_token(dst, dst_size, &first, "depth");
    if ((usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED) != 0)
        _ascii_append_usage_token(dst, dst_size, &first, "sampled");
    if ((usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_STORAGE) != 0)
        _ascii_append_usage_token(dst, dst_size, &first, "storage");
    if ((usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_COPY_SRC) != 0)
        _ascii_append_usage_token(dst, dst_size, &first, "copy_src");
    if ((usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_COPY_DST) != 0)
        _ascii_append_usage_token(dst, dst_size, &first, "copy_dst");
    if (first)
        dvz_strlcpy(dst, "none", dst_size);
}



/**
 * Format a byte count as a compact string.
 *
 * @param bytes the byte count
 * @param dst destination buffer
 * @param dst_size destination buffer size
 */
static void _ascii_byte_size(uint64_t bytes, char* dst, size_t dst_size)
{
    ANN(dst);
    if (bytes >= 1024 * 1024)
        dvz_snprintf(dst, dst_size, "%" PRIu64 " MB", bytes / (1024 * 1024));
    else if (bytes >= 1024)
        dvz_snprintf(dst, dst_size, "%" PRIu64 " KB", bytes / 1024);
    else
        dvz_snprintf(dst, dst_size, "%" PRIu64 " B", bytes);
}



/**
 * Return the horizontal arrow glyph for the requested mode.
 *
 * @param flags the terminal graph flags
 * @return the arrow glyph
 */
static const char* _ascii_arrow(uint32_t flags)
{
    return (flags & DVZ_FRAME_PLAN_ASCII_ASCII_ONLY) != 0 ? "->" : "──▶";
}



/**
 * Return the branch glyph for the requested mode.
 *
 * @param flags the terminal graph flags
 * @param last whether this is the last branch
 * @return the branch glyph
 */
static const char* _ascii_branch(uint32_t flags, bool last)
{
    if ((flags & DVZ_FRAME_PLAN_ASCII_ASCII_ONLY) != 0)
        return last ? "`-" : "+-";
    return last ? "└─" : "├─";
}



/**
 * Append a compact resource node line.
 *
 * @param builder the text builder
 * @param resource the graph resource
 * @param flags the terminal graph flags
 */
static void _ascii_append_resource_inline(
    AsciiBuilder* builder, const DvzFrameGraphResource* resource, uint32_t flags)
{
    ANN(builder);
    if (resource == NULL)
    {
        _ascii_append(builder, "(missing resource)\n");
        return;
    }

    char usage[128] = {0};
    _ascii_resource_usage(resource->usage_flags, usage, sizeof(usage));
    _ascii_append(
        builder, "(%s) %s %s %s usage=%s", resource->id,
        _ascii_graph_resource_kind_name(resource->kind),
        _ascii_graph_extent_kind_name(resource->extent_kind),
        _ascii_graph_lifetime_name(resource->lifetime), usage);

    uint32_t sample_count = resource->sample_count != 0 ? resource->sample_count : 1;
    if (resource->width > 0 || resource->height > 0 || resource->depth > 0 || sample_count > 1)
    {
        _ascii_append(
            builder, " extent=%" PRIu32 "x%" PRIu32 "x%" PRIu32 " samples=%" PRIu32,
            resource->width, resource->height, resource->depth, sample_count);
    }
    _ascii_append(builder, "\n");
    (void)flags;
}



/**
 * Append compact upload and readback plan nodes.
 *
 * @param builder the text builder
 * @param plan the FramePlan
 * @param flags the terminal graph flags
 */
static void
_ascii_append_plan_nodes(AsciiBuilder* builder, const DvzFramePlan* plan, uint32_t flags)
{
    ANN(builder);
    ANN(plan);
    bool show_uploads = (flags & DVZ_FRAME_PLAN_ASCII_SHOW_UPLOADS) != 0;
    bool show_readbacks = (flags & DVZ_FRAME_PLAN_ASCII_SHOW_READBACKS) != 0;
    if (!show_uploads && !show_readbacks)
        return;

    _ascii_append(builder, "Plan nodes:\n");
    for (uint32_t i = 0; i < plan->count; i++)
    {
        const DvzFramePlanNode* node = &plan->nodes[i];
        if (node->type == DVZ_FRAME_PLAN_NODE_UPLOAD && show_uploads)
        {
            char size[64] = {0};
            _ascii_byte_size(node->u.upload.byte_size, size, sizeof(size));
            _ascii_append(builder, "[upload #%" PRIu32 "]\n", i);
            _ascii_append(builder, "id: %s\n", node->u.upload.resource_id);
            _ascii_append(builder, "bytes: %s\n", size);
            if (node->u.upload.data_tag[0] != '\0')
                _ascii_append(builder, "tag: %s\n", node->u.upload.data_tag);
            _ascii_append(builder, "\n");
        }
        else if (node->type == DVZ_FRAME_PLAN_NODE_COPY && show_readbacks)
        {
            char size[64] = {0};
            _ascii_byte_size(node->u.copy.byte_size, size, sizeof(size));
            _ascii_append(builder, "[copy #%" PRIu32 "]\n", i);
            _ascii_append(
                builder, "%s %s (%s) %s\n", node->u.copy.src_resource_id, _ascii_arrow(flags),
                node->u.copy.dst_resource_id, size);
            _ascii_append(builder, "\n");
        }
        else if (node->type == DVZ_FRAME_PLAN_NODE_READBACK && show_readbacks)
        {
            _ascii_append(builder, "[readback #%" PRIu32 "]\n", i);
            _ascii_append(builder, "resource: %s\n", node->u.readback.resource_id);
            _ascii_append(builder, "request: %s\n\n", node->u.readback.request_id);
        }
    }
}



/**
 * Append the incoming read edges for one graph pass.
 *
 * @param builder the text builder
 * @param plan the FramePlan
 * @param pass the graph pass
 * @param flags the terminal graph flags
 */
static void _ascii_append_pass_reads(
    AsciiBuilder* builder, const DvzFramePlan* plan, const DvzFrameGraphPass* pass, uint32_t flags)
{
    ANN(builder);
    ANN(plan);
    ANN(pass);
    for (uint32_t i = 0; i < pass->read_count; i++)
    {
        const DvzFrameGraphAccess* read = &pass->reads[i];
        const DvzFrameGraphResource* resource = _ascii_resource_by_id(plan, read->resource_id);
        _ascii_append_resource_inline(builder, resource, flags);
        _ascii_append(
            builder, "        %s %s %s\n", _ascii_graph_access_usage_name(read->usage),
            _ascii_arrow(flags), pass->id);
    }
}



/**
 * Append the outgoing attachment and write edges for one graph pass.
 *
 * @param builder the text builder
 * @param plan the FramePlan
 * @param pass the graph pass
 * @param flags the terminal graph flags
 */
static void _ascii_append_pass_writes(
    AsciiBuilder* builder, const DvzFramePlan* plan, const DvzFrameGraphPass* pass, uint32_t flags)
{
    ANN(builder);
    ANN(plan);
    ANN(pass);
    uint32_t edge_count = pass->color_attachment_count + pass->write_count;
    if (pass->has_depth_attachment)
        edge_count++;
    if (pass->has_stencil_attachment)
        edge_count++;
    if (edge_count == 0)
        return;

    uint32_t edge_index = 0;
    for (uint32_t i = 0; i < pass->color_attachment_count; i++, edge_index++)
    {
        const DvzFrameGraphAttachment* attachment = &pass->color_attachments[i];
        const DvzFrameGraphResource* resource =
            _ascii_resource_by_id(plan, attachment->resource_id);
        _ascii_append(
            builder, "        %s color[%s/%s] %s ",
            _ascii_branch(flags, edge_index + 1 == edge_count),
            _ascii_graph_attachment_load_name(attachment->load_op),
            _ascii_graph_attachment_store_name(attachment->store_op), _ascii_arrow(flags));
        _ascii_append_resource_inline(builder, resource, flags);
    }
    if (pass->has_depth_attachment)
    {
        const DvzFrameGraphAttachment* attachment = &pass->depth_attachment;
        const DvzFrameGraphResource* resource =
            _ascii_resource_by_id(plan, attachment->resource_id);
        _ascii_append(
            builder, "        %s depth[%s/%s] %s ",
            _ascii_branch(flags, edge_index + 1 == edge_count),
            _ascii_graph_attachment_load_name(attachment->load_op),
            _ascii_graph_attachment_store_name(attachment->store_op), _ascii_arrow(flags));
        _ascii_append_resource_inline(builder, resource, flags);
        edge_index++;
    }
    if (pass->has_stencil_attachment)
    {
        const DvzFrameGraphAttachment* attachment = &pass->stencil_attachment;
        const DvzFrameGraphResource* resource =
            _ascii_resource_by_id(plan, attachment->resource_id);
        _ascii_append(
            builder, "        %s stencil[%s/%s] %s ",
            _ascii_branch(flags, edge_index + 1 == edge_count),
            _ascii_graph_attachment_load_name(attachment->load_op),
            _ascii_graph_attachment_store_name(attachment->store_op), _ascii_arrow(flags));
        _ascii_append_resource_inline(builder, resource, flags);
        edge_index++;
    }
    for (uint32_t i = 0; i < pass->write_count; i++, edge_index++)
    {
        const DvzFrameGraphAccess* write = &pass->writes[i];
        const DvzFrameGraphResource* resource = _ascii_resource_by_id(plan, write->resource_id);
        _ascii_append(
            builder, "        %s %s %s ", _ascii_branch(flags, edge_index + 1 == edge_count),
            _ascii_graph_access_usage_name(write->usage), _ascii_arrow(flags));
        _ascii_append_resource_inline(builder, resource, flags);
    }
}



/**
 * Append one graph pass node and its resource edges.
 *
 * @param builder the text builder
 * @param plan the FramePlan
 * @param pass_index the graph pass index
 * @param flags the terminal graph flags
 */
static void _ascii_append_pass(
    AsciiBuilder* builder, const DvzFramePlan* plan, uint32_t pass_index, uint32_t flags)
{
    ANN(builder);
    ANN(plan);
    ASSERT(pass_index < plan->graph_pass_count);
    const DvzFrameGraphPass* pass = &plan->graph_passes[pass_index];

    _ascii_append_pass_reads(builder, plan, pass, flags);
    _ascii_append(
        builder, "[%s #%" PRIu32 "]\n", _ascii_graph_pass_kind_name(pass->kind), pass_index);
    _ascii_append(builder, "id: %s\n", pass->id);
    if (pass->panel_id[0] != '\0')
        _ascii_append(builder, "panel: %s\n", pass->panel_id);
    if (pass->work_label[0] != '\0')
        _ascii_append(builder, "work: %s\n", pass->work_label);
    if ((flags & DVZ_FRAME_PLAN_ASCII_VERBOSE) != 0)
    {
        _ascii_append(
            builder, "reads: %" PRIu32 " writes: %" PRIu32 " colors: %" PRIu32 "\n",
            pass->read_count, pass->write_count, pass->color_attachment_count);
        if (pass->has_viewport)
        {
            _ascii_append(
                builder, "viewport: %.0f,%.0f %.0fx%.0f\n", (double)pass->viewport.x,
                (double)pass->viewport.y, (double)pass->viewport.width,
                (double)pass->viewport.height);
        }
    }
    _ascii_append_pass_writes(builder, plan, pass, flags);
    _ascii_append(builder, "\n");
}



/**
 * Append the deterministic dependency edge list.
 *
 * @param builder the text builder
 * @param plan the FramePlan
 */
static void _ascii_append_dependency_list(AsciiBuilder* builder, const DvzFramePlan* plan)
{
    ANN(builder);
    ANN(plan);
    uint32_t dependency_count = dvz_frame_plan_graph_dependency_count(plan);
    _ascii_append(builder, "Edges:\n");
    if (dependency_count == 0)
    {
        _ascii_append(builder, "  none\n");
        return;
    }

    for (uint32_t i = 0; i < dependency_count; i++)
    {
        DvzFrameGraphDependency dep = {0};
        if (!dvz_frame_plan_graph_dependency_get(plan, i, &dep))
            continue;
        _ascii_append(
            builder, "  [%s #%" PRIu32 " %s] -> [%s #%" PRIu32 " %s] via (%s) %s -> %s\n",
            _ascii_graph_pass_kind_name(plan->graph_passes[dep.producer_pass_index].kind),
            dep.producer_pass_index, dep.producer_pass_id,
            _ascii_graph_pass_kind_name(plan->graph_passes[dep.consumer_pass_index].kind),
            dep.consumer_pass_index, dep.consumer_pass_id, dep.resource_id,
            _ascii_graph_access_usage_name(dep.producer_usage),
            _ascii_graph_access_usage_name(dep.consumer_usage));
    }
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Serialize a FramePlan graph as a terminal-readable text sketch.
 *
 * @param plan the FramePlan
 * @param flags terminal graph formatting flags
 * @return an owned NUL-terminated text string, or NULL on failure
 */
char* dvz_frame_plan_graph_ascii(const DvzFramePlan* plan, uint32_t flags)
{
    if (plan == NULL)
        return NULL;

    AsciiBuilder builder = {0};
    if (!_ascii_builder_init(&builder))
        return NULL;

    _ascii_append(
        &builder,
        "FramePlan figure=%s frame=%" PRIu64 "\n"
        "Graph resources=%" PRIu32 " passes=%" PRIu32 " dependencies=%" PRIu32 "\n\n",
        plan->figure_id, plan->frame_index, plan->graph_resource_count, plan->graph_pass_count,
        dvz_frame_plan_graph_dependency_count(plan));

    _ascii_append_plan_nodes(&builder, plan, flags);

    for (uint32_t i = 0; i < plan->graph_pass_count; i++)
        _ascii_append_pass(&builder, plan, i, flags);

    _ascii_append_dependency_list(&builder, plan);

    if (builder.failed)
    {
        dvz_free(builder.data);
        return NULL;
    }
    return builder.data;
}



/**
 * Destroy a terminal graph text string.
 *
 * @param text the text string returned by `dvz_frame_plan_graph_ascii()`
 */
void dvz_frame_plan_graph_ascii_destroy(char* text)
{
    dvz_free(text);
}
