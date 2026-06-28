/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene text bitmap atlas helpers                                                              */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "_overflow.h"
#include "_scene.h"
#include "datoviz/scene.h"
#include "text_internal.h"



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
float _text_bitmap_layout_scale(const DvzTextStyle* style)
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
DvzSampledField* _text_bitmap_atlas_field(DvzScene* scene)
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

    DvzSampledFieldDesc desc = dvz_sampled_field_desc();
    desc.width = width;
    desc.height = height;
    DvzSampledField* field = dvz_sampled_field(scene, &desc);
    if (field == NULL ||
        !dvz_sampled_field_set_data(
            field, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView),
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
void _text_bitmap_atlas_uv(uint32_t ascii, float out[4])
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
    const char* string = text->legacy_string != NULL ? text->legacy_string : "";
    _text_measure_cells(string, &columns, &lines, &visible);
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
    while (_text_utf8_next(string, &i, &cp))
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
