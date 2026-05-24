/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Font defaults                                                                                */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "datoviz/common/macros.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzFontDesc
{
    const char* path;
    const char* family;
    const char* style;
    uint32_t face_index;
    uint32_t flags;
};
typedef struct DvzFontDesc DvzFontDesc;


struct DvzFontDefaults
{
    DvzFontDesc sans;
    DvzFontDesc mono;
    float ui_size_px;
    float mono_size_px;
    float text_size_px;
};
typedef struct DvzFontDefaults DvzFontDefaults;



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return Datoviz's shared default font policy.
 *
 * The returned descriptors use borrowed static strings. Runtime consumers build their own backend
 * font objects from this policy; ImGui fonts and scene text atlas fonts are not shared.
 *
 * @return default font policy
 */
DVZ_EXPORT DvzFontDefaults dvz_font_defaults(void);


EXTERN_C_OFF
