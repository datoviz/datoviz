/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* marker_symbols - marker symbol sets with built-in, bitmap, SDF, MSDF, and SVG-path sources.
 *
 * Scenario: feature.marker_symbols
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/marker_symbols
 * Run:    ./build/examples/c/features/marker_symbols
 * Smoke:  ./build/examples/c/features/marker_symbols 1
 * PNG:    DVZ_CAPTURE=png ./build/examples/c/features/marker_symbols 1
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "datoviz/app.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH         1600u
#define HEIGHT        1200u
#define SYMBOL_PIXELS 48u
#define ROW_COUNT     5u
#define ROW_SYMBOLS   5u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Clamp a scalar into [0, 1].
 *
 * @param x input value
 * @return clamped value
 */
static float _saturate(float x)
{
    return fminf(fmaxf(x, 0.0f), 1.0f);
}



/**
 * Quantize a normalized scalar to unorm8.
 *
 * @param x normalized scalar
 * @return unorm8 value
 */
static uint8_t _unorm8(float x)
{
    return (uint8_t)(255.0f * _saturate(x) + 0.5f);
}



/**
 * Convert a signed-distance value into SDF alpha.
 *
 * Negative distances are inside the shape. The marker shader expects 0.5 at the edge and larger
 * values inside.
 *
 * @param distance signed distance in normalized symbol coordinates
 * @param range signed-distance range in normalized symbol coordinates
 * @return encoded SDF value
 */
static uint8_t _encode_sdf(float distance, float range)
{
    return _unorm8(0.5f - distance / fmaxf(range, 1e-6f));
}



/**
 * Return a procedural signed distance for a compact symbol family.
 *
 * @param x normalized x coordinate in [-1, 1]
 * @param y normalized y coordinate in [-1, 1]
 * @param variant symbol variant index
 * @return signed distance, negative inside
 */
static float _shape_distance(float x, float y, uint32_t variant)
{
    switch (variant % 5u)
    {
    case 1:
        return fmaxf(fabsf(x) - 0.52f, fabsf(y) - 0.52f);
    case 2:
        return fabsf(x) + fabsf(y) - 0.72f;
    case 3:
    {
        const float bar = fminf(
            fmaxf(fabsf(x) - 0.68f, fabsf(y) - 0.15f),
            fmaxf(fabsf(x) - 0.15f, fabsf(y) - 0.68f));
        return bar;
    }
    case 4:
    {
        const float outer = sqrtf(x * x + y * y) - 0.66f;
        const float inner = 0.38f - sqrtf(x * x + y * y);
        return fmaxf(outer, inner);
    }
    default:
        return sqrtf(x * x + y * y) - 0.62f;
    }
}



/**
 * Fill one RGBA8 bitmap symbol.
 *
 * @param rgba output RGBA8 payload
 * @param variant symbol variant index
 */
static void _fill_bitmap_symbol(uint8_t rgba[SYMBOL_PIXELS * SYMBOL_PIXELS * 4], uint32_t variant)
{
    const float color_mix = (float)(variant % 5u) / 4.0f;
    for (uint32_t y = 0; y < SYMBOL_PIXELS; y++)
    {
        for (uint32_t x = 0; x < SYMBOL_PIXELS; x++)
        {
            const float px = (2.0f * ((float)x + 0.5f) / (float)SYMBOL_PIXELS) - 1.0f;
            const float py = (2.0f * ((float)y + 0.5f) / (float)SYMBOL_PIXELS) - 1.0f;
            const float d = _shape_distance(px, py, variant);
            const float edge = 0.030f;
            const float alpha = _saturate(0.5f - d / edge);
            const uint32_t i = 4u * (y * SYMBOL_PIXELS + x);
            rgba[i + 0u] = _unorm8(0.78f + 0.18f * color_mix);
            rgba[i + 1u] = _unorm8(0.82f - 0.18f * color_mix);
            rgba[i + 2u] = _unorm8(0.98f);
            rgba[i + 3u] = _unorm8(alpha);
        }
    }
}



/**
 * Fill one single-channel SDF symbol.
 *
 * @param sdf output R8 SDF payload
 * @param variant symbol variant index
 */
static void _fill_sdf_symbol(uint8_t sdf[SYMBOL_PIXELS * SYMBOL_PIXELS], uint32_t variant)
{
    for (uint32_t y = 0; y < SYMBOL_PIXELS; y++)
    {
        for (uint32_t x = 0; x < SYMBOL_PIXELS; x++)
        {
            const float px = (2.0f * ((float)x + 0.5f) / (float)SYMBOL_PIXELS) - 1.0f;
            const float py = (2.0f * ((float)y + 0.5f) / (float)SYMBOL_PIXELS) - 1.0f;
            sdf[y * SYMBOL_PIXELS + x] = _encode_sdf(_shape_distance(px, py, variant), 0.22f);
        }
    }
}



/**
 * Fill one RGB MSDF-compatible symbol.
 *
 * The three channels intentionally carry small offsets around the same signed-distance source so
 * the MSDF shader's median path is visible without external assets.
 *
 * @param msdf output RGB8 MSDF payload
 * @param variant symbol variant index
 */
static void _fill_msdf_symbol(uint8_t msdf[SYMBOL_PIXELS * SYMBOL_PIXELS * 3], uint32_t variant)
{
    for (uint32_t y = 0; y < SYMBOL_PIXELS; y++)
    {
        for (uint32_t x = 0; x < SYMBOL_PIXELS; x++)
        {
            const float px = (2.0f * ((float)x + 0.5f) / (float)SYMBOL_PIXELS) - 1.0f;
            const float py = (2.0f * ((float)y + 0.5f) / (float)SYMBOL_PIXELS) - 1.0f;
            const float d = _shape_distance(px, py, variant);
            const uint32_t i = 3u * (y * SYMBOL_PIXELS + x);
            msdf[i + 0u] = _encode_sdf(d - 0.010f * px, 0.22f);
            msdf[i + 1u] = _encode_sdf(d, 0.22f);
            msdf[i + 2u] = _encode_sdf(d - 0.010f * py, 0.22f);
        }
    }
}



/**
 * Register a row of bitmap-backed symbols.
 *
 * @param symbols symbol set
 * @param out output symbol ids
 * @return true on success
 */
static bool _register_bitmap_symbols(DvzSymbolSet* symbols, DvzSymbolId out[ROW_SYMBOLS])
{
    uint8_t rgba[ROW_SYMBOLS][SYMBOL_PIXELS * SYMBOL_PIXELS * 4] = {{0}};
    for (uint32_t i = 0; i < ROW_SYMBOLS; i++)
    {
        _fill_bitmap_symbol(rgba[i], i);
        out[i] = dvz_symbol_bitmap(symbols, "bitmap", rgba[i], SYMBOL_PIXELS, SYMBOL_PIXELS, NULL);
        if (out[i] == DVZ_SYMBOL_ID_INVALID)
            return false;
    }
    return true;
}



/**
 * Register a row of SDF-backed symbols.
 *
 * @param symbols symbol set
 * @param out output symbol ids
 * @return true on success
 */
static bool _register_sdf_symbols(DvzSymbolSet* symbols, DvzSymbolId out[ROW_SYMBOLS])
{
    uint8_t sdf[ROW_SYMBOLS][SYMBOL_PIXELS * SYMBOL_PIXELS] = {{0}};
    DvzSymbolImageDesc desc = dvz_symbol_image_desc();
    desc.distance_range_px = 5.0f;
    for (uint32_t i = 0; i < ROW_SYMBOLS; i++)
    {
        _fill_sdf_symbol(sdf[i], i);
        out[i] = dvz_symbol_sdf(symbols, "sdf", sdf[i], SYMBOL_PIXELS, SYMBOL_PIXELS, &desc);
        if (out[i] == DVZ_SYMBOL_ID_INVALID)
            return false;
    }
    return true;
}



/**
 * Register a row of MSDF-backed symbols.
 *
 * @param symbols symbol set
 * @param out output symbol ids
 * @return true on success
 */
static bool _register_msdf_symbols(DvzSymbolSet* symbols, DvzSymbolId out[ROW_SYMBOLS])
{
    uint8_t msdf[ROW_SYMBOLS][SYMBOL_PIXELS * SYMBOL_PIXELS * 3] = {{0}};
    DvzSymbolImageDesc desc = dvz_symbol_image_desc();
    desc.distance_range_px = 5.0f;
    for (uint32_t i = 0; i < ROW_SYMBOLS; i++)
    {
        _fill_msdf_symbol(msdf[i], i);
        out[i] = dvz_symbol_msdf(symbols, "msdf", msdf[i], SYMBOL_PIXELS, SYMBOL_PIXELS, &desc);
        if (out[i] == DVZ_SYMBOL_ID_INVALID)
            return false;
    }
    return true;
}



/**
 * Register SVG-path symbols when SVG support is available, otherwise register procedural MSDFs.
 *
 * @param symbols symbol set
 * @param out output symbol ids
 * @return true on success
 */
static bool _register_svg_symbols(DvzSymbolSet* symbols, DvzSymbolId out[ROW_SYMBOLS])
{
#if defined(DVZ_HAS_MSDF_SVG) && DVZ_HAS_MSDF_SVG
    DvzSymbolImageDesc desc = dvz_symbol_image_desc();
    desc.distance_range_px = 5.0f;
    static const char* paths[ROW_SYMBOLS] = {
        "M24 4 L29.9 17.5 L44.5 18.9 L33.5 28.6 L36.7 43 L24 35.6 L11.3 43 "
        "L14.5 28.6 L3.5 18.9 L18.1 17.5 Z",
        "M24 5 L42 24 L24 43 L6 24 Z",
        "M24 5 C35 5 43 13 43 24 C43 35 35 43 24 43 C13 43 5 35 5 24 C5 13 13 5 24 5 Z "
        "M24 15 C19 15 15 19 15 24 C15 29 19 33 24 33 C29 33 33 29 33 24 C33 19 29 15 24 15 Z",
        "M8 22 L20 22 L20 8 L28 8 L28 22 L40 22 L40 30 L28 30 L28 42 L20 42 L20 30 L8 30 Z",
        "M7 14 C7 8 13 5 18 8 L24 14 L30 8 C35 5 41 8 41 14 C41 23 31 31 24 40 C17 31 7 23 7 14 Z",
    };
    for (uint32_t i = 0; i < ROW_SYMBOLS; i++)
    {
        out[i] = dvz_symbol_svg_path(
            symbols, "svg-path", paths[i], SYMBOL_PIXELS, SYMBOL_PIXELS, &desc);
        if (out[i] == DVZ_SYMBOL_ID_INVALID)
            return false;
    }
    return true;
#else
    return _register_msdf_symbols(symbols, out);
#endif
}



/**
 * Register built-in marker symbols in one symbol set.
 *
 * @param symbols symbol set
 * @param out output symbol ids
 * @return true on success
 */
static bool _register_builtin_symbols(DvzSymbolSet* symbols, DvzSymbolId out[ROW_SYMBOLS])
{
    const DvzSymbolBuiltin builtins[ROW_SYMBOLS] = {
        DVZ_SYMBOL_DISC,
        DVZ_SYMBOL_TARGET,
        DVZ_SYMBOL_ARROW,
        DVZ_SYMBOL_HEART,
        DVZ_SYMBOL_ROUNDED_RECT,
    };
    for (uint32_t i = 0; i < ROW_SYMBOLS; i++)
    {
        out[i] = dvz_symbol_builtin(symbols, builtins[i]);
        if (out[i] == DVZ_SYMBOL_ID_INVALID)
            return false;
    }
    return true;
}



/**
 * Add one homogeneous symbol-encoding marker row.
 *
 * @param scene scene owning the marker visual
 * @param panel panel receiving the marker visual
 * @param symbols symbol set bound to the marker visual
 * @param ids per-item symbol ids
 * @param row row index
 * @param y row y coordinate
 * @param role row palette role
 * @return true on success
 */
static bool _add_symbol_row(
    DvzScene* scene, DvzPanel* panel, DvzSymbolSet* symbols, const DvzSymbolId ids[ROW_SYMBOLS],
    uint32_t row, float y, ExampleStyleColorRole role)
{
    DvzVisual* visual = dvz_marker(scene, 0);
    if (visual == NULL)
        return false;
    if (dvz_marker_set_symbols(visual, symbols) != 0)
        return false;

    DvzMarkerStyle style = dvz_marker_style();
    style.aspect = row == 0u ? DVZ_SHAPE_ASPECT_OUTLINE : DVZ_SHAPE_ASPECT_FILLED;
    style.edge_color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT);
    style.edge_color.a = 220u;
    style.stroke_width = 2.0f;
    if (dvz_marker_set_style(visual, &style) != 0)
        return false;

    vec3 positions[ROW_SYMBOLS] = {{0}};
    DvzColor colors[ROW_SYMBOLS] = {{0}};
    float diameters[ROW_SYMBOLS] = {0};
    float angles[ROW_SYMBOLS] = {0};
    uint32_t symbol_ids[ROW_SYMBOLS] = {0};

    for (uint32_t i = 0; i < ROW_SYMBOLS; i++)
    {
        const float t = ROW_SYMBOLS > 1u ? (float)i / (float)(ROW_SYMBOLS - 1u) : 0.0f;
        positions[i][0] = -0.68f + 1.36f * t;
        positions[i][1] = y;
        positions[i][2] = 0.0f;
        colors[i] = example_graphite_cyan_color(role);
        colors[i].a = 242u;
        diameters[i] = 58.0f + 5.0f * (float)((i + row) % 2u);
        angles[i] = 0.18f * (float)(i + row);
        symbol_ids[i] = ids[i];
    }

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = ROW_SYMBOLS},
        {.attr_name = "color", .data = colors, .item_count = ROW_SYMBOLS},
        {.attr_name = "diameter", .data = diameters, .item_count = ROW_SYMBOLS},
        {.attr_name = "angle", .data = angles, .item_count = ROW_SYMBOLS},
        {.attr_name = "symbol", .data = symbol_ids, .item_count = ROW_SYMBOLS},
    };
    if (dvz_visual_set_data_many(visual, updates, 5) != 0)
        return false;
    if (dvz_visual_set_alpha_mode(visual, DVZ_ALPHA_BLENDED) != 0)
        return false;
    return dvz_panel_add_visual(panel, visual, NULL) == 0;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the marker-symbol feature example.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    const uint32_t frame_count = example_frame_count_any(argc, argv);
    DvzAppCaptureConfig capture = dvz_app_capture_config_from_env("feature_marker_symbols");

    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;
    DvzView* win = NULL;

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");
    example_graphite_cyan_set_panel_background(panel);

    DvzSymbolSet* symbol_set = dvz_symbol_set(scene, 0);
    EXAMPLE_CHECK(symbol_set != NULL, "dvz_symbol_set() failed");

    DvzSymbolId builtin_ids[ROW_SYMBOLS] = {0};
    DvzSymbolId bitmap_ids[ROW_SYMBOLS] = {0};
    DvzSymbolId sdf_ids[ROW_SYMBOLS] = {0};
    DvzSymbolId msdf_ids[ROW_SYMBOLS] = {0};
    DvzSymbolId svg_ids[ROW_SYMBOLS] = {0};

    EXAMPLE_CHECK(_register_builtin_symbols(symbol_set, builtin_ids), "built-in symbol setup failed");
    EXAMPLE_CHECK(_register_bitmap_symbols(symbol_set, bitmap_ids), "bitmap symbol setup failed");
    EXAMPLE_CHECK(_register_sdf_symbols(symbol_set, sdf_ids), "SDF symbol setup failed");
    EXAMPLE_CHECK(_register_msdf_symbols(symbol_set, msdf_ids), "MSDF symbol setup failed");
    EXAMPLE_CHECK(_register_svg_symbols(symbol_set, svg_ids), "SVG/MSDF symbol setup failed");

    const float row_y[ROW_COUNT] = {+0.66f, +0.33f, 0.0f, -0.33f, -0.66f};
    const ExampleStyleColorRole row_roles[ROW_COUNT] = {
        EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
        EXAMPLE_STYLE_COLOR_TEXT,
        EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
        EXAMPLE_STYLE_COLOR_WARNING,
        EXAMPLE_STYLE_COLOR_ERROR,
    };
    const DvzSymbolId* row_ids[ROW_COUNT] = {builtin_ids, bitmap_ids, sdf_ids, msdf_ids, svg_ids};
    for (uint32_t row = 0; row < ROW_COUNT; row++)
    {
        EXAMPLE_CHECK(
            _add_symbol_row(scene, panel, symbol_set, row_ids[row], row, row_y[row], row_roles[row]),
            "marker symbol row setup failed");
    }

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "feature_marker_symbols");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzPanzoom* panzoom = dvz_view_panzoom(win, panel, NULL);
    EXAMPLE_CHECK(panzoom != NULL, "failed to create or bind panzoom controller");

    EXAMPLE_CHECK(
        example_run_with_capture(app, win, frame_count, &capture),
        "example_run_with_capture() failed");
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
