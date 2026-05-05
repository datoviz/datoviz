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

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <vulkan/vulkan_core.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_frame_plan.h"
#include "datoviz/drp2.h"
#include "datoviz/drp2/stream.h"
#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DRP2_ID_COLOR_TARGET 1
#define DRP2_ID_ENCODER 2
#define DRP2_ID_RENDER_PASS 3
#define DRP2_ID_COMMAND_BUFFER 4
#define DRP2_ID_SUBMISSION 5
#define DRP2_ID_COMPUTE_PASS 6
#define DRP2_ID_PIPELINE 10
#define DRP2_ID_COMPUTE_PIPELINE 30
#define DRP2_ID_RESOURCE_BASE 20
#define DRP2_ID_READBACK_BUFFER 12
#define DRP2_ID_BIND_GROUP_LAYOUT 100
#define DRP2_ID_BIND_GROUP 13
#define DRP2_ID_SAMPLER 200
#define DRP2_ID_VERTEX_SHADER 9000
#define DRP2_ID_FRAGMENT_SHADER 9001
#define DRP2_ID_COMPUTE_SHADER 9002
#define DRP2_MAX_FIXTURE_RESOURCES 64
#define DRP2_RUNTIME_TRANSIENT_ID_BASE 10000
#define DRP2_EMITTER_OBJECT_ID_BASE    5000

#define DRP2_VERTEX_WGSL                                                                        \
    "@vertex fn main() -> @builtin(position) vec4f { return vec4f(0.0, 0.0, 0.0, 1.0); }"
#define DRP2_FULLSCREEN_VERTEX_WGSL                                                             \
    "@vertex fn main(@builtin(vertex_index) idx: u32) -> @builtin(position) vec4f { var pos = " \
    "array<vec2f, 3>(vec2f(-1.0, -1.0), vec2f(3.0, -1.0), vec2f(-1.0, 3.0)); return "          \
    "vec4f(pos[idx], 0.0, 1.0); }"
#define DRP2_FRAGMENT_WGSL                                                                      \
    "@fragment fn main() -> @location(0) vec4f { return vec4f(1.0, 1.0, 1.0, 1.0); }"
#define DRP2_TEXTURE_VERTEX_WGSL                                                                \
    "@vertex fn main(@builtin(vertex_index) idx: u32) -> @builtin(position) vec4f { var pos = " \
    "array<vec2f, 3>(vec2f(-1.0, -1.0), vec2f(3.0, -1.0), vec2f(-1.0, 3.0)); return "          \
    "vec4f(pos[idx], 0.0, 1.0); }"
#define DRP2_TEXTURE_FRAGMENT_WGSL                                                              \
    "@group(0) @binding(0) var source: texture_2d<f32>; @group(0) @binding(1) var samp: "      \
    "sampler; @fragment fn main(@builtin(position) pos: vec4f) -> @location(0) vec4f { "       \
    "return textureSample(source, samp, vec2f(0.5, 0.5)); }"
#define DRP2_COMPUTE_WGSL                                                                       \
    "@group(0) @binding(0) var<storage, read> input: array<f32>; @group(0) @binding(1) "        \
    "var<storage, read_write> output: array<f32>; @compute @workgroup_size(9) fn main("        \
    "@builtin(global_invocation_id) id: vec3u) { output[id.x] = input[id.x]; }"

#define DRP2_VERTEX_GLSL                                                                        \
    "#version 450\nvoid main(){gl_Position=vec4(0.0,0.0,0.0,1.0);}"
#define DRP2_FULLSCREEN_VERTEX_GLSL                                                             \
    "#version 450\nvec2 p[3]=vec2[](vec2(-1,-1),vec2(3,-1),vec2(-1,3));"                       \
    "void main(){gl_Position=vec4(p[gl_VertexIndex],0,1);}"
#define DRP2_FRAGMENT_GLSL                                                                      \
    "#version 450\nlayout(location=0)out vec4 color;void main(){color=vec4(1.0);}"
#define DRP2_TEXTURE_VERTEX_GLSL                                                                \
    "#version 450\nvec2 p[3]=vec2[](vec2(-1,-1),vec2(3,-1),vec2(-1,3));"                       \
    "void main(){gl_Position=vec4(p[gl_VertexIndex],0,1);}"
#define DRP2_TEXTURE_FRAGMENT_GLSL                                                              \
    "#version 450\nlayout(set=0,binding=0)uniform sampler2D t;"                                \
    "layout(location=0)out vec4 c;void main(){c=texture(t,vec2(.5));}"
#define DRP2_COMPUTE_GLSL                                                                       \
    "#version 450\nlayout(local_size_x=1)in;void main(){}"

/* Point visual: separate vertex buffers for position (vec3), color (u8 RGBA→vec4), size (float). */
#define DRP2_POINT_VERTEX_GLSL                                                                  \
    "#version 450\n"                                                                            \
    "layout(location=0)in vec3 inPos;\n"                                                        \
    "layout(location=1)in vec4 inColor;\n"                                                      \
    "layout(location=2)in float inSize;\n"                                                      \
    "layout(location=0)out vec4 fragColor;\n"                                                   \
    "void main(){"                                                                               \
    "gl_Position=vec4(inPos.xy,0.0,1.0);"                                                       \
    "gl_PointSize=inSize;"                                                                       \
    "fragColor=inColor;}\n"
#define DRP2_POINT_FRAGMENT_GLSL                                                                \
    "#version 450\n"                                                                            \
    "layout(location=0)in vec4 fragColor;\n"                                                    \
    "layout(location=0)out vec4 outColor;\n"                                                    \
    "void main(){outColor=fragColor;}\n"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct ResourceId ResourceId;
typedef struct ConverterState ConverterState;

struct ResourceId
{
    char key[DVZ_SCENE_LABEL_SIZE];
    uint64_t id;
    char data_tag[DVZ_SCENE_LABEL_SIZE]; /* attribute name, e.g. "position", "color", "size" */
    uint64_t byte_size;                  /* total bytes uploaded to this buffer               */
};

struct ConverterState
{
    uint32_t count;
    uint64_t next_id;
    uint64_t first_vertex_buffer_id;
    uint64_t first_texture_id;
    uint64_t first_compute_input_id;
    uint64_t first_compute_output_id;
    uint64_t compute_buffer_size;
    ResourceId resources[DRP2_MAX_FIXTURE_RESOURCES];
};


struct DvzFramePlanEmitter
{
    ConverterState resources;
    ConverterState objects;
    uint64_t next_transient_id;
    bool handshake_sent;
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
 * Return the next runtime-mode transient id.
 *
 * @param emitter the persistent emitter
 * @return a unique transient DRP2 id
 */
static uint64_t _emitter_next_transient_id(DvzFramePlanEmitter* emitter)
{
    ANN(emitter);
    return emitter->next_transient_id++;
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



static const char* _shader_format_tag(const DvzFramePlanEmitConfig* cfg)
{
    if (cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
        return "g";
    return "w";
}



/* Look up data_tag and byte_size for a buffer by its DRP2 id. */
static const char* _resource_data_tag(const ConverterState* state, uint64_t id)
{
    for (uint32_t i = 0; i < state->count; i++)
        if (state->resources[i].id == id)
            return state->resources[i].data_tag;
    return "";
}

static uint64_t _resource_byte_size(const ConverterState* state, uint64_t id)
{
    for (uint32_t i = 0; i < state->count; i++)
        if (state->resources[i].id == id)
            return state->resources[i].byte_size;
    return 0;
}

/*
 * Return true when vertex_buffer_ids[0..n-1] carry data_tags "position", "color", "size"
 * (in any order), which identifies a DvzPoint visual.
 */
static bool _is_point_visual(
    const ConverterState* state, const uint64_t* ids, uint32_t n)
{
    if (n < 3)
        return false;
    bool has_pos = false, has_col = false, has_sz = false;
    for (uint32_t i = 0; i < n; i++)
    {
        const char* tag = _resource_data_tag(state, ids[i]);
        if (strcmp(tag, "position") == 0) has_pos = true;
        if (strcmp(tag, "color") == 0)    has_col = true;
        if (strcmp(tag, "size") == 0)     has_sz  = true;
    }
    return has_pos && has_col && has_sz;
}



static uint64_t _obj_id(DvzFramePlanEmitter* emitter, const char* key, bool* is_new)
{
    ANN(emitter);
    ANN(key);
    ANN(is_new);
    uint32_t n = emitter->objects.count;
    uint64_t id = _resource_id(&emitter->objects, key);
    *is_new = (id != 0) && (emitter->objects.count > n);
    return id;
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
    uint64_t extra = remainder == 0 ? 0 : 4;
    if (groups > (UINT64_MAX - extra) / 4)
        return false;

    uint64_t count = groups * 4 + extra;
    if (count == UINT64_MAX || count + 1 > out_size)
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
 * Return whether a render node targets the fixture texture-sampling path.
 *
 * @param node the render node
 * @return true when one visual id names an image or texture visual
 */
static bool _render_uses_texture(const DvzFramePlanNode* node)
{
    ANN(node);
    for (uint32_t i = 0; i < node->u.render.visual_count; i++)
    {
        const char* visual = node->u.render.visuals[i];
        if (strstr(visual, "image") != NULL || strstr(visual, "texture") != NULL)
            return true;
    }
    return false;
}


/**
 * Add a converter diagnostic message.
 *
 * @param report the optional diagnostic report
 * @param message the diagnostic message
 */
static void _diagnostic(DvzDiagnosticReport* report, const char* message)
{
    ANN(message);
    if (report != NULL)
        (void)dvz_diagnostic_report_add(report, message);
}



/**
 * Return whether the requested shader format is supported by the capability snapshot.
 *
 * @param caps the capability snapshot
 * @param cfg the emission config
 * @return whether the shader format is supported
 */
static bool
_validate_shader_format(const DvzCapabilitySnapshot* caps, const DvzFramePlanEmitConfig* cfg)
{
    ANN(caps);
    DvzSceneShaderFormat format =
        cfg != NULL ? cfg->shader_format : DVZ_SCENE_SHADER_FORMAT_WGSL;
    if (format == DVZ_SCENE_SHADER_FORMAT_GLSL)
        return caps->shader_format_glsl;
    return caps->shader_format_wgsl;
}



/**
 * Validate FramePlan conversion against a capability snapshot.
 *
 * @param plan the FramePlan
 * @param caps the capability snapshot
 * @param cfg the emission config
 * @param report the optional diagnostic report
 * @return whether the plan can be emitted
 */
static bool _validate_capabilities(
    const DvzFramePlan* plan, const DvzCapabilitySnapshot* caps, const DvzFramePlanEmitConfig* cfg,
    DvzDiagnosticReport* report)
{
    ANN(plan);
    ANN(caps);

    if (!_validate_shader_format(caps, cfg))
    {
        _diagnostic(report, "unsupported shader format");
        return false;
    }
    if (caps->max_texture_dimension_2d < 4)
    {
        _diagnostic(report, "max_texture_dimension_2d is too small for fixture render target");
        return false;
    }

    uint32_t upload_count = 0;
    bool has_compute = false;
    bool has_texture_render = false;
    uint64_t max_readback_size = 0;
    const DvzFramePlanNode* render = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_RENDER);
    if (render != NULL)
        has_texture_render = _render_uses_texture(render);

    for (uint32_t i = 0; i < plan->count; i++)
    {
        const DvzFramePlanNode* node = &plan->nodes[i];
        switch (node->type)
        {
        case DVZ_FRAME_PLAN_NODE_UPLOAD:
            upload_count++;
            if (node->u.upload.byte_offset + node->u.upload.byte_size > caps->max_buffer_size)
            {
                _diagnostic(report, "upload buffer exceeds max_buffer_size");
                return false;
            }
            break;
        case DVZ_FRAME_PLAN_NODE_COMPUTE:
            has_compute = true;
            break;
        case DVZ_FRAME_PLAN_NODE_COPY:
            if (node->u.copy.byte_size > max_readback_size)
                max_readback_size = node->u.copy.byte_size;
            break;
        default:
            break;
        }
    }

    if (!has_texture_render && upload_count > caps->max_vertex_buffers)
    {
        _diagnostic(report, "max_vertex_buffers is too small for fixture render pipeline");
        return false;
    }
    if ((has_texture_render || has_compute) && caps->max_bind_groups < 1)
    {
        _diagnostic(report, "max_bind_groups is too small for fixture bind groups");
        return false;
    }
    if (has_texture_render && caps->max_texture_dimension_2d < 2)
    {
        _diagnostic(report, "max_texture_dimension_2d is too small for fixture texture upload");
        return false;
    }
    if (max_readback_size > caps->max_buffer_size)
    {
        _diagnostic(report, "readback buffer exceeds max_buffer_size");
        return false;
    }
    return true;
}



/**
 * Return the DRP2 shader-format token for a scene fixture emission config.
 *
 * @param cfg the emission config
 * @return the shader-format token
 */
static const char* _shader_format_token(const DvzFramePlanEmitConfig* cfg)
{
    if (cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
        return "glsl";
    return "wgsl";
}



/**
 * Select shader source for a scene fixture emission config.
 *
 * @param cfg the emission config
 * @param wgsl the WGSL source
 * @param glsl the GLSL source
 * @return the selected shader source
 */
static const char*
_shader_source(const DvzFramePlanEmitConfig* cfg, const char* wgsl, const char* glsl)
{
    if (cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
        return glsl;
    return wgsl;
}


/**
 * Return the configured DRP2 color target id.
 *
 * @param cfg the emission config
 * @return the color target id
 */
static uint64_t _color_target_id(const DvzFramePlanEmitConfig* cfg)
{
    if (cfg != NULL && cfg->color_target_id != 0)
        return cfg->color_target_id;
    return DRP2_ID_COLOR_TARGET;
}


/**
 * Return the vertex shader source used for simple render emissions.
 *
 * @param cfg the emission config
 * @param wgsl output WGSL shader source
 * @param glsl output GLSL shader source
 */
static void _render_vertex_shader_source(
    const DvzFramePlanEmitConfig* cfg, const char** wgsl, const char** glsl)
{
    ANN(wgsl);
    ANN(glsl);
    if (cfg != NULL && cfg->fullscreen_triangle)
    {
        *wgsl = DRP2_FULLSCREEN_VERTEX_WGSL;
        *glsl = DRP2_FULLSCREEN_VERTEX_GLSL;
    }
    else
    {
        *wgsl = DRP2_VERTEX_WGSL;
        *glsl = DRP2_VERTEX_GLSL;
    }
}



/**
 * Emit a shader module command with the configured source format.
 *
 * @param stream the DRP2 command stream
 * @param id the shader id
 * @param stage the shader stage
 * @param wgsl the WGSL source
 * @param glsl the GLSL source
 * @param cfg the emission config
 * @return whether the command was emitted
 */
static bool _emit_shader(
    DvzDrp2CommandStream* stream, uint64_t id, const char* stage, const char* wgsl,
    const char* glsl, const DvzFramePlanEmitConfig* cfg)
{
    ANN(stream);
    ANN(stage);
    return dvz_drp2_stream_create_shader_module_format(
        stream, id, stage, _shader_format_token(cfg), _shader_source(cfg, wgsl, glsl));
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

    char data[128] = {0};
    if (!_zero_base64(node->u.upload.byte_size, data, sizeof(data)))
        return false;

    uint32_t usage = DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING | DVZ_DRP2_TEXTURE_USAGE_COPY_DST;
    return dvz_drp2_stream_create_texture_2d_usage(stream, id, 2, 2, usage) &&
           dvz_drp2_stream_write_texture_2d(stream, id, 0, 2, 2, 8, 2, data);
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

    char data[128] = {0};
    if (!_zero_base64(upload->u.upload.byte_size, data, sizeof(data)))
        return false;

    return dvz_drp2_stream_create_buffer(
               stream, input_id, upload->u.upload.byte_size,
               DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_STORAGE) &&
           dvz_drp2_stream_write_buffer(
               stream, input_id, upload->u.upload.byte_offset, upload->u.upload.byte_size, data) &&
           dvz_drp2_stream_create_buffer(
               stream, output_id, upload->u.upload.byte_size,
               DVZ_DRP2_BUFFER_USAGE_STORAGE | DVZ_DRP2_BUFFER_USAGE_VERTEX);
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
 * Emit a compute pass followed by a render pass in one encoder.
 *
 * @param stream the DRP2 command stream
 * @param compute the compute node
 * @param render the render node
 * @param state the converter state
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
               stream, DRP2_ID_COMPUTE_SHADER, "COMPUTE", DRP2_COMPUTE_WGSL, DRP2_COMPUTE_GLSL,
               cfg) &&
           dvz_drp2_stream_create_compute_pipeline_with_bind_group_layout(
               stream, DRP2_ID_COMPUTE_PIPELINE, DRP2_ID_COMPUTE_SHADER,
               DRP2_ID_BIND_GROUP_LAYOUT) &&
           dvz_drp2_stream_create_storage_bind_group(
               stream, DRP2_ID_BIND_GROUP, DRP2_ID_BIND_GROUP_LAYOUT,
               state->first_compute_input_id, state->first_compute_output_id,
               state->compute_buffer_size) &&
           _emit_shader(
               stream, DRP2_ID_VERTEX_SHADER, "VERTEX", DRP2_VERTEX_WGSL, DRP2_VERTEX_GLSL,
               cfg) &&
           _emit_shader(
               stream, DRP2_ID_FRAGMENT_SHADER, "FRAGMENT", DRP2_FRAGMENT_WGSL,
               DRP2_FRAGMENT_GLSL, cfg) &&
           dvz_drp2_stream_create_render_pipeline(
               stream, DRP2_ID_PIPELINE, DRP2_ID_VERTEX_SHADER, DRP2_ID_FRAGMENT_SHADER, 1) &&
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

    return dvz_drp2_stream_create_sampler(stream, DRP2_ID_SAMPLER) &&
           dvz_drp2_stream_create_texture_sampler_bind_group_layout(
               stream, DRP2_ID_BIND_GROUP_LAYOUT) &&
           _emit_shader(
               stream, DRP2_ID_VERTEX_SHADER, "VERTEX", DRP2_TEXTURE_VERTEX_WGSL,
               DRP2_TEXTURE_VERTEX_GLSL, cfg) &&
           _emit_shader(
               stream, DRP2_ID_FRAGMENT_SHADER, "FRAGMENT", DRP2_TEXTURE_FRAGMENT_WGSL,
               DRP2_TEXTURE_FRAGMENT_GLSL, cfg) &&
           dvz_drp2_stream_create_render_pipeline_with_bind_group_layout(
               stream, DRP2_ID_PIPELINE, DRP2_ID_VERTEX_SHADER, DRP2_ID_FRAGMENT_SHADER, 0,
               DRP2_ID_BIND_GROUP_LAYOUT) &&
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
               stream, DRP2_ID_VERTEX_SHADER, "VERTEX", DRP2_VERTEX_WGSL, DRP2_VERTEX_GLSL,
               cfg) &&
           _emit_shader(
               stream, DRP2_ID_FRAGMENT_SHADER, "FRAGMENT", DRP2_FRAGMENT_WGSL,
               DRP2_FRAGMENT_GLSL, cfg) &&
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


/**
 * Emit runtime-mode upload commands.
 *
 * @param emitter the persistent emitter
 * @param stream the DRP2 command stream
 * @param node the upload node
 * @return whether the commands were emitted
 */
static bool _emitter_emit_upload(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlanNode* node,
    uint64_t* out_id)
{
    ANN(emitter);
    ANN(stream);
    ANN(node);
    ANN(out_id);

    bool exists = false;
    for (uint32_t i = 0; i < emitter->resources.count; i++)
    {
        if (strcmp(emitter->resources.resources[i].key, node->u.upload.resource_id) == 0)
        {
            exists = true;
            break;
        }
    }

    uint64_t id = _resource_id(&emitter->resources, node->u.upload.resource_id);
    if (id == 0)
        return false;

    /* Store data_tag and byte_size for vertex layout inference in _emitter_emit_render. */
    for (uint32_t i = 0; i < emitter->resources.count; i++)
    {
        if (emitter->resources.resources[i].id == id)
        {
            dvz_strlcpy(emitter->resources.resources[i].data_tag,
                        node->u.upload.data_tag,
                        sizeof(emitter->resources.resources[i].data_tag));
            emitter->resources.resources[i].byte_size = node->u.upload.byte_size;
            break;
        }
    }

    uint64_t buffer_size = node->u.upload.byte_offset + node->u.upload.byte_size;
    uint32_t usage = DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_VERTEX;
    if (!exists && !dvz_drp2_stream_create_buffer(stream, id, buffer_size, usage))
        return false;
    if (emitter->resources.first_vertex_buffer_id == 0)
        emitter->resources.first_vertex_buffer_id = id;
    *out_id = id;

    if (node->u.upload.data != NULL)
    {
        /* Real vertex data provided — encode directly into the stream. */
        return dvz_drp2_stream_write_buffer_bytes(
            stream, id, node->u.upload.byte_offset, node->u.upload.byte_size,
            node->u.upload.data);
    }
    else
    {
        /* No data: write zeros (placeholder / test path). */
        char zero_data[128] = {0};
        if (!_zero_base64(node->u.upload.byte_size, zero_data, sizeof(zero_data)))
            return false;
        return dvz_drp2_stream_write_buffer(
            stream, id, node->u.upload.byte_offset, node->u.upload.byte_size, zero_data);
    }
}


/**
 * Emit runtime-mode texture upload commands.
 *
 * @param emitter the persistent emitter
 * @param stream the DRP2 command stream
 * @param node the upload node
 * @param out_id the emitted texture id
 * @return whether the commands were emitted
 */
static bool _emitter_emit_texture_upload(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlanNode* node,
    uint64_t* out_id)
{
    ANN(emitter);
    ANN(stream);
    ANN(node);
    ANN(out_id);

    bool exists = false;
    for (uint32_t i = 0; i < emitter->resources.count; i++)
    {
        if (strcmp(emitter->resources.resources[i].key, node->u.upload.resource_id) == 0)
        {
            exists = true;
            break;
        }
    }

    uint64_t id = _resource_id(&emitter->resources, node->u.upload.resource_id);
    if (id == 0)
        return false;

    char data[128] = {0};
    if (!_zero_base64(node->u.upload.byte_size, data, sizeof(data)))
        return false;

    uint32_t usage = DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING | DVZ_DRP2_TEXTURE_USAGE_COPY_DST;
    if (!exists && !dvz_drp2_stream_create_texture_2d_usage(stream, id, 2, 2, usage))
        return false;
    if (emitter->resources.first_texture_id == 0)
        emitter->resources.first_texture_id = id;
    *out_id = id;
    return dvz_drp2_stream_write_texture_2d(stream, id, 0, 2, 2, 8, 2, data);
}



/**
 * Emit runtime-mode compute input/output buffer commands.
 *
 * @param emitter the persistent emitter
 * @param stream the DRP2 command stream
 * @param upload the upload node backing the compute input
 * @param compute the compute node naming the output resource
 * @return whether the commands were emitted
 */
static bool _emitter_emit_compute_buffers(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlanNode* upload,
    const DvzFramePlanNode* compute)
{
    ANN(emitter);
    ANN(stream);
    ANN(upload);
    ANN(compute);
    if (compute->u.compute.write_count == 0)
        return false;

    bool input_exists = false;
    bool output_exists = false;
    for (uint32_t i = 0; i < emitter->resources.count; i++)
    {
        if (strcmp(emitter->resources.resources[i].key, upload->u.upload.resource_id) == 0)
            input_exists = true;
        if (strcmp(emitter->resources.resources[i].key, compute->u.compute.writes[0]) == 0)
            output_exists = true;
    }

    uint64_t input_id = _resource_id(&emitter->resources, upload->u.upload.resource_id);
    uint64_t output_id = _resource_id(&emitter->resources, compute->u.compute.writes[0]);
    if (input_id == 0 || output_id == 0)
        return false;

    emitter->resources.first_compute_input_id = input_id;
    emitter->resources.first_compute_output_id = output_id;
    emitter->resources.first_vertex_buffer_id = output_id;
    emitter->resources.compute_buffer_size = upload->u.upload.byte_size;

    char data[128] = {0};
    if (!_zero_base64(upload->u.upload.byte_size, data, sizeof(data)))
        return false;

    bool ok = true;
    if (!input_exists)
    {
        ok = ok && dvz_drp2_stream_create_buffer(
                       stream, input_id, upload->u.upload.byte_size,
                       DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_STORAGE);
    }
    ok = ok && dvz_drp2_stream_write_buffer(
                   stream, input_id, upload->u.upload.byte_offset, upload->u.upload.byte_size,
                   data);
    if (!output_exists)
    {
        ok = ok && dvz_drp2_stream_create_buffer(
                       stream, output_id, upload->u.upload.byte_size,
                       DVZ_DRP2_BUFFER_USAGE_STORAGE | DVZ_DRP2_BUFFER_USAGE_VERTEX);
    }
    return ok;
}



/**
 * Emit runtime-mode static render commands.
 *
 * @param emitter the persistent emitter
 * @param stream the DRP2 command stream
 * @param vertex_buffer_ids the vertex buffer ids
 * @param vertex_buffer_count the vertex buffer count
 * @param readback the optional readback copy node
 * @param cfg the emission config
 * @return whether the commands were emitted
 */
static bool _emitter_emit_render(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const uint64_t* vertex_buffer_ids,
    uint32_t vertex_buffer_count, const DvzFramePlanNode* readback,
    const DvzFramePlanEmitConfig* cfg)
{
    ANN(emitter);
    ANN(stream);
    ANN(vertex_buffer_ids);
    if (vertex_buffer_count == 0)
        return false;

    bool ok = true;
    bool is_new = false;
    const char* fmt = _shader_format_tag(cfg);

    /* Detect DvzPoint visual (position + color + size attributes). */
    bool is_point = _is_point_visual(&emitter->resources, vertex_buffer_ids, vertex_buffer_count);

    const char* vs_glsl = NULL;
    const char* fs_glsl = NULL;
    uint32_t topology = 0;
    uint32_t vertex_count = 3; /* default for stub / non-point path */

    char vs_key[32];
    char fs_key[16];
    char pipe_key[48];

    if (is_point && cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
    {
        /* Point visual: use type-specific shaders and POINT_LIST topology. */
        dvz_snprintf(vs_key, sizeof(vs_key), "_vs_point%s", fmt);
        dvz_snprintf(fs_key, sizeof(fs_key), "_fs_point%s", fmt);
        vs_glsl = DRP2_POINT_VERTEX_GLSL;
        fs_glsl = DRP2_POINT_FRAGMENT_GLSL;
        topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;

        /* Infer vertex count from position buffer byte_size / sizeof(vec3). */
        for (uint32_t i = 0; i < vertex_buffer_count; i++)
        {
            if (strcmp(_resource_data_tag(&emitter->resources, vertex_buffer_ids[i]),
                       "position") == 0)
            {
                uint64_t sz = _resource_byte_size(&emitter->resources, vertex_buffer_ids[i]);
                if (sz > 0)
                    vertex_count = (uint32_t)(sz / (3 * sizeof(float)));
                break;
            }
        }
    }
    else if (cfg != NULL && cfg->fullscreen_triangle)
    {
        dvz_snprintf(vs_key, sizeof(vs_key), "_vs_full%s", fmt);
        dvz_snprintf(fs_key, sizeof(fs_key), "_fs%s", fmt);
    }
    else
    {
        dvz_snprintf(vs_key, sizeof(vs_key), "_vs%s", fmt);
        dvz_snprintf(fs_key, sizeof(fs_key), "_fs%s", fmt);
    }

    uint64_t vs_id = _obj_id(emitter, vs_key, &is_new);
    if (vs_id == 0)
        return false;
    if (is_new)
    {
        if (vs_glsl != NULL)
        {
            ok = ok && _emit_shader(stream, vs_id, "VERTEX", NULL, vs_glsl, cfg);
        }
        else
        {
            const char* vertex_wgsl = NULL;
            const char* vertex_glsl_src = NULL;
            _render_vertex_shader_source(cfg, &vertex_wgsl, &vertex_glsl_src);
            ok = ok && _emit_shader(stream, vs_id, "VERTEX", vertex_wgsl, vertex_glsl_src, cfg);
        }
    }

    uint64_t fs_id = _obj_id(emitter, fs_key, &is_new);
    if (fs_id == 0)
        return false;
    if (ok && is_new)
    {
        if (fs_glsl != NULL)
            ok = ok && _emit_shader(stream, fs_id, "FRAGMENT", NULL, fs_glsl, cfg);
        else
            ok = ok && _emit_shader(
                           stream, fs_id, "FRAGMENT", DRP2_FRAGMENT_WGSL, DRP2_FRAGMENT_GLSL, cfg);
    }

    if (is_point && cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
        dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe_point%s", fmt);
    else
        dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe%u%s", vertex_buffer_count, fmt);

    uint64_t pipe_id = _obj_id(emitter, pipe_key, &is_new);
    if (pipe_id == 0)
        return false;
    if (ok && is_new)
    {
        if (is_point && cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
        {
            /* Explicit vertex layout: binding0=position(vec3), binding1=color(u8vec4), binding2=size(float) */
            uint32_t strides[3]   = {3*sizeof(float), 4*sizeof(uint8_t), sizeof(float)};
            uint32_t bindings[3]  = {0, 1, 2};
            uint32_t locations[3] = {0, 1, 2};
            uint32_t formats[3]   = {VK_FORMAT_R32G32B32_SFLOAT,
                                     VK_FORMAT_R8G8B8A8_UNORM,
                                     VK_FORMAT_R32_SFLOAT};
            uint32_t offsets[3]   = {0, 0, 0};
            ok = ok && dvz_drp2_stream_create_render_pipeline_ex(
                           stream, pipe_id, vs_id, fs_id, vertex_buffer_count,
                           topology,
                           3, strides,
                           3, bindings, locations, formats, offsets);
        }
        else
        {
            ok = ok && dvz_drp2_stream_create_render_pipeline(
                           stream, pipe_id, vs_id, fs_id, vertex_buffer_count);
        }
    }

    uint64_t color_id = 0;
    if (cfg != NULL && cfg->external_color_target)
    {
        color_id = _color_target_id(cfg);
    }
    else
    {
        color_id = _obj_id(emitter, "_ct", &is_new);
        if (color_id == 0)
            return false;
        if (ok && is_new)
        {
            uint32_t usage =
                DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC;
            ok = ok && dvz_drp2_stream_create_texture_2d_usage(stream, color_id, 4, 4, usage);
        }
    }

    uint64_t rb_id = 0;
    if (readback != NULL)
    {
        rb_id = _obj_id(emitter, "_rb", &is_new);
        if (rb_id == 0)
            return false;
        if (ok && is_new)
        {
            uint32_t usage = DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ;
            ok = ok &&
                 dvz_drp2_stream_create_buffer(stream, rb_id, readback->u.copy.byte_size, usage);
        }
    }

    if (!ok)
        return false;

    uint64_t encoder_id = _emitter_next_transient_id(emitter);
    uint64_t render_pass_id = _emitter_next_transient_id(emitter);
    uint64_t command_buffer_id = _emitter_next_transient_id(emitter);
    uint64_t submission_id = _emitter_next_transient_id(emitter);

    ok = dvz_drp2_stream_begin_command_encoder(stream, encoder_id) &&
         dvz_drp2_stream_begin_render_pass(stream, render_pass_id, encoder_id, color_id) &&
         dvz_drp2_stream_set_pipeline(stream, render_pass_id, pipe_id);
    for (uint32_t i = 0; ok && i < vertex_buffer_count; i++)
        ok = dvz_drp2_stream_set_vertex_buffer(stream, render_pass_id, i, vertex_buffer_ids[i], 0);
    ok = ok && dvz_drp2_stream_draw(stream, render_pass_id, vertex_count, 1, 0, 0) &&
         dvz_drp2_stream_end_render_pass(stream, render_pass_id);
    if (ok && readback != NULL)
    {
        ok = ok && dvz_drp2_stream_copy_texture_to_buffer(
                       stream, encoder_id, color_id, rb_id, 0, 1, 1, 4, 1);
    }
    ok = ok && dvz_drp2_stream_finish_command_encoder(stream, encoder_id, command_buffer_id);
    if (readback != NULL)
    {
        ok = ok && dvz_drp2_stream_queue_submit_readback(
                       stream, command_buffer_id, submission_id, rb_id, 0,
                       readback->u.copy.byte_size);
    }
    else
    {
        ok = ok && dvz_drp2_stream_queue_submit(stream, command_buffer_id, submission_id);
    }
    return ok;
}


/**
 * Emit runtime-mode texture render commands.
 *
 * @param emitter the persistent emitter
 * @param stream the DRP2 command stream
 * @param texture_id the sampled texture id
 * @param readback the optional readback copy node
 * @param cfg the emission config
 * @return whether the commands were emitted
 */
static bool _emitter_emit_texture_render(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, uint64_t texture_id,
    const DvzFramePlanNode* readback, const DvzFramePlanEmitConfig* cfg)
{
    ANN(emitter);
    ANN(stream);
    if (texture_id == 0)
        return false;

    bool ok = true;
    bool is_new = false;
    const char* fmt = _shader_format_tag(cfg);

    uint64_t sampler_id = _obj_id(emitter, "_sampler", &is_new);
    if (sampler_id == 0)
        return false;
    if (is_new)
        ok = ok && dvz_drp2_stream_create_sampler(stream, sampler_id);

    uint64_t bgl_id = _obj_id(emitter, "_bgl_tex", &is_new);
    if (bgl_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && dvz_drp2_stream_create_texture_sampler_bind_group_layout(stream, bgl_id);

    char vs_key[16];
    dvz_snprintf(vs_key, sizeof(vs_key), "_vs_tex%s", fmt);
    uint64_t vs_id = _obj_id(emitter, vs_key, &is_new);
    if (vs_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && _emit_shader(
                       stream, vs_id, "VERTEX", DRP2_TEXTURE_VERTEX_WGSL,
                       DRP2_TEXTURE_VERTEX_GLSL, cfg);

    char fs_key[16];
    dvz_snprintf(fs_key, sizeof(fs_key), "_fs_tex%s", fmt);
    uint64_t fs_id = _obj_id(emitter, fs_key, &is_new);
    if (fs_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && _emit_shader(
                       stream, fs_id, "FRAGMENT", DRP2_TEXTURE_FRAGMENT_WGSL,
                       DRP2_TEXTURE_FRAGMENT_GLSL, cfg);

    char pipe_key[32];
    dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe_tex%s_%" PRIu64, fmt, bgl_id);
    uint64_t pipe_id = _obj_id(emitter, pipe_key, &is_new);
    if (pipe_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && dvz_drp2_stream_create_render_pipeline_with_bind_group_layout(
                       stream, pipe_id, vs_id, fs_id, 0, bgl_id);

    char bg_key[32];
    dvz_snprintf(bg_key, sizeof(bg_key), "_bg_tex_%" PRIu64, texture_id);
    uint64_t bg_id = _obj_id(emitter, bg_key, &is_new);
    if (bg_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && dvz_drp2_stream_create_texture_sampler_bind_group(
                       stream, bg_id, bgl_id, texture_id, sampler_id);

    uint64_t color_id = 0;
    if (cfg != NULL && cfg->external_color_target)
    {
        color_id = _color_target_id(cfg);
    }
    else
    {
        color_id = _obj_id(emitter, "_ct", &is_new);
        if (color_id == 0)
            return false;
        if (ok && is_new)
        {
            uint32_t usage =
                DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC;
            ok = ok && dvz_drp2_stream_create_texture_2d_usage(stream, color_id, 4, 4, usage);
        }
    }

    uint64_t rb_id = 0;
    if (readback != NULL)
    {
        rb_id = _obj_id(emitter, "_rb", &is_new);
        if (rb_id == 0)
            return false;
        if (ok && is_new)
        {
            uint32_t usage = DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ;
            ok = ok &&
                 dvz_drp2_stream_create_buffer(stream, rb_id, readback->u.copy.byte_size, usage);
        }
    }

    if (!ok)
        return false;

    uint64_t encoder_id = _emitter_next_transient_id(emitter);
    uint64_t render_pass_id = _emitter_next_transient_id(emitter);
    uint64_t command_buffer_id = _emitter_next_transient_id(emitter);
    uint64_t submission_id = _emitter_next_transient_id(emitter);

    ok = dvz_drp2_stream_begin_command_encoder(stream, encoder_id) &&
         dvz_drp2_stream_begin_render_pass(stream, render_pass_id, encoder_id, color_id) &&
         dvz_drp2_stream_set_pipeline(stream, render_pass_id, pipe_id) &&
         dvz_drp2_stream_set_bind_group(stream, render_pass_id, 0, bg_id) &&
         dvz_drp2_stream_draw(stream, render_pass_id, 3, 1, 0, 0) &&
         dvz_drp2_stream_end_render_pass(stream, render_pass_id);
    if (ok && readback != NULL)
    {
        ok = ok && dvz_drp2_stream_copy_texture_to_buffer(
                       stream, encoder_id, color_id, rb_id, 0, 1, 1, 4, 1);
    }
    ok = ok && dvz_drp2_stream_finish_command_encoder(stream, encoder_id, command_buffer_id);
    if (readback != NULL)
    {
        ok = ok && dvz_drp2_stream_queue_submit_readback(
                       stream, command_buffer_id, submission_id, rb_id, 0,
                       readback->u.copy.byte_size);
    }
    else
    {
        ok = ok && dvz_drp2_stream_queue_submit(stream, command_buffer_id, submission_id);
    }
    return ok;
}


/**
 * Emit runtime-mode compute pass followed by render commands.
 *
 * @param emitter the persistent emitter
 * @param stream the DRP2 command stream
 * @param compute the compute node
 * @param readback the optional readback copy node
 * @param cfg the emission config
 * @return whether the commands were emitted
 */
static bool _emitter_emit_compute_assisted_render(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlanNode* compute,
    const DvzFramePlanNode* readback, const DvzFramePlanEmitConfig* cfg)
{
    ANN(emitter);
    ANN(stream);
    ANN(compute);
    if (emitter->resources.first_compute_input_id == 0 ||
        emitter->resources.first_compute_output_id == 0 ||
        emitter->resources.compute_buffer_size == 0)
        return false;

    bool ok = true;
    bool is_new = false;
    const char* fmt = _shader_format_tag(cfg);

    uint64_t bgl_stor_id = _obj_id(emitter, "_bgl_stor", &is_new);
    if (bgl_stor_id == 0)
        return false;
    if (is_new)
        ok = ok &&
             dvz_drp2_stream_create_storage_bind_group_layout(stream, bgl_stor_id);

    char cs_key[16];
    dvz_snprintf(cs_key, sizeof(cs_key), "_cs%s", fmt);
    uint64_t cs_id = _obj_id(emitter, cs_key, &is_new);
    if (cs_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && _emit_shader(
                       stream, cs_id, "COMPUTE", DRP2_COMPUTE_WGSL, DRP2_COMPUTE_GLSL, cfg);

    char cpipe_key[32];
    dvz_snprintf(cpipe_key, sizeof(cpipe_key), "_cpipe%s_%" PRIu64, fmt, bgl_stor_id);
    uint64_t cpipe_id = _obj_id(emitter, cpipe_key, &is_new);
    if (cpipe_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && dvz_drp2_stream_create_compute_pipeline_with_bind_group_layout(
                       stream, cpipe_id, cs_id, bgl_stor_id);

    char bg_stor_key[64];
    dvz_snprintf(
        bg_stor_key, sizeof(bg_stor_key), "_bg_stor_%" PRIu64 "_%" PRIu64,
        emitter->resources.first_compute_input_id,
        emitter->resources.first_compute_output_id);
    uint64_t bg_stor_id = _obj_id(emitter, bg_stor_key, &is_new);
    if (bg_stor_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && dvz_drp2_stream_create_storage_bind_group(
                       stream, bg_stor_id, bgl_stor_id,
                       emitter->resources.first_compute_input_id,
                       emitter->resources.first_compute_output_id,
                       emitter->resources.compute_buffer_size);

    char vs_key[16];
    dvz_snprintf(vs_key, sizeof(vs_key), "_vs%s", fmt);
    uint64_t vs_id = _obj_id(emitter, vs_key, &is_new);
    if (vs_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && _emit_shader(
                       stream, vs_id, "VERTEX", DRP2_VERTEX_WGSL, DRP2_VERTEX_GLSL, cfg);

    char fs_key[16];
    dvz_snprintf(fs_key, sizeof(fs_key), "_fs%s", fmt);
    uint64_t fs_id = _obj_id(emitter, fs_key, &is_new);
    if (fs_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && _emit_shader(
                       stream, fs_id, "FRAGMENT", DRP2_FRAGMENT_WGSL, DRP2_FRAGMENT_GLSL, cfg);

    char pipe_key[32];
    dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe1%s", fmt);
    uint64_t pipe_id = _obj_id(emitter, pipe_key, &is_new);
    if (pipe_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && dvz_drp2_stream_create_render_pipeline(stream, pipe_id, vs_id, fs_id, 1);

    uint64_t color_id = 0;
    if (cfg != NULL && cfg->external_color_target)
    {
        color_id = _color_target_id(cfg);
    }
    else
    {
        color_id = _obj_id(emitter, "_ct", &is_new);
        if (color_id == 0)
            return false;
        if (ok && is_new)
        {
            uint32_t usage =
                DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC;
            ok = ok && dvz_drp2_stream_create_texture_2d_usage(stream, color_id, 4, 4, usage);
        }
    }

    uint64_t rb_id = 0;
    if (readback != NULL)
    {
        rb_id = _obj_id(emitter, "_rb", &is_new);
        if (rb_id == 0)
            return false;
        if (ok && is_new)
        {
            uint32_t usage = DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ;
            ok = ok &&
                 dvz_drp2_stream_create_buffer(stream, rb_id, readback->u.copy.byte_size, usage);
        }
    }

    if (!ok)
        return false;

    uint64_t encoder_id = _emitter_next_transient_id(emitter);
    uint64_t compute_pass_id = _emitter_next_transient_id(emitter);
    uint64_t render_pass_id = _emitter_next_transient_id(emitter);
    uint64_t command_buffer_id = _emitter_next_transient_id(emitter);
    uint64_t submission_id = _emitter_next_transient_id(emitter);

    ok = dvz_drp2_stream_begin_command_encoder(stream, encoder_id) &&
         dvz_drp2_stream_begin_compute_pass(stream, compute_pass_id, encoder_id) &&
         dvz_drp2_stream_set_pipeline(stream, compute_pass_id, cpipe_id) &&
         dvz_drp2_stream_set_bind_group(stream, compute_pass_id, 0, bg_stor_id) &&
         dvz_drp2_stream_dispatch_workgroups(
             stream, compute_pass_id, compute->u.compute.dispatch[0],
             compute->u.compute.dispatch[1], compute->u.compute.dispatch[2]) &&
         dvz_drp2_stream_end_compute_pass(stream, compute_pass_id) &&
         dvz_drp2_stream_begin_render_pass(stream, render_pass_id, encoder_id, color_id) &&
         dvz_drp2_stream_set_pipeline(stream, render_pass_id, pipe_id) &&
         dvz_drp2_stream_set_vertex_buffer(
             stream, render_pass_id, 0, emitter->resources.first_compute_output_id, 0) &&
         dvz_drp2_stream_draw(stream, render_pass_id, 3, 1, 0, 0) &&
         dvz_drp2_stream_end_render_pass(stream, render_pass_id);
    if (ok && readback != NULL)
    {
        ok = ok && dvz_drp2_stream_copy_texture_to_buffer(
                       stream, encoder_id, color_id, rb_id, 0, 1, 1, 4, 1);
    }
    ok = ok && dvz_drp2_stream_finish_command_encoder(stream, encoder_id, command_buffer_id);
    if (readback != NULL)
    {
        ok = ok && dvz_drp2_stream_queue_submit_readback(
                       stream, command_buffer_id, submission_id, rb_id, 0,
                       readback->u.copy.byte_size);
    }
    else
    {
        ok = ok && dvz_drp2_stream_queue_submit(stream, command_buffer_id, submission_id);
    }
    return ok;
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
    DvzFramePlanEmitConfig cfg = {0};
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_WGSL;
    cfg.external_color_target = false;
    cfg.color_target_id = DRP2_ID_COLOR_TARGET;
    cfg.fullscreen_triangle = false;
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
    (void)caps;

    const DvzFramePlanNode* upload = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_UPLOAD);
    const DvzFramePlanNode* compute = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_COMPUTE);
    const DvzFramePlanNode* render = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_RENDER);
    const DvzFramePlanNode* copy = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_COPY);
    const DvzFramePlanNode* readback = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_READBACK);
    if (upload == NULL || render == NULL)
    {
        _diagnostic(report, "fixture converter requires upload+render");
        return NULL;
    }
    if (readback != NULL && copy == NULL)
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
    bool texture_render = _render_uses_texture(render);
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
            ok = texture_render ? _emit_texture_upload(&state, stream, &plan->nodes[i])
                                : _emit_upload(&state, stream, &plan->nodes[i]);
        }
    }
    ok = ok && (copy == NULL || _emit_readback_buffer(stream, copy)) &&
         (compute_render ? _emit_compute_assisted_render(stream, compute, render, &state, cfg)
                         : (texture_render
                                ? _emit_texture_render(stream, render, state.first_texture_id, cfg)
                                : _emit_render(
                                      stream, render, state.first_vertex_buffer_id, cfg))) &&
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
        _diagnostic(report, "failed to emit DRP2 fixture stream");
        dvz_drp2_stream_destroy(stream);
        return NULL;
    }
    return stream;
}



/**
 * Create a persistent FramePlan-to-DRP2 emitter for runtime-mode streams.
 *
 * @return the emitter
 */
DvzFramePlanEmitter* dvz_frame_plan_emitter(void)
{
    DvzFramePlanEmitter* emitter = (DvzFramePlanEmitter*)dvz_calloc(
        1, sizeof(DvzFramePlanEmitter));
    if (emitter == NULL)
        return NULL;
    _state_init(&emitter->resources);
    _state_init(&emitter->objects);
    emitter->objects.next_id = DRP2_EMITTER_OBJECT_ID_BASE;
    emitter->next_transient_id = DRP2_RUNTIME_TRANSIENT_ID_BASE;
    return emitter;
}



/**
 * Destroy a persistent FramePlan-to-DRP2 emitter.
 *
 * @param emitter the emitter
 */
void dvz_frame_plan_emitter_destroy(DvzFramePlanEmitter* emitter)
{
    if (emitter == NULL)
        return;
    dvz_free(emitter);
}



uint64_t dvz_frame_plan_emitter_object_id(const DvzFramePlanEmitter* emitter, const char* key)
{
    ANN(emitter);
    ANN(key);
    const ConverterState* state = &emitter->objects;
    for (uint32_t i = 0; i < state->count; i++)
    {
        if (strcmp(state->resources[i].key, key) == 0)
            return state->resources[i].id;
    }
    return 0;
}



/**
 * Emit a runtime-mode DRP2 command stream from a FramePlan.
 *
 * @param emitter the persistent emitter
 * @param plan the FramePlan
 * @param caps the capability snapshot
 * @param report the diagnostic report
 * @param cfg the emission configuration
 * @return an owned DRP2 command stream, or NULL on failure
 */
DvzDrp2CommandStream* dvz_frame_plan_emitter_emit_drp2(
    DvzFramePlanEmitter* emitter, const DvzFramePlan* plan, const DvzCapabilitySnapshot* caps,
    DvzDiagnosticReport* report, const DvzFramePlanEmitConfig* cfg)
{
    ANN(emitter);
    ANN(plan);

    const DvzFramePlanNode* upload = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_UPLOAD);
    const DvzFramePlanNode* compute = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_COMPUTE);
    const DvzFramePlanNode* render = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_RENDER);
    const DvzFramePlanNode* copy = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_COPY);
    const DvzFramePlanNode* readback = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_READBACK);

    if (upload == NULL || render == NULL)
    {
        _diagnostic(report, "runtime converter requires upload+render");
        return NULL;
    }
    bool texture_render = _render_uses_texture(render);
    if (compute != NULL)
    {
        if (compute->u.compute.write_count == 0)
        {
            _diagnostic(report, "runtime converter requires compute output");
            return NULL;
        }
    }
    if (readback != NULL && copy == NULL)
    {
        _diagnostic(report, "runtime converter requires copy before readback");
        return NULL;
    }
    if (caps != NULL && !_validate_capabilities(plan, caps, cfg, report))
        return NULL;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    bool ok = true;
    uint64_t vertex_buffer_ids[DVZ_SCENE_MAX_NODE_RESOURCES] = {0};
    uint32_t vertex_buffer_count = 0;
    uint64_t texture_id = 0;
    if (!emitter->handshake_sent)
    {
        ok = dvz_drp2_stream_hello_renderer(stream, "scene-runtime") &&
             dvz_drp2_stream_renderer_hello_reply(stream, "datoviz-drp2-runtime");
        emitter->handshake_sent = ok;
    }

    for (uint32_t i = 0; ok && i < plan->count; i++)
    {
        if (plan->nodes[i].type == DVZ_FRAME_PLAN_NODE_UPLOAD)
        {
            if (compute != NULL)
            {
                ok = _emitter_emit_compute_buffers(emitter, stream, &plan->nodes[i], compute);
            }
            else if (texture_render)
            {
                ok = _emitter_emit_texture_upload(emitter, stream, &plan->nodes[i], &texture_id);
            }
            else
            {
                if (vertex_buffer_count >= DVZ_SCENE_MAX_NODE_RESOURCES)
                {
                    ok = false;
                    break;
                }
                ok = _emitter_emit_upload(
                    emitter, stream, &plan->nodes[i], &vertex_buffer_ids[vertex_buffer_count]);
                if (ok)
                    vertex_buffer_count++;
            }
        }
    }

    ok = ok && (compute != NULL
                    ? _emitter_emit_compute_assisted_render(emitter, stream, compute, copy, cfg)
                    : texture_render
                    ? _emitter_emit_texture_render(emitter, stream, texture_id, copy, cfg)
                    : _emitter_emit_render(
                          emitter, stream, vertex_buffer_ids, vertex_buffer_count, copy, cfg));
    if (!ok)
    {
        _diagnostic(report, "failed to emit runtime DRP2 stream");
        dvz_drp2_stream_destroy(stream);
        return NULL;
    }
    return stream;
}
