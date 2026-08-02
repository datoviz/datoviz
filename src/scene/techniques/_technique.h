/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene technique planning                                                                     */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_scene.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

#define DVZ_SCENE_DEPTH_PEEL_ITERATIONS 4
#define DVZ_SCENE_DEPTH_PEEL_BIND_SET 3

typedef struct DvzSceneGBufferPlan
{
    bool enabled;
    bool needs_depth;
    bool needs_normal;
    bool needs_object_id;
    uint32_t producer_count;
} DvzSceneGBufferPlan;


typedef struct DvzSceneDepthPostProcessGraphDesc
{
    const char* color_suffix;
    const char* depth_suffix;
    const char* resolve_suffix;
    const char* resolve_work_label;
    uint32_t color_format;
    DvzFrameGraphAttachmentLoadOp resolve_load_op;
    bool resolve_clear;
} DvzSceneDepthPostProcessGraphDesc;


typedef struct DvzSceneTechniquePassPolicy
{
    DvzFramePlanRenderPassRole role;
    const char* work_label;
    bool graph_required;
    bool transparent_blend;
    bool wboit_accumulation;
    bool depth_peel;
    bool fullscreen_resolve;
    bool needs_wboit_resolve_layout;
    bool needs_depth_peel_sampled_layout;
    uint32_t sampled_texture_binding_count;
} DvzSceneTechniquePassPolicy;


typedef DvzAoDesc DvzSceneAoDesc;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

bool _scene_alpha_mode_is_wboit(DvzAlphaMode mode);

bool _scene_alpha_mode_is_depth_peel(DvzAlphaMode mode);

bool _scene_alpha_mode_is_blended(DvzAlphaMode mode);

const char* _scene_render_role_work_label(DvzFramePlanRenderPassRole role);

bool _scene_render_role_requires_graph_pass(DvzFramePlanRenderPassRole role);

bool _scene_technique_pass_policy(
    DvzFramePlanRenderPassRole role, DvzSceneTechniquePassPolicy* out);

bool _scene_visual_writes_depth(const DvzVisual* visual, const DvzPanelAttach* attach);

bool _scene_transparent_visual_needs_depth(
    const DvzVisual* visual, const DvzPanelAttach* attach);

void _scene_technique_state_init(DvzSceneTechniqueState* state);

void _scene_technique_state_enable_gbuffer(DvzSceneTechniqueState* state, bool enabled);

bool _scene_technique_state_gbuffer_enabled(const DvzSceneTechniqueState* state);

bool _scene_technique_gbuffer_enabled(const DvzScene* scene, const DvzPanel* panel);

bool _scene_technique_state_set_edl(
    DvzSceneTechniqueState* state, const DvzEdlDesc* desc);

bool _scene_technique_state_edl_enabled(const DvzSceneTechniqueState* state);

const DvzSceneEdlTechniqueState*
_scene_technique_edl_state(const DvzScene* scene, const DvzPanel* panel);

bool _scene_technique_state_set_ao(
    DvzSceneTechniqueState* state, const DvzSceneAoDesc* desc);

bool _scene_technique_state_ao_enabled(const DvzSceneTechniqueState* state);

const DvzSceneAoTechniqueState*
_scene_technique_ao_state(const DvzScene* scene, const DvzPanel* panel);

bool _scene_technique_state_set_msaa(
    DvzSceneTechniqueState* state, const DvzMsaaDesc* desc);

bool _scene_technique_state_msaa_enabled(const DvzSceneTechniqueState* state);

const DvzSceneMsaaTechniqueState*
_scene_technique_msaa_state(const DvzScene* scene, const DvzPanel* panel);

void _scene_technique_edl_uniform(
    const DvzSceneEdlTechniqueState* edl, const DvzMVP* mvp,
    const DvzSceneViewportUniform* viewport, DvzSceneEdlUniform* out);

void _scene_technique_ao_uniform(
    const DvzSceneAoTechniqueState* ao, const DvzMVP* mvp,
    const DvzSceneViewportUniform* viewport, DvzSceneAoUniform* out);

bool _scene_technique_emit_wboit_frame_graph(
    DvzFramePlan* plan, const char* panel_id, bool opaque_needs_depth,
    bool transparent_needs_depth);

bool _scene_technique_emit_blended_frame_graph(
    DvzFramePlan* plan, const char* panel_id, bool emit_opaque_pass, bool opaque_needs_depth,
    bool prior_depth_written, uint32_t pass_count, const bool* pass_needs_depth,
    const bool* pass_writes_depth, const DvzSceneMsaaTechniqueState* msaa);

bool _scene_technique_emit_volume_occlusion_frame_graph(
    DvzFramePlan* plan, const char* panel_id);

bool _scene_technique_emit_scene_occlusion_frame_graph(
    DvzFramePlan* plan, const char* panel_id);

bool _scene_technique_emit_depth_peel_frame_graph(
    DvzFramePlan* plan, const char* panel_id, bool opaque_needs_depth,
    bool transparent_needs_depth);

bool _scene_technique_emit_opaque_frame_graph(
    DvzFramePlan* plan, const char* panel_id, bool needs_depth,
    const DvzSceneMsaaTechniqueState* msaa);

bool _scene_technique_emit_depth_postprocess_frame_graph(
    DvzFramePlan* plan, const char* panel_id,
    const DvzSceneDepthPostProcessGraphDesc* desc);

bool _scene_technique_emit_edl_frame_graph(DvzFramePlan* plan, const char* panel_id);

void _scene_technique_gbuffer_plan_init(DvzSceneGBufferPlan* plan);

bool _scene_technique_gbuffer_plan_add_visual(
    DvzSceneGBufferPlan* plan, const DvzVisual* visual, const DvzPanelAttach* attach);

bool _scene_technique_emit_gbuffer_frame_graph(
    DvzFramePlan* plan, const char* panel_id, const DvzSceneGBufferPlan* gbuffer);

bool _scene_technique_emit_ssao_frame_graph(
    DvzFramePlan* plan, const char* panel_id, const DvzSceneGBufferPlan* gbuffer,
    const DvzSceneAoTechniqueState* ssao);
