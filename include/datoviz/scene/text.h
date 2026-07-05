/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene text                                                                                   */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/common/macros.h"
#include "datoviz/common/types.h"
#include "datoviz/scene/types.h"



EXTERN_C_ON

/*************************************************************************************************/
/*  Fonts and text                                                                               */
/*************************************************************************************************/

/**
 * Return the default retained text style.
 *
 * The returned style leaves `size_px` unresolved as 0.0f; retained text resolves that value from
 * the owning scene font defaults. Set a positive `size_px` to force an explicit text size.
 *
 * @return default text style
 */
DVZ_EXPORT DvzTextStyle dvz_text_style(void);


/**
 * Return the default retained text placement.
 *
 * The default is panel-local screen placement. Use `DVZ_TEXT_PLACEMENT_DATA` when text should stay
 * anchored to panel data coordinates; `DvzTextPlacement::offset` remains a logical-pixel offset in
 * every placement mode.
 *
 * @return default text placement
 */
DVZ_EXPORT DvzTextPlacement dvz_text_placement(void);


/**
 * Return the default retained text layout.
 *
 * @return default text layout
 */
DVZ_EXPORT DvzTextLayout dvz_text_layout(void);


/**
 * Create a scene-owned font resource.
 *
 * @param scene the scene
 * @param desc the font descriptor
 * @return the font
 */
DVZ_EXPORT DvzFont* dvz_font(DvzScene* scene, const DvzFontDesc* desc);


/**
 * Destroy a font resource.
 *
 * @param font the font
 */
DVZ_EXPORT void dvz_font_destroy(DvzFont* font);


/**
 * Resolve a text atlas specification from a renderer and rendered text size.
 *
 * @param renderer requested text renderer
 * @param size_px rendered text size in logical pixels
 * @return atlas generation spec
 */
DVZ_EXPORT DvzTextAtlasSpec dvz_text_atlas_spec(DvzTextRenderer renderer, float size_px);


/**
 * Ensure a font has an atlas for Datoviz's default text glyph set.
 *
 * @param font the font
 * @param spec requested atlas spec
 * @return true when the atlas is available
 */
DVZ_EXPORT bool dvz_font_atlas_ensure(DvzFont* font, const DvzTextAtlasSpec* spec);


/**
 * Ensure a font has an atlas that covers one UTF-8 string.
 *
 * @param font the font
 * @param spec requested atlas spec
 * @param string the UTF-8 string
 * @return true when the atlas is available
 */
DVZ_EXPORT bool
dvz_font_atlas_ensure_string(DvzFont* font, const DvzTextAtlasSpec* spec, const char* string);


/**
 * Ensure a font has an atlas that covers a list of UTF-8 strings.
 *
 * @param font the font
 * @param spec requested atlas spec
 * @param strings UTF-8 strings
 * @param count string count
 * @return true when the atlas is available
 */
DVZ_EXPORT bool dvz_font_atlas_ensure_strings(
    DvzFont* font, const DvzTextAtlasSpec* spec, const char* const* strings, uint32_t count);


/**
 * Return the font atlas matching a spec.
 *
 * The returned atlas is owned by the font's scene and remains valid until the font or scene is
 * destroyed. When the requested renderer falls back internally, this function returns the fallback
 * atlas.
 *
 * @param font the font
 * @param spec requested atlas spec
 * @return atlas pointer, or NULL when unavailable
 */
DVZ_EXPORT const DvzTextAtlas*
dvz_font_atlas(const DvzFont* font, const DvzTextAtlasSpec* spec);


/**
 * Return immutable atlas metadata.
 *
 * @param atlas the text atlas
 * @return atlas metadata; zeroed when atlas is NULL
 */
DVZ_EXPORT DvzTextAtlasInfo dvz_text_atlas_info(const DvzTextAtlas* atlas);


/**
 * Return the sampled field containing the atlas texture.
 *
 * The field is owned by the atlas's font scene. It may be bound to glyph visuals but must not be
 * destroyed or mutated by the caller.
 *
 * @param atlas the text atlas
 * @return sampled atlas field, or NULL
 */
DVZ_EXPORT DvzSampledField* dvz_text_atlas_field(const DvzTextAtlas* atlas);


/**
 * Return one atlas glyph, falling back to '?' for unsupported codepoints.
 *
 * @param atlas the text atlas
 * @param codepoint Unicode codepoint
 * @return glyph metrics, or NULL when unavailable
 */
DVZ_EXPORT const DvzTextAtlasGlyph*
dvz_text_atlas_glyph(const DvzTextAtlas* atlas, uint32_t codepoint);


/**
 * Create a retained text object attached to a panel.
 *
 * The text object owns semantic string, style, placement, and renderer state. Rendering lowers to
 * an internal glyph visual during frame preparation.
 *
 * @param panel the panel
 * @param flags creation flags
 * @return the text object, or NULL on allocation failure
 */
DVZ_EXPORT DvzText* dvz_text(DvzPanel* panel, uint32_t flags);


/**
 * Return the scene-local identity of a text object.
 *
 * @param text the text object
 * @return the scene-local identity, or DVZ_ID_NONE when text is NULL or destroyed
 */
DVZ_EXPORT DvzId dvz_text_id(const DvzText* text);


/**
 * Destroy a retained text object.
 *
 * @param text the text object
 */
DVZ_EXPORT void dvz_text_destroy(DvzText* text);


/**
 * Replace all items in a retained text collection.
 *
 * Strings, positions, offsets, anchors, sizes, colors, and angles are copied before return. Passing
 * `item_count == 0` clears the collection; otherwise `items` must point to `item_count` entries.
 * Use this function when several per-item properties should change atomically.
 *
 * @param text the text object
 * @param items text items to copy, or NULL when `item_count` is 0
 * @param item_count number of text items
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT DvzResult dvz_text_set_items(
    DvzText* text, const DvzTextItem* items, uint32_t item_count);


/**
 * Set the UTF-8 content of a retained one-item text collection.
 *
 * @param text the text object
 * @param string the string, or NULL to clear
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT DvzResult dvz_text_set_string(DvzText* text, const char* string);


/**
 * Set the position of a retained one-item text collection.
 *
 * @param text the text object
 * @param position the position in the current placement mode
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT DvzResult dvz_text_set_position(DvzText* text, const double position[3]);


/**
 * Set the layout of a retained text collection.
 *
 * @param text the text object
 * @param layout the layout descriptor, or NULL for defaults
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT DvzResult dvz_text_set_layout(DvzText* text, const DvzTextLayout* layout);


/**
 * Replace the UTF-8 strings of an existing retained text collection.
 *
 * String contents are copied before return. `item_count` must match the current collection item
 * count.
 *
 * @param text the text object
 * @param strings UTF-8 strings to copy
 * @param item_count number of strings
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT DvzResult
dvz_text_set_strings(DvzText* text, const char* const* strings, uint32_t item_count);


/**
 * Replace the positions of an existing retained text collection.
 *
 * Positions are copied before return. `item_count` must match the current collection item count.
 *
 * @param text the text object
 * @param positions positions in the current placement mode
 * @param item_count number of positions
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT DvzResult
dvz_text_set_positions(DvzText* text, const double (*positions)[3], uint32_t item_count);


/**
 * Replace the logical-pixel offsets of an existing retained text collection.
 *
 * Offsets are copied before return. `item_count` must match the current collection item count.
 *
 * @param text the text object
 * @param offsets logical-pixel offsets
 * @param item_count number of offsets
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT DvzResult
dvz_text_set_offsets(DvzText* text, const float (*offsets)[2], uint32_t item_count);


/**
 * Replace the text anchors of an existing retained text collection.
 *
 * Anchors are copied before return. `item_count` must match the current collection item count.
 *
 * @param text the text object
 * @param anchors normalized text anchors
 * @param item_count number of anchors
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT DvzResult
dvz_text_set_anchors(DvzText* text, const float (*anchors)[2], uint32_t item_count);


/**
 * Replace the text sizes of an existing retained text collection.
 *
 * Sizes are copied before return. `item_count` must match the current collection item count.
 *
 * @param text the text object
 * @param sizes_px logical-pixel text sizes
 * @param item_count number of sizes
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT DvzResult dvz_text_set_sizes(DvzText* text, const float* sizes_px, uint32_t item_count);


/**
 * Replace the colors of an existing retained text collection.
 *
 * Colors are copied before return. `item_count` must match the current collection item count.
 *
 * @param text the text object
 * @param colors text colors
 * @param item_count number of colors
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT DvzResult dvz_text_set_colors(DvzText* text, const DvzColor* colors, uint32_t item_count);


/**
 * Replace the rotation angles of an existing retained text collection.
 *
 * Angles are copied before return. `item_count` must match the current collection item count.
 *
 * @param text the text object
 * @param angles rotation angles in radians
 * @param item_count number of angles
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT DvzResult dvz_text_set_angles(DvzText* text, const float* angles, uint32_t item_count);


/**
 * Set the style of a retained text object.
 *
 * @param text the text object
 * @param style the style descriptor, or NULL for defaults
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT DvzResult dvz_text_set_style(DvzText* text, const DvzTextStyle* style);


/**
 * Set the placement of a retained text object.
 *
 * @param text the text object
 * @param placement the placement descriptor, or NULL for defaults
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT DvzResult dvz_text_set_placement(DvzText* text, const DvzTextPlacement* placement);


/**
 * Select the renderer used by a retained text object.
 *
 * @param text the text object
 * @param renderer renderer selection
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT DvzResult dvz_text_set_renderer(DvzText* text, DvzTextRenderer renderer);


EXTERN_C_OFF
