/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* panel_multi - multiple independent panels with panel-local panzoom controllers.
 *
 * Scenario: feature.panel_multi
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/panel_multi
 * Run:    ./build/examples/c/features/panel_multi --live
 * Smoke:  ./build/examples/c/features/panel_multi --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "_assertions.h"
#include "datoviz/scene.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_example_panel_multi_scenario(void);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH       1600u
#define HEIGHT      1200u
#define POINT_COUNT 48u
#define PATH_COUNT  96u

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Set a compact 2D data domain on one panel.
 *
 * @param panel target panel
 * @return true when both dimensions were set
 */
static bool _set_unit_domain(DvzPanel* panel)
{
    ANN(panel);

    int rc = dvz_panel_set_domain(panel, DVZ_DIM_X, -1.0, 1.0);
    if (rc != 0)
        return false;
    rc = dvz_panel_set_domain(panel, DVZ_DIM_Y, -1.0, 1.0);
    return rc == 0;
}



/**
 * Add deterministic point data to one panel.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @return true when the visual was added
 */
static bool _add_point_panel(DvzScene* scene, DvzPanel* panel)
{
    ANN(scene);
    ANN(panel);

    vec3 data_positions[POINT_COUNT] = {{0}};
    vec3 visual_positions[POINT_COUNT] = {{0}};
    DvzColor colors[POINT_COUNT] = {{0}};
    float diameters[POINT_COUNT] = {0};

    DvzColor primary = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY);
    DvzColor secondary = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY);
    for (uint32_t i = 0; i < POINT_COUNT; i++)
    {
        const float t = (float)i / (float)POINT_COUNT;
        const float theta = TAU * t;
        const float radius = 0.34f + 0.18f * sinf(3.0f * theta);

        data_positions[i][0] = radius * cosf(theta);
        data_positions[i][1] = radius * sinf(theta);
        data_positions[i][2] = 0.0f;
        colors[i] = i % 2u == 0u ? primary : secondary;
        colors[i].a = 238u;
        diameters[i] = 7.0f + 7.0f * (0.5f + 0.5f * sinf(5.0f * theta));
    }

    int rc = dvz_panel_data_to_visual_positions(
        panel, (const float*)data_positions, (float*)visual_positions, POINT_COUNT);
    if (rc != 0)
        return false;

    DvzVisual* visual = dvz_point(scene, 0);
    if (visual == NULL)
        return false;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = visual_positions, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = colors, .item_count = POINT_COUNT},
        {.attr_name = "diameter_px", .data = diameters, .item_count = POINT_COUNT},
    };
    if (dvz_visual_set_data_many(visual, updates, 3) != 0)
        return false;

    DvzPointStyleDesc style = dvz_point_style_desc();
    style.aspect = DVZ_SHAPE_ASPECT_FILLED;
    style.stroke_width_px = 0.0f;
    if (dvz_point_set_style(visual, &style) != 0)
        return false;
    if (dvz_visual_set_depth_test(visual, false) != 0)
        return false;
    DvzVisualAttachDesc attach = dvz_visual_attach_desc();
    attach.coord_space = DVZ_COORD_VIEW;
    return dvz_panel_add_visual(panel, visual, &attach) == 0;
}



/**
 * Add deterministic path data to one panel.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @return true when the visual was added
 */
static bool _add_path_panel(DvzScene* scene, DvzPanel* panel)
{
    ANN(scene);
    ANN(panel);

    vec3 data_positions[PATH_COUNT] = {{0}};
    vec3 visual_positions[PATH_COUNT] = {{0}};
    DvzColor colors[PATH_COUNT] = {{0}};
    float widths[PATH_COUNT] = {0};

    for (uint32_t i = 0; i < PATH_COUNT; i++)
    {
        const float t = PATH_COUNT > 1u ? (float)i / (float)(PATH_COUNT - 1u) : 0.0f;
        data_positions[i][0] = -0.88f + 1.76f * t;
        data_positions[i][1] = 0.32f * sinf(TAU * (1.5f * t + 0.08f)) +
                               0.18f * cosf(TAU * (3.0f * t + 0.21f));
        data_positions[i][2] = 0.0f;
        colors[i] = dvz_color_rgba(74, (uint8_t)(176.0f + 56.0f * t), 232, 242);
        widths[i] = 3.0f;
    }

    int rc = dvz_panel_data_to_visual_positions(
        panel, (const float*)data_positions, (float*)visual_positions, PATH_COUNT);
    if (rc != 0)
        return false;

    DvzVisual* visual = dvz_path(scene, 0);
    if (visual == NULL)
        return false;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = visual_positions, .item_count = PATH_COUNT},
        {.attr_name = "color", .data = colors, .item_count = PATH_COUNT},
        {.attr_name = "stroke_width_px", .data = widths, .item_count = PATH_COUNT},
    };
    if (dvz_visual_set_data_many(visual, updates, 3) != 0)
        return false;
    if (dvz_path_set_caps(visual, DVZ_SEGMENT_CAP_ROUND, DVZ_SEGMENT_CAP_ROUND) != 0)
        return false;
    if (dvz_path_set_join(visual, DVZ_PATH_JOIN_ROUND, 4.0f) != 0)
        return false;
    if (dvz_visual_set_depth_test(visual, false) != 0)
        return false;
    DvzVisualAttachDesc attach = dvz_visual_attach_desc();
    attach.coord_space = DVZ_COORD_VIEW;
    return dvz_panel_add_visual(panel, visual, &attach) == 0;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the multiple-independent-panel feature scenario.
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

    DvzPanel* left = dvz_panel(
        ctx->figure, (DvzPanelDesc){.x = 0.08f, .y = 0.16f, .width = 0.40f, .height = 0.68f});
    DvzPanel* right = dvz_panel(
        ctx->figure, (DvzPanelDesc){.x = 0.52f, .y = 0.16f, .width = 0.40f, .height = 0.68f});
    if (left == NULL || right == NULL)
        return false;

    DvzPanel* panels[2] = {left, right};
    for (uint32_t i = 0; i < 2u; i++)
    {
        example_graphite_cyan_set_panel_background(panels[i]);
        DvzPanelBorderDesc border = dvz_panel_border_desc();
        border.color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID);
        border.width_px = 1.5f;
        if (!dvz_panel_set_border(panels[i], &border))
            return false;
        if (!_set_unit_domain(panels[i]))
            return false;
    }

    if (!_add_point_panel(ctx->scene, left))
        return false;
    if (!_add_path_panel(ctx->scene, right))
        return false;

    DvzPanzoom* left_panzoom = dvz_scenario_panzoom(ctx, left, NULL, DVZ_DIM_MASK_XY);
    DvzPanzoom* right_panzoom = dvz_scenario_panzoom(ctx, right, NULL, DVZ_DIM_MASK_XY);
    if (left_panzoom == NULL || right_panzoom == NULL)
        return false;
    dvz_panzoom_zoom(left_panzoom, (vec2){1.20f, 1.20f});
    dvz_panzoom_pan(left_panzoom, (vec2){-0.12f, +0.08f});
    dvz_panzoom_zoom(right_panzoom, (vec2){1.65f, 1.10f});
    dvz_panzoom_pan(right_panzoom, (vec2){+0.18f, -0.04f});
    return true;
}



/**
 * Return the multiple-independent-panel scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_example_panel_multi_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_panel_multi",
        .title = "panel_multi",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_POINT_VISUAL | DVZ_SCENARIO_REQ_CONTROLLER |
                        DVZ_SCENARIO_REQ_PANZOOM,
        .init = _scenario_init,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the multiple-independent-panel feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_example_panel_multi_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
