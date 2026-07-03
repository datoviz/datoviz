/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* glyph - low-level font atlas glyph quads, distinct from retained semantic text.
 *
 * Scenario: visual.glyph
 * Style: visuals, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c visuals/glyph
 * Run:    ./build/examples/c/visuals/glyph --live
 * Smoke:  ./build/examples/c/visuals/glyph --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_assertions.h"
#include "datoviz/scene.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT
#define MAX_GLYPHS       32u
#define VERTEX_PER_GLYPH 6u
#define MAX_VERTICES     (MAX_GLYPHS * VERTEX_PER_GLYPH)

static const char* TEXT_STRING = "Datoviz Atlas caf" "\xC3" "\xA9";



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_visual_glyph_scenario(void);



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Decode the next UTF-8 codepoint from a string.
 *
 * @param string UTF-8 string
 * @param inout_index byte index, advanced on success
 * @param out_codepoint decoded codepoint
 * @return whether a codepoint was decoded
 */
static bool _utf8_next(const char* string, uint32_t* inout_index, uint32_t* out_codepoint)
{
    ANN(string);
    ANN(inout_index);
    ANN(out_codepoint);

    const unsigned char* s = (const unsigned char*)string;
    uint32_t i = *inout_index;
    if (s[i] == '\0')
        return false;

    unsigned char c = s[i++];
    if (c < 0x80u)
    {
        *out_codepoint = c;
        *inout_index = i;
        return true;
    }
    if ((c & 0xE0u) == 0xC0u && (s[i] & 0xC0u) == 0x80u)
    {
        *out_codepoint = ((uint32_t)(c & 0x1Fu) << 6) | (uint32_t)(s[i] & 0x3Fu);
        *inout_index = i + 1u;
        return true;
    }
    if (
        (c & 0xF0u) == 0xE0u && (s[i] & 0xC0u) == 0x80u &&
        (s[i + 1u] & 0xC0u) == 0x80u)
    {
        *out_codepoint = ((uint32_t)(c & 0x0Fu) << 12) | ((uint32_t)(s[i] & 0x3Fu) << 6) |
                         (uint32_t)(s[i + 1u] & 0x3Fu);
        *inout_index = i + 2u;
        return true;
    }
    if (
        (c & 0xF8u) == 0xF0u && (s[i] & 0xC0u) == 0x80u &&
        (s[i + 1u] & 0xC0u) == 0x80u && (s[i + 2u] & 0xC0u) == 0x80u)
    {
        *out_codepoint = ((uint32_t)(c & 0x07u) << 18) | ((uint32_t)(s[i] & 0x3Fu) << 12) |
                         ((uint32_t)(s[i + 1u] & 0x3Fu) << 6) |
                         (uint32_t)(s[i + 2u] & 0x3Fu);
        *inout_index = i + 3u;
        return true;
    }

    *out_codepoint = '?';
    *inout_index = i;
    return true;
}


/**
 * Repeat one glyph item as the six vertices expected by the glyph visual.
 *
 * @param vertex first vertex index
 * @param position glyph anchor
 * @param bounds glyph local pixel bounds
 * @param texcoords atlas UV bounds
 * @param color glyph color
 * @param positions output positions
 * @param all_bounds output bounds
 * @param all_texcoords output UV bounds
 * @param colors output colors
 * @param angles output angles
 */
static void _write_glyph(
    uint32_t vertex, const vec3 position, const vec4 bounds, const vec4 texcoords, DvzColor color,
    vec3 positions[MAX_VERTICES], vec4 all_bounds[MAX_VERTICES], vec4 all_texcoords[MAX_VERTICES],
    DvzColor colors[MAX_VERTICES], float angles[MAX_VERTICES])
{
    for (uint32_t i = 0; i < VERTEX_PER_GLYPH; i++)
    {
        const uint32_t dst = vertex + i;
        positions[dst][0] = position[0];
        positions[dst][1] = position[1];
        positions[dst][2] = position[2];

        all_bounds[dst][0] = bounds[0];
        all_bounds[dst][1] = bounds[1];
        all_bounds[dst][2] = bounds[2];
        all_bounds[dst][3] = bounds[3];

        all_texcoords[dst][0] = texcoords[0];
        all_texcoords[dst][1] = texcoords[1];
        all_texcoords[dst][2] = texcoords[2];
        all_texcoords[dst][3] = texcoords[3];

        colors[dst] = color;
        angles[dst] = 0.0f;
    }
}


/**
 * Fill glyph visual attributes from font atlas metrics.
 *
 * @param atlas font atlas
 * @param positions output positions
 * @param bounds output bounds
 * @param texcoords output UV bounds
 * @param colors output colors
 * @param angles output angles
 * @param out_vertex_count output vertex count
 * @return true when at least one glyph was written
 */
static bool _fill_text_glyphs(
    const DvzTextAtlas* atlas, vec3 positions[MAX_VERTICES], vec4 bounds[MAX_VERTICES],
    vec4 texcoords[MAX_VERTICES], DvzColor colors[MAX_VERTICES], float angles[MAX_VERTICES],
    uint32_t* out_vertex_count)
{
    ANN(atlas);
    ANN(positions);
    ANN(bounds);
    ANN(texcoords);
    ANN(colors);
    ANN(angles);
    ANN(out_vertex_count);

    DvzTextAtlasInfo info = dvz_text_atlas_info(atlas);
    const float scale = 1.0f;
    const float line_width = 610.0f;
    const vec3 anchor = {-0.34f, +0.10f, 0.0f};

    const ExampleStyleColorRole roles[4] = {
        EXAMPLE_STYLE_COLOR_TEXT,
        EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
        EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
        EXAMPLE_STYLE_COLOR_WARNING,
    };

    uint32_t vertex = 0;
    uint32_t glyph_index = 0;
    uint32_t byte_index = 0;
    uint32_t cp = 0;
    float cursor_x = 0.0f;
    while (_utf8_next(TEXT_STRING, &byte_index, &cp) && glyph_index < MAX_GLYPHS)
    {
        const DvzTextAtlasGlyph* glyph = dvz_text_atlas_glyph(atlas, cp);
        if (glyph == NULL)
            continue;

        const float advance = glyph->advance * scale;
        if (glyph->width <= 0.0f || glyph->height <= 0.0f)
        {
            cursor_x += advance;
            continue;
        }

        const float x0 = cursor_x + glyph->xoff * scale - 0.5f * line_width;
        const float y0 = -0.5f * info.ascent + info.ascent * scale + glyph->yoff * scale;
        const float x1 = x0 + glyph->width * scale;
        const float y1 = y0 + glyph->height * scale;
        const vec4 rect = {x0, y0, x1, y1};
        const vec4 uv = {glyph->uv[0], glyph->uv[1], glyph->uv[2], glyph->uv[3]};

        DvzColor color = example_graphite_cyan_color(roles[glyph_index % 4u]);
        color.a = 245u;
        _write_glyph(vertex, anchor, rect, uv, color, positions, bounds, texcoords, colors, angles);
        vertex += VERTEX_PER_GLYPH;
        glyph_index++;
        cursor_x += advance;
    }

    *out_vertex_count = vertex;
    return vertex > 0;
}


/**
 * Add the raw glyph visual to one panel.
 *
 * @param scene scene owning the visual
 * @param panel target panel
 * @param atlas font atlas
 * @return true when the visual was added
 */
static bool _add_glyphs(DvzScene* scene, DvzPanel* panel, const DvzTextAtlas* atlas)
{
    ANN(scene);
    ANN(panel);
    ANN(atlas);

    vec3 positions[MAX_VERTICES] = {{0}};
    vec4 bounds[MAX_VERTICES] = {{0}};
    vec4 texcoords[MAX_VERTICES] = {{0}};
    DvzColor colors[MAX_VERTICES] = {{0}};
    float angles[MAX_VERTICES] = {0};
    uint32_t vertex_count = 0;
    if (!_fill_text_glyphs(atlas, positions, bounds, texcoords, colors, angles, &vertex_count))
        return false;

    DvzVisual* glyph = dvz_glyph(scene, 0);
    if (glyph == NULL)
        return false;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = vertex_count},
        {.attr_name = "bounds", .data = bounds, .item_count = vertex_count},
        {.attr_name = "texcoords", .data = texcoords, .item_count = vertex_count},
        {.attr_name = "color", .data = colors, .item_count = vertex_count},
        {.attr_name = "angle", .data = angles, .item_count = vertex_count},
    };
    if (dvz_visual_set_data_many(glyph, updates, 5) != 0)
        return false;
    if (dvz_glyph_set_atlas(glyph, atlas) != 0)
        return false;
    if (dvz_visual_set_depth_test(glyph, false) != 0)
        return false;
    if (dvz_visual_set_alpha_mode(glyph, DVZ_ALPHA_BLENDED) != 0)
        return false;
    return dvz_panel_add_visual(panel, glyph, NULL) == 0;
}


/**
 * Create a font atlas for the example string.
 *
 * @param scene scene owning the font
 * @return generated atlas, or NULL on error
 */
static const DvzTextAtlas* _create_font_atlas(DvzScene* scene)
{
    ANN(scene);

    DvzFontDesc desc = dvz_font_desc();
    desc.family = "Roboto";
    desc.style = "Regular";
    DvzFont* font = dvz_font(scene, &desc);
    if (font == NULL)
        return NULL;

    DvzTextAtlasSpec spec = dvz_text_atlas_spec(DVZ_TEXT_RENDERER_MSDF_ATLAS, 64.0f);
    if (!dvz_font_atlas_ensure_string(font, &spec, TEXT_STRING))
        return NULL;
    return dvz_font_atlas(font, &spec);
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the raw glyph visual scenario.
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

    const DvzTextAtlas* atlas = _create_font_atlas(ctx->scene);
    if (atlas == NULL)
        return false;
    return _add_glyphs(ctx->scene, panel, atlas);
}


/**
 * Return the raw glyph visual scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_visual_glyph_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "visual_glyph",
        .title = "Font Atlas Glyphs",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .init = _scenario_init,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the raw glyph visual example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_visual_glyph_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
