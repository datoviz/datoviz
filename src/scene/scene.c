/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene graph — DvzScene / DvzFigure / DvzPanel / DvzVisual                                  */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_json.h"
#include "_log.h"
#include "_overflow.h"
#include "datoviz/math/_cglm.h"
#include "../drp2/_stream.h"
#include "_scene.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static const char* _visual_type_name(DvzVisualType type);

static void _scene_stream_release(void* owner);

static bool _scene_stream_register(DvzScene* scene, DvzDrp2CommandStream* stream);

static bool _scene_has_live_streams(const DvzScene* scene);

static bool _scene_visual_mutation_allowed(const DvzScene* scene, const char* action);

static void _format_state_copy(DvzSceneFormatState* dst, const DvzFormatDesc* src);

static void _scene_mark_scale_dirty(DvzScale* scale);

static void _scene_mark_colormap_dirty(DvzColormap* colormap);

static uint32_t _scene_field_index(const DvzScene* scene, const DvzSampledField* field);

static void _scene_mark_field_dirty(DvzSampledField* field);

static void _scene_mark_field_region_dirty(DvzSampledField* field, DvzFieldRegion region, bool full);

static void _scene_release_visual_field(DvzVisual* visual);

static bool _field_format_supported(DvzFieldFormat format);

static bool _field_format_is_scalar(DvzFieldFormat format);

static bool _field_format_is_rgba8(DvzFieldFormat format);

static bool _field_format_bytes_per_texel(DvzFieldFormat format, uint32_t* out_bytes);

static bool _field_expected_data_size(const DvzSampledFieldDesc* desc, uint64_t* out_size);

static uint64_t _field_default_bytes_per_row(const DvzSampledFieldDesc* desc);

static uint64_t _field_default_rows_per_image(const DvzSampledFieldDesc* desc);

static DvzFieldRegion _field_full_region(const DvzSampledFieldDesc* desc);

static bool _field_regions_union(
    const DvzFieldRegion* a, const DvzFieldRegion* b, DvzFieldRegion* out);

static bool _field_region_byte_size(
    DvzFieldFormat format, const DvzFieldRegion* region, uint64_t* out_size);

static bool _field_data_view_valid(
    const DvzSampledFieldDesc* desc, const DvzFieldDataView* view, const DvzFieldRegion* region);

static bool _field_read_scalar(
    const DvzSampledField* field, uint64_t sample_index, double* out_value);

static void _scene_refresh_field_dirty_state(DvzScene* scene, DvzSampledField* field);

static bool _visual_texture_ensure_upload(DvzVisualTexture* texture, uint64_t byte_size);

static double _half_to_double(uint16_t bits);

static bool _scene_color_from_colormap(
    const DvzColormap* colormap, double t, uint8_t out_rgba[4]);

static bool _scene_prepare_image_texture(
    DvzVisual* visual, DvzFieldRegion* out_region, const void** out_data);



static uint32_t _attr_item_size(DvzVisualType type, const char* name)
{
    switch (type)
    {
    case DVZ_VISUAL_TYPE_POINT:
        if (strcmp(name, "position") == 0) return 3 * sizeof(float);
        if (strcmp(name, "color") == 0)    return 4 * sizeof(uint8_t);
        if (strcmp(name, "size") == 0)     return sizeof(float);
        break;
    case DVZ_VISUAL_TYPE_PRIMITIVE:
    case DVZ_VISUAL_TYPE_PATH:
        if (strcmp(name, "position") == 0) return 3 * sizeof(float);
        if (strcmp(name, "color") == 0)    return 4 * sizeof(uint8_t);
        break;
    case DVZ_VISUAL_TYPE_IMAGE:
        if (strcmp(name, "position") == 0)  return 3 * sizeof(float);
        if (strcmp(name, "texcoords") == 0) return 2 * sizeof(float);
        break;
    default:
        break;
    }
    return 0;
}



static bool _attr_supported(DvzVisualType type, const char* name, uint32_t* item_size)
{
    ANN(name);
    ANN(item_size);
    *item_size = _attr_item_size(type, name);
    if (*item_size != 0)
        return true;

    const char* expected = "position, color, size";
    if (type == DVZ_VISUAL_TYPE_PRIMITIVE || type == DVZ_VISUAL_TYPE_PATH)
        expected = "position, color";
    else if (type == DVZ_VISUAL_TYPE_IMAGE)
        expected = "position, texcoords";

    log_error(
        "unsupported %s visual attribute '%s' (expected one of: %s)",
        _visual_type_name(type), name, expected);
    return false;
}


static int _attr_index(const DvzVisual* visual, const char* name)
{
    ANN(visual);
    ANN(name);
    for (uint32_t i = 0; i < visual->attr_count; i++)
    {
        if (strcmp(visual->attrs[i].name, name) == 0)
            return (int)i;
    }
    return -1;
}


static DvzVisualAttr* _attr_get_or_create(DvzVisual* visual, const char* name, uint32_t item_size)
{
    ANN(visual);
    ANN(name);
    int idx = _attr_index(visual, name);
    if (idx >= 0)
        return &visual->attrs[idx];
    if (visual->attr_count >= DVZ_SCENE_MAX_ITEM_ATTRS)
        return NULL;
    DvzVisualAttr* attr = &visual->attrs[visual->attr_count++];
    dvz_strlcpy(attr->name, name, sizeof(attr->name));
    attr->item_size = item_size;
    return attr;
}



static bool _visual_attr_count_consistent(
    const DvzVisual* visual, const char* attr_name, uint32_t item_count)
{
    ANN(visual);
    ANN(attr_name);
    if (item_count == 0)
        return false;

    for (uint32_t i = 0; i < visual->attr_count; i++)
    {
        const DvzVisualAttr* attr = &visual->attrs[i];
        if (strcmp(attr->name, attr_name) == 0 || attr->item_count == 0 || attr->data == NULL)
            continue;
        if (attr->item_count == item_count)
            continue;

        log_error(
            "%s visual attribute '%s' item_count %u does not match existing attribute '%s' "
            "item_count %u",
            _visual_type_name(visual->type), attr_name, item_count, attr->name, attr->item_count);
        return false;
    }
    return true;
}



static bool _figure_visual_index(const DvzFigure* figure, const DvzVisual* visual, uint32_t* out_index)
{
    ANN(out_index);
    *out_index = 0;
    if (figure == NULL || figure->scene == NULL || visual == NULL)
        return false;
    if (visual->scene != figure->scene)
        return false;
    for (uint32_t i = 0; i < figure->scene->visual_count; i++)
    {
        if (&figure->scene->visuals[i] == visual)
        {
            *out_index = i;
            return true;
        }
    }
    return false;
}



static void _scene_stream_release(void* owner)
{
    DvzScene* scene = (DvzScene*)owner;
    if (scene == NULL)
        return;
    if (scene->outstanding_emitted_streams == 0)
    {
        log_error("scene emitted stream release underflow");
        return;
    }
    scene->outstanding_emitted_streams--;
}



static bool _scene_stream_register(DvzScene* scene, DvzDrp2CommandStream* stream)
{
    ANN(scene);
    ANN(stream);
    if (scene->outstanding_emitted_streams == UINT32_MAX)
    {
        log_error("scene emitted stream count overflow");
        return false;
    }
    scene->outstanding_emitted_streams++;
    stream->owner = scene;
    stream->owner_release = _scene_stream_release;
    stream->owner_released = false;
    return true;
}



static bool _scene_has_live_streams(const DvzScene* scene)
{
    return scene != NULL && scene->outstanding_emitted_streams > 0;
}



static bool _scene_visual_mutation_allowed(const DvzScene* scene, const char* action)
{
    ANN(action);
    if (!_scene_has_live_streams(scene))
        return true;
    log_error(
        "cannot %s while an emitted stream is still live; destroy the stream first", action);
    return false;
}


static void _format_state_copy(DvzSceneFormatState* dst, const DvzFormatDesc* src)
{
    ANN(dst);
    dvz_memset(dst, sizeof(DvzSceneFormatState), 0, sizeof(DvzSceneFormatState));
    if (src == NULL)
        return;
    dst->precision = src->precision;
    dst->scientific = src->scientific;
    dst->trim_trailing_zeros = src->trim_trailing_zeros;
    dst->show_unit = src->show_unit;
    if (src->unit != NULL)
        dvz_strlcpy(dst->unit, src->unit, sizeof(dst->unit));
    if (src->prefix != NULL)
        dvz_strlcpy(dst->prefix, src->prefix, sizeof(dst->prefix));
    if (src->suffix != NULL)
        dvz_strlcpy(dst->suffix, src->suffix, sizeof(dst->suffix));
}


static void _scene_mark_scale_dirty(DvzScale* scale)
{
    if (scale == NULL || scale->scene == NULL)
        return;
    DvzScene* scene = scale->scene;
    for (uint32_t i = 0; i < scene->visual_count; i++)
    {
        DvzVisual* visual = &scene->visuals[i];
        if (visual->scene != scene || visual->scale != scale)
            continue;
        if (visual->type == DVZ_VISUAL_TYPE_IMAGE && visual->field != NULL &&
            _field_format_is_scalar(visual->field->desc.format))
        {
            visual->texture.dirty = true;
            visual->texture.field_dirty = false;
            visual->texture.field_dirty_full = false;
            dvz_memset(
                &visual->texture.field_dirty_region, sizeof(DvzFieldRegion), 0,
                sizeof(DvzFieldRegion));
        }
    }
}


static uint32_t _scene_field_index(const DvzScene* scene, const DvzSampledField* field)
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
            visual->texture.dirty = true;
            if (full)
            {
                visual->texture.field_dirty = true;
                visual->texture.field_dirty_full = true;
                visual->texture.field_dirty_region = _field_full_region(&field->desc);
            }
            else if (!visual->texture.field_dirty)
            {
                visual->texture.field_dirty = true;
                visual->texture.field_dirty_full = false;
                visual->texture.field_dirty_region = region;
            }
            else if (visual->texture.field_dirty_full)
            {
                visual->texture.field_dirty_region = _field_full_region(&field->desc);
            }
            else if (!_field_regions_union(
                         &visual->texture.field_dirty_region, &region,
                         &visual->texture.field_dirty_region))
            {
                visual->texture.field_dirty_full = true;
                visual->texture.field_dirty_region = _field_full_region(&field->desc);
            }
        }
    }
}


static void _scene_release_visual_field(DvzVisual* visual)
{
    if (visual == NULL)
        return;
    DvzSampledField* field = visual->field;
    bool owned = visual->field_owned;
    visual->field = NULL;
    visual->field_owned = false;
    dvz_memset(visual->field_slot, sizeof(visual->field_slot), 0, sizeof(visual->field_slot));
    visual->texture.dirty = false;
    visual->texture.field_dirty = false;
    visual->texture.field_dirty_full = false;
    dvz_memset(&visual->texture.field_dirty_region, sizeof(DvzFieldRegion), 0, sizeof(DvzFieldRegion));
    if (visual->texture.upload != NULL)
    {
        dvz_free(visual->texture.upload);
        visual->texture.upload = NULL;
        visual->texture.upload_size = 0;
    }
    if (owned && field != NULL)
        dvz_sampled_field_destroy(field);
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


static bool _field_format_is_scalar(DvzFieldFormat format)
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


static bool _field_format_bytes_per_texel(DvzFieldFormat format, uint32_t* out_bytes)
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


static bool _field_region_byte_size(
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


static void _scene_refresh_field_dirty_state(DvzScene* scene, DvzSampledField* field)
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


static void _scene_mark_colormap_dirty(DvzColormap* colormap)
{
    if (colormap == NULL || colormap->scene == NULL)
        return;
    DvzScene* scene = colormap->scene;
    for (uint32_t i = 0; i < scene->scale_count; i++)
    {
        DvzScale* scale = &scene->scales[i];
        if (scale->scene == scene && scale->colormap == colormap)
            _scene_mark_scale_dirty(scale);
    }
}


static bool _scene_color_from_colormap(
    const DvzColormap* colormap, double t, uint8_t out_rgba[4])
{
    ANN(out_rgba);
    if (t < 0.0)
        t = 0.0;
    if (t > 1.0)
        t = 1.0;

    if (colormap != NULL && colormap->stop_count >= 2)
    {
        const DvzColormapStop* lo = &colormap->stops[0];
        const DvzColormapStop* hi = &colormap->stops[colormap->stop_count - 1];
        for (uint32_t i = 1; i < colormap->stop_count; i++)
        {
            if (t <= colormap->stops[i].position)
            {
                lo = &colormap->stops[i - 1];
                hi = &colormap->stops[i];
                break;
            }
        }
        double span = hi->position - lo->position;
        double u = span > 0.0 ? (t - lo->position) / span : 0.0;
        if (u < 0.0)
            u = 0.0;
        if (u > 1.0)
            u = 1.0;
        for (uint32_t c = 0; c < 4; c++)
        {
            double value = (1.0 - u) * lo->rgba[c] + u * hi->rgba[c];
            out_rgba[c] = (uint8_t)(value + 0.5);
        }
        return true;
    }

    uint8_t gray = (uint8_t)(255.0 * t + 0.5);
    out_rgba[0] = gray;
    out_rgba[1] = gray;
    out_rgba[2] = gray;
    out_rgba[3] = 255;
    return true;
}


static bool _scene_prepare_image_texture(
    DvzVisual* visual, DvzFieldRegion* out_region, const void** out_data)
{
    ANN(visual);
    ANN(out_region);
    ANN(out_data);
    if (visual->type != DVZ_VISUAL_TYPE_IMAGE)
        return false;
    if (visual->field == NULL)
    {
        log_error("image visual requires a bound sampled field");
        return false;
    }
    const DvzSampledField* field = visual->field;
    if (field->scene != visual->scene)
    {
        log_error("image visual field belongs to a different scene");
        return false;
    }
    if (field->desc.dim != DVZ_FIELD_DIM_2D)
    {
        log_error("image visuals require a 2D sampled field");
        return false;
    }
    if (field->data == NULL || field->desc.width == 0 || field->desc.height == 0)
    {
        log_error("image visual sampled field has no uploaded data");
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
        log_error("image visual does not support sampled field format %d", (int)field->desc.format);
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



/*************************************************************************************************/
/*  Scene                                                                                        */
/*************************************************************************************************/

DvzScene* dvz_scene(void)
{
    DvzScene* scene = (DvzScene*)dvz_calloc(1, sizeof(DvzScene));
    if (scene == NULL)
        return NULL;
    dvz_capability_snapshot_default(&scene->caps);
    scene->emitter = dvz_frame_plan_emitter();
    if (scene->emitter == NULL)
    {
        dvz_free(scene);
        return NULL;
    }
    return scene;
}


void dvz_scene_set_capabilities(DvzScene* scene, const DvzCapabilitySnapshot* caps)
{
    ANN(scene);
    ANN(caps);
    dvz_capability_snapshot_copy(&scene->caps, caps);
}


void dvz_scene_destroy(DvzScene* scene)
{
    if (scene == NULL)
        return;
    if (!_scene_visual_mutation_allowed(scene, "destroy scene-owned visual data"))
        return;
    /* Destroy all visuals first (free attribute data) */
    for (uint32_t i = 0; i < scene->visual_count; i++)
    {
        DvzVisual* v = &scene->visuals[i];
        for (uint32_t j = 0; j < v->attr_count; j++)
        {
            if (v->attrs[j].data != NULL)
            {
                dvz_free(v->attrs[j].data);
                v->attrs[j].data = NULL;
            }
        }
        if (v->texture.rgba != NULL)
        {
            dvz_free(v->texture.rgba);
            v->texture.rgba = NULL;
            v->texture.rgba_size = 0;
        }
        if (v->texture.upload != NULL)
        {
            dvz_free(v->texture.upload);
            v->texture.upload = NULL;
            v->texture.upload_size = 0;
        }
        v->field = NULL;
        v->field_owned = false;
    }
    for (uint32_t i = 0; i < DVZ_SCENE_MAX_FIELDS; i++)
    {
        DvzSampledField* field = &scene->fields[i];
        if (field->data != NULL)
        {
            dvz_free(field->data);
            field->data = NULL;
        }
        field->scene = NULL;
    }
    if (scene->emitter != NULL)
    {
        dvz_frame_plan_emitter_destroy(scene->emitter);
        scene->emitter = NULL;
    }
    dvz_free(scene);
}



/*************************************************************************************************/
/*  Figure                                                                                       */
/*************************************************************************************************/

DvzFigure* dvz_figure(DvzScene* scene, uint32_t width, uint32_t height, uint32_t flags)
{
    ANN(scene);
    if (scene->figure_count >= DVZ_SCENE_MAX_FIGURES)
        return NULL;
    DvzFigure* fig = &scene->figures[scene->figure_count++];
    fig->scene  = scene;
    fig->width  = width;
    fig->height = height;
    fig->flags  = flags;
    return fig;
}


void dvz_figure_destroy(DvzFigure* figure)
{
    if (figure == NULL)
        return;
    /* Mark slot as empty */
    figure->scene = NULL;
}


DvzDrp2CommandStream* dvz_figure_emit_ex(
    DvzFigure* figure, const DvzCapabilitySnapshot* caps, DvzDiagnosticReport* report,
    const DvzFramePlanEmitConfig* cfg)
{
    ANN(figure);
    ANN(figure->scene);
    ANN(figure->scene->emitter);
    DvzFramePlanEmitter* emitter = figure->scene->emitter;

    /* Use a stable figure_id from its position in the scene array */
    char figure_id[64];
    dvz_strlcpy(figure_id, "fig0", sizeof(figure_id));
    if (figure->scene != NULL)
    {
        for (uint32_t i = 0; i < figure->scene->figure_count; i++)
        {
            if (&figure->scene->figures[i] == figure)
            {
                dvz_snprintf(figure_id, sizeof(figure_id), "fig%u", i);
                break;
            }
        }
    }

    /* Build a fresh FramePlan */
    DvzFramePlan* plan = dvz_frame_plan(figure_id, 0);
    if (plan == NULL)
        return NULL;

    /* --- Upload nodes: one per dirty visual attribute --- */
    for (uint32_t pi = 0; pi < figure->panel_count; pi++)
    {
        DvzPanel* panel = &figure->panels[pi];
        for (uint32_t vi = 0; vi < panel->visual_count; vi++)
        {
            DvzVisual* visual = panel->visuals[vi].visual;
            if (visual == NULL || !visual->visible)
                continue;
            /* Resolve visual membership by identity, avoiding cross-array pointer arithmetic. */
            uint32_t vidx = 0;
            if (!_figure_visual_index(figure, visual, &vidx))
                continue; /* not from this scene */
            for (uint32_t ai = 0; ai < visual->attr_count; ai++)
            {
                DvzVisualAttr* attr = &visual->attrs[ai];
                if (attr->dirty_item_count == 0 || attr->data == NULL || attr->item_count == 0)
                    continue;
                char resource_id[128];
                dvz_snprintf(resource_id, sizeof(resource_id), "v%u_%s", vidx, attr->name);
                uint64_t byte_offset =
                    (uint64_t)attr->dirty_first_item * attr->item_size;
                uint64_t byte_size =
                    (uint64_t)attr->dirty_item_count * attr->item_size;
                const void* data_ptr = (const uint8_t*)attr->data + byte_offset;
                dvz_frame_plan_upload_bytes(
                    plan, resource_id, byte_offset, byte_size, attr->name, data_ptr);
                if ((visual->type == DVZ_VISUAL_TYPE_PRIMITIVE ||
                     visual->type == DVZ_VISUAL_TYPE_PATH) &&
                    strcmp(attr->name, "position") == 0)
                {
                    dvz_frame_plan_upload_set_topology(plan, (uint32_t)visual->topology);
                }
            }
            if (visual->type == DVZ_VISUAL_TYPE_IMAGE && visual->field != NULL &&
                (visual->texture.dirty || visual->field->dirty))
            {
                DvzFieldRegion upload_region = {0};
                const void* upload_data = NULL;
                if (!_scene_prepare_image_texture(visual, &upload_region, &upload_data))
                    continue;
                char tex_resource_id[128];
                dvz_snprintf(tex_resource_id, sizeof(tex_resource_id), "v%u_texture", vidx);
                uint64_t bytes = 0;
                if (_field_region_byte_size(DVZ_FIELD_FORMAT_RGBA8_UNORM, &upload_region, &bytes))
                {
                    dvz_frame_plan_upload_bytes(plan, tex_resource_id, 0, bytes, "texture", upload_data);
                    dvz_frame_plan_upload_set_texture_extent(
                        plan, upload_region.width, upload_region.height);
                    dvz_frame_plan_upload_set_texture_region(
                        plan, upload_region.x, upload_region.y);
                }
                else
                {
                    log_error("image visual texture upload size overflow");
                    continue;
                }
            }
        }
    }

    /* --- Render nodes: one per panel --- */
    for (uint32_t pi = 0; pi < figure->panel_count; pi++)
    {
        DvzPanel* panel = &figure->panels[pi];

        char panel_id[64];
        dvz_snprintf(panel_id, sizeof(panel_id), "%s_p%u", figure_id, pi);
        /* Count drawable visuals — those with position data set. */
        uint32_t drawable_count = 0;
        for (uint32_t vi = 0; vi < panel->visual_count; vi++)
        {
            DvzVisual* visual = panel->visuals[vi].visual;
            if (visual == NULL || !visual->visible)
                continue;
            uint32_t vidx = 0;
            if (!_figure_visual_index(figure, visual, &vidx))
                continue;
            int pos_idx = _attr_index(visual, "position");
            if (pos_idx >= 0 && visual->attrs[pos_idx].item_count > 0)
                drawable_count++;
            else
                log_warn(
                    "%s visual (index %u) has no 'position' data — it will render nothing",
                    _visual_type_name(visual->type), vidx);
        }

        if (drawable_count == 0)
        {
            dvz_frame_plan_clear_panel(plan, panel_id, "rt", panel->desc);
            continue;
        }

        /* Build a stable z-layer-sorted index list (insertion sort: stable, small N). */
        uint32_t order[DVZ_SCENE_MAX_VISUALS];
        for (uint32_t k = 0; k < panel->visual_count; k++)
            order[k] = k;
        for (uint32_t k = 1; k < panel->visual_count; k++)
        {
            uint32_t cur = order[k];
            int32_t cur_z = panel->visuals[cur].z_layer;
            uint32_t cur_ins = panel->visuals[cur].insertion_index;
            uint32_t j = k;
            while (j > 0)
            {
                uint32_t prev = order[j - 1];
                int32_t prev_z = panel->visuals[prev].z_layer;
                uint32_t prev_ins = panel->visuals[prev].insertion_index;
                if (prev_z < cur_z || (prev_z == cur_z && prev_ins <= cur_ins))
                    break;
                order[j] = order[j - 1];
                j--;
            }
            order[j] = cur;
        }

        /* Pre-compute the panel's APPLY MVP (panzoom/arcball). Identity MVP for FIXED
         * visuals is computed by the converter from controller_modes[]. */
        DvzMVP panel_apply_mvp;
        glm_mat4_identity(panel_apply_mvp.model);
        glm_mat4_identity(panel_apply_mvp.view);
        glm_mat4_identity(panel_apply_mvp.proj);
        panel_apply_mvp.time  = 0.0f;
        panel_apply_mvp.flags = 0;
        if (panel->panzoom != NULL)
            dvz_panzoom_mvp(panel->panzoom, &panel_apply_mvp);
        if (panel->arcball != NULL)
            dvz_arcball_mvp(panel->arcball, &panel_apply_mvp);

        /* One render node per panel; populate visuals[] and controller_modes[] in z order. */
        DvzFramePlanNode* node = NULL;
        for (uint32_t k = 0; k < panel->visual_count; k++)
        {
            uint32_t vi = order[k];
            DvzPanelAttach* attach = &panel->visuals[vi];
            DvzVisual* visual = attach->visual;
            if (visual == NULL || !visual->visible)
                continue;
            uint32_t vidx = 0;
            if (!_figure_visual_index(figure, visual, &vidx))
                continue;
            int pos_idx = _attr_index(visual, "position");
            if (pos_idx < 0 || visual->attrs[pos_idx].item_count == 0)
                continue;

            if (node == NULL)
            {
                dvz_frame_plan_render_panel(plan, panel_id, "rt", false, panel->desc);
                node = dvz_frame_plan_last_render_node(plan);
                if (node != NULL)
                {
                    node->u.render.has_mvp = true;
                    node->u.render.apply_mvp = panel_apply_mvp;
                }
            }

            char visual_id[64];
            dvz_snprintf(visual_id, sizeof(visual_id), "v%u", vidx);
            dvz_frame_plan_render_visual(plan, visual_id);
            if (node != NULL)
                node->u.render.controller_modes[node->u.render.visual_count - 1] =
                    attach->controller_mode;
        }
    }

    /* Resolve nullable args */
    DvzCapabilitySnapshot default_caps;
    if (caps == NULL)
    {
        dvz_capability_snapshot_default(&default_caps);
        caps = &default_caps;
    }
    DvzFramePlanEmitConfig default_cfg = dvz_frame_plan_emit_config();
    if (cfg == NULL)
        cfg = &default_cfg;
    DvzDiagnosticReport local_report;
    if (report == NULL)
    {
        dvz_diagnostic_report_init(&local_report);
        report = &local_report;
    }

    DvzDrp2CommandStream* stream =
        dvz_frame_plan_emitter_emit_drp2(emitter, plan, caps, report, cfg);
    if (stream != NULL && !_scene_stream_register(figure->scene, stream))
    {
        dvz_drp2_stream_destroy(stream);
        stream = NULL;
    }

    /* Clear dirty flags after successful emit */
    if (stream != NULL)
    {
        for (uint32_t pi = 0; pi < figure->panel_count; pi++)
        {
            DvzPanel* panel = &figure->panels[pi];
            for (uint32_t vi = 0; vi < panel->visual_count; vi++)
            {
                DvzVisual* visual = panel->visuals[vi].visual;
                if (visual == NULL)
                    continue;
                for (uint32_t ai = 0; ai < visual->attr_count; ai++)
                    visual->attrs[ai].dirty_item_count = 0;
                if (visual->type == DVZ_VISUAL_TYPE_IMAGE)
                {
                    visual->texture.dirty = false;
                    visual->texture.field_dirty = false;
                    visual->texture.field_dirty_full = false;
                    dvz_memset(
                        &visual->texture.field_dirty_region, sizeof(DvzFieldRegion), 0,
                        sizeof(DvzFieldRegion));
                }
            }
        }
        for (uint32_t i = 0; i < figure->scene->field_count; i++)
            _scene_refresh_field_dirty_state(figure->scene, &figure->scene->fields[i]);
    }

    dvz_frame_plan_destroy(plan);
    return stream;
}



DvzDrp2CommandStream* dvz_figure_emit(
    DvzFigure* figure, const DvzCapabilitySnapshot* caps, DvzDiagnosticReport* report)
{
    return dvz_figure_emit_ex(figure, caps, report, NULL);
}



/*************************************************************************************************/
/*  Panel                                                                                        */
/*************************************************************************************************/

DvzPanel* dvz_panel(DvzFigure* figure, DvzPanelDesc desc)
{
    ANN(figure);
    if (figure->panel_count >= DVZ_SCENE_MAX_PANELS)
        return NULL;
    DvzPanel* panel     = &figure->panels[figure->panel_count++];
    panel->figure       = figure;
    panel->desc         = desc;
    panel->visual_count = 0;
    return panel;
}


void dvz_panel_destroy(DvzPanel* panel)
{
    if (panel == NULL)
        return;
    if (panel->panzoom != NULL)
    {
        dvz_panzoom_destroy(panel->panzoom);
        panel->panzoom = NULL;
    }
    if (panel->arcball != NULL)
    {
        dvz_arcball_destroy(panel->arcball);
        panel->arcball = NULL;
    }
    panel->figure       = NULL;
    panel->visual_count = 0;
    panel->colorbar_count = 0;
}


void dvz_panel_set_panzoom(DvzPanel* panel, DvzInputRouter* router, int flags)
{
    ANN(panel);
    if (panel->panzoom != NULL)
        dvz_panzoom_destroy(panel->panzoom);
    float w = panel->desc.width * (panel->figure ? (float)panel->figure->width : 800.0f);
    float h = panel->desc.height * (panel->figure ? (float)panel->figure->height : 600.0f);
    if (w <= 0)
        w = 800.0f;
    if (h <= 0)
        h = 600.0f;
    panel->panzoom = dvz_panzoom(w, h, flags);
    if (router != NULL)
        dvz_panzoom_connect(panel->panzoom, router);
}


void dvz_panel_set_arcball(DvzPanel* panel, DvzInputRouter* router, int flags)
{
    ANN(panel);
    if (panel->arcball != NULL)
        dvz_arcball_destroy(panel->arcball);
    float w = panel->desc.width * (panel->figure ? (float)panel->figure->width : 800.0f);
    float h = panel->desc.height * (panel->figure ? (float)panel->figure->height : 600.0f);
    if (w <= 0)
        w = 800.0f;
    if (h <= 0)
        h = 600.0f;
    panel->arcball = dvz_arcball(w, h, flags);
    if (router != NULL)
        dvz_arcball_connect(panel->arcball, router);
}


int dvz_panel_add_visual(DvzPanel* panel, DvzVisual* visual, const DvzVisualAttachDesc* desc)
{
    ANN(panel);
    ANN(visual);
    if (panel->figure == NULL || panel->figure->scene == NULL)
        return -1;
    if (visual->scene != panel->figure->scene)
        return -1;
    if (panel->visual_count >= DVZ_SCENE_MAX_VISUALS)
        return -1;
    DvzPanelAttach* slot = &panel->visuals[panel->visual_count];
    slot->visual = visual;
    slot->z_layer = desc ? desc->z_layer : 0;
    slot->controller_mode = desc ? desc->controller_mode : DVZ_CONTROLLER_APPLY;
    slot->insertion_index = panel->visual_count;
    panel->visual_count++;
    return 0;
}



void dvz_panel_set_background_color(DvzPanel* panel, float r, float g, float b, float a)
{
    ANN(panel);
    if (panel->figure == NULL || panel->figure->scene == NULL)
        return;
    DvzScene* scene = panel->figure->scene;

    /* Fullscreen quad in clip space, TRIANGLE_STRIP order (TL, BL, TR, BR). The visual
     * is attached with controller_mode=FIXED so the panzoom/arcball MVP doesn't move it,
     * and z_layer=-1 so it draws behind every default-layer visual. */
    static const float positions[4 * 3] = {
        -1.0f, +1.0f, 0.0f, /* TL */
        -1.0f, -1.0f, 0.0f, /* BL */
        +1.0f, +1.0f, 0.0f, /* TR */
        +1.0f, -1.0f, 0.0f, /* BR */
    };
    DvzColor color = {
        (uint8_t)(r * 255.0f + 0.5f),
        (uint8_t)(g * 255.0f + 0.5f),
        (uint8_t)(b * 255.0f + 0.5f),
        (uint8_t)(a * 255.0f + 0.5f),
    };
    DvzColor colors[4] = {
        {color[0], color[1], color[2], color[3]},
        {color[0], color[1], color[2], color[3]},
        {color[0], color[1], color[2], color[3]},
        {color[0], color[1], color[2], color[3]},
    };

    if (panel->background_visual == NULL)
    {
        DvzVisual* bg = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, 0);
        if (bg == NULL)
        {
            log_error("dvz_panel_set_background_color: failed to allocate background visual");
            return;
        }
        if (dvz_visual_set_data(bg, "position", positions, 4) != 0 ||
            dvz_visual_set_data(bg, "color", colors, 4) != 0)
        {
            log_error("dvz_panel_set_background_color: failed to set background data");
            return;
        }
        if (dvz_panel_add_visual(
                panel, bg,
                &(DvzVisualAttachDesc){
                    .z_layer = -1, .controller_mode = DVZ_CONTROLLER_FIXED}) != 0)
        {
            log_error("dvz_panel_set_background_color: failed to attach background visual");
            return;
        }
        panel->background_visual = bg;
    }
    else
    {
        /* Existing background — just update its color. Position is already correct. */
        dvz_visual_set_data(panel->background_visual, "color", colors, 4);
    }
}



/*************************************************************************************************/
/*  Scale / colormap / colorbar                                                                  */
/*************************************************************************************************/

/**
 * Create a scene-owned scale object.
 *
 * @param scene the scene
 * @param desc the scale descriptor, or NULL for defaults
 * @return the scale, or NULL on allocation failure
 */
DvzScale* dvz_scale(DvzScene* scene, const DvzScaleDesc* desc)
{
    ANN(scene);
    if (scene->scale_count >= DVZ_SCENE_MAX_SCALES)
    {
        log_error("maximum scale count reached");
        return NULL;
    }
    DvzScale* scale = &scene->scales[scene->scale_count++];
    dvz_memset(scale, sizeof(DvzScale), 0, sizeof(DvzScale));
    scale->scene = scene;
    scale->kind = desc != NULL ? desc->kind : DVZ_SCALE_CONTINUOUS;
    if (desc != NULL)
    {
        if (desc->label != NULL)
            dvz_strlcpy(scale->label, desc->label, sizeof(scale->label));
        if (desc->unit != NULL)
            dvz_strlcpy(scale->unit, desc->unit, sizeof(scale->unit));
        _format_state_copy(&scale->format, &desc->format);
    }
    return scale;
}


/**
 * Destroy a scale object.
 *
 * @param scale the scale
 */
void dvz_scale_destroy(DvzScale* scale)
{
    if (scale == NULL)
        return;
    scale->scene = NULL;
    scale->colormap = NULL;
    scale->has_domain = false;
    scale->has_view_range = false;
}


/**
 * Set the semantic domain on a scale.
 *
 * @param scale the scale
 * @param min the domain minimum
 * @param max the domain maximum
 */
void dvz_scale_set_domain(DvzScale* scale, double min, double max)
{
    ANN(scale);
    scale->domain_min = min;
    scale->domain_max = max;
    scale->has_domain = true;
    _scene_mark_scale_dirty(scale);
}


/**
 * Set the current visible range on a scale.
 *
 * @param scale the scale
 * @param min the view-range minimum
 * @param max the view-range maximum
 */
void dvz_scale_set_view_range(DvzScale* scale, double min, double max)
{
    ANN(scale);
    scale->view_min = min;
    scale->view_max = max;
    scale->has_view_range = true;
    _scene_mark_scale_dirty(scale);
}


/**
 * Bind a colormap to a scale.
 *
 * @param scale the scale
 * @param colormap the colormap
 */
void dvz_scale_set_colormap(DvzScale* scale, DvzColormap* colormap)
{
    ANN(scale);
    if (colormap != NULL && colormap->scene != scale->scene)
    {
        log_error("cannot bind a colormap from a different scene");
        return;
    }
    scale->colormap = colormap;
    _scene_mark_scale_dirty(scale);
}


/**
 * Override shared formatting policy on a scale.
 *
 * @param scale the scale
 * @param format the format descriptor, or NULL to clear the override
 */
void dvz_scale_set_format(DvzScale* scale, const DvzFormatDesc* format)
{
    ANN(scale);
    _format_state_copy(&scale->format, format);
    _scene_mark_scale_dirty(scale);
}


/**
 * Create a scene-owned colormap object.
 *
 * @param scene the scene
 * @param desc the colormap descriptor, or NULL for defaults
 * @return the colormap, or NULL on allocation failure
 */
DvzColormap* dvz_colormap(DvzScene* scene, const DvzColormapDesc* desc)
{
    ANN(scene);
    if (scene->colormap_count >= DVZ_SCENE_MAX_COLORMAPS)
    {
        log_error("maximum colormap count reached");
        return NULL;
    }
    DvzColormap* colormap = &scene->colormaps[scene->colormap_count++];
    dvz_memset(colormap, sizeof(DvzColormap), 0, sizeof(DvzColormap));
    colormap->scene = scene;
    colormap->kind = desc != NULL ? desc->kind : DVZ_COLORMAP_CONTINUOUS;
    colormap->builtin = desc != NULL ? desc->builtin : DVZ_BUILTIN_COLORMAP_NONE;
    if (desc != NULL)
    {
        colormap->center = desc->center;
        colormap->has_center = desc->center != 0.0;
        if (desc->label != NULL)
            dvz_strlcpy(colormap->label, desc->label, sizeof(colormap->label));
    }
    return colormap;
}


/**
 * Create a scene-owned built-in colormap object.
 *
 * @param scene the scene
 * @param builtin the built-in colormap selector
 * @return the colormap, or NULL on allocation failure
 */
DvzColormap* dvz_colormap_builtin(DvzScene* scene, DvzBuiltinColormap builtin)
{
    DvzColormapDesc desc = {
        .kind = DVZ_COLORMAP_CONTINUOUS,
        .builtin = builtin,
    };
    return dvz_colormap(scene, &desc);
}


/**
 * Destroy a colormap object.
 *
 * @param colormap the colormap
 */
void dvz_colormap_destroy(DvzColormap* colormap)
{
    if (colormap == NULL)
        return;
    colormap->scene = NULL;
    colormap->stop_count = 0;
    colormap->has_center = false;
}


/**
 * Set custom color stops on a colormap.
 *
 * @param colormap the colormap
 * @param stops the color stops
 * @param count the number of stops
 */
void dvz_colormap_set_stops(DvzColormap* colormap, const DvzColormapStop* stops, uint32_t count)
{
    ANN(colormap);
    if (count > DVZ_SCENE_MAX_COLOR_STOPS)
    {
        log_error("too many color stops: %u > %u", count, DVZ_SCENE_MAX_COLOR_STOPS);
        return;
    }
    if (count > 0)
        ANN(stops);
    colormap->stop_count = count;
    if (count > 0)
        dvz_memcpy(colormap->stops, sizeof(colormap->stops), stops, count * sizeof(DvzColormapStop));
    _scene_mark_colormap_dirty(colormap);
}


/**
 * Set the diverging center on a colormap.
 *
 * @param colormap the colormap
 * @param center the semantic center value
 */
void dvz_colormap_set_center(DvzColormap* colormap, double center)
{
    ANN(colormap);
    colormap->center = center;
    colormap->has_center = true;
    _scene_mark_colormap_dirty(colormap);
}


/**
 * Create a panel-attached colorbar bound to a scale.
 *
 * @param panel the panel
 * @param scale the scale
 * @param desc the colorbar descriptor, or NULL for defaults
 * @return the colorbar, or NULL on allocation failure
 */
DvzColorbar* dvz_colorbar(DvzPanel* panel, DvzScale* scale, const DvzColorbarDesc* desc)
{
    ANN(panel);
    ANN(scale);
    if (panel->figure == NULL || panel->figure->scene == NULL)
    {
        log_error("cannot create a colorbar on a detached panel");
        return NULL;
    }
    DvzScene* scene = panel->figure->scene;
    if (scale->scene != scene)
    {
        log_error("cannot attach a scale from a different scene to a panel colorbar");
        return NULL;
    }
    if (scene->colorbar_count >= DVZ_SCENE_MAX_COLORBARS)
    {
        log_error("maximum colorbar count reached");
        return NULL;
    }
    if (panel->colorbar_count >= DVZ_SCENE_MAX_PANEL_COLORBARS)
    {
        log_error("maximum panel colorbar count reached");
        return NULL;
    }
    DvzColorbar* colorbar = &scene->colorbars[scene->colorbar_count++];
    dvz_memset(colorbar, sizeof(DvzColorbar), 0, sizeof(DvzColorbar));
    colorbar->scene = scene;
    colorbar->panel = panel;
    colorbar->scale = scale;
    colorbar->orientation =
        desc != NULL ? desc->orientation : DVZ_COLORBAR_ORIENTATION_VERTICAL;
    colorbar->anchor = desc != NULL ? desc->anchor : DVZ_SCENE_ANCHOR_PANEL_RIGHT;
    colorbar->flags = desc != NULL ? desc->flags : 0;
    if (desc != NULL && desc->title != NULL)
        dvz_strlcpy(colorbar->title, desc->title, sizeof(colorbar->title));
    panel->colorbars[panel->colorbar_count++] = colorbar;
    return colorbar;
}


/**
 * Destroy a colorbar.
 *
 * @param colorbar the colorbar
 */
void dvz_colorbar_destroy(DvzColorbar* colorbar)
{
    if (colorbar == NULL)
        return;
    if (colorbar->panel != NULL)
    {
        DvzPanel* panel = colorbar->panel;
        for (uint32_t i = 0; i < panel->colorbar_count; i++)
        {
            if (panel->colorbars[i] != colorbar)
                continue;
            for (uint32_t j = i + 1; j < panel->colorbar_count; j++)
                panel->colorbars[j - 1] = panel->colorbars[j];
            panel->colorbars[panel->colorbar_count - 1] = NULL;
            panel->colorbar_count--;
            break;
        }
    }
    colorbar->scene = NULL;
    colorbar->panel = NULL;
    colorbar->scale = NULL;
    colorbar->has_format = false;
}


/**
 * Override formatting policy on a colorbar.
 *
 * @param colorbar the colorbar
 * @param format the format descriptor, or NULL to clear the override
 */
void dvz_colorbar_set_format(DvzColorbar* colorbar, const DvzFormatDesc* format)
{
    ANN(colorbar);
    colorbar->has_format = format != NULL;
    _format_state_copy(&colorbar->format, format);
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

    for (uint32_t i = 0; i < DVZ_SCENE_MAX_FIELDS; i++)
    {
        DvzSampledField* field = &scene->fields[i];
        if (field->scene != NULL)
            continue;
        dvz_memset(field, sizeof(DvzSampledField), 0, sizeof(DvzSampledField));
        field->scene = scene;
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
        if (i + 1 > scene->field_count)
            scene->field_count = i + 1;
        return field;
    }

    log_error("maximum sampled field count reached");
    return NULL;
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
                visual->field = NULL;
                visual->field_owned = false;
                dvz_memset(visual->field_slot, sizeof(visual->field_slot), 0,
                           sizeof(visual->field_slot));
                visual->texture.dirty = false;
                visual->texture.field_dirty = false;
                visual->texture.field_dirty_full = false;
                dvz_memset(
                    &visual->texture.field_dirty_region, sizeof(DvzFieldRegion), 0,
                    sizeof(DvzFieldRegion));
                if (visual->texture.upload != NULL)
                {
                    dvz_free(visual->texture.upload);
                    visual->texture.upload = NULL;
                    visual->texture.upload_size = 0;
                }
            }
        }
    }
    if (field->data != NULL)
    {
        dvz_free(field->data);
        field->data = NULL;
    }
    dvz_memset(field, sizeof(DvzSampledField), 0, sizeof(DvzSampledField));
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

    uint64_t bytes_per_row = view->bytes_per_row != 0 ? view->bytes_per_row
                                                      : _field_default_bytes_per_row(&field->desc);
    uint64_t rows_per_image = view->rows_per_image != 0 ? view->rows_per_image : field->desc.height;
    uint64_t copy_bytes_per_row = _field_default_bytes_per_row(&field->desc);
    const uint8_t* src = (const uint8_t*)view->data;
    uint8_t* dst = (uint8_t*)field->data;
    for (uint32_t z = 0; z < field->desc.depth; z++)
    {
        for (uint32_t y = 0; y < field->desc.height; y++)
        {
            uint64_t src_offset = ((uint64_t)z * rows_per_image + y) * bytes_per_row;
            uint64_t dst_offset = ((uint64_t)z * field->desc.height + y) * copy_bytes_per_row;
            dvz_memcpy(dst + dst_offset, copy_bytes_per_row, src + src_offset, copy_bytes_per_row);
        }
    }
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
    if (visual->type != DVZ_VISUAL_TYPE_IMAGE)
    {
        log_error("dvz_visual_set_field is only supported for image visuals in the first slice");
        return false;
    }
    if (strcmp(slot_name, "field") != 0)
    {
        log_error("unsupported image field slot '%s' (expected 'field')", slot_name);
        return false;
    }
    if (field != NULL && field->desc.dim != DVZ_FIELD_DIM_2D)
    {
        log_error("image visuals require a 2D sampled field");
        return false;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "bind sampled field"))
        return false;

    if (visual->field != field)
        _scene_release_visual_field(visual);
    visual->field = field;
    if (field != NULL)
    {
        dvz_strlcpy(visual->field_slot, slot_name, sizeof(visual->field_slot));
        visual->texture.dirty = true;
        visual->texture.field_dirty = false;
        visual->texture.field_dirty_full = false;
        dvz_memset(
            &visual->texture.field_dirty_region, sizeof(DvzFieldRegion), 0,
            sizeof(DvzFieldRegion));
    }
    return true;
}



/*************************************************************************************************/
/*  Visual — lifecycle and data                                                                  */
/*************************************************************************************************/

void dvz_visual_destroy(DvzVisual* visual)
{
    if (visual == NULL)
        return;
    if (!_scene_visual_mutation_allowed(visual->scene, "destroy scene-owned visual data"))
        return;
    for (uint32_t i = 0; i < visual->attr_count; i++)
    {
        if (visual->attrs[i].data != NULL)
        {
            dvz_free(visual->attrs[i].data);
            visual->attrs[i].data = NULL;
        }
    }
    _scene_release_visual_field(visual);
    if (visual->texture.rgba != NULL)
    {
        dvz_free(visual->texture.rgba);
        visual->texture.rgba = NULL;
        visual->texture.rgba_size = 0;
    }
    visual->attr_count = 0;
    visual->scene      = NULL;
}


void dvz_visual_set_visible(DvzVisual* visual, bool visible)
{
    ANN(visual);
    visual->visible = visible;
}


int dvz_visual_set_data(
    DvzVisual* visual, const char* attr_name, const void* data, uint32_t item_count)
{
    ANN(visual);
    ANN(attr_name);
    ANN(data);
    if (!_scene_visual_mutation_allowed(visual->scene, "mutate scene visual data"))
        return -1;
    if (item_count == 0)
    {
        log_error("visual attribute '%s' requires item_count > 0", attr_name);
        return -1;
    }

    uint32_t item_size = 0;
    if (!_attr_supported(visual->type, attr_name, &item_size))
        return -1;
    if (!_visual_attr_count_consistent(visual, attr_name, item_count))
        return -1;

    DvzVisualAttr* attr = _attr_get_or_create(visual, attr_name, item_size);
    if (attr == NULL)
    {
        log_error("visual attribute '%s' could not be registered", attr_name);
        return -1;
    }

    uint64_t byte_size = 0;
    if (_dvz_mul_u64_overflows(item_count, item_size, &byte_size))
    {
        log_error(
            "visual attribute '%s' byte size overflow for item_count=%u item_size=%u", attr_name,
            item_count, item_size);
        return -1;
    }

    /* Reallocate if total size changed */
    if (attr->data != NULL && attr->item_count != item_count)
    {
        dvz_free(attr->data);
        attr->data = NULL;
    }
    if (attr->data == NULL)
    {
        attr->data = dvz_malloc(byte_size);
        if (attr->data == NULL)
        {
            log_error(
                "visual attribute '%s' allocation failed for %" PRIu64 " bytes", attr_name,
                byte_size);
            attr->item_count       = 0;
            attr->dirty_first_item = 0;
            attr->dirty_item_count = 0;
            return -1;
        }
    }

    dvz_memcpy(attr->data, byte_size, data, byte_size);
    attr->item_count       = item_count;
    attr->dirty_first_item = 0;
    attr->dirty_item_count = item_count; /* whole buffer dirty */
    return 0;
}



int dvz_visual_set_data_range(
    DvzVisual* visual, const char* attr_name, const void* data,
    uint32_t first_item, uint32_t item_count)
{
    ANN(visual);
    ANN(attr_name);
    ANN(data);
    if (!_scene_visual_mutation_allowed(visual->scene, "mutate scene visual data"))
        return -1;
    if (item_count == 0)
    {
        log_error("visual attribute '%s' range update requires item_count > 0", attr_name);
        return -1;
    }

    uint32_t item_size = 0;
    if (!_attr_supported(visual->type, attr_name, &item_size))
        return -1;

    DvzVisualAttr* attr = _attr_get_or_create(visual, attr_name, item_size);
    if (attr == NULL)
    {
        log_error("visual attribute '%s' could not be registered", attr_name);
        return -1;
    }

    /* The attribute must already be fully allocated */
    if (attr->data == NULL || attr->item_count == 0)
    {
        log_error(
            "visual attribute '%s' range update requires prior full allocation with "
            "dvz_visual_set_data()",
            attr_name);
        return -1;
    }
    uint64_t item_end = 0;
    if (_dvz_add_u64_overflows(first_item, item_count, &item_end))
    {
        log_error(
            "visual attribute '%s' range update overflow for first_item=%u item_count=%u",
            attr_name, first_item, item_count);
        return -1;
    }
    if (item_end > attr->item_count)
    {
        log_error(
            "visual attribute '%s' range update [%u, %" PRIu64 ") exceeds item_count %u",
            attr_name, first_item, item_end, attr->item_count);
        return -1;
    }

    uint64_t byte_offset = 0;
    uint64_t byte_size   = 0;
    if (_dvz_mul_u64_overflows(first_item, item_size, &byte_offset))
    {
        log_error(
            "visual attribute '%s' byte offset overflow for first_item=%u item_size=%u", attr_name,
            first_item, item_size);
        return -1;
    }
    if (_dvz_mul_u64_overflows(item_count, item_size, &byte_size))
    {
        log_error(
            "visual attribute '%s' byte size overflow for item_count=%u item_size=%u", attr_name,
            item_count, item_size);
        return -1;
    }
    dvz_memcpy((uint8_t*)attr->data + byte_offset, byte_size, data, byte_size);

    /* Extend dirty range to cover the new update */
    if (attr->dirty_item_count == 0)
    {
        attr->dirty_first_item = first_item;
        attr->dirty_item_count = item_count;
    }
    else
    {
        uint64_t old_end = 0;
        uint64_t new_end = 0;
        if (_dvz_add_u64_overflows(attr->dirty_first_item, attr->dirty_item_count, &old_end))
            return -1;
        if (_dvz_add_u64_overflows(first_item, item_count, &new_end))
            return -1;
        uint64_t merged_first = attr->dirty_first_item < first_item
                                    ? attr->dirty_first_item
                                    : first_item;
        uint64_t merged_end = old_end > new_end ? old_end : new_end;
        attr->dirty_first_item = merged_first;
        attr->dirty_item_count = merged_end - merged_first;
    }
    return 0;
}



/*************************************************************************************************/
/*  Visual family constructors                                                                   */
/*************************************************************************************************/

DvzVisual* dvz_point(DvzScene* scene, uint32_t flags)
{
    ANN(scene);
    if (scene->visual_count >= DVZ_SCENE_MAX_VISUALS)
        return NULL;
    DvzVisual* visual = &scene->visuals[scene->visual_count++];
    dvz_memset(visual, sizeof(DvzVisual), 0, sizeof(DvzVisual));
    visual->scene   = scene;
    visual->type    = DVZ_VISUAL_TYPE_POINT;
    visual->flags   = flags;
    visual->visible = true;
    visual->z_layer = 0;
    return visual;
}



DvzVisual* dvz_primitive(DvzScene* scene, DvzPrimitiveTopology topology, uint32_t flags)
{
    ANN(scene);
    if (scene->visual_count >= DVZ_SCENE_MAX_VISUALS)
        return NULL;
    DvzVisual* visual = &scene->visuals[scene->visual_count++];
    dvz_memset(visual, sizeof(DvzVisual), 0, sizeof(DvzVisual));
    visual->scene    = scene;
    visual->type     = DVZ_VISUAL_TYPE_PRIMITIVE;
    visual->flags    = flags;
    visual->visible  = true;
    visual->z_layer  = 0;
    visual->topology = topology;
    return visual;
}



/**
 * Create a path visual.
 *
 * First-slice path visuals reuse the primitive line-strip execution path.
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual, or NULL on allocation failure
 */
DvzVisual* dvz_path(DvzScene* scene, uint32_t flags)
{
    ANN(scene);
    if (scene->visual_count >= DVZ_SCENE_MAX_VISUALS)
        return NULL;
    DvzVisual* visual = &scene->visuals[scene->visual_count++];
    dvz_memset(visual, sizeof(DvzVisual), 0, sizeof(DvzVisual));
    visual->scene    = scene;
    visual->type     = DVZ_VISUAL_TYPE_PATH;
    visual->flags    = flags;
    visual->visible  = true;
    visual->z_layer  = 0;
    visual->topology = DVZ_PRIMITIVE_TOPOLOGY_LINE_STRIP;
    return visual;
}



DvzVisual* dvz_image(DvzScene* scene, uint32_t flags)
{
    ANN(scene);
    if (scene->visual_count >= DVZ_SCENE_MAX_VISUALS)
        return NULL;
    DvzVisual* visual = &scene->visuals[scene->visual_count++];
    dvz_memset(visual, sizeof(DvzVisual), 0, sizeof(DvzVisual));
    visual->scene   = scene;
    visual->type    = DVZ_VISUAL_TYPE_IMAGE;
    visual->flags   = flags;
    visual->visible = true;
    visual->z_layer = 0;
    return visual;
}



int dvz_visual_set_texture(
    DvzVisual* visual, const void* rgba, uint32_t width, uint32_t height)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_IMAGE)
    {
        log_error("dvz_visual_set_texture is only supported for image visuals");
        return -1;
    }
    if (rgba == NULL || width == 0 || height == 0)
    {
        log_error("dvz_visual_set_texture: NULL data or zero extent (%ux%u)", width, height);
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "set image texture"))
        return -1;
    DvzSampledField* field = visual->field_owned ? visual->field : NULL;
    if (field == NULL || field->desc.format != DVZ_FIELD_FORMAT_RGBA8_UNORM ||
        field->desc.width != width || field->desc.height != height || field->desc.depth != 1)
    {
        if (field != NULL)
            dvz_sampled_field_destroy(field);
        field = dvz_sampled_field(
            visual->scene, &(DvzSampledFieldDesc){
                               .dim = DVZ_FIELD_DIM_2D,
                               .format = DVZ_FIELD_FORMAT_RGBA8_UNORM,
                               .semantic = DVZ_FIELD_SEMANTIC_COLOR,
                               .width = width,
                               .height = height,
                               .depth = 1,
                           });
        if (field == NULL)
            return -1;
    }
    if (!dvz_sampled_field_set_data(
            field, &(DvzFieldDataView){
                       .data = rgba,
                       .bytes_per_row = (uint64_t)width * 4u,
                       .rows_per_image = height,
                   }))
        return -1;
    if (!dvz_visual_set_field(visual, "field", field))
        return -1;
    visual->field_owned = true;
    return 0;
}


/**
 * Attach a 2D scalar F32 texture to an image visual.
 *
 * The scalar data must remain valid until emit time. The bound scale and
 * colormap are applied on the CPU during emit to produce the RGBA texture used
 * by the current first-slice image runtime path.
 *
 * @param visual the visual (must be of type IMAGE)
 * @param values scalar F32 pixel data, tightly packed, row-major
 * @param width the texture width in pixels
 * @param height the texture height in pixels
 * @return 0 on success, -1 on error
 */
int dvz_visual_set_texture_f32(
    DvzVisual* visual, const float* values, uint32_t width, uint32_t height)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_IMAGE)
    {
        log_error("dvz_visual_set_texture_f32 is only supported for image visuals");
        return -1;
    }
    if (values == NULL || width == 0 || height == 0)
    {
        log_error("dvz_visual_set_texture_f32: NULL data or zero extent (%ux%u)", width, height);
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "set scalar image texture"))
        return -1;
    DvzSampledField* field = visual->field_owned ? visual->field : NULL;
    if (field == NULL || field->desc.format != DVZ_FIELD_FORMAT_R32_FLOAT ||
        field->desc.width != width || field->desc.height != height || field->desc.depth != 1)
    {
        if (field != NULL)
            dvz_sampled_field_destroy(field);
        field = dvz_sampled_field(
            visual->scene, &(DvzSampledFieldDesc){
                               .dim = DVZ_FIELD_DIM_2D,
                               .format = DVZ_FIELD_FORMAT_R32_FLOAT,
                               .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                               .width = width,
                               .height = height,
                               .depth = 1,
                           });
        if (field == NULL)
            return -1;
    }
    if (!dvz_sampled_field_set_data(
            field, &(DvzFieldDataView){
                       .data = values,
                       .bytes_per_row = (uint64_t)width * sizeof(float),
                       .rows_per_image = height,
                   }))
        return -1;
    if (!dvz_visual_set_field(visual, "field", field))
        return -1;
    visual->field_owned = true;
    return 0;
}


/**
 * Bind a scene-owned scale to a named visual slot.
 *
 * First retained slice: image visuals accept the `"colormap"` slot. Other
 * visual families and slot names are rejected until their retained scale
 * wiring is implemented.
 *
 * @param visual the visual
 * @param slot_name the semantic slot name
 * @param scale the scale, or NULL to clear the binding
 * @return 0 on success, -1 on error
 */
int dvz_visual_set_scale(DvzVisual* visual, const char* slot_name, DvzScale* scale)
{
    ANN(visual);
    ANN(slot_name);
    if (scale != NULL && scale->scene != visual->scene)
    {
        log_error("cannot bind a scale from a different scene");
        return -1;
    }
    if (visual->type != DVZ_VISUAL_TYPE_IMAGE)
    {
        log_error("dvz_visual_set_scale is only supported for image visuals in the first slice");
        return -1;
    }
    if (strcmp(slot_name, "colormap") != 0)
    {
        log_error("unsupported image scale slot '%s' (expected 'colormap')", slot_name);
        return -1;
    }
    visual->scale = scale;
    dvz_memset(visual->scale_slot, sizeof(visual->scale_slot), 0, sizeof(visual->scale_slot));
    if (scale != NULL)
        dvz_strlcpy(visual->scale_slot, slot_name, sizeof(visual->scale_slot));
    return 0;
}



/*************************************************************************************************/
/*  Scene JSON serialization                                                                     */
/*************************************************************************************************/

static const char* _visual_type_name(DvzVisualType type)
{
    switch (type)
    {
    case DVZ_VISUAL_TYPE_POINT:
        return "point";
    case DVZ_VISUAL_TYPE_PIXEL:
        return "pixel";
    case DVZ_VISUAL_TYPE_MARKER:
        return "marker";
    case DVZ_VISUAL_TYPE_SEGMENT:
        return "segment";
    case DVZ_VISUAL_TYPE_PATH:
        return "path";
    case DVZ_VISUAL_TYPE_IMAGE:
        return "image";
    case DVZ_VISUAL_TYPE_MESH:
        return "mesh";
    case DVZ_VISUAL_TYPE_VOLUME:
        return "volume";
    case DVZ_VISUAL_TYPE_PRIMITIVE:
        return "primitive";
    default:
        return "unknown";
    }
}

/* Return the scene-global index of a visual, or UINT32_MAX if not found. */
static uint32_t _visual_index(const DvzScene* scene, const DvzVisual* visual)
{
    for (uint32_t i = 0; i < scene->visual_count; i++)
        if (&scene->visuals[i] == visual)
            return i;
    return UINT32_MAX;
}

char* dvz_scene_json(const DvzScene* scene)
{
    ANN(scene);

    JsonBuilder b = {0};
    if (!_json_init(&b))
        return NULL;

    _json_append(&b, "{\"fields\":[");
    bool first_field = true;
    for (uint32_t i = 0; i < scene->field_count; i++)
    {
        const DvzSampledField* field = &scene->fields[i];
        if (field->scene != scene)
            continue;
        _json_append(
            &b,
            "%s{\"id\":\"f%u\",\"dim\":%u,\"format\":%u,\"semantic\":%u,"
            "\"width\":%u,\"height\":%u,\"depth\":%u,\"data\":",
            first_field ? "" : ",", i, (uint32_t)field->desc.dim, (uint32_t)field->desc.format,
            (uint32_t)field->desc.semantic, field->desc.width, field->desc.height, field->desc.depth);
        if (field->data != NULL && field->data_size > 0)
            _json_append_base64(&b, (const uint8_t*)field->data, field->data_size);
        else
            _json_append(&b, "null");
        _json_append(
            &b,
            ",\"geometry\":{\"axis_order\":[%u,%u,%u],\"axis_flip\":[%s,%s,%s],"
            "\"origin\":[%.6g,%.6g,%.6g],\"spacing\":[%.6g,%.6g,%.6g],\"unit\":",
            field->geometry.axis_order[0], field->geometry.axis_order[1],
            field->geometry.axis_order[2], field->geometry.axis_flip[0] ? "true" : "false",
            field->geometry.axis_flip[1] ? "true" : "false",
            field->geometry.axis_flip[2] ? "true" : "false", field->geometry.origin[0],
            field->geometry.origin[1], field->geometry.origin[2], field->geometry.spacing[0],
            field->geometry.spacing[1], field->geometry.spacing[2]);
        _json_append_escaped_string(&b, field->geometry.unit);
        _json_append(&b, "}}");
        first_field = false;
    }

    _json_append(&b, "],\"figures\":[");
    for (uint32_t fi = 0; fi < scene->figure_count; fi++)
    {
        const DvzFigure* fig = &scene->figures[fi];
        if (fig->scene == NULL)
            continue;
        _json_append(&b, "%s{\"id\":\"fig%u\",\"width\":%u,\"height\":%u,\"panels\":[",
                     fi == 0 ? "" : ",", fi, fig->width, fig->height);

        for (uint32_t pi = 0; pi < fig->panel_count; pi++)
        {
            const DvzPanel* panel = &fig->panels[pi];
            _json_append(
                &b,
                "%s{\"id\":\"fig%u_p%u\","
                "\"desc\":{\"x\":%.6g,\"y\":%.6g,\"width\":%.6g,\"height\":%.6g},"
                "\"visuals\":[",
                pi == 0 ? "" : ",", fi, pi,
                (double)panel->desc.x, (double)panel->desc.y,
                (double)panel->desc.width, (double)panel->desc.height);

            for (uint32_t vi = 0; vi < panel->visual_count; vi++)
            {
                const DvzVisual* vis = panel->visuals[vi].visual;
                if (vis == NULL)
                    continue;
                uint32_t vidx = _visual_index(scene, vis);
                _json_append(
                    &b,
                    "%s{\"id\":\"v%u\",\"type\":\"%s\",\"visible\":%s,\"attrs\":[",
                    vi == 0 ? "" : ",", vidx,
                    _visual_type_name(vis->type),
                    vis->visible ? "true" : "false");

                for (uint32_t ai = 0; ai < vis->attr_count; ai++)
                {
                    const DvzVisualAttr* attr = &vis->attrs[ai];
                    uint64_t byte_size = (uint64_t)attr->item_count * attr->item_size;
                    _json_append(
                        &b,
                        "%s{\"name\":\"%s\",\"item_count\":%u,\"item_size\":%u,\"data\":",
                        ai == 0 ? "" : ",",
                        attr->name, attr->item_count, attr->item_size);
                    if (attr->data != NULL && byte_size > 0)
                        _json_append_base64(&b, (const uint8_t*)attr->data, byte_size);
                    else
                        _json_append(&b, "null");
                    _json_append(&b, "}");
                }
                _json_append(&b, "],\"scale\":");
                if (vis->scale != NULL && vis->scene != NULL)
                {
                    int64_t scale_idx = -1;
                    for (uint32_t si = 0; si < vis->scene->scale_count; si++)
                    {
                        if (&vis->scene->scales[si] == vis->scale)
                        {
                            scale_idx = (int64_t)si;
                            break;
                        }
                    }
                    if (scale_idx >= 0)
                    {
                        _json_append(&b, "{\"id\":\"s%" PRId64 "\",\"slot\":", scale_idx);
                        _json_append_escaped_string(&b, vis->scale_slot);
                        _json_append(&b, "}");
                    }
                    else
                    {
                        _json_append(&b, "null");
                    }
                }
                else
                {
                    _json_append(&b, "null");
                }
                _json_append(&b, ",\"field\":");
                if (vis->field != NULL && vis->scene != NULL)
                {
                    uint32_t field_idx = _scene_field_index(vis->scene, vis->field);
                    if (field_idx != UINT32_MAX)
                    {
                        _json_append(&b, "{\"id\":\"f%u\",\"slot\":", field_idx);
                        _json_append_escaped_string(&b, vis->field_slot);
                        _json_append(&b, "}");
                    }
                    else
                    {
                        _json_append(&b, "null");
                    }
                }
                else
                {
                    _json_append(&b, "null");
                }
                _json_append(&b, "}"); /* close visual */
            }
            _json_append(&b, "]}"); /* close visuals + panel */
        }
        _json_append(&b, "]}"); /* close panels + figure */
    }
    _json_append(&b, "]}"); /* close figures + root */

    return _json_finish(&b);
}



void dvz_scene_json_destroy(char* json)
{
    dvz_free(json);
}
