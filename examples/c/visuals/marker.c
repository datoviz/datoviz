/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* marker - retained marker visual with deterministic shape, fill, stroke, and size variation.
 *
 * Scenario: visual.marker
 * Style: visuals, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c visuals/marker
 * Run:    ./build/examples/c/visuals/marker
 * Smoke:  ./build/examples/c/visuals/marker 1
 * PNG:    DVZ_CAPTURE=png ./build/examples/c/visuals/marker 1
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/app.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH       1600u
#define HEIGHT      1200u
#define MARKER_COLS 6u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return the marker shape for one display column.
 *
 * @param index marker column index
 * @return marker shape enum value
 */
static uint32_t _marker_shape(uint32_t index)
{
    switch (index % MARKER_COLS)
    {
    case 1:
        return DVZ_MARKER_SHAPE_SQUARE;
    case 2:
        return DVZ_MARKER_SHAPE_TRIANGLE;
    case 3:
        return DVZ_MARKER_SHAPE_DIAMOND;
    case 4:
        return DVZ_MARKER_SHAPE_CROSS;
    case 5:
        return DVZ_MARKER_SHAPE_RING;
    default:
        return DVZ_MARKER_SHAPE_DISC;
    }
}



/**
 * Return one graphite/cyan fill color role.
 *
 * @param row marker row index
 * @param col marker column index
 * @return marker fill color role
 */
static ExampleStyleColorRole _marker_fill_role(uint32_t row, uint32_t col)
{
    static const ExampleStyleColorRole roles[][MARKER_COLS] = {
        {
            EXAMPLE_STYLE_COLOR_TEXT,
            EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
            EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
            EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
            EXAMPLE_STYLE_COLOR_TEXT,
            EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
        },
        {
            EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
            EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
            EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
            EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
            EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
            EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
        },
        {
            EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
            EXAMPLE_STYLE_COLOR_WARNING,
            EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
            EXAMPLE_STYLE_COLOR_WARNING,
            EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
            EXAMPLE_STYLE_COLOR_ERROR,
        },
    };
    const uint32_t row_count = DVZ_ARRAY_COUNT(roles);
    return roles[row % row_count][col % MARKER_COLS];
}



/**
 * Add one marker row with a visual-level stroke color role.
 *
 * @param scene scene owning the marker visual
 * @param panel panel receiving the marker visual
 * @param row marker row index
 * @param y row y coordinate
 * @param edge_role outline color role for the row
 * @return true when the row was added
 */
static bool
_add_marker_row(
    DvzScene* scene, DvzPanel* panel, uint32_t row, float y, ExampleStyleColorRole edge_role)
{
    DvzVisual* visual = dvz_marker(scene, 0);
    if (visual == NULL)
        return false;

    DvzColor edge_color = example_graphite_cyan_color(edge_role);
    edge_color.a = 245;

    DvzMarkerStyle style = dvz_marker_style();
    style.aspect = DVZ_SHAPE_ASPECT_OUTLINE;
    style.edge_color = edge_color;
    style.stroke_width = 2.25f;
    if (dvz_marker_set_style(visual, &style) != 0)
        return false;

    vec3 positions[MARKER_COLS] = {{0}};
    DvzColor colors[MARKER_COLS] = {{0}};
    float diameters[MARKER_COLS] = {0};
    float angles[MARKER_COLS] = {0};
    uint32_t shapes[MARKER_COLS] = {0};

    for (uint32_t col = 0; col < MARKER_COLS; col++)
    {
        const float t = MARKER_COLS > 1u ? (float)col / (float)(MARKER_COLS - 1u) : 0.0f;
        positions[col][0] = -0.78f + 1.56f * t;
        positions[col][1] = y;
        positions[col][2] = 0.0f;
        colors[col] = example_graphite_cyan_color(_marker_fill_role(row, col));
        colors[col].a = 238;
        diameters[col] = 42.0f + 8.0f * (float)((row + col) % 3u);
        angles[col] = 0.18f * (float)(row + col);
        shapes[col] = _marker_shape(col);
    }

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = MARKER_COLS},
        {.attr_name = "color", .data = colors, .item_count = MARKER_COLS},
        {.attr_name = "diameter", .data = diameters, .item_count = MARKER_COLS},
        {.attr_name = "angle", .data = angles, .item_count = MARKER_COLS},
        {.attr_name = "shape", .data = shapes, .item_count = MARKER_COLS},
    };
    if (dvz_visual_set_data_many(visual, updates, 5) != 0)
        return false;
    return dvz_panel_add_visual(panel, visual, NULL) == 0;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the retained marker visual example.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    const uint32_t frame_count = example_frame_count_any(argc, argv);
    DvzAppCaptureConfig capture = dvz_app_capture_config_from_env("visual_marker");

    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;
    DvzView* win = NULL;
    bool capture_started = false;

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");
    example_graphite_cyan_set_panel_background(panel);

    const float row_y[] = {+0.46f, 0.0f, -0.46f};
    const ExampleStyleColorRole edge_roles[] = {
        EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
        EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
        EXAMPLE_STYLE_COLOR_WARNING,
    };
    const char* row_errors[] = {
        "top marker row setup failed",
        "middle marker row setup failed",
        "bottom marker row setup failed",
    };
    for (uint32_t row = 0; row < DVZ_ARRAY_COUNT(row_y); row++)
    {
        EXAMPLE_CHECK(
            _add_marker_row(scene, panel, row, row_y[row], edge_roles[row]), row_errors[row]);
    }

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "visual_marker");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzPanzoom* panzoom = dvz_view_panzoom(win, panel, NULL);
    EXAMPLE_CHECK(panzoom != NULL, "failed to create or bind panzoom controller");

    EXAMPLE_CHECK(dvz_view_capture_start(win, &capture) == 0, "dvz_view_capture_start() failed");
    capture_started = true;

    dvz_app_run(app, frame_count);

    EXAMPLE_CHECK(dvz_view_capture_stop(win) == 0, "dvz_view_capture_stop() failed");
    capture_started = false;
    ret = 0;

cleanup:
    if (capture_started && win != NULL)
        (void)dvz_view_capture_stop(win);
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
