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
 * Prepare the product-specific multisample surface-record resolve.
 *
 * @param emitter persistent emitter
 * @param stream destination DRP2 command stream
 * @param plan source FramePlan
 * @param render surface-resolve render node
 * @param cfg optional frame-plan emit configuration
 * @param graph_targets live graph resource mappings
 * @param out output surface-resolve runtime
 * @return whether all sampled inputs, outputs, and pipeline state were prepared
 */
bool _emitter_prepare_surface_resolve_targets(
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
    if (pass == NULL || pass->read_count != 3 || pass->color_attachment_count != 3)
        return false;

    const uint32_t fallback_formats[3] = {
        DVZ_FORMAT_R32_SFLOAT,
        DVZ_FORMAT_R16G16B16A16_SFLOAT,
        DVZ_FORMAT_R8_UNORM,
    };
    uint32_t width = 0;
    uint32_t height = 0;
    _emit_target_extent(cfg, &width, &height);

    uint64_t sampled_ids[3] = {0};
    bool ok = true;
    for (uint32_t i = 0; ok && i < 3; i++)
    {
        const DvzFrameGraphResource* resource =
            _graph_resource_by_id(plan, pass->reads[i].resource_id);
        if (resource == NULL || _graph_resource_lowered_sample_count(emitter, resource) <= 1)
            return false;
        ok = _graph_resolve_texture_2d(
            emitter, stream, plan, cfg, resource, width, height, fallback_formats[i],
            &sampled_ids[i]);
        ok = ok && _graph_runtime_targets_add(graph_targets, resource->id, sampled_ids[i]);
    }
    ok =
        ok && _graph_prepare_render_color_targets(emitter, stream, plan, pass, cfg, graph_targets);
    if (!ok)
        return false;

    bool is_new = false;
    uint64_t bgl_id = _obj_id(emitter, "_bgl_surface_resolve", &is_new);
    if (bgl_id == 0)
        return false;
    if (is_new)
    {
        DvzDrp2BindGroupLayoutEntry entries[3] = {0};
        for (uint32_t i = 0; i < 3; i++)
        {
            entries[i].binding = i;
            entries[i].binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE;
            entries[i].visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT;
            entries[i].access = DVZ_DRP2_BINDING_ACCESS_READ;
        }
        ok = dvz_drp2_stream_create_bind_group_layout_entries(stream, bgl_id, 3, entries);
    }

    char bg_key[128] = {0};
    dvz_snprintf(
        bg_key, sizeof(bg_key), "_bg_surface_resolve_%" PRIu64 "_%" PRIu64 "_%" PRIu64,
        sampled_ids[0], sampled_ids[1], sampled_ids[2]);
    uint64_t bg_id = _obj_id(emitter, bg_key, &is_new);
    if (bg_id == 0)
        return false;
    if (ok && is_new)
    {
        DvzDrp2BindGroupEntry entries[3] = {0};
        for (uint32_t i = 0; i < 3; i++)
        {
            entries[i].binding = i;
            entries[i].binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE;
            entries[i].resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE;
            entries[i].resource_id = sampled_ids[i];
        }
        ok = dvz_drp2_stream_create_bind_group_entries(stream, bg_id, bgl_id, 3, entries);
    }

    const char* fmt = _shader_format_tag(cfg);
    char vs_key[40] = {0};
    char fs_key[40] = {0};
    char pipe_key[56] = {0};
    dvz_snprintf(vs_key, sizeof(vs_key), "_vs_surface_resolve%s", fmt);
    dvz_snprintf(fs_key, sizeof(fs_key), "_fs_surface_resolve%s", fmt);
    dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe_surface_resolve%s", fmt);

    uint64_t vs_id = _obj_id(emitter, vs_key, &is_new);
    if (vs_id == 0)
        return false;
    if (ok && is_new)
        ok = _emit_shader_spirv(
            stream, vs_id, "VERTEX", "fullscreen_vert",
            _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_SURFACE_RESOLVE, false), cfg);

    uint64_t fs_id = _obj_id(emitter, fs_key, &is_new);
    if (fs_id == 0)
        return false;
    if (ok && is_new)
        ok = _emit_shader_spirv(
            stream, fs_id, "FRAGMENT", "surface_resolve_frag",
            _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_SURFACE_RESOLVE, true), cfg);

    uint64_t pipeline_id = _obj_id(emitter, pipe_key, &is_new);
    if (pipeline_id == 0)
        return false;
    if (ok && is_new)
    {
        ok = _create_pipeline_with_layout(stream, pipeline_id, vs_id, fs_id, bgl_id);
        for (uint32_t i = 0; ok && i < 3; i++)
            ok = dvz_drp2_stream_pipeline_set_color_target(stream, i, fallback_formats[i]);
    }

    *out = (SceneWorkRuntime){
        .render = render,
        .provider = DVZ_SCENE_WORK_PROVIDER_SURFACE_RESOLVE,
        .depth_id = sampled_ids[0],
        .normal_id = sampled_ids[1],
        .coverage_id = sampled_ids[2],
        .pipeline_id = pipeline_id,
        .bind_group_layout_id = bgl_id,
        .bind_group_id = bg_id,
    };
    return ok;
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
    const DvzFrameGraphResource* source = _graph_resource_by_id(plan, pass->reads[0].resource_id);
    if (source == NULL || source->kind != DVZ_FRAME_GRAPH_RESOURCE_TEXTURE)
        return false;

    uint32_t width = 0;
    uint32_t height = 0;
    _emit_target_extent(cfg, &width, &height);
    uint64_t source_id = 0;
    bool ok = _graph_resolve_texture_2d(
        emitter, stream, plan, cfg, source, width, height,
        _render_pass_scene_color_target_format(cfg), &source_id);
    ok = ok && source_id != 0 && _graph_runtime_targets_add(graph_targets, source->id, source_id);
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
        bg_key, sizeof(bg_key), "_bg_presentation_%" PRIu64 "_%" PRIu64, source_id, sampler_id);
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
    dvz_snprintf(fs_key, sizeof(fs_key), "_fs_presentation_%s%s", encode ? "encode" : "copy", fmt);
    dvz_snprintf(
        pipe_key, sizeof(pipe_key), "_pipe_presentation_%s%s_%u", encode ? "encode" : "copy", fmt,
        target_format);

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
        emitter, stream, plan, cfg, color_resource, width, height,
        _render_pass_scene_color_target_format(cfg), &color_id);
    ok = ok && _graph_resolve_texture_2d(
                   emitter, stream, plan, cfg, depth_resource, width, height,
                   DVZ_FORMAT_R32_SFLOAT, &depth_id);
    ok = ok && _graph_runtime_targets_add(graph_targets, color_resource->id, color_id);
    ok = ok && _graph_runtime_targets_add(graph_targets, depth_resource->id, depth_id);
    ok =
        ok && _graph_prepare_render_color_targets(emitter, stream, plan, pass, cfg, graph_targets);
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
        DvzDrp2BindGroupEntry entries[4] = {
            {
                .binding = 0,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
                .resource_id = color_id,
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
 * Return a compact fingerprint for an GTAO sampled bind group dependency set.
 *
 * @param first_id first sampled texture id
 * @param second_id second sampled texture id
 * @param third_id third sampled texture id, or zero
 * @param fourth_id fourth sampled texture id, or zero
 * @param sampler_id sampler id
 * @param params_id GTAO uniform buffer id, or zero for visibility presentation
 * @return dependency fingerprint
 */
uint64_t _gtao_bind_group_fingerprint(
    uint64_t first_id, uint64_t second_id, uint64_t third_id, uint64_t fourth_id,
    uint64_t sampler_id, uint64_t params_id)
{
    uint64_t hash = UINT64_C(1469598103934665603);
    hash = (hash ^ first_id) * UINT64_C(1099511628211);
    hash = (hash ^ second_id) * UINT64_C(1099511628211);
    hash = (hash ^ third_id) * UINT64_C(1099511628211);
    hash = (hash ^ fourth_id) * UINT64_C(1099511628211);
    hash = (hash ^ sampler_id) * UINT64_C(1099511628211);
    hash = (hash ^ params_id) * UINT64_C(1099511628211);
    return hash != 0 ? hash : UINT64_C(1);
}



/**
 * Prepare graph-declared GTAO targets and fullscreen resources for one panel.
 *
 * @param emitter the persistent emitter
 * @param stream destination DRP2 command stream
 * @param plan the FramePlan
 * @param render the GTAO render node
 * @param cfg optional frame-plan emit configuration
 * @param out output GTAO target ids
 * @return whether all declared targets and fullscreen resources were prepared
 */
bool _emitter_prepare_gtao_targets(
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
    const DvzFrameGraphPass* denoise_x_pass = _graph_pass_by_composition_provider(
        plan, render->u.render.panel_id, DVZ_SCENE_WORK_PROVIDER_GTAO_DENOISE, 0);
    const DvzFrameGraphPass* denoise_y_pass = _graph_pass_by_composition_provider(
        plan, render->u.render.panel_id, DVZ_SCENE_WORK_PROVIDER_GTAO_DENOISE, 1);
    const DvzFrameGraphPass* composite_pass = _graph_pass_by_composition_provider(
        plan, render->u.render.panel_id, DVZ_SCENE_WORK_PROVIDER_GTAO_VISIBILITY_PRESENTATION, 0);
    if (pass == NULL || pass->read_count != 3 || pass->color_attachment_count != 1 ||
        ((denoise_x_pass == NULL) != (denoise_y_pass == NULL)) ||
        (composite_pass != NULL && composite_pass->read_count != 1))
        return false;

    uint32_t width = 0;
    uint32_t height = 0;
    _emit_target_extent(cfg, &width, &height);

    const DvzFrameGraphResource* normal_resource =
        _graph_resource_by_id(plan, pass->reads[0].resource_id);
    const DvzFrameGraphResource* depth_resource =
        _graph_resource_by_id(plan, pass->reads[1].resource_id);
    const DvzFrameGraphResource* coverage_resource =
        _graph_resource_by_id(plan, pass->reads[2].resource_id);
    const DvzFrameGraphResource* raw_visibility_resource =
        _graph_resource_by_id(plan, pass->color_attachments[0].resource_id);
    if (normal_resource == NULL || depth_resource == NULL || coverage_resource == NULL ||
        raw_visibility_resource == NULL)
        return false;
    const DvzFrameGraphResource* denoise_resource =
        denoise_x_pass != NULL && denoise_x_pass->color_attachment_count == 1
            ? _graph_resource_by_id(plan, denoise_x_pass->color_attachments[0].resource_id)
            : NULL;
    const DvzFrameGraphResource* denoised_visibility_resource =
        denoise_y_pass != NULL && denoise_y_pass->color_attachment_count == 1
            ? _graph_resource_by_id(plan, denoise_y_pass->color_attachments[0].resource_id)
            : NULL;
    if (denoise_x_pass != NULL &&
        (denoise_x_pass->read_count != 4 || denoise_y_pass->read_count != 4 ||
         denoise_resource == NULL || denoised_visibility_resource == NULL))
        return false;

    bool ok = _graph_resolve_texture_2d(
        emitter, stream, plan, cfg, normal_resource, width, height, DVZ_FORMAT_R16G16B16A16_SFLOAT,
        &out->normal_id);
    ok = ok && _graph_resolve_texture_2d(
                   emitter, stream, plan, cfg, depth_resource, width, height,
                   DVZ_FORMAT_R32_SFLOAT, &out->depth_id);
    ok = ok && _graph_resolve_texture_2d(
                   emitter, stream, plan, cfg, coverage_resource, width, height,
                   DVZ_FORMAT_R8_UNORM, &out->coverage_id);
    ok = ok && _graph_resolve_texture_2d(
                   emitter, stream, plan, cfg, raw_visibility_resource, width, height,
                   DVZ_FORMAT_R32_SFLOAT, &out->raw_visibility_id);
    ok = ok && _graph_runtime_targets_add(graph_targets, normal_resource->id, out->normal_id);
    ok = ok && _graph_runtime_targets_add(graph_targets, depth_resource->id, out->depth_id);
    ok = ok && _graph_runtime_targets_add(graph_targets, coverage_resource->id, out->coverage_id);
    ok = ok && _graph_runtime_targets_add(
                   graph_targets, raw_visibility_resource->id, out->raw_visibility_id);
    if (denoise_resource != NULL)
    {
        ok = ok && _graph_resolve_texture_2d(
                       emitter, stream, plan, cfg, denoise_resource, width, height,
                       DVZ_FORMAT_R32_SFLOAT, &out->denoise_intermediate_id);
        ok = ok && _graph_runtime_targets_add(
                       graph_targets, denoise_resource->id, out->denoise_intermediate_id);
    }
    if (denoised_visibility_resource != NULL)
    {
        ok = ok && _graph_resolve_texture_2d(
                       emitter, stream, plan, cfg, denoised_visibility_resource, width, height,
                       DVZ_FORMAT_R32_SFLOAT, &out->denoised_visibility_id);
        ok = ok &&
             _graph_runtime_targets_add(
                 graph_targets, denoised_visibility_resource->id, out->denoised_visibility_id);
    }
    if (!ok)
        return false;
    out->visibility_present_input_id = denoised_visibility_resource != NULL
                                           ? out->denoised_visibility_id
                                           : out->raw_visibility_id;

    ResourceId* params =
        _auxiliary_buffer_resource(emitter, plan, pass, cfg, DVZ_SCENE_AUXILIARY_GTAO_PARAMS);
    if (params == NULL || params->id == 0 || params->byte_size < sizeof(DvzSceneAoUniform))
        return false;
    out->params_id = params->id;

    bool is_new = false;
    out->sampler_id = _obj_id(emitter, "_sampler_gtao", &is_new);
    if (out->sampler_id == 0)
        return false;
    if (is_new)
        ok = ok && dvz_drp2_stream_create_sampler(stream, out->sampler_id);

    uint64_t dummy_bgl_id = _obj_id(emitter, "_bgl_unused_set", &is_new);
    if (dummy_bgl_id == 0)
        return false;
    if (ok && is_new)
        ok = _create_dummy_bind_group_layout(stream, dummy_bgl_id);
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
        ok = dvz_drp2_stream_create_bind_group_entries(
            stream, out->dummy_bg_id, dummy_bgl_id, 1, &entry);
    }

    out->ao_bgl_id = _obj_id(emitter, "_bgl_gtao", &is_new);
    if (out->ao_bgl_id == 0)
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
        ok = ok &&
             dvz_drp2_stream_create_bind_group_layout_entries(stream, out->ao_bgl_id, 5, entries);
    }

    char bg_key[160];
    dvz_snprintf(
        bg_key, sizeof(bg_key), "_bg_gtao_%" PRIu64 "_%" PRIu64 "_%" PRIu64 "_%" PRIu64,
        out->normal_id, out->depth_id, out->coverage_id, out->params_id);
    ResourceId* bg_resource = _resource_entry(&emitter->objects, bg_key, &is_new);
    if (bg_resource == NULL || bg_resource->id == 0)
        return false;
    out->ao_bg_id = bg_resource->id;
    uint64_t fingerprint = _gtao_bind_group_fingerprint(
        out->normal_id, out->depth_id, out->coverage_id, 0, out->sampler_id, out->params_id);
    if (!is_new && bg_resource->byte_size != fingerprint)
        is_new = true;
    bg_resource->byte_size = fingerprint;
    if (ok && is_new)
    {
        uint64_t normal_id =
            _graph_sampled_read_texture_id(pass, 0, 0, graph_targets, out->normal_id);
        uint64_t depth_id =
            _graph_sampled_read_texture_id(pass, 1, 0, graph_targets, out->depth_id);
        uint64_t coverage_id =
            _graph_sampled_read_texture_id(pass, 2, 0, graph_targets, out->coverage_id);
        DvzDrp2BindGroupEntry entries[5] = {
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
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
                .resource_id = coverage_id,
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
                .size = sizeof(DvzSceneAoUniform),
            },
        };
        ok = ok && dvz_drp2_stream_create_bind_group_entries(
                       stream, out->ao_bg_id, out->ao_bgl_id, 5, entries);
    }

    if (denoise_x_pass != NULL)
    {
        out->denoise_bgl_id = _obj_id(emitter, "_bgl_gtao_denoise", &is_new);
        if (out->denoise_bgl_id == 0)
            return false;
        if (ok && is_new)
        {
            DvzDrp2BindGroupLayoutEntry entries[6] = {
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
                    .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                    .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                    .access = DVZ_DRP2_BINDING_ACCESS_READ,
                },
                {
                    .binding = 4,
                    .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
                    .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                    .access = DVZ_DRP2_BINDING_ACCESS_READ,
                },
                {
                    .binding = 5,
                    .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
                    .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                    .access = DVZ_DRP2_BINDING_ACCESS_READ,
                },
            };
            ok = ok && dvz_drp2_stream_create_bind_group_layout_entries(
                           stream, out->denoise_bgl_id, 6, entries);
        }

        dvz_snprintf(
            bg_key, sizeof(bg_key),
            "_bg_gtao_denoise_%" PRIu64 "_%" PRIu64 "_%" PRIu64 "_%" PRIu64 "_%" PRIu64,
            out->raw_visibility_id, out->normal_id, out->depth_id, out->coverage_id,
            out->params_id);
        ResourceId* blur_bg = _resource_entry(&emitter->objects, bg_key, &is_new);
        if (blur_bg == NULL || blur_bg->id == 0)
            return false;
        out->denoise_bg_ids[0] = blur_bg->id;
        fingerprint = _gtao_bind_group_fingerprint(
            out->raw_visibility_id, out->normal_id, out->depth_id, out->coverage_id,
            out->sampler_id, out->params_id);
        if (!is_new && blur_bg->byte_size != fingerprint)
            is_new = true;
        blur_bg->byte_size = fingerprint;
        if (ok && is_new)
        {
            uint64_t raw_visibility_id = _graph_sampled_read_texture_id(
                denoise_x_pass, 0, 0, graph_targets, out->raw_visibility_id);
            uint64_t normal_id = _graph_sampled_read_texture_id(
                denoise_x_pass, 1, 0, graph_targets, out->normal_id);
            uint64_t depth_id =
                _graph_sampled_read_texture_id(denoise_x_pass, 2, 0, graph_targets, out->depth_id);
            uint64_t coverage_id = _graph_sampled_read_texture_id(
                denoise_x_pass, 3, 0, graph_targets, out->coverage_id);
            DvzDrp2BindGroupEntry entries[6] = {
                {
                    .binding = 0,
                    .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                    .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
                    .resource_id = raw_visibility_id,
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
                    .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                    .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
                    .resource_id = coverage_id,
                },
                {
                    .binding = 4,
                    .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
                    .resource_kind = DVZ_DRP2_BINDING_RESOURCE_SAMPLER,
                    .resource_id = out->sampler_id,
                },
                {
                    .binding = 5,
                    .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
                    .resource_kind = DVZ_DRP2_BINDING_RESOURCE_BUFFER,
                    .resource_id = out->params_id,
                    .offset = 0,
                    .size = sizeof(DvzSceneAoUniform),
                },
            };
            ok = ok && dvz_drp2_stream_create_bind_group_entries(
                           stream, out->denoise_bg_ids[0], out->denoise_bgl_id, 6, entries);
        }

        dvz_snprintf(
            bg_key, sizeof(bg_key),
            "_bg_gtao_denoise_y_%" PRIu64 "_%" PRIu64 "_%" PRIu64 "_%" PRIu64 "_%" PRIu64,
            out->denoise_intermediate_id, out->normal_id, out->depth_id, out->coverage_id,
            out->params_id);
        ResourceId* blur_y_bg = _resource_entry(&emitter->objects, bg_key, &is_new);
        if (blur_y_bg == NULL || blur_y_bg->id == 0)
            return false;
        out->denoise_bg_ids[1] = blur_y_bg->id;
        fingerprint = _gtao_bind_group_fingerprint(
            out->denoise_intermediate_id, out->normal_id, out->depth_id, out->coverage_id,
            out->sampler_id, out->params_id);
        if (!is_new && blur_y_bg->byte_size != fingerprint)
            is_new = true;
        blur_y_bg->byte_size = fingerprint;
        if (ok && is_new)
        {
            const uint64_t sampled_ids[4] = {
                _graph_sampled_read_texture_id(
                    denoise_y_pass, 0, 0, graph_targets, out->denoise_intermediate_id),
                _graph_sampled_read_texture_id(
                    denoise_y_pass, 1, 0, graph_targets, out->normal_id),
                _graph_sampled_read_texture_id(denoise_y_pass, 2, 0, graph_targets, out->depth_id),
                _graph_sampled_read_texture_id(
                    denoise_y_pass, 3, 0, graph_targets, out->coverage_id),
            };
            DvzDrp2BindGroupEntry entries[6] = {
                {.binding = 0,
                 .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                 .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
                 .resource_id = sampled_ids[0]},
                {.binding = 1,
                 .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                 .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
                 .resource_id = sampled_ids[1]},
                {.binding = 2,
                 .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                 .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
                 .resource_id = sampled_ids[2]},
                {.binding = 3,
                 .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                 .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
                 .resource_id = sampled_ids[3]},
                {.binding = 4,
                 .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
                 .resource_kind = DVZ_DRP2_BINDING_RESOURCE_SAMPLER,
                 .resource_id = out->sampler_id},
                {.binding = 5,
                 .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
                 .resource_kind = DVZ_DRP2_BINDING_RESOURCE_BUFFER,
                 .resource_id = out->params_id,
                 .offset = 0,
                 .size = sizeof(DvzSceneAoUniform)},
            };
            ok = ok && dvz_drp2_stream_create_bind_group_entries(
                           stream, out->denoise_bg_ids[1], out->denoise_bgl_id, 6, entries);
        }
    }

    out->ambient_bgl_id = _obj_id(emitter, "_bgl_ambient_visibility", &is_new);
    if (out->ambient_bgl_id == 0)
        return false;
    if (ok && is_new)
    {
        DvzDrp2BindGroupLayoutEntry entries[2] = {
            {.binding = 0,
             .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
             .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
             .access = DVZ_DRP2_BINDING_ACCESS_READ},
            {.binding = 1,
             .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
             .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
             .access = DVZ_DRP2_BINDING_ACCESS_READ},
        };
        ok = dvz_drp2_stream_create_bind_group_layout_entries(
            stream, out->ambient_bgl_id, 2, entries);
    }
    dvz_snprintf(
        bg_key, sizeof(bg_key), "_bg_ambient_visibility_%" PRIu64,
        out->visibility_present_input_id);
    ResourceId* ambient_bg = _resource_entry(&emitter->objects, bg_key, &is_new);
    if (ambient_bg == NULL || ambient_bg->id == 0)
        return false;
    out->ambient_bg_id = ambient_bg->id;
    fingerprint = _gtao_bind_group_fingerprint(
        out->visibility_present_input_id, 0, 0, 0, out->sampler_id, 0);
    if (!is_new && ambient_bg->byte_size != fingerprint)
        is_new = true;
    ambient_bg->byte_size = fingerprint;
    if (ok && is_new)
    {
        DvzDrp2BindGroupEntry entries[2] = {
            {.binding = 0,
             .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
             .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
             .resource_id = out->visibility_present_input_id},
            {.binding = 1,
             .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
             .resource_kind = DVZ_DRP2_BINDING_RESOURCE_SAMPLER,
             .resource_id = out->sampler_id},
        };
        ok = dvz_drp2_stream_create_bind_group_entries(
            stream, out->ambient_bg_id, out->ambient_bgl_id, 2, entries);
    }

    if (composite_pass != NULL)
    {
        out->composite_bgl_id = out->ambient_bgl_id;
        out->composite_bg_id = out->ambient_bg_id;
    }

    const char* fmt = _shader_format_tag(cfg);
    char vs_key[32];
    char fs_key[32];
    char pipe_key[40];
    dvz_snprintf(vs_key, sizeof(vs_key), "_vs_gtao%s", fmt);
    dvz_snprintf(fs_key, sizeof(fs_key), "_fs_gtao%s", fmt);
    dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe_gtao%s", fmt);

    uint64_t vs_id = _obj_id(emitter, vs_key, &is_new);
    if (vs_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && _emit_shader_spirv(
                       stream, vs_id, "VERTEX", "fullscreen_vert",
                       _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_GTAO, false), cfg);

    uint64_t fs_id = _obj_id(emitter, fs_key, &is_new);
    if (fs_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && _emit_shader_spirv(
                       stream, fs_id, "FRAGMENT", "gtao_frag",
                       _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_GTAO, true), cfg);

    out->ao_pipeline_id = _obj_id(emitter, pipe_key, &is_new);
    if (out->ao_pipeline_id == 0)
        return false;
    if (ok && is_new)
        ok = ok &&
             _create_pipeline_with_layout(
                 stream, out->ao_pipeline_id, vs_id, fs_id, out->ao_bgl_id) &&
             dvz_drp2_stream_pipeline_set_color_target(stream, 0, DVZ_FORMAT_R32_SFLOAT);

    if (denoise_x_pass != NULL)
    {
        dvz_snprintf(vs_key, sizeof(vs_key), "_vs_gtao_denoise%s", fmt);
        vs_id = _obj_id(emitter, vs_key, &is_new);
        if (vs_id == 0)
            return false;
        if (ok && is_new)
            ok =
                ok && _emit_shader_spirv(
                          stream, vs_id, "VERTEX", "fullscreen_vert",
                          _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_GTAO_DENOISE, false), cfg);
        for (uint32_t axis = 0; ok && axis < 2; axis++)
        {
            dvz_snprintf(fs_key, sizeof(fs_key), "_fs_gtao_denoise_%u%s", axis, fmt);
            dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe_gtao_denoise_%u%s", axis, fmt);
            fs_id = _obj_id(emitter, fs_key, &is_new);
            if (fs_id == 0)
                return false;
            if (is_new)
            {
                const char* define = axis == 0 ? "#define DVZ_GTAO_DENOISE_AXIS 1\n"
                                               : "#define DVZ_GTAO_DENOISE_AXIS 2\n";
                char* variant = _shader_glsl_variant(
                    _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_GTAO_DENOISE, true), define);
                if (variant == NULL)
                    return false;
                ok = _emit_shader_spirv(
                    stream, fs_id, "FRAGMENT",
                    axis == 0 ? "gtao_denoise_x_frag" : "gtao_denoise_y_frag", variant, cfg);
                _shader_glsl_variant_destroy(variant);
            }
            out->denoise_pipeline_ids[axis] = _obj_id(emitter, pipe_key, &is_new);
            if (out->denoise_pipeline_ids[axis] == 0)
                return false;
            if (ok && is_new)
                ok = _create_pipeline_with_layout(
                         stream, out->denoise_pipeline_ids[axis], vs_id, fs_id,
                         out->denoise_bgl_id) &&
                     dvz_drp2_stream_pipeline_set_color_target(stream, 0, DVZ_FORMAT_R32_SFLOAT);
        }
    }

    if (composite_pass != NULL)
    {
        dvz_snprintf(vs_key, sizeof(vs_key), "_vs_gtao_visibility_present%s", fmt);
        dvz_snprintf(fs_key, sizeof(fs_key), "_fs_gtao_visibility_present%s", fmt);
        uint32_t final_format = _render_pass_scene_color_target_format(cfg);
        dvz_snprintf(
            pipe_key, sizeof(pipe_key), "_pipe_gtao_visibility_present%s_%u", fmt, final_format);
        vs_id = _obj_id(emitter, vs_key, &is_new);
        if (vs_id == 0)
            return false;
        if (ok && is_new)
            ok = ok && _emit_shader_spirv(
                           stream, vs_id, "VERTEX", "fullscreen_vert",
                           _builtin_shader_glsl(
                               DVZ_SCENE_BUILTIN_SHADER_GTAO_VISIBILITY_PRESENTATION, false),
                           cfg);

        fs_id = _obj_id(emitter, fs_key, &is_new);
        if (fs_id == 0)
            return false;
        if (ok && is_new)
            ok = ok && _emit_shader_spirv(
                           stream, fs_id, "FRAGMENT", "gtao_visibility_present_frag",
                           _builtin_shader_glsl(
                               DVZ_SCENE_BUILTIN_SHADER_GTAO_VISIBILITY_PRESENTATION, true),
                           cfg);

        out->composite_pipeline_id = _obj_id(emitter, pipe_key, &is_new);
        if (out->composite_pipeline_id == 0)
            return false;
        if (ok && is_new)
        {
            ok = ok &&
                 _create_pipeline_with_layout(
                     stream, out->composite_pipeline_id, vs_id, fs_id, out->composite_bgl_id) &&
                 dvz_drp2_stream_pipeline_set_color_target(stream, 0, final_format);
        }
    }
    out->render = render;
    out->provider = DVZ_SCENE_WORK_PROVIDER_GTAO;
    return ok;
}



/**
 * Return a compact fingerprint for a WBOIT resolve bind group dependency set.
 *
 * @param accum_id accumulation texture id.
 * @param transmittance_id transparent transmittance texture id.
 * @param sampler_id sampler id.
 * @return dependency fingerprint.
 */
uint64_t
_wboit_bind_group_fingerprint(uint64_t accum_id, uint64_t transmittance_id, uint64_t sampler_id)
{
    uint64_t hash = UINT64_C(1469598103934665603);
    hash = (hash ^ accum_id) * UINT64_C(1099511628211);
    hash = (hash ^ transmittance_id) * UINT64_C(1099511628211);
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

    const DvzFrameGraphPass* graph_pass = _graph_pass_for_render(plan, render);
    const DvzSceneResolvedPass* resolved_accum = _graph_composition_pass(plan, graph_pass);
    if (resolved_accum == NULL ||
        resolved_accum->provider != DVZ_SCENE_WORK_PROVIDER_WBOIT_ACCUMULATION)
        return false;
    const DvzFrameGraphPass* resolve_graph_pass = _graph_pass_by_composition_provider(
        plan, render->u.render.panel_id, DVZ_SCENE_WORK_PROVIDER_WBOIT_RESOLVE,
        resolved_accum->ordinal);
    if (graph_pass == NULL || graph_pass->color_attachment_count != 2 ||
        resolve_graph_pass == NULL || resolve_graph_pass->read_count != 2)
        return false;

    const DvzFrameGraphResource* accum_resource =
        _graph_resource_by_id(plan, graph_pass->color_attachments[0].resource_id);
    const DvzFrameGraphResource* transmittance_resource =
        _graph_resource_by_id(plan, graph_pass->color_attachments[1].resource_id);
    const DvzFrameGraphResource* depth_resource = NULL;
    if (graph_pass->has_depth_attachment)
        depth_resource = _graph_resource_by_id(plan, graph_pass->depth_attachment.resource_id);
    if (accum_resource == NULL || transmittance_resource == NULL ||
        strcmp(resolve_graph_pass->reads[0].resource_id, accum_resource->id) != 0 ||
        strcmp(resolve_graph_pass->reads[1].resource_id, transmittance_resource->id) != 0)
        return false;

    ok = _graph_resolve_texture_2d(
        emitter, stream, plan, cfg, accum_resource, width, height, DVZ_FORMAT_R16G16B16A16_SFLOAT,
        &out->accum_id);
    ok = ok && _graph_resolve_texture_2d(
                   emitter, stream, plan, cfg, transmittance_resource, width, height,
                   DVZ_FORMAT_R16_SFLOAT, &out->transmittance_id);
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
    ok = ok && _graph_runtime_targets_add(graph_targets, accum_resource->id, out->accum_id);
    ok = ok && _graph_runtime_targets_add(
                   graph_targets, transmittance_resource->id, out->transmittance_id);
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
        bg_key, sizeof(bg_key), "_bg_wboit_%" PRIu64 "_%" PRIu64, out->accum_id,
        out->transmittance_id);
    ResourceId* bg_resource = _resource_entry(&emitter->objects, bg_key, &is_new);
    if (bg_resource == NULL || bg_resource->id == 0)
        return false;
    out->resolve_bg_id = bg_resource->id;

    uint64_t bg_fingerprint =
        _wboit_bind_group_fingerprint(out->accum_id, out->transmittance_id, out->sampler_id);
    if (!is_new && bg_resource->byte_size != bg_fingerprint)
        is_new = true;
    bg_resource->byte_size = bg_fingerprint;
    if (ok && is_new)
    {
        uint64_t accum_id =
            _graph_sampled_read_texture_id(resolve_graph_pass, 0, out->color_id, graph_targets, 0);
        uint64_t transmittance_id =
            _graph_sampled_read_texture_id(resolve_graph_pass, 1, out->color_id, graph_targets, 0);
        if (accum_id == 0 || transmittance_id == 0)
            return false;
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
                .resource_id = transmittance_id,
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
 * Return one graph pass belonging to a specific typed depth-peeling technique instance.
 *
 * @param plan source FramePlan
 * @param technique_instance_id technique instance identity
 * @param provider requested depth-peeling provider
 * @param work_index requested iteration index, ignored for non-iteration providers
 * @return matching graph pass, or NULL when absent
 */
static const DvzFrameGraphPass* _depth_peel_graph_pass(
    const DvzFramePlan* plan, DvzSceneTechniqueInstanceId technique_instance_id,
    DvzSceneWorkProviderKey provider, uint32_t work_index)
{
    ANN(plan);
    for (uint32_t i = 0; i < dvz_frame_plan_graph_pass_count(plan); i++)
    {
        const DvzFrameGraphPass* pass = dvz_frame_plan_graph_pass_get(plan, i);
        const DvzSceneResolvedPass* resolved = _graph_composition_pass(plan, pass);
        if (resolved == NULL || resolved->provider != provider ||
            resolved->technique_instance_id.value != technique_instance_id.value)
            continue;
        if (provider != DVZ_SCENE_WORK_PROVIDER_DEPTH_PEEL_ITERATION ||
            resolved->work_index == work_index)
            return pass;
    }
    return NULL;
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

    const DvzFrameGraphPass* init_graph_pass = _graph_pass_for_render(plan, render);
    const DvzSceneResolvedPass* init_resolved = _graph_composition_pass(plan, init_graph_pass);
    if (init_graph_pass == NULL || init_resolved == NULL ||
        init_resolved->provider != DVZ_SCENE_WORK_PROVIDER_DEPTH_PEEL_INIT)
        return false;
    for (uint32_t i = 0; ok && i < dvz_frame_plan_graph_pass_count(plan); i++)
    {
        const DvzFrameGraphPass* pass = dvz_frame_plan_graph_pass_get(plan, i);
        const DvzSceneResolvedPass* resolved = _graph_composition_pass(plan, pass);
        if (pass == NULL || resolved == NULL ||
            resolved->technique_instance_id.value != init_resolved->technique_instance_id.value)
            continue;
        ok = _graph_prepare_render_color_targets(emitter, stream, plan, pass, cfg, graph_targets);
        if (ok && pass->has_depth_attachment)
        {
            const DvzFrameGraphResource* depth_resource =
                _graph_resource_by_id(plan, pass->depth_attachment.resource_id);
            uint64_t depth_id = 0;
            ok = depth_resource != NULL &&
                 _graph_resolve_texture_2d(
                     emitter, stream, plan, cfg, depth_resource, width, height,
                     DVZ_FORMAT_D32_SFLOAT, &depth_id) &&
                 _graph_runtime_targets_add(graph_targets, depth_resource->id, depth_id);
            if (ok && pass == init_graph_pass)
                out->depth_id = depth_id;
        }
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

    const DvzFrameGraphPass* composite_pass = _depth_peel_graph_pass(
        plan, init_resolved->technique_instance_id, DVZ_SCENE_WORK_PROVIDER_DEPTH_PEEL_COMPOSITE,
        0);
    ok = ok && composite_pass != NULL;
    if (ok)
    {
        char composite_bg_key[DVZ_SCENE_LABEL_SIZE];
        char composite_bg_base_key[DVZ_SCENE_LABEL_SIZE];
        dvz_snprintf(
            composite_bg_base_key, sizeof(composite_bg_base_key),
            "_bg_depth_peel_composite_%" PRIu64, init_resolved->technique_instance_id.value);
        _runtime_scope_key(cfg, composite_bg_base_key, composite_bg_key, sizeof(composite_bg_key));
        ok = ok && _depth_peel_resolve_sampled_bind_group(
                       emitter, stream, plan, composite_pass, graph_targets, 0, composite_bg_key,
                       out->composite_bgl_id, out->sampler_id, &out->composite_bg_id);
    }

    for (uint32_t iter_idx = 0; ok && iter_idx < DVZ_SCENE_DEPTH_PEEL_ITERATIONS; iter_idx++)
    {
        const DvzFrameGraphPass* iter_pass = _depth_peel_graph_pass(
            plan, init_resolved->technique_instance_id,
            DVZ_SCENE_WORK_PROVIDER_DEPTH_PEEL_ITERATION, iter_idx);
        ok = ok && iter_pass != NULL;
        if (ok)
        {
            char iter_bg_base_key[DVZ_SCENE_LABEL_SIZE];
            char iter_bg_key[DVZ_SCENE_LABEL_SIZE];
            dvz_snprintf(
                iter_bg_base_key, sizeof(iter_bg_base_key),
                "_bg_depth_peel_iter_%" PRIu64 "_%" PRIu32,
                init_resolved->technique_instance_id.value, iter_idx);
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
