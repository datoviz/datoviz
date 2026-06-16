/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene field texture upload helpers                                                           */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_scene.h"
#include "field_internal.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Prepare a sampled field as a tightly packed texture upload.
 *
 * @param field the sampled field
 * @param out_region output uploaded region
 * @param out_data output packed upload bytes
 * @return whether the field can be uploaded as a texture
 */
bool _scene_prepare_field_texture(
    DvzSampledField* field, DvzFieldRegion* out_region, const void** out_data)
{
    ANN(field);
    ANN(out_region);
    ANN(out_data);
    if (field->data == NULL || field->desc.width == 0 || field->desc.height == 0 ||
        field->desc.depth == 0)
    {
        log_error("sampled field texture upload requires uploaded data");
        return false;
    }

    uint32_t bytes_per_texel = 0;
    if (!_field_format_bytes_per_texel(field->desc.format, &bytes_per_texel))
    {
        log_error("sampled field texture upload has unsupported format %d", (int)field->desc.format);
        return false;
    }

    *out_region = field->dirty ? field->dirty_region : _field_full_region(&field->desc);
    if (field->dirty_full)
        *out_region = _field_full_region(&field->desc);
    if (out_region->width == 0 || out_region->height == 0 || out_region->depth == 0)
        *out_region = _field_full_region(&field->desc);

    if (out_region->x == 0 && out_region->y == 0 && out_region->z == 0 &&
        out_region->width == field->desc.width && out_region->height == field->desc.height &&
        out_region->depth == field->desc.depth)
    {
        *out_data = field->data;
        return true;
    }

    uint64_t upload_size = 0;
    if (!_field_region_byte_size(field->desc.format, out_region, &upload_size) ||
        !_field_ensure_upload(field, upload_size))
    {
        log_error("sampled field texture upload scratch allocation failed");
        return false;
    }

    uint8_t* dst = (uint8_t*)field->upload;
    const uint8_t* src = (const uint8_t*)field->data;
    uint64_t src_bpr = _field_default_bytes_per_row(&field->desc);
    uint64_t row_bytes = (uint64_t)out_region->width * (uint64_t)bytes_per_texel;
    for (uint32_t z = 0; z < out_region->depth; z++)
    {
        for (uint32_t y = 0; y < out_region->height; y++)
        {
            uint64_t src_offset =
                ((uint64_t)(out_region->z + z) * field->desc.height + (out_region->y + y)) *
                    src_bpr +
                (uint64_t)out_region->x * (uint64_t)bytes_per_texel;
            uint64_t dst_offset = ((uint64_t)z * out_region->height + y) * row_bytes;
            dvz_memcpy(dst + dst_offset, row_bytes, src + src_offset, row_bytes);
        }
    }
    *out_data = field->upload;
    return true;
}



/**
 * Prepare sampled-field texture upload metadata and payload bytes.
 *
 * @param field the sampled field
 * @param out output texture upload payload
 * @return whether the payload is ready for FramePlan upload emission
 */
bool _scene_sampled_field_texture_upload_payload(
    DvzSampledField* field, DvzSampledFieldTextureUploadPayload* out)
{
    ANN(field);
    ANN(out);
    dvz_memset(
        out, sizeof(DvzSampledFieldTextureUploadPayload), 0,
        sizeof(DvzSampledFieldTextureUploadPayload));
    if (!_scene_prepare_field_texture(field, &out->region, &out->data))
        return false;

    if (!_field_region_byte_size(field->desc.format, &out->region, &out->byte_size) ||
        !_field_format_bytes_per_texel(field->desc.format, &out->bytes_per_texel) ||
        !_field_format_texture_format(field->desc.format, &out->texture_format))
    {
        log_error("sampled field texture upload size or format conversion failed");
        return false;
    }

    out->allocation_width = field->desc.width;
    out->allocation_height = field->desc.height;
    out->allocation_depth = field->desc.depth;
    out->color_role = field->desc.color_role;
    out->texture_3d = field->desc.dim == DVZ_FIELD_DIM_3D;
    return true;
}
