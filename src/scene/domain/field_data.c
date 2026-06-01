/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene field data helpers                                                                     */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_overflow.h"
#include "_scene.h"
#include "_visual_internal.h"
#include "field_internal.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

#define DVZ_FIELD_DATA_VIEW_KNOWN_FLAGS 0u



static bool _field_data_view_abi_validate(const DvzFieldDataView* view)
{
    if (view == NULL)
        return false;
    if (!DVZ_STRUCT_VALID(view, DvzFieldDataView, DVZ_FIELD_DATA_VIEW_KNOWN_FLAGS))
    {
        log_error("invalid DvzFieldDataView ABI prologue");
        return false;
    }
    return true;
}



DvzFieldDataView dvz_field_data_view(void)
{
    return (DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView)};
}



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



/**
 * Replace the entire field payload.
 *
 * @param field the sampled field
 * @param view the uploaded data view
 * @return true on success, false on error
 */
bool dvz_sampled_field_set_data(DvzSampledField* field, const DvzFieldDataView* view)
{
    ANN(field);
    ANN(view);
    if (!_field_data_view_abi_validate(view))
        return false;
    if (!_scene_visual_mutation_allowed(field->scene, "replace sampled field data"))
        return false;

    DvzFieldRegion full = _field_full_region(&field->desc);
    if (!_field_data_view_valid(&field->desc, view, &full))
        return false;

    if (field->data == NULL)
    {
        field->data = dvz_calloc(field->data_size, 1);
        if (field->data == NULL)
        {
            log_error("sampled field allocation failed for %" PRIu64 " bytes", field->data_size);
            return false;
        }
    }

    _field_copy_full_data(&field->desc, view, field->data);
    _scene_mark_field_region_dirty(field, full, true);
    return true;
}



/**
 * Change the sampled-field extent and replace its full payload.
 *
 * @param field the sampled field
 * @param width new field width in samples
 * @param height new field height in samples
 * @param depth new field depth in samples
 * @param view the uploaded data view for the new extent
 * @return true on success, false on error
 */
bool dvz_sampled_field_resize(
    DvzSampledField* field, uint32_t width, uint32_t height, uint32_t depth,
    const DvzFieldDataView* view)
{
    ANN(field);
    ANN(view);
    if (!_field_data_view_abi_validate(view))
        return false;
    if (!_scene_visual_mutation_allowed(field->scene, "resize sampled field"))
        return false;
    if (width == 0 || height == 0 || depth == 0)
    {
        log_error("sampled field dimensions must be non-zero");
        return false;
    }

    DvzSampledFieldDesc desc = field->desc;
    desc.width = width;
    desc.height = height;
    desc.depth = depth;
    if (desc.dim == DVZ_FIELD_DIM_2D && desc.depth != 1)
    {
        log_error("2D sampled fields must use depth=1");
        return false;
    }

    uint64_t data_size = 0;
    if (!_field_expected_data_size(&desc, &data_size))
    {
        log_error("sampled field size overflow");
        return false;
    }

    DvzFieldRegion full = _field_full_region(&desc);
    if (!_field_data_view_valid(&desc, view, &full))
        return false;

    void* data = dvz_calloc(data_size, 1);
    if (data == NULL)
    {
        log_error("sampled field allocation failed for %" PRIu64 " bytes", data_size);
        return false;
    }
    _field_copy_full_data(&desc, view, data);

    if (field->data != NULL)
        dvz_free(field->data);
    field->desc = desc;
    field->data = data;
    field->data_size = data_size;
    _scene_mark_field_region_dirty(field, full, true);
    return true;
}



/**
 * Update a field subregion in sample coordinates.
 *
 * @param field the sampled field
 * @param region the updated region
 * @param view the uploaded data view
 * @return true on success, false on error
 */
bool dvz_sampled_field_update_region(
    DvzSampledField* field, DvzFieldRegion region, const DvzFieldDataView* view)
{
    ANN(field);
    ANN(view);
    if (!_field_data_view_abi_validate(view))
        return false;
    if (!_scene_visual_mutation_allowed(field->scene, "update sampled field data"))
        return false;
    if (field->data == NULL)
    {
        log_error("sampled field range update requires prior full allocation");
        return false;
    }
    if (region.width == 0 || region.height == 0 || region.depth == 0)
    {
        log_error("sampled field update region requires non-zero extent");
        return false;
    }
    if (
        region.x + region.width > field->desc.width ||
        region.y + region.height > field->desc.height ||
        region.z + region.depth > field->desc.depth)
    {
        log_error("sampled field update region exceeds field dimensions");
        return false;
    }
    if (!_field_data_view_valid(&field->desc, view, &region))
        return false;

    uint32_t bytes_per_texel = 0;
    if (!_field_format_bytes_per_texel(field->desc.format, &bytes_per_texel))
        return false;
    uint64_t src_bytes_per_row =
        view->bytes_per_row != 0 ? view->bytes_per_row
                                 : (uint64_t)region.width * (uint64_t)bytes_per_texel;
    uint64_t src_rows_per_image =
        view->rows_per_image != 0 ? view->rows_per_image : region.height;
    uint64_t dst_bytes_per_row = _field_default_bytes_per_row(&field->desc);
    uint64_t copy_bytes = (uint64_t)region.width * (uint64_t)bytes_per_texel;
    const uint8_t* src = (const uint8_t*)view->data;
    uint8_t* dst = (uint8_t*)field->data;
    for (uint32_t z = 0; z < region.depth; z++)
    {
        for (uint32_t y = 0; y < region.height; y++)
        {
            uint64_t src_offset = ((uint64_t)z * src_rows_per_image + y) * src_bytes_per_row;
            uint64_t dst_offset =
                ((uint64_t)(region.z + z) * field->desc.height + (region.y + y)) *
                    dst_bytes_per_row +
                (uint64_t)region.x * (uint64_t)bytes_per_texel;
            dvz_memcpy(dst + dst_offset, copy_bytes, src + src_offset, copy_bytes);
        }
    }
    _scene_mark_field_region_dirty(field, region, false);
    return true;
}
