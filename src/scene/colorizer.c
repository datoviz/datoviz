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
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_scene.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_SCENE_DENSE_LABEL_PALETTE_MAX 4096u



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


/**
 * Pack one RGBA8 color into a shader-readable unsigned word.
 *
 * @param color the source color
 * @return packed RGBA word, with red in the low byte
 */
static uint32_t _colorizer_pack_rgba(DvzColor color)
{
    return (uint32_t)color.r | ((uint32_t)color.g << 8u) | ((uint32_t)color.b << 16u) |
           ((uint32_t)color.a << 24u);
}


/**
 * Pack one category id into the 32-bit key stored in label volumes.
 *
 * @param id category id
 * @param signed_keys whether the source field is signed
 * @param out_key output packed key
 * @return whether the id is representable by the source label format
 */
static bool _colorizer_pack_label_key(DvzCategoryId id, bool signed_keys, uint32_t* out_key)
{
    ANN(out_key);
    if (signed_keys)
    {
        if (id < (DvzCategoryId)INT32_MIN || id > (DvzCategoryId)INT32_MAX)
            return false;
        *out_key = (uint32_t)(int32_t)id;
        return true;
    }
    if (id < 0 || (uint64_t)id > UINT32_MAX)
        return false;
    *out_key = (uint32_t)(uint64_t)id;
    return true;
}


/**
 * Sort label lookup entries by packed key for shader-side binary search.
 *
 * @param entries lookup entry array, with slot zero reserved for the header
 * @param count populated lookup entry count excluding the header
 */
static void _colorizer_sort_label_lookup(DvzSceneLabelLookupEntry* entries, uint32_t count)
{
    ANN(entries);
    for (uint32_t i = 2; i <= count; i++)
    {
        DvzSceneLabelLookupEntry current = entries[i];
        uint32_t j = i;
        while (j > 1 && entries[j - 1].key > current.key)
        {
            entries[j] = entries[j - 1];
            j--;
        }
        entries[j] = current;
    }
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
    if (max_id + 1ull > DVZ_SCENE_DENSE_LABEL_PALETTE_MAX)
        return false;
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


/**
 * Return the number of entries needed for a sparse label lookup buffer.
 *
 * @param colorizer the categorical colorizer
 * @param out_entry_count output entry count including the header slot
 * @return whether a sparse table can be represented
 */
bool _scene_colorizer_label_lookup_extent(
    const DvzSceneColorizer* colorizer, uint32_t* out_entry_count)
{
    ANN(out_entry_count);
    *out_entry_count = 1;
    if (colorizer == NULL || colorizer->kind != DVZ_SCENE_COLORIZER_CATEGORICAL ||
        colorizer->scale == NULL)
    {
        return true;
    }
    const DvzScale* scale = colorizer->scale;
    if (scale->category_count > UINT32_MAX - 1u)
        return false;
    *out_entry_count = scale->category_count + 1u;
    return true;
}


/**
 * Build a sparse label lookup table from a categorical scale.
 *
 * @param colorizer the categorical colorizer
 * @param signed_keys whether the source label field is signed
 * @param out_entries output table entries, with slot zero used as a header
 * @param entry_count allocated output entry count
 * @return whether the table was filled
 */
bool _scene_colorizer_build_label_lookup(
    const DvzSceneColorizer* colorizer, bool signed_keys, DvzSceneLabelLookupEntry* out_entries,
    uint32_t entry_count)
{
    ANN(out_entries);
    if (entry_count == 0)
        return false;
    dvz_memset(
        out_entries, (size_t)entry_count * sizeof(DvzSceneLabelLookupEntry), 0,
        (size_t)entry_count * sizeof(DvzSceneLabelLookupEntry));
    out_entries[0].flags = signed_keys ? 1u : 0u;
    if (colorizer == NULL || colorizer->kind != DVZ_SCENE_COLORIZER_CATEGORICAL ||
        colorizer->scale == NULL)
    {
        return true;
    }

    const DvzScale* scale = colorizer->scale;
    uint32_t write_index = 1;
    for (uint32_t i = 0; i < scale->category_count; i++)
    {
        if (write_index >= entry_count)
            return false;
        uint32_t key = 0;
        if (!_colorizer_pack_label_key(scale->categories[i].category_id, signed_keys, &key))
            continue;
        out_entries[write_index].key = key;
        out_entries[write_index].rgba = _colorizer_pack_rgba(scale->categories[i].color);
        out_entries[write_index].metadata_index = i;
        out_entries[write_index].flags = scale->categories[i].flags;
        write_index++;
    }
    out_entries[0].key = write_index - 1u;
    _colorizer_sort_label_lookup(out_entries, out_entries[0].key);
    return true;
}
