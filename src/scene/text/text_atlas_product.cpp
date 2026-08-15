/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  CPU text atlas products                                                                      */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <vector>

#include "_alloc.h"
#include "_overflow.h"
#include "text_atlas_product_internal.h"

#if defined(DVZ_HAS_MSDF_ATLAS) && DVZ_HAS_MSDF_ATLAS
#include <ft2build.h>
#include FT_FREETYPE_H
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



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_TEXT_ATLAS_PRODUCT_CHANNELS 4u
#define DVZ_TEXT_ATLAS_PRODUCT_FIRST_CODEPOINT 32u
#define DVZ_TEXT_ATLAS_PRODUCT_DEFAULT_MAX_GLYPHS 256u
#define DVZ_TEXT_ATLAS_PRODUCT_DEFAULT_MAX_DIMENSION 4096u
#define DVZ_TEXT_ATLAS_PRODUCT_DEFAULT_MAX_RGBA_BYTES (64ull * 1024ull * 1024ull)



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return whether a codepoint is a supported visible Unicode scalar value.
 *
 * @param codepoint Unicode codepoint
 * @return whether the codepoint may enter an atlas product
 */
static bool _text_atlas_product_codepoint_valid(uint32_t codepoint)
{
    if (codepoint < DVZ_TEXT_ATLAS_PRODUCT_FIRST_CODEPOINT || codepoint > 0x10FFFFu)
        return false;
    return codepoint < 0xD800u || codepoint > 0xDFFFu;
}



/**
 * Return whether a floating-point value is finite and positive.
 *
 * @param value floating-point value
 * @return whether the value is finite and positive
 */
static bool _text_atlas_product_positive(double value)
{
    return isfinite(value) && value > 0.0;
}



/**
 * Return whether a floating-point value is finite.
 *
 * @param value floating-point value
 * @return whether the value is finite
 */
static bool _text_atlas_product_finite(float value)
{
    return isfinite((double)value);
}



/**
 * Return whether a font view is present and structurally valid.
 *
 * @param view borrowed font view
 * @param optional whether an absent view is accepted
 * @return whether the view is valid
 */
static bool _text_atlas_product_font_view_valid(
    const DvzTextAtlasFontView* view, bool optional)
{
    if (view == NULL)
        return optional;
    if (view->bytes == NULL || view->size == 0 || view->size > (uint64_t)LONG_MAX)
        return false;
    if (view->face_index < 0 || view->load_flags != 0)
        return false;
    return true;
}



/**
 * Return whether the generation request is structurally valid and bounded.
 *
 * @param primary borrowed primary font view
 * @param fallback optional borrowed fallback font view
 * @param spec atlas specification
 * @param codepoints canonical codepoints
 * @param codepoint_count codepoint count
 * @param budget hard product budget
 * @param params generation parameters
 * @param out_product zero-initialized output
 * @return whether the request is valid
 */
static bool _text_atlas_product_request_valid(
    const DvzTextAtlasFontView* primary, const DvzTextAtlasFontView* fallback,
    const DvzTextAtlasSpec* spec, const uint32_t* codepoints, uint32_t codepoint_count,
    const DvzTextAtlasProductBudget* budget, const DvzTextAtlasProductParams* params,
    const DvzTextAtlasProduct* out_product)
{
    if (!_text_atlas_product_font_view_valid(primary, false) ||
        !_text_atlas_product_font_view_valid(fallback, true))
        return false;
    if (spec == NULL || spec->backend != DVZ_TEXT_ATLAS_BACKEND_MSDF ||
        !_text_atlas_product_positive((double)spec->em_px) ||
        !_text_atlas_product_positive((double)spec->distance_range_px))
        return false;
    if (codepoints == NULL || codepoint_count == 0 || budget == NULL || params == NULL ||
        out_product == NULL)
        return false;
    if (out_product->rgba != NULL || out_product->glyphs != NULL || out_product->coverage != NULL)
        return false;
    if (budget->max_glyphs == 0 || budget->max_dimension == 0 ||
        budget->max_rgba_bytes == 0 || codepoint_count > budget->max_glyphs ||
        codepoint_count > (uint32_t)INT_MAX)
        return false;
    if (params->thread_count == 0 || params->thread_count > (uint32_t)INT_MAX ||
        !_text_atlas_product_codepoint_valid(params->fallback_codepoint) ||
        !_text_atlas_product_positive(params->max_corner_angle) ||
        !_text_atlas_product_positive(params->miter_limit))
        return false;
    for (uint32_t i = 0; i < codepoint_count; i++)
    {
        if (!_text_atlas_product_codepoint_valid(codepoints[i]))
            return false;
        if (i > 0 && codepoints[i - 1] >= codepoints[i])
            return false;
    }
    return true;
}



/**
 * Return the glyph-array index for a codepoint.
 *
 * @param glyphs glyph array
 * @param glyph_count glyph count
 * @param codepoint resolved codepoint
 * @return glyph index or UINT32_MAX when absent
 */
static uint32_t _text_atlas_product_find_glyph(
    const DvzTextAtlasGlyph* glyphs, uint32_t glyph_count, uint32_t codepoint)
{
    for (uint32_t i = 0; i < glyph_count; i++)
        if (glyphs[i].codepoint == codepoint)
            return i;
    return UINT32_MAX;
}



#if defined(DVZ_HAS_MSDF_ATLAS) && DVZ_HAS_MSDF_ATLAS

typedef struct DvzTextAtlasProductLoadedFont DvzTextAtlasProductLoadedFont;
struct DvzTextAtlasProductLoadedFont
{
    FT_Face face;
    msdfgen::FontHandle* handle;
};


/**
 * Own adopted FreeType faces for the duration of one product build.
 */
class DvzTextAtlasProductFontGuard
{
public:
    FT_Library library = NULL;
    DvzTextAtlasProductLoadedFont primary = {};
    DvzTextAtlasProductLoadedFont fallback = {};

    /** Destroy every owned face and the FreeType library. */
    ~DvzTextAtlasProductFontGuard()
    {
        _destroy_font(&fallback);
        _destroy_font(&primary);
        if (library != NULL)
            FT_Done_FreeType(library);
    }

    /**
     * Initialize the FreeType library.
     *
     * @return whether initialization succeeded
     */
    bool init()
    {
        return FT_Init_FreeType(&library) == 0;
    }

    /**
     * Load and adopt a borrowed memory face.
     *
     * @param view borrowed font bytes and face metadata
     * @param loaded destination face holder
     * @return whether loading and adoption succeeded
     */
    bool load(
        const DvzTextAtlasFontView* view, DvzTextAtlasProductLoadedFont* loaded)
    {
        if (view == NULL)
            return true;
        FT_Error error = FT_New_Memory_Face(
            library, (const FT_Byte*)view->bytes, (FT_Long)view->size,
            (FT_Long)view->face_index, &loaded->face);
        if (error != 0 || loaded->face == NULL)
            return false;
        loaded->handle = msdfgen::adoptFreetypeFont(loaded->face);
        if (loaded->handle == NULL)
        {
            FT_Done_Face(loaded->face);
            loaded->face = NULL;
            return false;
        }
        return true;
    }

private:
    /**
     * Destroy one adopted font handle and its owned FreeType face.
     *
     * @param loaded loaded face holder
     */
    static void _destroy_font(DvzTextAtlasProductLoadedFont* loaded)
    {
        if (loaded->handle != NULL)
            msdfgen::destroyFont(loaded->handle);
        if (loaded->face != NULL)
            FT_Done_Face(loaded->face);
        *loaded = {};
    }
};



/**
 * Resolve requested codepoints into exact glyphs or explicit fallback mappings.
 *
 * @param primary primary font handle
 * @param fallback optional fallback font handle
 * @param codepoints canonical codepoints
 * @param codepoint_count codepoint count
 * @param fallback_codepoint visible fallback codepoint
 * @param coverage output coverage array
 * @param primary_charset output primary charset
 * @param fallback_charset output fallback charset
 * @param fallback_mapping_count output number of fallback mappings
 * @return whether strict resolution succeeded
 */
static bool _text_atlas_product_resolve(
    msdfgen::FontHandle* primary, msdfgen::FontHandle* fallback, const uint32_t* codepoints,
    uint32_t codepoint_count, uint32_t fallback_codepoint,
    DvzTextAtlasProductCoverage* coverage, msdf_atlas::Charset* primary_charset,
    msdf_atlas::Charset* fallback_charset, uint32_t* fallback_mapping_count)
{
    msdfgen::GlyphIndex fallback_index;
    DvzTextAtlasProductFontRole replacement_role = DVZ_TEXT_ATLAS_PRODUCT_FONT_PRIMARY;
    bool replacement_available =
        msdfgen::getGlyphIndex(fallback_index, primary, fallback_codepoint);
    if (!replacement_available && fallback != NULL)
    {
        replacement_available =
            msdfgen::getGlyphIndex(fallback_index, fallback, fallback_codepoint);
        replacement_role = DVZ_TEXT_ATLAS_PRODUCT_FONT_FALLBACK;
    }
    *fallback_mapping_count = 0;
    for (uint32_t i = 0; i < codepoint_count; i++)
    {
        uint32_t codepoint = codepoints[i];
        DvzTextAtlasProductCoverage* item = &coverage[i];
        item->requested_codepoint = codepoint;
        item->resolved_codepoint = codepoint;
        item->glyph_index = UINT32_MAX;
        item->kind = DVZ_TEXT_ATLAS_PRODUCT_COVERAGE_EXACT;
        item->font_role = DVZ_TEXT_ATLAS_PRODUCT_FONT_PRIMARY;

        msdfgen::GlyphIndex glyph_index;
        if (msdfgen::getGlyphIndex(glyph_index, primary, codepoint))
        {
            primary_charset->add(codepoint);
            continue;
        }
        if (fallback != NULL && msdfgen::getGlyphIndex(glyph_index, fallback, codepoint))
        {
            fallback_charset->add(codepoint);
            item->font_role = DVZ_TEXT_ATLAS_PRODUCT_FONT_FALLBACK;
            continue;
        }
        if (!replacement_available)
            return false;

        item->resolved_codepoint = fallback_codepoint;
        item->kind = DVZ_TEXT_ATLAS_PRODUCT_COVERAGE_FALLBACK;
        item->font_role = replacement_role;
        (*fallback_mapping_count)++;
        if (replacement_role == DVZ_TEXT_ATLAS_PRODUCT_FONT_PRIMARY)
            primary_charset->add(fallback_codepoint);
        else
            fallback_charset->add(fallback_codepoint);
    }
    return true;
}



/**
 * Fill one public glyph record from an msdf-atlas-gen glyph.
 *
 * @param src source glyph geometry
 * @param scale realized atlas scale
 * @param atlas_width atlas width
 * @param atlas_height atlas height
 * @param glyph output glyph record
 * @return whether the glyph record is valid
 */
static bool _text_atlas_product_fill_glyph(
    const msdf_atlas::GlyphGeometry* src, double scale, uint32_t atlas_width,
    uint32_t atlas_height, DvzTextAtlasGlyph* glyph)
{
    glyph->codepoint = (uint32_t)src->getCodepoint();
    glyph->glyph_id = (uint32_t)src->getIndex();
    if (glyph->glyph_id == 0)
        return false;
    glyph->advance = (float)(src->getAdvance() * scale);

    double left = 0.0;
    double bottom = 0.0;
    double right = 0.0;
    double top = 0.0;
    src->getQuadPlaneBounds(left, bottom, right, top);

    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    src->getBoxRect(x, y, width, height);
    if (width <= 0 || height <= 0)
    {
        glyph->valid = glyph->advance > 0.0f;
        return glyph->valid;
    }
    if (x < 0 || y < 0 || (uint32_t)x > atlas_width || (uint32_t)y > atlas_height ||
        (uint32_t)width > atlas_width - (uint32_t)x ||
        (uint32_t)height > atlas_height - (uint32_t)y)
        return false;

    const double inset = 0.5;
    double plane_left = left;
    double plane_bottom = bottom;
    double plane_right = right;
    double plane_top = top;
    if (width > 2 && height > 2)
    {
        double fx = inset / (double)width;
        double fy = inset / (double)height;
        double plane_width = right - left;
        double plane_height = top - bottom;
        plane_left += fx * plane_width;
        plane_right -= fx * plane_width;
        plane_bottom += fy * plane_height;
        plane_top -= fy * plane_height;
    }

    glyph->xoff = (float)(plane_left * scale);
    glyph->yoff = (float)(-plane_top * scale);
    glyph->width = (float)((plane_right - plane_left) * scale);
    glyph->height = (float)((plane_top - plane_bottom) * scale);
    glyph->plane_bounds[0] = glyph->xoff;
    glyph->plane_bounds[1] = glyph->yoff;
    glyph->plane_bounds[2] = glyph->xoff + glyph->width;
    glyph->plane_bounds[3] = glyph->yoff + glyph->height;

    uint32_t top_y = atlas_height - (uint32_t)y - (uint32_t)height;
    glyph->atlas_bounds[0] = (float)x;
    glyph->atlas_bounds[1] = (float)top_y;
    glyph->atlas_bounds[2] = (float)(x + width);
    glyph->atlas_bounds[3] = (float)(top_y + (uint32_t)height);
    float padding_x = width > 2 ? (float)inset : 0.0f;
    float padding_y = height > 2 ? (float)inset : 0.0f;
    glyph->uv[0] = ((float)x + padding_x) / (float)atlas_width;
    glyph->uv[1] = ((float)top_y + padding_y) / (float)atlas_height;
    glyph->uv[2] = ((float)(x + width) - padding_x) / (float)atlas_width;
    glyph->uv[3] = ((float)(top_y + (uint32_t)height) - padding_y) / (float)atlas_height;
    glyph->valid = true;
    return true;
}



/**
 * Build one staged CPU MSDF product.
 *
 * @param primary borrowed primary font view
 * @param fallback optional borrowed fallback font view
 * @param spec atlas specification
 * @param codepoints canonical codepoints
 * @param codepoint_count codepoint count
 * @param budget hard product budget
 * @param params generation parameters
 * @param product staged output product
 * @return whether generation succeeded
 */
static bool _text_atlas_product_build_enabled(
    const DvzTextAtlasFontView* primary, const DvzTextAtlasFontView* fallback,
    const DvzTextAtlasSpec* spec, const uint32_t* codepoints, uint32_t codepoint_count,
    const DvzTextAtlasProductBudget* budget, const DvzTextAtlasProductParams* params,
    DvzTextAtlasProduct* product)
{
    DvzTextAtlasProductFontGuard fonts;
    if (!fonts.init() || !fonts.load(primary, &fonts.primary) ||
        !fonts.load(fallback, &fonts.fallback))
        return false;

    uint64_t coverage_size = 0;
    if (_dvz_mul_u64_overflows(
            (uint64_t)codepoint_count, sizeof(DvzTextAtlasProductCoverage), &coverage_size) ||
        coverage_size > SIZE_MAX)
        return false;
    product->coverage = (DvzTextAtlasProductCoverage*)dvz_calloc(1, coverage_size);
    if (product->coverage == NULL)
        return false;

    msdf_atlas::Charset primary_charset;
    msdf_atlas::Charset fallback_charset;
    if (!_text_atlas_product_resolve(
            fonts.primary.handle, fonts.fallback.handle, codepoints, codepoint_count,
            params->fallback_codepoint, product->coverage, &primary_charset, &fallback_charset,
            &product->fallback_mapping_count))
        return false;

    std::vector<msdf_atlas::GlyphGeometry> generated;
    msdf_atlas::FontGeometry primary_geometry(&generated);
    int loaded = primary_geometry.loadCharset(
        fonts.primary.handle, 1.0, primary_charset, params->preprocess_geometry,
        params->enable_kerning);
    if (loaded < 0)
        return false;
    if (fonts.fallback.handle != NULL && fallback_charset.size() > 0)
    {
        msdf_atlas::FontGeometry fallback_geometry(&generated);
        int fallback_loaded = fallback_geometry.loadCharset(
            fonts.fallback.handle, 1.0, fallback_charset, params->preprocess_geometry,
            params->enable_kerning);
        if (fallback_loaded < 0 || fallback_loaded > INT_MAX - loaded)
            return false;
        loaded += fallback_loaded;
    }
    if (loaded <= 0 || generated.empty() || generated.size() > budget->max_glyphs ||
        generated.size() > (size_t)UINT32_MAX || generated.size() > (size_t)INT_MAX)
        return false;

    for (msdf_atlas::GlyphGeometry& glyph : generated)
        glyph.edgeColoring(
            &msdfgen::edgeColoringInkTrap, params->max_corner_angle,
            (unsigned long long)params->edge_coloring_seed);

    msdf_atlas::TightAtlasPacker packer;
    packer.setDimensionsConstraint(msdf_atlas::DimensionsConstraint::SQUARE);
    packer.setMinimumScale((double)spec->em_px);
    packer.setPixelRange((double)spec->distance_range_px);
    packer.setMiterLimit(params->miter_limit);
    if (packer.pack(generated.data(), (int)generated.size()) != 0)
        return false;

    int width = 0;
    int height = 0;
    packer.getDimensions(width, height);
    if (width <= 0 || height <= 0 || (uint32_t)width > budget->max_dimension ||
        (uint32_t)height > budget->max_dimension)
        return false;

    uint64_t pixel_count = 0;
    uint64_t rgba_size = 0;
    if (_dvz_mul_u64_overflows((uint64_t)width, (uint64_t)height, &pixel_count) ||
        _dvz_mul_u64_overflows(pixel_count, DVZ_TEXT_ATLAS_PRODUCT_CHANNELS, &rgba_size) ||
        rgba_size > budget->max_rgba_bytes || rgba_size > SIZE_MAX)
        return false;

    msdf_atlas::ImmediateAtlasGenerator<
        float, 4, &msdf_atlas::mtsdfGenerator,
        msdf_atlas::BitmapAtlasStorage<uint8_t, 4>>
        generator(width, height);
    msdf_atlas::GeneratorAttributes attributes;
    attributes.config.overlapSupport = params->overlap_support;
    attributes.scanlinePass = params->scanline_pass;
    generator.setAttributes(attributes);
    generator.setThreadCount((int)params->thread_count);
    generator.generate(generated.data(), generated.size());
    msdfgen::BitmapConstRef<uint8_t, 4> bitmap = generator.atlasStorage();
    if (bitmap.pixels == NULL)
        return false;

    uint64_t glyph_bytes = 0;
    if (_dvz_mul_u64_overflows(
            (uint64_t)generated.size(), sizeof(DvzTextAtlasGlyph), &glyph_bytes) ||
        glyph_bytes > SIZE_MAX)
        return false;
    product->rgba = (uint8_t*)dvz_calloc(1, rgba_size);
    product->glyphs = (DvzTextAtlasGlyph*)dvz_calloc(1, glyph_bytes);
    if (product->rgba == NULL || product->glyphs == NULL)
        return false;

    uint32_t atlas_width = (uint32_t)width;
    uint32_t atlas_height = (uint32_t)height;
    for (uint32_t y = 0; y < atlas_height; y++)
    {
        for (uint32_t x = 0; x < atlas_width; x++)
        {
            uint64_t source = ((uint64_t)y * atlas_width + x) * 4u;
            uint64_t target = ((uint64_t)(atlas_height - 1u - y) * atlas_width + x) * 4u;
            product->rgba[target + 0] = bitmap.pixels[source + 0];
            product->rgba[target + 1] = bitmap.pixels[source + 1];
            product->rgba[target + 2] = bitmap.pixels[source + 2];
            product->rgba[target + 3] = bitmap.pixels[source + 3];
        }
    }

    product->spec = *spec;
    product->backend = DVZ_TEXT_ATLAS_BACKEND_MSDF;
    product->encoding = DVZ_TEXT_ATLAS_ENCODING_MSDF_RGB;
    product->params = *params;
    product->primary_face_index = primary->face_index;
    product->fallback_face_index = fallback != NULL ? fallback->face_index : -1;
    product->primary_load_flags = primary->load_flags;
    product->fallback_load_flags = fallback != NULL ? fallback->load_flags : 0;
    product->width = atlas_width;
    product->height = atlas_height;
    product->channels = DVZ_TEXT_ATLAS_PRODUCT_CHANNELS;
    product->glyph_count = (uint32_t)generated.size();
    product->coverage_count = codepoint_count;
    product->rgba_size = rgba_size;

    const double scale = packer.getScale();
    const msdfgen::FontMetrics& metrics = primary_geometry.getMetrics();
    product->em_px = (float)scale;
    product->distance_range_px = spec->distance_range_px;
    product->ascent = (float)(metrics.ascenderY * scale);
    product->descent = (float)(metrics.descenderY * scale);
    product->line_gap =
        (float)((metrics.lineHeight - metrics.ascenderY + metrics.descenderY) * scale);
    product->line_height = (float)(metrics.lineHeight * scale);
    if (product->line_height <= 0.0f)
        product->line_height = (float)scale;

    for (uint32_t i = 0; i < product->glyph_count; i++)
        if (!_text_atlas_product_fill_glyph(
                &generated[i], scale, atlas_width, atlas_height, &product->glyphs[i]))
            return false;
    for (uint32_t i = 0; i < product->coverage_count; i++)
    {
        uint32_t index = _text_atlas_product_find_glyph(
            product->glyphs, product->glyph_count, product->coverage[i].resolved_codepoint);
        if (index == UINT32_MAX)
            return false;
        product->coverage[i].glyph_index = index;
    }
    return true;
}
#endif



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return the bounded runtime atlas-product budget.
 *
 * @return default product budget
 */
DvzTextAtlasProductBudget _text_atlas_product_budget_default(void)
{
    DvzTextAtlasProductBudget budget = {};
    budget.max_glyphs = DVZ_TEXT_ATLAS_PRODUCT_DEFAULT_MAX_GLYPHS;
    budget.max_dimension = DVZ_TEXT_ATLAS_PRODUCT_DEFAULT_MAX_DIMENSION;
    budget.max_rgba_bytes = DVZ_TEXT_ATLAS_PRODUCT_DEFAULT_MAX_RGBA_BYTES;
    return budget;
}



/**
 * Return the runtime MSDF generation recipe.
 *
 * @return default deterministic generation parameters
 */
DvzTextAtlasProductParams _text_atlas_product_params_default(void)
{
    DvzTextAtlasProductParams params = {};
    params.thread_count = 8;
    params.fallback_codepoint = 63u;
    params.edge_coloring_seed = 0;
    params.max_corner_angle = 3.0;
    params.miter_limit = 1.0;
    params.overlap_support = true;
    params.scanline_pass = true;
    params.preprocess_geometry = true;
    params.enable_kerning = true;
    return params;
}



/**
 * Return whether runtime MSDF product generation is compiled in.
 *
 * @return whether the MSDF atlas dependency is available
 */
bool _text_atlas_product_msdf_available(void)
{
#if defined(DVZ_HAS_MSDF_ATLAS) && DVZ_HAS_MSDF_ATLAS
    return true;
#else
    return false;
#endif
}



/**
 * Validate the ownership, bounds, metrics, glyphs, and strict coverage of an atlas product.
 *
 * @param product atlas product
 * @param budget hard output and allocation limits
 * @return whether the product is complete and internally consistent
 */
bool _text_atlas_product_validate(
    const DvzTextAtlasProduct* product, const DvzTextAtlasProductBudget* budget)
{
    if (product == NULL || budget == NULL || product->rgba == NULL || product->glyphs == NULL ||
        product->coverage == NULL)
        return false;
    if (product->spec.backend != DVZ_TEXT_ATLAS_BACKEND_MSDF ||
        product->backend != DVZ_TEXT_ATLAS_BACKEND_MSDF ||
        product->encoding != DVZ_TEXT_ATLAS_ENCODING_MSDF_RGB ||
        product->channels != DVZ_TEXT_ATLAS_PRODUCT_CHANNELS)
        return false;
    if (product->width == 0 || product->height == 0 ||
        product->width > budget->max_dimension || product->height > budget->max_dimension ||
        product->glyph_count == 0 || product->glyph_count > budget->max_glyphs ||
        product->coverage_count == 0 || product->coverage_count > budget->max_glyphs)
        return false;

    uint64_t pixel_count = 0;
    uint64_t expected_size = 0;
    if (_dvz_mul_u64_overflows(product->width, product->height, &pixel_count) ||
        _dvz_mul_u64_overflows(
            pixel_count, DVZ_TEXT_ATLAS_PRODUCT_CHANNELS, &expected_size) ||
        expected_size != product->rgba_size || expected_size > budget->max_rgba_bytes)
        return false;
    if (!_text_atlas_product_positive((double)product->spec.em_px) ||
        !_text_atlas_product_positive((double)product->spec.distance_range_px) ||
        !_text_atlas_product_positive((double)product->em_px) ||
        !_text_atlas_product_positive((double)product->distance_range_px) ||
        !_text_atlas_product_finite(product->ascent) ||
        !_text_atlas_product_finite(product->descent) ||
        !_text_atlas_product_finite(product->line_gap) ||
        !_text_atlas_product_positive((double)product->line_height))
        return false;

    for (uint32_t i = 0; i < product->glyph_count; i++)
    {
        const DvzTextAtlasGlyph* glyph = &product->glyphs[i];
        if (!glyph->valid || !_text_atlas_product_codepoint_valid(glyph->codepoint) ||
            glyph->glyph_id == 0 || !_text_atlas_product_finite(glyph->advance) ||
            !_text_atlas_product_finite(glyph->width) ||
            !_text_atlas_product_finite(glyph->height))
            return false;
        if (i > 0 &&
            _text_atlas_product_find_glyph(product->glyphs, i, glyph->codepoint) != UINT32_MAX)
            return false;
        for (uint32_t j = 0; j < 4; j++)
        {
            if (!_text_atlas_product_finite(glyph->plane_bounds[j]) ||
                !_text_atlas_product_finite(glyph->atlas_bounds[j]) ||
                !_text_atlas_product_finite(glyph->uv[j]) || glyph->uv[j] < 0.0f ||
                glyph->uv[j] > 1.0f)
                return false;
        }
    }

    uint32_t fallback_mapping_count = 0;
    for (uint32_t i = 0; i < product->coverage_count; i++)
    {
        const DvzTextAtlasProductCoverage* item = &product->coverage[i];
        if (!_text_atlas_product_codepoint_valid(item->requested_codepoint) ||
            !_text_atlas_product_codepoint_valid(item->resolved_codepoint) ||
            item->glyph_index >= product->glyph_count ||
            product->glyphs[item->glyph_index].codepoint != item->resolved_codepoint ||
            (item->font_role != DVZ_TEXT_ATLAS_PRODUCT_FONT_PRIMARY &&
             item->font_role != DVZ_TEXT_ATLAS_PRODUCT_FONT_FALLBACK))
            return false;
        if (i > 0 &&
            product->coverage[i - 1].requested_codepoint >= item->requested_codepoint)
            return false;
        if (item->kind == DVZ_TEXT_ATLAS_PRODUCT_COVERAGE_EXACT)
        {
            if (item->resolved_codepoint != item->requested_codepoint)
                return false;
        }
        else if (item->kind == DVZ_TEXT_ATLAS_PRODUCT_COVERAGE_FALLBACK)
        {
            if (item->resolved_codepoint != product->params.fallback_codepoint ||
                item->requested_codepoint == item->resolved_codepoint)
                return false;
            fallback_mapping_count++;
        }
        else
            return false;
    }
    return fallback_mapping_count == product->fallback_mapping_count;
}



/**
 * Destroy an owned CPU atlas product.
 *
 * @param product atlas product, which is reset to an empty state
 */
void _text_atlas_product_destroy(DvzTextAtlasProduct* product)
{
    if (product == NULL)
        return;
    dvz_free(product->coverage);
    dvz_free(product->glyphs);
    dvz_free(product->rgba);
    *product = {};
}



/**
 * Build a complete owned MSDF atlas product without scene or GPU state.
 *
 * @param primary borrowed primary font bytes and face metadata
 * @param fallback optional borrowed fallback font bytes and face metadata
 * @param spec requested MSDF atlas specification
 * @param codepoints canonical strictly increasing codepoint array
 * @param codepoint_count number of requested codepoints
 * @param budget hard output and allocation limits
 * @param params deterministic generation parameters
 * @param out_product zero-initialized output product
 * @return whether every codepoint was generated exactly or explicitly mapped to the fallback glyph
 */
bool _text_atlas_product_build_msdf(
    const DvzTextAtlasFontView* primary, const DvzTextAtlasFontView* fallback,
    const DvzTextAtlasSpec* spec, const uint32_t* codepoints, uint32_t codepoint_count,
    const DvzTextAtlasProductBudget* budget, const DvzTextAtlasProductParams* params,
    DvzTextAtlasProduct* out_product)
{
    if (!_text_atlas_product_request_valid(
            primary, fallback, spec, codepoints, codepoint_count, budget, params, out_product))
        return false;
#if defined(DVZ_HAS_MSDF_ATLAS) && DVZ_HAS_MSDF_ATLAS
    DvzTextAtlasProduct staged = {};
    try
    {
        bool built = _text_atlas_product_build_enabled(
            primary, fallback, spec, codepoints, codepoint_count, budget, params, &staged);
        if (!built || !_text_atlas_product_validate(&staged, budget))
        {
            _text_atlas_product_destroy(&staged);
            return false;
        }
    }
    catch (...)
    {
        _text_atlas_product_destroy(&staged);
        return false;
    }
    *out_product = staged;
    return true;
#else
    return false;
#endif
}
