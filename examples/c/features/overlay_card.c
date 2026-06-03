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
 * Run:    ./build/examples/c/features/overlay_card
 * Smoke:  ./build/examples/c/features/overlay_card 1
 * PNG:    DVZ_CAPTURE=png ./build/examples/c/features/overlay_card 1
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
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the overlay-card feature example.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    const uint32_t frame_count = example_frame_count_any(argc, argv);
    DvzAppCaptureConfig capture = dvz_app_capture_config_from_env("feature_overlay_card");

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

    EXAMPLE_CHECK(_add_overlay_card(panel), "overlay card setup failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "overlay_card");
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
