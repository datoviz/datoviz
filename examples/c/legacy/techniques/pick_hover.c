/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* pick_hover - interactive point-query stress test.
 *
 * Opens a GLFW window showing a point grid.
 * Move the cursor over the panel to hover the frontmost point.
 * Click a point to toggle retained selection; click the background to clear it.
 * Left-drag to pan, right-drag or scroll to zoom, double-click to reset.
 *
 * Build:  just build
 * Run:    ./build/examples/c/techniques/pick_hover
 */

#include <stdint.h>

#include "datoviz/app.h"
#include "datoviz/scene.h"
#include "example_common.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH       1024
#define HEIGHT      1024
#define GRID_COLS   20
#define GRID_ROWS   15
#define POINT_COUNT (GRID_COLS * GRID_ROWS)
#define BASE_SIZE   40.0f
#define HOVER_SIZE  60.0f



/*************************************************************************************************/
/*  Functions                                                                                    */
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

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel() failed");

    DvzVisual* visual = dvz_point(scene, 0);
    EXAMPLE_CHECK(visual != NULL, "dvz_point() failed");

    vec3 positions[POINT_COUNT] = {0};
    DvzColor colors[POINT_COUNT] = {0};
    float sizes[POINT_COUNT] = {0};
    for (uint32_t row = 0; row < GRID_ROWS; row++)
    {
        for (uint32_t col = 0; col < GRID_COLS; col++)
        {
            uint32_t index = row * GRID_COLS + col;
            positions[index][0] = -0.95f + 1.90f * ((float)col / (float)(GRID_COLS - 1));
            positions[index][1] = -0.90f + 1.80f * ((float)row / (float)(GRID_ROWS - 1));
            positions[index][2] = 0.0f;

            colors[index] = dvz_color_rgb(
                (uint8_t)(40 + (215 * col) / (GRID_COLS - 1)),
                (uint8_t)(70 + (160 * row) / (GRID_ROWS - 1)),
                (uint8_t)(220 - (120 * row) / (GRID_ROWS - 1)));
            sizes[index] = BASE_SIZE;
        }
    }

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = colors, .item_count = POINT_COUNT},
        {.attr_name = "diameter_px", .data = sizes, .item_count = POINT_COUNT},
    };
    int rc = dvz_visual_set_data_many(visual, updates, 3);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_data_many() failed");
    dvz_visual_set_query_capabilities(visual, DVZ_QUERY_CAPABILITY_ITEM);

    rc = dvz_panel_add_visual(panel, visual, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual() failed");

    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.05f, 0.07f, 0.10f, 1.0f));

    DvzItemInteraction* pick = dvz_item_interaction(panel, NULL);
    EXAMPLE_CHECK(pick != NULL, "dvz_item_interaction() failed");
    DvzItemStateVisualStyle hover_style = dvz_item_state_visual_style();
    hover_style.visual_flags = DVZ_ITEM_STATE_VISUAL_SCALE;
    hover_style.scale = HOVER_SIZE / BASE_SIZE;
    rc = dvz_hover_set_visual_style(dvz_item_interaction_hover(pick), &hover_style);
    EXAMPLE_CHECK(rc == 0, "dvz_hover_set_visual_style() failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "pick_hover");
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
