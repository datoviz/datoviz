/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Common types                                                                                 */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzColor DvzColor;
typedef struct DvzColorf DvzColorf;
typedef struct DvzTime   DvzTime;

typedef int32_t DvzResult;
typedef intptr_t DvzExternalHandle;



/*************************************************************************************************/
/*  Result codes                                                                                 */
/*************************************************************************************************/

#define DVZ_OK    ((DvzResult)0)
#define DVZ_ERROR ((DvzResult)-1)

#define DVZ_EXTERNAL_HANDLE_INVALID ((DvzExternalHandle)-1)



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzColor
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};



struct DvzColorf
{
    float r;
    float g;
    float b;
    float a;
};



struct DvzTime
{
    uint64_t seconds;
    uint64_t nanoseconds;
};



#if defined(__cplusplus)
static_assert(sizeof(DvzColor) == 4, "DvzColor must stay a compact RGBA8 value");
static_assert(offsetof(DvzColor, r) == 0, "DvzColor.r must be byte 0");
static_assert(offsetof(DvzColor, g) == 1, "DvzColor.g must be byte 1");
static_assert(offsetof(DvzColor, b) == 2, "DvzColor.b must be byte 2");
static_assert(offsetof(DvzColor, a) == 3, "DvzColor.a must be byte 3");
#else
_Static_assert(sizeof(DvzColor) == 4, "DvzColor must stay a compact RGBA8 value");
_Static_assert(offsetof(DvzColor, r) == 0, "DvzColor.r must be byte 0");
_Static_assert(offsetof(DvzColor, g) == 1, "DvzColor.g must be byte 1");
_Static_assert(offsetof(DvzColor, b) == 2, "DvzColor.b must be byte 2");
_Static_assert(offsetof(DvzColor, a) == 3, "DvzColor.a must be byte 3");
#endif



/*************************************************************************************************/
/*  Color helpers                                                                                */
/*************************************************************************************************/

/**
 * Convert a normalized scalar to an unsigned byte channel.
 *
 * @param value the normalized value
 * @return the clamped and rounded byte value
 */
static inline uint8_t dvz_color_u8(float value)
{
    if (!isfinite(value) || value <= 0.0f)
        return 0;
    if (value >= 1.0f)
        return 255;
    return (uint8_t)(value * 255.0f + 0.5f);
}



/**
 * Create an RGBA8 color from byte channels.
 *
 * @param r the red channel
 * @param g the green channel
 * @param b the blue channel
 * @param a the alpha channel
 * @return the color
 */
static inline DvzColor dvz_color_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    DvzColor color = {r, g, b, a};
    return color;
}



/**
 * Create an opaque RGBA8 color from byte RGB channels.
 *
 * @param r the red channel
 * @param g the green channel
 * @param b the blue channel
 * @return the opaque color
 */
static inline DvzColor dvz_color_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    return dvz_color_rgba(r, g, b, 255);
}



/**
 * Create a display-encoded RGBA8 color from normalized display-space channels.
 *
 * @param r the red channel
 * @param g the green channel
 * @param b the blue channel
 * @param a the alpha channel
 * @return the color
 */
static inline DvzColor dvz_color_from_unit(float r, float g, float b, float a)
{
    return dvz_color_rgba(dvz_color_u8(r), dvz_color_u8(g), dvz_color_u8(b), dvz_color_u8(a));
}



/**
 * Create a linear-light float color.
 *
 * @param r the red channel
 * @param g the green channel
 * @param b the blue channel
 * @param a the alpha channel
 * @return the float color
 */
static inline DvzColorf dvz_colorf(float r, float g, float b, float a)
{
    DvzColorf color = {r, g, b, a};
    return color;
}



/**
 * Convert a display-encoded byte color to linear-light float RGBA.
 *
 * @param color the display-encoded RGBA8 color
 * @return the linear-light float color
 */
static inline DvzColorf dvz_color_to_linear(DvzColor color)
{
    const float r = (float)color.r / 255.0f;
    const float g = (float)color.g / 255.0f;
    const float b = (float)color.b / 255.0f;
    const float a = (float)color.a / 255.0f;

    return dvz_colorf(
        r <= 0.04045f ? r / 12.92f : powf((r + 0.055f) / 1.055f, 2.4f),
        g <= 0.04045f ? g / 12.92f : powf((g + 0.055f) / 1.055f, 2.4f),
        b <= 0.04045f ? b / 12.92f : powf((b + 0.055f) / 1.055f, 2.4f),
        a);
}



/**
 * Convert a linear-light float color to display-encoded RGBA8.
 *
 * @param color the linear-light float color
 * @return the display-encoded RGBA8 color
 */
static inline DvzColor dvz_color_from_linear(DvzColorf color)
{
    float r = color.r;
    float g = color.g;
    float b = color.b;
    float a = color.a;

    r = !isfinite(r) || r <= 0.0f ? 0.0f : (r >= 1.0f ? 1.0f : r);
    g = !isfinite(g) || g <= 0.0f ? 0.0f : (g >= 1.0f ? 1.0f : g);
    b = !isfinite(b) || b <= 0.0f ? 0.0f : (b >= 1.0f ? 1.0f : b);
    a = !isfinite(a) || a <= 0.0f ? 0.0f : (a >= 1.0f ? 1.0f : a);

    r = r <= 0.0031308f ? 12.92f * r : 1.055f * powf(r, 1.0f / 2.4f) - 0.055f;
    g = g <= 0.0031308f ? 12.92f * g : 1.055f * powf(g, 1.0f / 2.4f) - 0.055f;
    b = b <= 0.0031308f ? 12.92f * b : 1.055f * powf(b, 1.0f / 2.4f) - 0.055f;

    return dvz_color_rgba(dvz_color_u8(r), dvz_color_u8(g), dvz_color_u8(b), dvz_color_u8(a));
}



/**
 * Create an opaque color from an explicit 0xRRGGBB value.
 *
 * @param rgb the packed RGB constructor value
 * @return the opaque color
 */
static inline DvzColor dvz_color_hex_rgb(uint32_t rgb)
{
    return dvz_color_rgba(
        (uint8_t)((rgb >> 16) & 0xff), //
        (uint8_t)((rgb >> 8) & 0xff),  //
        (uint8_t)(rgb & 0xff),         //
        255);
}



/**
 * Create a color from an explicit 0xRRGGBBAA value.
 *
 * @param rgba the packed RGBA constructor value
 * @return the color
 */
static inline DvzColor dvz_color_hex_rgba(uint32_t rgba)
{
    return dvz_color_rgba(
        (uint8_t)((rgba >> 24) & 0xff), //
        (uint8_t)((rgba >> 16) & 0xff), //
        (uint8_t)((rgba >> 8) & 0xff),  //
        (uint8_t)(rgba & 0xff));
}
