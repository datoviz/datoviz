/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* scalebar - minimal retained scale bar attached to one 2D panel.
 *
 * Scenario: scale_bar
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/scalebar
 * Run:    ./build/examples/c/features/scalebar --live
 * Smoke:  ./build/examples/c/features/scalebar --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_assertions.h"
#include "datoviz/scene.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



DvzScenarioSpec dvz_example_scalebar_scenario(void);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH       1600u
#define HEIGHT      1200u
#define POINT_COUNT 5u



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
    vec3 visual_positions[POINT_COUNT] = {{0}};
    DvzColor colors[POINT_COUNT] = {{0}};
    float diameters[POINT_COUNT] = {0};

    DvzColor muted = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID);
    DvzColor accent = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY);
    for (uint32_t i = 0; i < POINT_COUNT; i++)
    {
        colors[i] = i == 0 || i == POINT_COUNT - 1u ? accent : muted;
        colors[i].a = 230u;
        diameters[i] = i == 0 || i == POINT_COUNT - 1u ? 16.0f : 10.0f;
    }

    int rc = dvz_panel_data_to_visual_positions(
        panel, (const float*)data_positions, (float*)visual_positions, POINT_COUNT);
    if (rc != 0)
        return false;

    DvzVisual* points = dvz_point(scene, 0);
    if (points == NULL)
        return false;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = visual_positions, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = colors, .item_count = POINT_COUNT},
        {.attr_name = "diameter", .data = diameters, .item_count = POINT_COUNT},
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
        .label_style = {
        DVZ_STRUCT_INIT_FIELDS(DvzTextStyle),
            .size_px = 17.0f,
            .renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS,
            .color = {color.r, color.g, color.b, 255u},
        },
    };

    DvzAnnotation* scalebar = dvz_annotation_scalebar(panel, &desc);
    return scalebar != NULL && dvz_scalebar_set_units((DvzScaleBar*)scalebar, length_units) == 0;
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

    if (!dvz_panel_set_layout_reserve(
        panel, &(DvzPanelLayoutReserve){.left = 0.08f, .right = 0.08f, .bottom = 0.12f,
                                        .top = 0.08f}))
        return false;
    if (dvz_panel_set_domain(panel, DVZ_DIM_X, -1.0, 9.0) != 0)
        return false;
    if (dvz_panel_set_domain(panel, DVZ_DIM_Y, -1.0, 1.0) != 0)
        return false;

    if (!_add_points(ctx->scene, panel))
        return false;
    if (!_add_scalebar(ctx->scene, panel))
        return false;

    DvzPanzoom* panzoom = dvz_scenario_panzoom(ctx, panel, NULL, DVZ_DIM_MASK_XY);
    return panzoom != NULL;
}



/**
 * Return the scale-bar scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_example_scalebar_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_scalebar",
        .title = "scalebar",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_POINT_VISUAL | DVZ_SCENARIO_REQ_TEXT_VISUAL |
                        DVZ_SCENARIO_REQ_CONTROLLER | DVZ_SCENARIO_REQ_PANZOOM,
        .init = _scenario_init,
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
