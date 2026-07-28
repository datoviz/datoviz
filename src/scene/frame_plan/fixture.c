/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene FramePlan fixture emission                                                             */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include <vulkan/vulkan_core.h>

#include "_alloc.h"
#include "_assertions.h"
#include "frame_plan/emit.h"
#include "_overflow.h"
#include "_shader_registry.h"
#include "datoviz/drp2/stream.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DRP2_FIXTURE_TRIANGLE_VERTEX_BYTES 36



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

#define DVZ_FRAME_PLAN_EMIT_CONFIG_KNOWN_FLAGS 0u



static bool _frame_plan_emit_config_validate(
    const DvzFramePlanEmitConfig* cfg, DvzDiagnosticReport* report)
{
    if (cfg == NULL)
    {
        _diagnostic(report, "missing DvzFramePlanEmitConfig");
        return false;
    }
    if (!DVZ_STRUCT_VALID(cfg, DvzFramePlanEmitConfig, DVZ_FRAME_PLAN_EMIT_CONFIG_KNOWN_FLAGS))
    {
        _diagnostic(report, "invalid DvzFramePlanEmitConfig ABI prologue");
        return false;
    }
    return true;
}

/**
 * Emit the fixture triangle pipeline with explicit portable vertex metadata.
 *
 * @param stream the DRP2 command stream
 * @return whether the pipeline command was emitted
 */
static bool _emit_fixture_triangle_pipeline(DvzDrp2CommandStream* stream)
{
    ANN(stream);
    uint32_t strides[1]   = {12};
    uint32_t bindings[1]  = {0};
    uint32_t locations[1] = {0};
    DvzFormat formats[1]  = {DVZ_FORMAT_R32G32B32_SFLOAT};
    uint32_t offsets[1]   = {0};
    DvzDrp2RenderPipelineDesc pipeline = dvz_drp2_render_pipeline_desc();
    pipeline.id = DRP2_ID_PIPELINE;
    pipeline.vertex_shader_module_id = DRP2_ID_VERTEX_SHADER;
    pipeline.fragment_shader_module_id = DRP2_ID_FRAGMENT_SHADER;
    pipeline.vertex_buffer_slots = 1;
    pipeline.binding_count = 1;
    pipeline.binding_strides = strides;
    pipeline.attr_count = 1;
    pipeline.attr_bindings = bindings;
    pipeline.attr_locations = locations;
    pipeline.attr_formats = formats;
    pipeline.attr_offsets = offsets;
    return dvz_drp2_stream_create_render_pipeline(stream, &pipeline);
}



/**
 * Return the fixture vertex shader that consumes the explicit triangle vertex layout.
 *
 * @return the WGSL source
 */
static const char* _fixture_vertex_input_wgsl(void)
{
    return "@vertex fn main(@location(0) position: vec3f) -> @builtin(position) vec4f { return "
           "vec4f(position, 1.0); }";
}



/**
 * Return the GLSL fallback that consumes the explicit triangle vertex layout.
 *
 * @return the GLSL source
 */
static const char* _fixture_vertex_input_glsl(void)
{
    return "#version 450\nlayout(location=0)in vec3 pos;void main(){gl_Position=vec4(pos,1.0);}";
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

    char* data = _zero_base64_alloc(node->u.upload.byte_size);
    if (data == NULL)
        return false;

    uint64_t buffer_size = 0;
    if (_dvz_add_u64_overflows(node->u.upload.byte_offset, node->u.upload.byte_size, &buffer_size))
    {
        dvz_free(data);
        return false;
    }
    if (buffer_size < DRP2_FIXTURE_TRIANGLE_VERTEX_BYTES)
        buffer_size = DRP2_FIXTURE_TRIANGLE_VERTEX_BYTES;
    uint32_t usage = node->u.upload.buffer_usage != 0
                         ? node->u.upload.buffer_usage
                         : (DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_VERTEX);
    bool ok = dvz_drp2_stream_create_buffer(stream, id, buffer_size, usage) &&
              dvz_drp2_stream_write_buffer_base64(
                  stream, id, node->u.upload.byte_offset, node->u.upload.byte_size, data);
    dvz_free(data);
    return ok;
}



/**
 * Emit DRP2 commands for a texture upload node.
 *
 * @param state the converter state
 * @param stream the DRP2 command stream
 * @param node the upload node
 * @return whether the commands were emitted
 */
static bool _emit_texture_upload(
    ConverterState* state, DvzDrp2CommandStream* stream, const DvzFramePlanNode* node)
{
    ANN(state);
    ANN(stream);
    ANN(node);

    uint64_t id = _resource_id(state, node->u.upload.resource_id);
    if (id == 0)
        return false;
    if (state->first_texture_id == 0)
        state->first_texture_id = id;

    char* data = _zero_base64_alloc(node->u.upload.byte_size);
    if (data == NULL)
        return false;

    uint32_t usage = DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING | DVZ_DRP2_TEXTURE_USAGE_COPY_DST;
    bool ok = dvz_drp2_stream_create_texture_2d_usage(stream, id, 2, 2, usage) &&
              dvz_drp2_stream_write_texture_2d_base64(stream, id, 0, 2, 2, 8, 2, data);
    dvz_free(data);
    return ok;
}



/**
 * Emit DRP2 commands for the compute-assisted input and output buffers.
 *
 * @param state the converter state
 * @param stream the DRP2 command stream
 * @param upload the input upload node
 * @param compute the compute node
 * @return whether the commands were emitted
 */
static bool _emit_compute_buffers(
    ConverterState* state, DvzDrp2CommandStream* stream, const DvzFramePlanNode* upload,
    const DvzFramePlanNode* compute)
{
    ANN(state);
    ANN(stream);
    ANN(upload);
    ANN(compute);
    if (compute->u.compute.write_count == 0)
        return false;

    uint64_t input_id = _resource_id(state, upload->u.upload.resource_id);
    uint64_t output_id = _resource_id(state, compute->u.compute.writes[0]);
    if (input_id == 0 || output_id == 0)
        return false;

    state->first_compute_input_id = input_id;
    state->first_compute_output_id = output_id;
    state->first_vertex_buffer_id = output_id;
    state->compute_buffer_size = upload->u.upload.byte_size;

    char* data = _zero_base64_alloc(upload->u.upload.byte_size);
    if (data == NULL)
        return false;

    bool ok = dvz_drp2_stream_create_buffer(
                  stream, input_id, upload->u.upload.byte_size,
                  DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_STORAGE) &&
              dvz_drp2_stream_write_buffer_base64(
                  stream, input_id, upload->u.upload.byte_offset, upload->u.upload.byte_size,
                  data) &&
              dvz_drp2_stream_create_buffer(
                  stream, output_id, upload->u.upload.byte_size,
                  DVZ_DRP2_BUFFER_USAGE_STORAGE | DVZ_DRP2_BUFFER_USAGE_VERTEX);
    dvz_free(data);
    return ok;
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
 * Create a texture and copy an uploaded buffer into it before rendering.
 */
static bool _emit_buffer_to_texture(
    ConverterState* state, DvzDrp2CommandStream* stream, const DvzFramePlanNode* copy)
{
    ANN(state);
    ANN(stream);
    ANN(copy);
    if (copy->u.copy.dst_origin[0] != 0 || copy->u.copy.dst_origin[1] != 0 ||
        copy->u.copy.dst_origin[2] != 0 || copy->u.copy.extent[2] != 1)
        return false;

    uint64_t src_id = _resource_lookup_id(state, copy->u.copy.src_resource_id);
    uint64_t dst_id = _resource_id(state, copy->u.copy.dst_resource_id);
    if (src_id == 0 || dst_id == 0)
        return false;
    state->first_texture_id = dst_id;
    uint32_t usage = DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING | DVZ_DRP2_TEXTURE_USAGE_COPY_DST;
    uint64_t encoder_id = state->next_id++;
    uint64_t command_buffer_id = state->next_id++;
    uint64_t submission_id = state->next_id++;
    return dvz_drp2_stream_create_texture_2d_format_usage(
               stream, dst_id, copy->u.copy.extent[0], copy->u.copy.extent[1],
               (DvzFormat)copy->u.copy.format, usage) &&
           dvz_drp2_stream_begin_command_encoder(stream, encoder_id) &&
           dvz_drp2_stream_copy_buffer_to_texture(
               stream, encoder_id, src_id, copy->u.copy.src_offset, dst_id,
               copy->u.copy.extent[0], copy->u.copy.extent[1],
               (uint32_t)copy->u.copy.bytes_per_row, copy->u.copy.rows_per_image) &&
           dvz_drp2_stream_finish_command_encoder(stream, encoder_id, command_buffer_id) &&
           dvz_drp2_stream_queue_submit(stream, command_buffer_id, submission_id);
}



/**
 * Emit a compute pass followed by a render pass in one encoder.
 *
 * @param stream the DRP2 command stream
 * @param compute the compute node
 * @param render the render node
 * @param state the converter state
 * @param cfg the emission config
 * @return whether the commands were emitted
 */
static bool _emit_compute_assisted_render(
    DvzDrp2CommandStream* stream, const DvzFramePlanNode* compute,
    const DvzFramePlanNode* render, const ConverterState* state, const DvzFramePlanEmitConfig* cfg)
{
    ANN(stream);
    ANN(compute);
    ANN(render);
    ANN(state);
    if (state->first_compute_input_id == 0 || state->first_compute_output_id == 0 ||
        state->compute_buffer_size == 0)
        return false;

    return dvz_drp2_stream_create_storage_bind_group_layout(stream, DRP2_ID_BIND_GROUP_LAYOUT) &&
           _emit_shader(
               stream, DRP2_ID_COMPUTE_SHADER, "COMPUTE", _compute_copy_wgsl(),
               _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_COMPUTE_COPY, false), cfg) &&
           dvz_drp2_stream_create_compute_pipeline_with_bind_group_layout(
               stream, DRP2_ID_COMPUTE_PIPELINE, DRP2_ID_COMPUTE_SHADER,
               DRP2_ID_BIND_GROUP_LAYOUT) &&
           dvz_drp2_stream_create_storage_bind_group(
               stream, DRP2_ID_BIND_GROUP, DRP2_ID_BIND_GROUP_LAYOUT,
               state->first_compute_input_id, state->first_compute_output_id,
               state->compute_buffer_size) &&
           _emit_shader(
               stream, DRP2_ID_VERTEX_SHADER, "VERTEX", _fixture_vertex_input_wgsl(),
               _fixture_vertex_input_glsl(), cfg) &&
           _emit_shader(
               stream, DRP2_ID_FRAGMENT_SHADER, "FRAGMENT", _fixture_fragment_wgsl(),
               _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_FIXTURE, true), cfg) &&
           _emit_fixture_triangle_pipeline(stream) &&
           dvz_drp2_stream_create_texture_2d(stream, DRP2_ID_COLOR_TARGET, 4, 4) &&
           dvz_drp2_stream_begin_command_encoder(stream, DRP2_ID_ENCODER) &&
           dvz_drp2_stream_begin_compute_pass(stream, DRP2_ID_COMPUTE_PASS, DRP2_ID_ENCODER) &&
           dvz_drp2_stream_set_pipeline(
               stream, DRP2_ID_COMPUTE_PASS, DRP2_ID_COMPUTE_PIPELINE) &&
           dvz_drp2_stream_set_bind_group(
               stream, DRP2_ID_COMPUTE_PASS, 0, DRP2_ID_BIND_GROUP) &&
           dvz_drp2_stream_dispatch_workgroups(
               stream, DRP2_ID_COMPUTE_PASS, compute->u.compute.dispatch[0],
               compute->u.compute.dispatch[1], compute->u.compute.dispatch[2]) &&
           dvz_drp2_stream_end_compute_pass(stream, DRP2_ID_COMPUTE_PASS) &&
           dvz_drp2_stream_resource_barrier(
               stream, DRP2_ID_ENCODER, state->first_compute_output_id, "COMPUTE",
               "STORAGE_WRITE", "VERTEX_INPUT", "VERTEX_READ", 0, state->compute_buffer_size) &&
           dvz_drp2_stream_begin_render_pass(
               stream, DRP2_ID_RENDER_PASS, DRP2_ID_ENCODER, DRP2_ID_COLOR_TARGET) &&
           dvz_drp2_stream_set_pipeline(stream, DRP2_ID_RENDER_PASS, DRP2_ID_PIPELINE) &&
           dvz_drp2_stream_set_vertex_buffer(
               stream, DRP2_ID_RENDER_PASS, 0, state->first_compute_output_id, 0) &&
           dvz_drp2_stream_draw(stream, DRP2_ID_RENDER_PASS, 3, 1, 0, 0) &&
           dvz_drp2_stream_end_render_pass(stream, DRP2_ID_RENDER_PASS);
}



/**
 * Emit DRP2 texture-sampling render-pass commands for a render node.
 *
 * @param stream the DRP2 command stream
 * @param node the render node
 * @param texture_id the sampled texture id
 * @param cfg the emission config
 * @return whether the commands were emitted
 */
static bool
_emit_texture_render(
    DvzDrp2CommandStream* stream, const DvzFramePlanNode* node, uint64_t texture_id,
    const DvzFramePlanEmitConfig* cfg)
{
    ANN(stream);
    ANN(node);
    (void)node;
    if (texture_id == 0)
        return false;

    uint64_t layouts[1] = {DRP2_ID_BIND_GROUP_LAYOUT};
    DvzDrp2RenderPipelineDesc pipeline = dvz_drp2_render_pipeline_desc();
    pipeline.id = DRP2_ID_PIPELINE;
    pipeline.vertex_shader_module_id = DRP2_ID_VERTEX_SHADER;
    pipeline.fragment_shader_module_id = DRP2_ID_FRAGMENT_SHADER;
    pipeline.bind_group_layout_count = 1;
    pipeline.bind_group_layout_ids = layouts;

    return dvz_drp2_stream_create_sampler(stream, DRP2_ID_SAMPLER) &&
           dvz_drp2_stream_create_texture_sampler_bind_group_layout(
               stream, DRP2_ID_BIND_GROUP_LAYOUT) &&
           _emit_shader(
               stream, DRP2_ID_VERTEX_SHADER, "VERTEX", _texture_vertex_wgsl(),
               _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_TEXTURE, false), cfg) &&
           _emit_shader(
               stream, DRP2_ID_FRAGMENT_SHADER, "FRAGMENT", _texture_fragment_wgsl(),
               _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_TEXTURE, true), cfg) &&
           dvz_drp2_stream_create_render_pipeline(stream, &pipeline) &&
           dvz_drp2_stream_create_texture_sampler_bind_group(
               stream, DRP2_ID_BIND_GROUP, DRP2_ID_BIND_GROUP_LAYOUT, texture_id, DRP2_ID_SAMPLER) &&
           dvz_drp2_stream_create_texture_2d(stream, DRP2_ID_COLOR_TARGET, 4, 4) &&
           dvz_drp2_stream_begin_command_encoder(stream, DRP2_ID_ENCODER) &&
           dvz_drp2_stream_begin_render_pass(
               stream, DRP2_ID_RENDER_PASS, DRP2_ID_ENCODER, DRP2_ID_COLOR_TARGET) &&
           dvz_drp2_stream_set_pipeline(stream, DRP2_ID_RENDER_PASS, DRP2_ID_PIPELINE) &&
           dvz_drp2_stream_set_bind_group(stream, DRP2_ID_RENDER_PASS, 0, DRP2_ID_BIND_GROUP) &&
           dvz_drp2_stream_draw(stream, DRP2_ID_RENDER_PASS, 3, 1, 0, 0) &&
           dvz_drp2_stream_end_render_pass(stream, DRP2_ID_RENDER_PASS);
}



/**
 * Emit DRP2 render-pass commands for a render node.
 *
 * @param stream the DRP2 command stream
 * @param node the render node
 * @param vertex_buffer_id the vertex buffer id
 * @param cfg the emission config
 * @return whether the commands were emitted
 */
static bool
_emit_render(
    DvzDrp2CommandStream* stream, const DvzFramePlanNode* node, uint64_t vertex_buffer_id,
    const DvzFramePlanEmitConfig* cfg)
{
    ANN(stream);
    ANN(node);
    (void)node;
    if (vertex_buffer_id == 0)
        return false;

    return _emit_shader(
               stream, DRP2_ID_VERTEX_SHADER, "VERTEX", _fixture_vertex_input_wgsl(),
               _fixture_vertex_input_glsl(), cfg) &&
           _emit_shader(
               stream, DRP2_ID_FRAGMENT_SHADER, "FRAGMENT", _fixture_fragment_wgsl(),
               _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_FIXTURE, true), cfg) &&
           _emit_fixture_triangle_pipeline(stream) &&
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
 * Emit DRP2 commands for a clear-only render pass in fixture mode.
 *
 * @param stream the DRP2 command stream
 * @param cfg the emission config
 * @return whether the commands were emitted
 */
static bool
_emit_clear_only(DvzDrp2CommandStream* stream, const DvzFramePlanEmitConfig* cfg)
{
    ANN(stream);

    float cr = cfg ? cfg->clear_color[0] : 0.0f;
    float cg = cfg ? cfg->clear_color[1] : 0.0f;
    float cb = cfg ? cfg->clear_color[2] : 0.0f;
    float ca = cfg ? cfg->clear_color[3] : 1.0f;

    return dvz_drp2_stream_create_texture_2d(stream, DRP2_ID_COLOR_TARGET, 4, 4) &&
           dvz_drp2_stream_begin_command_encoder(stream, DRP2_ID_ENCODER) &&
           dvz_drp2_stream_begin_render_pass_clear(
               stream, DRP2_ID_RENDER_PASS, DRP2_ID_ENCODER, DRP2_ID_COLOR_TARGET, cr, cg, cb,
               ca) &&
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
        stream, DRP2_ID_ENCODER, DRP2_ID_COLOR_TARGET, DRP2_ID_READBACK_BUFFER,
        copy->u.copy.dst_offset, copy->u.copy.extent[0], copy->u.copy.extent[1],
        (uint32_t)copy->u.copy.bytes_per_row, copy->u.copy.rows_per_image);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return the default FramePlan-to-DRP2 emission configuration.
 *
 * @return the default emission configuration
 */
DvzFramePlanEmitConfig dvz_frame_plan_emit_config(void)
{
    DvzFramePlanEmitConfig cfg = {DVZ_STRUCT_INIT_FIELDS(DvzFramePlanEmitConfig)};
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_WGSL;
    cfg.color_pipeline = DVZ_COLOR_PIPELINE_LINEAR_SRGB;
    cfg.external_color_target = false;
    cfg.color_target_id = DRP2_ID_COLOR_TARGET;
    cfg.color_target_format = 0;
    cfg.target_width = 4;
    cfg.target_height = 4;
    cfg.device_scale_x = 1.0f;
    cfg.device_scale_y = 1.0f;
    cfg.render_scale = 1.0f;
    cfg.user_scale = 1.0f;
    cfg.fullscreen_triangle = false;
    /* Default clear: opaque black. */
    cfg.clear_color[0] = 0.0f;
    cfg.clear_color[1] = 0.0f;
    cfg.clear_color[2] = 0.0f;
    cfg.clear_color[3] = 1.0f;
    return cfg;
}



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
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    return dvz_frame_plan_emit_drp2_ex(plan, caps, report, &cfg);
}



/**
 * Emit a DRP2 command stream from a FramePlan with explicit fixture options.
 *
 * @param plan the FramePlan
 * @param caps the capability snapshot
 * @param report the diagnostic report
 * @param cfg the emission configuration
 * @return an owned DRP2 command stream, or NULL on failure
 */
DvzDrp2CommandStream* dvz_frame_plan_emit_drp2_ex(
    const DvzFramePlan* plan, const DvzCapabilitySnapshot* caps, DvzDiagnosticReport* report,
    const DvzFramePlanEmitConfig* cfg)
{
    ANN(plan);
    if (!_frame_plan_emit_config_validate(cfg, report))
        return NULL;

    const DvzFramePlanNode* upload = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_UPLOAD);
    const DvzFramePlanNode* compute = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_COMPUTE);
    const DvzFramePlanNode* render = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_RENDER);
    const DvzFramePlanNode* clear = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_CLEAR);
    const DvzFramePlanNode* readback = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_READBACK);
    const DvzFramePlanNode* readback_copy = NULL;
    bool buffer_to_texture = false;
    for (uint32_t i = 0; i < plan->count; i++)
    {
        if (plan->nodes[i].type != DVZ_FRAME_PLAN_NODE_COPY)
            continue;
        if (plan->nodes[i].u.copy.direction == DVZ_FRAME_PLAN_COPY_BUFFER_TO_TEXTURE)
            buffer_to_texture = true;
        else if (
            readback_copy == NULL &&
            plan->nodes[i].u.copy.direction == DVZ_FRAME_PLAN_COPY_TEXTURE_TO_BUFFER)
            readback_copy = &plan->nodes[i];
    }
    bool clear_only = upload == NULL && compute == NULL && clear != NULL;
    if ((!clear_only && upload == NULL) || (clear_only ? clear == NULL : render == NULL))
    {
        _diagnostic(report, "fixture converter requires upload+render");
        return NULL;
    }
    if (readback != NULL && readback_copy == NULL)
    {
        _diagnostic(report, "fixture converter requires copy before readback");
        return NULL;
    }
    if (caps != NULL && !_validate_capabilities(plan, caps, cfg, report))
        return NULL;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    ConverterState state = {0};
    _state_init(&state);
    bool texture_render = !clear_only && _render_uses_texture(render);
    bool compute_render = compute != NULL;

    bool ok = dvz_drp2_stream_hello_renderer(stream, "scene-fixture") &&
              dvz_drp2_stream_renderer_hello_reply(stream, "datoviz-drp2-fixture");
    if (compute_render)
    {
        ok = ok && _emit_compute_buffers(&state, stream, upload, compute);
    }
    for (uint32_t i = 0; ok && !compute_render && i < plan->count; i++)
    {
        if (plan->nodes[i].type == DVZ_FRAME_PLAN_NODE_UPLOAD)
        {
            ok = texture_render && !buffer_to_texture
                     ? _emit_texture_upload(&state, stream, &plan->nodes[i])
                                : _emit_upload(&state, stream, &plan->nodes[i]);
        }
    }
    for (uint32_t i = 0; ok && i < plan->count; i++)
    {
        if (plan->nodes[i].type == DVZ_FRAME_PLAN_NODE_COPY &&
            plan->nodes[i].u.copy.direction == DVZ_FRAME_PLAN_COPY_BUFFER_TO_TEXTURE)
            ok = _emit_buffer_to_texture(&state, stream, &plan->nodes[i]);
    }
    ok = ok && (readback_copy == NULL || _emit_readback_buffer(stream, readback_copy)) &&
         (clear_only ? _emit_clear_only(stream, cfg)
                     : (compute_render
                            ? _emit_compute_assisted_render(stream, compute, render, &state, cfg)
                            : (texture_render
                                   ? _emit_texture_render(stream, render, state.first_texture_id, cfg)
                                   : _emit_render(
                                         stream, render, state.first_vertex_buffer_id, cfg)))) &&
         (readback_copy == NULL || readback == NULL ||
          _emit_readback(stream, readback_copy, readback)) &&
         dvz_drp2_stream_finish_command_encoder(stream, DRP2_ID_ENCODER, DRP2_ID_COMMAND_BUFFER) &&
         (readback != NULL
              ? dvz_drp2_stream_queue_submit_readback(
                    stream, DRP2_ID_COMMAND_BUFFER, DRP2_ID_SUBMISSION, DRP2_ID_READBACK_BUFFER,
                    0, readback_copy->u.copy.byte_size)
              : dvz_drp2_stream_queue_submit(
                    stream, DRP2_ID_COMMAND_BUFFER, DRP2_ID_SUBMISSION));
    if (!ok)
    {
        _diagnostic(report, "failed to emit DRP2 fixture stream");
        _state_destroy(&state);
        dvz_drp2_stream_destroy(stream);
        return NULL;
    }
    _state_destroy(&state);
    return stream;
}
