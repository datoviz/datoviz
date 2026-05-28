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
    _json_append_escaped_string(builder, _frame_graph_access_usage_name(access->usage));
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
        _json_append_escaped_string(&builder, _frame_graph_access_usage_name(dep.producer_usage));
        _json_append(&builder, ", \"consumer_usage\": ");
        _json_append_escaped_string(&builder, _frame_graph_access_usage_name(dep.consumer_usage));
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
