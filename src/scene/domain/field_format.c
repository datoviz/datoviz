/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene field format helpers                                                                          */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <volk.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_overflow.h"
#include "_scene.h"
#include "field_internal.h"
#include "sample_profile.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

bool _field_format_supported(DvzFieldFormat format)
{
    switch (format)
    {
    case DVZ_FIELD_FORMAT_R8_UNORM:
    case DVZ_FIELD_FORMAT_R8_SNORM:
    case DVZ_FIELD_FORMAT_R8_UINT:
    case DVZ_FIELD_FORMAT_R8_SINT:
    case DVZ_FIELD_FORMAT_R16_UNORM:
    case DVZ_FIELD_FORMAT_R16_SNORM:
    case DVZ_FIELD_FORMAT_R16_UINT:
    case DVZ_FIELD_FORMAT_R16_SINT:
    case DVZ_FIELD_FORMAT_R16_FLOAT:
    case DVZ_FIELD_FORMAT_R32_UINT:
    case DVZ_FIELD_FORMAT_R32_SINT:
    case DVZ_FIELD_FORMAT_R32_FLOAT:
    case DVZ_FIELD_FORMAT_RGBA8_UNORM:
        return true;
    default:
        return false;
    }
}


bool _field_format_is_scalar(DvzFieldFormat format)
{
    switch (format)
    {
    case DVZ_FIELD_FORMAT_R8_UNORM:
    case DVZ_FIELD_FORMAT_R8_SNORM:
    case DVZ_FIELD_FORMAT_R8_UINT:
    case DVZ_FIELD_FORMAT_R8_SINT:
    case DVZ_FIELD_FORMAT_R16_UNORM:
    case DVZ_FIELD_FORMAT_R16_SNORM:
    case DVZ_FIELD_FORMAT_R16_UINT:
    case DVZ_FIELD_FORMAT_R16_SINT:
    case DVZ_FIELD_FORMAT_R16_FLOAT:
    case DVZ_FIELD_FORMAT_R32_UINT:
    case DVZ_FIELD_FORMAT_R32_SINT:
    case DVZ_FIELD_FORMAT_R32_FLOAT:
        return true;
    default:
        return false;
    }
}


bool _field_format_is_rgba8(DvzFieldFormat format)
{
    return format == DVZ_FIELD_FORMAT_RGBA8_UNORM;
}


/**
 * Return the byte size of one texel for a sampled-field format.
 *
 * @param format the sampled-field format
 * @param out_bytes output byte size
 * @return whether the field format has a packed runtime representation
 */
bool _field_format_bytes_per_texel(DvzFieldFormat format, uint32_t* out_bytes)
{
    ANN(out_bytes);
    switch (format)
    {
    case DVZ_FIELD_FORMAT_R8_UNORM:
    case DVZ_FIELD_FORMAT_R8_SNORM:
    case DVZ_FIELD_FORMAT_R8_UINT:
    case DVZ_FIELD_FORMAT_R8_SINT:
        *out_bytes = 1;
        return true;
    case DVZ_FIELD_FORMAT_R16_UNORM:
    case DVZ_FIELD_FORMAT_R16_SNORM:
    case DVZ_FIELD_FORMAT_R16_UINT:
    case DVZ_FIELD_FORMAT_R16_SINT:
    case DVZ_FIELD_FORMAT_R16_FLOAT:
        *out_bytes = 2;
        return true;
    case DVZ_FIELD_FORMAT_R32_UINT:
    case DVZ_FIELD_FORMAT_R32_SINT:
    case DVZ_FIELD_FORMAT_R32_FLOAT:
        *out_bytes = 4;
        return true;
    case DVZ_FIELD_FORMAT_RGBA8_UNORM:
        *out_bytes = 4;
        return true;
    default:
        *out_bytes = 0;
        return false;
    }
}


/**
 * Return the runtime texture format for a sampled field format.
 *
 * @param format the sampled-field format
 * @param out_format output texture format, using VkFormat values
 * @return whether the field format can be realized as a runtime texture
 */
bool _field_format_texture_format(DvzFieldFormat format, uint32_t* out_format)
{
    ANN(out_format);
    switch (format)
    {
    case DVZ_FIELD_FORMAT_R8_UNORM:
        *out_format = VK_FORMAT_R8_UNORM;
        return true;
    case DVZ_FIELD_FORMAT_R8_SNORM:
        *out_format = VK_FORMAT_R8_SNORM;
        return true;
    case DVZ_FIELD_FORMAT_R8_UINT:
        *out_format = VK_FORMAT_R8_UINT;
        return true;
    case DVZ_FIELD_FORMAT_R8_SINT:
        *out_format = VK_FORMAT_R8_SINT;
        return true;
    case DVZ_FIELD_FORMAT_R16_UNORM:
        *out_format = VK_FORMAT_R16_UNORM;
        return true;
    case DVZ_FIELD_FORMAT_R16_SNORM:
        *out_format = VK_FORMAT_R16_SNORM;
        return true;
    case DVZ_FIELD_FORMAT_R16_UINT:
        *out_format = VK_FORMAT_R16_UINT;
        return true;
    case DVZ_FIELD_FORMAT_R16_SINT:
        *out_format = VK_FORMAT_R16_SINT;
        return true;
    case DVZ_FIELD_FORMAT_R16_FLOAT:
        *out_format = VK_FORMAT_R16_SFLOAT;
        return true;
    case DVZ_FIELD_FORMAT_R32_UINT:
        *out_format = VK_FORMAT_R32_UINT;
        return true;
    case DVZ_FIELD_FORMAT_R32_SINT:
        *out_format = VK_FORMAT_R32_SINT;
        return true;
    case DVZ_FIELD_FORMAT_R32_FLOAT:
        *out_format = VK_FORMAT_R32_SFLOAT;
        return true;
    case DVZ_FIELD_FORMAT_RGBA8_UNORM:
        *out_format = VK_FORMAT_R8G8B8A8_UNORM;
        return true;
    default:
        *out_format = 0;
        return false;
    }
}


bool _field_expected_data_size(const DvzSampledFieldDesc* desc, uint64_t* out_size)
{
    ANN(desc);
    ANN(out_size);
    uint32_t bytes_per_texel = 0;
    if (!_field_format_bytes_per_texel(desc->format, &bytes_per_texel))
        return false;
    uint64_t sample_count = 0;
    if (_dvz_mul_u64_overflows(desc->width, desc->height, &sample_count))
        return false;
    if (desc->dim == DVZ_FIELD_DIM_3D)
    {
        if (_dvz_mul_u64_overflows(sample_count, desc->depth, &sample_count))
            return false;
    }
    return !_dvz_mul_u64_overflows(sample_count, bytes_per_texel, out_size);
}


uint64_t _field_default_bytes_per_row(const DvzSampledFieldDesc* desc)
{
    ANN(desc);
    uint32_t bytes_per_texel = 0;
    if (!_field_format_bytes_per_texel(desc->format, &bytes_per_texel))
        return 0;
    uint64_t bytes_per_row = 0;
    if (_dvz_mul_u64_overflows(desc->width, bytes_per_texel, &bytes_per_row))
        return 0;
    return bytes_per_row;
}


uint64_t _field_default_rows_per_image(const DvzSampledFieldDesc* desc)
{
    ANN(desc);
    return desc->height;
}


DvzFieldRegion _field_full_region(const DvzSampledFieldDesc* desc)
{
    ANN(desc);
    return (DvzFieldRegion){
        .x = 0,
        .y = 0,
        .z = 0,
        .width = desc->width,
        .height = desc->height,
        .depth = desc->depth,
    };
}


bool _field_regions_union(
    const DvzFieldRegion* a, const DvzFieldRegion* b, DvzFieldRegion* out)
{
    ANN(a);
    ANN(b);
    ANN(out);
    uint64_t ax1 = 0, ay1 = 0, az1 = 0;
    uint64_t bx1 = 0, by1 = 0, bz1 = 0;
    if (_dvz_add_u64_overflows(a->x, a->width, &ax1) ||
        _dvz_add_u64_overflows(a->y, a->height, &ay1) ||
        _dvz_add_u64_overflows(a->z, a->depth, &az1) ||
        _dvz_add_u64_overflows(b->x, b->width, &bx1) ||
        _dvz_add_u64_overflows(b->y, b->height, &by1) ||
        _dvz_add_u64_overflows(b->z, b->depth, &bz1))
        return false;

    uint64_t x0 = a->x < b->x ? a->x : b->x;
    uint64_t y0 = a->y < b->y ? a->y : b->y;
    uint64_t z0 = a->z < b->z ? a->z : b->z;
    uint64_t x1 = ax1 > bx1 ? ax1 : bx1;
    uint64_t y1 = ay1 > by1 ? ay1 : by1;
    uint64_t z1 = az1 > bz1 ? az1 : bz1;
    out->x = (uint32_t)x0;
    out->y = (uint32_t)y0;
    out->z = (uint32_t)z0;
    out->width = (uint32_t)(x1 - x0);
    out->height = (uint32_t)(y1 - y0);
    out->depth = (uint32_t)(z1 - z0);
    return true;
}


bool _field_region_byte_size(
    DvzFieldFormat format, const DvzFieldRegion* region, uint64_t* out_size)
{
    ANN(region);
    ANN(out_size);
    uint32_t bytes_per_texel = 0;
    if (!_field_format_bytes_per_texel(format, &bytes_per_texel))
        return false;
    uint64_t sample_count = 0;
    if (_dvz_mul_u64_overflows(region->width, region->height, &sample_count) ||
        _dvz_mul_u64_overflows(sample_count, region->depth, &sample_count))
        return false;
    return !_dvz_mul_u64_overflows(sample_count, bytes_per_texel, out_size);
}
