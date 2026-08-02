/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene FramePlan runtime technique targets */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <vulkan/vulkan_core.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_frame_plan_runtime_internal.h"
#include "_frame_plan_runtime_upload.h"
#include "_render_pass.h"
#include "_scene.h"
#include "_scene_common_bindings.h"
#include "_scene_resource_key.h"
#include "_scene_shader_abi.h"
#include "_shader_registry.h"
#include "_technique.h"
#include "_visual_pipeline.h"
#include "_vk_utils.h"
#include "datoviz/drp2.h"
#include "datoviz/drp2/stream.h"
#include "datoviz/scene.h"
#include "frame_plan/emit.h"
#include "frame_plan/frame_plan.h"
#include "frame_plan/internal.h"
#include "render_contract/render_contract.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

static const char PRESENTATION_VERTEX_GLSL[] =
    "#version 450\n"
    "layout(location = 0) out vec2 localUv;\n"
    "void main()\n"
    "{\n"
    "    vec2 p[3] = vec2[](vec2(-1, -1), vec2(3, -1), vec2(-1, 3));\n"
    "    vec2 position = p[gl_VertexIndex];\n"
    "    localUv = position * 0.5 + 0.5;\n"
    "    gl_Position = vec4(position, 0, 1);\n"
    "}\n";

static const char PRESENTATION_FRAGMENT_GLSL[] =
    "#version 450\n"
    "layout(set = 0, binding = 0) uniform texture2D sourceTex;\n"
    "layout(set = 0, binding = 1) uniform sampler samp;\n"
    "layout(location = 0) in vec2 localUv;\n"
    "layout(location = 0) out vec4 outColor;\n"
    "void main()\n"
    "{\n"
    "    ivec2 extent = textureSize(sampler2D(sourceTex, samp), 0);\n"
    "    ivec2 p = clamp(ivec2(localUv * vec2(extent)), ivec2(0), extent - ivec2(1));\n"
    "    outColor = texelFetch(sampler2D(sourceTex, samp), p, 0);\n"
    "}\n";

static const char PRESENTATION_ENCODE_FRAGMENT_GLSL[] =
    "#version 450\n"
    "layout(set = 0, binding = 0) uniform texture2D sourceTex;\n"
    "layout(set = 0, binding = 1) uniform sampler samp;\n"
    "layout(location = 0) in vec2 localUv;\n"
    "layout(location = 0) out vec4 outColor;\n"
    "vec3 linearToSrgb(vec3 value)\n"
    "{\n"
    "    vec3 clipped = clamp(value, 0.0, 1.0);\n"
    "    vec3 lo = 12.92 * clipped;\n"
    "    vec3 hi = 1.055 * pow(clipped, vec3(1.0 / 2.4)) - 0.055;\n"
    "    return mix(hi, lo, lessThanEqual(clipped, vec3(0.0031308)));\n"
    "}\n"
    "void main()\n"
    "{\n"
    "    ivec2 extent = textureSize(sampler2D(sourceTex, samp), 0);\n"
    "    ivec2 p = clamp(ivec2(localUv * vec2(extent)), ivec2(0), extent - ivec2(1));\n"
    "    vec4 color = texelFetch(sampler2D(sourceTex, samp), p, 0);\n"
    "    outColor = vec4(linearToSrgb(color.rgb), clamp(color.a, 0.0, 1.0));\n"
    "}\n";

static const char PRESENTATION_VERTEX_WGSL[] =
    "struct VertexOutput {\n"
    "    @builtin(position) position: vec4f,\n"
    "    @location(0) local_uv: vec2f,\n"
    "};\n"
    "@vertex fn main(@builtin(vertex_index) index: u32) -> VertexOutput {\n"
    "    var p = array<vec2f, 3>(vec2f(-1.0, -1.0), vec2f(3.0, -1.0), "
    "vec2f(-1.0, 3.0));\n"
    "    var out: VertexOutput;\n"
    "    out.position = vec4f(p[index], 0.0, 1.0);\n"
    "    out.local_uv = p[index] * 0.5 + vec2f(0.5);\n"
    "    return out;\n"
    "}\n";

static const char PRESENTATION_FRAGMENT_WGSL[] =
    "@group(0) @binding(0) var source_tex: texture_2d<f32>;\n"
    "@group(0) @binding(1) var samp: sampler;\n"
    "@fragment fn main(@location(0) local_uv: vec2f) -> @location(0) vec4f {\n"
    "    let extent = vec2i(textureDimensions(source_tex));\n"
    "    let p = clamp(vec2i(local_uv * vec2f(extent)), vec2i(0), extent - vec2i(1));\n"
    "    return textureLoad(source_tex, p, 0);\n"
    "}\n";

static const char PRESENTATION_ENCODE_FRAGMENT_WGSL[] =
    "@group(0) @binding(0) var source_tex: texture_2d<f32>;\n"
    "@group(0) @binding(1) var samp: sampler;\n"
    "fn linear_to_srgb(value: vec3f) -> vec3f {\n"
    "    let clipped = clamp(value, vec3f(0.0), vec3f(1.0));\n"
    "    let lo = vec3f(12.92) * clipped;\n"
    "    let hi = vec3f(1.055) * pow(clipped, vec3f(1.0 / 2.4)) - vec3f(0.055);\n"
    "    return select(hi, lo, clipped <= vec3f(0.0031308));\n"
    "}\n"
    "@fragment fn main(@location(0) local_uv: vec2f) -> @location(0) vec4f {\n"
    "    let extent = vec2i(textureDimensions(source_tex));\n"
    "    let p = clamp(vec2i(local_uv * vec2f(extent)), vec2i(0), extent - vec2i(1));\n"
    "    let color = textureLoad(source_tex, p, 0);\n"
    "    return vec4f(linear_to_srgb(color.rgb), clamp(color.a, 0.0, 1.0));\n"
    "}\n";

static bool _create_pipeline_with_layout(
    DvzDrp2CommandStream* stream, uint64_t id, uint64_t vertex_shader_module_id,
    uint64_t fragment_shader_module_id, uint64_t bind_group_layout_id)
{
    uint64_t layouts[1] = {bind_group_layout_id};
    DvzDrp2RenderPipelineDesc desc = dvz_drp2_render_pipeline_desc();
    desc.id = id;
    desc.vertex_shader_module_id = vertex_shader_module_id;
    desc.fragment_shader_module_id = fragment_shader_module_id;
    desc.bind_group_layout_count = 1;
    desc.bind_group_layout_ids = layouts;
    return dvz_drp2_stream_create_render_pipeline(stream, &desc);
}



/**
 * Resolve a typed auxiliary upload binding to its live runtime buffer.
 *
 * @param emitter persistent emitter resource registry
 * @param plan source FramePlan
 * @param pass typed graph pass
 * @param cfg optional scoped emission configuration
 * @param kind auxiliary resource kind
 * @return live buffer resource, or NULL when unavailable
 */
static ResourceId* _auxiliary_buffer_resource(
    DvzFramePlanEmitter* emitter, const DvzFramePlan* plan, const DvzFrameGraphPass* pass,
    const DvzFramePlanEmitConfig* cfg, DvzSceneAuxiliaryKind kind)
{
    ANN(emitter);
    ANN(plan);
    const DvzSceneResolvedPass* resolved = _graph_composition_pass(plan, pass);
    if (resolved == NULL)
        return NULL;
    for (uint32_t i = 0; i < resolved->auxiliary_binding_count; i++)
    {
        const DvzSceneAuxiliaryBinding* binding = &resolved->auxiliary_bindings[i];
        if (binding->kind != kind || binding->upload_node_index >= plan->count)
            continue;
        const DvzFramePlanNode* upload = &plan->nodes[binding->upload_node_index];
        if (upload->type != DVZ_FRAME_PLAN_NODE_UPLOAD)
            return NULL;
        char scoped_key[DVZ_SCENE_LABEL_SIZE];
        _runtime_scope_key(cfg, upload->u.upload.resource_id, scoped_key, sizeof(scoped_key));
        ResourceId* resource = _resource_find(&emitter->resources, scoped_key);
        return resource != NULL
                   ? resource
                   : _resource_find(&emitter->resources, upload->u.upload.resource_id);
    }
    return NULL;
}



static bool _presentation_needs_encode(const DvzFramePlanEmitConfig* cfg)
{
    return cfg != NULL && cfg->external_color_target &&
           cfg->color_pipeline != DVZ_COLOR_PIPELINE_LEGACY_SRGB_BLEND &&
           dvz_format_requires_final_srgb_encode((VkFormat)cfg->color_target_format);
}



/**
 * Prepare the explicit panel-local scene-color presentation boundary.
 *
 * @param emitter persistent emitter
 * @param stream destination DRP2 command stream
 * @param plan source FramePlan
 * @param render presentation render node
 * @param cfg optional frame-plan emit configuration
 * @param graph_targets live graph resource mappings
 * @param out output presentation runtime
 * @return whether the local source and fullscreen pipeline were prepared
 */
bool _emitter_prepare_presentation_targets(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanNode* render, const DvzFramePlanEmitConfig* cfg,
    SceneGraphRuntimeTargets* graph_targets, SceneWorkRuntime* out)
{
    ANN(emitter);
    ANN(stream);
    ANN(plan);
    ANN(render);
    ANN(graph_targets);
    ANN(out);

    const DvzFrameGraphPass* pass = _graph_pass_for_render(plan, render);
    if (pass == NULL || pass->read_count != 1 || pass->color_attachment_count != 1)
        return false;
    const DvzFrameGraphResource* source =
        _graph_resource_by_id(plan, pass->reads[0].resource_id);
    if (source == NULL || source->kind != DVZ_FRAME_GRAPH_RESOURCE_TEXTURE)
        return false;

    uint32_t width = 0;
    uint32_t height = 0;
    _emit_target_extent(cfg, &width, &height);
    uint64_t source_id = 0;
    bool ok = _graph_resolve_texture_2d(
        emitter, stream, plan, cfg, source, width, height,
        _render_pass_scene_color_target_format(cfg), &source_id);
    ok = ok && source_id != 0 &&
         _graph_runtime_targets_add(graph_targets, source->id, source_id);
    if (!ok)
        return false;

    bool is_new = false;
    uint64_t sampler_id = _obj_id(emitter, "_sampler_presentation", &is_new);
    if (sampler_id == 0)
        return false;
    if (is_new)
        ok = dvz_drp2_stream_create_sampler(stream, sampler_id);

    uint64_t bgl_id = _obj_id(emitter, "_bgl_presentation", &is_new);
    if (bgl_id == 0)
        return false;
    if (ok && is_new)
    {
        DvzDrp2BindGroupLayoutEntry entries[2] = {
            {
                .binding = 0,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                .access = DVZ_DRP2_BINDING_ACCESS_READ,
            },
            {
                .binding = 1,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
                .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                .access = DVZ_DRP2_BINDING_ACCESS_READ,
            },
        };
        ok = dvz_drp2_stream_create_bind_group_layout_entries(stream, bgl_id, 2, entries);
    }

    char bg_key[112];
    dvz_snprintf(
        bg_key, sizeof(bg_key), "_bg_presentation_%" PRIu64 "_%" PRIu64, source_id,
        sampler_id);
    uint64_t bg_id = _obj_id(emitter, bg_key, &is_new);
    if (bg_id == 0)
        return false;
    if (ok && is_new)
    {
        DvzDrp2BindGroupEntry entries[2] = {
            {
                .binding = 0,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
                .resource_id = source_id,
            },
            {
                .binding = 1,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_SAMPLER,
                .resource_id = sampler_id,
            },
        };
        ok = dvz_drp2_stream_create_bind_group_entries(stream, bg_id, bgl_id, 2, entries);
    }

    const bool encode = _presentation_needs_encode(cfg);
    const char* fmt = _shader_format_tag(cfg);
    const uint32_t target_format = cfg != NULL && cfg->color_target_format != 0
                                       ? cfg->color_target_format
                                       : DVZ_FORMAT_R8G8B8A8_UNORM;
    char vs_key[40];
    char fs_key[48];
    char pipe_key[80];
    dvz_snprintf(vs_key, sizeof(vs_key), "_vs_presentation%s", fmt);
    dvz_snprintf(
        fs_key, sizeof(fs_key), "_fs_presentation_%s%s", encode ? "encode" : "copy", fmt);
    dvz_snprintf(
        pipe_key, sizeof(pipe_key), "_pipe_presentation_%s%s_%u",
        encode ? "encode" : "copy", fmt, target_format);

    uint64_t vs_id = _obj_id(emitter, vs_key, &is_new);
    if (vs_id == 0)
        return false;
    if (ok && is_new)
        ok = _emit_shader(
            stream, vs_id, "VERTEX", PRESENTATION_VERTEX_WGSL, PRESENTATION_VERTEX_GLSL, cfg);

    uint64_t fs_id = _obj_id(emitter, fs_key, &is_new);
    if (fs_id == 0)
        return false;
    if (ok && is_new)
        ok = _emit_shader(
            stream, fs_id, "FRAGMENT",
            encode ? PRESENTATION_ENCODE_FRAGMENT_WGSL : PRESENTATION_FRAGMENT_WGSL,
            encode ? PRESENTATION_ENCODE_FRAGMENT_GLSL : PRESENTATION_FRAGMENT_GLSL, cfg);

    uint64_t pipeline_id = _obj_id(emitter, pipe_key, &is_new);
    if (pipeline_id == 0)
        return false;
    if (ok && is_new)
        ok = _create_pipeline_with_layout(stream, pipeline_id, vs_id, fs_id, bgl_id) &&
             dvz_drp2_stream_pipeline_set_color_target(stream, 0, target_format);

    *out = (SceneWorkRuntime){
        .render = render,
        .provider = DVZ_SCENE_WORK_PROVIDER_PRESENTATION,
        .color_id = source_id,
        .sampler_id = sampler_id,
        .pipeline_id = pipeline_id,
        .bind_group_layout_id = bgl_id,
        .bind_group_id = bg_id,
    };
    return ok;
}



/**
 * Return a compact fingerprint for an EDL sampled bind group dependency set.
 *
 * @param color_id scene color texture id
 * @param depth_id scene depth texture id
 * @param sampler_id sampler id
 * @param params_id EDL uniform buffer id
 * @return dependency fingerprint
 */
uint64_t _edl_bind_group_fingerprint(
    uint64_t color_id, uint64_t depth_id, uint64_t sampler_id, uint64_t params_id)
{
    uint64_t hash = UINT64_C(1469598103934665603);
    hash = (hash ^ color_id) * UINT64_C(1099511628211);
    hash = (hash ^ depth_id) * UINT64_C(1099511628211);
    hash = (hash ^ sampler_id) * UINT64_C(1099511628211);
    hash = (hash ^ params_id) * UINT64_C(1099511628211);
    return hash != 0 ? hash : UINT64_C(1);
}



/**
 * Prepare graph-declared EDL targets and resolve resources for one panel.
 *
 * @param emitter the persistent emitter
 * @param stream destination DRP2 command stream
 * @param plan the FramePlan
 * @param render the EDL resolve render node
 * @param cfg optional frame-plan emit configuration
 * @param out output EDL target ids
 * @return whether all declared targets and resolve resources were prepared
 */
bool _emitter_prepare_edl_targets(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanNode* render, const DvzFramePlanEmitConfig* cfg,
    SceneGraphRuntimeTargets* graph_targets, SceneWorkRuntime* out)
{
    ANN(emitter);
    ANN(stream);
    ANN(plan);
    ANN(render);
    ANN(graph_targets);
    ANN(out);

    const DvzFrameGraphPass* pass = _graph_pass_for_render(plan, render);
    if (pass == NULL || pass->read_count < 2)
        return false;

    uint32_t width = 0;
    uint32_t height = 0;
    _emit_target_extent(cfg, &width, &height);

    const DvzFrameGraphResource* color_resource =
        _graph_resource_by_id(plan, pass->reads[0].resource_id);
    const DvzFrameGraphResource* depth_resource =
        _graph_resource_by_id(plan, pass->reads[1].resource_id);
    if (color_resource == NULL || depth_resource == NULL)
        return false;

    uint64_t color_id = 0;
    uint64_t depth_id = 0;
    bool ok = _graph_resolve_texture_2d(
        emitter, stream, plan, cfg, color_resource, width, height, DVZ_FORMAT_R8G8B8A8_UNORM,
        &color_id);
    ok = ok && _graph_resolve_texture_2d(
                   emitter, stream, plan, cfg, depth_resource, width, height,
                   DVZ_FORMAT_D32_SFLOAT, &depth_id);
    ok = ok && _graph_runtime_targets_add(graph_targets, color_resource->id, color_id);
    ok = ok && _graph_runtime_targets_add(graph_targets, depth_resource->id, depth_id);
    if (!ok)
        return false;

    ResourceId* params =
        _auxiliary_buffer_resource(emitter, plan, pass, cfg, DVZ_SCENE_AUXILIARY_EDL_PARAMS);
    if (params == NULL || params->id == 0 || params->byte_size < sizeof(DvzSceneEdlUniform))
        return false;
    uint64_t params_id = params->id;

    bool is_new = false;
    uint64_t sampler_id = _obj_id(emitter, "_sampler_edl", &is_new);
    if (sampler_id == 0)
        return false;
    if (is_new)
        ok = ok && dvz_drp2_stream_create_sampler(stream, sampler_id);

    uint64_t bgl_id = _obj_id(emitter, "_bgl_edl_resolve", &is_new);
    if (bgl_id == 0)
        return false;
    if (ok && is_new)
    {
        DvzDrp2BindGroupLayoutEntry entries[4] = {
            {
                .binding = 0,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                .access = DVZ_DRP2_BINDING_ACCESS_READ,
            },
            {
                .binding = 1,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                .access = DVZ_DRP2_BINDING_ACCESS_READ,
            },
            {
                .binding = 2,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
                .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                .access = DVZ_DRP2_BINDING_ACCESS_READ,
            },
            {
                .binding = 3,
                .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
                .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                .access = DVZ_DRP2_BINDING_ACCESS_READ,
            },
        };
        ok = ok && dvz_drp2_stream_create_bind_group_layout_entries(stream, bgl_id, 4, entries);
    }

    char bg_key[96];
    dvz_snprintf(
        bg_key, sizeof(bg_key), "_bg_edl_%" PRIu64 "_%" PRIu64 "_%" PRIu64, color_id, depth_id,
        params_id);
    ResourceId* bg_resource = _resource_entry(&emitter->objects, bg_key, &is_new);
    if (bg_resource == NULL || bg_resource->id == 0)
        return false;
    uint64_t bg_id = bg_resource->id;
    uint64_t fingerprint = _edl_bind_group_fingerprint(color_id, depth_id, sampler_id, params_id);
    if (!is_new && bg_resource->byte_size != fingerprint)
        is_new = true;
    bg_resource->byte_size = fingerprint;
    if (ok && is_new)
    {
        uint64_t sampled_color_id = _graph_sampled_read_texture_id(pass, 0, 0, graph_targets, 0);
        uint64_t sampled_depth_id = _graph_sampled_read_texture_id(pass, 1, 0, graph_targets, 0);
        DvzDrp2BindGroupEntry entries[4] = {
            {
                .binding = 0,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
                .resource_id = sampled_color_id,
            },
            {
                .binding = 1,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
                .resource_id = sampled_depth_id,
            },
            {
                .binding = 2,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_SAMPLER,
                .resource_id = sampler_id,
            },
            {
                .binding = 3,
                .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_BUFFER,
                .resource_id = params_id,
                .offset = 0,
                .size = sizeof(DvzSceneEdlUniform),
            },
        };
        ok = ok && dvz_drp2_stream_create_bind_group_entries(stream, bg_id, bgl_id, 4, entries);
    }

    const char* fmt = _shader_format_tag(cfg);
    uint32_t final_format = _render_pass_scene_color_target_format(cfg);
    char vs_key[32];
    char fs_key[32];
    char pipe_key[64];
    dvz_snprintf(vs_key, sizeof(vs_key), "_vs_edl_resolve%s", fmt);
    dvz_snprintf(fs_key, sizeof(fs_key), "_fs_edl_resolve%s", fmt);
    dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe_edl_resolve%s_%u", fmt, final_format);

    uint64_t vs_id = _obj_id(emitter, vs_key, &is_new);
    if (vs_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && _emit_shader_spirv(
                       stream, vs_id, "VERTEX", "fullscreen_vert",
                       _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_EDL_RESOLVE, false), cfg);

    uint64_t fs_id = _obj_id(emitter, fs_key, &is_new);
    if (fs_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && _emit_shader_spirv(
                       stream, fs_id, "FRAGMENT", "edl_resolve_frag",
                       _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_EDL_RESOLVE, true), cfg);

    uint64_t pipeline_id = _obj_id(emitter, pipe_key, &is_new);
    if (pipeline_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && _create_pipeline_with_layout(stream, pipeline_id, vs_id, fs_id, bgl_id) &&
             dvz_drp2_stream_pipeline_set_color_target(stream, 0, final_format);
    out->render = render;
    out->provider = DVZ_SCENE_WORK_PROVIDER_EDL;
    out->pipeline_id = pipeline_id;
    out->bind_group_layout_id = bgl_id;
    out->bind_group_id = bg_id;
    return ok;
}



/**
 * Return a compact fingerprint for an SSAO sampled bind group dependency set.
 *
 * @param first_id first sampled texture id
 * @param second_id second sampled texture id
 * @param third_id third sampled texture id, or zero
 * @param sampler_id sampler id
 * @param params_id SSAO uniform buffer id, or zero for composite
 * @return dependency fingerprint
 */
uint64_t _ssao_bind_group_fingerprint(
    uint64_t first_id, uint64_t second_id, uint64_t third_id, uint64_t sampler_id,
    uint64_t params_id)
{
    uint64_t hash = UINT64_C(1469598103934665603);
    hash = (hash ^ first_id) * UINT64_C(1099511628211);
    hash = (hash ^ second_id) * UINT64_C(1099511628211);
    hash = (hash ^ third_id) * UINT64_C(1099511628211);
    hash = (hash ^ sampler_id) * UINT64_C(1099511628211);
    hash = (hash ^ params_id) * UINT64_C(1099511628211);
    return hash != 0 ? hash : UINT64_C(1);
}



/**
 * Prepare graph-declared SSAO targets and fullscreen resources for one panel.
 *
 * @param emitter the persistent emitter
 * @param stream destination DRP2 command stream
 * @param plan the FramePlan
 * @param render the SSAO render node
 * @param cfg optional frame-plan emit configuration
 * @param out output SSAO target ids
 * @return whether all declared targets and fullscreen resources were prepared
 */
bool _emitter_prepare_ssao_targets(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanNode* render, const DvzFramePlanEmitConfig* cfg,
    SceneGraphRuntimeTargets* graph_targets, SceneWorkRuntime* out)
{
    ANN(emitter);
    ANN(stream);
    ANN(plan);
    ANN(render);
    ANN(graph_targets);
    ANN(out);

    const DvzFrameGraphPass* pass = _graph_pass_for_render(plan, render);
    const DvzFrameGraphPass* blur_pass = _graph_pass_by_composition_provider(
        plan, render->u.render.panel_id, DVZ_SCENE_WORK_PROVIDER_SSAO_BLUR, 0);
    const DvzFrameGraphPass* composite_pass = _graph_pass_by_composition_provider(
        plan, render->u.render.panel_id, DVZ_SCENE_WORK_PROVIDER_AMBIENT_COMPOSITE, 0);
    if (pass == NULL || composite_pass == NULL || pass->read_count < 2 ||
        pass->color_attachment_count < 1 || composite_pass->read_count < 1)
        return false;

    uint32_t width = 0;
    uint32_t height = 0;
    _emit_target_extent(cfg, &width, &height);

    const DvzFrameGraphResource* normal_resource =
        _graph_resource_by_id(plan, pass->reads[0].resource_id);
    const DvzFrameGraphResource* depth_resource =
        _graph_resource_by_id(plan, pass->reads[1].resource_id);
    const DvzFrameGraphResource* occlusion_resource =
        _graph_resource_by_id(plan, pass->color_attachments[0].resource_id);
    if (normal_resource == NULL || depth_resource == NULL || occlusion_resource == NULL)
        return false;
    const DvzFrameGraphResource* blur_resource =
        blur_pass != NULL && blur_pass->color_attachment_count > 0
            ? _graph_resource_by_id(plan, blur_pass->color_attachments[0].resource_id)
            : NULL;
    if (blur_pass != NULL && (blur_pass->read_count < 3 || blur_resource == NULL))
        return false;

    bool ok = _graph_resolve_texture_2d(
        emitter, stream, plan, cfg, normal_resource, width, height, DVZ_FORMAT_R16G16B16A16_SFLOAT,
        &out->normal_id);
    ok = ok && _graph_resolve_texture_2d(
                   emitter, stream, plan, cfg, depth_resource, width, height,
                   DVZ_FORMAT_D32_SFLOAT, &out->depth_id);
    ok = ok && _graph_resolve_texture_2d(
                   emitter, stream, plan, cfg, occlusion_resource, width, height,
                   DVZ_FORMAT_R8_UNORM, &out->occlusion_id);
    ok = ok && _graph_runtime_targets_add(graph_targets, normal_resource->id, out->normal_id);
    ok = ok && _graph_runtime_targets_add(graph_targets, depth_resource->id, out->depth_id);
    ok =
        ok && _graph_runtime_targets_add(graph_targets, occlusion_resource->id, out->occlusion_id);
    if (blur_resource != NULL)
    {
        ok = ok && _graph_resolve_texture_2d(
                       emitter, stream, plan, cfg, blur_resource, width, height,
                       DVZ_FORMAT_R8_UNORM, &out->blur_id);
        ok = ok && _graph_runtime_targets_add(graph_targets, blur_resource->id, out->blur_id);
    }
    if (!ok)
        return false;
    out->composite_input_id = blur_resource != NULL ? out->blur_id : out->occlusion_id;

    ResourceId* params =
        _auxiliary_buffer_resource(emitter, plan, pass, cfg, DVZ_SCENE_AUXILIARY_SSAO_PARAMS);
    if (params == NULL || params->id == 0 || params->byte_size < sizeof(DvzSceneSsaoUniform))
        return false;
    out->params_id = params->id;

    bool is_new = false;
    out->sampler_id = _obj_id(emitter, "_sampler_ssao", &is_new);
    if (out->sampler_id == 0)
        return false;
    if (is_new)
        ok = ok && dvz_drp2_stream_create_sampler(stream, out->sampler_id);

    out->ssao_bgl_id = _obj_id(emitter, "_bgl_ssao", &is_new);
    if (out->ssao_bgl_id == 0)
        return false;
    if (ok && is_new)
    {
        DvzDrp2BindGroupLayoutEntry entries[4] = {
            {
                .binding = 0,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                .access = DVZ_DRP2_BINDING_ACCESS_READ,
            },
            {
                .binding = 1,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                .access = DVZ_DRP2_BINDING_ACCESS_READ,
            },
            {
                .binding = 2,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
                .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                .access = DVZ_DRP2_BINDING_ACCESS_READ,
            },
            {
                .binding = 3,
                .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
                .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                .access = DVZ_DRP2_BINDING_ACCESS_READ,
            },
        };
        ok = ok && dvz_drp2_stream_create_bind_group_layout_entries(
                       stream, out->ssao_bgl_id, 4, entries);
    }

    char bg_key[112];
    dvz_snprintf(
        bg_key, sizeof(bg_key), "_bg_ssao_%" PRIu64 "_%" PRIu64 "_%" PRIu64, out->normal_id,
        out->depth_id, out->params_id);
    ResourceId* bg_resource = _resource_entry(&emitter->objects, bg_key, &is_new);
    if (bg_resource == NULL || bg_resource->id == 0)
        return false;
    out->ssao_bg_id = bg_resource->id;
    uint64_t fingerprint = _ssao_bind_group_fingerprint(
        out->normal_id, out->depth_id, 0, out->sampler_id, out->params_id);
    if (!is_new && bg_resource->byte_size != fingerprint)
        is_new = true;
    bg_resource->byte_size = fingerprint;
    if (ok && is_new)
    {
        uint64_t normal_id =
            _graph_sampled_read_texture_id(pass, 0, 0, graph_targets, out->normal_id);
        uint64_t depth_id =
            _graph_sampled_read_texture_id(pass, 1, 0, graph_targets, out->depth_id);
        DvzDrp2BindGroupEntry entries[4] = {
            {
                .binding = 0,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
                .resource_id = normal_id,
            },
            {
                .binding = 1,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
                .resource_id = depth_id,
            },
            {
                .binding = 2,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_SAMPLER,
                .resource_id = out->sampler_id,
            },
            {
                .binding = 3,
                .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_BUFFER,
                .resource_id = out->params_id,
                .offset = 0,
                .size = sizeof(DvzSceneSsaoUniform),
            },
        };
        ok = ok && dvz_drp2_stream_create_bind_group_entries(
                       stream, out->ssao_bg_id, out->ssao_bgl_id, 4, entries);
    }

    if (blur_pass != NULL)
    {
        out->blur_bgl_id = _obj_id(emitter, "_bgl_ssao_blur", &is_new);
        if (out->blur_bgl_id == 0)
            return false;
        if (ok && is_new)
        {
            DvzDrp2BindGroupLayoutEntry entries[5] = {
                {
                    .binding = 0,
                    .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                    .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                    .access = DVZ_DRP2_BINDING_ACCESS_READ,
                },
                {
                    .binding = 1,
                    .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                    .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                    .access = DVZ_DRP2_BINDING_ACCESS_READ,
                },
                {
                    .binding = 2,
                    .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                    .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                    .access = DVZ_DRP2_BINDING_ACCESS_READ,
                },
                {
                    .binding = 3,
                    .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
                    .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                    .access = DVZ_DRP2_BINDING_ACCESS_READ,
                },
                {
                    .binding = 4,
                    .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
                    .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                    .access = DVZ_DRP2_BINDING_ACCESS_READ,
                },
            };
            ok = ok && dvz_drp2_stream_create_bind_group_layout_entries(
                           stream, out->blur_bgl_id, 5, entries);
        }

        dvz_snprintf(
            bg_key, sizeof(bg_key), "_bg_ssao_blur_%" PRIu64 "_%" PRIu64 "_%" PRIu64 "_%" PRIu64,
            out->occlusion_id, out->normal_id, out->depth_id, out->params_id);
        ResourceId* blur_bg = _resource_entry(&emitter->objects, bg_key, &is_new);
        if (blur_bg == NULL || blur_bg->id == 0)
            return false;
        out->blur_bg_id = blur_bg->id;
        fingerprint = _ssao_bind_group_fingerprint(
            out->occlusion_id, out->normal_id, out->depth_id, out->sampler_id, out->params_id);
        if (!is_new && blur_bg->byte_size != fingerprint)
            is_new = true;
        blur_bg->byte_size = fingerprint;
        if (ok && is_new)
        {
            uint64_t occlusion_id =
                _graph_sampled_read_texture_id(blur_pass, 0, 0, graph_targets, out->occlusion_id);
            uint64_t normal_id =
                _graph_sampled_read_texture_id(blur_pass, 1, 0, graph_targets, out->normal_id);
            uint64_t depth_id =
                _graph_sampled_read_texture_id(blur_pass, 2, 0, graph_targets, out->depth_id);
            DvzDrp2BindGroupEntry entries[5] = {
                {
                    .binding = 0,
                    .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                    .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
                    .resource_id = occlusion_id,
                },
                {
                    .binding = 1,
                    .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                    .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
                    .resource_id = normal_id,
                },
                {
                    .binding = 2,
                    .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                    .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
                    .resource_id = depth_id,
                },
                {
                    .binding = 3,
                    .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
                    .resource_kind = DVZ_DRP2_BINDING_RESOURCE_SAMPLER,
                    .resource_id = out->sampler_id,
                },
                {
                    .binding = 4,
                    .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
                    .resource_kind = DVZ_DRP2_BINDING_RESOURCE_BUFFER,
                    .resource_id = out->params_id,
                    .offset = 0,
                    .size = sizeof(DvzSceneSsaoUniform),
                },
            };
            ok = ok && dvz_drp2_stream_create_bind_group_entries(
                           stream, out->blur_bg_id, out->blur_bgl_id, 5, entries);
        }
    }

    out->composite_bgl_id = _obj_id(emitter, "_bgl_ssao_composite", &is_new);
    if (out->composite_bgl_id == 0)
        return false;
    if (ok && is_new)
    {
        DvzDrp2BindGroupLayoutEntry entries[3] = {
            {
                .binding = 0,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                .access = DVZ_DRP2_BINDING_ACCESS_READ,
            },
            {
                .binding = 1,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
                .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                .access = DVZ_DRP2_BINDING_ACCESS_READ,
            },
            {
                .binding = 2,
                .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
                .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                .access = DVZ_DRP2_BINDING_ACCESS_READ,
            },
        };
        ok = ok && dvz_drp2_stream_create_bind_group_layout_entries(
                       stream, out->composite_bgl_id, 3, entries);
    }

    dvz_snprintf(
        bg_key, sizeof(bg_key), "_bg_ssao_composite_%" PRIu64 "_%" PRIu64, out->composite_input_id,
        out->params_id);
    ResourceId* composite_bg = _resource_entry(&emitter->objects, bg_key, &is_new);
    if (composite_bg == NULL || composite_bg->id == 0)
        return false;
    out->composite_bg_id = composite_bg->id;
    fingerprint = _ssao_bind_group_fingerprint(
        out->composite_input_id, 0, 0, out->sampler_id, out->params_id);
    if (!is_new && composite_bg->byte_size != fingerprint)
        is_new = true;
    composite_bg->byte_size = fingerprint;
    if (ok && is_new)
    {
        uint64_t composite_input_id = _graph_sampled_read_texture_id(
            composite_pass, 0, 0, graph_targets, out->composite_input_id);
        DvzDrp2BindGroupEntry entries[3] = {
            {
                .binding = 0,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
                .resource_id = composite_input_id,
            },
            {
                .binding = 1,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_SAMPLER,
                .resource_id = out->sampler_id,
            },
            {
                .binding = 2,
                .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_BUFFER,
                .resource_id = out->params_id,
                .offset = 0,
                .size = sizeof(DvzSceneSsaoUniform),
            },
        };
        ok = ok && dvz_drp2_stream_create_bind_group_entries(
                       stream, out->composite_bg_id, out->composite_bgl_id, 3, entries);
    }

    const char* fmt = _shader_format_tag(cfg);
    char vs_key[32];
    char fs_key[32];
    char pipe_key[40];
    dvz_snprintf(vs_key, sizeof(vs_key), "_vs_ssao%s", fmt);
    dvz_snprintf(fs_key, sizeof(fs_key), "_fs_ssao%s", fmt);
    dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe_ssao%s", fmt);

    uint64_t vs_id = _obj_id(emitter, vs_key, &is_new);
    if (vs_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && _emit_shader_spirv(
                       stream, vs_id, "VERTEX", "fullscreen_vert",
                       _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_SSAO, false), cfg);

    uint64_t fs_id = _obj_id(emitter, fs_key, &is_new);
    if (fs_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && _emit_shader_spirv(
                       stream, fs_id, "FRAGMENT", "ssao_frag",
                       _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_SSAO, true), cfg);

    out->ssao_pipeline_id = _obj_id(emitter, pipe_key, &is_new);
    if (out->ssao_pipeline_id == 0)
        return false;
    if (ok && is_new)
        ok = ok &&
             _create_pipeline_with_layout(
                 stream, out->ssao_pipeline_id, vs_id, fs_id, out->ssao_bgl_id) &&
             dvz_drp2_stream_pipeline_set_color_target(stream, 0, DVZ_FORMAT_R8_UNORM);

    if (blur_pass != NULL)
    {
        dvz_snprintf(vs_key, sizeof(vs_key), "_vs_ssao_blur%s", fmt);
        dvz_snprintf(fs_key, sizeof(fs_key), "_fs_ssao_blur%s", fmt);
        dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe_ssao_blur%s", fmt);
        vs_id = _obj_id(emitter, vs_key, &is_new);
        if (vs_id == 0)
            return false;
        if (ok && is_new)
            ok = ok && _emit_shader_spirv(
                           stream, vs_id, "VERTEX", "fullscreen_vert",
                           _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_SSAO_BLUR, false), cfg);

        fs_id = _obj_id(emitter, fs_key, &is_new);
        if (fs_id == 0)
            return false;
        if (ok && is_new)
            ok = ok && _emit_shader_spirv(
                           stream, fs_id, "FRAGMENT", "ssao_blur_frag",
                           _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_SSAO_BLUR, true), cfg);

        out->blur_pipeline_id = _obj_id(emitter, pipe_key, &is_new);
        if (out->blur_pipeline_id == 0)
            return false;
        if (ok && is_new)
            ok = ok &&
                 _create_pipeline_with_layout(
                     stream, out->blur_pipeline_id, vs_id, fs_id, out->blur_bgl_id) &&
                 dvz_drp2_stream_pipeline_set_color_target(stream, 0, DVZ_FORMAT_R8_UNORM);
    }

    dvz_snprintf(vs_key, sizeof(vs_key), "_vs_ssao_comp%s", fmt);
    dvz_snprintf(fs_key, sizeof(fs_key), "_fs_ssao_comp%s", fmt);
    uint32_t final_format = _render_pass_scene_color_target_format(cfg);
    dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe_ssao_comp%s_%u", fmt, final_format);
    vs_id = _obj_id(emitter, vs_key, &is_new);
    if (vs_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && _emit_shader_spirv(
                       stream, vs_id, "VERTEX", "fullscreen_vert",
                       _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_SSAO_COMPOSITE, false), cfg);

    fs_id = _obj_id(emitter, fs_key, &is_new);
    if (fs_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && _emit_shader_spirv(
                       stream, fs_id, "FRAGMENT", "ssao_composite_frag",
                       _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_SSAO_COMPOSITE, true), cfg);

    out->composite_pipeline_id = _obj_id(emitter, pipe_key, &is_new);
    if (out->composite_pipeline_id == 0)
        return false;
    if (ok && is_new)
    {
        ok = ok &&
             _create_pipeline_with_layout(
                 stream, out->composite_pipeline_id, vs_id, fs_id, out->composite_bgl_id) &&
             dvz_drp2_stream_pipeline_set_color_target(stream, 0, final_format) &&
             dvz_drp2_stream_pipeline_set_color_blend(
                 stream, 0, DVZ_BLEND_FACTOR_SRC_ALPHA, DVZ_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                 DVZ_BLEND_OP_ADD, DVZ_BLEND_FACTOR_ONE, DVZ_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                 DVZ_BLEND_OP_ADD,
                 DVZ_MASK_COLOR_R | DVZ_MASK_COLOR_G | DVZ_MASK_COLOR_B | DVZ_MASK_COLOR_A);
    }
    out->render = render;
    out->provider = DVZ_SCENE_WORK_PROVIDER_SSAO;
    return ok;
}



/**
 * Return a compact fingerprint for a WBOIT resolve bind group dependency set.
 *
 * @param accum_id accumulation texture id.
 * @param weight_id weight texture id.
 * @param sampler_id sampler id.
 * @return dependency fingerprint.
 */
uint64_t _wboit_bind_group_fingerprint(uint64_t accum_id, uint64_t weight_id, uint64_t sampler_id)
{
    uint64_t hash = UINT64_C(1469598103934665603);
    hash = (hash ^ accum_id) * UINT64_C(1099511628211);
    hash = (hash ^ weight_id) * UINT64_C(1099511628211);
    hash = (hash ^ sampler_id) * UINT64_C(1099511628211);
    return hash != 0 ? hash : UINT64_C(1);
}



/**
 * Prepare WBOIT intermediate targets and resolve pipeline resources for one panel.
 *
 * @param emitter the persistent emitter.
 * @param stream destination DRP2 command stream.
 * @param render transparent accumulation render node.
 * @param color_id final color target id.
 * @param cfg optional frame-plan emit configuration.
 * @param out output WBOIT target ids.
 * @return whether all resources were prepared.
 */
bool _emitter_prepare_wboit_targets(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanNode* render, uint64_t color_id, const DvzFramePlanEmitConfig* cfg,
    SceneGraphRuntimeTargets* graph_targets, SceneWorkRuntime* out)
{
    ANN(emitter);
    ANN(stream);
    ANN(render);
    ANN(graph_targets);
    ANN(out);

    uint32_t width = 0;
    uint32_t height = 0;
    _emit_target_extent(cfg, &width, &height);

    bool ok = true;
    bool is_new = false;
    out->color_id = color_id;

    char accum_key[DVZ_SCENE_LABEL_SIZE];
    char weight_key[DVZ_SCENE_LABEL_SIZE];
    char scoped_accum_key[DVZ_SCENE_LABEL_SIZE];
    char scoped_weight_key[DVZ_SCENE_LABEL_SIZE];
    dvz_snprintf(accum_key, sizeof(accum_key), "_wboit_accum_%s", render->u.render.panel_id);
    dvz_snprintf(weight_key, sizeof(weight_key), "_wboit_weight_%s", render->u.render.panel_id);
    _runtime_scope_key(cfg, accum_key, scoped_accum_key, sizeof(scoped_accum_key));
    _runtime_scope_key(cfg, weight_key, scoped_weight_key, sizeof(scoped_weight_key));

    const DvzFrameGraphPass* graph_pass = _graph_pass_for_render(plan, render);
    const DvzFrameGraphResource* accum_resource = NULL;
    const DvzFrameGraphResource* weight_resource = NULL;
    const DvzFrameGraphResource* depth_resource = NULL;
    if (graph_pass != NULL && graph_pass->color_attachment_count >= 2)
    {
        accum_resource = _graph_resource_by_id(plan, graph_pass->color_attachments[0].resource_id);
        weight_resource =
            _graph_resource_by_id(plan, graph_pass->color_attachments[1].resource_id);
        if (graph_pass->has_depth_attachment)
            depth_resource = _graph_resource_by_id(plan, graph_pass->depth_attachment.resource_id);
    }

    uint32_t fallback_usage =
        DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING;

    ok = ok && (accum_resource != NULL
                    ? _graph_resolve_texture_2d(
                          emitter, stream, plan, cfg, accum_resource, width, height,
                          DVZ_FORMAT_R16G16B16A16_SFLOAT, &out->accum_id)
                    : _runtime_resolve_texture_2d(
                          emitter, stream, scoped_accum_key, width, height,
                          DVZ_FORMAT_R16G16B16A16_SFLOAT, fallback_usage, 1, &out->accum_id));
    ok = ok && (weight_resource != NULL
                    ? _graph_resolve_texture_2d(
                          emitter, stream, plan, cfg, weight_resource, width, height,
                          DVZ_FORMAT_R16_SFLOAT, &out->weight_id)
                    : _runtime_resolve_texture_2d(
                          emitter, stream, scoped_weight_key, width, height, DVZ_FORMAT_R16_SFLOAT,
                          fallback_usage, 1, &out->weight_id));
    if (!ok)
        return false;
    if (depth_resource != NULL)
    {
        ok = _graph_resolve_texture_2d(
            emitter, stream, plan, cfg, depth_resource, width, height, DVZ_FORMAT_D32_SFLOAT,
            &out->depth_id);
    }
    if (!ok)
        return false;
    ok = ok && (accum_resource == NULL ||
                _graph_runtime_targets_add(graph_targets, accum_resource->id, out->accum_id));
    ok = ok && (weight_resource == NULL ||
                _graph_runtime_targets_add(graph_targets, weight_resource->id, out->weight_id));
    ok = ok && (depth_resource == NULL ||
                _graph_runtime_targets_add(graph_targets, depth_resource->id, out->depth_id));
    if (!ok)
        return false;

    out->sampler_id = _obj_id(emitter, "_sampler_wboit", &is_new);
    if (out->sampler_id == 0)
        return false;
    if (is_new)
        ok = ok && dvz_drp2_stream_create_sampler(stream, out->sampler_id);

    out->resolve_bgl_id = _obj_id(emitter, "_bgl_wboit_resolve", &is_new);
    if (out->resolve_bgl_id == 0)
        return false;
    if (ok && is_new)
    {
        DvzDrp2BindGroupLayoutEntry entries[3] = {
            {
                .binding = 0,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                .access = DVZ_DRP2_BINDING_ACCESS_READ,
            },
            {
                .binding = 1,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                .access = DVZ_DRP2_BINDING_ACCESS_READ,
            },
            {
                .binding = 2,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
                .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                .access = DVZ_DRP2_BINDING_ACCESS_READ,
            },
        };
        ok = ok && dvz_drp2_stream_create_bind_group_layout_entries(
                       stream, out->resolve_bgl_id, 3, entries);
    }

    char bg_key[96];
    dvz_snprintf(
        bg_key, sizeof(bg_key), "_bg_wboit_%" PRIu64 "_%" PRIu64, out->accum_id, out->weight_id);
    ResourceId* bg_resource = _resource_entry(&emitter->objects, bg_key, &is_new);
    if (bg_resource == NULL || bg_resource->id == 0)
        return false;
    out->resolve_bg_id = bg_resource->id;

    uint64_t bg_fingerprint =
        _wboit_bind_group_fingerprint(out->accum_id, out->weight_id, out->sampler_id);
    if (!is_new && bg_resource->byte_size != bg_fingerprint)
        is_new = true;
    bg_resource->byte_size = bg_fingerprint;
    if (ok && is_new)
    {
        const DvzFrameGraphPass* resolve_graph_pass = _graph_pass_by_composition_provider(
            plan, render->u.render.panel_id, DVZ_SCENE_WORK_PROVIDER_WBOIT_RESOLVE, 0);
        uint64_t accum_id = _graph_sampled_read_texture_id(
            resolve_graph_pass, 0, out->color_id, graph_targets, out->accum_id);
        uint64_t weight_id = _graph_sampled_read_texture_id(
            resolve_graph_pass, 1, out->color_id, graph_targets, out->weight_id);
        DvzDrp2BindGroupEntry entries[3] = {
            {
                .binding = 0,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
                .resource_id = accum_id,
            },
            {
                .binding = 1,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
                .resource_id = weight_id,
            },
            {
                .binding = 2,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_SAMPLER,
                .resource_id = out->sampler_id,
            },
        };
        ok = ok && dvz_drp2_stream_create_bind_group_entries(
                       stream, out->resolve_bg_id, out->resolve_bgl_id, 3, entries);
    }

    const char* fmt = _shader_format_tag(cfg);
    uint32_t final_format = _render_pass_scene_color_target_format(cfg);
    char vs_key[32];
    char fs_key[32];
    char pipe_key[64];
    dvz_snprintf(vs_key, sizeof(vs_key), "_vs_wboit_resolve%s", fmt);
    dvz_snprintf(fs_key, sizeof(fs_key), "_fs_wboit_resolve%s", fmt);
    dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe_wboit_resolve%s_%u", fmt, final_format);

    uint64_t vs_id = _obj_id(emitter, vs_key, &is_new);
    if (vs_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && _emit_shader(
                       stream, vs_id, "VERTEX", NULL,
                       _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_WBOIT_RESOLVE, false), cfg);

    uint64_t fs_id = _obj_id(emitter, fs_key, &is_new);
    if (fs_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && _emit_shader(
                       stream, fs_id, "FRAGMENT", NULL,
                       _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_WBOIT_RESOLVE, true), cfg);

    out->resolve_pipeline_id = _obj_id(emitter, pipe_key, &is_new);
    if (out->resolve_pipeline_id == 0)
        return false;
    if (ok && is_new)
    {
        ok = ok &&
             _create_pipeline_with_layout(
                 stream, out->resolve_pipeline_id, vs_id, fs_id, out->resolve_bgl_id) &&
             dvz_drp2_stream_pipeline_set_color_target(stream, 0, final_format) &&
             dvz_drp2_stream_pipeline_set_color_blend(
                 stream, 0, DVZ_BLEND_FACTOR_SRC_ALPHA, DVZ_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                 DVZ_BLEND_OP_ADD, DVZ_BLEND_FACTOR_ONE, DVZ_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                 DVZ_BLEND_OP_ADD,
                 DVZ_MASK_COLOR_R | DVZ_MASK_COLOR_G | DVZ_MASK_COLOR_B | DVZ_MASK_COLOR_A);
    }
    out->render = render;
    out->provider = DVZ_SCENE_WORK_PROVIDER_WBOIT_ACCUMULATION;
    return ok;
}



/**
 * Return a compact fingerprint for a depth-peel sampled bind group dependency set.
 *
 * @param texture_ids sampled texture ids.
 * @param texture_count sampled texture count.
 * @param sampler_id sampler id.
 * @return dependency fingerprint.
 */
uint64_t _depth_peel_bind_group_fingerprint(
    const uint64_t* texture_ids, uint32_t texture_count, uint64_t sampler_id)
{
    ANN(texture_ids);
    uint64_t hash = UINT64_C(1469598103934665603);
    for (uint32_t i = 0; i < texture_count; i++)
        hash = (hash ^ texture_ids[i]) * UINT64_C(1099511628211);
    hash = (hash ^ sampler_id) * UINT64_C(1099511628211);
    return hash != 0 ? hash : UINT64_C(1);
}



/**
 * Resolve one sampled bind group for a depth-peeling graph pass.
 *
 * @param emitter the persistent emitter.
 * @param stream destination DRP2 command stream.
 * @param pass graph pass whose reads are sampled.
 * @param targets runtime graph target map.
 * @param key bind group cache key.
 * @param bgl_id sampled bind group layout id.
 * @param sampler_id sampler id.
 * @param out_bg_id output bind group id.
 * @return whether the bind group is available.
 */
bool _depth_peel_resolve_sampled_bind_group(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFrameGraphPass* pass, const SceneGraphRuntimeTargets* targets, uint32_t binding_set,
    const char* key, uint64_t bgl_id, uint64_t sampler_id, uint64_t* out_bg_id)
{
    ANN(emitter);
    ANN(stream);
    ANN(plan);
    ANN(pass);
    ANN(targets);
    ANN(key);
    ANN(out_bg_id);
    if (!pass->has_composition_pass)
        return false;

    const DvzPanelCompositionSnapshot* snapshot =
        _frame_plan_composition_get(plan, pass->panel_id);
    if (snapshot == NULL)
        return false;
    const DvzSceneResolvedPass* resolved = NULL;
    for (uint32_t i = 0; i < snapshot->pass_count; i++)
    {
        if (snapshot->passes[i].id.value == pass->composition_pass_id.value)
        {
            resolved = &snapshot->passes[i];
            break;
        }
    }
    if (resolved == NULL)
        return false;

    uint64_t texture_ids[DVZ_DRP2_MAX_BINDINGS] = {0};
    uint32_t texture_count = 0;
    for (uint32_t i = 0; i < resolved->binding_count; i++)
    {
        const DvzSceneWorkBinding* binding = &resolved->bindings[i];
        if (binding->usage != DVZ_SCENE_WORK_BINDING_SAMPLED || binding->set != binding_set)
            continue;
        if (binding->binding >= DVZ_DRP2_MAX_BINDINGS - 1 || texture_ids[binding->binding] != 0)
            return false;
        const DvzSceneGraphRealization* realization = _frame_plan_realization_get(
            plan, pass->panel_id, binding->ref_kind, binding->product_id, binding->scratch_id);
        if (realization == NULL || realization->graph_resource_index >= plan->graph_resource_count)
            return false;
        const DvzFrameGraphResource* resource =
            &plan->graph_resources[realization->graph_resource_index];
        texture_ids[binding->binding] = _graph_runtime_targets_get(targets, resource->id);
        if (texture_ids[binding->binding] == 0)
            return false;
        if (binding->binding + 1 > texture_count)
            texture_count = binding->binding + 1;
    }
    if (texture_count == 0 || texture_count + 1 > DVZ_DRP2_MAX_BINDINGS)
        return false;
    for (uint32_t i = 0; i < texture_count; i++)
        if (texture_ids[i] == 0)
            return false;

    bool is_new = false;
    ResourceId* resource = _resource_entry(&emitter->objects, key, &is_new);
    if (resource == NULL || resource->id == 0)
        return false;
    uint64_t bg_id = resource->id;
    uint64_t fingerprint =
        _depth_peel_bind_group_fingerprint(texture_ids, texture_count, sampler_id);
    if (!is_new && resource->byte_size != fingerprint)
        is_new = true;
    resource->byte_size = fingerprint;
    if (is_new)
    {
        DvzDrp2BindGroupEntry entries[DVZ_DRP2_MAX_BINDINGS] = {0};
        for (uint32_t i = 0; i < texture_count; i++)
        {
            entries[i].binding = i;
            entries[i].binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE;
            entries[i].resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE;
            entries[i].resource_id = texture_ids[i];
        }
        entries[texture_count].binding = texture_count;
        entries[texture_count].binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER;
        entries[texture_count].resource_kind = DVZ_DRP2_BINDING_RESOURCE_SAMPLER;
        entries[texture_count].resource_id = sampler_id;
        if (!dvz_drp2_stream_create_bind_group_entries(
                stream, bg_id, bgl_id, texture_count + 1, entries))
            return false;
    }

    *out_bg_id = bg_id;
    return true;
}


/**
 * Prepare depth-peeling graph resources and composite state for one panel.
 *
 * @param emitter the persistent emitter.
 * @param stream destination DRP2 command stream.
 * @param plan the FramePlan.
 * @param render depth-peel init render node.
 * @param color_id final color target id.
 * @param cfg optional frame-plan emit configuration.
 * @param out output depth-peeling target ids.
 * @return whether all resources were prepared.
 */
bool _emitter_prepare_depth_peel_targets(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanNode* render, uint64_t color_id, const DvzFramePlanEmitConfig* cfg,
    SceneGraphRuntimeTargets* graph_targets, SceneWorkRuntime* out)
{
    ANN(emitter);
    ANN(stream);
    ANN(plan);
    ANN(render);
    ANN(graph_targets);
    ANN(out);

    uint32_t width = 0;
    uint32_t height = 0;
    _emit_target_extent(cfg, &width, &height);

    bool ok = true;
    bool is_new = false;
    out->color_id = color_id;

    for (uint32_t i = 0; ok && i < dvz_frame_plan_graph_resource_count(plan); i++)
    {
        const DvzFrameGraphResource* resource = dvz_frame_plan_graph_resource_get(plan, i);
        if (resource == NULL || resource->kind == DVZ_FRAME_GRAPH_RESOURCE_EXTERNAL_TARGET)
            continue;
        size_t panel_id_len = strlen(render->u.render.panel_id);
        if (strncmp(resource->id, render->u.render.panel_id, panel_id_len) != 0 ||
            resource->id[panel_id_len] != '.')
            continue;

        uint64_t texture_id = 0;
        uint32_t format = resource->format;
        if ((resource->usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_DEPTH_ATTACHMENT) != 0)
            format = DVZ_FORMAT_D32_SFLOAT;
        if (format == 0)
            format = DVZ_FORMAT_R16G16B16A16_SFLOAT;
        ok = _graph_resolve_texture_2d(
            emitter, stream, plan, cfg, resource, width, height, format, &texture_id);
        ok = ok && _graph_runtime_targets_add(graph_targets, resource->id, texture_id);
        if (ok && (resource->usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_DEPTH_ATTACHMENT) != 0)
            out->depth_id = texture_id;
    }
    if (!ok)
        return false;

    out->sampler_id = _obj_id(emitter, "_sampler_depth_peel", &is_new);
    if (out->sampler_id == 0)
        return false;
    if (is_new)
        ok = ok && dvz_drp2_stream_create_sampler(stream, out->sampler_id);

    out->composite_bgl_id = _obj_id(emitter, "_bgl_depth_peel_composite", &is_new);
    if (out->composite_bgl_id == 0)
        return false;
    if (ok && is_new)
    {
        DvzDrp2BindGroupLayoutEntry entries[3] = {
            {
                .binding = 0,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                .access = DVZ_DRP2_BINDING_ACCESS_READ,
            },
            {
                .binding = 1,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                .access = DVZ_DRP2_BINDING_ACCESS_READ,
            },
            {
                .binding = 2,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
                .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                .access = DVZ_DRP2_BINDING_ACCESS_READ,
            },
        };
        ok = ok && dvz_drp2_stream_create_bind_group_layout_entries(
                       stream, out->composite_bgl_id, 3, entries);
    }

    out->iter_bgl_id = _obj_id(emitter, "_bgl_depth_peel_iter", &is_new);
    if (out->iter_bgl_id == 0)
        return false;
    if (ok && is_new)
    {
        DvzDrp2BindGroupLayoutEntry entries[2] = {
            {
                .binding = 0,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                .access = DVZ_DRP2_BINDING_ACCESS_READ,
            },
            {
                .binding = 1,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
                .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                .access = DVZ_DRP2_BINDING_ACCESS_READ,
            },
        };
        ok = ok && dvz_drp2_stream_create_bind_group_layout_entries(
                       stream, out->iter_bgl_id, 2, entries);
    }

    uint64_t dummy_bgl_id = _obj_id(emitter, "_bgl_unused_set", &is_new);
    if (dummy_bgl_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && _create_dummy_bind_group_layout(stream, dummy_bgl_id);
    out->dummy_bg_id = _obj_id(emitter, "_bg_unused_set", &is_new);
    if (out->dummy_bg_id == 0)
        return false;
    if (ok && is_new)
    {
        DvzDrp2BindGroupEntry entry = {
            .binding = 0,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
            .resource_kind = DVZ_DRP2_BINDING_RESOURCE_SAMPLER,
            .resource_id = out->sampler_id,
        };
        ok = ok && dvz_drp2_stream_create_bind_group_entries(
                       stream, out->dummy_bg_id, dummy_bgl_id, 1, &entry);
    }

    const DvzFrameGraphPass* composite_pass = _graph_pass_by_composition_provider(
        plan, render->u.render.panel_id, DVZ_SCENE_WORK_PROVIDER_DEPTH_PEEL_COMPOSITE, 0);
    ok = ok && composite_pass != NULL;
    if (ok)
    {
        char composite_bg_key[DVZ_SCENE_LABEL_SIZE];
        _runtime_scope_key(
            cfg, "_bg_depth_peel_composite", composite_bg_key, sizeof(composite_bg_key));
        ok = ok && _depth_peel_resolve_sampled_bind_group(
                       emitter, stream, plan, composite_pass, graph_targets, 0, composite_bg_key,
                       out->composite_bgl_id, out->sampler_id, &out->composite_bg_id);
    }

    for (uint32_t iter_idx = 0; ok && iter_idx < DVZ_SCENE_DEPTH_PEEL_ITERATIONS; iter_idx++)
    {
        const DvzFrameGraphPass* iter_pass = _graph_pass_by_composition_provider(
            plan, render->u.render.panel_id, DVZ_SCENE_WORK_PROVIDER_DEPTH_PEEL_ITERATION,
            iter_idx);
        ok = ok && iter_pass != NULL;
        if (ok)
        {
            char iter_bg_base_key[DVZ_SCENE_LABEL_SIZE];
            char iter_bg_key[DVZ_SCENE_LABEL_SIZE];
            dvz_snprintf(
                iter_bg_base_key, sizeof(iter_bg_base_key), "_bg_depth_peel_iter_%" PRIu32,
                iter_idx);
            _runtime_scope_key(cfg, iter_bg_base_key, iter_bg_key, sizeof(iter_bg_key));
            ok = _depth_peel_resolve_sampled_bind_group(
                emitter, stream, plan, iter_pass, graph_targets, DVZ_SCENE_DEPTH_PEEL_BIND_SET,
                iter_bg_key, out->iter_bgl_id, out->sampler_id, &out->iter_bg_ids[iter_idx]);
        }
    }

    const char* fmt = _shader_format_tag(cfg);
    uint32_t final_format = _render_pass_scene_color_target_format(cfg);
    char vs_key[40];
    char fs_key[40];
    char pipe_key[64];
    dvz_snprintf(vs_key, sizeof(vs_key), "_vs_depth_peel_comp%s", fmt);
    dvz_snprintf(fs_key, sizeof(fs_key), "_fs_depth_peel_comp%s", fmt);
    dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe_depth_peel_comp%s_%u", fmt, final_format);

    uint64_t vs_id = _obj_id(emitter, vs_key, &is_new);
    if (vs_id == 0)
        return false;
    if (ok && is_new)
        ok = ok &&
             _emit_shader_spirv(
                 stream, vs_id, "VERTEX", "fullscreen_vert",
                 _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_DEPTH_PEEL_COMPOSITE, false), cfg);

    uint64_t fs_id = _obj_id(emitter, fs_key, &is_new);
    if (fs_id == 0)
        return false;
    if (ok && is_new)
        ok = ok &&
             _emit_shader_spirv(
                 stream, fs_id, "FRAGMENT", "depth_peel_composite_frag",
                 _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_DEPTH_PEEL_COMPOSITE, true), cfg);

    out->composite_pipeline_id = _obj_id(emitter, pipe_key, &is_new);
    if (out->composite_pipeline_id == 0)
        return false;
    if (ok && is_new)
    {
        ok = ok &&
             _create_pipeline_with_layout(
                 stream, out->composite_pipeline_id, vs_id, fs_id, out->composite_bgl_id) &&
             dvz_drp2_stream_pipeline_set_color_target(stream, 0, final_format) &&
             dvz_drp2_stream_pipeline_set_color_blend(
                 stream, 0, DVZ_BLEND_FACTOR_SRC_ALPHA, DVZ_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                 DVZ_BLEND_OP_ADD, DVZ_BLEND_FACTOR_ONE, DVZ_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                 DVZ_BLEND_OP_ADD,
                 DVZ_MASK_COLOR_R | DVZ_MASK_COLOR_G | DVZ_MASK_COLOR_B | DVZ_MASK_COLOR_A);
    }
    out->render = render;
    out->provider = DVZ_SCENE_WORK_PROVIDER_DEPTH_PEEL_INIT;
    return ok;
}
