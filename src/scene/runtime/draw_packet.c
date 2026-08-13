/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene draw packets                                                                           */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "frame_plan/emit.h"
#include "_frame_plan_runtime_internal.h"
#include "_visual_pipeline.h"
#include "_visual_pipeline_internal.h"
#include "datoviz/drp2.h"
#include "datoviz/drp2/stream.h"
#include "datoviz/scene.h"


/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return a compact resource-role name for draw resource diagnostics.
 *
 * @param state resource id state
 * @param id resource id
 * @return the resource-role name
 */
static const char* _draw_packet_role_name(const ConverterState* state, uint64_t id)
{
    ANN(state);
    const char* tag = _resource_data_tag(state, id);
    if (tag != NULL && tag[0] != '\0')
        return tag;

    switch (_resource_role(state, id))
    {
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION:
        return "position";
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION_START:
        return "position_start";
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION_END:
        return "position_end";
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION_NEXT:
        return "position_next";
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_COLOR:
        return "color";
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_SIZE:
        return "size";
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_SIGMA:
        return "sigma";
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_ANGLE:
        return "angle";
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_SHAPE:
        return "shape";
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_LINE_WIDTH:
        return "line_width";
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXCOORDS:
        return "texcoords";
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXTURE:
        return "texture";
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_NORMAL:
        return "normal";
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_INDEX:
        return "index";
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_PRIMITIVE_SHADING:
        return "primitive_shading";
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_MATERIAL_PARAMS:
        return "material_params";
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_ITEM_STATE:
        return "item_state";
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_ITEM_STATE_STYLE:
        return "item_state_style";
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_PATH_FLAGS:
        return "path_flags";
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_PATH_DISTANCE:
        return "path_distance";
    case DVZ_FRAME_PLAN_RESOURCE_ROLE_NONE:
    default:
        return "unknown";
    }
}



/**
 * Resolve the logical item count for a draw-bound buffer.
 *
 * @param state resource id state
 * @param id resource id
 * @param stride required stride in bytes
 * @param kind visual descriptor kind
 * @param role resource role name
 * @param report optional diagnostic report
 * @param out_count output logical item count
 * @return whether a valid logical count was resolved
 */
static bool _draw_packet_resource_count(
    const ConverterState* state, uint64_t id, uint32_t stride, DvzSceneVisualDescKind kind,
    const char* role, DvzDiagnosticReport* report, uint64_t* out_count)
{
    ANN(state);
    ANN(role);
    ANN(out_count);

    uint64_t logical_count = _resource_logical_item_count(state, id);
    if (_resource_has_logical_extent(state, id))
    {
        if (logical_count == 0)
        {
            char message[DVZ_SCENE_DIAGNOSTIC_SIZE];
            dvz_snprintf(
                message, sizeof(message),
                "scene draw resource validation failed: visual=%s role=%s resource_id=%" PRIu64
                " logical_item_count=0",
                _scene_visual_desc_kind_name(kind), role, id);
            _diagnostic(report, message);
            return false;
        }
        *out_count = logical_count;
        return true;
    }

    uint64_t byte_size = _resource_byte_size(state, id);
    if (stride == 0 || byte_size == 0 || byte_size % stride != 0 || byte_size / stride == 0)
    {
        char message[DVZ_SCENE_DIAGNOSTIC_SIZE];
        dvz_snprintf(
            message, sizeof(message),
            "scene draw resource validation failed: visual=%s role=%s resource_id=%" PRIu64
            " byte_size=%" PRIu64 " stride=%" PRIu32,
            _scene_visual_desc_kind_name(kind), role, id, byte_size, stride);
        _diagnostic(report, message);
        return false;
    }

    *out_count = byte_size / stride;
    return true;
}



/**
 * Validate the logical coverage of all draw-bound resources in one packet.
 *
 * @param packet draw packet to validate
 * @param report optional diagnostic report
 * @return whether all draw-bound resources cover the packet draw range
 */
static bool _draw_packet_validate(const SceneDrawPacket* packet, DvzDiagnosticReport* report)
{
    ANN(packet);

    for (uint32_t i = 0; i < packet->vertex_buffer_count; i++)
    {
        const SceneDrawVertexBuffer* vertex = &packet->vertex_buffers[i];
        if (!vertex->validates_draw)
            continue;

        uint64_t draw_count =
            vertex->step_mode == DVZ_DRP2_VERTEX_STEP_MODE_INSTANCE
                ? (uint64_t)packet->first_instance + packet->instance_count
                : (uint64_t)packet->first_vertex + packet->vertex_count;
        if (draw_count > vertex->logical_count)
        {
            char message[DVZ_SCENE_DIAGNOSTIC_SIZE];
            dvz_snprintf(
                message, sizeof(message),
                "scene draw resource validation failed: visual=%s role=%s resource_id=%" PRIu64
                " draw_count=%" PRIu64 " logical_count=%" PRIu64,
                _scene_visual_desc_kind_name(packet->kind), vertex->role, vertex->buffer_id,
                draw_count, vertex->logical_count);
            _diagnostic(report, message);
            return false;
        }
    }

    if (packet->indexed)
    {
        uint64_t draw_count = (uint64_t)packet->first_index + packet->index_count;
        if (draw_count > packet->index_logical_count)
        {
            char message[DVZ_SCENE_DIAGNOSTIC_SIZE];
            dvz_snprintf(
                message, sizeof(message),
                "scene draw resource validation failed: visual=%s role=index resource_id=%" PRIu64
                " draw_count=%" PRIu64 " logical_count=%" PRIu64,
                _scene_visual_desc_kind_name(packet->kind), packet->index_buffer_id,
                draw_count, packet->index_logical_count);
            _diagnostic(report, message);
            return false;
        }
    }

    return true;
}



/**
 * Fill the vertex-buffer section of one draw packet from a visual and pipeline contract.
 *
 * @param state resource id state
 * @param visual visual descriptor being packetized
 * @param pipeline pipeline vertex-input descriptor
 * @param report optional diagnostic report
 * @param out output draw packet
 * @return whether all required vertex bindings were packetized
 */
static bool _draw_packet_fill_vertex_buffers(
    const ConverterState* state, const DvzSceneVisualDesc* visual,
    const DvzSceneVisualPipelineDesc* pipeline, DvzDiagnosticReport* report,
    SceneDrawPacket* out)
{
    ANN(state);
    ANN(visual);
    ANN(pipeline);
    ANN(out);

    if (pipeline->binding_count > visual->vbuf_count)
    {
        char message[DVZ_SCENE_DIAGNOSTIC_SIZE];
        dvz_snprintf(
            message, sizeof(message),
            "scene draw resource validation failed: visual=%s binding_count=%" PRIu32
            " vbuf_count=%" PRIu32,
            _scene_visual_desc_kind_name(visual->kind), pipeline->binding_count,
            visual->vbuf_count);
        _diagnostic(report, message);
        return false;
    }

    out->vertex_buffer_count = visual->vbuf_count;
    out->validation_binding_count = pipeline->binding_count;
    for (uint32_t slot = 0; slot < visual->vbuf_count; slot++)
    {
        SceneDrawVertexBuffer* vertex = &out->vertex_buffers[slot];
        vertex->slot = slot;
        vertex->buffer_id = visual->vbuf_ids[slot];
        vertex->role = _draw_packet_role_name(state, vertex->buffer_id);
        if (slot >= pipeline->binding_count)
            continue;

        vertex->stride = pipeline->strides[slot];
        vertex->step_mode = pipeline->step_modes[slot];
        vertex->validates_draw = true;
        if (!_draw_packet_resource_count(
                state, vertex->buffer_id, vertex->stride, visual->kind, vertex->role, report,
                &vertex->logical_count))
            return false;
    }

    return true;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Initialize and validate one retained-visual draw packet.
 *
 * @param state resource id state
 * @param visual visual descriptor being packetized
 * @param pipeline pipeline vertex-input descriptor
 * @param pipeline_id emitted DRP2 pipeline id
 * @param bg_set0 bind group for set 0, or zero
 * @param bg_set1 bind group for set 1, or zero
 * @param bg_set2 bind group for set 2, or zero
 * @param bg_set3 bind group for set 3, or zero
 * @param clip_rect pass-local clip rectangle selection
 * @param viewport_rect pass-local viewport rectangle selection
 * @param report optional diagnostic report
 * @param out output draw packet
 * @return whether the packet is valid for DRP2 lowering
 */
bool _scene_draw_packet_init(
    const ConverterState* state, const DvzSceneVisualDesc* visual,
    const DvzSceneVisualPipelineDesc* pipeline, uint64_t pipeline_id, uint64_t bg_set0,
    uint64_t bg_set1, uint64_t bg_set2, uint64_t bg_set3, DvzFramePlanClipRect clip_rect,
    DvzFramePlanViewportRect viewport_rect, DvzSceneShaderFormat shader_format,
    DvzDiagnosticReport* report, SceneDrawPacket* out)
{
    ANN(state);
    ANN(visual);
    ANN(pipeline);
    ANN(out);

    dvz_memset(out, sizeof(SceneDrawPacket), 0, sizeof(SceneDrawPacket));
    out->kind = visual->kind;
    out->pipeline_id = pipeline_id;
    out->bg_set0 = bg_set0;
    out->bg_set1 = bg_set1;
    out->bg_set2 = bg_set2;
    out->bg_set3 = bg_set3;
    out->clip_rect = clip_rect;
    out->viewport_rect = viewport_rect;
    DvzSceneVisualDrawDesc draw_desc = {0};
    if (!_scene_visual_draw_desc(visual, shader_format, &draw_desc))
    {
        char message[DVZ_SCENE_DIAGNOSTIC_SIZE];
        dvz_snprintf(
            message, sizeof(message),
            "scene draw descriptor resolution failed: visual=%s visual_type=%u shader_format=%u",
            _scene_visual_desc_kind_name(visual->kind), visual->visual_type,
            (uint32_t)shader_format);
        _diagnostic(report, message);
        return false;
    }
    out->vertex_count = draw_desc.vertex_count;
    out->instance_count = draw_desc.instance_count;
    out->first_vertex = draw_desc.first_vertex;
    out->first_instance = draw_desc.first_instance;
    out->index_buffer_id = draw_desc.index_buffer_id;
    out->index_format = draw_desc.index_format;
    out->first_index = draw_desc.first_index;
    out->base_vertex = draw_desc.base_vertex;
    out->index_count = draw_desc.index_count;
    out->indexed = draw_desc.indexed;

    if (!_draw_packet_fill_vertex_buffers(state, visual, pipeline, report, out))
        return false;

    if (out->indexed)
    {
        out->index_stride = _resource_item_stride(state, visual->index_buffer_id);
        if (out->index_stride == 0)
            out->index_stride = visual->index_format != NULL &&
                                        strcmp(visual->index_format, "uint16") == 0
                                    ? sizeof(uint16_t)
                                    : sizeof(uint32_t);
        if (!_draw_packet_resource_count(
                state, visual->index_buffer_id, out->index_stride, visual->kind, "index", report,
                &out->index_logical_count))
            return false;
    }

    return _draw_packet_validate(out, report);
}



/**
 * Lower one validated scene draw packet to DRP2 render-pass commands.
 *
 * @param stream destination DRP2 command stream
 * @param render_pass_id active render pass id
 * @param packet validated draw packet
 * @return whether all DRP2 commands were appended successfully
 */
bool _scene_draw_packet_emit(
    DvzDrp2CommandStream* stream, uint64_t render_pass_id, const SceneDrawPacket* packet)
{
    ANN(stream);
    ANN(packet);

    bool ok = true;
    for (uint32_t i = 0; ok && i < packet->vertex_buffer_count; i++)
    {
        const SceneDrawVertexBuffer* vertex = &packet->vertex_buffers[i];
        ok = dvz_drp2_stream_set_vertex_buffer(
            stream, render_pass_id, vertex->slot, vertex->buffer_id, 0);
    }
    if (ok && packet->indexed)
    {
        ok = dvz_drp2_stream_set_index_buffer(
                 stream, render_pass_id, packet->index_buffer_id, packet->index_format, 0) &&
             dvz_drp2_stream_draw_indexed(
                 stream, render_pass_id, packet->index_count, packet->instance_count,
                 packet->first_index, packet->base_vertex, packet->first_instance);
    }
    else
    {
        ok = ok && dvz_drp2_stream_draw(
                       stream, render_pass_id, packet->vertex_count, packet->instance_count,
                       packet->first_vertex, packet->first_instance);
    }

    return ok;
}
