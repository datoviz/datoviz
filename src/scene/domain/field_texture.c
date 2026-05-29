/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene field texture preparation                                                              */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "_overflow.h"
#include "_scene.h"
#include "annotation/colormap_internal.h"
#include "field_internal.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static bool _visual_texture_ensure_upload(DvzVisualTexture* texture, uint64_t byte_size)
{
    ANN(texture);
    if (texture->upload != NULL && texture->upload_size == byte_size)
        return true;
    if (texture->upload != NULL)
    {
        dvz_free(texture->upload);
        texture->upload = NULL;
        texture->upload_size = 0;
    }
    texture->upload = dvz_calloc(byte_size, 1);
    if (texture->upload == NULL)
        return false;
    texture->upload_size = byte_size;
    return true;
}




/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

bool _scene_prepare_volume_texture(
    DvzVisual* visual, DvzFieldRegion* out_region, const void** out_data,
    uint32_t* out_format, uint32_t* out_bytes_per_texel)
{
    ANN(visual);
    ANN(out_region);
    ANN(out_data);
    ANN(out_format);
    ANN(out_bytes_per_texel);
    if (visual->type != DVZ_VISUAL_TYPE_VOLUME || visual->field == NULL)
        return false;

    DvzSampledField* field = visual->field;
    if (!_scene_prepare_field_texture(field, out_region, out_data) ||
        !_field_format_texture_format(field->desc.format, out_format) ||
        !_field_format_bytes_per_texel(field->desc.format, out_bytes_per_texel))
        return false;
    return true;
}




bool _scene_prepare_image_texture(
    DvzVisual* visual, DvzFieldRegion* out_region, const void** out_data)
{
    ANN(visual);
    ANN(out_region);
    ANN(out_data);
    if (visual->type != DVZ_VISUAL_TYPE_IMAGE && visual->type != DVZ_VISUAL_TYPE_GLYPH)
        return false;
    if (visual->field == NULL)
    {
        log_error("image or glyph visual requires a bound sampled field");
        return false;
    }
    const DvzSampledField* field = visual->field;
    if (field->scene != visual->scene)
    {
        log_error("image or glyph visual field belongs to a different scene");
        return false;
    }
    if (field->desc.dim != DVZ_FIELD_DIM_2D)
    {
        log_error("image and glyph visuals require a 2D sampled field");
        return false;
    }
    if (field->data == NULL || field->desc.width == 0 || field->desc.height == 0)
    {
        log_error("image or glyph visual sampled field has no uploaded data");
        return false;
    }

    visual->texture.width = field->desc.width;
    visual->texture.height = field->desc.height;
    *out_region = _field_full_region(&field->desc);
    if (visual->texture.field_dirty)
        *out_region = visual->texture.field_dirty_full ? _field_full_region(&field->desc)
                                                       : visual->texture.field_dirty_region;
    else if (visual->texture.dirty)
        *out_region = _field_full_region(&field->desc);

    if (_field_format_is_rgba8(field->desc.format))
    {
        if (out_region->x == 0 && out_region->y == 0 && out_region->width == field->desc.width &&
            out_region->height == field->desc.height)
        {
            *out_data = field->data;
            return true;
        }

        uint64_t upload_size = 0;
        if (!_field_region_byte_size(field->desc.format, out_region, &upload_size) ||
            !_visual_texture_ensure_upload(&visual->texture, upload_size))
        {
            log_error("RGBA image field upload scratch allocation failed");
            return false;
        }
        uint8_t* dst = (uint8_t*)visual->texture.upload;
        const uint8_t* src = (const uint8_t*)field->data;
        uint64_t src_bpr = _field_default_bytes_per_row(&field->desc);
        uint64_t row_bytes = (uint64_t)out_region->width * 4ull;
        for (uint32_t y = 0; y < out_region->height; y++)
        {
            uint64_t src_offset =
                (uint64_t)(out_region->y + y) * src_bpr + (uint64_t)out_region->x * 4ull;
            uint64_t dst_offset = (uint64_t)y * row_bytes;
            dvz_memcpy(dst + dst_offset, row_bytes, src + src_offset, row_bytes);
        }
        *out_data = visual->texture.upload;
        return true;
    }
    if (!_field_format_is_scalar(field->desc.format))
    {
        log_error("image or glyph visual does not support sampled field format %d", (int)field->desc.format);
        return false;
    }
    if (visual->scale == NULL || visual->scale->colormap == NULL)
    {
        log_error("scalar image field requires a bound scale with a colormap");
        return false;
    }

    uint64_t pixel_count = 0;
    uint64_t rgba_size = 0;
    if (_dvz_mul_u64_overflows(field->desc.width, field->desc.height, &pixel_count) ||
        _dvz_mul_u64_overflows(pixel_count, 4, &rgba_size))
    {
        log_error("scalar image field RGBA staging size overflow");
        return false;
    }
    if (visual->texture.rgba == NULL || visual->texture.rgba_size != rgba_size)
    {
        if (visual->texture.rgba != NULL)
            dvz_free(visual->texture.rgba);
        visual->texture.rgba = dvz_calloc(rgba_size, 1);
        if (visual->texture.rgba == NULL)
        {
            visual->texture.rgba_size = 0;
            log_error("scalar image field RGBA staging allocation failed");
            return false;
        }
        visual->texture.rgba_size = rgba_size;
    }

    uint8_t* rgba = (uint8_t*)visual->texture.rgba;
    double domain_min = 0.0;
    double domain_max = 1.0;
    if (visual->scale->has_view_range)
    {
        domain_min = visual->scale->view_min;
        domain_max = visual->scale->view_max;
    }
    else if (visual->scale->has_domain)
    {
        domain_min = visual->scale->domain_min;
        domain_max = visual->scale->domain_max;
    }
    double denom = domain_max - domain_min;
    if (denom == 0.0)
        denom = 1.0;

    for (uint32_t y = out_region->y; y < out_region->y + out_region->height; y++)
    {
        for (uint32_t x = out_region->x; x < out_region->x + out_region->width; x++)
        {
            uint64_t i = (uint64_t)y * field->desc.width + x;
            double value = 0.0;
            if (!_field_read_scalar(field, i, &value))
            {
                log_error("failed to sample scalar field format %d", (int)field->desc.format);
                return false;
            }
            double t = (value - domain_min) / denom;
            _scene_color_from_colormap(visual->scale->colormap, t, &rgba[4 * i]);
        }
    }

    if (out_region->x == 0 && out_region->y == 0 && out_region->width == field->desc.width &&
        out_region->height == field->desc.height)
    {
        *out_data = visual->texture.rgba;
        return true;
    }

    uint64_t upload_size = 0;
    if (!_field_region_byte_size(DVZ_FIELD_FORMAT_RGBA8_UNORM, out_region, &upload_size) ||
        !_visual_texture_ensure_upload(&visual->texture, upload_size))
    {
        log_error("scalar image field upload scratch allocation failed");
        return false;
    }
    uint8_t* dst = (uint8_t*)visual->texture.upload;
    uint64_t src_bpr = field->desc.width * 4ull;
    uint64_t row_bytes = (uint64_t)out_region->width * 4ull;
    for (uint32_t y = 0; y < out_region->height; y++)
    {
        uint64_t src_offset =
            (uint64_t)(out_region->y + y) * src_bpr + (uint64_t)out_region->x * 4ull;
        uint64_t dst_offset = (uint64_t)y * row_bytes;
        dvz_memcpy(dst + dst_offset, row_bytes, rgba + src_offset, row_bytes);
    }
    *out_data = visual->texture.upload;
    return true;
}
