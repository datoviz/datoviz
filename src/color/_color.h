/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Color                                                                                        */
/*************************************************************************************************/

#ifndef DVZ_HEADER_COLOR
#define DVZ_HEADER_COLOR



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <assert.h>
#include <libgen.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



/*************************************************************************************************/
/*  Colors                                                                                       */
/*************************************************************************************************/

// Colors
#define COLOR_UINT_MAX  255
#define COLOR_FLOAT_MAX 1.0

#define ALPHA_U2F(x) (x / 255.0)
#define ALPHA_F2U(x) ((uint8_t)round(CLIP((x), 0.0, 1.0) * 255.0))

#define COLOR_U2FV(r8, g8, b8, a8) ALPHA_U2F(r8), ALPHA_U2F(g8), ALPHA_U2F(b8), ALPHA_U2F(a8)
#define COLOR_U2F(rgba8)           COLOR_U2FV(rgba8[0], rgba8[1], rgba8[2], rgba8[3])

#define COLOR_F2UV(rf, gf, bf, af) ALPHA_F2U(rf), ALPHA_F2U(gf), ALPHA_F2U(bf), ALPHA_F2U(af)
#define COLOR_F2U(rgbaf)           COLOR_F2UV(rgbaf[0], rgbaf[1], rgbaf[2], rgbaf[3])



#define DVZ_FORMAT_COLOR DVZ_FORMAT_R8G8B8A8_UNORM
#define COLOR_MAX        COLOR_UINT_MAX

// convert from default to float
#define COLOR_D2F(rgba8)           COLOR_U2F(rgba8)
#define COLOR_D2FV(r8, g8, b8, a8) COLOR_U2FV(r8, g8, b8, a8)

// from default to uint
#define COLOR_D2U(rgba8)           rgba8
#define COLOR_D2UV(r8, g8, b8, a8) r8, g8, b8, a8

// from uint to default
#define COLOR_U2D(rgba8)           rgba8
#define COLOR_U2DV(r8, g8, b8, a8) r8, g8, b8, a8

// from float to default
#define COLOR_F2D(rgbaf) COLOR_F2U(rgbaf)

// from default to float
#define ALPHA_D2F(a) ALPHA_U2F(a)

// from float to default
#define ALPHA_F2D(a) ALPHA_F2U(a)

// from default to uint
#define ALPHA_D2U(a) a

// from uint to default
#define ALPHA_U2D(a) a


// HACK: other modules may define symbols with the same name
#ifdef WHITE
#undef WHITE
#endif

#ifdef BLACK
#undef BLACK
#endif

#ifdef GRAY
#undef GRAY
#endif

#define ALPHA_MAX      COLOR_MAX
#define GRAY(v8)       COLOR_U2DV(v8, v8, v8, 255)
#define WHITE          GRAY(255)
#define BLACK          GRAY(0)
#define RED            COLOR_U2DV(255, 0, 0, 255)
#define GREEN          COLOR_U2DV(0, 255, 0, 255)
#define BLUE           COLOR_U2DV(0, 0, 255, 255)
#define CYAN           COLOR_U2DV(0, 255, 255, 255)
#define PURPLE         COLOR_U2DV(255, 0, 255, 255)
#define YELLOW         COLOR_U2DV(0, 255, 255, 255)
#define WHITE_ALPHA(a) COLOR_U2DV(255, 255, 255, a)



#endif
