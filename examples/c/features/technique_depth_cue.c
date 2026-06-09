/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* depth_cue - depth-dependent fading applied to a 3D point stack.
 *
 * Scenario: feature.depth_cue
 * Style: features, graphite_cyan, 1600x1200 capture target
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

#include "datoviz/scene.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH       1600u
#define HEIGHT      1200u
#define POINT_COUNT 18u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Add depth-separated points with optional depth cueing.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param cue_enabled whether depth cueing is enabled
 * @return true on success
 */
static bool _add_points(DvzScene* scene, DvzPanel* panel, bool cue_enabled)
{
    vec3 positions[POINT_COUNT] = {{0}};
    DvzColor colors[POINT_COUNT] = {{0}};
    float diameters[POINT_COUNT] = {0};

    for (uint32_t i = 0; i < POINT_COUNT; i++)
    {
        const float u = (float)i / (float)(POINT_COUNT - 1u);
        positions[i][0] = -0.74f + 1.48f * u;
        positions[i][1] = -0.28f + 0.56f * ((float)(i % 3u) - 1.0f);
        positions[i][2] = -0.70f + 1.65f * u;
        colors[i] = example_graphite_cyan_color(
            i % 3u == 0   ? EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY
            : i % 3u == 1 ? EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY
                          : EXAMPLE_STYLE_COLOR_WARNING);
        diameters[i] = 50.0f;
    }

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
    if (cue_enabled)
    {
        DvzDepthCueDesc cue = dvz_depth_cue_desc();
        cue.mode = DVZ_DEPTH_CUE_FADE_TO_BACKGROUND;
        cue.metric = DVZ_DEPTH_CUE_METRIC_EYE_DISTANCE;
        cue.falloff = DVZ_DEPTH_CUE_FALLOFF_LINEAR;
        cue.near_depth = 1.20f;
        cue.far_depth = 3.80f;
        cue.strength = 0.88f;
        cue.background_color[0] = 0.035f;
        cue.background_color[1] = 0.047f;
        cue.background_color[2] = 0.067f;
        cue.background_color[3] = 1.0f;
        if (dvz_visual_set_depth_cue(point, &cue) != 0)
            return false;
    }
    return dvz_panel_add_visual(panel, point, NULL) == 0;
}



/**
 * Set the depth-cue camera.
 *
 * @param panel target panel
 * @return true on success
 */
static bool _set_camera(DvzPanel* panel)
{
    DvzCameraDesc camera = dvz_camera_desc();
    camera.eye[0] = 0.0f;
    camera.eye[1] = -3.20f;
    camera.eye[2] = 0.86f;
    camera.up[1] = 0.0f;
    camera.up[2] = 1.0f;
    camera.fov_y = 0.58f;
    camera.near = 0.05f;
    camera.far = 100.0f;
    return dvz_panel_set_camera(panel, &camera) != NULL;
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
    DvzPanel* cued = dvz_grid_panel(grid, 0, 1);
    if (plain == NULL || cued == NULL)
        return false;
    example_graphite_cyan_set_panel_background(plain);
    example_graphite_cyan_set_panel_background(cued);

    if (!_set_camera(plain) || !_set_camera(cued))
        return false;
    return _add_points(ctx->scene, plain, false) && _add_points(ctx->scene, cued, true);
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
        .title = "depth_cue",
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
 * Run the depth-cue feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _depth_cue_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
