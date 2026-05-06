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

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_frame_plan.h"
#include "_json.h"
#include "_log.h"
#include "_overflow.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static void _copy_label(char* dst, uint64_t dst_size, const char* src)
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



static DvzFramePlanNode* _append_node(DvzFramePlan* plan, DvzFramePlanNodeType type)
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



static DvzFramePlanNode* _last_node(DvzFramePlan* plan, DvzFramePlanNodeType type)
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



static void _json_append_string_array(
    JsonBuilder* builder, uint32_t count, const char values[][DVZ_SCENE_LABEL_SIZE])
{
    ANN(builder);
    _json_append(builder, "[");
    for (uint32_t i = 0; i < count; i++)
        _json_append(builder, "%s\"%s\"", i == 0 ? "" : ", ", values[i]);
    _json_append(builder, "]");
}



static void _json_append_node(JsonBuilder* builder, const DvzFramePlanNode* node)
{
    ANN(builder);
    ANN(node);

    switch (node->type)
    {
    case DVZ_FRAME_PLAN_NODE_UPLOAD:
        _json_append(
            builder,
            "{ \"type\": \"%s\", \"resource_id\": \"%s\", \"byte_offset\": %" PRIu64
            ", \"byte_size\": %" PRIu64 ", \"data_tag\": \"%s\" }",
            _node_type_name(node->type), node->u.upload.resource_id, node->u.upload.byte_offset,
            node->u.upload.byte_size, node->u.upload.data_tag);
        break;
    case DVZ_FRAME_PLAN_NODE_COMPUTE:
        _json_append(
            builder,
            "{ \"type\": \"%s\", \"shader_key\": \"%s\", \"dispatch\": { \"x\": %" PRIu32
            ", \"y\": %" PRIu32 ", \"z\": %" PRIu32 " }, \"reads\": ",
            _node_type_name(node->type), node->u.compute.shader_key, node->u.compute.dispatch[0],
            node->u.compute.dispatch[1], node->u.compute.dispatch[2]);
        _json_append_string_array(builder, node->u.compute.read_count, node->u.compute.reads);
        _json_append(builder, ", \"writes\": ");
        _json_append_string_array(builder, node->u.compute.write_count, node->u.compute.writes);
        _json_append(builder, " }");
        break;
    case DVZ_FRAME_PLAN_NODE_RENDER:
        _json_append(
            builder,
            "{ \"type\": \"%s\", \"panel_id\": \"%s\", \"render_target_id\": \"%s\", "
            "\"visuals\": ",
            _node_type_name(node->type), node->u.render.panel_id, node->u.render.render_target_id);
        _json_append_string_array(builder, node->u.render.visual_count, node->u.render.visuals);
        _json_append(builder, ", \"picking\": %s }", node->u.render.picking ? "true" : "false");
        break;
    case DVZ_FRAME_PLAN_NODE_COPY:
        _json_append(
            builder,
            "{ \"type\": \"%s\", \"src_resource_id\": \"%s\", \"dst_resource_id\": \"%s\", "
            "\"byte_size\": %" PRIu64 " }",
            _node_type_name(node->type), node->u.copy.src_resource_id,
            node->u.copy.dst_resource_id, node->u.copy.byte_size);
        break;
    case DVZ_FRAME_PLAN_NODE_READBACK:
        _json_append(
            builder, "{ \"type\": \"%s\", \"resource_id\": \"%s\", \"request_id\": \"%s\" }",
            _node_type_name(node->type), node->u.readback.resource_id, node->u.readback.request_id);
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
    snapshot->shader_format_wgsl = true;
    snapshot->shader_format_glsl = true;
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
    _copy_label(
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
    _copy_label(plan->figure_id, DVZ_SCENE_LABEL_SIZE, figure_id ? figure_id : "");
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
    dvz_free(plan->nodes);
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
    DvzFramePlanNode* node = _append_node(plan, DVZ_FRAME_PLAN_NODE_UPLOAD);
    if (node == NULL)
        return false;
    _copy_label(node->u.upload.resource_id, DVZ_SCENE_LABEL_SIZE, resource_id ? resource_id : "");
    node->u.upload.byte_offset = byte_offset;
    node->u.upload.byte_size = byte_size;
    _copy_label(node->u.upload.data_tag, DVZ_SCENE_LABEL_SIZE, data_tag ? data_tag : "");
    node->u.upload.data = data;
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
    DvzFramePlanNode* node = _append_node(plan, DVZ_FRAME_PLAN_NODE_COMPUTE);
    if (node == NULL)
        return false;
    _copy_label(node->u.compute.shader_key, DVZ_SCENE_LABEL_SIZE, shader_key ? shader_key : "");
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
    DvzFramePlanNode* node = _last_node(plan, DVZ_FRAME_PLAN_NODE_COMPUTE);
    if (node == NULL || node->u.compute.read_count >= DVZ_SCENE_MAX_NODE_RESOURCES)
        return false;
    _copy_label(
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
    DvzFramePlanNode* node = _last_node(plan, DVZ_FRAME_PLAN_NODE_COMPUTE);
    if (node == NULL || node->u.compute.write_count >= DVZ_SCENE_MAX_NODE_RESOURCES)
        return false;
    _copy_label(
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
    DvzFramePlanNode* node = _append_node(plan, DVZ_FRAME_PLAN_NODE_RENDER);
    if (node == NULL)
        return false;
    _copy_label(node->u.render.panel_id, DVZ_SCENE_LABEL_SIZE, panel_id ? panel_id : "");
    _copy_label(
        node->u.render.render_target_id, DVZ_SCENE_LABEL_SIZE,
        render_target_id ? render_target_id : "");
    node->u.render.picking = picking;
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
    DvzFramePlanNode* node = _last_node(plan, DVZ_FRAME_PLAN_NODE_RENDER);
    if (node == NULL || node->u.render.visual_count >= DVZ_SCENE_MAX_RENDER_VISUALS)
        return false;
    _copy_label(
        node->u.render.visuals[node->u.render.visual_count], DVZ_SCENE_LABEL_SIZE,
        visual_id ? visual_id : "");
    node->u.render.visual_count++;
    return true;
}



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
    DvzFramePlanNode* node = _append_node(plan, DVZ_FRAME_PLAN_NODE_COPY);
    if (node == NULL)
        return false;
    _copy_label(
        node->u.copy.src_resource_id, DVZ_SCENE_LABEL_SIZE,
        src_resource_id ? src_resource_id : "");
    _copy_label(
        node->u.copy.dst_resource_id, DVZ_SCENE_LABEL_SIZE,
        dst_resource_id ? dst_resource_id : "");
    node->u.copy.byte_size = byte_size;
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
    DvzFramePlanNode* node = _append_node(plan, DVZ_FRAME_PLAN_NODE_READBACK);
    if (node == NULL)
        return false;
    _copy_label(
        node->u.readback.resource_id, DVZ_SCENE_LABEL_SIZE, resource_id ? resource_id : "");
    _copy_label(node->u.readback.request_id, DVZ_SCENE_LABEL_SIZE, request_id ? request_id : "");
    return true;
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
        "    \"figure_id\": \"%s\",\n"
        "    \"frame_index\": %" PRIu64 ",\n"
        "    \"nodes\": [\n",
        plan->figure_id, plan->frame_index);

    for (uint32_t i = 0; i < plan->count; i++)
    {
        _json_append(&builder, "      ");
        _json_append_node(&builder, &plan->nodes[i]);
        _json_append(&builder, "%s\n", i + 1 < plan->count ? "," : "");
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
