/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Default values                                                                               */
/*************************************************************************************************/

#ifndef DVZ_HEADER_PUBLIC_DEFAULTS
#define DVZ_HEADER_PUBLIC_DEFAULTS



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "datoviz_macros.h"



/*************************************************************************************************/
/*  Defaults                                                                                     */
/*************************************************************************************************/

#define DVZ_DEFAULT_FORMAT DVZ_FORMAT_B8G8R8A8_UNORM

#define DVZ_DEFAULT_VIEWPORT (vec2){0, 0}

#define DVZ_DEFAULT_CLEAR_COLOR (cvec4){0, 0, 0, 0}

#define DVZ_DEFAULT_LIGHT_POS (vec4){-2, +2, +10, 1}
// #define DVZ_DEFAULT_LIGHT1_POS (vec4){0, 8, 10, 1}
// #define DVZ_DEFAULT_LIGHT2_POS (vec4){-6, -5, 10, 1}
// #define DVZ_DEFAULT_LIGHT3_POS (vec4){6, -5, 10, 1}

#define DVZ_DEFAULT_LIGHT_DIR (vec4){0.577, -0.577, -0.577, 0.0}

#define DVZ_DEFAULT_LIGHT_COLOR WHITE
// #define DVZ_DEFAULT_LIGHT1_COLOR (cvec4){255,   0,   0, 255}
// #define DVZ_DEFAULT_LIGHT2_COLOR (cvec4){  0, 255,   0, 255}
// #define DVZ_DEFAULT_LIGHT3_COLOR (cvec4){  0,   0, 255, 255}

#define DVZ_DEFAULT_AMBIENT   (vec4){0.35, 0.35, 0.35} // R, G, B
#define DVZ_DEFAULT_DIFFUSE   (vec4){0.5, 0.5, 0.5}
#define DVZ_DEFAULT_SPECULAR  (vec4){0.1, 0.1, 0.1}
#define DVZ_DEFAULT_EMISSION  (vec4){0.0, 0.0, 0.0}
#define DVZ_DEFAULT_SHINE     1.0f
#define DVZ_DEFAULT_EMIT      0.0f
#define DVZ_DEFAULT_LINEWIDTH 0.1f
#define DVZ_DEFAULT_EDGECOLOR 0, 0, 0, 96



#endif
