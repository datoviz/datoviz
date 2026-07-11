/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* This example places a screen-space readout card above a simple data trace.
 *
 * What to look for: the background path uploads position, color, and stroke_width_px arrays in
 * view coordinates, and a highlighted point marks the sample being described. The overlay card is
 * anchored to the panel's top-right corner with explicit padding, width, text size, and text
 * renderer. This pattern is useful for lightweight status readouts that should stay readable while
 * the underlying data view changes.
 *
 * Scenario: features_overlay_card
 * Style: features, graphite_cyan, 1280x720 window target
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
#include <math.h>

#include "datoviz/scene.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_example_overlay_card_scenario(void);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT
#define PATH_COUNT  72u
#define POINT_COUNT 1u
/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Add a small signal and highlighted sample behind the overlay.
 *
 * @param scene scene owning visuals
 * @param panel panel receiving visuals
 * @return true on success
 */
static bool _add_signal(DvzScene* scene, DvzPanel* panel)
{
    if (scene == NULL || panel == NULL)
        return false;

    vec3 positions[PATH_COUNT] = {{0}};
    DvzColor colors[PATH_COUNT] = {{0}};
    float widths[PATH_COUNT] = {0};
    for (uint32_t i = 0; i < PATH_COUNT; i++)
    {
        const float t = PATH_COUNT > 1u ? (float)i / (float)(PATH_COUNT - 1u) : 0.0f;
        const float x = -0.86f + 1.72f * t;
        positions[i][0] = x;
        positions[i][1] = 0.16f * sinf(6.2831853f * (1.6f * t + 0.08f)) +
                          0.10f * cosf(6.2831853f * (3.1f * t - 0.15f));
        positions[i][2] = 0.0f;
        colors[i] = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY);
        colors[i].a = 235u;
        widths[i] = 3.0f;
    }

    DvzVisual* path = dvz_path(scene, 0);
    if (path == NULL)
        return false;
    DvzVisualDataUpdate path_updates[] = {
        {.attr_name = "position", .data = positions, .item_count = PATH_COUNT},
        {.attr_name = "color", .data = colors, .item_count = PATH_COUNT},
        {.attr_name = "stroke_width_px", .data = widths, .item_count = PATH_COUNT},
    };
    if (dvz_visual_set_data_many(path, path_updates, 3) != 0)
        return false;
    if (dvz_path_set_caps(path, DVZ_SEGMENT_CAP_ROUND, DVZ_SEGMENT_CAP_ROUND) != 0)
        return false;
    DvzVisualAttachDesc attach = dvz_visual_attach_desc();
    attach.coord_space = DVZ_VISUAL_COORD_VIEW;
    if (dvz_panel_add_visual(panel, path, &attach) != 0)
        return false;

    const vec3 point_pos[POINT_COUNT] = {{0.22f, 0.13f, 0.0f}};
    const DvzColor point_color[POINT_COUNT] = {
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING)};
    const float diameters[POINT_COUNT] = {42.0f};
    DvzVisual* point = dvz_point(scene, 0);
    if (point == NULL)
        return false;
    DvzVisualDataUpdate point_updates[] = {
        {.attr_name = "position", .data = point_pos, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = point_color, .item_count = POINT_COUNT},
        {.attr_name = "diameter_px", .data = diameters, .item_count = POINT_COUNT},
    };
    if (dvz_visual_set_data_many(point, point_updates, 3) != 0)
        return false;
    if (dvz_visual_set_depth_test(point, false) != 0)
        return false;
    return dvz_panel_add_visual(panel, point, &attach) == 0;
}



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
    desc.text = "selected sample 42   value 0.43   status stable";
    desc.placement = DVZ_OVERLAY_CARD_PLACEMENT_TOP_RIGHT;
    desc.offset_px[0] = 24.0f;
    desc.offset_px[1] = 24.0f;
    DvzOverlayCard* card = dvz_overlay_card(overlay, &desc);
    return card != NULL && dvz_overlay_card_set_style(card, &style) == 0;
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

    if (!_add_signal(ctx->scene, panel))
        return false;
    return _add_overlay_card(panel);
}



/**
 * Return the overlay-card scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_example_overlay_card_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "features_overlay_card",
        .title = "Overlay Card",
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
 * Run the overlay-card feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_example_overlay_card_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
