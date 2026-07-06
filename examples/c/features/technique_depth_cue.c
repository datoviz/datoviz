/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* depth_cue compares a plain 3D sphere lattice with depth-dependent fading.
 *
 * What to look for: both panels upload the same 3x3x3 sphere position, color, and radius arrays,
 * but only the right panel applies a depth-cue descriptor to the visual. In live mode, use the GUI
 * to change cue mode, depth metric, falloff, near/far depth, strength, density, and background
 * color while the linked arcball keeps both views aligned. Depth cueing helps dense 3D plots read
 * as depth instead of a flat pile of symbols.
 *
 * Scenario: feature.depth_cue
 * Style: features, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c features/technique_depth_cue
 * Run:    ./build/examples/c/features/technique_depth_cue --live
 * Smoke:  ./build/examples/c/features/technique_depth_cue --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "_alloc.h"
#include "datoviz/gui.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT
#define LATTICE_SIDE  3u
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
    int mode;
    int metric;
    int falloff;
    float near_depth;
    float far_depth;
    float strength;
    float density;
    float background_color[4];
} DepthCueDemoState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static void _reset_depth_cue_controls(DepthCueDemoState* state)
{
    if (state == NULL)
        return;

    state->enabled = true;
    state->mode = 0;
    state->metric = 1;
    state->falloff = 0;
    state->near_depth = 3.80f;
    state->far_depth = 5.80f;
    state->strength = 1.0f;
    state->density = 0.45f;
    state->background_color[0] = 0.035f;
    state->background_color[1] = 0.047f;
    state->background_color[2] = 0.067f;
    state->background_color[3] = 1.0f;
}



static DvzDepthCueDesc _depth_cue_desc_from_controls(const DepthCueDemoState* state)
{
    DvzDepthCueDesc cue = dvz_depth_cue_desc();
    if (state == NULL)
        return cue;

    cue.mode = (DvzDepthCueMode)(state->mode + 1);
    cue.metric = (DvzDepthCueMetric)state->metric;
    cue.falloff = (DvzDepthCueFalloff)state->falloff;
    cue.near_depth = state->near_depth;
    cue.far_depth = state->far_depth;
    cue.strength = state->strength;
    cue.density = state->density;
    cue.background_color[0] = state->background_color[0];
    cue.background_color[1] = state->background_color[1];
    cue.background_color[2] = state->background_color[2];
    cue.background_color[3] = state->background_color[3];
    return cue;
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

    if (state->near_depth < CUE_DISTANCE_MIN)
        state->near_depth = CUE_DISTANCE_MIN;
    if (state->near_depth > CUE_DISTANCE_MAX - CUE_DISTANCE_EPS)
        state->near_depth = CUE_DISTANCE_MAX - CUE_DISTANCE_EPS;
    if (state->far_depth > CUE_DISTANCE_MAX)
        state->far_depth = CUE_DISTANCE_MAX;
    if (state->far_depth <= state->near_depth + CUE_DISTANCE_EPS)
        state->far_depth = state->near_depth + CUE_DISTANCE_EPS;

    DvzDepthCueDesc cue = _depth_cue_desc_from_controls(state);
    if (dvz_visual_set_depth_cue(state->cued_sphere, &cue) != 0)
        dvz_fprintf(stderr, "technique_depth_cue: dvz_visual_set_depth_cue() failed\n");
}



static void _print_settings(const DepthCueDemoState* state)
{
    if (state == NULL)
        return;

    dvz_fprintf(stderr, "technique_depth_cue settings:\n");
    dvz_fprintf(stderr, "enabled = %s;\n", state->enabled ? "true" : "false");
    dvz_fprintf(stderr, "cue.mode = %d;\n", state->mode + 1);
    dvz_fprintf(stderr, "cue.metric = %d;\n", state->metric);
    dvz_fprintf(stderr, "cue.falloff = %d;\n", state->falloff);
    dvz_fprintf(stderr, "cue.near_depth = %.6ff;\n", state->near_depth);
    dvz_fprintf(stderr, "cue.far_depth = %.6ff;\n", state->far_depth);
    dvz_fprintf(stderr, "cue.strength = %.6ff;\n", state->strength);
    dvz_fprintf(stderr, "cue.density = %.6ff;\n", state->density);
    dvz_fprintf(
        stderr, "cue.background_color = {%.6ff, %.6ff, %.6ff, %.6ff};\n",
        state->background_color[0], state->background_color[1], state->background_color[2],
        state->background_color[3]);
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

    for (uint32_t z = 0; z < LATTICE_SIDE; z++)
    {
        for (uint32_t y = 0; y < LATTICE_SIDE; y++)
        {
            for (uint32_t x = 0; x < LATTICE_SIDE; x++)
            {
                const uint32_t i = z * LATTICE_SIDE * LATTICE_SIDE + y * LATTICE_SIDE + x;
                positions[i][0] = -0.58f + 0.58f * (float)x;
                positions[i][1] = -0.44f + 0.44f * (float)y;
                positions[i][2] = -0.72f + 0.72f * (float)z;
                colors[i] = example_graphite_cyan_color(
                    z == 0u   ? EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY
                    : z == 1u ? EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY
                              : EXAMPLE_STYLE_COLOR_WARNING);
                radii[i] = 0.115f;
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
    material.light_direction[0] = -0.32f;
    material.light_direction[1] = +0.55f;
    material.light_direction[2] = +0.76f;
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



static DvzController* _bind_arcball(DvzScenarioContext* ctx, DvzPanel* panel)
{
    DvzController* controller = dvz_arcball(ctx->scene, NULL);
    if (controller == NULL)
        return NULL;
    DvzArcball* arcball = dvz_controller_arcball(controller);
    if (arcball == NULL)
        return NULL;
    if (dvz_scenario_bind_controller(ctx, panel, controller, DVZ_DIM_MASK_XYZ) != 0)
        return NULL;
    dvz_arcball_set(arcball, (vec3){+0.48f, -0.18f, +0.20f});
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

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        return false;

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
    DvzController* plain_controller = _bind_arcball(ctx, plain);
    DvzController* cued_controller = _bind_arcball(ctx, cued);
    if (plain_controller == NULL || cued_controller == NULL)
        return false;
    if (dvz_controller_link(
            ctx->scene, plain_controller, cued_controller,
            DVZ_CONTROLLER_LINK_ROTATION | DVZ_CONTROLLER_LINK_PAN | DVZ_CONTROLLER_LINK_ZOOM,
            DVZ_CONTROLLER_LINK_TWO_WAY) == NULL)
        return false;

    DvzDepthCueDesc cue = _depth_cue_desc_from_controls(state);
    return _add_sphere_lattice(ctx->scene, plain, NULL, NULL) &&
           _add_sphere_lattice(ctx->scene, cued, &cue, &state->cued_sphere);
}



static void _depth_cue_gui(DvzGui* gui, DvzView* view, void* user_data)
{
    (void)view;
    DepthCueDemoState* state = (DepthCueDemoState*)user_data;
    if (gui == NULL || state == NULL)
        return;

    if (dvz_gui_begin(gui, "Depth cue settings", NULL, 0))
    {
        bool changed = false;
        static const char* const mode_items[] = {"Fade to background", "Desaturate", "Darken"};
        static const char* const metric_items[] = {"Clip depth", "Eye distance", "World distance"};
        static const char* const falloff_items[] = {"Linear", "Exponential"};

        changed |= dvz_gui_checkbox(gui, "Enable depth cue", &state->enabled);
        changed |= dvz_gui_combo(gui, "Mode", &state->mode, mode_items, 3);
        changed |= dvz_gui_combo(gui, "Metric", &state->metric, metric_items, 3);
        changed |= dvz_gui_combo(gui, "Falloff", &state->falloff, falloff_items, 2);
        changed |= dvz_gui_slider_float(
            gui, "Near depth", &state->near_depth, CUE_DISTANCE_MIN, CUE_DISTANCE_MAX);
        changed |= dvz_gui_slider_float(
            gui, "Far depth", &state->far_depth, CUE_DISTANCE_MIN, CUE_DISTANCE_MAX);
        changed |= dvz_gui_slider_float(gui, "Strength", &state->strength, 0.0f, 1.0f);
        if (state->falloff == DVZ_DEPTH_CUE_FALLOFF_EXPONENTIAL)
            changed |= dvz_gui_slider_float(gui, "Density", &state->density, 0.01f, 3.0f);
        if (state->mode == 0)
            changed |= dvz_gui_color_edit4(gui, "Background", state->background_color, 0);

        if (changed)
            _apply_depth_cue(state);
        if (dvz_gui_button(gui, "Reset"))
        {
            _reset_depth_cue_controls(state);
            _apply_depth_cue(state);
        }
        dvz_gui_same_line(gui, 0.0f, 8.0f);
        if (dvz_gui_button(gui, "Print settings"))
            _print_settings(state);
    }
    dvz_gui_end(gui);
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

    DvzGui* gui = dvz_view_gui(view, NULL);
    if (gui == NULL)
        return true;
    dvz_view_set_gui_callback(view, _depth_cue_gui, state);
    return true;
}



static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    dvz_free(user);
}



/**
 * Return the depth-cue scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _depth_cue_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "technique_depth_cue",
        .title = "Depth Cue",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .init = _scenario_init,
        .destroy = _scenario_destroy,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

static bool _cli_wants_live_gui(int argc, char** argv)
{
    for (int i = 1; i < argc; i++)
    {
        if (argv[i] == NULL)
            continue;
        if (strcmp(argv[i], "--live") == 0 || strcmp(argv[i], "--live-record") == 0)
            return true;
    }
    return false;
}



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
    if (_cli_wants_live_gui(argc, argv))
        spec.native_view = _scenario_native_view;
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
