/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* multi_panel — live multi-panel scene smoke example.
 *
 * Opens a GLFW window with four panels backed by one figure. Each panel has its own background,
 * visual, viewport/scissor region, and panzoom controller connected to the same input router.
 *
 * Build:  just example-c multi_panel
 * Run:    ./build/examples/c/techniques/multi_panel
 * Smoke:  ./build/examples/c/techniques/multi_panel 300
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "datoviz/app.h"
#include "datoviz/scene.h"
#include "example_common.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH       1000
#define HEIGHT      800
#define POINT_COLS  12
#define POINT_ROWS  10
#define POINT_COUNT (POINT_COLS * POINT_ROWS)
#define PATH_COUNT  96
#define IMG         32



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
            positions[index][0] = -0.90f + 1.80f * ((float)col / (float)(POINT_COLS - 1));
            positions[index][1] = -0.85f + 1.70f * ((float)row / (float)(POINT_ROWS - 1));
            positions[index][2] = 0.0f;

            colors[index] = dvz_color_rgb(
                (uint8_t)(red_base + (95 * col) / (POINT_COLS - 1)),
                (uint8_t)(green_base + (95 * row) / (POINT_ROWS - 1)), blue_base);
            sizes[index] = 18.0f + 8.0f * ((float)((row + col) % 3) / 2.0f);
        }
    }

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = colors, .item_count = POINT_COUNT},
        {.attr_name = "diameter_px", .data = sizes, .item_count = POINT_COUNT},
    };
    int rc = dvz_visual_set_data_many(visual, updates, 3);
    if (rc != 0)
        return false;
    rc = dvz_panel_add_visual(panel, visual, NULL);
    return rc == 0;
}



/**
 * Add a procedural RGBA image to one panel.
 *
 * @param scene scene owning the visual and field
 * @param panel panel receiving the visual
 * @return true on success, false on error
 */
static bool _add_image_panel(DvzScene* scene, DvzPanel* panel)
{
    if (scene == NULL || panel == NULL)
        return false;

    DvzVisual* visual = dvz_image(scene, 0);
    if (visual == NULL)
        return false;

    vec3 positions[4] = {
        {-0.88f, -0.78f, 0.0f},
        {-0.88f, 0.78f, 0.0f},
        {0.88f, -0.78f, 0.0f},
        {0.88f, 0.78f, 0.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };
    uint8_t pixels[IMG * IMG * 4] = {0};
    for (uint32_t y = 0; y < IMG; y++)
    {
        for (uint32_t x = 0; x < IMG; x++)
        {
            uint32_t i = 4 * (y * IMG + x);
            pixels[i + 0] = (uint8_t)(40 + (190 * x) / (IMG - 1));
            pixels[i + 1] = (uint8_t)(55 + (160 * y) / (IMG - 1));
            pixels[i + 2] = (uint8_t)(190 - (80 * ((x + y) % 8)) / 7);
            pixels[i + 3] = 255;
        }
    }

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_RGBA8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_COLOR,
                   .width = IMG,
                   .height = IMG,
                   .depth = 1,
               });
    if (field == NULL)
        return false;
    if (!dvz_sampled_field_set_data(
            field, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView),
                       .data = pixels,
                       .bytes_per_row = IMG * 4,
                       .rows_per_image = IMG,
                   }))
    {
        return false;
    }

    if (dvz_visual_set_data(visual, "position", positions, 4) != 0 ||
        dvz_visual_set_data(visual, "texcoords", texcoords, 4) != 0 ||
        !dvz_visual_set_field(visual, "field", field))
    {
        return false;
    }
    return dvz_panel_add_visual(panel, visual, NULL) == 0;
}



/**
 * Add a path visual to one panel.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @return true on success, false on error
 */
static bool _add_path_panel(DvzScene* scene, DvzPanel* panel)
{
    if (scene == NULL || panel == NULL)
        return false;

    DvzVisual* visual = dvz_path(scene, 0);
    if (visual == NULL)
        return false;

    vec3 positions[PATH_COUNT] = {0};
    DvzColor colors[PATH_COUNT] = {0};
    for (uint32_t i = 0; i < PATH_COUNT; i++)
    {
        float t = (float)i / (float)(PATH_COUNT - 1);
        float x = -0.92f + 1.84f * t;
        float y = 0.52f * sinf(6.28318530718f * 2.0f * t);
        positions[i][0] = x;
        positions[i][1] = y;
        positions[i][2] = 0.0f;
        colors[i] = dvz_color_rgb(
            (uint8_t)(245 - (120 * i) / (PATH_COUNT - 1)),
            (uint8_t)(140 + (80 * i) / (PATH_COUNT - 1)), 70);
    }

    if (dvz_visual_set_data(visual, "position", positions, PATH_COUNT) != 0 ||
        dvz_visual_set_data(visual, "color", colors, PATH_COUNT) != 0)
    {
        return false;
    }
    return dvz_panel_add_visual(panel, visual, NULL) == 0;
}



/**
 * Attach view panzoom controllers to every panel.
 *
 * @param win view owning the input router
 * @param panels panels to connect
 * @param count number of panels
 * @return true on success, false on error
 */
static bool
_attach_panzoom(DvzView* win, DvzPanel** panels, uint32_t count)
{
    if (win == NULL || panels == NULL)
        return false;

    for (uint32_t i = 0; i < count; i++)
    {
        if (panels[i] == NULL)
            return false;
        DvzPanzoom* panzoom = dvz_view_panzoom(win, panels[i], NULL);
        if (panzoom == NULL)
        {
            return false;
        }
    }
    return true;
}



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

    DvzPanel* panels[4] = {
        dvz_panel(figure, &(DvzPanelDesc){.x = 0.00f, .y = 0.00f, .width = 0.50f, .height = 0.50f}),
        dvz_panel(figure, &(DvzPanelDesc){.x = 0.50f, .y = 0.00f, .width = 0.50f, .height = 0.50f}),
        dvz_panel(figure, &(DvzPanelDesc){.x = 0.00f, .y = 0.50f, .width = 0.50f, .height = 0.50f}),
        dvz_panel(figure, &(DvzPanelDesc){.x = 0.50f, .y = 0.50f, .width = 0.50f, .height = 0.50f}),
    };
    for (uint32_t i = 0; i < 4; i++)
    {
        EXAMPLE_CHECK(panels[i] != NULL, "dvz_panel() failed");
    }

    dvz_panel_set_background_color(panels[0], dvz_color_from_unit(0.045f, 0.060f, 0.075f, 1.0f));
    dvz_panel_set_background_color(panels[1], dvz_color_from_unit(0.070f, 0.055f, 0.050f, 1.0f));
    dvz_panel_set_background_color(panels[2], dvz_color_from_unit(0.055f, 0.060f, 0.045f, 1.0f));
    dvz_panel_set_background_color(panels[3], dvz_color_from_unit(0.050f, 0.050f, 0.070f, 1.0f));

    bool ok = _add_point_grid(scene, panels[0], 55, 85, 210);
    EXAMPLE_CHECK(ok, "_add_point_grid(panel 0) failed");

    ok = _add_image_panel(scene, panels[1]);
    EXAMPLE_CHECK(ok, "_add_image_panel(panel 1) failed");

    ok = _add_path_panel(scene, panels[2]);
    EXAMPLE_CHECK(ok, "_add_path_panel(panel 2) failed");

    ok = _add_point_grid(scene, panels[3], 145, 65, 120);
    EXAMPLE_CHECK(ok, "_add_point_grid(panel 3) failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_window(app, figure, WIDTH, HEIGHT, "multi_panel");
    EXAMPLE_CHECK(win != NULL, "dvz_view_window() failed (GLFW unavailable?)");

    ok = _attach_panzoom(win, panels, 4);
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
