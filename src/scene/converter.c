/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene to DRP2 converter                                                                      */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_frame_plan.h"
#include "datoviz/drp2.h"
#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DRP2_ID_COLOR_TARGET 1
#define DRP2_ID_ENCODER 2
#define DRP2_ID_RENDER_PASS 3
#define DRP2_ID_COMMAND_BUFFER 4
#define DRP2_ID_SUBMISSION 5
#define DRP2_ID_PIPELINE 10
#define DRP2_ID_RESOURCE_BASE 20
#define DRP2_ID_READBACK_BUFFER 12
#define DRP2_ID_VERTEX_SHADER 9000
#define DRP2_ID_FRAGMENT_SHADER 9001
#define DRP2_MAX_FIXTURE_RESOURCES 64

#define DRP2_VERTEX_WGSL                                                                        \
    "@vertex fn main() -> @builtin(position) vec4f { return vec4f(0.0, 0.0, 0.0, 1.0); }"
#define DRP2_FRAGMENT_WGSL                                                                      \
    "@fragment fn main() -> @location(0) vec4f { return vec4f(1.0, 1.0, 1.0, 1.0); }"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct ResourceId ResourceId;
typedef struct ConverterState ConverterState;

struct ResourceId
{
    char key[DVZ_SCENE_LABEL_SIZE];
    uint64_t id;
};

struct ConverterState
{
    uint32_t count;
    uint64_t next_id;
    uint64_t first_vertex_buffer_id;
    ResourceId resources[DRP2_MAX_FIXTURE_RESOURCES];
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Initialize converter state.
 *
 * @param state the converter state
 */
static void _state_init(ConverterState* state)
{
    ANN(state);
    dvz_memset(state, sizeof(ConverterState), 0, sizeof(ConverterState));
    state->next_id = DRP2_ID_RESOURCE_BASE;
}



/**
 * Return a deterministic DRP2 id for a scene resource key.
 *
 * @param state the converter state
 * @param key the scene resource key
 * @return the DRP2 id, or 0 when the map is full
 */
static uint64_t _resource_id(ConverterState* state, const char* key)
{
    ANN(state);
    ANN(key);
    for (uint32_t i = 0; i < state->count; i++)
    {
        if (strcmp(state->resources[i].key, key) == 0)
            return state->resources[i].id;
    }
    if (state->count >= DRP2_MAX_FIXTURE_RESOURCES)
        return 0;

    ResourceId* resource = &state->resources[state->count++];
    dvz_strlcpy(resource->key, key, sizeof(resource->key));
    resource->id = state->next_id++;
    return resource->id;
}

/**
 * Fill a base64 string representing zero-initialized bytes.
 *
 * @param byte_size the decoded byte size
 * @param out the output string
 * @param out_size the output string capacity
 * @return whether the string was written
 */
static bool _zero_base64(uint64_t byte_size, char* out, uint64_t out_size)
{
    ANN(out);
    if (out_size == 0)
        return false;

    uint64_t groups = byte_size / 3;
    uint64_t remainder = byte_size % 3;
    uint64_t count = groups * 4 + (remainder == 0 ? 0 : 4);
    if (count + 1 > out_size)
        return false;

    for (uint64_t i = 0; i < count; i++)
        out[i] = 'A';
    if (remainder == 1)
    {
        out[count - 2] = '=';
        out[count - 1] = '=';
    }
    else if (remainder == 2)
    {
        out[count - 1] = '=';
    }
    out[count] = '\0';
    return true;
}



/**
 * Return the first node of a given type.
 *
 * @param plan the FramePlan
 * @param type the node type
 * @return the first matching node, or NULL
 */
static const DvzFramePlanNode* _first_node_of_type(
    const DvzFramePlan* plan, DvzFramePlanNodeType type)
{
    ANN(plan);
    for (uint32_t i = 0; i < plan->count; i++)
    {
        if (plan->nodes[i].type == type)
            return &plan->nodes[i];
    }
    return NULL;
}



/**
 * Emit DRP2 commands for an upload node.
 *
 * @param state the converter state
 * @param stream the DRP2 command stream
 * @param node the upload node
 * @return whether the commands were emitted
 */
static bool
_emit_upload(ConverterState* state, DvzDrp2CommandStream* stream, const DvzFramePlanNode* node)
{
    ANN(state);
    ANN(stream);
    ANN(node);

    uint64_t id = _resource_id(state, node->u.upload.resource_id);
    if (id == 0)
        return false;
    if (state->first_vertex_buffer_id == 0)
        state->first_vertex_buffer_id = id;

    char data[128] = {0};
    if (!_zero_base64(node->u.upload.byte_size, data, sizeof(data)))
        return false;

    uint64_t buffer_size = node->u.upload.byte_offset + node->u.upload.byte_size;
    uint32_t usage = DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_VERTEX;
    return dvz_drp2_stream_create_buffer(
               stream, id, buffer_size, usage) &&
           dvz_drp2_stream_write_buffer(
               stream, id, node->u.upload.byte_offset, node->u.upload.byte_size, data);
}



/**
 * Emit DRP2 setup commands for a readback buffer.
 *
 * @param stream the DRP2 command stream
 * @param node the copy node
 * @return whether the commands were emitted
 */
static bool _emit_readback_buffer(DvzDrp2CommandStream* stream, const DvzFramePlanNode* node)
{
    ANN(stream);
    ANN(node);

    uint32_t usage = DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ;
    return dvz_drp2_stream_create_buffer(
        stream, DRP2_ID_READBACK_BUFFER, node->u.copy.byte_size, usage);
}



/**
 * Emit DRP2 render-pass commands for a render node.
 *
 * @param stream the DRP2 command stream
 * @param node the render node
 * @param vertex_buffer_id the vertex buffer id
 * @return whether the commands were emitted
 */
static bool
_emit_render(DvzDrp2CommandStream* stream, const DvzFramePlanNode* node, uint64_t vertex_buffer_id)
{
    ANN(stream);
    ANN(node);
    (void)node;
    if (vertex_buffer_id == 0)
        return false;

    return dvz_drp2_stream_create_shader_module(
               stream, DRP2_ID_VERTEX_SHADER, "VERTEX", DRP2_VERTEX_WGSL) &&
           dvz_drp2_stream_create_shader_module(
               stream, DRP2_ID_FRAGMENT_SHADER, "FRAGMENT", DRP2_FRAGMENT_WGSL) &&
           dvz_drp2_stream_create_render_pipeline(
               stream, DRP2_ID_PIPELINE, DRP2_ID_VERTEX_SHADER, DRP2_ID_FRAGMENT_SHADER, 1) &&
           dvz_drp2_stream_create_texture_2d(stream, DRP2_ID_COLOR_TARGET, 4, 4) &&
           dvz_drp2_stream_begin_command_encoder(stream, DRP2_ID_ENCODER) &&
           dvz_drp2_stream_begin_render_pass(
               stream, DRP2_ID_RENDER_PASS, DRP2_ID_ENCODER, DRP2_ID_COLOR_TARGET) &&
           dvz_drp2_stream_set_pipeline(stream, DRP2_ID_RENDER_PASS, DRP2_ID_PIPELINE) &&
           dvz_drp2_stream_set_vertex_buffer(
               stream, DRP2_ID_RENDER_PASS, 0, vertex_buffer_id, 0) &&
           dvz_drp2_stream_draw(stream, DRP2_ID_RENDER_PASS, 3, 1, 0, 0) &&
           dvz_drp2_stream_end_render_pass(stream, DRP2_ID_RENDER_PASS);
}



/**
 * Emit DRP2 commands for a copy/readback path.
 *
 * @param stream the DRP2 command stream
 * @param copy the copy node
 * @param readback the readback node
 * @return whether the commands were emitted
 */
static bool _emit_readback(
    DvzDrp2CommandStream* stream, const DvzFramePlanNode* copy, const DvzFramePlanNode* readback)
{
    ANN(stream);
    ANN(copy);
    ANN(readback);
    (void)readback;

    return dvz_drp2_stream_copy_texture_to_buffer(
        stream, DRP2_ID_ENCODER, DRP2_ID_COLOR_TARGET, DRP2_ID_READBACK_BUFFER, 0, 1, 1, 4, 1);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Emit a DRP2 command stream from a FramePlan in fixture mode.
 *
 * @param plan the FramePlan
 * @param caps the capability snapshot
 * @param report the diagnostic report
 * @return an owned DRP2 command stream, or NULL on failure
 */
DvzDrp2CommandStream* dvz_frame_plan_emit_drp2(
    const DvzFramePlan* plan, const DvzCapabilitySnapshot* caps, DvzDiagnosticReport* report)
{
    ANN(plan);
    (void)caps;

    const DvzFramePlanNode* upload = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_UPLOAD);
    const DvzFramePlanNode* render = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_RENDER);
    const DvzFramePlanNode* copy = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_COPY);
    const DvzFramePlanNode* readback = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_READBACK);
    if (upload == NULL || render == NULL)
    {
        if (report != NULL)
            (void)dvz_diagnostic_report_add(report, "fixture converter requires upload+render");
        return NULL;
    }
    if (readback != NULL && copy == NULL)
    {
        if (report != NULL)
            (void)dvz_diagnostic_report_add(report, "fixture converter requires copy before readback");
        return NULL;
    }

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    ConverterState state = {0};
    _state_init(&state);

    bool ok = dvz_drp2_stream_hello_renderer(stream, "scene-fixture") &&
              dvz_drp2_stream_renderer_hello_reply(stream, "datoviz-drp2-fixture");
    for (uint32_t i = 0; ok && i < plan->count; i++)
    {
        if (plan->nodes[i].type == DVZ_FRAME_PLAN_NODE_UPLOAD)
            ok = _emit_upload(&state, stream, &plan->nodes[i]);
    }
    ok = ok && (copy == NULL || _emit_readback_buffer(stream, copy)) &&
         _emit_render(stream, render, state.first_vertex_buffer_id) &&
         (copy == NULL || readback == NULL || _emit_readback(stream, copy, readback)) &&
         dvz_drp2_stream_finish_command_encoder(stream, DRP2_ID_ENCODER, DRP2_ID_COMMAND_BUFFER) &&
         (readback != NULL
              ? dvz_drp2_stream_queue_submit_readback(
                    stream, DRP2_ID_COMMAND_BUFFER, DRP2_ID_SUBMISSION, DRP2_ID_READBACK_BUFFER,
                    0, copy->u.copy.byte_size)
              : dvz_drp2_stream_queue_submit(
                    stream, DRP2_ID_COMMAND_BUFFER, DRP2_ID_SUBMISSION));
    if (!ok)
    {
        if (report != NULL)
            (void)dvz_diagnostic_report_add(report, "failed to emit DRP2 fixture stream");
        dvz_drp2_stream_destroy(stream);
        return NULL;
    }
    return stream;
}
