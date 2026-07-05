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
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <vector>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_overflow.h"
#include "_scene.h"
#include "_time_utils.h"
#include "datoviz/fileio/fileio.h"
#include "datoviz/scene.h"
#include "text/text_internal.h"

#if defined(DVZ_HAS_ZLIB) && DVZ_HAS_ZLIB
#include <zlib.h>
#endif

#if defined(DVZ_HAS_FREETYPE) && DVZ_HAS_FREETYPE
#include <ft2build.h>
#include FT_FREETYPE_H
#endif

#if defined(DVZ_HAS_MSDF_ATLAS) && DVZ_HAS_MSDF_ATLAS
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wint-in-bool-context"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wswitch-default"
#endif
#include <msdf-atlas-gen/msdf-atlas-gen.h>
#include <msdf-atlas-gen/types.h>
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
#endif

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
#define DVZ_TEXT_BITMAP_PADDING 1u
#define DVZ_TEXT_ATLAS_MIN_EM_PX 8.0f
#define DVZ_TEXT_ATLAS_DEFAULT_EM_PX 32.0f
#define DVZ_TEXT_ATLAS_MAX_EM_PX 128.0f
#define DVZ_TEXT_MSDF_REFERENCE_EM_PX 32.0f
#define DVZ_TEXT_MSDF_REFERENCE_RANGE_PX 4.0f
#define DVZ_TEXT_MSDF_MAX_RANGE_PX 16.0f
#define DVZ_TEXT_SDF_REFERENCE_RANGE_PX ((float)DVZ_TEXT_SDF_PADDING)
#define DVZ_TEXT_SDF_MAX_RANGE_PX 32.0f



/*************************************************************************************************/
/*  Function prototypes                                                                          */
/*************************************************************************************************/

typedef struct DvzTextAtlasBuildSet DvzTextAtlasBuildSet;
struct DvzTextAtlasBuildSet
{
    uint32_t codepoints[DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS];
    uint32_t count;
};

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverlength-strings"
#endif
#include "text_default_msdf_atlas_generated.inc"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

typedef struct DvzTextDefaultMsdfAtlasData DvzTextDefaultMsdfAtlasData;
struct DvzTextDefaultMsdfAtlasData
{
    float spec_em_px;
    float spec_range_px;
    uint32_t width;
    uint32_t height;
    uint32_t glyph_count;
    float em_px;
    float range_px;
    float ascent;
    float descent;
    float line_gap;
    float line_height;
    uint32_t rgba_size;
    uint32_t z_size;
    const uint8_t* z;
    const DvzTextAtlasGlyph* glyphs;
};

static const DvzTextDefaultMsdfAtlasData DVZ_TEXT_DEFAULT_MSDF_ATLASES[] = {
    {32.0f,                                4.0f,
     DVZ_TEXT_DEFAULT_MSDF_32_WIDTH,        DVZ_TEXT_DEFAULT_MSDF_32_HEIGHT,
     DVZ_TEXT_DEFAULT_MSDF_32_GLYPH_COUNT,  DVZ_TEXT_DEFAULT_MSDF_32_EM_PX,
     DVZ_TEXT_DEFAULT_MSDF_32_RANGE_PX,     DVZ_TEXT_DEFAULT_MSDF_32_ASCENT,
     DVZ_TEXT_DEFAULT_MSDF_32_DESCENT,      DVZ_TEXT_DEFAULT_MSDF_32_LINE_GAP,
     DVZ_TEXT_DEFAULT_MSDF_32_LINE_HEIGHT,  DVZ_TEXT_DEFAULT_MSDF_32_RGBA_SIZE,
     DVZ_TEXT_DEFAULT_MSDF_32_RGBA_Z_SIZE,  DVZ_TEXT_DEFAULT_MSDF_32_RGBA_Z,
     DVZ_TEXT_DEFAULT_MSDF_32_GLYPHS},
    {64.0f,                                8.0f,
     DVZ_TEXT_DEFAULT_MSDF_64_WIDTH,        DVZ_TEXT_DEFAULT_MSDF_64_HEIGHT,
     DVZ_TEXT_DEFAULT_MSDF_64_GLYPH_COUNT,  DVZ_TEXT_DEFAULT_MSDF_64_EM_PX,
     DVZ_TEXT_DEFAULT_MSDF_64_RANGE_PX,     DVZ_TEXT_DEFAULT_MSDF_64_ASCENT,
     DVZ_TEXT_DEFAULT_MSDF_64_DESCENT,      DVZ_TEXT_DEFAULT_MSDF_64_LINE_GAP,
     DVZ_TEXT_DEFAULT_MSDF_64_LINE_HEIGHT,  DVZ_TEXT_DEFAULT_MSDF_64_RGBA_SIZE,
     DVZ_TEXT_DEFAULT_MSDF_64_RGBA_Z_SIZE,  DVZ_TEXT_DEFAULT_MSDF_64_RGBA_Z,
     DVZ_TEXT_DEFAULT_MSDF_64_GLYPHS},
    {128.0f,                               16.0f,
     DVZ_TEXT_DEFAULT_MSDF_128_WIDTH,       DVZ_TEXT_DEFAULT_MSDF_128_HEIGHT,
     DVZ_TEXT_DEFAULT_MSDF_128_GLYPH_COUNT, DVZ_TEXT_DEFAULT_MSDF_128_EM_PX,
     DVZ_TEXT_DEFAULT_MSDF_128_RANGE_PX,    DVZ_TEXT_DEFAULT_MSDF_128_ASCENT,
     DVZ_TEXT_DEFAULT_MSDF_128_DESCENT,     DVZ_TEXT_DEFAULT_MSDF_128_LINE_GAP,
     DVZ_TEXT_DEFAULT_MSDF_128_LINE_HEIGHT, DVZ_TEXT_DEFAULT_MSDF_128_RGBA_SIZE,
     DVZ_TEXT_DEFAULT_MSDF_128_RGBA_Z_SIZE, DVZ_TEXT_DEFAULT_MSDF_128_RGBA_Z,
     DVZ_TEXT_DEFAULT_MSDF_128_GLYPHS},
};
static const uint32_t DVZ_TEXT_DEFAULT_MSDF_ATLAS_COUNT =
    sizeof(DVZ_TEXT_DEFAULT_MSDF_ATLASES) / sizeof(DVZ_TEXT_DEFAULT_MSDF_ATLASES[0]);


static bool _text_atlas_codepoint_renderable(uint32_t codepoint);
static float _text_atlas_clamp(float value, float lo, float hi);
static float _text_atlas_bitmap_em_px(float size_px);
static float _text_atlas_sdf_em_px(float size_px);
static float _text_atlas_msdf_range_px(float em_px);
static float _text_atlas_sdf_range_px(float em_px);
static bool _text_sdf_font_bytes(DvzFont* font);
static bool _text_atlas_upload_rgba(
    const DvzFont* font, DvzTextAtlas* atlas, uint8_t* rgba, uint32_t width, uint32_t height);
static bool _text_atlas_changed_region(
    const DvzSampledField* field, const DvzSampledField* src, DvzFieldRegion* out_region);
static bool _text_atlas_replace_field_data(DvzSampledField* field, const DvzSampledField* src);
static bool _text_atlas_try_append(
    DvzFont* font, DvzTextAtlas* atlas, const DvzTextAtlasBuildSet* requested_set);
static bool _text_default_msdf_build_atlas(
    DvzFont* font, const DvzTextAtlasSpec* spec, const DvzTextAtlasBuildSet* set,
    DvzTextAtlas** out_atlas);


static double _text_atlas_elapsed_ms(uint64_t start_ns)
{
    uint64_t end_ns = dvz_time_monotonic_ns();
    if (start_ns == 0 || end_ns <= start_ns)
        return 0.0;
    return (double)(end_ns - start_ns) / 1000000.0;
}


#if defined(DVZ_HAS_MSDF_ATLAS) && DVZ_HAS_MSDF_ATLAS
/**
 * Build an RGB MSDF atlas with msdf-atlas-gen.
 *
 * @param font the font
 * @param set the requested codepoint set
 * @param out_atlas output atlas metadata
 * @return whether atlas creation succeeded
 */
static bool _text_msdf_build_atlas(
    DvzFont* font, const DvzTextAtlasSpec* spec, const DvzTextAtlasBuildSet* set,
    DvzTextAtlas** out_atlas)
{
    ANN(font);
    ANN(spec);
    ANN(set);
    ANN(out_atlas);
    *out_atlas = NULL;
    if (set->count == 0 || set->count > DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS)
        return false;
    if (!_text_sdf_font_bytes(font))
        return false;

    uint64_t start_ns = dvz_time_monotonic_ns();
    log_debug(
        "text atlas: building runtime MSDF atlas em=%.3f range=%.3f glyphs=%u",
        (double)spec->em_px, (double)spec->distance_range_px, set->count);

    msdfgen::FreetypeHandle* ft = msdfgen::initializeFreetype();
    if (ft == NULL)
    {
        log_error("failed to initialize msdfgen FreeType handle");
        return false;
    }
#if OS_WINDOWS
    msdfgen::FontHandle* msdf_font =
        msdfgen::loadFontData(ft, (const unsigned char*)font->ttf_bytes, (int)font->ttf_size);
#else
    msdfgen::FontHandle* msdf_font =
        msdfgen::loadFontData(ft, (const unsigned char*)font->ttf_bytes, font->ttf_size);
#endif
    if (msdf_font == NULL)
    {
        log_error("failed to load msdfgen font data");
        msdfgen::deinitializeFreetype(ft);
        return false;
    }

    std::vector<msdf_atlas::GlyphGeometry> glyphs;
    msdf_atlas::FontGeometry font_geometry(&glyphs);
    msdf_atlas::Charset charset;
    const uint32_t glyph_count = set->count;
    for (uint32_t i = 0; i < glyph_count; i++)
        charset.add(set->codepoints[i]);

    if (font_geometry.loadCharset(msdf_font, 1.0, charset) <= 0 || glyphs.empty())
    {
        log_error("failed to load MSDF charset");
        msdfgen::destroyFont(msdf_font);
        msdfgen::deinitializeFreetype(ft);
        return false;
    }

    const double max_corner_angle = 3.0;
    for (msdf_atlas::GlyphGeometry& glyph : glyphs)
        glyph.edgeColoring(&msdfgen::edgeColoringInkTrap, max_corner_angle, 0);

    float em_px = spec->em_px > 0.0f ? spec->em_px : DVZ_TEXT_ATLAS_DEFAULT_EM_PX;
    float distance_range_px =
        spec->distance_range_px > 0.0f ? spec->distance_range_px :
                                         DVZ_TEXT_MSDF_REFERENCE_RANGE_PX;
    msdf_atlas::TightAtlasPacker packer;
    packer.setDimensionsConstraint(msdf_atlas::DimensionsConstraint::SQUARE);
    packer.setMinimumScale((double)em_px);
    packer.setPixelRange((double)distance_range_px);
    packer.setMiterLimit(1.0);
    if (packer.pack(glyphs.data(), (int)glyphs.size()) != 0)
    {
        log_error("failed to pack MSDF atlas");
        msdfgen::destroyFont(msdf_font);
        msdfgen::deinitializeFreetype(ft);
        return false;
    }

    int width = 0;
    int height = 0;
    packer.getDimensions(width, height);
    if (width <= 0 || height <= 0)
    {
        log_error("MSDF atlas has invalid dimensions");
        msdfgen::destroyFont(msdf_font);
        msdfgen::deinitializeFreetype(ft);
        return false;
    }

    msdf_atlas::ImmediateAtlasGenerator<
        float, 4, &msdf_atlas::mtsdfGenerator, msdf_atlas::BitmapAtlasStorage<uint8_t, 4>>
        generator(width, height);
    msdf_atlas::GeneratorAttributes attributes;
    attributes.config.overlapSupport = true;
    attributes.scanlinePass = true;
    generator.setAttributes(attributes);
    generator.setThreadCount(8);
    generator.generate(glyphs.data(), glyphs.size());
    msdfgen::BitmapConstRef<uint8_t, 4> bitmap = generator.atlasStorage();

    uint64_t pixel_count = 0;
    uint64_t byte_size = 0;
    if (_dvz_mul_u64_overflows((uint64_t)width, (uint64_t)height, &pixel_count) ||
        _dvz_mul_u64_overflows(pixel_count, 4u, &byte_size) || byte_size > SIZE_MAX)
    {
        log_error("MSDF atlas byte size overflow");
        msdfgen::destroyFont(msdf_font);
        msdfgen::deinitializeFreetype(ft);
        return false;
    }

    uint8_t* rgba = (uint8_t*)dvz_calloc((DvzSize)byte_size, 1);
    DvzTextAtlas* atlas = (DvzTextAtlas*)dvz_calloc(1, sizeof(DvzTextAtlas));
    if (rgba == NULL || atlas == NULL)
    {
        log_error("MSDF atlas allocation failed");
        dvz_free(rgba);
        dvz_free(atlas);
        msdfgen::destroyFont(msdf_font);
        msdfgen::deinitializeFreetype(ft);
        return false;
    }

    uint32_t atlas_width = (uint32_t)width;
    uint32_t atlas_height = (uint32_t)height;
    for (uint32_t y = 0; y < atlas_height; y++)
    {
        for (uint32_t x = 0; x < atlas_width; x++)
        {
            uint64_t src = ((uint64_t)y * atlas_width + x) * 4u;
            uint64_t dst = ((uint64_t)(atlas_height - 1u - y) * atlas_width + x) * 4u;
            rgba[dst + 0] = bitmap.pixels[src + 0];
            rgba[dst + 1] = bitmap.pixels[src + 1];
            rgba[dst + 2] = bitmap.pixels[src + 2];
            rgba[dst + 3] = bitmap.pixels[src + 3];
        }
    }

    const double scale = packer.getScale();
    const msdfgen::FontMetrics& metrics = font_geometry.getMetrics();
    atlas->spec = *spec;
    atlas->backend = DVZ_TEXT_ATLAS_BACKEND_MSDF;
    atlas->encoding = DVZ_TEXT_ATLAS_ENCODING_MSDF_RGB;
    atlas->width = atlas_width;
    atlas->height = atlas_height;
    atlas->glyph_count = glyph_count;
    atlas->channels = 4;
    atlas->em_px = (float)scale;
    atlas->distance_range_px = distance_range_px;
    atlas->ascent = (float)(metrics.ascenderY * scale);
    atlas->descent = (float)(metrics.descenderY * scale);
    atlas->line_gap = (float)((metrics.lineHeight - metrics.ascenderY + metrics.descenderY) * scale);
    atlas->line_height = (float)(metrics.lineHeight * scale);
    if (atlas->line_height <= 0.0f)
        atlas->line_height = (float)scale;
    for (uint32_t i = 0; i < glyph_count; i++)
        atlas->glyphs[i].codepoint = set->codepoints[i];

    for (const msdf_atlas::GlyphGeometry& src_glyph : glyphs)
    {
        uint32_t codepoint = (uint32_t)src_glyph.getCodepoint();
        uint32_t index = UINT32_MAX;
        for (uint32_t i = 0; i < glyph_count; i++)
        {
            if (set->codepoints[i] == codepoint)
            {
                index = i;
                break;
            }
        }
        if (index >= DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS)
            continue;
        DvzTextAtlasGlyph* glyph = &atlas->glyphs[index];
        glyph->codepoint = codepoint;
        glyph->glyph_id = (uint32_t)src_glyph.getIndex();
        if (glyph->glyph_id == 0 && codepoint != DVZ_TEXT_SDF_FALLBACK)
        {
            atlas->missing_glyph_count++;
            continue;
        }
        glyph->advance = (float)(src_glyph.getAdvance() * scale);

        double l = 0.0;
        double b = 0.0;
        double r = 0.0;
        double t = 0.0;
        src_glyph.getQuadPlaneBounds(l, b, r, t);

        int x = 0;
        int y = 0;
        int w = 0;
        int h = 0;
        src_glyph.getBoxRect(x, y, w, h);
        if (w <= 0 || h <= 0)
        {
            glyph->valid = glyph->advance > 0.0f;
            continue;
        }

        const double inset_x = 0.5;
        const double inset_y = 0.5;
        double plane_l = l;
        double plane_b = b;
        double plane_r = r;
        double plane_t = t;
        if (w > 2 && h > 2)
        {
            double fx = inset_x / (double)w;
            double fy = inset_y / (double)h;
            double plane_w = r - l;
            double plane_h = t - b;
            plane_l += fx * plane_w;
            plane_r -= fx * plane_w;
            plane_b += fy * plane_h;
            plane_t -= fy * plane_h;
        }

        glyph->xoff = (float)(plane_l * scale);
        glyph->yoff = (float)(-plane_t * scale);
        glyph->width = (float)((plane_r - plane_l) * scale);
        glyph->height = (float)((plane_t - plane_b) * scale);
        glyph->plane_bounds[0] = glyph->xoff;
        glyph->plane_bounds[1] = glyph->yoff;
        glyph->plane_bounds[2] = glyph->xoff + glyph->width;
        glyph->plane_bounds[3] = glyph->yoff + glyph->height;

        uint32_t top_y = atlas_height - (uint32_t)y - (uint32_t)h;
        glyph->atlas_bounds[0] = (float)x;
        glyph->atlas_bounds[1] = (float)top_y;
        glyph->atlas_bounds[2] = (float)(x + w);
        glyph->atlas_bounds[3] = (float)(top_y + (uint32_t)h);
        float pad_x = w > 2 ? (float)inset_x : 0.0f;
        float pad_y = h > 2 ? (float)inset_y : 0.0f;
        glyph->uv[0] = ((float)x + pad_x) / (float)atlas_width;
        glyph->uv[1] = ((float)top_y + pad_y) / (float)atlas_height;
        glyph->uv[2] = ((float)(x + w) - pad_x) / (float)atlas_width;
        glyph->uv[3] = ((float)(top_y + (uint32_t)h) - pad_y) / (float)atlas_height;
        glyph->valid = true;
    }
    for (uint32_t i = 0; i < glyph_count; i++)
    {
        if (!atlas->glyphs[i].valid && atlas->glyphs[i].codepoint != DVZ_TEXT_SDF_FALLBACK)
            atlas->missing_glyph_count++;
    }

    bool ok = _text_atlas_upload_rgba(font, atlas, rgba, atlas_width, atlas_height);
    dvz_free(rgba);
    msdfgen::destroyFont(msdf_font);
    msdfgen::deinitializeFreetype(ft);
    if (!ok)
    {
        _scene_text_atlas_destroy(atlas);
        return false;
    }

    *out_atlas = atlas;
    log_debug(
        "text atlas: runtime MSDF atlas ready em=%.3f range=%.3f glyphs=%u size=%ux%u "
        "rgba=%llu in %.3f ms",
        (double)atlas->em_px, (double)atlas->distance_range_px, atlas->glyph_count,
        atlas->width, atlas->height, (unsigned long long)byte_size,
        _text_atlas_elapsed_ms(start_ns));
    return true;
}
#endif



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return whether a codepoint may be requested from a font-backed atlas.
 *
 * @param codepoint the Unicode codepoint
 * @return whether the codepoint can be requested
 */
static bool _text_atlas_codepoint_renderable(uint32_t codepoint)
{
    if (codepoint < DVZ_TEXT_SDF_FIRST_CHAR || codepoint > 0x10FFFFu)
        return false;
    if (codepoint >= 0xD800u && codepoint <= 0xDFFFu)
        return false;
    return true;
}



/**
 * Return whether a build set already contains a codepoint.
 *
 * @param set the build set
 * @param codepoint the Unicode codepoint
 * @return whether the codepoint is already present
 */
static bool _text_atlas_build_set_contains(
    const DvzTextAtlasBuildSet* set, uint32_t codepoint)
{
    ANN(set);
    for (uint32_t i = 0; i < set->count; i++)
        if (set->codepoints[i] == codepoint)
            return true;
    return false;
}



/**
 * Add one codepoint to a build set.
 *
 * @param set the build set
 * @param codepoint the Unicode codepoint
 * @return whether the codepoint was accepted
 */
static bool _text_atlas_build_set_add(DvzTextAtlasBuildSet* set, uint32_t codepoint)
{
    ANN(set);
    if (codepoint == '\n' || codepoint == '\t')
        return true;
    if (!_text_atlas_codepoint_renderable(codepoint))
        codepoint = DVZ_TEXT_SDF_FALLBACK;
    if (_text_atlas_build_set_contains(set, codepoint))
        return true;
    if (set->count >= DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS)
    {
        log_error("text atlas glyph capacity reached");
        return false;
    }
    set->codepoints[set->count++] = codepoint;
    return true;
}



/**
 * Fill a build set with the default printable ASCII range.
 *
 * @param set the build set
 * @return whether the default set was added
 */
static bool _text_atlas_build_set_add_default(DvzTextAtlasBuildSet* set)
{
    ANN(set);
    for (uint32_t codepoint = DVZ_TEXT_SDF_FIRST_CHAR; codepoint <= DVZ_TEXT_SDF_LAST_CHAR;
         codepoint++)
    {
        if (!_text_atlas_build_set_add(set, codepoint))
            return false;
    }
    return _text_atlas_build_set_add(set, DVZ_TEXT_SDF_FALLBACK);
}



/**
 * Preserve all codepoints already present in an atlas.
 *
 * @param set the build set
 * @param atlas the existing atlas
 * @return whether all codepoints were added
 */
static bool _text_atlas_build_set_add_existing(
    DvzTextAtlasBuildSet* set, const DvzTextAtlas* atlas)
{
    ANN(set);
    if (atlas == NULL)
        return true;
    for (uint32_t i = 0; i < atlas->glyph_count; i++)
    {
        if (!_text_atlas_build_set_add(set, atlas->glyphs[i].codepoint))
            return false;
    }
    return true;
}



/**
 * Decode one UTF-8 codepoint, replacing malformed input with the fallback codepoint.
 *
 * @param string the UTF-8 string
 * @param inout_index byte index, advanced by the consumed sequence
 * @param out_codepoint output Unicode codepoint
 * @return whether a codepoint was decoded
 */
static bool _text_atlas_utf8_next(
    const char* string, uint32_t* inout_index, uint32_t* out_codepoint)
{
    ANN(string);
    ANN(inout_index);
    ANN(out_codepoint);
    uint32_t i = *inout_index;
    if (string[i] == '\0')
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
        *out_codepoint = DVZ_TEXT_SDF_FALLBACK;
        *inout_index = i + 1;
        return true;
    }

    for (uint32_t j = 1; j < needed; j++)
    {
        if (string[i + j] == '\0' || (s[i + j] & 0xC0u) != 0x80u)
        {
            *out_codepoint = DVZ_TEXT_SDF_FALLBACK;
            *inout_index = i + 1;
            return true;
        }
        cp = (cp << 6) | (uint32_t)(s[i + j] & 0x3Fu);
    }

    if (cp < min_cp || !_text_atlas_codepoint_renderable(cp))
        cp = DVZ_TEXT_SDF_FALLBACK;
    *out_codepoint = cp;
    *inout_index = i + needed;
    return true;
}



/**
 * Add all codepoints from one string to a build set.
 *
 * @param set the build set
 * @param string the UTF-8 string
 * @return whether all requested codepoints were added
 */
static bool _text_atlas_build_set_add_string(DvzTextAtlasBuildSet* set, const char* string)
{
    ANN(set);
    if (string == NULL)
        return true;
    uint32_t index = 0;
    uint32_t codepoint = 0;
    while (_text_atlas_utf8_next(string, &index, &codepoint))
        if (!_text_atlas_build_set_add(set, codepoint))
            return false;
    return true;
}



/**
 * Add all codepoints from a string array to a build set.
 *
 * @param set the build set
 * @param strings the UTF-8 strings
 * @param count string count
 * @return whether all requested codepoints were added
 */
static bool _text_atlas_build_set_add_strings(
    DvzTextAtlasBuildSet* set, const char* const* strings, uint32_t count)
{
    ANN(set);
    if (strings == NULL)
        return true;
    for (uint32_t i = 0; i < count; i++)
        if (!_text_atlas_build_set_add_string(set, strings[i]))
            return false;
    return true;
}



/**
 * Return an exact atlas glyph entry by codepoint.
 *
 * @param atlas the atlas
 * @param codepoint the Unicode codepoint
 * @return glyph entry, or NULL if absent
 */
static const DvzTextAtlasGlyph* _text_atlas_find_glyph(
    const DvzTextAtlas* atlas, uint32_t codepoint)
{
    if (atlas == NULL)
        return NULL;
    for (uint32_t i = 0; i < atlas->glyph_count && i < DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS; i++)
    {
        const DvzTextAtlasGlyph* glyph = &atlas->glyphs[i];
        if (glyph->codepoint == codepoint)
            return glyph;
    }
    return NULL;
}



/**
 * Return whether an atlas contains a codepoint entry.
 *
 * @param atlas the atlas
 * @param codepoint the Unicode codepoint
 * @return whether the codepoint is present in the atlas metadata
 */
static bool _text_atlas_contains_codepoint(const DvzTextAtlas* atlas, uint32_t codepoint)
{
    if (atlas == NULL)
        return false;
    if (!_text_atlas_codepoint_renderable(codepoint))
        codepoint = DVZ_TEXT_SDF_FALLBACK;
    return _text_atlas_find_glyph(atlas, codepoint) != NULL;
}



/**
 * Return whether an atlas covers a build set.
 *
 * @param atlas the atlas
 * @param set the requested codepoint set
 * @return whether every requested codepoint is present
 */
static bool _text_atlas_contains_set(
    const DvzTextAtlas* atlas, const DvzTextAtlasBuildSet* set)
{
    ANN(set);
    if (atlas == NULL || atlas->field == NULL)
        return false;
    for (uint32_t i = 0; i < set->count; i++)
    {
        if (!_text_atlas_contains_codepoint(atlas, set->codepoints[i]))
            return false;
    }
    return true;
}



/**
 * Clamp a floating-point value.
 *
 * @param value input value
 * @param lo lower bound
 * @param hi upper bound
 * @return clamped value
 */
static float _text_atlas_clamp(float value, float lo, float hi)
{
    if (value < lo)
        return lo;
    if (value > hi)
        return hi;
    return value;
}



/**
 * Return a bitmap atlas em size for a rendered text size.
 *
 * @param size_px rendered text size in pixels
 * @return atlas em size in pixels
 */
static float _text_atlas_bitmap_em_px(float size_px)
{
    if (size_px <= 0.0f)
        size_px = DVZ_TEXT_ATLAS_DEFAULT_EM_PX;
    return _text_atlas_clamp(floorf(size_px + 0.5f), DVZ_TEXT_ATLAS_MIN_EM_PX,
                             DVZ_TEXT_ATLAS_MAX_EM_PX);
}



/**
 * Return the quantized SDF/MSDF atlas em size for a rendered text size.
 *
 * @param size_px rendered text size in pixels
 * @return atlas em size in pixels
 */
static float _text_atlas_sdf_em_px(float size_px)
{
    if (size_px <= 0.0f)
        return DVZ_TEXT_ATLAS_DEFAULT_EM_PX;
    if (size_px <= 48.0f)
        return 32.0f;
    if (size_px <= 96.0f)
        return 64.0f;
    return 128.0f;
}



/**
 * Return the baked MSDF distance range for an atlas em size.
 *
 * @param em_px atlas em size in pixels
 * @return MSDF distance range in atlas pixels
 */
static float _text_atlas_msdf_range_px(float em_px)
{
    float range =
        DVZ_TEXT_MSDF_REFERENCE_RANGE_PX * em_px / DVZ_TEXT_MSDF_REFERENCE_EM_PX;
    return _text_atlas_clamp(
        range, DVZ_TEXT_MSDF_REFERENCE_RANGE_PX, DVZ_TEXT_MSDF_MAX_RANGE_PX);
}



/**
 * Return the baked STB SDF distance range for an atlas em size.
 *
 * @param em_px atlas em size in pixels
 * @return SDF distance range in atlas pixels
 */
static float _text_atlas_sdf_range_px(float em_px)
{
    float range =
        DVZ_TEXT_SDF_REFERENCE_RANGE_PX * em_px / DVZ_TEXT_ATLAS_DEFAULT_EM_PX;
    return _text_atlas_clamp(
        range, DVZ_TEXT_SDF_REFERENCE_RANGE_PX, DVZ_TEXT_SDF_MAX_RANGE_PX);
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
    DvzSize embedded_size = 0;
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
        "data/assets/fonts/Roboto-Regular.ttf",
        "data/assets/fonts/Roboto-Medium.ttf",
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
 * Return whether one font is the built-in Roboto Regular default.
 *
 * @param font the scene font
 * @return whether embedded default atlas data may be used
 */
static bool _text_default_msdf_font_matches(const DvzFont* font)
{
    ANN(font);
    if (font->path[0] != '\0' || font->face_index != 0)
        return false;
    bool family = font->family[0] == '\0' || strcmp(font->family, "Roboto") == 0;
    bool style = font->style[0] == '\0' || strcmp(font->style, "Regular") == 0;
    return family && style;
}



/**
 * Resolve an embedded default MSDF atlas index for one spec.
 *
 * @param spec requested atlas spec
 * @param out_index output atlas index
 * @return whether the spec has embedded default data
 */
static bool _text_default_msdf_spec_index(const DvzTextAtlasSpec* spec, uint32_t* out_index)
{
    ANN(spec);
    ANN(out_index);
    if (spec->backend != DVZ_TEXT_ATLAS_BACKEND_MSDF || spec->flags != 0)
        return false;
    for (uint32_t i = 0; i < DVZ_TEXT_DEFAULT_MSDF_ATLAS_COUNT; i++)
    {
        const DvzTextDefaultMsdfAtlasData* data = &DVZ_TEXT_DEFAULT_MSDF_ATLASES[i];
        if (fabsf(spec->em_px - data->spec_em_px) <= 0.001f &&
            fabsf(spec->distance_range_px - data->spec_range_px) <= 0.001f)
        {
            *out_index = i;
            return true;
        }
    }
    return false;
}



/**
 * Return whether the embedded printable-ASCII atlas covers one build set.
 *
 * @param set requested codepoints
 * @return whether all requested codepoints are embedded
 */
static bool _text_default_msdf_covers_set(const DvzTextAtlasBuildSet* set)
{
    ANN(set);
    for (uint32_t i = 0; i < set->count; i++)
    {
        uint32_t cp = set->codepoints[i];
        if (cp < DVZ_TEXT_SDF_FIRST_CHAR || cp > DVZ_TEXT_SDF_LAST_CHAR)
            return false;
    }
    return true;
}



/**
 * Try to build an atlas from embedded default MSDF data.
 *
 * @param font the scene font
 * @param spec requested atlas spec
 * @param set requested codepoint set
 * @param out_atlas output atlas
 * @return whether an embedded atlas was loaded
 */
static bool _text_default_msdf_build_atlas(
    DvzFont* font, const DvzTextAtlasSpec* spec, const DvzTextAtlasBuildSet* set,
    DvzTextAtlas** out_atlas)
{
    ANN(font);
    ANN(spec);
    ANN(set);
    ANN(out_atlas);
    *out_atlas = NULL;
#if defined(DVZ_HAS_ZLIB) && DVZ_HAS_ZLIB
    uint32_t index = 0;
    if (!_text_default_msdf_font_matches(font) || !_text_default_msdf_spec_index(spec, &index) ||
        !_text_default_msdf_covers_set(set))
    {
        return false;
    }

    const DvzTextDefaultMsdfAtlasData* data = &DVZ_TEXT_DEFAULT_MSDF_ATLASES[index];
    uint64_t start_ns = dvz_time_monotonic_ns();

    uint8_t* rgba = (uint8_t*)dvz_malloc(data->rgba_size);
    DvzTextAtlas* atlas = (DvzTextAtlas*)dvz_calloc(1, sizeof(DvzTextAtlas));
    if (rgba == NULL || atlas == NULL)
    {
        dvz_free(rgba);
        dvz_free(atlas);
        return false;
    }

    uLongf dest_len = (uLongf)data->rgba_size;
    bool ok =
        uncompress(rgba, &dest_len, data->z, (uLong)data->z_size) == Z_OK &&
        dest_len == (uLongf)data->rgba_size;
    if (!ok)
    {
        log_debug(
            "text atlas: embedded MSDF atlas decompress failed em=%.3f range=%.3f",
            (double)data->em_px, (double)data->range_px);
        dvz_free(rgba);
        dvz_free(atlas);
        return false;
    }

    atlas->spec = *spec;
    atlas->backend = DVZ_TEXT_ATLAS_BACKEND_MSDF;
    atlas->encoding = DVZ_TEXT_ATLAS_ENCODING_MSDF_RGB;
    atlas->width = data->width;
    atlas->height = data->height;
    atlas->glyph_count = data->glyph_count;
    atlas->channels = 4;
    atlas->em_px = data->em_px;
    atlas->distance_range_px = data->range_px;
    atlas->ascent = data->ascent;
    atlas->descent = data->descent;
    atlas->line_gap = data->line_gap;
    atlas->line_height = data->line_height;
    dvz_memcpy(
        atlas->glyphs, sizeof(DvzTextAtlasGlyph) * data->glyph_count, data->glyphs,
        sizeof(DvzTextAtlasGlyph) * data->glyph_count);

    ok = _text_atlas_upload_rgba(font, atlas, rgba, data->width, data->height);
    dvz_free(rgba);
    if (!ok)
    {
        _scene_text_atlas_destroy(atlas);
        return false;
    }
    *out_atlas = atlas;
    log_debug(
        "text atlas: embedded MSDF atlas ready em=%.3f range=%.3f glyphs=%u size=%ux%u "
        "z=%u rgba=%u in %.3f ms",
        (double)data->em_px, (double)data->range_px, data->glyph_count, data->width,
        data->height, data->z_size, data->rgba_size, _text_atlas_elapsed_ms(start_ns));
    return true;
#else
    (void)font;
    (void)spec;
    (void)set;
    return false;
#endif
}



/**
 * Convert an SDF value to an atlas threshold scaling constant.
 *
 * @param distance_range_px baked SDF range in atlas pixels
 * @return pixel distance scale passed to stb_truetype SDF generation
 */
static float _text_sdf_pixel_dist_scale(float distance_range_px)
{
    if (distance_range_px < 1.0f)
        distance_range_px = DVZ_TEXT_SDF_REFERENCE_RANGE_PX;
    return (float)DVZ_TEXT_SDF_ONEDGE / distance_range_px;
}



/**
 * Return whether two atlas specs address the same font-owned cache entry.
 *
 * @param a first spec
 * @param b second spec
 * @return whether the specs are equivalent
 */
static bool _text_atlas_spec_equal(const DvzTextAtlasSpec* a, const DvzTextAtlasSpec* b)
{
    ANN(a);
    ANN(b);
    return a->backend == b->backend && a->flags == b->flags &&
           fabsf(a->em_px - b->em_px) <= 0.001f &&
           fabsf(a->distance_range_px - b->distance_range_px) <= 0.001f;
}



/**
 * Return the fallback STB SDF spec matching a requested atlas scale.
 *
 * @param spec requested atlas spec
 * @return fallback SDF spec
 */
static DvzTextAtlasSpec _text_atlas_fallback_spec(const DvzTextAtlasSpec* spec)
{
    ANN(spec);
    DvzTextAtlasSpec fallback = _scene_text_atlas_spec(DVZ_TEXT_ATLAS_BACKEND_STB_SDF, spec->em_px);
    fallback.flags = spec->flags;
    return fallback;
}



/**
 * Find an existing atlas cache slot for one spec.
 *
 * @param font the font
 * @param spec atlas spec
 * @return pointer to the matching font-owned atlas cache slot, or NULL
 */
static DvzTextAtlas** _text_atlas_find_slot(DvzFont* font, const DvzTextAtlasSpec* spec)
{
    ANN(font);
    ANN(spec);
    for (uint32_t i = 0; i < font->atlas_count && i < DVZ_SCENE_MAX_TEXT_ATLASES_PER_FONT; i++)
    {
        DvzTextAtlas* atlas = font->atlases[i];
        if (atlas != NULL && _text_atlas_spec_equal(&atlas->spec, spec))
            return &font->atlases[i];
    }
    return NULL;
}



/**
 * Find an existing atlas cache entry for one spec without mutating the font.
 *
 * @param font the font
 * @param spec atlas spec
 * @return matching atlas, or NULL
 */
static const DvzTextAtlas* _text_atlas_find_const(
    const DvzFont* font, const DvzTextAtlasSpec* spec)
{
    ANN(font);
    ANN(spec);
    for (uint32_t i = 0; i < font->atlas_count && i < DVZ_SCENE_MAX_TEXT_ATLASES_PER_FONT; i++)
    {
        const DvzTextAtlas* atlas = font->atlases[i];
        if (atlas != NULL && _text_atlas_spec_equal(&atlas->spec, spec))
            return atlas;
    }
    return NULL;
}



/**
 * Find or allocate an atlas cache slot for one spec.
 *
 * @param font the font
 * @param spec atlas spec
 * @return pointer to a font-owned atlas cache slot, or NULL when the cache is full
 */
static DvzTextAtlas** _text_atlas_acquire_slot(DvzFont* font, const DvzTextAtlasSpec* spec)
{
    ANN(font);
    ANN(spec);
    DvzTextAtlas** slot = _text_atlas_find_slot(font, spec);
    if (slot != NULL)
        return slot;
    if (font->atlas_count >= DVZ_SCENE_MAX_TEXT_ATLASES_PER_FONT)
    {
        log_error("text atlas cache is full");
        return NULL;
    }
    return &font->atlases[font->atlas_count++];
}



/**
 * Upload an RGBA atlas payload into a scene sampled field.
 *
 * @param font the font whose scene owns the field
 * @param atlas the atlas metadata
 * @param rgba the atlas bytes
 * @param width atlas width
 * @param height atlas height
 * @return whether upload succeeded
 */
static bool _text_atlas_upload_rgba(
    const DvzFont* font, DvzTextAtlas* atlas, uint8_t* rgba, uint32_t width,
    uint32_t height)
{
    ANN(font);
    ANN(atlas);
    ANN(rgba);
    DvzSampledFieldDesc desc = dvz_sampled_field_desc();
    desc.width = width;
    desc.height = height;
    DvzSampledField* field = dvz_sampled_field(font->scene, &desc);
    DvzFieldDataView view = dvz_field_data_view();
    view.data = rgba;
    view.bytes_per_row = (uint64_t)width * 4u;
    view.rows_per_image = height;
    if (field == NULL || !dvz_sampled_field_set_data(field, &view))
        return false;
    atlas->field = field;
    return true;
}



/**
 * Find the bounding region that differs between two atlas fields.
 *
 * @param field the current sampled field
 * @param src the rebuilt sampled field
 * @param out_region output changed region
 * @return whether a non-empty changed region was found
 */
static bool _text_atlas_changed_region(
    const DvzSampledField* field, const DvzSampledField* src, DvzFieldRegion* out_region)
{
    ANN(field);
    ANN(src);
    ANN(out_region);
    dvz_memset(out_region, sizeof(DvzFieldRegion), 0, sizeof(DvzFieldRegion));
    if (field->data == NULL || src->data == NULL)
        return false;
    if (field->desc.width != src->desc.width || field->desc.height != src->desc.height ||
        field->desc.depth != src->desc.depth || field->desc.format != src->desc.format ||
        field->desc.dim != src->desc.dim)
    {
        return false;
    }
    if (field->desc.format != DVZ_FIELD_FORMAT_RGBA8_UNORM || field->desc.dim != DVZ_FIELD_DIM_2D ||
        field->desc.depth != 1)
    {
        return false;
    }

    const uint8_t* old_data = (const uint8_t*)field->data;
    const uint8_t* new_data = (const uint8_t*)src->data;
    uint32_t min_x = field->desc.width;
    uint32_t min_y = field->desc.height;
    uint32_t max_x = 0;
    uint32_t max_y = 0;
    bool changed = false;
    for (uint32_t y = 0; y < field->desc.height; y++)
    {
        for (uint32_t x = 0; x < field->desc.width; x++)
        {
            uint64_t offset = ((uint64_t)y * field->desc.width + x) * 4u;
            if (old_data[offset + 0] == new_data[offset + 0] &&
                old_data[offset + 1] == new_data[offset + 1] &&
                old_data[offset + 2] == new_data[offset + 2] &&
                old_data[offset + 3] == new_data[offset + 3])
            {
                continue;
            }
            if (!changed)
            {
                min_x = max_x = x;
                min_y = max_y = y;
                changed = true;
            }
            else
            {
                if (x < min_x)
                    min_x = x;
                if (y < min_y)
                    min_y = y;
                if (x > max_x)
                    max_x = x;
                if (y > max_y)
                    max_y = y;
            }
        }
    }
    if (!changed)
        return false;
    out_region->x = min_x;
    out_region->y = min_y;
    out_region->z = 0;
    out_region->width = max_x - min_x + 1u;
    out_region->height = max_y - min_y + 1u;
    out_region->depth = 1;
    return true;
}



/**
 * Replace one sampled-field payload, using a subregion update when possible.
 *
 * @param field the destination sampled field
 * @param src the source sampled field
 * @return whether replacement succeeded
 */
static bool _text_atlas_replace_field_data(DvzSampledField* field, const DvzSampledField* src)
{
    ANN(field);
    ANN(src);
    if (src->data == NULL)
        return false;
    if (field->desc.width != src->desc.width || field->desc.height != src->desc.height ||
        field->desc.depth != src->desc.depth || field->desc.format != src->desc.format ||
        field->desc.dim != src->desc.dim)
    {
        return false;
    }
    DvzFieldRegion region = {};
    if (_text_atlas_changed_region(field, src, &region))
    {
        const uint8_t* data = (const uint8_t*)src->data;
        uint64_t offset = ((uint64_t)region.y * src->desc.width + region.x) * 4u;
        DvzFieldDataView region_view = dvz_field_data_view();
        region_view.data = data + offset;
        region_view.bytes_per_row = (uint64_t)src->desc.width * 4u;
        region_view.rows_per_image = src->desc.height;
        return dvz_sampled_field_update_region(field, region, &region_view);
    }
    if (field->data != NULL && field->desc.format == DVZ_FIELD_FORMAT_RGBA8_UNORM &&
        field->desc.dim == DVZ_FIELD_DIM_2D && field->desc.depth == 1)
    {
        return true;
    }
    DvzFieldDataView view = dvz_field_data_view();
    view.data = src->data;
    view.bytes_per_row = (uint64_t)src->desc.width * 4u;
    view.rows_per_image = src->desc.height;
    return dvz_sampled_field_set_data(field, &view);
}



#if defined(DVZ_HAS_FREETYPE) && DVZ_HAS_FREETYPE
/**
 * Copy one FreeType glyph bitmap into an RGBA alpha atlas.
 *
 * @param atlas_rgba destination RGBA atlas bytes
 * @param atlas_width destination atlas width
 * @param dst_x destination glyph x origin
 * @param dst_y destination glyph y origin
 * @param bitmap source FreeType bitmap
 */
static void _text_ft_copy_bitmap(
    uint8_t* atlas_rgba, uint32_t atlas_width, uint32_t dst_x, uint32_t dst_y,
    const FT_Bitmap* bitmap)
{
    ANN(atlas_rgba);
    ANN(bitmap);
    for (uint32_t y = 0; y < (uint32_t)bitmap->rows; y++)
    {
        for (uint32_t x = 0; x < (uint32_t)bitmap->width; x++)
        {
            uint8_t coverage = 0;
            if (bitmap->pixel_mode == FT_PIXEL_MODE_GRAY)
            {
                const uint8_t* row = bitmap->buffer + (uint64_t)y * (uint32_t)bitmap->pitch;
                coverage = row[x];
            }
            else if (bitmap->pixel_mode == FT_PIXEL_MODE_MONO)
            {
                const uint8_t* row = bitmap->buffer + (uint64_t)y * (uint32_t)bitmap->pitch;
                coverage = (row[x / 8u] & (uint8_t)(0x80u >> (x % 8u))) != 0 ? 255 : 0;
            }
            uint64_t dst = ((uint64_t)(dst_y + y) * atlas_width + dst_x + x) * 4u;
            atlas_rgba[dst + 0] = 0;
            atlas_rgba[dst + 1] = 0;
            atlas_rgba[dst + 2] = 0;
            atlas_rgba[dst + 3] = coverage;
        }
    }
}



/**
 * Build a hinted FreeType bitmap atlas for printable ASCII glyphs.
 *
 * @param font the font
 * @param spec atlas generation spec
 * @param set the requested codepoint set
 * @param out_atlas output atlas metadata
 * @return whether atlas creation succeeded
 */
static bool _text_ft_build_bitmap_atlas(
    DvzFont* font, const DvzTextAtlasSpec* spec, const DvzTextAtlasBuildSet* set,
    DvzTextAtlas** out_atlas)
{
    ANN(font);
    ANN(spec);
    ANN(set);
    ANN(out_atlas);
    *out_atlas = NULL;
    if (set->count == 0 || set->count > DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS)
        return false;
    if (!_text_sdf_font_bytes(font))
        return false;

    FT_Library library = NULL;
    FT_Face face = NULL;
    if (FT_Init_FreeType(&library) != 0)
    {
        log_error("failed to initialize FreeType");
        return false;
    }
    if (FT_New_Memory_Face(
            library, (const FT_Byte*)font->ttf_bytes, (FT_Long)font->ttf_size,
            (FT_Long)font->face_index, &face) != 0)
    {
        log_error("failed to load FreeType font face");
        FT_Done_FreeType(library);
        return false;
    }

    float em_px = spec->em_px > 0.0f ? spec->em_px : DVZ_TEXT_ATLAS_DEFAULT_EM_PX;
    if (FT_Set_Pixel_Sizes(face, 0, (FT_UInt)em_px) != 0)
    {
        log_error("failed to set FreeType pixel size");
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        return false;
    }

    const uint32_t glyph_count = set->count;
    uint32_t glyph_widths[DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS] = {};
    uint32_t glyph_heights[DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS] = {};
    int glyph_lefts[DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS] = {};
    int glyph_tops[DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS] = {};
    float glyph_advances[DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS] = {};
    uint32_t cell_width = 1;
    uint32_t cell_height = 1;

    for (uint32_t i = 0; i < glyph_count; i++)
    {
        uint32_t codepoint = set->codepoints[i];
        if (FT_Load_Char(face, (FT_ULong)codepoint, FT_LOAD_DEFAULT) != 0 ||
            FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL) != 0)
            continue;
        glyph_widths[i] = (uint32_t)face->glyph->bitmap.width;
        glyph_heights[i] = (uint32_t)face->glyph->bitmap.rows;
        glyph_lefts[i] = face->glyph->bitmap_left;
        glyph_tops[i] = face->glyph->bitmap_top;
        glyph_advances[i] = (float)face->glyph->advance.x / 64.0f;
        if (glyph_widths[i] > cell_width)
            cell_width = glyph_widths[i];
        if (glyph_heights[i] > cell_height)
            cell_height = glyph_heights[i];
    }

    cell_width += 2u * DVZ_TEXT_BITMAP_PADDING;
    cell_height += 2u * DVZ_TEXT_BITMAP_PADDING;
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
        log_error("text FreeType atlas dimensions overflow");
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        return false;
    }

    uint32_t atlas_width = (uint32_t)width64;
    uint32_t atlas_height = (uint32_t)height64;
    uint8_t* rgba = (uint8_t*)dvz_calloc((DvzSize)byte_size, 1);
    DvzTextAtlas* atlas = (DvzTextAtlas*)dvz_calloc(1, sizeof(DvzTextAtlas));
    if (rgba == NULL || atlas == NULL)
    {
        log_error("text FreeType atlas allocation failed");
        dvz_free(rgba);
        dvz_free(atlas);
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        return false;
    }

    atlas->spec = *spec;
    atlas->backend = DVZ_TEXT_ATLAS_BACKEND_FREETYPE_BITMAP;
    atlas->encoding = DVZ_TEXT_ATLAS_ENCODING_BITMAP_ALPHA;
    atlas->width = atlas_width;
    atlas->height = atlas_height;
    atlas->glyph_count = glyph_count;
    atlas->channels = 4;
    atlas->em_px = em_px;
    atlas->ascent = face->size != NULL ? (float)face->size->metrics.ascender / 64.0f : em_px;
    atlas->descent = face->size != NULL ? (float)face->size->metrics.descender / 64.0f : 0.0f;
    atlas->line_gap = 0.0f;
    atlas->line_height =
        face->size != NULL ? (float)face->size->metrics.height / 64.0f : em_px;
    if (atlas->line_height <= 0.0f)
        atlas->line_height = em_px;

    for (uint32_t i = 0; i < glyph_count; i++)
    {
        uint32_t codepoint = set->codepoints[i];
        DvzTextAtlasGlyph* glyph = &atlas->glyphs[i];
        glyph->codepoint = codepoint;
        glyph->glyph_id = (uint32_t)FT_Get_Char_Index(face, (FT_ULong)codepoint);
        if (glyph->glyph_id == 0 && codepoint != DVZ_TEXT_SDF_FALLBACK)
        {
            atlas->missing_glyph_count++;
            continue;
        }
        glyph->advance = glyph_advances[i];
        uint32_t col = i % DVZ_TEXT_SDF_COLUMNS;
        uint32_t row = i / DVZ_TEXT_SDF_COLUMNS;
        uint32_t x = col * cell_width + DVZ_TEXT_BITMAP_PADDING;
        uint32_t y = row * cell_height + DVZ_TEXT_BITMAP_PADDING;
        uint32_t sample_x0 = x;
        uint32_t sample_y0 = y;
        uint32_t sample_x1 = x + glyph_widths[i];
        uint32_t sample_y1 = y + glyph_heights[i];
        if (glyph_widths[i] > 0 && glyph_heights[i] > 0)
        {
            sample_x0 -= DVZ_TEXT_BITMAP_PADDING;
            sample_y0 -= DVZ_TEXT_BITMAP_PADDING;
            sample_x1 += DVZ_TEXT_BITMAP_PADDING;
            sample_y1 += DVZ_TEXT_BITMAP_PADDING;
        }
        glyph->xoff = (float)glyph_lefts[i] - (float)(x - sample_x0);
        glyph->yoff = -(float)glyph_tops[i] - (float)(y - sample_y0);
        glyph->width = (float)(sample_x1 - sample_x0);
        glyph->height = (float)(sample_y1 - sample_y0);
        glyph->atlas_bounds[0] = (float)x;
        glyph->atlas_bounds[1] = (float)y;
        glyph->atlas_bounds[2] = (float)(x + glyph_widths[i]);
        glyph->atlas_bounds[3] = (float)(y + glyph_heights[i]);
        glyph->plane_bounds[0] = glyph->xoff;
        glyph->plane_bounds[1] = glyph->yoff;
        glyph->plane_bounds[2] = glyph->xoff + glyph->width;
        glyph->plane_bounds[3] = glyph->yoff + glyph->height;
        glyph->uv[0] = (float)sample_x0 / (float)atlas_width;
        glyph->uv[1] = (float)sample_y0 / (float)atlas_height;
        glyph->uv[2] = (float)sample_x1 / (float)atlas_width;
        glyph->uv[3] = (float)sample_y1 / (float)atlas_height;
        glyph->valid = glyph->advance > 0.0f || glyph_widths[i] > 0 || glyph_heights[i] > 0;

        if (glyph_widths[i] == 0 || glyph_heights[i] == 0)
            continue;
        if (FT_Load_Char(face, (FT_ULong)codepoint, FT_LOAD_DEFAULT) != 0 ||
            FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL) != 0)
            continue;
        _text_ft_copy_bitmap(rgba, atlas_width, x, y, &face->glyph->bitmap);
    }
    for (uint32_t i = 0; i < glyph_count; i++)
    {
        if (!atlas->glyphs[i].valid && atlas->glyphs[i].codepoint != DVZ_TEXT_SDF_FALLBACK)
            atlas->missing_glyph_count++;
    }

    bool ok = _text_atlas_upload_rgba(font, atlas, rgba, atlas_width, atlas_height);
    dvz_free(rgba);
    FT_Done_Face(face);
    FT_Done_FreeType(library);
    if (!ok)
    {
        _scene_text_atlas_destroy(atlas);
        return false;
    }

    *out_atlas = atlas;
    return true;
}
#endif



/**
 * Build an stb_truetype SDF atlas for the requested codepoint set.
 *
 * @param font the font
 * @param spec atlas generation spec
 * @param set the requested codepoint set
 * @param out_atlas output atlas metadata
 * @return whether atlas creation succeeded
 */
static bool _text_sdf_build_atlas(
    DvzFont* font, const DvzTextAtlasSpec* spec, const DvzTextAtlasBuildSet* set,
    DvzTextAtlas** out_atlas)
{
    ANN(font);
    ANN(spec);
    ANN(set);
    ANN(out_atlas);
    *out_atlas = NULL;
    if (set->count == 0 || set->count > DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS)
        return false;
    if (!_text_sdf_font_bytes(font))
        return false;

    stbtt_fontinfo info = {};
    if (!_text_sdf_init_font(font, &info))
        return false;

    const float em_px = spec->em_px > 0.0f ? spec->em_px : DVZ_TEXT_ATLAS_DEFAULT_EM_PX;
    const float distance_range_px = spec->distance_range_px > 0.0f
                                        ? spec->distance_range_px
                                        : DVZ_TEXT_SDF_REFERENCE_RANGE_PX;
    const int padding = (int)(distance_range_px + 0.5f);
    const float scale = stbtt_ScaleForPixelHeight(&info, em_px);
    const uint32_t glyph_count = set->count;
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
        uint32_t codepoint = set->codepoints[i];
        unsigned char* sdf = stbtt_GetCodepointSDF(
            &info, scale, (int)codepoint, padding, (unsigned char)DVZ_TEXT_SDF_ONEDGE,
            _text_sdf_pixel_dist_scale(distance_range_px), &width, &height, &xoff, &yoff);
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
    atlas->spec = *spec;
    atlas->backend = DVZ_TEXT_ATLAS_BACKEND_STB_SDF;
    atlas->encoding = DVZ_TEXT_ATLAS_ENCODING_SDF_ALPHA;
    atlas->width = atlas_width;
    atlas->height = atlas_height;
    atlas->glyph_count = glyph_count;
    atlas->channels = 4;
    atlas->em_px = em_px;
    atlas->distance_range_px = distance_range_px;
    atlas->ascent = (float)ascent * scale;
    atlas->descent = (float)descent * scale;
    atlas->line_gap = (float)line_gap * scale;
    atlas->line_height = (float)(ascent - descent + line_gap) * scale;
    if (atlas->line_height <= 0.0f)
        atlas->line_height = em_px;

    for (uint32_t i = 0; i < glyph_count; i++)
    {
        uint32_t codepoint = set->codepoints[i];
        DvzTextAtlasGlyph* glyph = &atlas->glyphs[i];
        glyph->codepoint = codepoint;
        glyph->glyph_id = (uint32_t)stbtt_FindGlyphIndex(&info, (int)codepoint);
        if (glyph->glyph_id == 0 && codepoint != DVZ_TEXT_SDF_FALLBACK)
        {
            atlas->missing_glyph_count++;
            continue;
        }
        int advance = 0;
        int left_bearing = 0;
        stbtt_GetCodepointHMetrics(&info, (int)codepoint, &advance, &left_bearing);
        (void)left_bearing;
        glyph->advance = (float)advance * scale;
        if (glyph_sdfs[i] == NULL)
        {
            glyph->valid = glyph->advance > 0.0f;
            if (!glyph->valid && codepoint != DVZ_TEXT_SDF_FALLBACK)
                atlas->missing_glyph_count++;
            continue;
        }

        uint32_t col = i % DVZ_TEXT_SDF_COLUMNS;
        uint32_t row = i / DVZ_TEXT_SDF_COLUMNS;
        uint32_t x = col * cell_width;
        uint32_t y = row * cell_height;
        _text_sdf_copy_glyph(
            rgba, atlas_width, x, y, glyph_sdfs[i], glyph_widths[i], glyph_heights[i]);
        for (uint32_t gy = 0; gy < glyph_heights[i]; gy++)
        {
            for (uint32_t gx = 0; gx < glyph_widths[i]; gx++)
            {
                uint64_t index = ((uint64_t)(y + gy) * atlas_width + x + gx) * 4u;
                rgba[index + 0] = rgba[index + 3];
                rgba[index + 1] = rgba[index + 3];
                rgba[index + 2] = rgba[index + 3];
            }
        }

        glyph->xoff = (float)glyph_xoffs[i];
        glyph->yoff = (float)glyph_yoffs[i];
        glyph->width = (float)glyph_widths[i];
        glyph->height = (float)glyph_heights[i];
        glyph->atlas_bounds[0] = (float)x;
        glyph->atlas_bounds[1] = (float)y;
        glyph->atlas_bounds[2] = (float)(x + glyph_widths[i]);
        glyph->atlas_bounds[3] = (float)(y + glyph_heights[i]);
        glyph->plane_bounds[0] = glyph->xoff;
        glyph->plane_bounds[1] = glyph->yoff;
        glyph->plane_bounds[2] = glyph->xoff + glyph->width;
        glyph->plane_bounds[3] = glyph->yoff + glyph->height;
        glyph->uv[0] = (float)x / (float)atlas_width;
        glyph->uv[1] = (float)y / (float)atlas_height;
        glyph->uv[2] = (float)(x + glyph_widths[i]) / (float)atlas_width;
        glyph->uv[3] = (float)(y + glyph_heights[i]) / (float)atlas_height;
        glyph->valid = true;
    }

    for (uint32_t i = 0; i < glyph_count; i++)
        if (glyph_sdfs[i] != NULL)
            stbtt_FreeSDF(glyph_sdfs[i], NULL);

    bool ok = _text_atlas_upload_rgba(font, atlas, rgba, atlas_width, atlas_height);
    dvz_free(rgba);
    if (!ok)
    {
        _scene_text_atlas_destroy(atlas);
        return false;
    }

    *out_atlas = atlas;
    return true;
}



/**
 * Build a temporary atlas for a specific backend.
 *
 * @param font the font
 * @param spec atlas generation spec
 * @param set requested codepoint set
 * @param out_atlas output temporary atlas
 * @return whether atlas generation succeeded
 */
static bool _text_atlas_build_backend(
    DvzFont* font, const DvzTextAtlasSpec* spec, const DvzTextAtlasBuildSet* set,
    DvzTextAtlas** out_atlas)
{
    ANN(font);
    ANN(spec);
    ANN(set);
    ANN(out_atlas);
    *out_atlas = NULL;
    switch (spec->backend)
    {
    case DVZ_TEXT_ATLAS_BACKEND_FREETYPE_BITMAP:
#if defined(DVZ_HAS_FREETYPE) && DVZ_HAS_FREETYPE
        return _text_ft_build_bitmap_atlas(font, spec, set, out_atlas);
#else
        return false;
#endif
    case DVZ_TEXT_ATLAS_BACKEND_MSDF:
#if defined(DVZ_HAS_MSDF_ATLAS) && DVZ_HAS_MSDF_ATLAS
        return _text_msdf_build_atlas(font, spec, set, out_atlas);
#else
        return false;
#endif
    case DVZ_TEXT_ATLAS_BACKEND_STB_SDF:
    case DVZ_TEXT_ATLAS_BACKEND_BUILTIN_BITMAP:
    default:
        return _text_sdf_build_atlas(font, spec, set, out_atlas);
    }
}



/**
 * Mark occupied atlas texels for one glyph rectangle.
 *
 * @param occupied atlas occupancy map
 * @param width atlas width
 * @param height atlas height
 * @param glyph glyph metadata
 */
static void _text_atlas_mark_occupied(
    uint8_t* occupied, uint32_t width, uint32_t height, const DvzTextAtlasGlyph* glyph)
{
    ANN(occupied);
    ANN(glyph);
    int32_t x0 = (int32_t)glyph->atlas_bounds[0];
    int32_t y0 = (int32_t)glyph->atlas_bounds[1];
    int32_t x1 = (int32_t)glyph->atlas_bounds[2];
    int32_t y1 = (int32_t)glyph->atlas_bounds[3];
    if (x0 < 0)
        x0 = 0;
    if (y0 < 0)
        y0 = 0;
    if (x1 > (int32_t)width)
        x1 = (int32_t)width;
    if (y1 > (int32_t)height)
        y1 = (int32_t)height;
    if (x1 <= x0 || y1 <= y0)
        return;
    for (uint32_t y = (uint32_t)y0; y < (uint32_t)y1; y++)
    {
        for (uint32_t x = (uint32_t)x0; x < (uint32_t)x1; x++)
            occupied[(uint64_t)y * width + x] = 1;
    }
}



/**
 * Find a free atlas rectangle with a simple top-left scan.
 *
 * @param occupied atlas occupancy map
 * @param width atlas width
 * @param height atlas height
 * @param rect_width requested rectangle width
 * @param rect_height requested rectangle height
 * @param out_x output x position
 * @param out_y output y position
 * @return whether a free rectangle was found
 */
static bool _text_atlas_find_free_rect(
    const uint8_t* occupied, uint32_t width, uint32_t height, uint32_t rect_width,
    uint32_t rect_height, uint32_t* out_x, uint32_t* out_y)
{
    ANN(occupied);
    ANN(out_x);
    ANN(out_y);
    if (rect_width == 0 || rect_height == 0 || rect_width > width || rect_height > height)
        return false;
    for (uint32_t y = 0; y <= height - rect_height; y++)
    {
        for (uint32_t x = 0; x <= width - rect_width; x++)
        {
            bool free_rect = true;
            for (uint32_t yy = 0; yy < rect_height && free_rect; yy++)
            {
                for (uint32_t xx = 0; xx < rect_width; xx++)
                {
                    if (occupied[(uint64_t)(y + yy) * width + x + xx] != 0)
                    {
                        free_rect = false;
                        break;
                    }
                }
            }
            if (free_rect)
            {
                *out_x = x;
                *out_y = y;
                return true;
            }
        }
    }
    return false;
}



/**
 * Append missing glyphs into an existing atlas without rearranging existing glyphs.
 *
 * @param font the font
 * @param atlas the existing atlas to mutate
 * @param requested_set requested codepoints
 * @return whether all missing glyphs were appended
 */
static bool _text_atlas_try_append(
    DvzFont* font, DvzTextAtlas* atlas, const DvzTextAtlasBuildSet* requested_set)
{
    ANN(font);
    ANN(atlas);
    ANN(requested_set);
    if (atlas->field == NULL || atlas->field->data == NULL || atlas->width == 0 ||
        atlas->height == 0)
    {
        return false;
    }

    DvzTextAtlasBuildSet missing_set = {};
    for (uint32_t i = 0; i < requested_set->count; i++)
    {
        uint32_t codepoint = requested_set->codepoints[i];
        if (!_text_atlas_contains_codepoint(atlas, codepoint) &&
            !_text_atlas_build_set_add(&missing_set, codepoint))
        {
            return false;
        }
    }
    if (missing_set.count == 0)
        return true;
    if (atlas->glyph_count + missing_set.count > DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS)
        return false;

    DvzTextAtlas* delta = NULL;
    if (!_text_atlas_build_backend(font, &atlas->spec, &missing_set, &delta))
        return false;
    ANN(delta);
    if (delta->field == NULL || delta->field->data == NULL)
    {
        _scene_text_atlas_destroy(delta);
        return false;
    }

    uint32_t target_width = atlas->width * 2u;
    uint32_t target_height = atlas->height * 2u;
    if (target_width <= atlas->width || target_height <= atlas->height)
    {
        _scene_text_atlas_destroy(delta);
        return false;
    }

    uint64_t occupied_size = (uint64_t)target_width * target_height;
    if (occupied_size > SIZE_MAX)
    {
        _scene_text_atlas_destroy(delta);
        return false;
    }
    uint8_t* occupied = (uint8_t*)dvz_calloc((DvzSize)occupied_size, 1);
    if (occupied == NULL)
    {
        _scene_text_atlas_destroy(delta);
        return false;
    }
    for (uint32_t i = 0; i < atlas->glyph_count; i++)
        _text_atlas_mark_occupied(occupied, target_width, target_height, &atlas->glyphs[i]);

    DvzTextAtlasGlyph staged[DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS] = {};
    DvzFieldRegion dst_regions[DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS] = {};
    DvzFieldRegion src_regions[DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS] = {};
    uint32_t staged_count = 0;
    bool ok = true;
    for (uint32_t i = 0; i < missing_set.count && ok; i++)
    {
        uint32_t codepoint = missing_set.codepoints[i];
        const DvzTextAtlasGlyph* src_glyph = _text_atlas_find_glyph(delta, codepoint);
        if (src_glyph == NULL)
        {
            staged[staged_count].codepoint = codepoint;
            staged_count++;
            atlas->missing_glyph_count++;
            continue;
        }

        DvzTextAtlasGlyph glyph = *src_glyph;
        uint32_t sx0 = (uint32_t)glyph.atlas_bounds[0];
        uint32_t sy0 = (uint32_t)glyph.atlas_bounds[1];
        uint32_t sx1 = (uint32_t)glyph.atlas_bounds[2];
        uint32_t sy1 = (uint32_t)glyph.atlas_bounds[3];
        uint32_t rect_width = sx1 > sx0 ? sx1 - sx0 : 0;
        uint32_t rect_height = sy1 > sy0 ? sy1 - sy0 : 0;
        if (!glyph.valid || rect_width == 0 || rect_height == 0)
        {
            staged[staged_count++] = glyph;
            if (codepoint != DVZ_TEXT_SDF_FALLBACK)
                atlas->missing_glyph_count++;
            continue;
        }

        uint32_t dx = 0;
        uint32_t dy = 0;
        if (!_text_atlas_find_free_rect(
                occupied, target_width, target_height, rect_width, rect_height, &dx, &dy))
        {
            ok = false;
            break;
        }

        float u0 = glyph.uv[0] * (float)delta->width;
        float v0 = glyph.uv[1] * (float)delta->height;
        float u1 = glyph.uv[2] * (float)delta->width;
        float v1 = glyph.uv[3] * (float)delta->height;
        glyph.atlas_bounds[0] = (float)dx;
        glyph.atlas_bounds[1] = (float)dy;
        glyph.atlas_bounds[2] = (float)(dx + rect_width);
        glyph.atlas_bounds[3] = (float)(dy + rect_height);
        glyph.uv[0] = ((float)dx + (u0 - (float)sx0)) / (float)target_width;
        glyph.uv[1] = ((float)dy + (v0 - (float)sy0)) / (float)target_height;
        glyph.uv[2] = ((float)dx + (u1 - (float)sx0)) / (float)target_width;
        glyph.uv[3] = ((float)dy + (v1 - (float)sy0)) / (float)target_height;

        staged[staged_count] = glyph;
        src_regions[staged_count].x = sx0;
        src_regions[staged_count].y = sy0;
        src_regions[staged_count].z = 0;
        src_regions[staged_count].width = rect_width;
        src_regions[staged_count].height = rect_height;
        src_regions[staged_count].depth = 1;
        dst_regions[staged_count].x = dx;
        dst_regions[staged_count].y = dy;
        dst_regions[staged_count].z = 0;
        dst_regions[staged_count].width = rect_width;
        dst_regions[staged_count].height = rect_height;
        dst_regions[staged_count].depth = 1;
        staged_count++;
        _text_atlas_mark_occupied(occupied, target_width, target_height, &glyph);
    }

    if (ok)
    {
        uint64_t grown_pixel_count = 0;
        uint64_t grown_byte_size = 0;
        if (_dvz_mul_u64_overflows(target_width, target_height, &grown_pixel_count) ||
            _dvz_mul_u64_overflows(grown_pixel_count, 4u, &grown_byte_size) ||
            grown_byte_size > SIZE_MAX)
        {
            ok = false;
        }
        uint8_t* grown = ok ? (uint8_t*)dvz_calloc((DvzSize)grown_byte_size, 1) : NULL;
        if (ok && grown == NULL)
            ok = false;
        if (ok)
        {
            const uint8_t* old_data = (const uint8_t*)atlas->field->data;
            for (uint32_t y = 0; y < atlas->height; y++)
            {
                uint64_t src_offset = (uint64_t)y * atlas->width * 4u;
                uint64_t dst_offset = (uint64_t)y * target_width * 4u;
                dvz_memcpy(
                    grown + dst_offset, (uint64_t)atlas->width * 4u, old_data + src_offset,
                    (uint64_t)atlas->width * 4u);
            }
        }

        const uint8_t* delta_data = (const uint8_t*)delta->field->data;
        for (uint32_t i = 0; i < staged_count && ok; i++)
        {
            if (!staged[i].valid || dst_regions[i].width == 0 || dst_regions[i].height == 0)
                continue;
            for (uint32_t y = 0; y < dst_regions[i].height; y++)
            {
                uint64_t src_offset =
                    ((uint64_t)(src_regions[i].y + y) * delta->width + src_regions[i].x) * 4u;
                uint64_t dst_offset =
                    ((uint64_t)(dst_regions[i].y + y) * target_width + dst_regions[i].x) * 4u;
                dvz_memcpy(
                    grown + dst_offset, (uint64_t)dst_regions[i].width * 4u,
                    delta_data + src_offset, (uint64_t)dst_regions[i].width * 4u);
            }
        }
        if (ok)
        {
            DvzFieldDataView view = dvz_field_data_view();
            view.data = grown;
            view.bytes_per_row = (uint64_t)target_width * 4u;
            view.rows_per_image = target_height;
            ok = dvz_sampled_field_resize(atlas->field, target_width, target_height, 1, &view);
        }
        if (ok)
        {
            float old_width = (float)atlas->width;
            float old_height = (float)atlas->height;
            for (uint32_t i = 0; i < atlas->glyph_count; i++)
            {
                atlas->glyphs[i].uv[0] = atlas->glyphs[i].uv[0] * old_width / target_width;
                atlas->glyphs[i].uv[1] = atlas->glyphs[i].uv[1] * old_height / target_height;
                atlas->glyphs[i].uv[2] = atlas->glyphs[i].uv[2] * old_width / target_width;
                atlas->glyphs[i].uv[3] = atlas->glyphs[i].uv[3] * old_height / target_height;
            }
            atlas->width = target_width;
            atlas->height = target_height;
        }
        dvz_free(grown);
    }

    if (ok)
    {
        for (uint32_t i = 0; i < staged_count; i++)
            atlas->glyphs[atlas->glyph_count++] = staged[i];
    }

    dvz_free(occupied);
    _scene_text_atlas_destroy(delta);
    return ok;
}



/**
 * Store a newly built atlas in the font cache.
 *
 * @param font the font
 * @param spec atlas generation spec
 * @param atlas the new atlas
 * @return whether the atlas was stored
 */
static bool _text_atlas_store(DvzFont* font, const DvzTextAtlasSpec* spec, DvzTextAtlas* atlas)
{
    ANN(font);
    ANN(spec);
    ANN(atlas);
    DvzTextAtlas** slot = _text_atlas_acquire_slot(font, spec);
    if (slot == NULL)
        return false;
    if (*slot != NULL && (*slot)->field != NULL && atlas->field != NULL &&
        _text_atlas_replace_field_data((*slot)->field, atlas->field))
    {
        DvzSampledField* field = (*slot)->field;
        DvzSampledField* temporary_field = atlas->field;
        (*slot)->field = NULL;
        atlas->field = field;
        _scene_text_atlas_destroy(*slot);
        dvz_sampled_field_destroy(temporary_field);
        *slot = atlas;
        font->version++;
        atlas->generation = font->version;
        return true;
    }
    if (*slot != NULL)
        _scene_text_atlas_destroy(*slot);
    *slot = atlas;
    font->version++;
    atlas->generation = font->version;
    return true;
}



/**
 * Ensure one exact font atlas spec covers a requested build set.
 *
 * @param font the scene font
 * @param spec requested atlas spec
 * @param requested_set the requested codepoints
 * @return whether an atlas is available
 */
static bool _text_atlas_ensure_exact_set(
    DvzFont* font, const DvzTextAtlasSpec* spec, const DvzTextAtlasBuildSet* requested_set)
{
    ANN(font);
    ANN(spec);
    ANN(requested_set);
    if (font->scene == NULL)
        return false;

    DvzTextAtlasBuildSet set = *requested_set;
    DvzTextAtlas** slot = _text_atlas_find_slot(font, spec);
    DvzTextAtlas* existing = slot != NULL ? *slot : NULL;
    if (!_text_atlas_build_set_add_existing(&set, existing))
        return existing != NULL && existing->field != NULL;
    if (_text_atlas_contains_set(existing, &set))
    {
        log_trace(
            "text atlas: cache hit backend=%d em=%.3f range=%.3f glyphs=%u",
            (int)spec->backend, (double)spec->em_px, (double)spec->distance_range_px,
            set.count);
        return true;
    }
    if (existing != NULL && _text_atlas_try_append(font, existing, &set))
    {
        log_debug(
            "text atlas: appended glyphs backend=%d em=%.3f range=%.3f glyphs=%u",
            (int)spec->backend, (double)spec->em_px, (double)spec->distance_range_px,
            existing->glyph_count);
        font->version++;
        existing->generation = font->version;
        return true;
    }

    DvzTextAtlas* atlas = NULL;
    if (_text_default_msdf_build_atlas(font, spec, &set, &atlas))
    {
        if (_text_atlas_store(font, spec, atlas))
            return true;
        _scene_text_atlas_destroy(atlas);
        return existing != NULL && existing->field != NULL;
    }

    atlas = NULL;
    log_debug(
        "text atlas: building backend atlas backend=%d em=%.3f range=%.3f glyphs=%u",
        (int)spec->backend, (double)spec->em_px, (double)spec->distance_range_px, set.count);
    if (!_text_atlas_build_backend(font, spec, &set, &atlas))
        return existing != NULL && existing->field != NULL;
    if (!_text_atlas_store(font, spec, atlas))
    {
        _scene_text_atlas_destroy(atlas);
        return existing != NULL && existing->field != NULL;
    }
    return true;
}



/**
 * Ensure one font atlas covers a requested build set, falling back to STB SDF when needed.
 *
 * @param font the scene font
 * @param spec requested atlas spec
 * @param requested_set the requested codepoints
 * @return whether an atlas is available
 */
static bool _text_atlas_ensure_set(
    DvzFont* font, const DvzTextAtlasSpec* spec, const DvzTextAtlasBuildSet* requested_set)
{
    ANN(font);
    ANN(spec);
    ANN(requested_set);
    if (_text_atlas_ensure_exact_set(font, spec, requested_set))
        return true;
    if (spec->backend == DVZ_TEXT_ATLAS_BACKEND_MSDF)
        log_debug("MSDF atlas generation failed; falling back to SDF atlas");
    if (spec->backend == DVZ_TEXT_ATLAS_BACKEND_FREETYPE_BITMAP)
        log_debug("FreeType bitmap atlas generation failed; falling back to SDF atlas");
    if (spec->backend == DVZ_TEXT_ATLAS_BACKEND_STB_SDF)
        return false;
    DvzTextAtlasSpec fallback = _text_atlas_fallback_spec(spec);
    return _text_atlas_ensure_exact_set(font, &fallback, requested_set);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

extern "C" {

/**
 * Resolve a text atlas specification from a renderer and rendered text size.
 *
 * @param renderer requested text renderer
 * @param size_px rendered text size in pixels
 * @return atlas generation spec
 */
DvzTextAtlasSpec dvz_text_atlas_spec(DvzTextRenderer renderer, float size_px)
{
    DvzTextAtlasBackend backend = DVZ_TEXT_ATLAS_BACKEND_MSDF;
    switch (renderer)
    {
    case DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS:
        backend = DVZ_TEXT_ATLAS_BACKEND_BUILTIN_BITMAP;
        break;
    case DVZ_TEXT_RENDERER_BITMAP_ATLAS:
#if defined(DVZ_HAS_FREETYPE) && DVZ_HAS_FREETYPE
        backend = DVZ_TEXT_ATLAS_BACKEND_FREETYPE_BITMAP;
#else
        backend = DVZ_TEXT_ATLAS_BACKEND_BUILTIN_BITMAP;
#endif
        break;
    case DVZ_TEXT_RENDERER_AUTO:
        backend = size_px < 14.0f ? DVZ_TEXT_ATLAS_BACKEND_BUILTIN_BITMAP
                                  : DVZ_TEXT_ATLAS_BACKEND_MSDF;
        break;
    case DVZ_TEXT_RENDERER_MSDF_ATLAS:
    case DVZ_TEXT_RENDERER_VECTOR_GPU:
    default:
        backend = DVZ_TEXT_ATLAS_BACKEND_MSDF;
        break;
    }
    return _scene_text_atlas_spec(backend, size_px);
}


/**
 * Ensure a font has an atlas for Datoviz's default text glyph set.
 *
 * @param font the font
 * @param spec requested atlas spec
 * @return true when the atlas is available
 */
bool dvz_font_atlas_ensure(DvzFont* font, const DvzTextAtlasSpec* spec)
{
    if (font == NULL || spec == NULL)
        return false;
    return _scene_text_atlas_ensure(font, spec);
}


/**
 * Ensure a font has an atlas that covers one UTF-8 string.
 *
 * @param font the font
 * @param spec requested atlas spec
 * @param string the UTF-8 string
 * @return true when the atlas is available
 */
bool dvz_font_atlas_ensure_string(DvzFont* font, const DvzTextAtlasSpec* spec, const char* string)
{
    if (font == NULL || spec == NULL || string == NULL)
        return false;
    return _scene_text_atlas_ensure_string(font, spec, string);
}


/**
 * Ensure a font has an atlas that covers a list of UTF-8 strings.
 *
 * @param font the font
 * @param spec requested atlas spec
 * @param strings the UTF-8 strings
 * @param count string count
 * @return true when the atlas is available
 */
bool dvz_font_atlas_ensure_strings(
    DvzFont* font, const DvzTextAtlasSpec* spec, const char* const* strings, uint32_t count)
{
    if (font == NULL || spec == NULL || strings == NULL || count == 0)
        return false;
    return _scene_text_atlas_ensure_strings(font, spec, strings, count);
}


/**
 * Return the font atlas matching a requested spec, including fallback atlases.
 *
 * @param font the scene font
 * @param spec requested atlas spec
 * @return atlas pointer, or NULL when unavailable
 */
const DvzTextAtlas* dvz_font_atlas(const DvzFont* font, const DvzTextAtlasSpec* spec)
{
    if (font == NULL || spec == NULL)
        return NULL;
    const DvzTextAtlas* atlas = _text_atlas_find_const(font, spec);
    if (atlas != NULL)
        return atlas;
    if (spec->backend == DVZ_TEXT_ATLAS_BACKEND_STB_SDF)
        return NULL;
    DvzTextAtlasSpec fallback = _text_atlas_fallback_spec(spec);
    return _text_atlas_find_const(font, &fallback);
}


/**
 * Return immutable atlas metadata.
 *
 * @param atlas the text atlas
 * @return atlas metadata; zeroed when atlas is NULL
 */
DvzTextAtlasInfo dvz_text_atlas_info(const DvzTextAtlas* atlas)
{
    DvzTextAtlasInfo info = {};
    if (atlas == NULL)
        return info;
    info.spec = atlas->spec;
    info.backend = atlas->backend;
    info.encoding = atlas->encoding;
    info.width = atlas->width;
    info.height = atlas->height;
    info.glyph_count = atlas->glyph_count;
    info.channels = atlas->channels;
    info.em_px = atlas->em_px;
    info.distance_range_px = atlas->distance_range_px;
    info.ascent = atlas->ascent;
    info.descent = atlas->descent;
    info.line_gap = atlas->line_gap;
    info.line_height = atlas->line_height;
    info.missing_glyph_count = atlas->missing_glyph_count;
    info.generation = atlas->generation;
    return info;
}


/**
 * Return the sampled field containing the atlas texture.
 *
 * @param atlas the text atlas
 * @return sampled atlas field, or NULL
 */
DvzSampledField* dvz_text_atlas_field(const DvzTextAtlas* atlas)
{
    if (atlas == NULL)
        return NULL;
    return atlas->field;
}


/**
 * Return one atlas glyph, falling back to '?' for unsupported codepoints.
 *
 * @param atlas the text atlas
 * @param codepoint Unicode codepoint
 * @return glyph metrics, or NULL when unavailable
 */
const DvzTextAtlasGlyph* dvz_text_atlas_glyph(const DvzTextAtlas* atlas, uint32_t codepoint)
{
    if (atlas == NULL)
        return NULL;
    if (!_text_atlas_codepoint_renderable(codepoint))
        codepoint = DVZ_TEXT_SDF_FALLBACK;

    for (uint32_t i = 0; i < atlas->glyph_count && i < DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS; i++)
    {
        const DvzTextAtlasGlyph* glyph = &atlas->glyphs[i];
        if (glyph->codepoint == codepoint && glyph->valid)
            return glyph;
    }
    if (codepoint == DVZ_TEXT_SDF_FALLBACK)
        return NULL;
    for (uint32_t i = 0; i < atlas->glyph_count && i < DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS; i++)
    {
        const DvzTextAtlasGlyph* glyph = &atlas->glyphs[i];
        if (glyph->codepoint == DVZ_TEXT_SDF_FALLBACK && glyph->valid)
            return glyph;
    }
    return NULL;
}


/**
 * Resolve a text atlas spec from a backend and rendered size.
 *
 * @param backend requested atlas backend
 * @param size_px rendered text size in pixels
 * @return atlas generation spec
 */
DvzTextAtlasSpec _scene_text_atlas_spec(DvzTextAtlasBackend backend, float size_px)
{
    DvzTextAtlasSpec spec = {};
    spec.backend = backend;
    if (backend == DVZ_TEXT_ATLAS_BACKEND_FREETYPE_BITMAP)
    {
        spec.em_px = _text_atlas_bitmap_em_px(size_px);
        return spec;
    }
    spec.em_px = _text_atlas_sdf_em_px(size_px);
    if (backend == DVZ_TEXT_ATLAS_BACKEND_MSDF)
        spec.distance_range_px = _text_atlas_msdf_range_px(spec.em_px);
    else if (backend == DVZ_TEXT_ATLAS_BACKEND_STB_SDF)
        spec.distance_range_px = _text_atlas_sdf_range_px(spec.em_px);
    return spec;
}


/**
 * Ensure a scene font has owned TrueType bytes available.
 *
 * @param font the font
 * @return whether TTF bytes are available
 */
bool _scene_font_ensure_bytes(DvzFont* font)
{
    ANN(font);
    return _text_sdf_font_bytes(font);
}



/**
 * Return a font atlas matching a requested spec, including SDF fallback atlases.
 *
 * @param font the scene font
 * @param spec requested atlas spec
 * @return atlas pointer, or NULL when unavailable
 */
DvzTextAtlas* _scene_text_atlas_get(DvzFont* font, const DvzTextAtlasSpec* spec)
{
    ANN(font);
    ANN(spec);
    DvzTextAtlas** slot = _text_atlas_find_slot(font, spec);
    if (slot != NULL)
        return *slot;
    if (spec->backend == DVZ_TEXT_ATLAS_BACKEND_STB_SDF)
        return NULL;
    DvzTextAtlasSpec fallback = _text_atlas_fallback_spec(spec);
    slot = _text_atlas_find_slot(font, &fallback);
    return slot != NULL ? *slot : NULL;
}



/**
 * Ensure one font has a scene-owned font atlas.
 *
 * @param font the scene font
 * @param spec requested atlas spec
 * @return whether the atlas is available
 */
bool _scene_text_atlas_ensure(DvzFont* font, const DvzTextAtlasSpec* spec)
{
    ANN(font);
    ANN(spec);
    DvzTextAtlasBuildSet set = {};
    if (!_text_atlas_build_set_add_default(&set))
        return false;
    return _text_atlas_ensure_set(font, spec, &set);
}



/**
 * Ensure one font has an atlas that covers one UTF-8 string.
 *
 * @param font the scene font
 * @param spec requested atlas spec
 * @param string the UTF-8 string
 * @return whether the atlas is available
 */
bool _scene_text_atlas_ensure_string(
    DvzFont* font, const DvzTextAtlasSpec* spec, const char* string)
{
    ANN(font);
    ANN(spec);
    const char* strings[1] = {string};
    return _scene_text_atlas_ensure_strings(font, spec, strings, 1);
}



/**
 * Ensure one font has an atlas that covers a list of UTF-8 strings.
 *
 * @param font the scene font
 * @param spec requested atlas spec
 * @param strings the UTF-8 strings
 * @param count string count
 * @return whether the atlas is available
 */
bool _scene_text_atlas_ensure_strings(
    DvzFont* font, const DvzTextAtlasSpec* spec, const char* const* strings, uint32_t count)
{
    ANN(font);
    ANN(spec);
    DvzTextAtlasBuildSet set = {};
    if (!_text_atlas_build_set_add_default(&set))
        return false;
    if (!_text_atlas_build_set_add_strings(&set, strings, count))
        log_debug("text atlas request exceeded glyph capacity; using the accepted subset");
    return _text_atlas_ensure_set(font, spec, &set);
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
    if (!_text_atlas_codepoint_renderable(codepoint))
        codepoint = DVZ_TEXT_SDF_FALLBACK;

    for (uint32_t i = 0; i < atlas->glyph_count && i < DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS; i++)
    {
        DvzTextAtlasGlyph* glyph = &atlas->glyphs[i];
        if (glyph->codepoint == codepoint && glyph->valid)
            return glyph;
    }
    if (codepoint == DVZ_TEXT_SDF_FALLBACK)
        return NULL;
    for (uint32_t i = 0; i < atlas->glyph_count && i < DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS; i++)
    {
        DvzTextAtlasGlyph* glyph = &atlas->glyphs[i];
        if (glyph->codepoint == DVZ_TEXT_SDF_FALLBACK && glyph->valid)
            return glyph;
    }
    return NULL;
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
    for (uint32_t i = 0; i < font->atlas_count && i < DVZ_SCENE_MAX_TEXT_ATLASES_PER_FONT; i++)
    {
        _scene_text_atlas_destroy(font->atlases[i]);
        font->atlases[i] = NULL;
    }
    font->atlas_count = 0;
    if (font->ttf_bytes != NULL)
    {
        dvz_free(font->ttf_bytes);
        font->ttf_bytes = NULL;
        font->ttf_size = 0;
    }
    font->scene = NULL;
}

}
