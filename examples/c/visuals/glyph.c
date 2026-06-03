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

#include <math.h>
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
#define GLYPH_COUNT       9u
#define VERTEX_PER_GLYPH  6u
#define VERTEX_COUNT      (GLYPH_COUNT * VERTEX_PER_GLYPH)
#define ATLAS_GRID        3u
#define ATLAS_CELL_SIZE   64u
#define ATLAS_PADDING_PX  6.0f
#define ATLAS_SIZE        (ATLAS_GRID * ATLAS_CELL_SIZE)

static const float SDF_EDGE_PX = 3.5f;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return the signed distance to a centered rectangle.
 *
 * @param x normalized x in [-1, +1]
 * @param y normalized y in [-1, +1]
 * @param half_width rectangle half-width
 * @param half_height rectangle half-height
 * @return signed distance in normalized cell units
 */
static float _sd_box(float x, float y, float half_width, float half_height)
{
    const float dx = fabsf(x) - half_width;
    const float dy = fabsf(y) - half_height;
    const float outside_x = fmaxf(dx, 0.0f);
    const float outside_y = fmaxf(dy, 0.0f);
    const float outside = sqrtf(outside_x * outside_x + outside_y * outside_y);
    const float inside = fminf(fmaxf(dx, dy), 0.0f);
    return outside + inside;
}



/**
 * Return the signed distance to a simple atlas glyph.
 *
 * @param glyph glyph index
 * @param x normalized x in [-1, +1]
 * @param y normalized y in [-1, +1]
 * @return signed distance in normalized cell units, negative inside
 */
static float _sd_glyph(uint32_t glyph, float x, float y)
{
    switch (glyph % GLYPH_COUNT)
    {
    case 0:
        return sqrtf(x * x + y * y) - 0.58f;
    case 1:
        return _sd_box(x, y, 0.48f, 0.48f);
    case 2:
        return fmaxf(fabsf(x) - 0.12f, fabsf(y) - 0.62f);
    case 3:
        return fmaxf(fabsf(x) + fabsf(y) - 0.72f, -_sd_box(x, y, 0.18f, 0.18f));
    case 4:
        return fminf(_sd_box(x, y, 0.58f, 0.11f), _sd_box(x, y, 0.11f, 0.58f));
    case 5:
        return fabsf(sqrtf(x * x + y * y) - 0.50f) - 0.12f;
    case 6:
        return fminf(_sd_box(x + 0.22f, y, 0.10f, 0.58f), _sd_box(x - 0.22f, y, 0.10f, 0.58f));
    case 7:
        return fminf(
            _sd_box(x, y + 0.22f, 0.58f, 0.10f),
            _sd_box(x, y - 0.22f, 0.58f, 0.10f));
    default:
        return fminf(
            _sd_box(x + y, x - y, 0.13f, 0.74f),
            _sd_box(x - y, x + y, 0.13f, 0.74f));
    }
}



/**
 * Fill a compact RGBA atlas texture with MSDF-compatible single-channel SDF glyphs.
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
            const uint32_t cell_x = x / ATLAS_CELL_SIZE;
            const uint32_t cell_y = y / ATLAS_CELL_SIZE;
            const uint32_t glyph = cell_y * ATLAS_GRID + cell_x;
            const float local_x =
                (float)(x % ATLAS_CELL_SIZE) + 0.5f - 0.5f * (float)ATLAS_CELL_SIZE;
            const float local_y =
                (float)(y % ATLAS_CELL_SIZE) + 0.5f - 0.5f * (float)ATLAS_CELL_SIZE;
            const float scale = 2.0f / ((float)ATLAS_CELL_SIZE - 2.0f * ATLAS_PADDING_PX);
            const float nx = local_x * scale;
            const float ny = local_y * scale;
            const float dist_norm = _sd_glyph(glyph, nx, ny);
            const float dist_px = dist_norm / scale;
            const float sdf = fminf(fmaxf(0.5f - dist_px / SDF_EDGE_PX, 0.0f), 1.0f);
            const uint8_t value = (uint8_t)(255.0f * sdf + 0.5f);
            const uint32_t i = 4u * (y * ATLAS_SIZE + x);
            pixels[i + 0] = value;
            pixels[i + 1] = value;
            pixels[i + 2] = value;
            pixels[i + 3] = value;
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
        const float u0 = (float)col / (float)ATLAS_GRID;
        const float v0 = (float)row / (float)ATLAS_GRID;
        const float u1 = (float)(col + 1u) / (float)ATLAS_GRID;
        const float v1 = (float)(row + 1u) / (float)ATLAS_GRID;
        const vec4 uv = {u0, v0, u1, v1};
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
    if (dvz_visual_set_alpha_mode(glyph, DVZ_ALPHA_BLENDED) != 0)
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
