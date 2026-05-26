/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* overlay_card - minimal public overlay-card example.
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

#define POINT_COUNT 4



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

    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.06f, .y = 0.08f, .width = 0.88f, .height = 0.84f});
    EXAMPLE_CHECK(panel != NULL, "dvz_panel() failed");
    dvz_panel_set_background_color(panel, 0.05f, 0.06f, 0.075f, 1.0f);

    DvzVisual* points = dvz_point(scene, 0);
    EXAMPLE_CHECK(points != NULL, "dvz_point() failed");
    vec3 positions[POINT_COUNT] = {
        {-0.55f, -0.35f, 0.0f},
        {-0.10f, +0.30f, 0.0f},
        {+0.28f, -0.15f, 0.0f},
        {+0.60f, +0.38f, 0.0f},
    };
    DvzColor colors[POINT_COUNT] = {
        {236, 98, 85, 255},
        {86, 183, 139, 255},
        {80, 142, 229, 255},
        {241, 194, 83, 255},
    };
    float diameters[POINT_COUNT] = {24.0f, 34.0f, 28.0f, 40.0f};
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
    DvzOverlayCardStyle style = dvz_overlay_card_style();
    style.background_color = dvz_color_rgba(18, 28, 42, 232);
    style.text_color = dvz_color_rgb(245, 248, 255);
    DvzOverlayCard* card = dvz_overlay_card(
        overlay,
        &(DvzOverlayCardDesc){
            .text = "overlay card",
            .anchor_px = {32.0f, 28.0f},
            .offset_px = {0.0f, 0.0f},
            .style = &style});
    EXAMPLE_CHECK(card != NULL, "dvz_overlay_card() failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_glfw(app, figure, 800, 600, "overlay_card");
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
