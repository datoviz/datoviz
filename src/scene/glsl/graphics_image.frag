/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

#version 450
#include "antialias.glsl"
#include "common.glsl"
#include "colormaps.glsl"
#include "markers.glsl"
#include "params_image.glsl"

layout(constant_id = 0) const int MODE = 0; // color mode, 0=fill color, 1=RGBA texture, 2=R texture with colormap
layout(constant_id = 1) const int BORDER = 0; // 0=no border, 1=border

layout(binding = (USER_BINDING + 1)) uniform sampler2D tex;

layout(location = 0) in vec2 in_uv;
layout(location = 1) in vec3 in_size; // w, h, zoom
layout(location = 2) in vec4 in_color;

layout(location = 0) out vec4 out_color;

void main()
{
    CLIP;

    vec2 size = in_size.xy;
    float zoom = in_size.z;

    vec2 P = in_uv - .5;

    if (MODE == 0)
    {
        out_color = in_color;
    }
    else if (MODE == 1)
    {
        out_color = texture(tex, in_uv);
    }
    else if (MODE == 2)
    {
        float value = texture(tex, in_uv).r;
        // NOTE: only works with a few colormaps, to improve
        out_color = colormap(params.cmap, value);
    }

    if (BORDER == 1)
    {
        float lw = params.linewidth;
        vec2 c = size + 2 * lw + antialias;
        float radius = params.radius * zoom;
        float d = marker_rounded_rect(P * c, size, radius);
        vec4 edgecolor = params.edgecolor;
        out_color = outline(d, lw, edgecolor, out_color);
    }
}
