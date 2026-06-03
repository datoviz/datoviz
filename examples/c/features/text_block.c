/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* text_block - compact retained text block with stable screen placement.
 *
 * Scenario: feature.text_block
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/text_block
 * Run:    ./build/examples/c/features/text_block
 * Smoke:  ./build/examples/c/features/text_block 1
 * PNG:    DVZ_CAPTURE=png ./build/examples/c/features/text_block 1
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

#define WIDTH      1600u
#define HEIGHT     1200u
#define LINE_COUNT 5u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Add one line in a retained text block.
 *
 * @param panel target panel
 * @param string text string
 * @param x screen X in logical pixels
 * @param y screen Y in logical pixels
 * @param size text size
 * @param role graphite-cyan color role
 * @return true on success
 */
static bool _add_line(
    DvzPanel* panel, const char* string, float x, float y, float size,
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
    placement.position[2] = 0.0;
    placement.text_anchor[0] = 0.0f;
    placement.text_anchor[1] = 0.5f;
    placement.has_text_anchor = true;
    dvz_text_set_placement(text, &placement);
    dvz_text_set_string(text, string);
    return true;
}



/**
 * Add a compact multiline annotation block.
 *
 * @param panel target panel
 * @return true on success
 */
static bool _add_text_block(DvzPanel* panel)
{
    const char* lines[LINE_COUNT] = {
        "Retained text block",
        "panel anchor: top left",
        "renderer: MSDF atlas",
        "placement: screen pixels",
        "strings stay scene-owned",
    };
    const ExampleStyleColorRole roles[LINE_COUNT] = {
        EXAMPLE_STYLE_COLOR_TEXT,
        EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
        EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
        EXAMPLE_STYLE_COLOR_MINOR_TICK,
        EXAMPLE_STYLE_COLOR_WARNING,
    };
    const float sizes[LINE_COUNT] = {58.0f, 32.0f, 32.0f, 32.0f, 32.0f};

    for (uint32_t i = 0; i < LINE_COUNT; i++)
    {
        if (!_add_line(panel, lines[i], 138.0f, 245.0f + 74.0f * (float)i, sizes[i], roles[i]))
            return false;
    }
    return true;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the retained text-block feature example.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    const uint32_t frame_count = example_frame_count_any(argc, argv);
    DvzAppCaptureConfig capture = dvz_app_capture_config_from_env("feature_text_block");

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

    EXAMPLE_CHECK(_add_text_block(panel), "text block setup failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "text_block");
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
