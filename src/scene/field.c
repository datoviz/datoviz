/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene field helpers                                                                          */
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



/*************************************************************************************************/
/*  Function prototypes                                                                          */
/*************************************************************************************************/

static double _half_to_double(uint16_t bits);

static void _field_copy_full_data(
    const DvzSampledFieldDesc* desc, const DvzFieldDataView* view, void* dst);

static DvzSampledField* _scene_alloc_field_slot(DvzScene* scene);

static DvzSceneBuffer* _scene_alloc_buffer_slot(DvzScene* scene);

static void _scene_mark_field_region_dirty(
    DvzSampledField* field, DvzFieldRegion region, bool full);

static bool _field_ensure_upload(DvzSampledField* field, uint64_t byte_size);



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

uint32_t _scene_field_index(const DvzScene* scene, const DvzSampledField* field)
{
    if (scene == NULL || field == NULL)
        return UINT32_MAX;
    for (uint32_t i = 0; i < DVZ_SCENE_MAX_FIELDS; i++)
    {
        if (&scene->fields[i] == field && field->scene == scene)
            return i;
    }
    return UINT32_MAX;
}


uint32_t _scene_buffer_index(const DvzScene* scene, const DvzSceneBuffer* buffer)
{
    if (scene == NULL || buffer == NULL)
        return UINT32_MAX;
    for (uint32_t i = 0; i < scene->buffer_count; i++)
    {
        if (&scene->buffers[i] == buffer && buffer->scene == scene)
            return i;
    }
    return UINT32_MAX;
}


/**
 * Return the stable scene index of a scale.
 *
 * @param scene the owning scene
 * @param scale the scale
 * @return the scale index, or UINT32_MAX when absent
 */
uint32_t _scene_scale_index(const DvzScene* scene, const DvzScale* scale)
{
    if (scene == NULL || scale == NULL)
        return UINT32_MAX;
    for (uint32_t i = 0; i < scene->scale_count; i++)
    {
        if (&scene->scales[i] == scale && scale->scene == scene)
            return i;
    }
    return UINT32_MAX;
}


static bool _field_format_supported(DvzFieldFormat format)
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


static bool _field_format_is_rgba8(DvzFieldFormat format)
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


static bool _field_expected_data_size(const DvzSampledFieldDesc* desc, uint64_t* out_size)
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


static uint64_t _field_default_bytes_per_row(const DvzSampledFieldDesc* desc)
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


static uint64_t _field_default_rows_per_image(const DvzSampledFieldDesc* desc)
{
    ANN(desc);
    return desc->height;
}


static DvzFieldRegion _field_full_region(const DvzSampledFieldDesc* desc)
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


static bool _field_regions_union(
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


static bool _field_data_view_valid(
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
static void _field_copy_full_data(
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


static bool _field_read_scalar(
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
 * Ensure one sampled field has scratch storage for a packed upload.
 *
 * @param field the sampled field
 * @param byte_size the required scratch size
 * @return whether the scratch storage is available
 */
static bool _field_ensure_upload(DvzSampledField* field, uint64_t byte_size)
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



/**
 * Reset one sampled-field slot to its empty state.
 *
 * @param field the field slot
 */
void _scene_field_reset(DvzSampledField* field)
{
    if (field == NULL)
        return;
    if (field->data != NULL)
    {
        dvz_free(field->data);
        field->data = NULL;
    }
    if (field->upload != NULL)
    {
        dvz_free(field->upload);
        field->upload = NULL;
        field->upload_size = 0;
    }
    dvz_memset(field, sizeof(DvzSampledField), 0, sizeof(DvzSampledField));
}



/**
 * Reset one scene-buffer slot to its empty state.
 *
 * @param buffer the buffer slot
 */
void _scene_buffer_reset(DvzSceneBuffer* buffer)
{
    if (buffer == NULL)
        return;
    if (buffer->data != NULL)
    {
        dvz_free(buffer->data);
        buffer->data = NULL;
    }
    dvz_memset(buffer, sizeof(DvzSceneBuffer), 0, sizeof(DvzSceneBuffer));
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
/*  Sampled fields                                                                               */
/*************************************************************************************************/

/**
 * Create a scene-owned sampled field.
 *
 * @param scene the scene
 * @param desc the field descriptor
 * @return the sampled field, or NULL on error
 */
DvzSampledField* dvz_sampled_field(DvzScene* scene, const DvzSampledFieldDesc* desc)
{
    ANN(scene);
    ANN(desc);
    if (!_field_format_supported(desc->format))
    {
        log_error("unsupported sampled field format %d", (int)desc->format);
        return NULL;
    }
    if (desc->dim != DVZ_FIELD_DIM_2D && desc->dim != DVZ_FIELD_DIM_3D)
    {
        log_error("unsupported sampled field dimensionality %d", (int)desc->dim);
        return NULL;
    }
    if (desc->width == 0 || desc->height == 0 || desc->depth == 0)
    {
        log_error("sampled field dimensions must be non-zero");
        return NULL;
    }
    if (desc->dim == DVZ_FIELD_DIM_2D && desc->depth != 1)
    {
        log_error("2D sampled fields must use depth=1");
        return NULL;
    }
    uint64_t data_size = 0;
    if (!_field_expected_data_size(desc, &data_size))
    {
        log_error("sampled field size overflow");
        return NULL;
    }

    DvzSampledField* field = _scene_alloc_field_slot(scene);
    if (field == NULL)
    {
        log_error("maximum sampled field count reached");
        return NULL;
    }
    field->desc = *desc;
    field->data_size = data_size;
    field->geometry.axis_order[0] = 0;
    field->geometry.axis_order[1] = 1;
    field->geometry.axis_order[2] = 2;
    field->geometry.spacing[0] = 1.0;
    field->geometry.spacing[1] = 1.0;
    field->geometry.spacing[2] = 1.0;
    field->dirty = false;
    field->dirty_full = false;
    return field;

}


/**
 * Destroy a sampled field.
 *
 * @param field the sampled field
 * @return true on success, false on error
 */
bool dvz_sampled_field_destroy(DvzSampledField* field)
{
    if (field == NULL)
        return false;
    if (!_scene_visual_mutation_allowed(field->scene, "destroy sampled field"))
        return false;
    DvzScene* scene = field->scene;
    if (scene != NULL)
    {
        for (uint32_t i = 0; i < scene->visual_count; i++)
        {
            DvzVisual* visual = &scene->visuals[i];
            if (visual->field == field)
            {
                _visual_binding_clear(visual, DVZ_VISUAL_BINDING_FIELD);
                _scene_visual_texture_mark_clean(visual);
                if (visual->texture.upload != NULL)
                {
                    dvz_free(visual->texture.upload);
                    visual->texture.upload = NULL;
                    visual->texture.upload_size = 0;
                }
            }
        }
    }
    _scene_field_reset(field);
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
    if (region.x + region.width > field->desc.width || region.y + region.height > field->desc.height ||
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
                ((uint64_t)(region.z + z) * field->desc.height + (region.y + y)) * dst_bytes_per_row +
                (uint64_t)region.x * (uint64_t)bytes_per_texel;
            dvz_memcpy(dst + dst_offset, copy_bytes, src + src_offset, copy_bytes);
        }
    }
    _scene_mark_field_region_dirty(field, region, false);
    return true;
}



/**
 * Update the field geometry metadata.
 *
 * @param field the sampled field
 * @param geometry the geometry descriptor
 * @return true on success, false on error
 */
bool dvz_sampled_field_set_geometry(
    DvzSampledField* field, const DvzFieldGeometry* geometry)
{
    ANN(field);
    ANN(geometry);
    if (!_scene_visual_mutation_allowed(field->scene, "update sampled field geometry"))
        return false;
    field->geometry = *geometry;
    return true;
}



/**
 * Return the immutable field descriptor.
 *
 * @param field the sampled field
 * @return the descriptor, or NULL on error
 */
const DvzSampledFieldDesc* dvz_sampled_field_desc(const DvzSampledField* field)
{
    return field != NULL ? &field->desc : NULL;
}



/**
 * Bind a scene-owned sampled field to a named visual slot.
 *
 * @param visual the visual
 * @param slot_name the slot name
 * @param field the field, or NULL to clear the binding
 * @return true on success, false on error
 */
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
        visual->type != DVZ_VISUAL_TYPE_VOLUME)
    {
        log_error("dvz_visual_set_field is only supported for image, glyph, and volume visuals");
        return false;
    }
    if (strcmp(slot_name, "field") != 0)
    {
        log_error("unsupported visual field slot '%s' (expected 'field')", slot_name);
        return false;
    }
    if (field != NULL &&
        (visual->type == DVZ_VISUAL_TYPE_IMAGE || visual->type == DVZ_VISUAL_TYPE_GLYPH) &&
        field->desc.dim != DVZ_FIELD_DIM_2D)
    {
        log_error("image and glyph visuals require a 2D sampled field");
        return false;
    }
    if (field != NULL && visual->type == DVZ_VISUAL_TYPE_VOLUME &&
        field->desc.dim != DVZ_FIELD_DIM_3D)
    {
        log_error("volume visuals require a 3D sampled field");
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



/**
 * Create a scene-owned buffer resource.
 *
 * @param scene the scene
 * @param desc the buffer descriptor
 * @return the buffer, or NULL on error
 */
DvzSceneBuffer* dvz_scene_buffer(DvzScene* scene, const DvzSceneBufferDesc* desc)
{
    ANN(scene);
    ANN(desc);
    if (desc->usage == 0)
    {
        log_error("scene buffer usage must be non-zero");
        return NULL;
    }
    if (desc->stride == 0)
    {
        log_error("scene buffer stride must be non-zero");
        return NULL;
    }
    if (scene->buffer_count >= DVZ_SCENE_MAX_BUFFERS)
    {
        log_error("maximum scene buffer count reached");
        return NULL;
    }
    DvzSceneBuffer* buffer = _scene_alloc_buffer_slot(scene);
    if (buffer == NULL)
    {
        log_error("maximum scene buffer count reached");
        return NULL;
    }
    buffer->desc = *desc;
    return buffer;
}



/**
 * Destroy a scene-owned buffer resource.
 *
 * @param buffer the buffer
 */
void dvz_scene_buffer_destroy(DvzSceneBuffer* buffer)
{
    if (buffer == NULL)
        return;
    if (!_scene_visual_mutation_allowed(buffer->scene, "destroy scene buffer"))
        return;
    DvzScene* scene = buffer->scene;
    if (scene != NULL)
    {
        for (uint32_t i = 0; i < scene->visual_count; i++)
        {
            DvzVisual* visual = &scene->visuals[i];
            if (visual->buffer == buffer)
                _scene_release_visual_buffer(visual);
            for (uint32_t ai = 0; ai < visual->attr_count; ai++)
            {
                if (visual->attrs[ai].buffer == buffer)
                {
                    visual->attrs[ai].buffer = NULL;
                    visual->attrs[ai].buffer_byte_offset = 0;
                    if (visual->attrs[ai].data == NULL)
                        visual->attrs[ai].item_count = 0;
                }
            }
        }
    }
    _scene_buffer_reset(buffer);
}



/**
 * Replace the full payload of a scene-owned buffer resource.
 *
 * @param buffer the buffer
 * @param data the packed payload
 * @param byte_size the payload size
 * @return true on success, false on error
 */
bool dvz_scene_buffer_set_data(DvzSceneBuffer* buffer, const void* data, uint64_t byte_size)
{
    ANN(buffer);
    ANN(data);
    if (!_scene_visual_mutation_allowed(buffer->scene, "replace scene buffer data"))
        return false;
    if (byte_size == 0)
    {
        log_error("scene buffer payload size must be non-zero");
        return false;
    }
    if (byte_size % buffer->desc.stride != 0)
    {
        log_error(
            "scene buffer payload size %" PRIu64 " is not aligned to stride %u", byte_size,
            buffer->desc.stride);
        return false;
    }
    if (buffer->data != NULL && buffer->desc.byte_size != byte_size)
    {
        dvz_free(buffer->data);
        buffer->data = NULL;
    }
    if (buffer->data == NULL)
    {
        buffer->data = dvz_malloc(byte_size);
        if (buffer->data == NULL)
        {
            log_error("scene buffer allocation failed for %" PRIu64 " bytes", byte_size);
            return false;
        }
    }
    dvz_memcpy(buffer->data, byte_size, data, byte_size);
    buffer->desc.byte_size = byte_size;
    buffer->dirty = true;
    _scene_notify_buffer_changed(buffer);
    return true;
}



/**
 * Return the immutable buffer descriptor.
 *
 * @param buffer the buffer
 * @return the descriptor, or NULL on error
 */
const DvzSceneBufferDesc* dvz_scene_buffer_desc(const DvzSceneBuffer* buffer)
{
    return buffer != NULL ? &buffer->desc : NULL;
}



/**
 * Bind a scene-owned buffer to a named visual slot.
 *
 * @param visual the visual
 * @param slot_name the slot name
 * @param buffer the buffer, or NULL to clear the binding
 * @return true on success, false on error
 */
bool dvz_visual_set_buffer(DvzVisual* visual, const char* slot_name, DvzSceneBuffer* buffer)
{
    ANN(visual);
    ANN(slot_name);
    if (buffer != NULL && buffer->scene != visual->scene)
    {
        log_error("cannot bind a buffer from a different scene");
        return false;
    }
    if (visual->type != DVZ_VISUAL_TYPE_PRIMITIVE && visual->type != DVZ_VISUAL_TYPE_MESH)
    {
        log_error("dvz_visual_set_buffer is only supported for primitive and mesh visuals in the first slice");
        return false;
    }
    if (strcmp(slot_name, "index") != 0)
    {
        log_error("unsupported indexed buffer slot '%s' (expected 'index')", slot_name);
        return false;
    }
    if (buffer != NULL)
    {
        if ((buffer->desc.usage & DVZ_SCENE_BUFFER_USAGE_INDEX) == 0)
        {
            log_error("indexed draw slot requires a buffer with INDEX usage");
            return false;
        }
        if (buffer->desc.stride != sizeof(uint16_t) && buffer->desc.stride != sizeof(uint32_t))
        {
            log_error("indexed draw buffers require stride 2 or 4 bytes");
            return false;
        }
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "bind scene buffer"))
        return false;
    _scene_release_visual_buffer(visual);
    if (buffer != NULL)
        _visual_binding_assign(visual, DVZ_VISUAL_BINDING_BUFFER, slot_name, buffer, false);
    else
        _visual_binding_clear(visual, DVZ_VISUAL_BINDING_BUFFER);
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
static void _scene_visual_texture_mark_full_dirty(
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
static void _scene_visual_texture_mark_region_dirty(
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



/**
 * Allocate one free sampled-field slot from a scene.
 *
 * @param scene the scene
 * @return the zero-initialized slot, or NULL when full
 */
static DvzSampledField* _scene_alloc_field_slot(DvzScene* scene)
{
    ANN(scene);
    for (uint32_t i = 0; i < DVZ_SCENE_MAX_FIELDS; i++)
    {
        DvzSampledField* field = &scene->fields[i];
        if (field->scene != NULL)
            continue;
        dvz_memset(field, sizeof(DvzSampledField), 0, sizeof(DvzSampledField));
        field->scene = scene;
        if (i + 1 > scene->field_count)
            scene->field_count = i + 1;
        return field;
    }
    return NULL;
}



/**
 * Allocate one free scene-buffer slot from a scene.
 *
 * @param scene the scene
 * @return the zero-initialized slot, or NULL when full
 */
static DvzSceneBuffer* _scene_alloc_buffer_slot(DvzScene* scene)
{
    ANN(scene);
    for (uint32_t i = 0; i < DVZ_SCENE_MAX_BUFFERS; i++)
    {
        DvzSceneBuffer* buffer = &scene->buffers[i];
        if (buffer->scene != NULL)
            continue;
        dvz_memset(buffer, sizeof(DvzSceneBuffer), 0, sizeof(DvzSceneBuffer));
        buffer->scene = scene;
        if (i + 1 > scene->buffer_count)
            scene->buffer_count = i + 1;
        return buffer;
    }
    return NULL;
}





static void _scene_mark_field_dirty(DvzSampledField* field)
{
    if (field == NULL || field->scene == NULL)
        return;
    _scene_mark_field_region_dirty(field, _field_full_region(&field->desc), true);
}


static void _scene_mark_field_region_dirty(DvzSampledField* field, DvzFieldRegion region, bool full)
{
    if (field == NULL || field->scene == NULL)
        return;
    if (full)
    {
        region = _field_full_region(&field->desc);
        field->dirty_region = region;
        field->dirty = true;
        field->dirty_full = true;
    }
    else if (!field->dirty)
    {
        field->dirty_region = region;
        field->dirty = true;
        field->dirty_full = false;
    }
    else if (field->dirty_full)
    {
        field->dirty_region = _field_full_region(&field->desc);
    }
    else if (!_field_regions_union(&field->dirty_region, &region, &field->dirty_region))
    {
        field->dirty_region = region;
        field->dirty_full = true;
    }
    else
    {
        field->dirty_full = false;
    }
    field->dirty = true;
    DvzScene* scene = field->scene;
    for (uint32_t i = 0; i < scene->visual_count; i++)
    {
        DvzVisual* visual = &scene->visuals[i];
        if (visual->scene == scene && visual->field == field)
        {
            if (full)
            {
                _scene_visual_texture_mark_full_dirty(visual, &field->desc);
            }
            else
                _scene_visual_texture_mark_region_dirty(visual, &field->desc, region);
            _scene_notify_visual_changed(visual);
        }
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


void _scene_release_visual_buffer(DvzVisual* visual)
{
    if (visual == NULL)
        return;
    _visual_binding_clear(visual, DVZ_VISUAL_BINDING_BUFFER);
}




void _scene_refresh_field_dirty_state(DvzScene* scene, DvzSampledField* field)
{
    if (scene == NULL || field == NULL || field->scene != scene)
        return;
    bool any_pending = false;
    bool any_full = false;
    DvzFieldRegion merged = {0};
    for (uint32_t i = 0; i < scene->visual_count; i++)
    {
        const DvzVisual* visual = &scene->visuals[i];
        if (visual->scene != scene || visual->field != field || !visual->texture.field_dirty)
            continue;
        any_pending = true;
        if (visual->texture.field_dirty_full)
        {
            any_full = true;
            merged = _field_full_region(&field->desc);
            break;
        }
        if (merged.width == 0 && merged.height == 0 && merged.depth == 0)
            merged = visual->texture.field_dirty_region;
        else if (!_field_regions_union(&merged, &visual->texture.field_dirty_region, &merged))
        {
            any_full = true;
            merged = _field_full_region(&field->desc);
            break;
        }
    }
    field->dirty = any_pending;
    field->dirty_full = any_full;
    if (any_pending)
        field->dirty_region = merged;
    else
        dvz_memset(&field->dirty_region, sizeof(DvzFieldRegion), 0, sizeof(DvzFieldRegion));
}



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
            uint64_t dst_offset =
                ((uint64_t)z * out_region->height + y) * row_bytes;
            dvz_memcpy(dst + dst_offset, row_bytes, src + src_offset, row_bytes);
        }
    }
    *out_data = field->upload;
    return true;
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
