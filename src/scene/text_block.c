/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene text block helpers                                                                     */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_overflow.h"
#include "_scene.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Store the first diagnostic emitted while parsing a text block.
 *
 * @param block the text block
 * @param message the diagnostic message
 * @param offset source byte offset
 */
static void _text_block_diag(DvzTextBlock* block, const char* message, uint32_t offset)
{
    ANN(block);
    ANN(message);
    if (block->diagnostic[0] == '\0')
        dvz_snprintf(block->diagnostic, sizeof(block->diagnostic), "%s at byte %u", message, offset);
}



/**
 * Return whether the source at one offset starts with a literal token.
 *
 * @param source UTF-8 source text
 * @param offset byte offset
 * @param token ASCII token
 * @return whether the token matches
 */
static bool _text_block_starts_with(const char* source, uint32_t offset, const char* token)
{
    ANN(source);
    ANN(token);
    size_t token_len = strlen(token);
    return strncmp(source + offset, token, token_len) == 0;
}


/**
 * Return the integer value of one hexadecimal character.
 *
 * @param c character
 * @param out output nibble
 * @return whether the character is hexadecimal
 */
static bool _text_block_hex(char c, uint8_t* out)
{
    ANN(out);
    if (c >= '0' && c <= '9')
    {
        *out = (uint8_t)(c - '0');
        return true;
    }
    if (c >= 'a' && c <= 'f')
    {
        *out = (uint8_t)(10 + c - 'a');
        return true;
    }
    if (c >= 'A' && c <= 'F')
    {
        *out = (uint8_t)(10 + c - 'A');
        return true;
    }
    return false;
}


/**
 * Parse a `<color=#RRGGBB>` opening tag.
 *
 * @param source UTF-8 source text
 * @param offset byte offset
 * @param out_color output color
 * @param out_len output token length
 * @return whether a valid color tag was parsed
 */
static bool _text_block_parse_color_open(
    const char* source, uint32_t offset, DvzColor* out_color, uint32_t* out_len)
{
    ANN(source);
    ANN(out_color);
    ANN(out_len);
    const char* prefix = "<color=#";
    if (!_text_block_starts_with(source, offset, prefix))
        return false;
    uint32_t start = offset + (uint32_t)strlen(prefix);
    if (source[start + 6] != '>')
        return false;

    uint8_t n[6] = {0};
    for (uint32_t i = 0; i < 6; i++)
    {
        if (!_text_block_hex(source[start + i], &n[i]))
            return false;
    }
    *out_color = dvz_color_rgba(
        (uint8_t)((n[0] << 4u) | n[1]), (uint8_t)((n[2] << 4u) | n[3]),
        (uint8_t)((n[4] << 4u) | n[5]), 255);
    *out_len = (uint32_t)strlen(prefix) + 7u;
    return true;
}



/**
 * Append one parsed style run.
 *
 * @param block the text block
 * @param run the run to append
 * @return 0 on success, -1 on overflow
 */
static int _text_block_push_run(DvzTextBlock* block, const DvzTextBlockRun* run)
{
    ANN(block);
    ANN(run);
    if (run->text_end <= run->text_start)
        return 0;
    if (block->run_count >= DVZ_SCENE_TEXT_BLOCK_MAX_RUNS)
    {
        _text_block_diag(block, "too many text block runs", run->source_start);
        return -1;
    }
    block->runs[block->run_count++] = *run;
    return 0;
}



/**
 * Append one visible ASCII byte to the parsed text.
 *
 * @param block the text block
 * @param byte the visible byte
 * @param source_start source byte offset before the source token
 * @param source_end source byte offset after the source token
 * @param style_flags active style flags
 * @param has_color whether the active run overrides the default text color
 * @param color active run color
 * @param active whether a current run is active
 * @param current current run state
 * @return 0 on success, -1 on text buffer overflow
 */
static int _text_block_append_byte(
    DvzTextBlock* block, char byte, uint32_t source_start, uint32_t source_end,
    uint32_t style_flags, bool has_color, DvzColor color, bool* active,
    DvzTextBlockRun* current)
{
    ANN(block);
    ANN(active);
    ANN(current);
    if (block->text_size + 1 >= DVZ_SCENE_TEXT_BLOCK_TEXT_SIZE)
    {
        _text_block_diag(block, "text block output overflow", source_start);
        return -1;
    }
    if (!*active)
    {
        current->source_start = source_start;
        current->text_start = block->text_size;
        current->style_flags = style_flags;
        current->has_color = has_color;
        current->color = color;
        *active = true;
    }
    block->text[block->text_size++] = byte;
    block->text[block->text_size] = '\0';
    current->source_end = source_end;
    current->text_end = block->text_size;
    return 0;
}



/**
 * Close the current run before a style transition.
 *
 * @param block the text block
 * @param active whether a current run is active
 * @param current current run state
 * @return 0 on success, -1 on run overflow
 */
static int _text_block_close_run(
    DvzTextBlock* block, bool* active, DvzTextBlockRun* current)
{
    ANN(block);
    ANN(active);
    ANN(current);
    if (!*active)
        return 0;
    if (_text_block_push_run(block, current) != 0)
        return -1;
    dvz_memset(current, sizeof(DvzTextBlockRun), 0, sizeof(DvzTextBlockRun));
    *active = false;
    return 0;
}



/**
 * Return a positive integer pixel size from a logical pixel value.
 *
 * @param value logical pixel value
 * @param fallback fallback integer value
 * @return rounded positive integer
 */
static uint32_t _text_block_px(float value, uint32_t fallback)
{
    if (value <= 0.0f)
        return fallback;
    float rounded = value + 0.5f;
    if (rounded > (float)UINT32_MAX)
        return fallback;
    uint32_t out = (uint32_t)rounded;
    return out > 0 ? out : fallback;
}


/**
 * Return a positive integer pixel size after applying a raster scale.
 *
 * @param value logical pixel value
 * @param scale raster scale
 * @param fallback fallback integer value
 * @return rounded positive integer
 */
static uint32_t _text_block_scaled_px(float value, float scale, uint32_t fallback)
{
    if (scale <= 0.0f)
        scale = 1.0f;
    return _text_block_px(value * scale, fallback);
}



/**
 * Return style flags active at one parsed text byte.
 *
 * @param block the text block
 * @param text_index parsed text byte index
 * @return active style flags
 */
static uint32_t _text_block_style_at(const DvzTextBlock* block, uint32_t text_index)
{
    ANN(block);
    for (uint32_t i = 0; i < block->run_count; i++)
    {
        const DvzTextBlockRun* run = &block->runs[i];
        if (text_index >= run->text_start && text_index < run->text_end)
            return run->style_flags;
    }
    return DVZ_TEXT_BLOCK_STYLE_NONE;
}


/**
 * Return the text color active at one parsed text byte.
 *
 * @param block the text block
 * @param text_index parsed text byte index
 * @param fallback fallback color
 * @return active text color
 */
static DvzColor _text_block_color_at(
    const DvzTextBlock* block, uint32_t text_index, DvzColor fallback)
{
    ANN(block);
    for (uint32_t i = 0; i < block->run_count; i++)
    {
        const DvzTextBlockRun* run = &block->runs[i];
        if (text_index >= run->text_start && text_index < run->text_end)
            return run->has_color ? run->color : fallback;
    }
    return fallback;
}



/**
 * Write one RGBA pixel in a text-block raster.
 *
 * @param block the text block
 * @param x x coordinate
 * @param y y coordinate
 * @param color RGBA color
 */
static void _text_block_put_px(DvzTextBlock* block, uint32_t x, uint32_t y, const DvzColor color)
{
    ANN(block);
    ANN(block->rgba);
    if (x >= block->raster_width || y >= block->raster_height)
        return;
    uint64_t idx = 4u * ((uint64_t)y * block->raster_width + x);
    block->rgba[idx + 0] = color.r;
    block->rgba[idx + 1] = color.g;
    block->rgba[idx + 2] = color.b;
    block->rgba[idx + 3] = color.a;
}



/**
 * Return whether one pseudo-glyph cell is covered.
 *
 * @param byte visible text byte
 * @param col pseudo-glyph column
 * @param row pseudo-glyph row
 * @return whether the cell is covered
 */
static bool _text_block_glyph_cell(char byte, uint32_t col, uint32_t row)
{
    uint32_t c = (uint32_t)(uint8_t)byte;
    if (row == 0 || row == 6)
        return col > 0 && col < 4;
    if (col == 0)
        return (c & (1u << (row - 1u))) != 0 || row == 1 || row == 5;
    if (col == 4)
        return (c & (1u << (row + 1u))) != 0 || row == 2 || row == 4;
    return ((c + 3u * col + 5u * row) & 7u) < 3u;
}



/**
 * Draw one visible byte into a text-block raster.
 *
 * @param block the text block
 * @param byte visible byte
 * @param x origin x coordinate
 * @param y origin y coordinate
 * @param style_flags active style flags
 * @param color text color
 * @param scale raster scale
 */
static void _text_block_draw_byte(
    DvzTextBlock* block, char byte, uint32_t x, uint32_t y, uint32_t style_flags,
    const DvzColor color, float scale)
{
    ANN(block);
    if (byte == ' ' || byte == '\t' || byte == '\n')
        return;

    uint32_t char_w = _text_block_scaled_px(block->layout.char_width_px, scale, 7);
    uint32_t line_h = _text_block_scaled_px(block->layout.line_height_px, scale, 14);
    uint32_t glyph_w = char_w > 2 ? char_w - 2u : char_w;
    uint32_t glyph_h = line_h > 4 ? line_h - 4u : line_h;
    uint32_t thickness = (style_flags & DVZ_TEXT_BLOCK_STYLE_BOLD) != 0 ? 2u : 1u;
    uint32_t underline_y = y + line_h - 2u;
    for (uint32_t gy = 0; gy < glyph_h; gy++)
    {
        uint32_t row = glyph_h > 1 ? (7u * gy) / glyph_h : 0;
        uint32_t italic_shift =
            (style_flags & DVZ_TEXT_BLOCK_STYLE_ITALIC) != 0 ? (glyph_h - gy) / 4u : 0u;
        for (uint32_t gx = 0; gx < glyph_w; gx++)
        {
            uint32_t col = glyph_w > 1 ? (5u * gx) / glyph_w : 0;
            if (!_text_block_glyph_cell(byte, col, row))
                continue;
            for (uint32_t t = 0; t < thickness; t++)
                _text_block_put_px(block, x + 1u + gx + italic_shift + t, y + 2u + gy, color);
        }
    }
    if ((style_flags & DVZ_TEXT_BLOCK_STYLE_UNDERLINE) != 0)
    {
        for (uint32_t ux = 1; ux + 1 < char_w; ux++)
            _text_block_put_px(block, x + ux, underline_y, color);
    }
}


/**
 * Compute fixed-advance text positions with simple word wrapping.
 *
 * @param block the text block
 * @param wrap_chars maximum visible characters per line, or zero for no wrapping
 * @param out_max_line_chars output maximum line length
 * @param out_line_count output line count
 */
static void _text_block_layout_chars(
    DvzTextBlock* block, uint32_t wrap_chars, uint32_t* out_max_line_chars,
    uint32_t* out_line_count)
{
    ANN(block);
    ANN(out_max_line_chars);
    ANN(out_line_count);
    uint32_t line_chars = 0;
    uint32_t max_line_chars = 0;
    uint32_t line_count = 1;
    uint32_t i = 0;
    while (i < block->text_size)
    {
        if (block->text[i] == '\n')
        {
            block->layout_x[i] = line_chars;
            block->layout_y[i] = line_count - 1u;
            if (line_chars > max_line_chars)
                max_line_chars = line_chars;
            line_chars = 0;
            line_count++;
            i++;
            continue;
        }

        uint32_t word_len = 0;
        while (i + word_len < block->text_size)
        {
            char c = block->text[i + word_len];
            if (c == ' ' || c == '\t' || c == '\n')
                break;
            word_len++;
        }
        bool word_start =
            i == 0 || block->text[i - 1u] == ' ' || block->text[i - 1u] == '\t' ||
            block->text[i - 1u] == '\n';
        if (wrap_chars > 0 && word_start && word_len > 0 && line_chars > 0 &&
            line_chars + word_len > wrap_chars)
        {
            if (line_chars > max_line_chars)
                max_line_chars = line_chars;
            line_chars = 0;
            line_count++;
        }

        char c = block->text[i];
        if ((c == ' ' || c == '\t') && wrap_chars > 0 && line_chars == 0)
        {
            block->layout_x[i] = 0;
            block->layout_y[i] = line_count - 1u;
            i++;
            continue;
        }
        if (wrap_chars > 0 && line_chars >= wrap_chars)
        {
            if (line_chars > max_line_chars)
                max_line_chars = line_chars;
            line_chars = 0;
            line_count++;
        }
        block->layout_x[i] = line_chars;
        block->layout_y[i] = line_count - 1u;
        line_chars++;
        i++;
    }
    if (line_chars > max_line_chars)
        max_line_chars = line_chars;
    *out_max_line_chars = max_line_chars;
    *out_line_count = line_count;
    block->layout_line_count = line_count;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Initialize a private text-block object from UTF-8 source text.
 *
 * @param block the text block
 * @param source UTF-8 source text, or NULL for an empty block
 */
void _scene_text_block_init(DvzTextBlock* block, const char* source)
{
    ANN(block);
    dvz_memset(block, sizeof(DvzTextBlock), 0, sizeof(DvzTextBlock));
    if (source == NULL)
        source = "";
    dvz_strlcpy(block->source, source, sizeof(block->source));
    block->source_size = (uint32_t)strlen(block->source);
    if (strlen(source) >= sizeof(block->source))
        _text_block_diag(block, "text block source truncated", block->source_size);
}



/**
 * Release owned memory held by a private text-block object.
 *
 * @param block the text block
 */
void _scene_text_block_destroy(DvzTextBlock* block)
{
    if (block == NULL)
        return;
    if (block->image_visual != NULL)
    {
        dvz_visual_set_visible(block->image_visual, false);
        block->image_visual = NULL;
    }
    if (block->image_field != NULL)
    {
        (void)dvz_sampled_field_destroy(block->image_field);
        block->image_field = NULL;
    }
    if (block->rgba != NULL)
    {
        dvz_free(block->rgba);
        block->rgba = NULL;
    }
    block->rgba_size = 0;
    block->raster_width = 0;
    block->raster_height = 0;
    block->image_width = 0;
    block->image_height = 0;
    block->image_attached = false;
}



/**
 * Parse the text block source into plain text and style runs.
 *
 * @param block the text block
 * @return 0 on success, -1 on fixed-capacity overflow
 */
int _scene_text_block_parse(DvzTextBlock* block)
{
    ANN(block);
    block->text[0] = '\0';
    block->text_size = 0;
    block->run_count = 0;
    block->valid = false;

    uint32_t style_flags = DVZ_TEXT_BLOCK_STYLE_NONE;
    bool has_color = false;
    DvzColor active_color = {0};
    DvzTextBlockRun current = {0};
    bool active = false;
    uint32_t i = 0;
    while (i < block->source_size && block->source[i] != '\0')
    {
        if (_text_block_starts_with(block->source, i, "<b>"))
        {
            if ((style_flags & DVZ_TEXT_BLOCK_STYLE_BOLD) != 0)
            {
                _text_block_diag(block, "duplicate bold markup", i);
            }
            else
            {
                if (_text_block_close_run(block, &active, &current) != 0)
                    return -1;
                style_flags |= DVZ_TEXT_BLOCK_STYLE_BOLD;
                i += 3;
                continue;
            }
        }
        else if (_text_block_starts_with(block->source, i, "</b>"))
        {
            if ((style_flags & DVZ_TEXT_BLOCK_STYLE_BOLD) == 0)
            {
                _text_block_diag(block, "unmatched bold close markup", i);
            }
            else
            {
                if (_text_block_close_run(block, &active, &current) != 0)
                    return -1;
                style_flags &= ~((uint32_t)DVZ_TEXT_BLOCK_STYLE_BOLD);
                i += 4;
                continue;
            }
        }
        else if (_text_block_starts_with(block->source, i, "<i>"))
        {
            if ((style_flags & DVZ_TEXT_BLOCK_STYLE_ITALIC) != 0)
            {
                _text_block_diag(block, "duplicate italic markup", i);
            }
            else
            {
                if (_text_block_close_run(block, &active, &current) != 0)
                    return -1;
                style_flags |= DVZ_TEXT_BLOCK_STYLE_ITALIC;
                i += 3;
                continue;
            }
        }
        else if (_text_block_starts_with(block->source, i, "</i>"))
        {
            if ((style_flags & DVZ_TEXT_BLOCK_STYLE_ITALIC) == 0)
            {
                _text_block_diag(block, "unmatched italic close markup", i);
            }
            else
            {
                if (_text_block_close_run(block, &active, &current) != 0)
                    return -1;
                style_flags &= ~((uint32_t)DVZ_TEXT_BLOCK_STYLE_ITALIC);
                i += 4;
                continue;
            }
        }
        else if (_text_block_starts_with(block->source, i, "<u>"))
        {
            if ((style_flags & DVZ_TEXT_BLOCK_STYLE_UNDERLINE) != 0)
            {
                _text_block_diag(block, "duplicate underline markup", i);
            }
            else
            {
                if (_text_block_close_run(block, &active, &current) != 0)
                    return -1;
                style_flags |= DVZ_TEXT_BLOCK_STYLE_UNDERLINE;
                i += 3;
                continue;
            }
        }
        else if (_text_block_starts_with(block->source, i, "</u>"))
        {
            if ((style_flags & DVZ_TEXT_BLOCK_STYLE_UNDERLINE) == 0)
            {
                _text_block_diag(block, "unmatched underline close markup", i);
            }
            else
            {
                if (_text_block_close_run(block, &active, &current) != 0)
                    return -1;
                style_flags &= ~((uint32_t)DVZ_TEXT_BLOCK_STYLE_UNDERLINE);
                i += 4;
                continue;
            }
        }
        else
        {
            DvzColor parsed_color = {0};
            uint32_t color_tag_len = 0;
            if (_text_block_parse_color_open(block->source, i, &parsed_color, &color_tag_len))
            {
                if (has_color)
                {
                    _text_block_diag(block, "nested color markup", i);
                }
                else
                {
                    if (_text_block_close_run(block, &active, &current) != 0)
                        return -1;
                    active_color = parsed_color;
                    has_color = true;
                    i += color_tag_len;
                    continue;
                }
            }
        }
        if (_text_block_starts_with(block->source, i, "</color>"))
        {
            if (!has_color)
            {
                _text_block_diag(block, "unmatched color close markup", i);
            }
            else
            {
                if (_text_block_close_run(block, &active, &current) != 0)
                    return -1;
                has_color = false;
                active_color = (DvzColor){0};
                i += 8;
                continue;
            }
        }
        else if (_text_block_starts_with(block->source, i, "&lt;"))
        {
            if (_text_block_append_byte(
                    block, '<', i, i + 4, style_flags, has_color, active_color, &active,
                    &current) != 0)
                return -1;
            i += 4;
            continue;
        }
        else if (_text_block_starts_with(block->source, i, "&gt;"))
        {
            if (_text_block_append_byte(
                    block, '>', i, i + 4, style_flags, has_color, active_color, &active,
                    &current) != 0)
                return -1;
            i += 4;
            continue;
        }
        else if (_text_block_starts_with(block->source, i, "&amp;"))
        {
            if (_text_block_append_byte(
                    block, '&', i, i + 5, style_flags, has_color, active_color, &active,
                    &current) != 0)
                return -1;
            i += 5;
            continue;
        }
        else if (block->source[i] == '<' || block->source[i] == '&')
        {
            _text_block_diag(block, "literal unsupported markup", i);
        }

        if (_text_block_append_byte(
                block, block->source[i], i, i + 1, style_flags, has_color, active_color, &active,
                &current) != 0)
            return -1;
        i++;
    }

    if (_text_block_close_run(block, &active, &current) != 0)
        return -1;
    if (style_flags != DVZ_TEXT_BLOCK_STYLE_NONE || has_color)
        _text_block_diag(block, "unclosed text block markup", block->source_size);
    block->valid = true;
    return 0;
}



/**
 * Measure a parsed text block with simple fixed-advance wrapping.
 *
 * @param block the text block
 * @param layout optional layout constraints, or NULL for defaults
 * @return 0 on success, -1 when the block has not been parsed successfully
 */
int _scene_text_block_measure(DvzTextBlock* block, const DvzTextBlockLayout* layout)
{
    ANN(block);
    if (!block->valid)
        return -1;

    DvzTextBlockLayout resolved = {
        .char_width_px = 7.0f,
        .line_height_px = 14.0f,
    };
    if (layout != NULL)
        resolved = *layout;
    if (resolved.char_width_px <= 0.0f)
        resolved.char_width_px = 7.0f;
    if (resolved.line_height_px <= 0.0f)
        resolved.line_height_px = 14.0f;
    block->layout = resolved;

    uint32_t wrap_chars = 0;
    if (resolved.max_width_px > 0.0f)
    {
        float content_width = resolved.max_width_px - 2.0f * resolved.padding_px[0];
        if (content_width > resolved.char_width_px)
            wrap_chars = (uint32_t)(content_width / resolved.char_width_px);
    }

    uint32_t max_line_chars = 0;
    uint32_t line_count = 1;
    _text_block_layout_chars(block, wrap_chars, &max_line_chars, &line_count);

    float width_px = (float)max_line_chars * resolved.char_width_px + 2.0f * resolved.padding_px[0];
    if (resolved.max_width_px > 0.0f && width_px > resolved.max_width_px)
        width_px = resolved.max_width_px;
    float height_px = (float)line_count * resolved.line_height_px + 2.0f * resolved.padding_px[1];

    block->metrics.advance[0] = width_px;
    block->metrics.advance[1] = height_px;
    block->metrics.ink_bounds[0] = 0.0f;
    block->metrics.ink_bounds[1] = 0.0f;
    block->metrics.ink_bounds[2] = width_px;
    block->metrics.ink_bounds[3] = height_px;
    block->metrics.layout_bounds[0] = 0.0f;
    block->metrics.layout_bounds[1] = 0.0f;
    block->metrics.layout_bounds[2] = width_px;
    block->metrics.layout_bounds[3] = height_px;
    block->metrics.baseline = resolved.padding_px[1] + resolved.line_height_px;
    block->metrics.ascender = resolved.line_height_px;
    block->metrics.descender = 0.0f;
    block->metrics.line_height = resolved.line_height_px;
    return 0;
}



/**
 * Rasterize a parsed text block into owned RGBA8 pixels.
 *
 * @param block the text block
 * @param desc optional raster colors, or NULL for transparent background and white text
 * @return 0 on success, -1 on invalid input or allocation failure
 */
int _scene_text_block_rasterize(DvzTextBlock* block, const DvzTextBlockRasterDesc* desc)
{
    ANN(block);
    if (!block->valid)
        return -1;
    if (block->metrics.advance[0] <= 0.0f || block->metrics.advance[1] <= 0.0f)
    {
        if (_scene_text_block_measure(block, NULL) != 0)
            return -1;
    }

    DvzTextBlockRasterDesc resolved = {
        .text_color = {255, 255, 255, 255},
        .background_color = {0, 0, 0, 0},
        .scale = 1.0f,
    };
    if (desc != NULL)
        resolved = *desc;
    if (resolved.scale <= 0.0f)
        resolved.scale = 1.0f;

    uint32_t width = _text_block_scaled_px(block->metrics.advance[0], resolved.scale, 1);
    uint32_t height = _text_block_scaled_px(block->metrics.advance[1], resolved.scale, 1);
    uint64_t pixel_count = 0;
    uint64_t byte_count = 0;
    if (_dvz_mul_u64_overflows(width, height, &pixel_count) ||
        _dvz_mul_u64_overflows(pixel_count, 4u, &byte_count))
    {
        _text_block_diag(block, "text block raster size overflow", block->source_size);
        return -1;
    }

    if (byte_count != block->rgba_size)
    {
        if (block->rgba != NULL)
            dvz_free(block->rgba);
        block->rgba = (uint8_t*)dvz_calloc(byte_count, sizeof(uint8_t));
        if (block->rgba == NULL)
        {
            block->rgba_size = 0;
            block->raster_width = 0;
            block->raster_height = 0;
            _text_block_diag(block, "text block raster allocation failed", block->source_size);
            return -1;
        }
        block->rgba_size = byte_count;
    }
    block->raster_width = width;
    block->raster_height = height;
    block->raster_scale = resolved.scale;

    for (uint64_t i = 0; i < pixel_count; i++)
    {
        block->rgba[4u * i + 0] = resolved.background_color.r;
        block->rgba[4u * i + 1] = resolved.background_color.g;
        block->rgba[4u * i + 2] = resolved.background_color.b;
        block->rgba[4u * i + 3] = resolved.background_color.a;
    }

    uint32_t char_w = _text_block_scaled_px(block->layout.char_width_px, resolved.scale, 7);
    uint32_t line_h = _text_block_scaled_px(block->layout.line_height_px, resolved.scale, 14);
    uint32_t pad_x = _text_block_scaled_px(block->layout.padding_px[0], resolved.scale, 0);
    uint32_t pad_y = _text_block_scaled_px(block->layout.padding_px[1], resolved.scale, 0);
    for (uint32_t i = 0; i < block->text_size; i++)
    {
        if (block->text[i] == '\n')
            continue;

        uint32_t x = pad_x + block->layout_x[i] * char_w;
        uint32_t y = pad_y + block->layout_y[i] * line_h;
        DvzColor color = _text_block_color_at(block, i, resolved.text_color);
        _text_block_draw_byte(
            block, block->text[i], x, y, _text_block_style_at(block, i), color, resolved.scale);
    }

    block->raster_version++;
    return 0;
}



/**
 * Realize a rasterized text block as one image-like scene visual.
 *
 * @param block the text block
 * @param panel the owning panel
 * @param desc image placement descriptor, or NULL for centered defaults
 * @return 0 on success, -1 on invalid input or scene allocation failure
 */
int _scene_text_block_realize_image(
    DvzTextBlock* block, DvzPanel* panel, const DvzTextBlockImageDesc* desc)
{
    ANN(block);
    ANN(panel);
    if (panel->figure == NULL || panel->figure->scene == NULL || block->rgba == NULL ||
        block->raster_width == 0 || block->raster_height == 0)
    {
        return -1;
    }

    DvzTextBlockImageDesc resolved = {
        .position = {0.0f, 0.0f, 0.0f},
        .extent = {1.0f, 0.25f},
        .anchor = {0.0f, 0.0f},
        .z_layer = 0,
        .controller_mode = DVZ_CONTROLLER_APPLY,
    };
    if (desc != NULL)
        resolved = *desc;

    DvzScene* scene = panel->figure->scene;
    if (block->image_visual == NULL)
    {
        block->image_visual = dvz_image(scene, 0);
        if (block->image_visual == NULL)
            return -1;
        block->image_visual->visible = false;
        if (dvz_visual_set_alpha_mode(block->image_visual, DVZ_ALPHA_BLENDED) != 0)
            return -1;
        if (dvz_visual_set_depth_test(block->image_visual, false) != 0)
            return -1;
    }

    if (block->image_field == NULL)
    {
        block->image_field = dvz_sampled_field(
            scene, &(DvzSampledFieldDesc){
                       .dim = DVZ_FIELD_DIM_2D,
                       .format = DVZ_FIELD_FORMAT_RGBA8_UNORM,
                       .semantic = DVZ_FIELD_SEMANTIC_COLOR,
                       .width = block->raster_width,
                       .height = block->raster_height,
                       .depth = 1,
                   });
        if (block->image_field == NULL)
            return -1;
        block->image_width = block->raster_width;
        block->image_height = block->raster_height;
    }

    DvzFieldDataView view = {
        .data = block->rgba,
        .bytes_per_row = 4u * (uint64_t)block->raster_width,
        .rows_per_image = block->raster_height,
    };
    if (block->image_width != block->raster_width || block->image_height != block->raster_height)
    {
        if (!dvz_sampled_field_resize(
                block->image_field, block->raster_width, block->raster_height, 1, &view))
            return -1;
        block->image_width = block->raster_width;
        block->image_height = block->raster_height;
    }
    else
    {
        if (!dvz_sampled_field_set_data(block->image_field, &view))
            return -1;
    }

    vec3 positions[1] = {{resolved.position[0], resolved.position[1], resolved.position[2]}};
    vec2 extents[1] = {{resolved.extent[0], resolved.extent[1]}};
    vec2 anchors[1] = {{resolved.anchor[0], resolved.anchor[1]}};
    DvzVisualDataUpdate updates[3] = {
        {.attr_name = "position", .data = positions, .item_count = 1},
        {.attr_name = "extent", .data = extents, .item_count = 1},
        {.attr_name = "anchor", .data = anchors, .item_count = 1},
    };
    if (dvz_visual_set_data_many(block->image_visual, updates, 3) != 0)
        return -1;
    if (!dvz_visual_set_field(block->image_visual, "field", block->image_field))
        return -1;
    if (!block->image_attached)
    {
        if (dvz_panel_add_visual(
                panel, block->image_visual,
                &(DvzVisualAttachDesc){
                    .z_layer = resolved.z_layer,
                    .controller_mode = resolved.controller_mode,
                }) != 0)
            return -1;
        block->image_attached = true;
    }
    dvz_visual_set_visible(block->image_visual, true);
    return 0;
}
