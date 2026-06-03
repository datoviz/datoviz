/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* glyph - raw atlas-backed glyph quads with explicit retained attributes.
 *
 * Scenario: visual.glyph
 * Style: visuals, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c visuals/glyph
 * Run:    ./build/examples/c/visuals/glyph
 * Smoke:  ./build/examples/c/visuals/glyph 1
 * PNG:    DVZ_CAPTURE=png ./build/examples/c/visuals/glyph 1
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_assertions.h"
#include "datoviz/app.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH            1600u
#define HEIGHT           1200u
#define GLYPH_COUNT      9u
#define VERTEX_PER_GLYPH 6u
#define VERTEX_COUNT     (GLYPH_COUNT * VERTEX_PER_GLYPH)
#define ATLAS_SIZE       4u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Fill a tiny RGBA atlas texture.
 *
 * @param pixels output tightly packed RGBA8 pixels
 */
static void _fill_atlas(uint8_t pixels[ATLAS_SIZE * ATLAS_SIZE * 4])
{
    ANN(pixels);

    for (uint32_t y = 0; y < ATLAS_SIZE; y++)
    {
        for (uint32_t x = 0; x < ATLAS_SIZE; x++)
        {
            const uint32_t i = 4u * (y * ATLAS_SIZE + x);
            const bool edge = x == 0u || y == 0u || x + 1u == ATLAS_SIZE || y + 1u == ATLAS_SIZE;
            pixels[i + 0] = 255u;
            pixels[i + 1] = 255u;
            pixels[i + 2] = 255u;
            pixels[i + 3] = edge ? 190u : 255u;
        }
    }
}



/**
 * Repeat one glyph item as the six vertices expected by the glyph visual.
 *
 * @param glyph_index glyph item index
 * @param position glyph anchor
 * @param bounds glyph local pixel bounds
 * @param texcoords atlas UV bounds
 * @param color glyph color
 * @param angle glyph angle in radians
 * @param positions output positions
 * @param all_bounds output bounds
 * @param all_texcoords output UV bounds
 * @param colors output colors
 * @param angles output angles
 */
static void _set_glyph(
    uint32_t glyph_index, const vec3 position, const vec4 bounds, const vec4 texcoords,
    DvzColor color, float angle, vec3 positions[VERTEX_COUNT], vec4 all_bounds[VERTEX_COUNT],
    vec4 all_texcoords[VERTEX_COUNT], DvzColor colors[VERTEX_COUNT], float angles[VERTEX_COUNT])
{
    const uint32_t base = glyph_index * VERTEX_PER_GLYPH;
    for (uint32_t i = 0; i < VERTEX_PER_GLYPH; i++)
    {
        positions[base + i][0] = position[0];
        positions[base + i][1] = position[1];
        positions[base + i][2] = position[2];

        all_bounds[base + i][0] = bounds[0];
        all_bounds[base + i][1] = bounds[1];
        all_bounds[base + i][2] = bounds[2];
        all_bounds[base + i][3] = bounds[3];

        all_texcoords[base + i][0] = texcoords[0];
        all_texcoords[base + i][1] = texcoords[1];
        all_texcoords[base + i][2] = texcoords[2];
        all_texcoords[base + i][3] = texcoords[3];

        colors[base + i] = color;
        angles[base + i] = angle;
    }
}



/**
 * Fill a compact raw glyph grid.
 *
 * @param positions output glyph positions
 * @param bounds output glyph bounds
 * @param texcoords output glyph UV bounds
 * @param colors output glyph colors
 * @param angles output glyph angles
 */
static void _fill_glyphs(
    vec3 positions[VERTEX_COUNT], vec4 bounds[VERTEX_COUNT], vec4 texcoords[VERTEX_COUNT],
    DvzColor colors[VERTEX_COUNT], float angles[VERTEX_COUNT])
{
    ANN(positions);
    ANN(bounds);
    ANN(texcoords);
    ANN(colors);
    ANN(angles);

    const vec4 uv = {0.0f, 0.0f, 1.0f, 1.0f};
    const ExampleStyleColorRole roles[GLYPH_COUNT] = {
        EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY, EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
        EXAMPLE_STYLE_COLOR_TEXT,           EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
        EXAMPLE_STYLE_COLOR_WARNING,        EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
        EXAMPLE_STYLE_COLOR_TEXT,           EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
        EXAMPLE_STYLE_COLOR_ERROR,
    };

    for (uint32_t i = 0; i < GLYPH_COUNT; i++)
    {
        const uint32_t row = i / 3u;
        const uint32_t col = i % 3u;
        const float px = -0.48f + 0.48f * (float)col;
        const float py = +0.38f - 0.38f * (float)row;
        const float size = 92.0f + 18.0f * (float)((i + row) % 3u);
        const float half = 0.5f * size;
        const vec3 position = {px, py, 0.0f};
        const vec4 rect = {-half, -half, +half, +half};
        DvzColor color = example_graphite_cyan_color(roles[i]);
        color.a = 238u;
        _set_glyph(
            i, position, rect, uv, color, -0.22f + 0.11f * (float)i, positions, bounds, texcoords,
            colors, angles);
    }
}



/**
 * Add the raw glyph visual to one panel.
 *
 * @param scene scene owning the visual
 * @param panel target panel
 * @param pixels atlas pixels
 * @return true when the visual was added
 */
static bool
_add_glyphs(DvzScene* scene, DvzPanel* panel, uint8_t pixels[ATLAS_SIZE * ATLAS_SIZE * 4])
{
    ANN(scene);
    ANN(panel);
    ANN(pixels);

    vec3 positions[VERTEX_COUNT] = {{0}};
    vec4 bounds[VERTEX_COUNT] = {{0}};
    vec4 texcoords[VERTEX_COUNT] = {{0}};
    DvzColor colors[VERTEX_COUNT] = {{0}};
    float angles[VERTEX_COUNT] = {0};
    _fill_glyphs(positions, bounds, texcoords, colors, angles);

    DvzVisual* glyph = dvz_glyph(scene, 0);
    if (glyph == NULL)
        return false;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = VERTEX_COUNT},
        {.attr_name = "bounds", .data = bounds, .item_count = VERTEX_COUNT},
        {.attr_name = "texcoords", .data = texcoords, .item_count = VERTEX_COUNT},
        {.attr_name = "color", .data = colors, .item_count = VERTEX_COUNT},
        {.attr_name = "angle", .data = angles, .item_count = VERTEX_COUNT},
    };
    if (dvz_visual_set_data_many(glyph, updates, 5) != 0)
        return false;
    if (dvz_visual_set_texture(glyph, pixels, ATLAS_SIZE, ATLAS_SIZE) != 0)
        return false;
    if (dvz_visual_set_depth_test(glyph, false) != 0)
        return false;
    return dvz_panel_add_visual(panel, glyph, NULL) == 0;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the raw glyph visual example.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    const uint32_t frame_count = example_frame_count_any(argc, argv);
    DvzAppCaptureConfig capture = dvz_app_capture_config_from_env("visual_glyph");

    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;
    DvzView* win = NULL;
    bool capture_started = false;
    uint8_t pixels[ATLAS_SIZE * ATLAS_SIZE * 4] = {0};
    _fill_atlas(pixels);

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");
    example_graphite_cyan_set_panel_background(panel);

    EXAMPLE_CHECK(_add_glyphs(scene, panel, pixels), "glyph visual setup failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "visual_glyph");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    EXAMPLE_CHECK(dvz_view_capture_start(win, &capture) == 0, "dvz_view_capture_start() failed");
    capture_started = true;

    dvz_app_run(app, frame_count);

    EXAMPLE_CHECK(dvz_view_capture_stop(win) == 0, "dvz_view_capture_stop() failed");
    capture_started = false;
    ret = 0;

cleanup:
    if (capture_started && win != NULL)
        (void)dvz_view_capture_stop(win);
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
