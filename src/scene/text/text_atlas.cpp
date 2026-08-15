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
#include <stdio.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_overflow.h"
#include "_scene.h"
#include "_time_utils.h"
#include "datoviz/fileio/fileio.h"
#include "datoviz/scene.h"
#include "text/text_atlas_product_internal.h"
#include "text/text_internal.h"

#if defined(DVZ_HAS_ZLIB) && DVZ_HAS_ZLIB
#include <zlib.h>
#endif

#if defined(DVZ_HAS_FREETYPE) && DVZ_HAS_FREETYPE
#include <ft2build.h>
#include FT_FREETYPE_H
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
static void* _text_sdf_load_builtin_font(const DvzFont* font, DvzSize* out_size);
static void* _text_sdf_load_scientific_fallback(DvzSize* out_size);
static bool _text_atlas_upload_rgba(
    const DvzFont* font, DvzTextAtlas* atlas, uint8_t* rgba, uint32_t width, uint32_t height);
static bool _text_atlas_changed_region(
    const DvzSampledField* field, const DvzSampledField* src, DvzFieldRegion* out_region);
static bool _text_atlas_replace_field_data(DvzSampledField* field, const DvzSampledField* src);
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


/**
 * Validate one atlas extent against the runtime CPU product budget.
 *
 * @param width atlas width
 * @param height atlas height
 * @param out_byte_size output RGBA byte size
 * @return whether the extent is within the configured budget
 */
static bool _text_atlas_extent_within_budget(
    uint64_t width, uint64_t height, uint64_t* out_byte_size)
{
    ANN(out_byte_size);
    *out_byte_size = 0;
    DvzTextAtlasProductBudget budget = _text_atlas_product_budget_default();
    if (width == 0 || height == 0 || width > budget.max_dimension ||
        height > budget.max_dimension)
    {
        return false;
    }
    uint64_t pixel_count = 0;
    uint64_t byte_size = 0;
    if (_dvz_mul_u64_overflows(width, height, &pixel_count) ||
        _dvz_mul_u64_overflows(pixel_count, 4u, &byte_size) || byte_size > SIZE_MAX ||
        byte_size > budget.max_rgba_bytes)
    {
        return false;
    }
    *out_byte_size = byte_size;
    return true;
}


#if defined(DVZ_HAS_MSDF_ATLAS) && DVZ_HAS_MSDF_ATLAS
/**
 * Copy a build set into canonical increasing codepoint order.
 *
 * @param set source build set
 * @param codepoints output codepoint array
 */
static void _text_msdf_canonical_codepoints(
    const DvzTextAtlasBuildSet* set, uint32_t* codepoints)
{
    ANN(set);
    ANN(codepoints);
    for (uint32_t i = 0; i < set->count; i++)
    {
        uint32_t codepoint = set->codepoints[i];
        uint32_t j = i;
        while (j > 0 && codepoints[j - 1] > codepoint)
        {
            codepoints[j] = codepoints[j - 1];
            j--;
        }
        codepoints[j] = codepoint;
    }
}



/**
 * Build an RGB MSDF atlas through the pure CPU product boundary.
 *
 * @param font the font
 * @param spec requested atlas specification
 * @param set requested codepoint set
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
    if (set->count == 0 || set->count > DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS ||
        font->face_index > INT32_MAX)
        return false;
    if (!_text_sdf_font_bytes(font))
        return false;

    uint64_t start_ns = dvz_time_monotonic_ns();
    log_debug(
        "text atlas: building runtime MSDF product em=%.3f range=%.3f glyphs=%u",
        (double)spec->em_px, (double)spec->distance_range_px, set->count);

    uint32_t codepoints[DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS] = {};
    _text_msdf_canonical_codepoints(set, codepoints);

    DvzSize fallback_size = 0;
    void* fallback_bytes = _text_sdf_load_scientific_fallback(&fallback_size);
    DvzTextAtlasFontView primary_view = {};
    primary_view.bytes = (const uint8_t*)font->ttf_bytes;
    primary_view.size = font->ttf_size;
    primary_view.face_index = (int32_t)font->face_index;
    DvzTextAtlasFontView fallback_view = {};
    fallback_view.bytes = (const uint8_t*)fallback_bytes;
    fallback_view.size = fallback_size;
    const DvzTextAtlasFontView* fallback =
        fallback_bytes != NULL && fallback_size > 0 ? &fallback_view : NULL;

    DvzTextAtlasProductBudget budget = _text_atlas_product_budget_default();
    DvzTextAtlasProductParams params = _text_atlas_product_params_default();
    DvzTextAtlasProduct product = {};
    bool built = _text_atlas_product_build_msdf(
        &primary_view, fallback, spec, codepoints, set->count, &budget, &params, &product);
    dvz_free(fallback_bytes);
    if (!built)
    {
        log_error("failed to build bounded runtime MSDF product");
        return false;
    }
    if (product.coverage_count > DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS)
    {
        _text_atlas_product_destroy(&product);
        return false;
    }

    DvzTextAtlas* atlas = (DvzTextAtlas*)dvz_calloc(1, sizeof(DvzTextAtlas));
    if (atlas == NULL)
    {
        _text_atlas_product_destroy(&product);
        return false;
    }
    atlas->spec = product.spec;
    atlas->backend = product.backend;
    atlas->encoding = product.encoding;
    atlas->width = product.width;
    atlas->height = product.height;
    atlas->glyph_count = product.coverage_count;
    atlas->channels = product.channels;
    atlas->em_px = product.em_px;
    atlas->distance_range_px = product.distance_range_px;
    atlas->ascent = product.ascent;
    atlas->descent = product.descent;
    atlas->line_gap = product.line_gap;
    atlas->line_height = product.line_height;
    atlas->missing_glyph_count = product.fallback_mapping_count;

    for (uint32_t i = 0; i < product.coverage_count; i++)
    {
        const DvzTextAtlasProductCoverage* coverage = &product.coverage[i];
        DvzTextAtlasGlyph* glyph = &atlas->glyphs[i];
        glyph->codepoint = coverage->requested_codepoint;
        if (coverage->kind == DVZ_TEXT_ATLAS_PRODUCT_COVERAGE_EXACT)
        {
            *glyph = product.glyphs[coverage->glyph_index];
            glyph->codepoint = coverage->requested_codepoint;
        }
    }

    uint64_t rgba_size = product.rgba_size;
    bool uploaded = _text_atlas_upload_rgba(
        font, atlas, product.rgba, product.width, product.height);
    _text_atlas_product_destroy(&product);
    if (!uploaded)
    {
        _scene_text_atlas_destroy(atlas);
        return false;
    }

    *out_atlas = atlas;
    log_debug(
        "text atlas: runtime MSDF product ready em=%.3f range=%.3f glyphs=%u size=%ux%u "
        "rgba=%llu in %.3f ms",
        (double)atlas->em_px, (double)atlas->distance_range_px, atlas->glyph_count,
        atlas->width, atlas->height, (unsigned long long)rgba_size,
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
 * Load one admitted built-in font from embedded resources or the source tree.
 *
 * @param font requested scene font identity
 * @param out_size output byte size
 * @return owned TTF bytes, or NULL when the built-in role is unavailable
 */
static void* _text_sdf_load_builtin_font(const DvzFont* font, DvzSize* out_size)
{
    ANN(font);
    ANN(out_size);
    const char* resource = NULL;
    const char* path = NULL;
    switch (font->source_id)
    {
    case DVZ_FONT_SOURCE_SOURCE_SANS_3_BOLD:
        resource = "SourceSans3_Bold";
        path = "assets/runtime/fonts/SourceSans3-Bold.ttf";
        break;
    case DVZ_FONT_SOURCE_SOURCE_SANS_3_ITALIC:
        resource = "SourceSans3_It";
        path = "assets/runtime/fonts/SourceSans3-It.ttf";
        break;
    case DVZ_FONT_SOURCE_SOURCE_SANS_3_BOLD_ITALIC:
        resource = "SourceSans3_BoldIt";
        path = "assets/runtime/fonts/SourceSans3-BoldIt.ttf";
        break;
    case DVZ_FONT_SOURCE_SOURCE_CODE_PRO_REGULAR:
        resource = "SourceCodePro_Regular";
        path = "assets/runtime/fonts/SourceCodePro-Regular.ttf";
        break;
    case DVZ_FONT_SOURCE_NOTO_SANS_MATH_REGULAR:
        resource = "NotoSansMath_Regular";
        path = "assets/runtime/fonts/NotoSansMath-Regular.ttf";
        break;
    case DVZ_FONT_SOURCE_SOURCE_SANS_3_REGULAR:
    case DVZ_FONT_SOURCE_NONE:
    default:
        resource = "SourceSans3_Regular";
        path = "assets/runtime/fonts/SourceSans3-Regular.ttf";
        break;
    }

#if defined(DVZ_HAS_EMBEDDED_FONTS) && DVZ_HAS_EMBEDDED_FONTS
    DvzSize embedded_size = 0;
    const unsigned char* embedded = dvz_resource_font(resource, &embedded_size);
    if (embedded != NULL && embedded_size > 0)
    {
        void* bytes = dvz_malloc(embedded_size);
        if (bytes != NULL)
        {
            dvz_memcpy(bytes, (size_t)embedded_size, embedded, (size_t)embedded_size);
            *out_size = embedded_size;
            return bytes;
        }
    }
#endif
    DvzSize size = 0;
    void* bytes = dvz_read_file(path, &size);
    if (bytes == NULL || size == 0)
    {
        dvz_free(bytes);
        return NULL;
    }
    *out_size = size;
    return bytes;
}


/**
 * Load the built-in scientific fallback font.
 *
 * @param out_size output byte size
 * @return owned TTF bytes, or NULL when the fallback is unavailable
 */
static void* _text_sdf_load_scientific_fallback(DvzSize* out_size)
{
    ANN(out_size);
    DvzFont fallback = {};
    snprintf(fallback.family, sizeof(fallback.family), "%s", "Noto Sans Math");
    snprintf(fallback.style, sizeof(fallback.style), "%s", "Regular");
    fallback.source_id = DVZ_FONT_SOURCE_NOTO_SANS_MATH_REGULAR;
    return _text_sdf_load_builtin_font(&fallback, out_size);
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
        bytes = _text_sdf_load_builtin_font(font, &size);

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
 * Return whether one font matches the baked Source Sans 3 Regular atlas.
 *
 * @param font the scene font
 * @return whether embedded default atlas data may be used
 */
static bool _text_default_msdf_font_matches(const DvzFont* font)
{
    ANN(font);
    return font->source_id == DVZ_FONT_SOURCE_SOURCE_SANS_3_REGULAR && font->path[0] == '\0' &&
           font->face_index == 0;
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
    if (!_text_default_msdf_font_matches(font))
    {
        log_trace(
            "text atlas: embedded MSDF skipped reason=source_mismatch source=%d",
            (int)font->source_id);
        return false;
    }
    if (!_text_default_msdf_spec_index(spec, &index))
    {
        log_trace(
            "text atlas: embedded MSDF skipped reason=spec_mismatch backend=%d em=%.3f "
            "range=%.3f",
            (int)spec->backend, (double)spec->em_px, (double)spec->distance_range_px);
        return false;
    }
    if (!_text_default_msdf_covers_set(set))
    {
        log_debug(
            "text atlas: path=runtime reason=request_not_embedded requested_glyphs=%u", set->count);
        return false;
    }

    const DvzTextDefaultMsdfAtlasData* data = &DVZ_TEXT_DEFAULT_MSDF_ATLASES[index];
    uint64_t start_ns = dvz_time_monotonic_ns();
    uint64_t expected_rgba_size = 0;
    if (data->glyph_count == 0 || data->glyph_count > DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS ||
        !_text_atlas_extent_within_budget(data->width, data->height, &expected_rgba_size) ||
        expected_rgba_size != data->rgba_size)
    {
        log_error("embedded MSDF atlas metadata exceeds the runtime product budget");
        return false;
    }

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
 * Find an existing atlas cache entry for one requested spec.
 *
 * @param font the font
 * @param spec atlas spec
 * @return matching font-owned atlas cache entry, or NULL
 */
static DvzTextAtlasCacheEntry* _text_atlas_find_entry(
    DvzFont* font, const DvzTextAtlasSpec* spec)
{
    ANN(font);
    ANN(spec);
    for (uint32_t i = 0; i < font->atlas_count && i < DVZ_SCENE_MAX_TEXT_ATLASES_PER_FONT; i++)
    {
        DvzTextAtlasCacheEntry* entry = &font->atlas_entries[i];
        if (entry->active && _text_atlas_spec_equal(&entry->request_spec, spec))
            return entry;
    }
    return NULL;
}



/**
 * Find an existing realized atlas for one requested spec without mutating the font.
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
        const DvzTextAtlasCacheEntry* entry = &font->atlas_entries[i];
        if (entry->active && _text_atlas_spec_equal(&entry->request_spec, spec))
            return entry->atlas;
    }
    return NULL;
}



/**
 * Find or allocate an atlas cache entry for one requested spec.
 *
 * @param font the font
 * @param spec atlas spec
 * @return font-owned atlas cache entry, or NULL when the cache is full
 */
static DvzTextAtlasCacheEntry* _text_atlas_acquire_entry(
    DvzFont* font, const DvzTextAtlasSpec* spec)
{
    ANN(font);
    ANN(spec);
    DvzTextAtlasCacheEntry* entry = _text_atlas_find_entry(font, spec);
    if (entry != NULL)
        return entry;
    if (font->atlas_count >= DVZ_SCENE_MAX_TEXT_ATLASES_PER_FONT)
    {
        log_error("text atlas cache is full");
        return NULL;
    }
    entry = &font->atlas_entries[font->atlas_count++];
    dvz_memset(entry, sizeof(DvzTextAtlasCacheEntry), 0, sizeof(DvzTextAtlasCacheEntry));
    entry->request_spec = *spec;
    entry->active = true;
    return entry;
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
    if (field == NULL || dvz_sampled_field_set_data(field, &view) != DVZ_OK)
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
    if (field->desc.format != src->desc.format || field->desc.dim != src->desc.dim ||
        field->desc.depth != src->desc.depth)
    {
        return false;
    }
    if (field->desc.width != src->desc.width || field->desc.height != src->desc.height)
    {
        DvzFieldDataView view = dvz_field_data_view();
        view.data = src->data;
        view.bytes_per_row = (uint64_t)src->desc.width * 4u;
        view.rows_per_image = src->desc.height;
        return dvz_sampled_field_resize(
                   field, src->desc.width, src->desc.height, src->desc.depth, &view) == DVZ_OK;
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
        return dvz_sampled_field_update_region(field, region, &region_view) == DVZ_OK;
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
    return dvz_sampled_field_set_data(field, &view) == DVZ_OK;
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

    DvzSize fallback_size = 0;
    void* fallback_bytes = _text_sdf_load_scientific_fallback(&fallback_size);
    FT_Face fallback_face = NULL;
    if (fallback_bytes != NULL && fallback_size > 0)
    {
        if (FT_New_Memory_Face(
                library, (const FT_Byte*)fallback_bytes, (FT_Long)fallback_size, 0,
                &fallback_face) != 0)
            fallback_face = NULL;
    }
    auto cleanup_faces = [&]() {
        if (fallback_face != NULL)
            FT_Done_Face(fallback_face);
        FT_Done_Face(face);
        dvz_free(fallback_bytes);
        FT_Done_FreeType(library);
    };

    float em_px = spec->em_px > 0.0f ? spec->em_px : DVZ_TEXT_ATLAS_DEFAULT_EM_PX;
    if (FT_Set_Pixel_Sizes(face, 0, (FT_UInt)em_px) != 0)
    {
        log_error("failed to set FreeType pixel size");
        cleanup_faces();
        return false;
    }
    if (fallback_face != NULL && FT_Set_Pixel_Sizes(fallback_face, 0, (FT_UInt)em_px) != 0)
    {
        FT_Done_Face(fallback_face);
        fallback_face = NULL;
    }

    const uint32_t glyph_count = set->count;
    uint32_t glyph_widths[DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS] = {};
    uint32_t glyph_heights[DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS] = {};
    int glyph_lefts[DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS] = {};
    int glyph_tops[DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS] = {};
    float glyph_advances[DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS] = {};
    bool glyph_fallbacks[DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS] = {};
    uint32_t cell_width = 1;
    uint32_t cell_height = 1;

    for (uint32_t i = 0; i < glyph_count; i++)
    {
        uint32_t codepoint = set->codepoints[i];
        FT_Face selected = face;
        if (
            FT_Get_Char_Index(face, (FT_ULong)codepoint) == 0 && fallback_face != NULL &&
            FT_Get_Char_Index(fallback_face, (FT_ULong)codepoint) != 0)
        {
            selected = fallback_face;
            glyph_fallbacks[i] = true;
        }
        if (FT_Load_Char(selected, (FT_ULong)codepoint, FT_LOAD_DEFAULT) != 0 ||
            FT_Render_Glyph(selected->glyph, FT_RENDER_MODE_NORMAL) != 0)
            continue;
        glyph_widths[i] = (uint32_t)selected->glyph->bitmap.width;
        glyph_heights[i] = (uint32_t)selected->glyph->bitmap.rows;
        glyph_lefts[i] = selected->glyph->bitmap_left;
        glyph_tops[i] = selected->glyph->bitmap_top;
        glyph_advances[i] = (float)selected->glyph->advance.x / 64.0f;
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
    DvzTextAtlasProductBudget budget = _text_atlas_product_budget_default();
    if (_dvz_mul_u64_overflows(DVZ_TEXT_SDF_COLUMNS, cell_width, &width64) ||
        _dvz_mul_u64_overflows(rows, cell_height, &height64) ||
        _dvz_mul_u64_overflows(width64, height64, &pixel_count) ||
        _dvz_mul_u64_overflows(pixel_count, 4u, &byte_size) || width64 > UINT32_MAX ||
        height64 > UINT32_MAX || width64 > budget.max_dimension ||
        height64 > budget.max_dimension || byte_size > SIZE_MAX ||
        byte_size > budget.max_rgba_bytes)
    {
        log_error("text FreeType atlas dimensions overflow");
        cleanup_faces();
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
        cleanup_faces();
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
        FT_Face selected = glyph_fallbacks[i] ? fallback_face : face;
        glyph->glyph_id = (uint32_t)FT_Get_Char_Index(selected, (FT_ULong)codepoint);
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
        if (FT_Load_Char(selected, (FT_ULong)codepoint, FT_LOAD_DEFAULT) != 0 ||
            FT_Render_Glyph(selected->glyph, FT_RENDER_MODE_NORMAL) != 0)
            continue;
        _text_ft_copy_bitmap(rgba, atlas_width, x, y, &selected->glyph->bitmap);
    }
    for (uint32_t i = 0; i < glyph_count; i++)
    {
        if (!atlas->glyphs[i].valid && atlas->glyphs[i].codepoint != DVZ_TEXT_SDF_FALLBACK)
            atlas->missing_glyph_count++;
    }

    bool ok = _text_atlas_upload_rgba(font, atlas, rgba, atlas_width, atlas_height);
    dvz_free(rgba);
    cleanup_faces();
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

    DvzSize fallback_size = 0;
    void* fallback_bytes = _text_sdf_load_scientific_fallback(&fallback_size);
    stbtt_fontinfo fallback_info = {};
    bool fallback_available = false;
    if (fallback_bytes != NULL && fallback_size > 0 && fallback_size <= INT32_MAX)
    {
        const unsigned char* bytes = (const unsigned char*)fallback_bytes;
        int offset = stbtt_GetFontOffsetForIndex(bytes, 0);
        fallback_available = offset >= 0 && stbtt_InitFont(&fallback_info, bytes, offset) != 0;
    }

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
    float glyph_scales[DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS] = {};
    bool glyph_fallbacks[DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS] = {};
    uint32_t cell_width = 1;
    uint32_t cell_height = 1;

    for (uint32_t i = 0; i < glyph_count; i++)
    {
        int width = 0;
        int height = 0;
        int xoff = 0;
        int yoff = 0;
        uint32_t codepoint = set->codepoints[i];
        stbtt_fontinfo* selected = &info;
        float selected_scale = scale;
        if (
            stbtt_FindGlyphIndex(&info, (int)codepoint) == 0 && fallback_available &&
            stbtt_FindGlyphIndex(&fallback_info, (int)codepoint) != 0)
        {
            selected = &fallback_info;
            selected_scale = stbtt_ScaleForPixelHeight(&fallback_info, em_px);
            glyph_fallbacks[i] = true;
        }
        unsigned char* sdf = stbtt_GetCodepointSDF(
            selected, selected_scale, (int)codepoint, padding,
            (unsigned char)DVZ_TEXT_SDF_ONEDGE,
            _text_sdf_pixel_dist_scale(distance_range_px), &width, &height, &xoff, &yoff);
        glyph_scales[i] = selected_scale;
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
    DvzTextAtlasProductBudget budget = _text_atlas_product_budget_default();
    if (_dvz_mul_u64_overflows(DVZ_TEXT_SDF_COLUMNS, cell_width, &width64) ||
        _dvz_mul_u64_overflows(rows, cell_height, &height64) ||
        _dvz_mul_u64_overflows(width64, height64, &pixel_count) ||
        _dvz_mul_u64_overflows(pixel_count, 4u, &byte_size) || width64 > UINT32_MAX ||
        height64 > UINT32_MAX || width64 > budget.max_dimension ||
        height64 > budget.max_dimension || byte_size > SIZE_MAX ||
        byte_size > budget.max_rgba_bytes)
    {
        log_error("text SDF atlas dimensions overflow");
        for (uint32_t i = 0; i < glyph_count; i++)
            if (glyph_sdfs[i] != NULL)
                stbtt_FreeSDF(glyph_sdfs[i], NULL);
        dvz_free(fallback_bytes);
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
        dvz_free(fallback_bytes);
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
        stbtt_fontinfo* selected = glyph_fallbacks[i] ? &fallback_info : &info;
        float selected_scale = glyph_scales[i] > 0.0f ? glyph_scales[i] : scale;
        glyph->glyph_id = (uint32_t)stbtt_FindGlyphIndex(selected, (int)codepoint);
        if (glyph->glyph_id == 0 && codepoint != DVZ_TEXT_SDF_FALLBACK)
        {
            atlas->missing_glyph_count++;
            continue;
        }
        int advance = 0;
        int left_bearing = 0;
        stbtt_GetCodepointHMetrics(selected, (int)codepoint, &advance, &left_bearing);
        (void)left_bearing;
        glyph->advance = (float)advance * selected_scale;
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
    dvz_free(fallback_bytes);

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
    DvzTextAtlasCacheEntry* entry = _text_atlas_acquire_entry(font, spec);
    if (entry == NULL)
        return false;
    if (entry->atlas == NULL)
    {
        font->version++;
        atlas->generation = font->version;
        entry->atlas = atlas;
        return true;
    }

    DvzTextAtlas* realized = entry->atlas;
    if (realized->field == NULL || atlas->field == NULL ||
        !_text_atlas_replace_field_data(realized->field, atlas->field))
    {
        return false;
    }

    DvzSampledField* stable_field = realized->field;
    DvzSampledField* temporary_field = atlas->field;
    uint64_t generation = font->version + 1u;
    *realized = *atlas;
    realized->field = stable_field;
    realized->generation = generation;
    atlas->field = temporary_field;
    _scene_text_atlas_destroy(atlas);
    font->version++;
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
    DvzTextAtlasCacheEntry* entry = _text_atlas_find_entry(font, spec);
    DvzTextAtlas* existing = entry != NULL ? entry->atlas : NULL;
    if (!_text_atlas_build_set_add_existing(&set, existing))
        return false;
    if (_text_atlas_contains_set(existing, &set))
    {
        log_trace(
            "text atlas: cache hit backend=%d em=%.3f range=%.3f glyphs=%u",
            (int)spec->backend, (double)spec->em_px, (double)spec->distance_range_px,
            set.count);
        return true;
    }
    DvzTextAtlas* atlas = NULL;
    if (_text_default_msdf_build_atlas(font, spec, &set, &atlas))
    {
        if (_text_atlas_store(font, spec, atlas))
            return true;
        _scene_text_atlas_destroy(atlas);
        return false;
    }

    atlas = NULL;
    log_debug(
        "text atlas: building backend atlas backend=%d em=%.3f range=%.3f glyphs=%u",
        (int)spec->backend, (double)spec->em_px, (double)spec->distance_range_px, set.count);
    if (_text_atlas_build_backend(font, spec, &set, &atlas))
    {
        if (_text_atlas_store(font, spec, atlas))
            return true;
        _scene_text_atlas_destroy(atlas);
        return false;
    }

    if (spec->backend == DVZ_TEXT_ATLAS_BACKEND_STB_SDF)
        return false;
    if (spec->backend == DVZ_TEXT_ATLAS_BACKEND_MSDF)
        log_debug("MSDF atlas generation failed; falling back to SDF atlas");
    if (spec->backend == DVZ_TEXT_ATLAS_BACKEND_FREETYPE_BITMAP)
        log_debug("FreeType bitmap atlas generation failed; falling back to SDF atlas");
    DvzTextAtlasSpec fallback = _text_atlas_fallback_spec(spec);
    atlas = NULL;
    if (!_text_atlas_build_backend(font, &fallback, &set, &atlas))
        return false;
    if (!_text_atlas_store(font, spec, atlas))
    {
        _scene_text_atlas_destroy(atlas);
        return false;
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
    return _text_atlas_ensure_exact_set(font, spec, requested_set);
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
    return _text_atlas_find_const(font, spec);
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
const DvzSampledField* dvz_text_atlas_field(const DvzTextAtlas* atlas)
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
    DvzTextAtlasCacheEntry* entry = _text_atlas_find_entry(font, spec);
    return entry != NULL ? entry->atlas : NULL;
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
        return false;
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
        DvzTextAtlasCacheEntry* entry = &font->atlas_entries[i];
        _scene_text_atlas_destroy(entry->atlas);
        dvz_memset(entry, sizeof(DvzTextAtlasCacheEntry), 0, sizeof(DvzTextAtlasCacheEntry));
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
