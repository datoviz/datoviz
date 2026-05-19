/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene text atlas                                                                             */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <float.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "_overflow.h"
#include "_scene.h"
#include "datoviz/fileio/fileio.h"
#include "datoviz/scene.h"

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wswitch-default"
#pragma GCC diagnostic ignored "-Wundef"
#endif
#define STBTT_STATIC
#define STBTT_malloc(x, u) ((void)(u), dvz_malloc((DvzSize)(x)))
#define STBTT_free(x, u)   ((void)(u), dvz_free(x))
#define STB_TRUETYPE_IMPLEMENTATION
#include "imgui/imstb_truetype.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_TEXT_SDF_FIRST_CHAR 32u
#define DVZ_TEXT_SDF_LAST_CHAR  126u
#define DVZ_TEXT_SDF_FALLBACK   63u
#define DVZ_TEXT_SDF_COLUMNS    16u
#define DVZ_TEXT_SDF_PADDING    8
#define DVZ_TEXT_SDF_CELL_GAP   2u
#define DVZ_TEXT_SDF_ONEDGE     128u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return whether a codepoint belongs to the first atlas charset.
 *
 * @param codepoint the Unicode codepoint
 * @return whether the codepoint is in the printable ASCII atlas range
 */
static bool _text_sdf_codepoint_supported(uint32_t codepoint)
{
    return codepoint >= DVZ_TEXT_SDF_FIRST_CHAR && codepoint <= DVZ_TEXT_SDF_LAST_CHAR;
}



/**
 * Clamp the requested atlas font size to a practical SDF generation range.
 *
 * @param size_pts requested font size in pixels/points
 * @return clamped atlas pixel height
 */
static float _text_sdf_pixel_height(float size_pts)
{
    float pixel_height = size_pts > 0.0f ? size_pts : 32.0f;
    if (pixel_height < 8.0f)
        pixel_height = 8.0f;
    if (pixel_height > 128.0f)
        pixel_height = 128.0f;
    return pixel_height;
}



/**
 * Try to load a font file from a small list of repository-relative defaults.
 *
 * @param out_size output byte size
 * @return owned TTF bytes, or NULL when no default font could be loaded
 */
static void* _text_sdf_load_default_font(DvzSize* out_size)
{
    ANN(out_size);
#if defined(DVZ_HAS_EMBEDDED_FONTS) && DVZ_HAS_EMBEDDED_FONTS
    unsigned long embedded_size = 0;
    const unsigned char* embedded = dvz_resource_font("Roboto_Regular", &embedded_size);
    if (embedded != NULL && embedded_size > 0)
    {
        void* bytes = dvz_malloc((DvzSize)embedded_size);
        if (bytes != NULL)
        {
            dvz_memcpy(bytes, (size_t)embedded_size, embedded, (size_t)embedded_size);
            *out_size = (DvzSize)embedded_size;
            return bytes;
        }
    }
#endif
    const char* paths[] = {
        "data/fonts/Roboto-Regular.ttf",
        "data/fonts/Roboto-Medium.ttf",
        "external/imgui/misc/fonts/Roboto-Medium.ttf",
        "external/cimgui/imgui/misc/fonts/Roboto-Medium.ttf",
    };
    for (uint32_t i = 0; i < (uint32_t)(sizeof(paths) / sizeof(paths[0])); i++)
    {
        DvzSize size = 0;
        void* bytes = dvz_read_file(paths[i], &size);
        if (bytes != NULL && size > 0)
        {
            *out_size = size;
            return bytes;
        }
        if (bytes != NULL)
            dvz_free(bytes);
    }
    return NULL;
}



/**
 * Ensure a font has owned TrueType bytes available.
 *
 * @param font the font
 * @return whether TTF bytes are available
 */
static bool _text_sdf_font_bytes(DvzFont* font)
{
    ANN(font);
    if (font->ttf_bytes != NULL && font->ttf_size > 0)
        return true;

    DvzSize size = 0;
    void* bytes = NULL;
    if (font->path[0] != '\0')
        bytes = dvz_read_file(font->path, &size);
    else
        bytes = _text_sdf_load_default_font(&size);

    if (bytes == NULL || size == 0)
    {
        if (bytes != NULL)
            dvz_free(bytes);
        log_error("failed to load text font bytes");
        return false;
    }
    font->ttf_bytes = bytes;
    font->ttf_size = (uint64_t)size;
    return true;
}



/**
 * Copy one generated SDF glyph into the packed RGBA atlas.
 *
 * @param atlas_rgba destination RGBA atlas bytes
 * @param atlas_width destination atlas width
 * @param dst_x destination glyph x origin
 * @param dst_y destination glyph y origin
 * @param glyph_sdf source SDF alpha bytes
 * @param glyph_width source glyph width
 * @param glyph_height source glyph height
 */
static void _text_sdf_copy_glyph(
    uint8_t* atlas_rgba, uint32_t atlas_width, uint32_t dst_x, uint32_t dst_y,
    const uint8_t* glyph_sdf, uint32_t glyph_width, uint32_t glyph_height)
{
    ANN(atlas_rgba);
    ANN(glyph_sdf);
    for (uint32_t y = 0; y < glyph_height; y++)
    {
        for (uint32_t x = 0; x < glyph_width; x++)
        {
            uint64_t dst = ((uint64_t)(dst_y + y) * atlas_width + dst_x + x) * 4u;
            uint64_t src = (uint64_t)y * glyph_width + x;
            atlas_rgba[dst + 0] = 0;
            atlas_rgba[dst + 1] = 0;
            atlas_rgba[dst + 2] = 0;
            atlas_rgba[dst + 3] = glyph_sdf[src];
        }
    }
}



/**
 * Initialize the stb_truetype font object.
 *
 * @param font the scene font
 * @param out_info output stb font info
 * @return whether initialization succeeded
 */
static bool _text_sdf_init_font(const DvzFont* font, stbtt_fontinfo* out_info)
{
    ANN(font);
    ANN(out_info);
    if (font->ttf_bytes == NULL || font->ttf_size == 0 || font->ttf_size > INT32_MAX)
        return false;
    const unsigned char* bytes = (const unsigned char*)font->ttf_bytes;
    int offset = stbtt_GetFontOffsetForIndex(bytes, (int)font->face_index);
    if (offset < 0)
    {
        log_error("failed to find requested TrueType face index %u", font->face_index);
        return false;
    }
    if (stbtt_InitFont(out_info, bytes, offset) == 0)
    {
        log_error("failed to initialize TrueType font");
        return false;
    }
    return true;
}



/**
 * Convert an SDF value to an atlas threshold scaling constant.
 *
 * @return pixel distance scale passed to stb_truetype SDF generation
 */
static float _text_sdf_pixel_dist_scale(void)
{
    return (float)DVZ_TEXT_SDF_ONEDGE / (float)DVZ_TEXT_SDF_PADDING;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

extern "C" {

/**
 * Ensure one font has a scene-owned SDF atlas.
 *
 * @param font the scene font
 * @return whether the atlas is available
 */
bool _scene_text_atlas_ensure(DvzFont* font)
{
    ANN(font);
    if (font->sdf_atlas != NULL && font->sdf_atlas->field != NULL)
        return true;
    if (font->scene == NULL)
        return false;
    if (!_text_sdf_font_bytes(font))
        return false;

    stbtt_fontinfo info = {};
    if (!_text_sdf_init_font(font, &info))
        return false;

    const float pixel_height = _text_sdf_pixel_height(font->size_pts);
    const float scale = stbtt_ScaleForPixelHeight(&info, pixel_height);
    const uint32_t glyph_count = DVZ_TEXT_SDF_LAST_CHAR - DVZ_TEXT_SDF_FIRST_CHAR + 1u;

    uint8_t* glyph_sdfs[DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS] = {};
    uint32_t glyph_widths[DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS] = {};
    uint32_t glyph_heights[DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS] = {};
    int glyph_xoffs[DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS] = {};
    int glyph_yoffs[DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS] = {};
    uint32_t cell_width = 1;
    uint32_t cell_height = 1;

    for (uint32_t i = 0; i < glyph_count; i++)
    {
        int width = 0;
        int height = 0;
        int xoff = 0;
        int yoff = 0;
        uint32_t codepoint = DVZ_TEXT_SDF_FIRST_CHAR + i;
        unsigned char* sdf = stbtt_GetCodepointSDF(
            &info, scale, (int)codepoint, DVZ_TEXT_SDF_PADDING,
            (unsigned char)DVZ_TEXT_SDF_ONEDGE, _text_sdf_pixel_dist_scale(), &width, &height,
            &xoff, &yoff);
        if (sdf == NULL || width <= 0 || height <= 0)
            continue;
        glyph_sdfs[i] = sdf;
        glyph_widths[i] = (uint32_t)width;
        glyph_heights[i] = (uint32_t)height;
        glyph_xoffs[i] = xoff;
        glyph_yoffs[i] = yoff;
        if ((uint32_t)width > cell_width)
            cell_width = (uint32_t)width;
        if ((uint32_t)height > cell_height)
            cell_height = (uint32_t)height;
    }

    cell_width += DVZ_TEXT_SDF_CELL_GAP;
    cell_height += DVZ_TEXT_SDF_CELL_GAP;
    uint32_t rows = (glyph_count + DVZ_TEXT_SDF_COLUMNS - 1u) / DVZ_TEXT_SDF_COLUMNS;
    uint64_t width64 = 0;
    uint64_t height64 = 0;
    uint64_t pixel_count = 0;
    uint64_t byte_size = 0;
    if (_dvz_mul_u64_overflows(DVZ_TEXT_SDF_COLUMNS, cell_width, &width64) ||
        _dvz_mul_u64_overflows(rows, cell_height, &height64) ||
        _dvz_mul_u64_overflows(width64, height64, &pixel_count) ||
        _dvz_mul_u64_overflows(pixel_count, 4u, &byte_size) || width64 > UINT32_MAX ||
        height64 > UINT32_MAX || byte_size > SIZE_MAX)
    {
        log_error("text SDF atlas dimensions overflow");
        for (uint32_t i = 0; i < glyph_count; i++)
            if (glyph_sdfs[i] != NULL)
                stbtt_FreeSDF(glyph_sdfs[i], NULL);
        return false;
    }

    uint32_t atlas_width = (uint32_t)width64;
    uint32_t atlas_height = (uint32_t)height64;
    uint8_t* rgba = (uint8_t*)dvz_calloc((DvzSize)byte_size, 1);
    DvzTextAtlas* atlas = (DvzTextAtlas*)dvz_calloc(1, sizeof(DvzTextAtlas));
    if (rgba == NULL || atlas == NULL)
    {
        log_error("text SDF atlas allocation failed");
        dvz_free(rgba);
        dvz_free(atlas);
        for (uint32_t i = 0; i < glyph_count; i++)
            if (glyph_sdfs[i] != NULL)
                stbtt_FreeSDF(glyph_sdfs[i], NULL);
        return false;
    }

    int ascent = 0;
    int descent = 0;
    int line_gap = 0;
    stbtt_GetFontVMetrics(&info, &ascent, &descent, &line_gap);
    atlas->width = atlas_width;
    atlas->height = atlas_height;
    atlas->glyph_count = glyph_count;
    atlas->pixel_height = pixel_height;
    atlas->ascent = (float)ascent * scale;
    atlas->descent = (float)descent * scale;
    atlas->line_gap = (float)line_gap * scale;
    atlas->line_height = (float)(ascent - descent + line_gap) * scale;
    if (atlas->line_height <= 0.0f)
        atlas->line_height = pixel_height;

    for (uint32_t i = 0; i < glyph_count; i++)
    {
        uint32_t codepoint = DVZ_TEXT_SDF_FIRST_CHAR + i;
        DvzTextAtlasGlyph* glyph = &atlas->glyphs[i];
        glyph->codepoint = codepoint;
        int advance = 0;
        int left_bearing = 0;
        stbtt_GetCodepointHMetrics(&info, (int)codepoint, &advance, &left_bearing);
        (void)left_bearing;
        glyph->advance = (float)advance * scale;
        if (glyph_sdfs[i] == NULL)
            continue;

        uint32_t col = i % DVZ_TEXT_SDF_COLUMNS;
        uint32_t row = i / DVZ_TEXT_SDF_COLUMNS;
        uint32_t x = col * cell_width;
        uint32_t y = row * cell_height;
        _text_sdf_copy_glyph(
            rgba, atlas_width, x, y, glyph_sdfs[i], glyph_widths[i], glyph_heights[i]);

        glyph->xoff = (float)glyph_xoffs[i];
        glyph->yoff = (float)glyph_yoffs[i];
        glyph->width = (float)glyph_widths[i];
        glyph->height = (float)glyph_heights[i];
        glyph->uv[0] = (float)x / (float)atlas_width;
        glyph->uv[1] = (float)y / (float)atlas_height;
        glyph->uv[2] = (float)(x + glyph_widths[i]) / (float)atlas_width;
        glyph->uv[3] = (float)(y + glyph_heights[i]) / (float)atlas_height;
        glyph->valid = true;
    }

    for (uint32_t i = 0; i < glyph_count; i++)
        if (glyph_sdfs[i] != NULL)
            stbtt_FreeSDF(glyph_sdfs[i], NULL);

    DvzSampledFieldDesc desc = {};
    desc.dim = DVZ_FIELD_DIM_2D;
    desc.format = DVZ_FIELD_FORMAT_RGBA8_UNORM;
    desc.semantic = DVZ_FIELD_SEMANTIC_COLOR;
    desc.width = atlas_width;
    desc.height = atlas_height;
    desc.depth = 1;
    DvzSampledField* field = dvz_sampled_field(font->scene, &desc);
    DvzFieldDataView view = {};
    view.data = rgba;
    view.bytes_per_row = (uint64_t)atlas_width * 4u;
    view.rows_per_image = atlas_height;
    bool ok = field != NULL && dvz_sampled_field_set_data(field, &view);
    dvz_free(rgba);
    if (!ok)
    {
        _scene_text_atlas_destroy(atlas);
        return false;
    }

    atlas->field = field;
    font->sdf_atlas = atlas;
    font->version++;
    return true;
}



/**
 * Return one atlas glyph, falling back to '?' for unsupported codepoints.
 *
 * @param atlas the text atlas
 * @param codepoint the Unicode codepoint
 * @return the glyph metadata, or NULL when unavailable
 */
DvzTextAtlasGlyph* _scene_text_atlas_glyph(DvzTextAtlas* atlas, uint32_t codepoint)
{
    ANN(atlas);
    if (!_text_sdf_codepoint_supported(codepoint))
        codepoint = DVZ_TEXT_SDF_FALLBACK;
    uint32_t index = codepoint - DVZ_TEXT_SDF_FIRST_CHAR;
    if (index >= atlas->glyph_count || index >= DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS)
        return NULL;
    DvzTextAtlasGlyph* glyph = &atlas->glyphs[index];
    return glyph->valid ? glyph : NULL;
}



/**
 * Destroy an SDF atlas object.
 *
 * @param atlas the atlas
 */
void _scene_text_atlas_destroy(DvzTextAtlas* atlas)
{
    if (atlas == NULL)
        return;
    if (atlas->field != NULL)
    {
        dvz_sampled_field_destroy(atlas->field);
        atlas->field = NULL;
    }
    dvz_free(atlas);
}



/**
 * Release private font resources.
 *
 * @param font the font
 */
void _scene_font_release(DvzFont* font)
{
    if (font == NULL)
        return;
    if (font->sdf_atlas != NULL)
    {
        _scene_text_atlas_destroy(font->sdf_atlas);
        font->sdf_atlas = NULL;
    }
    if (font->ttf_bytes != NULL)
    {
        dvz_free(font->ttf_bytes);
        font->ttf_bytes = NULL;
        font->ttf_size = 0;
    }
    font->scene = NULL;
}

}
