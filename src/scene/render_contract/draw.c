/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene render draw contracts                                                                  */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "internal.h"

#include <vulkan/vulkan_core.h>

#include "_alloc.h"
#include "_assertions.h"
#include "scene_emit/visual_lowering.h"
#include "_visual_pipeline.h"
#include "_visual_pipeline_internal.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return whether one render-pass role carries retained scene visual draws.
 *
 * @param role the render-pass role
 * @return whether the role may contain ordinary visual draws
 */
static bool _role_is_visual_pass(DvzFramePlanRenderPassRole role)
{
    return role == DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE ||
           role == DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND ||
           role == DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION ||
           role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT ||
           role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER;
}



/**
 * Return whether one draw contract belongs in one pass role.
 *
 * @param draw the resolved draw contract
 * @return whether the alpha mode and pass role match
 */
bool _draw_pass_role_matches(const DvzSceneDrawContract* draw)
{
    ANN(draw);
    switch (draw->pass_role)
    {
    case DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE:
        return draw->alpha_mode == DVZ_ALPHA_OPAQUE || draw->alpha_mode == DVZ_ALPHA_MASK;
    case DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND:
        return draw->alpha_mode == DVZ_ALPHA_BLENDED;
    case DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION:
        return draw->alpha_mode == DVZ_ALPHA_WBOIT;
    case DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT:
    case DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER:
        return draw->alpha_mode == DVZ_ALPHA_DEPTH_PEEL;
    case DVZ_FRAME_PLAN_RENDER_PASS_VOLUME_OCCLUSION:
        return draw->writes_volume_occlusion_depth;
    case DVZ_FRAME_PLAN_RENDER_PASS_SCENE_OCCLUSION:
        return draw->writes_scene_occlusion_depth;
    default:
        return true;
    }
}



/**
 * Resolve a draw depth policy from its component depth requirements.
 *
 * @param depth_test whether fixed-function depth testing is required
 * @param depth_write whether fixed-function depth writes are required
 * @param samples_depth whether the shader samples a produced depth resource
 * @return depth-policy bit mask
 */
static uint32_t _draw_depth_policy(bool depth_test, bool depth_write, bool samples_depth)
{
    uint32_t policy = DVZ_SCENE_DEPTH_POLICY_NONE;
    if (depth_test)
        policy |= DVZ_SCENE_DEPTH_POLICY_TEST;
    if (depth_write)
        policy |= DVZ_SCENE_DEPTH_POLICY_WRITE;
    if (samples_depth)
        policy |= DVZ_SCENE_DEPTH_POLICY_SAMPLE;
    return policy;
}



/**
 * Resolve a draw blend policy from alpha mode and render-pass role.
 *
 * @param facts visual facts used by the resolver matrix
 * @param pass_role the render-pass role carrying the draw
 * @return resolved blend policy
 */
static DvzSceneBlendPolicy _draw_blend_policy(
    const DvzSceneDrawFacts* facts, DvzFramePlanRenderPassRole pass_role)
{
    ANN(facts);
    switch (pass_role)
    {
    case DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND:
        return DVZ_SCENE_BLEND_POLICY_SOURCE_OVER;
    case DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION:
        return DVZ_SCENE_BLEND_POLICY_WBOIT;
    case DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT:
    case DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER:
        return DVZ_SCENE_BLEND_POLICY_DEPTH_PEEL;
    case DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE:
        if (facts->uses_segment_pipeline &&
            (facts->alpha_mode == DVZ_ALPHA_OPAQUE || facts->alpha_mode == DVZ_ALPHA_MASK))
            return DVZ_SCENE_BLEND_POLICY_SEGMENT_COVERAGE;
        return facts->alpha_mode == DVZ_ALPHA_OPAQUE || facts->alpha_mode == DVZ_ALPHA_MASK
                   ? DVZ_SCENE_BLEND_POLICY_OPAQUE
                   : DVZ_SCENE_BLEND_POLICY_NONE;
    default:
        return DVZ_SCENE_BLEND_POLICY_NONE;
    }
}



/**
 * Resolve exact color-target blend contracts from a draw blend policy.
 *
 * @param blend_policy the resolved draw blend policy
 * @param targets output color-target contracts
 * @param target_count output target contract count
 */
void _draw_blend_target_contracts(
    DvzSceneBlendPolicy blend_policy, DvzSceneBlendTargetContract* targets,
    uint32_t* target_count)
{
    ANN(targets);
    ANN(target_count);
    *target_count = 0;

    if (
        blend_policy == DVZ_SCENE_BLEND_POLICY_SOURCE_OVER ||
        blend_policy == DVZ_SCENE_BLEND_POLICY_SEGMENT_COVERAGE)
    {
        targets[0] = (DvzSceneBlendTargetContract){
            .target_index = 0,
            .blend_enabled = true,
            .src_color_blend_factor = DVZ_BLEND_FACTOR_SRC_ALPHA,
            .dst_color_blend_factor = DVZ_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .color_blend_op = DVZ_BLEND_OP_ADD,
            .src_alpha_blend_factor = DVZ_BLEND_FACTOR_ONE,
            .dst_alpha_blend_factor = DVZ_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .alpha_blend_op = DVZ_BLEND_OP_ADD,
            .color_write_mask = DVZ_MASK_COLOR_R | DVZ_MASK_COLOR_G |
                                DVZ_MASK_COLOR_B | DVZ_MASK_COLOR_A,
        };
        *target_count = 1;
    }
    else if (blend_policy == DVZ_SCENE_BLEND_POLICY_WBOIT)
    {
        targets[0] = (DvzSceneBlendTargetContract){
            .target_index = 0,
            .format = DVZ_FORMAT_R16G16B16A16_SFLOAT,
            .blend_enabled = true,
            .src_color_blend_factor = DVZ_BLEND_FACTOR_ONE,
            .dst_color_blend_factor = DVZ_BLEND_FACTOR_ONE,
            .color_blend_op = DVZ_BLEND_OP_ADD,
            .src_alpha_blend_factor = DVZ_BLEND_FACTOR_ONE,
            .dst_alpha_blend_factor = DVZ_BLEND_FACTOR_ONE,
            .alpha_blend_op = DVZ_BLEND_OP_ADD,
            .color_write_mask = DVZ_MASK_COLOR_R | DVZ_MASK_COLOR_G |
                                DVZ_MASK_COLOR_B | DVZ_MASK_COLOR_A,
        };
        targets[1] = targets[0];
        targets[1].target_index = 1;
        targets[1].format = DVZ_FORMAT_R16_SFLOAT;
        targets[1].color_write_mask = DVZ_MASK_COLOR_R;
        *target_count = 2;
    }
    else if (blend_policy == DVZ_SCENE_BLEND_POLICY_DEPTH_PEEL)
    {
        for (uint32_t i = 0; i < 3; i++)
        {
            targets[i] = (DvzSceneBlendTargetContract){
                .target_index = i,
                .format = i < 2 ? DVZ_FORMAT_R16G16B16A16_SFLOAT : DVZ_FORMAT_R32G32_SFLOAT,
                .blend_enabled = true,
                .src_color_blend_factor = i == 0 ? DVZ_BLEND_FACTOR_ONE_MINUS_DST_ALPHA :
                                                    DVZ_BLEND_FACTOR_ONE,
                .dst_color_blend_factor = i == 0 ? DVZ_BLEND_FACTOR_ONE :
                                                    i == 1 ? DVZ_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA :
                                                             DVZ_BLEND_FACTOR_ONE,
                .color_blend_op = i < 2 ? DVZ_BLEND_OP_ADD : DVZ_BLEND_OP_MAX,
                .src_alpha_blend_factor = i == 0 ? DVZ_BLEND_FACTOR_ONE_MINUS_DST_ALPHA :
                                                   DVZ_BLEND_FACTOR_ONE,
                .dst_alpha_blend_factor = i == 0 ? DVZ_BLEND_FACTOR_ONE :
                                                    i == 1 ? DVZ_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA :
                                                             DVZ_BLEND_FACTOR_ONE,
                .alpha_blend_op = i < 2 ? DVZ_BLEND_OP_ADD : DVZ_BLEND_OP_MAX,
                .color_write_mask = i < 2 ? DVZ_MASK_COLOR_R | DVZ_MASK_COLOR_G |
                                                DVZ_MASK_COLOR_B |
                                                DVZ_MASK_COLOR_A
                                          : DVZ_MASK_COLOR_R | DVZ_MASK_COLOR_G,
            };
        }
        *target_count = 3;
    }
    else if (blend_policy == DVZ_SCENE_BLEND_POLICY_OPAQUE)
    {
        targets[0] = (DvzSceneBlendTargetContract){
            .target_index = 0,
            .blend_enabled = false,
            .color_write_mask = DVZ_MASK_COLOR_R | DVZ_MASK_COLOR_G |
                                DVZ_MASK_COLOR_B | DVZ_MASK_COLOR_A,
        };
        *target_count = 1;
    }
}



/**
 * Resolve fixed-function raster-state requirements from visual facts and pass role.
 *
 * @param facts visual facts used by the resolver matrix
 * @param pass_role the render pass role carrying the draw
 * @param out_has_raster_state output flag indicating whether raster state is contracted
 * @param out_cull_mode output Vulkan cull mode when raster state is contracted
 * @param out_front_face output Vulkan front face when raster state is contracted
 */
void _draw_raster_state_contract(
    const DvzSceneDrawFacts* facts, DvzFramePlanRenderPassRole pass_role,
    bool* out_has_raster_state, uint32_t* out_cull_mode, uint32_t* out_front_face)
{
    ANN(facts);
    ANN(out_has_raster_state);
    ANN(out_cull_mode);
    ANN(out_front_face);
    *out_has_raster_state = false;
    *out_cull_mode = 0;
    *out_front_face = 0;

    if (pass_role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT)
    {
        *out_has_raster_state = true;
        *out_cull_mode = DVZ_CULL_MODE_NONE;
        *out_front_face = DVZ_FRONT_FACE_COUNTER_CLOCKWISE;
    }
    else if (pass_role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER)
    {
        *out_has_raster_state = true;
        *out_cull_mode = DVZ_CULL_MODE_NONE;
        *out_front_face = DVZ_FRONT_FACE_COUNTER_CLOCKWISE;
    }
    else if (_scene_visual_desc_is_volume((DvzSceneVisualDescKind)facts->desc_kind))
    {
        *out_has_raster_state = true;
        *out_cull_mode = DVZ_CULL_MODE_BACK;
        *out_front_face = DVZ_FRONT_FACE_CLOCKWISE;
    }
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve one visual-facts row and pass role into an explicit draw contract.
 *
 * @param facts visual facts used by the resolver matrix
 * @param pass_role the render pass role that will carry the draw
 * @param out the output draw contract
 * @return whether the draw contract was resolved
 */
bool _scene_draw_contract_resolve(
    const DvzSceneDrawFacts* facts, DvzFramePlanRenderPassRole pass_role,
    DvzSceneDrawContract* out)
{
    ANN(facts);
    ANN(out);
    dvz_memset(out, sizeof(DvzSceneDrawContract), 0, sizeof(DvzSceneDrawContract));

    bool scene_depth_pass = pass_role == DVZ_FRAME_PLAN_RENDER_PASS_SCENE_OCCLUSION;
    bool ordinary_visual_pass = _role_is_visual_pass(pass_role);
    out->visual_type = facts->visual_type;
    out->alpha_mode = facts->alpha_mode;
    out->pass_role = pass_role;
    out->depth_test = facts->can_depth_test && (ordinary_visual_pass || scene_depth_pass);
    out->depth_write = facts->writes_depth || (scene_depth_pass && facts->can_write_depth);
    out->samples_depth =
        facts->samples_depth && ordinary_visual_pass &&
        pass_role != DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE;
    out->samples_volume_occlusion = facts->volume_occluded && ordinary_visual_pass;
    out->samples_scene_occlusion = facts->scene_occluded && ordinary_visual_pass;
    out->writes_volume_occlusion_depth =
        pass_role == DVZ_FRAME_PLAN_RENDER_PASS_VOLUME_OCCLUSION &&
        _scene_visual_desc_is_volume((DvzSceneVisualDescKind)facts->desc_kind);
    out->writes_scene_occlusion_depth =
        pass_role == DVZ_FRAME_PLAN_RENDER_PASS_SCENE_OCCLUSION && facts->scene_occluder;
    out->needs_common_set = facts->uses_common_set;
    out->needs_material_set = facts->uses_material_set;
    out->needs_labels_set = facts->uses_labels_set && facts->uses_image_set;
    out->needs_glyph_set = facts->uses_glyph_set && facts->uses_image_set;
    out->needs_image_set =
        facts->uses_image_set && !out->needs_labels_set && !out->needs_glyph_set;
    out->needs_volume_set = facts->uses_volume_set;
    if (pass_role == DVZ_FRAME_PLAN_RENDER_PASS_GBUFFER &&
        !_scene_visual_desc_is_sphere((DvzSceneVisualDescKind)facts->desc_kind))
        out->needs_material_set = false;
    if (scene_depth_pass)
    {
        out->needs_material_set = false;
        out->needs_image_set = false;
        out->needs_glyph_set = false;
    }
    out->needs_scene_occlusion_set = out->samples_scene_occlusion;
    out->depth_policy =
        _draw_depth_policy(out->depth_test, out->depth_write, out->samples_depth);
    out->blend_policy = _draw_blend_policy(facts, pass_role);
    _draw_blend_target_contracts(
        out->blend_policy, out->blend_targets, &out->blend_target_count);
    _draw_raster_state_contract(
        facts, pass_role, &out->has_raster_state, &out->cull_mode, &out->front_face);
    if (out->samples_depth)
        out->shader_feature_mask |= DVZ_SCENE_SHADER_FEATURE_SAMPLE_DEPTH;
    if (out->samples_volume_occlusion)
        out->shader_feature_mask |= DVZ_SCENE_SHADER_FEATURE_SAMPLE_VOLUME_OCCLUSION;
    if (out->samples_scene_occlusion)
        out->shader_feature_mask |= DVZ_SCENE_SHADER_FEATURE_SAMPLE_SCENE_OCCLUSION;
    if (out->writes_volume_occlusion_depth)
        out->shader_feature_mask |= DVZ_SCENE_SHADER_FEATURE_WRITE_VOLUME_OCCLUSION;
    if (out->writes_scene_occlusion_depth)
        out->shader_feature_mask |= DVZ_SCENE_SHADER_FEATURE_WRITE_SCENE_OCCLUSION;
    if (out->needs_common_set)
        out->bind_group_layout_mask |= DVZ_SCENE_BIND_GROUP_REQUIREMENT_COMMON;
    if (out->needs_material_set)
        out->bind_group_layout_mask |= DVZ_SCENE_BIND_GROUP_REQUIREMENT_MATERIAL;
    if (out->needs_image_set)
        out->bind_group_layout_mask |= DVZ_SCENE_BIND_GROUP_REQUIREMENT_IMAGE;
    if (out->needs_labels_set)
        out->bind_group_layout_mask |= DVZ_SCENE_BIND_GROUP_REQUIREMENT_LABELS;
    if (out->needs_glyph_set)
        out->bind_group_layout_mask |= DVZ_SCENE_BIND_GROUP_REQUIREMENT_GLYPH;
    if (out->needs_volume_set)
        out->bind_group_layout_mask |= DVZ_SCENE_BIND_GROUP_REQUIREMENT_VOLUME;
    if (out->needs_scene_occlusion_set)
        out->bind_group_layout_mask |= DVZ_SCENE_BIND_GROUP_REQUIREMENT_SCENE_OCCLUSION;
    return true;
}



/**
 * Resolve one retained visual draw into a passive render contract.
 *
 * @param visual the retained visual
 * @param attach the panel attachment
 * @param pass_role the render pass role that will carry the draw
 * @param out the output draw contract
 * @return whether the draw contract was resolved
 */
bool _scene_draw_contract_from_visual(
    const DvzVisual* visual, const DvzPanelAttach* attach, DvzFramePlanRenderPassRole pass_role,
    DvzSceneDrawContract* out)
{
    ANN(visual);
    ANN(attach);
    ANN(out);

    DvzSceneVisualPassCaps caps = {0};
    if (!_scene_visual_pass_caps_from_visual(visual, attach, &caps))
        return false;
    bool forward_depth_compare = visual->depth_compare_op == DVZ_COMPARE_OP_LESS ||
                                 visual->depth_compare_op == DVZ_COMPARE_OP_LESS_OR_EQUAL;

    DvzSceneDrawFacts facts = {
        .visual_type = (uint32_t)visual->type,
        .desc_kind = (uint32_t)caps.kind,
        .alpha_mode = visual->alpha_mode,
        .can_depth_test = caps.can_depth_test,
        .can_write_depth = caps.can_write_depth,
        .writes_depth = caps.writes_depth && forward_depth_compare,
        .samples_depth = caps.samples_depth,
        .volume_occluded = _scene_visual_lowering_volume_occluded(visual),
        .scene_occluded = visual->scene_occluded,
        .scene_occluder = visual->scene_occluder,
        .uses_segment_pipeline = _scene_visual_desc_uses_coverage_blend(caps.kind),
        .uses_common_set = caps.uses_common_set,
        .uses_material_set = caps.uses_material_set,
        .uses_image_set = caps.uses_image_set,
        .uses_labels_set = caps.uses_labels_set,
        .uses_glyph_set = caps.uses_glyph_set,
        .uses_volume_set = caps.uses_volume_set,
    };
    return _scene_draw_contract_resolve(&facts, pass_role, out);
}
