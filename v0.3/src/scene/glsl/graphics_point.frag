/*
* Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
* Licensed under the MIT license. See LICENSE file in the project root for details.
* SPDX-License-Identifier: MIT
*/

#version 450
#include "antialias.glsl"
#include "common.glsl"

layout(location = 0) in vec4 in_color;
layout(location = 1) in float in_size;

layout(location = 0) out vec4 out_color;

float marker_disc(vec2 P, float size) { return length(P) - size / 2; }

void main()
{
    CLIP;

    vec2 P = gl_PointCoord.xy - vec2(0.5, 0.5);
    float distance = marker_disc(P * (in_size + 1), in_size);
    out_color = filled(distance, 0, in_color);
    if (out_color.a < .05)
        discard;
}
