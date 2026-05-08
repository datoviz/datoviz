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
#include "_overflow.h"
#include "datoviz/drp2.h"
#include "datoviz/drp2/stream.h"
#include "datoviz/fileio/fileio.h"
#include "datoviz/math/_cglm.h"
#include "datoviz/scene.h"
#include "datoviz/scene/panzoom.h"
#include "_scene.h"



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
    "layout(set=0,binding=0)uniform MVP{mat4 model;mat4 view;mat4 proj;float time;uint flags;}mvp;\n" \
    "layout(location=0)in vec3 inPos;\n"                                                        \
    "layout(location=1)in vec4 inColor;\n"                                                      \
    "layout(location=2)in float inSize;\n"                                                      \
    "layout(location=0)out vec4 fragColor;\n"                                                   \
    "void main(){"                                                                               \
    "gl_Position=mvp.proj*mvp.view*mvp.model*vec4(inPos,1.0);"                                 \
    "gl_PointSize=inSize;"                                                                       \
    "fragColor=inColor;}\n"
#define DRP2_POINT_FRAGMENT_GLSL                                                                \
    "#version 450\n"                                                                            \
    "layout(location=0)in vec4 fragColor;\n"                                                    \
    "layout(location=0)out vec4 outColor;\n"                                                    \
    "void main(){outColor=fragColor;}\n"

/* Primitive visual: position (vec3) + color (u8 RGBA→vec4); topology selected per visual. */
#define DRP2_PRIMITIVE_VERTEX_GLSL                                                              \
    "#version 450\n"                                                                            \
    "layout(set=0,binding=0)uniform MVP{mat4 model;mat4 view;mat4 proj;float time;uint flags;}mvp;\n" \
    "layout(location=0)in vec3 inPos;\n"                                                        \
    "layout(location=1)in vec4 inColor;\n"                                                      \
    "layout(location=0)out vec4 fragColor;\n"                                                   \
    "void main(){gl_Position=mvp.proj*mvp.view*mvp.model*vec4(inPos,1.0);fragColor=inColor;}\n"
#define DRP2_PRIMITIVE_FRAGMENT_GLSL                                                            \
    "#version 450\n"                                                                            \
    "layout(location=0)in vec4 fragColor;\n"                                                    \
    "layout(location=0)out vec4 outColor;\n"                                                    \
    "void main(){outColor=fragColor;}\n"

/* Image visual: position (vec3) + texcoords (vec2); samples a 2D RGBA8 texture. */
#define DRP2_IMAGE_VERTEX_GLSL                                                                  \
    "#version 450\n"                                                                            \
    "layout(location=0)in vec3 inPos;\n"                                                        \
    "layout(location=1)in vec2 inUV;\n"                                                         \
    "layout(location=0)out vec2 fragUV;\n"                                                      \
    "void main(){gl_Position=vec4(inPos,1.0);fragUV=inUV;}\n"
#define DRP2_IMAGE_FRAGMENT_GLSL                                                                \
    "#version 450\n"                                                                            \
    "layout(set=0,binding=0)uniform sampler2D tex;\n"                                           \
    "layout(location=0)in vec2 fragUV;\n"                                                       \
    "layout(location=0)out vec4 outColor;\n"                                                    \
    "void main(){outColor=texture(tex,fragUV);}\n"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct ResourceId ResourceId;
typedef struct ConverterState ConverterState;
typedef struct SceneRenderStateCache SceneRenderStateCache;

struct ResourceId
{
    char key[DVZ_SCENE_LABEL_SIZE];
    uint64_t id;
    char data_tag[DVZ_SCENE_LABEL_SIZE]; /* attribute name, e.g. "position", "color", "size" */
    uint64_t byte_size;                  /* total bytes uploaded to this buffer               */
    uint32_t topology;                   /* primitive topology hint (UINT32_MAX = unset)      */
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


struct SceneRenderStateCache
{
    uint64_t pipeline_id;
    uint64_t bg_set0;
};


struct DvzFramePlanEmitter
{
    ConverterState resources;
    ConverterState objects;
    uint64_t next_transient_id;
    bool handshake_sent;

    /* MVP cache: APPLY slots are panel-specific, FIXED uses a shared identity slot. */
    char mvp_panel_ids[DVZ_SCENE_MAX_PANELS][DVZ_SCENE_LABEL_SIZE];
    DvzMVP mvp_cache[DVZ_SCENE_MAX_PANELS];
    uint32_t mvp_panel_count;
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



static DvzMVP* _emitter_mvp_slot(DvzFramePlanEmitter* emitter, const char* key)
{
    ANN(emitter);
    ANN(key);
    for (uint32_t i = 0; i < emitter->mvp_panel_count; i++)
    {
        if (strncmp(emitter->mvp_panel_ids[i], key, DVZ_SCENE_LABEL_SIZE) == 0)
            return &emitter->mvp_cache[i];
    }
    if (emitter->mvp_panel_count >= DVZ_SCENE_MAX_PANELS)
        return NULL;
    uint32_t slot = emitter->mvp_panel_count++;
    strncpy(emitter->mvp_panel_ids[slot], key, DVZ_SCENE_LABEL_SIZE - 1);
    return &emitter->mvp_cache[slot];
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
 * Look up an existing deterministic DRP2 id for a scene resource key.
 *
 * @param state the converter state
 * @param key the scene resource key
 * @return the DRP2 id, or 0 when the key is unknown
 */
static uint64_t _resource_lookup_id(const ConverterState* state, const char* key)
{
    ANN(state);
    ANN(key);
    for (uint32_t i = 0; i < state->count; i++)
    {
        if (strcmp(state->resources[i].key, key) == 0)
            return state->resources[i].id;
    }
    return 0;
}



/**
 * Return the mutable resource entry for a scene resource key.
 *
 * @param state the converter state
 * @param key the scene resource key
 * @return the resource entry, or NULL when not found
 */
static ResourceId* _resource_find(ConverterState* state, const char* key)
{
    ANN(state);
    ANN(key);
    for (uint32_t i = 0; i < state->count; i++)
    {
        if (strcmp(state->resources[i].key, key) == 0)
            return &state->resources[i];
    }
    return NULL;
}



/**
 * Return a resource entry, creating it when needed.
 *
 * @param state the converter state
 * @param key the scene resource key
 * @param is_new whether a new entry was created
 * @return the resource entry, or NULL when the map is full
 */
static ResourceId* _resource_entry(ConverterState* state, const char* key, bool* is_new)
{
    ANN(state);
    ANN(key);
    ANN(is_new);

    ResourceId* resource = _resource_find(state, key);
    if (resource != NULL)
    {
        *is_new = false;
        return resource;
    }
    if (state->count >= DRP2_MAX_FIXTURE_RESOURCES)
        return NULL;

    resource = &state->resources[state->count++];
    dvz_memset(resource, sizeof(ResourceId), 0, sizeof(ResourceId));
    dvz_strlcpy(resource->key, key, sizeof(resource->key));
    resource->id = state->next_id++;
    resource->topology = UINT32_MAX;
    *is_new = true;
    return resource;
}



/**
 * Ensure a persisted resource has enough byte capacity.
 *
 * @param state the converter state
 * @param resource the resource entry
 * @param required_size the required byte size
 * @param needs_create whether a CreateBuffer command must be emitted
 * @return whether the resource was sized successfully
 */
static bool _resource_ensure_byte_size(
    ConverterState* state, ResourceId* resource, uint64_t required_size, bool* needs_create)
{
    ANN(state);
    ANN(resource);
    ANN(needs_create);

    if (*needs_create || resource->byte_size == 0)
    {
        resource->byte_size = required_size;
        *needs_create = true;
        return true;
    }
    if (required_size <= resource->byte_size)
        return true;

    if (state->next_id == UINT64_MAX)
        return false;
    resource->id = state->next_id++;
    resource->byte_size = required_size;
    *needs_create = true;
    return true;
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

static uint32_t _resource_topology(const ConverterState* state, uint64_t id)
{
    for (uint32_t i = 0; i < state->count; i++)
        if (state->resources[i].id == id)
            return state->resources[i].topology;
    return UINT32_MAX;
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

/*
 * Return true when ids carry exactly "position" + "color" with a topology hint on the
 * position resource, identifying a DvzPrimitive visual.
 */
static bool _is_primitive_visual(
    const ConverterState* state, const uint64_t* ids, uint32_t n)
{
    if (n != 2)
        return false;
    bool has_pos = false, has_col = false, has_topo = false;
    for (uint32_t i = 0; i < n; i++)
    {
        const char* tag = _resource_data_tag(state, ids[i]);
        if (strcmp(tag, "position") == 0)
        {
            has_pos = true;
            if (_resource_topology(state, ids[i]) != UINT32_MAX)
                has_topo = true;
        }
        if (strcmp(tag, "color") == 0)
            has_col = true;
    }
    return has_pos && has_col && has_topo;
}

/*
 * Return true when ids carry exactly "position" + "texcoords" + "texture", identifying
 * a DvzImage visual. Outputs the position id, texcoords id, and texture id.
 */
static bool _is_image_visual(
    const ConverterState* state, const uint64_t* ids, uint32_t n,
    uint64_t* out_pos, uint64_t* out_uv, uint64_t* out_tex)
{
    if (n != 3)
        return false;
    uint64_t pos = 0, uv = 0, tex = 0;
    for (uint32_t i = 0; i < n; i++)
    {
        const char* tag = _resource_data_tag(state, ids[i]);
        if (strcmp(tag, "position") == 0)  pos = ids[i];
        else if (strcmp(tag, "texcoords") == 0) uv = ids[i];
        else if (strcmp(tag, "texture") == 0)   tex = ids[i];
    }
    if (pos == 0 || uv == 0 || tex == 0)
        return false;
    if (out_pos) *out_pos = pos;
    if (out_uv)  *out_uv  = uv;
    if (out_tex) *out_tex = tex;
    return true;
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
 * Return a persistent runtime object id for a buffer with at least the requested size.
 *
 * @param emitter the persistent emitter
 * @param key the object key
 * @param byte_size the required byte size
 * @param is_new whether a CreateBuffer command must be emitted
 * @return the object id, or 0 on failure
 */
static uint64_t
_obj_buffer_id(DvzFramePlanEmitter* emitter, const char* key, uint64_t byte_size, bool* is_new)
{
    ANN(emitter);
    ANN(key);
    ANN(is_new);

    ResourceId* resource = _resource_entry(&emitter->objects, key, is_new);
    if (resource == NULL)
        return 0;
    if (!_resource_ensure_byte_size(&emitter->objects, resource, byte_size, is_new))
        return 0;
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
 * Allocate a base64 string representing zero-initialized bytes.
 *
 * @param byte_size the decoded byte size
 * @return the owned base64 string, or NULL on failure
 */
static char* _zero_base64_alloc(uint64_t byte_size)
{
    uint64_t groups = byte_size / 3;
    uint64_t remainder = byte_size % 3;
    uint64_t extra = remainder == 0 ? 0 : 4;
    if (groups > (UINT64_MAX - extra) / 4)
        return NULL;

    uint64_t count = groups * 4 + extra;
    if (count == UINT64_MAX)
        return NULL;

    char* out = (char*)dvz_malloc(count + 1);
    if (out == NULL)
        return NULL;
    if (!_zero_base64(byte_size, out, count + 1))
    {
        dvz_free(out);
        return NULL;
    }
    return out;
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
    bool has_scene_render = false;  /* scene nodes do per-visual draws, not one composite draw */
    uint64_t max_readback_size = 0;
    const DvzFramePlanNode* render = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_RENDER);
    if (render != NULL)
    {
        has_texture_render = _render_uses_texture(render);
        has_scene_render   = render->u.render.visual_count > 0;
    }

    for (uint32_t i = 0; i < plan->count; i++)
    {
        const DvzFramePlanNode* node = &plan->nodes[i];
        switch (node->type)
        {
        case DVZ_FRAME_PLAN_NODE_UPLOAD:
            upload_count++;
            uint64_t upload_end = 0;
            if (_dvz_add_u64_overflows(
                    node->u.upload.byte_offset, node->u.upload.byte_size, &upload_end) ||
                upload_end > caps->max_buffer_size)
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

    /* For the fixture (non-scene) render pipeline, all uploads become vertex buffers in one draw.
     * For scene render nodes, each visual uses at most DVZ_SCENE_MAX_NODE_RESOURCES buffers per
     * draw, so check that bound rather than total upload count. */
    if (!has_texture_render)
    {
        uint32_t effective_count =
            has_scene_render ? DVZ_SCENE_MAX_NODE_RESOURCES : upload_count;
        if (effective_count > caps->max_vertex_buffers)
        {
            _diagnostic(report, "max_vertex_buffers is too small for fixture render pipeline");
            return false;
        }
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



/* Emit a shader using precompiled SPIR-V when available; fall back to runtime GLSL. */
static bool _emit_shader_spirv(
    DvzDrp2CommandStream* stream, uint64_t id, const char* stage,
    const char* spirv_key, const char* glsl, const DvzFramePlanEmitConfig* cfg)
{
    ANN(stream);
    ANN(stage);
    unsigned long spv_size = 0;
    const unsigned char* spv = dvz_resource_shader(spirv_key, &spv_size);
    if (spv != NULL && spv_size > 0)
        return dvz_drp2_stream_create_shader_module_spirv(
            stream, id, stage, spv, (uint64_t)spv_size);
    /* Fallback: runtime GLSL compilation. */
    return dvz_drp2_stream_create_shader_module_format(stream, id, stage, "glsl", glsl);
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
    uint32_t usage = DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_VERTEX;
    bool ok = dvz_drp2_stream_create_buffer(stream, id, buffer_size, usage) &&
              dvz_drp2_stream_write_buffer(
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
              dvz_drp2_stream_write_texture_2d(stream, id, 0, 2, 2, 8, 2, data);
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
              dvz_drp2_stream_write_buffer(
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

    /* Texture upload: routed when texture_width > 0 (RGBA8 2D). */
    if (node->u.upload.texture_width > 0 && node->u.upload.texture_height > 0)
    {
        bool is_new = false;
        ResourceId* resource =
            _resource_entry(&emitter->resources, node->u.upload.resource_id, &is_new);
        if (resource == NULL)
            return false;
        dvz_strlcpy(resource->data_tag, node->u.upload.data_tag, sizeof(resource->data_tag));
        resource->byte_size = node->u.upload.byte_size;
        uint64_t id = resource->id;
        uint32_t w  = node->u.upload.texture_width;
        uint32_t h  = node->u.upload.texture_height;
        uint32_t bpr = w * 4;
        if (is_new)
        {
            uint32_t usage =
                DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING | DVZ_DRP2_TEXTURE_USAGE_COPY_DST;
            if (!dvz_drp2_stream_create_texture_2d_usage(stream, id, w, h, usage))
                return false;
        }
        if (emitter->resources.first_texture_id == 0)
            emitter->resources.first_texture_id = id;
        *out_id = id;
        if (node->u.upload.data == NULL)
            return false;
        return dvz_drp2_stream_write_texture_2d_bytes(
            stream, id, 0, w, h, bpr, h, node->u.upload.data);
    }

    uint64_t buffer_size = 0;
    if (_dvz_add_u64_overflows(node->u.upload.byte_offset, node->u.upload.byte_size, &buffer_size))
        return false;

    bool is_new = false;
    ResourceId* resource =
        _resource_entry(&emitter->resources, node->u.upload.resource_id, &is_new);
    if (resource == NULL)
        return false;
    if (!_resource_ensure_byte_size(&emitter->resources, resource, buffer_size, &is_new))
        return false;

    dvz_strlcpy(resource->data_tag, node->u.upload.data_tag, sizeof(resource->data_tag));
    if (node->u.upload.topology != UINT32_MAX)
        resource->topology = node->u.upload.topology;
    uint64_t id = resource->id;
    uint32_t usage = DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_VERTEX;
    if (is_new && !dvz_drp2_stream_create_buffer(stream, id, buffer_size, usage))
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
        char* zero_data = _zero_base64_alloc(node->u.upload.byte_size);
        if (zero_data == NULL)
            return false;
        bool ok = dvz_drp2_stream_write_buffer(
            stream, id, node->u.upload.byte_offset, node->u.upload.byte_size, zero_data);
        dvz_free(zero_data);
        return ok;
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

    char* data = _zero_base64_alloc(node->u.upload.byte_size);
    if (data == NULL)
        return false;

    uint32_t usage = DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING | DVZ_DRP2_TEXTURE_USAGE_COPY_DST;
    if (!exists && !dvz_drp2_stream_create_texture_2d_usage(stream, id, 2, 2, usage))
    {
        dvz_free(data);
        return false;
    }
    if (emitter->resources.first_texture_id == 0)
        emitter->resources.first_texture_id = id;
    *out_id = id;
    bool ok = dvz_drp2_stream_write_texture_2d(stream, id, 0, 2, 2, 8, 2, data);
    dvz_free(data);
    return ok;
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

    uint64_t input_size = 0;
    if (_dvz_add_u64_overflows(
            upload->u.upload.byte_offset, upload->u.upload.byte_size, &input_size))
        return false;

    bool input_create = false;
    bool output_create = false;
    ResourceId* input =
        _resource_entry(&emitter->resources, upload->u.upload.resource_id, &input_create);
    ResourceId* output =
        _resource_entry(&emitter->resources, compute->u.compute.writes[0], &output_create);
    if (input == NULL || output == NULL)
        return false;
    if (!_resource_ensure_byte_size(&emitter->resources, input, input_size, &input_create))
        return false;
    if (!_resource_ensure_byte_size(&emitter->resources, output, input_size, &output_create))
        return false;

    uint64_t input_id = input->id;
    uint64_t output_id = output->id;
    emitter->resources.first_compute_input_id = input_id;
    emitter->resources.first_compute_output_id = output_id;
    emitter->resources.first_vertex_buffer_id = output_id;
    emitter->resources.compute_buffer_size = input_size;

    char* data = _zero_base64_alloc(upload->u.upload.byte_size);
    if (data == NULL)
        return false;

    bool ok = true;
    if (input_create)
    {
        ok = ok && dvz_drp2_stream_create_buffer(
                       stream, input_id, input_size,
                       DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_STORAGE);
    }
    ok = ok && dvz_drp2_stream_write_buffer(
                   stream, input_id, upload->u.upload.byte_offset, upload->u.upload.byte_size,
                   data);
    if (output_create)
    {
        ok = ok && dvz_drp2_stream_create_buffer(
                       stream, output_id, input_size,
                       DVZ_DRP2_BUFFER_USAGE_STORAGE | DVZ_DRP2_BUFFER_USAGE_VERTEX);
    }
    dvz_free(data);
    return ok;
}



/**
 * Resolve persistent vertex-buffer ids for a render node with no new uploads.
 *
 * @param emitter the persistent emitter
 * @param render the render node
 * @param out_ids the output vertex buffer ids
 * @param out_count the output vertex buffer count
 * @return whether all ids were resolved
 */
static bool _emitter_resolve_render_vertex_buffers(
    DvzFramePlanEmitter* emitter, const DvzFramePlanNode* render, uint64_t* out_ids,
    uint32_t* out_count)
{
    ANN(emitter);
    ANN(render);
    ANN(out_ids);
    ANN(out_count);

    *out_count = 0;
    for (uint32_t i = 0; i < render->u.render.visual_count; i++)
    {
        const char* visual_id = render->u.render.visuals[i];
        /* "position" is always required. Other attrs are family-dependent and optional. */
        char pos_id[DVZ_SCENE_LABEL_SIZE];
        dvz_snprintf(pos_id, sizeof(pos_id), "%s_position", visual_id);
        uint64_t pos = _resource_lookup_id(&emitter->resources, pos_id);
        if (pos == 0)
            return false;
        if (*out_count >= DVZ_SCENE_MAX_NODE_RESOURCES)
            return false;
        out_ids[(*out_count)++] = pos;

        /* Optional attrs — collect any that exist. Order matches family pipeline expectations:
         * POINT      = position, color, size
         * PRIMITIVE  = position, color
         * IMAGE      = position, texcoords (+ texture, registered alongside).
         */
        const char* optional[] = {"color", "size", "texcoords", "texture"};
        for (uint32_t ai = 0; ai < 4; ai++)
        {
            char rid[DVZ_SCENE_LABEL_SIZE];
            dvz_snprintf(rid, sizeof(rid), "%s_%s", visual_id, optional[ai]);
            uint64_t id = _resource_lookup_id(&emitter->resources, rid);
            if (id == 0)
                continue;
            if (*out_count >= DVZ_SCENE_MAX_NODE_RESOURCES)
                return false;
            out_ids[(*out_count)++] = id;
        }
    }
    return *out_count > 0;
}



/* Scene render path: one panel's draws emitted inside an already-open render pass. */
static bool _emitter_emit_render_multi_in_pass(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlanNode* render,
    uint64_t render_pass_id, const DvzFramePlanEmitConfig* cfg, SceneRenderStateCache* cache)
{
    ANN(emitter);
    ANN(stream);
    ANN(render);

    bool ok = true;
    bool is_new = false;
    const char* fmt = _shader_format_tag(cfg);

    /* --- MVP UBO infrastructure (one BGL shared across all panels) --- */
    uint64_t mvp_bgl_id = _obj_id(emitter, "_bgl_mvp", &is_new);
    if (mvp_bgl_id == 0)
        return false;
    if (is_new)
        ok = ok && dvz_drp2_stream_create_uniform_bind_group_layout(stream, mvp_bgl_id);

    /* Determine which modes are needed for this panel. */
    bool needs_apply = false, needs_fixed = false;
    for (uint32_t i = 0; i < render->u.render.visual_count; i++)
    {
        if (render->u.render.controller_modes[i] == DVZ_CONTROLLER_FIXED)
            needs_fixed = true;
        else
            needs_apply = true;
    }

    uint64_t apply_bg_id = 0, fixed_bg_id = 0;

    if (needs_apply && ok)
    {
        char buf_key[128], bg_key[128], slot_key[128];
        dvz_snprintf(buf_key, sizeof(buf_key), "_mvp_buf_%s_apply", render->u.render.panel_id);
        dvz_snprintf(bg_key, sizeof(bg_key), "_mvp_bg_%s_apply", render->u.render.panel_id);
        dvz_snprintf(slot_key, sizeof(slot_key), "%s_apply", render->u.render.panel_id);

        uint64_t buf_id = _obj_id(emitter, buf_key, &is_new);
        if (buf_id == 0)
            return false;
        if (is_new)
        {
            uint32_t usage = DVZ_DRP2_BUFFER_USAGE_UNIFORM | DVZ_DRP2_BUFFER_USAGE_MAP_WRITE |
                             DVZ_DRP2_BUFFER_USAGE_COPY_DST;
            ok = ok && dvz_drp2_stream_create_buffer(stream, buf_id, sizeof(DvzMVP), usage);
        }
        uint64_t bg_id = _obj_id(emitter, bg_key, &is_new);
        if (bg_id == 0)
            return false;
        if (ok && is_new)
            ok = ok && dvz_drp2_stream_create_uniform_bind_group(
                           stream, bg_id, mvp_bgl_id, buf_id, 0, sizeof(DvzMVP));

        DvzMVP* slot = _emitter_mvp_slot(emitter, slot_key);
        if (slot != NULL)
            *slot = render->u.render.apply_mvp;
        ok = ok && dvz_drp2_stream_write_buffer_bytes(
                       stream, buf_id, 0, sizeof(DvzMVP),
                       slot ? slot : &render->u.render.apply_mvp);
        apply_bg_id = bg_id;
    }

    if (needs_fixed && ok)
    {
        const char* buf_key = "_mvp_buf_fixed";
        const char* bg_key = "_mvp_bg_fixed";
        const char* slot_key = "_fixed";

        uint64_t buf_id = _obj_id(emitter, buf_key, &is_new);
        if (buf_id == 0)
            return false;
        if (is_new)
        {
            uint32_t usage = DVZ_DRP2_BUFFER_USAGE_UNIFORM | DVZ_DRP2_BUFFER_USAGE_MAP_WRITE |
                             DVZ_DRP2_BUFFER_USAGE_COPY_DST;
            ok = ok && dvz_drp2_stream_create_buffer(stream, buf_id, sizeof(DvzMVP), usage);
        }
        uint64_t bg_id = _obj_id(emitter, bg_key, &is_new);
        if (bg_id == 0)
            return false;
        if (ok && is_new)
            ok = ok && dvz_drp2_stream_create_uniform_bind_group(
                           stream, bg_id, mvp_bgl_id, buf_id, 0, sizeof(DvzMVP));

        DvzMVP* slot = _emitter_mvp_slot(emitter, slot_key);
        if (slot != NULL)
        {
            glm_mat4_identity(slot->model);
            glm_mat4_identity(slot->view);
            glm_mat4_identity(slot->proj);
            slot->time  = 0.0f;
            slot->flags = 0;
        }
        DvzMVP local_identity = {0};
        glm_mat4_identity(local_identity.model);
        glm_mat4_identity(local_identity.view);
        glm_mat4_identity(local_identity.proj);
        ok = ok && dvz_drp2_stream_write_buffer_bytes(
                       stream, buf_id, 0, sizeof(DvzMVP),
                       slot ? slot : &local_identity);
        fixed_bg_id = bg_id;
    }

    /* Image BGL + sampler (shared, created lazily on first image visual). */
    uint64_t img_bgl_id = 0, img_sampler_id = 0;

    /* Per-visual draw descriptors. */
    struct {
        uint64_t pipeline_id;
        uint64_t bg_set0;  /* MVP bg (point/prim) or texture bg (image); 0 = none */
        uint64_t vbuf_ids[DVZ_SCENE_MAX_NODE_RESOURCES];
        uint32_t vbuf_count;
        uint32_t vertex_count;
    } draws[DVZ_SCENE_MAX_RENDER_VISUALS];
    uint32_t draw_count = 0;

    for (uint32_t i = 0; ok && i < render->u.render.visual_count; i++)
    {
        const char* visual_id = render->u.render.visuals[i];

        /* Resolve vertex buffers for this visual. */
        uint64_t vbuf_ids[DVZ_SCENE_MAX_NODE_RESOURCES] = {0};
        uint32_t vbuf_count = 0;

        char pos_key[DVZ_SCENE_LABEL_SIZE];
        dvz_snprintf(pos_key, sizeof(pos_key), "%s_position", visual_id);
        uint64_t pos_buf = _resource_lookup_id(&emitter->resources, pos_key);
        if (pos_buf == 0)
            continue;
        vbuf_ids[vbuf_count++] = pos_buf;

        const char* optionals[] = {"color", "size", "texcoords", "texture"};
        for (uint32_t ai = 0; ai < 4; ai++)
        {
            char rid[DVZ_SCENE_LABEL_SIZE];
            dvz_snprintf(rid, sizeof(rid), "%s_%s", visual_id, optionals[ai]);
            uint64_t rid_id = _resource_lookup_id(&emitter->resources, rid);
            if (rid_id == 0 || vbuf_count >= DVZ_SCENE_MAX_NODE_RESOURCES)
                continue;
            vbuf_ids[vbuf_count++] = rid_id;
        }

        /* Detect visual family. */
        bool vis_is_point = _is_point_visual(&emitter->resources, vbuf_ids, vbuf_count);
        bool vis_is_prim =
            !vis_is_point && _is_primitive_visual(&emitter->resources, vbuf_ids, vbuf_count);
        uint64_t img_pos = 0, img_uv = 0, img_tex = 0;
        bool vis_is_image =
            !vis_is_point && !vis_is_prim &&
            _is_image_visual(&emitter->resources, vbuf_ids, vbuf_count, &img_pos, &img_uv, &img_tex);

        if (!vis_is_point && !vis_is_prim && !vis_is_image)
            continue;

        /* Vertex count from position buffer. */
        uint64_t pos_sz = _resource_byte_size(&emitter->resources, pos_buf);
        uint32_t vertex_count = (pos_sz > 0) ? (uint32_t)(pos_sz / (3 * sizeof(float))) : 3;

        /* Topology. */
        uint32_t topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        if (vis_is_prim)
            topology = _resource_topology(&emitter->resources, pos_buf);
        else if (vis_is_image)
            topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

        /* Shader keys. */
        char vs_key[32], fs_key[16], pipe_key[48];
        const char* vs_glsl      = NULL;
        const char* fs_glsl      = NULL;
        const char* vs_spirv_key = NULL;
        const char* fs_spirv_key = NULL;

        if (vis_is_point)
        {
            dvz_snprintf(vs_key, sizeof(vs_key), "_vs_point%s", fmt);
            dvz_snprintf(fs_key, sizeof(fs_key), "_fs_point%s", fmt);
            vs_glsl      = DRP2_POINT_VERTEX_GLSL;
            fs_glsl      = DRP2_POINT_FRAGMENT_GLSL;
            vs_spirv_key = "point_vert";
            fs_spirv_key = "point_frag";
        }
        else if (vis_is_prim)
        {
            dvz_snprintf(vs_key, sizeof(vs_key), "_vs_prim%s", fmt);
            dvz_snprintf(fs_key, sizeof(fs_key), "_fs_prim%s", fmt);
            vs_glsl      = DRP2_PRIMITIVE_VERTEX_GLSL;
            fs_glsl      = DRP2_PRIMITIVE_FRAGMENT_GLSL;
            vs_spirv_key = "primitive_vert";
            fs_spirv_key = "primitive_frag";
        }
        else /* vis_is_image */
        {
            dvz_snprintf(vs_key, sizeof(vs_key), "_vs_img%s", fmt);
            dvz_snprintf(fs_key, sizeof(fs_key), "_fs_img%s", fmt);
            vs_glsl      = DRP2_IMAGE_VERTEX_GLSL;
            fs_glsl      = DRP2_IMAGE_FRAGMENT_GLSL;
            vs_spirv_key = "image_vert";
            fs_spirv_key = "image_frag";
        }

        /* Shaders (cached). */
        uint64_t vs_id = _obj_id(emitter, vs_key, &is_new);
        if (vs_id == 0) { ok = false; break; }
        if (is_new)
            ok = ok && _emit_shader_spirv(stream, vs_id, "VERTEX", vs_spirv_key, vs_glsl, cfg);

        uint64_t fs_id = _obj_id(emitter, fs_key, &is_new);
        if (fs_id == 0) { ok = false; break; }
        if (ok && is_new)
            ok = ok && _emit_shader_spirv(stream, fs_id, "FRAGMENT", fs_spirv_key, fs_glsl, cfg);

        /* Pipeline (cached by family + topology). */
        if (vis_is_point)
            dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe_point%s", fmt);
        else if (vis_is_prim)
            dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe_prim_t%u%s", topology, fmt);
        else
            dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe_img%s", fmt);

        uint64_t pipe_id = _obj_id(emitter, pipe_key, &is_new);
        if (pipe_id == 0) { ok = false; break; }
        if (ok && is_new)
        {
            if (vis_is_point)
            {
                uint32_t strides[3]   = {3 * sizeof(float), 4 * sizeof(uint8_t), sizeof(float)};
                uint32_t bindings[3]  = {0, 1, 2};
                uint32_t locations[3] = {0, 1, 2};
                uint32_t formats[3]   = {VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R8G8B8A8_UNORM,
                                       VK_FORMAT_R32_SFLOAT};
                uint32_t offsets[3]   = {0, 0, 0};
                ok = ok && dvz_drp2_stream_create_render_pipeline_ex(
                               stream, pipe_id, vs_id, fs_id, 3, topology,
                               3, strides, 3, bindings, locations, formats, offsets);
                if (ok && mvp_bgl_id != 0)
                    ok = dvz_drp2_stream_pipeline_set_bind_group_layout(stream, mvp_bgl_id);
            }
            else if (vis_is_prim)
            {
                uint32_t strides[2]   = {3 * sizeof(float), 4 * sizeof(uint8_t)};
                uint32_t bindings[2]  = {0, 1};
                uint32_t locations[2] = {0, 1};
                uint32_t formats[2]   = {VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R8G8B8A8_UNORM};
                uint32_t offsets[2]   = {0, 0};
                ok = ok && dvz_drp2_stream_create_render_pipeline_ex(
                               stream, pipe_id, vs_id, fs_id, 2, topology,
                               2, strides, 2, bindings, locations, formats, offsets);
                if (ok && mvp_bgl_id != 0)
                    ok = dvz_drp2_stream_pipeline_set_bind_group_layout(stream, mvp_bgl_id);
            }
            else /* vis_is_image */
            {
                uint32_t strides[2]   = {3 * sizeof(float), 2 * sizeof(float)};
                uint32_t bindings[2]  = {0, 1};
                uint32_t locations[2] = {0, 1};
                uint32_t formats[2]   = {VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32_SFLOAT};
                uint32_t offsets[2]   = {0, 0};

                /* Image BGL (lazy). */
                if (img_bgl_id == 0)
                {
                    img_bgl_id = _obj_id(emitter, "_bgl_img", &is_new);
                    if (img_bgl_id == 0) { ok = false; break; }
                    if (is_new)
                        ok = ok &&
                             dvz_drp2_stream_create_texture_sampler_bind_group_layout(
                                 stream, img_bgl_id);
                }
                ok = ok && dvz_drp2_stream_create_render_pipeline_ex(
                               stream, pipe_id, vs_id, fs_id, 2,
                               VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
                               2, strides, 2, bindings, locations, formats, offsets);
                if (ok && img_bgl_id != 0)
                    ok = dvz_drp2_stream_pipeline_set_bind_group_layout(stream, img_bgl_id);
            }
        }

        /* Bind group at set 0. */
        uint64_t vis_bg_set0 = 0;
        if (vis_is_point || vis_is_prim)
        {
            vis_bg_set0 = (render->u.render.controller_modes[i] == DVZ_CONTROLLER_FIXED)
                              ? fixed_bg_id
                              : apply_bg_id;
        }
        else /* vis_is_image */
        {
            /* Image BGL + sampler (lazy). */
            if (img_bgl_id == 0)
            {
                img_bgl_id = _obj_id(emitter, "_bgl_img", &is_new);
                if (img_bgl_id == 0) { ok = false; break; }
                if (is_new)
                    ok = ok && dvz_drp2_stream_create_texture_sampler_bind_group_layout(
                                   stream, img_bgl_id);
            }
            if (img_sampler_id == 0)
            {
                img_sampler_id = _obj_id(emitter, "_sampler_img", &is_new);
                if (img_sampler_id == 0) { ok = false; break; }
                if (ok && is_new)
                    ok = ok && dvz_drp2_stream_create_sampler(stream, img_sampler_id);
            }
            char img_bg_key[64];
            dvz_snprintf(img_bg_key, sizeof(img_bg_key), "_bg_img_%" PRIu64, img_tex);
            uint64_t img_bg_id = _obj_id(emitter, img_bg_key, &is_new);
            if (img_bg_id == 0) { ok = false; break; }
            if (ok && is_new)
                ok = ok && dvz_drp2_stream_create_texture_sampler_bind_group(
                               stream, img_bg_id, img_bgl_id, img_tex, img_sampler_id);
            vis_bg_set0 = img_bg_id;

            /* Narrow vertex buffers to (position, texcoords) for image draw. */
            vbuf_ids[0] = img_pos;
            vbuf_ids[1] = img_uv;
            vbuf_count  = 2;
        }

        if (!ok)
            break;

        draws[draw_count].pipeline_id = pipe_id;
        draws[draw_count].bg_set0     = vis_bg_set0;
        draws[draw_count].vertex_count = vertex_count;
        draws[draw_count].vbuf_count  = vbuf_count;
        for (uint32_t j = 0; j < vbuf_count; j++)
            draws[draw_count].vbuf_ids[j] = vbuf_ids[j];
        draw_count++;
    }

    if (!ok || draw_count == 0)
        return false;

    if (!ok)
        return false;

    ok = ok && dvz_drp2_stream_set_viewport(
                   stream, render_pass_id, render->u.render.desc.x, render->u.render.desc.y,
                   render->u.render.desc.width, render->u.render.desc.height) &&
         dvz_drp2_stream_set_scissor(
             stream, render_pass_id, render->u.render.desc.x, render->u.render.desc.y,
             render->u.render.desc.width, render->u.render.desc.height);

    uint64_t last_pipeline = (cache != NULL) ? cache->pipeline_id : 0;
    uint64_t last_bg_set0 = (cache != NULL) ? cache->bg_set0 : 0;
    for (uint32_t d = 0; ok && d < draw_count; d++)
    {
        if (draws[d].pipeline_id != last_pipeline)
        {
            ok = ok && dvz_drp2_stream_set_pipeline(stream, render_pass_id, draws[d].pipeline_id);
            last_pipeline = draws[d].pipeline_id;
            last_bg_set0  = 0;
        }
        if (draws[d].bg_set0 != 0 && draws[d].bg_set0 != last_bg_set0)
        {
            ok = ok &&
                 dvz_drp2_stream_set_bind_group(stream, render_pass_id, 0, draws[d].bg_set0);
            last_bg_set0 = draws[d].bg_set0;
        }
        for (uint32_t j = 0; ok && j < draws[d].vbuf_count; j++)
            ok = ok && dvz_drp2_stream_set_vertex_buffer(
                           stream, render_pass_id, j, draws[d].vbuf_ids[j], 0);
        ok = ok && dvz_drp2_stream_draw(stream, render_pass_id, draws[d].vertex_count, 1, 0, 0);
    }

    if (cache != NULL)
    {
        cache->pipeline_id = last_pipeline;
        cache->bg_set0 = last_bg_set0;
    }

    return ok;
}



/* Scene render path: one BeginRenderPass per panel, one Draw per visual inside it. */
static bool _emitter_emit_render_multi(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlanNode* render,
    const DvzFramePlanNode* readback, bool clear, const DvzFramePlanEmitConfig* cfg,
    SceneRenderStateCache* cache)
{
    ANN(emitter);
    ANN(stream);
    ANN(render);

    bool ok = true;
    bool is_new = false;

    /* Color target. */
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
        if (is_new)
        {
            uint32_t usage =
                DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC;
            ok = ok && dvz_drp2_stream_create_texture_2d_usage(stream, color_id, 4, 4, usage);
        }
    }

    uint64_t rb_id = 0;
    if (readback != NULL)
    {
        rb_id = _obj_buffer_id(emitter, "_rb", readback->u.copy.byte_size, &is_new);
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

    float cr = cfg ? cfg->clear_color[0] : 0.0f;
    float cg = cfg ? cfg->clear_color[1] : 0.0f;
    float cb = cfg ? cfg->clear_color[2] : 0.0f;
    float ca = cfg ? cfg->clear_color[3] : 1.0f;

    ok = dvz_drp2_stream_begin_command_encoder(stream, encoder_id) &&
         dvz_drp2_stream_begin_render_pass_region_clear(
             stream, render_pass_id, encoder_id, color_id, cr, cg, cb, ca, 0.0f, 0.0f, 1.0f,
             1.0f, clear) &&
         _emitter_emit_render_multi_in_pass(
             emitter, stream, render, render_pass_id, cfg, cache) &&
         dvz_drp2_stream_end_render_pass(stream, render_pass_id);
    if (ok && readback != NULL)
        ok = ok && dvz_drp2_stream_copy_texture_to_buffer(
                       stream, encoder_id, color_id, rb_id, 0, 1, 1, 4, 1);
    ok = ok && dvz_drp2_stream_finish_command_encoder(stream, encoder_id, command_buffer_id);
    if (readback != NULL)
        ok = ok && dvz_drp2_stream_queue_submit_readback(
                       stream, command_buffer_id, submission_id, rb_id, 0,
                       readback->u.copy.byte_size);
    else
        ok = ok && dvz_drp2_stream_queue_submit(stream, command_buffer_id, submission_id);
    return ok;
}



/**
 * Emit all scene render nodes inside one figure-wide render pass.
 *
 * @param emitter the persistent emitter
 * @param stream the DRP2 command stream
 * @param plan the FramePlan
 * @param readback the optional readback copy node
 * @param cfg the emission config
 * @return whether the commands were emitted
 */
static bool _emitter_emit_scene_figure_renders(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanNode* readback, const DvzFramePlanEmitConfig* cfg)
{
    ANN(emitter);
    ANN(stream);
    ANN(plan);

    bool ok = true;
    bool is_new = false;

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
        if (is_new)
        {
            uint32_t usage =
                DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC;
            ok = ok && dvz_drp2_stream_create_texture_2d_usage(stream, color_id, 4, 4, usage);
        }
    }

    uint64_t rb_id = 0;
    if (readback != NULL)
    {
        rb_id = _obj_buffer_id(emitter, "_rb", readback->u.copy.byte_size, &is_new);
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

    float cr = cfg ? cfg->clear_color[0] : 0.0f;
    float cg = cfg ? cfg->clear_color[1] : 0.0f;
    float cb = cfg ? cfg->clear_color[2] : 0.0f;
    float ca = cfg ? cfg->clear_color[3] : 1.0f;

    ok = dvz_drp2_stream_begin_command_encoder(stream, encoder_id) &&
         dvz_drp2_stream_begin_render_pass_region_clear(
             stream, render_pass_id, encoder_id, color_id, cr, cg, cb, ca, 0.0f, 0.0f, 1.0f,
             1.0f, true);

    SceneRenderStateCache scene_cache = {0};
    for (uint32_t i = 0; ok && i < plan->count; i++)
    {
        const DvzFramePlanNode* render = &plan->nodes[i];
        if (render->type != DVZ_FRAME_PLAN_NODE_RENDER || render->u.render.visual_count == 0)
            continue;
        ok = _emitter_emit_render_multi_in_pass(
            emitter, stream, render, render_pass_id, cfg, &scene_cache);
    }

    ok = ok && dvz_drp2_stream_end_render_pass(stream, render_pass_id);
    if (ok && readback != NULL)
        ok = ok && dvz_drp2_stream_copy_texture_to_buffer(
                       stream, encoder_id, color_id, rb_id, 0, 1, 1, 4, 1);
    ok = ok && dvz_drp2_stream_finish_command_encoder(stream, encoder_id, command_buffer_id);
    if (readback != NULL)
        ok = ok && dvz_drp2_stream_queue_submit_readback(
                       stream, command_buffer_id, submission_id, rb_id, 0,
                       readback->u.copy.byte_size);
    else
        ok = ok && dvz_drp2_stream_queue_submit(stream, command_buffer_id, submission_id);
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
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlanNode* render,
    const uint64_t* vertex_buffer_ids, uint32_t vertex_buffer_count,
    const DvzFramePlanNode* readback, bool clear, const DvzFramePlanEmitConfig* cfg,
    SceneRenderStateCache* cache)
{
    ANN(emitter);
    ANN(stream);
    ANN(render);

    /* Scene render node: per-visual multi-draw in a single pass. */
    if (vertex_buffer_count == 0 && render->u.render.visual_count > 0 &&
        cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
        return _emitter_emit_render_multi(emitter, stream, render, readback, clear, cfg, cache);

    /* Generic single-draw path (non-scene nodes, WGSL, or fallback). */
    ANN(vertex_buffer_ids);
    if (vertex_buffer_count == 0)
        return false;

    bool ok = true;
    bool is_new = false;
    const char* fmt = _shader_format_tag(cfg);

    /* Detect DvzPoint visual (position + color + size attributes). */
    bool is_point = _is_point_visual(&emitter->resources, vertex_buffer_ids, vertex_buffer_count);
    bool is_primitive =
        !is_point && _is_primitive_visual(&emitter->resources, vertex_buffer_ids, vertex_buffer_count);
    uint64_t image_pos = 0, image_uv = 0, image_tex = 0;
    bool is_image = !is_point && !is_primitive &&
                    _is_image_visual(&emitter->resources, vertex_buffer_ids, vertex_buffer_count,
                                     &image_pos, &image_uv, &image_tex);

    const char* vs_glsl = NULL;
    const char* fs_glsl = NULL;
    uint32_t topology = 0;
    uint32_t vertex_count = 3; /* default for stub / non-point path */
    uint64_t bgl_id = 0;
    uint64_t bg_id  = 0;

    /* MVP UBO bind group IDs — used for GLSL point/primitive path. */
    uint64_t mvp_bgl_id = 0;
    uint64_t mvp_buf_id = 0;
    uint64_t mvp_bg_id  = 0;
    bool uses_mvp =
        (is_point || is_primitive) &&
        cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL;

    /* When IMAGE: re-narrow vertex_buffer_ids to (position, texcoords) only — the texture
     * is bound through a bind group, not as a vertex buffer. */
    uint64_t image_vertex_ids[2];
    if (is_image)
    {
        image_vertex_ids[0] = image_pos;
        image_vertex_ids[1] = image_uv;
        vertex_buffer_ids   = image_vertex_ids;
        vertex_buffer_count = 2;
    }

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
    else if (is_primitive && cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
    {
        /* Primitive visual: pass-through shaders with visual-selected topology. */
        for (uint32_t i = 0; i < vertex_buffer_count; i++)
        {
            if (strcmp(_resource_data_tag(&emitter->resources, vertex_buffer_ids[i]),
                       "position") == 0)
            {
                uint64_t sz = _resource_byte_size(&emitter->resources, vertex_buffer_ids[i]);
                if (sz > 0)
                    vertex_count = (uint32_t)(sz / (3 * sizeof(float)));
                topology = _resource_topology(&emitter->resources, vertex_buffer_ids[i]);
                break;
            }
        }
        dvz_snprintf(vs_key, sizeof(vs_key), "_vs_prim%s", fmt);
        dvz_snprintf(fs_key, sizeof(fs_key), "_fs_prim%s", fmt);
        vs_glsl = DRP2_PRIMITIVE_VERTEX_GLSL;
        fs_glsl = DRP2_PRIMITIVE_FRAGMENT_GLSL;
    }
    else if (is_image && cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
    {
        /* Image visual: textured-quad shaders, TRIANGLE_STRIP topology, 4 vertices. */
        uint64_t pos_size = _resource_byte_size(&emitter->resources, image_pos);
        if (pos_size > 0)
            vertex_count = (uint32_t)(pos_size / (3 * sizeof(float)));
        topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        dvz_snprintf(vs_key, sizeof(vs_key), "_vs_img%s", fmt);
        dvz_snprintf(fs_key, sizeof(fs_key), "_fs_img%s", fmt);
        vs_glsl = DRP2_IMAGE_VERTEX_GLSL;
        fs_glsl = DRP2_IMAGE_FRAGMENT_GLSL;

        /* Sampler + texture-sampler bind-group layout + bind-group, all persistent. */
        bool bgl_new = false;
        bgl_id = _obj_id(emitter, "_bgl_img", &bgl_new);
        if (bgl_id == 0)
            return false;
        if (bgl_new)
            ok = ok && dvz_drp2_stream_create_texture_sampler_bind_group_layout(stream, bgl_id);

        bool sampler_new = false;
        uint64_t sampler_id = _obj_id(emitter, "_sampler_img", &sampler_new);
        if (sampler_id == 0)
            return false;
        if (ok && sampler_new)
            ok = ok && dvz_drp2_stream_create_sampler(stream, sampler_id);

        char bg_key[48];
        dvz_snprintf(bg_key, sizeof(bg_key), "_bg_img_%" PRIu64, image_tex);
        bool bg_new = false;
        bg_id = _obj_id(emitter, bg_key, &bg_new);
        if (bg_id == 0)
            return false;
        if (ok && bg_new)
            ok = ok && dvz_drp2_stream_create_texture_sampler_bind_group(
                           stream, bg_id, bgl_id, image_tex, sampler_id);
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

    /* MVP UBO infrastructure (GLSL point/primitive path only). */
    if (uses_mvp)
    {
        bool mvp_bgl_new = false;
        mvp_bgl_id = _obj_id(emitter, "_bgl_mvp", &mvp_bgl_new);
        if (mvp_bgl_id == 0)
            return false;
        if (mvp_bgl_new)
            ok = ok && dvz_drp2_stream_create_uniform_bind_group_layout(stream, mvp_bgl_id);

        const char* mode_tag = (render->u.render.controller_modes[0] == DVZ_CONTROLLER_FIXED)
                                   ? "fixed"
                                   : "apply";
        char mvp_buf_key[128], mvp_bg_key[128];
        dvz_snprintf(
            mvp_buf_key, sizeof(mvp_buf_key), "_mvp_buf_%s_%s", render->u.render.panel_id,
            mode_tag);
        dvz_snprintf(
            mvp_bg_key, sizeof(mvp_bg_key), "_mvp_bg_%s_%s", render->u.render.panel_id, mode_tag);

        bool mvp_buf_new = false;
        mvp_buf_id = _obj_id(emitter, mvp_buf_key, &mvp_buf_new);
        if (mvp_buf_id == 0)
            return false;
        if (mvp_buf_new)
        {
            uint32_t usage =
                DVZ_DRP2_BUFFER_USAGE_UNIFORM |
                DVZ_DRP2_BUFFER_USAGE_MAP_WRITE |
                DVZ_DRP2_BUFFER_USAGE_COPY_DST;
            ok = ok && dvz_drp2_stream_create_buffer(stream, mvp_buf_id, sizeof(DvzMVP), usage);
        }

        bool mvp_bg_new = false;
        mvp_bg_id = _obj_id(emitter, mvp_bg_key, &mvp_bg_new);
        if (mvp_bg_id == 0)
            return false;
        if (mvp_bg_new)
            ok = ok && dvz_drp2_stream_create_uniform_bind_group(
                           stream, mvp_bg_id, mvp_bgl_id, mvp_buf_id, 0, sizeof(DvzMVP));

        /* Copy MVP into the emitter's per-(panel, controller_mode) cache (persists past
         * frame plan destruction so write_buffer_bytes' borrowed pointer stays valid). */
        char mvp_slot_key[128];
        dvz_snprintf(
            mvp_slot_key, sizeof(mvp_slot_key), "%s_%s", render->u.render.panel_id, mode_tag);
        DvzMVP* mvp_slot = _emitter_mvp_slot(emitter, mvp_slot_key);
        if (mvp_slot != NULL)
            *mvp_slot = render->u.render.apply_mvp;
        ok = ok && dvz_drp2_stream_write_buffer_bytes(
                       stream, mvp_buf_id, 0, sizeof(DvzMVP),
                       mvp_slot ? mvp_slot : &render->u.render.apply_mvp);
    }

    /* SPIR-V resource names (stem of .vert.spv / .frag.spv after embed_resources key mangling). */
    const char* vs_spirv_key = NULL;
    const char* fs_spirv_key = NULL;
    if (is_point)
    {
        vs_spirv_key = "point_vert";
        fs_spirv_key = "point_frag";
    }
    else if (is_primitive)
    {
        vs_spirv_key = "primitive_vert";
        fs_spirv_key = "primitive_frag";
    }
    else if (is_image)
    {
        vs_spirv_key = "image_vert";
        fs_spirv_key = "image_frag";
    }

    uint64_t vs_id = _obj_id(emitter, vs_key, &is_new);
    if (vs_id == 0)
        return false;
    if (is_new)
    {
        if (vs_glsl != NULL && vs_spirv_key != NULL &&
            cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
        {
            ok = ok && _emit_shader_spirv(stream, vs_id, "VERTEX", vs_spirv_key, vs_glsl, cfg);
        }
        else if (vs_glsl != NULL)
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
        if (fs_glsl != NULL && fs_spirv_key != NULL &&
            cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
        {
            ok = ok && _emit_shader_spirv(stream, fs_id, "FRAGMENT", fs_spirv_key, fs_glsl, cfg);
        }
        else if (fs_glsl != NULL)
        {
            ok = ok && _emit_shader(stream, fs_id, "FRAGMENT", NULL, fs_glsl, cfg);
        }
        else
        {
            ok = ok && _emit_shader(
                           stream, fs_id, "FRAGMENT", DRP2_FRAGMENT_WGSL, DRP2_FRAGMENT_GLSL, cfg);
        }
    }

    if (is_point && cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
        dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe_point%s", fmt);
    else if (is_primitive && cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
        dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe_prim_t%u%s", topology, fmt);
    else if (is_image && cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
        dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe_img%s", fmt);
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
            if (ok && uses_mvp && mvp_bgl_id != 0)
                ok = dvz_drp2_stream_pipeline_set_bind_group_layout(stream, mvp_bgl_id);
        }
        else if (is_primitive && cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
        {
            /* binding0=position(vec3), binding1=color(u8vec4) */
            uint32_t strides[2]   = {3*sizeof(float), 4*sizeof(uint8_t)};
            uint32_t bindings[2]  = {0, 1};
            uint32_t locations[2] = {0, 1};
            uint32_t formats[2]   = {VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R8G8B8A8_UNORM};
            uint32_t offsets[2]   = {0, 0};
            ok = ok && dvz_drp2_stream_create_render_pipeline_ex(
                           stream, pipe_id, vs_id, fs_id, vertex_buffer_count,
                           topology,
                           2, strides,
                           2, bindings, locations, formats, offsets);
            if (ok && uses_mvp && mvp_bgl_id != 0)
                ok = dvz_drp2_stream_pipeline_set_bind_group_layout(stream, mvp_bgl_id);
        }
        else if (is_image && cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
        {
            /* binding0=position(vec3), binding1=texcoords(vec2); bgl=img */
            uint32_t strides[2]   = {3*sizeof(float), 2*sizeof(float)};
            uint32_t bindings[2]  = {0, 1};
            uint32_t locations[2] = {0, 1};
            uint32_t formats[2]   = {VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32_SFLOAT};
            uint32_t offsets[2]   = {0, 0};
            ok = ok && dvz_drp2_stream_create_render_pipeline_ex(
                           stream, pipe_id, vs_id, fs_id, vertex_buffer_count,
                           topology,
                           2, strides,
                           2, bindings, locations, formats, offsets);
            ok = ok && dvz_drp2_stream_pipeline_set_bind_group_layout(stream, bgl_id);
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
        rb_id = _obj_buffer_id(emitter, "_rb", readback->u.copy.byte_size, &is_new);
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

    float cr = cfg ? cfg->clear_color[0] : 0.0f;
    float cg = cfg ? cfg->clear_color[1] : 0.0f;
    float cb = cfg ? cfg->clear_color[2] : 0.0f;
    float ca = cfg ? cfg->clear_color[3] : 1.0f;
    ok = dvz_drp2_stream_begin_command_encoder(stream, encoder_id) &&
         dvz_drp2_stream_begin_render_pass_region_clear(
             stream, render_pass_id, encoder_id, color_id, cr, cg, cb, ca, render->u.render.desc.x,
             render->u.render.desc.y, render->u.render.desc.width, render->u.render.desc.height,
             clear) &&
         dvz_drp2_stream_set_pipeline(stream, render_pass_id, pipe_id);
    if (ok && is_image && bg_id != 0)
        ok = dvz_drp2_stream_set_bind_group(stream, render_pass_id, 0, bg_id);
    if (ok && uses_mvp && mvp_bg_id != 0)
        ok = dvz_drp2_stream_set_bind_group(stream, render_pass_id, 0, mvp_bg_id);
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
 * Emit all plain render nodes in a runtime-mode FramePlan.
 *
 * @param emitter the persistent emitter
 * @param stream the DRP2 command stream
 * @param plan the FramePlan
 * @param fallback_vertex_buffer_ids uploaded vertex buffer ids used when visual ids are generic
 * @param fallback_vertex_buffer_count number of fallback vertex buffer ids
 * @param readback the optional readback copy node
 * @param cfg the emission config
 * @return whether all render commands were emitted
 */
static bool _emitter_emit_plain_renders(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const uint64_t* fallback_vertex_buffer_ids, uint32_t fallback_vertex_buffer_count,
    const DvzFramePlanNode* readback, const DvzFramePlanEmitConfig* cfg)
{
    ANN(emitter);
    ANN(stream);
    ANN(plan);

    uint32_t render_node_count = 0;
    uint32_t scene_render_node_count = 0;
    for (uint32_t i = 0; i < plan->count; i++)
    {
        const DvzFramePlanNode* render = &plan->nodes[i];
        if (render->type != DVZ_FRAME_PLAN_NODE_RENDER)
            continue;
        render_node_count++;
        if (render->u.render.visual_count > 0 &&
            cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
        {
            char probe[DVZ_SCENE_LABEL_SIZE];
            dvz_snprintf(probe, sizeof(probe), "%s_position", render->u.render.visuals[0]);
            if (_resource_lookup_id(&emitter->resources, probe) != 0)
                scene_render_node_count++;
        }
    }
    if (render_node_count > 0 && render_node_count == scene_render_node_count)
        return _emitter_emit_scene_figure_renders(emitter, stream, plan, readback, cfg);

    bool ok = true;
    uint32_t render_count = 0;
    SceneRenderStateCache scene_cache = {0};
    for (uint32_t i = 0; ok && i < plan->count; i++)
    {
        const DvzFramePlanNode* render = &plan->nodes[i];
        if (render->type != DVZ_FRAME_PLAN_NODE_RENDER)
            continue;

        uint64_t vertex_buffer_ids[DVZ_SCENE_MAX_NODE_RESOURCES] = {0};
        uint32_t vertex_buffer_count = 0;

        /* Scene render nodes (visual_count > 0 with named resources) skip flat resolution;
         * _emitter_emit_render dispatches to _emitter_emit_render_multi instead. */
        bool is_scene_node = false;
        if (render->u.render.visual_count > 0 &&
            cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
        {
            char probe[DVZ_SCENE_LABEL_SIZE];
            dvz_snprintf(probe, sizeof(probe), "%s_position", render->u.render.visuals[0]);
            is_scene_node = _resource_lookup_id(&emitter->resources, probe) != 0;
        }

        if (!is_scene_node)
        {
            ok = _emitter_resolve_render_vertex_buffers(
                emitter, render, vertex_buffer_ids, &vertex_buffer_count);
            if (!ok && fallback_vertex_buffer_ids != NULL && fallback_vertex_buffer_count > 0)
            {
                ok = true;
                vertex_buffer_count = fallback_vertex_buffer_count;
                for (uint32_t j = 0; j < vertex_buffer_count; j++)
                    vertex_buffer_ids[j] = fallback_vertex_buffer_ids[j];
            }
            scene_cache.pipeline_id = 0;
            scene_cache.bg_set0 = 0;
        }

        if (ok)
        {
            ok = _emitter_emit_render(
                emitter, stream, render, vertex_buffer_ids, vertex_buffer_count,
                render_count == 0 ? readback : NULL, render_count == 0, cfg,
                is_scene_node ? &scene_cache : NULL);
        }
        render_count++;
    }
    return ok && render_count > 0;
}



/**
 * Emit runtime-mode clear-only render commands.
 *
 * @param emitter the persistent emitter
 * @param stream the DRP2 command stream
 * @param readback the optional readback copy node
 * @param cfg the emission config
 * @return whether the commands were emitted
 */
static bool _emitter_emit_clear_only(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlanNode* clear_node,
    const DvzFramePlanNode* readback, bool clear, const DvzFramePlanEmitConfig* cfg)
{
    ANN(emitter);
    ANN(stream);
    ANN(clear_node);

    bool ok = true;
    bool is_new = false;

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
        if (is_new)
        {
            uint32_t usage =
                DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC;
            ok = ok && dvz_drp2_stream_create_texture_2d_usage(stream, color_id, 4, 4, usage);
        }
    }

    uint64_t rb_id = 0;
    if (readback != NULL)
    {
        rb_id = _obj_buffer_id(emitter, "_rb", readback->u.copy.byte_size, &is_new);
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

    float cr = cfg ? cfg->clear_color[0] : 0.0f;
    float cg = cfg ? cfg->clear_color[1] : 0.0f;
    float cb = cfg ? cfg->clear_color[2] : 0.0f;
    float ca = cfg ? cfg->clear_color[3] : 1.0f;
    ok = dvz_drp2_stream_begin_command_encoder(stream, encoder_id) &&
         dvz_drp2_stream_begin_render_pass_region_clear(
             stream, render_pass_id, encoder_id, color_id, cr, cg, cb, ca,
             clear_node->u.clear.desc.x, clear_node->u.clear.desc.y, clear_node->u.clear.desc.width,
             clear_node->u.clear.desc.height, clear) &&
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
        rb_id = _obj_buffer_id(emitter, "_rb", readback->u.copy.byte_size, &is_new);
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

    float cr2 = cfg ? cfg->clear_color[0] : 0.0f;
    float cg2 = cfg ? cfg->clear_color[1] : 0.0f;
    float cb2 = cfg ? cfg->clear_color[2] : 0.0f;
    float ca2 = cfg ? cfg->clear_color[3] : 1.0f;
    ok = dvz_drp2_stream_begin_command_encoder(stream, encoder_id) &&
         dvz_drp2_stream_begin_render_pass_clear(
             stream, render_pass_id, encoder_id, color_id, cr2, cg2, cb2, ca2) &&
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
        rb_id = _obj_buffer_id(emitter, "_rb", readback->u.copy.byte_size, &is_new);
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
         dvz_drp2_stream_begin_render_pass_clear(
             stream, render_pass_id, encoder_id, color_id,
             cfg ? cfg->clear_color[0] : 0.0f, cfg ? cfg->clear_color[1] : 0.0f,
             cfg ? cfg->clear_color[2] : 0.0f, cfg ? cfg->clear_color[3] : 1.0f) &&
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
    (void)caps;

    const DvzFramePlanNode* upload = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_UPLOAD);
    const DvzFramePlanNode* compute = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_COMPUTE);
    const DvzFramePlanNode* render = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_RENDER);
    const DvzFramePlanNode* clear = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_CLEAR);
    const DvzFramePlanNode* copy = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_COPY);
    const DvzFramePlanNode* readback = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_READBACK);
    bool clear_only = upload == NULL && compute == NULL && clear != NULL;
    if ((!clear_only && upload == NULL) || (clear_only ? clear == NULL : render == NULL))
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
            ok = texture_render ? _emit_texture_upload(&state, stream, &plan->nodes[i])
                                : _emit_upload(&state, stream, &plan->nodes[i]);
        }
    }
    ok = ok && (copy == NULL || _emit_readback_buffer(stream, copy)) &&
         (clear_only ? _emit_clear_only(stream, cfg)
                     : (compute_render
                            ? _emit_compute_assisted_render(stream, compute, render, &state, cfg)
                            : (texture_render
                                   ? _emit_texture_render(stream, render, state.first_texture_id, cfg)
                                   : _emit_render(
                                         stream, render, state.first_vertex_buffer_id, cfg)))) &&
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
    const DvzFramePlanNode* clear = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_CLEAR);
    const DvzFramePlanNode* copy = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_COPY);
    const DvzFramePlanNode* readback = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_READBACK);
    bool clear_only = upload == NULL && compute == NULL && clear != NULL;
    bool retained_render = upload == NULL && compute == NULL && render != NULL &&
                           render->u.render.visual_count > 0;

    if ((!clear_only && !retained_render && upload == NULL) || (clear_only ? clear == NULL : render == NULL))
    {
        _diagnostic(report, "runtime converter requires upload+render");
        return NULL;
    }
    bool texture_render = !clear_only && _render_uses_texture(render);
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
    uint64_t fallback_vertex_buffer_ids[DVZ_SCENE_MAX_NODE_RESOURCES] = {0};
    uint32_t fallback_vertex_buffer_count = 0;
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
                uint64_t uploaded_id = 0;
                ok = _emitter_emit_upload(
                    emitter, stream, &plan->nodes[i], &uploaded_id);
                if (ok && fallback_vertex_buffer_count < DVZ_SCENE_MAX_NODE_RESOURCES)
                    fallback_vertex_buffer_ids[fallback_vertex_buffer_count++] = uploaded_id;
            }
        }
    }

    ok = ok && (clear_only
                    ? _emitter_emit_clear_only(emitter, stream, clear, copy, true, cfg)
                    : compute != NULL
                    ? _emitter_emit_compute_assisted_render(emitter, stream, compute, copy, cfg)
                    : texture_render
                    ? _emitter_emit_texture_render(emitter, stream, texture_id, copy, cfg)
                    : _emitter_emit_plain_renders(
                          emitter, stream, plan, fallback_vertex_buffer_ids,
                          fallback_vertex_buffer_count, copy, cfg));
    if (!ok)
    {
        _diagnostic(report, "failed to emit runtime DRP2 stream");
        dvz_drp2_stream_destroy(stream);
        return NULL;
    }
    return stream;
}
