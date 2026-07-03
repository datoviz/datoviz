/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* user_scale - screen-space marker, path, and axis scaling controlled by a GUI slider.
 *
 * Scenario: feature.user_scale
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
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_example_user_scale_scenario(void);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT
#define PATH_COUNT   192u
#define MARKER_COUNT 9u

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct UserScaleState
{
    DvzView* view;
    float user_scale;
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

    if (!dvz_panel_set_axes_2d(panel, &axes))
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
    state->user_scale = ctx->user_scale > 0.0f ? ctx->user_scale : 1.0f;

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        goto error;

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

    *out_user = state;
    return true;

error:
    dvz_free(state);
    return false;
}



/**
 * Draw the user-scale GUI.
 *
 * @param gui GUI context
 * @param view app view
 * @param user_data example state
 */
static void _gui_callback(DvzGui* gui, DvzView* view, void* user_data)
{
    UserScaleState* state = (UserScaleState*)user_data;
    if (state == NULL)
        return;

    if (dvz_gui_begin(gui, "User scale", NULL, 0))
    {
        if (dvz_gui_slider_float(gui, "Scale", &state->user_scale, 0.5f, 2.5f))
            dvz_view_set_user_scale(view, state->user_scale);
    }
    dvz_gui_end(gui);
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
static bool _scenario_native_view(DvzScenarioContext* ctx, DvzApp* app, DvzView* view, void* user)
{
    (void)ctx;
    (void)app;
    UserScaleState* state = (UserScaleState*)user;
    if (state == NULL || view == NULL)
        return false;
    state->view = view;
    state->user_scale = dvz_view_user_scale(view);
    if (ctx == NULL || ctx->presentation != DVZ_RUNNER_PRESENT_GLFW)
        return true;

    DvzGui* gui = dvz_view_gui(view, NULL);
    if (gui == NULL)
        return true;
    dvz_view_set_gui_callback(view, _gui_callback, state);
    return true;
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
    dvz_free(user);
}



/**
 * Return the user-scale scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_example_user_scale_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_user_scale",
        .title = "user_scale",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_MARKER_VISUAL | DVZ_SCENARIO_REQ_CONTROLLER |
                        DVZ_SCENARIO_REQ_PANZOOM | DVZ_SCENARIO_REQ_NATIVE_VIEW,
        .init = _scenario_init,
        .native_view = _scenario_native_view,
        .destroy = _scenario_destroy,
    };
}



/*************************************************************************************************/
/*  Main                                                                                         */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_example_user_scale_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv);
}
