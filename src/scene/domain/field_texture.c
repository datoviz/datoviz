/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene field texture helpers                                                                          */
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




/**
 * Ensure one image-like visual owns a compatible sampled field.
 *
 * @param visual the image or glyph visual
 * @param format the field format
 * @param semantic the field semantic
 * @param width the field width
 * @param height the field height
 * @return the owned field, or NULL on error
 */
static DvzSampledField* _scene_ensure_owned_image_field(
    DvzVisual* visual, DvzFieldFormat format, DvzFieldSemantic semantic, uint32_t width,
    uint32_t height)
{
    ANN(visual);
    DvzSampledField* field = visual->field_owned ? visual->field : NULL;
    if (field != NULL && field->desc.format == format && field->desc.width == width &&
        field->desc.height == height && field->desc.depth == 1)
    {
        return field;
    }
    if (field != NULL)
        dvz_sampled_field_destroy(field);
    field = dvz_sampled_field(
        visual->scene, &(DvzSampledFieldDesc){
                           .dim = DVZ_FIELD_DIM_2D,
                           .format = format,
                           .semantic = semantic,
                           .width = width,
                           .height = height,
                           .depth = 1,
                       });
    return field;
}





/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Advance a retained visual texture version.
 *
 * @param visual the visual whose texture changed
 */
static void _visual_texture_bump_version(DvzVisual* visual)
{
    ANN(visual);
    visual->texture.version =
        visual->texture.version == UINT64_MAX ? 1 : visual->texture.version + 1;
}

/*************************************************************************************************/
/*  Visual texture API                                                                           */
/*************************************************************************************************/

bool dvz_visual_set_field(DvzVisual* visual, const char* slot_name, DvzSampledField* field)
{
    ANN(visual);
    ANN(slot_name);
    if (field != NULL && field->scene != visual->scene)
    {
        log_error("cannot bind a sampled field from a different scene");
        return false;
    }
    if (visual->type != DVZ_VISUAL_TYPE_IMAGE && visual->type != DVZ_VISUAL_TYPE_GLYPH &&
        visual->type != DVZ_VISUAL_TYPE_VOLUME && visual->type != DVZ_VISUAL_TYPE_LABELS &&
        visual->type != DVZ_VISUAL_TYPE_MESH)
    {
        log_error(
            "dvz_visual_set_field is only supported for image, glyph, volume, labels, and mesh visuals");
        return false;
    }
    bool mesh_texture_slot =
        visual->type == DVZ_VISUAL_TYPE_MESH && strcmp(slot_name, "texture") == 0;
    if (strcmp(slot_name, "field") != 0 && !mesh_texture_slot)
    {
        log_error(
            "unsupported visual field slot '%s' (expected 'field' or mesh 'texture')",
            slot_name);
        return false;
    }
    if (field != NULL &&
        (visual->type == DVZ_VISUAL_TYPE_IMAGE || visual->type == DVZ_VISUAL_TYPE_GLYPH ||
         visual->type == DVZ_VISUAL_TYPE_LABELS || visual->type == DVZ_VISUAL_TYPE_MESH) &&
        field->desc.dim != DVZ_FIELD_DIM_2D)
    {
        log_error("image, glyph, labels, and mesh visuals require a 2D sampled field");
        return false;
    }
    if (field != NULL && visual->type == DVZ_VISUAL_TYPE_MESH &&
        field->desc.format != DVZ_FIELD_FORMAT_RGBA8_UNORM)
    {
        log_error("mesh texture fields require RGBA8_UNORM format in the first slice");
        return false;
    }
    if (field != NULL && visual->type == DVZ_VISUAL_TYPE_LABELS &&
        field->desc.semantic != DVZ_FIELD_SEMANTIC_LABEL)
    {
        log_error("labels visuals require a sampled field with LABEL semantic");
        return false;
    }
    DvzSceneSampleProfile profile = {0};
    bool supported_profile =
        field != NULL &&
        _scene_sample_profile_resolve(
            field->desc.format, field->desc.semantic, field->desc.dim, &profile);
    if (field != NULL && visual->type == DVZ_VISUAL_TYPE_LABELS && !supported_profile)
    {
        log_error("labels visuals require an R8/R16/R32 signed or unsigned integer field");
        return false;
    }
    if (field != NULL && visual->type == DVZ_VISUAL_TYPE_VOLUME &&
        field->desc.dim != DVZ_FIELD_DIM_3D)
    {
        log_error("volume visuals require a 3D sampled field");
        return false;
    }
    if (
        field != NULL && visual->type == DVZ_VISUAL_TYPE_VOLUME && supported_profile &&
        _scene_sample_profile_is_integer_label(&profile) &&
        visual->volume.render_mode == DVZ_VOLUME_RENDER_MIP)
    {
        log_error("label volumes only support slice and composite render modes");
        return false;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "bind sampled field"))
        return false;

    if (visual->field != field)
        _scene_release_visual_field(visual);
    if (field != NULL)
    {
        _visual_binding_assign(visual, DVZ_VISUAL_BINDING_FIELD, slot_name, field, false);
        _scene_visual_texture_mark_clean(visual);
        visual->texture.dirty = true;
        _visual_texture_bump_version(visual);
    }
    else
    {
        _visual_binding_clear(visual, DVZ_VISUAL_BINDING_FIELD);
    }
    _scene_notify_visual_changed(visual);
    return true;
}



int dvz_visual_set_texture(
    DvzVisual* visual, const void* rgba, uint32_t width, uint32_t height)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_IMAGE && visual->type != DVZ_VISUAL_TYPE_GLYPH)
    {
        log_error("dvz_visual_set_texture is only supported for image and glyph visuals");
        return -1;
    }
    if (rgba == NULL || width == 0 || height == 0)
    {
        log_error("dvz_visual_set_texture: NULL data or zero extent (%ux%u)", width, height);
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "set image texture"))
        return -1;
    DvzSampledField* field = _scene_ensure_owned_image_field(
        visual, DVZ_FIELD_FORMAT_RGBA8_UNORM, DVZ_FIELD_SEMANTIC_COLOR, width, height);
    if (field == NULL)
        return -1;
    if (!dvz_sampled_field_set_data(
            field, &(DvzFieldDataView){
                       .data = rgba,
                       .bytes_per_row = (uint64_t)width * 4u,
                       .rows_per_image = height,
                   }))
        return -1;
    if (!dvz_visual_set_field(visual, "field", field))
        return -1;
    _visual_binding_assign(visual, DVZ_VISUAL_BINDING_FIELD, "field", field, true);
    return 0;
}


/**
 * Attach a 2D scalar F32 texture to an image or glyph visual.
 *
 * The scalar data must remain valid until emit time. The bound scale and
 * colormap are applied on the CPU during emit to produce the RGBA texture used
 * by the current first-slice image runtime path.
 *
 * @param visual the visual (must be of type IMAGE or GLYPH)
 * @param values scalar F32 pixel data, tightly packed, row-major
 * @param width the texture width in pixels
 * @param height the texture height in pixels
 * @return 0 on success, -1 on error
 */
int dvz_visual_set_texture_f32(
    DvzVisual* visual, const float* values, uint32_t width, uint32_t height)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_IMAGE && visual->type != DVZ_VISUAL_TYPE_GLYPH)
    {
        log_error("dvz_visual_set_texture_f32 is only supported for image and glyph visuals");
        return -1;
    }
    if (values == NULL || width == 0 || height == 0)
    {
        log_error("dvz_visual_set_texture_f32: NULL data or zero extent (%ux%u)", width, height);
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "set scalar image texture"))
        return -1;
    DvzSampledField* field = _scene_ensure_owned_image_field(
        visual, DVZ_FIELD_FORMAT_R32_FLOAT, DVZ_FIELD_SEMANTIC_SCALAR, width, height);
    if (field == NULL)
        return -1;
    if (!dvz_sampled_field_set_data(
            field, &(DvzFieldDataView){
                       .data = values,
                       .bytes_per_row = (uint64_t)width * sizeof(float),
                       .rows_per_image = height,
                   }))
        return -1;
    if (!dvz_visual_set_field(visual, "field", field))
        return -1;
    _visual_binding_assign(visual, DVZ_VISUAL_BINDING_FIELD, "field", field, true);
    return 0;
}




/**
 * Mark one visual texture upload state as clean after a successful emit.
 *
 * @param visual the visual
 */
void _scene_visual_texture_mark_clean(DvzVisual* visual)
{
    ANN(visual);
    visual->texture.dirty = false;
    visual->texture.field_dirty = false;
    visual->texture.field_dirty_full = false;
    dvz_memset(
        &visual->texture.field_dirty_region, sizeof(DvzFieldRegion), 0, sizeof(DvzFieldRegion));
}



/**
 * Mark one visual texture state as requiring a full field upload.
 *
 * @param visual the visual
 * @param desc the sampled field descriptor
 */
void _scene_visual_texture_mark_full_dirty(
    DvzVisual* visual, const DvzSampledFieldDesc* desc)
{
    ANN(visual);
    ANN(desc);
    visual->texture.dirty = true;
    visual->texture.field_dirty = true;
    visual->texture.field_dirty_full = true;
    visual->texture.field_dirty_region = _field_full_region(desc);
    _visual_texture_bump_version(visual);
}



/**
 * Mark one visual texture state as dirty for one field subregion.
 *
 * @param visual the visual
 * @param desc the sampled field descriptor
 * @param region the dirty field region
 */
void _scene_visual_texture_mark_region_dirty(
    DvzVisual* visual, const DvzSampledFieldDesc* desc, DvzFieldRegion region)
{
    ANN(visual);
    ANN(desc);
    visual->texture.dirty = true;
    _visual_texture_bump_version(visual);
    if (!visual->texture.field_dirty)
    {
        visual->texture.field_dirty = true;
        visual->texture.field_dirty_full = false;
        visual->texture.field_dirty_region = region;
        return;
    }
    if (visual->texture.field_dirty_full)
    {
        visual->texture.field_dirty_region = _field_full_region(desc);
        return;
    }
    if (!_field_regions_union(
            &visual->texture.field_dirty_region, &region, &visual->texture.field_dirty_region))
    {
        visual->texture.field_dirty_full = true;
        visual->texture.field_dirty_region = _field_full_region(desc);
    }
}



void _scene_release_visual_field(DvzVisual* visual)
{
    if (visual == NULL)
        return;
    DvzSampledField* field = visual->field;
    const DvzVisualBinding* binding = _visual_binding_const(visual, DVZ_VISUAL_BINDING_FIELD);
    bool owned = binding != NULL ? binding->owned : visual->field_owned;
    _visual_binding_clear(visual, DVZ_VISUAL_BINDING_FIELD);
    _scene_visual_texture_mark_clean(visual);
    if (visual->texture.upload != NULL)
    {
        dvz_free(visual->texture.upload);
        visual->texture.upload = NULL;
        visual->texture.upload_size = 0;
    }
    if (owned && field != NULL)
        dvz_sampled_field_destroy(field);
}


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
