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
#include "frame_plan/frame_plan.h"
#include "frame_plan/emit.h"
#include "_frame_plan_runtime_internal.h"
#include "_frame_plan_runtime_upload.h"
#include "_scene.h"
#include "_scene_common_bindings.h"
#include "_scene_resource_key.h"
#include "_scene_shader_abi.h"
#include "_shader_registry.h"
#include "_technique.h"
#include "_visual_pipeline.h"
#include "_visual_pipeline_internal.h"
#include "datoviz/drp2.h"
#include "datoviz/drp2/stream.h"
#include "datoviz/scene.h"
#include "render_contract/render_contract.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Prepare resources for one panel's draws before opening the render pass.
 *
 * @param emitter frame-plan emitter carrying scene/runtime state.
 * @param stream destination DRP2 command stream.
 * @param render render node to prepare.
 * @param cfg optional frame-plan emit configuration.
 * @param pass_has_depth_attachment whether the render pass will carry a depth attachment.
 * @param force_point_depth whether point-like visuals must emit depth writes.
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
    const DvzFramePlanEmitConfig* cfg, bool pass_has_depth_attachment, bool force_point_depth,
    uint64_t sampled_depth_id, bool sampled_depth_is_volume_occlusion,
    uint64_t scene_occlusion_depth_id, uint64_t depth_peel_sampled_bgl_id,
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
    bool wboit_accumulation =
        render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION;
    bool volume_occlusion_pass =
        render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_VOLUME_OCCLUSION;
    bool scene_occlusion_pass =
        render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_SCENE_OCCLUSION;
    bool gbuffer_pass = render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_GBUFFER;
    bool depth_peel_pass =
        render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT ||
        render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER;
    DvzSceneShaderFormat shader_format =
        cfg != NULL ? cfg->shader_format : DVZ_SCENE_SHADER_FORMAT_GLSL;

    uint64_t common_bgl_id = 0;
    uint64_t apply_bg_id = 0;
    uint64_t fixed_bg_id = 0;
    uint64_t isotropic_bg_id = 0;
    if (!_scene_common_bindings_resolve_panel_sets(
            emitter, stream, render, &common_bgl_id, &apply_bg_id, &fixed_bg_id, &isotropic_bg_id))
        return false;

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

        DvzSceneVisualShaderDesc shader = {0};
        char* scene_occlusion_fragment_glsl = NULL;
        bool special_pass_handled = false;
        bool special_pass_skip = false;
        ok = _scene_visual_shader_desc_for_pass(
            &desc, render->u.render.pass_role, fmt, &shader, &scene_occlusion_fragment_glsl,
            &special_pass_handled, &special_pass_skip);
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
        if (!special_pass_handled && !_scene_visual_shader_desc(
                     &desc, render->u.render.picking, wboit_accumulation, fmt, &shader))
            continue;
        DvzAlphaMode alpha_mode = render->u.render.visual_metadata[i].has_metadata
                                      ? render->u.render.visual_metadata[i].alpha_mode
                                      : DVZ_ALPHA_OPAQUE;
        bool query_shader_applied = false;
        if (render->u.render.picking && cfg != NULL)
        {
            ok = _scene_visual_shader_desc_apply_query_pick(
                &desc, cfg->color_target_format, &shader, &query_shader_applied);
            if (!ok)
            {
                _diagnostic(report, "scene query shader descriptor setup failed");
                break;
            }
            (void)query_shader_applied;
        }
        bool scene_occluded_shader =
            desc.scene_occluded && scene_occlusion_depth_id != 0 && !scene_occlusion_pass;
        bool scene_occlusion_uses_set2 = _scene_visual_bind_desc_uses_scene_occlusion_set2(
            &desc, render->u.render.pass_role);
        bool segment_coverage_blend = false;
        ok = _scene_visual_shader_desc_apply_pass_policy(
            &desc, render->u.render.pass_role, alpha_mode, render->u.render.controller_modes[i],
            render->u.render.picking, pass_has_depth_attachment, force_point_depth,
            wboit_accumulation, pass_sample_count, pass_alpha_to_coverage, scene_occluded_shader,
            scene_occlusion_uses_set2, &shader, &scene_occlusion_fragment_glsl,
            &segment_coverage_blend);
        if (!ok)
        {
            _diagnostic(report, "scene pass shader descriptor setup failed");
            _shader_glsl_variant_destroy(scene_occlusion_fragment_glsl);
            break;
        }

        /* Shaders (cached). */
        uint64_t vs_id = _obj_id(emitter, shader.vertex_key, &is_new);
        if (vs_id == 0)
        {
            ok = false;
            break;
        }
        if (is_new)
        {
            if (cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_WGSL)
            {
                if (shader.vertex_wgsl == NULL)
                    ok = false;
                else
                    ok = ok &&
                         _emit_shader(
                             stream, vs_id, "VERTEX", shader.vertex_wgsl, shader.vertex_glsl, cfg);
            }
            else if (shader.vertex_spirv_key != NULL)
                ok = ok && _emit_shader_spirv(
                               stream, vs_id, "VERTEX", shader.vertex_spirv_key,
                               shader.vertex_glsl, cfg);
            else
                ok = ok && _emit_shader(stream, vs_id, "VERTEX", NULL, shader.vertex_glsl, cfg);
            if (ok && shader.builtin_family != NULL && shader.builtin_variant != NULL)
                ok = dvz_drp2_stream_shader_set_builtin_identity(
                    stream, vs_id, shader.builtin_family, shader.builtin_variant, 1);
        }

        uint64_t fs_id = _obj_id(emitter, shader.fragment_key, &is_new);
        if (fs_id == 0)
        {
            ok = false;
            break;
        }
        if (ok && is_new)
        {
            if (cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_WGSL)
            {
                if (shader.fragment_wgsl == NULL)
                    ok = false;
                else
                    ok = ok && _emit_shader(
                                   stream, fs_id, "FRAGMENT", shader.fragment_wgsl,
                                   shader.fragment_glsl, cfg);
            }
            else if (shader.fragment_spirv_key != NULL)
                ok = ok && _emit_shader_spirv(
                               stream, fs_id, "FRAGMENT", shader.fragment_spirv_key,
                               shader.fragment_glsl, cfg);
            else
                ok =
                    ok && _emit_shader(stream, fs_id, "FRAGMENT", NULL, shader.fragment_glsl, cfg);
            if (ok && shader.builtin_family != NULL && shader.builtin_variant != NULL)
                ok = dvz_drp2_stream_shader_set_builtin_identity(
                    stream, fs_id, shader.builtin_family, shader.builtin_variant, 1);
        }
        _shader_glsl_variant_destroy(scene_occlusion_fragment_glsl);

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
        if (render->u.render.picking && cfg != NULL)
            _scene_visual_pipeline_desc_apply_query_pick(
                &desc, cfg->color_target_format, &pipeline);
        _scene_visual_pipeline_desc_apply_pass_policy(
            &desc, render->u.render.pass_role, force_point_depth, pass_sample_count,
            pass_alpha_to_coverage, &pipeline);
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
                ok = ok &&
                     _resolve_textured_mesh_bind_group_layout(
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
                    ok = ok && dvz_drp2_stream_create_texture_sampler_bind_group_layout(
                                   stream, img_bgl_id);
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
            ok = ok && dvz_drp2_stream_create_render_pipeline_ex2(
                           stream, pipe_id, vs_id, fs_id, pipeline.vertex_buffer_count,
                           pipeline.topology, pipeline.binding_count, pipeline.strides,
                           pipeline.step_modes, pipeline.attr_count, pipeline.bindings,
                           pipeline.locations, pipeline.formats, pipeline.offsets);
            if (ok && pass_sample_count > 1)
                ok = dvz_drp2_stream_pipeline_set_multisampling(
                    stream, pass_sample_count, pipeline.alpha_to_coverage);
            if (ok && shader.builtin_pipeline != NULL)
                ok = dvz_drp2_stream_pipeline_set_builtin_identity(
                    stream, pipe_id, shader.builtin_pipeline, 1);
            if (ok)
            {
                uint64_t layouts[DVZ_DRP2_MAX_BIND_GROUPS] = {0};
                uint32_t layout_count = 0;
                bool depth_peel_iter_pass =
                    render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER &&
                    depth_peel_sampled_bgl_id != 0;
                if (depth_peel_iter_pass)
                {
                    uint64_t dummy_bgl_id = _obj_id(emitter, "_bgl_unused_set", &is_new);
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
                        item_state_style_bgl_id, scene_occlusion_bgl_id,
                        scene_occlusion_uses_set2, layouts, &layout_count);
                }
                if (layout_count > 0)
                    ok = dvz_drp2_stream_pipeline_set_bind_group_layouts(
                        stream, layout_count, layouts);
            }
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
                     dvz_drp2_stream_pipeline_set_color_target(stream, 1, VK_FORMAT_R16_SFLOAT) &&
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
            else if (ok && depth_peel_pass)
            {
                ok = ok &&
                     dvz_drp2_stream_pipeline_set_color_target(
                         stream, 0, VK_FORMAT_R16G16B16A16_SFLOAT) &&
                     dvz_drp2_stream_pipeline_set_color_target(
                         stream, 1, VK_FORMAT_R16G16B16A16_SFLOAT) &&
                     dvz_drp2_stream_pipeline_set_color_target(
                         stream, 2, VK_FORMAT_R16G16B16A16_SFLOAT) &&
                     dvz_drp2_stream_pipeline_set_color_blend(
                         stream, 0, VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA, VK_BLEND_FACTOR_ONE,
                         VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
                         VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD,
                         VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                             VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT) &&
                     dvz_drp2_stream_pipeline_set_color_blend(
                         stream, 1, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                         VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE,
                         VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD,
                         VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                             VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT) &&
                     dvz_drp2_stream_pipeline_set_color_blend(
                         stream, 2, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_MAX,
                         VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_MAX,
                         VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT);
                if (ok)
                    ok = dvz_drp2_stream_pipeline_set_raster_state(
                        stream, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
            }
            else if (ok && gbuffer_pass)
            {
                ok = dvz_drp2_stream_pipeline_set_color_target(
                    stream, 0, VK_FORMAT_R16G16B16A16_SFLOAT);
            }
            else if (ok && (volume_occlusion_pass || scene_occlusion_pass))
            {
                ok = dvz_drp2_stream_pipeline_set_color_target(stream, 0, VK_FORMAT_R32_SFLOAT);
            }
            else if (ok && cfg != NULL && cfg->color_target_format != 0)
            {
                ok = dvz_drp2_stream_pipeline_set_color_target(
                    stream, 0, cfg->color_target_format);
            }
            else if (ok && (_scene_alpha_mode_is_blended(alpha_mode) || segment_coverage_blend))
            {
                ok = dvz_drp2_stream_pipeline_set_color_blend(
                    stream, 0, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                    VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                    VK_BLEND_OP_ADD,
                    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);
            }
            if (!ok)
                _diagnostic(report, "scene render pipeline setup failed");
        }

        /* Bind group at set 0. */
        uint64_t vis_bg_set0 = 0;
        uint64_t vis_bg_set1 = 0;
        uint64_t vis_bg_set2 = 0;
        uint64_t vis_bg_set3 = 0;
        DvzSceneVisualBindDesc bind = {0};
        if (!_scene_visual_bind_desc(&desc, render->u.render.controller_modes[i], &bind))
        {
            ok = false;
            break;
        }
        _scene_visual_bind_desc_apply_pass_policy(
            &bind, render->u.render.pass_role, sampled_depth_id, sampled_depth_is_volume_occlusion,
            scene_occlusion_depth_id);
        if (bind.uses_common_set0)
        {
            if (render->u.render.visual_has_mvp[i])
            {
                ok = _scene_common_bindings_resolve_visual_set(
                    emitter, stream, render, i, common_bgl_id, &vis_bg_set0);
                if (!ok)
                    break;
            }
            else if (bind.uses_fixed_common)
                vis_bg_set0 = fixed_bg_id;
            else if (bind.controller_mode == DVZ_CONTROLLER_APPLY_ISOTROPIC_LOCAL)
                vis_bg_set0 = isotropic_bg_id;
            else
                vis_bg_set0 = apply_bg_id;
        }
        if (bind.uses_item_state_style_set1)
        {
            if (bind.material_buffer_id == 0 || bind.item_state_style_buffer_id == 0)
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
                           bind.item_state_style_buffer_id, &item_state_style_bg_id);
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
            char material_bg_key[64];
            dvz_snprintf(
                material_bg_key, sizeof(material_bg_key), "_bg_material_params_%" PRIu64,
                bind.material_buffer_id);
            uint64_t material_bg_id = _obj_id(emitter, material_bg_key, &is_new);
            if (material_bg_id == 0)
            {
                ok = false;
                break;
            }
            if (ok && is_new)
                ok = ok && dvz_drp2_stream_create_uniform_bind_group(
                               stream, material_bg_id, material_bgl_id, bind.material_buffer_id, 0,
                               sizeof(DvzSceneMaterialParams));
            vis_bg_set1 = material_bg_id;
        }
        if (bind.uses_image_set1 && bind.uses_textured_mesh_set1)
        {
            if (!_resolve_textured_mesh_bind_group_layout(emitter, stream, &textured_mesh_bgl_id))
            {
                ok = false;
                break;
            }
            if (bind.material_buffer_id == 0)
            {
                _diagnostic(report, "textured mesh render missing material params buffer");
                ok = false;
                break;
            }
            if (img_sampler_linear_id == 0)
            {
                img_sampler_linear_id = _obj_id(emitter, "_sampler_img", &is_new);
                if (img_sampler_linear_id == 0)
                {
                    ok = false;
                    break;
                }
                if (ok && is_new)
                    ok = ok && dvz_drp2_stream_create_sampler_filter(
                                   stream, img_sampler_linear_id, DVZ_DRP2_FILTER_LINEAR,
                                   DVZ_DRP2_FILTER_LINEAR);
            }
            uint64_t mesh_bg_id = 0;
            ok = ok && _resolve_textured_mesh_bind_group(
                           emitter, stream, textured_mesh_bgl_id, bind.material_buffer_id,
                           bind.image_texture_id, img_sampler_linear_id, &mesh_bg_id);
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
                    ok = ok && dvz_drp2_stream_create_texture_sampler_bind_group_layout(
                                   stream, img_bgl_id);
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
                    DvzDrp2FilterMode filter = bind.image_nearest_sampler ?
                                                   DVZ_DRP2_FILTER_NEAREST :
                                                   DVZ_DRP2_FILTER_LINEAR;
                    ok = ok && dvz_drp2_stream_create_sampler_filter(
                                   stream, *img_sampler_id, filter, filter);
                }
            }
            char img_bg_key[64];
            dvz_snprintf(
                img_bg_key, sizeof(img_bg_key), bind.image_nearest_sampler ?
                                                    "_bg_img_nearest_%" PRIu64 :
                                                    "_bg_img_%" PRIu64,
                bind.image_texture_id);
            uint64_t img_bg_id = _obj_id(emitter, img_bg_key, &is_new);
            if (img_bg_id == 0)
            {
                ok = false;
                break;
            }
            if (ok && is_new)
                ok = ok &&
                     dvz_drp2_stream_create_texture_sampler_bind_group(
                         stream, img_bg_id, img_bgl_id, bind.image_texture_id, *img_sampler_id);
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
            ok = ok && _resolve_labels_bind_group(
                           emitter, stream, labels_bgl_id, labels_sampler_id, &bind,
                           &labels_bg_id);
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
        if (render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER)
        {
            if (vis_bg_set1 == 0)
                vis_bg_set1 = depth_peel_dummy_bg_id;
            if (vis_bg_set2 == 0)
                vis_bg_set2 = depth_peel_dummy_bg_id;
            vis_bg_set3 = depth_peel_sampled_bg_id;
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
            &emitter->resources, &desc, &pipeline, pipe_id, vis_bg_set0, vis_bg_set1,
            vis_bg_set2, vis_bg_set3, clip_rect, shader_format, report, &draws[draw_count]);
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
