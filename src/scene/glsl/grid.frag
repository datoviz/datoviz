/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

#version 450
#include "common.glsl"
#include "constants.glsl"

layout(location = 0) in vec3 in_world_pos;
layout(location = 0) out vec4 out_color;

#define FADE_PARAM 1000

layout(std140, binding = 2) uniform GridParams
{
    vec4 color;         /* Line color. */
    float linewidth;    /* Line width. */
    float scale;        /* Grid scaling. */
    float elevation;    /* Grid elevation on the y axis. */
}
params;

// source: https://www.shadertoy.com/view/mdVfWw
float grid(in vec2 uv, vec2 linewidth)
{
    vec2 ddx = dFdx(uv);
    vec2 ddy = dFdy(uv);
    vec2 uvDeriv = vec2(length(vec2(ddx.x, ddy.x)), length(vec2(ddx.y, ddy.y)));
    bvec2 invertLine = bvec2(linewidth.x > 0.5, linewidth.y > 0.5);
    vec2 targetWidth = vec2(
      invertLine.x ? 1.0 - linewidth.x : linewidth.x,
      invertLine.y ? 1.0 - linewidth.y : linewidth.y
      );
    vec2 drawWidth = clamp(targetWidth, uvDeriv, vec2(0.5));
    vec2 lineAA = uvDeriv * 1.5;
    vec2 gridUV = abs(fract(uv) * 2.0 - 1.0);
    gridUV.x = invertLine.x ? gridUV.x : 1.0 - gridUV.x;
    gridUV.y = invertLine.y ? gridUV.y : 1.0 - gridUV.y;
    vec2 grid2 = smoothstep(drawWidth + lineAA, drawWidth - lineAA, gridUV);

    grid2 *= clamp(targetWidth / drawWidth, 0.0, 1.0);
    grid2 = mix(grid2, targetWidth, clamp(uvDeriv * 2.0 - 1.0, 0.0, 1.0));
    grid2.x = invertLine.x ? 1.0 - grid2.x : grid2.x;
    grid2.y = invertLine.y ? 1.0 - grid2.y : grid2.y;
    return mix(grid2.x, 1.0, grid2.y);
}

void main()
{
    // NOTE: This only supports fine lines for now. TODO: implement coarse vs fine colors
    vec2 coord = in_world_pos.xz;
    float mask_total = grid(coord * params.scale, vec2(params.linewidth));
    if (mask_total <= 0.001)
        discard;
    vec3 color = params.color.rgb;

    // Radial fade from origin (XZ plane)
    float d = length(coord);
    float fade = exp(-d * d / FADE_PARAM);

    out_color = vec4(color, params.color.a * mask_total * fade);
}
