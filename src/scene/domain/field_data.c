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

#include <stdbool.h>
#include <stdint.h>

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
