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
        .stroke_width = 0.0f,
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
        .stroke_width = style->stroke_width,
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
int dvz_marker_set_symbols(DvzVisual* visual, DvzSymbolSet* symbols)
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
int dvz_marker_set_symbol(DvzVisual* visual, DvzSymbolBuiltin builtin)
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
int dvz_marker_set_style(DvzVisual* visual, const DvzMarkerStyle* style)
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
    if (!isfinite(marker_style.stroke_width) || marker_style.stroke_width < 0.0f)
    {
        log_error("marker stroke_width must be finite and nonnegative");
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
    for (uint32_t i = 0; i < item_count; i++)
    {
        if (!_symbol_set_has_renderable_marker_id(symbol_set, symbols[i]))
        {
            const DvzSymbolSource* source = _symbol_set_source(symbol_set, symbols[i]);
            if (source != NULL)
            {
                log_error(
                    "marker texture-backed symbol id %u requires the atlas-backed marker pipeline",
                    symbols[i]);
            }
            else
            {
                log_error(
                    "marker symbol id %u is not available in the bound symbol set", symbols[i]);
            }
            return false;
        }
    }
    return true;
}
