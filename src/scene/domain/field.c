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
#include "field_internal.h"
#include "sample_profile.h"



/*************************************************************************************************/
/*  Function prototypes                                                                          */
/*************************************************************************************************/

static DvzSampledField* _scene_alloc_field_slot(DvzScene* scene);

static void _scene_mark_field_region_dirty(
    DvzSampledField* field, DvzFieldRegion region, bool full);



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



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

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
