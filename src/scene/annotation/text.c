/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene text / annotation                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_overflow.h"
#include "_scale_ticks.h"
#include "_scene.h"
#include "datoviz/scene.h"
#include "text_internal.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_TEXT_BITMAP_GLYPH_WIDTH  6u
#define DVZ_TEXT_BITMAP_GLYPH_HEIGHT 8u
#define DVZ_TEXT_BITMAP_LINE_HEIGHT  9u
#define DVZ_TEXT_BITMAP_FIRST_CHAR   32u
#define DVZ_TEXT_BITMAP_GLYPH_COUNT  96u
#define DVZ_TEXT_BITMAP_FALLBACK     63u
#define DVZ_TEXT_BITMAP_ATLAS_COLS   16u
#define DVZ_TEXT_BITMAP_ATLAS_ROWS   6u
#define DVZ_TEXT_BITMAP_ATLAS_PAD    1u
#define DVZ_TEXT_BITMAP_ATLAS_CELL_W (DVZ_TEXT_BITMAP_GLYPH_WIDTH + 2u * DVZ_TEXT_BITMAP_ATLAS_PAD)
#define DVZ_TEXT_BITMAP_ATLAS_CELL_H (DVZ_TEXT_BITMAP_GLYPH_HEIGHT + 2u * DVZ_TEXT_BITMAP_ATLAS_PAD)



/*************************************************************************************************/
/*  Glyphs                                                                                       */
/*************************************************************************************************/

/* Built-in 6x8 monochrome glyphs for printable ASCII, used as the deterministic small-text path. */
static const uint8_t _text_font_6x8[DVZ_TEXT_BITMAP_GLYPH_COUNT * 6] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0xE3, 0x84, 0x10, 0x01, 0x00,
    0x6D, 0xB4, 0x80, 0x00, 0x00, 0x00, 0x00, 0xA7, 0xCA, 0x29, 0xF2, 0x80,
    0x20, 0xE4, 0x0C, 0x09, 0xC1, 0x00, 0x65, 0x90, 0x84, 0x21, 0x34, 0xC0,
    0x21, 0x45, 0x08, 0x55, 0x23, 0x40, 0x30, 0xC2, 0x00, 0x00, 0x00, 0x00,
    0x10, 0x82, 0x08, 0x20, 0x81, 0x00, 0x20, 0x41, 0x04, 0x10, 0x42, 0x00,
    0x00, 0xA3, 0x9F, 0x38, 0xA0, 0x00, 0x00, 0x41, 0x1F, 0x10, 0x40, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xC3, 0x08, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xC3, 0x00, 0x00, 0x10, 0x84, 0x21, 0x00, 0x00,
    0x39, 0x14, 0xD5, 0x65, 0x13, 0x80, 0x10, 0xC1, 0x04, 0x10, 0x43, 0x80,
    0x39, 0x10, 0x46, 0x21, 0x07, 0xC0, 0x39, 0x10, 0x4E, 0x05, 0x13, 0x80,
    0x08, 0x62, 0x92, 0x7C, 0x20, 0x80, 0x7D, 0x04, 0x1E, 0x05, 0x13, 0x80,
    0x18, 0x84, 0x1E, 0x45, 0x13, 0x80, 0x7C, 0x10, 0x84, 0x20, 0x82, 0x00,
    0x39, 0x14, 0x4E, 0x45, 0x13, 0x80, 0x39, 0x14, 0x4F, 0x04, 0x23, 0x00,
    0x00, 0x03, 0x0C, 0x00, 0xC3, 0x00, 0x00, 0x03, 0x0C, 0x00, 0xC3, 0x08,
    0x08, 0x42, 0x10, 0x20, 0x40, 0x80, 0x00, 0x07, 0xC0, 0x01, 0xF0, 0x00,
    0x20, 0x40, 0x81, 0x08, 0x42, 0x00, 0x39, 0x10, 0x46, 0x10, 0x01, 0x00,
    0x39, 0x15, 0xD5, 0x5D, 0x03, 0x80, 0x39, 0x14, 0x51, 0x7D, 0x14, 0x40,
    0x79, 0x14, 0x5E, 0x45, 0x17, 0x80, 0x39, 0x14, 0x10, 0x41, 0x13, 0x80,
    0x79, 0x14, 0x51, 0x45, 0x17, 0x80, 0x7D, 0x04, 0x1E, 0x41, 0x07, 0xC0,
    0x7D, 0x04, 0x1E, 0x41, 0x04, 0x00, 0x39, 0x14, 0x17, 0x45, 0x13, 0xC0,
    0x45, 0x14, 0x5F, 0x45, 0x14, 0x40, 0x38, 0x41, 0x04, 0x10, 0x43, 0x80,
    0x04, 0x10, 0x41, 0x45, 0x13, 0x80, 0x45, 0x25, 0x18, 0x51, 0x24, 0x40,
    0x41, 0x04, 0x10, 0x41, 0x07, 0xC0, 0x45, 0xB5, 0x51, 0x45, 0x14, 0x40,
    0x45, 0x95, 0x53, 0x45, 0x14, 0x40, 0x39, 0x14, 0x51, 0x45, 0x13, 0x80,
    0x79, 0x14, 0x5E, 0x41, 0x04, 0x00, 0x39, 0x14, 0x51, 0x55, 0x23, 0x40,
    0x79, 0x14, 0x5E, 0x49, 0x14, 0x40, 0x39, 0x14, 0x0E, 0x05, 0x13, 0x80,
    0x7C, 0x41, 0x04, 0x10, 0x41, 0x00, 0x45, 0x14, 0x51, 0x45, 0x13, 0x80,
    0x45, 0x14, 0x51, 0x44, 0xA1, 0x00, 0x45, 0x15, 0x55, 0x55, 0x52, 0x80,
    0x45, 0x12, 0x84, 0x29, 0x14, 0x40, 0x45, 0x14, 0x4A, 0x10, 0x41, 0x00,
    0x78, 0x21, 0x08, 0x41, 0x07, 0x80, 0x38, 0x82, 0x08, 0x20, 0x83, 0x80,
    0x01, 0x02, 0x04, 0x08, 0x10, 0x00, 0x38, 0x20, 0x82, 0x08, 0x23, 0x80,
    0x10, 0xA4, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F,
    0x30, 0xC1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x81, 0x3D, 0x13, 0xC0,
    0x41, 0x07, 0x91, 0x45, 0x17, 0x80, 0x00, 0x03, 0x91, 0x41, 0x13, 0x80,
    0x04, 0x13, 0xD1, 0x45, 0x13, 0xC0, 0x00, 0x03, 0x91, 0x79, 0x03, 0x80,
    0x18, 0x82, 0x1E, 0x20, 0x82, 0x00, 0x00, 0x03, 0xD1, 0x44, 0xF0, 0x4E,
    0x41, 0x07, 0x12, 0x49, 0x24, 0x80, 0x10, 0x01, 0x04, 0x10, 0x41, 0x80,
    0x08, 0x01, 0x82, 0x08, 0x24, 0x8C, 0x41, 0x04, 0x94, 0x61, 0x44, 0x80,
    0x10, 0x41, 0x04, 0x10, 0x41, 0x80, 0x00, 0x06, 0x95, 0x55, 0x14, 0x40,
    0x00, 0x07, 0x12, 0x49, 0x24, 0x80, 0x00, 0x03, 0x91, 0x45, 0x13, 0x80,
    0x00, 0x07, 0x91, 0x45, 0x17, 0x90, 0x00, 0x03, 0xD1, 0x45, 0x13, 0xC1,
    0x00, 0x05, 0x89, 0x20, 0x87, 0x00, 0x00, 0x03, 0x90, 0x38, 0x13, 0x80,
    0x00, 0x87, 0x88, 0x20, 0xA1, 0x00, 0x00, 0x04, 0x92, 0x49, 0x62, 0x80,
    0x00, 0x04, 0x51, 0x44, 0xA1, 0x00, 0x00, 0x04, 0x51, 0x55, 0xF2, 0x80,
    0x00, 0x04, 0x92, 0x31, 0x24, 0x80, 0x00, 0x04, 0x92, 0x48, 0xE1, 0x18,
    0x00, 0x07, 0x82, 0x31, 0x07, 0x80, 0x18, 0x82, 0x18, 0x20, 0x81, 0x80,
    0x10, 0x41, 0x00, 0x10, 0x41, 0x00, 0x30, 0x20, 0x83, 0x08, 0x23, 0x00,
    0x29, 0x40, 0x00, 0x00, 0x00, 0x00, 0x10, 0xE6, 0xD1, 0x45, 0xF0, 0x00,
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return the resolved text size in pixels.
 *
 * @param style the text style
 * @param fallback_size_px fallback text size in pixels
 * @return the resolved size
 */
static float _text_style_size_px(const DvzTextStyle* style, float fallback_size_px)
{
    ANN(style);
    float size_px = style->size_px;
    if (size_px <= 0.0f)
        size_px = fallback_size_px > 0.0f ? fallback_size_px : dvz_font_defaults().text_size_px;
    return size_px;
}



/**
 * Resolve a public text renderer to an internal atlas backend.
 *
 * @param renderer the requested text renderer
 * @param style the text style
 * @param fallback_size_px fallback text size in pixels
 * @return the internal atlas backend
 */
static DvzTextAtlasBackend _text_renderer_backend(
    DvzTextRenderer renderer, const DvzTextStyle* style, float fallback_size_px)
{
    ANN(style);
    if (renderer == DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS)
        return DVZ_TEXT_ATLAS_BACKEND_BUILTIN_BITMAP;
    if (renderer == DVZ_TEXT_RENDERER_BITMAP_ATLAS)
    {
#if defined(DVZ_HAS_FREETYPE) && DVZ_HAS_FREETYPE
        return DVZ_TEXT_ATLAS_BACKEND_FREETYPE_BITMAP;
#else
        return DVZ_TEXT_ATLAS_BACKEND_BUILTIN_BITMAP;
#endif
    }
    if (renderer == DVZ_TEXT_RENDERER_MSDF_ATLAS)
        return DVZ_TEXT_ATLAS_BACKEND_MSDF;
    if (renderer == DVZ_TEXT_RENDERER_AUTO)
    {
        if (_text_style_size_px(style, fallback_size_px) < 14.0f)
        {
#if defined(DVZ_HAS_FREETYPE) && DVZ_HAS_FREETYPE
            return DVZ_TEXT_ATLAS_BACKEND_FREETYPE_BITMAP;
#else
            return DVZ_TEXT_ATLAS_BACKEND_BUILTIN_BITMAP;
#endif
        }
        return DVZ_TEXT_ATLAS_BACKEND_MSDF;
    }
    return DVZ_TEXT_ATLAS_BACKEND_BUILTIN_BITMAP;
}



/**
 * Return the scene default SDF font, creating it lazily.
 *
 * @param scene the scene
 * @return the default font, or NULL on allocation failure
 */
static DvzFont* _text_default_sdf_font(DvzScene* scene)
{
    ANN(scene);
    DvzFontDesc desc = scene->font_defaults.sans;
    if (desc.family == NULL || desc.family[0] == '\0')
        desc.family = "Roboto";
    if (desc.style == NULL || desc.style[0] == '\0')
        desc.style = "Regular";
    for (uint32_t i = 0; i < scene->font_count; i++)
    {
        bool path_matches = false;
        if (desc.path == NULL || desc.path[0] == '\0')
            path_matches = scene->fonts[i].path[0] == '\0';
        else
            path_matches = strcmp(scene->fonts[i].path, desc.path) == 0;

        if (
            path_matches && strcmp(scene->fonts[i].family, desc.family) == 0 &&
            strcmp(scene->fonts[i].style, desc.style) == 0)
            return &scene->fonts[i];
    }
    return dvz_font(scene, &desc);
}



/**
 * Resolve the font used by an SDF text style.
 *
 * @param scene the scene
 * @param style the text style
 * @return a scene-owned font, or NULL on failure
 */
static DvzFont* _text_sdf_font(DvzScene* scene, const DvzTextStyle* style)
{
    ANN(scene);
    ANN(style);
    if (style->font != NULL)
        return style->font;
    return _text_default_sdf_font(scene);
}


/**
 * Return the font-owned atlas for a requested spec after ensure has run.
 *
 * @param font the font
 * @param spec requested atlas spec
 * @return the resolved atlas, including fallback atlases
 */
static DvzTextAtlas* _text_font_atlas(DvzFont* font, const DvzTextAtlasSpec* spec)
{
    ANN(font);
    ANN(spec);
    return _scene_text_atlas_get(font, spec);
}



/**
 * Resolve the style color, using opaque white for zero-initialized styles.
 *
 * @param style the text style
 * @param out_color the output RGBA color
 */
void _text_style_color(const DvzTextStyle* style, uint8_t out_color[4])
{
    ANN(style);
    ANN(out_color);
    out_color[0] = style->color[0];
    out_color[1] = style->color[1];
    out_color[2] = style->color[2];
    out_color[3] = style->color[3];
    if (out_color[3] == 0)
    {
        if (out_color[0] == 0 && out_color[1] == 0 && out_color[2] == 0)
        {
            out_color[0] = 255;
            out_color[1] = 255;
            out_color[2] = 255;
        }
        out_color[3] = 255;
    }
}



/**
 * Return the integer pixel scale for the built-in 6x8 bitmap font.
 *
 * @param style the text style
 * @return the pixel scale
 */
static uint32_t _text_bitmap_scale(const DvzTextStyle* style)
{
    ANN(style);
    float size_px = style->size_px;
    if (size_px <= 0)
        size_px = 12.0f;
    uint32_t scale = (uint32_t)floorf(size_px / 8.0f + 0.5f);
    if (scale == 0)
        scale = 1;
    if (scale > 32)
        scale = 32;
    return scale;
}



/**
 * Return the continuous layout scale for atlas-backed bitmap glyph quads.
 *
 * @param style the text style
 * @return the floating-point layout scale
 */
static float _text_bitmap_layout_scale(const DvzTextStyle* style)
{
    ANN(style);
    float size_px = style->size_px;
    if (size_px <= 0)
        size_px = 12.0f;
    float scale = size_px / 8.0f;
    if (scale < 0.25f)
        scale = 0.25f;
    if (scale > 32.0f)
        scale = 32.0f;
    return scale;
}



/**
 * Return whether a built-in font glyph bit is set.
 *
 * @param ascii the ASCII codepoint
 * @param x the glyph-local x pixel
 * @param y the glyph-local y pixel
 * @return whether the bit is set
 */
static bool _text_font_bit(uint32_t ascii, uint32_t x, uint32_t y)
{
    if (ascii < DVZ_TEXT_BITMAP_FIRST_CHAR ||
        ascii >= DVZ_TEXT_BITMAP_FIRST_CHAR + DVZ_TEXT_BITMAP_GLYPH_COUNT)
        ascii = DVZ_TEXT_BITMAP_FALLBACK;
    uint32_t glyph = ascii - DVZ_TEXT_BITMAP_FIRST_CHAR;
    uint32_t bit = y * DVZ_TEXT_BITMAP_GLYPH_WIDTH + x;
    uint8_t byte = _text_font_6x8[6 * glyph + bit / 8u];
    return (byte & (uint8_t)(0x80u >> (bit % 8u))) != 0;
}



/**
 * Convert an input byte to a printable bitmap glyph codepoint.
 *
 * @param byte the input byte
 * @return the printable ASCII codepoint
 */
static uint32_t _text_printable_ascii(uint32_t codepoint)
{
    if (codepoint >= DVZ_TEXT_BITMAP_FIRST_CHAR &&
        codepoint < DVZ_TEXT_BITMAP_FIRST_CHAR + DVZ_TEXT_BITMAP_GLYPH_COUNT)
        return codepoint;
    return DVZ_TEXT_BITMAP_FALLBACK;
}



/**
 * Decode one UTF-8 codepoint, replacing malformed input with '?'.
 *
 * @param string the UTF-8 string
 * @param inout_index byte index, advanced by the consumed sequence
 * @param out_codepoint output Unicode codepoint
 * @return whether a codepoint was decoded
 */
static bool _text_utf8_next(
    const char* string, uint32_t* inout_index, uint32_t* out_codepoint)
{
    ANN(string);
    ANN(inout_index);
    ANN(out_codepoint);
    uint32_t i = *inout_index;
    if (i >= DVZ_SCENE_LABEL_SIZE || string[i] == '\0')
        return false;

    const uint8_t* s = (const uint8_t*)string;
    uint8_t b0 = s[i];
    if (b0 < 0x80u)
    {
        *out_codepoint = b0;
        *inout_index = i + 1;
        return true;
    }

    uint32_t needed = 0;
    uint32_t cp = 0;
    uint32_t min_cp = 0;
    if ((b0 & 0xE0u) == 0xC0u)
    {
        needed = 2;
        cp = b0 & 0x1Fu;
        min_cp = 0x80u;
    }
    else if ((b0 & 0xF0u) == 0xE0u)
    {
        needed = 3;
        cp = b0 & 0x0Fu;
        min_cp = 0x800u;
    }
    else if ((b0 & 0xF8u) == 0xF0u)
    {
        needed = 4;
        cp = b0 & 0x07u;
        min_cp = 0x10000u;
    }
    else
    {
        *out_codepoint = DVZ_TEXT_BITMAP_FALLBACK;
        *inout_index = i + 1;
        return true;
    }

    for (uint32_t j = 1; j < needed; j++)
    {
        if (i + j >= DVZ_SCENE_LABEL_SIZE || string[i + j] == '\0' ||
            (s[i + j] & 0xC0u) != 0x80u)
        {
            *out_codepoint = DVZ_TEXT_BITMAP_FALLBACK;
            *inout_index = i + 1;
            return true;
        }
        cp = (cp << 6) | (uint32_t)(s[i + j] & 0x3Fu);
    }

    if (cp < min_cp || cp > 0x10FFFFu || (cp >= 0xD800u && cp <= 0xDFFFu))
        cp = DVZ_TEXT_BITMAP_FALLBACK;
    *out_codepoint = cp;
    *inout_index = i + needed;
    return true;
}



/**
 * Measure a string in built-in bitmap font cells.
 *
 * @param string the string
 * @param out_columns output maximum line columns
 * @param out_lines output line count
 * @param out_visible output visible glyph count
 */
static void _text_measure_cells(
    const char* string, uint32_t* out_columns, uint32_t* out_lines, uint32_t* out_visible)
{
    ANN(out_columns);
    ANN(out_lines);
    ANN(out_visible);
    uint32_t columns = 0;
    uint32_t max_columns = 0;
    uint32_t lines = 1;
    uint32_t visible = 0;
    if (string != NULL)
    {
        uint32_t i = 0;
        uint32_t cp = 0;
        while (_text_utf8_next(string, &i, &cp))
        {
            if (cp == '\n')
            {
                if (columns > max_columns)
                    max_columns = columns;
                columns = 0;
                lines++;
                continue;
            }
            uint32_t advance = cp == '\t' ? 4u : 1u;
            columns += advance;
            visible += advance;
        }
    }
    if (columns > max_columns)
        max_columns = columns;
    *out_columns = max_columns;
    *out_lines = lines;
    *out_visible = visible;
}



/**
 * Paint one scaled built-in bitmap glyph into an RGBA texture.
 *
 * @param rgba the texture data
 * @param width the texture width
 * @param x0 the glyph origin x
 * @param y0 the glyph origin y
 * @param scale the integer bitmap scale
 * @param ascii the printable ASCII codepoint
 * @param color the RGBA color
 */
static void _text_paint_glyph(
    uint8_t* rgba, uint32_t width, uint32_t x0, uint32_t y0, uint32_t scale, uint32_t ascii,
    const uint8_t color[4])
{
    ANN(rgba);
    ANN(color);
    for (uint32_t gy = 0; gy < DVZ_TEXT_BITMAP_GLYPH_HEIGHT; gy++)
    {
        for (uint32_t gx = 0; gx < DVZ_TEXT_BITMAP_GLYPH_WIDTH; gx++)
        {
            if (!_text_font_bit(ascii, gx, gy))
                continue;
            for (uint32_t sy = 0; sy < scale; sy++)
            {
                for (uint32_t sx = 0; sx < scale; sx++)
                {
                    uint32_t px = x0 + gx * scale + sx;
                    uint32_t py = y0 + gy * scale + sy;
                    uint64_t index = ((uint64_t)py * width + px) * 4u;
                    rgba[index + 0] = color[0];
                    rgba[index + 1] = color[1];
                    rgba[index + 2] = color[2];
                    rgba[index + 3] = color[3];
                }
            }
        }
    }
}



/**
 * Return the shared scene-owned bitmap glyph atlas field.
 *
 * @param scene the scene
 * @return the atlas field, or NULL on failure
 */
static DvzSampledField* _text_bitmap_atlas_field(DvzScene* scene)
{
    ANN(scene);
    if (scene->text_bitmap_atlas != NULL)
        return scene->text_bitmap_atlas;

    uint32_t width = DVZ_TEXT_BITMAP_ATLAS_COLS * DVZ_TEXT_BITMAP_ATLAS_CELL_W;
    uint32_t height = DVZ_TEXT_BITMAP_ATLAS_ROWS * DVZ_TEXT_BITMAP_ATLAS_CELL_H;
    uint64_t pixel_count = 0;
    uint64_t byte_size = 0;
    if (_dvz_mul_u64_overflows(width, height, &pixel_count) ||
        _dvz_mul_u64_overflows(pixel_count, 4u, &byte_size) || byte_size > SIZE_MAX)
    {
        log_error("text bitmap atlas size overflow");
        return NULL;
    }
    uint8_t* rgba = (uint8_t*)dvz_calloc((DvzSize)byte_size, 1);
    if (rgba == NULL)
    {
        log_error("text bitmap atlas allocation failed");
        return NULL;
    }

    const uint8_t white[4] = {255, 255, 255, 255};
    for (uint32_t glyph = 0; glyph < DVZ_TEXT_BITMAP_GLYPH_COUNT; glyph++)
    {
        uint32_t col = glyph % DVZ_TEXT_BITMAP_ATLAS_COLS;
        uint32_t row = glyph / DVZ_TEXT_BITMAP_ATLAS_COLS;
        _text_paint_glyph(
            rgba, width, col * DVZ_TEXT_BITMAP_ATLAS_CELL_W + DVZ_TEXT_BITMAP_ATLAS_PAD,
            row * DVZ_TEXT_BITMAP_ATLAS_CELL_H + DVZ_TEXT_BITMAP_ATLAS_PAD, 1,
            glyph + DVZ_TEXT_BITMAP_FIRST_CHAR, white);
    }

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_RGBA8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_COLOR,
                   .width = width,
                   .height = height,
                   .depth = 1,
               });
    if (field == NULL ||
        !dvz_sampled_field_set_data(
            field, &(DvzFieldDataView){
                       .data = rgba,
                       .bytes_per_row = (uint64_t)width * 4u,
                       .rows_per_image = height,
                   }))
    {
        dvz_free(rgba);
        return NULL;
    }

    scene->text_bitmap_atlas = field;
    dvz_free(rgba);
    return field;
}



/**
 * Resolve bitmap atlas UVs for one printable ASCII glyph.
 *
 * @param ascii the printable ASCII codepoint
 * @param out output u0, v0, u1, v1
 */
static void _text_bitmap_atlas_uv(uint32_t ascii, float out[4])
{
    ANN(out);
    ascii = _text_printable_ascii(ascii);
    uint32_t glyph = ascii - DVZ_TEXT_BITMAP_FIRST_CHAR;
    uint32_t col = glyph % DVZ_TEXT_BITMAP_ATLAS_COLS;
    uint32_t row = glyph / DVZ_TEXT_BITMAP_ATLAS_COLS;
    float width = (float)(DVZ_TEXT_BITMAP_ATLAS_COLS * DVZ_TEXT_BITMAP_ATLAS_CELL_W);
    float height = (float)(DVZ_TEXT_BITMAP_ATLAS_ROWS * DVZ_TEXT_BITMAP_ATLAS_CELL_H);
    uint32_t x0 = col * DVZ_TEXT_BITMAP_ATLAS_CELL_W + DVZ_TEXT_BITMAP_ATLAS_PAD;
    uint32_t y0 = row * DVZ_TEXT_BITMAP_ATLAS_CELL_H + DVZ_TEXT_BITMAP_ATLAS_PAD;
    out[0] = (float)x0 / width;
    out[1] = (float)y0 / height;
    out[2] = (float)(x0 + DVZ_TEXT_BITMAP_GLYPH_WIDTH) / width;
    out[3] = (float)(y0 + DVZ_TEXT_BITMAP_GLYPH_HEIGHT) / height;
}



/**
 * Return the style-to-atlas scale for SDF layout.
 *
 * @param style the text style
 * @param atlas the SDF atlas
 * @return the text scale relative to the atlas pixel height
 */
static float _text_sdf_layout_scale(const DvzTextStyle* style, const DvzTextAtlas* atlas)
{
    ANN(style);
    ANN(atlas);
    float size_px = style->size_px;
    if (size_px <= 0.0f)
        size_px = atlas->em_px;
    if (size_px < 1.0f)
        size_px = 1.0f;
    if (size_px > 512.0f)
        size_px = 512.0f;
    return atlas->em_px > 0.0f ? size_px / atlas->em_px : 1.0f;
}



/**
 * Measure a string with SDF atlas metrics.
 *
 * @param string the string
 * @param atlas the SDF atlas
 * @param scale the layout scale
 * @param out_width output layout width
 * @param out_height output layout height
 * @param out_visible output visible glyph count
 */
static void _text_sdf_measure(
    const char* string, DvzTextAtlas* atlas, float scale, float* out_width, float* out_height,
    uint32_t* out_visible)
{
    ANN(atlas);
    ANN(out_width);
    ANN(out_height);
    ANN(out_visible);
    float line_width = 0.0f;
    float max_width = 0.0f;
    uint32_t lines = 1;
    uint32_t visible = 0;
    DvzTextAtlasGlyph* space = _scene_text_atlas_glyph(atlas, ' ');
    float tab_advance = space != NULL ? 4.0f * space->advance * scale : 2.0f * atlas->em_px;
    if (string != NULL)
    {
        uint32_t byte_index = 0;
        uint32_t cp = 0;
        while (_text_utf8_next(string, &byte_index, &cp))
        {
            if (cp == '\n')
            {
                if (line_width > max_width)
                    max_width = line_width;
                line_width = 0.0f;
                lines++;
                continue;
            }
            if (cp == '\t')
            {
                line_width += tab_advance;
                continue;
            }
            DvzTextAtlasGlyph* glyph = _scene_text_atlas_glyph(atlas, cp);
            if (glyph == NULL)
                continue;
            line_width += glyph->advance * scale;
            visible++;
        }
    }
    if (line_width > max_width)
        max_width = line_width;
    *out_width = max_width;
    *out_height = ((float)(lines - 1u) * atlas->line_height + atlas->em_px) * scale;
    *out_visible = visible;
}



/**
 * Build an RGBA bitmap texture for a retained text string.
 *
 * @param text the text object
 * @param out_rgba output owned texture data
 * @param out_width output texture width
 * @param out_height output texture height
 * @return whether the texture was built
 */
static bool _text_build_bitmap(
    const DvzText* text, uint8_t** out_rgba, uint32_t* out_width, uint32_t* out_height)
{
    ANN(text);
    ANN(out_rgba);
    ANN(out_width);
    ANN(out_height);
    *out_rgba = NULL;
    *out_width = 0;
    *out_height = 0;

    uint32_t columns = 0;
    uint32_t lines = 0;
    uint32_t visible = 0;
    _text_measure_cells(text->string, &columns, &lines, &visible);
    if (columns == 0 || visible == 0)
        return false;

    uint32_t scale = _text_bitmap_scale(&text->style);
    uint64_t width64 = 0;
    uint64_t height64 = 0;
    if (_dvz_mul_u64_overflows(columns, DVZ_TEXT_BITMAP_GLYPH_WIDTH * scale, &width64) ||
        _dvz_mul_u64_overflows(
            (uint64_t)(lines - 1) * DVZ_TEXT_BITMAP_LINE_HEIGHT + DVZ_TEXT_BITMAP_GLYPH_HEIGHT,
            scale, &height64) ||
        width64 > UINT32_MAX || height64 > UINT32_MAX)
    {
        log_error("text bitmap dimensions overflow");
        return false;
    }
    uint32_t width = (uint32_t)width64;
    uint32_t height = (uint32_t)height64;

    uint64_t pixel_count = 0;
    uint64_t byte_size = 0;
    if (_dvz_mul_u64_overflows(width, height, &pixel_count) ||
        _dvz_mul_u64_overflows(pixel_count, 4u, &byte_size) || byte_size > SIZE_MAX)
    {
        log_error("text bitmap byte size overflow");
        return false;
    }
    uint8_t* rgba = (uint8_t*)dvz_calloc((DvzSize)byte_size, 1);
    if (rgba == NULL)
    {
        log_error("text bitmap allocation failed");
        return false;
    }

    uint8_t color[4] = {0};
    _text_style_color(&text->style, color);

    uint32_t column = 0;
    uint32_t row = 0;
    uint32_t i = 0;
    uint32_t cp = 0;
    while (_text_utf8_next(text->string, &i, &cp))
    {
        if (cp == '\n')
        {
            column = 0;
            row++;
            continue;
        }
        if (cp == '\t')
        {
            column += 4u;
            continue;
        }
        _text_paint_glyph(
            rgba, width, column * DVZ_TEXT_BITMAP_GLYPH_WIDTH * scale,
            row * DVZ_TEXT_BITMAP_LINE_HEIGHT * scale, scale, _text_printable_ascii(cp), color);
        column++;
    }

    *out_rgba = rgba;
    *out_width = width;
    *out_height = height;
    return true;
}



/**
 * Convert figure pixel coordinates to fixed clip-space coordinates.
 *
 * @param figure the figure
 * @param x the x coordinate in pixels from the figure left
 * @param y the y coordinate in pixels from the figure top
 * @param z the clip-space z coordinate
 * @param out output 3D clip-space position
 */
static void _text_pixel_to_clip(
    const DvzFigure* figure, float x, float y, float z, float out[3])
{
    ANN(figure);
    ANN(out);
    out[0] = figure->width > 0 ? 2.0f * x / (float)figure->width - 1.0f : -1.0f;
    out[1] = figure->height > 0 ? 1.0f - 2.0f * y / (float)figure->height : 1.0f;
    out[2] = z;
}


/**
 * Convert panel-local pixel coordinates to fixed clip-space coordinates.
 *
 * @param panel the panel that owns the text visual
 * @param x the x coordinate in panel-local pixels
 * @param y the y coordinate in panel-local pixels from the panel top
 * @param z the clip-space z coordinate
 * @param out output 3D clip-space position
 */
void _text_panel_pixel_to_clip(
    const DvzPanel* panel, float x, float y, float z, float out[3])
{
    ANN(panel);
    ANN(out);
    float panel_x = 0.0f;
    float panel_y = 0.0f;
    float panel_width = 0.0f;
    float panel_height = 0.0f;
    _scene_panel_pixel_rect(panel, &panel_x, &panel_y, &panel_width, &panel_height);
    (void)panel_x;
    (void)panel_y;
    out[0] = panel_width > 0.0f ? 2.0f * x / panel_width - 1.0f : -1.0f;
    out[1] = panel_height > 0.0f ? 1.0f - 2.0f * y / panel_height : 1.0f;
    out[2] = z;
}



/**
 * Resolve a text anchor in figure pixels.
 *
 * @param text the text object
 * @param out_x output anchor x coordinate
 * @param out_y output anchor y coordinate
 */
static void _text_anchor_pixels(const DvzText* text, float* out_x, float* out_y)
{
    ANN(text);
    ANN(text->panel);
    ANN(out_x);
    ANN(out_y);
    float px = 0;
    float py = 0;
    float pw = 0;
    float ph = 0;
    _scene_panel_pixel_rect(text->panel, &px, &py, &pw, &ph);

    switch (text->placement.anchor)
    {
    case DVZ_SCENE_ANCHOR_PANEL_TOP:
        *out_x = px + .5f * pw;
        *out_y = py;
        break;
    case DVZ_SCENE_ANCHOR_PANEL_TOP_RIGHT:
        *out_x = px + pw;
        *out_y = py;
        break;
    case DVZ_SCENE_ANCHOR_PANEL_LEFT:
        *out_x = px;
        *out_y = py + .5f * ph;
        break;
    case DVZ_SCENE_ANCHOR_PANEL_CENTER:
        *out_x = px + .5f * pw;
        *out_y = py + .5f * ph;
        break;
    case DVZ_SCENE_ANCHOR_PANEL_RIGHT:
        *out_x = px + pw;
        *out_y = py + .5f * ph;
        break;
    case DVZ_SCENE_ANCHOR_PANEL_BOTTOM_LEFT:
        *out_x = px;
        *out_y = py + ph;
        break;
    case DVZ_SCENE_ANCHOR_PANEL_BOTTOM:
        *out_x = px + .5f * pw;
        *out_y = py + ph;
        break;
    case DVZ_SCENE_ANCHOR_PANEL_BOTTOM_RIGHT:
        *out_x = px + pw;
        *out_y = py + ph;
        break;
    case DVZ_SCENE_ANCHOR_SCREEN:
        *out_x = (float)text->placement.position[0];
        *out_y = (float)text->placement.position[1];
        return;
    case DVZ_SCENE_ANCHOR_NONE:
    case DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT:
    default:
        *out_x = px;
        *out_y = py;
        break;
    }
    *out_x += (float)text->placement.position[0];
    *out_y += (float)text->placement.position[1];
}



/**
 * Resolve the default text-box anchor for a target anchor.
 *
 * @param anchor the scene anchor
 * @param out output anchor in top-left text-box coordinates
 */
static void _text_default_box_anchor(DvzSceneAnchor anchor, float out[2])
{
    ANN(out);
    out[0] = 0.0f;
    out[1] = 0.0f;
    switch (anchor)
    {
    case DVZ_SCENE_ANCHOR_PANEL_TOP:
    case DVZ_SCENE_ANCHOR_PANEL_CENTER:
    case DVZ_SCENE_ANCHOR_PANEL_BOTTOM:
        out[0] = 0.5f;
        break;
    case DVZ_SCENE_ANCHOR_PANEL_TOP_RIGHT:
    case DVZ_SCENE_ANCHOR_PANEL_RIGHT:
    case DVZ_SCENE_ANCHOR_PANEL_BOTTOM_RIGHT:
        out[0] = 1.0f;
        break;
    default:
        break;
    }
    switch (anchor)
    {
    case DVZ_SCENE_ANCHOR_PANEL_LEFT:
    case DVZ_SCENE_ANCHOR_PANEL_CENTER:
    case DVZ_SCENE_ANCHOR_PANEL_RIGHT:
        out[1] = 0.5f;
        break;
    case DVZ_SCENE_ANCHOR_PANEL_BOTTOM_LEFT:
    case DVZ_SCENE_ANCHOR_PANEL_BOTTOM:
    case DVZ_SCENE_ANCHOR_PANEL_BOTTOM_RIGHT:
        out[1] = 1.0f;
        break;
    default:
        break;
    }
}



/**
 * Resolve text box alignment for a placement.
 *
 * @param placement the text placement
 * @param width the text box width
 * @param height the text box height
 * @param out_x output local x offset
 * @param out_y output local y offset
 */
static void _text_placement_alignment(
    const DvzTextPlacement* placement, float width, float height, float* out_x, float* out_y)
{
    ANN(placement);
    ANN(out_x);
    ANN(out_y);
    float text_anchor[2] = {0.0f, 0.0f};
    if (placement->has_text_anchor)
    {
        text_anchor[0] = placement->text_anchor[0];
        text_anchor[1] = placement->text_anchor[1];
        if (text_anchor[0] < 0.0f)
            text_anchor[0] = 0.0f;
        if (text_anchor[0] > 1.0f)
            text_anchor[0] = 1.0f;
        if (text_anchor[1] < 0.0f)
            text_anchor[1] = 0.0f;
        if (text_anchor[1] > 1.0f)
            text_anchor[1] = 1.0f;
    }
    else
    {
        _text_default_box_anchor(placement->anchor, text_anchor);
    }

    *out_x = -text_anchor[0] * width;
    *out_y = -text_anchor[1] * height;
}



/**
 * Attach or update the generated glyph visual with the text visual attachment metadata.
 *
 * @param panel the owning panel
 * @param glyph_visual the generated glyph visual
 * @param desc the desired attachment descriptor
 * @return whether the glyph visual is attached with the desired metadata
 */
static bool _text_sync_glyph_visual_attach(
    DvzPanel* panel, DvzVisual* glyph_visual, const DvzVisualAttachDesc* desc)
{
    ANN(panel);
    ANN(glyph_visual);
    ANN(desc);
    for (uint32_t i = 0; i < panel->visual_count; i++)
    {
        DvzPanelAttach* attach = &panel->visuals[i];
        if (attach->visual != glyph_visual)
            continue;
        bool changed =
            attach->z_layer != desc->z_layer || attach->controller_mode != desc->controller_mode;
        attach->z_layer = desc->z_layer;
        attach->controller_mode = desc->controller_mode;
        if (changed)
            _scene_notify_request_frame(panel->figure);
        return true;
    }
    return dvz_panel_add_visual(panel, glyph_visual, desc) == 0;
}



/**
 * Write one glyph vertex record consumed by the shader-side quad generator.
 *
 * @param anchor_position the glyph anchor in the generated visual coordinate space
 * @param bounds_rect the local glyph pixel bounds as x0, y0, x1, y1
 * @param uv_rect the atlas UV rectangle as u0, v0, u1, v1
 * @param color the glyph color
 * @param angle the glyph rotation angle in radians
 * @param vertex_index the destination vertex index
 * @param positions the destination anchor positions
 * @param bounds the destination local bounds
 * @param texcoords the destination atlas UV bounds
 * @param colors the destination colors
 * @param angles the destination angles
 */
static void _text_write_glyph_vertex(
    const float anchor_position[3], const float bounds_rect[4], const float uv_rect[4],
    const uint8_t color[4], float angle, uint32_t vertex_index, float* positions, float* bounds,
    float* texcoords, uint8_t* colors, float* angles)
{
    ANN(anchor_position);
    ANN(bounds_rect);
    ANN(uv_rect);
    ANN(color);
    ANN(positions);
    ANN(bounds);
    ANN(texcoords);
    ANN(colors);
    ANN(angles);

    positions[3 * vertex_index + 0] = anchor_position[0];
    positions[3 * vertex_index + 1] = anchor_position[1];
    positions[3 * vertex_index + 2] = anchor_position[2];
    bounds[4 * vertex_index + 0] = bounds_rect[0];
    bounds[4 * vertex_index + 1] = bounds_rect[1];
    bounds[4 * vertex_index + 2] = bounds_rect[2];
    bounds[4 * vertex_index + 3] = bounds_rect[3];
    texcoords[4 * vertex_index + 0] = uv_rect[0];
    texcoords[4 * vertex_index + 1] = uv_rect[1];
    texcoords[4 * vertex_index + 2] = uv_rect[2];
    texcoords[4 * vertex_index + 3] = uv_rect[3];
    colors[4 * vertex_index + 0] = color[0];
    colors[4 * vertex_index + 1] = color[1];
    colors[4 * vertex_index + 2] = color[2];
    colors[4 * vertex_index + 3] = color[3];
    angles[vertex_index] = angle;
}



/**
 * Update or create the internal glyph visual for one retained text object.
 *
 * @param figure the figure being emitted
 * @param text the text object
 * @return whether preparation succeeded
 */
static bool _text_prepare_visual(DvzFigure* figure, DvzText* text)
{
    ANN(figure);
    ANN(text);
    if (text->scene == NULL || text->panel == NULL || text->panel->figure != figure)
        return true;
    float default_size_px = text->scene->font_defaults.text_size_px;
    DvzTextAtlasBackend backend =
        _text_renderer_backend(text->style.renderer, &text->style, default_size_px);
    bool use_builtin = backend == DVZ_TEXT_ATLAS_BACKEND_BUILTIN_BITMAP;
    bool screen_placement = text->placement.mode == DVZ_TEXT_PLACEMENT_SCREEN;
    DvzVisualAttachDesc attach = {
        .z_layer = INT32_MAX / 4,
        .controller_mode =
            screen_placement ? DVZ_CONTROLLER_FIXED : DVZ_CONTROLLER_APPLY_ISOTROPIC_LOCAL,
    };
    uint32_t visible = 0;
    DvzSampledField* atlas = NULL;
    DvzTextAtlas* font_atlas = NULL;
    uint64_t atlas_generation = 0;
    float scale = 1.0f;
    float glyph_w = 0.0f;
    float glyph_h = 0.0f;
    float line_h = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    if (!use_builtin)
    {
        DvzFont* font = _text_sdf_font(text->scene, &text->style);
        DvzTextAtlasSpec spec =
            _scene_text_atlas_spec(backend, _text_style_size_px(&text->style, default_size_px));
        if (font == NULL || !_scene_text_atlas_ensure_string(font, &spec, text->string))
            return false;
        font_atlas = _text_font_atlas(font, &spec);
        ANN(font_atlas);
        atlas = font_atlas->field;
        atlas_generation = font_atlas->generation;
        scale = _text_sdf_layout_scale(&text->style, font_atlas);
        _text_sdf_measure(text->string, font_atlas, scale, &width, &height, &visible);
        line_h = font_atlas->line_height * scale;
    }
    else
    {
        uint32_t columns = 0;
        uint32_t lines = 0;
        _text_measure_cells(text->string, &columns, &lines, &visible);
        atlas = _text_bitmap_atlas_field(text->scene);
        scale = _text_bitmap_layout_scale(&text->style);
        glyph_w = (float)DVZ_TEXT_BITMAP_GLYPH_WIDTH * scale;
        glyph_h = (float)DVZ_TEXT_BITMAP_GLYPH_HEIGHT * scale;
        line_h = (float)DVZ_TEXT_BITMAP_LINE_HEIGHT * scale;
        width = (float)columns * glyph_w;
        height = (float)(lines - 1u) * line_h + glyph_h;
    }
    if (text->visual != NULL && text->visual->field != NULL &&
        text->visual_version == text->version &&
        text->visual_atlas_generation == atlas_generation &&
        text->visual_figure_width == figure->width && text->visual_figure_height == figure->height)
    {
        return true;
    }
    if (visible == 0 || width <= 0.0f || height <= 0.0f)
    {
        if (text->visual != NULL)
            dvz_visual_set_visible(text->visual, false);
        return true;
    }
    if (atlas == NULL)
        return false;
    if (!isfinite(width) || !isfinite(height) || width <= 0.0f || height <= 0.0f ||
        width > (float)UINT32_MAX || height > (float)UINT32_MAX)
    {
        log_error("text glyph dimensions overflow");
        return false;
    }

    uint64_t max_vertices = 0;
    uint64_t position_bytes = 0;
    uint64_t bounds_bytes = 0;
    uint64_t texcoord_bytes = 0;
    uint64_t color_bytes = 0;
    uint64_t angle_bytes = 0;
    if (_dvz_mul_u64_overflows(visible, 6u, &max_vertices) ||
        _dvz_mul_u64_overflows(max_vertices, 3u * sizeof(float), &position_bytes) ||
        _dvz_mul_u64_overflows(max_vertices, 4u * sizeof(float), &bounds_bytes) ||
        _dvz_mul_u64_overflows(max_vertices, 4u * sizeof(float), &texcoord_bytes) ||
        _dvz_mul_u64_overflows(max_vertices, 4u * sizeof(uint8_t), &color_bytes) ||
        _dvz_mul_u64_overflows(max_vertices, sizeof(float), &angle_bytes) ||
        max_vertices > UINT32_MAX || position_bytes > SIZE_MAX || bounds_bytes > SIZE_MAX ||
        texcoord_bytes > SIZE_MAX || color_bytes > SIZE_MAX || angle_bytes > SIZE_MAX)
    {
        log_error("text glyph vertex buffer size overflow");
        return false;
    }

    float* positions = (float*)dvz_calloc((DvzSize)position_bytes, 1);
    float* bounds = (float*)dvz_calloc((DvzSize)bounds_bytes, 1);
    float* texcoords = (float*)dvz_calloc((DvzSize)texcoord_bytes, 1);
    uint8_t* colors = (uint8_t*)dvz_calloc((DvzSize)color_bytes, 1);
    float* angles = (float*)dvz_calloc((DvzSize)angle_bytes, 1);
    if (positions == NULL || bounds == NULL || texcoords == NULL || colors == NULL ||
        angles == NULL)
    {
        dvz_free(positions);
        dvz_free(bounds);
        dvz_free(texcoords);
        dvz_free(colors);
        dvz_free(angles);
        log_error("text glyph vertex allocation failed");
        return false;
    }

    uint8_t color[4] = {0};
    _text_style_color(&text->style, color);
    float anchor_clip[3] = {0};
    if (screen_placement)
    {
        float anchor_x = 0;
        float anchor_y = 0;
        _text_anchor_pixels(text, &anchor_x, &anchor_y);
        float z = (float)text->placement.position[2];
        _text_pixel_to_clip(figure, anchor_x, anchor_y, z, anchor_clip);
    }
    else
    {
        anchor_clip[0] = (float)text->placement.position[0];
        anchor_clip[1] = (float)text->placement.position[1];
        anchor_clip[2] = (float)text->placement.position[2];
    }
    float align_x = 0;
    float align_y = 0;
    _text_placement_alignment(&text->placement, (float)width, (float)height, &align_x, &align_y);
    align_x += text->placement.offset[0];
    align_y += text->placement.offset[1];

    uint32_t column = 0;
    uint32_t row = 0;
    uint32_t byte_index = 0;
    uint32_t cp = 0;
    uint32_t vertex_count = 0;
    float cursor_x = 0.0f;
    while (_text_utf8_next(text->string, &byte_index, &cp))
    {
        if (cp == '\n')
        {
            column = 0;
            cursor_x = 0.0f;
            row++;
            continue;
        }
        if (cp == '\t')
        {
            if (!use_builtin)
            {
                DvzTextAtlasGlyph* space = _scene_text_atlas_glyph(font_atlas, ' ');
                cursor_x += space != NULL ? 4.0f * space->advance * scale :
                                             2.0f * font_atlas->em_px * scale;
            }
            else
            {
                column += 4u;
            }
            continue;
        }

        float uv[4] = {0};
        float x0 = 0.0f;
        float y0 = 0.0f;
        float x1 = 0.0f;
        float y1 = 0.0f;
        if (!use_builtin)
        {
            DvzTextAtlasGlyph* glyph = _scene_text_atlas_glyph(font_atlas, cp);
            if (glyph == NULL)
                continue;
            float advance = glyph->advance * scale;
            if (glyph->width <= 0.0f || glyph->height <= 0.0f)
            {
                cursor_x += advance;
                continue;
            }
            x0 = align_x + cursor_x + glyph->xoff * scale;
            y0 = align_y + (float)row * line_h + font_atlas->ascent * scale +
                 glyph->yoff * scale;
            x1 = x0 + glyph->width * scale;
            y1 = y0 + glyph->height * scale;
            uv[0] = glyph->uv[0];
            uv[1] = glyph->uv[1];
            uv[2] = glyph->uv[2];
            uv[3] = glyph->uv[3];
            cursor_x += advance;
        }
        else
        {
            x0 = align_x + (float)column * glyph_w;
            y0 = align_y + (float)row * line_h;
            x1 = x0 + glyph_w;
            y1 = y0 + glyph_h;
            _text_bitmap_atlas_uv(cp, uv);
            column++;
        }
        float bounds_rect[4] = {x0, y0, x1, y1};
        for (uint32_t j = 0; j < 6; j++)
        {
            _text_write_glyph_vertex(
                anchor_clip, bounds_rect, uv, color, text->placement.angle, vertex_count,
                positions, bounds, texcoords, colors, angles);
            vertex_count++;
        }
    }
    if (vertex_count == 0)
    {
        dvz_free(positions);
        dvz_free(bounds);
        dvz_free(texcoords);
        dvz_free(colors);
        dvz_free(angles);
        if (text->visual != NULL)
            dvz_visual_set_visible(text->visual, false);
        return true;
    }

    bool ok = true;
    if (text->visual != NULL && text->visual->type != DVZ_VISUAL_TYPE_GLYPH)
    {
        dvz_visual_set_visible(text->visual, false);
        text->visual = NULL;
    }
    if (text->visual == NULL)
    {
        text->visual = dvz_glyph(text->scene, 0);
        if (text->visual == NULL)
            ok = false;
    }
    if (ok && !_text_sync_glyph_visual_attach(text->panel, text->visual, &attach))
        ok = false;
    if (ok && dvz_visual_set_alpha_mode(text->visual, DVZ_ALPHA_BLENDED) != 0)
        ok = false;
    if (ok && dvz_visual_set_depth_test(text->visual, text->placement.depth_test) != 0)
        ok = false;

    if (ok)
    {
        text->visual->glyph_atlas_encoding =
            font_atlas != NULL ? font_atlas->encoding : DVZ_TEXT_ATLAS_ENCODING_BITMAP_ALPHA;
        text->visual->glyph_distance_range_px =
            font_atlas != NULL ? font_atlas->distance_range_px : 1.0f;
        DvzVisualDataUpdate updates[5] = {
            {.attr_name = "position", .data = positions, .item_count = vertex_count},
            {.attr_name = "bounds", .data = bounds, .item_count = vertex_count},
            {.attr_name = "texcoords", .data = texcoords, .item_count = vertex_count},
            {.attr_name = "color", .data = colors, .item_count = vertex_count},
            {.attr_name = "angle", .data = angles, .item_count = vertex_count},
        };
        if (dvz_visual_set_data_many(text->visual, updates, 5) != 0 ||
            !dvz_visual_set_field(text->visual, "field", atlas))
        {
            ok = false;
        }
    }

    if (ok)
    {
        dvz_visual_set_visible(text->visual, true);
        text->metrics.advance[0] = (float)width;
        text->metrics.advance[1] = 0;
        text->metrics.ink_bounds[0] = 0;
        text->metrics.ink_bounds[1] = 0;
        text->metrics.ink_bounds[2] = (float)width;
        text->metrics.ink_bounds[3] = (float)height;
        text->metrics.layout_bounds[0] = 0;
        text->metrics.layout_bounds[1] = 0;
        text->metrics.layout_bounds[2] = (float)width;
        text->metrics.layout_bounds[3] = (float)height;
        text->metrics.baseline = font_atlas != NULL ? font_atlas->ascent * scale : 7.0f * scale;
        text->metrics.ascender =
            font_atlas != NULL ? font_atlas->ascent * scale : text->metrics.baseline;
        text->metrics.descender =
            font_atlas != NULL ? -font_atlas->descent * scale : 1.0f * scale;
        text->metrics.line_height = line_h;
        text->dirty_flags = DVZ_TEXT_DIRTY_NONE;
        text->visual_version = text->version;
        text->visual_atlas_generation = atlas_generation;
        text->visual_figure_width = figure->width;
        text->visual_figure_height = figure->height;
    }
    else if (text->visual != NULL)
    {
        dvz_visual_set_visible(text->visual, false);
    }

    dvz_free(positions);
    dvz_free(bounds);
    dvz_free(texcoords);
    dvz_free(colors);
    dvz_free(angles);
    return ok;
}



/**
 * Update or create the internal glyph visual for one retained annotation label.
 *
 * @param figure the figure being emitted
 * @param annotation the annotation object
 * @return whether preparation succeeded
 */
static bool _annotation_prepare_visual(DvzFigure* figure, DvzAnnotation* annotation)
{
    ANN(figure);
    ANN(annotation);
    if (annotation->scene == NULL || annotation->panel == NULL ||
        annotation->panel->figure != figure)
        return true;
    if (annotation->kind == DVZ_ANNOTATION_SCALEBAR)
        return _scalebar_prepare_visual(figure, annotation);
    if (annotation->kind != DVZ_ANNOTATION_LABEL)
    {
        if (annotation->visual != NULL)
            dvz_visual_set_visible(annotation->visual, false);
        if (annotation->scalebar_visual != NULL)
            dvz_visual_set_visible(annotation->scalebar_visual, false);
        return true;
    }

    DvzText proxy = {0};
    proxy.scene = annotation->scene;
    proxy.panel = annotation->panel;
    dvz_strlcpy(proxy.string, annotation->text, sizeof(proxy.string));
    proxy.style = annotation->style;
    proxy.placement = annotation->placement;
    proxy.flags = annotation->flags;
    proxy.dirty_flags = annotation->dirty_flags;
    proxy.version = annotation->version;
    proxy.metrics = annotation->metrics;
    proxy.visual = annotation->visual;
    proxy.visual_version = annotation->visual_version;
    proxy.visual_figure_width = annotation->visual_figure_width;
    proxy.visual_figure_height = annotation->visual_figure_height;

    bool ok = _text_prepare_visual(figure, &proxy);
    annotation->metrics = proxy.metrics;
    annotation->visual = proxy.visual;
    annotation->visual_version = proxy.visual_version;
    annotation->visual_figure_width = proxy.visual_figure_width;
    annotation->visual_figure_height = proxy.visual_figure_height;
    if (ok)
        annotation->dirty_flags = proxy.dirty_flags;
    return ok;
}



/**
 * Return a dense per-item attribute from a visual.
 *
 * @param visual the visual
 * @param name the attribute name
 * @return the attribute, or NULL when absent
 */
static const DvzVisualAttr* _text_visual_attr(const DvzVisual* visual, const char* name)
{
    ANN(visual);
    ANN(name);
    int idx = _attr_index(visual, name);
    if (idx < 0)
        return NULL;
    const DvzVisualAttr* attr = &visual->attrs[idx];
    return attr->data != NULL && attr->item_count > 0 ? attr : NULL;
}



/**
 * Resolve a text-visual realization version from strings and per-item attributes.
 *
 * @param visual the text visual
 * @return the realization version
 */
static uint64_t _text_visual_version(const DvzVisual* visual)
{
    ANN(visual);
    uint64_t version = visual->text.strings_version + visual->text.renderer_version;
    for (uint32_t i = 0; i < visual->attr_count; i++)
        version += visual->attrs[i].version;
    return version;
}



/**
 * Resolve a text-visual realization version excluding anchor positions.
 *
 * @param visual the text visual
 * @return the layout realization version
 */
static uint64_t _text_visual_layout_version(const DvzVisual* visual)
{
    ANN(visual);
    uint64_t version = visual->text.strings_version + visual->text.renderer_version;
    for (uint32_t i = 0; i < visual->attr_count; i++)
    {
        if (strcmp(visual->attrs[i].name, "position") == 0)
            continue;
        version += visual->attrs[i].version;
    }
    return version;
}



/**
 * Replace only the derived glyph anchor-position attribute for one text visual.
 *
 * @param figure the figure being emitted
 * @param panel the panel carrying the text visual
 * @param attach the panel attachment for the text visual
 * @param visual the batched text visual
 * @param position_attr the source per-string position attribute
 * @param version the full realized text version after the update
 * @param layout_version the layout-only realized text version after the update
 * @return whether the derived glyph positions were updated
 */
static bool _text_visual_update_glyph_positions(
    DvzFigure* figure, DvzPanel* panel, const DvzPanelAttach* attach, DvzVisual* visual,
    const DvzVisualAttr* position_attr, uint64_t version, uint64_t layout_version)
{
    ANN(figure);
    ANN(panel);
    ANN(attach);
    ANN(visual);
    ANN(position_attr);
    DvzVisual* glyph_visual = visual->text.glyph_visual;
    if (glyph_visual == NULL || visual->text.spans == NULL)
        return false;

    int glyph_pos_idx = _attr_index(glyph_visual, "position");
    if (glyph_pos_idx < 0 || glyph_visual->attrs[glyph_pos_idx].item_count == 0)
        return false;
    uint32_t vertex_capacity = glyph_visual->attrs[glyph_pos_idx].item_count;

    uint64_t position_bytes = 0;
    if (_dvz_mul_u64_overflows(vertex_capacity, 3u * sizeof(float), &position_bytes) ||
        position_bytes > SIZE_MAX)
    {
        log_error("text visual glyph position buffer size overflow");
        return false;
    }
    float* positions = (float*)dvz_calloc((DvzSize)position_bytes, 1);
    if (positions == NULL)
    {
        log_error("text visual glyph position allocation failed");
        return false;
    }

    const float(*target)[3] = (const float(*)[3])position_attr->data;
    for (uint32_t i = 0; i < visual->text.span_count; i++)
    {
        if (i >= position_attr->item_count)
            break;
        DvzTextGlyphSpan* span = &visual->text.spans[i];
        uint64_t first_vertex64 = (uint64_t)span->first_glyph * 6u;
        uint64_t vertex_count64 = (uint64_t)span->glyph_count * 6u;
        if (
            first_vertex64 > UINT32_MAX || vertex_count64 > UINT32_MAX ||
            first_vertex64 + vertex_count64 > vertex_capacity)
        {
            dvz_free(positions);
            log_error("text visual glyph span exceeds reserved position capacity");
            return false;
        }

        float anchor_position[3] = {0};
        if (attach->controller_mode == DVZ_CONTROLLER_FIXED)
        {
            _text_panel_pixel_to_clip(
                panel, target[i][0], target[i][1], target[i][2], anchor_position);
        }
        else
        {
            anchor_position[0] = target[i][0];
            anchor_position[1] = target[i][1];
            anchor_position[2] = target[i][2];
        }
        uint32_t first_vertex = (uint32_t)first_vertex64;
        uint32_t vertex_count = (uint32_t)vertex_count64;
        for (uint32_t j = 0; j < vertex_count; j++)
        {
            uint32_t vertex_index = first_vertex + j;
            positions[3 * vertex_index + 0] = anchor_position[0];
            positions[3 * vertex_index + 1] = anchor_position[1];
            positions[3 * vertex_index + 2] = anchor_position[2];
        }
    }

    bool ok = dvz_visual_set_data(glyph_visual, "position", positions, vertex_capacity) == 0;
    dvz_free(positions);
    if (!ok)
        return false;
    dvz_visual_set_visible(glyph_visual, true);
    visual->text.realized_version = version;
    visual->text.realized_layout_version = layout_version;
    visual->text.realized_controller_mode = attach->controller_mode;
    visual->text.visual_figure_width = figure->width;
    visual->text.visual_figure_height = figure->height;
    return true;
}



/**
 * Update or create the internal glyph visual for one batched text visual.
 *
 * @param figure the figure being emitted
 * @param panel the panel carrying the text visual
 * @param attach the panel attachment for the text visual
 * @param visual the batched text visual
 * @return whether preparation succeeded
 */
bool _text_visual_prepare(
    DvzFigure* figure, DvzPanel* panel, const DvzPanelAttach* attach, DvzVisual* visual)
{
    ANN(figure);
    ANN(panel);
    ANN(attach);
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_TEXT)
        return true;
    if (!visual->visible)
    {
        if (visual->text.glyph_visual != NULL)
            dvz_visual_set_visible(visual->text.glyph_visual, false);
        return true;
    }

    const uint32_t count = visual->text.string_count;
    const DvzVisualAttr* position_attr = _text_visual_attr(visual, "position");
    if (count == 0 || visual->text.strings == NULL || position_attr == NULL ||
        position_attr->item_count != count)
    {
        if (visual->text.glyph_visual != NULL)
            dvz_visual_set_visible(visual->text.glyph_visual, false);
        return true;
    }

    const DvzVisualAttr* anchor_attr = _text_visual_attr(visual, "anchor");
    const DvzVisualAttr* size_attr = _text_visual_attr(visual, "size");
    const DvzVisualAttr* color_attr = _text_visual_attr(visual, "color");
    const DvzVisualAttr* angle_attr = _text_visual_attr(visual, "angle");
    if ((anchor_attr != NULL && anchor_attr->item_count != count) ||
        (size_attr != NULL && size_attr->item_count != count) ||
        (color_attr != NULL && color_attr->item_count != count) ||
        (angle_attr != NULL && angle_attr->item_count != count))
    {
        log_error("text visual attributes must match string count");
        return false;
    }

    uint64_t version = _text_visual_version(visual);
    uint64_t layout_version = _text_visual_layout_version(visual);
    float screen_scale = _scene_screen_scale(figure);

    DvzTextRenderer renderer = visual->text.renderer;
    float spec_size_px = 0.0f;
    if (size_attr != NULL && size_attr->data != NULL)
    {
        const float* item_sizes = (const float*)size_attr->data;
        for (uint32_t i = 0; i < count; i++)
        {
            if (item_sizes[i] > spec_size_px)
                spec_size_px = item_sizes[i];
        }
    }
    if (spec_size_px <= 0.0f)
        spec_size_px = visual->scene->font_defaults.text_size_px;
    spec_size_px *= screen_scale;
    DvzTextStyle backend_style = {
        .size_px = spec_size_px,
        .renderer = renderer,
    };
    DvzTextAtlasBackend backend =
        _text_renderer_backend(renderer, &backend_style, visual->scene->font_defaults.text_size_px);
    bool use_builtin = backend == DVZ_TEXT_ATLAS_BACKEND_BUILTIN_BITMAP;
    DvzTextAtlas* font_atlas = NULL;
    DvzSampledField* atlas = NULL;
    uint64_t atlas_generation = 0;
    if (!use_builtin)
    {
        DvzTextStyle atlas_style = {
            .font = NULL,
            .size_px = backend_style.size_px,
            .renderer = renderer,
        };
        DvzFont* font = _text_sdf_font(visual->scene, &atlas_style);
        const char* const* strings = (const char* const*)visual->text.strings;
        DvzTextAtlasSpec spec = _scene_text_atlas_spec(backend, spec_size_px);
        if (font == NULL || !_scene_text_atlas_ensure_strings(
                                font, &spec, strings, visual->text.string_count))
            return false;
        font_atlas = _text_font_atlas(font, &spec);
        ANN(font_atlas);
        atlas = font_atlas->field;
        atlas_generation = font_atlas->generation;
    }
    else
    {
        atlas = _text_bitmap_atlas_field(visual->scene);
    }
    if (atlas == NULL)
        return false;
    DvzVisualAttachDesc glyph_attach = {
        .z_layer = attach->z_layer,
        .controller_mode = attach->controller_mode,
    };
    bool realized_cache_valid =
        visual->text.glyph_visual != NULL && visual->text.glyph_visual->field != NULL &&
        visual->text.realized_version == version &&
        visual->text.atlas_generation == atlas_generation &&
        visual->text.realized_controller_mode == attach->controller_mode &&
        fabsf(visual->text.screen_scale - screen_scale) <= 1e-6f &&
        visual->text.visual_figure_width == figure->width &&
        visual->text.visual_figure_height == figure->height;
    if (realized_cache_valid)
    {
        return _text_sync_glyph_visual_attach(panel, visual->text.glyph_visual, &glyph_attach);
    }
    bool position_only_dirty =
        visual->text.glyph_visual != NULL && visual->text.glyph_visual->field != NULL &&
        visual->text.realized_layout_version == layout_version &&
        visual->text.atlas_generation == atlas_generation &&
        fabsf(visual->text.screen_scale - screen_scale) <= 1e-6f;
    if (position_only_dirty)
    {
        if (!_text_sync_glyph_visual_attach(panel, visual->text.glyph_visual, &glyph_attach))
            return false;
        return _text_visual_update_glyph_positions(
            figure, panel, attach, visual, position_attr, version, layout_version);
    }

    uint64_t vertex_count64 = 0;
    for (uint32_t i = 0; i < count; i++)
    {
        uint32_t columns = 0;
        uint32_t lines = 0;
        uint32_t visible = 0;
        _text_measure_cells(visual->text.strings[i], &columns, &lines, &visible);
        (void)columns;
        (void)lines;
        uint64_t vertices = 0;
        uint64_t next_vertex_count = 0;
        if (_dvz_mul_u64_overflows(visible, 6u, &vertices) ||
            _dvz_add_u64_overflows(vertex_count64, vertices, &next_vertex_count))
        {
            log_error("text visual glyph vertex count overflow");
            return false;
        }
        vertex_count64 = next_vertex_count;
    }
    if (vertex_count64 == 0)
    {
        if (visual->text.glyph_visual != NULL)
            dvz_visual_set_visible(visual->text.glyph_visual, false);
        return true;
    }
    if (vertex_count64 > UINT32_MAX)
    {
        log_error("text visual glyph vertex count exceeds uint32");
        return false;
    }
    uint32_t vertex_count_max = (uint32_t)vertex_count64;
    uint32_t allocation_vertex_count = vertex_count_max;
    if (visual->text.reserved_glyph_vertices > allocation_vertex_count)
        allocation_vertex_count = visual->text.reserved_glyph_vertices;

    uint64_t position_bytes = 0;
    uint64_t bounds_bytes = 0;
    uint64_t texcoord_bytes = 0;
    uint64_t color_bytes = 0;
    uint64_t angle_bytes = 0;
    if (_dvz_mul_u64_overflows(allocation_vertex_count, 3u * sizeof(float), &position_bytes) ||
        _dvz_mul_u64_overflows(allocation_vertex_count, 4u * sizeof(float), &bounds_bytes) ||
        _dvz_mul_u64_overflows(allocation_vertex_count, 4u * sizeof(float), &texcoord_bytes) ||
        _dvz_mul_u64_overflows(allocation_vertex_count, 4u * sizeof(uint8_t), &color_bytes) ||
        _dvz_mul_u64_overflows(allocation_vertex_count, sizeof(float), &angle_bytes) ||
        position_bytes > SIZE_MAX || bounds_bytes > SIZE_MAX || texcoord_bytes > SIZE_MAX ||
        color_bytes > SIZE_MAX || angle_bytes > SIZE_MAX)
    {
        log_error("text visual glyph buffer size overflow");
        return false;
    }

    float* positions = (float*)dvz_calloc((DvzSize)position_bytes, 1);
    float* bounds = (float*)dvz_calloc((DvzSize)bounds_bytes, 1);
    float* texcoords = (float*)dvz_calloc((DvzSize)texcoord_bytes, 1);
    uint8_t* colors = (uint8_t*)dvz_calloc((DvzSize)color_bytes, 1);
    float* glyph_angles = (float*)dvz_calloc((DvzSize)angle_bytes, 1);
    DvzTextGlyphSpan* spans = (DvzTextGlyphSpan*)dvz_calloc(count, sizeof(DvzTextGlyphSpan));
    if (positions == NULL || bounds == NULL || texcoords == NULL || colors == NULL ||
        glyph_angles == NULL || spans == NULL)
    {
        dvz_free(positions);
        dvz_free(bounds);
        dvz_free(texcoords);
        dvz_free(colors);
        dvz_free(glyph_angles);
        dvz_free(spans);
        log_error("text visual glyph allocation failed");
        return false;
    }

    const float(*target)[3] = (const float(*)[3])position_attr->data;
    const float(*text_anchors)[2] =
        anchor_attr != NULL ? (const float(*)[2])anchor_attr->data : NULL;
    const float* sizes = size_attr != NULL ? (const float*)size_attr->data : NULL;
    const uint8_t(*item_colors)[4] =
        color_attr != NULL ? (const uint8_t(*)[4])color_attr->data : NULL;
    const float* angles = angle_attr != NULL ? (const float*)angle_attr->data : NULL;
    uint32_t vertex_count = 0;

    for (uint32_t i = 0; i < count; i++)
    {
        DvzTextStyle style = {
            .size_px = (sizes != NULL ? sizes[i] : 12.0f) * screen_scale,
            .renderer = renderer,
            .color = {255, 255, 255, 255},
        };
        if (item_colors != NULL)
        {
            style.color[0] = item_colors[i][0];
            style.color[1] = item_colors[i][1];
            style.color[2] = item_colors[i][2];
            style.color[3] = item_colors[i][3];
        }
        uint8_t color[4] = {0};
        _text_style_color(&style, color);

        uint32_t visible = 0;
        float scale = 1.0f;
        float glyph_w = 0.0f;
        float glyph_h = 0.0f;
        float line_h = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
        if (!use_builtin)
        {
            scale = _text_sdf_layout_scale(&style, font_atlas);
            _text_sdf_measure(
                visual->text.strings[i], font_atlas, scale, &width, &height, &visible);
            line_h = font_atlas->line_height * scale;
        }
        else
        {
            uint32_t columns = 0;
            uint32_t lines = 0;
            _text_measure_cells(visual->text.strings[i], &columns, &lines, &visible);
            scale = _text_bitmap_layout_scale(&style);
            glyph_w = (float)DVZ_TEXT_BITMAP_GLYPH_WIDTH * scale;
            glyph_h = (float)DVZ_TEXT_BITMAP_GLYPH_HEIGHT * scale;
            line_h = (float)DVZ_TEXT_BITMAP_LINE_HEIGHT * scale;
            width = (float)columns * glyph_w;
            height = (float)(lines - 1u) * line_h + glyph_h;
        }
        if (visible == 0 || width <= 0.0f || height <= 0.0f)
        {
            spans[i].first_glyph = vertex_count / 6u;
            spans[i].glyph_count = 0;
            continue;
        }

        float text_anchor[2] = {0.0f, 0.0f};
        if (text_anchors != NULL)
        {
            text_anchor[0] = text_anchors[i][0];
            text_anchor[1] = text_anchors[i][1];
        }
        float align_x = -text_anchor[0] * width;
        float align_y = -text_anchor[1] * height;
        float angle = angles != NULL ? angles[i] : 0.0f;
        float anchor_position[3] = {0};
        if (attach->controller_mode == DVZ_CONTROLLER_FIXED)
        {
            _text_panel_pixel_to_clip(
                panel, target[i][0], target[i][1], target[i][2], anchor_position);
        }
        else
        {
            anchor_position[0] = target[i][0];
            anchor_position[1] = target[i][1];
            anchor_position[2] = target[i][2];
        }
        spans[i].first_glyph = vertex_count / 6u;

        uint32_t column = 0;
        uint32_t row = 0;
        uint32_t byte_index = 0;
        uint32_t cp = 0;
        float cursor_x = 0.0f;
        while (_text_utf8_next(visual->text.strings[i], &byte_index, &cp))
        {
            if (cp == '\n')
            {
                column = 0;
                cursor_x = 0.0f;
                row++;
                continue;
            }
            if (cp == '\t')
            {
                if (!use_builtin)
                {
                    DvzTextAtlasGlyph* space = _scene_text_atlas_glyph(font_atlas, ' ');
                    cursor_x += space != NULL ? 4.0f * space->advance * scale :
                                                 2.0f * font_atlas->em_px * scale;
                }
                else
                {
                    column += 4u;
                }
                continue;
            }

            float uv[4] = {0};
            float x0 = 0.0f;
            float y0 = 0.0f;
            float x1 = 0.0f;
            float y1 = 0.0f;
            if (!use_builtin)
            {
                DvzTextAtlasGlyph* glyph = _scene_text_atlas_glyph(font_atlas, cp);
                if (glyph == NULL)
                    continue;
                float advance = glyph->advance * scale;
                if (glyph->width <= 0.0f || glyph->height <= 0.0f)
                {
                    cursor_x += advance;
                    continue;
                }
                x0 = align_x + cursor_x + glyph->xoff * scale;
                y0 = align_y + (float)row * line_h + font_atlas->ascent * scale +
                     glyph->yoff * scale;
                x1 = x0 + glyph->width * scale;
                y1 = y0 + glyph->height * scale;
                uv[0] = glyph->uv[0];
                uv[1] = glyph->uv[1];
                uv[2] = glyph->uv[2];
                uv[3] = glyph->uv[3];
                cursor_x += advance;
            }
            else
            {
                x0 = align_x + (float)column * glyph_w;
                y0 = align_y + (float)row * line_h;
                x1 = x0 + glyph_w;
                y1 = y0 + glyph_h;
                _text_bitmap_atlas_uv(cp, uv);
                column++;
            }
            float bounds_rect[4] = {x0, y0, x1, y1};
            for (uint32_t j = 0; j < 6; j++)
            {
                _text_write_glyph_vertex(
                    anchor_position, bounds_rect, uv, color, angle, vertex_count, positions,
                    bounds, texcoords, colors, glyph_angles);
                vertex_count++;
            }
        }
        spans[i].glyph_count = vertex_count / 6u - spans[i].first_glyph;
    }

    bool ok = atlas != NULL;
    if (ok && visual->text.glyph_visual == NULL)
    {
        visual->text.glyph_visual = dvz_glyph(visual->scene, 0);
        if (visual->text.glyph_visual == NULL)
            ok = false;
    }
    if (ok && !_text_sync_glyph_visual_attach(panel, visual->text.glyph_visual, &glyph_attach))
        ok = false;
    if (ok && dvz_visual_set_alpha_mode(visual->text.glyph_visual, DVZ_ALPHA_BLENDED) != 0)
        ok = false;
    if (ok && dvz_visual_set_depth_test(visual->text.glyph_visual, false) != 0)
        ok = false;
    if (ok)
    {
        visual->text.glyph_visual->glyph_atlas_encoding =
            font_atlas != NULL ? font_atlas->encoding : DVZ_TEXT_ATLAS_ENCODING_BITMAP_ALPHA;
        visual->text.glyph_visual->glyph_distance_range_px =
            font_atlas != NULL ? font_atlas->distance_range_px : 1.0f;
        uint32_t upload_vertex_count = vertex_count;
        if (visual->text.reserved_glyph_vertices > upload_vertex_count)
            upload_vertex_count = visual->text.reserved_glyph_vertices;
        DvzVisualDataUpdate updates[5] = {
            {.attr_name = "position", .data = positions, .item_count = upload_vertex_count},
            {.attr_name = "bounds", .data = bounds, .item_count = upload_vertex_count},
            {.attr_name = "texcoords", .data = texcoords, .item_count = upload_vertex_count},
            {.attr_name = "color", .data = colors, .item_count = upload_vertex_count},
            {.attr_name = "angle", .data = glyph_angles, .item_count = upload_vertex_count},
        };
        ok = dvz_visual_set_data_many(visual->text.glyph_visual, updates, 5) == 0 &&
             dvz_visual_set_field(visual->text.glyph_visual, "field", atlas);
    }
    if (ok)
    {
        dvz_visual_set_visible(visual->text.glyph_visual, true);
        dvz_free(visual->text.spans);
        visual->text.spans = spans;
        visual->text.span_count = count;
        spans = NULL;
        visual->text.realized_version = version;
        visual->text.realized_layout_version = layout_version;
        visual->text.atlas_generation = atlas_generation;
        visual->text.realized_controller_mode = attach->controller_mode;
        visual->text.screen_scale = screen_scale;
        visual->text.visual_figure_width = figure->width;
        visual->text.visual_figure_height = figure->height;
    }
    else if (visual->text.glyph_visual != NULL)
    {
        dvz_visual_set_visible(visual->text.glyph_visual, false);
    }

    dvz_free(positions);
    dvz_free(bounds);
    dvz_free(texcoords);
    dvz_free(colors);
    dvz_free(glyph_angles);
    dvz_free(spans);
    return ok;
}



/**
 * Return a compact version sum for all scene font resources.
 *
 * @param scene the scene
 * @return the summed font version value
 */
static uint64_t _text_scene_font_version_sum(const DvzScene* scene)
{
    ANN(scene);
    uint64_t version = 0;
    for (uint32_t i = 0; i < scene->font_count; i++)
        version += scene->fonts[i].version;
    return version;
}



/*************************************************************************************************/
/*  Internal text realization                                                                    */
/*************************************************************************************************/

/**
 * Prepare image-backed visuals for retained text attached to one figure.
 *
 * @param figure the figure being emitted
 */
void _scene_prepare_text_visuals(DvzFigure* figure)
{
    ANN(figure);
    ANN(figure->scene);
    DvzScene* scene = figure->scene;
    for (uint32_t pass = 0; pass < 3; pass++)
    {
        uint64_t font_version_before = _text_scene_font_version_sum(scene);
        _scene_prepare_pinned_readout_cards(figure);
        _scene_prepare_selection_cards(figure);
        _scene_prepare_overlay_cards(figure);
        for (uint32_t pi = 0; pi < figure->panel_count; pi++)
        {
            DvzPanel* panel = &figure->panels[pi];
            uint32_t visual_count = panel->visual_count;
            for (uint32_t vi = 0; vi < visual_count; vi++)
            {
                DvzPanelAttach* attach = &panel->visuals[vi];
                DvzVisual* visual = attach->visual;
                if (visual != NULL && visual->type == DVZ_VISUAL_TYPE_TEXT &&
                    !_text_visual_prepare(figure, panel, attach, visual))
                {
                    log_error("failed to prepare batched text visual %u", vi);
                }
            }
        }
        for (uint32_t i = 0; i < scene->annotation_count; i++)
        {
            if (!_annotation_prepare_visual(figure, &scene->annotations[i]))
                log_error("failed to prepare retained annotation visual %u", i);
        }
        for (uint32_t i = 0; i < scene->text_count; i++)
        {
            if (!_text_prepare_visual(figure, &scene->texts[i]))
                log_error("failed to prepare retained text visual %u", i);
        }
        if (_text_scene_font_version_sum(scene) == font_version_before)
            break;
    }
}



/*************************************************************************************************/
/*  Fonts                                                                                        */
/*************************************************************************************************/

/**
 * Create a scene-owned font resource.
 *
 * @param scene the scene
 * @param desc the font descriptor
 * @return the font, or NULL on allocation failure
 */
DvzFont* dvz_font(DvzScene* scene, const DvzFontDesc* desc)
{
    ANN(scene);
    ANN(desc);
    if (scene->font_count >= DVZ_SCENE_MAX_FONTS)
    {
        log_error("maximum font count reached");
        return NULL;
    }
    DvzFont* font = &scene->fonts[scene->font_count++];
    dvz_memset(font, sizeof(DvzFont), 0, sizeof(DvzFont));
    font->scene = scene;
    font->face_index = desc->face_index;
    font->flags = desc->flags;
    font->version = 1;
    if (desc->path != NULL)
        dvz_strlcpy(font->path, desc->path, sizeof(font->path));
    if (desc->family != NULL)
        dvz_strlcpy(font->family, desc->family, sizeof(font->family));
    if (desc->style != NULL)
        dvz_strlcpy(font->style, desc->style, sizeof(font->style));
    return font;
}



/**
 * Destroy a scene-owned font resource.
 *
 * @param font the font
 */
void dvz_font_destroy(DvzFont* font)
{
    if (font == NULL)
        return;
    _scene_font_release(font);
}



/*************************************************************************************************/
/*  Text                                                                                         */
/*************************************************************************************************/

/**
 * Return the default retained text style.
 *
 * @param scene the scene
 * @return default text style
 */
static DvzTextStyle _text_default_style(const DvzScene* scene)
{
    float size_px = dvz_font_defaults().text_size_px;
    if (scene != NULL && scene->font_defaults.text_size_px > 0.0f)
        size_px = scene->font_defaults.text_size_px;
    DvzTextStyle style = {0};
    style.size_px = size_px;
    style.renderer = DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS;
    style.color[0] = 255;
    style.color[1] = 255;
    style.color[2] = 255;
    style.color[3] = 255;
    return style;
}



/**
 * Return the default retained text placement.
 *
 * @return default text placement
 */
static DvzTextPlacement _text_default_placement(void)
{
    DvzTextPlacement placement = {0};
    placement.mode = DVZ_TEXT_PLACEMENT_SCREEN;
    placement.anchor = DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT;
    return placement;
}



/**
 * Return whether a renderer enum is implemented by the retained text path.
 *
 * @param renderer the renderer
 * @return whether the renderer is supported
 */
static bool _text_renderer_supported(DvzTextRenderer renderer)
{
    return renderer == DVZ_TEXT_RENDERER_AUTO ||
           renderer == DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS ||
           renderer == DVZ_TEXT_RENDERER_BITMAP_ATLAS ||
           renderer == DVZ_TEXT_RENDERER_MSDF_ATLAS;
}



/**
 * Mark a retained text object dirty and request a frame.
 *
 * @param text the text object
 * @param flags dirty flags
 */
static void _text_mark_dirty(DvzText* text, uint32_t flags)
{
    ANN(text);
    text->dirty_flags |= flags;
    text->version++;
    _scene_notify_request_frame(text->panel != NULL ? text->panel->figure : NULL);
}



/**
 * Create a retained text object attached to a panel.
 *
 * @param panel the panel
 * @param flags creation flags
 * @return the text object, or NULL on allocation failure
 */
DvzText* dvz_text(DvzPanel* panel, uint32_t flags)
{
    ANN(panel);
    if (panel->figure == NULL || panel->figure->scene == NULL)
        return NULL;
    DvzScene* scene = panel->figure->scene;
    if (scene->text_count >= DVZ_SCENE_MAX_TEXTS)
    {
        log_error("maximum text count reached");
        return NULL;
    }
    DvzText* text = &scene->texts[scene->text_count++];
    dvz_memset(text, sizeof(DvzText), 0, sizeof(DvzText));
    text->scene = scene;
    text->panel = panel;
    text->style = _text_default_style(scene);
    text->placement = _text_default_placement();
    text->flags = flags;
    text->dirty_flags = DVZ_TEXT_DIRTY_ALL;
    text->version = 1;
    _scene_notify_request_frame(panel->figure);
    return text;
}



/**
 * Destroy a retained text object.
 *
 * @param text the text object
 */
void dvz_text_destroy(DvzText* text)
{
    if (text == NULL)
        return;
    if (text->visual != NULL)
        dvz_visual_set_visible(text->visual, false);
    _scene_notify_request_frame(text->panel != NULL ? text->panel->figure : NULL);
    text->scene = NULL;
    text->panel = NULL;
    text->string[0] = '\0';
    text->dirty_flags = DVZ_TEXT_DIRTY_NONE;
}



/**
 * Set the UTF-8 content of a retained text object.
 *
 * @param text the text object
 * @param string the string, or NULL to clear
 */
void dvz_text_set_string(DvzText* text, const char* string)
{
    ANN(text);
    const char* src = string != NULL ? string : "";
    if (strcmp(text->string, src) == 0)
        return;
    dvz_strlcpy(text->string, src, sizeof(text->string));
    _text_mark_dirty(text, DVZ_TEXT_DIRTY_STRING | DVZ_TEXT_DIRTY_LAYOUT | DVZ_TEXT_DIRTY_RENDER);
}



/**
 * Set the style of a retained text object.
 *
 * @param text the text object
 * @param style the style descriptor, or NULL for defaults
 * @return 0 on success, -1 on error
 */
int dvz_text_set_style(DvzText* text, const DvzTextStyle* style)
{
    ANN(text);
    DvzTextStyle resolved = style != NULL ? *style : _text_default_style(text->scene);
    if (resolved.font != NULL && resolved.font->scene != text->scene)
    {
        log_error("cannot bind a font from a different scene");
        return -1;
    }
    if (!_text_renderer_supported(resolved.renderer))
    {
        log_error("text renderer %d is not implemented for retained text yet", resolved.renderer);
        return -1;
    }
    text->style = resolved;
    _text_mark_dirty(text, DVZ_TEXT_DIRTY_STYLE | DVZ_TEXT_DIRTY_LAYOUT | DVZ_TEXT_DIRTY_RENDER);
    return 0;
}



/**
 * Set the placement of a retained text object.
 *
 * @param text the text object
 * @param placement the placement descriptor, or NULL for defaults
 */
void dvz_text_set_placement(DvzText* text, const DvzTextPlacement* placement)
{
    ANN(text);
    text->placement = placement != NULL ? *placement : _text_default_placement();
    _text_mark_dirty(text, DVZ_TEXT_DIRTY_PLACEMENT | DVZ_TEXT_DIRTY_LAYOUT);
}



/**
 * Select the renderer used by a retained text object.
 *
 * @param text the text object
 * @param renderer renderer selection
 * @return 0 on success, -1 on error
 */
int dvz_text_set_renderer(DvzText* text, DvzTextRenderer renderer)
{
    ANN(text);
    if (!_text_renderer_supported(renderer))
    {
        log_error("text renderer %d is not implemented for retained text yet", renderer);
        return -1;
    }
    if (text->style.renderer == renderer)
        return 0;
    text->style.renderer = renderer;
    _text_mark_dirty(text, DVZ_TEXT_DIRTY_STYLE | DVZ_TEXT_DIRTY_LAYOUT | DVZ_TEXT_DIRTY_RENDER);
    return 0;
}



/*************************************************************************************************/
/*  Annotations                                                                                  */
/*************************************************************************************************/

/**
 * Create a retained annotation object attached to a panel.
 *
 * @param panel the panel
 * @param desc the annotation descriptor
 * @return the annotation, or NULL on allocation failure
 */
DvzAnnotation* dvz_annotation(DvzPanel* panel, const DvzAnnotationDesc* desc)
{
    ANN(panel);
    ANN(desc);
    if (panel->figure == NULL || panel->figure->scene == NULL)
        return NULL;
    DvzScene* scene = panel->figure->scene;
    if (scene->annotation_count >= DVZ_SCENE_MAX_ANNOTATIONS)
    {
        log_error("maximum annotation count reached");
        return NULL;
    }
    if (desc->style.font != NULL && desc->style.font->scene != scene)
    {
        log_error("cannot bind a font from a different scene");
        return NULL;
    }
    DvzAnnotation* annotation = &scene->annotations[scene->annotation_count++];
    dvz_memset(annotation, sizeof(DvzAnnotation), 0, sizeof(DvzAnnotation));
    annotation->scene = scene;
    annotation->panel = panel;
    annotation->kind = desc->kind;
    annotation->style = desc->style;
    annotation->placement = desc->placement;
    annotation->flags = desc->flags;
    annotation->dirty_flags = DVZ_TEXT_DIRTY_ALL;
    annotation->version = 1;
    if (desc->text != NULL)
        dvz_strlcpy(annotation->text, desc->text, sizeof(annotation->text));
    _scene_notify_request_frame(panel->figure);
    return annotation;
}



/**
 * Create a retained label annotation.
 *
 * @param panel the panel
 * @param desc the label descriptor
 * @return the annotation, or NULL on allocation failure
 */
DvzAnnotation* dvz_annotation_label(DvzPanel* panel, const DvzLabelDesc* desc)
{
    ANN(desc);
    return dvz_annotation(
        panel, &(DvzAnnotationDesc){
                   .kind = DVZ_ANNOTATION_LABEL,
                   .text = desc->text,
                   .style = desc->style,
                   .placement = desc->placement,
                   .flags = desc->flags});
}



/**
 * Destroy a retained annotation object.
 *
 * @param annotation the annotation
 */
void dvz_annotation_destroy(DvzAnnotation* annotation)
{
    if (annotation == NULL)
        return;
    if (annotation->visual != NULL)
        dvz_visual_set_visible(annotation->visual, false);
    if (annotation->scalebar_visual != NULL)
        dvz_visual_set_visible(annotation->scalebar_visual, false);
    _scene_notify_request_frame(annotation->panel != NULL ? annotation->panel->figure : NULL);
    annotation->scene = NULL;
    annotation->panel = NULL;
    annotation->has_format = false;
}



/**
 * Override formatting policy on an annotation.
 *
 * @param annotation the annotation
 * @param format the format descriptor, or NULL to clear the override
 */
void dvz_annotation_set_format(DvzAnnotation* annotation, const DvzFormatDesc* format)
{
    ANN(annotation);
    annotation->has_format = format != NULL;
    _scene_format_state_copy(&annotation->format, format);
    if (annotation->kind == DVZ_ANNOTATION_SCALEBAR)
    {
        if (format != NULL)
            annotation->scalebar.format = *format;
        else
            dvz_memset(
                &annotation->scalebar.format, sizeof(DvzFormatDesc), 0, sizeof(DvzFormatDesc));
    }
    annotation->dirty_flags |=
        DVZ_TEXT_DIRTY_STRING | DVZ_TEXT_DIRTY_LAYOUT | DVZ_TEXT_DIRTY_RENDER;
    annotation->version++;
    _scene_notify_request_frame(annotation->panel != NULL ? annotation->panel->figure : NULL);
}
