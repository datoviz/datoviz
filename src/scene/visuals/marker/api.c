/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Marker visual API                                                                            */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "marker/internal.h"

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_scene.h"
#include "_visual_internal.h"
#include "core/scene_notify_internal.h"
#include "datoviz/scene.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

#define DVZ_MARKER_STYLE_KNOWN_FLAGS 0u
#define DVZ_SYMBOL_IMAGE_DESC_KNOWN_FLAGS 0u
#define DVZ_SYMBOL_CUSTOM_ID_BASE ((DvzSymbolId)DVZ_MARKER_SHAPE_ROUNDED_RECT + 1u)
#define DVZ_SYMBOL_ATLAS_GUTTER 1u

/**
 * Return whether a built-in symbol value is supported by the code-SDF marker path.
 *
 * @param builtin the built-in symbol value
 * @return whether the value is supported
 */
static bool _symbol_builtin_valid(DvzSymbolBuiltin builtin)
{
    return builtin >= DVZ_SYMBOL_DISC && builtin <= DVZ_SYMBOL_ROUNDED_RECT;
}



/**
 * Return whether a symbol id is supported by the current marker shader path.
 *
 * @param symbols the symbol set
 * @param id the symbol id
 * @return whether the id is present
 */
static bool _symbol_set_has_renderable_marker_id(const DvzSymbolSet* symbols, DvzSymbolId id)
{
    if (symbols == NULL)
        return id <= (DvzSymbolId)DVZ_SYMBOL_ROUNDED_RECT;
    if (!symbols->active || id > (DvzSymbolId)DVZ_SYMBOL_ROUNDED_RECT)
        return false;
    return symbols->builtins[id];
}



/**
 * Return the active texture-backed marker symbol atlas page.
 *
 * @param visual the marker visual
 * @param out_page output atlas page pointer
 * @param out_kind output symbol source kind
 * @return whether an active texture-backed symbol atlas page exists
 */
bool _scene_marker_symbol_atlas_page(
    DvzVisual* visual, const DvzSymbolAtlasPage** out_page, DvzSymbolSourceKind* out_kind)
{
    ANN(visual);
    ANN(out_page);
    ANN(out_kind);

    *out_page = NULL;
    *out_kind = DVZ_SYMBOL_SOURCE_NONE;
    if (visual->type != DVZ_VISUAL_TYPE_MARKER)
        return false;

    DvzVisualFamilyState* state = _visual_family_state(visual);
    if (state->symbol_source_kind == DVZ_SYMBOL_SOURCE_NONE || state->symbol_set == NULL)
        return false;

    DvzSymbolAtlasPage* page = &state->symbol_set->atlas_pages[state->symbol_source_kind];
    if (!page->active || page->data == NULL || page->byte_size == 0)
        return false;

    *out_page = page;
    *out_kind = state->symbol_source_kind;
    return true;
}



/**
 * Return a symbol-set source slot for one custom symbol id.
 *
 * @param symbols the symbol set
 * @param id the symbol id
 * @return source slot, or NULL when the id is not a registered custom source
 */
static const DvzSymbolSource* _symbol_set_source(const DvzSymbolSet* symbols, DvzSymbolId id)
{
    if (symbols == NULL || !symbols->active || id < DVZ_SYMBOL_CUSTOM_ID_BASE)
        return NULL;
    const uint32_t source_idx = id - DVZ_SYMBOL_CUSTOM_ID_BASE;
    if (source_idx >= symbols->source_count || source_idx >= DVZ_SCENE_MAX_SYMBOLS_PER_SET)
        return NULL;
    const DvzSymbolSource* source = &symbols->sources[source_idx];
    return source->active ? source : NULL;
}


/**
 * Classify one marker symbol payload for the active marker render path.
 *
 * @param symbol_set the bound symbol set, or NULL for built-ins only
 * @param symbols per-item symbol ids
 * @param item_count number of symbol ids
 * @param out_kind output symbol source kind
 * @return whether the payload is accepted
 */
static bool _marker_symbol_payload_kind(
    const DvzSymbolSet* symbol_set, const uint32_t* symbols, uint32_t item_count,
    DvzSymbolSourceKind* out_kind, float* out_distance_range_px)
{
    ANN(symbols);
    ANN(out_kind);
    *out_kind = DVZ_SYMBOL_SOURCE_NONE;
    if (out_distance_range_px != NULL)
        *out_distance_range_px = 0.0f;
    for (uint32_t i = 0; i < item_count; i++)
    {
        if (_symbol_set_has_renderable_marker_id(symbol_set, symbols[i]))
        {
            if (*out_kind != DVZ_SYMBOL_SOURCE_NONE)
            {
                log_error("marker symbol payload cannot mix built-in and texture-backed symbols");
                return false;
            }
            continue;
        }

        const DvzSymbolSource* source = _symbol_set_source(symbol_set, symbols[i]);
        if (source == NULL)
        {
            log_error("marker symbol id %u is not available in the bound symbol set", symbols[i]);
            return false;
        }
        if (source->kind != DVZ_SYMBOL_SOURCE_BITMAP && source->kind != DVZ_SYMBOL_SOURCE_SDF &&
            source->kind != DVZ_SYMBOL_SOURCE_MSDF)
        {
            log_error(
                "marker symbol id %u uses unsupported source kind %u", symbols[i],
                (uint32_t)source->kind);
            return false;
        }
        if (*out_kind != DVZ_SYMBOL_SOURCE_NONE && *out_kind != source->kind)
        {
            log_error("marker symbol payload cannot mix texture-backed symbol encodings");
            return false;
        }
        *out_kind = source->kind;
        if (source->kind == DVZ_SYMBOL_SOURCE_SDF || source->kind == DVZ_SYMBOL_SOURCE_MSDF)
        {
            float range = source->distance_range_px > 0.0f ? source->distance_range_px : 4.0f;
            if (out_distance_range_px != NULL)
            {
                if (*out_distance_range_px > 0.0f &&
                    fabsf(*out_distance_range_px - range) > 0.001f)
                {
                    log_error(
                        "marker distance-field symbols in one visual must share a distance range");
                    return false;
                }
                *out_distance_range_px = range;
            }
        }
    }
    return true;
}


/**
 * Return the channel count used by one image-backed symbol source kind.
 *
 * @param kind source kind
 * @return channel count, or zero for non-image sources
 */
static uint32_t _symbol_source_channels(DvzSymbolSourceKind kind)
{
    switch (kind)
    {
    case DVZ_SYMBOL_SOURCE_BITMAP:
        return 4;
    case DVZ_SYMBOL_SOURCE_SDF:
        return 1;
    case DVZ_SYMBOL_SOURCE_MSDF:
        return 3;
    default:
        return 0;
    }
}


/**
 * Return the channel count used by one image-backed symbol atlas page.
 *
 * @param kind source kind
 * @return atlas channel count, or zero for non-image sources
 */
static uint32_t _symbol_atlas_channels(DvzSymbolSourceKind kind)
{
    if (kind == DVZ_SYMBOL_SOURCE_MSDF)
        return 4;
    return _symbol_source_channels(kind);
}


/**
 * Rebuild one same-encoding symbol atlas page.
 *
 * @param symbols the symbol set
 * @param kind source kind
 * @return whether the page was rebuilt
 */
static bool _symbol_set_rebuild_atlas_page(DvzSymbolSet* symbols, DvzSymbolSourceKind kind)
{
    ANN(symbols);
    const uint32_t source_channels = _symbol_source_channels(kind);
    const uint32_t atlas_channels = _symbol_atlas_channels(kind);
    if (source_channels == 0 || atlas_channels == 0 || (uint32_t)kind > DVZ_SYMBOL_SOURCE_MSDF)
        return false;

    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t entry_count = 0;
    for (uint32_t i = 0; i < symbols->source_count; i++)
    {
        DvzSymbolSource* source = &symbols->sources[i];
        if (!source->active || source->kind != kind)
            continue;
        const uint32_t gutter = 2u * DVZ_SYMBOL_ATLAS_GUTTER;
        if (source->width > UINT32_MAX - gutter || source->height > UINT32_MAX - gutter)
            return false;
        const uint32_t padded_width = source->width + gutter;
        const uint32_t padded_height = source->height + gutter;
        if (UINT32_MAX - width < padded_width)
            return false;
        width += padded_width;
        height = MAX(height, padded_height);
        entry_count++;
    }

    DvzSymbolAtlasPage* page = &symbols->atlas_pages[kind];
    if (page->data != NULL)
    {
        dvz_free(page->data);
        page->data = NULL;
    }
    dvz_memset(page, sizeof(DvzSymbolAtlasPage), 0, sizeof(DvzSymbolAtlasPage));
    if (entry_count == 0)
        return true;

    const uint64_t row_stride = (uint64_t)width * atlas_channels;
    const uint64_t byte_size = row_stride * height;
    if (row_stride > UINT32_MAX || byte_size == 0 || byte_size > (uint64_t)SIZE_MAX)
        return false;
    uint8_t* atlas = (uint8_t*)dvz_calloc((DvzSize)byte_size, 1);
    if (atlas == NULL)
        return false;

    uint32_t x = 0;
    for (uint32_t i = 0; i < symbols->source_count; i++)
    {
        DvzSymbolSource* source = &symbols->sources[i];
        if (!source->active || source->kind != kind)
            continue;
        const uint32_t dst_x = x + DVZ_SYMBOL_ATLAS_GUTTER;
        const uint32_t dst_y = DVZ_SYMBOL_ATLAS_GUTTER;
        for (uint32_t y = 0; y < source->height; y++)
        {
            const uint64_t src_offset = (uint64_t)y * source->row_stride;
            const uint64_t dst_offset = ((uint64_t)(dst_y + y) * width + dst_x) * atlas_channels;
            if (source_channels == atlas_channels)
            {
                const uint64_t row_bytes = (uint64_t)source->width * atlas_channels;
                dvz_memcpy(
                    atlas + dst_offset, (DvzSize)row_bytes, source->data + src_offset,
                    (DvzSize)row_bytes);
            }
            else if (kind == DVZ_SYMBOL_SOURCE_MSDF && source_channels == 3 && atlas_channels == 4)
            {
                for (uint32_t px = 0; px < source->width; px++)
                {
                    const uint8_t* src = source->data + src_offset + 3u * px;
                    uint8_t* dst = atlas + dst_offset + 4u * px;
                    dst[0] = src[0];
                    dst[1] = src[1];
                    dst[2] = src[2];
                    dst[3] = 255u;
                }
            }
            else
            {
                dvz_free(atlas);
                return false;
            }
        }
        source->atlas_x = dst_x;
        source->atlas_y = dst_y;
        source->atlas_uv[0] = ((float)dst_x + 0.5f) / (float)width;
        source->atlas_uv[1] = ((float)dst_y + 0.5f) / (float)height;
        source->atlas_uv[2] = ((float)(dst_x + source->width) - 0.5f) / (float)width;
        source->atlas_uv[3] = ((float)(dst_y + source->height) - 0.5f) / (float)height;
        x += source->width + 2u * DVZ_SYMBOL_ATLAS_GUTTER;
    }

    page->active = true;
    page->dirty = true;
    page->kind = kind;
    page->width = width;
    page->height = height;
    page->channels = atlas_channels;
    page->row_stride = (uint32_t)row_stride;
    page->byte_size = byte_size;
    page->data = atlas;
    return true;
}


/**
 * Update generated per-item marker atlas UV rectangles.
 *
 * @param visual the marker visual
 * @param symbols per-item symbol ids
 * @param item_count number of symbol ids
 * @return whether generated state was updated
 */
static bool _marker_update_symbol_tex_rects(
    DvzVisual* visual, const uint32_t* symbols, uint32_t item_count)
{
    ANN(visual);
    ANN(symbols);
    DvzSymbolSourceKind kind = DVZ_SYMBOL_SOURCE_NONE;
    float distance_range_px = 0.0f;
    DvzSymbolSet* symbol_set = _visual_family_state(visual)->symbol_set;
    if (!_marker_symbol_payload_kind(symbol_set, symbols, item_count, &kind, &distance_range_px))
        return false;

    _visual_family_state(visual)->symbol_source_kind = kind;
    _visual_family_state(visual)->glyph_atlas_encoding = DVZ_TEXT_ATLAS_ENCODING_BITMAP_ALPHA;
    _visual_family_state(visual)->glyph_distance_range_px = 0.0f;
    if (kind == DVZ_SYMBOL_SOURCE_SDF)
    {
        _visual_family_state(visual)->glyph_atlas_encoding = DVZ_TEXT_ATLAS_ENCODING_SDF_ALPHA;
        _visual_family_state(visual)->glyph_distance_range_px = distance_range_px;
    }
    else if (kind == DVZ_SYMBOL_SOURCE_MSDF)
    {
        _visual_family_state(visual)->glyph_atlas_encoding = DVZ_TEXT_ATLAS_ENCODING_MSDF_RGB;
        _visual_family_state(visual)->glyph_distance_range_px = distance_range_px;
    }
    if (kind == DVZ_SYMBOL_SOURCE_NONE)
        return true;

    float* rects = (float*)dvz_calloc(item_count, 4 * sizeof(float));
    if (rects == NULL)
    {
        log_error("marker symbol tex_rect allocation failed");
        return false;
    }
    for (uint32_t i = 0; i < item_count; i++)
    {
        const DvzSymbolSource* source = _symbol_set_source(symbol_set, symbols[i]);
        if (source == NULL)
        {
            dvz_free(rects);
            return false;
        }
        rects[4 * i + 0] = source->atlas_uv[0];
        rects[4 * i + 1] = source->atlas_uv[1];
        rects[4 * i + 2] = source->atlas_uv[2];
        rects[4 * i + 3] = source->atlas_uv[3];
    }
    int ret = dvz_visual_set_data(visual, "tex_rect", rects, item_count);
    dvz_free(rects);
    return ret == 0;
}


/**
 * Register a copied image-backed symbol source.
 *
 * @param symbols the symbol set
 * @param kind source kind
 * @param name optional diagnostic name
 * @param data source payload
 * @param width source width in pixels
 * @param height source height in pixels
 * @param desc optional image descriptor
 * @param channels channel count
 * @return the symbol id, or DVZ_SYMBOL_ID_INVALID on error
 */
static DvzSymbolId _symbol_image_source(
    DvzSymbolSet* symbols, DvzSymbolSourceKind kind, const char* name, const void* data,
    uint32_t width, uint32_t height, const DvzSymbolImageDesc* desc, uint32_t channels)
{
    if (symbols == NULL || !symbols->active)
        return DVZ_SYMBOL_ID_INVALID;
    if (data == NULL || width == 0 || height == 0 || channels == 0)
        return DVZ_SYMBOL_ID_INVALID;
    if (symbols->source_count >= DVZ_SCENE_MAX_SYMBOLS_PER_SET)
        return DVZ_SYMBOL_ID_INVALID;
    if (desc != NULL && !DVZ_STRUCT_VALID(desc, DvzSymbolImageDesc, DVZ_SYMBOL_IMAGE_DESC_KNOWN_FLAGS))
        return DVZ_SYMBOL_ID_INVALID;

    DvzSymbolImageDesc defaults = dvz_symbol_image_desc();
    DvzSymbolImageDesc image_desc = desc != NULL ? *desc : defaults;
    const uint32_t tight_stride = width * channels;
    if (image_desc.row_stride == 0)
        image_desc.row_stride = tight_stride;
    if (image_desc.row_stride < tight_stride)
        return DVZ_SYMBOL_ID_INVALID;

    const uint64_t byte_size = (uint64_t)image_desc.row_stride * (uint64_t)height;
    if (byte_size == 0 || byte_size > (uint64_t)SIZE_MAX)
        return DVZ_SYMBOL_ID_INVALID;
    uint8_t* copy = (uint8_t*)dvz_calloc((DvzSize)byte_size, 1);
    if (copy == NULL)
        return DVZ_SYMBOL_ID_INVALID;
    dvz_memcpy(copy, (size_t)byte_size, data, (size_t)byte_size);

    const uint32_t idx = symbols->source_count++;
    DvzSymbolSource* source = &symbols->sources[idx];
    dvz_memset(source, sizeof(DvzSymbolSource), 0, sizeof(DvzSymbolSource));
    source->active = true;
    source->kind = kind;
    if (name != NULL)
        dvz_strlcpy(source->name, name, sizeof(source->name));
    source->width = width;
    source->height = height;
    source->channels = channels;
    source->row_stride = image_desc.row_stride;
    source->distance_range_px = image_desc.distance_range_px;
    source->byte_size = byte_size;
    source->data = copy;
    if (!_symbol_set_rebuild_atlas_page(symbols, kind))
    {
        source->active = false;
        dvz_free(source->data);
        source->data = NULL;
        symbols->source_count--;
        return DVZ_SYMBOL_ID_INVALID;
    }
    return DVZ_SYMBOL_CUSTOM_ID_BASE + idx;
}



/**
 * Return default marker styling.
 *
 * @return default marker style descriptor
 */
DvzMarkerStyle dvz_marker_style(void)
{
    DvzMarkerStyle style = {
        DVZ_STRUCT_INIT_FIELDS(DvzMarkerStyle),
        .edge_color = {0, 0, 0, 255},
        .stroke_width_px = 0.0f,
        .aspect = DVZ_SHAPE_ASPECT_FILLED,
    };
    return style;
}



/**
 * Convert a marker style to the shared point-like material payload.
 *
 * @param style the marker style
 * @return equivalent point style descriptor
 */
DvzPointStyleDesc _marker_style_to_point_style(const DvzMarkerStyle* style)
{
    ANN(style);
    DvzPointStyleDesc out = {
        DVZ_STRUCT_INIT_FIELDS(DvzPointStyleDesc),
        .edge_color = style->edge_color,
        .stroke_width_px = style->stroke_width_px,
        .aspect = style->aspect,
    };
    return out;
}



/**
 * Create a marker visual.
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual, or NULL on allocation failure
 */
DvzVisual* dvz_marker(DvzScene* scene, uint32_t flags)
{
    ANN(scene);
    DvzVisual* visual = _scene_alloc_visual(scene, DVZ_VISUAL_TYPE_MARKER, flags);
    if (visual == NULL)
        return NULL;
    _visual_family_state(visual)->topology = DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST;
    _visual_family_state(visual)->material_params_dirty = true;
    return visual;
}


/**
 * Create a scene-owned reusable symbol set.
 *
 * @param scene the scene
 * @param flags reserved flags
 * @return the symbol set, or NULL on error
 */
DvzSymbolSet* dvz_symbol_set(DvzScene* scene, uint32_t flags)
{
    ANN(scene);
    if (scene->symbol_set_count >= DVZ_SCENE_MAX_SYMBOL_SETS)
        return NULL;

    DvzSymbolSet* symbols = &scene->symbol_sets[scene->symbol_set_count++];
    dvz_memset(symbols, sizeof(DvzSymbolSet), 0, sizeof(DvzSymbolSet));
    symbols->scene = scene;
    symbols->flags = flags;
    symbols->active = true;
    return symbols;
}



/**
 * Register or return a built-in symbol id in one symbol set.
 *
 * @param symbols the symbol set
 * @param builtin the built-in symbol
 * @return the symbol id, or DVZ_SYMBOL_ID_INVALID on error
 */
DvzSymbolId dvz_symbol_builtin(DvzSymbolSet* symbols, DvzSymbolBuiltin builtin)
{
    if (symbols == NULL || !symbols->active || !_symbol_builtin_valid(builtin))
        return DVZ_SYMBOL_ID_INVALID;

    const DvzSymbolId id = (DvzSymbolId)builtin;
    if (!symbols->builtins[id])
    {
        symbols->builtins[id] = true;
        symbols->builtin_count++;
    }
    return id;
}


/**
 * Return default image-backed symbol source options.
 *
 * @return default symbol image descriptor
 */
DvzSymbolImageDesc dvz_symbol_image_desc(void)
{
    return (DvzSymbolImageDesc){DVZ_STRUCT_INIT_FIELDS(DvzSymbolImageDesc)};
}


/**
 * Register an RGBA bitmap symbol source in one symbol set.
 *
 * @param symbols the symbol set
 * @param name optional diagnostic name
 * @param rgba RGBA8 payload
 * @param width source width in pixels
 * @param height source height in pixels
 * @param desc optional image source options
 * @return the symbol id, or DVZ_SYMBOL_ID_INVALID on error
 */
DvzSymbolId dvz_symbol_bitmap(
    DvzSymbolSet* symbols, const char* name, const void* rgba, uint32_t width, uint32_t height,
    const DvzSymbolImageDesc* desc)
{
    return _symbol_image_source(
        symbols, DVZ_SYMBOL_SOURCE_BITMAP, name, rgba, width, height, desc, 4);
}


/**
 * Register a single-channel SDF symbol source in one symbol set.
 *
 * @param symbols the symbol set
 * @param name optional diagnostic name
 * @param sdf R8 SDF payload
 * @param width source width in pixels
 * @param height source height in pixels
 * @param desc optional image source options
 * @return the symbol id, or DVZ_SYMBOL_ID_INVALID on error
 */
DvzSymbolId dvz_symbol_sdf(
    DvzSymbolSet* symbols, const char* name, const void* sdf, uint32_t width, uint32_t height,
    const DvzSymbolImageDesc* desc)
{
    return _symbol_image_source(symbols, DVZ_SYMBOL_SOURCE_SDF, name, sdf, width, height, desc, 1);
}


/**
 * Register an RGB MSDF symbol source in one symbol set.
 *
 * @param symbols the symbol set
 * @param name optional diagnostic name
 * @param msdf RGB8 MSDF payload
 * @param width source width in pixels
 * @param height source height in pixels
 * @param desc optional image source options
 * @return the symbol id, or DVZ_SYMBOL_ID_INVALID on error
 */
DvzSymbolId dvz_symbol_msdf(
    DvzSymbolSet* symbols, const char* name, const void* msdf, uint32_t width, uint32_t height,
    const DvzSymbolImageDesc* desc)
{
    return _symbol_image_source(symbols, DVZ_SYMBOL_SOURCE_MSDF, name, msdf, width, height, desc, 3);
}



/**
 * Bind a reusable symbol set to a marker visual.
 *
 * @param visual the marker visual
 * @param symbols the symbol set, or NULL to clear
 * @return 0 on success, -1 on error
 */
DvzResult dvz_marker_set_symbols(DvzVisual* visual, DvzSymbolSet* symbols)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_MARKER)
    {
        log_error("dvz_marker_set_symbols requires a marker visual");
        return -1;
    }
    if (symbols != NULL && symbols->scene != visual->scene)
    {
        log_error("cannot bind a symbol set from a different scene");
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "bind marker symbols"))
        return -1;

    _visual_family_state(visual)->symbol_set = symbols;
    _visual_family_state(visual)->symbol_source_kind = DVZ_SYMBOL_SOURCE_NONE;
    _visual_family_state(visual)->glyph_atlas_encoding = DVZ_TEXT_ATLAS_ENCODING_BITMAP_ALPHA;
    _visual_family_state(visual)->glyph_distance_range_px = 0.0f;
    const int shape_idx = _attr_index(visual, "shape");
    if (shape_idx >= 0 && visual->attrs[shape_idx].data != NULL &&
        visual->attrs[shape_idx].item_count > 0)
    {
        if (!_marker_update_symbol_tex_rects(
                visual, (const uint32_t*)visual->attrs[shape_idx].data,
                (uint32_t)visual->attrs[shape_idx].item_count))
        {
            return -1;
        }
    }
    _scene_notify_visual_changed(visual);
    return 0;
}



/**
 * Set every existing marker item to one built-in symbol.
 *
 * @param visual the marker visual
 * @param builtin the built-in symbol
 * @return 0 on success, -1 on error
 */
DvzResult dvz_marker_set_symbol(DvzVisual* visual, DvzSymbolBuiltin builtin)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_MARKER)
    {
        log_error("dvz_marker_set_symbol requires a marker visual");
        return -1;
    }
    if (!_symbol_builtin_valid(builtin))
    {
        log_error("invalid marker built-in symbol");
        return -1;
    }
    DvzSymbolSet* symbol_set = _visual_family_state(visual)->symbol_set;
    if (symbol_set != NULL && dvz_symbol_builtin(symbol_set, builtin) == DVZ_SYMBOL_ID_INVALID)
    {
        log_error("marker built-in symbol could not be registered in the bound symbol set");
        return -1;
    }

    const int pos_idx = _attr_index(visual, "position");
    if (pos_idx < 0 || visual->attrs[pos_idx].item_count == 0)
    {
        log_error("dvz_marker_set_symbol requires existing marker position data");
        return -1;
    }
    if (visual->attrs[pos_idx].item_count > UINT32_MAX)
    {
        log_error("marker item count exceeds supported symbol update size");
        return -1;
    }
    const uint32_t item_count = (uint32_t)visual->attrs[pos_idx].item_count;
    uint32_t* symbols = (uint32_t*)dvz_calloc(item_count, sizeof(uint32_t));
    if (symbols == NULL)
    {
        log_error("marker symbol allocation failed");
        return -1;
    }
    for (uint32_t i = 0; i < item_count; i++)
        symbols[i] = (uint32_t)builtin;

    const int ret = dvz_visual_set_data(visual, "symbol", symbols, item_count);
    dvz_free(symbols);
    return ret;
}



/**
 * Configure marker fill/stroke styling.
 *
 * @param visual the marker visual
 * @param style the marker style descriptor, or NULL to restore defaults
 * @return 0 on success, -1 on error
 */
DvzResult dvz_marker_set_style(DvzVisual* visual, const DvzMarkerStyle* style)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_MARKER)
    {
        log_error("dvz_marker_set_style requires a marker visual");
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "update marker style"))
        return -1;

    if (style != NULL && !DVZ_STRUCT_VALID(style, DvzMarkerStyle, DVZ_MARKER_STYLE_KNOWN_FLAGS))
    {
        log_error("invalid DvzMarkerStyle ABI prologue");
        return -1;
    }
    DvzMarkerStyle marker_style = style != NULL ? *style : dvz_marker_style();
    if (!isfinite(marker_style.stroke_width_px) || marker_style.stroke_width_px < 0.0f)
    {
        log_error("marker stroke_width_px must be finite and nonnegative");
        return -1;
    }
    if (marker_style.aspect < DVZ_SHAPE_ASPECT_FILLED ||
        marker_style.aspect > DVZ_SHAPE_ASPECT_OUTLINE)
    {
        log_error("marker aspect must be filled, stroke, or outline");
        return -1;
    }

    DvzPointStyleDesc point_style = _marker_style_to_point_style(&marker_style);
    visual->material.point_style = point_style;
    visual->material.point_style_enabled = _point_style_enabled(&point_style);
    _visual_material_mark_dirty(visual);
    return 0;
}



/**
 * Validate marker-specific attribute payloads.
 *
 * @param visual the visual
 * @param attr_name the storage attribute name
 * @param data attribute payload
 * @param item_count number of items
 * @return whether the payload is valid
 */
bool _scene_marker_visual_validate_attr(
    const DvzVisual* visual, const char* attr_name, const void* data, uint32_t item_count)
{
    ANN(visual);
    ANN(attr_name);
    ANN(data);
    if (strcmp(attr_name, "shape") != 0)
        return true;

    const uint32_t* symbols = (const uint32_t*)data;
    const DvzSymbolSet* symbol_set = _visual_family_state(visual)->symbol_set;
    DvzSymbolSourceKind kind = DVZ_SYMBOL_SOURCE_NONE;
    return _marker_symbol_payload_kind(symbol_set, symbols, item_count, &kind, NULL);
}


/**
 * Update marker-generated attrs after dense payload changes.
 *
 * @param visual the visual
 * @param attr_name changed storage attribute name
 * @param item_count number of changed items
 * @return whether generated attrs were updated
 */
bool _scene_marker_visual_after_attr_set(
    DvzVisual* visual, const char* attr_name, uint32_t item_count)
{
    ANN(visual);
    ANN(attr_name);
    (void)item_count;
    if (strcmp(attr_name, "shape") != 0)
        return true;

    const int shape_idx = _attr_index(visual, "shape");
    if (shape_idx < 0 || visual->attrs[shape_idx].data == NULL)
        return true;
    if (visual->attrs[shape_idx].item_count > UINT32_MAX)
    {
        log_error("marker symbol count exceeds generated tex_rect capacity");
        return false;
    }
    return _marker_update_symbol_tex_rects(
        visual, (const uint32_t*)visual->attrs[shape_idx].data,
        (uint32_t)visual->attrs[shape_idx].item_count);
}
