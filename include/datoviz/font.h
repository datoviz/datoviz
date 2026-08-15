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
    uint32_t struct_size;
    uint32_t flags;
    const char* path;
    const char* family;
    const char* style;
    uint32_t face_index;
    uint32_t font_flags;
};
typedef struct DvzFontDesc DvzFontDesc;


struct DvzFontDefaults
{
    uint32_t struct_size;
    uint32_t flags;
    const char* sans_path;
    const char* sans_family;
    const char* sans_style;
    uint32_t sans_face_index;
    uint32_t sans_font_flags;
    const char* mono_path;
    const char* mono_family;
    const char* mono_style;
    uint32_t mono_face_index;
    uint32_t mono_font_flags;
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
 * Return an empty font descriptor.
 *
 * Set `path` to select a font file. `family` and `style` are copied as diagnostic identity and do
 * not perform operating-system font discovery. `face_index` selects a face in a font collection.
 * `font_flags` is reserved for loader policy; no portable public flag bits are defined yet.
 *
 * @return default font descriptor
 */
DVZ_EXPORT DvzFontDesc dvz_font_desc(void);


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
