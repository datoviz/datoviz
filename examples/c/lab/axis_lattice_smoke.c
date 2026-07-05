/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* axis_lattice_smoke - manual panzoom/axis alignment smoke tool.
 *
 * Scenario: axis_lattice_smoke
 * Style: tools, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c tools/axis_lattice_smoke
 * Run:    ./build/examples/c/lab/axis_lattice_smoke
 * Smoke:  ./build/examples/c/lab/axis_lattice_smoke probe_resize
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "datoviz/app.h"
#include "datoviz/canvas.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT
#define GRID_MAX   20u
#define GRID_SIDE  (GRID_MAX + 1u)
#define GRID_COUNT (GRID_SIDE * GRID_SIDE)



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Fill a 0..20 integer lattice in data coordinates.
 *
 * @param positions output data-space positions
 * @param colors output point colors
 * @param diameters output point diameters in pixels
 */
static void _fill_lattice(vec3* positions, DvzColor* colors, float* diameters)
{
    ANN(positions);
    ANN(colors);
    ANN(diameters);

    for (uint32_t j = 0; j < GRID_SIDE; j++)
    {
        for (uint32_t i = 0; i < GRID_SIDE; i++)
        {
            uint32_t idx = j * GRID_SIDE + i;
            positions[idx][0] = (float)i;
            positions[idx][1] = (float)j;
            positions[idx][2] = 0.0f;

            bool major = (i % 5u == 0) || (j % 5u == 0);
            colors[idx] = major ? dvz_color_rgba(235, 190, 96, 255) :
                                  dvz_color_rgba(72, 194, 222, 180);
            diameters[idx] = major ? 12.0f : 7.0f;
        }
    }
}



/**
 * Fill the single probe point used by the offscreen alignment check.
 *
 * @param positions output data-space position
 * @param colors output point color
 * @param diameters output point diameter_px in pixels
 */
static void _fill_probe_point(vec3* positions, DvzColor* colors, float* diameters, bool raw)
{
    ANN(positions);
    ANN(colors);
    ANN(diameters);

    positions[0][0] = raw ? 0.0f : 10.0f;
    positions[0][1] = raw ? -0.103f : 10.0f;
    positions[0][2] = 0.0f;
    colors[0] = dvz_color_rgba(255, 190, 64, 255);
    diameters[0] = 17.0f;
}



/**
 * Upload point arrays to a retained point visual.
 *
 * @param visual point visual
 * @param positions visual-space positions
 * @param colors point colors
 * @param diameters point diameters in pixels
 * @param count point count
 * @return true when uploads succeed
 */
static bool _upload_lattice(
    DvzVisual* visual, vec3* positions, DvzColor* colors, float* diameters, uint32_t count)
{
    ANN(visual);
    ANN(positions);
    ANN(colors);
    ANN(diameters);

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = count},
        {.attr_name = "color", .data = colors, .item_count = count},
        {.attr_name = "diameter_px", .data = diameters, .item_count = count},
    };
    return dvz_visual_set_data_many(visual, updates, 3) == 0;
}



typedef struct
{
    double x;
    double y;
    uint32_t count;
} ProbeCentroid;



/**
 * Return whether one captured pixel belongs to the probe point.
 *
 * @param px RGBA pixel
 * @return whether the pixel is point-colored
 */
static bool _probe_is_point_pixel(const uint8_t* px)
{
    ANN(px);
    return px[0] > 180 && px[1] > 110 && px[1] < 230 && px[2] < 120;
}



/**
 * Return whether one captured pixel belongs to the horizontal y-grid.
 *
 * @param px RGBA pixel
 * @return whether the pixel is y-grid-colored
 */
static bool _probe_is_y_grid_pixel(const uint8_t* px)
{
    ANN(px);
    return px[1] > 150 && px[0] < 100 && px[2] < 130;
}



/**
 * Return whether one captured pixel belongs to the vertical x-grid.
 *
 * @param px RGBA pixel
 * @return whether the pixel is x-grid-colored
 */
static bool _probe_is_x_grid_pixel(const uint8_t* px)
{
    ANN(px);
    return px[2] > 150 && px[0] < 100 && px[1] < 130;
}



/**
 * Return a pointer to one captured RGBA pixel.
 *
 * @param rgba captured RGBA buffer
 * @param width capture width
 * @param x pixel x coordinate
 * @param y pixel y coordinate
 * @return pointer to the pixel
 */
static const uint8_t* _probe_pixel_at(const uint8_t* rgba, uint32_t width, uint32_t x, uint32_t y)
{
    ANN(rgba);
    return &rgba[4 * ((uint64_t)y * width + x)];
}



/**
 * Find the color centroid of the probe point in a capture.
 *
 * @param rgba captured RGBA buffer
 * @param width capture width
 * @param height capture height
 * @param out output centroid
 * @return whether a point centroid was found
 */
static bool _probe_measure_point(
    const uint8_t* rgba, uint32_t width, uint32_t height, ProbeCentroid* out)
{
    ANN(rgba);
    ANN(out);

    double sx = 0.0;
    double sy = 0.0;
    uint32_t count = 0;
    for (uint32_t y = 0; y < height; y++)
    {
        for (uint32_t x = 0; x < width; x++)
        {
            if (!_probe_is_point_pixel(_probe_pixel_at(rgba, width, x, y)))
                continue;
            sx += (double)x;
            sy += (double)y;
            count++;
        }
    }
    if (count == 0)
        return false;

    out->x = sx / (double)count;
    out->y = sy / (double)count;
    out->count = count;
    return true;
}



/**
 * Find the strongest horizontal y-grid row near the probe point.
 *
 * @param rgba captured RGBA buffer
 * @param width capture width
 * @param height capture height
 * @param point probe point centroid
 * @return row index, or -1 when no row was found
 */
static int32_t _probe_measure_y_grid_row(
    const uint8_t* rgba, uint32_t width, uint32_t height, ProbeCentroid point)
{
    ANN(rgba);

    int32_t cy = (int32_t)(point.y + 0.5);
    int32_t min_y = cy - 80;
    int32_t max_y = cy + 80;
    if (min_y < 0)
        min_y = 0;
    if (max_y >= (int32_t)height)
        max_y = (int32_t)height - 1;

    for (int32_t radius = 0; radius <= 80; radius++)
    {
        int32_t candidates[2] = {cy - radius, cy + radius};
        uint32_t candidate_count = radius == 0 ? 1u : 2u;
        for (uint32_t ci = 0; ci < candidate_count; ci++)
        {
            int32_t y = candidates[ci];
            if (y < min_y || y > max_y)
                continue;
            uint32_t score = 0;
            for (uint32_t x = 0; x < width; x++)
            {
                if (_probe_is_y_grid_pixel(_probe_pixel_at(rgba, width, x, (uint32_t)y)))
                    score++;
            }
            if (score > 10)
                return y;
        }
    }
    return -1;
}



/**
 * Find the strongest vertical x-grid column near the probe point.
 *
 * @param rgba captured RGBA buffer
 * @param width capture width
 * @param height capture height
 * @param point probe point centroid
 * @return column index, or -1 when no column was found
 */
static int32_t _probe_measure_x_grid_col(
    const uint8_t* rgba, uint32_t width, uint32_t height, ProbeCentroid point)
{
    ANN(rgba);

    int32_t cx = (int32_t)(point.x + 0.5);
    int32_t min_x = cx - 80;
    int32_t max_x = cx + 80;
    if (min_x < 0)
        min_x = 0;
    if (max_x >= (int32_t)width)
        max_x = (int32_t)width - 1;

    for (int32_t radius = 0; radius <= 80; radius++)
    {
        int32_t candidates[2] = {cx - radius, cx + radius};
        uint32_t candidate_count = radius == 0 ? 1u : 2u;
        for (uint32_t ci = 0; ci < candidate_count; ci++)
        {
            int32_t x = candidates[ci];
            if (x < min_x || x > max_x)
                continue;
            uint32_t score = 0;
            for (uint32_t y = 0; y < height; y++)
            {
                if (_probe_is_x_grid_pixel(_probe_pixel_at(rgba, width, (uint32_t)x, y)))
                    score++;
            }
            if (score > 10)
                return x;
        }
    }
    return -1;
}



/**
 * Capture the offscreen view and print measured probe-grid alignment.
 *
 * @param app app to advance
 * @param win offscreen view
 * @return true when probe capture and measurement succeed
 */
static bool _probe_capture_alignment(DvzApp* app, DvzView* win)
{
    ANN(app);
    ANN(win);

    DvzCanvas* canvas = dvz_view_canvas(win);
    if (canvas == NULL)
        return false;

    uint32_t width = 0, height = 0;
    uint8_t* rgba = NULL;
    for (uint32_t frame = 0; frame < 3; frame++)
    {
        dvz_app_run(app, 1);
        if (rgba != NULL)
            dvz_free(rgba);
        rgba = NULL;
        if (dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) != 0)
            return false;
    }

    ProbeCentroid point = {0};
    bool ok = rgba != NULL && _probe_measure_point(rgba, width, height, &point);
    int32_t y_grid = ok ? _probe_measure_y_grid_row(rgba, width, height, point) : -1;
    int32_t x_grid = ok ? _probe_measure_x_grid_col(rgba, width, height, point) : -1;
    ok = ok && x_grid >= 0 && y_grid >= 0;
    if (ok)
    {
        dvz_fprintf(
            stdout,
            "axis_lattice_probe point=(%.2f, %.2f, n=%u) grid=(%d, %d) "
            "delta=(%.2f, %.2f)\n",
            point.x, point.y, point.count, x_grid, y_grid, point.x - (double)x_grid,
            point.y - (double)y_grid);
    }
    (void)dvz_view_capture_png(win, "/tmp/axis_lattice_probe.png");
    dvz_free(rgba);
    return ok;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the live integer lattice axis-alignment smoke tool.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;
    vec3* data_positions = NULL;
    DvzColor* colors = NULL;
    float* diameters = NULL;
    bool probe = argc >= 2 && strcmp(argv[1], "probe") == 0;
    bool raw_probe = argc >= 2 && strcmp(argv[1], "probe_raw") == 0;
    bool event_probe = argc >= 2 && strcmp(argv[1], "probe_events") == 0;
    bool resize_probe = argc >= 2 && strcmp(argv[1], "probe_resize") == 0;
    raw_probe = raw_probe || event_probe;
    probe = probe || raw_probe || resize_probe;
    uint32_t point_count = probe ? 1u : GRID_COUNT;

    data_positions = (vec3*)dvz_calloc(point_count, sizeof(*data_positions));
    colors = (DvzColor*)dvz_calloc(point_count, sizeof(*colors));
    diameters = (float*)dvz_calloc(point_count, sizeof(*diameters));
    EXAMPLE_CHECK(
        data_positions != NULL && colors != NULL && diameters != NULL,
        "lattice allocation failed");

    if (probe)
        _fill_probe_point(data_positions, colors, diameters, raw_probe);
    else
        _fill_lattice(data_positions, colors, diameters);

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");
    example_graphite_cyan_set_panel_background(panel);

    int rc = 0;
    if (!raw_probe)
    {
        rc = dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, (double)GRID_MAX);
        EXAMPLE_CHECK(rc == 0, "dvz_panel_set_domain() failed for X");
        rc = dvz_panel_set_domain(panel, DVZ_DIM_Y, 0.0, (double)GRID_MAX);
        EXAMPLE_CHECK(rc == 0, "dvz_panel_set_domain() failed for Y");
    }
    DvzVisual* points = dvz_point(scene, 0);
    EXAMPLE_CHECK(points != NULL, "dvz_point() failed");

    DvzAxis* x_axis = dvz_panel_axis(panel, DVZ_DIM_X);
    DvzAxis* y_axis = dvz_panel_axis(panel, DVZ_DIM_Y);
    EXAMPLE_CHECK(x_axis != NULL && y_axis != NULL, "dvz_panel_axis() failed");

    DvzAxisTickPolicy ticks = dvz_axis_tick_policy();
    ticks.target_count = 11;
    ticks.min_pixel_spacing = 75.0f;
    ticks.minor_per_interval = 0;
    bool ok = dvz_axis_set_tick_policy(x_axis, &ticks) == DVZ_OK;
    EXAMPLE_CHECK(ok, "dvz_axis_set_tick_policy() failed for X");
    ok = dvz_axis_set_tick_policy(y_axis, &ticks) == DVZ_OK;
    EXAMPLE_CHECK(ok, "dvz_axis_set_tick_policy() failed for Y");

    DvzAxisStyle x_style = example_graphite_cyan_axis_style(false, NULL);
    DvzAxisStyle y_style = example_graphite_cyan_axis_style(true, NULL);
    if (probe)
    {
        x_style.grid_width = 2.0f;
        y_style.grid_width = 2.0f;
        x_style.grid_color[0] = 0;
        x_style.grid_color[1] = 64;
        x_style.grid_color[2] = 255;
        x_style.grid_color[3] = 255;
        y_style.grid_color[0] = 0;
        y_style.grid_color[1] = 220;
        y_style.grid_color[2] = 80;
        y_style.grid_color[3] = 255;
    }
    ok = dvz_axis_set_style(x_axis, &x_style) == DVZ_OK;
    EXAMPLE_CHECK(ok, "dvz_axis_set_style() failed for X");
    ok = dvz_axis_set_style(y_axis, &y_style) == DVZ_OK;
    EXAMPLE_CHECK(ok, "dvz_axis_set_style() failed for Y");
    ok = dvz_axis_set_grid(x_axis, true) == DVZ_OK;
    EXAMPLE_CHECK(ok, "dvz_axis_set_grid() failed for X");
    ok = dvz_axis_set_grid(y_axis, true) == DVZ_OK;
    EXAMPLE_CHECK(ok, "dvz_axis_set_grid() failed for Y");
    if (!probe)
    {
        ok = dvz_axis_set_label(x_axis, "x") == DVZ_OK;
        EXAMPLE_CHECK(ok, "dvz_axis_set_label() failed for X");
        ok = dvz_axis_set_label(y_axis, "y") == DVZ_OK;
        EXAMPLE_CHECK(ok, "dvz_axis_set_label() failed for Y");
    }

    ok = _upload_lattice(points, data_positions, colors, diameters, point_count);
    EXAMPLE_CHECK(ok, "point lattice upload failed");

    DvzPointStyleDesc point_style = dvz_point_style_desc();
    point_style.aspect = DVZ_SHAPE_ASPECT_FILLED;
    point_style.stroke_width_px = 0.0f;
    rc = dvz_point_set_style(points, &point_style);
    EXAMPLE_CHECK(rc == 0, "dvz_point_set_style() failed");

    rc = dvz_visual_set_depth_test(points, false);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_depth_test() failed");
    rc = dvz_panel_add_visual(panel, points, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual() failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = probe ? dvz_view_offscreen(app, figure, WIDTH, HEIGHT) :
                           dvz_view_window(app, figure, WIDTH, HEIGHT, "axis_lattice_smoke");
    EXAMPLE_CHECK(win != NULL, "view creation failed (GPU/display unavailable?)");

    DvzPanzoom* panzoom = dvz_view_panzoom(win, panel, NULL);
    EXAMPLE_CHECK(panzoom != NULL, "failed to create or bind panzoom controller");

    if (probe)
    {
        if (event_probe)
        {
            int emit_rc = dvz_view_emit_wheel(
                win, 0.5f * (float)WIDTH, 0.5f * (float)HEIGHT, (float)WIDTH, (float)HEIGHT,
                0.0f, 36.0f, 0);
            EXAMPLE_CHECK(emit_rc == 0, "dvz_view_emit_wheel() failed");
            emit_rc = dvz_view_emit_pointer(
                win, DVZ_POINTER_EVENT_DRAG, 0.5f * (float)WIDTH, 0.5f * (float)HEIGHT,
                (float)WIDTH, (float)HEIGHT, DVZ_POINTER_BUTTON_LEFT, 0);
            EXAMPLE_CHECK(emit_rc == 0, "dvz_view_emit_pointer() failed");
        }
        else
        {
            dvz_panzoom_zoom(
                panzoom, raw_probe ? (vec2){1000.0f, 700.0f} : (vec2){1.65f, 1.45f});
            dvz_panzoom_pan(panzoom, raw_probe ? (vec2){0.0f, 0.103f} : (vec2){-0.22f, 0.18f});
        }
        if (resize_probe)
        {
            int resize_rc = dvz_view_resize(win, 900, 700);
            EXAMPLE_CHECK(resize_rc == 0, "dvz_view_resize() failed");
        }
        EXAMPLE_CHECK(_probe_capture_alignment(app, win), "alignment probe capture failed");
    }
    else
    {
        dvz_app_run(app, example_frame_count(argc, argv));
    }
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    dvz_free(diameters);
    dvz_free(colors);
    dvz_free(data_positions);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
