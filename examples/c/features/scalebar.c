/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* scalebar attaches a retained metric length scale bar to a 2D panel.
 *
 * What to look for: five data-space points mark a simple ruler from 0 to 8 along X, while the
 * scale bar is anchored near the bottom-left of the panel and uses built-in metric length units.
 * Pan or zoom the live view and compare the bar label and tick length with the point spacing. A
 * scale bar gives readers physical context without requiring full axes.
 *
 * Scenario: features_scalebar
 * Style: features, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c features/scalebar
 * Run:    ./build/examples/c/features/scalebar --live
 * Smoke:  ./build/examples/c/features/scalebar --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "datoviz/scene.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



DvzScenarioSpec dvz_example_scalebar_scenario(void);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT
#define POINT_COUNT 5u

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct ScaleBarState
{
    DvzPanzoom* panzoom;
} ScaleBarState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Add a small ruler-like point visual in data coordinates.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @return true when the visual was added
 */
static bool _add_points(DvzScene* scene, DvzPanel* panel)
{
    ANN(scene);
    ANN(panel);

    vec3 data_positions[POINT_COUNT] = {
        {0.0f, 0.0f, 0.0f},
        {2.0f, 0.0f, 0.0f},
        {4.0f, 0.0f, 0.0f},
        {6.0f, 0.0f, 0.0f},
        {8.0f, 0.0f, 0.0f},
    };
    DvzColor colors[POINT_COUNT] = {{0}};
    float diameters[POINT_COUNT] = {0};

    DvzColor inner = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT);
    DvzColor accent = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY);
    for (uint32_t i = 0; i < POINT_COUNT; i++)
    {
        const bool endpoint = i == 0 || i == POINT_COUNT - 1u;
        colors[i] = endpoint ? accent : inner;
        colors[i].a = endpoint ? 255u : 232u;
        diameters[i] = endpoint ? 22.0f : 16.0f;
    }

    DvzVisual* points = dvz_point(scene, 0);
    if (points == NULL)
        return false;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = data_positions, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = colors, .item_count = POINT_COUNT},
        {.attr_name = "diameter_px", .data = diameters, .item_count = POINT_COUNT},
    };
    if (dvz_visual_set_data_many(points, updates, 3) != 0)
        return false;
    if (dvz_visual_set_depth_test(points, false) != 0)
        return false;
    return dvz_panel_add_visual(panel, points, NULL) == 0;
}



/**
 * Add one retained 2D scale bar in panel data coordinates.
 *
 * @param scene scene owning unit objects
 * @param panel panel receiving the annotation
 * @return true when the scale bar was added
 */
static bool _add_scalebar(DvzScene* scene, DvzPanel* panel)
{
    ANN(scene);
    ANN(panel);

    DvzUnits* length_units = dvz_units_builtin(scene, DVZ_UNIT_LADDER_METRIC_LENGTH, 0.001);
    if (length_units == NULL)
        return false;

    DvzColor color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY);
    DvzScaleBarDesc desc = {
        DVZ_STRUCT_INIT_FIELDS(DvzScaleBarDesc),
        .dimension = DVZ_DIM_X,
        .anchor = DVZ_SCENE_ANCHOR_BOTTOM_LEFT,
        .label_position = DVZ_SCALEBAR_LABEL_ABOVE,
        .target_length_px = 220.0f,
        .min_length_px = 160.0f,
        .max_length_px = 300.0f,
        .offset_px = {72.0f, 82.0f},
        .tick_length_px = 18.0f,
        .line_width_px = 4.0f,
        .line_color = {color.r, color.g, color.b, 255u},
    };
    DvzTextStyle label_style = {
        DVZ_STRUCT_INIT_FIELDS(DvzTextStyle),
        .size_px = 17.0f,
        .renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS,
        .color = {color.r, color.g, color.b, 255u},
    };

    DvzScaleBar* scalebar = dvz_scale_bar(panel, &desc);
    return scalebar != NULL && dvz_scale_bar_set_label_style(scalebar, &label_style) == 0 &&
           dvz_scale_bar_set_units(scalebar, length_units) == 0;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the minimal retained scale-bar scenario.
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

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        return false;

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL)
        return false;
    example_graphite_cyan_set_panel_background(panel);

    if (dvz_panel_set_domain(panel, DVZ_DIM_X, -1.0, 9.0) != 0)
        return false;
    if (dvz_panel_set_domain(panel, DVZ_DIM_Y, -1.0, 1.0) != 0)
        return false;

    if (!_add_points(ctx->scene, panel))
        return false;
    if (!_add_scalebar(ctx->scene, panel))
        return false;

    DvzPanzoom* panzoom = dvz_scenario_panzoom(ctx, panel, NULL, DVZ_DIM_MASK_XY);
    if (panzoom == NULL)
        return false;

    ScaleBarState* state = (ScaleBarState*)dvz_calloc(1, sizeof(*state));
    if (state == NULL)
        return false;
    state->panzoom = panzoom;
    if (out_user != NULL)
        *out_user = state;
    return true;
}



/**
 * Animate the scale-bar view span for generated gallery media.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_frame(DvzScenarioContext* ctx, void* user)
{
    ScaleBarState* state = (ScaleBarState*)user;
    if (ctx == NULL || !ctx->preview_mode || state == NULL || state->panzoom == NULL)
        return;

    const uint64_t count = ctx->preview_frame_count > 0 ? ctx->preview_frame_count : 1;
    const float phase = (float)(ctx->preview_frame_index % count) / (float)count;
    const float zoom_x = 1.42f + 0.42f * sinf(TAU * phase);
    (void)dvz_panzoom_pan(state->panzoom, (vec2){0.0f, 0.0f});
    (void)dvz_panzoom_zoom(state->panzoom, (vec2){zoom_x, 1.0f});
}



/**
 * Destroy the scale-bar scenario state.
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
 * Return the scale-bar scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_example_scalebar_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "features_scalebar",
        .title = "Scale Bar",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_POINT_VISUAL | DVZ_SCENARIO_REQ_TEXT_VISUAL |
                        DVZ_SCENARIO_REQ_CONTROLLER | DVZ_SCENARIO_REQ_PANZOOM |
                        DVZ_SCENARIO_REQ_FRAME_CALLBACKS,
        .init = _scenario_init,
        .frame = _scenario_frame,
        .destroy = _scenario_destroy,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the minimal retained scale-bar feature proof through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_example_scalebar_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
