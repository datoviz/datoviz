/*
* Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
* Licensed under the MIT license. See LICENSE file in the project root for details.
* SPDX-License-Identifier: MIT
*/

#version 450
#include "antialias.glsl"
#include "common.glsl"

layout(location = 0) in vec4 in_color;
layout(location = 1) in vec2 in_texcoord;
layout(location = 2) in float in_length;
layout(location = 3) in float in_linewidth;
layout(location = 4) in float in_cap;

layout(location = 0) out vec4 out_color;

void main(void)
{
    CLIP;

    if (in_texcoord.x < 0.0)
    {
        out_color = cap(int(round(in_cap)), in_texcoord.x, in_texcoord.y, in_linewidth, in_color);
    }
    else if (in_texcoord.x > in_length)
    {
        out_color = cap(
            int(round(in_cap)), in_texcoord.x - in_length, in_texcoord.y, in_linewidth, in_color);
    }
    else
    {
        out_color = stroke(in_texcoord.y, in_linewidth, in_color);
    }
}
