/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* offscreen_capture - render an exact-pixel offscreen view once and write a PNG.
 *
 * Scenario: feature.offscreen_capture
 * Style: features, graphite_cyan, 1920x1080 output target
 *
 * Build:  just example-c features/offscreen_capture
 * Run:    ./build/examples/c/features/offscreen_capture
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "datoviz/app.h"
#include "datoviz/canvas/enums.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_OUTPUT_WIDTH
#define HEIGHT EXAMPLE_OUTPUT_HEIGHT
#define POINT_COUNT 4u



/*************************************************************************************************/
/*  Main                                                                                         */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    (void)argc;

    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;

    char png_path[512] = {0};
    example_outpath(argv[0], "offscreen_capture.png", png_path, sizeof(png_path));

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    DvzPanel* panel = figure != NULL ? dvz_panel_full(figure) : NULL;
    DvzVisual* point = dvz_point(scene, 0);
    EXAMPLE_CHECK(
        figure != NULL && panel != NULL && point != NULL, "failed to create scene objects");
    example_graphite_cyan_set_panel_background(panel);

    vec3 positions[POINT_COUNT] = {
        {-0.55f, -0.35f, 0.0f},
        {-0.18f, +0.35f, 0.0f},
        {+0.18f, -0.20f, 0.0f},
        {+0.55f, +0.25f, 0.0f},
    };
    DvzColor colors[POINT_COUNT] = {
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT),
    };
    float diameters[POINT_COUNT] = {38.0f, 54.0f, 44.0f, 62.0f};
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = colors, .item_count = POINT_COUNT},
        {.attr_name = "diameter_px", .data = diameters, .item_count = POINT_COUNT},
    };
    EXAMPLE_CHECK(dvz_visual_set_data_many(point, updates, 3) == 0, "failed to upload point data");
    EXAMPLE_CHECK(dvz_panel_add_visual(panel, point, NULL) == 0, "dvz_panel_add_visual() failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU?)");

    DvzView* view = dvz_view_offscreen(app, figure, WIDTH, HEIGHT);
    EXAMPLE_CHECK(view != NULL, "dvz_view_offscreen() failed");
    uint32_t framebuffer_width = 0;
    uint32_t framebuffer_height = 0;
    dvz_view_framebuffer_size(view, &framebuffer_width, &framebuffer_height);
    EXAMPLE_CHECK(
        framebuffer_width == WIDTH && framebuffer_height == HEIGHT,
        "offscreen framebuffer size must match requested PNG pixels");
    EXAMPLE_CHECK(
        dvz_view_render_once(view) == DVZ_CANVAS_FRAME_READY, "dvz_view_render_once() failed");
    EXAMPLE_CHECK(dvz_view_capture_png(view, png_path) == 0, "dvz_view_capture_png() failed");

    dvz_fprintf(stdout, "offscreen_capture: wrote %s (%ux%u exact pixels)\n", png_path, WIDTH, HEIGHT);
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
