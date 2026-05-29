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

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_overflow.h"
#include "_scene.h"
#include "datoviz/scene/text.h"
#include "text/text_internal.h"

#if defined(DVZ_HAS_FREETYPE) && DVZ_HAS_FREETYPE
#include <ft2build.h>
#include FT_FREETYPE_H
#endif



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
        dvz_snprintf(
            block->diagnostic, sizeof(block->diagnostic), "%s at byte %u", message, offset);
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
 * Composite one RGBA pixel with coverage into a text-block raster.
 *
 * @param block the text block
 * @param x x coordinate
 * @param y y coordinate
 * @param color source text color
 * @param coverage source coverage in the range 0..255
 */
static void _text_block_put_coverage_px(
    DvzTextBlock* block, uint32_t x, uint32_t y, const DvzColor color, uint8_t coverage)
{
    ANN(block);
    ANN(block->rgba);
    if (coverage == 0 || x >= block->raster_width || y >= block->raster_height)
        return;

    uint64_t idx = 4u * ((uint64_t)y * block->raster_width + x);
    uint32_t src_a = ((uint32_t)color.a * (uint32_t)coverage + 127u) / 255u;
    uint32_t inv_a = 255u - src_a;
    block->rgba[idx + 0] =
        (uint8_t)(((uint32_t)color.r * src_a + (uint32_t)block->rgba[idx + 0] * inv_a) / 255u);
    block->rgba[idx + 1] =
        (uint8_t)(((uint32_t)color.g * src_a + (uint32_t)block->rgba[idx + 1] * inv_a) / 255u);
    block->rgba[idx + 2] =
        (uint8_t)(((uint32_t)color.b * src_a + (uint32_t)block->rgba[idx + 2] * inv_a) / 255u);
    block->rgba[idx + 3] =
        (uint8_t)(src_a + ((uint32_t)block->rgba[idx + 3] * inv_a + 127u) / 255u);
}



#if defined(DVZ_HAS_FREETYPE) && DVZ_HAS_FREETYPE
typedef struct DvzTextBlockFtCtx DvzTextBlockFtCtx;
typedef struct DvzTextBlockLayoutItem DvzTextBlockLayoutItem;

struct DvzTextBlockFtCtx
{
    FT_Library library;
    FT_Face faces[DVZ_TEXT_BLOCK_FACE_COUNT];
    DvzFont* fonts[DVZ_TEXT_BLOCK_FACE_COUNT];
    bool owns_library;
    float scale;
    float ascender;
    float descender;
    float line_height;
};


struct DvzTextBlockLayoutItem
{
    uint32_t text_start;
    uint32_t text_end;
    uint32_t codepoint;
    uint32_t requested_style;
    uint32_t style_flags;
    uint32_t face_slot;
    uint32_t glyph_index;
    float advance;
    DvzColor color;
    bool visible;
    bool whitespace;
    bool newline;
};


static uint8_t _text_block_ft_coverage(const FT_Bitmap* bitmap, uint32_t x, uint32_t y);


/**
 * Return whether one path can be read as a font file.
 *
 * @param path filesystem path
 * @return whether the file exists and has non-zero length
 */
static bool _text_block_font_path_available(const char* path)
{
    if (path == NULL || path[0] == '\0')
        return false;

    FILE* fp = fopen(path, "rb");
    if (fp == NULL)
        return false;
    int seek_rc = fseek(fp, 0, SEEK_END);
    long size = seek_rc == 0 ? ftell(fp) : 0;
    fclose(fp);
    return size > 0;
}


/**
 * Return the repository default path for a known font face.
 *
 * @param family font family
 * @param style font style
 * @return path, or NULL when no deterministic file is known
 */
static const char* _text_block_known_font_path(const char* family, const char* style)
{
    if (family == NULL || family[0] == '\0')
        family = "Roboto";
    if (style == NULL || style[0] == '\0')
        style = "Regular";

    if (strcmp(family, "Roboto") == 0)
    {
        if (strcmp(style, "Regular") == 0)
            return "data/fonts/Roboto-Regular.ttf";
        if (strcmp(style, "Bold") == 0)
            return "data/fonts/Roboto-Bold.ttf";
        if (strcmp(style, "Italic") == 0)
            return "data/fonts/Roboto-Italic.ttf";
        if (strcmp(style, "Bold Italic") == 0)
            return "data/fonts/Roboto-BoldItalic.ttf";
        if (strcmp(style, "Medium") == 0)
            return "data/fonts/Roboto-Medium.ttf";
        if (strcmp(style, "Medium Italic") == 0)
            return "data/fonts/Roboto-MediumItalic.ttf";
        if (strcmp(style, "Light") == 0)
            return "data/fonts/Roboto-Light.ttf";
        if (strcmp(style, "Light Italic") == 0)
            return "data/fonts/Roboto-LightItalic.ttf";
        if (strcmp(style, "Black") == 0)
            return "data/fonts/Roboto-Black.ttf";
        if (strcmp(style, "Black Italic") == 0)
            return "data/fonts/Roboto-BlackItalic.ttf";
    }
    if (strcmp(family, "Roboto Mono") == 0 || strcmp(family, "RobotoMono") == 0)
    {
        if (strcmp(style, "Regular") == 0 || strcmp(style, "Medium") == 0)
            return "data/fonts/RobotoMono-Medium.ttf";
    }
    if (strcmp(family, "Inconsolata") == 0 && strcmp(style, "Regular") == 0)
        return "data/fonts/Inconsolata-Regular.ttf";
    if (strcmp(family, "Droid Sans") == 0 && strcmp(style, "Regular") == 0)
        return "data/fonts/DroidSans.ttf";
    return NULL;
}


/**
 * Return an existing scene font matching a descriptor.
 *
 * @param scene the scene
 * @param desc font descriptor
 * @return matching scene font, or NULL
 */
static DvzFont* _text_block_find_font(DvzScene* scene, const DvzFontDesc* desc)
{
    ANN(scene);
    ANN(desc);

    for (uint32_t i = 0; i < scene->font_count; i++)
    {
        bool path_matches = false;
        if (desc->path == NULL || desc->path[0] == '\0')
            path_matches = scene->fonts[i].path[0] == '\0';
        else
            path_matches = strcmp(scene->fonts[i].path, desc->path) == 0;

        if (
            path_matches && strcmp(scene->fonts[i].family, desc->family) == 0 &&
            strcmp(scene->fonts[i].style, desc->style) == 0 &&
            scene->fonts[i].face_index == desc->face_index)
        {
            return &scene->fonts[i];
        }
    }
    return NULL;
}


/**
 * Resolve or create a scene-owned font matching a descriptor.
 *
 * @param scene the scene
 * @param desc font descriptor
 * @return matching scene font, or NULL on allocation failure
 */
static DvzFont* _text_block_get_font(DvzScene* scene, const DvzFontDesc* desc)
{
    ANN(scene);
    ANN(desc);
    DvzFont* existing = _text_block_find_font(scene, desc);
    if (existing != NULL)
        return existing;
    return dvz_font(scene, desc);
}


/**
 * Resolve one requested text-block font slot.
 *
 * @param scene the scene
 * @param layout layout descriptor with optional explicit style fonts
 * @param slot requested face slot
 * @return scene-owned font, or NULL when the real face is unavailable
 */
static DvzFont*
_text_block_resolve_font_slot(DvzScene* scene, const DvzTextBlockLayout* layout, uint32_t slot)
{
    ANN(scene);
    ANN(layout);
    if (slot == DVZ_TEXT_BLOCK_FACE_REGULAR && layout->font != NULL)
        return layout->font;
    if (slot == DVZ_TEXT_BLOCK_FACE_BOLD && layout->bold_font != NULL)
        return layout->bold_font;
    if (slot == DVZ_TEXT_BLOCK_FACE_ITALIC && layout->italic_font != NULL)
        return layout->italic_font;
    if (slot == DVZ_TEXT_BLOCK_FACE_BOLD_ITALIC && layout->bold_italic_font != NULL)
        return layout->bold_italic_font;

    DvzFontDesc desc = scene->font_defaults.sans;
    const char* family = desc.family != NULL && desc.family[0] != '\0' ? desc.family : "Roboto";
    const char* style = "Regular";
    if (slot == DVZ_TEXT_BLOCK_FACE_BOLD)
        style = "Bold";
    else if (slot == DVZ_TEXT_BLOCK_FACE_ITALIC)
        style = "Italic";
    else if (slot == DVZ_TEXT_BLOCK_FACE_BOLD_ITALIC)
        style = "Bold Italic";

    const char* path = NULL;
    if (slot == DVZ_TEXT_BLOCK_FACE_REGULAR && desc.path != NULL && desc.path[0] != '\0')
        path = desc.path;
    else if (slot != DVZ_TEXT_BLOCK_FACE_REGULAR)
        path = _text_block_known_font_path(family, style);
    if (slot != DVZ_TEXT_BLOCK_FACE_REGULAR && path == NULL)
        return NULL;
    if (path != NULL && !_text_block_font_path_available(path))
        return NULL;

    desc.path = path;
    desc.family = family;
    desc.style = style;
    return _text_block_get_font(scene, &desc);
}


/**
 * Emit one warning when a requested real style face is unavailable.
 *
 * @param block the text block
 * @param style_flags unavailable style combination
 */
static void _text_block_warn_missing_style(DvzTextBlock* block, uint32_t style_flags)
{
    ANN(block);
    uint32_t face_bits =
        style_flags & (DVZ_TEXT_BLOCK_STYLE_BOLD | DVZ_TEXT_BLOCK_STYLE_ITALIC);
    if (face_bits == 0)
        return;

    uint32_t face_bit = 0;
    if (face_bits == (DVZ_TEXT_BLOCK_STYLE_BOLD | DVZ_TEXT_BLOCK_STYLE_ITALIC))
        face_bit = 1u << DVZ_TEXT_BLOCK_FACE_BOLD_ITALIC;
    else if ((face_bits & DVZ_TEXT_BLOCK_STYLE_BOLD) != 0)
        face_bit = 1u << DVZ_TEXT_BLOCK_FACE_BOLD;
    else if ((face_bits & DVZ_TEXT_BLOCK_STYLE_ITALIC) != 0)
        face_bit = 1u << DVZ_TEXT_BLOCK_FACE_ITALIC;
    if (face_bit == 0 || (block->missing_style_flags & face_bit) != 0)
        return;

    block->missing_style_flags |= face_bit;
    if (face_bits == (DVZ_TEXT_BLOCK_STYLE_BOLD | DVZ_TEXT_BLOCK_STYLE_ITALIC))
    {
        log_warn(
            "rich text block requested bold italic, but no real bold-italic face is available");
        _text_block_diag(block, "missing bold italic text block face", 0);
    }
    else if ((face_bits & DVZ_TEXT_BLOCK_STYLE_BOLD) != 0)
    {
        log_warn("rich text block requested bold, but no real bold face is available");
        _text_block_diag(block, "missing bold text block face", 0);
    }
    else if ((face_bits & DVZ_TEXT_BLOCK_STYLE_ITALIC) != 0)
    {
        log_warn("rich text block requested italic, but no real italic face is available");
        _text_block_diag(block, "missing italic text block face", 0);
    }
}


/**
 * Resolve the exact face slot for requested style flags.
 *
 * @param block the text block
 * @param requested_style requested style flags
 * @param out_style effective style flags
 * @return face slot used for layout and rasterization
 */
static uint32_t _text_block_effective_face(
    DvzTextBlock* block, uint32_t requested_style, uint32_t* out_style)
{
    ANN(block);
    ANN(out_style);
    uint32_t face_bits =
        requested_style & (DVZ_TEXT_BLOCK_STYLE_BOLD | DVZ_TEXT_BLOCK_STYLE_ITALIC);
    uint32_t slot = DVZ_TEXT_BLOCK_FACE_REGULAR;
    if (face_bits == DVZ_TEXT_BLOCK_STYLE_BOLD)
        slot = DVZ_TEXT_BLOCK_FACE_BOLD;
    else if (face_bits == DVZ_TEXT_BLOCK_STYLE_ITALIC)
        slot = DVZ_TEXT_BLOCK_FACE_ITALIC;
    else if (face_bits == (DVZ_TEXT_BLOCK_STYLE_BOLD | DVZ_TEXT_BLOCK_STYLE_ITALIC))
        slot = DVZ_TEXT_BLOCK_FACE_BOLD_ITALIC;

    if (slot != DVZ_TEXT_BLOCK_FACE_REGULAR && block->layout_fonts[slot] == NULL)
    {
        _text_block_warn_missing_style(block, face_bits);
        face_bits = 0;
        slot = DVZ_TEXT_BLOCK_FACE_REGULAR;
    }
    *out_style = (requested_style & ~(
                                      (uint32_t)DVZ_TEXT_BLOCK_STYLE_BOLD |
                                      (uint32_t)DVZ_TEXT_BLOCK_STYLE_ITALIC)) |
                 face_bits;
    return slot;
}


/**
 * Decode one UTF-8 codepoint from a parsed text block.
 *
 * @param block the text block
 * @param inout_index text byte index, advanced by the consumed sequence
 * @param out_start output start byte
 * @param out_end output end byte
 * @param out_codepoint output codepoint
 * @return whether a codepoint was decoded
 */
static bool _text_block_utf8_next(
    DvzTextBlock* block, uint32_t* inout_index, uint32_t* out_start, uint32_t* out_end,
    uint32_t* out_codepoint)
{
    ANN(block);
    ANN(inout_index);
    ANN(out_start);
    ANN(out_end);
    ANN(out_codepoint);
    uint32_t i = *inout_index;
    if (i >= block->text_size || block->text[i] == '\0')
        return false;

    *out_start = i;
    const uint8_t* s = (const uint8_t*)block->text;
    uint8_t b0 = s[i];
    if (b0 < 0x80u)
    {
        *out_codepoint = b0;
        *inout_index = i + 1u;
        *out_end = *inout_index;
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
        _text_block_diag(block, "invalid UTF-8 in text block", i);
        log_warn("rich text block skipped invalid UTF-8 byte at %u", i);
        *inout_index = i + 1u;
        *out_end = *inout_index;
        *out_codepoint = 0;
        return true;
    }

    for (uint32_t j = 1; j < needed; j++)
    {
        if (i + j >= block->text_size || (s[i + j] & 0xC0u) != 0x80u)
        {
            _text_block_diag(block, "invalid UTF-8 in text block", i);
            log_warn("rich text block skipped invalid UTF-8 sequence at %u", i);
            *inout_index = i + 1u;
            *out_end = *inout_index;
            *out_codepoint = 0;
            return true;
        }
        cp = (cp << 6) | (uint32_t)(s[i + j] & 0x3Fu);
    }

    if (cp < min_cp || cp > 0x10FFFFu || (cp >= 0xD800u && cp <= 0xDFFFu))
    {
        _text_block_diag(block, "invalid UTF-8 in text block", i);
        log_warn("rich text block skipped invalid UTF-8 codepoint at %u", i);
        cp = 0;
    }
    *inout_index = i + needed;
    *out_end = *inout_index;
    *out_codepoint = cp;
    return true;
}


/**
 * Open all resolved FreeType faces for a text block.
 *
 * @param block the text block
 * @param scale raster scale applied to the requested font size
 * @param out output FreeType context
 * @return whether at least the regular face is available
 */
static bool _text_block_ft_open(DvzTextBlock* block, float scale, DvzTextBlockFtCtx* out)
{
    ANN(block);
    ANN(out);
    dvz_memset(out, sizeof(DvzTextBlockFtCtx), 0, sizeof(DvzTextBlockFtCtx));
    if (FT_Init_FreeType(&out->library) != 0)
        return false;
    out->owns_library = true;
    out->scale = scale > 0.0f ? scale : 1.0f;

    float font_size = block->layout.font_size_px > 0.0f ? block->layout.font_size_px :
                                                           0.78f * block->layout.line_height_px;
    if (font_size <= 0.0f)
        font_size = 12.0f;
    uint32_t font_px = _text_block_scaled_px(font_size, out->scale, 10);

    for (uint32_t i = 0; i < DVZ_TEXT_BLOCK_FACE_COUNT; i++)
    {
        DvzFont* font = block->layout_fonts[i];
        if (font == NULL || !_scene_font_ensure_bytes(font))
            continue;

        if (FT_New_Memory_Face(
                out->library, (const FT_Byte*)font->ttf_bytes, (FT_Long)font->ttf_size,
                (FT_Long)font->face_index, &out->faces[i]) != 0)
        {
            out->faces[i] = NULL;
            continue;
        }
        if (FT_Set_Pixel_Sizes(out->faces[i], 0, (FT_UInt)font_px) != 0)
        {
            FT_Done_Face(out->faces[i]);
            out->faces[i] = NULL;
            continue;
        }
        out->fonts[i] = font;
    }

    FT_Face face = out->faces[DVZ_TEXT_BLOCK_FACE_REGULAR];
    if (face == NULL)
    {
        for (uint32_t i = 0; i < DVZ_TEXT_BLOCK_FACE_COUNT; i++)
        {
            if (out->faces[i] != NULL)
                FT_Done_Face(out->faces[i]);
        }
        FT_Done_FreeType(out->library);
        dvz_memset(out, sizeof(DvzTextBlockFtCtx), 0, sizeof(DvzTextBlockFtCtx));
        return false;
    }
    out->ascender = face->size != NULL ? (float)face->size->metrics.ascender / 64.0f : 0.0f;
    out->descender = face->size != NULL ? (float)face->size->metrics.descender / 64.0f : 0.0f;
    out->line_height = face->size != NULL ? (float)face->size->metrics.height / 64.0f : 0.0f;
    if (out->line_height <= 0.0f)
        out->line_height = 1.2f * font_size * out->scale;
    return true;
}


/**
 * Release FreeType faces opened for a text block.
 *
 * @param ctx FreeType context
 */
static void _text_block_ft_close(DvzTextBlockFtCtx* ctx)
{
    if (ctx == NULL)
        return;
    for (uint32_t i = 0; i < DVZ_TEXT_BLOCK_FACE_COUNT; i++)
    {
        if (ctx->faces[i] != NULL)
        {
            FT_Done_Face(ctx->faces[i]);
            ctx->faces[i] = NULL;
        }
    }
    if (ctx->owns_library)
        FT_Done_FreeType(ctx->library);
    dvz_memset(ctx, sizeof(DvzTextBlockFtCtx), 0, sizeof(DvzTextBlockFtCtx));
}


/**
 * Return kerning between two glyphs when they share a FreeType face.
 *
 * @param ctx FreeType context
 * @param face_slot face slot
 * @param left left glyph index
 * @param right right glyph index
 * @return kerning advance in logical or raster pixels for the context scale
 */
static float _text_block_kerning(
    const DvzTextBlockFtCtx* ctx, uint32_t face_slot, uint32_t left, uint32_t right)
{
    ANN(ctx);
    if (face_slot >= DVZ_TEXT_BLOCK_FACE_COUNT || left == 0 || right == 0)
        return 0.0f;
    FT_Face face = ctx->faces[face_slot];
    if (face == NULL || !FT_HAS_KERNING(face))
        return 0.0f;

    FT_Vector kerning = {0};
    if (FT_Get_Kerning(face, left, right, FT_KERNING_DEFAULT, &kerning) != 0)
        return 0.0f;
    return (float)kerning.x / 64.0f;
}


/**
 * Return the measured width of a contiguous glyph item range.
 *
 * @param ctx FreeType context
 * @param items item array
 * @param first first item index
 * @param last one past the last item index
 * @return range width in logical pixels
 */
static float _text_block_item_range_width(
    const DvzTextBlockFtCtx* ctx, const DvzTextBlockLayoutItem* items, uint32_t first,
    uint32_t last)
{
    ANN(ctx);
    ANN(items);
    float width = 0.0f;
    uint32_t previous_slot = UINT32_MAX;
    uint32_t previous_glyph = 0;
    for (uint32_t i = first; i < last; i++)
    {
        const DvzTextBlockLayoutItem* item = &items[i];
        if (item->newline)
            break;
        if (item->visible && previous_slot == item->face_slot)
            width += _text_block_kerning(ctx, item->face_slot, previous_glyph, item->glyph_index);
        width += item->advance;
        previous_slot = item->visible ? item->face_slot : UINT32_MAX;
        previous_glyph = item->visible ? item->glyph_index : 0;
    }
    return width;
}


/**
 * Draw one rendered FreeType glyph bitmap into a text-block raster.
 *
 * @param block the text block
 * @param bitmap source glyph bitmap
 * @param dst_x destination x origin, signed before clipping
 * @param dst_y destination y origin, signed before clipping
 * @param color text color
 */
static void _text_block_draw_ft_bitmap(
    DvzTextBlock* block, const FT_Bitmap* bitmap, int32_t dst_x, int32_t dst_y,
    const DvzColor color)
{
    ANN(block);
    ANN(bitmap);
    uint32_t rows = (uint32_t)bitmap->rows;
    uint32_t width = (uint32_t)bitmap->width;
    for (uint32_t y = 0; y < rows; y++)
    {
        for (uint32_t x = 0; x < width; x++)
        {
            uint8_t coverage = _text_block_ft_coverage(bitmap, x, y);
            if (coverage == 0)
                continue;
            int32_t px = dst_x + (int32_t)x;
            int32_t py = dst_y + (int32_t)y;
            if (px < 0 || py < 0)
                continue;
            _text_block_put_coverage_px(block, (uint32_t)px, (uint32_t)py, color, coverage);
        }
    }
}


/**
 * Return one FreeType bitmap coverage sample.
 *
 * @param bitmap source glyph bitmap
 * @param x x coordinate in the bitmap
 * @param y y coordinate in the bitmap
 * @return coverage in the range 0..255
 */
static uint8_t _text_block_ft_coverage(const FT_Bitmap* bitmap, uint32_t x, uint32_t y)
{
    ANN(bitmap);
    if (x >= (uint32_t)bitmap->width || y >= (uint32_t)bitmap->rows || bitmap->buffer == NULL)
        return 0;

    int pitch = bitmap->pitch;
    uint32_t row_index = y;
    if (pitch < 0)
    {
        pitch = -pitch;
        row_index = (uint32_t)bitmap->rows - 1u - y;
    }
    const uint8_t* row = bitmap->buffer + (uint64_t)row_index * (uint32_t)pitch;
    if (bitmap->pixel_mode == FT_PIXEL_MODE_GRAY)
        return row[x];
    if (bitmap->pixel_mode == FT_PIXEL_MODE_MONO)
        return (row[x / 8u] & (uint8_t)(0x80u >> (x % 8u))) != 0 ? 255 : 0;
    return 0;
}


/**
 * Resolve text-block fonts and record real faces available to layout.
 *
 * @param block the text block
 * @return whether a regular face is available
 */
static bool _text_block_resolve_fonts(DvzTextBlock* block)
{
    ANN(block);
    DvzScene* scene = block->layout.scene;
    if (scene == NULL && block->layout.font != NULL)
        scene = block->layout.font->scene;
    if (scene == NULL && block->layout.bold_font != NULL)
        scene = block->layout.bold_font->scene;
    if (scene == NULL && block->layout.italic_font != NULL)
        scene = block->layout.italic_font->scene;
    if (scene == NULL && block->layout.bold_italic_font != NULL)
        scene = block->layout.bold_italic_font->scene;
    if (scene == NULL)
        return false;

    for (uint32_t i = 0; i < DVZ_TEXT_BLOCK_FACE_COUNT; i++)
        block->layout_fonts[i] = _text_block_resolve_font_slot(scene, &block->layout, i);
    return block->layout_fonts[DVZ_TEXT_BLOCK_FACE_REGULAR] != NULL;
}


/**
 * Append one measured layout item to a text block.
 *
 * @param block the text block
 * @param item source layout item
 * @param x logical x position
 * @param baseline_y logical baseline y position
 * @return whether the item was appended
 */
static bool _text_block_append_layout_item(
    DvzTextBlock* block, const DvzTextBlockLayoutItem* item, float x, float baseline_y)
{
    ANN(block);
    ANN(item);
    if (block->layout_glyph_count >= DVZ_SCENE_TEXT_BLOCK_TEXT_SIZE)
    {
        _text_block_diag(block, "text block layout capacity exceeded", item->text_start);
        return false;
    }
    uint32_t idx = block->layout_glyph_count++;
    block->layout_text_start[idx] = item->text_start;
    block->layout_text_end[idx] = item->text_end;
    block->layout_codepoint[idx] = item->codepoint;
    block->layout_glyph_index[idx] = item->glyph_index;
    block->layout_style_flags[idx] = item->style_flags;
    block->layout_face_slot[idx] = item->face_slot;
    block->layout_pos_x[idx] = x;
    block->layout_baseline_y[idx] = baseline_y;
    block->layout_advance[idx] = item->advance;
    block->layout_color[idx] = item->color;
    block->layout_visible[idx] = item->visible;
    return true;
}


/**
 * Measure and lay out a text block with real FreeType glyph advances.
 *
 * @param block the text block
 * @param text_color fallback text color
 * @return whether FreeType layout succeeded
 */
static bool _text_block_measure_freetype(DvzTextBlock* block, DvzColor text_color)
{
    ANN(block);

    if (!_text_block_resolve_fonts(block))
        return false;
    DvzTextBlockFtCtx ctx = {0};
    if (!_text_block_ft_open(block, 1.0f, &ctx))
        return false;
    for (uint32_t i = 0; i < DVZ_TEXT_BLOCK_FACE_COUNT; i++)
    {
        if (ctx.faces[i] == NULL)
            block->layout_fonts[i] = NULL;
    }

    block->missing_style_flags = 0;
    DvzTextBlockLayoutItem items[DVZ_SCENE_TEXT_BLOCK_TEXT_SIZE] = {0};
    uint32_t item_count = 0;
    uint32_t byte_index = 0;
    uint32_t start = 0;
    uint32_t end = 0;
    uint32_t cp = 0;
    while (_text_block_utf8_next(block, &byte_index, &start, &end, &cp))
    {
        if (cp == 0)
            continue;
        if (item_count >= DVZ_SCENE_TEXT_BLOCK_TEXT_SIZE)
        {
            _text_block_diag(block, "text block layout capacity exceeded", start);
            _text_block_ft_close(&ctx);
            return false;
        }

        DvzTextBlockLayoutItem* item = &items[item_count++];
        item->text_start = start;
        item->text_end = end;
        item->codepoint = cp;
        item->requested_style = _text_block_style_at(block, start);
        item->face_slot =
            _text_block_effective_face(block, item->requested_style, &item->style_flags);
        item->color = _text_block_color_at(block, start, text_color);
        item->newline = cp == '\n';
        item->whitespace = cp == ' ' || cp == '\t';
        if (item->newline)
            continue;

        FT_Face item_face = ctx.faces[item->face_slot];
        if (item_face == NULL)
            continue;
        item->glyph_index = FT_Get_Char_Index(item_face, (FT_ULong)cp);
        if (item->glyph_index == 0 && !item->whitespace)
        {
            _text_block_diag(block, "missing text block glyph", start);
            log_warn("rich text block skipped missing glyph U+%04X", cp);
            continue;
        }
        if (item->glyph_index != 0 &&
            FT_Load_Glyph(item_face, item->glyph_index, FT_LOAD_DEFAULT) == 0)
        {
            item->advance = (float)item_face->glyph->advance.x / 64.0f;
            item->visible = !item->whitespace;
        }
        if (item->whitespace && item->advance <= 0.0f)
            item->advance = cp == '\t' ? 4.0f * block->layout.font_size_px :
                                          0.25f * block->layout.font_size_px;
    }

    block->layout_glyph_count = 0;
    float pad_x = block->layout.padding_px[0];
    float pad_y = block->layout.padding_px[1];
    float line_height = block->layout.line_height_px > 0.0f ? block->layout.line_height_px :
                                                              ctx.line_height;
    float ascender = ctx.ascender;
    if (line_height <= 0.0f)
        line_height = 14.0f;
    if (ascender <= 0.0f)
        ascender = 0.78f * line_height;
    float max_content_width = 0.0f;
    if (block->layout.max_width_px > 2.0f * pad_x)
        max_content_width = block->layout.max_width_px - 2.0f * pad_x;

    float cursor_x = 0.0f;
    float max_line_width = 0.0f;
    uint32_t line = 0;
    uint32_t previous_slot = UINT32_MAX;
    uint32_t previous_glyph = 0;
    for (uint32_t i = 0; i < item_count;)
    {
        if (items[i].newline)
        {
            if (cursor_x > max_line_width)
                max_line_width = cursor_x;
            cursor_x = 0.0f;
            previous_slot = UINT32_MAX;
            previous_glyph = 0;
            line++;
            i++;
            continue;
        }
        if (items[i].whitespace)
        {
            if (cursor_x > 0.0f)
                cursor_x += items[i].advance;
            previous_slot = UINT32_MAX;
            previous_glyph = 0;
            i++;
            continue;
        }

        uint32_t word_end = i;
        while (word_end < item_count && !items[word_end].newline && !items[word_end].whitespace)
            word_end++;
        float word_width = _text_block_item_range_width(&ctx, items, i, word_end);
        if (max_content_width > 0.0f && cursor_x > 0.0f &&
            cursor_x + word_width > max_content_width)
        {
            if (cursor_x > max_line_width)
                max_line_width = cursor_x;
            cursor_x = 0.0f;
            previous_slot = UINT32_MAX;
            previous_glyph = 0;
            line++;
        }

        for (; i < word_end; i++)
        {
            DvzTextBlockLayoutItem* item = &items[i];
            float kern = 0.0f;
            if (item->visible && previous_slot == item->face_slot)
                kern =
                    _text_block_kerning(&ctx, item->face_slot, previous_glyph, item->glyph_index);
            if (max_content_width > 0.0f && cursor_x > 0.0f &&
                cursor_x + kern + item->advance > max_content_width)
            {
                if (cursor_x > max_line_width)
                    max_line_width = cursor_x;
                cursor_x = 0.0f;
                previous_slot = UINT32_MAX;
                previous_glyph = 0;
                line++;
                kern = 0.0f;
            }
            cursor_x += kern;
            float baseline_y = pad_y + ascender + (float)line * line_height;
            if (!_text_block_append_layout_item(block, item, pad_x + cursor_x, baseline_y))
            {
                _text_block_ft_close(&ctx);
                return false;
            }
            cursor_x += item->advance;
            previous_slot = item->visible ? item->face_slot : UINT32_MAX;
            previous_glyph = item->visible ? item->glyph_index : 0;
        }
    }
    if (cursor_x > max_line_width)
        max_line_width = cursor_x;

    uint32_t line_count = line + 1u;
    float width_px = max_line_width + 2.0f * pad_x;
    float height_px = (float)line_count * line_height + 2.0f * pad_y;
    if (width_px <= 0.0f)
        width_px = 1.0f;
    if (height_px <= 0.0f)
        height_px = line_height > 0.0f ? line_height : 1.0f;

    block->layout_line_count = line_count;
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
    block->metrics.baseline = pad_y + ascender;
    block->metrics.ascender = ascender;
    block->metrics.descender = ctx.descender;
    block->metrics.line_height = line_height;

    _text_block_ft_close(&ctx);
    return true;
}


/**
 * Rasterize a measured text block through FreeType.
 *
 * @param block the text block
 * @param desc resolved raster descriptor
 * @return whether FreeType drawing was used successfully
 */
static bool _text_block_rasterize_freetype(
    DvzTextBlock* block, const DvzTextBlockRasterDesc* desc)
{
    ANN(block);
    ANN(desc);
    if (block->layout_glyph_count == 0)
        return false;
    DvzTextBlockFtCtx ctx = {0};
    if (!_text_block_ft_open(block, desc->scale, &ctx))
        return false;

    for (uint32_t i = 0; i < block->layout_glyph_count; i++)
    {
        if (!block->layout_visible[i])
            continue;
        uint32_t face_slot = block->layout_face_slot[i];
        if (face_slot >= DVZ_TEXT_BLOCK_FACE_COUNT || ctx.faces[face_slot] == NULL)
            continue;
        FT_Face face = ctx.faces[face_slot];
        if (FT_Load_Glyph(face, block->layout_glyph_index[i], FT_LOAD_DEFAULT) != 0 ||
            FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL) != 0)
        {
            _text_block_ft_close(&ctx);
            return false;
        }

        float scale = desc->scale > 0.0f ? desc->scale : 1.0f;
        int32_t dst_x = (int32_t)lroundf(block->layout_pos_x[i] * scale) +
                        face->glyph->bitmap_left;
        int32_t dst_y = (int32_t)lroundf(block->layout_baseline_y[i] * scale) -
                        face->glyph->bitmap_top;
        _text_block_draw_ft_bitmap(
            block, &face->glyph->bitmap, dst_x, dst_y, block->layout_color[i]);

        if ((block->layout_style_flags[i] & DVZ_TEXT_BLOCK_STYLE_UNDERLINE) != 0)
        {
            uint32_t underline_y =
                (uint32_t)lroundf((block->layout_baseline_y[i] + 2.0f) * scale);
            uint32_t x0 = (uint32_t)fmaxf(0.0f, floorf(block->layout_pos_x[i] * scale));
            uint32_t x1 = (uint32_t)ceilf(
                (block->layout_pos_x[i] + block->layout_advance[i]) * scale);
            uint32_t thickness = scale > 2.0f ? (uint32_t)floorf(scale) : 1u;
            for (uint32_t y = 0; y < thickness; y++)
            {
                for (uint32_t x = x0; x < x1; x++)
                    _text_block_put_coverage_px(
                        block, x, underline_y + y, block->layout_color[i], 255);
            }
        }
    }

    _text_block_ft_close(&ctx);
    return true;
}
#endif



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
 * @param style_flags active style flags; only underline is honored by this no-font path
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
    uint32_t underline_y = y + line_h - 2u;
    for (uint32_t gy = 0; gy < glyph_h; gy++)
    {
        uint32_t row = glyph_h > 1 ? (7u * gy) / glyph_h : 0;
        for (uint32_t gx = 0; gx < glyph_w; gx++)
        {
            uint32_t col = glyph_w > 1 ? (5u * gx) / glyph_w : 0;
            if (!_text_block_glyph_cell(byte, col, row))
                continue;
            _text_block_put_px(block, x + 1u + gx, y + 2u + gy, color);
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


/**
 * Return whether a layout requests real font-backed text shaping.
 *
 * @param layout layout descriptor
 * @return whether scene/font-backed layout is requested
 */
static bool _text_block_layout_uses_fonts(const DvzTextBlockLayout* layout)
{
    if (layout == NULL)
        return false;
    return layout->scene != NULL || layout->font != NULL || layout->bold_font != NULL ||
           layout->italic_font != NULL || layout->bold_italic_font != NULL;
}



/**
 * Return whether two text-block image placement descriptors match exactly.
 *
 * @param a first descriptor
 * @param b second descriptor
 * @return true when all placement fields are identical
 */
static bool _text_block_image_desc_equal(
    const DvzTextBlockImageDesc* a, const DvzTextBlockImageDesc* b)
{
    ANN(a);
    ANN(b);
    return a->position[0] == b->position[0] && a->position[1] == b->position[1] &&
           a->position[2] == b->position[2] && a->extent[0] == b->extent[0] &&
           a->extent[1] == b->extent[1] && a->position_px[0] == b->position_px[0] &&
           a->position_px[1] == b->position_px[1] &&
           a->position_px[2] == b->position_px[2] && a->extent_px[0] == b->extent_px[0] &&
           a->extent_px[1] == b->extent_px[1] && a->anchor[0] == b->anchor[0] &&
           a->anchor[1] == b->anchor[1] && a->pixel_space == b->pixel_space &&
           a->z_layer == b->z_layer && a->controller_mode == b->controller_mode;
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
    _scene_text_block_set_source(block, source);
}



/**
 * Replace source text while preserving realized image resources.
 *
 * @param block the text block
 * @param source UTF-8 source text, or NULL for an empty block
 */
void _scene_text_block_set_source(DvzTextBlock* block, const char* source)
{
    ANN(block);
    uint8_t* rgba = block->rgba;
    uint64_t rgba_size = block->rgba_size;
    uint64_t raster_version = block->raster_version;
    DvzVisual* image_visual = block->image_visual;
    DvzSampledField* image_field = block->image_field;
    uint32_t image_width = block->image_width;
    uint32_t image_height = block->image_height;
    uint64_t image_raster_version = block->image_raster_version;
    DvzTextBlockImageDesc image_desc = block->image_desc;
    bool image_desc_valid = block->image_desc_valid;
    bool image_attached = block->image_attached;

    dvz_memset(block, sizeof(DvzTextBlock), 0, sizeof(DvzTextBlock));
    block->rgba = rgba;
    block->rgba_size = rgba_size;
    block->raster_version = raster_version;
    block->image_visual = image_visual;
    block->image_field = image_field;
    block->image_width = image_width;
    block->image_height = image_height;
    block->image_raster_version = image_raster_version;
    block->image_desc = image_desc;
    block->image_desc_valid = image_desc_valid;
    block->image_attached = image_attached;

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
    block->image_raster_version = 0;
    dvz_memset(
        &block->image_desc, sizeof(DvzTextBlockImageDesc), 0, sizeof(DvzTextBlockImageDesc));
    block->image_desc_valid = false;
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
 * Measure a parsed text block.
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
    if (resolved.font_size_px <= 0.0f)
        resolved.font_size_px = resolved.line_height_px > 0.0f ? 0.78f * resolved.line_height_px :
                                                                  12.0f;
    if (resolved.char_width_px <= 0.0f)
        resolved.char_width_px = 7.0f;
    if (resolved.line_height_px <= 0.0f)
        resolved.line_height_px = 1.25f * resolved.font_size_px;
    block->layout = resolved;

    bool uses_fonts = _text_block_layout_uses_fonts(&resolved);
#if defined(DVZ_HAS_FREETYPE) && DVZ_HAS_FREETYPE
    if (uses_fonts)
    {
        DvzColor default_color = {255, 255, 255, 255};
        if (_text_block_measure_freetype(block, default_color))
            return 0;
        log_warn("rich text block FreeType measurement failed");
        return -1;
    }
#else
    if (uses_fonts)
    {
        log_warn("rich text block requested font-backed layout without FreeType support");
        return -1;
    }
#endif

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

    float width_px =
        (float)max_line_chars * resolved.char_width_px + 2.0f * resolved.padding_px[0];
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

    DvzTextBlockRasterDesc resolved = {
        .text_color = {255, 255, 255, 255},
        .background_color = {0, 0, 0, 0},
        .scale = 1.0f,
    };
    if (desc != NULL)
        resolved = *desc;
    if (resolved.scale <= 0.0f)
        resolved.scale = 1.0f;
    bool layout_changed = false;
    if (resolved.scene != NULL && block->layout.scene == NULL)
    {
        block->layout.scene = resolved.scene;
        layout_changed = true;
    }
    if (resolved.font != NULL && block->layout.font == NULL)
    {
        block->layout.font = resolved.font;
        layout_changed = true;
    }
    if (resolved.font_size_px > 0.0f && block->layout.font_size_px <= 0.0f)
    {
        block->layout.font_size_px = resolved.font_size_px;
        layout_changed = true;
    }
    if (
        block->metrics.advance[0] <= 0.0f || block->metrics.advance[1] <= 0.0f ||
        layout_changed)
    {
        if (_scene_text_block_measure(block, &block->layout) != 0)
            return -1;
    }

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

    bool used_freetype = false;
    bool uses_fonts = _text_block_layout_uses_fonts(&block->layout);
#if defined(DVZ_HAS_FREETYPE) && DVZ_HAS_FREETYPE
    if (block->layout_glyph_count > 0)
    {
        for (uint32_t i = 0; i < block->layout_glyph_count; i++)
        {
            block->layout_color[i] =
                _text_block_color_at(block, block->layout_text_start[i], resolved.text_color);
        }
        used_freetype = _text_block_rasterize_freetype(block, &resolved);
        if (!used_freetype)
        {
            log_warn("rich text block FreeType rasterization failed");
            return -1;
        }
    }
#endif

    if (!used_freetype && !uses_fonts)
    {
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
                block, block->text[i], x, y, _text_block_style_at(block, i), color,
                resolved.scale);
        }
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
        block->image_raster_version = 0;
    }

    DvzFieldDataView view = {
        .data = block->rgba,
        .bytes_per_row = 4u * (uint64_t)block->raster_width,
        .rows_per_image = block->raster_height,
    };
    bool image_size_changed =
        block->image_width != block->raster_width || block->image_height != block->raster_height;
    if (image_size_changed)
    {
        if (!dvz_sampled_field_resize(
                block->image_field, block->raster_width, block->raster_height, 1, &view))
            return -1;
        block->image_width = block->raster_width;
        block->image_height = block->raster_height;
        block->image_raster_version = block->raster_version;
    }
    else if (block->image_raster_version != block->raster_version)
    {
        if (!dvz_sampled_field_set_data(block->image_field, &view))
            return -1;
        block->image_raster_version = block->raster_version;
    }

    const char* position_attr = resolved.pixel_space ? "position_px" : "position";
    const char* extent_attr = resolved.pixel_space ? "extent_px" : "extent";
    vec3 positions[1] = {0};
    vec2 extents[1] = {0};
    if (resolved.pixel_space)
    {
        positions[0][0] = resolved.position_px[0];
        positions[0][1] = resolved.position_px[1];
        positions[0][2] = resolved.position_px[2];
        extents[0][0] = resolved.extent_px[0];
        extents[0][1] = resolved.extent_px[1];
    }
    else
    {
        positions[0][0] = resolved.position[0];
        positions[0][1] = resolved.position[1];
        positions[0][2] = resolved.position[2];
        extents[0][0] = resolved.extent[0];
        extents[0][1] = resolved.extent[1];
    }
    vec2 anchors[1] = {{resolved.anchor[0], resolved.anchor[1]}};
    DvzVisualDataUpdate updates[3] = {
        {.attr_name = position_attr, .data = positions, .item_count = 1},
        {.attr_name = extent_attr, .data = extents, .item_count = 1},
        {.attr_name = "anchor", .data = anchors, .item_count = 1},
    };
    if (
        !block->image_desc_valid ||
        !_text_block_image_desc_equal(&block->image_desc, &resolved))
    {
        if (dvz_visual_set_data_many(block->image_visual, updates, 3) != 0)
            return -1;
        block->image_desc = resolved;
        block->image_desc_valid = true;
    }
    if (_visual_family_state(block->image_visual)->field != block->image_field)
    {
        if (!dvz_visual_set_field(block->image_visual, "field", block->image_field))
            return -1;
    }
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
    if (!block->image_visual->visible)
        dvz_visual_set_visible(block->image_visual, true);
    return 0;
}
