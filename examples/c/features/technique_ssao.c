/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* ssao - screen-space ambient occlusion on normal-producing mesh geometry.
 *
 * Scenario: feature.ssao
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
#include <string.h>

#include "_alloc.h"
#include "datoviz/gui.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_gui_controls.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT



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
} SsaoDemoState;



static DvzSsaoDesc _ssao_desc_from_controls(const DvzExampleGuiSsaoControls* controls)
{
    DvzSsaoDesc ssao = dvz_ssao_desc();
    if (controls == NULL)
        return ssao;

    ssao.radius = controls->radius;
    ssao.strength = controls->strength;
    ssao.bias = controls->bias;
    ssao.power = controls->power;
    ssao.min_visibility = controls->min_visibility;
    ssao.sample_count = (uint32_t)(controls->samples + 0.5f);
    ssao.blur_enabled = controls->blur;
    ssao.blur_radius = controls->blur_radius;
    ssao.blur_depth_sigma = controls->blur_depth_sigma;
    ssao.blur_normal_sigma = controls->blur_normal_sigma;
    ssao.debug_view = controls->debug_view;
    return ssao;
}



static void _apply_ssao(SsaoDemoState* state)
{
    if (state == NULL || state->ssao_panel == NULL)
        return;

    if (!state->ssao.enabled)
    {
        (void)dvz_panel_set_ssao(state->ssao_panel, NULL);
        return;
    }

    DvzSsaoDesc ssao = _ssao_desc_from_controls(&state->ssao);
    (void)dvz_panel_set_ssao(state->ssao_panel, &ssao);
}



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



static void _sync_arcball_controls(SsaoDemoState* state)
{
    if (state == NULL || state->plain_arcball == NULL)
        return;

    dvz_arcball_angles(state->plain_arcball, state->arcball_angles);
    DvzArcballState arcball = {0};
    if (dvz_arcball_state(state->plain_arcball, &arcball))
    {
        state->arcball_zoom = arcball.zoom;
        state->arcball_pan[0] = arcball.pan[0];
        state->arcball_pan[1] = arcball.pan[1];
    }
}



static void _print_settings(const SsaoDemoState* state)
{
    if (state == NULL)
        return;

    dvz_fprintf(stderr, "technique_ssao settings:\n");
    dvz_fprintf(stderr, "dvz_arcball_set(arcball, (vec3){%+.6ff, %+.6ff, %+.6ff});\n",
            state->arcball_angles[0], state->arcball_angles[1], state->arcball_angles[2]);
    dvz_fprintf(stderr, "dvz_arcball_zoom(arcball, %.6ff);\n", state->arcball_zoom);
    dvz_fprintf(stderr, "dvz_arcball_pan(arcball, (vec2){%+.6ff, %+.6ff});\n",
            state->arcball_pan[0], state->arcball_pan[1]);
    dvz_fprintf(stderr, "ssao.radius = %.6ff;\n", state->ssao.radius);
    dvz_fprintf(stderr, "ssao.strength = %.6ff;\n", state->ssao.strength);
    dvz_fprintf(stderr, "ssao.bias = %.6ff;\n", state->ssao.bias);
    dvz_fprintf(stderr, "ssao.power = %.6ff;\n", state->ssao.power);
    dvz_fprintf(stderr, "ssao.min_visibility = %.6ff;\n", state->ssao.min_visibility);
    dvz_fprintf(stderr, "ssao.sample_count = %uu;\n", (uint32_t)(state->ssao.samples + 0.5f));
    dvz_fprintf(stderr, "ssao.blur_enabled = %s;\n", state->ssao.blur ? "true" : "false");
    dvz_fprintf(stderr, "ssao.blur_radius = %.6ff;\n", state->ssao.blur_radius);
    dvz_fprintf(stderr, "ssao.blur_depth_sigma = %.6ff;\n", state->ssao.blur_depth_sigma);
    dvz_fprintf(stderr, "ssao.blur_normal_sigma = %.6ff;\n", state->ssao.blur_normal_sigma);
    dvz_fprintf(stderr, "ssao.debug_view = %s;\n", state->ssao.debug_view ? "true" : "false");
}

/**
 * Add an opaque sphere cluster with close contact shadows for SSAO tuning.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @return true on success
 */
static bool _add_sphere_cluster(DvzScene* scene, DvzPanel* panel)
{
    const vec3 positions[12] = {
        {-0.62f, -0.42f, -0.18f},
        {-0.26f, -0.50f, -0.07f},
        {+0.12f, -0.45f, +0.01f},
        {+0.52f, -0.28f, -0.15f},
        {-0.50f, +0.00f, +0.04f},
        {-0.10f, -0.04f, +0.19f},
        {+0.31f, +0.04f, +0.13f},
        {+0.70f, +0.22f, -0.05f},
        {-0.33f, +0.42f, -0.04f},
        {+0.06f, +0.41f, +0.08f},
        {+0.48f, +0.48f, -0.11f},
        {+0.02f, +0.03f, -0.34f},
    };
    const float radii[12] = {
        0.19f,
        0.16f,
        0.18f,
        0.15f,
        0.20f,
        0.24f,
        0.19f,
        0.15f,
        0.17f,
        0.21f,
        0.16f,
        0.34f,
    };
    DvzColor colors[12] = {
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID),
    };

    DvzVisual* spheres = dvz_sphere(scene, DVZ_SPHERE_FLAGS_LIGHTING);
    if (spheres == NULL)
        return false;
    if (dvz_sphere_set_mode(spheres, DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR) != 0)
        return false;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = 12},
        {.attr_name = "radius", .data = radii, .item_count = 12},
        {.attr_name = "color", .data = colors, .item_count = 12},
    };
    if (dvz_visual_set_data_many(spheres, updates, 3) != 0)
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
    label.text = "plain depth";
    DvzAnnotation* annotation = dvz_annotation_label(plain, &label);
    if (annotation == NULL || dvz_annotation_set_style(annotation, &label_style) != 0 ||
        dvz_annotation_set_placement(annotation, &label_placement) != 0)
        return false;
    label.text = "SSAO resolve";
    annotation = dvz_annotation_label(ssao_panel, &label);
    if (annotation == NULL || dvz_annotation_set_style(annotation, &label_style) != 0 ||
        dvz_annotation_set_placement(annotation, &label_placement) != 0)
        return false;
    if (example_set_default_3d_camera(plain, 1.0f) == NULL ||
        example_set_default_3d_camera(ssao_panel, 1.0f) == NULL)
        return false;
    if (!_add_sphere_cluster(ctx->scene, plain) || !_add_sphere_cluster(ctx->scene, ssao_panel))
        return false;

    state->arcball_angles[0] = +0.708f;
    state->arcball_angles[1] = -0.354f;
    state->arcball_angles[2] = +0.244f;
    state->arcball_zoom = 1.0f;
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
        .radius = 0.245f,
        .strength = 2.650f,
        .bias = 0.014f,
        .power = 2.250f,
        .min_visibility = 0.220f,
        .samples = 32.0f,
        .min_samples = 4.0f,
        .max_samples = 64.0f,
        .blur_radius = 10.0f,
        .blur_radius_max = 24.0f,
        .blur_depth_sigma = 0.750f,
        .blur_normal_sigma = 0.380f,
    };
    _apply_ssao(state);
    return true;
}



static void _ssao_gui(DvzGui* gui, DvzView* view, void* user_data)
{
    (void)view;
    SsaoDemoState* state = (SsaoDemoState*)user_data;
    if (gui == NULL || state == NULL)
        return;

    if (dvz_gui_begin(gui, "SSAO calibration", NULL, 0))
    {
        dvz_gui_separator_text(gui, "Occlusion");
        if (example_gui_ssao(gui, &state->ssao))
            _apply_ssao(state);

        dvz_gui_separator_text(gui, "Arcball");
        _sync_arcball_controls(state);
        bool arcball_changed = false;
        arcball_changed |=
            dvz_gui_slider_float3(gui, "Angles", state->arcball_angles, -3.14159f, +3.14159f);
        arcball_changed |= dvz_gui_slider_float(gui, "Zoom", &state->arcball_zoom, 0.20f, 4.0f);
        arcball_changed |=
            dvz_gui_slider_float2(gui, "Pan", state->arcball_pan, -2.0f, +2.0f);
        if (arcball_changed)
            _apply_arcball(state);

        if (dvz_gui_button(gui, "Reset arcball"))
        {
            state->arcball_angles[0] = +0.708f;
            state->arcball_angles[1] = -0.354f;
            state->arcball_angles[2] = +0.244f;
            state->arcball_zoom = 1.0f;
            state->arcball_pan[0] = +0.000f;
            state->arcball_pan[1] = -0.020f;
            _apply_arcball(state);
        }
        dvz_gui_same_line(gui, 0.0f, 8.0f);
        if (dvz_gui_button(gui, "Print settings"))
            _print_settings(state);
    }
    dvz_gui_end(gui);
}



static bool _scenario_native_view(DvzScenarioContext* ctx, DvzApp* app, DvzView* view, void* user)
{
    (void)app;
    SsaoDemoState* state = (SsaoDemoState*)user;
    if (
        ctx == NULL || ctx->presentation != DVZ_RUNNER_PRESENT_GLFW || state == NULL ||
        view == NULL)
        return true;

    DvzGui* gui = dvz_view_gui(view, NULL);
    if (gui == NULL)
        return true;
    dvz_view_set_gui_callback(view, _ssao_gui, state);
    return true;
}



static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    dvz_free(user);
}



/**
 * Return the SSAO scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _ssao_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "technique_ssao",
        .title = "Screen-Space Ambient Occlusion",
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
 * Run the SSAO feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _ssao_scenario();
    if (_cli_wants_live_gui(argc, argv))
        spec.native_view = _scenario_native_view;
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
