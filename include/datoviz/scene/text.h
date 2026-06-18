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
#include "datoviz/scene/types.h"



EXTERN_C_ON

/*************************************************************************************************/
/*  Fonts and text                                                                               */
/*************************************************************************************************/

/**
 * Return the default retained text style.
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
 * Destroy a retained text object.
 *
 * @param text the text object
 */
DVZ_EXPORT void dvz_text_destroy(DvzText* text);


/**
 * Set the UTF-8 content of a retained text object.
 *
 * @param text the text object
 * @param string the string, or NULL to clear
 */
DVZ_EXPORT void dvz_text_set_string(DvzText* text, const char* string);


/**
 * Set the style of a retained text object.
 *
 * @param text the text object
 * @param style the style descriptor, or NULL for defaults
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_text_set_style(DvzText* text, const DvzTextStyle* style);


/**
 * Set the placement of a retained text object.
 *
 * @param text the text object
 * @param placement the placement descriptor, or NULL for defaults
 */
DVZ_EXPORT void dvz_text_set_placement(DvzText* text, const DvzTextPlacement* placement);


/**
 * Select the renderer used by a retained text object.
 *
 * @param text the text object
 * @param renderer renderer selection
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_text_set_renderer(DvzText* text, DvzTextRenderer renderer);


EXTERN_C_OFF
