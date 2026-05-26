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
 * @param active whether a current run is active
 * @param current current run state
 * @return 0 on success, -1 on text buffer overflow
 */
static int _text_block_append_byte(
    DvzTextBlock* block, char byte, uint32_t source_start, uint32_t source_end,
    uint32_t style_flags, bool* active, DvzTextBlockRun* current)
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
        else if (_text_block_starts_with(block->source, i, "&lt;"))
        {
            if (_text_block_append_byte(
                    block, '<', i, i + 4, style_flags, &active, &current) != 0)
                return -1;
            i += 4;
            continue;
        }
        else if (_text_block_starts_with(block->source, i, "&gt;"))
        {
            if (_text_block_append_byte(
                    block, '>', i, i + 4, style_flags, &active, &current) != 0)
                return -1;
            i += 4;
            continue;
        }
        else if (_text_block_starts_with(block->source, i, "&amp;"))
        {
            if (_text_block_append_byte(
                    block, '&', i, i + 5, style_flags, &active, &current) != 0)
                return -1;
            i += 5;
            continue;
        }
        else if (block->source[i] == '<' || block->source[i] == '&')
        {
            _text_block_diag(block, "literal unsupported markup", i);
        }

        if (_text_block_append_byte(
                block, block->source[i], i, i + 1, style_flags, &active, &current) != 0)
            return -1;
        i++;
    }

    if (_text_block_close_run(block, &active, &current) != 0)
        return -1;
    if (style_flags != DVZ_TEXT_BLOCK_STYLE_NONE)
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

    uint32_t line_chars = 0;
    uint32_t max_line_chars = 0;
    uint32_t line_count = 1;
    for (uint32_t i = 0; i < block->text_size; i++)
    {
        if (block->text[i] == '\n')
        {
            if (line_chars > max_line_chars)
                max_line_chars = line_chars;
            line_chars = 0;
            line_count++;
            continue;
        }
        if (wrap_chars > 0 && line_chars >= wrap_chars)
        {
            if (line_chars > max_line_chars)
                max_line_chars = line_chars;
            line_chars = 0;
            line_count++;
        }
        line_chars++;
    }
    if (line_chars > max_line_chars)
        max_line_chars = line_chars;

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
