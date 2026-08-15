/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  CPU text atlas products                                                                      */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/common/macros.h"
#include "datoviz/scene/types.h"



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_TEXT_ATLAS_PRODUCT_COVERAGE_EXACT = 0,
    DVZ_TEXT_ATLAS_PRODUCT_COVERAGE_FALLBACK,
} DvzTextAtlasProductCoverageKind;


typedef enum
{
    DVZ_TEXT_ATLAS_PRODUCT_FONT_PRIMARY = 0,
    DVZ_TEXT_ATLAS_PRODUCT_FONT_FALLBACK,
} DvzTextAtlasProductFontRole;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

/**
 * Borrowed immutable bytes and load metadata for one font face.
 *
 * @note `load_flags` is reserved for source identity and must currently be zero.
 */
typedef struct DvzTextAtlasFontView DvzTextAtlasFontView;
struct DvzTextAtlasFontView
{
    const uint8_t* bytes;
    uint64_t size;
    int32_t face_index;
    uint32_t load_flags;
};


/**
 * Hard allocation and output limits for one atlas product.
 */
typedef struct DvzTextAtlasProductBudget DvzTextAtlasProductBudget;
struct DvzTextAtlasProductBudget
{
    uint32_t max_glyphs;
    uint32_t max_dimension;
    uint64_t max_rgba_bytes;
};


/**
 * Complete deterministic MSDF generation parameters.
 */
typedef struct DvzTextAtlasProductParams DvzTextAtlasProductParams;
struct DvzTextAtlasProductParams
{
    uint32_t thread_count;
    uint32_t fallback_codepoint;
    uint64_t edge_coloring_seed;
    double max_corner_angle;
    double miter_limit;
    bool overlap_support;
    bool scanline_pass;
    bool preprocess_geometry;
    bool enable_kerning;
};


/**
 * Resolution of one requested codepoint into one generated glyph.
 */
typedef struct DvzTextAtlasProductCoverage DvzTextAtlasProductCoverage;
struct DvzTextAtlasProductCoverage
{
    uint32_t requested_codepoint;
    uint32_t resolved_codepoint;
    uint32_t glyph_index;
    DvzTextAtlasProductCoverageKind kind;
    DvzTextAtlasProductFontRole font_role;
};


/**
 * Owned, scene-independent CPU atlas result.
 */
typedef struct DvzTextAtlasProduct DvzTextAtlasProduct;
struct DvzTextAtlasProduct
{
    DvzTextAtlasSpec spec;
    DvzTextAtlasBackend backend;
    DvzTextAtlasEncoding encoding;
    DvzTextAtlasProductParams params;
    int32_t primary_face_index;
    int32_t fallback_face_index;
    uint32_t primary_load_flags;
    uint32_t fallback_load_flags;
    uint32_t width;
    uint32_t height;
    uint32_t channels;
    uint32_t glyph_count;
    uint32_t coverage_count;
    uint32_t fallback_mapping_count;
    float em_px;
    float distance_range_px;
    float ascent;
    float descent;
    float line_gap;
    float line_height;
    uint64_t rgba_size;
    uint8_t* rgba;
    DvzTextAtlasGlyph* glyphs;
    DvzTextAtlasProductCoverage* coverage;
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

EXTERN_C_ON

/**
 * Return the bounded runtime atlas-product budget.
 *
 * @return default product budget
 */
DvzTextAtlasProductBudget _text_atlas_product_budget_default(void);

/**
 * Return the runtime MSDF generation recipe.
 *
 * @return default deterministic generation parameters
 */
DvzTextAtlasProductParams _text_atlas_product_params_default(void);

/**
 * Return whether runtime MSDF product generation is compiled in.
 *
 * @return whether the MSDF atlas dependency is available
 */
bool _text_atlas_product_msdf_available(void);

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
 * @note Failure leaves `out_product` empty. The caller owns and must destroy a successful product.
 */
bool _text_atlas_product_build_msdf(
    const DvzTextAtlasFontView* primary, const DvzTextAtlasFontView* fallback,
    const DvzTextAtlasSpec* spec, const uint32_t* codepoints, uint32_t codepoint_count,
    const DvzTextAtlasProductBudget* budget, const DvzTextAtlasProductParams* params,
    DvzTextAtlasProduct* out_product);

/**
 * Validate the ownership, bounds, metrics, glyphs, and strict coverage of an atlas product.
 *
 * @param product atlas product
 * @param budget hard output and allocation limits
 * @return whether the product is complete and internally consistent
 */
bool _text_atlas_product_validate(
    const DvzTextAtlasProduct* product, const DvzTextAtlasProductBudget* budget);

/**
 * Destroy an owned CPU atlas product.
 *
 * @param product atlas product, which is reset to an empty state
 */
void _text_atlas_product_destroy(DvzTextAtlasProduct* product);

EXTERN_C_OFF
