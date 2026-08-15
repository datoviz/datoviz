/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene FramePlan runtime render preparation                                                   */
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
#include "_frame_plan_runtime_internal.h"
#include "_frame_plan_runtime_upload.h"
#include "_scene.h"
#include "_scene_common_bindings.h"
#include "_scene_resource_key.h"
#include "_scene_shader_abi.h"
#include "_shader_registry.h"
#include "_technique.h"
#include "_visual_internal.h"
#include "_visual_pipeline.h"
#include "_visual_pipeline_internal.h"
#include "datoviz/drp2.h"
#include "datoviz/drp2/stream.h"
#include "datoviz/scene.h"
#include "frame_plan/emit.h"
#include "frame_plan/frame_plan.h"
#include "render_contract/render_contract.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve the neutral default panel-light payload used by hand-authored FramePlans.
 *
 * @param emitter frame-plan emitter carrying persistent object identities
 * @param stream destination DRP2 command stream
 * @param out_id resolved buffer id
 * @return whether the buffer was resolved
 */
static bool _resolve_default_panel_lights(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, uint64_t* out_id)
{
    ANN(emitter);
    ANN(stream);
    ANN(out_id);

    bool is_new = false;
    uint64_t id = _obj_id(emitter, "_buf_default_panel_lights_v1", &is_new);
    if (id == 0)
        return false;
    if (is_new)
    {
        DvzScenePanelLightsGpu payload = {.active_count = 2};
        DvzLightDesc ambient = dvz_light_desc(DVZ_LIGHT_AMBIENT);
        DvzLightDesc directional = dvz_light_desc(DVZ_LIGHT_DIRECTIONAL);
        const DvzLightDesc* lights[2] = {&ambient, &directional};
        for (uint32_t i = 0; i < 2; i++)
        {
            payload.lights[i].color_intensity[0] = lights[i]->color[0];
            payload.lights[i].color_intensity[1] = lights[i]->color[1];
            payload.lights[i].color_intensity[2] = lights[i]->color[2];
            payload.lights[i].color_intensity[3] = lights[i]->intensity;
            payload.lights[i].direction_type[0] = lights[i]->direction[0];
            payload.lights[i].direction_type[1] = lights[i]->direction[1];
            payload.lights[i].direction_type[2] = lights[i]->direction[2];
            payload.lights[i].direction_type[3] = (float)lights[i]->type;
        }
        uint32_t usage = DVZ_DRP2_BUFFER_USAGE_UNIFORM | DVZ_DRP2_BUFFER_USAGE_MAP_WRITE |
                         DVZ_DRP2_BUFFER_USAGE_COPY_DST;
        if (!dvz_drp2_stream_create_buffer(stream, id, sizeof(payload), usage) ||
            !dvz_drp2_stream_write_buffer_bytes(stream, id, 0, sizeof(payload), &payload))
        {
            return false;
        }
    }
    *out_id = id;
    return true;
}



/**
 * Emit the exact DRP2 color-target state for one resolved blend policy.
 *
 * @param stream destination DRP2 command stream
 * @param blend_policy resolved scene blend policy
 * @return whether all target state was emitted
 */
static bool _emit_blend_policy(DvzDrp2CommandStream* stream, DvzSceneBlendPolicy blend_policy)
{
    ANN(stream);
    DvzSceneBlendTargetContract targets[DVZ_DRP2_MAX_COLOR_ATTACHMENTS] = {0};
    uint32_t target_count = 0;
    _draw_blend_target_contracts(blend_policy, targets, &target_count);
    for (uint32_t i = 0; i < target_count; i++)
    {
        const DvzSceneBlendTargetContract* target = &targets[i];
        if (target->format != 0 && !dvz_drp2_stream_pipeline_set_color_target(
                                       stream, target->target_index, target->format))
        {
            return false;
        }
        if (target->blend_enabled &&
            !dvz_drp2_stream_pipeline_set_color_blend(
                stream, target->target_index, (DvzBlendFactor)target->src_color_blend_factor,
                (DvzBlendFactor)target->dst_color_blend_factor, (DvzBlendOp)target->color_blend_op,
                (DvzBlendFactor)target->src_alpha_blend_factor,
                (DvzBlendFactor)target->dst_alpha_blend_factor, (DvzBlendOp)target->alpha_blend_op,
                (DvzColorMask)target->color_write_mask))
        {
            return false;
        }
    }
    return true;
}



/**
 * Prepare resources for one panel's draws before opening the render pass.
 *
 * @param emitter frame-plan emitter carrying scene/runtime state.
 * @param stream destination DRP2 command stream.
 * @param render render node to prepare.
 * @param provider typed work provider selecting render policy.
 * @param cfg optional frame-plan emit configuration.
 * @param pass_has_depth_attachment whether the render pass will carry a depth attachment.
 * @param force_point_depth whether point-like visuals must emit depth writes.
 * @param color_target_formats effective color target formats for this render pass.
 * @param color_target_count number of effective color target formats.
 * @param sampled_depth_id depth texture sampled by volume shaders, or zero.
 * @param sampled_depth_is_volume_occlusion whether sampled_depth_id is a volume occlusion texture.
 * @param depth_peel_sampled_bgl_id sampled bind-group layout for peel iteration bounds.
 * @param depth_peel_sampled_bg_id sampled bind group for this peel iteration, or zero.
 * @param depth_peel_dummy_bg_id dummy bind group for unused peel iteration set slots.
 * @param report diagnostic report receiving recoverable emission errors.
 * @param draws output draw descriptors filled from prepared visuals.
 * @param draw_count_out output number of prepared draw descriptors.
 * @return true when the render node has drawable prepared visuals, false otherwise.
 */
bool _emitter_prepare_render_multi(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlanNode* render,
    DvzSceneWorkProviderKey provider, const DvzFramePlanEmitConfig* cfg,
    bool pass_has_depth_attachment, bool force_point_depth, const uint32_t* color_target_formats,
    uint32_t color_target_count, uint64_t sampled_depth_id, bool sampled_depth_is_volume_occlusion,
    uint64_t scene_occlusion_depth_id, uint64_t ambient_visibility_bgl_id,
    uint64_t ambient_visibility_bg_id, uint64_t depth_peel_sampled_bgl_id,
    uint64_t depth_peel_sampled_bg_id, uint64_t depth_peel_dummy_bg_id, uint32_t pass_sample_count,
    bool pass_alpha_to_coverage, DvzDiagnosticReport* report, SceneDrawPacket* draws,
    uint32_t* draw_count_out)
{
    ANN(emitter);
    ANN(stream);
    ANN(render);
    ANN(draws);
    ANN(draw_count_out);

    bool ok = true;
    bool is_new = false;
    const char* fmt = _shader_format_tag(cfg);
    bool wboit_accumulation = provider == DVZ_SCENE_WORK_PROVIDER_WBOIT_ACCUMULATION;
    bool volume_occlusion_pass = provider == DVZ_SCENE_WORK_PROVIDER_VOLUME_OCCLUSION;
    bool scene_occlusion_pass = provider == DVZ_SCENE_WORK_PROVIDER_SCENE_OCCLUSION;
    bool gbuffer_pass = provider == DVZ_SCENE_WORK_PROVIDER_SURFACE_CAPTURE;
    bool depth_peel_pass = provider == DVZ_SCENE_WORK_PROVIDER_DEPTH_PEEL_INIT ||
                           provider == DVZ_SCENE_WORK_PROVIDER_DEPTH_PEEL_ITERATION;
    const bool ambient_visibility_pass =
        provider == DVZ_SCENE_WORK_PROVIDER_OPAQUE && ambient_visibility_bgl_id != 0 &&
        ambient_visibility_bg_id != 0;
    DvzSceneShaderFormat shader_format =
        cfg != NULL ? cfg->shader_format : DVZ_SCENE_SHADER_FORMAT_GLSL;
    uint32_t color_target_format =
        color_target_formats != NULL && color_target_count > 0 ? color_target_formats[0] : 0;
    bool surface_depth_output =
        provider == DVZ_SCENE_WORK_PROVIDER_OPAQUE && color_target_formats != NULL &&
        color_target_count > 1 && color_target_formats[1] == DVZ_FORMAT_R32_SFLOAT;

    uint64_t common_bgl_id = 0;
    uint64_t apply_bg_id = 0;
    uint64_t fixed_bg_id = 0;
    uint64_t isotropic_bg_id = 0;
    if (!_scene_common_bindings_resolve_panel_sets(
            emitter, stream, render, &common_bgl_id, &apply_bg_id, &fixed_bg_id, &isotropic_bg_id))
        return false;

    if (ambient_visibility_pass)
    {
        uint64_t ambient_dummy_bgl_id = _obj_id(emitter, "_bgl_unused_set", &is_new);
        if (ambient_dummy_bgl_id == 0 ||
            (is_new && !_create_dummy_bind_group_layout(stream, ambient_dummy_bgl_id)))
            return false;
    }

    /* Image BGL + sampler (shared, created lazily on first image visual). */
    uint64_t img_bgl_id = 0, img_sampler_linear_id = 0, img_sampler_nearest_id = 0;
    uint64_t textured_mesh_bgl_id = 0;
    uint64_t labels_bgl_id = 0, labels_sampler_id = 0;
    uint64_t glyph_bgl_id = 0, glyph_sampler_id = 0;
    uint64_t volume_bgl_id = 0, volume_sampler_linear_id = 0, volume_sampler_nearest_id = 0;
    uint64_t scene_occlusion_bgl_id = 0, scene_occlusion_sampler_id = 0;
    uint64_t item_state_style_bgl_id = 0;

    uint32_t draw_count = 0;

    for (uint32_t i = 0; ok && i < render->u.render.visual_count; i++)
    {
        DvzSceneVisualDesc desc = {0};
        const char* visual_error = NULL;
        if (!_scene_visual_desc_from_render(emitter, render, i, &desc, &visual_error))
        {
            _diagnostic(report, visual_error != NULL ? visual_error : "invalid visual metadata");
            ok = false;
            break;
        }

        uint64_t vis_bg_set0 = 0;
        uint64_t vis_bg_set1 = 0;
        uint64_t vis_bg_set2 = 0;
        uint64_t vis_bg_set3 = 0;
        DvzFramePlanViewportRect viewport_rect =
            render->u.render.visual_metadata[i].has_metadata
                ? render->u.render.visual_metadata[i].viewport_rect
                : DVZ_FRAME_PLAN_VIEWPORT_PANEL;
        DvzSceneVisualBindDesc bind = {0};
        if (!_scene_visual_bind_desc(&desc, render->u.render.controller_modes[i], &bind))
        {
            ok = false;
            break;
        }
        _scene_visual_bind_desc_apply_pass_policy(
            &bind, provider, sampled_depth_id, sampled_depth_is_volume_occlusion,
            scene_occlusion_depth_id);
        const bool attachment_local = provider == DVZ_SCENE_WORK_PROVIDER_SURFACE_CAPTURE ||
                                      provider == DVZ_SCENE_WORK_PROVIDER_VOLUME_OCCLUSION ||
                                      provider == DVZ_SCENE_WORK_PROVIDER_SCENE_OCCLUSION ||
                                      provider == DVZ_SCENE_WORK_PROVIDER_WBOIT_ACCUMULATION ||
                                      provider == DVZ_SCENE_WORK_PROVIDER_DEPTH_PEEL_INIT ||
                                      provider == DVZ_SCENE_WORK_PROVIDER_DEPTH_PEEL_ITERATION;
        if (!attachment_local && render->u.render.has_viewport)
        {
            bind.sampled_panel_origin[0] = render->u.render.viewport.x;
            bind.sampled_panel_origin[1] = render->u.render.viewport.y;
        }
        if (bind.uses_common_set0)
        {
            ok = _scene_common_bindings_resolve_visual_set(
                emitter, stream, render, provider, i, common_bgl_id, viewport_rect, &vis_bg_set0);
            if (!ok)
                break;
        }

        DvzSceneVisualShaderDesc shader = {0};
        char* scene_occlusion_fragment_glsl = NULL;
        bool special_pass_handled = false;
        bool special_pass_skip = false;
        ok = _scene_visual_shader_desc_for_pass(
            &desc, provider, fmt, &shader, &scene_occlusion_fragment_glsl, &special_pass_handled,
            &special_pass_skip);
        if (!ok)
        {
            _shader_glsl_variant_destroy(scene_occlusion_fragment_glsl);
            break;
        }
        if (special_pass_skip)
        {
            _shader_glsl_variant_destroy(scene_occlusion_fragment_glsl);
            continue;
        }
        if (!special_pass_handled &&
            !_scene_visual_shader_desc(
                &desc, render->u.render.picking, wboit_accumulation, fmt, &shader))
            continue;
        DvzAlphaMode alpha_mode = render->u.render.visual_metadata[i].has_metadata
                                      ? render->u.render.visual_metadata[i].alpha_mode
                                      : DVZ_ALPHA_OPAQUE;
        DvzSceneBlendPolicy blend_policy =
            render->u.render.visual_metadata[i].has_draw_contract
                ? (DvzSceneBlendPolicy)render->u.render.visual_metadata[i].draw_blend_policy
                : DVZ_SCENE_BLEND_POLICY_NONE;
        bool query_shader_applied = false;
        if (render->u.render.picking && color_target_format != 0)
        {
            ok = _scene_visual_shader_desc_apply_query_pick(
                &desc, color_target_format, &shader, &query_shader_applied);
            if (!ok)
            {
                _diagnostic(report, "scene query shader descriptor setup failed");
                break;
            }
            (void)query_shader_applied;
        }
        bool scene_occluded_shader =
            render->u.render.visual_metadata[i].draw_scene_occlusion_resource_id[0] != '\0' &&
            scene_occlusion_depth_id != 0 && !scene_occlusion_pass;
        bool scene_occlusion_uses_set2 =
            _scene_visual_bind_desc_uses_scene_occlusion_set2(&desc, provider);
        bool segment_coverage_blend = false;
        ok = _scene_visual_shader_desc_apply_pass_policy(
            &desc, provider, alpha_mode, render->u.render.controller_modes[i],
            render->u.render.picking, pass_has_depth_attachment, force_point_depth,
            wboit_accumulation, surface_depth_output, pass_sample_count, pass_alpha_to_coverage,
            scene_occluded_shader, scene_occlusion_uses_set2, &shader,
            &scene_occlusion_fragment_glsl,
            &segment_coverage_blend);
        if (!ok)
        {
            _diagnostic(report, "scene pass shader descriptor setup failed");
            _shader_glsl_variant_destroy(scene_occlusion_fragment_glsl);
            break;
        }
        if (ambient_visibility_pass && !render->u.render.picking && desc.material_buffer_id != 0)
        {
            char* ambient_variant = _shader_glsl_variant(
                shader.fragment_glsl, "#define DVZ_AMBIENT_VISIBILITY 1\n");
            if (ambient_variant == NULL)
            {
                _shader_glsl_variant_destroy(scene_occlusion_fragment_glsl);
                ok = false;
                break;
            }
            _shader_glsl_variant_destroy(scene_occlusion_fragment_glsl);
            scene_occlusion_fragment_glsl = ambient_variant;
            shader.fragment_glsl = ambient_variant;
            shader.fragment_spirv_key = NULL;
            ok = _runtime_key_appendf(
                shader.fragment_key, sizeof(shader.fragment_key), report,
                "_ambient_visibility");
            if (!ok)
                break;
            ok = _runtime_key_appendf(
                shader.pipeline_key, sizeof(shader.pipeline_key), report,
                "_ambient_visibility");
            if (!ok)
                break;
        }
        DvzSceneBlendPolicy effective_blend_policy =
            segment_coverage_blend ? DVZ_SCENE_BLEND_POLICY_SEGMENT_COVERAGE : blend_policy;

        DvzSceneResolvedShader resolved_shader = {0};
        if (!_scene_runtime_shader_resolve(
                &shader, &desc, provider, shader_format, &resolved_shader, report))
        {
            _shader_glsl_variant_destroy(scene_occlusion_fragment_glsl);
            ok = false;
            break;
        }

        /* Shaders (cached). */
        uint64_t vs_id = 0;
        uint64_t fs_id = 0;
        if (!_scene_runtime_shader_emit(emitter, stream, &resolved_shader, cfg, &vs_id, &fs_id))
        {
            _shader_glsl_variant_destroy(scene_occlusion_fragment_glsl);
            ok = false;
            break;
        }
        _shader_glsl_variant_destroy(scene_occlusion_fragment_glsl);

        for (uint32_t target_idx = 0; ok && target_idx < color_target_count; target_idx++)
        {
            ok = _runtime_key_appendf(
                shader.pipeline_key, sizeof(shader.pipeline_key), report, "_fmt%u_%u", target_idx,
                color_target_formats[target_idx]);
        }
        if (ok)
            ok = _runtime_key_appendf(
                shader.pipeline_key, sizeof(shader.pipeline_key), report, "_bp%u",
                (uint32_t)effective_blend_policy);
        if (!ok)
            break;

        uint64_t pipe_id = _obj_id(emitter, shader.pipeline_key, &is_new);
        if (pipe_id == 0)
        {
            ok = false;
            break;
        }
        DvzSceneVisualPipelineDesc pipeline = {0};
        if (!_scene_visual_pipeline_desc(
                &desc, render->u.render.picking, pass_has_depth_attachment,
                wboit_accumulation || depth_peel_pass, alpha_mode,
                render->u.render.controller_modes[i], shader_format, &pipeline))
        {
            ok = false;
            break;
        }
        if (render->u.render.picking && color_target_format != 0)
            _scene_visual_pipeline_desc_apply_query_pick(&desc, color_target_format, &pipeline);
        _scene_visual_pipeline_desc_apply_pass_policy(
            &desc, provider, force_point_depth, pass_sample_count, pass_alpha_to_coverage,
            &pipeline);
        pipeline.needs_ambient_visibility_layout =
            ambient_visibility_pass && !render->u.render.picking && desc.material_buffer_id != 0;
        if (!pipeline.needs_item_state_style_layout)
            bind.uses_item_state_style_set1 = false;
        if (!pipeline.needs_material_layout)
            bind.uses_material_set1 = false;
        if (!pipeline.needs_image_layout)
        {
            bind.uses_image_set1 = false;
            bind.uses_textured_mesh_set1 = false;
        }
        if (!pipeline.needs_labels_layout)
            bind.uses_labels_set1 = false;
        if (!pipeline.needs_glyph_layout)
            bind.uses_glyph_set1 = false;
        if (!pipeline.needs_volume_layout)
            bind.uses_volume_set1 = false;
        if (!pipeline.needs_scene_occlusion_layout)
            bind.uses_scene_occlusion_set2 = false;
        if (
            bind.panel_light_buffer_id == 0 &&
            (bind.uses_material_set1 || bind.uses_item_state_style_set1 ||
             bind.uses_textured_mesh_set1) &&
            !_resolve_default_panel_lights(emitter, stream, &bind.panel_light_buffer_id))
        {
            _diagnostic(report, "default panel lights buffer resolution failed");
            ok = false;
            break;
        }
        if (ok && is_new)
        {
            uint64_t material_bgl_id = 0;
            uint64_t visual_material_bgl_id = 0;
            if (pipeline.needs_item_state_style_layout)
            {
                if (!_resolve_item_state_style_bind_group_layout(
                        emitter, stream, &item_state_style_bgl_id))
                {
                    ok = false;
                    break;
                }
                visual_material_bgl_id = item_state_style_bgl_id;
            }
            else if (pipeline.needs_material_layout)
            {
                if (!_resolve_material_bind_group_layout(emitter, stream, &material_bgl_id))
                {
                    ok = false;
                    break;
                }
                visual_material_bgl_id = material_bgl_id;
            }
            if (pipeline.needs_image_layout && pipeline.uses_textured_mesh_layout)
            {
                ok = ok && _resolve_textured_mesh_bind_group_layout(
                               emitter, stream, &textured_mesh_bgl_id);
            }
            else if (pipeline.needs_image_layout && img_bgl_id == 0)
            {
                img_bgl_id = _obj_id(emitter, "_bgl_img", &is_new);
                if (img_bgl_id == 0)
                {
                    ok = false;
                    break;
                }
                if (is_new)
                    ok = ok && _create_image_bind_group_layout(stream, img_bgl_id);
            }
            if (pipeline.needs_labels_layout && labels_bgl_id == 0)
            {
                labels_bgl_id = _obj_id(emitter, "_bgl_labels", &is_new);
                if (labels_bgl_id == 0)
                {
                    ok = false;
                    break;
                }
                if (is_new)
                    ok = ok && _create_labels_bind_group_layout(stream, labels_bgl_id);
            }
            if (pipeline.needs_glyph_layout && glyph_bgl_id == 0)
            {
                glyph_bgl_id = _obj_id(emitter, "_bgl_glyph", &is_new);
                if (glyph_bgl_id == 0)
                {
                    ok = false;
                    break;
                }
                if (is_new)
                    ok = ok && _create_glyph_bind_group_layout(stream, glyph_bgl_id);
            }
            if (pipeline.needs_volume_layout && volume_bgl_id == 0)
            {
                volume_bgl_id = _obj_id(emitter, "_bgl_volume", &is_new);
                if (volume_bgl_id == 0)
                {
                    ok = false;
                    break;
                }
                if (is_new)
                    ok = ok && _create_volume_bind_group_layout(stream, volume_bgl_id);
            }
            if (pipeline.needs_scene_occlusion_layout && scene_occlusion_bgl_id == 0)
            {
                scene_occlusion_bgl_id = _obj_id(emitter, "_bgl_scene_occ", &is_new);
                if (scene_occlusion_bgl_id == 0)
                {
                    ok = false;
                    break;
                }
                if (is_new)
                    ok = ok &&
                         _create_scene_occlusion_bind_group_layout(stream, scene_occlusion_bgl_id);
            }
            DvzDrp2RenderPipelineDesc pipe_desc = dvz_drp2_render_pipeline_desc();
            pipe_desc.id = pipe_id;
            pipe_desc.vertex_shader_module_id = vs_id;
            pipe_desc.fragment_shader_module_id = fs_id;
            pipe_desc.vertex_buffer_slots = pipeline.vertex_buffer_count;
            pipe_desc.topology = pipeline.topology;
            pipe_desc.binding_count = pipeline.binding_count;
            pipe_desc.binding_strides = pipeline.strides;
            pipe_desc.binding_step_modes = pipeline.step_modes;
            pipe_desc.attr_count = pipeline.attr_count;
            pipe_desc.attr_bindings = pipeline.bindings;
            pipe_desc.attr_locations = pipeline.locations;
            pipe_desc.attr_formats = pipeline.formats;
            pipe_desc.attr_offsets = pipeline.offsets;
            ok = ok && dvz_drp2_stream_create_render_pipeline(stream, &pipe_desc);
            if (ok && pass_sample_count > 1)
                ok = dvz_drp2_stream_pipeline_set_multisampling(
                    stream, pass_sample_count, pipeline.alpha_to_coverage);
            if (ok && shader.builtin_pipeline != NULL)
                ok = dvz_drp2_stream_pipeline_set_builtin_identity(
                    stream, pipe_id, shader.builtin_pipeline,
                    shader.builtin_pipeline_version != 0
                        ? shader.builtin_pipeline_version
                        : DVZ_SCENE_SHADER_BUILTIN_CONTRACT_VERSION);
            if (ok)
            {
                uint64_t layouts[DVZ_DRP2_MAX_BIND_GROUPS] = {0};
                uint32_t layout_count = 0;
                uint64_t dummy_bgl_id = 0;
                if (pipeline.needs_ambient_visibility_layout)
                {
                    dummy_bgl_id = _obj_id(emitter, "_bgl_unused_set", &is_new);
                    if (dummy_bgl_id == 0)
                        ok = false;
                    else if (is_new)
                        ok = _create_dummy_bind_group_layout(stream, dummy_bgl_id);
                    if (!ok)
                        break;
                }
                bool depth_peel_iter_pass =
                    provider == DVZ_SCENE_WORK_PROVIDER_DEPTH_PEEL_ITERATION &&
                    depth_peel_sampled_bgl_id != 0;
                if (depth_peel_iter_pass)
                {
                    dummy_bgl_id = _obj_id(emitter, "_bgl_unused_set", &is_new);
                    if (dummy_bgl_id == 0)
                    {
                        ok = false;
                    }
                    else if (is_new)
                    {
                        ok = _create_dummy_bind_group_layout(stream, dummy_bgl_id);
                    }
                    if (!ok)
                        break;

                    layouts[0] = pipeline.needs_common_layout && common_bgl_id != 0 ? common_bgl_id
                                                                                    : dummy_bgl_id;
                    if (pipeline.needs_item_state_style_layout && item_state_style_bgl_id != 0)
                        layouts[1] = item_state_style_bgl_id;
                    else if (pipeline.needs_material_layout && visual_material_bgl_id != 0)
                        layouts[1] = visual_material_bgl_id;
                    else if (pipeline.needs_image_layout && img_bgl_id != 0)
                        layouts[1] = img_bgl_id;
                    else if (pipeline.needs_labels_layout && labels_bgl_id != 0)
                        layouts[1] = labels_bgl_id;
                    else if (pipeline.needs_glyph_layout && glyph_bgl_id != 0)
                        layouts[1] = glyph_bgl_id;
                    else if (pipeline.needs_volume_layout && volume_bgl_id != 0)
                        layouts[1] = volume_bgl_id;
                    else if (
                        pipeline.needs_scene_occlusion_layout && scene_occlusion_bgl_id != 0 &&
                        !scene_occlusion_uses_set2)
                        layouts[1] = scene_occlusion_bgl_id;
                    else
                        layouts[1] = dummy_bgl_id;

                    layouts[2] = pipeline.needs_scene_occlusion_layout &&
                                         scene_occlusion_bgl_id != 0 && scene_occlusion_uses_set2
                                     ? scene_occlusion_bgl_id
                                     : dummy_bgl_id;
                    layouts[DVZ_SCENE_DEPTH_PEEL_BIND_SET] = depth_peel_sampled_bgl_id;
                    layout_count = DVZ_SCENE_DEPTH_PEEL_BIND_SET + 1;
                }
                else
                {
                    _pipeline_bind_group_layouts(
                        &pipeline, common_bgl_id,
                        pipeline.uses_textured_mesh_layout ? textured_mesh_bgl_id : img_bgl_id,
                        labels_bgl_id, glyph_bgl_id, volume_bgl_id, visual_material_bgl_id,
                        item_state_style_bgl_id, scene_occlusion_bgl_id, scene_occlusion_uses_set2,
                        ambient_visibility_bgl_id, dummy_bgl_id, layouts, &layout_count);
                }
                if (layout_count > 0)
                    ok = dvz_drp2_stream_pipeline_set_bind_group_layouts(
                        stream, layout_count, layouts);
                if (!ok)
                    _diagnostic(report, "scene render pipeline bind-group layouts failed");
            }
            if (ok && pipeline.has_depth_state)
                ok = dvz_drp2_stream_pipeline_set_depth_state(
                    stream, pipeline.depth_write_enabled, pipeline.depth_compare_op);
            if (ok && pipeline.has_raster_state)
                ok = dvz_drp2_stream_pipeline_set_raster_state(
                    stream, pipeline.cull_mode, pipeline.front_face);
            if (ok && wboit_accumulation)
            {
                ok = _emit_blend_policy(stream, effective_blend_policy);
            }
            else if (ok && depth_peel_pass)
            {
                ok = _emit_blend_policy(stream, effective_blend_policy);
                if (ok)
                    ok = dvz_drp2_stream_pipeline_set_raster_state(
                        stream, DVZ_CULL_MODE_NONE, DVZ_FRONT_FACE_COUNTER_CLOCKWISE);
            }
            else if (ok && gbuffer_pass)
            {
                for (uint32_t target_idx = 0; ok && target_idx < color_target_count; target_idx++)
                {
                    ok = color_target_formats[target_idx] != 0 &&
                         dvz_drp2_stream_pipeline_set_color_target(
                             stream, target_idx, color_target_formats[target_idx]);
                }
            }
            else if (ok && (volume_occlusion_pass || scene_occlusion_pass))
            {
                ok = dvz_drp2_stream_pipeline_set_color_target(stream, 0, DVZ_FORMAT_R32_SFLOAT);
            }
            else
            {
                for (uint32_t target_idx = 0; ok && target_idx < color_target_count; target_idx++)
                {
                    ok = color_target_formats[target_idx] != 0 &&
                         dvz_drp2_stream_pipeline_set_color_target(
                             stream, target_idx, color_target_formats[target_idx]);
                }
                if (ok)
                    ok = _emit_blend_policy(stream, effective_blend_policy);
            }
            if (!ok)
                _diagnostic(report, "scene render pipeline setup failed");
        }

        /* Visual-specific bind groups. */
        if (bind.uses_item_state_style_set1)
        {
            if (
                bind.material_buffer_id == 0 || bind.panel_light_buffer_id == 0 ||
                bind.item_state_style_buffer_id == 0)
            {
                _diagnostic(report, "item-state render missing style or material params buffer");
                ok = false;
                break;
            }
            if (!_resolve_item_state_style_bind_group_layout(
                    emitter, stream, &item_state_style_bgl_id))
            {
                ok = false;
                break;
            }
            uint64_t item_state_style_bg_id = 0;
            ok = ok && _resolve_item_state_style_bind_group(
                           emitter, stream, item_state_style_bgl_id, bind.material_buffer_id,
                           bind.panel_light_buffer_id, bind.item_state_style_buffer_id,
                           &item_state_style_bg_id);
            vis_bg_set1 = item_state_style_bg_id;
        }
        else if (bind.uses_material_set1)
        {
            uint64_t material_bgl_id = 0;
            if (!_resolve_material_bind_group_layout(emitter, stream, &material_bgl_id))
            {
                ok = false;
                break;
            }
            if (bind.material_buffer_id == 0 || bind.panel_light_buffer_id == 0)
            {
                _diagnostic(report, "material render missing params or panel lights buffer");
                ok = false;
                break;
            }
            char material_bg_key[96];
            dvz_snprintf(
                material_bg_key, sizeof(material_bg_key),
                "_bg_material_params_%" PRIu64 "_l%" PRIu64, bind.material_buffer_id,
                bind.panel_light_buffer_id);
            uint64_t material_bg_id = _obj_id(emitter, material_bg_key, &is_new);
            if (material_bg_id == 0)
            {
                ok = false;
                break;
            }
            if (ok && is_new)
            {
                DvzDrp2BindGroupEntry entries[2] = {
                    {
                        .binding = DVZ_SCENE_SHADER_BINDING_MATERIAL_PARAMS,
                        .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
                        .resource_kind = DVZ_DRP2_BINDING_RESOURCE_BUFFER,
                        .resource_id = bind.material_buffer_id,
                        .offset = 0,
                        .size = sizeof(DvzSceneMaterialParams),
                    },
                    {
                        .binding = DVZ_SCENE_SHADER_BINDING_PANEL_LIGHTS,
                        .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
                        .resource_kind = DVZ_DRP2_BINDING_RESOURCE_BUFFER,
                        .resource_id = bind.panel_light_buffer_id,
                        .offset = 0,
                        .size = sizeof(DvzScenePanelLightsGpu),
                    },
                };
                ok = ok && dvz_drp2_stream_create_bind_group_entries(
                               stream, material_bg_id, material_bgl_id, 2, entries);
            }
            vis_bg_set1 = material_bg_id;
        }
        if (bind.uses_image_set1 && bind.uses_textured_mesh_set1)
        {
            if (!_resolve_textured_mesh_bind_group_layout(emitter, stream, &textured_mesh_bgl_id))
            {
                ok = false;
                break;
            }
            if (bind.material_buffer_id == 0 || bind.panel_light_buffer_id == 0)
            {
                _diagnostic(report, "textured mesh render missing material params buffer");
                ok = false;
                break;
            }
            uint64_t* mesh_sampler_id =
                bind.image_nearest_sampler ? &img_sampler_nearest_id : &img_sampler_linear_id;
            if (*mesh_sampler_id == 0)
            {
                *mesh_sampler_id = _obj_id(
                    emitter, bind.image_nearest_sampler ? "_sampler_img_nearest" : "_sampler_img",
                    &is_new);
                if (*mesh_sampler_id == 0)
                {
                    ok = false;
                    break;
                }
                if (ok && is_new)
                {
                    DvzDrp2FilterMode filter = bind.image_nearest_sampler ? DVZ_DRP2_FILTER_NEAREST
                                                                          : DVZ_DRP2_FILTER_LINEAR;
                    ok = ok && dvz_drp2_stream_create_sampler_filter(
                                   stream, *mesh_sampler_id, filter, filter);
                }
            }
            uint64_t mesh_bg_id = 0;
            ok = ok &&
                 _resolve_textured_mesh_bind_group(
                     emitter, stream, textured_mesh_bgl_id, bind.material_buffer_id,
                     bind.panel_light_buffer_id, bind.image_texture_id, *mesh_sampler_id,
                     bind.image_color_role, &mesh_bg_id);
            vis_bg_set1 = mesh_bg_id;
        }
        else if (bind.uses_image_set1)
        {
            /* Image BGL + sampler (lazy). */
            if (img_bgl_id == 0)
            {
                img_bgl_id = _obj_id(emitter, "_bgl_img", &is_new);
                if (img_bgl_id == 0)
                {
                    ok = false;
                    break;
                }
                if (is_new)
                    ok = ok && _create_image_bind_group_layout(stream, img_bgl_id);
            }
            uint64_t* img_sampler_id =
                bind.image_nearest_sampler ? &img_sampler_nearest_id : &img_sampler_linear_id;
            if (*img_sampler_id == 0)
            {
                *img_sampler_id = _obj_id(
                    emitter, bind.image_nearest_sampler ? "_sampler_img_nearest" : "_sampler_img",
                    &is_new);
                if (*img_sampler_id == 0)
                {
                    ok = false;
                    break;
                }
                if (ok && is_new)
                {
                    DvzDrp2FilterMode filter = bind.image_nearest_sampler ? DVZ_DRP2_FILTER_NEAREST
                                                                          : DVZ_DRP2_FILTER_LINEAR;
                    ok = ok && dvz_drp2_stream_create_sampler_filter(
                                   stream, *img_sampler_id, filter, filter);
                }
            }
            uint64_t img_bg_id = 0;
            ok = ok && _resolve_image_bind_group(
                           emitter, stream, img_bgl_id, bind.image_texture_id, *img_sampler_id,
                           bind.image_nearest_sampler, bind.image_color_role, &img_bg_id);
            vis_bg_set1 = img_bg_id;
        }
        if (bind.uses_labels_set1)
        {
            if (labels_bgl_id == 0)
            {
                labels_bgl_id = _obj_id(emitter, "_bgl_labels", &is_new);
                if (labels_bgl_id == 0)
                {
                    ok = false;
                    break;
                }
                if (is_new)
                    ok = ok && _create_labels_bind_group_layout(stream, labels_bgl_id);
            }
            if (labels_sampler_id == 0)
            {
                labels_sampler_id = _obj_id(emitter, "_sampler_labels_nearest", &is_new);
                if (labels_sampler_id == 0)
                {
                    ok = false;
                    break;
                }
                if (ok && is_new)
                    ok = ok && dvz_drp2_stream_create_sampler_filter(
                                   stream, labels_sampler_id, DVZ_DRP2_FILTER_NEAREST,
                                   DVZ_DRP2_FILTER_NEAREST);
            }
            uint64_t labels_bg_id = 0;
            ok =
                ok && _resolve_labels_bind_group(
                          emitter, stream, labels_bgl_id, labels_sampler_id, &bind, &labels_bg_id);
            vis_bg_set1 = labels_bg_id;
        }
        if (bind.uses_glyph_set1)
        {
            if (glyph_bgl_id == 0)
            {
                glyph_bgl_id = _obj_id(emitter, "_bgl_glyph", &is_new);
                if (glyph_bgl_id == 0)
                {
                    ok = false;
                    break;
                }
                if (is_new)
                    ok = ok && _create_glyph_bind_group_layout(stream, glyph_bgl_id);
            }
            if (glyph_sampler_id == 0)
            {
                glyph_sampler_id = _obj_id(emitter, "_sampler_glyph", &is_new);
                if (glyph_sampler_id == 0)
                {
                    ok = false;
                    break;
                }
                if (ok && is_new)
                    ok = ok && dvz_drp2_stream_create_sampler(stream, glyph_sampler_id);
            }
            uint64_t glyph_bg_id = 0;
            ok = ok && _resolve_glyph_bind_group(
                           emitter, stream, glyph_bgl_id, glyph_sampler_id, &bind, &glyph_bg_id);
            vis_bg_set1 = glyph_bg_id;
        }
        if (bind.uses_volume_set1)
        {
            if (volume_bgl_id == 0)
            {
                volume_bgl_id = _obj_id(emitter, "_bgl_volume", &is_new);
                if (volume_bgl_id == 0)
                {
                    ok = false;
                    break;
                }
                if (is_new)
                    ok = ok && _create_volume_bind_group_layout(stream, volume_bgl_id);
            }
            bool nearest = bind.volume_state.sampling == DVZ_VOLUME_SAMPLING_NEAREST;
            uint64_t* volume_sampler_id =
                nearest ? &volume_sampler_nearest_id : &volume_sampler_linear_id;
            if (*volume_sampler_id == 0)
            {
                *volume_sampler_id = _obj_id(
                    emitter, nearest ? "_sampler_volume_nearest" : "_sampler_volume_linear",
                    &is_new);
                if (*volume_sampler_id == 0)
                {
                    ok = false;
                    break;
                }
                if (ok && is_new)
                {
                    DvzDrp2FilterMode filter =
                        nearest ? DVZ_DRP2_FILTER_NEAREST : DVZ_DRP2_FILTER_LINEAR;
                    ok = ok && dvz_drp2_stream_create_sampler_filter(
                                   stream, *volume_sampler_id, filter, filter);
                }
            }
            uint64_t volume_bg_id = 0;
            ok = ok &&
                 _resolve_volume_bind_group(
                     emitter, stream, volume_bgl_id, *volume_sampler_id, &bind, &volume_bg_id);
            vis_bg_set1 = volume_bg_id;
        }
        if (bind.uses_scene_occlusion_set2 && bind.scene_occlusion_depth_texture_id != 0)
        {
            if (scene_occlusion_bgl_id == 0)
            {
                scene_occlusion_bgl_id = _obj_id(emitter, "_bgl_scene_occ", &is_new);
                if (scene_occlusion_bgl_id == 0)
                {
                    ok = false;
                    break;
                }
                if (is_new)
                    ok = ok &&
                         _create_scene_occlusion_bind_group_layout(stream, scene_occlusion_bgl_id);
            }
            if (scene_occlusion_sampler_id == 0)
            {
                scene_occlusion_sampler_id = _obj_id(emitter, "_sampler_scene_occ", &is_new);
                if (scene_occlusion_sampler_id == 0)
                {
                    ok = false;
                    break;
                }
                if (ok && is_new)
                    ok = ok && dvz_drp2_stream_create_sampler(stream, scene_occlusion_sampler_id);
            }
            uint64_t scene_occ_bg_id = 0;
            ok = ok && _resolve_scene_occlusion_bind_group(
                           emitter, stream, scene_occlusion_bgl_id, scene_occlusion_sampler_id,
                           &bind, &scene_occ_bg_id);
            if (scene_occlusion_uses_set2)
                vis_bg_set2 = scene_occ_bg_id;
            else
                vis_bg_set1 = scene_occ_bg_id;
        }
        if (provider == DVZ_SCENE_WORK_PROVIDER_DEPTH_PEEL_ITERATION)
        {
            if (vis_bg_set1 == 0)
                vis_bg_set1 = depth_peel_dummy_bg_id;
            if (vis_bg_set2 == 0)
                vis_bg_set2 = depth_peel_dummy_bg_id;
            vis_bg_set3 = depth_peel_sampled_bg_id;
        }
        else if (pipeline.needs_ambient_visibility_layout)
        {
            if (vis_bg_set2 == 0)
                vis_bg_set2 = depth_peel_dummy_bg_id;
            vis_bg_set3 = ambient_visibility_bg_id;
        }

        if (!ok)
        {
            _diagnostic(report, "scene render bind setup failed");
            break;
        }

        DvzFramePlanClipRect clip_rect = render->u.render.visual_metadata[i].has_metadata
                                             ? render->u.render.visual_metadata[i].clip_rect
                                             : DVZ_FRAME_PLAN_CLIP_RECT_PANEL;
        ok = _scene_draw_packet_init(
            &emitter->resources, &desc, &pipeline, pipe_id, vis_bg_set0, vis_bg_set1, vis_bg_set2,
            vis_bg_set3, clip_rect, viewport_rect, shader_format, report, &draws[draw_count]);
        if (!ok)
            break;
        draw_count++;
    }

    if (!ok || draw_count == 0)
    {
        if (ok && draw_count == 0)
            _diagnostic(report, "scene render batch has no prepared draws");
        return false;
    }

    *draw_count_out = draw_count;
    return true;
}
