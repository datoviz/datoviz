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
layout(location = 1) in float in_zoom;

layout(location = 0) out vec4 out_color;



// p is the position (x, y) assuming x=0 vertical line
// v is the function value
int wiggle(vec2 p, float v)
{
    float x = p.x;
    float y = p.y;
    if ((v <= 0) && (x < v))       // outside left
        return 0;                  //
    else if ((v < x) && (x <= 0))  // negative
        return 1;                  //
    else if ((0 <= x) && (x < v))  // positive
        return 2;                  //
    else if ((v < x))              // outside right
        return 4;                  //
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
    vec4 color = vec4(1);

    for (int i = 0; i < channels; i++)
    {
        // Determine where we are in the wiggle plot.
        float a = channels >= 2 ? float(i) / (channels - 1) : .5;
        float v = scale * texture(tex, vec2(y, a)).r;
        float xi = x0 + (xl - x0) * a;
        vec2 q = vec2(x - xi, y);
        int w = wiggle(q, v);

        // Positive or negative part.
        if (w == 1)
            color = negative_color;
        else if (w == 2)
            color = positive_color;

        // Stroke.
        float lw = 4;
        float c = length(viewport.size) * in_zoom;
        float d = abs(q.x - v);
        if (d * c <= 1 * lw) {
            float alpha = stroke(d * c, lw, edgecolor).a;
            color.rgb = mix(color.rgb, edgecolor.rgb, alpha);
            break;
        }

        // Stop at the first channel
        if (w == 1 || w == 2) break;
    }

    return color;
}



void main()
{
    CLIP;

    vec2 p = in_uv;

    // NOTE: from pixels to NDC.
    float linewidth = 1.0; // TODO: param
    float lw = .5 * linewidth / viewport.size.x;
    float scale = params.scale * (params.xrange.y - params.xrange.x) / params.channels;

    // DEBUG
    // float v = +0.1;
    // p.x -= .5;
    // int w = wiggle(p, v);
    // out_color = vec4(1, w / 4.0, 0, 1);

    out_color = wiggle_color(
        p, tex, params.channels, params.xrange, scale, lw, //
        params.negative_color, params.positive_color, params.edgecolor);
}
