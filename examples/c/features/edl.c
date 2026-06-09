/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* edl - Eye-Dome Lighting applied to a compact depth-separated point cloud.
 *
 * Scenario: feature.edl
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/edl
 * Run:    ./build/examples/c/features/edl --live
 * Smoke:  ./build/examples/c/features/edl --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "datoviz/scene.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH       1600u
#define HEIGHT      1200u
#define POINT_COLS  13u
#define POINT_ROWS  9u
#define POINT_COUNT (POINT_COLS * POINT_ROWS)



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Fill deterministic depth-separated point data.
 *
 * @param positions output point positions
 * @param colors output point colors
 * @param diameters output point diameters
 */
static void _fill_points(
    vec3 positions[POINT_COUNT], DvzColor colors[POINT_COUNT], float diameters[POINT_COUNT])
{
    const ExampleStyleColorRole roles[4] = {
        EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
        EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
        EXAMPLE_STYLE_COLOR_WARNING,
        EXAMPLE_STYLE_COLOR_TEXT,
    };

    for (uint32_t row = 0; row < POINT_ROWS; row++)
    {
        for (uint32_t col = 0; col < POINT_COLS; col++)
        {
            const uint32_t i = row * POINT_COLS + col;
            const float u = (float)col / (float)(POINT_COLS - 1u);
            const float v = (float)row / (float)(POINT_ROWS - 1u);
            positions[i][0] = -1.00f + 2.00f * u;
            positions[i][1] = -0.70f + 1.40f * v;
            positions[i][2] = 0.50f * sinf(7.0f * u) + 0.35f * cosf(5.0f * v);
            colors[i] = example_graphite_cyan_color(roles[(col + 2u * row) % 4u]);
            diameters[i] = 24.0f + 10.0f * (0.5f + 0.5f * sinf(11.0f * u + 3.0f * v));
        }
    }
}



/**
 * Add one point cloud visual to a panel.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @return true on success
 */
static bool _add_point_cloud(DvzScene* scene, DvzPanel* panel)
{
    vec3 positions[POINT_COUNT] = {{0}};
    DvzColor colors[POINT_COUNT] = {{0}};
    float diameters[POINT_COUNT] = {0};
    _fill_points(positions, colors, diameters);

    DvzVisual* point = dvz_point(scene, 0);
    if (point == NULL)
        return false;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = colors, .item_count = POINT_COUNT},
        {.attr_name = "diameter", .data = diameters, .item_count = POINT_COUNT},
    };
    if (dvz_visual_set_data_many(point, updates, 3) != 0)
        return false;

    DvzPointStyleDesc style = dvz_point_style_desc();
    style.aspect = DVZ_SHAPE_ASPECT_FILLED;
    style.stroke_width = 0.0f;
    if (dvz_point_set_style(point, &style) != 0)
        return false;

    return dvz_panel_add_visual(panel, point, NULL) == 0;
}



/**
 * Set the shared 3D camera.
 *
 * @param panel target panel
 * @return true on success
 */
static bool _set_camera(DvzPanel* panel)
{
    DvzCameraDesc camera = dvz_camera_desc();
    camera.eye[0] = 0.0f;
    camera.eye[1] = -3.10f;
    camera.eye[2] = 1.15f;
    camera.up[1] = 0.0f;
    camera.up[2] = 1.0f;
    camera.fov_y = 0.62f;
    camera.near = 0.05f;
    camera.far = 100.0f;
    return dvz_panel_set_camera(panel, &camera) != NULL;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the EDL feature scenario.
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

    DvzGrid* grid = dvz_figure_grid(ctx->figure, 1, 2);
    if (grid == NULL)
        return false;
    if (!dvz_grid_set_margins(
            grid, &(DvzPanelReserve){
                      .left_px = 42.0f, .right_px = 42.0f, .top_px = 38.0f, .bottom_px = 38.0f}))
        return false;
    if (!dvz_grid_set_gutter(grid, 30.0f, 0.0f))
        return false;

    DvzPanel* plain = dvz_grid_panel(grid, 0, 0);
    DvzPanel* lit = dvz_grid_panel(grid, 0, 1);
    if (plain == NULL || lit == NULL)
        return false;
    example_graphite_cyan_set_panel_background(plain);
    example_graphite_cyan_set_panel_background(lit);

    if (!_set_camera(plain) || !_set_camera(lit))
        return false;
    if (!_add_point_cloud(ctx->scene, plain) || !_add_point_cloud(ctx->scene, lit))
        return false;

    DvzEdlDesc edl = dvz_edl_desc();
    edl.radius = 2.0f;
    edl.strength = 58.0f;
    edl.depth_scale = 1.0f;
    return dvz_panel_set_edl(lit, &edl);
}



/**
 * Return the EDL scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _edl_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_edl",
        .title = "edl",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .init = _scenario_init,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the Eye-Dome Lighting feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _edl_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
