/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* This example compares a plain 3D sphere lattice with depth-dependent fading.
 *
 * What to look for: both panels upload the same uniformly colored 5x5x5 sphere lattice, but only
 * the right panel applies a depth-cue descriptor to the visual. The uniform material makes the
 * progressive fade toward the background attributable to depth rather than a color mapping. In
 * live mode, use the GUI to change cue mode, depth metric, falloff, near/far depth, strength,
 * density, and background color while the linked arcball keeps both views aligned.
 *
 * Scenario: features_technique_depth_cue
 * Style: features, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c features/technique_depth_cue
 * Run:    ./build/examples/c/features/technique_depth_cue --live
 * Smoke:  ./build/examples/c/features/technique_depth_cue --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

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
#define LATTICE_SIDE  5u
#define SPHERE_COUNT  (LATTICE_SIDE * LATTICE_SIDE * LATTICE_SIDE)
#define CUE_DISTANCE_MIN 0.05f
#define CUE_DISTANCE_MAX 6.00f
#define CUE_DISTANCE_EPS 0.01f



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DepthCueDemoState
{
    DvzVisual* cued_sphere;
    bool enabled;
    DvzDepthCueDesc cue;
    DvzArcball* arcball;
    vec3 arcball_angles;
    float arcball_zoom;
    ExampleTuner tuner;
} DepthCueDemoState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static void _reset_depth_cue_controls(DepthCueDemoState* state)
{
    if (state == NULL)
        return;

    state->enabled = true;
    state->cue = dvz_depth_cue_desc();
    state->cue.mode = DVZ_DEPTH_CUE_FADE_TO_BACKGROUND;
    state->cue.metric = DVZ_DEPTH_CUE_METRIC_EYE_DISTANCE;
    state->cue.falloff = DVZ_DEPTH_CUE_FALLOFF_LINEAR;
    state->cue.near_depth = 4.055000f;
    state->cue.far_depth = 5.605000f;
    state->cue.strength = 0.959000f;
    state->cue.density = 0.774000f;
    state->cue.background_color[0] = 0.035f;
    state->cue.background_color[1] = 0.047f;
    state->cue.background_color[2] = 0.067f;
    state->cue.background_color[3] = 1.0f;
}



static DvzDepthCueDesc _depth_cue_desc_from_controls(const DepthCueDemoState* state)
{
    DvzDepthCueDesc cue = dvz_depth_cue_desc();
    if (state == NULL)
        return cue;
    return state->cue;
}



static void _apply_depth_cue(DepthCueDemoState* state)
{
    if (state == NULL || state->cued_sphere == NULL)
        return;

    if (!state->enabled)
    {
        (void)dvz_visual_set_depth_cue(state->cued_sphere, NULL);
        return;
    }

    if (state->cue.near_depth < CUE_DISTANCE_MIN)
        state->cue.near_depth = CUE_DISTANCE_MIN;
    if (state->cue.near_depth > CUE_DISTANCE_MAX - CUE_DISTANCE_EPS)
        state->cue.near_depth = CUE_DISTANCE_MAX - CUE_DISTANCE_EPS;
    if (state->cue.far_depth > CUE_DISTANCE_MAX)
        state->cue.far_depth = CUE_DISTANCE_MAX;
    if (state->cue.far_depth <= state->cue.near_depth + CUE_DISTANCE_EPS)
        state->cue.far_depth = state->cue.near_depth + CUE_DISTANCE_EPS;

    DvzDepthCueDesc cue = _depth_cue_desc_from_controls(state);
    if (dvz_visual_set_depth_cue(state->cued_sphere, &cue) != 0)
        dvz_fprintf(stderr, "technique_depth_cue: dvz_visual_set_depth_cue() failed\n");
}



/**
 * Add a regular 3D sphere lattice with optional depth cueing.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param cue optional depth-cue descriptor
 * @param out_sphere optional output visual pointer
 * @return true on success
 */
static bool _add_sphere_lattice(
    DvzScene* scene, DvzPanel* panel, const DvzDepthCueDesc* cue, DvzVisual** out_sphere)
{
    vec3 positions[SPHERE_COUNT] = {{0}};
    DvzColor colors[SPHERE_COUNT] = {{0}};
    float radii[SPHERE_COUNT] = {0};
    const float denominator = (float)(LATTICE_SIDE - 1u);

    for (uint32_t z = 0; z < LATTICE_SIDE; z++)
    {
        for (uint32_t y = 0; y < LATTICE_SIDE; y++)
        {
            for (uint32_t x = 0; x < LATTICE_SIDE; x++)
            {
                const uint32_t i = z * LATTICE_SIDE * LATTICE_SIDE + y * LATTICE_SIDE + x;
                positions[i][0] = -0.64f + 1.28f * (float)x / denominator;
                positions[i][1] = -0.52f + 1.04f * (float)y / denominator;
                positions[i][2] = -0.78f + 1.56f * (float)z / denominator;
                colors[i] =
                    example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY);
                radii[i] = 0.070f;
            }
        }
    }

    DvzVisual* sphere = dvz_sphere(scene, DVZ_SPHERE_FLAGS_LIGHTING);
    if (sphere == NULL)
        return false;
    if (dvz_sphere_set_mode(sphere, DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR) != 0)
        return false;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = SPHERE_COUNT},
        {.attr_name = "color", .data = colors, .item_count = SPHERE_COUNT},
        {.attr_name = "radius", .data = radii, .item_count = SPHERE_COUNT},
    };
    if (dvz_visual_set_data_many(sphere, updates, 3) != 0)
        return false;

    DvzMaterialDesc material = dvz_standard_material_desc();
    material.standard.roughness = 0.46f;
    material.standard.specular = 0.44f;
    material.standard.rim_strength = 0.18f;
    if (dvz_visual_set_material(sphere, &material) != 0)
        return false;
    if (cue != NULL && dvz_visual_set_depth_cue(sphere, cue) != 0)
        return false;
    if (dvz_panel_add_visual(panel, sphere, NULL) != 0)
        return false;
    if (out_sphere != NULL)
        *out_sphere = sphere;
    return true;
}



static DvzController* _bind_arcball(
    DvzScenarioContext* ctx, DvzPanel* panel, vec3 angles, float zoom, vec2 pan)
{
    DvzController* controller = dvz_arcball(ctx->scene, NULL);
    if (controller == NULL)
        return NULL;
    DvzArcball* arcball = dvz_controller_arcball(controller);
    if (arcball == NULL)
        return NULL;
    if (dvz_scenario_bind_controller(ctx, panel, controller, DVZ_DIM_MASK_XYZ) != 0)
        return NULL;
    if (
        dvz_arcball_initial(arcball, angles) != DVZ_OK ||
        dvz_arcball_zoom(arcball, zoom) != DVZ_OK || dvz_arcball_pan(arcball, pan) != DVZ_OK)
        return NULL;
    return controller;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the depth-cue feature scenario.
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

    DepthCueDemoState* state = (DepthCueDemoState*)dvz_calloc(1, sizeof(*state));
    if (state == NULL)
        return false;
    _reset_depth_cue_controls(state);
    if (out_user != NULL)
        *out_user = state;
    state->tuner = example_tuner("Depth cue settings");

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
    DvzPanel* cued = dvz_grid_panel(grid, 0, 1);
    if (plain == NULL || cued == NULL)
        return false;
    example_graphite_cyan_set_panel_background(plain);
    example_graphite_cyan_set_panel_background(cued);
    const float light_direction[3] = {-0.32f, +0.55f, +0.76f};
    if (!example_panel_directional_light(ctx->scene, plain, light_direction) ||
        !example_panel_directional_light(ctx->scene, cued, light_direction))
        return false;

    DvzTextStyle label_style = example_graphite_cyan_text_style(EXAMPLE_STYLE_TEXT_PANEL_LABEL);
    label_style.renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS;
    DvzTextPlacement label_placement = dvz_text_placement();
    label_placement.mode = DVZ_TEXT_PLACEMENT_SCREEN;
    label_placement.anchor = DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT;
    label_placement.position[0] = EXAMPLE_PANEL_LABEL_X_PX;
    label_placement.position[1] = EXAMPLE_PANEL_LABEL_Y_PX;
    label_placement.text_anchor[0] = 0.0f;
    label_placement.text_anchor[1] = 0.0f;
    label_placement.has_text_anchor = true;
    DvzLabelDesc label = dvz_label_desc();
    label.text = "plain depth";
    DvzAnnotation* annotation = dvz_annotation_label(plain, &label);
    if (annotation == NULL || dvz_annotation_set_style(annotation, &label_style) != 0 ||
        dvz_annotation_set_placement(annotation, &label_placement) != 0)
        return false;
    label.text = "depth cue";
    annotation = dvz_annotation_label(cued, &label);
    if (annotation == NULL || dvz_annotation_set_style(annotation, &label_style) != 0 ||
        dvz_annotation_set_placement(annotation, &label_placement) != 0)
        return false;

    if (example_set_default_3d_camera(plain, 1.0f) == NULL ||
        example_set_default_3d_camera(cued, 1.0f) == NULL)
        return false;
    state->arcball_angles[0] = +1.565489f;
    state->arcball_angles[1] = -0.429644f;
    state->arcball_angles[2] = +0.391422f;
    state->arcball_zoom = 0.403531f;
    vec2 arcball_pan = {+0.000000f, +0.000000f};
    DvzController* plain_controller =
        _bind_arcball(ctx, plain, state->arcball_angles, state->arcball_zoom, arcball_pan);
    DvzController* cued_controller =
        _bind_arcball(ctx, cued, state->arcball_angles, state->arcball_zoom, arcball_pan);
    if (plain_controller == NULL || cued_controller == NULL)
        return false;
    if (dvz_controller_link(
            ctx->scene, plain_controller, cued_controller,
            DVZ_CONTROLLER_LINK_ROTATION | DVZ_CONTROLLER_LINK_PAN | DVZ_CONTROLLER_LINK_ZOOM,
            DVZ_CONTROLLER_LINK_TWO_WAY) == NULL)
        return false;
    state->arcball = dvz_controller_arcball(plain_controller);
    if (state->arcball == NULL)
        return false;

    DvzDepthCueDesc cue = _depth_cue_desc_from_controls(state);
    if (!_add_sphere_lattice(ctx->scene, plain, NULL, NULL) ||
        !_add_sphere_lattice(ctx->scene, cued, &cue, &state->cued_sphere))
        return false;

    example_tuner_arcball(
        &state->tuner, "Arcball", state->arcball, state->arcball_angles, state->arcball_zoom,
        arcball_pan);
    example_tuner_depth_cue(
        &state->tuner, "Depth cue", state->cued_sphere, &state->enabled, &state->cue);
    return true;
}



static bool
_scenario_native_view(DvzScenarioContext* ctx, DvzApp* app, DvzView* view, void* user)
{
    (void)app;
    DepthCueDemoState* state = (DepthCueDemoState*)user;
    if (
        ctx == NULL || ctx->presentation != DVZ_RUNNER_PRESENT_GLFW || state == NULL ||
        view == NULL)
        return true;

    return example_tuner_attach(&state->tuner, view);
}



static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    DepthCueDemoState* state = (DepthCueDemoState*)user;
    if (state != NULL)
        example_tuner_detach(&state->tuner);
    dvz_free(state);
}


static void _scenario_frame(DvzScenarioContext* ctx, void* user)
{
    DepthCueDemoState* state = (DepthCueDemoState*)user;
    if (ctx == NULL || !ctx->preview_mode || state == NULL)
        return;
    ExamplePreviewArcballDesc desc = example_preview_arcball_cube_desc();
    desc.base_angles[0] = state->arcball_angles[0];
    desc.base_angles[1] = state->arcball_angles[1] - desc.amplitude[1];
    desc.base_angles[2] = state->arcball_angles[2] - desc.amplitude[2] * sinf(0.72f);
    desc.zoom = state->arcball_zoom;
    example_preview_arcball(
        state->arcball, ctx->preview_frame_index, ctx->preview_frame_count, &desc);
}



/**
 * Return the depth-cue scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _depth_cue_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "features_technique_depth_cue",
        .title = "Depth Cue",
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
 * Run the depth-cue feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _depth_cue_scenario();
    if (example_cli_wants_live_gui(argc, argv))
        spec.native_view = _scenario_native_view;
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
