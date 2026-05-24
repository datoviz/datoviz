/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* scatter_axes — interactive 2D scatter plot with WIP axes.
 *
 * Opens a GLFW window showing random discs with per-point color and diameter.
 * Left-drag to pan, right-drag or scroll to zoom, double-click to reset.
 * Axis tick labels and axis labels render through the scene text visual path.
 *
 * Build:  just example-c scatter_axes
 * Run:    ./build/examples/c/techniques/scatter_axes
 */

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "datoviz/app.h"
#include "datoviz/scene.h"
#include "example_common.h"

#define N 2000



/**
 * Return a reproducible random float in [0, 1].
 *
 * @return random float
 */
static float _randf(void)
{
    return (float)rand() / (float)RAND_MAX;
}



/**
 * Fill scatter arrays with synthetic clustered points.
 *
 * @param positions output data-space positions
 * @param colors output RGBA colors
 * @param sizes output diameters in pixels
 */
static void _make_scatter(float positions[N][3], uint8_t colors[N][4], float sizes[N])
{
    for (uint32_t i = 0; i < N; i++)
    {
        float t = (float)i / (float)(N - 1);
        float angle = 8.0f * 3.14159265358979323846f * t;
        float radius = 0.35f + 2.35f * t;
        float x = radius * cosf(angle) + 0.35f * (_randf() - 0.5f);
        float y = radius * sinf(angle) + 0.35f * (_randf() - 0.5f);

        positions[i][0] = x;
        positions[i][1] = y;
        positions[i][2] = 0.0f;

        colors[i][0] = (uint8_t)(40.0f + 215.0f * t);
        colors[i][1] = (uint8_t)(180.0f + 60.0f * _randf());
        colors[i][2] = (uint8_t)(255.0f - 180.0f * t);
        colors[i][3] = 230;

        sizes[i] = 4.0f + 14.0f * _randf();
    }
}



int main(int argc, char** argv)
{
    srand(42);

    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, 1000, 700, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.08f, 0.06f, 0.86f, 0.86f});
    EXAMPLE_CHECK(panel != NULL, "dvz_panel() failed");
    dvz_panel_set_background_color(panel, 0.07f, 0.08f, 0.11f, 1.0f);
    bool ok = dvz_panel_set_layout_reserve(
        panel, &(DvzPanelLayoutReserve){.left = 0.14f, .right = 0.0f, .bottom = 0.18f,
                                        .top = 0.0f});
    EXAMPLE_CHECK(ok, "dvz_panel_set_layout_reserve() failed");

    DvzVisual* visual = dvz_point(scene, 0);
    EXAMPLE_CHECK(visual != NULL, "dvz_point() failed");

    float data_positions[N][3];
    float visual_positions[N][3];
    uint8_t colors[N][4];
    float sizes[N];
    _make_scatter(data_positions, colors, sizes);

    dvz_panel_set_domain(panel, DVZ_DIM_X, -5.0, +5.0);
    dvz_panel_set_domain(panel, DVZ_DIM_Y, -3.0, +3.0);
    int rc =
        dvz_panel_data_to_visual_positions(panel, &data_positions[0][0], &visual_positions[0][0], N);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_data_to_visual_positions() failed");

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = visual_positions, .item_count = N},
        {.attr_name = "color", .data = colors, .item_count = N},
        {.attr_name = "diameter", .data = sizes, .item_count = N},
    };
    rc = dvz_visual_set_data_many(visual, updates, 3);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_data_many() failed");
    rc = dvz_panel_add_visual(panel, visual, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual() failed");

    DvzAxis* x_axis = dvz_panel_axis(panel, DVZ_DIM_X);
    DvzAxis* y_axis = dvz_panel_axis(panel, DVZ_DIM_Y);
    dvz_axis_set_grid(x_axis, true);
    dvz_axis_set_grid(y_axis, true);
    dvz_axis_set_label(x_axis, "x");
    dvz_axis_set_label(y_axis, "y");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzAppWindow* win = dvz_app_window_glfw(app, figure, 1000, 700, "scatter_axes");
    EXAMPLE_CHECK(win != NULL, "dvz_app_window_glfw() failed (GLFW unavailable?)");

    DvzPanzoom* panzoom = dvz_app_window_panel_panzoom(win, panel, NULL);
    EXAMPLE_CHECK(panzoom != NULL, "failed to create or bind panzoom controller");

    dvz_app_run(app, example_frame_count(argc, argv));
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
