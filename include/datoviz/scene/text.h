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
 * @param panel the panel
 * @param desc the text descriptor
 * @return the text object
 */
DVZ_EXPORT DvzText* dvz_text(DvzPanel* panel, const DvzTextDesc* desc);


/**
 * Destroy a retained text object.
 *
 * @param text the text object
 */
DVZ_EXPORT void dvz_text_destroy(DvzText* text);


/**
 * Update the string content of a retained text object.
 *
 * @param text the text object
 * @param string the string content
 */
DVZ_EXPORT void dvz_text_set_string(DvzText* text, const char* string);


/**
 * Update the style of a retained text object.
 *
 * @param text the text object
 * @param style the style descriptor
 */
DVZ_EXPORT void dvz_text_set_style(DvzText* text, const DvzTextStyle* style);


/**
 * Update the placement of a retained text object.
 *
 * @param text the text object
 * @param placement the placement descriptor
 */
DVZ_EXPORT void dvz_text_set_placement(DvzText* text, const DvzTextPlacement* placement);


EXTERN_C_OFF
