/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* text - This example places retained semantic text items in panel coordinates.
 *
 * What to look for: each text item provides a string, x/y screen position, font size, angle, and
 * color role, then the text system lowers it to atlas glyphs. Compare the headline, smaller
 * annotations, and rotated label to see how titles, units, and short scientific notes can be added
 * without managing glyph quads manually.
 *
 * Scenario: visual.text
 * Style: visuals, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c visuals/text
 * Run:    ./build/examples/c/visuals/text --live
 * Smoke:  ./build/examples/c/visuals/text --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_assertions.h"
#include "datoviz/scene.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT
#define TEXT_COUNT 5u



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_visual_text_scenario(void);



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

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
    const float sizes[TEXT_COUNT] = {64.0f, 36.0f, 30.0f, 24.0f, 32.0f};
    const float angles[TEXT_COUNT] = {0.0f, 0.0f, 0.0f, 0.0f, -0.34f};
    const ExampleStyleColorRole roles[TEXT_COUNT] = {
        EXAMPLE_STYLE_COLOR_TEXT,
        EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
        EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
        EXAMPLE_STYLE_COLOR_MINOR_TICK,
        EXAMPLE_STYLE_COLOR_WARNING,
    };

    DvzText* text = dvz_text(panel, 0);
    if (text == NULL)
        return false;
    DvzTextStyle style = dvz_text_style();
    style.renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS;
    if (dvz_text_set_style(text, &style) != 0)
        return false;
    DvzTextPlacement placement = dvz_text_placement();
    placement.mode = DVZ_TEXT_PLACEMENT_SCREEN;
    placement.anchor = DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT;
    if (dvz_text_set_placement(text, &placement) != 0)
        return false;

    DvzTextItem items[TEXT_COUNT] = {0};
    for (uint32_t i = 0; i < TEXT_COUNT; i++)
    {
        DvzColor color = example_graphite_cyan_color(roles[i]);
        items[i] = (DvzTextItem){DVZ_STRUCT_INIT_FIELDS(DvzTextItem),
            .string = strings[i],
            .position = {x[i], y[i], 0.0},
            .anchor = {0.0f, 0.5f},
            .size_px = sizes[i],
            .color = color,
            .angle = angles[i]};
    }
    return dvz_text_set_items(text, items, TEXT_COUNT) == 0;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the retained semantic text scenario.
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

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL)
        return false;
    example_graphite_cyan_set_panel_background(panel);
    return _add_texts(panel);
}



/**
 * Return the semantic text scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_visual_text_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "visual_text",
        .title = "Text",
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
 * Run the retained semantic text example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_visual_text_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
