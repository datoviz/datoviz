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
/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/



/**
 * Add a compact multiline annotation block.
 *
 * @param panel target panel
 * @return true on success
 */
static bool _add_text_block(DvzPanel* panel)
{
    DvzText* text = dvz_text(panel, 0);
    if (text == NULL)
        return false;

    DvzColor color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT);
    DvzTextStyle style = dvz_text_style();
    style.size_px = 30.0f;
    style.renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS;
    style.color[0] = color.r;
    style.color[1] = color.g;
    style.color[2] = color.b;
    style.color[3] = color.a;
    if (dvz_text_set_style(text, &style) != 0)
        return false;

    DvzTextLayout layout = dvz_text_layout();
    layout.line_height = 1.18f;
    layout.line_gap_px = 8.0f;
    if (dvz_text_set_layout(text, &layout) != 0)
        return false;

    DvzTextPlacement placement = dvz_text_placement();
    placement.mode = DVZ_TEXT_PLACEMENT_SCREEN;
    placement.anchor = DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT;
    placement.position[0] = 138.0;
    placement.position[1] = 245.0;
    placement.position[2] = 0.0;
    placement.text_anchor[0] = 0.0f;
    placement.text_anchor[1] = 0.0f;
    placement.has_text_anchor = true;
    if (dvz_text_set_placement(text, &placement) != 0)
        return false;

    return dvz_text_set_string(
               text,
               "Retained text can hold a compact note that reads\n"
               "like ordinary prose across multiple explicit lines.\n"
               "The whole paragraph remains one scene-owned string,\n"
               "so placement, style, and updates stay together.") == 0;
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
