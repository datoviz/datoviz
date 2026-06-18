/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene render-pass helpers                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include <vulkan/vulkan_core.h>

#include "_assertions.h"
#include "_compat.h"
#include "frame_plan/emit.h"
#include "_render_pass.h"
#include "_shader_registry.h"
#include "_vk_utils.h"
#include "datoviz/drp2.h"
#include "datoviz/drp2/stream.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

static const char ENCODE_GLSL[] =
    "#version 450\n"
    "layout(set = 0, binding = 0) uniform texture2D sourceTex;\n"
    "layout(set = 0, binding = 1) uniform sampler samp;\n"
    "layout(location = 0) out vec4 outColor;\n"
    "vec3 linearToSrgb(vec3 linearColor)\n"
    "{\n"
    "    vec3 clipped = clamp(linearColor, 0.0, 1.0);\n"
    "    vec3 lo = 12.92 * clipped;\n"
    "    vec3 hi = 1.055 * pow(clipped, vec3(1.0 / 2.4)) - 0.055;\n"
    "    return mix(hi, lo, lessThanEqual(clipped, vec3(0.0031308)));\n"
    "}\n"
    "void main()\n"
    "{\n"
    "    ivec2 p = ivec2(gl_FragCoord.xy);\n"
    "    ivec2 extent = textureSize(sampler2D(sourceTex, samp), 0);\n"
    "    if (p.x < 0 || p.y < 0 || p.x >= extent.x || p.y >= extent.y)\n"
    "        discard;\n"
    "    vec4 color = texelFetch(sampler2D(sourceTex, samp), p, 0);\n"
    "    outColor = vec4(linearToSrgb(color.rgb), clamp(color.a, 0.0, 1.0));\n"
    "}\n";

static const char ENCODE_WGSL[] =
    "@group(0) @binding(0) var source_tex: texture_2d<f32>;\n"
    "@group(0) @binding(1) var samp: sampler;\n"
    "fn linear_to_srgb(linear_color: vec3f) -> vec3f {\n"
    "    let clipped = clamp(linear_color, vec3f(0.0), vec3f(1.0));\n"
    "    let lo = vec3f(12.92) * clipped;\n"
    "    let hi = vec3f(1.055) * pow(clipped, vec3f(1.0 / 2.4)) - vec3f(0.055);\n"
    "    return select(hi, lo, clipped <= vec3f(0.0031308));\n"
    "}\n"
    "@fragment fn main(@builtin(position) pos: vec4f) -> @location(0) vec4f {\n"
    "    let p = vec2i(pos.xy);\n"
    "    let color = textureLoad(source_tex, p, 0);\n"
    "    return vec4f(linear_to_srgb(color.rgb), clamp(color.a, 0.0, 1.0));\n"
    "}\n";



static bool _render_pass_external_target_needs_encode(const DvzFramePlanEmitConfig* cfg)
{
    return cfg != NULL && cfg->external_color_target &&
           dvz_format_requires_final_srgb_encode((VkFormat)cfg->color_target_format);
}



uint32_t _render_pass_scene_color_target_format(const DvzFramePlanEmitConfig* cfg)
{
    if (_render_pass_external_target_needs_encode(cfg))
        return VK_FORMAT_R8G8B8A8_UNORM;
    return cfg != NULL ? cfg->color_target_format : 0;
}



static bool _render_pass_resolve_linear_intermediate(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream,
    const DvzFramePlanEmitConfig* cfg, uint64_t* out_id)
{
    ANN(emitter);
    ANN(stream);
    ANN(out_id);

    bool needs_create = false;
    ResourceId* resource = _resource_entry(&emitter->objects, "_ct_linear_display", &needs_create);
    if (resource == NULL || resource->id == 0)
        return false;

    uint32_t width = cfg != NULL && cfg->target_width > 0 ? cfg->target_width : 4;
    uint32_t height = cfg != NULL && cfg->target_height > 0 ? cfg->target_height : 4;
    uint32_t format = VK_FORMAT_R8G8B8A8_UNORM;
    if (
        needs_create || resource->texture_width == 0 || resource->texture_height == 0 ||
        resource->texture_depth == 0 || resource->texture_format != format ||
        resource->texture_width != width || resource->texture_height != height)
    {
        resource->texture_width = width;
        resource->texture_height = height;
        resource->texture_depth = 1;
        resource->texture_format = format;
        needs_create = true;
    }
    if (needs_create)
    {
        uint32_t usage = DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT |
                         DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING |
                         DVZ_DRP2_TEXTURE_USAGE_COPY_SRC;
        if (!dvz_drp2_stream_create_texture_2d_format_usage(
                stream, resource->id, width, height, format, usage))
            return false;
    }
    *out_id = resource->id;
    return true;
}

/**
 * Resolve or create the color target used by a runtime render pass.
 *
 * @param emitter the persistent emitter
 * @param stream the DRP2 command stream
 * @param cfg the emission config
 * @param out_id the resolved color target id
 * @return whether the color target was resolved
 */
bool _render_pass_resolve_color_target(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream,
    const DvzFramePlanEmitConfig* cfg, uint64_t* out_id)
{
    ANN(emitter);
    ANN(stream);
    ANN(out_id);

    *out_id = 0;
    if (cfg != NULL && cfg->external_color_target)
    {
        *out_id = cfg->color_target_id;
        return true;
    }

    bool needs_create = false;
    ResourceId* resource = _resource_entry(&emitter->objects, "_ct", &needs_create);
    if (resource == NULL || resource->id == 0)
        return false;
    uint32_t width = cfg != NULL && cfg->target_width > 0 ? cfg->target_width : 4;
    uint32_t height = cfg != NULL && cfg->target_height > 0 ? cfg->target_height : 4;
    uint32_t format = cfg != NULL && cfg->color_target_format != 0 ?
                          cfg->color_target_format :
                          VK_FORMAT_R8G8B8A8_UNORM;
    if (
        needs_create || resource->texture_width == 0 || resource->texture_height == 0 ||
        resource->texture_depth == 0)
    {
        resource->texture_width = width;
        resource->texture_height = height;
        resource->texture_depth = 1;
        resource->texture_format = format;
        needs_create = true;
    }
    else if (
        width != resource->texture_width || height != resource->texture_height ||
        resource->texture_depth != 1 || resource->texture_format != format)
    {
        resource->texture_width = width;
        resource->texture_height = height;
        resource->texture_depth = 1;
        resource->texture_format = format;
        needs_create = true;
    }
    if (needs_create)
    {
        uint32_t usage =
            DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC;
        if (!dvz_drp2_stream_create_texture_2d_format_usage(
                stream, resource->id, width, height, format, usage))
            return false;
    }
    *out_id = resource->id;
    return true;
}



/**
 * Resolve the render target for scene arithmetic and the final display target.
 *
 * If the external final target is an 8-bit UNORM display target, scene draws are redirected to a
 * linear intermediate texture so blending stays linear; a later final pass encodes to sRGB.
 */
bool _render_pass_resolve_scene_color_target(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream,
    const DvzFramePlanEmitConfig* cfg, uint64_t* out_scene_id, uint64_t* out_final_id)
{
    ANN(out_scene_id);
    ANN(out_final_id);

    *out_scene_id = 0;
    *out_final_id = 0;
    if (!_render_pass_resolve_color_target(emitter, stream, cfg, out_final_id))
        return false;
    if (_render_pass_external_target_needs_encode(cfg))
        return _render_pass_resolve_linear_intermediate(emitter, stream, cfg, out_scene_id);
    *out_scene_id = *out_final_id;
    return true;
}



bool _render_pass_emit_final_encode(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream,
    const DvzFramePlanEmitConfig* cfg, uint64_t encoder_id, uint64_t scene_color_id,
    uint64_t final_color_id)
{
    ANN(emitter);
    ANN(stream);
    if (!_render_pass_external_target_needs_encode(cfg) || scene_color_id == final_color_id)
        return true;

    bool is_new = false;
    uint64_t sampler_id = _obj_id(emitter, "_sampler_final_srgb_encode", &is_new);
    if (sampler_id == 0)
        return false;
    bool ok = true;
    if (is_new)
        ok = ok && dvz_drp2_stream_create_sampler(stream, sampler_id);

    uint64_t bgl_id = _obj_id(emitter, "_bgl_final_srgb_encode", &is_new);
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
        ok = ok && dvz_drp2_stream_create_bind_group_layout_entries(stream, bgl_id, 2, entries);
    }

    char bg_key[96];
    dvz_snprintf(bg_key, sizeof(bg_key), "_bg_final_srgb_encode_%" PRIu64, scene_color_id);
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
                .resource_id = scene_color_id,
            },
            {
                .binding = 1,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_SAMPLER,
                .resource_id = sampler_id,
            },
        };
        ok = ok && dvz_drp2_stream_create_bind_group_entries(stream, bg_id, bgl_id, 2, entries);
    }

    const char* fmt = _shader_format_tag(cfg);
    char vs_key[48];
    char fs_key[48];
    char pipe_key[80];
    dvz_snprintf(vs_key, sizeof(vs_key), "_vs_final_srgb_encode%s", fmt);
    dvz_snprintf(fs_key, sizeof(fs_key), "_fs_final_srgb_encode%s", fmt);
    dvz_snprintf(
        pipe_key, sizeof(pipe_key), "_pipe_final_srgb_encode%s_%u", fmt,
        cfg != NULL ? cfg->color_target_format : 0);

    uint64_t vs_id = _obj_id(emitter, vs_key, &is_new);
    if (vs_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && _emit_shader(stream, vs_id, "VERTEX", _fullscreen_vertex_wgsl(),
                                _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_WBOIT_RESOLVE, false),
                                cfg);

    uint64_t fs_id = _obj_id(emitter, fs_key, &is_new);
    if (fs_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && _emit_shader(stream, fs_id, "FRAGMENT", ENCODE_WGSL, ENCODE_GLSL, cfg);

    uint64_t pipe_id = _obj_id(emitter, pipe_key, &is_new);
    if (pipe_id == 0)
        return false;
    if (ok && is_new)
        ok = ok &&
             dvz_drp2_stream_create_render_pipeline_with_bind_group_layout(
                 stream, pipe_id, vs_id, fs_id, 0, bgl_id) &&
             dvz_drp2_stream_pipeline_set_color_target(
                 stream, 0, cfg != NULL ? cfg->color_target_format : VK_FORMAT_R8G8B8A8_UNORM);

    uint64_t pass_id = _emitter_next_transient_id(emitter);
    ok = ok &&
         dvz_drp2_stream_begin_render_pass_region_clear(
             stream, pass_id, encoder_id, final_color_id, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
             1.0f, 1.0f, true) &&
         dvz_drp2_stream_set_pipeline(stream, pass_id, pipe_id) &&
         dvz_drp2_stream_set_bind_group(stream, pass_id, 0, bg_id) &&
         dvz_drp2_stream_draw(stream, pass_id, 3, 1, 0, 0) &&
         dvz_drp2_stream_end_render_pass(stream, pass_id);
    return ok;
}



/**
 * Resolve or create the optional readback buffer for a render-pass copy node.
 *
 * @param emitter the persistent emitter
 * @param stream the DRP2 command stream
 * @param copy the optional readback copy node
 * @param out_id the resolved readback buffer id
 * @return whether the readback buffer was resolved
 */
bool _render_pass_resolve_readback_buffer(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlanNode* copy,
    uint64_t* out_id)
{
    ANN(emitter);
    ANN(stream);
    ANN(out_id);

    *out_id = 0;
    if (copy == NULL)
        return true;

    bool is_new = false;
    uint64_t rb_id = _obj_buffer_id(emitter, "_rb", copy->u.copy.byte_size, &is_new);
    if (rb_id == 0)
        return false;
    if (is_new)
    {
        uint32_t usage = DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ;
        if (!dvz_drp2_stream_create_buffer(stream, rb_id, copy->u.copy.byte_size, usage))
            return false;
    }
    *out_id = rb_id;
    return true;
}



/**
 * Allocate transient command ids for one runtime render pass submission.
 *
 * @param emitter the persistent emitter
 * @param encoder_id the command encoder id
 * @param render_pass_id the render pass id
 * @param command_buffer_id the command buffer id
 * @param submission_id the queue submission id
 */
void _render_pass_next_ids(
    DvzFramePlanEmitter* emitter, uint64_t* encoder_id, uint64_t* render_pass_id,
    uint64_t* command_buffer_id, uint64_t* submission_id)
{
    ANN(emitter);
    ANN(encoder_id);
    ANN(render_pass_id);
    ANN(command_buffer_id);
    ANN(submission_id);

    *encoder_id = _emitter_next_transient_id(emitter);
    *render_pass_id = _emitter_next_transient_id(emitter);
    *command_buffer_id = _emitter_next_transient_id(emitter);
    *submission_id = _emitter_next_transient_id(emitter);
}



/**
 * Finish a render encoder and submit it, optionally with a readback copy.
 *
 * @param stream the DRP2 command stream
 * @param encoder_id the command encoder id
 * @param command_buffer_id the command buffer id
 * @param submission_id the queue submission id
 * @param color_id the rendered color texture id
 * @param readback_buffer_id the optional readback buffer id
 * @param copy the optional readback copy node
 * @return whether the copy, finish, and submit commands were emitted
 */
bool _render_pass_copy_finish_submit(
    DvzDrp2CommandStream* stream, uint64_t encoder_id, uint64_t command_buffer_id,
    uint64_t submission_id, uint64_t color_id, uint64_t readback_buffer_id,
    const DvzFramePlanNode* copy)
{
    ANN(stream);

    bool ok = true;
    if (copy != NULL)
    {
        if (color_id == 0 || readback_buffer_id == 0)
            return false;
        ok = ok && dvz_drp2_stream_copy_texture_to_buffer(
                       stream, encoder_id, color_id, readback_buffer_id,
                       copy->u.copy.dst_offset, copy->u.copy.extent[0], copy->u.copy.extent[1],
                       (uint32_t)copy->u.copy.bytes_per_row, copy->u.copy.rows_per_image);
    }

    ok = ok && dvz_drp2_stream_finish_command_encoder(stream, encoder_id, command_buffer_id);
    if (copy != NULL)
        ok = ok && dvz_drp2_stream_queue_submit_readback(
                       stream, command_buffer_id, submission_id, readback_buffer_id, 0,
                       copy->u.copy.byte_size);
    else
        ok = ok && dvz_drp2_stream_queue_submit(stream, command_buffer_id, submission_id);
    return ok;
}
