/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene field data helpers                                                                          */
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
/*  Function prototypes                                                                          */
/*************************************************************************************************/

static double _half_to_double(uint16_t bits);



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

bool _field_data_view_valid(
    const DvzSampledFieldDesc* desc, const DvzFieldDataView* view, const DvzFieldRegion* region)
{
    ANN(desc);
    ANN(view);
    ANN(view->data);
    ANN(region);
    if (region->width == 0 || region->height == 0 || region->depth == 0)
    {
        log_error("sampled field data view requires non-zero update extent");
        return false;
    }
    uint64_t full_bytes_per_row = _field_default_bytes_per_row(desc);
    uint64_t full_rows_per_image = _field_default_rows_per_image(desc);
    if (full_bytes_per_row == 0 || full_rows_per_image == 0)
    {
        log_error("sampled field descriptor produced zero row/image stride");
        return false;
    }

    uint32_t bytes_per_texel = 0;
    if (!_field_format_bytes_per_texel(desc->format, &bytes_per_texel))
    {
        log_error("unsupported sampled field format %d", (int)desc->format);
        return false;
    }

    uint64_t region_bytes_per_row = 0;
    if (_dvz_mul_u64_overflows(region->width, bytes_per_texel, &region_bytes_per_row))
    {
        log_error("sampled field row-byte size overflow");
        return false;
    }
    uint64_t bytes_per_row =
        view->bytes_per_row != 0 ? view->bytes_per_row : region_bytes_per_row;
    uint64_t rows_per_image =
        view->rows_per_image != 0 ? view->rows_per_image : region->height;
    if (bytes_per_row < region_bytes_per_row)
    {
        log_error("sampled field bytes_per_row is too small for the update width");
        return false;
    }
    if (rows_per_image < region->height)
    {
        log_error("sampled field rows_per_image is too small for the update height");
        return false;
    }
    return true;
}


/**
 * Copy a full sampled-field payload into tightly packed owned storage.
 *
 * @param desc the sampled-field descriptor
 * @param view the source data view
 * @param dst the destination buffer
 */
void _field_copy_full_data(
    const DvzSampledFieldDesc* desc, const DvzFieldDataView* view, void* dst)
{
    ANN(desc);
    ANN(view);
    ANN(dst);
    uint64_t bytes_per_row =
        view->bytes_per_row != 0 ? view->bytes_per_row : _field_default_bytes_per_row(desc);
    uint64_t rows_per_image = view->rows_per_image != 0 ? view->rows_per_image : desc->height;
    uint64_t copy_bytes_per_row = _field_default_bytes_per_row(desc);
    const uint8_t* src = (const uint8_t*)view->data;
    uint8_t* dst_bytes = (uint8_t*)dst;
    for (uint32_t z = 0; z < desc->depth; z++)
    {
        for (uint32_t y = 0; y < desc->height; y++)
        {
            uint64_t src_offset = ((uint64_t)z * rows_per_image + y) * bytes_per_row;
            uint64_t dst_offset = ((uint64_t)z * desc->height + y) * copy_bytes_per_row;
            dvz_memcpy(
                dst_bytes + dst_offset, copy_bytes_per_row, src + src_offset,
                copy_bytes_per_row);
        }
    }
}


bool _field_read_scalar(
    const DvzSampledField* field, uint64_t sample_index, double* out_value)
{
    ANN(field);
    ANN(out_value);
    ANN(field->data);
    const uint8_t* bytes = (const uint8_t*)field->data;
    switch (field->desc.format)
    {
    case DVZ_FIELD_FORMAT_R8_UNORM:
        *out_value = (double)bytes[sample_index] / 255.0;
        return true;
    case DVZ_FIELD_FORMAT_R8_SNORM:
    {
        int8_t v = ((const int8_t*)field->data)[sample_index];
        *out_value = v == INT8_MIN ? -1.0 : (double)v / 127.0;
        return true;
    }
    case DVZ_FIELD_FORMAT_R8_UINT:
        *out_value = (double)((const uint8_t*)field->data)[sample_index];
        return true;
    case DVZ_FIELD_FORMAT_R8_SINT:
        *out_value = (double)((const int8_t*)field->data)[sample_index];
        return true;
    case DVZ_FIELD_FORMAT_R16_UNORM:
        *out_value = (double)((const uint16_t*)field->data)[sample_index] / 65535.0;
        return true;
    case DVZ_FIELD_FORMAT_R16_SNORM:
    {
        int16_t v = ((const int16_t*)field->data)[sample_index];
        *out_value = v == INT16_MIN ? -1.0 : (double)v / 32767.0;
        return true;
    }
    case DVZ_FIELD_FORMAT_R16_UINT:
        *out_value = (double)((const uint16_t*)field->data)[sample_index];
        return true;
    case DVZ_FIELD_FORMAT_R16_SINT:
        *out_value = (double)((const int16_t*)field->data)[sample_index];
        return true;
    case DVZ_FIELD_FORMAT_R16_FLOAT:
        *out_value = _half_to_double(((const uint16_t*)field->data)[sample_index]);
        return true;
    case DVZ_FIELD_FORMAT_R32_UINT:
        *out_value = (double)((const uint32_t*)field->data)[sample_index];
        return true;
    case DVZ_FIELD_FORMAT_R32_SINT:
        *out_value = (double)((const int32_t*)field->data)[sample_index];
        return true;
    case DVZ_FIELD_FORMAT_R32_FLOAT:
        *out_value = (double)((const float*)field->data)[sample_index];
        return true;
    default:
        return false;
    }
}


static double _half_to_double(uint16_t bits)
{
    uint32_t sign = (bits >> 15) & 0x1u;
    uint32_t exp = (bits >> 10) & 0x1fu;
    uint32_t frac = bits & 0x3ffu;

    if (exp == 0)
    {
        if (frac == 0)
            return sign ? -0.0 : 0.0;
        double value = ldexp((double)frac / 1024.0, -14);
        return sign ? -value : value;
    }
    if (exp == 31)
    {
        if (frac == 0)
            return sign ? -INFINITY : INFINITY;
        return NAN;
    }

    double value = ldexp(1.0 + (double)frac / 1024.0, (int32_t)exp - 15);
    return sign ? -value : value;
}


/**
 * Ensure one sampled field has scratch storage for a packed upload.
 *
 * @param field the sampled field
 * @param byte_size the required scratch size
 * @return whether the scratch storage is available
 */
bool _field_ensure_upload(DvzSampledField* field, uint64_t byte_size)
{
    ANN(field);
    if (field->upload != NULL && field->upload_size == byte_size)
        return true;
    if (field->upload != NULL)
    {
        dvz_free(field->upload);
        field->upload = NULL;
        field->upload_size = 0;
    }
    field->upload = dvz_calloc(byte_size, 1);
    if (field->upload == NULL)
        return false;
    field->upload_size = byte_size;
    return true;
}
