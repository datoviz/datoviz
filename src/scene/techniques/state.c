/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene technique state */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <float.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <vulkan/vulkan_core.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_scene.h"
#include "_scene_resource_key.h"
#include "_technique.h"
#include "_technique_internal.h"
#include "_visual_pipeline.h"
#include "datoviz/scene.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_EDL_DESC_KNOWN_FLAGS  0u
#define DVZ_MSAA_DESC_KNOWN_FLAGS 0u
#define DVZ_AO_DESC_KNOWN_FLAGS   0u



static bool _edl_desc_validate(const DvzEdlDesc* desc)
{
    if (desc == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(desc, DvzEdlDesc, DVZ_EDL_DESC_KNOWN_FLAGS))
    {
        log_error("invalid DvzEdlDesc ABI prologue");
        return false;
    }
    return true;
}



static bool _msaa_desc_validate(const DvzMsaaDesc* desc)
{
    if (desc == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(desc, DvzMsaaDesc, DVZ_MSAA_DESC_KNOWN_FLAGS))
    {
        log_error("invalid DvzMsaaDesc ABI prologue");
        return false;
    }
    return true;
}



static bool _ao_desc_validate(const DvzAoDesc* desc)
{
    if (desc == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(desc, DvzAoDesc, DVZ_AO_DESC_KNOWN_FLAGS))
    {
        log_error("invalid DvzAoDesc ABI prologue");
        return false;
    }
    if (desc->quality < DVZ_AO_QUALITY_LOW || desc->quality > DVZ_AO_QUALITY_ULTRA)
    {
        log_error("invalid ambient-occlusion quality");
        return false;
    }
    if (desc->debug_mode < DVZ_AO_DEBUG_NONE || desc->debug_mode > DVZ_AO_DEBUG_VISIBILITY)
    {
        log_error("invalid ambient-occlusion debug mode");
        return false;
    }
    return true;
}



const DvzSceneTechniquePassPolicy TECHNIQUE_PASS_POLICIES[] = {
    {
        .role = DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE,
    },
    {
        .role = DVZ_FRAME_PLAN_RENDER_PASS_GBUFFER,
    },
    {
        .role = DVZ_FRAME_PLAN_RENDER_PASS_SURFACE_RESOLVE,
    },
    {
        .role = DVZ_FRAME_PLAN_RENDER_PASS_VOLUME_OCCLUSION,
    },
    {
        .role = DVZ_FRAME_PLAN_RENDER_PASS_SCENE_OCCLUSION,
    },
    {
        .role = DVZ_FRAME_PLAN_RENDER_PASS_GTAO,
    },
    {
        .role = DVZ_FRAME_PLAN_RENDER_PASS_GTAO_DENOISE,
    },
    {
        .role = DVZ_FRAME_PLAN_RENDER_PASS_GTAO_VISIBILITY_PRESENTATION,
        .fullscreen_resolve = true,
    },
    {
        .role = DVZ_FRAME_PLAN_RENDER_PASS_EDL_RESOLVE,
        .fullscreen_resolve = true,
    },
    {
        .role = DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION,
        .wboit_accumulation = true,
    },
    {
        .role = DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND,
        .transparent_blend = true,
    },
    {
        .role = DVZ_FRAME_PLAN_RENDER_PASS_WBOIT_RESOLVE,
        .fullscreen_resolve = true,
        .needs_wboit_resolve_layout = true,
        .sampled_texture_binding_count = 2,
    },
    {
        .role = DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT,
        .depth_peel = true,
    },
    {
        .role = DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER,
        .depth_peel = true,
        .needs_depth_peel_sampled_layout = true,
        .sampled_texture_binding_count = 1,
    },
    {
        .role = DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_COMPOSITE,
        .fullscreen_resolve = true,
        .needs_depth_peel_sampled_layout = true,
        .sampled_texture_binding_count = 2,
    },
    {
        .role = DVZ_FRAME_PLAN_RENDER_PASS_PRESENTATION,
        .fullscreen_resolve = true,
        .sampled_texture_binding_count = 1,
    },
    {
        .role = DVZ_FRAME_PLAN_RENDER_PASS_PICKING,
    },
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return whether a sample count is supported by the scene MSAA planner.
 *
 * @param sample_count requested sample count
 * @return whether the value maps to a Vulkan color sample count
 */
bool _scene_msaa_sample_count_valid(uint32_t sample_count)
{
    return sample_count == 1 || sample_count == 2 || sample_count == 4 || sample_count == 8 ||
           sample_count == 16;
}



/**
 * Return whether visual capabilities can feed a primitive/mesh G-buffer.
 *
 * @param caps the resolved visual pass capabilities
 * @return whether the visual can write the G-buffer depth and normal targets
 */
bool _scene_caps_support_gbuffer(const DvzSceneVisualPassCaps* caps)
{
    ANN(caps);
    return caps->eligible_for_gbuffer;
}



/**
 * Reset an internal G-buffer plan.
 *
 * @param plan the G-buffer plan
 */
void _scene_technique_gbuffer_plan_init(DvzSceneGBufferPlan* plan)
{
    ANN(plan);
    dvz_memset(plan, sizeof(DvzSceneGBufferPlan), 0, sizeof(DvzSceneGBufferPlan));
}



/**
 * Add one retained visual to an internal G-buffer plan when it is eligible.
 *
 * @param plan the G-buffer plan
 * @param visual the retained visual
 * @param attach the panel attachment
 * @return whether the visual was accepted as a G-buffer producer
 */
bool _scene_technique_gbuffer_plan_add_visual(
    DvzSceneGBufferPlan* plan, const DvzVisual* visual, const DvzPanelAttach* attach)
{
    ANN(plan);
    ANN(visual);
    ANN(attach);

    DvzSceneVisualPassCaps caps = {0};
    if (!_scene_visual_pass_caps_from_visual(visual, attach, &caps) ||
        !_scene_caps_support_gbuffer(&caps))
        return false;

    plan->enabled = true;
    plan->needs_depth = true;
    plan->needs_normal = true;
    plan->producer_count++;
    return true;
}



/**
 * Clamp a float into a closed range.
 *
 * @param value input value
 * @param min_value minimum accepted value
 * @param max_value maximum accepted value
 * @return clamped value
 */
float _clampf(float value, float min_value, float max_value)
{
    if (value < min_value)
        return min_value;
    if (value > max_value)
        return max_value;
    return value;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Reset internal scene technique activation state.
 *
 * @param state the technique state
 */
void _scene_technique_state_init(DvzSceneTechniqueState* state)
{
    ANN(state);
    dvz_memset(state, sizeof(DvzSceneTechniqueState), 0, sizeof(DvzSceneTechniqueState));
    state->edl.radius = 1.5f;
    state->edl.strength = 35.0f;
    state->edl.depth_scale = 1.0f;
    state->ao.radius = 1.0f;
    state->ao.intensity = 1.0f;
    state->ao.thickness = 0.25f;
    state->ao.quality = DVZ_AO_QUALITY_MEDIUM;
    state->ao.debug_mode = DVZ_AO_DEBUG_NONE;
    state->ao.denoise_enabled = true;
    state->msaa.sample_count = 1;
}



/**
 * Enable or disable the internal G-buffer technique.
 *
 * @param state the technique state
 * @param enabled whether the G-buffer technique is enabled
 */
void _scene_technique_state_enable_gbuffer(DvzSceneTechniqueState* state, bool enabled)
{
    ANN(state);
    state->gbuffer.enabled = enabled;
}



/**
 * Return whether a technique state enables the G-buffer technique.
 *
 * @param state the technique state
 * @return whether G-buffer is enabled
 */
bool _scene_technique_state_gbuffer_enabled(const DvzSceneTechniqueState* state)
{
    return state != NULL && state->gbuffer.enabled;
}



/**
 * Return whether the effective scene/panel state enables the G-buffer technique.
 *
 * @param scene the scene-level state owner
 * @param panel the panel-level state owner
 * @return whether G-buffer is enabled for this panel
 */
bool _scene_technique_gbuffer_enabled(const DvzScene* scene, const DvzPanel* panel)
{
    return (scene != NULL && _scene_technique_state_gbuffer_enabled(&scene->techniques)) ||
           (panel != NULL && _scene_technique_state_gbuffer_enabled(&panel->techniques));
}



/**
 * Configure internal EDL state.
 *
 * @param state the technique state
 * @param desc EDL descriptor, or NULL to disable
 * @return whether the state was updated
 */
bool _scene_technique_state_set_edl(DvzSceneTechniqueState* state, const DvzEdlDesc* desc)
{
    ANN(state);
    if (!_edl_desc_validate(desc))
        return false;
    if (desc == NULL)
    {
        state->edl.enabled = false;
        return true;
    }

    state->edl.enabled = true;
    state->edl.radius = _clampf(desc->radius, 1.0f, 8.0f);
    state->edl.strength = _clampf(desc->strength, 0.0f, 200.0f);
    state->edl.depth_scale = _clampf(desc->depth_scale, 0.001f, 1000.0f);
    return true;
}



/**
 * Return whether a technique state enables EDL.
 *
 * @param state the technique state
 * @return whether EDL is enabled
 */
bool _scene_technique_state_edl_enabled(const DvzSceneTechniqueState* state)
{
    return state != NULL && state->edl.enabled;
}



/**
 * Return the effective EDL state for one scene/panel pair.
 *
 * @param scene the scene
 * @param panel the panel
 * @return the effective EDL state, or NULL when disabled
 */
const DvzSceneEdlTechniqueState*
_scene_technique_edl_state(const DvzScene* scene, const DvzPanel* panel)
{
    if (panel != NULL && _scene_technique_state_edl_enabled(&panel->techniques))
        return &panel->techniques.edl;
    if (scene != NULL && _scene_technique_state_edl_enabled(&scene->techniques))
        return &scene->techniques.edl;
    return NULL;
}



/**
 * Configure internal ambient-occlusion state.
 *
 * @param state the technique state
 * @param desc AO descriptor, or NULL to disable
 * @return whether the state was updated
 */
bool _scene_technique_state_set_ao(DvzSceneTechniqueState* state, const DvzSceneAoDesc* desc)
{
    ANN(state);
    if (!_ao_desc_validate(desc))
        return false;
    if (desc == NULL)
    {
        state->ao.enabled = false;
        return true;
    }

    state->ao.enabled = true;
    state->ao.radius = _clampf(desc->radius, 0.001f, 64.0f);
    state->ao.intensity = _clampf(desc->intensity, 0.0f, 16.0f);
    state->ao.thickness = _clampf(desc->thickness, 0.001f, 64.0f);
    state->ao.min_visibility = _clampf(desc->min_visibility, 0.0f, 1.0f);
    state->ao.quality = desc->quality;
    state->ao.debug_mode = desc->debug_mode;
    state->ao.denoise_enabled = true;
    return true;
}



/**
 * Return whether a technique state enables ambient occlusion.
 *
 * @param state the technique state
 * @return whether GTAO is enabled
 */
bool _scene_technique_state_ao_enabled(const DvzSceneTechniqueState* state)
{
    return state != NULL && state->ao.enabled;
}



/**
 * Return the effective GTAO state for one scene/panel pair.
 *
 * @param scene the scene
 * @param panel the panel
 * @return the effective GTAO state, or NULL when disabled
 */
const DvzSceneAoTechniqueState*
_scene_technique_ao_state(const DvzScene* scene, const DvzPanel* panel)
{
    if (panel != NULL && _scene_technique_state_ao_enabled(&panel->techniques))
        return &panel->techniques.ao;
    if (scene != NULL && _scene_technique_state_ao_enabled(&scene->techniques))
        return &scene->techniques.ao;
    return NULL;
}


/**
 * Configure internal MSAA state.
 *
 * @param state the technique state
 * @param desc MSAA descriptor, or NULL to disable
 * @return whether the state was updated
 */
bool _scene_technique_state_set_msaa(DvzSceneTechniqueState* state, const DvzMsaaDesc* desc)
{
    ANN(state);
    if (!_msaa_desc_validate(desc))
        return false;
    if (desc == NULL || !desc->enabled)
    {
        state->msaa.enabled = false;
        state->msaa.sample_count = 1;
        state->msaa.alpha_to_coverage = false;
        return true;
    }

    uint32_t sample_count = desc->sample_count == 0 ? 4 : desc->sample_count;
    if (!_scene_msaa_sample_count_valid(sample_count) || sample_count <= 1)
        return false;

    state->msaa.enabled = true;
    state->msaa.sample_count = sample_count;
    state->msaa.alpha_to_coverage = desc->alpha_to_coverage;
    return true;
}


/**
 * Return whether a technique state enables MSAA.
 *
 * @param state the technique state
 * @return whether MSAA is enabled
 */
bool _scene_technique_state_msaa_enabled(const DvzSceneTechniqueState* state)
{
    return state != NULL && state->msaa.enabled && state->msaa.sample_count > 1;
}


/**
 * Return the effective MSAA state for one scene/panel pair.
 *
 * @param scene the scene
 * @param panel the panel
 * @return the effective MSAA state, or NULL when disabled
 */
const DvzSceneMsaaTechniqueState*
_scene_technique_msaa_state(const DvzScene* scene, const DvzPanel* panel)
{
    if (panel != NULL && _scene_technique_state_msaa_enabled(&panel->techniques))
        return &panel->techniques.msaa;
    if (scene != NULL && _scene_technique_state_msaa_enabled(&scene->techniques))
        return &scene->techniques.msaa;
    return NULL;
}



/**
 * Fill the EDL shader uniform from retained technique state.
 *
 * @param edl the effective EDL state
 * @param mvp panel APPLY transform
 * @param viewport panel viewport
 * @param out output shader uniform
 */
void _scene_technique_edl_uniform(
    const DvzSceneEdlTechniqueState* edl, const DvzMVP* mvp,
    const DvzSceneViewportUniform* viewport, DvzSceneEdlUniform* out)
{
    ANN(out);
    dvz_memset(out, sizeof(DvzSceneEdlUniform), 0, sizeof(DvzSceneEdlUniform));
    if (edl == NULL)
        return;
    if (mvp != NULL)
    {
        mat4 proj = {0};
        dvz_memcpy(proj, sizeof(proj), mvp->proj, sizeof(mvp->proj));
        glm_mat4_inv(proj, out->inv_proj);
    }
    if (viewport != NULL)
    {
        out->viewport[0] = viewport->x;
        out->viewport[1] = viewport->y;
        out->viewport[2] = viewport->width > 0.0f ? viewport->width : 1.0f;
        out->viewport[3] = viewport->height > 0.0f ? viewport->height : 1.0f;
    }
    out->params[0] = edl->radius;
    out->params[1] = edl->strength;
    out->params[2] = edl->depth_scale;
    out->params[3] = edl->enabled ? 1.0f : 0.0f;
}



/**
 * Fill the AO shader uniform from retained technique state.
 *
 * @param ao the effective AO state
 * @param out output shader uniform
 */
void _scene_technique_ao_uniform(
    const DvzSceneAoTechniqueState* ao, const DvzMVP* mvp, const DvzSceneViewportUniform* viewport,
    DvzSceneAoUniform* out)
{
    ANN(out);
    dvz_memset(out, sizeof(DvzSceneAoUniform), 0, sizeof(DvzSceneAoUniform));
    if (ao == NULL)
        return;
    if (mvp != NULL)
    {
        mat4 proj = {0};
        dvz_memcpy(proj, sizeof(proj), mvp->proj, sizeof(mvp->proj));
        glm_mat4_copy(proj, out->proj);
        glm_mat4_inv(proj, out->inv_proj);
    }
    if (viewport != NULL)
    {
        out->viewport[0] = viewport->x;
        out->viewport[1] = viewport->y;
        out->viewport[2] = viewport->width > 0.0f ? viewport->width : 1.0f;
        out->viewport[3] = viewport->height > 0.0f ? viewport->height : 1.0f;
        out->extent[0] = out->viewport[2];
        out->extent[1] = out->viewport[3];
        out->extent[2] = 1.0f / out->extent[0];
        out->extent[3] = 1.0f / out->extent[1];
    }
    out->appearance[0] = ao->radius;
    out->appearance[1] = ao->intensity;
    out->appearance[2] = ao->thickness;
    out->appearance[3] = ao->min_visibility;
    out->sampling[0] = ao->radius;
    out->sampling[1] = 0.025f;
    out->sampling[2] = fmaxf(ao->thickness, 0.001f);
    out->sampling[3] = 0.35f;
    if (ao->quality == DVZ_AO_QUALITY_LOW)
    {
        out->mode[0] = 2;
        out->mode[1] = 3;
    }
    else if (ao->quality == DVZ_AO_QUALITY_MEDIUM)
    {
        out->mode[0] = 3;
        out->mode[1] = 4;
    }
    else if (ao->quality == DVZ_AO_QUALITY_HIGH)
    {
        out->mode[0] = 4;
        out->mode[1] = 6;
    }
    else
    {
        out->mode[0] = 6;
        out->mode[1] = 8;
    }
    out->mode[2] = 0;
    out->mode[3] = ao->debug_mode == DVZ_AO_DEBUG_VISIBILITY ? 1 : 0;
}



/**
 * Return whether an alpha mode belongs in the transparent WBOIT accumulation pass.
 *
 * @param mode the visual alpha mode
 * @return whether the visual should be planned through WBOIT
 */
bool _scene_alpha_mode_is_wboit(DvzAlphaMode mode) { return mode == DVZ_ALPHA_WBOIT; }



/**
 * Return whether an alpha mode belongs in the depth-peeling path.
 *
 * @param mode the visual alpha mode
 * @return whether the visual should be planned through depth peeling
 */
bool _scene_alpha_mode_is_depth_peel(DvzAlphaMode mode) { return mode == DVZ_ALPHA_DEPTH_PEEL; }



/**
 * Return whether an alpha mode belongs in an ordinary transparent blend pass.
 *
 * @param mode the visual alpha mode
 * @return whether the visual should be planned after opaque geometry with source-over blending
 */
bool _scene_alpha_mode_is_blended(DvzAlphaMode mode) { return mode == DVZ_ALPHA_BLENDED; }



/**
 * Return the centralized policy for one render-pass role.
 *
 * @param role the FramePlan render-pass role
 * @param out output pass policy
 * @return whether the role has a registered policy
 */
bool _scene_technique_pass_policy(
    DvzFramePlanRenderPassRole role, DvzSceneTechniquePassPolicy* out)
{
    ANN(out);
    for (uint32_t i = 0; i < sizeof(TECHNIQUE_PASS_POLICIES) / sizeof(TECHNIQUE_PASS_POLICIES[0]);
         i++)
    {
        if (TECHNIQUE_PASS_POLICIES[i].role == role)
        {
            *out = TECHNIQUE_PASS_POLICIES[i];
            return true;
        }
    }
    dvz_memset(out, sizeof(DvzSceneTechniquePassPolicy), 0, sizeof(DvzSceneTechniquePassPolicy));
    out->role = role;
    return false;
}



/**
 * Return whether one retained visual writes scene depth.
 *
 * @param visual the retained visual
 * @param attach the panel attachment
 * @return whether the visual writes depth
 */
bool _scene_visual_writes_depth(const DvzVisual* visual, const DvzPanelAttach* attach)
{
    ANN(visual);
    ANN(attach);
    DvzSceneVisualPassCaps caps = {0};
    if (!_scene_visual_pass_caps_from_visual(visual, attach, &caps))
        return false;
    return caps.writes_depth;
}



/**
 * Return whether one retained visual needs to sample scene depth.
 *
 * @param visual the retained visual
 * @param attach the panel attachment
 * @return whether the visual samples depth
 */
bool _scene_visual_samples_depth(const DvzVisual* visual, const DvzPanelAttach* attach)
{
    ANN(visual);
    ANN(attach);
    DvzSceneVisualPassCaps caps = {0};
    if (!_scene_visual_pass_caps_from_visual(visual, attach, &caps))
        return false;
    return caps.samples_depth;
}



/**
 * Return whether one transparent visual needs access to scene depth.
 *
 * @param visual the retained visual
 * @param attach the panel attachment
 * @return whether the transparent visual needs depth
 */
bool _scene_transparent_visual_needs_depth(const DvzVisual* visual, const DvzPanelAttach* attach)
{
    ANN(visual);
    ANN(attach);
    DvzSceneVisualPassCaps caps = {0};
    if (!_scene_visual_pass_caps_from_visual(visual, attach, &caps))
        return false;
    return caps.can_depth_test || _scene_visual_samples_depth(visual, attach);
}
