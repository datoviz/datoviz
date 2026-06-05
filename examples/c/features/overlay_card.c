/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* overlay_card - screen-space overlay card attached to a panel.
 *
 * Scenario: feature.overlay_card
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/overlay_card
 * Run:    ./build/examples/c/features/overlay_card --live
 * Smoke:  ./build/examples/c/features/overlay_card --png
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
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH       1600u
#define HEIGHT      1200u
/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/



/**
 * Add one overlay card.
 *
 * @param panel panel owning the overlay
 * @return true on success
 */
static bool _add_overlay_card(DvzPanel* panel)
{
    DvzOverlay* overlay = dvz_overlay(panel, 0);
    if (overlay == NULL)
        return false;

    DvzOverlayCardStyle style = dvz_overlay_card_style();
    DvzColor background = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_PANEL_BG);
    background.a = 238u;
    DvzColor text = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT);
    style.background_color = background;
    style.text_color = text;
    style.padding_px[0] = 16.0f;
    style.padding_px[1] = 10.0f;
    style.min_width_px = 300.0f;
    style.height_px = 46.0f;
    style.glyph_advance_px = 8.8f;
    style.text_size_px = 18.0f;
    style.text_renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS;
    style.max_text_chars = 96u;

    DvzOverlayCardDesc desc = dvz_overlay_card_desc();
    desc.text = "Overlay card  sample 42  status stable";
    desc.placement = DVZ_OVERLAY_CARD_PLACEMENT_TOP_RIGHT;
    desc.offset_px[0] = 24.0f;
    desc.offset_px[1] = 24.0f;
    desc.style = &style;
    return dvz_overlay_card(overlay, &desc) != NULL;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the overlay-card scenario.
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

    return _add_overlay_card(panel);
}



/**
 * Return the overlay-card scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _overlay_card_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_overlay_card",
        .title = "overlay_card",
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
 * Run the overlay-card feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _overlay_card_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
