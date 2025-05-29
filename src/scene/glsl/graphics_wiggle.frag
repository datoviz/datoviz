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

void main()
{
    CLIP;

    // DEBUG
    out_color=vec4(1,0,0,1);
}
