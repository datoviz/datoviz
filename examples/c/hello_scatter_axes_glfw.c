/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* hello_scatter_axes_glfw — interactive 2D scatter plot with WIP axes.
 *
 * Opens a GLFW window showing random discs with per-point color and diameter.
 * Left-drag to pan, right-drag or scroll to zoom, double-click to reset.
 * Axis text labels are retained by the scene API but not rendered yet.
 *
 * Build:  just example-c hello_scatter_axes_glfw
 * Run:    ./build/examples/c/hello_scatter_axes_glfw
 */

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "datoviz/app.h"
#include "datoviz/scene.h"

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
 * Map a data coordinate to visual coordinates for the current WIP axis slice.
 *
 * @param value data value
 * @param min data-domain minimum
 * @param max data-domain maximum
 * @return visual coordinate in [-1, +1]
 */
static float _data_to_visual(float value, float min, float max)
{
    return -1.0f + 2.0f * (value - min) / (max - min);
}



/**
 * Fill scatter arrays with synthetic clustered points.
 *
 * @param positions output visual-space positions
 * @param colors output RGBA colors
 * @param sizes output diameters in pixels
 */
static void _make_scatter(float positions[N][3], uint8_t colors[N][4], float sizes[N])
{
    const float xmin = -5.0f;
    const float xmax = +5.0f;
    const float ymin = -3.0f;
    const float ymax = +3.0f;

    for (uint32_t i = 0; i < N; i++)
    {
        float t = (float)i / (float)(N - 1);
        float angle = 8.0f * 3.14159265358979323846f * t;
        float radius = 0.35f + 2.35f * t;
        float x = radius * cosf(angle) + 0.35f * (_randf() - 0.5f);
        float y = radius * sinf(angle) + 0.35f * (_randf() - 0.5f);

        positions[i][0] = _data_to_visual(x, xmin, xmax);
        positions[i][1] = _data_to_visual(y, ymin, ymax);
        positions[i][2] = 0.0f;

        colors[i][0] = (uint8_t)(40.0f + 215.0f * t);
        colors[i][1] = (uint8_t)(180.0f + 60.0f * _randf());
        colors[i][2] = (uint8_t)(255.0f - 180.0f * t);
        colors[i][3] = 230;

        sizes[i] = 4.0f + 14.0f * _randf();
    }
}



int main(void)
{
    srand(42);

    DvzScene* scene = dvz_scene();
    if (scene == NULL)
    {
        fprintf(stderr, "dvz_scene() failed\n");
        return 1;
    }

    DvzFigure* figure = dvz_figure(scene, 1000, 700, 0);
    if (figure == NULL)
    {
        fprintf(stderr, "dvz_figure() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.08f, 0.06f, 0.86f, 0.86f});
    if (panel == NULL)
    {
        fprintf(stderr, "dvz_panel() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_panel_set_background_color(panel, 0.07f, 0.08f, 0.11f, 1.0f);

    DvzVisual* visual = dvz_point(scene, 0);
    if (visual == NULL)
    {
        fprintf(stderr, "dvz_point() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    float positions[N][3];
    uint8_t colors[N][4];
    float sizes[N];
    _make_scatter(positions, colors, sizes);

    dvz_visual_set_data(visual, "position", positions, N);
    dvz_visual_set_data(visual, "color", colors, N);
    dvz_visual_set_data(visual, "diameter", sizes, N);
    dvz_panel_add_visual(panel, visual, NULL);

    dvz_panel_set_domain(panel, DVZ_DIM_X, -5.0, +5.0);
    dvz_panel_set_domain(panel, DVZ_DIM_Y, -3.0, +3.0);
    DvzAxis* x_axis = dvz_panel_axis(panel, DVZ_DIM_X);
    DvzAxis* y_axis = dvz_panel_axis(panel, DVZ_DIM_Y);
    dvz_axis_set_grid(x_axis, true);
    dvz_axis_set_grid(y_axis, true);
    dvz_axis_set_label(x_axis, "x");
    dvz_axis_set_label(y_axis, "y");

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        fprintf(stderr, "dvz_app() failed (no GPU or display?)\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzAppWindow* win = dvz_app_window_glfw(app, figure, 1000, 700, "hello_scatter_axes_glfw");
    if (win == NULL)
    {
        fprintf(stderr, "dvz_app_window_glfw() failed (GLFW unavailable?)\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }

    dvz_panel_set_panzoom(panel, dvz_app_window_input(win), 0);
    dvz_app_run(app, 0);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}
