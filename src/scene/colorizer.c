/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene colorizers                                                                             */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "colorizer.h"

#include <inttypes.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_scene.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return the categorical scale entry for one category id.
 *
 * @param colorizer the colorizer
 * @param id category id
 * @return the retained category entry, or NULL when not found
 */
static const DvzScaleCategoryState* _colorizer_category(
    const DvzSceneColorizer* colorizer, DvzCategoryId id)
{
    ANN(colorizer);
    if (colorizer->kind != DVZ_SCENE_COLORIZER_CATEGORICAL || colorizer->scale == NULL)
        return NULL;
    const DvzScale* scale = colorizer->scale;
    for (uint32_t i = 0; i < scale->category_count; i++)
    {
        const DvzScaleCategoryState* category = &scale->categories[i];
        if (category->category_id == id)
            return category;
    }
    return NULL;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Lower a scene scale into a colorizer view.
 *
 * @param scale the bound scene scale
 * @param kind expected colorizer kind
 * @param out output colorizer
 * @return whether the scale can provide the requested colorizer kind
 */
bool _scene_colorizer_from_scale(
    const DvzScale* scale, DvzSceneColorizerKind kind, DvzSceneColorizer* out)
{
    ANN(out);
    dvz_memset(out, sizeof(DvzSceneColorizer), 0, sizeof(DvzSceneColorizer));
    out->kind = kind;
    out->scale = scale;

    if (kind == DVZ_SCENE_COLORIZER_NONE || kind == DVZ_SCENE_COLORIZER_DIRECT_RGBA)
        return true;
    if (scale == NULL)
        return false;
    if (kind == DVZ_SCENE_COLORIZER_CATEGORICAL)
        return scale->kind == DVZ_SCALE_CATEGORICAL;
    if (kind == DVZ_SCENE_COLORIZER_COLORMAP_1D || kind == DVZ_SCENE_COLORIZER_TRANSFER_1D)
        return scale->kind == DVZ_SCALE_CONTINUOUS && scale->colormap != NULL;
    return false;
}



/**
 * Return the categorical color for one category id.
 *
 * @param colorizer the colorizer
 * @param id category id
 * @param out_color output category color
 * @return whether the category had a retained color
 */
bool _scene_colorizer_category_color(
    const DvzSceneColorizer* colorizer, DvzCategoryId id, DvzColor* out_color)
{
    ANN(out_color);
    const DvzScaleCategoryState* category = _colorizer_category(colorizer, id);
    if (category == NULL)
        return false;
    *out_color = category->color;
    return true;
}



/**
 * Return the categorical label for one category id.
 *
 * @param colorizer the colorizer
 * @param id category id
 * @param out_label output label
 * @param label_size output label capacity
 * @return whether a retained label was found
 */
bool _scene_colorizer_category_label(
    const DvzSceneColorizer* colorizer, DvzCategoryId id, char* out_label, uint64_t label_size)
{
    ANN(out_label);
    const DvzScaleCategoryState* category = _colorizer_category(colorizer, id);
    if (category != NULL && category->has_label)
    {
        dvz_strlcpy(out_label, category->label, label_size);
        return true;
    }
    dvz_snprintf(out_label, label_size, "label %" PRIi64, id);
    return false;
}



/**
 * Return the dense categorical palette extent required for non-negative category ids.
 *
 * @param colorizer the colorizer
 * @param out_count output palette entry count
 * @return whether the categorical scale can be represented as a dense non-negative palette
 */
bool _scene_colorizer_dense_palette_extent(
    const DvzSceneColorizer* colorizer, uint32_t* out_count)
{
    ANN(out_count);
    *out_count = 0;
    if (colorizer == NULL || colorizer->kind != DVZ_SCENE_COLORIZER_CATEGORICAL ||
        colorizer->scale == NULL)
    {
        return false;
    }
    const DvzScale* scale = colorizer->scale;
    uint64_t max_id = 0;
    for (uint32_t i = 0; i < scale->category_count; i++)
    {
        DvzCategoryId id = scale->categories[i].category_id;
        if (id < 0 || (uint64_t)id > UINT32_MAX - 1ull)
            return false;
        if ((uint64_t)id > max_id)
            max_id = (uint64_t)id;
    }
    *out_count = (uint32_t)max_id + 1u;
    return true;
}



/**
 * Build a dense categorical palette from a categorical colorizer.
 *
 * @param colorizer the categorical colorizer
 * @param fallback fallback color for missing ids
 * @param out_palette output palette entries
 * @param palette_count output palette entry count
 * @return whether the palette was filled
 */
bool _scene_colorizer_build_dense_palette(
    const DvzSceneColorizer* colorizer, DvzColor fallback, DvzColor* out_palette,
    uint32_t palette_count)
{
    ANN(out_palette);
    if (colorizer == NULL || colorizer->kind != DVZ_SCENE_COLORIZER_CATEGORICAL ||
        colorizer->scale == NULL)
    {
        return false;
    }
    for (uint32_t i = 0; i < palette_count; i++)
        out_palette[i] = fallback;

    const DvzScale* scale = colorizer->scale;
    for (uint32_t i = 0; i < scale->category_count; i++)
    {
        DvzCategoryId id = scale->categories[i].category_id;
        if (id < 0 || (uint64_t)id >= palette_count)
            return false;
        out_palette[(uint32_t)id] = scale->categories[i].color;
    }
    return true;
}

