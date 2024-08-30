/*
* Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
* Licensed under the MIT license. See LICENSE file in the project root for details.
* SPDX-License-Identifier: MIT
*/

#version 450
#include "common.glsl"
#include "params_glyph.glsl"


layout(binding = USER_BINDING + 1) uniform sampler2D tex;

layout(location = 0) in vec2 in_uv;
layout(location = 1) in vec4 in_color;

layout(location = 0) out vec4 out_color;

float median(float r, float g, float b) { return max(min(r, g), min(max(r, g), b)); }

void main()
{
    CLIP;

    // DEBUG
    // out_color = vec4(1, 1, 1, 1);
    // return;
    // out_color = in_color;
    // out_color = vec4(in_uv, 1, 1);

    // from https://github.com/Chlumsky/msdfgen#using-a-multi-channel-distance-field
    vec3 msd = texture(tex, in_uv).rgb;
    float sd = median(msd.r, msd.g, msd.b);
    if (sd < .05)
        discard;

    float screenPxDistance = 4 * (sd - 0.5);
    float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
    out_color = mix(params.bgcolor, in_color, opacity);
}
