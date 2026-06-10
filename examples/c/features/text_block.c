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
 * Run:    ./build/examples/c/features/text_block --live
 * Smoke:  ./build/examples/c/features/text_block --png
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
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_example_text_block_scenario(void);



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
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the retained text-block scenario.
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

    return _add_text_block(panel);
}



/**
 * Return the text-block scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_example_text_block_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_text_block",
        .title = "text_block",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_TEXT_VISUAL,
        .init = _scenario_init,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the retained text-block feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_example_text_block_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
