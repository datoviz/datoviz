/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene scale helpers                                                                           */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <math.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_scene.h"
#include "core/scene_notify_internal.h"
#include "core/format_state_internal.h"
#include "domain/field_internal.h"
#include "sample_profile.h"
#include "scale_internal.h"
#include "visuals/_visual_internal.h"



/*************************************************************************************************/
/*  Function prototypes                                                                          */
/*************************************************************************************************/

#define DVZ_SCALE_DESC_KNOWN_FLAGS 0u

static void _scene_texture_bump_version(DvzVisual* visual);

static bool _scale_categories_have_duplicate_ids(
    const DvzScaleCategory* categories, uint32_t count);

static bool _scale_reserve_categories(DvzScale* scale, uint32_t capacity);

static void _scale_release_categories(DvzScale* scale);

static int32_t _scale_category_index(const DvzScale* scale, DvzCategoryId id);

static void _scale_category_copy(DvzScaleCategoryState* dst, const DvzScaleCategory* src);

static bool _scale_desc_validate(const DvzScaleDesc* desc);



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

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



/* Scale changes notify retained visuals, colorbars, and legends that consume shared scale state. */
/**
 * Mark visuals depending on one scale as needing refreshed texture data.
 *
 * @param scale the scale
 */
void _scene_mark_scale_dirty(DvzScale* scale)
{
    if (scale == NULL || scale->scene == NULL)
        return;
    DvzScene* scene = scale->scene;
    for (uint32_t i = 0; i < scene->visual_count; i++)
    {
        DvzVisual* visual = &scene->visuals[i];
        if (visual->scene != scene || _visual_family_state(visual)->scale != scale)
            continue;
        DvzSceneSampleProfile profile = {0};
        bool has_profile =
            _visual_family_state(visual)->field != NULL &&
            _scene_sample_profile_resolve(
                _visual_family_state(visual)->field->desc.format, _visual_family_state(visual)->field->desc.semantic, _visual_family_state(visual)->field->desc.dim,
                &profile);
        if ((visual->type == DVZ_VISUAL_TYPE_IMAGE || visual->type == DVZ_VISUAL_TYPE_VOLUME) &&
            has_profile &&
            (_scene_sample_profile_uses_continuous_colorizer(&profile) ||
             _scene_sample_profile_is_integer_label(&profile)))
        {
            _scene_visual_texture_mark_clean(visual);
            _visual_family_state(visual)->texture.dirty = true;
            _scene_texture_bump_version(visual);
            _scene_notify_visual_changed(visual);
        }
        if (visual->type == DVZ_VISUAL_TYPE_LABELS)
        {
            _scene_texture_bump_version(visual);
            _scene_notify_visual_changed(visual);
        }
        if (
            strcmp(_visual_family_state(visual)->scale_slot, "color") == 0 &&
            (visual->type == DVZ_VISUAL_TYPE_POINT || visual->type == DVZ_VISUAL_TYPE_PIXEL))
        {
            int attr_idx = _attr_index(visual, "color");
            if (attr_idx >= 0)
            {
                DvzVisualAttr* attr = &visual->attrs[attr_idx];
                if (
                    attr->format == DVZ_VISUAL_ATTR_FORMAT_SCALAR_F32 && attr->data != NULL &&
                    attr->item_count > 0)
                {
                    attr->dirty_first_item = 0;
                    attr->dirty_item_count = attr->item_count;
                    _visual_bump_version(&attr->version);
                    _scene_notify_visual_changed(visual);
                }
            }
        }
    }
    for (uint32_t i = 0; i < scene->colorbar_count; i++)
    {
        DvzColorbar* colorbar = &scene->colorbars[i];
        if (colorbar->scene == scene && colorbar->scale == scale)
            _scene_mark_colorbar_dirty(colorbar);
    }
    for (uint32_t i = 0; i < scene->legend_count; i++)
    {
        DvzLegend* legend = &scene->legends[i];
        if (legend->scene == scene && legend->scale == scale)
            _scene_mark_legend_dirty(legend);
    }
}


/**
 * Advance a retained visual texture version.
 *
 * @param visual the image visual
 */
static void _scene_texture_bump_version(DvzVisual* visual)
{
    ANN(visual);
    _visual_family_state(visual)->texture.version =
        _visual_family_state(visual)->texture.version == UINT64_MAX ? 1 : _visual_family_state(visual)->texture.version + 1;
}



/**
 * Return whether a category table contains duplicate category ids.
 *
 * @param categories category descriptors
 * @param count number of descriptors
 * @return whether any category id appears more than once
 */
static bool _scale_categories_have_duplicate_ids(
    const DvzScaleCategory* categories, uint32_t count)
{
    ANN(categories);
    for (uint32_t i = 0; i < count; i++)
    {
        for (uint32_t j = i + 1; j < count; j++)
        {
            if (categories[i].category_id == categories[j].category_id)
                return true;
        }
    }
    return false;
}


/**
 * Reserve retained storage for categorical scale entries.
 *
 * @param scale the categorical scale
 * @param capacity required category capacity
 * @return whether storage was available
 */
static bool _scale_reserve_categories(DvzScale* scale, uint32_t capacity)
{
    ANN(scale);
    if (capacity <= scale->category_capacity)
        return true;
    if (capacity > DVZ_SCENE_MAX_SCALE_CATEGORIES)
    {
        log_error("too many scale categories: %u > %u", capacity, DVZ_SCENE_MAX_SCALE_CATEGORIES);
        return false;
    }

    uint64_t size = (uint64_t)capacity * sizeof(DvzScaleCategoryState);
    DvzScaleCategoryState* categories = dvz_calloc(size, 1);
    if (categories == NULL)
        return false;
    if (scale->categories != NULL && scale->category_count > 0)
    {
        dvz_memcpy(
            categories, size, scale->categories,
            (uint64_t)scale->category_count * sizeof(DvzScaleCategoryState));
    }
    dvz_free(scale->categories);
    scale->categories = categories;
    scale->category_capacity = capacity;
    return true;
}


/**
 * Release retained storage for categorical scale entries.
 *
 * @param scale the categorical scale
 */
static void _scale_release_categories(DvzScale* scale)
{
    if (scale == NULL)
        return;
    dvz_free(scale->categories);
    scale->categories = NULL;
    scale->category_count = 0;
    scale->category_capacity = 0;
}


/**
 * Return the index of a retained category id.
 *
 * @param scale the categorical scale
 * @param id the category id
 * @return the category index, or -1 when absent
 */
static int32_t _scale_category_index(const DvzScale* scale, DvzCategoryId id)
{
    ANN(scale);
    for (uint32_t i = 0; i < scale->category_count; i++)
    {
        if (scale->categories[i].category_id == id)
            return (int32_t)i;
    }
    return -1;
}


/**
 * Copy a public category descriptor into retained category state.
 *
 * @param dst retained category state
 * @param src public category descriptor
 */
static void _scale_category_copy(DvzScaleCategoryState* dst, const DvzScaleCategory* src)
{
    ANN(dst);
    ANN(src);
    dvz_memset(dst, sizeof(DvzScaleCategoryState), 0, sizeof(DvzScaleCategoryState));
    dst->category_id = src->category_id;
    dst->order = src->order;
    dst->flags = src->flags;
    dst->has_label = src->label != NULL && src->label[0] != '\0';
    if (dst->has_label)
        dvz_strlcpy(dst->label, src->label, sizeof(dst->label));
    dst->color = src->color;
}


/**
 * Validate public scale descriptor ABI fields.
 *
 * @param desc the scale descriptor
 * @return whether the descriptor is accepted
 */
static bool _scale_desc_validate(const DvzScaleDesc* desc)
{
    if (desc == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(desc, DvzScaleDesc, DVZ_SCALE_DESC_KNOWN_FLAGS))
    {
        log_error("invalid scale descriptor ABI");
        return false;
    }
    if (!_scene_format_desc_is_zero(&desc->format) &&
        !_scene_format_desc_validate(&desc->format))
        return false;
    return true;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return the default scale descriptor.
 *
 * @return default scale descriptor
 */
DvzScaleDesc dvz_scale_desc(void)
{
    return (DvzScaleDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzScaleDesc),
        .kind = DVZ_SCALE_CONTINUOUS,
        .format = {DVZ_STRUCT_INIT_FIELDS(DvzFormatDesc)},
    };
}


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
    if (!_scale_desc_validate(desc))
        return NULL;
    DvzScaleDesc resolved = desc != NULL ? *desc : dvz_scale_desc();
    if (scene->scale_count >= DVZ_SCENE_MAX_SCALES)
    {
        log_error("maximum scale count reached");
        return NULL;
    }
    DvzScale* scale = &scene->scales[scene->scale_count++];
    dvz_memset(scale, sizeof(DvzScale), 0, sizeof(DvzScale));
    scale->scene = scene;
    scale->id = _scene_next_id(scene);
    scale->kind = resolved.kind;
    if (resolved.label != NULL)
        dvz_strlcpy(scale->label, resolved.label, sizeof(scale->label));
    if (resolved.unit != NULL)
        dvz_strlcpy(scale->unit, resolved.unit, sizeof(scale->unit));
    if (!_scene_format_desc_is_zero(&resolved.format))
        _scene_format_state_copy(&scale->format, &resolved.format);
    return scale;
}


DvzId dvz_scale_id(const DvzScale* scale)
{
    return scale != NULL && scale->scene != NULL ? scale->id : DVZ_ID_NONE;
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
    _scale_release_categories(scale);
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
    if (!_scene_format_desc_validate(format))
        return;
    _scene_format_state_copy(&scale->format, format);
    _scene_mark_scale_dirty(scale);
}


/**
 * Replace retained categorical entries on a scale.
 *
 * @param scale the scale
 * @param categories category entry array, or NULL to clear
 * @param count the number of category entries
 * @return true when the category table was accepted
 */
bool dvz_scale_set_categories(
    DvzScale* scale, const DvzScaleCategory* categories, uint32_t count)
{
    ANN(scale);
    if (scale->kind != DVZ_SCALE_CATEGORICAL)
    {
        log_error("scale categories are valid only for categorical scales");
        return false;
    }
    if (categories == NULL || count == 0)
    {
        _scale_release_categories(scale);
        _scene_mark_scale_dirty(scale);
        return true;
    }
    if (_scale_categories_have_duplicate_ids(categories, count))
    {
        log_error("duplicate scale category id");
        return false;
    }
    if (!_scale_reserve_categories(scale, count))
        return false;

    for (uint32_t i = 0; i < count; i++)
    {
        _scale_category_copy(&scale->categories[i], &categories[i]);
    }
    for (uint32_t i = count; i < scale->category_count; i++)
    {
        dvz_memset(
            &scale->categories[i], sizeof(DvzScaleCategoryState), 0,
            sizeof(DvzScaleCategoryState));
    }
    scale->category_count = count;
    _scene_mark_scale_dirty(scale);
    return true;
}


/**
 * Update or append retained categorical entries on a scale.
 *
 * @param scale the scale
 * @param categories category entry array
 * @param count the number of category entries
 * @return true when the category table was accepted
 */
bool dvz_scale_update_categories(
    DvzScale* scale, const DvzScaleCategory* categories, uint32_t count)
{
    ANN(scale);
    if (scale->kind != DVZ_SCALE_CATEGORICAL)
    {
        log_error("scale categories are valid only for categorical scales");
        return false;
    }
    if (categories == NULL || count == 0)
        return true;
    if (_scale_categories_have_duplicate_ids(categories, count))
    {
        log_error("duplicate scale category id");
        return false;
    }

    uint32_t append_count = 0;
    for (uint32_t i = 0; i < count; i++)
    {
        if (_scale_category_index(scale, categories[i].category_id) < 0)
            append_count++;
    }
    if (append_count > DVZ_SCENE_MAX_SCALE_CATEGORIES - scale->category_count)
    {
        log_error(
            "too many scale categories: %u > %u", scale->category_count + append_count,
            DVZ_SCENE_MAX_SCALE_CATEGORIES);
        return false;
    }
    if (!_scale_reserve_categories(scale, scale->category_count + append_count))
        return false;

    for (uint32_t i = 0; i < count; i++)
    {
        int32_t index = _scale_category_index(scale, categories[i].category_id);
        if (index < 0)
            index = (int32_t)scale->category_count++;
        _scale_category_copy(&scale->categories[(uint32_t)index], &categories[i]);
    }
    _scene_mark_scale_dirty(scale);
    return true;
}


/**
 * Remove retained categorical entries from a scale.
 *
 * @param scale the scale
 * @param ids category ids to remove
 * @param count the number of ids
 * @return true when the category table was updated
 */
bool dvz_scale_remove_categories(DvzScale* scale, const DvzCategoryId* ids, uint32_t count)
{
    ANN(scale);
    if (scale->kind != DVZ_SCALE_CATEGORICAL)
    {
        log_error("scale categories are valid only for categorical scales");
        return false;
    }
    if (ids == NULL || count == 0)
        return true;

    bool changed = false;
    for (uint32_t i = 0; i < count; i++)
    {
        int32_t index = _scale_category_index(scale, ids[i]);
        if (index < 0)
            continue;
        uint32_t ui = (uint32_t)index;
        for (uint32_t j = ui + 1; j < scale->category_count; j++)
            scale->categories[j - 1] = scale->categories[j];
        scale->category_count--;
        dvz_memset(
            &scale->categories[scale->category_count], sizeof(DvzScaleCategoryState), 0,
            sizeof(DvzScaleCategoryState));
        changed = true;
    }
    if (changed)
        _scene_mark_scale_dirty(scale);
    return true;
}
