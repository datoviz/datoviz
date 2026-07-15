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
#define DVZ_SSAO_DESC_KNOWN_FLAGS 0u



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



static bool _ssao_desc_validate(const DvzSsaoDesc* desc)
{
    if (desc == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(desc, DvzSsaoDesc, DVZ_SSAO_DESC_KNOWN_FLAGS))
    {
        log_error("invalid DvzSsaoDesc ABI prologue");
        return false;
    }
    return true;
}



const DvzSceneTechniquePassPolicy TECHNIQUE_PASS_POLICIES[] = {
    {
        .role = DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE,
        .work_label = "opaque",
    },
    {
        .role = DVZ_FRAME_PLAN_RENDER_PASS_GBUFFER,
        .work_label = "gbuffer",
        .graph_required = true,
    },
    {
        .role = DVZ_FRAME_PLAN_RENDER_PASS_VOLUME_OCCLUSION,
        .work_label = "volume_occlusion",
        .graph_required = true,
    },
    {
        .role = DVZ_FRAME_PLAN_RENDER_PASS_SCENE_OCCLUSION,
        .work_label = "scene_occlusion",
        .graph_required = true,
    },
    {
        .role = DVZ_FRAME_PLAN_RENDER_PASS_SSAO,
        .work_label = "ssao",
        .graph_required = true,
    },
    {
        .role = DVZ_FRAME_PLAN_RENDER_PASS_SSAO_BLUR,
        .work_label = "ssao_blur",
        .graph_required = true,
    },
    {
        .role = DVZ_FRAME_PLAN_RENDER_PASS_SSAO_COMPOSITE,
        .work_label = "ssao_composite",
        .graph_required = true,
        .fullscreen_resolve = true,
    },
    {
        .role = DVZ_FRAME_PLAN_RENDER_PASS_EDL_RESOLVE,
        .work_label = "edl_resolve",
        .graph_required = true,
        .fullscreen_resolve = true,
    },
    {
        .role = DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION,
        .work_label = "wboit_accum",
        .graph_required = true,
        .wboit_accumulation = true,
    },
    {
        .role = DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND,
        .work_label = "transparent_blend",
        .graph_required = true,
        .source_over_blend = true,
    },
    {
        .role = DVZ_FRAME_PLAN_RENDER_PASS_WBOIT_RESOLVE,
        .work_label = "wboit_resolve",
        .graph_required = true,
        .fullscreen_resolve = true,
        .needs_wboit_resolve_layout = true,
        .sampled_texture_binding_count = 2,
    },
    {
        .role = DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT,
        .work_label = "depth_peel_init",
        .graph_required = true,
        .depth_peel = true,
    },
    {
        .role = DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER,
        .work_label = "depth_peel_iter",
        .graph_required = true,
        .depth_peel = true,
        .needs_depth_peel_sampled_layout = true,
        .sampled_texture_binding_count = 1,
    },
    {
        .role = DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_COMPOSITE,
        .work_label = "depth_peel_composite",
        .graph_required = true,
        .fullscreen_resolve = true,
        .needs_depth_peel_sampled_layout = true,
        .sampled_texture_binding_count = 2,
    },
    {
        .role = DVZ_FRAME_PLAN_RENDER_PASS_PICKING,
        .work_label = "picking",
    },
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return whether a graph resource id is already declared.
 *
 * @param plan the frame plan
 * @param resource_id the resource id
 * @return whether the graph resource exists
 */
bool _scene_frame_graph_has_resource(const DvzFramePlan* plan, const char* resource_id)
{
    ANN(plan);
    ANN(resource_id);
    for (uint32_t i = 0; i < dvz_frame_plan_graph_resource_count(plan); i++)
    {
        const DvzFrameGraphResource* resource = dvz_frame_plan_graph_resource_get(plan, i);
        if (resource != NULL && strcmp(resource->id, resource_id) == 0)
            return true;
    }
    return false;
}


/**
 * Return whether a graph pass already writes a color attachment.
 *
 * @param plan the frame plan
 * @param resource_id the color attachment resource id
 * @return whether the color attachment is already written
 */
bool _scene_frame_graph_color_written(const DvzFramePlan* plan, const char* resource_id)
{
    ANN(plan);
    ANN(resource_id);
    for (uint32_t i = 0; i < dvz_frame_plan_graph_pass_count(plan); i++)
    {
        const DvzFrameGraphPass* pass = dvz_frame_plan_graph_pass_get(plan, i);
        if (pass == NULL)
            continue;
        for (uint32_t j = 0; j < pass->color_attachment_count; j++)
        {
            if (strcmp(pass->color_attachments[j].resource_id, resource_id) == 0)
                return true;
        }
    }
    return false;
}



/**
 * Add a graph resource unless it already exists.
 *
 * @param plan the frame plan
 * @param resource the graph resource descriptor
 * @return whether the resource exists or was added
 */
bool _scene_frame_graph_resource_once(DvzFramePlan* plan, const DvzFrameGraphResource* resource)
{
    ANN(plan);
    ANN(resource);
    if (_scene_frame_graph_has_resource(plan, resource->id))
        return true;
    return dvz_frame_plan_graph_resource(plan, resource);
}


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
 * Fill a color attachment descriptor.
 *
 * @param attachment the graph attachment descriptor
 * @param resource_id the resource id
 * @param load_op the attachment load op
 * @param clear whether the attachment starts with a clear value
 */
void _scene_frame_graph_color_attachment(
    DvzFrameGraphAttachment* attachment, const char* resource_id,
    DvzFrameGraphAttachmentLoadOp load_op, bool clear)
{
    ANN(attachment);
    ANN(resource_id);
    dvz_memset(attachment, sizeof(DvzFrameGraphAttachment), 0, sizeof(DvzFrameGraphAttachment));
    dvz_strlcpy(attachment->resource_id, resource_id, sizeof(attachment->resource_id));
    attachment->load_op = load_op;
    attachment->store_op = DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE;
    attachment->access = DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE;
    attachment->clear_color[3] = clear ? 0.0f : 1.0f;
}



/**
 * Fill a depth attachment descriptor.
 *
 * @param attachment the graph attachment descriptor
 * @param resource_id the resource id
 * @param load_op the attachment load op
 * @param access the attachment access
 */
void _scene_frame_graph_depth_attachment(
    DvzFrameGraphAttachment* attachment, const char* resource_id,
    DvzFrameGraphAttachmentLoadOp load_op, DvzFrameGraphAttachmentAccess access)
{
    ANN(attachment);
    ANN(resource_id);
    dvz_memset(attachment, sizeof(DvzFrameGraphAttachment), 0, sizeof(DvzFrameGraphAttachment));
    dvz_strlcpy(attachment->resource_id, resource_id, sizeof(attachment->resource_id));
    attachment->load_op = load_op;
    attachment->store_op = DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE;
    attachment->access = access;
    attachment->clear_depth = 1.0f;
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
    state->ssao.radius = 0.5f;
    state->ssao.strength = 1.0f;
    state->ssao.bias = 0.025f;
    state->ssao.sample_count = 16;
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
 * Configure internal SSAO state.
 *
 * @param state the technique state
 * @param desc SSAO descriptor, or NULL to disable
 * @return whether the state was updated
 */
bool _scene_technique_state_set_ssao(DvzSceneTechniqueState* state, const DvzSceneSsaoDesc* desc)
{
    ANN(state);
    if (!_ssao_desc_validate(desc))
        return false;
    if (desc == NULL)
    {
        state->ssao.enabled = false;
        return true;
    }

    state->ssao.enabled = true;
    state->ssao.radius = _clampf(desc->radius, 0.001f, 64.0f);
    state->ssao.strength = _clampf(desc->strength, 0.0f, 16.0f);
    state->ssao.bias = _clampf(desc->bias, 0.0f, 1.0f);
    state->ssao.power = desc->power > 0.0f ? _clampf(desc->power, 0.1f, 8.0f) : 1.0f;
    state->ssao.min_visibility = _clampf(desc->min_visibility, 0.0f, 1.0f);
    state->ssao.blur_radius =
        desc->blur_radius > 0.0f ? _clampf(desc->blur_radius, 1.0f, 16.0f) : 2.0f;
    state->ssao.blur_depth_sigma =
        desc->blur_depth_sigma > 0.0f ? _clampf(desc->blur_depth_sigma, 0.001f, 10.0f) : 0.65f;
    state->ssao.blur_normal_sigma =
        desc->blur_normal_sigma > 0.0f ? _clampf(desc->blur_normal_sigma, 0.001f, 1.0f) : 0.35f;
    state->ssao.sample_count = desc->sample_count == 0 ? 16 : desc->sample_count;
    if (state->ssao.sample_count < 4)
        state->ssao.sample_count = 4;
    if (state->ssao.sample_count > 32)
        state->ssao.sample_count = 32;
    state->ssao.blur_enabled = desc->blur_enabled;
    state->ssao.debug_view = desc->debug_view;
    return true;
}



/**
 * Return whether a technique state enables SSAO.
 *
 * @param state the technique state
 * @return whether SSAO is enabled
 */
bool _scene_technique_state_ssao_enabled(const DvzSceneTechniqueState* state)
{
    return state != NULL && state->ssao.enabled;
}



/**
 * Return the effective SSAO state for one scene/panel pair.
 *
 * @param scene the scene
 * @param panel the panel
 * @return the effective SSAO state, or NULL when disabled
 */
const DvzSceneSsaoTechniqueState*
_scene_technique_ssao_state(const DvzScene* scene, const DvzPanel* panel)
{
    if (panel != NULL && _scene_technique_state_ssao_enabled(&panel->techniques))
        return &panel->techniques.ssao;
    if (scene != NULL && _scene_technique_state_ssao_enabled(&scene->techniques))
        return &scene->techniques.ssao;
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
 * Fill the SSAO shader uniform from retained technique state.
 *
 * @param ssao the effective SSAO state
 * @param out output shader uniform
 */
void _scene_technique_ssao_uniform(
    const DvzSceneSsaoTechniqueState* ssao, const DvzMVP* mvp,
    const DvzSceneViewportUniform* viewport, DvzSceneSsaoUniform* out)
{
    ANN(out);
    dvz_memset(out, sizeof(DvzSceneSsaoUniform), 0, sizeof(DvzSceneSsaoUniform));
    if (ssao == NULL)
        return;
    if (mvp != NULL)
    {
        mat4 proj = {0};
        mat4 view = {0};
        dvz_memcpy(proj, sizeof(proj), mvp->proj, sizeof(mvp->proj));
        dvz_memcpy(view, sizeof(view), mvp->view, sizeof(mvp->view));
        glm_mat4_inv(proj, out->inv_proj);
        glm_mat4_copy(view, out->view);
    }
    if (viewport != NULL)
    {
        out->viewport[0] = viewport->x;
        out->viewport[1] = viewport->y;
        out->viewport[2] = viewport->width > 0.0f ? viewport->width : 1.0f;
        out->viewport[3] = viewport->height > 0.0f ? viewport->height : 1.0f;
    }
    out->params[0] = ssao->radius;
    out->params[1] = ssao->strength;
    out->params[2] = ssao->bias;
    out->params[3] = (float)ssao->sample_count;
    out->params2[0] = ssao->power;
    out->params2[1] = ssao->min_visibility;
    out->params2[2] = ssao->blur_radius;
    out->params2[3] = ssao->debug_view ? 1.0f : 0.0f;
    out->params3[0] = ssao->blur_depth_sigma;
    out->params3[1] = ssao->blur_normal_sigma;
    out->params3[2] = ssao->blur_enabled ? 1.0f : 0.0f;
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
    out->work_label = "";
    return false;
}



/**
 * Return the graph work label used by one render-pass role.
 *
 * @param role the FramePlan render-pass role
 * @return the graph work label, or an empty string when none is expected
 */
const char* _scene_render_role_work_label(DvzFramePlanRenderPassRole role)
{
    DvzSceneTechniquePassPolicy policy = {0};
    if (!_scene_technique_pass_policy(role, &policy))
        return "";
    return policy.work_label;
}



/**
 * Return whether one render-pass role must have a matching graph pass.
 *
 * @param role the FramePlan render-pass role
 * @return whether the role is graph-backed
 */
bool _scene_render_role_requires_graph_pass(DvzFramePlanRenderPassRole role)
{
    DvzSceneTechniquePassPolicy policy = {0};
    if (!_scene_technique_pass_policy(role, &policy))
        return false;
    return policy.graph_required;
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
