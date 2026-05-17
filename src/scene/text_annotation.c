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

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_overflow.h"
#include "_scene.h"
#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_TEXT_BITMAP_GLYPH_WIDTH  6u
#define DVZ_TEXT_BITMAP_GLYPH_HEIGHT 8u
#define DVZ_TEXT_BITMAP_LINE_HEIGHT  9u
#define DVZ_TEXT_BITMAP_FIRST_CHAR   32u
#define DVZ_TEXT_BITMAP_GLYPH_COUNT  96u
#define DVZ_TEXT_BITMAP_FALLBACK     63u



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
 * Return whether a renderer selection can use the built-in bitmap path.
 *
 * @param renderer the requested text renderer
 * @return whether the bitmap path is selected
 */
static bool _text_renderer_uses_bitmap(DvzTextRenderer renderer)
{
    return renderer == DVZ_TEXT_RENDERER_AUTO ||
           renderer == DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS ||
           renderer == DVZ_TEXT_RENDERER_BITMAP_ATLAS;
}



/**
 * Resolve the style color, using opaque white for zero-initialized styles.
 *
 * @param style the text style
 * @param out_color the output RGBA color
 */
static void _text_style_color(const DvzTextStyle* style, uint8_t out_color[4])
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
    float size_pts = style->size_pts;
    if (size_pts <= 0 && style->font != NULL)
        size_pts = style->font->size_pts;
    if (size_pts <= 0)
        size_pts = 12.0f;
    uint32_t scale = (uint32_t)floorf(size_pts / 8.0f + 0.5f);
    if (scale == 0)
        scale = 1;
    if (scale > 32)
        scale = 32;
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
static uint32_t _text_printable_ascii(uint8_t byte)
{
    if (byte >= DVZ_TEXT_BITMAP_FIRST_CHAR &&
        byte < DVZ_TEXT_BITMAP_FIRST_CHAR + DVZ_TEXT_BITMAP_GLYPH_COUNT)
        return byte;
    return DVZ_TEXT_BITMAP_FALLBACK;
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
        for (uint32_t i = 0; i < DVZ_SCENE_LABEL_SIZE && string[i] != '\0'; i++)
        {
            if (string[i] == '\n')
            {
                if (columns > max_columns)
                    max_columns = columns;
                columns = 0;
                lines++;
                continue;
            }
            uint32_t advance = string[i] == '\t' ? 4u : 1u;
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
    for (uint32_t i = 0; i < DVZ_SCENE_LABEL_SIZE && text->string[i] != '\0'; i++)
    {
        uint8_t byte = (uint8_t)text->string[i];
        if (byte == '\n')
        {
            column = 0;
            row++;
            continue;
        }
        if (byte == '\t')
        {
            column += 4u;
            continue;
        }
        _text_paint_glyph(
            rgba, width, column * DVZ_TEXT_BITMAP_GLYPH_WIDTH * scale,
            row * DVZ_TEXT_BITMAP_LINE_HEIGHT * scale, scale, _text_printable_ascii(byte), color);
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
        return;
    case DVZ_SCENE_ANCHOR_PANEL_TOP_RIGHT:
        *out_x = px + pw;
        *out_y = py;
        return;
    case DVZ_SCENE_ANCHOR_PANEL_LEFT:
        *out_x = px;
        *out_y = py + .5f * ph;
        return;
    case DVZ_SCENE_ANCHOR_PANEL_CENTER:
        *out_x = px + .5f * pw;
        *out_y = py + .5f * ph;
        return;
    case DVZ_SCENE_ANCHOR_PANEL_RIGHT:
        *out_x = px + pw;
        *out_y = py + .5f * ph;
        return;
    case DVZ_SCENE_ANCHOR_PANEL_BOTTOM_LEFT:
        *out_x = px;
        *out_y = py + ph;
        return;
    case DVZ_SCENE_ANCHOR_PANEL_BOTTOM:
        *out_x = px + .5f * pw;
        *out_y = py + ph;
        return;
    case DVZ_SCENE_ANCHOR_PANEL_BOTTOM_RIGHT:
        *out_x = px + pw;
        *out_y = py + ph;
        return;
    case DVZ_SCENE_ANCHOR_SCREEN:
        *out_x = (float)text->placement.position[0];
        *out_y = (float)text->placement.position[1];
        return;
    case DVZ_SCENE_ANCHOR_NONE:
    case DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT:
    default:
        *out_x = px;
        *out_y = py;
        return;
    }
}



/**
 * Resolve text box alignment for an anchor.
 *
 * @param anchor the scene anchor
 * @param width the text box width
 * @param height the text box height
 * @param out_x output local x offset
 * @param out_y output local y offset
 */
static void _text_anchor_alignment(
    DvzSceneAnchor anchor, float width, float height, float* out_x, float* out_y)
{
    ANN(out_x);
    ANN(out_y);
    *out_x = 0;
    *out_y = 0;
    switch (anchor)
    {
    case DVZ_SCENE_ANCHOR_PANEL_TOP:
    case DVZ_SCENE_ANCHOR_PANEL_CENTER:
    case DVZ_SCENE_ANCHOR_PANEL_BOTTOM:
        *out_x = -.5f * width;
        break;
    case DVZ_SCENE_ANCHOR_PANEL_TOP_RIGHT:
    case DVZ_SCENE_ANCHOR_PANEL_RIGHT:
    case DVZ_SCENE_ANCHOR_PANEL_BOTTOM_RIGHT:
        *out_x = -width;
        break;
    default:
        break;
    }
    switch (anchor)
    {
    case DVZ_SCENE_ANCHOR_PANEL_LEFT:
    case DVZ_SCENE_ANCHOR_PANEL_CENTER:
    case DVZ_SCENE_ANCHOR_PANEL_RIGHT:
        *out_y = -.5f * height;
        break;
    case DVZ_SCENE_ANCHOR_PANEL_BOTTOM_LEFT:
    case DVZ_SCENE_ANCHOR_PANEL_BOTTOM:
    case DVZ_SCENE_ANCHOR_PANEL_BOTTOM_RIGHT:
        *out_y = -height;
        break;
    default:
        break;
    }
}



/**
 * Write one rotated text-corner position in fixed clip coordinates.
 *
 * @param figure the figure
 * @param anchor_x the anchor x in figure pixels
 * @param anchor_y the anchor y in figure pixels
 * @param local_x the local x offset from the anchor
 * @param local_y the local y offset from the anchor
 * @param angle the rotation angle in radians
 * @param z the clip-space z coordinate
 * @param out output 3D clip-space position
 */
static void _text_corner_position(
    const DvzFigure* figure, float anchor_x, float anchor_y, float local_x, float local_y,
    float angle, float z, float out[3])
{
    ANN(figure);
    ANN(out);
    float c = cosf(angle);
    float s = sinf(angle);
    float x = anchor_x + c * local_x - s * local_y;
    float y = anchor_y + s * local_x + c * local_y;
    _text_pixel_to_clip(figure, x, y, z, out);
}



/**
 * Update or create the internal image visual for one retained text object.
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
    if (!_text_renderer_uses_bitmap(text->style.renderer) ||
        text->placement.mode != DVZ_TEXT_PLACEMENT_SCREEN)
    {
        if (text->visual != NULL)
            dvz_visual_set_visible(text->visual, false);
        return true;
    }
    if (text->visual != NULL && text->visual_version == text->version &&
        text->visual_figure_width == figure->width && text->visual_figure_height == figure->height)
    {
        return true;
    }

    uint8_t* rgba = NULL;
    uint32_t width = 0;
    uint32_t height = 0;
    if (!_text_build_bitmap(text, &rgba, &width, &height))
    {
        if (text->visual != NULL)
            dvz_visual_set_visible(text->visual, false);
        return true;
    }

    bool ok = true;
    if (text->visual == NULL)
    {
        text->visual = dvz_image(text->scene, 0);
        if (text->visual == NULL)
            ok = false;
        DvzVisualAttachDesc attach = {
            .z_layer = INT32_MAX / 4,
            .controller_mode = DVZ_CONTROLLER_FIXED,
        };
        if (ok && dvz_panel_add_visual(text->panel, text->visual, &attach) != 0)
            ok = false;
        if (ok && dvz_visual_set_alpha_mode(text->visual, DVZ_ALPHA_BLENDED) != 0)
            ok = false;
        if (ok && dvz_visual_set_depth_test(text->visual, false) != 0)
            ok = false;
    }

    if (ok)
    {
        float anchor_x = 0;
        float anchor_y = 0;
        _text_anchor_pixels(text, &anchor_x, &anchor_y);

        float align_x = 0;
        float align_y = 0;
        _text_anchor_alignment(
            text->placement.anchor, (float)width, (float)height, &align_x, &align_y);
        align_x += text->placement.offset[0];
        align_y += text->placement.offset[1];

        float positions[4][3] = {0};
        float z = (float)text->placement.position[2];
        _text_corner_position(
            figure, anchor_x, anchor_y, align_x, align_y, text->placement.angle, z, positions[0]);
        _text_corner_position(
            figure, anchor_x, anchor_y, align_x, align_y + (float)height,
            text->placement.angle, z, positions[1]);
        _text_corner_position(
            figure, anchor_x, anchor_y, align_x + (float)width, align_y,
            text->placement.angle, z, positions[2]);
        _text_corner_position(
            figure, anchor_x, anchor_y, align_x + (float)width, align_y + (float)height,
            text->placement.angle, z, positions[3]);

        const float texcoords[4][2] = {{0, 0}, {0, 1}, {1, 0}, {1, 1}};
        if (dvz_visual_set_data(text->visual, "position", positions, 4) != 0 ||
            dvz_visual_set_data(text->visual, "texcoords", texcoords, 4) != 0 ||
            dvz_visual_set_texture(text->visual, rgba, width, height) != 0)
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
        text->metrics.baseline = 7.0f * (float)_text_bitmap_scale(&text->style);
        text->metrics.ascender = text->metrics.baseline;
        text->metrics.descender = 1.0f * (float)_text_bitmap_scale(&text->style);
        text->metrics.line_height = DVZ_TEXT_BITMAP_LINE_HEIGHT *
                                    (float)_text_bitmap_scale(&text->style);
        text->dirty_flags = DVZ_TEXT_DIRTY_NONE;
        text->visual_version = text->version;
        text->visual_figure_width = figure->width;
        text->visual_figure_height = figure->height;
    }
    else if (text->visual != NULL)
    {
        dvz_visual_set_visible(text->visual, false);
    }

    dvz_free(rgba);
    return ok;
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
    for (uint32_t i = 0; i < scene->text_count; i++)
    {
        if (!_text_prepare_visual(figure, &scene->texts[i]))
            log_error("failed to prepare retained text visual %u", i);
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
    font->size_pts = desc->size_pts;
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
    font->scene = NULL;
}



/*************************************************************************************************/
/*  Text                                                                                         */
/*************************************************************************************************/

/**
 * Create a retained text object attached to a panel.
 *
 * @param panel the panel
 * @param desc the text descriptor
 * @return the text object, or NULL on allocation failure
 */
DvzText* dvz_text(DvzPanel* panel, const DvzTextDesc* desc)
{
    ANN(panel);
    ANN(desc);
    if (panel->figure == NULL || panel->figure->scene == NULL)
        return NULL;
    DvzScene* scene = panel->figure->scene;
    if (scene->text_count >= DVZ_SCENE_MAX_TEXTS)
    {
        log_error("maximum text count reached");
        return NULL;
    }
    if (desc->style.font != NULL && desc->style.font->scene != scene)
    {
        log_error("cannot bind a font from a different scene");
        return NULL;
    }
    DvzText* text = &scene->texts[scene->text_count++];
    dvz_memset(text, sizeof(DvzText), 0, sizeof(DvzText));
    text->scene = scene;
    text->panel = panel;
    text->style = desc->style;
    text->placement = desc->placement;
    text->flags = desc->flags;
    text->dirty_flags = DVZ_TEXT_DIRTY_ALL;
    text->version = 1;
    if (desc->string != NULL)
        dvz_strlcpy(text->string, desc->string, sizeof(text->string));
    return text;
}



/**
 * Destroy a retained text object.
 *
 * @param text the text
 */
void dvz_text_destroy(DvzText* text)
{
    if (text == NULL)
        return;
    text->scene = NULL;
    text->panel = NULL;
}



/**
 * Update the content string on a retained text object.
 *
 * @param text the text
 * @param string the new string
 */
void dvz_text_set_string(DvzText* text, const char* string)
{
    ANN(text);
    text->string[0] = '\0';
    if (string != NULL)
        dvz_strlcpy(text->string, string, sizeof(text->string));
    text->dirty_flags |= DVZ_TEXT_DIRTY_STRING | DVZ_TEXT_DIRTY_LAYOUT | DVZ_TEXT_DIRTY_RENDER;
    text->version++;
}



/**
 * Update the style on a retained text object.
 *
 * @param text the text
 * @param style the new style
 */
void dvz_text_set_style(DvzText* text, const DvzTextStyle* style)
{
    ANN(text);
    ANN(style);
    if (style->font != NULL && (text->scene == NULL || style->font->scene != text->scene))
    {
        log_error("cannot bind a font from a different scene");
        return;
    }
    text->style = *style;
    text->dirty_flags |= DVZ_TEXT_DIRTY_STYLE | DVZ_TEXT_DIRTY_LAYOUT | DVZ_TEXT_DIRTY_RENDER;
    text->version++;
}



/**
 * Update the placement on a retained text object.
 *
 * @param text the text
 * @param placement the new placement
 */
void dvz_text_set_placement(DvzText* text, const DvzTextPlacement* placement)
{
    ANN(text);
    ANN(placement);
    text->placement = *placement;
    text->dirty_flags |= DVZ_TEXT_DIRTY_PLACEMENT | DVZ_TEXT_DIRTY_LAYOUT | DVZ_TEXT_DIRTY_RENDER;
    text->version++;
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
    annotation->dirty_flags |=
        DVZ_TEXT_DIRTY_STRING | DVZ_TEXT_DIRTY_LAYOUT | DVZ_TEXT_DIRTY_RENDER;
    annotation->version++;
}
