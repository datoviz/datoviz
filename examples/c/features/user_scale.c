/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* This example shows how one user-scale value affects screen-space visual sizes and axes.
 *
 * What to look for: the panel combines a data-space path, outlined markers, axes, and panzoom.
 * The path uploads position, color, and stroke_width_px arrays, while the markers upload position,
 * color, diameter_px, angle, and symbol arrays. Live mode animates the user scale sinusoidally by
 * default; disable animation or move the GUI scale slider to inspect a fixed value and compare
 * marker diameters, stroke widths, text, and axis styling while data coordinates stay fixed. User
 * scale is useful for HiDPI displays, screenshots, and accessibility-sized scientific figures. The
 * GUI is attached only for `--live`/`--live-record` and starts docked on the left.
 *
 * Scenario: features_user_scale
 * Style: features, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c features/user_scale
 * Run:    ./build/examples/c/features/user_scale --live
 * Smoke:  ./build/examples/c/features/user_scale --png --user-scale 1.4
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "datoviz/gui.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"
#include "example_tuner.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_example_user_scale_scenario(void);

static bool _gui_controls(DvzGui* gui, void* user_data);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH        EXAMPLE_WINDOW_WIDTH
#define HEIGHT       EXAMPLE_WINDOW_HEIGHT
#define PATH_COUNT   192u
#define MARKER_COUNT 9u
#define SCALE_PERIOD 2.0

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct UserScaleState
{
    ExampleTuner tuner;
    DvzView* view;
    float user_scale;
    bool animate;
} UserScaleState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Fill a deterministic curve in data coordinates.
 *
 * @param positions output curve positions
 * @param colors output curve colors
 * @param widths output logical stroke widths
 */
static void
_fill_path(vec3 positions[PATH_COUNT], DvzColor colors[PATH_COUNT], float widths[PATH_COUNT])
{
    ANN(positions);
    ANN(colors);
    ANN(widths);

    for (uint32_t i = 0; i < PATH_COUNT; i++)
    {
        const float t = (float)i / (float)(PATH_COUNT - 1u);
        positions[i][0] = -3.0f + 6.0f * t;
        positions[i][1] = 0.55f * sinf(2.0f * TAU * t) + 0.18f * sinf(7.0f * TAU * t);
        positions[i][2] = 0.0f;
        colors[i] = example_graphite_cyan_color(
            i < PATH_COUNT / 2u ? EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY
                                : EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY);
        widths[i] = 5.0f;
    }
}



/**
 * Add a retained stroked path.
 *
 * @param scene scene owning the visual
 * @param panel target panel
 * @return true on success
 */
static bool _add_path(DvzScene* scene, DvzPanel* panel)
{
    ANN(scene);
    ANN(panel);

    vec3 data_positions[PATH_COUNT] = {{0}};
    DvzColor colors[PATH_COUNT] = {{0}};
    float widths[PATH_COUNT] = {0};
    _fill_path(data_positions, colors, widths);

    DvzVisual* path = dvz_path(scene, 0);
    if (path == NULL)
        return false;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = data_positions, .item_count = PATH_COUNT},
        {.attr_name = "color", .data = colors, .item_count = PATH_COUNT},
        {.attr_name = "stroke_width_px", .data = widths, .item_count = PATH_COUNT},
    };
    if (dvz_visual_set_data_many(path, updates, 3) != 0)
        return false;
    if (dvz_path_set_caps(path, DVZ_SEGMENT_CAP_ROUND, DVZ_SEGMENT_CAP_ROUND) != 0)
        return false;
    if (dvz_path_set_join(path, DVZ_PATH_JOIN_ROUND, 4.0f) != 0)
        return false;
    DvzVisualAttachDesc attach = dvz_visual_attach_desc();
    attach.coord_space = DVZ_VISUAL_COORD_DATA;
    return dvz_panel_add_visual(panel, path, &attach) == 0;
}



/**
 * Add outlined data-space markers.
 *
 * @param scene scene owning the visual
 * @param panel target panel
 * @return true on success
 */
static bool _add_markers(DvzScene* scene, DvzPanel* panel)
{
    ANN(scene);
    ANN(panel);

    DvzVisual* markers = dvz_marker(scene, 0);
    if (markers == NULL)
        return false;

    DvzMarkerStyle style = dvz_marker_style();
    style.aspect = DVZ_SHAPE_ASPECT_OUTLINE;
    style.edge_color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT);
    style.stroke_width_px = 2.75f;
    if (dvz_marker_set_style(markers, &style) != 0)
        return false;

    vec3 positions[MARKER_COUNT] = {{0}};
    DvzColor colors[MARKER_COUNT] = {{0}};
    float diameters[MARKER_COUNT] = {0};
    float angles[MARKER_COUNT] = {0};
    uint32_t symbols[MARKER_COUNT] = {0};
    for (uint32_t i = 0; i < MARKER_COUNT; i++)
    {
        const float t = (float)i / (float)(MARKER_COUNT - 1u);
        positions[i][0] = -2.75f + 5.5f * t;
        positions[i][1] = -0.95f;
        positions[i][2] = 0.0f;
        colors[i] = example_graphite_cyan_color(
            i % 3u == 0u   ? EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY
            : i % 3u == 1u ? EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY
                           : EXAMPLE_STYLE_COLOR_WARNING);
        diameters[i] = 34.0f + 8.0f * (float)(i % 3u);
        angles[i] = 0.16f * (float)i;
        symbols[i] = (uint32_t)(i % 3u == 0u   ? DVZ_SYMBOL_DISC
                                : i % 3u == 1u ? DVZ_SYMBOL_TRIANGLE
                                               : DVZ_SYMBOL_DIAMOND);
    }

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = MARKER_COUNT},
        {.attr_name = "color", .data = colors, .item_count = MARKER_COUNT},
        {.attr_name = "diameter_px", .data = diameters, .item_count = MARKER_COUNT},
        {.attr_name = "angle", .data = angles, .item_count = MARKER_COUNT},
        {.attr_name = "symbol", .data = symbols, .item_count = MARKER_COUNT},
    };
    if (dvz_visual_set_data_many(markers, updates, 5) != 0)
        return false;
    if (dvz_visual_set_alpha_mode(markers, DVZ_ALPHA_BLENDED) != 0)
        return false;
    return dvz_panel_add_visual(panel, markers, NULL) == 0;
}



/**
 * Configure panel axes.
 *
 * @param panel target panel
 * @return true on success
 */
static bool _add_axes(DvzPanel* panel)
{
    ANN(panel);

    DvzPanelAxes2DDesc axes = dvz_panel_axes_2d_desc();
    axes.x_label = "x";
    axes.y_label = "amplitude";

    if (dvz_panel_set_axes_2d(panel, &axes) != DVZ_OK)
        return false;
    return example_graphite_cyan_style_axes_2d(panel, NULL);
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the user-scale feature scenario.
 *
 * @param ctx scenario context
 * @param out_user scenario state output
 * @return true on success
 */
static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL || out_user == NULL)
        return false;

    UserScaleState* state = (UserScaleState*)dvz_calloc(1, sizeof(UserScaleState));
    if (state == NULL)
        return false;
    state->tuner = example_tuner("User scale");
    state->user_scale = ctx->user_scale > 0.0f ? ctx->user_scale : 1.0f;
    state->animate = true;

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        goto error;
    example_tuner_figure(&state->tuner, ctx->figure);

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL)
        goto error;
    example_graphite_cyan_set_panel_background(panel);

    if (dvz_panel_set_domain(panel, DVZ_DIM_X, -3.25, 3.25) != 0)
        goto error;
    if (dvz_panel_set_domain(panel, DVZ_DIM_Y, -1.45, 1.20) != 0)
        goto error;
    if (!_add_path(ctx->scene, panel))
        goto error;
    if (!_add_markers(ctx->scene, panel))
        goto error;
    if (!_add_axes(panel))
        goto error;

    DvzPanzoom* panzoom = dvz_scenario_panzoom(ctx, panel, NULL, DVZ_DIM_MASK_XY);
    if (panzoom == NULL)
        goto error;
#ifndef DVZ_EXAMPLE_NO_APP
    if (!example_tuner_add_component(
            &state->tuner, "Display scale", state, NULL, _gui_controls, NULL, NULL, NULL))
    {
        goto error;
    }
#endif

    *out_user = state;
    return true;

error:
    dvz_free(state);
    return false;
}



/**
 * Draw the user-scale tuner component.
 *
 * @param gui GUI context
 * @param user_data example state
 * @return whether an animation or scale control changed
 */
static bool _gui_controls(DvzGui* gui, void* user_data)
{
    UserScaleState* state = (UserScaleState*)user_data;
    if (gui == NULL || state == NULL || state->view == NULL)
        return false;

    const bool animate_changed = dvz_gui_checkbox(gui, "Animate", &state->animate);
    const bool scale_changed =
        dvz_gui_slider_float(gui, "Scale", &state->user_scale, 0.5f, 2.5f);
    if (scale_changed)
    {
        state->animate = false;
        (void)dvz_view_set_user_scale(state->view, state->user_scale);
    }
    return animate_changed || scale_changed;
}



/**
 * Attach the live GUI to the native view.
 *
 * @param ctx scenario context
 * @param app app
 * @param view native view
 * @param user scenario state
 * @return true on success
 */
static bool _scenario_view(DvzScenarioContext* ctx, DvzApp* app, DvzView* view, void* user)
{
    (void)ctx;
    (void)app;
    UserScaleState* state = (UserScaleState*)user;
    if (state == NULL || view == NULL)
        return false;
    state->view = view;
    state->user_scale = dvz_view_user_scale(view);
    return true;
}


/**
 * Attach the left-docked tuner in explicit live mode.
 *
 * @param ctx scenario context
 * @param app app
 * @param view native view
 * @param user scenario state
 * @return true on success
 */
static bool _scenario_live_view(DvzScenarioContext* ctx, DvzApp* app, DvzView* view, void* user)
{
    if (!_scenario_view(ctx, app, view, user))
        return false;
    if (ctx == NULL || ctx->presentation != DVZ_RUNNER_PRESENT_GLFW)
        return true;
    UserScaleState* state = (UserScaleState*)user;
    return example_tuner_attach(&state->tuner, view);
}



/**
 * Animate the user scale for live runs and generated gallery media.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_frame(DvzScenarioContext* ctx, void* user)
{
    UserScaleState* state = (UserScaleState*)user;
    if (ctx == NULL || state == NULL || state->view == NULL)
        return;

    double phase = 0.0;
    if (ctx->preview_mode)
    {
        phase = dvz_scenario_preview_phase(ctx, DVZ_SCENARIO_PREVIEW_PHASE_SEAMLESS_LOOP);
    }
    else if (ctx->presentation == DVZ_RUNNER_PRESENT_GLFW && state->animate)
    {
        phase = fmod(fmax(ctx->time, 0.0) / SCALE_PERIOD, 1.0);
    }
    else
    {
        return;
    }

    state->user_scale = 1.35f + 0.55f * sinf(TAU * (float)phase);
    (void)dvz_view_set_user_scale(state->view, state->user_scale);
}



/**
 * Destroy the user-scale feature state.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    UserScaleState* state = (UserScaleState*)user;
    if (state == NULL)
        return;
    example_tuner_detach(&state->tuner);
    dvz_free(state);
}



/**
 * Return the user-scale scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_example_user_scale_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "features_user_scale",
        .title = "User Scale",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_MARKER_VISUAL | DVZ_SCENARIO_REQ_CONTROLLER |
                        DVZ_SCENARIO_REQ_PANZOOM | DVZ_SCENARIO_REQ_NATIVE_VIEW |
                        DVZ_SCENARIO_REQ_FRAME_CALLBACKS,
        .init = _scenario_init,
        .frame = _scenario_frame,
        .native_view = _scenario_view,
        .destroy = _scenario_destroy,
    };
}



/*************************************************************************************************/
/*  Main                                                                                         */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_example_user_scale_scenario();
    if (example_cli_wants_live_gui(argc, argv))
        spec.native_view = _scenario_live_view;
    return dvz_scenario_run_native_cli(&spec, argc, argv);
}
