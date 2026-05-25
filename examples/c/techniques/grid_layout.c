/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* grid_layout — retained grid/subplot layout smoke example.
 *
 * Opens a GLFW window with four plot panels and one fixed-width colorbar-style panel created from
 * one retained figure grid. Resize the window to verify the colorbar column remains 60 logical
 * pixels wide while plot columns absorb the remaining space.
 *
 * Build:  just example-c grid_layout
 * Run:    ./build/examples/c/techniques/grid_layout
 * Smoke:  ./build/examples/c/techniques/grid_layout 300
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "datoviz/app.h"
#include "datoviz/scene.h"
#include "example_common.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH       1000
#define HEIGHT      760
#define POINT_COLS  14
#define POINT_ROWS  12
#define POINT_COUNT (POINT_COLS * POINT_ROWS)



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Add a colored point grid to one panel.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param red_base base red component
 * @param green_base base green component
 * @param blue_base base blue component
 * @return true on success, false on error
 */
static bool _add_point_grid(
    DvzScene* scene, DvzPanel* panel, uint8_t red_base, uint8_t green_base, uint8_t blue_base)
{
    if (scene == NULL || panel == NULL)
        return false;

    DvzVisual* visual = dvz_point(scene, 0);
    if (visual == NULL)
        return false;

    vec3 positions[POINT_COUNT] = {0};
    DvzColor colors[POINT_COUNT] = {0};
    float sizes[POINT_COUNT] = {0};
    for (uint32_t row = 0; row < POINT_ROWS; row++)
    {
        for (uint32_t col = 0; col < POINT_COLS; col++)
        {
            uint32_t index = row * POINT_COLS + col;
            float x = POINT_COLS > 1 ? (float)col / (float)(POINT_COLS - 1) : 0.0f;
            float y = POINT_ROWS > 1 ? (float)row / (float)(POINT_ROWS - 1) : 0.0f;
            positions[index][0] = -0.82f + 1.64f * x;
            positions[index][1] = -0.78f + 1.56f * y;
            positions[index][2] = 0.0f;
            colors[index][0] = (uint8_t)(red_base + (uint8_t)(42.0f * x));
            colors[index][1] = (uint8_t)(green_base + (uint8_t)(36.0f * y));
            colors[index][2] = (uint8_t)(blue_base + (uint8_t)(28.0f * (1.0f - y)));
            colors[index][3] = 255;
            sizes[index] = 8.0f + 4.0f * sinf((float)(row + col) * 0.31f);
        }
    }

    int rc = dvz_visual_set_data(visual, "position", positions, POINT_COUNT);
    if (rc != 0)
        return false;
    rc = dvz_visual_set_data(visual, "color", colors, POINT_COUNT);
    if (rc != 0)
        return false;
    rc = dvz_visual_set_data(visual, "size", sizes, POINT_COUNT);
    if (rc != 0)
        return false;
    return dvz_panel_add_visual(panel, visual, NULL) == 0;
}




/**
 * Attach one panzoom controller to each plot panel.
 *
 * @param scene scene owning the controllers
 * @param win app view owning the input router
 * @param panels plot panels
 * @param count panel count
 * @return true on success, false on error
 */
static bool _attach_panzoom(DvzScene* scene, DvzView* win, DvzPanel** panels, uint32_t count)
{
    if (scene == NULL || win == NULL || panels == NULL)
        return false;

    DvzInputRouter* router = dvz_view_input(win);
    if (router == NULL)
        return false;

    for (uint32_t i = 0; i < count; i++)
    {
        DvzController* controller = dvz_panzoom(scene, NULL);
        if (controller == NULL)
            return false;
        int rc = dvz_panel_bind_controller(panels[i], controller, DVZ_DIM_MASK_XY);
        if (rc != 0)
            return false;
        rc = dvz_panel_connect_input(panels[i], router);
        if (rc != 0)
            return false;
    }
    return true;
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

    DvzGrid* grid = dvz_figure_grid(figure, 2, 3);
    EXAMPLE_CHECK(grid != NULL, "dvz_figure_grid() failed");
    bool ok = dvz_grid_set_margins(
        grid, &(DvzPanelReserve){.left_px = 24.0f, .right_px = 24.0f, .top_px = 24.0f,
                                 .bottom_px = 24.0f});
    EXAMPLE_CHECK(ok, "dvz_grid_set_margins() failed");
    ok = dvz_grid_set_gutter(grid, 12.0f, 12.0f);
    EXAMPLE_CHECK(ok, "dvz_grid_set_gutter() failed");
    ok = dvz_grid_col_size(grid, 2, DVZ_GRID_SIZE_FIXED_PX, 60.0f);
    EXAMPLE_CHECK(ok, "dvz_grid_col_size() failed");

    DvzPanel* panels[4] = {
        dvz_grid_panel(grid, 0, 0),
        dvz_grid_panel(grid, 0, 1),
        dvz_grid_panel(grid, 1, 0),
        dvz_grid_panel(grid, 1, 1),
    };
    DvzPanel* colorbar_panel = dvz_grid_panel_span(grid, 0, 2, 2, 1);
    EXAMPLE_CHECK(colorbar_panel != NULL, "dvz_grid_panel_span() failed");
    for (uint32_t i = 0; i < 4; i++)
        EXAMPLE_CHECK(panels[i] != NULL, "dvz_grid_panel() failed");

    dvz_panel_set_background_color(panels[0], 0.045f, 0.060f, 0.075f, 1.0f);
    dvz_panel_set_background_color(panels[1], 0.070f, 0.055f, 0.050f, 1.0f);
    dvz_panel_set_background_color(panels[2], 0.055f, 0.060f, 0.045f, 1.0f);
    dvz_panel_set_background_color(panels[3], 0.050f, 0.050f, 0.070f, 1.0f);
    ok = dvz_panel_set_background(
        colorbar_panel,
        &(DvzPanelBackgroundDesc){
            .type = DVZ_PANEL_BACKGROUND_LINEAR_GRADIENT,
            .gradient =
                {
                    .start = {0.5f, 1.0f},
                    .end = {0.5f, 0.0f},
                    .color0 = {0.10f, 0.14f, 0.32f, 1.0f},
                    .color1 = {0.98f, 0.72f, 0.18f, 1.0f},
                },
        });
    EXAMPLE_CHECK(ok, "dvz_panel_set_background() failed");

    ok = _add_point_grid(scene, panels[0], 55, 85, 180);
    EXAMPLE_CHECK(ok, "_add_point_grid(panel 0) failed");
    ok = _add_point_grid(scene, panels[1], 145, 65, 95);
    EXAMPLE_CHECK(ok, "_add_point_grid(panel 1) failed");
    ok = _add_point_grid(scene, panels[2], 80, 125, 80);
    EXAMPLE_CHECK(ok, "_add_point_grid(panel 2) failed");
    ok = _add_point_grid(scene, panels[3], 120, 90, 170);
    EXAMPLE_CHECK(ok, "_add_point_grid(panel 3) failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "grid_layout");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    ok = _attach_panzoom(scene, win, panels, 4);
    EXAMPLE_CHECK(ok, "panzoom setup failed");

    dvz_app_run(app, example_frame_count(argc, argv));
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
