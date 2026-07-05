/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Font defaults                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/font.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return an empty font descriptor.
 *
 * @return default font descriptor
 */
DvzFontDesc dvz_font_desc(void)
{
    return (DvzFontDesc){DVZ_STRUCT_INIT_FIELDS(DvzFontDesc)};
}


/**
 * Return Datoviz's shared default font policy.
 *
 * @return default font policy
 */
DvzFontDefaults dvz_font_defaults(void)
{
    DvzFontDefaults defaults = {
        DVZ_STRUCT_INIT_FIELDS(DvzFontDefaults),
        .sans_path = NULL,
        .sans_family = "Roboto",
        .sans_style = "Regular",
        .sans_face_index = 0,
        .sans_font_flags = 0,
        .mono_path = NULL,
        .mono_family = "Roboto Mono",
        .mono_style = "Regular",
        .mono_face_index = 0,
        .mono_font_flags = 0,
        .ui_size_px = 16.0f,
        .mono_size_px = 16.0f,
        .text_size_px = 14.0f,
    };
    return defaults;
}
