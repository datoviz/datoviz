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
#include "_log.h"
#include "_scene.h"
#include "_visual_internal.h"
#include "core/scene_notify_internal.h"
#include "datoviz/scene.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

#define DVZ_MARKER_STYLE_KNOWN_FLAGS 0u

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
 * Return whether a symbol set contains a built-in symbol id.
 *
 * @param symbols the symbol set
 * @param id the symbol id
 * @return whether the id is present
 */
static bool _symbol_set_has_id(const DvzSymbolSet* symbols, DvzSymbolId id)
{
    if (symbols == NULL)
        return id <= (DvzSymbolId)DVZ_SYMBOL_ROUNDED_RECT;
    if (!symbols->active || id > (DvzSymbolId)DVZ_SYMBOL_ROUNDED_RECT)
        return false;
    return symbols->builtins[id];
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
        if (!_symbol_set_has_id(symbol_set, symbols[i]))
        {
            log_error("marker symbol id %u is not available in the bound symbol set", symbols[i]);
            return false;
        }
    }
    return true;
}
