/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* text - retained semantic text objects lowered to glyph visuals.
 *
 * Scenario: visual.text
 * Style: visuals, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c visuals/text
 * Run:    ./build/examples/c/visuals/text
 * Smoke:  ./build/examples/c/visuals/text 1
 * PNG:    DVZ_CAPTURE=png ./build/examples/c/visuals/text 1
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_assertions.h"
#include "datoviz/app.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH      1600u
#define HEIGHT     1200u
#define TEXT_COUNT 5u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Add one retained text object.
 *
 * @param panel target panel
 * @param string UTF-8 text string
 * @param x screen X coordinate in logical pixels
 * @param y screen Y coordinate in logical pixels
 * @param size text size in logical pixels
 * @param angle text angle in radians
 * @param role graphite-cyan color role
 * @return true when the object was added
 */
static bool _add_text(
    DvzPanel* panel, const char* string, float x, float y, float size, float angle,
    ExampleStyleColorRole role)
{
    DvzText* text = dvz_text(panel, 0);
    if (text == NULL)
        return false;

    DvzColor color = example_graphite_cyan_color(role);
    DvzTextStyle style = dvz_text_style();
    style.size_px = size;
    style.renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS;
    style.color[0] = color.r;
    style.color[1] = color.g;
    style.color[2] = color.b;
    style.color[3] = color.a;
    if (dvz_text_set_style(text, &style) != 0)
        return false;

    DvzTextPlacement placement = dvz_text_placement();
    placement.mode = DVZ_TEXT_PLACEMENT_SCREEN;
    placement.anchor = DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT;
    placement.position[0] = x;
    placement.position[1] = y;
    placement.position[2] = 0.0f;
    placement.text_anchor[0] = 0.0f;
    placement.text_anchor[1] = 0.5f;
    placement.has_text_anchor = true;
    placement.angle = angle;
    dvz_text_set_placement(text, &placement);
    dvz_text_set_string(text, string);
    return true;
}



/**
 * Add the retained text examples to one panel.
 *
 * @param panel target panel
 * @return true when all text objects were added
 */
static bool _add_texts(DvzPanel* panel)
{
    ANN(panel);

    const char* strings[TEXT_COUNT] = {
        "Retained text",       "semantic strings, panel anchored",
        "MSDF atlas renderer", "screen placement in logical pixels",
        "rotated label",
    };
    const float x[TEXT_COUNT] = {128.0f, 132.0f, 134.0f, 136.0f, 1040.0f};
    const float y[TEXT_COUNT] = {210.0f, 344.0f, 454.0f, 544.0f, 730.0f};
    const float sizes[TEXT_COUNT] = {84.0f, 46.0f, 38.0f, 30.0f, 42.0f};
    const float angles[TEXT_COUNT] = {0.0f, 0.0f, 0.0f, 0.0f, -0.34f};
    const ExampleStyleColorRole roles[TEXT_COUNT] = {
        EXAMPLE_STYLE_COLOR_TEXT,
        EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
        EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
        EXAMPLE_STYLE_COLOR_MINOR_TICK,
        EXAMPLE_STYLE_COLOR_WARNING,
    };

    for (uint32_t i = 0; i < TEXT_COUNT; i++)
    {
        if (!_add_text(panel, strings[i], x[i], y[i], sizes[i], angles[i], roles[i]))
            return false;
    }
    return true;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the retained semantic text example.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    const uint32_t frame_count = example_frame_count_any(argc, argv);
    DvzAppCaptureConfig capture = dvz_app_capture_config_from_env("visual_text");

    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;
    DvzView* win = NULL;

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");
    example_graphite_cyan_set_panel_background(panel);

    EXAMPLE_CHECK(_add_texts(panel), "text setup failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "visual_text");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    EXAMPLE_CHECK(
        example_run_with_capture(app, win, frame_count, &capture),
        "example_run_with_capture() failed");
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
