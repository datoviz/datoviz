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
 * Return Datoviz's shared default font policy.
 *
 * @return default font policy
 */
DvzFontDefaults dvz_font_defaults(void)
{
    DvzFontDefaults defaults = {
        .sans =
            {
                .path = NULL,
                .family = "Roboto",
                .style = "Regular",
                .face_index = 0,
                .flags = 0,
            },
        .mono =
            {
                .path = NULL,
                .family = "Roboto Mono",
                .style = "Regular",
                .face_index = 0,
                .flags = 0,
            },
        .ui_size_px = 16.0f,
        .mono_size_px = 16.0f,
        .text_size_px = 14.0f,
    };
    return defaults;
}
