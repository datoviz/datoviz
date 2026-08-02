/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Immutable panel technique composition                                                        */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <stdarg.h>
#include <string.h>

#include "_assertions.h"
#include "_compat.h"
#include "_technique.h"
#include "frame_plan/internal.h"
#include "scene_emit/panel_render_plan.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define COMPOSITION_CAP_SAMPLED_RENDER_TARGET    0x01u
#define COMPOSITION_CAP_RGBA16FLOAT              0x02u
#define COMPOSITION_CAP_R16FLOAT                 0x04u
#define COMPOSITION_CAP_COLOR_BLEND              0x08u
#define COMPOSITION_CAP_DUAL_COLOR_ATTACHMENTS   0x10u
#define COMPOSITION_CAP_TRIPLE_COLOR_ATTACHMENTS 0x20u

#define COMPOSITION_ADAPT_MSAA_REDUCED             0x01u
#define COMPOSITION_ADAPT_LEGACY_AMBIENT_COMPOSITE 0x02u
#define COMPOSITION_ADAPT_CAPABILITY_FALLBACK      0x04u
#define COMPOSITION_EXPAND_SSAO_BLUR               0x01u

#define PRODUCT_BIT(_kind) (UINT64_C(1) << (uint32_t)(_kind))
#define LAYER_BIT(_layer)  (1u << (uint32_t)(_layer))



/*************************************************************************************************/
/*  Immutable technique contracts                                                               */
/*************************************************************************************************/

typedef struct DvzSceneTechniqueContract
{
    DvzSceneTechniqueId id;
    uint32_t version;
    DvzSceneTechniquePhase phase;
    uint32_t must_follow_phase_mask;
    uint64_t required_inputs;
    uint64_t optional_inputs;
    uint64_t outputs;
    uint64_t optional_outputs;
    uint32_t participating_layer_mask;
    uint32_t required_capability_mask;
    uint32_t optional_capability_mask;
    DvzSceneTechniqueFallback fallback;
    uint32_t pass_template_count;
    struct
    {
        DvzFramePlanRenderPassRole role;
        uint32_t repeat;
    } pass_templates[4];
} DvzSceneTechniqueContract;

#define COMPOSITION_PASS_ONCE                  1u
#define COMPOSITION_PASS_OPTIONAL_SSAO_BLUR    UINT32_MAX
#define COMPOSITION_PASS_DEPTH_PEEL_ITERATIONS (UINT32_MAX - 1u)



static const DvzSceneTechniqueContract TECHNIQUE_CONTRACTS[] = {
    {
        .id = DVZ_SCENE_TECHNIQUE_SCENE_OCCLUSION,
        .version = 1,
        .phase = DVZ_SCENE_PHASE_SURFACE_CAPTURE,
        .outputs = PRODUCT_BIT(DVZ_RENDER_PRODUCT_SCENE_OCCLUSION_DEPTH),
        .participating_layer_mask = LAYER_BIT(DVZ_SCENE_VISUAL_LAYER_SURFACE_OPAQUE) |
                                    LAYER_BIT(DVZ_SCENE_VISUAL_LAYER_SURFACE_MASKED),
        .pass_template_count = 1,
        .pass_templates = {{DVZ_FRAME_PLAN_RENDER_PASS_SCENE_OCCLUSION, COMPOSITION_PASS_ONCE}},
    },
    {
        .id = DVZ_SCENE_TECHNIQUE_VOLUME_OCCLUSION,
        .version = 1,
        .phase = DVZ_SCENE_PHASE_SURFACE_CAPTURE,
        .outputs = PRODUCT_BIT(DVZ_RENDER_PRODUCT_VOLUME_FIRST_HIT_DEPTH),
        .participating_layer_mask = LAYER_BIT(DVZ_SCENE_VISUAL_LAYER_VOLUME),
        .pass_template_count = 1,
        .pass_templates = {{DVZ_FRAME_PLAN_RENDER_PASS_VOLUME_OCCLUSION, COMPOSITION_PASS_ONCE}},
    },
    {
        .id = DVZ_SCENE_TECHNIQUE_SURFACE_CAPTURE,
        .version = 1,
        .phase = DVZ_SCENE_PHASE_SURFACE_CAPTURE,
        .outputs = PRODUCT_BIT(DVZ_RENDER_PRODUCT_SURFACE_DEPTH) |
                   PRODUCT_BIT(DVZ_RENDER_PRODUCT_SURFACE_NORMAL) |
                   PRODUCT_BIT(DVZ_RENDER_PRODUCT_SURFACE_COVERAGE),
        .participating_layer_mask = LAYER_BIT(DVZ_SCENE_VISUAL_LAYER_SURFACE_OPAQUE) |
                                    LAYER_BIT(DVZ_SCENE_VISUAL_LAYER_SURFACE_MASKED),
        .required_capability_mask = COMPOSITION_CAP_RGBA16FLOAT,
        .fallback = DVZ_SCENE_TECHNIQUE_FALLBACK_LEGACY_TRANSITION,
        .pass_template_count = 1,
        .pass_templates = {{DVZ_FRAME_PLAN_RENDER_PASS_GBUFFER, COMPOSITION_PASS_ONCE}},
    },
    {
        .id = DVZ_SCENE_TECHNIQUE_AMBIENT_VISIBILITY,
        .version = 1,
        .phase = DVZ_SCENE_PHASE_SURFACE_ANALYSIS,
        .must_follow_phase_mask = 1u << (uint32_t)DVZ_SCENE_PHASE_SURFACE_CAPTURE,
        .required_inputs = PRODUCT_BIT(DVZ_RENDER_PRODUCT_SURFACE_DEPTH) |
                           PRODUCT_BIT(DVZ_RENDER_PRODUCT_SURFACE_NORMAL) |
                           PRODUCT_BIT(DVZ_RENDER_PRODUCT_SURFACE_COVERAGE),
        .outputs = PRODUCT_BIT(DVZ_RENDER_PRODUCT_AMBIENT_VISIBILITY),
        .participating_layer_mask = LAYER_BIT(DVZ_SCENE_VISUAL_LAYER_SURFACE_OPAQUE) |
                                    LAYER_BIT(DVZ_SCENE_VISUAL_LAYER_SURFACE_MASKED),
        .required_capability_mask = COMPOSITION_CAP_SAMPLED_RENDER_TARGET |
                                    COMPOSITION_CAP_RGBA16FLOAT | COMPOSITION_CAP_R16FLOAT,
        .fallback = DVZ_SCENE_TECHNIQUE_FALLBACK_LEGACY_TRANSITION,
        .pass_template_count = 2,
        .pass_templates =
            {
                {DVZ_FRAME_PLAN_RENDER_PASS_SSAO, COMPOSITION_PASS_ONCE},
                {DVZ_FRAME_PLAN_RENDER_PASS_SSAO_BLUR, COMPOSITION_PASS_OPTIONAL_SSAO_BLUR},
            },
    },
    {
        .id = DVZ_SCENE_TECHNIQUE_OPAQUE_SHADING,
        .version = 1,
        .phase = DVZ_SCENE_PHASE_OPAQUE_SHADING,
        .must_follow_phase_mask = (1u << (uint32_t)DVZ_SCENE_PHASE_SURFACE_CAPTURE) |
                                  (1u << (uint32_t)DVZ_SCENE_PHASE_SURFACE_ANALYSIS),
        .optional_inputs = PRODUCT_BIT(DVZ_RENDER_PRODUCT_AMBIENT_VISIBILITY) |
                           PRODUCT_BIT(DVZ_RENDER_PRODUCT_SCENE_OCCLUSION_DEPTH) |
                           PRODUCT_BIT(DVZ_RENDER_PRODUCT_VOLUME_FIRST_HIT_DEPTH),
        .outputs = PRODUCT_BIT(DVZ_RENDER_PRODUCT_SCENE_COLOR),
        .optional_outputs = PRODUCT_BIT(DVZ_RENDER_PRODUCT_SURFACE_DEPTH),
        .participating_layer_mask = LAYER_BIT(DVZ_SCENE_VISUAL_LAYER_SURFACE_OPAQUE) |
                                    LAYER_BIT(DVZ_SCENE_VISUAL_LAYER_SURFACE_MASKED),
        .pass_template_count = 1,
        .pass_templates = {{DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE, COMPOSITION_PASS_ONCE}},
    },
    {
        .id = DVZ_SCENE_TECHNIQUE_AMBIENT_COMPOSITE,
        .version = 1,
        .phase = DVZ_SCENE_PHASE_SURFACE_POSTPROCESS,
        .must_follow_phase_mask = 1u << (uint32_t)DVZ_SCENE_PHASE_OPAQUE_SHADING,
        .required_inputs = PRODUCT_BIT(DVZ_RENDER_PRODUCT_SCENE_COLOR) |
                           PRODUCT_BIT(DVZ_RENDER_PRODUCT_AMBIENT_VISIBILITY),
        .outputs = PRODUCT_BIT(DVZ_RENDER_PRODUCT_SCENE_COLOR),
        .participating_layer_mask = LAYER_BIT(DVZ_SCENE_VISUAL_LAYER_SURFACE_OPAQUE) |
                                    LAYER_BIT(DVZ_SCENE_VISUAL_LAYER_SURFACE_MASKED),
        .required_capability_mask =
            COMPOSITION_CAP_SAMPLED_RENDER_TARGET | COMPOSITION_CAP_COLOR_BLEND,
        .fallback = DVZ_SCENE_TECHNIQUE_FALLBACK_LEGACY_TRANSITION,
        .pass_template_count = 1,
        .pass_templates = {{DVZ_FRAME_PLAN_RENDER_PASS_SSAO_COMPOSITE, COMPOSITION_PASS_ONCE}},
    },
    {
        .id = DVZ_SCENE_TECHNIQUE_EDL,
        .version = 1,
        .phase = DVZ_SCENE_PHASE_SURFACE_POSTPROCESS,
        .must_follow_phase_mask = 1u << (uint32_t)DVZ_SCENE_PHASE_OPAQUE_SHADING,
        .required_inputs = PRODUCT_BIT(DVZ_RENDER_PRODUCT_SCENE_COLOR) |
                           PRODUCT_BIT(DVZ_RENDER_PRODUCT_SURFACE_DEPTH),
        .outputs = PRODUCT_BIT(DVZ_RENDER_PRODUCT_SCENE_COLOR),
        .participating_layer_mask = LAYER_BIT(DVZ_SCENE_VISUAL_LAYER_SURFACE_OPAQUE) |
                                    LAYER_BIT(DVZ_SCENE_VISUAL_LAYER_SURFACE_MASKED),
        .required_capability_mask = COMPOSITION_CAP_SAMPLED_RENDER_TARGET,
        .fallback = DVZ_SCENE_TECHNIQUE_FALLBACK_LEGACY_TRANSITION,
        .pass_template_count = 1,
        .pass_templates = {{DVZ_FRAME_PLAN_RENDER_PASS_EDL_RESOLVE, COMPOSITION_PASS_ONCE}},
    },
    {
        .id = DVZ_SCENE_TECHNIQUE_TRANSPARENT_BLEND,
        .version = 1,
        .phase = DVZ_SCENE_PHASE_TRANSPARENT_SHADING,
        .must_follow_phase_mask = 1u << (uint32_t)DVZ_SCENE_PHASE_SURFACE_POSTPROCESS,
        .required_inputs = PRODUCT_BIT(DVZ_RENDER_PRODUCT_SCENE_COLOR),
        .optional_inputs = PRODUCT_BIT(DVZ_RENDER_PRODUCT_SCENE_OCCLUSION_DEPTH) |
                           PRODUCT_BIT(DVZ_RENDER_PRODUCT_VOLUME_FIRST_HIT_DEPTH),
        .outputs = PRODUCT_BIT(DVZ_RENDER_PRODUCT_SCENE_COLOR),
        .participating_layer_mask = LAYER_BIT(DVZ_SCENE_VISUAL_LAYER_TRANSPARENT) |
                                    LAYER_BIT(DVZ_SCENE_VISUAL_LAYER_VOLUME),
        .required_capability_mask = COMPOSITION_CAP_COLOR_BLEND,
        .pass_template_count = 1,
        .pass_templates = {{DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND, COMPOSITION_PASS_ONCE}},
    },
    {
        .id = DVZ_SCENE_TECHNIQUE_WBOIT,
        .version = 1,
        .phase = DVZ_SCENE_PHASE_TRANSPARENT_SHADING,
        .must_follow_phase_mask = 1u << (uint32_t)DVZ_SCENE_PHASE_SURFACE_POSTPROCESS,
        .required_inputs = PRODUCT_BIT(DVZ_RENDER_PRODUCT_SCENE_COLOR),
        .optional_inputs = PRODUCT_BIT(DVZ_RENDER_PRODUCT_SCENE_OCCLUSION_DEPTH),
        .outputs = PRODUCT_BIT(DVZ_RENDER_PRODUCT_SCENE_COLOR) |
                   PRODUCT_BIT(DVZ_RENDER_PRODUCT_TRANSPARENT_ACCUMULATION) |
                   PRODUCT_BIT(DVZ_RENDER_PRODUCT_TRANSPARENT_TRANSMITTANCE),
        .participating_layer_mask = LAYER_BIT(DVZ_SCENE_VISUAL_LAYER_TRANSPARENT),
        .required_capability_mask = COMPOSITION_CAP_SAMPLED_RENDER_TARGET |
                                    COMPOSITION_CAP_RGBA16FLOAT | COMPOSITION_CAP_R16FLOAT |
                                    COMPOSITION_CAP_COLOR_BLEND |
                                    COMPOSITION_CAP_DUAL_COLOR_ATTACHMENTS,
        .pass_template_count = 2,
        .pass_templates =
            {
                {DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION, COMPOSITION_PASS_ONCE},
                {DVZ_FRAME_PLAN_RENDER_PASS_WBOIT_RESOLVE, COMPOSITION_PASS_ONCE},
            },
    },
    {
        .id = DVZ_SCENE_TECHNIQUE_DEPTH_PEEL,
        .version = 1,
        .phase = DVZ_SCENE_PHASE_TRANSPARENT_SHADING,
        .must_follow_phase_mask = 1u << (uint32_t)DVZ_SCENE_PHASE_SURFACE_POSTPROCESS,
        .required_inputs = PRODUCT_BIT(DVZ_RENDER_PRODUCT_SCENE_COLOR),
        .optional_inputs = PRODUCT_BIT(DVZ_RENDER_PRODUCT_SCENE_OCCLUSION_DEPTH),
        .outputs = PRODUCT_BIT(DVZ_RENDER_PRODUCT_SCENE_COLOR) |
                   PRODUCT_BIT(DVZ_RENDER_PRODUCT_TRANSPARENT_ACCUMULATION) |
                   PRODUCT_BIT(DVZ_RENDER_PRODUCT_TRANSPARENT_PEEL_DEPTH),
        .participating_layer_mask = LAYER_BIT(DVZ_SCENE_VISUAL_LAYER_TRANSPARENT),
        .required_capability_mask = COMPOSITION_CAP_SAMPLED_RENDER_TARGET |
                                    COMPOSITION_CAP_RGBA16FLOAT | COMPOSITION_CAP_COLOR_BLEND |
                                    COMPOSITION_CAP_TRIPLE_COLOR_ATTACHMENTS,
        .pass_template_count = 3,
        .pass_templates =
            {
                {DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT, COMPOSITION_PASS_ONCE},
                {DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER,
                 COMPOSITION_PASS_DEPTH_PEEL_ITERATIONS},
                {DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_COMPOSITE, COMPOSITION_PASS_ONCE},
            },
    },
    {
        .id = DVZ_SCENE_TECHNIQUE_VOLUME_SHADING,
        .version = 1,
        .phase = DVZ_SCENE_PHASE_VOLUME_SHADING,
        .must_follow_phase_mask = 1u << (uint32_t)DVZ_SCENE_PHASE_TRANSPARENT_SHADING,
        .required_inputs = PRODUCT_BIT(DVZ_RENDER_PRODUCT_SCENE_COLOR),
        .optional_inputs = PRODUCT_BIT(DVZ_RENDER_PRODUCT_SCENE_OCCLUSION_DEPTH) |
                           PRODUCT_BIT(DVZ_RENDER_PRODUCT_VOLUME_FIRST_HIT_DEPTH),
        .outputs = PRODUCT_BIT(DVZ_RENDER_PRODUCT_SCENE_COLOR),
        .participating_layer_mask = LAYER_BIT(DVZ_SCENE_VISUAL_LAYER_VOLUME),
        .required_capability_mask = COMPOSITION_CAP_COLOR_BLEND,
        .pass_template_count = 1,
        .pass_templates = {{DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND, COMPOSITION_PASS_ONCE}},
    },
    {
        .id = DVZ_SCENE_TECHNIQUE_OVERLAY_COMPOSITE,
        .version = 1,
        .phase = DVZ_SCENE_PHASE_OVERLAY,
        .must_follow_phase_mask = (1u << (uint32_t)DVZ_SCENE_PHASE_TRANSPARENT_SHADING) |
                                  (1u << (uint32_t)DVZ_SCENE_PHASE_VOLUME_SHADING) |
                                  (1u << (uint32_t)DVZ_SCENE_PHASE_SCENE_POSTPROCESS),
        .required_inputs = PRODUCT_BIT(DVZ_RENDER_PRODUCT_SCENE_COLOR),
        .outputs = PRODUCT_BIT(DVZ_RENDER_PRODUCT_SCENE_COLOR),
        .participating_layer_mask = LAYER_BIT(DVZ_SCENE_VISUAL_LAYER_OVERLAY),
        .optional_capability_mask = COMPOSITION_CAP_COLOR_BLEND,
        .pass_template_count = 1,
        .pass_templates = {{DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND, COMPOSITION_PASS_ONCE}},
    },
    {
        .id = DVZ_SCENE_TECHNIQUE_PRESENTATION,
        .version = 1,
        .phase = DVZ_SCENE_PHASE_PRESENTATION,
        .must_follow_phase_mask = (1u << (uint32_t)DVZ_SCENE_PHASE_SURFACE_POSTPROCESS) |
                                  (1u << (uint32_t)DVZ_SCENE_PHASE_TRANSPARENT_SHADING) |
                                  (1u << (uint32_t)DVZ_SCENE_PHASE_VOLUME_SHADING) |
                                  (1u << (uint32_t)DVZ_SCENE_PHASE_OVERLAY),
        .required_inputs = PRODUCT_BIT(DVZ_RENDER_PRODUCT_SCENE_COLOR),
        .outputs = PRODUCT_BIT(DVZ_RENDER_PRODUCT_PRESENTATION_COLOR),
    },
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static uint64_t _composition_product_bit(DvzRenderProductKind kind)
{
    return kind > DVZ_RENDER_PRODUCT_NONE && kind < 64 ? UINT64_C(1) << (uint32_t)kind : 0;
}



static bool _composition_mark_unrealized(DvzSceneResolvedPass* pass, DvzRenderProductId id)
{
    ANN(pass);
    if (id.value == 0)
        return false;
    for (uint32_t i = 0; i < pass->unrealized_product_count; i++)
        if (pass->unrealized_product_ids[i].value == id.value)
            return true;
    if (pass->unrealized_product_count >= DVZ_PANEL_COMPOSITION_MAX_UNREALIZED_PRODUCTS)
        return false;
    pass->unrealized_product_ids[pass->unrealized_product_count++] = id;
    return true;
}



static uint32_t _composition_available_capabilities(const DvzCapabilitySnapshot* caps)
{
    if (caps == NULL || caps->struct_size < sizeof(DvzCapabilitySnapshot))
        return UINT32_MAX;
    uint32_t available = 0;
    if (caps->supports_render_target_sampling)
        available |= COMPOSITION_CAP_SAMPLED_RENDER_TARGET;
    if (caps->render_target_format_rgba16float)
        available |= COMPOSITION_CAP_RGBA16FLOAT;
    if (caps->render_target_format_r16float)
        available |= COMPOSITION_CAP_R16FLOAT;
    if (caps->supports_color_blending)
        available |= COMPOSITION_CAP_COLOR_BLEND;
    if (caps->max_color_attachments >= 2)
        available |= COMPOSITION_CAP_DUAL_COLOR_ATTACHMENTS;
    if (caps->max_color_attachments >= 3)
        available |= COMPOSITION_CAP_TRIPLE_COLOR_ATTACHMENTS;
    return available;
}



static bool _composition_report(DvzDiagnosticReport* report, const char* fmt, ...)
{
    ANN(fmt);
    if (report != NULL)
    {
        char message[DVZ_SCENE_DIAGNOSTIC_SIZE] = {0};
        va_list args;
        va_start(args, fmt);
        int written = dvz_vsnprintf(message, sizeof(message), fmt, args);
        va_end(args);
        if (written >= 0)
            (void)dvz_diagnostic_report_add(report, message);
    }
    return false;
}



static uint32_t _composition_effective_sample_count(
    const DvzPanelRenderPlan* plan, const DvzCapabilitySnapshot* caps, bool* reduced)
{
    ANN(plan);
    ANN(reduced);
    *reduced = false;
    if (plan->msaa_state == NULL || !plan->msaa_state->enabled ||
        plan->msaa_state->sample_count <= 1)
        return 1;

    uint32_t requested = plan->msaa_state->sample_count;
    if (caps == NULL)
        return requested;
    uint32_t color_max = caps->max_color_sample_count > 0 ? caps->max_color_sample_count : 1;
    uint32_t depth_max = caps->max_depth_sample_count > 0 ? caps->max_depth_sample_count : 1;
    uint32_t maximum = color_max < depth_max ? color_max : depth_max;
    uint32_t effective = 1;
    const uint32_t supported[] = {16, 8, 4, 2};
    for (uint32_t i = 0; i < sizeof(supported) / sizeof(supported[0]); i++)
    {
        if (requested >= supported[i] && maximum >= supported[i])
        {
            effective = supported[i];
            break;
        }
    }
    *reduced = effective != requested;
    return effective;
}



static const DvzSceneTechniqueContract* _composition_contract(DvzSceneTechniqueId id)
{
    for (uint32_t i = 0; i < sizeof(TECHNIQUE_CONTRACTS) / sizeof(TECHNIQUE_CONTRACTS[0]); i++)
    {
        if (TECHNIQUE_CONTRACTS[i].id == id)
            return &TECHNIQUE_CONTRACTS[i];
    }
    return NULL;
}



static const char* _composition_technique_name(DvzSceneTechniqueId id)
{
    switch (id)
    {
    case DVZ_SCENE_TECHNIQUE_SURFACE_CAPTURE:
        return "surface capture";
    case DVZ_SCENE_TECHNIQUE_AMBIENT_VISIBILITY:
        return "ambient visibility";
    case DVZ_SCENE_TECHNIQUE_AMBIENT_COMPOSITE:
        return "ambient composite";
    case DVZ_SCENE_TECHNIQUE_EDL:
        return "EDL";
    case DVZ_SCENE_TECHNIQUE_TRANSPARENT_BLEND:
        return "alpha blending";
    case DVZ_SCENE_TECHNIQUE_WBOIT:
        return "WBOIT";
    case DVZ_SCENE_TECHNIQUE_DEPTH_PEEL:
        return "depth peeling";
    case DVZ_SCENE_TECHNIQUE_VOLUME_SHADING:
        return "volume shading";
    case DVZ_SCENE_TECHNIQUE_OVERLAY_COMPOSITE:
        return "overlay composition";
    default:
        return "render technique";
    }
}



static bool _composition_validate_capabilities(
    DvzPanelCompositionSnapshot* snapshot, const char* panel_id, DvzDiagnosticReport* report)
{
    ANN(snapshot);
    ANN(panel_id);
    for (uint32_t i = 0; i < snapshot->technique_count; i++)
    {
        DvzSceneResolvedTechnique* technique = &snapshot->techniques[i];
        uint32_t missing =
            technique->required_capability_mask & ~snapshot->available_capability_mask;
        technique->missing_capability_mask = missing;
        if (missing == 0)
            continue;
        if (technique->fallback == DVZ_SCENE_TECHNIQUE_FALLBACK_LEGACY_TRANSITION)
        {
            technique->capability_adaptations |= COMPOSITION_ADAPT_CAPABILITY_FALLBACK;
            continue;
        }
        const char* requirement = "an unavailable rendering capability";
        if ((missing & COMPOSITION_CAP_TRIPLE_COLOR_ATTACHMENTS) != 0)
            requirement = "at least three color attachments";
        else if ((missing & COMPOSITION_CAP_DUAL_COLOR_ATTACHMENTS) != 0)
            requirement = "at least two color attachments";
        else if ((missing & COMPOSITION_CAP_RGBA16FLOAT) != 0)
            requirement = "rgba16float render-target support";
        else if ((missing & COMPOSITION_CAP_R16FLOAT) != 0)
            requirement = "r16float render-target support";
        else if ((missing & COMPOSITION_CAP_SAMPLED_RENDER_TARGET) != 0)
            requirement = "sampling intermediate render targets";
        else if ((missing & COMPOSITION_CAP_COLOR_BLEND) != 0)
            requirement = "color blending support";
        return _composition_report(
            report, "panel %s %s requires %s", panel_id,
            _composition_technique_name(technique->id), requirement);
    }
    return true;
}



static bool _composition_add_technique(
    DvzPanelCompositionSnapshot* snapshot, DvzSceneTechniqueId id, uint64_t inputs,
    uint64_t outputs, uint32_t adaptations, DvzSceneTechniqueFallback fallback)
{
    ANN(snapshot);
    const DvzSceneTechniqueContract* contract = _composition_contract(id);
    if (contract == NULL || (inputs & contract->required_inputs) != contract->required_inputs ||
        (inputs & ~(contract->required_inputs | contract->optional_inputs)) != 0 ||
        (outputs & contract->outputs) != contract->outputs ||
        (outputs & ~(contract->outputs | contract->optional_outputs)) != 0)
        return false;
    if (snapshot->technique_count >= DVZ_PANEL_COMPOSITION_MAX_TECHNIQUES)
        return false;
    DvzSceneResolvedTechnique* technique = &snapshot->techniques[snapshot->technique_count++];
    *technique = (DvzSceneResolvedTechnique){
        .instance_id = {.value = snapshot->technique_count},
        .id = id,
        .version = contract->version,
        .phase = contract->phase,
        .must_follow_phase_mask = contract->must_follow_phase_mask,
        .input_product_mask = inputs,
        .output_product_mask = outputs,
        .participating_layer_mask = contract->participating_layer_mask,
        .required_capability_mask = contract->required_capability_mask,
        .capability_adaptations = adaptations,
        .fallback = fallback != DVZ_SCENE_TECHNIQUE_FALLBACK_NONE ? fallback : contract->fallback,
    };
    for (uint32_t kind = DVZ_RENDER_PRODUCT_SCENE_COLOR;
         kind <= DVZ_RENDER_PRODUCT_PRESENTATION_COLOR; kind++)
    {
        uint64_t bit = _composition_product_bit((DvzRenderProductKind)kind);
        if ((inputs & bit) != 0 &&
            technique->input_count < DVZ_PANEL_COMPOSITION_MAX_PRODUCTS_PER_TECHNIQUE)
            technique->inputs[technique->input_count++] = (DvzRenderProductKind)kind;
        if ((outputs & bit) != 0 &&
            technique->output_count < DVZ_PANEL_COMPOSITION_MAX_PRODUCTS_PER_TECHNIQUE)
            technique->outputs[technique->output_count++] = (DvzRenderProductKind)kind;
    }
    snapshot->required_product_mask |= inputs | outputs;
    return true;
}



static bool _composition_authored_bounds(
    const DvzPanelRenderVisualPlan* visuals, uint32_t count, uint32_t blend_group,
    uint32_t* out_begin, uint32_t* out_end)
{
    ANN(out_begin);
    ANN(out_end);
    *out_begin = UINT32_MAX;
    *out_end = UINT32_MAX;
    for (uint32_t i = 0; i < count; i++)
    {
        if (blend_group != DVZ_PANEL_RENDER_INVALID_INDEX && visuals[i].blend_group != blend_group)
            continue;
        if (*out_begin == UINT32_MAX || visuals[i].authored_order < *out_begin)
            *out_begin = visuals[i].authored_order;
        if (*out_end == UINT32_MAX || visuals[i].authored_order > *out_end)
            *out_end = visuals[i].authored_order;
    }
    return *out_begin != UINT32_MAX;
}



static uint32_t
_composition_blend_group_layers(const DvzPanelRenderPlan* plan, uint32_t blend_group)
{
    ANN(plan);
    uint32_t layers = 0;
    for (uint32_t i = 0; i < plan->blended_visual_count; i++)
    {
        const DvzPanelRenderVisualPlan* visual = &plan->blended_visuals[i];
        if (visual->blend_group == blend_group && visual->layer > DVZ_SCENE_VISUAL_LAYER_NONE &&
            visual->layer < 32)
            layers |= 1u << (uint32_t)visual->layer;
    }
    return layers;
}



static bool
_composition_blend_group_uses_source_over(const DvzPanelRenderPlan* plan, uint32_t blend_group)
{
    ANN(plan);
    for (uint32_t i = 0; i < plan->blended_visual_count; i++)
    {
        const DvzPanelRenderVisualPlan* visual = &plan->blended_visuals[i];
        if (visual->blend_group == blend_group && visual->caps.uses_source_over_blend)
            return true;
    }
    return false;
}



static uint32_t _composition_visual_phase_bit(DvzSceneTechniquePhase phase)
{
    switch (phase)
    {
    case DVZ_SCENE_PHASE_SURFACE_CAPTURE:
        return DVZ_SCENE_VISUAL_PHASE_SURFACE_CAPTURE;
    case DVZ_SCENE_PHASE_OPAQUE_SHADING:
        return DVZ_SCENE_VISUAL_PHASE_OPAQUE_SHADING;
    case DVZ_SCENE_PHASE_TRANSPARENT_SHADING:
        return DVZ_SCENE_VISUAL_PHASE_TRANSPARENT_SHADING;
    case DVZ_SCENE_PHASE_VOLUME_SHADING:
        return DVZ_SCENE_VISUAL_PHASE_VOLUME_SHADING;
    case DVZ_SCENE_PHASE_OVERLAY:
        return DVZ_SCENE_VISUAL_PHASE_OVERLAY;
    case DVZ_SCENE_PHASE_QUERY:
        return DVZ_SCENE_VISUAL_PHASE_QUERY;
    default:
        return DVZ_SCENE_VISUAL_PHASE_NONE;
    }
}



static bool _composition_validate_technique_visuals(
    const DvzPanelRenderPlan* plan, const DvzPanelRenderVisualPlan* visuals, uint32_t count,
    uint32_t blend_group, const DvzSceneResolvedTechnique* technique, DvzDiagnosticReport* report)
{
    ANN(plan);
    ANN(visuals);
    ANN(technique);
    uint32_t phase_bit = _composition_visual_phase_bit(technique->phase);
    for (uint32_t i = 0; i < count; i++)
    {
        const DvzPanelRenderVisualPlan* visual = &visuals[i];
        if (blend_group != DVZ_PANEL_RENDER_INVALID_INDEX && visual->blend_group != blend_group)
            continue;
        uint32_t layer_bit = LAYER_BIT(visual->layer);
        if ((technique->participating_layer_mask & layer_bit) == 0 || phase_bit == 0 ||
            (visual->caps.phase_participation & phase_bit) == 0)
            return _composition_report(
                report,
                "panel %s technique %u is incompatible with visual %" PRIu32
                " layer/phase participation",
                plan->panel_id, (uint32_t)technique->id, visual->visual_index);
    }
    return true;
}

static bool _composition_add_pass(
    DvzPanelCompositionSnapshot* snapshot, DvzSceneTechniqueInstanceId technique_instance_id,
    DvzSceneTechniqueId technique_id, DvzSceneTechniquePhase phase,
    DvzFramePlanRenderPassRole role, uint32_t authored_begin, uint32_t authored_end)
{
    ANN(snapshot);
    if (snapshot->pass_count >= DVZ_PANEL_COMPOSITION_MAX_PASSES)
        return false;
    uint32_t ordinal = 0;
    for (uint32_t i = 0; i < snapshot->pass_count; i++)
    {
        if (snapshot->passes[i].role == role)
            ordinal++;
    }
    if (technique_instance_id.value == 0)
        return false;
    DvzSceneResolvedPass* pass = &snapshot->passes[snapshot->pass_count];
    *pass = (DvzSceneResolvedPass){
        .id = {.value = snapshot->pass_count + 1},
        .technique_instance_id = technique_instance_id,
        .technique_id = technique_id,
        .phase = phase,
        .role = role,
        .ordinal = ordinal,
        .authored_order_begin = authored_begin,
        .authored_order_end = authored_end,
    };
    snapshot->pass_count++;
    return true;
}



static DvzSceneWorkProviderKey _composition_provider(DvzFramePlanRenderPassRole role)
{
    switch (role)
    {
    case DVZ_FRAME_PLAN_RENDER_PASS_SCENE_OCCLUSION:
        return DVZ_SCENE_WORK_PROVIDER_SCENE_OCCLUSION;
    case DVZ_FRAME_PLAN_RENDER_PASS_VOLUME_OCCLUSION:
        return DVZ_SCENE_WORK_PROVIDER_VOLUME_OCCLUSION;
    case DVZ_FRAME_PLAN_RENDER_PASS_GBUFFER:
        return DVZ_SCENE_WORK_PROVIDER_SURFACE_CAPTURE;
    case DVZ_FRAME_PLAN_RENDER_PASS_SSAO:
        return DVZ_SCENE_WORK_PROVIDER_SSAO;
    case DVZ_FRAME_PLAN_RENDER_PASS_SSAO_BLUR:
        return DVZ_SCENE_WORK_PROVIDER_SSAO_BLUR;
    case DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE:
        return DVZ_SCENE_WORK_PROVIDER_OPAQUE;
    case DVZ_FRAME_PLAN_RENDER_PASS_SSAO_COMPOSITE:
        return DVZ_SCENE_WORK_PROVIDER_AMBIENT_COMPOSITE;
    case DVZ_FRAME_PLAN_RENDER_PASS_EDL_RESOLVE:
        return DVZ_SCENE_WORK_PROVIDER_EDL;
    case DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION:
        return DVZ_SCENE_WORK_PROVIDER_WBOIT_ACCUMULATION;
    case DVZ_FRAME_PLAN_RENDER_PASS_WBOIT_RESOLVE:
        return DVZ_SCENE_WORK_PROVIDER_WBOIT_RESOLVE;
    case DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT:
        return DVZ_SCENE_WORK_PROVIDER_DEPTH_PEEL_INIT;
    case DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER:
        return DVZ_SCENE_WORK_PROVIDER_DEPTH_PEEL_ITERATION;
    case DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_COMPOSITE:
        return DVZ_SCENE_WORK_PROVIDER_DEPTH_PEEL_COMPOSITE;
    case DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND:
        return DVZ_SCENE_WORK_PROVIDER_TRANSPARENT_BLEND;
    default:
        return DVZ_SCENE_WORK_PROVIDER_NONE;
    }
}



static DvzSceneScratchResourceId _composition_scratch(
    DvzPanelCompositionSnapshot* snapshot, DvzSceneTechniqueInstanceId technique_id,
    DvzSceneScratchScope scope, DvzSceneScratchKind kind, uint32_t usage_mask)
{
    if (scope == DVZ_SCENE_SCRATCH_SCOPE_PANEL)
        technique_id = (DvzSceneTechniqueInstanceId){0};
    for (uint32_t i = 0; i < snapshot->scratch_resource_count; i++)
    {
        DvzSceneScratchResource* resource = &snapshot->scratch_resources[i];
        if (resource->technique_instance_id.value == technique_id.value &&
            resource->scope == scope && resource->kind == kind)
        {
            resource->usage_mask |= usage_mask;
            return resource->id;
        }
    }
    if (snapshot->scratch_resource_count >= DVZ_PANEL_COMPOSITION_MAX_SCRATCH_RESOURCES)
        return (DvzSceneScratchResourceId){0};
    DvzRenderProductFormatClass format = DVZ_RENDER_PRODUCT_FORMAT_SCALAR_FLOAT;
    if (kind == DVZ_SCENE_SCRATCH_Z_ONLY_DEPTH || kind == DVZ_SCENE_SCRATCH_SURFACE_DEPTH ||
        kind == DVZ_SCENE_SCRATCH_FORWARD_DEPTH || kind == DVZ_SCENE_SCRATCH_PEEL_FORWARD_DEPTH ||
        kind == DVZ_SCENE_SCRATCH_EDL_DEPTH)
    {
        format = DVZ_RENDER_PRODUCT_FORMAT_DEPTH_FLOAT;
    }
    else if (kind == DVZ_SCENE_SCRATCH_SURFACE_NORMAL_LEGACY)
        format = DVZ_RENDER_PRODUCT_FORMAT_NORMAL_FLOAT;
    else if (kind == DVZ_SCENE_SCRATCH_PEEL_BACK_ACCUM)
        format = DVZ_RENDER_PRODUCT_FORMAT_LINEAR_COLOR;
    else if (
        kind == DVZ_SCENE_SCRATCH_PEEL_DEPTH_PING || kind == DVZ_SCENE_SCRATCH_PEEL_DEPTH_PONG)
        format = DVZ_RENDER_PRODUCT_FORMAT_VECTOR2_FLOAT;
    else if (kind == DVZ_SCENE_SCRATCH_EDL_COLOR)
        format = DVZ_RENDER_PRODUCT_FORMAT_PRESENTATION_COLOR;
    if (kind <= DVZ_SCENE_SCRATCH_NONE || kind > DVZ_SCENE_SCRATCH_EDL_DEPTH || usage_mask == 0)
        return (DvzSceneScratchResourceId){0};
    DvzSceneScratchResource* resource =
        &snapshot->scratch_resources[snapshot->scratch_resource_count++];
    *resource = (DvzSceneScratchResource){
        .id = {.value = snapshot->scratch_resource_count},
        .technique_instance_id = technique_id,
        .scope = scope,
        .kind = kind,
        .format = kind == DVZ_SCENE_SCRATCH_SSAO_RAW
                      ? DVZ_FORMAT_R8_UNORM
                      : (kind == DVZ_SCENE_SCRATCH_SCENE_OCCLUSION_DEVICE_DEPTH ||
                                 kind == DVZ_SCENE_SCRATCH_VOLUME_OCCLUSION_DEVICE_DEPTH
                             ? DVZ_FORMAT_R32_SFLOAT
                             : (kind == DVZ_SCENE_SCRATCH_WBOIT_WEIGHT
                                    ? DVZ_FORMAT_R16_SFLOAT
                                    : (kind == DVZ_SCENE_SCRATCH_SURFACE_NORMAL_LEGACY ||
                                               kind == DVZ_SCENE_SCRATCH_PEEL_BACK_ACCUM
                                           ? DVZ_FORMAT_R16G16B16A16_SFLOAT
                                           : (kind == DVZ_SCENE_SCRATCH_PEEL_DEPTH_PING ||
                                                      kind == DVZ_SCENE_SCRATCH_PEEL_DEPTH_PONG
                                                  ? DVZ_FORMAT_R32G32_SFLOAT
                                                  : (kind == DVZ_SCENE_SCRATCH_EDL_COLOR
                                                         ? DVZ_FORMAT_R8G8B8A8_UNORM
                                                         : DVZ_FORMAT_D32_SFLOAT))))),
        .format_class = format,
        .extent_policy = DVZ_RENDER_PRODUCT_EXTENT_PANEL_RELATIVE,
        .sample_domain = DVZ_RENDER_PRODUCT_SAMPLES_SINGLE,
        .sample_count = 1,
        .usage_mask = usage_mask,
        .lifetime = scope == DVZ_SCENE_SCRATCH_SCOPE_PANEL ? DVZ_SCENE_SCRATCH_LIFETIME_FRAME
                                                           : DVZ_SCENE_SCRATCH_LIFETIME_TECHNIQUE,
    };
    return resource->id;
}



static bool _composition_scratch_samples(
    DvzPanelCompositionSnapshot* snapshot, DvzSceneScratchResourceId id, uint32_t sample_count)
{
    ANN(snapshot);
    if (id.value == 0 || id.value > snapshot->scratch_resource_count || sample_count == 0)
        return false;
    DvzSceneScratchResource* scratch = &snapshot->scratch_resources[id.value - 1];
    if (scratch->sample_count != 1 && scratch->sample_count != sample_count)
        return false;
    scratch->sample_count = sample_count;
    scratch->sample_domain = sample_count > 1 ? DVZ_RENDER_PRODUCT_SAMPLES_MULTISAMPLE
                                              : DVZ_RENDER_PRODUCT_SAMPLES_SINGLE;
    return true;
}



static bool _composition_pass_has_scratch(
    const DvzPanelCompositionSnapshot* snapshot, const DvzSceneResolvedPass* pass,
    DvzSceneScratchKind kind)
{
    ANN(snapshot);
    ANN(pass);
    for (uint32_t i = 0; i < pass->binding_count; i++)
    {
        const DvzSceneWorkBinding* binding = &pass->bindings[i];
        if (binding->ref_kind == DVZ_SCENE_RESOURCE_REF_SCRATCH && binding->scratch_id.value > 0 &&
            binding->scratch_id.value <= snapshot->scratch_resource_count &&
            snapshot->scratch_resources[binding->scratch_id.value - 1].kind == kind)
            return true;
    }
    return false;
}



static bool _composition_bind(
    DvzSceneResolvedPass* pass, DvzSceneResourceRefKind ref_kind, DvzRenderProductId product_id,
    DvzSceneScratchResourceId scratch_id, DvzSceneWorkBindingUsage usage,
    DvzSceneWorkAccess access, uint32_t slot, uint32_t set, uint32_t binding,
    DvzSceneAttachmentLoad load, DvzSceneAttachmentStore store, bool clear, bool depth_attachment)
{
    if (pass->binding_count >= DVZ_PANEL_COMPOSITION_MAX_WORK_BINDINGS)
        return false;
    pass->bindings[pass->binding_count++] = (DvzSceneWorkBinding){
        .ref_kind = ref_kind,
        .product_id = product_id,
        .scratch_id = scratch_id,
        .usage = usage,
        .access = access,
        .slot = slot,
        .set = set,
        .binding = binding,
        .load = load,
        .store = store,
        .clear = clear,
        .clear_value_kind = clear ? DVZ_SCENE_CLEAR_VALUE_LITERAL : DVZ_SCENE_CLEAR_VALUE_NONE,
        .depth_attachment = depth_attachment,
    };
    return true;
}



static void _composition_frame_clear_value(DvzSceneResolvedPass* pass)
{
    ANN(pass);
    ASSERT(pass->binding_count > 0);
    pass->bindings[pass->binding_count - 1].clear_value_kind = DVZ_SCENE_CLEAR_VALUE_FRAME;
}



static void
_composition_clear_value(DvzSceneResolvedPass* pass, float x, float y, float z, float w)
{
    ANN(pass);
    ASSERT(pass->binding_count > 0);
    DvzSceneWorkBinding* binding = &pass->bindings[pass->binding_count - 1];
    binding->clear_value[0] = x;
    binding->clear_value[1] = y;
    binding->clear_value[2] = z;
    binding->clear_value[3] = w;
}



static bool
_composition_has_technique(const DvzPanelCompositionSnapshot* snapshot, DvzSceneTechniqueId id)
{
    ANN(snapshot);
    for (uint32_t i = 0; i < snapshot->technique_count; i++)
        if (snapshot->techniques[i].id == id)
            return true;
    return false;
}



static uint32_t _composition_blend_group_for_technique(
    const DvzPanelRenderPlan* render_plan, const DvzSceneResolvedTechnique* technique)
{
    ANN(render_plan);
    ANN(technique);
    for (uint32_t group = 0; group < render_plan->blended_group_count; group++)
    {
        uint32_t begin = UINT32_MAX;
        uint32_t end = UINT32_MAX;
        if (_composition_authored_bounds(
                render_plan->blended_visuals, render_plan->blended_visual_count, group, &begin,
                &end) &&
            begin == technique->authored_order_begin && end == technique->authored_order_end)
            return group;
    }
    return UINT32_MAX;
}



typedef struct
{
    const DvzPanelRenderPlan* render_plan;
    uint32_t opaque_samples;
    bool effective_edl;
    bool forward_depth_written;
} CompositionWorkContext;



static bool _composition_declare_work(
    DvzPanelCompositionSnapshot* snapshot, DvzSceneResolvedPass* pass,
    const DvzSceneResolvedTechnique* technique, uint32_t work_index,
    CompositionWorkContext* context)
{
    ANN(snapshot);
    ANN(pass);
    ANN(technique);
    ANN(context);
    ANN(context->render_plan);
    const DvzPanelRenderPlan* render_plan = context->render_plan;
    pass->work_index = work_index;
    pass->work_class = (pass->role == DVZ_FRAME_PLAN_RENDER_PASS_SSAO ||
                        pass->role == DVZ_FRAME_PLAN_RENDER_PASS_SSAO_COMPOSITE ||
                        pass->role == DVZ_FRAME_PLAN_RENDER_PASS_EDL_RESOLVE ||
                        pass->role == DVZ_FRAME_PLAN_RENDER_PASS_WBOIT_RESOLVE ||
                        pass->role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_COMPOSITE ||
                        pass->role == DVZ_FRAME_PLAN_RENDER_PASS_SSAO_BLUR)
                           ? DVZ_SCENE_WORK_FULLSCREEN
                           : DVZ_SCENE_WORK_VISUAL_DRAWS;
    pass->provider = _composition_provider(pass->role);
    pass->coordinate_space = DVZ_RENDER_PRODUCT_COORDINATES_PANEL_LOCAL;
    pass->viewport_panel_local = true;
    pass->scissor_panel_local = true;
    pass->sample_count =
        pass->role == DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE ? context->opaque_samples : 1;
    pass->resolve_policy = DVZ_RENDER_PRODUCT_RESOLVE_NONE;
    if (pass->role == DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE && context->opaque_samples > 1)
        pass->resolve_policy = DVZ_RENDER_PRODUCT_RESOLVE_LINEAR_COLOR;
    pass->alpha_to_coverage = pass->role == DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE &&
                              context->opaque_samples > 1 && render_plan->msaa_state != NULL &&
                              render_plan->msaa_state->alpha_to_coverage;
    pass->visual_layer_filter = technique->participating_layer_mask;
    pass->visual_order_begin = technique->authored_order_begin;
    pass->visual_order_end = technique->authored_order_end;
    DvzSceneAuxiliaryKind auxiliary_kind = DVZ_SCENE_AUXILIARY_NONE;
    uint32_t auxiliary_binding = 0;
    if (pass->provider == DVZ_SCENE_WORK_PROVIDER_EDL)
    {
        auxiliary_kind = DVZ_SCENE_AUXILIARY_EDL_PARAMS;
        auxiliary_binding = 3;
    }
    else if (pass->provider == DVZ_SCENE_WORK_PROVIDER_SSAO)
    {
        auxiliary_kind = DVZ_SCENE_AUXILIARY_SSAO_PARAMS;
        auxiliary_binding = 3;
    }
    else if (pass->provider == DVZ_SCENE_WORK_PROVIDER_SSAO_BLUR)
    {
        auxiliary_kind = DVZ_SCENE_AUXILIARY_SSAO_PARAMS;
        auxiliary_binding = 4;
    }
    else if (pass->provider == DVZ_SCENE_WORK_PROVIDER_AMBIENT_COMPOSITE)
    {
        auxiliary_kind = DVZ_SCENE_AUXILIARY_SSAO_PARAMS;
        auxiliary_binding = 2;
    }
    if (auxiliary_kind != DVZ_SCENE_AUXILIARY_NONE)
    {
        pass->auxiliary_binding_count = 1;
        pass->auxiliary_bindings[0] = (DvzSceneAuxiliaryBinding){
            .kind = auxiliary_kind,
            .upload_node_index = UINT32_MAX,
            .set = 0,
            .binding = auxiliary_binding,
            .byte_size = auxiliary_kind == DVZ_SCENE_AUXILIARY_EDL_PARAMS
                             ? sizeof(DvzSceneEdlUniform)
                             : sizeof(DvzSceneSsaoUniform),
        };
    }
    DvzRenderProductId in[DVZ_RENDER_PRODUCT_PRESENTATION_COLOR + 1] = {{0}};
    DvzRenderProductId out[DVZ_RENDER_PRODUCT_PRESENTATION_COLOR + 1] = {{0}};
    for (uint32_t i = 0; i < technique->input_count; i++)
        in[technique->inputs[i]] = technique->input_ids[i];
    for (uint32_t i = 0; i < technique->output_count; i++)
        out[technique->outputs[i]] = technique->output_ids[i];
#define SAMPLE(_p, _binding)                                                                      \
    _composition_bind(                                                                            \
        pass, DVZ_SCENE_RESOURCE_REF_PRODUCT, in[_p], (DvzSceneScratchResourceId){0},             \
        DVZ_SCENE_WORK_BINDING_SAMPLED, DVZ_SCENE_WORK_ACCESS_READ, UINT32_MAX, 0, _binding,      \
        DVZ_SCENE_ATTACHMENT_LOAD_NONE, DVZ_SCENE_ATTACHMENT_STORE_NONE, false, false)
#define SAMPLE_OUT(_p, _binding)                                                                  \
    _composition_bind(                                                                            \
        pass, DVZ_SCENE_RESOURCE_REF_PRODUCT, out[_p], (DvzSceneScratchResourceId){0},            \
        DVZ_SCENE_WORK_BINDING_SAMPLED, DVZ_SCENE_WORK_ACCESS_READ, UINT32_MAX, 0, _binding,      \
        DVZ_SCENE_ATTACHMENT_LOAD_NONE, DVZ_SCENE_ATTACHMENT_STORE_NONE, false, false)
#define ATTACH(_p, _slot, _load)                                                                  \
    _composition_bind(                                                                            \
        pass, DVZ_SCENE_RESOURCE_REF_PRODUCT, out[_p], (DvzSceneScratchResourceId){0},            \
        DVZ_SCENE_WORK_BINDING_ATTACHMENT,                                                        \
        (_load) == DVZ_SCENE_ATTACHMENT_LOAD_LOAD ? DVZ_SCENE_WORK_ACCESS_READ_WRITE              \
                                                  : DVZ_SCENE_WORK_ACCESS_WRITE,                  \
        _slot, UINT32_MAX, UINT32_MAX, _load, DVZ_SCENE_ATTACHMENT_STORE_STORE,                   \
        _load == DVZ_SCENE_ATTACHMENT_LOAD_CLEAR, false)
#define SCRATCH(                                                                                  \
    _kind, _scope, _resource_usage, _usage, _access, _slot, _binding, _load, _store, _depth)      \
    _composition_bind(                                                                            \
        pass, DVZ_SCENE_RESOURCE_REF_SCRATCH, (DvzRenderProductId){0},                            \
        _composition_scratch(snapshot, technique->instance_id, _scope, _kind, _resource_usage),   \
        _usage, _access, _slot, _usage == DVZ_SCENE_WORK_BINDING_SAMPLED ? 0 : UINT32_MAX,        \
        _binding, _load, _store, (_load) == DVZ_SCENE_ATTACHMENT_LOAD_CLEAR, _depth)
#define SCRATCH_PROVIDER(_kind)                                                                   \
    _composition_bind(                                                                            \
        pass, DVZ_SCENE_RESOURCE_REF_SCRATCH, (DvzRenderProductId){0},                            \
        _composition_scratch(                                                                     \
            snapshot, technique->instance_id, DVZ_SCENE_SCRATCH_SCOPE_PANEL, _kind, COLOR_USAGE), \
        DVZ_SCENE_WORK_BINDING_SAMPLED, DVZ_SCENE_WORK_ACCESS_READ, UINT32_MAX, UINT32_MAX,       \
        UINT32_MAX, DVZ_SCENE_ATTACHMENT_LOAD_NONE, DVZ_SCENE_ATTACHMENT_STORE_NONE, false,       \
        false)
#define COLOR_USAGE                                                                               \
    (DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT | DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED)
#define DEPTH_USAGE DVZ_FRAME_GRAPH_RESOURCE_USAGE_DEPTH_ATTACHMENT
#define DEPTH_SAMPLED_USAGE                                                                       \
    (DVZ_FRAME_GRAPH_RESOURCE_USAGE_DEPTH_ATTACHMENT | DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED)
    bool ok = true;
    switch (pass->role)
    {
    case DVZ_FRAME_PLAN_RENDER_PASS_SCENE_OCCLUSION:
        ok = SCRATCH(
                 DVZ_SCENE_SCRATCH_SCENE_OCCLUSION_DEVICE_DEPTH, DVZ_SCENE_SCRATCH_SCOPE_PANEL,
                 COLOR_USAGE, DVZ_SCENE_WORK_BINDING_ATTACHMENT, DVZ_SCENE_WORK_ACCESS_WRITE, 0,
                 UINT32_MAX, DVZ_SCENE_ATTACHMENT_LOAD_CLEAR, DVZ_SCENE_ATTACHMENT_STORE_STORE,
                 false) &&
             SCRATCH(
                 DVZ_SCENE_SCRATCH_Z_ONLY_DEPTH, DVZ_SCENE_SCRATCH_SCOPE_TECHNIQUE, DEPTH_USAGE,
                 DVZ_SCENE_WORK_BINDING_ATTACHMENT, DVZ_SCENE_WORK_ACCESS_WRITE, UINT32_MAX,
                 UINT32_MAX, DVZ_SCENE_ATTACHMENT_LOAD_CLEAR, DVZ_SCENE_ATTACHMENT_STORE_STORE,
                 true);
        if (ok)
        {
            _composition_clear_value(pass, 1, 1, 1, 1);
            pass->bindings[0].clear_value[0] = 1.0f;
            pass->bindings[0].clear_value[1] = 1.0f;
            pass->bindings[0].clear_value[2] = 1.0f;
            pass->bindings[0].clear_value[3] = 1.0f;
            pass->legacy_transition = true;
            ok = _composition_mark_unrealized(pass, out[DVZ_RENDER_PRODUCT_SCENE_OCCLUSION_DEPTH]);
        }
        break;
    case DVZ_FRAME_PLAN_RENDER_PASS_VOLUME_OCCLUSION:
        ok = SCRATCH(
            DVZ_SCENE_SCRATCH_VOLUME_OCCLUSION_DEVICE_DEPTH, DVZ_SCENE_SCRATCH_SCOPE_PANEL,
            COLOR_USAGE, DVZ_SCENE_WORK_BINDING_ATTACHMENT, DVZ_SCENE_WORK_ACCESS_WRITE, 0,
            UINT32_MAX, DVZ_SCENE_ATTACHMENT_LOAD_CLEAR, DVZ_SCENE_ATTACHMENT_STORE_STORE, false);
        pass->legacy_transition = true;
        ok = ok &&
             _composition_mark_unrealized(pass, out[DVZ_RENDER_PRODUCT_VOLUME_FIRST_HIT_DEPTH]);
        break;
    case DVZ_FRAME_PLAN_RENDER_PASS_GBUFFER:
        ok = SCRATCH(
                 DVZ_SCENE_SCRATCH_SURFACE_NORMAL_LEGACY, DVZ_SCENE_SCRATCH_SCOPE_PANEL,
                 COLOR_USAGE, DVZ_SCENE_WORK_BINDING_ATTACHMENT, DVZ_SCENE_WORK_ACCESS_WRITE, 0,
                 UINT32_MAX, DVZ_SCENE_ATTACHMENT_LOAD_CLEAR, DVZ_SCENE_ATTACHMENT_STORE_STORE,
                 false) &&
             SCRATCH(
                 DVZ_SCENE_SCRATCH_SURFACE_DEPTH, DVZ_SCENE_SCRATCH_SCOPE_PANEL,
                 DEPTH_SAMPLED_USAGE, DVZ_SCENE_WORK_BINDING_ATTACHMENT,
                 DVZ_SCENE_WORK_ACCESS_WRITE, UINT32_MAX, UINT32_MAX,
                 DVZ_SCENE_ATTACHMENT_LOAD_CLEAR, DVZ_SCENE_ATTACHMENT_STORE_STORE, true);
        if (ok)
        {
            pass->bindings[0].clear_value[0] = 0.5f;
            pass->bindings[0].clear_value[1] = 0.5f;
            pass->bindings[0].clear_value[2] = 1.0f;
            _composition_clear_value(pass, 1, 0, 0, 0);
            pass->legacy_transition = true;
            ok = _composition_mark_unrealized(pass, out[DVZ_RENDER_PRODUCT_SURFACE_NORMAL]) &&
                 _composition_mark_unrealized(pass, out[DVZ_RENDER_PRODUCT_SURFACE_DEPTH]) &&
                 _composition_mark_unrealized(pass, out[DVZ_RENDER_PRODUCT_SURFACE_COVERAGE]);
        }
        break;
    case DVZ_FRAME_PLAN_RENDER_PASS_SSAO:
        ok =
            SCRATCH(
                DVZ_SCENE_SCRATCH_SURFACE_NORMAL_LEGACY, DVZ_SCENE_SCRATCH_SCOPE_PANEL,
                COLOR_USAGE, DVZ_SCENE_WORK_BINDING_SAMPLED, DVZ_SCENE_WORK_ACCESS_READ,
                UINT32_MAX, 0, DVZ_SCENE_ATTACHMENT_LOAD_NONE, DVZ_SCENE_ATTACHMENT_STORE_NONE,
                false) &&
            SCRATCH(
                DVZ_SCENE_SCRATCH_SURFACE_DEPTH, DVZ_SCENE_SCRATCH_SCOPE_PANEL,
                DEPTH_SAMPLED_USAGE, DVZ_SCENE_WORK_BINDING_SAMPLED, DVZ_SCENE_WORK_ACCESS_READ,
                UINT32_MAX, 1, DVZ_SCENE_ATTACHMENT_LOAD_NONE, DVZ_SCENE_ATTACHMENT_STORE_NONE,
                false) &&
            ((technique->expansion_flags & COMPOSITION_EXPAND_SSAO_BLUR) != 0
                 ? SCRATCH(
                       DVZ_SCENE_SCRATCH_SSAO_RAW, DVZ_SCENE_SCRATCH_SCOPE_TECHNIQUE, COLOR_USAGE,
                       DVZ_SCENE_WORK_BINDING_ATTACHMENT, DVZ_SCENE_WORK_ACCESS_WRITE, 0,
                       UINT32_MAX, DVZ_SCENE_ATTACHMENT_LOAD_CLEAR,
                       DVZ_SCENE_ATTACHMENT_STORE_STORE, false)
                 : ATTACH(
                       DVZ_RENDER_PRODUCT_AMBIENT_VISIBILITY, 0, DVZ_SCENE_ATTACHMENT_LOAD_CLEAR));
        if (ok)
        {
            _composition_clear_value(pass, 1, 1, 1, 1);
            pass->legacy_transition = true;
            ok = _composition_mark_unrealized(pass, in[DVZ_RENDER_PRODUCT_SURFACE_NORMAL]) &&
                 _composition_mark_unrealized(pass, in[DVZ_RENDER_PRODUCT_SURFACE_DEPTH]) &&
                 _composition_mark_unrealized(pass, in[DVZ_RENDER_PRODUCT_SURFACE_COVERAGE]);
        }
        break;
    case DVZ_FRAME_PLAN_RENDER_PASS_SSAO_BLUR:
        ok = SCRATCH(
                 DVZ_SCENE_SCRATCH_SSAO_RAW, DVZ_SCENE_SCRATCH_SCOPE_TECHNIQUE, COLOR_USAGE,
                 DVZ_SCENE_WORK_BINDING_SAMPLED, DVZ_SCENE_WORK_ACCESS_READ, UINT32_MAX, 0,
                 DVZ_SCENE_ATTACHMENT_LOAD_NONE, DVZ_SCENE_ATTACHMENT_STORE_NONE, false) &&
             SCRATCH(
                 DVZ_SCENE_SCRATCH_SURFACE_NORMAL_LEGACY, DVZ_SCENE_SCRATCH_SCOPE_PANEL,
                 COLOR_USAGE, DVZ_SCENE_WORK_BINDING_SAMPLED, DVZ_SCENE_WORK_ACCESS_READ,
                 UINT32_MAX, 1, DVZ_SCENE_ATTACHMENT_LOAD_NONE, DVZ_SCENE_ATTACHMENT_STORE_NONE,
                 false) &&
             SCRATCH(
                 DVZ_SCENE_SCRATCH_SURFACE_DEPTH, DVZ_SCENE_SCRATCH_SCOPE_PANEL,
                 DEPTH_SAMPLED_USAGE, DVZ_SCENE_WORK_BINDING_SAMPLED, DVZ_SCENE_WORK_ACCESS_READ,
                 UINT32_MAX, 2, DVZ_SCENE_ATTACHMENT_LOAD_NONE, DVZ_SCENE_ATTACHMENT_STORE_NONE,
                 false) &&
             ATTACH(DVZ_RENDER_PRODUCT_AMBIENT_VISIBILITY, 0, DVZ_SCENE_ATTACHMENT_LOAD_CLEAR);
        if (ok)
        {
            _composition_clear_value(pass, 1, 1, 1, 1);
            pass->legacy_transition = true;
            ok = _composition_mark_unrealized(pass, in[DVZ_RENDER_PRODUCT_SURFACE_NORMAL]) &&
                 _composition_mark_unrealized(pass, in[DVZ_RENDER_PRODUCT_SURFACE_DEPTH]) &&
                 _composition_mark_unrealized(pass, in[DVZ_RENDER_PRODUCT_SURFACE_COVERAGE]);
        }
        break;
    case DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE:
    {
        const bool depth_peel =
            _composition_has_technique(snapshot, DVZ_SCENE_TECHNIQUE_DEPTH_PEEL);
        const bool forward_depth_required =
            render_plan->opaque_needs_depth ||
            (render_plan->blended_group_count > 0 && render_plan->transparent_needs_depth);
        if (context->effective_edl)
        {
            ok = SCRATCH(
                     DVZ_SCENE_SCRATCH_EDL_COLOR, DVZ_SCENE_SCRATCH_SCOPE_PANEL, COLOR_USAGE,
                     DVZ_SCENE_WORK_BINDING_ATTACHMENT, DVZ_SCENE_WORK_ACCESS_WRITE, 0, UINT32_MAX,
                     DVZ_SCENE_ATTACHMENT_LOAD_CLEAR, DVZ_SCENE_ATTACHMENT_STORE_STORE, false) &&
                 SCRATCH(
                     DVZ_SCENE_SCRATCH_EDL_DEPTH, DVZ_SCENE_SCRATCH_SCOPE_PANEL,
                     DEPTH_SAMPLED_USAGE, DVZ_SCENE_WORK_BINDING_ATTACHMENT,
                     DVZ_SCENE_WORK_ACCESS_WRITE, UINT32_MAX, UINT32_MAX,
                     DVZ_SCENE_ATTACHMENT_LOAD_CLEAR, DVZ_SCENE_ATTACHMENT_STORE_STORE, true);
            if (ok)
            {
                _composition_clear_value(pass, 1, 0, 0, 0);
                pass->legacy_transition = true;
                for (uint32_t i = 0; i < technique->output_count; i++)
                    ok = ok && _composition_mark_unrealized(pass, technique->output_ids[i]);
            }
        }
        else
        {
            ok = ATTACH(DVZ_RENDER_PRODUCT_SCENE_COLOR, 0, DVZ_SCENE_ATTACHMENT_LOAD_CLEAR);
            if (ok)
                _composition_frame_clear_value(pass);
        }
        if (ok && forward_depth_required && !context->effective_edl)
            ok = SCRATCH(
                depth_peel ? DVZ_SCENE_SCRATCH_PEEL_FORWARD_DEPTH
                           : DVZ_SCENE_SCRATCH_FORWARD_DEPTH,
                DVZ_SCENE_SCRATCH_SCOPE_PANEL,
                !depth_peel && (render_plan->blended_group_count > 0 ||
                                render_plan->wboit_visual_count > 0)
                    ? DEPTH_SAMPLED_USAGE
                    : DEPTH_USAGE,
                DVZ_SCENE_WORK_BINDING_ATTACHMENT, DVZ_SCENE_WORK_ACCESS_WRITE, UINT32_MAX,
                UINT32_MAX, DVZ_SCENE_ATTACHMENT_LOAD_CLEAR, DVZ_SCENE_ATTACHMENT_STORE_STORE,
                true);
        if (ok && forward_depth_required && !context->effective_edl)
        {
            ok = _composition_scratch_samples(
                snapshot, pass->bindings[pass->binding_count - 1].scratch_id,
                context->opaque_samples);
            _composition_clear_value(pass, 1, 0, 0, 0);
            context->forward_depth_written = !depth_peel;
        }
        if (ok && in[DVZ_RENDER_PRODUCT_AMBIENT_VISIBILITY].value != 0)
        {
            pass->legacy_transition = true;
            ok = _composition_mark_unrealized(pass, in[DVZ_RENDER_PRODUCT_AMBIENT_VISIBILITY]);
        }
        break;
    }
    case DVZ_FRAME_PLAN_RENDER_PASS_SSAO_COMPOSITE:
        ok = SAMPLE(DVZ_RENDER_PRODUCT_AMBIENT_VISIBILITY, 0) &&
             ATTACH(DVZ_RENDER_PRODUCT_SCENE_COLOR, 0, DVZ_SCENE_ATTACHMENT_LOAD_LOAD);
        break;
    case DVZ_FRAME_PLAN_RENDER_PASS_EDL_RESOLVE:
        ok = SCRATCH(
                 DVZ_SCENE_SCRATCH_EDL_COLOR, DVZ_SCENE_SCRATCH_SCOPE_PANEL, COLOR_USAGE,
                 DVZ_SCENE_WORK_BINDING_SAMPLED, DVZ_SCENE_WORK_ACCESS_READ, UINT32_MAX, 0,
                 DVZ_SCENE_ATTACHMENT_LOAD_NONE, DVZ_SCENE_ATTACHMENT_STORE_NONE, false) &&
             SCRATCH(
                 DVZ_SCENE_SCRATCH_EDL_DEPTH, DVZ_SCENE_SCRATCH_SCOPE_PANEL, DEPTH_SAMPLED_USAGE,
                 DVZ_SCENE_WORK_BINDING_SAMPLED, DVZ_SCENE_WORK_ACCESS_READ, UINT32_MAX, 1,
                 DVZ_SCENE_ATTACHMENT_LOAD_NONE, DVZ_SCENE_ATTACHMENT_STORE_NONE, false) &&
             ATTACH(DVZ_RENDER_PRODUCT_SCENE_COLOR, 0, DVZ_SCENE_ATTACHMENT_LOAD_CLEAR);
        pass->legacy_transition = true;
        for (uint32_t i = 0; i < technique->input_count; i++)
            ok = ok && _composition_mark_unrealized(pass, technique->input_ids[i]);
        if (ok)
            _composition_frame_clear_value(pass);
        break;
    case DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND:
    {
        const uint32_t group = _composition_blend_group_for_technique(render_plan, technique);
        const bool needs_depth =
            group < render_plan->blended_group_count && render_plan->blended_needs_depth[group];
        const bool writes_depth =
            group < render_plan->blended_group_count && render_plan->blended_writes_depth[group];
        ok = ATTACH(DVZ_RENDER_PRODUCT_SCENE_COLOR, 0, DVZ_SCENE_ATTACHMENT_LOAD_LOAD);
        if (ok && needs_depth)
        {
            const DvzSceneAttachmentLoad load = context->forward_depth_written
                                                    ? DVZ_SCENE_ATTACHMENT_LOAD_LOAD
                                                    : DVZ_SCENE_ATTACHMENT_LOAD_CLEAR;
            const DvzSceneWorkAccess access =
                writes_depth ? (context->forward_depth_written ? DVZ_SCENE_WORK_ACCESS_READ_WRITE
                                                               : DVZ_SCENE_WORK_ACCESS_WRITE)
                             : (context->forward_depth_written ? DVZ_SCENE_WORK_ACCESS_READ
                                                               : DVZ_SCENE_WORK_ACCESS_WRITE);
            ok = SCRATCH(
                DVZ_SCENE_SCRATCH_FORWARD_DEPTH, DVZ_SCENE_SCRATCH_SCOPE_PANEL,
                DEPTH_SAMPLED_USAGE, DVZ_SCENE_WORK_BINDING_ATTACHMENT, access, UINT32_MAX,
                UINT32_MAX, load,
                writes_depth ? DVZ_SCENE_ATTACHMENT_STORE_STORE
                             : DVZ_SCENE_ATTACHMENT_STORE_DONT_CARE,
                true);
            if (ok && load == DVZ_SCENE_ATTACHMENT_LOAD_CLEAR)
                _composition_clear_value(pass, 1, 0, 0, 0);
            /* CLEAR establishes a valid depth surface even when this group only reads it. */
            context->forward_depth_written = true;
        }
        break;
    }
    case DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION:
        ok =
            ATTACH(
                DVZ_RENDER_PRODUCT_TRANSPARENT_ACCUMULATION, 0, DVZ_SCENE_ATTACHMENT_LOAD_CLEAR) &&
            SCRATCH(
                DVZ_SCENE_SCRATCH_WBOIT_WEIGHT, DVZ_SCENE_SCRATCH_SCOPE_TECHNIQUE, COLOR_USAGE,
                DVZ_SCENE_WORK_BINDING_ATTACHMENT, DVZ_SCENE_WORK_ACCESS_WRITE, 1, UINT32_MAX,
                DVZ_SCENE_ATTACHMENT_LOAD_CLEAR, DVZ_SCENE_ATTACHMENT_STORE_STORE, false);
        pass->legacy_transition = true;
        ok = ok &&
             _composition_mark_unrealized(pass, out[DVZ_RENDER_PRODUCT_TRANSPARENT_TRANSMITTANCE]);
        if (ok && render_plan->transparent_needs_depth)
        {
            ok = SCRATCH(
                DVZ_SCENE_SCRATCH_FORWARD_DEPTH, DVZ_SCENE_SCRATCH_SCOPE_PANEL,
                DEPTH_SAMPLED_USAGE, DVZ_SCENE_WORK_BINDING_ATTACHMENT,
                context->forward_depth_written ? DVZ_SCENE_WORK_ACCESS_READ
                                               : DVZ_SCENE_WORK_ACCESS_WRITE,
                UINT32_MAX, UINT32_MAX,
                context->forward_depth_written ? DVZ_SCENE_ATTACHMENT_LOAD_LOAD
                                               : DVZ_SCENE_ATTACHMENT_LOAD_CLEAR,
                DVZ_SCENE_ATTACHMENT_STORE_STORE, true);
            if (ok && !context->forward_depth_written)
                _composition_clear_value(pass, 1, 0, 0, 0);
            context->forward_depth_written = true;
        }
        break;
    case DVZ_FRAME_PLAN_RENDER_PASS_WBOIT_RESOLVE:
        ok = SAMPLE_OUT(DVZ_RENDER_PRODUCT_TRANSPARENT_ACCUMULATION, 0) &&
             SCRATCH(
                 DVZ_SCENE_SCRATCH_WBOIT_WEIGHT, DVZ_SCENE_SCRATCH_SCOPE_TECHNIQUE, COLOR_USAGE,
                 DVZ_SCENE_WORK_BINDING_SAMPLED, DVZ_SCENE_WORK_ACCESS_READ, UINT32_MAX, 1,
                 DVZ_SCENE_ATTACHMENT_LOAD_NONE, DVZ_SCENE_ATTACHMENT_STORE_NONE, false) &&
             ATTACH(DVZ_RENDER_PRODUCT_SCENE_COLOR, 0, DVZ_SCENE_ATTACHMENT_LOAD_LOAD);
        pass->legacy_transition = true;
        ok = ok &&
             _composition_mark_unrealized(pass, out[DVZ_RENDER_PRODUCT_TRANSPARENT_TRANSMITTANCE]);
        break;
    case DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT:
        ok =
            ATTACH(
                DVZ_RENDER_PRODUCT_TRANSPARENT_ACCUMULATION, 0, DVZ_SCENE_ATTACHMENT_LOAD_CLEAR) &&
            SCRATCH(
                DVZ_SCENE_SCRATCH_PEEL_BACK_ACCUM, DVZ_SCENE_SCRATCH_SCOPE_TECHNIQUE, COLOR_USAGE,
                DVZ_SCENE_WORK_BINDING_ATTACHMENT, DVZ_SCENE_WORK_ACCESS_WRITE, 1, UINT32_MAX,
                DVZ_SCENE_ATTACHMENT_LOAD_CLEAR, DVZ_SCENE_ATTACHMENT_STORE_STORE, false) &&
            SCRATCH(
                DVZ_SCENE_SCRATCH_PEEL_DEPTH_PING, DVZ_SCENE_SCRATCH_SCOPE_TECHNIQUE, COLOR_USAGE,
                DVZ_SCENE_WORK_BINDING_ATTACHMENT, DVZ_SCENE_WORK_ACCESS_WRITE, 2, UINT32_MAX,
                DVZ_SCENE_ATTACHMENT_LOAD_CLEAR, DVZ_SCENE_ATTACHMENT_STORE_STORE, false);
        if (ok)
        {
            _composition_clear_value(pass, -1, -1, 0, 0);
        }
        if (ok && render_plan->transparent_needs_depth)
        {
            ok = SCRATCH(
                DVZ_SCENE_SCRATCH_PEEL_FORWARD_DEPTH, DVZ_SCENE_SCRATCH_SCOPE_PANEL, DEPTH_USAGE,
                DVZ_SCENE_WORK_BINDING_ATTACHMENT,
                render_plan->opaque_needs_depth ? DVZ_SCENE_WORK_ACCESS_READ
                                                : DVZ_SCENE_WORK_ACCESS_WRITE,
                UINT32_MAX, UINT32_MAX,
                render_plan->opaque_needs_depth ? DVZ_SCENE_ATTACHMENT_LOAD_LOAD
                                                : DVZ_SCENE_ATTACHMENT_LOAD_CLEAR,
                DVZ_SCENE_ATTACHMENT_STORE_DONT_CARE, true);
            if (ok && !render_plan->opaque_needs_depth)
                _composition_clear_value(pass, 1, 0, 0, 0);
        }
        pass->legacy_transition = true;
        ok = ok &&
             _composition_mark_unrealized(pass, out[DVZ_RENDER_PRODUCT_TRANSPARENT_PEEL_DEPTH]);
        break;
    case DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER:
    {
        const DvzSceneScratchKind previous = work_index % 2 == 0
                                                 ? DVZ_SCENE_SCRATCH_PEEL_DEPTH_PING
                                                 : DVZ_SCENE_SCRATCH_PEEL_DEPTH_PONG;
        const DvzSceneScratchKind next = work_index % 2 == 0 ? DVZ_SCENE_SCRATCH_PEEL_DEPTH_PONG
                                                             : DVZ_SCENE_SCRATCH_PEEL_DEPTH_PING;
        ok =
            SCRATCH(
                previous, DVZ_SCENE_SCRATCH_SCOPE_TECHNIQUE, COLOR_USAGE,
                DVZ_SCENE_WORK_BINDING_SAMPLED, DVZ_SCENE_WORK_ACCESS_READ, UINT32_MAX, 0,
                DVZ_SCENE_ATTACHMENT_LOAD_NONE, DVZ_SCENE_ATTACHMENT_STORE_NONE, false) &&
            _composition_bind(
                pass, DVZ_SCENE_RESOURCE_REF_PRODUCT,
                out[DVZ_RENDER_PRODUCT_TRANSPARENT_ACCUMULATION], (DvzSceneScratchResourceId){0},
                DVZ_SCENE_WORK_BINDING_ATTACHMENT, DVZ_SCENE_WORK_ACCESS_READ_WRITE, 0, UINT32_MAX,
                UINT32_MAX, DVZ_SCENE_ATTACHMENT_LOAD_LOAD, DVZ_SCENE_ATTACHMENT_STORE_STORE,
                false, false) &&
            SCRATCH(
                DVZ_SCENE_SCRATCH_PEEL_BACK_ACCUM, DVZ_SCENE_SCRATCH_SCOPE_TECHNIQUE, COLOR_USAGE,
                DVZ_SCENE_WORK_BINDING_ATTACHMENT, DVZ_SCENE_WORK_ACCESS_READ_WRITE, 1, UINT32_MAX,
                DVZ_SCENE_ATTACHMENT_LOAD_LOAD, DVZ_SCENE_ATTACHMENT_STORE_STORE, false) &&
            SCRATCH(
                next, DVZ_SCENE_SCRATCH_SCOPE_TECHNIQUE, COLOR_USAGE,
                DVZ_SCENE_WORK_BINDING_ATTACHMENT, DVZ_SCENE_WORK_ACCESS_WRITE, 2, UINT32_MAX,
                DVZ_SCENE_ATTACHMENT_LOAD_CLEAR, DVZ_SCENE_ATTACHMENT_STORE_STORE, false);
        if (ok)
        {
            pass->bindings[0].set = DVZ_SCENE_DEPTH_PEEL_BIND_SET;
            _composition_clear_value(pass, -1, -1, 0, 0);
        }
        if (ok && render_plan->transparent_needs_depth)
        {
            ok = SCRATCH(
                DVZ_SCENE_SCRATCH_PEEL_FORWARD_DEPTH, DVZ_SCENE_SCRATCH_SCOPE_PANEL, DEPTH_USAGE,
                DVZ_SCENE_WORK_BINDING_ATTACHMENT,
                render_plan->opaque_needs_depth ? DVZ_SCENE_WORK_ACCESS_READ
                                                : DVZ_SCENE_WORK_ACCESS_WRITE,
                UINT32_MAX, UINT32_MAX,
                render_plan->opaque_needs_depth ? DVZ_SCENE_ATTACHMENT_LOAD_LOAD
                                                : DVZ_SCENE_ATTACHMENT_LOAD_CLEAR,
                DVZ_SCENE_ATTACHMENT_STORE_DONT_CARE, true);
            if (ok && !render_plan->opaque_needs_depth)
                _composition_clear_value(pass, 1, 0, 0, 0);
        }
        pass->legacy_transition = true;
        ok = ok &&
             _composition_mark_unrealized(pass, out[DVZ_RENDER_PRODUCT_TRANSPARENT_PEEL_DEPTH]);
        break;
    }
    case DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_COMPOSITE:
        ok = SAMPLE_OUT(DVZ_RENDER_PRODUCT_TRANSPARENT_ACCUMULATION, 0) &&
             SCRATCH(
                 DVZ_SCENE_SCRATCH_PEEL_BACK_ACCUM, DVZ_SCENE_SCRATCH_SCOPE_TECHNIQUE, COLOR_USAGE,
                 DVZ_SCENE_WORK_BINDING_SAMPLED, DVZ_SCENE_WORK_ACCESS_READ, UINT32_MAX, 1,
                 DVZ_SCENE_ATTACHMENT_LOAD_NONE, DVZ_SCENE_ATTACHMENT_STORE_NONE, false) &&
             ATTACH(DVZ_RENDER_PRODUCT_SCENE_COLOR, 0, DVZ_SCENE_ATTACHMENT_LOAD_LOAD);
        context->forward_depth_written = false;
        break;
    default:
        ok = false;
        break;
    }
    if (ok && pass->work_class == DVZ_SCENE_WORK_VISUAL_DRAWS &&
        in[DVZ_RENDER_PRODUCT_SCENE_OCCLUSION_DEPTH].value != 0 &&
        !_composition_pass_has_scratch(
            snapshot, pass, DVZ_SCENE_SCRATCH_SCENE_OCCLUSION_DEVICE_DEPTH))
    {
        ok = SCRATCH_PROVIDER(DVZ_SCENE_SCRATCH_SCENE_OCCLUSION_DEVICE_DEPTH);
        pass->legacy_transition = true;
        ok = _composition_mark_unrealized(pass, in[DVZ_RENDER_PRODUCT_SCENE_OCCLUSION_DEPTH]);
    }
    if (ok && pass->work_class == DVZ_SCENE_WORK_VISUAL_DRAWS &&
        in[DVZ_RENDER_PRODUCT_VOLUME_FIRST_HIT_DEPTH].value != 0 &&
        !_composition_pass_has_scratch(
            snapshot, pass, DVZ_SCENE_SCRATCH_VOLUME_OCCLUSION_DEVICE_DEPTH))
    {
        ok = SCRATCH_PROVIDER(DVZ_SCENE_SCRATCH_VOLUME_OCCLUSION_DEVICE_DEPTH);
        pass->legacy_transition = true;
        ok = _composition_mark_unrealized(pass, in[DVZ_RENDER_PRODUCT_VOLUME_FIRST_HIT_DEPTH]);
    }
#undef SAMPLE
#undef SAMPLE_OUT
#undef ATTACH
#undef SCRATCH
#undef SCRATCH_PROVIDER
#undef COLOR_USAGE
#undef DEPTH_USAGE
#undef DEPTH_SAMPLED_USAGE
    if (!ok)
        return false;
    for (uint32_t i = 0; i < pass->binding_count; i++)
    {
        DvzSceneWorkBinding* binding = &pass->bindings[i];
        if (binding->load != DVZ_SCENE_ATTACHMENT_LOAD_LOAD)
            continue;
        if (binding->ref_kind == DVZ_SCENE_RESOURCE_REF_SCRATCH)
        {
            binding->load_source_ref_kind = DVZ_SCENE_RESOURCE_REF_SCRATCH;
            binding->load_source_scratch_id = binding->scratch_id;
        }
        else if (
            binding->product_id.value == out[DVZ_RENDER_PRODUCT_SCENE_COLOR].value &&
            in[DVZ_RENDER_PRODUCT_SCENE_COLOR].value != 0)
        {
            binding->load_source_ref_kind = DVZ_SCENE_RESOURCE_REF_PRODUCT;
            binding->load_source_product_id = in[DVZ_RENDER_PRODUCT_SCENE_COLOR];
        }
        else
        {
            binding->load_source_ref_kind = DVZ_SCENE_RESOURCE_REF_PRODUCT;
            binding->load_source_product_id = binding->product_id;
        }
    }
    return pass->provider != DVZ_SCENE_WORK_PROVIDER_NONE;
}



static bool _composition_set_latest_expansion(
    DvzPanelCompositionSnapshot* snapshot, uint32_t authored_begin, uint32_t authored_end,
    uint32_t expansion_flags)
{
    ANN(snapshot);
    if (snapshot->technique_count == 0)
        return false;
    DvzSceneResolvedTechnique* technique = &snapshot->techniques[snapshot->technique_count - 1];
    technique->authored_order_begin = authored_begin;
    technique->authored_order_end = authored_end;
    technique->expansion_flags = expansion_flags;
    return true;
}



static bool _composition_expand_all(
    DvzPanelCompositionSnapshot* snapshot, const DvzPanelRenderPlan* render_plan,
    uint32_t opaque_samples)
{
    ANN(snapshot);
    ANN(render_plan);
    CompositionWorkContext context = {
        .render_plan = render_plan,
        .opaque_samples = opaque_samples,
        .effective_edl = _composition_has_technique(snapshot, DVZ_SCENE_TECHNIQUE_EDL),
    };
    for (uint32_t k = 0; k < snapshot->technique_count; k++)
    {
        const DvzSceneResolvedTechnique* technique = &snapshot->techniques[k];
        const DvzSceneTechniqueContract* contract = _composition_contract(technique->id);
        if (contract == NULL)
            return false;
        for (uint32_t i = 0; i < contract->pass_template_count; i++)
        {
            uint32_t repeat = contract->pass_templates[i].repeat;
            if (repeat == COMPOSITION_PASS_OPTIONAL_SSAO_BLUR)
                repeat = (technique->expansion_flags & COMPOSITION_EXPAND_SSAO_BLUR) != 0;
            else if (repeat == COMPOSITION_PASS_DEPTH_PEEL_ITERATIONS)
                repeat = DVZ_SCENE_DEPTH_PEEL_ITERATIONS;
            for (uint32_t j = 0; j < repeat; j++)
            {
                if (!_composition_add_pass(
                        snapshot, technique->instance_id, technique->id, contract->phase,
                        contract->pass_templates[i].role, technique->authored_order_begin,
                        technique->authored_order_end))
                    return false;
                if (!_composition_declare_work(
                        snapshot, &snapshot->passes[snapshot->pass_count - 1], technique, j,
                        &context))
                    return false;
            }
        }
    }
    return true;
}



bool _scene_panel_composition_contract_validate(
    const DvzPanelCompositionSnapshot* snapshot, DvzDiagnosticReport* report)
{
    ANN(snapshot);
    uint32_t pass_index = 0;
    uint32_t role_ordinals[DVZ_FRAME_PLAN_RENDER_PASS_PICKING + 1] = {0};
    for (uint32_t k = 0; k < snapshot->technique_count; k++)
    {
        const DvzSceneResolvedTechnique* technique = &snapshot->techniques[k];
        const DvzSceneTechniqueContract* contract = _composition_contract(technique->id);
        if (contract == NULL || technique->version != contract->version ||
            technique->phase != contract->phase ||
            technique->must_follow_phase_mask != contract->must_follow_phase_mask ||
            (technique->input_product_mask & contract->required_inputs) !=
                contract->required_inputs ||
            (technique->input_product_mask &
             ~(contract->required_inputs | contract->optional_inputs)) != 0 ||
            (technique->output_product_mask & contract->outputs) != contract->outputs ||
            (technique->output_product_mask & ~(contract->outputs | contract->optional_outputs)) !=
                0 ||
            technique->participating_layer_mask != contract->participating_layer_mask ||
            (technique->required_capability_mask & contract->required_capability_mask) !=
                contract->required_capability_mask ||
            (technique->required_capability_mask &
             ~(contract->required_capability_mask | contract->optional_capability_mask)) != 0)
            return _composition_report(
                report, "panel %s technique instance %u drifts from its immutable contract",
                snapshot->panel_id, technique->instance_id.value);

        uint32_t allowed_adaptations = 0;
        uint32_t required_adaptations = 0;
        DvzSceneTechniqueFallback expected_fallback = contract->fallback;
        const bool samples_reduced =
            snapshot->requested_sample_count > snapshot->effective_sample_count;
        if (technique->id == DVZ_SCENE_TECHNIQUE_SURFACE_CAPTURE ||
            technique->id == DVZ_SCENE_TECHNIQUE_OPAQUE_SHADING)
        {
            allowed_adaptations |= COMPOSITION_ADAPT_MSAA_REDUCED;
            if (samples_reduced)
            {
                required_adaptations |= COMPOSITION_ADAPT_MSAA_REDUCED;
                expected_fallback = DVZ_SCENE_TECHNIQUE_FALLBACK_REDUCE_SAMPLES;
            }
        }
        if (technique->id == DVZ_SCENE_TECHNIQUE_AMBIENT_COMPOSITE)
        {
            allowed_adaptations |= COMPOSITION_ADAPT_LEGACY_AMBIENT_COMPOSITE;
            required_adaptations |= COMPOSITION_ADAPT_LEGACY_AMBIENT_COMPOSITE;
        }
        if (technique->missing_capability_mask != 0)
        {
            if (contract->fallback != DVZ_SCENE_TECHNIQUE_FALLBACK_LEGACY_TRANSITION)
                return _composition_report(
                    report, "panel %s technique instance %u has no immutable capability fallback",
                    snapshot->panel_id, technique->instance_id.value);
            allowed_adaptations |= COMPOSITION_ADAPT_CAPABILITY_FALLBACK;
            required_adaptations |= COMPOSITION_ADAPT_CAPABILITY_FALLBACK;
        }
        if (technique->fallback != expected_fallback ||
            (technique->capability_adaptations & required_adaptations) != required_adaptations ||
            (technique->capability_adaptations & ~allowed_adaptations) != 0)
            return _composition_report(
                report, "panel %s technique instance %u drifts from fallback semantics",
                snapshot->panel_id, technique->instance_id.value);

        const uint32_t allowed_expansions = technique->id == DVZ_SCENE_TECHNIQUE_AMBIENT_VISIBILITY
                                                ? COMPOSITION_EXPAND_SSAO_BLUR
                                                : 0;
        if ((technique->expansion_flags & ~allowed_expansions) != 0)
            return _composition_report(
                report, "panel %s technique instance %u has undeclared expansion flags",
                snapshot->panel_id, technique->instance_id.value);
        for (uint32_t i = 0; i < contract->pass_template_count; i++)
        {
            uint32_t repeat = contract->pass_templates[i].repeat;
            if (repeat == COMPOSITION_PASS_OPTIONAL_SSAO_BLUR)
                repeat = (technique->expansion_flags & COMPOSITION_EXPAND_SSAO_BLUR) != 0;
            else if (repeat == COMPOSITION_PASS_DEPTH_PEEL_ITERATIONS)
                repeat = DVZ_SCENE_DEPTH_PEEL_ITERATIONS;
            for (uint32_t j = 0; j < repeat; j++)
            {
                if (pass_index >= snapshot->pass_count)
                    return _composition_report(
                        report, "panel %s technique instance %u is missing a declared pass",
                        snapshot->panel_id, technique->instance_id.value);
                const DvzSceneResolvedPass* pass = &snapshot->passes[pass_index++];
                uint32_t expected_ordinal = role_ordinals[contract->pass_templates[i].role]++;
                if (pass->technique_instance_id.value != technique->instance_id.value ||
                    pass->technique_id != technique->id || pass->phase != technique->phase ||
                    pass->role != contract->pass_templates[i].role ||
                    pass->id.value != pass_index || pass->ordinal != expected_ordinal ||
                    pass->authored_order_begin != technique->authored_order_begin ||
                    pass->authored_order_end != technique->authored_order_end)
                    return _composition_report(
                        report, "panel %s technique instance %u has pass-template drift",
                        snapshot->panel_id, technique->instance_id.value);
            }
        }
    }
    if (pass_index != snapshot->pass_count)
        return _composition_report(
            report, "panel %s composition contains undeclared extra passes", snapshot->panel_id);
    if (snapshot->work_declaration_fingerprint == 0 ||
        snapshot->work_declaration_fingerprint !=
            _frame_plan_composition_work_fingerprint(snapshot))
        return _composition_report(
            report, "panel %s declarative work drifts from its immutable contract",
            snapshot->panel_id);
    return true;
}



static bool
_composition_validate_visuals(const DvzPanelRenderPlan* plan, DvzDiagnosticReport* report)
{
    ANN(plan);
    uint32_t last_order = 0;
    for (uint32_t i = 0; i < plan->visual_count; i++)
    {
        const DvzPanelRenderVisualPlan* visual = &plan->visuals[i];
        if (visual->layer == DVZ_SCENE_VISUAL_LAYER_NONE)
            return _composition_report(
                report, "panel %s visual %" PRIu32 " has no semantic layer", plan->panel_id,
                visual->visual_index);
        if (visual->caps.layer != visual->layer)
            return _composition_report(
                report, "panel %s visual %" PRIu32 " has inconsistent layer facts", plan->panel_id,
                visual->visual_index);
        if (i > 0 && visual->authored_order <= last_order)
            return _composition_report(
                report, "panel %s visual authored order is not strictly increasing",
                plan->panel_id);
        if (visual->layer == DVZ_SCENE_VISUAL_LAYER_TRANSPARENT &&
            (visual->caps.phase_participation & DVZ_SCENE_VISUAL_PHASE_TRANSPARENT_SHADING) == 0)
            return _composition_report(
                report,
                "panel %s transparent visual %" PRIu32 " has incompatible phase participation",
                plan->panel_id, visual->visual_index);
        if ((visual->layer == DVZ_SCENE_VISUAL_LAYER_SURFACE_OPAQUE ||
             visual->layer == DVZ_SCENE_VISUAL_LAYER_SURFACE_MASKED) &&
            (visual->caps.phase_participation & DVZ_SCENE_VISUAL_PHASE_OPAQUE_SHADING) == 0)
            return _composition_report(
                report, "panel %s surface visual %" PRIu32 " has incompatible phase participation",
                plan->panel_id, visual->visual_index);
        if (visual->layer == DVZ_SCENE_VISUAL_LAYER_VOLUME &&
            (visual->caps.phase_participation & DVZ_SCENE_VISUAL_PHASE_VOLUME_SHADING) == 0)
            return _composition_report(
                report, "panel %s volume visual %" PRIu32 " has incompatible phase participation",
                plan->panel_id, visual->visual_index);
        if (visual->layer == DVZ_SCENE_VISUAL_LAYER_OVERLAY &&
            (visual->caps.phase_participation & DVZ_SCENE_VISUAL_PHASE_OVERLAY) == 0)
            return _composition_report(
                report, "panel %s overlay visual %" PRIu32 " has incompatible phase participation",
                plan->panel_id, visual->visual_index);
        if (visual->layer == DVZ_SCENE_VISUAL_LAYER_QUERY &&
            (visual->caps.phase_participation & DVZ_SCENE_VISUAL_PHASE_QUERY) == 0)
            return _composition_report(
                report, "panel %s query visual %" PRIu32 " has incompatible phase participation",
                plan->panel_id, visual->visual_index);
        last_order = visual->authored_order;
    }
    return true;
}



static bool _composition_resolve_product_chain(
    DvzPanelCompositionSnapshot* snapshot, const char* panel_id, DvzDiagnosticReport* report)
{
    ANN(snapshot);
    ANN(panel_id);
    DvzRenderProductId current[DVZ_RENDER_PRODUCT_PRESENTATION_COLOR + 1] = {0};
    uint32_t next_product_id = 1;
    DvzSceneTechniquePhase previous_phase = DVZ_SCENE_PHASE_NONE;
    for (uint32_t i = 0; i < snapshot->technique_count; i++)
    {
        DvzSceneResolvedTechnique* technique = &snapshot->techniques[i];
        if (technique->phase < previous_phase && technique->phase != DVZ_SCENE_PHASE_QUERY)
            return _composition_report(
                report, "panel %s technique phase cycle at technique %u", panel_id,
                (uint32_t)technique->id);
        previous_phase = technique->phase;
        for (uint32_t j = 0; j < technique->input_count; j++)
        {
            DvzRenderProductKind kind = technique->inputs[j];
            if (kind <= DVZ_RENDER_PRODUCT_NONE || kind > DVZ_RENDER_PRODUCT_PRESENTATION_COLOR)
                return _composition_report(
                    report, "panel %s technique %u has an incompatible input product", panel_id,
                    (uint32_t)technique->id);
            if (current[kind].value == 0)
            {
                bool produced_later = false;
                for (uint32_t k = i + 1; k < snapshot->technique_count; k++)
                {
                    if ((snapshot->techniques[k].output_product_mask &
                         _composition_product_bit(kind)) != 0)
                    {
                        produced_later = true;
                        break;
                    }
                }
                return _composition_report(
                    report,
                    produced_later ? "panel %s technique %u forms a product dependency cycle"
                                   : "panel %s technique %u has no producer for input product %u",
                    panel_id, (uint32_t)technique->id, (uint32_t)kind);
            }
            technique->input_ids[j] = current[kind];
        }
        for (uint32_t j = 0; j < technique->output_count; j++)
        {
            DvzRenderProductKind kind = technique->outputs[j];
            if (next_product_id == 0)
                return _composition_report(report, "panel %s product identity overflow", panel_id);
            technique->output_ids[j] = (DvzRenderProductId){.value = next_product_id++};
            current[kind] = technique->output_ids[j];
        }
    }
    return true;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve one deterministic immutable composition snapshot from panel-local semantic facts.
 *
 * @param render_plan the classified panel render facts
 * @param caps the active capability snapshot
 * @param out output snapshot, unchanged on failure
 * @param report optional diagnostic report
 * @return whether composition succeeded
 */
bool _scene_panel_composition_resolve(
    const DvzPanelRenderPlan* render_plan, const DvzCapabilitySnapshot* caps,
    DvzPanelCompositionSnapshot* out, DvzDiagnosticReport* report)
{
    ANN(render_plan);
    ANN(out);
    DvzPanelCompositionSnapshot draft = {0};
    dvz_snprintf(draft.panel_id, sizeof(draft.panel_id), "%s", render_plan->panel_id);
    draft.origin_x = render_plan->origin_x;
    draft.origin_y = render_plan->origin_y;
    draft.width = render_plan->width > 0 ? render_plan->width : 1;
    draft.height = render_plan->height > 0 ? render_plan->height : 1;
    draft.render_scale = render_plan->render_scale > 0 ? render_plan->render_scale : 1.0f;
    dvz_memcpy(
        draft.local_to_target, sizeof(draft.local_to_target), render_plan->local_to_target,
        sizeof(render_plan->local_to_target));
    if (draft.local_to_target[0] == 0.0f && draft.local_to_target[1] == 0.0f)
    {
        draft.local_to_target[0] = 1.0f;
        draft.local_to_target[1] = 1.0f;
        draft.local_to_target[2] = (float)draft.origin_x;
        draft.local_to_target[3] = (float)draft.origin_y;
    }
    if (caps != NULL)
        draft.capabilities = *caps;
    draft.available_capability_mask = _composition_available_capabilities(caps);

    if (!_composition_validate_visuals(render_plan, report))
        return false;
    if (render_plan->unsupported_noncontiguous_oit)
        return _composition_report(
            report,
            "panel %s has noncontiguous repeated OIT runs unsupported by legacy R2 lowering",
            render_plan->panel_id);

    bool msaa_reduced = false;
    draft.requested_sample_count =
        render_plan->msaa_state != NULL && render_plan->msaa_state->enabled
            ? render_plan->msaa_state->sample_count
            : 1;
    draft.effective_sample_count =
        _composition_effective_sample_count(render_plan, caps, &msaa_reduced);
    draft.alpha_to_coverage = render_plan->msaa_state != NULL &&
                              render_plan->msaa_state->alpha_to_coverage &&
                              draft.effective_sample_count > 1;
    uint32_t msaa_adaptation = msaa_reduced ? COMPOSITION_ADAPT_MSAA_REDUCED : 0;

    const uint64_t scene_color = _composition_product_bit(DVZ_RENDER_PRODUCT_SCENE_COLOR);
    const uint64_t surface_record = _composition_product_bit(DVZ_RENDER_PRODUCT_SURFACE_DEPTH) |
                                    _composition_product_bit(DVZ_RENDER_PRODUCT_SURFACE_NORMAL) |
                                    _composition_product_bit(DVZ_RENDER_PRODUCT_SURFACE_COVERAGE);
    const uint64_t ambient_visibility =
        _composition_product_bit(DVZ_RENDER_PRODUCT_AMBIENT_VISIBILITY);
    const uint64_t scene_occlusion =
        _composition_product_bit(DVZ_RENDER_PRODUCT_SCENE_OCCLUSION_DEPTH);
    const uint64_t volume_first_hit =
        _composition_product_bit(DVZ_RENDER_PRODUCT_VOLUME_FIRST_HIT_DEPTH);
    const bool effective_ssao = render_plan->ssao_enabled && render_plan->gbuffer_visual_count > 0;
    const bool effective_edl = render_plan->edl_enabled && render_plan->edl_has_depth_producer;
    if (render_plan->ssao_enabled && !effective_ssao)
        draft.disabled_optional_technique_mask |=
            1u << (uint32_t)DVZ_SCENE_TECHNIQUE_AMBIENT_VISIBILITY;
    if (render_plan->edl_enabled && !effective_edl)
        draft.disabled_optional_technique_mask |= 1u << (uint32_t)DVZ_SCENE_TECHNIQUE_EDL;
    const bool needs_surface_record = render_plan->gbuffer_enabled || effective_ssao;

    uint32_t begin = UINT32_MAX;
    uint32_t end = UINT32_MAX;
    if (render_plan->scene_occlusion_enabled)
    {
        if (render_plan->scene_occlusion_count == 0)
            return _composition_report(
                report, "panel %s scene occlusion has no product producer", render_plan->panel_id);
        if (!_composition_add_technique(
                &draft, DVZ_SCENE_TECHNIQUE_SCENE_OCCLUSION, 0, scene_occlusion, 0,
                DVZ_SCENE_TECHNIQUE_FALLBACK_NONE) ||
            !_composition_authored_bounds(
                render_plan->scene_occlusion, render_plan->scene_occlusion_count,
                DVZ_PANEL_RENDER_INVALID_INDEX, &begin, &end) ||
            !_composition_set_latest_expansion(&draft, begin, end, 0))
            return _composition_report(
                report, "panel %s scene occlusion expansion failed", render_plan->panel_id);
    }

    if (render_plan->volume_occlusion_enabled)
    {
        if (!render_plan->has_volume_occluder)
            return _composition_report(
                report, "panel %s volume occlusion has no product producer",
                render_plan->panel_id);
        if (!_composition_add_technique(
                &draft, DVZ_SCENE_TECHNIQUE_VOLUME_OCCLUSION, 0, volume_first_hit, 0,
                DVZ_SCENE_TECHNIQUE_FALLBACK_NONE) ||
            !_composition_set_latest_expansion(&draft, UINT32_MAX, UINT32_MAX, 0))
            return _composition_report(
                report, "panel %s volume occlusion expansion failed", render_plan->panel_id);
    }

    if (needs_surface_record)
    {
        if (render_plan->gbuffer_visual_count == 0)
            return _composition_report(
                report, "panel %s coherent surface record has no compatible producer",
                render_plan->panel_id);
        _composition_authored_bounds(
            render_plan->gbuffer_visuals, render_plan->gbuffer_visual_count,
            DVZ_PANEL_RENDER_INVALID_INDEX, &begin, &end);
        if (!_composition_add_technique(
                &draft, DVZ_SCENE_TECHNIQUE_SURFACE_CAPTURE, 0, surface_record, msaa_adaptation,
                DVZ_SCENE_TECHNIQUE_FALLBACK_LEGACY_TRANSITION) ||
            !_composition_set_latest_expansion(&draft, begin, end, 0))
            return _composition_report(
                report, "panel %s surface capture expansion failed", render_plan->panel_id);
    }

    if (effective_ssao)
    {
        if ((draft.required_product_mask & surface_record) != surface_record)
            return _composition_report(
                report, "panel %s ambient visibility is missing the coherent surface record",
                render_plan->panel_id);
        if (!_composition_add_technique(
                &draft, DVZ_SCENE_TECHNIQUE_AMBIENT_VISIBILITY, surface_record, ambient_visibility,
                0, DVZ_SCENE_TECHNIQUE_FALLBACK_NONE) ||
            !_composition_set_latest_expansion(
                &draft, begin, end,
                render_plan->ssao_state != NULL && render_plan->ssao_state->blur_enabled
                    ? COMPOSITION_EXPAND_SSAO_BLUR
                    : 0))
            return _composition_report(
                report, "panel %s ambient visibility expansion failed", render_plan->panel_id);
    }

    if (render_plan->opaque_visual_count > 0 || render_plan->has_transparent)
    {
        uint64_t opaque_inputs = effective_ssao ? ambient_visibility : 0;
        if (render_plan->scene_occlusion_enabled)
            opaque_inputs |= scene_occlusion;
        if (render_plan->volume_occlusion_enabled)
            opaque_inputs |= volume_first_hit;
        uint64_t opaque_outputs = scene_color;
        if (effective_edl && !needs_surface_record)
            opaque_outputs |= _composition_product_bit(DVZ_RENDER_PRODUCT_SURFACE_DEPTH);
        _composition_authored_bounds(
            render_plan->opaque_visuals, render_plan->opaque_visual_count,
            DVZ_PANEL_RENDER_INVALID_INDEX, &begin, &end);
        if (!_composition_add_technique(
                &draft, DVZ_SCENE_TECHNIQUE_OPAQUE_SHADING, opaque_inputs, opaque_outputs,
                msaa_adaptation,
                msaa_reduced ? DVZ_SCENE_TECHNIQUE_FALLBACK_REDUCE_SAMPLES
                             : DVZ_SCENE_TECHNIQUE_FALLBACK_NONE) ||
            !_composition_set_latest_expansion(&draft, begin, end, 0))
            return _composition_report(
                report, "panel %s opaque shading expansion failed", render_plan->panel_id);
    }

    if (effective_ssao)
    {
        if (!_composition_add_technique(
                &draft, DVZ_SCENE_TECHNIQUE_AMBIENT_COMPOSITE, scene_color | ambient_visibility,
                scene_color, COMPOSITION_ADAPT_LEGACY_AMBIENT_COMPOSITE,
                DVZ_SCENE_TECHNIQUE_FALLBACK_LEGACY_TRANSITION) ||
            !_composition_set_latest_expansion(&draft, begin, end, 0))
            return _composition_report(
                report, "panel %s ambient transition expansion failed", render_plan->panel_id);
    }

    if (effective_edl)
    {
        if (!_composition_add_technique(
                &draft, DVZ_SCENE_TECHNIQUE_EDL,
                scene_color | _composition_product_bit(DVZ_RENDER_PRODUCT_SURFACE_DEPTH),
                scene_color, 0, DVZ_SCENE_TECHNIQUE_FALLBACK_NONE) ||
            !_composition_set_latest_expansion(&draft, begin, end, 0))
            return _composition_report(
                report, "panel %s EDL expansion failed", render_plan->panel_id);
    }

    for (uint32_t i = 0; i < render_plan->transparent_pass_count; i++)
    {
        const DvzPanelRenderTransparentPassPlan* transparent = &render_plan->transparent_passes[i];
        if (transparent->kind == DVZ_PANEL_RENDER_TRANSPARENT_BLENDED)
        {
            _composition_authored_bounds(
                render_plan->blended_visuals, render_plan->blended_visual_count,
                transparent->index, &begin, &end);
            if (transparent->index >= render_plan->blended_group_count)
                return _composition_report(
                    report, "panel %s transparent pass references an invalid blend group",
                    render_plan->panel_id);
            uint32_t group_layers =
                _composition_blend_group_layers(render_plan, transparent->index);
            const uint32_t volume_layer = LAYER_BIT(DVZ_SCENE_VISUAL_LAYER_VOLUME);
            const uint32_t overlay_layer = LAYER_BIT(DVZ_SCENE_VISUAL_LAYER_OVERLAY);
            const uint32_t transparent_layers =
                LAYER_BIT(DVZ_SCENE_VISUAL_LAYER_TRANSPARENT) | volume_layer;
            if (group_layers == overlay_layer)
            {
                if (!_composition_add_technique(
                        &draft, DVZ_SCENE_TECHNIQUE_OVERLAY_COMPOSITE, scene_color, scene_color, 0,
                        DVZ_SCENE_TECHNIQUE_FALLBACK_NONE))
                    return _composition_report(
                        report, "panel %s overlay composition selection failed",
                        render_plan->panel_id);
                if (!_composition_validate_technique_visuals(
                        render_plan, render_plan->blended_visuals,
                        render_plan->blended_visual_count, transparent->index,
                        &draft.techniques[draft.technique_count - 1], report))
                    return false;
                if (_composition_blend_group_uses_source_over(render_plan, transparent->index))
                    draft.techniques[draft.technique_count - 1].required_capability_mask |=
                        COMPOSITION_CAP_COLOR_BLEND;
                if (!_composition_set_latest_expansion(&draft, begin, end, 0))
                    return _composition_report(
                        report, "panel %s overlay composition expansion failed",
                        render_plan->panel_id);
                continue;
            }
            if (group_layers == 0 || (group_layers & ~transparent_layers) != 0)
                return _composition_report(
                    report, "panel %s blend group has incompatible visual-layer participation",
                    render_plan->panel_id);
            uint64_t blend_inputs = scene_color;
            if (render_plan->scene_occlusion_enabled)
                blend_inputs |= scene_occlusion;
            if ((group_layers & volume_layer) != 0)
            {
                if (render_plan->volume_occlusion_enabled)
                    blend_inputs |= volume_first_hit;
            }
            if (!_composition_add_technique(
                    &draft, DVZ_SCENE_TECHNIQUE_TRANSPARENT_BLEND, blend_inputs, scene_color, 0,
                    DVZ_SCENE_TECHNIQUE_FALLBACK_NONE))
                return _composition_report(
                    report, "panel %s transparent blend selection failed", render_plan->panel_id);
            if (!_composition_validate_technique_visuals(
                    render_plan, render_plan->blended_visuals, render_plan->blended_visual_count,
                    transparent->index, &draft.techniques[draft.technique_count - 1], report))
                return false;
            if (!_composition_set_latest_expansion(&draft, begin, end, 0))
                return _composition_report(
                    report, "panel %s transparent blend expansion failed", render_plan->panel_id);
        }
        else if (transparent->kind == DVZ_PANEL_RENDER_TRANSPARENT_WBOIT)
        {
            _composition_authored_bounds(
                render_plan->wboit_visuals, render_plan->wboit_visual_count,
                DVZ_PANEL_RENDER_INVALID_INDEX, &begin, &end);
            const uint64_t wboit_products =
                _composition_product_bit(DVZ_RENDER_PRODUCT_TRANSPARENT_ACCUMULATION) |
                _composition_product_bit(DVZ_RENDER_PRODUCT_TRANSPARENT_TRANSMITTANCE);
            uint64_t wboit_inputs = scene_color;
            if (render_plan->scene_occlusion_enabled)
                wboit_inputs |= scene_occlusion;
            if (!_composition_add_technique(
                    &draft, DVZ_SCENE_TECHNIQUE_WBOIT, wboit_inputs, scene_color | wboit_products,
                    0, DVZ_SCENE_TECHNIQUE_FALLBACK_NONE))
                return _composition_report(
                    report,
                    "panel %s WBOIT requires floating-point render targets and color blending",
                    render_plan->panel_id);
            if (!_composition_validate_technique_visuals(
                    render_plan, render_plan->wboit_visuals, render_plan->wboit_visual_count,
                    DVZ_PANEL_RENDER_INVALID_INDEX, &draft.techniques[draft.technique_count - 1],
                    report))
                return false;
            if (!_composition_set_latest_expansion(&draft, begin, end, 0))
                return _composition_report(
                    report, "panel %s WBOIT expansion failed", render_plan->panel_id);
        }
        else
        {
            _composition_authored_bounds(
                render_plan->depth_peel_visuals, render_plan->depth_peel_visual_count,
                DVZ_PANEL_RENDER_INVALID_INDEX, &begin, &end);
            const uint64_t peel_products =
                _composition_product_bit(DVZ_RENDER_PRODUCT_TRANSPARENT_ACCUMULATION) |
                _composition_product_bit(DVZ_RENDER_PRODUCT_TRANSPARENT_PEEL_DEPTH);
            uint64_t peel_inputs = scene_color;
            if (render_plan->scene_occlusion_enabled)
                peel_inputs |= scene_occlusion;
            if (!_composition_add_technique(
                    &draft, DVZ_SCENE_TECHNIQUE_DEPTH_PEEL, peel_inputs,
                    scene_color | peel_products, 0, DVZ_SCENE_TECHNIQUE_FALLBACK_NONE))
                return _composition_report(
                    report, "panel %s depth-peel selection failed", render_plan->panel_id);
            if (!_composition_validate_technique_visuals(
                    render_plan, render_plan->depth_peel_visuals,
                    render_plan->depth_peel_visual_count, DVZ_PANEL_RENDER_INVALID_INDEX,
                    &draft.techniques[draft.technique_count - 1], report))
                return false;
            if (!_composition_set_latest_expansion(&draft, begin, end, 0))
                return _composition_report(
                    report, "panel %s depth-peel expansion failed", render_plan->panel_id);
        }
    }

    if (render_plan->drawable_count > 0 &&
        !_composition_add_technique(
            &draft, DVZ_SCENE_TECHNIQUE_PRESENTATION, scene_color,
            _composition_product_bit(DVZ_RENDER_PRODUCT_PRESENTATION_COLOR), 0,
            DVZ_SCENE_TECHNIQUE_FALLBACK_NONE))
        return _composition_report(
            report, "panel %s presentation selection failed", render_plan->panel_id);

    bool blended_depth = false;
    for (uint32_t i = 0; i < render_plan->blended_group_count; i++)
        blended_depth = blended_depth || render_plan->blended_needs_depth[i];
    const uint32_t opaque_samples =
        draft.effective_sample_count > 1 && render_plan->wboit_visual_count == 0 &&
                render_plan->depth_peel_visual_count == 0 && !effective_edl &&
                (render_plan->blended_group_count == 0 || !blended_depth)
            ? draft.effective_sample_count
            : 1;
    if (!_composition_validate_capabilities(&draft, render_plan->panel_id, report) ||
        !_composition_resolve_product_chain(&draft, render_plan->panel_id, report) ||
        !_composition_expand_all(&draft, render_plan, opaque_samples))
        return false;
    draft.work_declaration_fingerprint = _frame_plan_composition_work_fingerprint(&draft);
    if (!_scene_panel_composition_contract_validate(&draft, report))
        return false;
    draft.valid = true;
    if (!_frame_plan_composition_validate(&draft, report))
        return false;
    *out = draft;
    return true;
}
