/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* axes_2d - deterministic path with retained 2D axes and tick labels.
 *
 * Scenario: path_axes_2d
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/axes_2d
 * Run:    ./build/examples/c/features/axes_2d --live
 * Smoke:  ./build/examples/c/features/axes_2d --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "_assertions.h"
#include "_compat.h"
#include "datoviz/scene.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_example_axes_2d_scenario(void);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH      1600u
#define HEIGHT     1200u
#define PATH_COUNT 384u

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Fill deterministic path samples in data coordinates.
 *
 * @param positions output data-space path positions
 * @param colors output path colors
 * @param widths output path stroke widths in pixels
 * @param count sample count
 */
static void _fill_curve(vec3* positions, DvzColor* colors, float* widths, uint32_t count)
{
    ANN(positions);
    ANN(colors);
    ANN(widths);

    const ExampleStyleColorRole roles[] = {
        EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
        EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
        EXAMPLE_STYLE_COLOR_WARNING,
    };

    const float inv_count = count > 1 ? 1.0f / (float)(count - 1u) : 1.0f;
    for (uint32_t i = 0; i < count; i++)
    {
        const float t = (float)i * inv_count;
        const float x = 10.0f * t;
        const float envelope = expf(-0.12f * x);
        const float y = envelope * (1.65f * sinf(1.25f * TAU * t) +
                                    0.35f * sinf(4.0f * TAU * t + 0.35f));

        positions[i][0] = x;
        positions[i][1] = y;
        positions[i][2] = 0.0f;

        uint32_t role_index = (uint32_t)(t * (float)DVZ_ARRAY_COUNT(roles));
        if (role_index >= DVZ_ARRAY_COUNT(roles))
            role_index = DVZ_ARRAY_COUNT(roles) - 1u;
        colors[i] = example_graphite_cyan_color(roles[role_index]);
        widths[i] = 4.0f;
    }
}



/**
 * Upload a single stroked path to a retained path visual.
 *
 * @param visual path visual
 * @param positions visual-space path positions
 * @param colors path colors
 * @param widths path stroke widths
 * @param count sample count
 * @return true when all uploads succeed
 */
static bool _upload_path(
    DvzVisual* visual, vec3* positions, DvzColor* colors, float* widths, uint32_t count)
{
    ANN(visual);
    ANN(positions);
    ANN(colors);
    ANN(widths);

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = count},
        {.attr_name = "color", .data = colors, .item_count = count},
        {.attr_name = "stroke_width", .data = widths, .item_count = count},
    };
    if (dvz_visual_set_data_many(visual, updates, 3) != 0)
        return false;
    if (dvz_path_set_caps(visual, DVZ_SEGMENT_CAP_ROUND, DVZ_SEGMENT_CAP_ROUND) != 0)
        return false;
    return dvz_path_set_join(visual, DVZ_PATH_JOIN_ROUND, 4.0f) == 0;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the deterministic 2D axes and path feature scenario.
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

    vec3 data_positions[PATH_COUNT] = {{0}};
    vec3 visual_positions[PATH_COUNT] = {{0}};
    DvzColor colors[PATH_COUNT] = {{0}};
    float widths[PATH_COUNT] = {0};

    _fill_curve(data_positions, colors, widths, PATH_COUNT);

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        return false;

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL)
        return false;
    example_graphite_cyan_set_panel_background(panel);

    if (dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 10.0) != 0)
        return false;
    if (dvz_panel_set_domain(panel, DVZ_DIM_Y, -2.0, 2.0) != 0)
        return false;

    int rc = dvz_panel_data_to_visual_positions(
        panel, (const float*)data_positions, (float*)visual_positions, PATH_COUNT);
    if (rc != 0)
        return false;

    DvzVisual* path = dvz_path(ctx->scene, 0);
    if (path == NULL)
        return false;

    bool ok = _upload_path(path, visual_positions, colors, widths, PATH_COUNT);
    if (!ok)
        return false;

    DvzVisualAttachDesc attach = dvz_visual_attach_desc();
    attach.coord_space = DVZ_COORD_VIEW;
    rc = dvz_panel_add_visual(panel, path, &attach);
    if (rc != 0)
        return false;

    DvzAxis* x_axis = dvz_panel_axis(panel, DVZ_DIM_X);
    DvzAxis* y_axis = dvz_panel_axis(panel, DVZ_DIM_Y);
    if (x_axis == NULL || y_axis == NULL)
        return false;

    ok = example_graphite_cyan_apply_axis_style(x_axis, false, NULL);
    if (!ok)
        return false;
    ok = example_graphite_cyan_apply_axis_style(y_axis, true, NULL);
    if (!ok)
        return false;

    ok = dvz_axis_set_grid(x_axis, true);
    if (!ok)
        return false;
    ok = dvz_axis_set_grid(y_axis, true);
    if (!ok)
        return false;
    ok = dvz_axis_set_label(x_axis, "time (s)");
    if (!ok)
        return false;
    ok = dvz_axis_set_label(y_axis, "signal");
    if (!ok)
        return false;

    DvzPanzoom* panzoom = dvz_scenario_panzoom(ctx, panel, NULL, DVZ_DIM_MASK_XY);
    return panzoom != NULL;
}



/**
 * Return the deterministic 2D axes and path scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_example_axes_2d_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "path_axes_2d",
        .title = "path_axes_2d",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_TEXT_VISUAL | DVZ_SCENARIO_REQ_CONTROLLER |
                        DVZ_SCENARIO_REQ_PANZOOM,
        .init = _scenario_init,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the deterministic 2D axes and path feature proof through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_example_axes_2d_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
