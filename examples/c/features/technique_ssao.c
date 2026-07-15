/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* This example compares a porous sphere aggregate with and without screen-space ambient occlusion.
 *
 * What to look for: both panels render the same uniformly colored, close-packed sphere aggregate,
 * while the right panel applies SSAO. Contact seams, pores, and the open central cavity become
 * legible without a floor or color mapping. In live mode, use the GUI and linked arcball to inspect
 * how radius, strength, bias, power, visibility, sample count, and blur affect local occlusion.
 *
 * Scenario: features_technique_ssao
 * Style: features, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c features/technique_ssao
 * Run:    ./build/examples/c/features/technique_ssao --live
 * Smoke:  ./build/examples/c/features/technique_ssao --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "_alloc.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_controller_preview.h"
#include "example_style.h"
#include "example_tuner.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT
#define AGGREGATE_SIDE         9u
#define AGGREGATE_MAX_SPHERES  (AGGREGATE_SIDE * AGGREGATE_SIDE * AGGREGATE_SIDE)



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

typedef struct SsaoDemoState
{
    DvzPanel* ssao_panel;
    DvzArcball* plain_arcball;
    DvzArcball* ssao_arcball;
    DvzExampleGuiSsaoControls ssao;
    vec3 arcball_angles;
    vec2 arcball_pan;
    float arcball_zoom;
    ExampleTuner tuner;
} SsaoDemoState;



static void _apply_arcball(SsaoDemoState* state)
{
    if (state == NULL)
        return;

    if (state->plain_arcball != NULL)
    {
        dvz_arcball_set(state->plain_arcball, state->arcball_angles);
        dvz_arcball_zoom(state->plain_arcball, state->arcball_zoom);
        dvz_arcball_pan(state->plain_arcball, state->arcball_pan);
    }
    if (state->ssao_arcball != NULL)
    {
        dvz_arcball_set(state->ssao_arcball, state->arcball_angles);
        dvz_arcball_zoom(state->ssao_arcball, state->arcball_zoom);
        dvz_arcball_pan(state->ssao_arcball, state->arcball_pan);
    }
}



/**
 * Add a rounded close-packed sphere aggregate with pores and an open central cavity.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @return true on success
 */
static bool _add_sphere_cluster(DvzScene* scene, DvzPanel* panel)
{
    vec3 positions[AGGREGATE_MAX_SPHERES] = {{0}};
    float radii[AGGREGATE_MAX_SPHERES] = {0};
    DvzColor colors[AGGREGATE_MAX_SPHERES] = {{0}};
    uint32_t count = 0;
    const int32_t center = (int32_t)(AGGREGATE_SIDE / 2u);

    for (uint32_t z = 0; z < AGGREGATE_SIDE; z++)
    {
        for (uint32_t y = 0; y < AGGREGATE_SIDE; y++)
        {
            for (uint32_t x = 0; x < AGGREGATE_SIDE; x++)
            {
                const int32_t ix = (int32_t)x - center;
                const int32_t iy = (int32_t)y - center;
                const int32_t iz = (int32_t)z - center;
                const float stagger = ((x + y + z) & 1u) != 0u ? 0.0817f : 0.0f;
                const float px = 0.1633f * (float)ix + stagger;
                const float py = 0.1462f * (float)iy;
                const float pz = 0.1385f * (float)iz;
                const float distance2 = px * px + py * py + pz * pz;
                const bool outside = distance2 > 0.47f;
                const bool cavity = pz > -0.08f && px * px + py * py < 0.045f;
                const bool pore = (7u * x + 11u * y + 13u * z) % 23u == 0u;
                if (outside || cavity || pore)
                    continue;

                positions[count][0] = px;
                positions[count][1] = py;
                positions[count][2] = pz;
                radii[count] = 0.084f;
                colors[count] =
                    example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY);
                count++;
            }
        }
    }
    if (count == 0)
        return false;

    DvzVisual* spheres = dvz_sphere(scene, DVZ_SPHERE_FLAGS_LIGHTING);
    if (spheres == NULL)
        return false;
    if (dvz_sphere_set_mode(spheres, DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR) != 0)
        return false;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = count},
        {.attr_name = "radius", .data = radii, .item_count = count},
        {.attr_name = "color", .data = colors, .item_count = count},
    };
    if (dvz_visual_set_data_many(spheres, updates, 3) != 0)
        return false;

    DvzMaterialDesc material = dvz_standard_material_desc();
    material.light_direction[0] = -0.38f;
    material.light_direction[1] = +0.52f;
    material.light_direction[2] = +0.76f;
    material.standard.roughness = 0.72f;
    material.standard.specular = 0.22f;
    material.standard.rim_strength = 0.10f;
    if (dvz_visual_set_material(spheres, &material) != 0)
        return false;
    return dvz_panel_add_visual(panel, spheres, NULL) == 0;
}



static DvzController* _bind_arcball(DvzScenarioContext* ctx, DvzPanel* panel, vec3 angles)
{
    DvzController* controller = dvz_arcball(ctx->scene, NULL);
    if (controller == NULL)
        return NULL;
    DvzArcball* arcball = dvz_controller_arcball(controller);
    if (arcball == NULL)
        return NULL;
    if (dvz_scenario_bind_controller(ctx, panel, controller, DVZ_DIM_MASK_XYZ) != 0)
        return NULL;
    dvz_arcball_set(arcball, angles);
    return controller;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the SSAO feature scenario.
 *
 * @param ctx scenario context
 * @param out_user scenario state output
 * @return true on success
 */
static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL)
        return false;
    if (out_user != NULL)
        *out_user = NULL;

    SsaoDemoState* state = (SsaoDemoState*)dvz_calloc(1, sizeof(*state));
    if (state == NULL)
        return false;
    if (out_user != NULL)
        *out_user = state;
    state->tuner = example_tuner("SSAO calibration");

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        return false;
    example_tuner_figure(&state->tuner, ctx->figure);

    DvzGrid* grid = dvz_figure_grid(ctx->figure, 1, 2);
    if (grid == NULL)
        return false;
    if (dvz_grid_set_margins(
            grid, &(DvzPanelReserve){
                      .left_px = 42.0f, .right_px = 42.0f, .top_px = 38.0f, .bottom_px = 38.0f}) != DVZ_OK)
        return false;
    if (dvz_grid_set_gutter(grid, 30.0f, 0.0f) != DVZ_OK)
        return false;

    DvzPanel* plain = dvz_grid_panel(grid, 0, 0);
    DvzPanel* ssao_panel = dvz_grid_panel(grid, 0, 1);
    if (plain == NULL || ssao_panel == NULL)
        return false;
    state->ssao_panel = ssao_panel;
    example_graphite_cyan_set_panel_background(plain);
    example_graphite_cyan_set_panel_background(ssao_panel);

    DvzTextStyle label_style = example_graphite_cyan_text_style(EXAMPLE_STYLE_TEXT_PANEL_LABEL);
    label_style.renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS;
    label_style.size_px = EXAMPLE_PANEL_LABEL_LARGE_SIZE;
    DvzTextPlacement label_placement = dvz_text_placement();
    label_placement.mode = DVZ_TEXT_PLACEMENT_SCREEN;
    label_placement.anchor = DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT;
    label_placement.position[0] = EXAMPLE_PANEL_LABEL_LARGE_X_PX;
    label_placement.position[1] = EXAMPLE_PANEL_LABEL_LARGE_Y_PX;
    label_placement.text_anchor[0] = 0.0f;
    label_placement.text_anchor[1] = 0.0f;
    label_placement.has_text_anchor = true;
    DvzLabelDesc label = dvz_label_desc();
    label.text = "plain lighting";
    DvzAnnotation* annotation = dvz_annotation_label(plain, &label);
    if (annotation == NULL || dvz_annotation_set_style(annotation, &label_style) != 0 ||
        dvz_annotation_set_placement(annotation, &label_placement) != 0)
        return false;
    label.text = "ambient occlusion";
    annotation = dvz_annotation_label(ssao_panel, &label);
    if (annotation == NULL || dvz_annotation_set_style(annotation, &label_style) != 0 ||
        dvz_annotation_set_placement(annotation, &label_placement) != 0)
        return false;
    if (example_set_default_3d_camera(plain, 1.0f) == NULL ||
        example_set_default_3d_camera(ssao_panel, 1.0f) == NULL)
        return false;
    if (!_add_sphere_cluster(ctx->scene, plain) || !_add_sphere_cluster(ctx->scene, ssao_panel))
        return false;

    state->arcball_angles[0] = +0.180f;
    state->arcball_angles[1] = -0.120f;
    state->arcball_angles[2] = +0.000f;
    state->arcball_zoom = 0.82f;
    state->arcball_pan[0] = +0.000f;
    state->arcball_pan[1] = -0.020f;
    DvzController* plain_controller = _bind_arcball(ctx, plain, state->arcball_angles);
    DvzController* ssao_controller = _bind_arcball(ctx, ssao_panel, state->arcball_angles);
    if (plain_controller == NULL || ssao_controller == NULL)
        return false;
    state->plain_arcball = dvz_controller_arcball(plain_controller);
    state->ssao_arcball = dvz_controller_arcball(ssao_controller);
    if (state->plain_arcball == NULL || state->ssao_arcball == NULL)
        return false;
    if (dvz_controller_link(
            ctx->scene, plain_controller, ssao_controller,
            DVZ_CONTROLLER_LINK_ROTATION | DVZ_CONTROLLER_LINK_PAN | DVZ_CONTROLLER_LINK_ZOOM,
            DVZ_CONTROLLER_LINK_TWO_WAY) == NULL)
        return false;
    _apply_arcball(state);

    state->ssao = (DvzExampleGuiSsaoControls){
        .enabled = true,
        .blur = true,
        .debug_view = false,
        .show_blur_sigmas = true,
        .show_debug_view = true,
        .radius = 0.28f,
        .strength = 4.0f,
        .bias = 0.000f,
        .power = 1.25f,
        .min_visibility = 0.30f,
        .samples = 32.0f,
        .min_samples = 4.0f,
        .max_samples = 32.0f,
        .blur_radius = 3.0f,
        .blur_radius_max = 16.0f,
        .blur_depth_sigma = 0.65f,
        .blur_normal_sigma = 0.35f,
    };
    example_tuner_ssao(&state->tuner, "Occlusion", state->ssao_panel, &state->ssao);
    example_tuner_arcball(
        &state->tuner, "Arcball", state->plain_arcball, state->arcball_angles,
        state->arcball_zoom, state->arcball_pan);
    return true;
}


static bool _scenario_native_view(DvzScenarioContext* ctx, DvzApp* app, DvzView* view, void* user)
{
    (void)app;
    SsaoDemoState* state = (SsaoDemoState*)user;
    if (
        ctx == NULL || ctx->presentation != DVZ_RUNNER_PRESENT_GLFW || state == NULL ||
        view == NULL)
        return true;

    return example_tuner_attach(&state->tuner, view);
}



static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    SsaoDemoState* state = (SsaoDemoState*)user;
    if (state != NULL)
        example_tuner_detach(&state->tuner);
    dvz_free(state);
}


static void _scenario_frame(DvzScenarioContext* ctx, void* user)
{
    SsaoDemoState* state = (SsaoDemoState*)user;
    if (ctx == NULL || !ctx->preview_mode || state == NULL)
        return;
    ExamplePreviewArcballDesc desc = example_preview_arcball_cube_desc();
    example_preview_arcball(
        state->plain_arcball, ctx->preview_frame_index, ctx->preview_frame_count, &desc);
    example_preview_arcball(
        state->ssao_arcball, ctx->preview_frame_index, ctx->preview_frame_count, &desc);
}



/**
 * Return the SSAO scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _ssao_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "features_technique_ssao",
        .title = "Screen-Space Ambient Occlusion",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .init = _scenario_init,
        .frame = _scenario_frame,
        .destroy = _scenario_destroy,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the SSAO feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _ssao_scenario();
    if (example_cli_wants_live_gui(argc, argv))
        spec.native_view = _scenario_native_view;
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
