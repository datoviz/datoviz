/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* overlay_card - public overlay cards with GPU text.
 *
 * Build:  just example-c overlay_card
 * Run:    ./build/examples/c/techniques/overlay_card
 * Smoke:  ./build/examples/c/techniques/overlay_card 120
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
#define POINT_COLS  22
#define POINT_ROWS  15
#define POINT_COUNT (POINT_COLS * POINT_ROWS)



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Fill a deterministic point cloud used as a lively overlay backdrop.
 *
 * @param positions output point positions
 * @param colors output point colors
 * @param diameters output point diameters
 */
static void _fill_sensor_points(
    vec3 positions[POINT_COUNT], DvzColor colors[POINT_COUNT], float diameters[POINT_COUNT])
{
    for (uint32_t row = 0; row < POINT_ROWS; row++)
    {
        for (uint32_t col = 0; col < POINT_COLS; col++)
        {
            uint32_t i = row * POINT_COLS + col;
            float x = -0.88f + 1.76f * (float)col / (float)(POINT_COLS - 1);
            float y = -0.68f + 1.36f * (float)row / (float)(POINT_ROWS - 1);
            float dx = (((col * 17u + row * 11u) % 9u) - 4.0f) * 0.006f;
            float dy = (((col * 7u + row * 19u) % 9u) - 4.0f) * 0.005f;
            positions[i][0] = x + dx;
            positions[i][1] = y + dy;
            positions[i][2] = 0.0f;

            uint32_t band = (col + 2u * row) % 6u;
            if (band == 0)
                colors[i] = dvz_color_rgba(56, 176, 221, 218);
            else if (band == 1)
                colors[i] = dvz_color_rgba(94, 205, 142, 226);
            else if (band == 2)
                colors[i] = dvz_color_rgba(246, 190, 84, 224);
            else if (band == 3)
                colors[i] = dvz_color_rgba(235, 103, 95, 226);
            else if (band == 4)
                colors[i] = dvz_color_rgba(165, 121, 226, 220);
            else
                colors[i] = dvz_color_rgba(238, 238, 232, 196);
            diameters[i] = 8.0f + (float)((col * col + 3u * row) % 17u);
        }
    }
}


/**
 * Create one semantically placed overlay card.
 *
 * @param overlay the overlay
 * @param text card text
 * @param placement semantic placement
 * @param offset_x horizontal inset in logical pixels
 * @param offset_y vertical inset in logical pixels
 * @param background card background color
 * @param foreground card text color
 * @param min_width minimum card width
 * @return the created card, or NULL on error
 */
static DvzOverlayCard* _add_card(
    DvzOverlay* overlay, const char* text, DvzOverlayCardPlacement placement, float offset_x,
    float offset_y, DvzColor background, DvzColor foreground, float min_width)
{
    DvzOverlayCardStyle style = dvz_overlay_card_style();
    style.background_color = background;
    style.text_color = foreground;
    style.padding_px[0] = 13.0f;
    style.padding_px[1] = 8.0f;
    style.min_width_px = min_width;
    style.height_px = 34.0f;
    style.glyph_advance_px = 8.3f;
    style.text_size_px = 16.0f;
    style.text_renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS;
    style.max_text_chars = 128;

    DvzOverlayCard* card = dvz_overlay_card(
        overlay,
        &(DvzOverlayCardDesc){DVZ_STRUCT_INIT_FIELDS(DvzOverlayCardDesc),
            .text = text,
            .placement = placement,
            .offset_px = {offset_x, offset_y},
        });
    if (card == NULL || dvz_overlay_card_set_style(card, &style) != 0)
        return NULL;
    return card;
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
        figure, &(DvzPanelDesc){.x = 0.045f, .y = 0.06f, .width = 0.91f, .height = 0.88f});
    EXAMPLE_CHECK(panel != NULL, "dvz_panel() failed");
    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.018f, 0.022f, 0.030f, 1.0f));

    DvzVisual* points = dvz_point(scene, 0);
    EXAMPLE_CHECK(points != NULL, "dvz_point() failed");
    vec3 positions[POINT_COUNT] = {0};
    DvzColor colors[POINT_COUNT] = {0};
    float diameters[POINT_COUNT] = {0};
    _fill_sensor_points(positions, colors, diameters);
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = colors, .item_count = POINT_COUNT},
        {.attr_name = "diameter_px", .data = diameters, .item_count = POINT_COUNT},
    };
    int rc = dvz_visual_set_data_many(points, updates, 3);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_data_many() failed");
    rc = dvz_panel_add_visual(panel, points, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual() failed");

    DvzOverlay* overlay = dvz_overlay(panel, 0);
    EXAMPLE_CHECK(overlay != NULL, "dvz_overlay() failed");
    EXAMPLE_CHECK(
        _add_card(
            overlay, "SENSOR FIELD A-17", DVZ_OVERLAY_CARD_PLACEMENT_TOP_LEFT, 18.0f,
            18.0f, dvz_color_rgba(8, 14, 24, 238), dvz_color_rgb(241, 247, 255), 230.0f) != NULL,
        "failed to create title overlay card");
    EXAMPLE_CHECK(
        _add_card(
            overlay, "rate 24.8 kHz  drift +0.03", DVZ_OVERLAY_CARD_PLACEMENT_TOP_RIGHT,
            18.0f, 18.0f, dvz_color_rgba(26, 42, 54, 232), dvz_color_rgb(131, 232, 202),
            250.0f) != NULL,
        "failed to create metric overlay card");
    EXAMPLE_CHECK(
        _add_card(
            overlay, "armed channel B7  threshold 0.82", DVZ_OVERLAY_CARD_PLACEMENT_BOTTOM_LEFT,
            18.0f, 18.0f, dvz_color_rgba(42, 24, 32, 235), dvz_color_rgb(255, 198, 117),
            290.0f) != NULL,
        "failed to create status overlay card");
    EXAMPLE_CHECK(
        _add_card(
            overlay, "GPU MSDF overlay text", DVZ_OVERLAY_CARD_PLACEMENT_BOTTOM_RIGHT, 18.0f,
            18.0f, dvz_color_rgba(20, 30, 58, 232), dvz_color_rgb(185, 205, 255), 220.0f) != NULL,
        "failed to create renderer overlay card");
    EXAMPLE_CHECK(
        _add_card(
            overlay, "LOCKED", DVZ_OVERLAY_CARD_PLACEMENT_CENTER, -38.0f, -4.0f,
            dvz_color_rgba(232, 238, 226, 224), dvz_color_rgb(18, 25, 32), 90.0f) != NULL,
        "failed to create center overlay card");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_window(app, figure, WIDTH, HEIGHT, "overlay_card");
    EXAMPLE_CHECK(win != NULL, "dvz_view_window() failed (GLFW unavailable?)");

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
