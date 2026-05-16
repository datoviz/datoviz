/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene technique planning                                                                     */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <string.h>

#include <vulkan/vulkan_core.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_technique.h"
#include "_visual_pipeline.h"



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
static bool _scene_frame_graph_has_resource(const DvzFramePlan* plan, const char* resource_id)
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
static bool _scene_frame_graph_color_written(const DvzFramePlan* plan, const char* resource_id)
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
static bool
_scene_frame_graph_resource_once(DvzFramePlan* plan, const DvzFrameGraphResource* resource)
{
    ANN(plan);
    ANN(resource);
    if (_scene_frame_graph_has_resource(plan, resource->id))
        return true;
    return dvz_frame_plan_graph_resource(plan, resource);
}



/**
 * Fill a color attachment descriptor.
 *
 * @param attachment the graph attachment descriptor
 * @param resource_id the resource id
 * @param load_op the attachment load op
 * @param clear whether the attachment starts with a clear value
 */
static void _scene_frame_graph_color_attachment(
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
static void _scene_frame_graph_depth_attachment(
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
static bool _scene_caps_support_gbuffer(const DvzSceneVisualPassCaps* caps)
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
static float _clampf(float value, float min_value, float max_value)
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
bool _scene_technique_state_set_ssao(
    DvzSceneTechniqueState* state, const DvzSceneSsaoDesc* desc)
{
    ANN(state);
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
        desc->blur_radius > 0.0f ? _clampf(desc->blur_radius, 1.0f, 8.0f) : 2.0f;
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
 * Fill the EDL shader uniform from retained technique state.
 *
 * @param edl the effective EDL state
 * @param out output shader uniform
 */
void _scene_technique_edl_uniform(
    const DvzSceneEdlTechniqueState* edl, DvzSceneEdlUniform* out)
{
    ANN(out);
    dvz_memset(out, sizeof(DvzSceneEdlUniform), 0, sizeof(DvzSceneEdlUniform));
    if (edl == NULL)
        return;
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
bool _scene_alpha_mode_is_wboit(DvzAlphaMode mode)
{
    return mode == DVZ_ALPHA_WBOIT;
}



/**
 * Return whether an alpha mode belongs in the depth-peeling path.
 *
 * @param mode the visual alpha mode
 * @return whether the visual should be planned through depth peeling
 */
bool _scene_alpha_mode_is_depth_peel(DvzAlphaMode mode)
{
    return mode == DVZ_ALPHA_DEPTH_PEEL;
}



/**
 * Return whether an alpha mode belongs in an ordinary transparent blend pass.
 *
 * @param mode the visual alpha mode
 * @return whether the visual should be planned after opaque geometry with source-over blending
 */
bool _scene_alpha_mode_is_blended(DvzAlphaMode mode)
{
    return mode == DVZ_ALPHA_BLENDED;
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
static bool _scene_visual_samples_depth(const DvzVisual* visual, const DvzPanelAttach* attach)
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
bool _scene_transparent_visual_needs_depth(
    const DvzVisual* visual, const DvzPanelAttach* attach)
{
    ANN(visual);
    ANN(attach);
    DvzSceneVisualPassCaps caps = {0};
    if (!_scene_visual_pass_caps_from_visual(visual, attach, &caps))
        return false;
    return caps.can_depth_test || _scene_visual_samples_depth(visual, attach);
}



/**
 * Emit graph descriptors for one WBOIT panel plan.
 *
 * @param plan the frame plan
 * @param panel_id the panel id
 * @param opaque_needs_depth whether the opaque pass writes depth
 * @param transparent_needs_depth whether the accumulation pass reads depth
 * @return whether graph descriptors were emitted
 */
bool _scene_technique_emit_wboit_frame_graph(
    DvzFramePlan* plan, const char* panel_id, bool opaque_needs_depth,
    bool transparent_needs_depth)
{
    ANN(plan);
    ANN(panel_id);

    char accum_id[DVZ_SCENE_LABEL_SIZE];
    char weight_id[DVZ_SCENE_LABEL_SIZE];
    char depth_id[DVZ_SCENE_LABEL_SIZE];
    char opaque_pass_id[DVZ_SCENE_LABEL_SIZE];
    char accum_pass_id[DVZ_SCENE_LABEL_SIZE];
    char resolve_pass_id[DVZ_SCENE_LABEL_SIZE];
    dvz_snprintf(accum_id, sizeof(accum_id), "%s.wboit.accum", panel_id);
    dvz_snprintf(weight_id, sizeof(weight_id), "%s.wboit.weight", panel_id);
    dvz_snprintf(depth_id, sizeof(depth_id), "%s.depth", panel_id);
    dvz_snprintf(opaque_pass_id, sizeof(opaque_pass_id), "%s.opaque", panel_id);
    dvz_snprintf(accum_pass_id, sizeof(accum_pass_id), "%s.wboit.accum", panel_id);
    dvz_snprintf(resolve_pass_id, sizeof(resolve_pass_id), "%s.wboit.resolve", panel_id);

    DvzFrameGraphResource rt = {0};
    dvz_strlcpy(rt.id, "rt", sizeof(rt.id));
    rt.kind = DVZ_FRAME_GRAPH_RESOURCE_EXTERNAL_TARGET;
    rt.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
    rt.usage_flags =
        DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT | DVZ_FRAME_GRAPH_RESOURCE_USAGE_COPY_SRC;
    rt.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_BORROWED;
    if (!_scene_frame_graph_resource_once(plan, &rt))
        return false;

    DvzFrameGraphResource accum = {0};
    dvz_strlcpy(accum.id, accum_id, sizeof(accum.id));
    accum.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
    accum.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    accum.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
    accum.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT |
                        DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED;
    accum.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
    if (!_scene_frame_graph_resource_once(plan, &accum))
        return false;

    DvzFrameGraphResource weight = {0};
    dvz_strlcpy(weight.id, weight_id, sizeof(weight.id));
    weight.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
    weight.format = VK_FORMAT_R16_SFLOAT;
    weight.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
    weight.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT |
                         DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED;
    weight.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
    if (!_scene_frame_graph_resource_once(plan, &weight))
        return false;

    bool shared_depth = opaque_needs_depth && transparent_needs_depth;
    if (shared_depth)
    {
        DvzFrameGraphResource depth = {0};
        dvz_strlcpy(depth.id, depth_id, sizeof(depth.id));
        depth.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
        depth.format = VK_FORMAT_D32_SFLOAT;
        depth.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
        depth.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_DEPTH_ATTACHMENT |
                            DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED;
        depth.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
        if (!_scene_frame_graph_resource_once(plan, &depth))
            return false;
    }

    DvzFrameGraphAttachment color = {0};
    DvzFrameGraphAttachment depth = {0};
    DvzFrameGraphPass opaque = {0};
    dvz_strlcpy(opaque.id, opaque_pass_id, sizeof(opaque.id));
    dvz_strlcpy(opaque.panel_id, panel_id, sizeof(opaque.panel_id));
    dvz_strlcpy(opaque.work_label, "opaque", sizeof(opaque.work_label));
    opaque.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    bool color_written = _scene_frame_graph_color_written(plan, "rt");
    _scene_frame_graph_color_attachment(
        &color, "rt",
        color_written ? DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD :
                        DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR,
        !color_written);
    if (!dvz_frame_graph_pass_color_attachment(&opaque, &color))
        return false;
    if (shared_depth)
    {
        _scene_frame_graph_depth_attachment(
            &depth, depth_id, DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR,
            DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE);
        if (!dvz_frame_graph_pass_depth_attachment(&opaque, &depth))
            return false;
    }
    if (!dvz_frame_plan_graph_pass(plan, &opaque))
        return false;

    DvzFrameGraphPass accum_pass = {0};
    dvz_strlcpy(accum_pass.id, accum_pass_id, sizeof(accum_pass.id));
    dvz_strlcpy(accum_pass.panel_id, panel_id, sizeof(accum_pass.panel_id));
    dvz_strlcpy(accum_pass.work_label, "wboit_accum", sizeof(accum_pass.work_label));
    accum_pass.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    _scene_frame_graph_color_attachment(
        &color, accum_id, DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR, true);
    if (!dvz_frame_graph_pass_color_attachment(&accum_pass, &color))
        return false;
    _scene_frame_graph_color_attachment(
        &color, weight_id, DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR, true);
    if (!dvz_frame_graph_pass_color_attachment(&accum_pass, &color))
        return false;
    if (shared_depth)
    {
        _scene_frame_graph_depth_attachment(
            &depth, depth_id, DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD,
            DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ);
        if (!dvz_frame_graph_pass_depth_attachment(&accum_pass, &depth))
            return false;
    }
    if (!dvz_frame_plan_graph_pass(plan, &accum_pass))
        return false;

    DvzFrameGraphPass resolve = {0};
    dvz_strlcpy(resolve.id, resolve_pass_id, sizeof(resolve.id));
    dvz_strlcpy(resolve.panel_id, panel_id, sizeof(resolve.panel_id));
    dvz_strlcpy(resolve.work_label, "wboit_resolve", sizeof(resolve.work_label));
    resolve.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    if (!dvz_frame_graph_pass_read(&resolve, accum_id, DVZ_FRAME_GRAPH_ACCESS_SAMPLED) ||
        !dvz_frame_graph_pass_read(&resolve, weight_id, DVZ_FRAME_GRAPH_ACCESS_SAMPLED))
        return false;
    _scene_frame_graph_color_attachment(
        &color, "rt", DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD, false);
    if (!dvz_frame_graph_pass_color_attachment(&resolve, &color))
        return false;
    return dvz_frame_plan_graph_pass(plan, &resolve);
}



/**
 * Emit graph descriptors for one ordinary blended transparent panel plan.
 *
 * @param plan the frame plan
 * @param panel_id the panel id
 * @param opaque_needs_depth whether the opaque pass writes depth
 * @param transparent_needs_depth whether the transparent pass reads depth
 * @return whether graph descriptors were emitted
 */
bool _scene_technique_emit_blended_frame_graph(
    DvzFramePlan* plan, const char* panel_id, bool opaque_needs_depth,
    bool transparent_needs_depth)
{
    ANN(plan);
    ANN(panel_id);

    char depth_id[DVZ_SCENE_LABEL_SIZE];
    char opaque_pass_id[DVZ_SCENE_LABEL_SIZE];
    char blend_pass_id[DVZ_SCENE_LABEL_SIZE];
    dvz_snprintf(depth_id, sizeof(depth_id), "%s.depth", panel_id);
    dvz_snprintf(opaque_pass_id, sizeof(opaque_pass_id), "%s.opaque", panel_id);
    dvz_snprintf(blend_pass_id, sizeof(blend_pass_id), "%s.transparent_blend", panel_id);

    DvzFrameGraphResource rt = {0};
    dvz_strlcpy(rt.id, "rt", sizeof(rt.id));
    rt.kind = DVZ_FRAME_GRAPH_RESOURCE_EXTERNAL_TARGET;
    rt.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
    rt.usage_flags =
        DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT | DVZ_FRAME_GRAPH_RESOURCE_USAGE_COPY_SRC;
    rt.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_BORROWED;
    if (!_scene_frame_graph_resource_once(plan, &rt))
        return false;

    bool shared_depth = opaque_needs_depth && transparent_needs_depth;
    if (shared_depth)
    {
        DvzFrameGraphResource depth = {0};
        dvz_strlcpy(depth.id, depth_id, sizeof(depth.id));
        depth.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
        depth.format = VK_FORMAT_D32_SFLOAT;
        depth.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
        depth.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_DEPTH_ATTACHMENT |
                            DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED;
        depth.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
        if (!_scene_frame_graph_resource_once(plan, &depth))
            return false;
    }

    DvzFrameGraphAttachment color = {0};
    DvzFrameGraphAttachment depth = {0};
    DvzFrameGraphPass opaque = {0};
    dvz_strlcpy(opaque.id, opaque_pass_id, sizeof(opaque.id));
    dvz_strlcpy(opaque.panel_id, panel_id, sizeof(opaque.panel_id));
    dvz_strlcpy(opaque.work_label, "opaque", sizeof(opaque.work_label));
    opaque.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    bool color_written = _scene_frame_graph_color_written(plan, "rt");
    _scene_frame_graph_color_attachment(
        &color, "rt",
        color_written ? DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD :
                        DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR,
        !color_written);
    if (!dvz_frame_graph_pass_color_attachment(&opaque, &color))
        return false;
    if (shared_depth)
    {
        _scene_frame_graph_depth_attachment(
            &depth, depth_id, DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR,
            DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE);
        if (!dvz_frame_graph_pass_depth_attachment(&opaque, &depth))
            return false;
    }
    if (!dvz_frame_plan_graph_pass(plan, &opaque))
        return false;

    DvzFrameGraphPass blend = {0};
    dvz_strlcpy(blend.id, blend_pass_id, sizeof(blend.id));
    dvz_strlcpy(blend.panel_id, panel_id, sizeof(blend.panel_id));
    dvz_strlcpy(blend.work_label, "transparent_blend", sizeof(blend.work_label));
    blend.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    _scene_frame_graph_color_attachment(
        &color, "rt", DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD, false);
    if (!dvz_frame_graph_pass_color_attachment(&blend, &color))
        return false;
    if (shared_depth)
    {
        _scene_frame_graph_depth_attachment(
            &depth, depth_id, DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD,
            DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ);
        if (!dvz_frame_graph_pass_depth_attachment(&blend, &depth))
            return false;
    }
    return dvz_frame_plan_graph_pass(plan, &blend);
}



/**
 * Emit graph descriptors for one depth-peeling panel plan.
 *
 * @param plan the frame plan
 * @param panel_id the panel id
 * @param opaque_needs_depth whether the opaque pass writes depth
 * @param transparent_needs_depth whether peeling passes read depth
 * @return whether graph descriptors were emitted
 */
bool _scene_technique_emit_depth_peel_frame_graph(
    DvzFramePlan* plan, const char* panel_id, bool opaque_needs_depth,
    bool transparent_needs_depth)
{
    ANN(plan);
    ANN(panel_id);

    char opaque_depth_id[DVZ_SCENE_LABEL_SIZE];
    char front_ping_id[DVZ_SCENE_LABEL_SIZE];
    char back_ping_id[DVZ_SCENE_LABEL_SIZE];
    char depth_ping_id[DVZ_SCENE_LABEL_SIZE];
    char front_pong_id[DVZ_SCENE_LABEL_SIZE];
    char back_pong_id[DVZ_SCENE_LABEL_SIZE];
    char depth_pong_id[DVZ_SCENE_LABEL_SIZE];
    char opaque_pass_id[DVZ_SCENE_LABEL_SIZE];
    char init_pass_id[DVZ_SCENE_LABEL_SIZE];
    char iter_pass_id[DVZ_SCENE_LABEL_SIZE];
    char composite_pass_id[DVZ_SCENE_LABEL_SIZE];
    dvz_snprintf(opaque_depth_id, sizeof(opaque_depth_id), "%s.depth.opaque", panel_id);
    dvz_snprintf(front_ping_id, sizeof(front_ping_id), "%s.peel.front_ping", panel_id);
    dvz_snprintf(back_ping_id, sizeof(back_ping_id), "%s.peel.back_ping", panel_id);
    dvz_snprintf(depth_ping_id, sizeof(depth_ping_id), "%s.peel.depth_ping", panel_id);
    dvz_snprintf(front_pong_id, sizeof(front_pong_id), "%s.peel.front_pong", panel_id);
    dvz_snprintf(back_pong_id, sizeof(back_pong_id), "%s.peel.back_pong", panel_id);
    dvz_snprintf(depth_pong_id, sizeof(depth_pong_id), "%s.peel.depth_pong", panel_id);
    dvz_snprintf(opaque_pass_id, sizeof(opaque_pass_id), "%s.opaque", panel_id);
    dvz_snprintf(init_pass_id, sizeof(init_pass_id), "%s.peel.init", panel_id);
    dvz_snprintf(iter_pass_id, sizeof(iter_pass_id), "%s.peel.iter.0", panel_id);
    dvz_snprintf(composite_pass_id, sizeof(composite_pass_id), "%s.peel.composite", panel_id);

    DvzFrameGraphResource rt = {0};
    dvz_strlcpy(rt.id, "rt", sizeof(rt.id));
    rt.kind = DVZ_FRAME_GRAPH_RESOURCE_EXTERNAL_TARGET;
    rt.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
    rt.usage_flags =
        DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT | DVZ_FRAME_GRAPH_RESOURCE_USAGE_COPY_SRC;
    rt.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_BORROWED;
    if (!_scene_frame_graph_resource_once(plan, &rt))
        return false;

    bool shared_depth = opaque_needs_depth && transparent_needs_depth;
    if (shared_depth)
    {
        DvzFrameGraphResource depth = {0};
        dvz_strlcpy(depth.id, opaque_depth_id, sizeof(depth.id));
        depth.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
        depth.format = VK_FORMAT_D32_SFLOAT;
        depth.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
        depth.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_DEPTH_ATTACHMENT;
        depth.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
        if (!_scene_frame_graph_resource_once(plan, &depth))
            return false;
    }

    const char* peel_ids[6] = {
        front_ping_id, back_ping_id, depth_ping_id, front_pong_id, back_pong_id, depth_pong_id};
    for (uint32_t i = 0; i < 6; i++)
    {
        DvzFrameGraphResource resource = {0};
        dvz_strlcpy(resource.id, peel_ids[i], sizeof(resource.id));
        resource.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
        resource.format = VK_FORMAT_R16G16B16A16_SFLOAT;
        resource.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
        resource.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT |
                               DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED;
        resource.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
        if (!_scene_frame_graph_resource_once(plan, &resource))
            return false;
    }

    DvzFrameGraphAttachment color = {0};
    DvzFrameGraphAttachment depth = {0};
    DvzFrameGraphPass opaque = {0};
    dvz_strlcpy(opaque.id, opaque_pass_id, sizeof(opaque.id));
    dvz_strlcpy(opaque.panel_id, panel_id, sizeof(opaque.panel_id));
    dvz_strlcpy(opaque.work_label, "opaque", sizeof(opaque.work_label));
    opaque.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    _scene_frame_graph_color_attachment(
        &color, "rt", DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR, true);
    if (!dvz_frame_graph_pass_color_attachment(&opaque, &color))
        return false;
    if (shared_depth)
    {
        _scene_frame_graph_depth_attachment(
            &depth, opaque_depth_id, DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR,
            DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE);
        if (!dvz_frame_graph_pass_depth_attachment(&opaque, &depth))
            return false;
    }
    if (!dvz_frame_plan_graph_pass(plan, &opaque))
        return false;

    DvzFrameGraphPass init = {0};
    dvz_strlcpy(init.id, init_pass_id, sizeof(init.id));
    dvz_strlcpy(init.panel_id, panel_id, sizeof(init.panel_id));
    dvz_strlcpy(init.work_label, "depth_peel_init", sizeof(init.work_label));
    init.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    for (uint32_t i = 0; i < 3; i++)
    {
        _scene_frame_graph_color_attachment(
            &color, peel_ids[i], DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR, true);
        if (!dvz_frame_graph_pass_color_attachment(&init, &color))
            return false;
    }
    if (shared_depth)
    {
        _scene_frame_graph_depth_attachment(
            &depth, opaque_depth_id, DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD,
            DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ);
        depth.store_op = DVZ_FRAME_GRAPH_ATTACHMENT_STORE_DONT_CARE;
        if (!dvz_frame_graph_pass_depth_attachment(&init, &depth))
            return false;
    }
    if (!dvz_frame_plan_graph_pass(plan, &init))
        return false;

    DvzFrameGraphPass iter = {0};
    dvz_strlcpy(iter.id, iter_pass_id, sizeof(iter.id));
    dvz_strlcpy(iter.panel_id, panel_id, sizeof(iter.panel_id));
    dvz_strlcpy(iter.work_label, "depth_peel_iter", sizeof(iter.work_label));
    iter.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    for (uint32_t i = 3; i < 6; i++)
    {
        _scene_frame_graph_color_attachment(
            &color, peel_ids[i], DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR, true);
        if (!dvz_frame_graph_pass_color_attachment(&iter, &color))
            return false;
    }
    if (shared_depth)
    {
        _scene_frame_graph_depth_attachment(
            &depth, opaque_depth_id, DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD,
            DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ);
        depth.store_op = DVZ_FRAME_GRAPH_ATTACHMENT_STORE_DONT_CARE;
        if (!dvz_frame_graph_pass_depth_attachment(&iter, &depth))
            return false;
    }
    if (!dvz_frame_plan_graph_pass(plan, &iter))
        return false;

    DvzFrameGraphPass composite = {0};
    dvz_strlcpy(composite.id, composite_pass_id, sizeof(composite.id));
    dvz_strlcpy(composite.panel_id, panel_id, sizeof(composite.panel_id));
    dvz_strlcpy(composite.work_label, "depth_peel_composite", sizeof(composite.work_label));
    composite.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    if (!dvz_frame_graph_pass_read(&composite, front_ping_id, DVZ_FRAME_GRAPH_ACCESS_SAMPLED) ||
        !dvz_frame_graph_pass_read(&composite, back_pong_id, DVZ_FRAME_GRAPH_ACCESS_SAMPLED) ||
        !dvz_frame_graph_pass_read(&composite, depth_pong_id, DVZ_FRAME_GRAPH_ACCESS_SAMPLED))
        return false;
    _scene_frame_graph_color_attachment(
        &color, "rt", DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD, false);
    if (!dvz_frame_graph_pass_color_attachment(&composite, &color))
        return false;
    return dvz_frame_plan_graph_pass(plan, &composite);
}



/**
 * Emit graph descriptors for one ordinary opaque panel render pass.
 *
 * @param plan the frame plan
 * @param panel_id the panel id
 * @param needs_depth whether the opaque pass writes depth
 * @return whether graph descriptors were emitted
 */
bool _scene_technique_emit_opaque_frame_graph(
    DvzFramePlan* plan, const char* panel_id, bool needs_depth)
{
    ANN(plan);
    ANN(panel_id);

    char depth_id[DVZ_SCENE_LABEL_SIZE];
    char opaque_pass_id[DVZ_SCENE_LABEL_SIZE];
    dvz_snprintf(depth_id, sizeof(depth_id), "%s.depth", panel_id);
    dvz_snprintf(opaque_pass_id, sizeof(opaque_pass_id), "%s.opaque", panel_id);

    DvzFrameGraphResource rt = {0};
    dvz_strlcpy(rt.id, "rt", sizeof(rt.id));
    rt.kind = DVZ_FRAME_GRAPH_RESOURCE_EXTERNAL_TARGET;
    rt.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
    rt.usage_flags =
        DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT | DVZ_FRAME_GRAPH_RESOURCE_USAGE_COPY_SRC;
    rt.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_BORROWED;
    if (!_scene_frame_graph_resource_once(plan, &rt))
        return false;

    if (needs_depth)
    {
        DvzFrameGraphResource depth = {0};
        dvz_strlcpy(depth.id, depth_id, sizeof(depth.id));
        depth.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
        depth.format = VK_FORMAT_D32_SFLOAT;
        depth.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
        depth.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_DEPTH_ATTACHMENT;
        depth.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
        if (!_scene_frame_graph_resource_once(plan, &depth))
            return false;
    }

    DvzFrameGraphAttachment color = {0};
    DvzFrameGraphAttachment depth = {0};
    DvzFrameGraphPass opaque = {0};
    dvz_strlcpy(opaque.id, opaque_pass_id, sizeof(opaque.id));
    dvz_strlcpy(opaque.panel_id, panel_id, sizeof(opaque.panel_id));
    dvz_strlcpy(opaque.work_label, "opaque", sizeof(opaque.work_label));
    opaque.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    _scene_frame_graph_color_attachment(
        &color, "rt", DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR, true);
    if (!dvz_frame_graph_pass_color_attachment(&opaque, &color))
        return false;
    if (needs_depth)
    {
        _scene_frame_graph_depth_attachment(
            &depth, depth_id, DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR,
            DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE);
        if (!dvz_frame_graph_pass_depth_attachment(&opaque, &depth))
            return false;
    }
    return dvz_frame_plan_graph_pass(plan, &opaque);
}



/**
 * Emit graph descriptors for one depth-based post-processing panel plan.
 *
 * @param plan the frame plan
 * @param panel_id the panel id
 * @param desc the depth post-process graph descriptor
 * @return whether graph descriptors were emitted
 */
bool _scene_technique_emit_depth_postprocess_frame_graph(
    DvzFramePlan* plan, const char* panel_id,
    const DvzSceneDepthPostProcessGraphDesc* desc)
{
    ANN(plan);
    ANN(panel_id);
    ANN(desc);
    ANN(desc->color_suffix);
    ANN(desc->depth_suffix);
    ANN(desc->resolve_suffix);
    ANN(desc->resolve_work_label);

    char color_id[DVZ_SCENE_LABEL_SIZE];
    char depth_id[DVZ_SCENE_LABEL_SIZE];
    char opaque_pass_id[DVZ_SCENE_LABEL_SIZE];
    char resolve_pass_id[DVZ_SCENE_LABEL_SIZE];
    dvz_snprintf(color_id, sizeof(color_id), "%s.%s", panel_id, desc->color_suffix);
    dvz_snprintf(depth_id, sizeof(depth_id), "%s.%s", panel_id, desc->depth_suffix);
    dvz_snprintf(opaque_pass_id, sizeof(opaque_pass_id), "%s.opaque", panel_id);
    dvz_snprintf(resolve_pass_id, sizeof(resolve_pass_id), "%s.%s", panel_id,
                 desc->resolve_suffix);

    DvzFrameGraphResource rt = {0};
    dvz_strlcpy(rt.id, "rt", sizeof(rt.id));
    rt.kind = DVZ_FRAME_GRAPH_RESOURCE_EXTERNAL_TARGET;
    rt.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
    rt.usage_flags =
        DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT | DVZ_FRAME_GRAPH_RESOURCE_USAGE_COPY_SRC;
    rt.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_BORROWED;
    if (!_scene_frame_graph_resource_once(plan, &rt))
        return false;

    DvzFrameGraphResource color = {0};
    dvz_strlcpy(color.id, color_id, sizeof(color.id));
    color.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
    color.format = desc->color_format;
    color.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
    color.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT |
                        DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED;
    color.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
    if (!_scene_frame_graph_resource_once(plan, &color))
        return false;

    DvzFrameGraphResource depth = {0};
    dvz_strlcpy(depth.id, depth_id, sizeof(depth.id));
    depth.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
    depth.format = VK_FORMAT_D32_SFLOAT;
    depth.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
    depth.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_DEPTH_ATTACHMENT |
                        DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED;
    depth.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
    if (!_scene_frame_graph_resource_once(plan, &depth))
        return false;

    DvzFrameGraphAttachment attachment = {0};
    DvzFrameGraphAttachment depth_attachment = {0};
    DvzFrameGraphPass opaque = {0};
    dvz_strlcpy(opaque.id, opaque_pass_id, sizeof(opaque.id));
    dvz_strlcpy(opaque.panel_id, panel_id, sizeof(opaque.panel_id));
    dvz_strlcpy(opaque.work_label, "opaque", sizeof(opaque.work_label));
    opaque.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    _scene_frame_graph_color_attachment(
        &attachment, color_id, DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR, true);
    if (!dvz_frame_graph_pass_color_attachment(&opaque, &attachment))
        return false;
    _scene_frame_graph_depth_attachment(
        &depth_attachment, depth_id, DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR,
        DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE);
    if (!dvz_frame_graph_pass_depth_attachment(&opaque, &depth_attachment))
        return false;
    if (!dvz_frame_plan_graph_pass(plan, &opaque))
        return false;

    DvzFrameGraphPass resolve = {0};
    dvz_strlcpy(resolve.id, resolve_pass_id, sizeof(resolve.id));
    dvz_strlcpy(resolve.panel_id, panel_id, sizeof(resolve.panel_id));
    dvz_strlcpy(resolve.work_label, desc->resolve_work_label, sizeof(resolve.work_label));
    resolve.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    if (!dvz_frame_graph_pass_read(&resolve, color_id, DVZ_FRAME_GRAPH_ACCESS_SAMPLED) ||
        !dvz_frame_graph_pass_read(&resolve, depth_id, DVZ_FRAME_GRAPH_ACCESS_SAMPLED))
        return false;
    _scene_frame_graph_color_attachment(
        &attachment, "rt", desc->resolve_load_op, desc->resolve_clear);
    if (!dvz_frame_graph_pass_color_attachment(&resolve, &attachment))
        return false;
    return dvz_frame_plan_graph_pass(plan, &resolve);
}



/**
 * Emit graph descriptors for one EDL post-processing panel plan.
 *
 * @param plan the frame plan
 * @param panel_id the panel id
 * @return whether graph descriptors were emitted
 */
bool _scene_technique_emit_edl_frame_graph(DvzFramePlan* plan, const char* panel_id)
{
    const DvzSceneDepthPostProcessGraphDesc desc = {
        .color_suffix = "edl.color",
        .depth_suffix = "edl.depth",
        .resolve_suffix = "edl.resolve",
        .resolve_work_label = "edl_resolve",
        .color_format = VK_FORMAT_R8G8B8A8_UNORM,
        .resolve_load_op = DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR,
        .resolve_clear = true,
    };
    return _scene_technique_emit_depth_postprocess_frame_graph(plan, panel_id, &desc);
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
    if (!_scene_visual_pass_caps_from_visual(visual, attach, &caps))
        return false;
    if (!_scene_caps_support_gbuffer(&caps))
        return false;

    plan->enabled = true;
    plan->needs_depth = true;
    plan->needs_normal = true;
    plan->producer_count++;
    return true;
}



/**
 * Emit graph descriptors for one panel G-buffer plan.
 *
 * @param plan the frame plan
 * @param panel_id the panel id
 * @param gbuffer the G-buffer plan
 * @return whether graph descriptors were emitted
 */
bool _scene_technique_emit_gbuffer_frame_graph(
    DvzFramePlan* plan, const char* panel_id, const DvzSceneGBufferPlan* gbuffer)
{
    ANN(plan);
    ANN(panel_id);
    ANN(gbuffer);

    if (!gbuffer->enabled || gbuffer->producer_count == 0)
        return true;
    if (!gbuffer->needs_depth && !gbuffer->needs_normal && !gbuffer->needs_object_id)
        return false;

    char depth_id[DVZ_SCENE_LABEL_SIZE];
    char normal_id[DVZ_SCENE_LABEL_SIZE];
    char object_id[DVZ_SCENE_LABEL_SIZE];
    char pass_id[DVZ_SCENE_LABEL_SIZE];
    dvz_snprintf(depth_id, sizeof(depth_id), "%s.gbuffer.depth", panel_id);
    dvz_snprintf(normal_id, sizeof(normal_id), "%s.gbuffer.normal", panel_id);
    dvz_snprintf(object_id, sizeof(object_id), "%s.gbuffer.object_id", panel_id);
    dvz_snprintf(pass_id, sizeof(pass_id), "%s.gbuffer", panel_id);

    if (gbuffer->needs_depth)
    {
        DvzFrameGraphResource depth = {0};
        dvz_strlcpy(depth.id, depth_id, sizeof(depth.id));
        depth.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
        depth.format = VK_FORMAT_D32_SFLOAT;
        depth.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
        depth.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_DEPTH_ATTACHMENT |
                            DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED;
        depth.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
        if (!_scene_frame_graph_resource_once(plan, &depth))
            return false;
    }

    if (gbuffer->needs_normal)
    {
        DvzFrameGraphResource normal = {0};
        dvz_strlcpy(normal.id, normal_id, sizeof(normal.id));
        normal.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
        normal.format = VK_FORMAT_R16G16B16A16_SFLOAT;
        normal.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
        normal.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT |
                             DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED;
        normal.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
        if (!_scene_frame_graph_resource_once(plan, &normal))
            return false;
    }

    if (gbuffer->needs_object_id)
    {
        DvzFrameGraphResource object = {0};
        dvz_strlcpy(object.id, object_id, sizeof(object.id));
        object.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
        object.format = VK_FORMAT_R32_UINT;
        object.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
        object.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT |
                             DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED;
        object.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
        if (!_scene_frame_graph_resource_once(plan, &object))
            return false;
    }

    DvzFrameGraphPass pass = {0};
    DvzFrameGraphAttachment attachment = {0};
    dvz_strlcpy(pass.id, pass_id, sizeof(pass.id));
    dvz_strlcpy(pass.panel_id, panel_id, sizeof(pass.panel_id));
    dvz_strlcpy(pass.work_label, "gbuffer", sizeof(pass.work_label));
    pass.kind = DVZ_FRAME_GRAPH_PASS_RENDER;

    if (gbuffer->needs_normal)
    {
        _scene_frame_graph_color_attachment(
            &attachment, normal_id, DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR, true);
        attachment.clear_color[0] = 0.5f;
        attachment.clear_color[1] = 0.5f;
        attachment.clear_color[2] = 1.0f;
        attachment.clear_color[3] = 0.0f;
        if (!dvz_frame_graph_pass_color_attachment(&pass, &attachment))
            return false;
    }

    if (gbuffer->needs_object_id)
    {
        _scene_frame_graph_color_attachment(
            &attachment, object_id, DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR, true);
        attachment.clear_color[0] = 0.0f;
        attachment.clear_color[1] = 0.0f;
        attachment.clear_color[2] = 0.0f;
        attachment.clear_color[3] = 0.0f;
        if (!dvz_frame_graph_pass_color_attachment(&pass, &attachment))
            return false;
    }

    if (gbuffer->needs_depth)
    {
        _scene_frame_graph_depth_attachment(
            &attachment, depth_id, DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR,
            DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE);
        if (!dvz_frame_graph_pass_depth_attachment(&pass, &attachment))
            return false;
    }

    return dvz_frame_plan_graph_pass(plan, &pass);
}



/**
 * Emit graph descriptors for one SSAO graph-only panel plan.
 *
 * @param plan the frame plan
 * @param panel_id the panel id
 * @param gbuffer the G-buffer plan providing normal and depth inputs
 * @param ssao_state the effective SSAO state
 * @return whether graph descriptors were emitted
 */
bool _scene_technique_emit_ssao_frame_graph(
    DvzFramePlan* plan, const char* panel_id, const DvzSceneGBufferPlan* gbuffer,
    const DvzSceneSsaoTechniqueState* ssao_state)
{
    ANN(plan);
    ANN(panel_id);
    ANN(gbuffer);

    if (!gbuffer->enabled || gbuffer->producer_count == 0)
        return true;
    if (!gbuffer->needs_depth || !gbuffer->needs_normal)
        return false;

    char depth_id[DVZ_SCENE_LABEL_SIZE];
    char normal_id[DVZ_SCENE_LABEL_SIZE];
    char occlusion_id[DVZ_SCENE_LABEL_SIZE];
    char blur_id[DVZ_SCENE_LABEL_SIZE];
    char pass_id[DVZ_SCENE_LABEL_SIZE];
    char blur_pass_id[DVZ_SCENE_LABEL_SIZE];
    char composite_id[DVZ_SCENE_LABEL_SIZE];
    dvz_snprintf(depth_id, sizeof(depth_id), "%s.gbuffer.depth", panel_id);
    dvz_snprintf(normal_id, sizeof(normal_id), "%s.gbuffer.normal", panel_id);
    dvz_snprintf(occlusion_id, sizeof(occlusion_id), "%s.ssao.occlusion", panel_id);
    dvz_snprintf(blur_id, sizeof(blur_id), "%s.ssao.blur", panel_id);
    dvz_snprintf(pass_id, sizeof(pass_id), "%s.ssao", panel_id);
    dvz_snprintf(blur_pass_id, sizeof(blur_pass_id), "%s.ssao.blur", panel_id);
    dvz_snprintf(composite_id, sizeof(composite_id), "%s.ssao.composite", panel_id);
    bool blur_enabled = ssao_state != NULL && ssao_state->blur_enabled;

    DvzFrameGraphResource occlusion = {0};
    dvz_strlcpy(occlusion.id, occlusion_id, sizeof(occlusion.id));
    occlusion.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
    occlusion.format = VK_FORMAT_R8_UNORM;
    occlusion.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
    occlusion.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT |
                             DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED;
    occlusion.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
    if (!_scene_frame_graph_resource_once(plan, &occlusion))
        return false;

    DvzFrameGraphAttachment attachment = {0};
    DvzFrameGraphPass ssao = {0};
    dvz_strlcpy(ssao.id, pass_id, sizeof(ssao.id));
    dvz_strlcpy(ssao.panel_id, panel_id, sizeof(ssao.panel_id));
    dvz_strlcpy(ssao.work_label, "ssao", sizeof(ssao.work_label));
    ssao.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    if (!dvz_frame_graph_pass_read(&ssao, normal_id, DVZ_FRAME_GRAPH_ACCESS_SAMPLED) ||
        !dvz_frame_graph_pass_read(&ssao, depth_id, DVZ_FRAME_GRAPH_ACCESS_SAMPLED))
        return false;
    _scene_frame_graph_color_attachment(
        &attachment, occlusion_id, DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR, true);
    attachment.clear_color[0] = 1.0f;
    attachment.clear_color[1] = 1.0f;
    attachment.clear_color[2] = 1.0f;
    attachment.clear_color[3] = 1.0f;
    if (!dvz_frame_graph_pass_color_attachment(&ssao, &attachment))
        return false;
    if (!dvz_frame_plan_graph_pass(plan, &ssao))
        return false;

    if (blur_enabled)
    {
        DvzFrameGraphResource blur = {0};
        dvz_strlcpy(blur.id, blur_id, sizeof(blur.id));
        blur.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
        blur.format = VK_FORMAT_R8_UNORM;
        blur.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
        blur.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT |
                           DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED;
        blur.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
        if (!_scene_frame_graph_resource_once(plan, &blur))
            return false;

        DvzFrameGraphPass blur_pass = {0};
        dvz_strlcpy(blur_pass.id, blur_pass_id, sizeof(blur_pass.id));
        dvz_strlcpy(blur_pass.panel_id, panel_id, sizeof(blur_pass.panel_id));
        dvz_strlcpy(blur_pass.work_label, "ssao_blur", sizeof(blur_pass.work_label));
        blur_pass.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
        if (!dvz_frame_graph_pass_read(&blur_pass, occlusion_id, DVZ_FRAME_GRAPH_ACCESS_SAMPLED) ||
            !dvz_frame_graph_pass_read(&blur_pass, normal_id, DVZ_FRAME_GRAPH_ACCESS_SAMPLED) ||
            !dvz_frame_graph_pass_read(&blur_pass, depth_id, DVZ_FRAME_GRAPH_ACCESS_SAMPLED))
            return false;
        _scene_frame_graph_color_attachment(
            &attachment, blur_id, DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR, true);
        attachment.clear_color[0] = 1.0f;
        attachment.clear_color[1] = 1.0f;
        attachment.clear_color[2] = 1.0f;
        attachment.clear_color[3] = 1.0f;
        if (!dvz_frame_graph_pass_color_attachment(&blur_pass, &attachment))
            return false;
        if (!dvz_frame_plan_graph_pass(plan, &blur_pass))
            return false;
    }

    DvzFrameGraphPass composite = {0};
    dvz_strlcpy(composite.id, composite_id, sizeof(composite.id));
    dvz_strlcpy(composite.panel_id, panel_id, sizeof(composite.panel_id));
    dvz_strlcpy(composite.work_label, "ssao_composite", sizeof(composite.work_label));
    composite.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    if (!dvz_frame_graph_pass_read(
            &composite, blur_enabled ? blur_id : occlusion_id, DVZ_FRAME_GRAPH_ACCESS_SAMPLED))
        return false;
    _scene_frame_graph_color_attachment(
        &attachment, "rt", DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD, false);
    if (!dvz_frame_graph_pass_color_attachment(&composite, &attachment))
        return false;
    return dvz_frame_plan_graph_pass(plan, &composite);
}
