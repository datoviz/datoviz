/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Mock                                                                                         */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <math.h>

#include "_alloc.h"
#include "_assertions.h"
#include "datoviz/math/mock.h"
#include "datoviz/math/rand.h"
#include "datoviz/math/types.h"
#include "datoviz/color/types.h"



/*************************************************************************************************/
/*  Mock random data                                                                             */
/*************************************************************************************************/

vec3* dvz_mock_pos_2D(uint32_t count, float std)
{
    ASSERT(count > 0);
    vec3* pos = (vec3*)dvz_calloc(count, sizeof(vec3));
    ANN(pos);
    for (uint32_t i = 0; i < count; i++)
    {
        pos[i][0] = std * dvz_rand_normal();
        pos[i][1] = std * dvz_rand_normal();
    }
    return pos;
}



vec3* dvz_mock_circle(uint32_t count, float radius)
{
    ASSERT(count > 1);
    vec3* pos = (vec3*)dvz_calloc(count, sizeof(vec3));
    ANN(pos);
    // bool closed = (flags & DVZ_MOCK_FLAGS_CLOSED) > 0;
    uint32_t n = count; // closed ? count - 1 : count;
    for (uint32_t i = 0; i < count; i++)
    {
        pos[i][0] = radius * cos(M_2PI * i * 1.0 / n);
        pos[i][1] = radius * sin(M_2PI * i * 1.0 / n);
    }
    return pos;
}



vec3* dvz_mock_band(uint32_t count, vec2 size)
{
    ASSERT(count > 0);
    float a = size[0] * .5;
    float b = size[1] * .5;
    vec3* pos = (vec3*)dvz_calloc(count, sizeof(vec3));
    ANN(pos);
    for (uint32_t i = 0; i < count; i++)
    {
        pos[i][0] = -a + 2 * a * (i / 2) * 1. / (count / 2 - 1);
        pos[i][1] = -b + 2 * b * (i % 2);
    }
    return pos;
}



vec3* dvz_mock_pos_3D(uint32_t count, float std)
{
    ASSERT(count > 0);
    vec3* pos = (vec3*)dvz_calloc(count, sizeof(vec3));
    ANN(pos);
    for (uint32_t i = 0; i < count; i++)
    {
        pos[i][0] = std * dvz_rand_normal();
        pos[i][1] = std * dvz_rand_normal();
        pos[i][2] = std * dvz_rand_normal();
    }
    return pos;
}



vec3* dvz_mock_fixed(uint32_t count, vec3 fixed)
{
    ASSERT(count > 0);
    vec3* pos = (vec3*)dvz_calloc(count, sizeof(vec3));
    ANN(pos);

    for (uint32_t i = 0; i < count; i++)
    {
        pos[i][0] = fixed[0];
        pos[i][1] = fixed[1];
        pos[i][2] = fixed[2];
    }
    return pos;
}



vec3* dvz_mock_line(uint32_t count, vec3 p0, vec3 p1)
{
    ASSERT(count > 1);
    vec3* pos = (vec3*)dvz_calloc(count, sizeof(vec3));
    ANN(pos);

    vec3 u = {0};
    u[0] = (p1[0] - p0[0]) * 1. / (count - 1);
    u[1] = (p1[1] - p0[1]) * 1. / (count - 1);
    u[2] = (p1[2] - p0[2]) * 1. / (count - 1);

    for (uint32_t i = 0; i < count; i++)
    {
        pos[i][0] = p0[0] + i * u[0];
        pos[i][1] = p0[1] + i * u[1];
        pos[i][2] = p0[2] + i * u[2];
    }
    return pos;
}



float* dvz_mock_uniform(uint32_t count, float vmin, float vmax)
{
    ASSERT(count > 0);
    ASSERT(vmin <= vmax);
    float* size = (float*)dvz_calloc(count, sizeof(float));
    ANN(size);
    float a = vmax - vmin;
    for (uint32_t i = 0; i < count; i++)
    {
        size[i] = vmin + a * dvz_rand_float();
    }
    return size;
}



float* dvz_mock_full(uint32_t count, float value)
{
    ASSERT(count > 0);
    float* values = (float*)dvz_calloc(count, sizeof(float));
    ANN(values);
    for (uint32_t i = 0; i < count; i++)
    {
        values[i] = value;
    }
    return values;
}



uint32_t* dvz_mock_range(uint32_t count, uint32_t initial)
{
    ASSERT(count > 1);
    uint32_t* values = (uint32_t*)dvz_calloc(count, sizeof(uint32_t));
    ANN(values);
    for (uint32_t i = 0; i < count; i++)
    {
        values[i] = initial + i;
    }
    return values;
}



float* dvz_mock_linspace(uint32_t count, float initial, float final)
{
    ASSERT(count > 1);
    float* values = (float*)dvz_calloc(count, sizeof(float));
    ANN(values);
    for (uint32_t i = 0; i < count; i++)
    {
        values[i] = initial + (final - initial) * i * 1.0 / (count - 1);
    }
    return values;
}



/*************************************************************************************************/
/*  Mock colors                                                                                  */
/*************************************************************************************************/

DvzColor* dvz_mock_color(uint32_t count, DvzAlpha alpha)
{
    ASSERT(count > 0);
    DvzColor* color = (DvzColor*)dvz_calloc(count, sizeof(DvzColor));
    ANN(color);
    const uint8_t k = 64;
    for (uint32_t i = 0; i < count; i++)
    {
        // dvz_colormap(DVZ_CMAP_HSV, i % 256, color[i]);
        color[i][0] = k + (dvz_rand_byte() % (256 - k));
        color[i][1] = k + (dvz_rand_byte() % (256 - k));
        color[i][2] = k + (dvz_rand_byte() % (256 - k));
        color[i][3] = alpha;
    }
    return color;
}



DvzColor* dvz_mock_monochrome(uint32_t count, DvzColor mono)
{
    ASSERT(count > 0);
    DvzColor* color = (DvzColor*)dvz_calloc(count, sizeof(DvzColor));
    ANN(color);
    for (uint32_t i = 0; i < count; i++)
    {
        color[i][0] = mono[0];
        color[i][1] = mono[1];
        color[i][2] = mono[2];
        color[i][3] = mono[3];
    }
    return color;
}



// TODO: move to color module
// DvzColor* dvz_mock_cmap(uint32_t count, DvzColormap cmap, DvzAlpha alpha)
// {
//     ASSERT(count > 0);
//     DvzColor* color = (DvzColor*)calloc(count, sizeof(DvzColor));
//     for (uint32_t i = 0; i < count; i++)
//     {
//         dvz_colormap_scale(cmap, i, 0, count, color[i]);
//         color[i][3] = alpha;
//     }
//     return color;
// }
