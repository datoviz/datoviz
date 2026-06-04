/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Marker SVG symbol import                                                                     */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdint.h>

#include "datoviz/scene.h"

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "_overflow.h"

#if defined(DVZ_HAS_MSDF_SVG) && DVZ_HAS_MSDF_SVG
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wswitch-default"
#endif
#include <msdfgen-ext.h>
#include <msdfgen.h>
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
#endif



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

#if defined(DVZ_HAS_MSDF_SVG) && DVZ_HAS_MSDF_SVG
/**
 * Convert one normalized floating-point MSDF channel to unorm8.
 *
 * @param value floating-point distance-field channel
 * @return clamped 8-bit channel value
 */
static uint8_t _msdf_float_to_u8(float value)
{
    if (value <= 0.0f)
        return 0;
    if (value >= 1.0f)
        return 255;
    return (uint8_t)lrintf(value * 255.0f);
}
#endif



extern "C" {

DvzSymbolId dvz_symbol_svg_path(
    DvzSymbolSet* symbols, const char* name, const char* svg_path, uint32_t width, uint32_t height,
    const DvzSymbolImageDesc* desc)
{
    ANN(symbols);
    ANN(svg_path);
    if (width == 0 || height == 0)
    {
        log_error("symbol SVG path dimensions must be nonzero");
        return DVZ_SYMBOL_ID_INVALID;
    }

#if defined(DVZ_HAS_MSDF_SVG) && DVZ_HAS_MSDF_SVG
    DvzSymbolImageDesc defaults = dvz_symbol_image_desc();
    DvzSymbolImageDesc image_desc = desc != NULL ? *desc : defaults;
    float range_px = image_desc.distance_range_px > 0.0f ? image_desc.distance_range_px : 4.0f;
    image_desc.distance_range_px = range_px;

    msdfgen::Shape shape;
    if (!msdfgen::buildShapeFromSvgPath(shape, svg_path))
    {
        log_error("failed to parse symbol SVG path");
        return DVZ_SYMBOL_ID_INVALID;
    }
    shape.normalize();
    msdfgen::edgeColoringSimple(shape, 3.0);

    msdfgen::Bitmap<float, 3> bitmap((int)width, (int)height);
    double scale_x = width > 2u * (uint32_t)range_px ? (double)width - 2.0 * (double)range_px :
                                                       (double)width;
    double scale_y = height > 2u * (uint32_t)range_px ? (double)height - 2.0 * (double)range_px :
                                                        (double)height;
    msdfgen::generateMSDF(
        bitmap, shape, (double)range_px, msdfgen::Vector2(scale_x, scale_y),
        msdfgen::Vector2((double)range_px, (double)range_px));

    uint64_t pixel_count = 0;
    uint64_t byte_size = 0;
    if (_dvz_mul_u64_overflows((uint64_t)width, (uint64_t)height, &pixel_count) ||
        _dvz_mul_u64_overflows(pixel_count, 3u, &byte_size) || byte_size > SIZE_MAX)
    {
        log_error("symbol SVG path MSDF byte size overflow");
        return DVZ_SYMBOL_ID_INVALID;
    }
    uint8_t* rgb = (uint8_t*)dvz_calloc((DvzSize)byte_size, 1);
    if (rgb == NULL)
    {
        log_error("symbol SVG path MSDF allocation failed");
        return DVZ_SYMBOL_ID_INVALID;
    }

    msdfgen::BitmapConstRef<float, 3> source = bitmap;
    for (uint32_t y = 0; y < height; y++)
    {
        for (uint32_t x = 0; x < width; x++)
        {
            uint64_t src = ((uint64_t)y * width + x) * 3u;
            uint64_t dst = ((uint64_t)(height - 1u - y) * width + x) * 3u;
            rgb[dst + 0] = _msdf_float_to_u8(source.pixels[src + 0]);
            rgb[dst + 1] = _msdf_float_to_u8(source.pixels[src + 1]);
            rgb[dst + 2] = _msdf_float_to_u8(source.pixels[src + 2]);
        }
    }

    DvzSymbolId id = dvz_symbol_msdf(symbols, name, rgb, width, height, &image_desc);
    dvz_free(rgb);
    return id;
#else
    log_error("symbol SVG path import requires DVZ_HAS_MSDF_SVG");
    return DVZ_SYMBOL_ID_INVALID;
#endif
}

}
