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
