/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

#version 450
#include "antialias.glsl"
#include "common.glsl"

layout(std140, binding = USER_BINDING) uniform WiggleParams
{
    vec4 negative_color;
    vec4 positive_color;
    vec4 edgecolor;
    vec2 xrange;
    int channels;
    float scale;
    // int swizzle; // TODO: generic swizzle system with spec constants (planned for v0.4)
}
params;

layout(binding = (USER_BINDING + 1)) uniform sampler2D tex;

layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_color;



int wiggle(vec2 p, float v)
{
    float xp = p.x;
    float y = p.y;
    if ((v <= 0) && (xp < v))       // outside left
        return 0;                   //
    else if ((v < xp) && (xp <= 0)) // negative
        return 1;                   //
    else if ((0 <= xp) && (xp < v)) // positive
        return 2;                   //
    else if ((v < xp))              // outside right
        return 4;                   //
    else
        return -1;
}



vec4 wiggle_color(
    vec2 p, sampler2D tex, int channels, vec2 xrange, float scale, float linewidth,
    vec4 negative_color, vec4 positive_color, vec4 edgecolor)
{
    float x = p.x;
    float y = p.y;
    float x0 = xrange.x;
    float xl = xrange.y;

    for (int i = 0; i < channels; i++)
    {
        float a = channels >= 2 ? float(i) / (channels - 1) : .5;
        float v = scale * texture(tex, vec2(y, a)).r;

        // DEBUG
        // v = .1;

        float xi = x0 + (xl - x0) * a;

        // In x=xi line:
        if (length(x - xi) < linewidth)
            return edgecolor;

        vec2 q = vec2(x - xi, y);
        int w = wiggle(q, v);

        // Stop at the first channel
        if (w == 1)
            return negative_color;
        else if (w == 2)
            return positive_color;
    }

    return vec4(32, 32, 32, 32);
}



void main()
{
    CLIP;

    vec2 p = in_uv;

    // NOTE: from pixels to NDC.
    float linewidth = 2.0; // TODO: param
    float lw = .5 * linewidth / viewport.size.x;

    // DEBUG
    // float v = +0.1;
    // p.x -= .5;
    // int w = wiggle(p, v);
    // out_color = vec4(1, w / 4.0, 0, 1);

    out_color = wiggle_color(
        p, tex, params.channels, params.xrange, params.scale / params.channels, lw, //
        params.negative_color, params.positive_color, params.edgecolor);
}
