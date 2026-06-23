/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* rich_text_block - private rich text-block lowering without overlay cards.
 *
 * This example intentionally includes the private scene header. It demonstrates the internal
 * text-block backend from TEXT_BLOCK_BACKENDS.md: markup is parsed, measured, CPU-rasterized to
 * RGBA8, and lowered to a normal image visual attached to a panel.
 *
 * Build:  just example-c rich_text_block
 * Run:    ./build/examples/c/techniques/rich_text_block
 * Smoke:  ./build/examples/c/techniques/rich_text_block 120
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "_scene.h"
#include "datoviz/app.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "text/text_internal.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH       980u
#define HEIGHT      680u
#define POINT_COUNT 144u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Fill a deterministic event cloud behind the rich text block.
 *
 * @param positions output point positions
 * @param colors output point colors
 * @param diameters output point diameters
 */
static void _fill_context_points(
    vec3 positions[POINT_COUNT], DvzColor colors[POINT_COUNT], float diameters[POINT_COUNT])
{
    for (uint32_t i = 0; i < POINT_COUNT; i++)
    {
        uint32_t a = (i * 29u) % POINT_COUNT;
        uint32_t b = (i * 47u) % POINT_COUNT;
        float x = -0.92f + 1.84f * (float)a / (float)(POINT_COUNT - 1u);
        float y = -0.58f + 1.16f * (float)b / (float)(POINT_COUNT - 1u);
        positions[i][0] = x;
        positions[i][1] = y;
        positions[i][2] = 0.0f;

        if (i % 4u == 0)
            colors[i] = dvz_color_rgba(64, 168, 214, 180);
        else if (i % 4u == 1)
            colors[i] = dvz_color_rgba(102, 214, 154, 190);
        else if (i % 4u == 2)
            colors[i] = dvz_color_rgba(243, 185, 78, 188);
        else
            colors[i] = dvz_color_rgba(229, 94, 125, 184);
        diameters[i] = 8.0f + (float)((i * 13u) % 24u);
    }
}


/**
 * Add one screen-space MSDF text line.
 *
 * @param panel destination panel
 * @param string text string
 * @param x x position in panel pixels
 * @param y y position in panel pixels
 * @param size text size in pixels
 * @param color text color
 * @return true on success, false on error
 */
static bool _add_screen_text(
    DvzPanel* panel, const char* string, float x, float y, float size, DvzColor color)
{
    DvzText* text = dvz_text(panel, 0);
    if (text == NULL)
        return false;

    int rc = dvz_text_set_style(
        text,
        &(DvzTextStyle){DVZ_STRUCT_INIT_FIELDS(DvzTextStyle),
            .size_px = size,
            .renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS,
            .color = {color.r, color.g, color.b, color.a},
        });
    if (rc != 0)
        return false;

    dvz_text_set_placement(
        text,
        &(DvzTextPlacement){DVZ_STRUCT_INIT_FIELDS(DvzTextPlacement),
            .mode = DVZ_TEXT_PLACEMENT_SCREEN,
            .anchor = DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT,
            .position = {x, y, 0.0f},
            .text_anchor = {0.0f, 0.5f},
            .has_text_anchor = true,
        });
    dvz_text_set_string(text, string);
    return true;
}



/**
 * Place the rich text-block image visual in panel pixels.
 *
 * @param block text block storage
 * @param panel destination panel
 * @return true on success, false on error
 */
static bool _place_rich_text_block(DvzTextBlock* block, DvzPanel* panel)
{
    if (block == NULL || panel == NULL)
        return false;

    float scale = block->raster_scale > 0.0f ? block->raster_scale : 1.0f;
    float block_width = (float)block->raster_width / scale;
    float block_height = (float)block->raster_height / scale;
    float block_x = 54.0f;
    float block_y = 154.0f;
    int rc = _scene_text_block_realize_image(
        block, panel,
        &(DvzTextBlockImageDesc){
            .position_px = {block_x, block_y, 0.0f},
            .extent_px = {block_width, block_height},
            .anchor = {-1.0f, -1.0f},
            .pixel_space = true,
            .z_layer = 2,
            .controller_mode = DVZ_CONTROLLER_FIXED,
        });
    return rc == 0;
}


/**
 * Build and attach the private rich text block as a normal image visual.
 *
 * @param block text block storage
 * @param panel destination panel
 * @return true on success, false on error
 */
static bool _add_rich_text_block(DvzTextBlock* block, DvzPanel* panel)
{
    if (block == NULL || panel == NULL)
        return false;
    DvzScene* scene = panel->figure != NULL ? panel->figure->scene : NULL;
    if (scene == NULL)
        return false;

    _scene_text_block_init(
        block,
        "<b>Private rich text block</b> lowered as an image visual. "
        "<color=#40A8D6>Blue spans</color>, <color=#66D69A>green spans</color>, "
        "<u>underlines</u>, <i>italic slant</i>, and <b><color=#F3B94E>bold color</color></b> "
        "all come from the CPU raster path, not overlay-card GPU glyphs.");

    int rc = _scene_text_block_parse(block);
    if (rc != 0)
        return false;

    rc = _scene_text_block_measure(
        block,
        &(DvzTextBlockLayout){
            .scene = scene,
            .font_size_px = 22.0f,
            .max_width_px = 760.0f,
            .line_height_px = 30.0f,
            .padding_px = {24.0f, 18.0f},
        });
    if (rc != 0)
        return false;

    rc = _scene_text_block_rasterize(
        block,
        &(DvzTextBlockRasterDesc){
            .scene = scene,
            .text_color = {236, 241, 248, 255},
            .background_color = {12, 18, 28, 232},
            .font_size_px = 22.0f,
            .scale = 3.0f,
        });
    if (rc != 0)
        return false;

    return _place_rich_text_block(block, panel);
}



/*************************************************************************************************/
/*  Main                                                                                         */
/*************************************************************************************************/

/**
 * Run the private rich text-block lowering example.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;
    DvzTextBlock block = {0};
    bool block_initialized = false;

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.045f, .y = 0.06f, .width = 0.91f, .height = 0.88f});
    EXAMPLE_CHECK(panel != NULL, "dvz_panel() failed");
    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.018f, 0.022f, 0.030f, 1.0f));

    DvzVisual* points = dvz_point(scene, 0);
    EXAMPLE_CHECK(points != NULL, "dvz_point() failed");
    vec3 positions[POINT_COUNT] = {0};
    DvzColor colors[POINT_COUNT] = {0};
    float diameters[POINT_COUNT] = {0};
    _fill_context_points(positions, colors, diameters);
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = colors, .item_count = POINT_COUNT},
        {.attr_name = "diameter_px", .data = diameters, .item_count = POINT_COUNT},
    };
    int rc = dvz_visual_set_data_many(points, updates, 3);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_data_many() failed");
    rc = dvz_panel_add_visual(panel, points, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual() failed");

    bool ok = _add_screen_text(
        panel, "GPU text visual", 52.0f, 68.0f, 34.0f, dvz_color_rgb(242, 247, 255));
    EXAMPLE_CHECK(ok, "failed to create GPU title text");
    ok = _add_screen_text(
        panel, "Below: private CPU-raster rich text, attached as a regular image visual", 54.0f,
        112.0f, 18.0f, dvz_color_rgb(158, 174, 196));
    EXAMPLE_CHECK(ok, "failed to create GPU subtitle text");

    ok = _add_rich_text_block(&block, panel);
    block_initialized = true;
    EXAMPLE_CHECK(ok, "failed to create private rich text block");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "rich_text_block");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzPanzoom* panzoom = dvz_view_panzoom(win, panel, NULL);
    EXAMPLE_CHECK(panzoom != NULL, "failed to create or bind panzoom controller");

    dvz_app_run(app, example_frame_count(argc, argv));
    ret = 0;

cleanup:
    if (block_initialized)
        _scene_text_block_destroy(&block);
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
