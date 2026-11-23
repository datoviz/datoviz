/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* Portable helper support for C scenarios. */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "runner/scenario_runner.h"
#include "runner/scenario_profile.h"

#include "datoviz/scene.h"

#include <math.h>



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Report the generated profile identity for host/runtime consistency checks.
 * @return Shared presentation configuration identity.
 */
const char* scenario_presentation_version(void)
{
    return EXAMPLE_PRESENTATION_VERSION;
}



double dvz_scenario_preview_fps(const DvzScenarioContext* ctx)
{
    if (ctx == NULL || ctx->preview_fps <= 0.0)
        return 60.0;
    return ctx->preview_fps;
}



double dvz_scenario_preview_time_scale(const DvzScenarioContext* ctx)
{
    if (ctx == NULL || ctx->preview_time_scale <= 0.0)
        return 1.0;
    return ctx->preview_time_scale;
}



double dvz_scenario_preview_time(const DvzScenarioContext* ctx)
{
    if (ctx == NULL)
        return 0.0;
    return (double)ctx->preview_frame_index * dvz_scenario_preview_time_scale(ctx) /
           dvz_scenario_preview_fps(ctx);
}



double dvz_scenario_preview_dt(const DvzScenarioContext* ctx)
{
    return dvz_scenario_preview_time_scale(ctx) / dvz_scenario_preview_fps(ctx);
}



/**
 * Report whether an explicit scenario presentation override is active.
 *
 * @param ctx scenario context
 * @return whether the preview requests static presentation or sway
 */
bool scenario_preview_presentation_active(const DvzScenarioContext* ctx)
{
    return ctx != NULL &&
           ctx->preview_presentation != DVZ_SCENARIO_PRESENTATION_DEFAULT;
}



/**
 * Apply shared defaults or a validated host override before scenario initialization.
 * @param ctx Scenario context.
 * @param id Scenario identifier.
 * @param kind Presentation kind, or -1 to use the manifest default.
 * @param amplitude_deg Sway amplitude in degrees.
 * @param period Presentation period in seconds.
 * @return Zero on success, or -1 for an invalid configuration.
 */
int scenario_presentation_configure(
    DvzScenarioContext* ctx, const char* id, int kind, double amplitude_deg, double period)
{
    ScenarioPresentation profile = kind == -1 ? scenario_presentation_default(id) :
                                              (ScenarioPresentation){kind, amplitude_deg, period};
    if (ctx == NULL || !scenario_presentation_valid(profile))
        return -1;
    ctx->preview_presentation = (DvzScenarioPresentationKind)profile.kind;
    ctx->preview_presentation_amplitude_deg = profile.amplitude_deg;
    ctx->preview_presentation_period = profile.period_seconds;
    return 0;
}



/**
 * Evaluate bounded presentation sway using output time instead of scientific time.
 *
 * @param ctx scenario context
 * @return presentation angle in radians
 */
double scenario_preview_presentation_angle(const DvzScenarioContext* ctx)
{
    if (!scenario_preview_presentation_active(ctx) ||
        ctx->preview_presentation != DVZ_SCENARIO_PRESENTATION_SWAY ||
        ctx->preview_presentation_period <= 0.0)
        return 0.0;

    const uint64_t stride = ctx->preview_sample_stride > 0 ? ctx->preview_sample_stride : 1;
    const uint64_t output_frame = ctx->preview_frame_index / stride;
    const double elapsed = ctx->preview_mode ?
        (double)output_frame / dvz_scenario_preview_fps(ctx) : ctx->time;
    return scenario_presentation_angle(
        (ScenarioPresentation){ctx->preview_presentation,
                               ctx->preview_presentation_amplitude_deg,
                               ctx->preview_presentation_period}, elapsed);
}



double dvz_scenario_preview_phase(
    const DvzScenarioContext* ctx, DvzScenarioPreviewPhasePolicy policy)
{
    if (ctx == NULL)
        return 0.0;
    const uint64_t stride = ctx->preview_sample_stride > 0 ? ctx->preview_sample_stride : 1;
    const uint64_t count = ctx->preview_frame_count > 0 ? ctx->preview_frame_count : 1;
    const uint64_t index = ctx->preview_frame_index / stride;
    if (policy == DVZ_SCENARIO_PREVIEW_PHASE_ENDPOINT)
        return count > 1 ? (double)(index % count) / (double)(count - 1) : 0.0;
    return (double)(index % count) / (double)count;
}



double dvz_scenario_preview_cycles(
    const DvzScenarioContext* ctx, double cycles, DvzScenarioPreviewPhasePolicy policy)
{
    return dvz_scenario_preview_phase(ctx, policy) * cycles;
}



int dvz_scenario_register_visual(DvzScenarioContext* ctx, const char* name, DvzVisual* visual)
{
    if (ctx == NULL || visual == NULL)
        return -1;
    if (ctx->visual_target_count >= DVZ_SCENARIO_MAX_VISUAL_TARGETS)
        return -1;

    DvzScenarioVisualTarget* target = &ctx->visual_targets[ctx->visual_target_count++];
    target->name = name;
    target->visual = visual;
    return 0;
}



int dvz_scenario_set_primary_visual(DvzScenarioContext* ctx, DvzVisual* visual)
{
    if (ctx == NULL || visual == NULL)
        return -1;
    ctx->primary_visual = visual;
    for (uint32_t i = 0; i < ctx->visual_target_count; i++)
    {
        DvzScenarioVisualTarget* target = &ctx->visual_targets[i];
        if (target->visual == visual)
        {
            if (target->name == NULL)
                target->name = "primary";
            return 0;
        }
    }
    return dvz_scenario_register_visual(ctx, "primary", visual);
}



int dvz_scenario_bind_controller(
    DvzScenarioContext* ctx, DvzPanel* panel, DvzController* controller, DvzDimMask dims)
{
    if (ctx == NULL || panel == NULL || controller == NULL)
        return -1;

    for (uint32_t i = 0; i < ctx->controller_binding_count; i++)
    {
        DvzScenarioControllerBinding* binding = &ctx->controller_bindings[i];
        if (binding->panel == panel && binding->controller == controller && binding->dims == dims)
            return dvz_panel_bind_controller(panel, controller, dims);
    }

    if (ctx->controller_binding_count >= DVZ_SCENARIO_MAX_CONTROLLER_BINDINGS)
        return -1;
    if (dvz_panel_bind_controller(panel, controller, dims) != 0)
        return -1;

    DvzScenarioControllerBinding* binding =
        &ctx->controller_bindings[ctx->controller_binding_count++];
    binding->panel = panel;
    binding->controller = controller;
    binding->dims = dims;
    return 0;
}



DvzPanzoom* dvz_scenario_panzoom(
    DvzScenarioContext* ctx, DvzPanel* panel, const DvzPanzoomDesc* desc, DvzDimMask dims)
{
    if (ctx == NULL || ctx->scene == NULL || panel == NULL)
        return NULL;

    DvzController* controller = dvz_panzoom(ctx->scene, desc);
    DvzPanzoom* panzoom = dvz_controller_panzoom(controller);
    if (panzoom == NULL)
        return NULL;
    if (dvz_scenario_bind_controller(ctx, panel, controller, dims) != 0)
        return NULL;
    return panzoom;
}
