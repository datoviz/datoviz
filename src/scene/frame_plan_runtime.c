/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene FramePlan runtime emission                                                             */
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
#include "_frame_plan_emit.h"
#include "_frame_plan.h"
#include "_frame_plan_runtime_upload.h"
#include "_scene_common_bindings.h"
#include "_render_pass.h"
#include "_shader_registry.h"
#include "_visual_pipeline.h"
#include "datoviz/drp2.h"
#include "datoviz/drp2/stream.h"
#include "datoviz/scene.h"
#include "_scene.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct SceneRenderDraw SceneRenderDraw;
typedef struct SceneRenderBatch SceneRenderBatch;
typedef struct SceneWboitTargets SceneWboitTargets;

struct SceneRenderDraw
{
    uint64_t pipeline_id;
    uint64_t bg_set0;  /* MVP bg; 0 = none */
    uint64_t bg_set1;  /* image texture or primitive shading bg; 0 = none */
    DvzSceneVisualDesc visual;
};


struct SceneRenderBatch
{
    const DvzFramePlanNode* render;
    SceneRenderDraw draws[DVZ_SCENE_MAX_RENDER_VISUALS];
    uint32_t draw_count;
};


struct SceneWboitTargets
{
    uint64_t color_id;
    uint64_t accum_id;
    uint64_t weight_id;
    uint64_t depth_id;
    uint64_t sampler_id;
    uint64_t resolve_bgl_id;
    uint64_t resolve_bg_id;
    uint64_t resolve_pipeline_id;
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return whether an alpha mode uses ordinary source-over blending.
 *
 * @param mode the visual alpha mode
 * @return whether the mode needs a blended final-target pipeline
 */
static bool _alpha_mode_is_standard_blend(DvzAlphaMode mode)
{
    return mode == DVZ_ALPHA_BLENDED;
}


/**
 * Create the volume bind group layout used by slice/raymarch shaders.
 *
 * @param stream destination DRP2 command stream.
 * @param id bind group layout id.
 * @return whether the command was appended.
 */
static bool _create_volume_bind_group_layout(DvzDrp2CommandStream* stream, uint64_t id)
{
    ANN(stream);

    DvzDrp2BindGroupLayoutEntry entries[4] = {
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
        {
            .binding = 3,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
            .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
            .access = DVZ_DRP2_BINDING_ACCESS_READ,
        },
    };
    return dvz_drp2_stream_create_bind_group_layout_entries(stream, id, 4, entries);
}



/**
 * Resolve a 1x1 far-depth texture for volume passes without shared scene depth.
 *
 * @param emitter frame-plan emitter carrying persistent object ids.
 * @param stream destination DRP2 command stream.
 * @param out_id output texture id.
 * @return whether the fallback texture is available.
 */
static bool _resolve_volume_dummy_depth(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, uint64_t* out_id)
{
    ANN(emitter);
    ANN(stream);
    ANN(out_id);

    static const float depth_value = 1.0f;
    bool is_new = false;
    uint64_t depth_id = _obj_id(emitter, "_tex_volume_dummy_depth", &is_new);
    if (depth_id == 0)
        return false;
    if (is_new)
    {
        uint32_t usage = DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING | DVZ_DRP2_TEXTURE_USAGE_COPY_DST;
        if (!dvz_drp2_stream_create_texture_2d_format_usage(
                stream, depth_id, 1, 1, VK_FORMAT_R32_SFLOAT, usage))
            return false;
        if (!dvz_drp2_stream_write_texture_2d_bytes(
                stream, depth_id, 0, 1, 1, sizeof(float), 1, &depth_value))
            return false;
    }
    *out_id = depth_id;
    return true;
}


/**
 * Convert retained volume state into the shader uniform payload.
 *
 * @param state retained volume state.
 * @param transfer_rgba whether the bound volume texture already contains RGBA transfer colors.
 * @param out output uniform payload.
 */
static void _volume_uniform_from_state(
    const DvzVolumeState* state, bool transfer_rgba, DvzSceneVolumeUniform* out)
{
    ANN(state);
    ANN(out);

    dvz_memset(out, sizeof(DvzSceneVolumeUniform), 0, sizeof(DvzSceneVolumeUniform));
    for (uint32_t i = 0; i < 3; i++)
    {
        out->clip_min[i] = state->clipping_enabled ? (float)state->clip_min[i] : 0.0f;
        out->clip_max[i] = state->clipping_enabled ? (float)state->clip_max[i] : 1.0f;
    }
    out->clip_min[3] = transfer_rgba ? 1.0f : 0.0f;
    out->clip_max[3] = 1.0f;
    out->params[0] = state->opacity;
    out->params[1] = state->clipping_enabled ? 1.0f : 0.0f;
    out->params[2] = (float)state->step_count;
    out->params[3] = (float)state->render_mode;
}


/**
 * Resolve the volume texture/sampler/parameter bind group for one visual.
 *
 * @param emitter frame-plan emitter carrying persistent object ids and uniform cache.
 * @param stream destination DRP2 command stream.
 * @param bgl_id volume bind group layout id.
 * @param sampler_id shared volume sampler id.
 * @param bind volume bind descriptor.
 * @param out_bg_id resolved bind group id.
 * @return whether the bind group was resolved.
 */
static bool _resolve_volume_bind_group(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, uint64_t bgl_id,
    uint64_t sampler_id, const DvzSceneVisualBindDesc* bind, uint64_t* out_bg_id)
{
    ANN(emitter);
    ANN(stream);
    ANN(bind);
    ANN(out_bg_id);
    *out_bg_id = 0;

    uint64_t depth_texture_id = bind->volume_depth_texture_id;
    if (depth_texture_id == 0 &&
        !_resolve_volume_dummy_depth(emitter, stream, &depth_texture_id))
        return false;

    bool is_new = false;
    char params_buf_key[64], params_slot_key[64], bg_key[96];
    dvz_snprintf(
        params_buf_key, sizeof(params_buf_key), "_buf_volume_params_%" PRIu64,
        bind->volume_texture_id);
    dvz_snprintf(
        params_slot_key, sizeof(params_slot_key), "_slot_volume_params_%" PRIu64,
        bind->volume_texture_id);
    dvz_snprintf(
        bg_key, sizeof(bg_key), "_bg_volume_%" PRIu64 "_depth_%" PRIu64,
        bind->volume_texture_id, depth_texture_id);

    uint32_t usage = DVZ_DRP2_BUFFER_USAGE_UNIFORM | DVZ_DRP2_BUFFER_USAGE_MAP_WRITE |
                     DVZ_DRP2_BUFFER_USAGE_COPY_DST;
    uint64_t params_buf_id = _obj_id(emitter, params_buf_key, &is_new);
    if (params_buf_id == 0)
        return false;
    if (is_new && !dvz_drp2_stream_create_buffer(
                      stream, params_buf_id, sizeof(DvzSceneVolumeUniform), usage))
        return false;

    uint64_t bg_id = _obj_id(emitter, bg_key, &is_new);
    if (bg_id == 0)
        return false;
    if (is_new)
    {
        DvzDrp2BindGroupEntry entries[4] = {
            {
                .binding = 0,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
                .resource_id = bind->volume_texture_id,
            },
            {
                .binding = 1,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_SAMPLER,
                .resource_id = sampler_id,
            },
            {
                .binding = 2,
                .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_BUFFER,
                .resource_id = params_buf_id,
                .offset = 0,
                .size = sizeof(DvzSceneVolumeUniform),
            },
            {
                .binding = 3,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
                .resource_id = depth_texture_id,
            },
        };
        if (!dvz_drp2_stream_create_bind_group_entries(stream, bg_id, bgl_id, 4, entries))
            return false;
    }

    DvzSceneVolumeUniform* slot = _emitter_volume_slot(emitter, params_slot_key);
    if (slot == NULL)
        return false;
    _volume_uniform_from_state(&bind->volume_state, bind->volume_transfer_rgba, slot);
    if (!dvz_drp2_stream_write_buffer_bytes(
            stream, params_buf_id, 0, sizeof(DvzSceneVolumeUniform), slot))
        return false;

    *out_bg_id = bg_id;
    return true;
}



/**
 * Attach scene/runtime labels to ids in an emitted DRP2 stream.
 *
 * @param emitter frame-plan emitter carrying scene/resource id maps
 * @param stream emitted DRP2 command stream
 * @param cfg optional emission configuration with borrowed target id
 */
static void _emitter_label_stream_ids(
    const DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream,
    const DvzFramePlanEmitConfig* cfg)
{
    ANN(emitter);
    ANN(stream);

    for (uint32_t i = 0; i < emitter->resources.count; i++)
    {
        const ResourceId* resource = &emitter->resources.resources[i];
        if (resource->id != 0 && resource->key[0] != '\0')
            dvz_drp2_stream_set_label(stream, resource->id, resource->key);
    }

    for (uint32_t i = 0; i < emitter->objects.count; i++)
    {
        const ResourceId* object = &emitter->objects.resources[i];
        if (object->id != 0 && object->key[0] != '\0')
            dvz_drp2_stream_set_label(stream, object->id, object->key);
    }

    if (cfg != NULL && cfg->color_target_id != 0)
        dvz_drp2_stream_set_label(stream, cfg->color_target_id, "rt");
}



/**
 * Prepare resources for one panel's draws before opening the render pass.
 *
 * @param emitter frame-plan emitter carrying scene/runtime state.
 * @param stream destination DRP2 command stream.
 * @param render render node to prepare.
 * @param cfg optional frame-plan emit configuration.
 * @param sampled_depth_id depth texture sampled by volume shaders, or zero.
 * @param report diagnostic report receiving recoverable emission errors.
 * @param draws output draw descriptors filled from prepared visuals.
 * @param draw_count_out output number of prepared draw descriptors.
 * @return true when the render node has drawable prepared visuals, false otherwise.
 */
static bool _emitter_prepare_render_multi(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlanNode* render,
    const DvzFramePlanEmitConfig* cfg, uint64_t sampled_depth_id, DvzDiagnosticReport* report,
    SceneRenderDraw* draws, uint32_t* draw_count_out)
{
    ANN(emitter);
    ANN(stream);
    ANN(render);
    ANN(draws);
    ANN(draw_count_out);

    bool ok = true;
    bool is_new = false;
    const char* fmt = _shader_format_tag(cfg);
    bool wboit_accumulation =
        render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION;
    bool pass_needs_depth = _scene_render_needs_depth(emitter, render);

    uint64_t common_bgl_id = 0;
    uint64_t apply_bg_id = 0;
    uint64_t fixed_bg_id = 0;
    if (!_scene_common_bindings_resolve_panel_sets(
            emitter, stream, render, &common_bgl_id, &apply_bg_id, &fixed_bg_id))
        return false;

    /* Image BGL + sampler (shared, created lazily on first image visual). */
    uint64_t img_bgl_id = 0, img_sampler_id = 0;
    uint64_t volume_bgl_id = 0, volume_sampler_id = 0;

    uint32_t draw_count = 0;

    for (uint32_t i = 0; ok && i < render->u.render.visual_count; i++)
    {
        DvzSceneVisualDesc desc = {0};
        const char* visual_error = NULL;
        if (!_scene_visual_desc_from_render(emitter, render, i, &desc, &visual_error))
        {
            if (render->u.render.visual_metadata[i].has_metadata)
            {
                _diagnostic(
                    report, visual_error != NULL ? visual_error :
                                                  "invalid typed visual metadata");
                ok = false;
                break;
            }
            continue;
        }

        DvzSceneVisualShaderDesc shader = {0};
        if (!_scene_visual_shader_desc(
                &desc, render->u.render.picking, wboit_accumulation, fmt, &shader))
            continue;
        DvzAlphaMode alpha_mode = render->u.render.visual_metadata[i].has_metadata
                                      ? render->u.render.visual_metadata[i].alpha_mode
                                      : DVZ_ALPHA_OPAQUE;
        if (_alpha_mode_is_standard_blend(alpha_mode))
        {
            size_t key_len = strlen(shader.pipeline_key);
            dvz_snprintf(
                shader.pipeline_key + key_len, sizeof(shader.pipeline_key) - key_len, "_blend");
        }
        if (render->u.render.controller_modes[i] == DVZ_CONTROLLER_FIXED)
        {
            size_t key_len = strlen(shader.pipeline_key);
            dvz_snprintf(
                shader.pipeline_key + key_len, sizeof(shader.pipeline_key) - key_len, "_fixed");
        }

        /* Shaders (cached). */
        uint64_t vs_id = _obj_id(emitter, shader.vertex_key, &is_new);
        if (vs_id == 0) { ok = false; break; }
        if (is_new)
        {
            if (cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_WGSL)
            {
                if (shader.vertex_wgsl == NULL)
                    ok = false;
                else
                    ok = ok && _emit_shader(
                                     stream, vs_id, "VERTEX", shader.vertex_wgsl,
                                     shader.vertex_glsl, cfg);
            }
            else if (shader.vertex_spirv_key != NULL)
                ok = ok && _emit_shader_spirv(
                               stream, vs_id, "VERTEX", shader.vertex_spirv_key,
                               shader.vertex_glsl, cfg);
            else
                ok = ok && _emit_shader(stream, vs_id, "VERTEX", NULL, shader.vertex_glsl, cfg);
        }

        uint64_t fs_id = _obj_id(emitter, shader.fragment_key, &is_new);
        if (fs_id == 0) { ok = false; break; }
        if (ok && is_new)
        {
            if (cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_WGSL)
            {
                if (shader.fragment_wgsl == NULL)
                    ok = false;
                else
                    ok = ok &&
                         _emit_shader(
                             stream, fs_id, "FRAGMENT", shader.fragment_wgsl,
                             shader.fragment_glsl, cfg);
            }
            else if (shader.fragment_spirv_key != NULL)
                ok = ok && _emit_shader_spirv(
                               stream, fs_id, "FRAGMENT", shader.fragment_spirv_key,
                               shader.fragment_glsl, cfg);
            else
                ok = ok &&
                     _emit_shader(stream, fs_id, "FRAGMENT", NULL, shader.fragment_glsl, cfg);
        }

        uint64_t pipe_id = _obj_id(emitter, shader.pipeline_key, &is_new);
        if (pipe_id == 0) { ok = false; break; }
        if (ok && is_new)
        {
            DvzSceneVisualPipelineDesc pipeline = {0};
            if (!_scene_visual_pipeline_desc(
                    &desc, render->u.render.picking, pass_needs_depth, wboit_accumulation,
                    alpha_mode, render->u.render.controller_modes[i], &pipeline))
            {
                ok = false;
                break;
            }
            uint64_t shading_bgl_id = 0;
            if (pipeline.needs_shading_layout)
            {
                bool shading_bgl_new = false;
                shading_bgl_id = _obj_id(emitter, "_bgl_prim_shading", &shading_bgl_new);
                if (shading_bgl_id == 0) { ok = false; break; }
                if (shading_bgl_new)
                    ok = ok &&
                         dvz_drp2_stream_create_uniform_bind_group_layout(stream, shading_bgl_id);
            }
            if (pipeline.needs_image_layout && img_bgl_id == 0)
            {
                img_bgl_id = _obj_id(emitter, "_bgl_img", &is_new);
                if (img_bgl_id == 0) { ok = false; break; }
                if (is_new)
                    ok = ok && dvz_drp2_stream_create_texture_sampler_bind_group_layout(
                                   stream, img_bgl_id);
            }
            if (pipeline.needs_volume_layout && volume_bgl_id == 0)
            {
                volume_bgl_id = _obj_id(emitter, "_bgl_volume", &is_new);
                if (volume_bgl_id == 0) { ok = false; break; }
                if (is_new)
                    ok = ok && _create_volume_bind_group_layout(stream, volume_bgl_id);
            }
            ok = ok && dvz_drp2_stream_create_render_pipeline_ex(
                           stream, pipe_id, vs_id, fs_id, pipeline.vertex_buffer_count,
                           pipeline.topology, pipeline.binding_count, pipeline.strides,
                           pipeline.attr_count, pipeline.bindings, pipeline.locations,
                           pipeline.formats, pipeline.offsets);
            if (ok && pipeline.needs_common_layout && common_bgl_id != 0)
                ok = dvz_drp2_stream_pipeline_set_bind_group_layout(stream, common_bgl_id);
            if (ok && pipeline.needs_image_layout && img_bgl_id != 0 &&
                !pipeline.needs_common_layout)
                ok = dvz_drp2_stream_pipeline_set_bind_group_layout(stream, img_bgl_id);
            if (ok && pipeline.needs_image_layout && img_bgl_id != 0 &&
                pipeline.needs_common_layout)
                ok = dvz_drp2_stream_pipeline_set_bind_group_layout2(stream, img_bgl_id);
            if (ok && pipeline.needs_volume_layout && volume_bgl_id != 0 &&
                !pipeline.needs_common_layout)
                ok = dvz_drp2_stream_pipeline_set_bind_group_layout(stream, volume_bgl_id);
            if (ok && pipeline.needs_volume_layout && volume_bgl_id != 0 &&
                pipeline.needs_common_layout)
                ok = dvz_drp2_stream_pipeline_set_bind_group_layout2(stream, volume_bgl_id);
            if (ok && pipeline.needs_shading_layout)
                ok = dvz_drp2_stream_pipeline_set_bind_group_layout2(stream, shading_bgl_id);
            if (ok && pipeline.has_depth_state)
                ok = dvz_drp2_stream_pipeline_set_depth_state(
                    stream, pipeline.depth_write_enabled, pipeline.depth_compare_op);
            if (ok && pipeline.has_raster_state)
                ok = dvz_drp2_stream_pipeline_set_raster_state(
                    stream, pipeline.cull_mode, pipeline.front_face);
            if (ok && wboit_accumulation)
            {
                ok = ok &&
                     dvz_drp2_stream_pipeline_set_color_target(
                         stream, 0, VK_FORMAT_R16G16B16A16_SFLOAT) &&
                     dvz_drp2_stream_pipeline_set_color_target(
                         stream, 1, VK_FORMAT_R16_SFLOAT) &&
                     dvz_drp2_stream_pipeline_set_color_blend(
                         stream, 0, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD,
                         VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD,
                         VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                             VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT) &&
                     dvz_drp2_stream_pipeline_set_color_blend(
                         stream, 1, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD,
                         VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD,
                         VK_COLOR_COMPONENT_R_BIT);
            }
            else if (ok && _alpha_mode_is_standard_blend(alpha_mode))
            {
                ok = dvz_drp2_stream_pipeline_set_color_blend(
                    stream, 0, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                    VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                    VK_BLEND_OP_ADD,
                    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);
            }
        }

        /* Bind group at set 0. */
        uint64_t vis_bg_set0 = 0;
        uint64_t vis_bg_set1 = 0;
        DvzSceneVisualBindDesc bind = {0};
        if (!_scene_visual_bind_desc(&desc, render->u.render.controller_modes[i], &bind))
        {
            ok = false;
            break;
        }
        if (bind.uses_volume_set1)
            bind.volume_depth_texture_id = sampled_depth_id;
        if (bind.uses_common_set0)
            vis_bg_set0 = bind.uses_fixed_common ? fixed_bg_id : apply_bg_id;
        if (bind.uses_shading_set1)
        {
            bool shading_bgl_new = false;
            uint64_t shading_bgl_id = _obj_id(emitter, "_bgl_prim_shading", &shading_bgl_new);
            if (shading_bgl_id == 0) { ok = false; break; }
            if (shading_bgl_new)
                ok = ok && dvz_drp2_stream_create_uniform_bind_group_layout(
                               stream, shading_bgl_id);
            char shading_bg_key[64];
            dvz_snprintf(
                shading_bg_key, sizeof(shading_bg_key), "_bg_prim_shading_%" PRIu64,
                bind.shading_buffer_id);
            uint64_t shading_bg_id = _obj_id(emitter, shading_bg_key, &is_new);
            if (shading_bg_id == 0) { ok = false; break; }
            if (ok && is_new)
                ok = ok && dvz_drp2_stream_create_uniform_bind_group(
                               stream, shading_bg_id, shading_bgl_id,
                               bind.shading_buffer_id, 0,
                               sizeof(DvzPrimitiveShadingState));
            vis_bg_set1 = shading_bg_id;
        }
        if (bind.uses_image_set1)
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
            dvz_snprintf(
                img_bg_key, sizeof(img_bg_key), "_bg_img_%" PRIu64, bind.image_texture_id);
            uint64_t img_bg_id = _obj_id(emitter, img_bg_key, &is_new);
            if (img_bg_id == 0) { ok = false; break; }
            if (ok && is_new)
                ok = ok && dvz_drp2_stream_create_texture_sampler_bind_group(
                               stream, img_bg_id, img_bgl_id, bind.image_texture_id,
                               img_sampler_id);
            vis_bg_set1 = img_bg_id;
        }
        if (bind.uses_volume_set1)
        {
            if (volume_bgl_id == 0)
            {
                volume_bgl_id = _obj_id(emitter, "_bgl_volume", &is_new);
                if (volume_bgl_id == 0) { ok = false; break; }
                if (is_new)
                    ok = ok && _create_volume_bind_group_layout(stream, volume_bgl_id);
            }
            if (volume_sampler_id == 0)
            {
                volume_sampler_id = _obj_id(emitter, "_sampler_volume", &is_new);
                if (volume_sampler_id == 0) { ok = false; break; }
                if (ok && is_new)
                    ok = ok && dvz_drp2_stream_create_sampler(stream, volume_sampler_id);
            }
            uint64_t volume_bg_id = 0;
            ok = ok && _resolve_volume_bind_group(
                           emitter, stream, volume_bgl_id, volume_sampler_id, &bind,
                           &volume_bg_id);
            vis_bg_set1 = volume_bg_id;
        }

        if (!ok)
            break;

        draws[draw_count].pipeline_id = pipe_id;
        draws[draw_count].bg_set0     = vis_bg_set0;
        draws[draw_count].bg_set1     = vis_bg_set1;
        draws[draw_count].visual      = desc;
        draw_count++;
    }

    if (!ok || draw_count == 0)
        return false;

    *draw_count_out = draw_count;
    return true;
}



/**
 * Emit one panel's already-prepared draws inside an open render pass.
 *
 * @param stream destination DRP2 command stream.
 * @param render render node whose viewport/scissor and visuals are emitted.
 * @param render_pass_id active render-pass id.
 * @param draws prepared draw descriptors.
 * @param draw_count number of prepared draw descriptors.
 * @param cache optional state cache shared across panels in the same render pass.
 * @return true when all draw commands were emitted successfully, false otherwise.
 */
static bool _emitter_emit_render_multi_draws(
    DvzDrp2CommandStream* stream, const DvzFramePlanNode* render, uint64_t render_pass_id,
    const SceneRenderDraw* draws, uint32_t draw_count, SceneRenderStateCache* cache)
{
    ANN(stream);
    ANN(render);
    ANN(draws);

    bool ok = dvz_drp2_stream_set_viewport(
                  stream, render_pass_id, render->u.render.desc.x, render->u.render.desc.y,
                  render->u.render.desc.width, render->u.render.desc.height) &&
              dvz_drp2_stream_set_scissor(
                  stream, render_pass_id, render->u.render.desc.x, render->u.render.desc.y,
                  render->u.render.desc.width, render->u.render.desc.height);

    uint64_t last_pipeline = (cache != NULL) ? cache->pipeline_id : 0;
    uint64_t last_bg_set0 = (cache != NULL) ? cache->bg_set0 : 0;
    uint64_t last_bg_set1 = 0;
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
        if (draws[d].bg_set1 != 0 && draws[d].bg_set1 != last_bg_set1)
        {
            ok = ok &&
                 dvz_drp2_stream_set_bind_group(stream, render_pass_id, 1, draws[d].bg_set1);
            last_bg_set1 = draws[d].bg_set1;
        }
        for (uint32_t j = 0; ok && j < draws[d].visual.vbuf_count; j++)
            ok = ok && dvz_drp2_stream_set_vertex_buffer(
                           stream, render_pass_id, j, draws[d].visual.vbuf_ids[j], 0);
        if (ok && draws[d].visual.index_buffer_id != 0)
        {
            ok = ok &&
                 dvz_drp2_stream_set_index_buffer(
                     stream, render_pass_id, draws[d].visual.index_buffer_id,
                     draws[d].visual.index_format, 0) &&
                 dvz_drp2_stream_draw_indexed(
                     stream, render_pass_id, draws[d].visual.index_count, 1, 0, 0, 0);
        }
        else
        {
            ok = ok &&
                 dvz_drp2_stream_draw(
                     stream, render_pass_id, draws[d].visual.vertex_count, 1, 0, 0);
        }
    }

    if (cache != NULL)
    {
        cache->pipeline_id = last_pipeline;
        cache->bg_set0 = last_bg_set0;
    }

    return ok;
}


/**
 * Return the configured render-target extent, falling back to fixture dimensions.
 *
 * @param cfg optional frame-plan emit configuration.
 * @param width output width in pixels.
 * @param height output height in pixels.
 */
static void _emit_target_extent(
    const DvzFramePlanEmitConfig* cfg, uint32_t* width, uint32_t* height)
{
    ANN(width);
    ANN(height);
    *width = (cfg != NULL && cfg->target_width > 0) ? cfg->target_width : 4;
    *height = (cfg != NULL && cfg->target_height > 0) ? cfg->target_height : 4;
}



/**
 * Return a graph resource descriptor by id.
 *
 * @param plan the FramePlan.
 * @param resource_id the graph resource id.
 * @return the resource descriptor, or NULL when absent.
 */
static const DvzFrameGraphResource*
_graph_resource_by_id(const DvzFramePlan* plan, const char* resource_id)
{
    ANN(plan);
    ANN(resource_id);
    for (uint32_t i = 0; i < dvz_frame_plan_graph_resource_count(plan); i++)
    {
        const DvzFrameGraphResource* resource = dvz_frame_plan_graph_resource_get(plan, i);
        if (resource != NULL && strcmp(resource->id, resource_id) == 0)
            return resource;
    }
    return NULL;
}



/**
 * Return a graph pass descriptor by panel and work label.
 *
 * @param plan the FramePlan.
 * @param panel_id the panel id.
 * @param work_label the graph pass work label.
 * @return the graph pass descriptor, or NULL when absent.
 */
static const DvzFrameGraphPass* _graph_pass_by_panel_work(
    const DvzFramePlan* plan, const char* panel_id, const char* work_label)
{
    ANN(plan);
    ANN(panel_id);
    ANN(work_label);
    for (uint32_t i = 0; i < dvz_frame_plan_graph_pass_count(plan); i++)
    {
        const DvzFrameGraphPass* pass = dvz_frame_plan_graph_pass_get(plan, i);
        if (pass != NULL && strcmp(pass->panel_id, panel_id) == 0 &&
            strcmp(pass->work_label, work_label) == 0)
            return pass;
    }
    return NULL;
}



/**
 * Return the graph work label used for a render pass role.
 *
 * @param role the FramePlan render pass role.
 * @return the graph work label.
 */
static const char* _graph_work_label_for_render_role(DvzFramePlanRenderPassRole role)
{
    switch (role)
    {
    case DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE:
        return "opaque";
    case DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION:
        return "wboit_accum";
    case DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND:
        return "transparent_blend";
    case DVZ_FRAME_PLAN_RENDER_PASS_WBOIT_RESOLVE:
        return "wboit_resolve";
    case DVZ_FRAME_PLAN_RENDER_PASS_PICKING:
        return "picking";
    default:
        return "";
    }
}



/**
 * Return the graph pass associated with a render node.
 *
 * @param plan the FramePlan.
 * @param render render node.
 * @return the graph pass descriptor, or NULL when absent.
 */
static const DvzFrameGraphPass*
_graph_pass_for_render(const DvzFramePlan* plan, const DvzFramePlanNode* render)
{
    ANN(plan);
    ANN(render);
    if (render->type != DVZ_FRAME_PLAN_NODE_RENDER)
        return NULL;
    const char* work_label = _graph_work_label_for_render_role(render->u.render.pass_role);
    if (work_label[0] == '\0')
        return NULL;
    return _graph_pass_by_panel_work(plan, render->u.render.panel_id, work_label);
}



/**
 * Return the render node associated with a graph pass descriptor.
 *
 * @param plan the FramePlan.
 * @param pass graph pass descriptor.
 * @return the matching render node, or NULL when absent.
 */
static const DvzFramePlanNode*
_graph_render_for_pass(const DvzFramePlan* plan, const DvzFrameGraphPass* pass)
{
    ANN(plan);
    ANN(pass);
    for (uint32_t i = 0; i < plan->count; i++)
    {
        const DvzFramePlanNode* render = &plan->nodes[i];
        if (render->type != DVZ_FRAME_PLAN_NODE_RENDER)
            continue;
        const char* work_label = _graph_work_label_for_render_role(render->u.render.pass_role);
        if (work_label[0] != '\0' && strcmp(render->u.render.panel_id, pass->panel_id) == 0 &&
            strcmp(work_label, pass->work_label) == 0)
            return render;
    }
    return NULL;
}



/**
 * Convert graph texture usage flags to DRP2 texture usage flags.
 *
 * @param usage_flags graph resource usage flags.
 * @return DRP2 texture usage flags.
 */
static uint32_t _graph_texture_usage_to_drp2(uint32_t usage_flags)
{
    uint32_t out = 0;
    if ((usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT) != 0 ||
        (usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_DEPTH_ATTACHMENT) != 0)
        out |= DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT;
    if ((usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED) != 0)
        out |= DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING;
    if ((usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_STORAGE) != 0)
        out |= DVZ_DRP2_TEXTURE_USAGE_STORAGE_BINDING;
    if ((usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_COPY_SRC) != 0)
        out |= DVZ_DRP2_TEXTURE_USAGE_COPY_SRC;
    if ((usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_COPY_DST) != 0)
        out |= DVZ_DRP2_TEXTURE_USAGE_COPY_DST;
    return out != 0 ? out : DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT;
}



/**
 * Convert one declared graph access into DRP2 texture usage flags.
 *
 * @param usage graph pass access usage.
 * @return DRP2 texture usage flags.
 */
static uint32_t _graph_access_usage_to_drp2(DvzFrameGraphAccessUsage usage)
{
    switch (usage)
    {
    case DVZ_FRAME_GRAPH_ACCESS_SAMPLED:
        return DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING;
    case DVZ_FRAME_GRAPH_ACCESS_STORAGE_READ:
    case DVZ_FRAME_GRAPH_ACCESS_STORAGE_WRITE:
        return DVZ_DRP2_TEXTURE_USAGE_STORAGE_BINDING;
    case DVZ_FRAME_GRAPH_ACCESS_COLOR_ATTACHMENT:
    case DVZ_FRAME_GRAPH_ACCESS_DEPTH_ATTACHMENT_READ:
    case DVZ_FRAME_GRAPH_ACCESS_DEPTH_ATTACHMENT_WRITE:
        return DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT;
    case DVZ_FRAME_GRAPH_ACCESS_COPY_SRC:
        return DVZ_DRP2_TEXTURE_USAGE_COPY_SRC;
    case DVZ_FRAME_GRAPH_ACCESS_COPY_DST:
        return DVZ_DRP2_TEXTURE_USAGE_COPY_DST;
    default:
        return 0;
    }
}



/**
 * Return the graph access implied by a depth attachment declaration.
 *
 * @param attachment graph attachment descriptor.
 * @return graph access usage.
 */
static DvzFrameGraphAccessUsage
_graph_depth_attachment_usage(const DvzFrameGraphAttachment* attachment)
{
    ANN(attachment);
    if (attachment->access == DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ)
        return DVZ_FRAME_GRAPH_ACCESS_DEPTH_ATTACHMENT_READ;
    if (attachment->access == DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_NONE)
        return DVZ_FRAME_GRAPH_ACCESS_NONE;
    return DVZ_FRAME_GRAPH_ACCESS_DEPTH_ATTACHMENT_WRITE;
}



/**
 * Compute DRP2 texture usage from all graph pass access declarations for a resource.
 *
 * @param plan the FramePlan.
 * @param resource_id graph resource id.
 * @return DRP2 texture usage flags implied by graph passes.
 */
static uint32_t
_graph_declared_texture_usage_to_drp2(const DvzFramePlan* plan, const char* resource_id)
{
    ANN(plan);
    ANN(resource_id);
    uint32_t usage = 0;
    for (uint32_t i = 0; i < dvz_frame_plan_graph_pass_count(plan); i++)
    {
        const DvzFrameGraphPass* pass = dvz_frame_plan_graph_pass_get(plan, i);
        if (pass == NULL)
            continue;
        for (uint32_t j = 0; j < pass->read_count; j++)
        {
            if (strcmp(pass->reads[j].resource_id, resource_id) == 0)
                usage |= _graph_access_usage_to_drp2(pass->reads[j].usage);
        }
        for (uint32_t j = 0; j < pass->write_count; j++)
        {
            if (strcmp(pass->writes[j].resource_id, resource_id) == 0)
                usage |= _graph_access_usage_to_drp2(pass->writes[j].usage);
        }
        for (uint32_t j = 0; j < pass->color_attachment_count; j++)
        {
            if (strcmp(pass->color_attachments[j].resource_id, resource_id) == 0)
                usage |= _graph_access_usage_to_drp2(
                    DVZ_FRAME_GRAPH_ACCESS_COLOR_ATTACHMENT);
        }
        if (pass->has_depth_attachment &&
            strcmp(pass->depth_attachment.resource_id, resource_id) == 0)
        {
            usage |= _graph_access_usage_to_drp2(
                _graph_depth_attachment_usage(&pass->depth_attachment));
        }
    }
    return usage;
}



/**
 * Convert a graph attachment load operation to a DRP2 attachment load operation.
 *
 * @param op graph attachment load operation.
 * @return DRP2 attachment load operation.
 */
static DvzDrp2AttachmentLoadOp _graph_load_op_to_drp2(DvzFrameGraphAttachmentLoadOp op)
{
    switch (op)
    {
    case DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR:
        return DVZ_DRP2_ATTACHMENT_LOAD_CLEAR;
    case DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD:
        return DVZ_DRP2_ATTACHMENT_LOAD_LOAD;
    case DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_DONT_CARE:
        return DVZ_DRP2_ATTACHMENT_LOAD_DONT_CARE;
    default:
        return DVZ_DRP2_ATTACHMENT_LOAD_LOAD;
    }
}



/**
 * Convert a graph attachment store operation to a DRP2 attachment store operation.
 *
 * @param op graph attachment store operation.
 * @return DRP2 attachment store operation.
 */
static DvzDrp2AttachmentStoreOp _graph_store_op_to_drp2(DvzFrameGraphAttachmentStoreOp op)
{
    switch (op)
    {
    case DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE:
        return DVZ_DRP2_ATTACHMENT_STORE_STORE;
    case DVZ_FRAME_GRAPH_ATTACHMENT_STORE_DONT_CARE:
        return DVZ_DRP2_ATTACHMENT_STORE_DONT_CARE;
    default:
        return DVZ_DRP2_ATTACHMENT_STORE_STORE;
    }
}



/**
 * Convert graph attachment access to DRP2 attachment access.
 *
 * @param access graph attachment access.
 * @return DRP2 attachment access.
 */
static DvzDrp2AttachmentAccess _graph_attachment_access_to_drp2(
    DvzFrameGraphAttachmentAccess access)
{
    switch (access)
    {
    case DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ:
        return DVZ_DRP2_ATTACHMENT_ACCESS_READ;
    case DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ_WRITE:
        return DVZ_DRP2_ATTACHMENT_ACCESS_READ_WRITE;
    case DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_NONE:
    case DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE:
    default:
        return DVZ_DRP2_ATTACHMENT_ACCESS_WRITE;
    }
}



/**
 * Apply graph color attachment load/store operations to the current DRP2 render pass command.
 *
 * @param stream destination DRP2 command stream.
 * @param pass graph pass descriptor.
 * @return whether the command was updated.
 */
static bool _stream_apply_graph_color_ops(
    DvzDrp2CommandStream* stream, const DvzFrameGraphPass* pass)
{
    ANN(stream);
    if (pass == NULL)
        return true;
    bool ok = true;
    for (uint32_t i = 0; ok && i < pass->color_attachment_count; i++)
    {
        const DvzFrameGraphAttachment* attachment = &pass->color_attachments[i];
        ok = dvz_drp2_stream_begin_render_pass_set_color_attachment_ops(
            stream, i, _graph_load_op_to_drp2(attachment->load_op),
            _graph_store_op_to_drp2(attachment->store_op)) &&
             dvz_drp2_stream_begin_render_pass_set_color_attachment_access(
                 stream, i, _graph_attachment_access_to_drp2(attachment->access));
    }
    return ok;
}



/**
 * Apply graph depth attachment state to the current DRP2 render pass command.
 *
 * @param stream destination DRP2 command stream.
 * @param pass graph pass descriptor.
 * @param depth_id named depth texture id, or zero for no graph depth.
 * @return whether the command was updated.
 */
static bool _stream_apply_graph_depth(
    DvzDrp2CommandStream* stream, const DvzFrameGraphPass* pass, uint64_t depth_id)
{
    ANN(stream);
    if (pass == NULL || !pass->has_depth_attachment || depth_id == 0)
        return true;
    const DvzFrameGraphAttachment* attachment = &pass->depth_attachment;
    return dvz_drp2_stream_begin_render_pass_set_depth_texture(
               stream, depth_id, attachment->clear_depth) &&
           dvz_drp2_stream_begin_render_pass_set_depth_ops(
               stream, _graph_load_op_to_drp2(attachment->load_op),
               _graph_store_op_to_drp2(attachment->store_op)) &&
           dvz_drp2_stream_begin_render_pass_set_depth_access(
               stream, _graph_attachment_access_to_drp2(attachment->access));
}



/**
 * Resolve or create one runtime 2D texture.
 *
 * @param emitter the persistent emitter.
 * @param stream destination DRP2 command stream.
 * @param key persistent resource key.
 * @param width texture width in pixels.
 * @param height texture height in pixels.
 * @param format Vulkan texture format.
 * @param usage DRP2 texture usage flags.
 * @param out_id output texture id.
 * @return whether the texture id is available.
 */
static bool _runtime_resolve_texture_2d(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const char* key, uint32_t width,
    uint32_t height, uint32_t format, uint32_t usage, uint64_t* out_id)
{
    ANN(emitter);
    ANN(stream);
    ANN(key);
    ANN(out_id);

    bool is_new = false;
    ResourceId* resource = _resource_entry(&emitter->resources, key, &is_new);
    if (resource == NULL)
        return false;
    if (!_resource_ensure_texture_2d(&emitter->resources, resource, width, height, &is_new))
        return false;

    if (is_new)
    {
        if (!dvz_drp2_stream_create_texture_2d_format_usage(
                stream, resource->id, width, height, format, usage))
            return false;
    }
    *out_id = resource->id;
    return true;
}



/**
 * Resolve or create one graph-declared 2D texture resource.
 *
 * @param emitter the persistent emitter.
 * @param stream destination DRP2 command stream.
 * @param plan the FramePlan carrying access declarations.
 * @param resource graph resource descriptor.
 * @param width texture width in pixels.
 * @param height texture height in pixels.
 * @param fallback_format fallback Vulkan format when the graph format is zero.
 * @param out_id output texture id.
 * @return whether the texture id is available.
 */
static bool _graph_resolve_texture_2d(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFrameGraphResource* resource, uint32_t width, uint32_t height,
    uint32_t fallback_format, uint64_t* out_id)
{
    ANN(resource);
    uint32_t format = resource->format != 0 ? resource->format : fallback_format;
    uint32_t usage = _graph_texture_usage_to_drp2(resource->usage_flags);
    if (plan != NULL)
        usage |= _graph_declared_texture_usage_to_drp2(plan, resource->id);
    return _runtime_resolve_texture_2d(
        emitter, stream, resource->id, width, height, format, usage, out_id);
}



/**
 * Resolve the named graph depth texture for a render node.
 *
 * @param emitter the persistent emitter.
 * @param stream destination DRP2 command stream.
 * @param plan the FramePlan.
 * @param render render node.
 * @param cfg optional frame-plan emit configuration.
 * @param graph_pass output graph pass descriptor, or NULL.
 * @param out_depth_id output depth texture id, or zero when no graph depth exists.
 * @return whether graph depth resolution succeeded.
 */
static bool _graph_resolve_render_depth(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanNode* render, const DvzFramePlanEmitConfig* cfg,
    const DvzFrameGraphPass** graph_pass, uint64_t* out_depth_id)
{
    ANN(emitter);
    ANN(stream);
    ANN(plan);
    ANN(render);
    ANN(graph_pass);
    ANN(out_depth_id);

    *graph_pass = _graph_pass_for_render(plan, render);
    *out_depth_id = 0;
    if (*graph_pass == NULL || !(*graph_pass)->has_depth_attachment)
        return true;

    const DvzFrameGraphResource* depth_resource =
        _graph_resource_by_id(plan, (*graph_pass)->depth_attachment.resource_id);
    if (depth_resource == NULL)
        return true;

    uint32_t width = 0;
    uint32_t height = 0;
    _emit_target_extent(cfg, &width, &height);
    return _graph_resolve_texture_2d(
        emitter, stream, plan, depth_resource, width, height, VK_FORMAT_D32_SFLOAT, out_depth_id);
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
static bool _emitter_prepare_wboit_targets(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanNode* render, uint64_t color_id, const DvzFramePlanEmitConfig* cfg,
    SceneWboitTargets* out)
{
    ANN(emitter);
    ANN(stream);
    ANN(render);
    ANN(out);

    uint32_t width = 0;
    uint32_t height = 0;
    _emit_target_extent(cfg, &width, &height);

    bool ok = true;
    bool is_new = false;
    out->color_id = color_id;

    char accum_key[DVZ_SCENE_LABEL_SIZE];
    char weight_key[DVZ_SCENE_LABEL_SIZE];
    dvz_snprintf(accum_key, sizeof(accum_key), "_wboit_accum_%s", render->u.render.panel_id);
    dvz_snprintf(weight_key, sizeof(weight_key), "_wboit_weight_%s", render->u.render.panel_id);

    const DvzFrameGraphPass* graph_pass = _graph_pass_for_render(plan, render);
    const DvzFrameGraphResource* accum_resource = NULL;
    const DvzFrameGraphResource* weight_resource = NULL;
    const DvzFrameGraphResource* depth_resource = NULL;
    if (graph_pass != NULL && graph_pass->color_attachment_count >= 2)
    {
        accum_resource = _graph_resource_by_id(
            plan, graph_pass->color_attachments[0].resource_id);
        weight_resource = _graph_resource_by_id(
            plan, graph_pass->color_attachments[1].resource_id);
        if (graph_pass->has_depth_attachment)
            depth_resource = _graph_resource_by_id(plan, graph_pass->depth_attachment.resource_id);
    }

    uint32_t fallback_usage =
        DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING;

    ok = ok && (accum_resource != NULL
                    ? _graph_resolve_texture_2d(
                          emitter, stream, plan, accum_resource, width, height,
                          VK_FORMAT_R16G16B16A16_SFLOAT, &out->accum_id)
                    : _runtime_resolve_texture_2d(
                          emitter, stream, accum_key, width, height,
                          VK_FORMAT_R16G16B16A16_SFLOAT, fallback_usage, &out->accum_id));
    ok = ok && (weight_resource != NULL
                    ? _graph_resolve_texture_2d(
                          emitter, stream, plan, weight_resource, width, height,
                          VK_FORMAT_R16_SFLOAT, &out->weight_id)
                    : _runtime_resolve_texture_2d(
                          emitter, stream, weight_key, width, height, VK_FORMAT_R16_SFLOAT,
                          fallback_usage, &out->weight_id));
    if (!ok)
        return false;
    if (depth_resource != NULL)
    {
        ok = _graph_resolve_texture_2d(
            emitter, stream, plan, depth_resource, width, height, VK_FORMAT_D32_SFLOAT,
            &out->depth_id);
    }
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
        ok = ok &&
             dvz_drp2_stream_create_bind_group_layout_entries(
                 stream, out->resolve_bgl_id, 3, entries);
    }

    char bg_key[96];
    dvz_snprintf(
        bg_key, sizeof(bg_key), "_bg_wboit_%" PRIu64 "_%" PRIu64, out->accum_id,
        out->weight_id);
    out->resolve_bg_id = _obj_id(emitter, bg_key, &is_new);
    if (out->resolve_bg_id == 0)
        return false;
    if (ok && is_new)
    {
        DvzDrp2BindGroupEntry entries[3] = {
            {
                .binding = 0,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
                .resource_id = out->accum_id,
            },
            {
                .binding = 1,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
                .resource_id = out->weight_id,
            },
            {
                .binding = 2,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_SAMPLER,
                .resource_id = out->sampler_id,
            },
        };
        ok = ok &&
             dvz_drp2_stream_create_bind_group_entries(
                 stream, out->resolve_bg_id, out->resolve_bgl_id, 3, entries);
    }

    const char* fmt = _shader_format_tag(cfg);
    char vs_key[32];
    char fs_key[32];
    char pipe_key[48];
    dvz_snprintf(vs_key, sizeof(vs_key), "_vs_wboit_resolve%s", fmt);
    dvz_snprintf(fs_key, sizeof(fs_key), "_fs_wboit_resolve%s", fmt);
    dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe_wboit_resolve%s", fmt);

    uint64_t vs_id = _obj_id(emitter, vs_key, &is_new);
    if (vs_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && _emit_shader(
                       stream, vs_id, "VERTEX", NULL,
                       _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_WBOIT_RESOLVE, false),
                       cfg);

    uint64_t fs_id = _obj_id(emitter, fs_key, &is_new);
    if (fs_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && _emit_shader(
                       stream, fs_id, "FRAGMENT", NULL,
                       _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_WBOIT_RESOLVE, true),
                       cfg);

    out->resolve_pipeline_id = _obj_id(emitter, pipe_key, &is_new);
    if (out->resolve_pipeline_id == 0)
        return false;
    if (ok && is_new)
    {
        ok = ok &&
             dvz_drp2_stream_create_render_pipeline_with_bind_group_layout(
                 stream, out->resolve_pipeline_id, vs_id, fs_id, 0, out->resolve_bgl_id) &&
             dvz_drp2_stream_pipeline_set_color_blend(
                 stream, 0, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                 VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                 VK_BLEND_OP_ADD,
                 VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                     VK_COLOR_COMPONENT_A_BIT);
    }
    return ok;
}



/**
 * Return WBOIT targets associated with a panel id.
 *
 * @param targets target array.
 * @param renders render-node array parallel to targets.
 * @param count target count.
 * @param panel_id panel id to find.
 * @return target entry, or NULL when absent.
 */
static const SceneWboitTargets* _wboit_targets_for_panel(
    const SceneWboitTargets* targets, const DvzFramePlanNode* const* renders, uint32_t count,
    const char* panel_id)
{
    ANN(targets);
    ANN(renders);
    ANN(panel_id);

    for (uint32_t i = 0; i < count; i++)
    {
        if (renders[i] != NULL && strcmp(renders[i]->u.render.panel_id, panel_id) == 0)
            return &targets[i];
    }
    return NULL;
}



/**
 * Return the prepared draw batch for a render node.
 *
 * @param batches prepared render batches.
 * @param count number of prepared render batches.
 * @param render render node.
 * @return the matching batch, or NULL when no draws were prepared.
 */
static const SceneRenderBatch* _render_batch_for_node(
    const SceneRenderBatch* batches, uint32_t count, const DvzFramePlanNode* render)
{
    ANN(render);
    for (uint32_t i = 0; i < count; i++)
    {
        if (batches[i].render == render)
            return &batches[i];
    }
    return NULL;
}



/**
 * Emit a WBOIT resolve pass into the final color target.
 *
 * @param stream destination DRP2 command stream.
 * @param render resolve render node.
 * @param render_pass_id active render-pass id.
 * @param targets WBOIT target ids.
 * @return whether all commands were emitted.
 */
static bool _emitter_emit_wboit_resolve(
    DvzDrp2CommandStream* stream, const DvzFramePlanNode* render, uint64_t render_pass_id,
    const SceneWboitTargets* targets)
{
    ANN(stream);
    ANN(render);
    ANN(targets);

    return dvz_drp2_stream_set_viewport(
               stream, render_pass_id, render->u.render.desc.x, render->u.render.desc.y,
               render->u.render.desc.width, render->u.render.desc.height) &&
           dvz_drp2_stream_set_scissor(
               stream, render_pass_id, render->u.render.desc.x, render->u.render.desc.y,
               render->u.render.desc.width, render->u.render.desc.height) &&
           dvz_drp2_stream_set_pipeline(stream, render_pass_id, targets->resolve_pipeline_id) &&
           dvz_drp2_stream_set_bind_group(stream, render_pass_id, 0, targets->resolve_bg_id) &&
           dvz_drp2_stream_draw(stream, render_pass_id, 3, 1, 0, 0);
}



/* Scene render path: one BeginRenderPass per panel, one Draw per visual inside it. */
static bool _emitter_emit_render_multi(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanNode* render, const DvzFramePlanNode* readback, bool clear,
    const DvzFramePlanEmitConfig* cfg, SceneRenderStateCache* cache, DvzDiagnosticReport* report)
{
    ANN(emitter);
    ANN(stream);
    ANN(plan);
    ANN(render);

    uint64_t color_id = 0;
    if (!_render_pass_resolve_color_target(emitter, stream, cfg, &color_id))
        return false;

    uint64_t rb_id = 0;
    if (!_render_pass_resolve_readback_buffer(emitter, stream, readback, &rb_id))
        return false;

    bool ok = true;

    uint64_t encoder_id = 0;
    uint64_t render_pass_id = 0;
    uint64_t command_buffer_id = 0;
    uint64_t submission_id = 0;
    _render_pass_next_ids(
        emitter, &encoder_id, &render_pass_id, &command_buffer_id, &submission_id);

    float cr = cfg ? cfg->clear_color[0] : 0.0f;
    float cg = cfg ? cfg->clear_color[1] : 0.0f;
    float cb = cfg ? cfg->clear_color[2] : 0.0f;
    float ca = cfg ? cfg->clear_color[3] : 1.0f;
    bool needs_depth = _scene_render_needs_depth(emitter, render);
    const DvzFrameGraphPass* graph_pass = NULL;
    uint64_t graph_depth_id = 0;
    if (!_graph_resolve_render_depth(
            emitter, stream, plan, render, cfg, &graph_pass, &graph_depth_id))
        return false;
    uint64_t sampled_depth_id =
        graph_pass != NULL && graph_pass->has_depth_attachment &&
                graph_pass->depth_attachment.access == DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ
            ? graph_depth_id
            : 0;

    SceneRenderDraw draws[DVZ_SCENE_MAX_RENDER_VISUALS] = {0};
    uint32_t draw_count = 0;
    ok = _emitter_prepare_render_multi(
        emitter, stream, render, cfg, sampled_depth_id, report, draws, &draw_count);
    if (!ok)
        return false;

    ok = dvz_drp2_stream_begin_command_encoder(stream, encoder_id) &&
         dvz_drp2_stream_begin_render_pass_region_clear(
             stream, render_pass_id, encoder_id, color_id, cr, cg, cb, ca, 0.0f, 0.0f, 1.0f,
             1.0f, clear);
    ok = ok && _stream_apply_graph_color_ops(stream, graph_pass);
    ok = ok && _stream_apply_graph_depth(stream, graph_pass, graph_depth_id);
    if (ok && needs_depth && graph_depth_id == 0)
        ok = dvz_drp2_stream_begin_render_pass_set_depth(stream, 1.0f);
    ok = ok &&
         _emitter_emit_render_multi_draws(
             stream, render, render_pass_id, draws, draw_count, cache) &&
         dvz_drp2_stream_end_render_pass(stream, render_pass_id);
    ok = ok && _render_pass_copy_finish_submit(
                   stream, encoder_id, command_buffer_id, submission_id, color_id, rb_id,
                   readback);
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
    const DvzFramePlanNode* readback, const DvzFramePlanEmitConfig* cfg,
    DvzDiagnosticReport* report)
{
    ANN(emitter);
    ANN(stream);
    ANN(plan);

    uint64_t color_id = 0;
    if (!_render_pass_resolve_color_target(emitter, stream, cfg, &color_id))
        return false;

    uint64_t rb_id = 0;
    if (!_render_pass_resolve_readback_buffer(emitter, stream, readback, &rb_id))
        return false;

    bool ok = true;

    uint64_t encoder_id = 0;
    uint64_t render_pass_id = 0;
    uint64_t command_buffer_id = 0;
    uint64_t submission_id = 0;
    _render_pass_next_ids(
        emitter, &encoder_id, &render_pass_id, &command_buffer_id, &submission_id);

    float cr = cfg ? cfg->clear_color[0] : 0.0f;
    float cg = cfg ? cfg->clear_color[1] : 0.0f;
    float cb = cfg ? cfg->clear_color[2] : 0.0f;
    float ca = cfg ? cfg->clear_color[3] : 1.0f;

    SceneRenderBatch* batches =
        (SceneRenderBatch*)dvz_calloc(plan->count, sizeof(SceneRenderBatch));
    if (batches == NULL)
        return false;
    uint32_t batch_count = 0;
    for (uint32_t i = 0; ok && i < plan->count; i++)
    {
        const DvzFramePlanNode* render = &plan->nodes[i];
        if (render->type != DVZ_FRAME_PLAN_NODE_RENDER || render->u.render.visual_count == 0)
            continue;
        SceneRenderBatch* batch = &batches[batch_count];
        batch->render = render;
        ok = _emitter_prepare_render_multi(
            emitter, stream, render, cfg, 0, report, batch->draws, &batch->draw_count);
        if (ok)
            batch_count++;
    }
    if (!ok || batch_count == 0)
    {
        dvz_free(batches);
        return false;
    }

    ok = dvz_drp2_stream_begin_command_encoder(stream, encoder_id) &&
         dvz_drp2_stream_begin_render_pass_region_clear(
             stream, render_pass_id, encoder_id, color_id, cr, cg, cb, ca, 0.0f, 0.0f, 1.0f,
             1.0f, true);

    SceneRenderStateCache scene_cache = {0};
    for (uint32_t i = 0; ok && i < batch_count; i++)
    {
        ok = _emitter_emit_render_multi_draws(
            stream, batches[i].render, render_pass_id, batches[i].draws, batches[i].draw_count,
            &scene_cache);
    }

    ok = ok && dvz_drp2_stream_end_render_pass(stream, render_pass_id);
    ok = ok && _render_pass_copy_finish_submit(
                   stream, encoder_id, command_buffer_id, submission_id, color_id, rb_id,
                   readback);
    dvz_free(batches);
    return ok;
}


/**
 * Return whether the plan contains WBOIT render-pass roles.
 *
 * @param plan the FramePlan.
 * @return whether transparent accumulation or WBOIT resolve nodes are present.
 */
static bool _plan_has_wboit_roles(const DvzFramePlan* plan)
{
    ANN(plan);
    for (uint32_t i = 0; i < plan->count; i++)
    {
        const DvzFramePlanNode* node = &plan->nodes[i];
        if (node->type != DVZ_FRAME_PLAN_NODE_RENDER)
            continue;
        if (node->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION ||
            node->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND ||
            node->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_WBOIT_RESOLVE)
            return true;
    }
    return false;
}



/**
 * Emit scene render nodes with WBOIT accumulation and resolve passes.
 *
 * @param emitter the persistent emitter.
 * @param stream destination DRP2 command stream.
 * @param plan the FramePlan.
 * @param readback optional readback copy node.
 * @param cfg frame-plan emit configuration.
 * @param report diagnostic report receiving recoverable emission errors.
 * @return whether the commands were emitted.
 */
static bool _emitter_emit_scene_wboit_renders(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanNode* readback, const DvzFramePlanEmitConfig* cfg,
    DvzDiagnosticReport* report)
{
    ANN(emitter);
    ANN(stream);
    ANN(plan);

    uint64_t color_id = 0;
    if (!_render_pass_resolve_color_target(emitter, stream, cfg, &color_id))
        return false;

    uint64_t rb_id = 0;
    if (!_render_pass_resolve_readback_buffer(emitter, stream, readback, &rb_id))
        return false;

    SceneRenderBatch* batches =
        (SceneRenderBatch*)dvz_calloc(plan->count, sizeof(SceneRenderBatch));
    SceneWboitTargets* wboit_targets =
        (SceneWboitTargets*)dvz_calloc(plan->count, sizeof(SceneWboitTargets));
    const DvzFramePlanNode** wboit_renders =
        (const DvzFramePlanNode**)dvz_calloc(plan->count, sizeof(DvzFramePlanNode*));
    if (batches == NULL || wboit_targets == NULL || wboit_renders == NULL)
    {
        dvz_free(wboit_renders);
        dvz_free(wboit_targets);
        dvz_free(batches);
        return false;
    }

    bool ok = true;
    uint32_t batch_count = 0;
    uint32_t target_count = 0;
    for (uint32_t i = 0; ok && i < plan->count; i++)
    {
        const DvzFramePlanNode* render = &plan->nodes[i];
        if (render->type != DVZ_FRAME_PLAN_NODE_RENDER)
            continue;
        if (render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_WBOIT_RESOLVE)
            continue;

        uint64_t sampled_depth_id = 0;
        if (render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION)
        {
            ok = _emitter_prepare_wboit_targets(
                emitter, stream, plan, render, color_id, cfg, &wboit_targets[target_count]);
            if (ok)
            {
                sampled_depth_id = wboit_targets[target_count].depth_id;
                wboit_renders[target_count++] = render;
            }
        }
        else if (render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND)
        {
            const DvzFrameGraphPass* graph_pass = NULL;
            ok = _graph_resolve_render_depth(
                emitter, stream, plan, render, cfg, &graph_pass,
                &wboit_targets[target_count].depth_id);
            if (ok)
            {
                wboit_targets[target_count].color_id = color_id;
                sampled_depth_id = wboit_targets[target_count].depth_id;
                wboit_renders[target_count++] = render;
            }
        }

        if (render->u.render.visual_count > 0)
        {
            SceneRenderBatch* batch = &batches[batch_count];
            batch->render = render;
            ok = _emitter_prepare_render_multi(
                emitter, stream, render, cfg, sampled_depth_id, report, batch->draws,
                &batch->draw_count);
            if (ok)
                batch_count++;
        }
    }
    if (!ok)
    {
        dvz_free(wboit_renders);
        dvz_free(wboit_targets);
        dvz_free(batches);
        return false;
    }

    uint64_t encoder_id = _emitter_next_transient_id(emitter);

    float cr = cfg ? cfg->clear_color[0] : 0.0f;
    float cg = cfg ? cfg->clear_color[1] : 0.0f;
    float cb = cfg ? cfg->clear_color[2] : 0.0f;
    float ca = cfg ? cfg->clear_color[3] : 1.0f;
    bool clear_final = true;
    SceneRenderStateCache scene_cache = {0};

    ok = dvz_drp2_stream_begin_command_encoder(stream, encoder_id);
    bool use_graph_order = dvz_frame_plan_graph_pass_count(plan) > 0;
    uint32_t order_count = use_graph_order ? dvz_frame_plan_graph_pass_count(plan) : plan->count;
    for (uint32_t i = 0; ok && i < order_count; i++)
    {
        const DvzFrameGraphPass* ordered_graph_pass =
            use_graph_order ? dvz_frame_plan_graph_pass_get(plan, i) : NULL;
        const DvzFramePlanNode* render =
            use_graph_order ? _graph_render_for_pass(plan, ordered_graph_pass) : &plan->nodes[i];
        if (render == NULL || render->type != DVZ_FRAME_PLAN_NODE_RENDER)
            continue;

        if (render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE)
        {
            const SceneWboitTargets* targets =
                _wboit_targets_for_panel(
                    wboit_targets, wboit_renders, target_count, render->u.render.panel_id);
            const DvzFrameGraphPass* graph_pass =
                ordered_graph_pass != NULL ? ordered_graph_pass : _graph_pass_for_render(plan, render);
            uint64_t pass_id = _emitter_next_transient_id(emitter);
            const SceneRenderBatch* batch = _render_batch_for_node(batches, batch_count, render);
            bool has_draws = batch != NULL;
            ok = dvz_drp2_stream_begin_render_pass_region_clear(
                     stream, pass_id, encoder_id, color_id, cr, cg, cb, ca, 0.0f, 0.0f,
                     1.0f, 1.0f, clear_final);
            ok = ok && _stream_apply_graph_color_ops(stream, graph_pass);
            if (ok && targets != NULL)
                ok = _stream_apply_graph_depth(stream, graph_pass, targets->depth_id);
            if (ok && has_draws && targets != NULL && targets->depth_id == 0 &&
                _scene_render_needs_depth(emitter, render))
                ok = dvz_drp2_stream_begin_render_pass_set_depth(stream, 1.0f);
            if (ok && has_draws)
            {
                ok = _emitter_emit_render_multi_draws(
                    stream, render, pass_id, batch->draws, batch->draw_count, &scene_cache);
            }
            ok = ok && dvz_drp2_stream_end_render_pass(stream, pass_id);
            clear_final = false;
        }
        else if (render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION)
        {
            const SceneWboitTargets* targets =
                _wboit_targets_for_panel(
                    wboit_targets, wboit_renders, target_count, render->u.render.panel_id);
            if (targets == NULL)
            {
                ok = false;
                break;
            }
            uint64_t pass_id = _emitter_next_transient_id(emitter);
            const DvzFrameGraphPass* graph_pass =
                ordered_graph_pass != NULL ? ordered_graph_pass : _graph_pass_for_render(plan, render);
            ok = dvz_drp2_stream_begin_render_pass_region_clear(
                     stream, pass_id, encoder_id, targets->accum_id, 0.0f, 0.0f, 0.0f, 0.0f,
                     render->u.render.desc.x, render->u.render.desc.y,
                     render->u.render.desc.width, render->u.render.desc.height, true) &&
                 dvz_drp2_stream_begin_render_pass_add_color_attachment(
                     stream, targets->weight_id, 0.0f, 0.0f, 0.0f, 0.0f, true);
            ok = ok && _stream_apply_graph_color_ops(stream, graph_pass);
            ok = ok && _stream_apply_graph_depth(stream, graph_pass, targets->depth_id);
            if (ok && targets->depth_id == 0 && _scene_render_needs_depth(emitter, render))
                ok = dvz_drp2_stream_begin_render_pass_set_depth(stream, 1.0f);
            const SceneRenderBatch* batch = _render_batch_for_node(batches, batch_count, render);
            bool has_draws = batch != NULL;
            if (ok && has_draws)
            {
                scene_cache.pipeline_id = 0;
                scene_cache.bg_set0 = 0;
                ok = _emitter_emit_render_multi_draws(
                    stream, render, pass_id, batch->draws, batch->draw_count, &scene_cache);
            }
            ok = ok && dvz_drp2_stream_end_render_pass(stream, pass_id);
        }
        else if (render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND)
        {
            const SceneWboitTargets* targets =
                _wboit_targets_for_panel(
                    wboit_targets, wboit_renders, target_count, render->u.render.panel_id);
            if (targets == NULL)
            {
                ok = false;
                break;
            }
            uint64_t pass_id = _emitter_next_transient_id(emitter);
            const DvzFrameGraphPass* graph_pass =
                ordered_graph_pass != NULL ? ordered_graph_pass : _graph_pass_for_render(plan, render);
            ok = dvz_drp2_stream_begin_render_pass_region_clear(
                stream, pass_id, encoder_id, color_id, cr, cg, cb, ca,
                render->u.render.desc.x, render->u.render.desc.y, render->u.render.desc.width,
                render->u.render.desc.height, false);
            ok = ok && _stream_apply_graph_color_ops(stream, graph_pass);
            ok = ok && _stream_apply_graph_depth(stream, graph_pass, targets->depth_id);
            const SceneRenderBatch* batch = _render_batch_for_node(batches, batch_count, render);
            bool has_draws = batch != NULL;
            if (ok && has_draws)
            {
                scene_cache.pipeline_id = 0;
                scene_cache.bg_set0 = 0;
                ok = _emitter_emit_render_multi_draws(
                    stream, render, pass_id, batch->draws, batch->draw_count, &scene_cache);
            }
            ok = ok && dvz_drp2_stream_end_render_pass(stream, pass_id);
            clear_final = false;
        }
        else if (render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_WBOIT_RESOLVE)
        {
            const SceneWboitTargets* targets =
                _wboit_targets_for_panel(
                    wboit_targets, wboit_renders, target_count, render->u.render.panel_id);
            if (targets == NULL)
            {
                ok = false;
                break;
            }
            uint64_t pass_id = _emitter_next_transient_id(emitter);
            const DvzFrameGraphPass* graph_pass =
                ordered_graph_pass != NULL ? ordered_graph_pass : _graph_pass_for_render(plan, render);
            ok = dvz_drp2_stream_begin_render_pass_region_clear(
                     stream, pass_id, encoder_id, color_id, 0.0f, 0.0f, 0.0f, 0.0f,
                     render->u.render.desc.x, render->u.render.desc.y,
                     render->u.render.desc.width, render->u.render.desc.height, false) &&
                 _stream_apply_graph_color_ops(stream, graph_pass) &&
                 _emitter_emit_wboit_resolve(stream, render, pass_id, targets) &&
                 dvz_drp2_stream_end_render_pass(stream, pass_id);
        }
    }

    uint64_t command_buffer_id = _emitter_next_transient_id(emitter);
    uint64_t submission_id = _emitter_next_transient_id(emitter);
    ok = ok && _render_pass_copy_finish_submit(
                   stream, encoder_id, command_buffer_id, submission_id, color_id, rb_id,
                   readback);
    dvz_free(wboit_renders);
    dvz_free(wboit_targets);
    dvz_free(batches);
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
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanNode* render, const uint64_t* vertex_buffer_ids, uint32_t vertex_buffer_count,
    const DvzFramePlanNode* readback, bool clear, const DvzFramePlanEmitConfig* cfg,
    SceneRenderStateCache* cache, DvzDiagnosticReport* report)
{
    ANN(emitter);
    ANN(stream);
    ANN(plan);
    ANN(render);

    /* Scene render node: per-visual multi-draw in a single pass. */
    if (vertex_buffer_count == 0 && render->u.render.visual_count > 0 &&
        cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
        return _emitter_emit_render_multi(
            emitter, stream, plan, render, readback, clear, cfg, cache, report);

    /* Generic single-draw path (non-scene nodes, WGSL, or fallback). */
    ANN(vertex_buffer_ids);
    if (vertex_buffer_count == 0)
        return false;

    bool ok = true;
    bool is_new = false;
    const char* fmt = _shader_format_tag(cfg);

    uint32_t visual_type = DVZ_VISUAL_TYPE_NONE;
    if (render->u.render.visual_count == 1 &&
        render->u.render.visual_metadata[0].has_metadata)
        visual_type = render->u.render.visual_metadata[0].visual_type;

    /* Detect point-like visual data (position + color + size attributes). */
    bool is_point = _is_point_visual(&emitter->resources, vertex_buffer_ids, vertex_buffer_count);
    bool is_pixel = is_point && visual_type == DVZ_VISUAL_TYPE_PIXEL;
    bool is_point_like = is_point;
    bool is_primitive =
        !is_point_like && _is_primitive_visual(&emitter->resources, vertex_buffer_ids, vertex_buffer_count);
    uint64_t image_pos = 0, image_uv = 0, image_tex = 0;
    bool is_image = !is_point_like && !is_primitive &&
                    _is_image_visual(&emitter->resources, vertex_buffer_ids, vertex_buffer_count,
                                     &image_pos, &image_uv, &image_tex);

    const char* vs_glsl = NULL;
    const char* fs_glsl = NULL;
    const char* vs_wgsl = NULL;
    const char* fs_wgsl = NULL;
    uint32_t topology = 0;
    uint32_t vertex_count = 3; /* default for stub / non-point path */
    uint64_t bgl_id = 0;
    uint64_t bg_id  = 0;
    DvzScenePointLikeLoweringDesc point_like_lowering = {0};
    bool has_point_like_lowering = false;

    /* Common bind group IDs used for GLSL/WGSL point, primitive, and image paths. */
    uint64_t common_bgl_id = 0;
    uint64_t common_bg_id  = 0;
    bool uses_common =
        (is_point || is_primitive || is_image) &&
        cfg != NULL &&
        (cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL ||
         (cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_WGSL &&
          (is_point || is_primitive || is_image)));

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

    if (is_point_like)
    {
        /* Point-like visuals: native points for GLSL, instanced quads for WGSL. */
        DvzSceneBuiltinShader shader =
            is_pixel ? DVZ_SCENE_BUILTIN_SHADER_PIXEL : DVZ_SCENE_BUILTIN_SHADER_POINT;
        const char* key = is_pixel ? "pixel" : "point";
        dvz_snprintf(vs_key, sizeof(vs_key), "_vs_%s%s", key, fmt);
        dvz_snprintf(fs_key, sizeof(fs_key), "_fs_%s%s", key, fmt);
        vs_glsl = _builtin_shader_glsl(shader, false);
        fs_glsl = _builtin_shader_glsl(shader, true);
        vs_wgsl = _builtin_shader_wgsl(shader, false);
        fs_wgsl = _builtin_shader_wgsl(shader, true);

        uint64_t pos_id = _scene_visual_resource_by_role(
            &emitter->resources, vertex_buffer_ids, vertex_buffer_count,
            DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION);
        if (pos_id != 0)
        {
            uint64_t sz = _resource_byte_size(&emitter->resources, pos_id);
            if (sz > 0)
                vertex_count = (uint32_t)(sz / (3 * sizeof(float)));
        }
        DvzSceneShaderFormat shader_format =
            cfg != NULL ? cfg->shader_format : DVZ_SCENE_SHADER_FORMAT_GLSL;
        has_point_like_lowering = _scene_point_like_lowering_desc(
            is_pixel ? DVZ_SCENE_POINT_LIKE_PIXEL : DVZ_SCENE_POINT_LIKE_POINT,
            shader_format, vertex_count, &point_like_lowering);
        if (!has_point_like_lowering)
            return false;
        topology = point_like_lowering.topology;
    }
    else if (is_primitive)
    {
        uint64_t pos_id = _scene_visual_resource_by_role(
            &emitter->resources, vertex_buffer_ids, vertex_buffer_count,
            DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION);
        if (pos_id != 0)
        {
            uint64_t sz = _resource_byte_size(&emitter->resources, pos_id);
            if (sz > 0)
                vertex_count = (uint32_t)(sz / (3 * sizeof(float)));
            topology = _resource_topology(&emitter->resources, pos_id);
        }
        /* Primitive visual: pass-through shaders with visual-selected topology. */
        dvz_snprintf(vs_key, sizeof(vs_key), "_vs_prim%s", fmt);
        dvz_snprintf(fs_key, sizeof(fs_key), "_fs_prim%s", fmt);
        vs_glsl = _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE, false);
        fs_glsl = _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE, true);
        vs_wgsl = _builtin_shader_wgsl(DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE, false);
        fs_wgsl = _builtin_shader_wgsl(DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE, true);
    }
    else if (is_image)
    {
        /* Image visual: textured-quad shaders, TRIANGLE_STRIP topology, 4 vertices. */
        uint64_t pos_size = _resource_byte_size(&emitter->resources, image_pos);
        if (pos_size > 0)
            vertex_count = (uint32_t)(pos_size / (3 * sizeof(float)));
        topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        dvz_snprintf(vs_key, sizeof(vs_key), "_vs_img%s", fmt);
        dvz_snprintf(fs_key, sizeof(fs_key), "_fs_img%s", fmt);
        vs_glsl = _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_IMAGE, false);
        fs_glsl = _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_IMAGE, true);
        vs_wgsl = _builtin_shader_wgsl(DVZ_SCENE_BUILTIN_SHADER_IMAGE, false);
        fs_wgsl = _builtin_shader_wgsl(DVZ_SCENE_BUILTIN_SHADER_IMAGE, true);

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

    /* Common bind group infrastructure. */
    if (uses_common)
    {
        ok = ok && _scene_common_bindings_resolve_single_set(
                       emitter, stream, render, &common_bgl_id, &common_bg_id);
        if (!ok)
            return false;
    }

    /* SPIR-V resource names (stem of .vert.spv / .frag.spv after embed_resources key mangling). */
    const char* vs_spirv_key = NULL;
    const char* fs_spirv_key = NULL;
    if (is_point_like)
    {
        vs_spirv_key = is_pixel ? "pixel_vert" : "point_vert";
        fs_spirv_key = is_pixel ? "pixel_frag" : "point_frag";
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
        if (cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_WGSL && vs_wgsl != NULL)
        {
            ok = ok && _emit_shader(stream, vs_id, "VERTEX", vs_wgsl, vs_glsl, cfg);
        }
        else if (vs_glsl != NULL && vs_spirv_key != NULL &&
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
        if (cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_WGSL && fs_wgsl != NULL)
        {
            ok = ok && _emit_shader(stream, fs_id, "FRAGMENT", fs_wgsl, fs_glsl, cfg);
        }
        else if (fs_glsl != NULL && fs_spirv_key != NULL &&
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
                           stream, fs_id, "FRAGMENT", _fixture_fragment_wgsl(), _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_FIXTURE, true), cfg);
        }
    }

    if (is_point_like)
        dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe_%s%s", is_pixel ? "pixel" : "point", fmt);
    else if (is_primitive)
        dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe_prim_t%u%s", topology, fmt);
    else if (is_image)
        dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe_img%s", fmt);
    else
        dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe%u%s", vertex_buffer_count, fmt);

    uint64_t pipe_id = _obj_id(emitter, pipe_key, &is_new);
    if (pipe_id == 0)
        return false;
    if (ok && is_new)
    {
        if (is_point_like)
        {
            /* Explicit vertex layout: binding0=position(vec3), binding1=color(u8vec4), binding2=size(float) */
            uint32_t strides[3]   = {3*sizeof(float), 4*sizeof(uint8_t), sizeof(float)};
            uint32_t bindings[3]  = {0, 1, 2};
            uint32_t locations[3] = {0, 1, 2};
            uint32_t formats[3]   = {VK_FORMAT_R32G32B32_SFLOAT,
                                     VK_FORMAT_R8G8B8A8_UNORM,
                                     VK_FORMAT_R32_SFLOAT};
            uint32_t offsets[3]   = {0, 0, 0};
            if (point_like_lowering.lowering ==
                DVZ_SCENE_POINT_LIKE_LOWERING_INSTANCED_QUADS)
            {
                uint32_t step_modes[3] = {
                    point_like_lowering.vertex_step_mode,
                    point_like_lowering.vertex_step_mode,
                    point_like_lowering.vertex_step_mode,
                };
                ok = ok && dvz_drp2_stream_create_render_pipeline_ex2(
                               stream, pipe_id, vs_id, fs_id, vertex_buffer_count,
                               topology, 3, strides, step_modes,
                               3, bindings, locations, formats, offsets);
            }
            else
            {
                ok = ok && dvz_drp2_stream_create_render_pipeline_ex(
                               stream, pipe_id, vs_id, fs_id, vertex_buffer_count,
                               topology,
                               3, strides,
                               3, bindings, locations, formats, offsets);
            }
            if (ok && uses_common && common_bgl_id != 0)
                ok = dvz_drp2_stream_pipeline_set_bind_group_layout(stream, common_bgl_id);
        }
        else if (is_primitive)
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
            if (ok && uses_common && common_bgl_id != 0)
                ok = dvz_drp2_stream_pipeline_set_bind_group_layout(stream, common_bgl_id);
        }
        else if (is_image)
        {
            /* binding0=position(vec3), binding1=texcoords(vec2); set0=common, set1=image */
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
            if (ok && uses_common && common_bgl_id != 0)
                ok = dvz_drp2_stream_pipeline_set_bind_group_layout(stream, common_bgl_id);
            if (ok && bgl_id != 0)
                ok = dvz_drp2_stream_pipeline_set_bind_group_layout2(stream, bgl_id);
        }
        else
        {
            ok = ok && dvz_drp2_stream_create_render_pipeline(
                           stream, pipe_id, vs_id, fs_id, vertex_buffer_count);
        }
    }

    uint64_t color_id = 0;
    ok = ok && _render_pass_resolve_color_target(emitter, stream, cfg, &color_id);

    uint64_t rb_id = 0;
    ok = ok && _render_pass_resolve_readback_buffer(emitter, stream, readback, &rb_id);

    if (!ok)
        return false;

    uint64_t encoder_id = 0;
    uint64_t render_pass_id = 0;
    uint64_t command_buffer_id = 0;
    uint64_t submission_id = 0;
    _render_pass_next_ids(
        emitter, &encoder_id, &render_pass_id, &command_buffer_id, &submission_id);

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
    if (ok && uses_common && common_bg_id != 0)
        ok = dvz_drp2_stream_set_bind_group(stream, render_pass_id, 0, common_bg_id);
    if (ok && is_image && bg_id != 0)
        ok = dvz_drp2_stream_set_bind_group(stream, render_pass_id, 1, bg_id);
    for (uint32_t i = 0; ok && i < vertex_buffer_count; i++)
        ok = dvz_drp2_stream_set_vertex_buffer(stream, render_pass_id, i, vertex_buffer_ids[i], 0);
    uint32_t draw_vertex_count = vertex_count;
    uint32_t draw_instance_count = 1;
    if (is_point_like && has_point_like_lowering)
    {
        draw_vertex_count = point_like_lowering.draw_vertex_count;
        draw_instance_count = point_like_lowering.draw_instance_count;
    }
    ok = ok && dvz_drp2_stream_draw(
                   stream, render_pass_id, draw_vertex_count, draw_instance_count, 0, 0) &&
         dvz_drp2_stream_end_render_pass(stream, render_pass_id);
    ok = ok && _render_pass_copy_finish_submit(
                   stream, encoder_id, command_buffer_id, submission_id, color_id, rb_id,
                   readback);
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
    const DvzFramePlanNode* readback, const DvzFramePlanEmitConfig* cfg,
    DvzDiagnosticReport* report)
{
    ANN(emitter);
    ANN(stream);
    ANN(plan);

    if (cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL &&
        _plan_has_wboit_roles(plan))
        return _emitter_emit_scene_wboit_renders(emitter, stream, plan, readback, cfg, report);

    uint32_t render_node_count = 0;
    uint32_t scene_render_node_count = 0;
    bool any_scene_render_needs_depth = false;
    for (uint32_t i = 0; i < plan->count; i++)
    {
        const DvzFramePlanNode* render = &plan->nodes[i];
        if (render->type != DVZ_FRAME_PLAN_NODE_RENDER)
            continue;
        render_node_count++;
        if (render->u.render.visual_count > 0 &&
            cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
        {
            if (_scene_render_visual_has_position_resource(emitter, render, 0))
            {
                scene_render_node_count++;
                any_scene_render_needs_depth =
                    any_scene_render_needs_depth || _scene_render_needs_depth(emitter, render);
            }
        }
    }
    if (dvz_frame_plan_graph_pass_count(plan) == 0 && render_node_count > 0 &&
        render_node_count == scene_render_node_count &&
        !any_scene_render_needs_depth)
        return _emitter_emit_scene_figure_renders(emitter, stream, plan, readback, cfg, report);

    bool ok = true;
    uint32_t render_count = 0;
    SceneRenderStateCache scene_cache = {0};
    bool use_graph_order = dvz_frame_plan_graph_pass_count(plan) > 0;
    uint32_t order_count = use_graph_order ? dvz_frame_plan_graph_pass_count(plan) : plan->count;
    for (uint32_t i = 0; ok && i < order_count; i++)
    {
        const DvzFrameGraphPass* graph_pass =
            use_graph_order ? dvz_frame_plan_graph_pass_get(plan, i) : NULL;
        const DvzFramePlanNode* render =
            use_graph_order ? _graph_render_for_pass(plan, graph_pass) : &plan->nodes[i];
        if (render == NULL || render->type != DVZ_FRAME_PLAN_NODE_RENDER)
            continue;

        uint64_t vertex_buffer_ids[DVZ_SCENE_MAX_NODE_RESOURCES] = {0};
        uint32_t vertex_buffer_count = 0;

        /* Scene render nodes (visual_count > 0 with named resources) skip flat resolution;
         * _emitter_emit_render dispatches to _emitter_emit_render_multi instead. */
        bool is_scene_node = false;
        if (render->u.render.visual_count > 0 &&
            cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
        {
            is_scene_node = _scene_render_visual_has_position_resource(emitter, render, 0);
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
                emitter, stream, plan, render, vertex_buffer_ids, vertex_buffer_count,
                render_count == 0 ? readback : NULL, render_count == 0, cfg,
                is_scene_node ? &scene_cache : NULL, report);
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

    uint64_t color_id = 0;
    if (!_render_pass_resolve_color_target(emitter, stream, cfg, &color_id))
        return false;

    uint64_t rb_id = 0;
    if (!_render_pass_resolve_readback_buffer(emitter, stream, readback, &rb_id))
        return false;

    bool ok = true;

    uint64_t encoder_id = 0;
    uint64_t render_pass_id = 0;
    uint64_t command_buffer_id = 0;
    uint64_t submission_id = 0;
    _render_pass_next_ids(
        emitter, &encoder_id, &render_pass_id, &command_buffer_id, &submission_id);

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
    ok = ok && _render_pass_copy_finish_submit(
                   stream, encoder_id, command_buffer_id, submission_id, color_id, rb_id,
                   readback);
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
                       stream, vs_id, "VERTEX", _texture_vertex_wgsl(),
                       _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_TEXTURE, false), cfg);

    char fs_key[16];
    dvz_snprintf(fs_key, sizeof(fs_key), "_fs_tex%s", fmt);
    uint64_t fs_id = _obj_id(emitter, fs_key, &is_new);
    if (fs_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && _emit_shader(
                       stream, fs_id, "FRAGMENT", _texture_fragment_wgsl(),
                       _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_TEXTURE, true), cfg);

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
    ok = ok && _render_pass_resolve_color_target(emitter, stream, cfg, &color_id);

    uint64_t rb_id = 0;
    ok = ok && _render_pass_resolve_readback_buffer(emitter, stream, readback, &rb_id);

    if (!ok)
        return false;

    uint64_t encoder_id = 0;
    uint64_t render_pass_id = 0;
    uint64_t command_buffer_id = 0;
    uint64_t submission_id = 0;
    _render_pass_next_ids(
        emitter, &encoder_id, &render_pass_id, &command_buffer_id, &submission_id);

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
    ok = ok && _render_pass_copy_finish_submit(
                   stream, encoder_id, command_buffer_id, submission_id, color_id, rb_id,
                   readback);
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
                       stream, cs_id, "COMPUTE", _compute_copy_wgsl(), _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_COMPUTE_COPY, false), cfg);

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
                       stream, vs_id, "VERTEX", _fixture_vertex_wgsl(), _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_FIXTURE, false), cfg);

    char fs_key[16];
    dvz_snprintf(fs_key, sizeof(fs_key), "_fs%s", fmt);
    uint64_t fs_id = _obj_id(emitter, fs_key, &is_new);
    if (fs_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && _emit_shader(
                       stream, fs_id, "FRAGMENT", _fixture_fragment_wgsl(), _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_FIXTURE, true), cfg);

    char pipe_key[32];
    dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe1%s", fmt);
    uint64_t pipe_id = _obj_id(emitter, pipe_key, &is_new);
    if (pipe_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && dvz_drp2_stream_create_render_pipeline(stream, pipe_id, vs_id, fs_id, 1);

    uint64_t color_id = 0;
    ok = ok && _render_pass_resolve_color_target(emitter, stream, cfg, &color_id);

    uint64_t rb_id = 0;
    ok = ok && _render_pass_resolve_readback_buffer(emitter, stream, readback, &rb_id);

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
    ok = ok && _render_pass_copy_finish_submit(
                   stream, encoder_id, command_buffer_id, submission_id, color_id, rb_id,
                   readback);
    return ok;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

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
    bool clear_only = compute == NULL && clear != NULL && render == NULL;
    bool retained_render = upload == NULL && compute == NULL && render != NULL &&
                           render->u.render.visual_count > 0;

    if ((!clear_only && !retained_render && upload == NULL) || (!clear_only && render == NULL))
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
                          fallback_vertex_buffer_count, copy, cfg, report));
    if (!ok)
    {
        _diagnostic(report, "failed to emit runtime DRP2 stream");
        dvz_drp2_stream_destroy(stream);
        return NULL;
    }
    _emitter_label_stream_ids(emitter, stream, cfg);
    return stream;
}
