/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* overlay_rich_card - public rich overlay cards lowered through a raster image.
 *
 * Build:  just example-c overlay_rich_card
 * Run:    ./build/examples/c/techniques/overlay_rich_card
 * Smoke:  ./build/examples/c/techniques/overlay_rich_card 120
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>
#include <stdlib.h>

#include "datoviz/app.h"
#include "datoviz/scene.h"
#include "example_common.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH       980
#define HEIGHT      680
#define POINT_COUNT 180



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Fill a deterministic diagonal event trail.
 *
 * @param positions output point positions
 * @param colors output point colors
 * @param diameters output point diameters
 */
static void _fill_event_trail(
    vec3 positions[POINT_COUNT], DvzColor colors[POINT_COUNT], float diameters[POINT_COUNT])
{
    for (uint32_t i = 0; i < POINT_COUNT; i++)
    {
        uint32_t k = (i * 37u) % POINT_COUNT;
        float t = (float)i / (float)(POINT_COUNT - 1);
        float jitter = (float)((k % 17u) - 8.0f) * 0.012f;
        positions[i][0] = -0.82f + 1.64f * t;
        positions[i][1] = -0.48f + 0.92f * t + jitter;
        positions[i][2] = 0.0f;

        if (i < POINT_COUNT / 3)
            colors[i] = dvz_color_rgba(74, 184, 217, 210);
        else if (i < 2u * POINT_COUNT / 3u)
            colors[i] = dvz_color_rgba(247, 187, 84, 225);
        else
            colors[i] = dvz_color_rgba(226, 86, 122, 228);
        diameters[i] = 7.0f + (float)((i * 11u) % 29u);
    }
}


/**
 * Create a compact plain overlay card with GPU text.
 *
 * @param overlay the overlay
 * @return the created card, or NULL on error
 */
static DvzOverlayCard* _add_plain_header(DvzOverlay* overlay)
{
    DvzOverlayCardStyle style = dvz_overlay_card_style();
    style.background_color = dvz_color_rgba(9, 16, 26, 238);
    style.text_color = dvz_color_rgb(241, 246, 255);
    style.padding_px[0] = 14.0f;
    style.padding_px[1] = 8.0f;
    style.height_px = 34.0f;
    style.min_width_px = 290.0f;
    style.glyph_advance_px = 8.1f;
    style.text_size_px = 16.0f;
    style.text_renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS;

    return dvz_overlay_card(
        overlay,
        &(DvzOverlayCardDesc){
            .text = "rich overlay card pipeline",
            .placement = DVZ_OVERLAY_CARD_PLACEMENT_TOP_LEFT,
            .offset_px = {18.0f, 18.0f},
            .style = &style,
        });
}



/*************************************************************************************************/
/*  Main                                                                                         */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.045f, .y = 0.06f, .width = 0.91f, .height = 0.88f});
    EXAMPLE_CHECK(panel != NULL, "dvz_panel() failed");
    dvz_panel_set_background_color(panel, 0.020f, 0.022f, 0.028f, 1.0f);

    DvzVisual* points = dvz_point(scene, 0);
    EXAMPLE_CHECK(points != NULL, "dvz_point() failed");
    vec3 positions[POINT_COUNT] = {0};
    DvzColor colors[POINT_COUNT] = {0};
    float diameters[POINT_COUNT] = {0};
    _fill_event_trail(positions, colors, diameters);
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = colors, .item_count = POINT_COUNT},
        {.attr_name = "diameter", .data = diameters, .item_count = POINT_COUNT},
    };
    int rc = dvz_visual_set_data_many(points, updates, 3);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_data_many() failed");
    rc = dvz_panel_add_visual(panel, points, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual() failed");

    DvzOverlay* overlay = dvz_overlay(panel, 0);
    EXAMPLE_CHECK(overlay != NULL, "dvz_overlay() failed");
    EXAMPLE_CHECK(_add_plain_header(overlay) != NULL, "failed to create header overlay card");

    DvzOverlayCardStyle rich_style = dvz_overlay_card_style();
    rich_style.background_color = dvz_color_rgba(13, 20, 30, 242);
    rich_style.padding_px[0] = 16.0f;
    rich_style.padding_px[1] = 14.0f;
    rich_style.min_width_px = 330.0f;
    DvzOverlayCard* rich = dvz_overlay_card(
        overlay,
        &(DvzOverlayCardDesc){
            .text = "fallback",
            .placement = DVZ_OVERLAY_CARD_PLACEMENT_BOTTOM_RIGHT,
            .offset_px = {22.0f, 22.0f},
            .style = &rich_style,
        });
    EXAMPLE_CHECK(rich != NULL, "failed to create rich overlay card shell");
    rc = dvz_overlay_card_set_rich_text(
        rich,
        &(DvzOverlayRichTextDesc){
            .source =
                "<b>Event packet 42</b> resolved from <color=#4AB8D9>image probe</color>. "
                "<u>Confidence 0.97</u> with <color=#F7BB54>two saturated bins</color> and "
                "<i>stable drift</i> across the last frame window.",
            .max_width_px = 330.0f,
            .char_width_px = 7.2f,
            .line_height_px = 13.5f,
            .scale = 2.0f,
            .text_color = {237, 242, 248, 255},
            .background_color = {0, 0, 0, 0},
        });
    EXAMPLE_CHECK(rc == 0, "failed to set rich overlay card text");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "overlay_rich_card");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzPanzoom* panzoom = dvz_view_panzoom(win, panel, NULL);
    EXAMPLE_CHECK(panzoom != NULL, "failed to create or bind panzoom controller");

    dvz_app_run(app, example_frame_count(argc, argv));
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
