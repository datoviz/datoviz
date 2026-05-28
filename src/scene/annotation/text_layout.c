/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene text layout helpers                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "_assertions.h"
#include "_scene.h"
#include "datoviz/scene.h"
#include "text_internal.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return the resolved text size in pixels.
 *
 * @param style the text style
 * @param fallback_size_px fallback text size in pixels
 * @return the resolved size
 */
float _text_style_size_px(const DvzTextStyle* style, float fallback_size_px)
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
DvzTextAtlasBackend _text_renderer_backend(
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
 * Decode one UTF-8 codepoint, replacing malformed input with '?'.
 *
 * @param string the UTF-8 string
 * @param inout_index byte index, advanced by the consumed sequence
 * @param out_codepoint output Unicode codepoint
 * @return whether a codepoint was decoded
 */
bool _text_utf8_next(const char* string, uint32_t* inout_index, uint32_t* out_codepoint)
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
void _text_measure_cells(
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
 * Return the style-to-atlas scale for SDF layout.
 *
 * @param style the text style
 * @param atlas the SDF atlas
 * @return the text scale relative to the atlas pixel height
 */
float _text_sdf_layout_scale(const DvzTextStyle* style, const DvzTextAtlas* atlas)
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
void _text_sdf_measure(
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
 * Convert figure pixel coordinates to fixed clip-space coordinates.
 *
 * @param figure the figure
 * @param x the x coordinate in pixels from the figure left
 * @param y the y coordinate in pixels from the figure top
 * @param z the clip-space z coordinate
 * @param out output 3D clip-space position
 */
void _text_pixel_to_clip(const DvzFigure* figure, float x, float y, float z, float out[3])
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
void _text_placement_alignment(
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
