/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

#version 450
#include "common.glsl"
#define EPS 1e-4

layout(location = 0) in vec4 in_color;
layout(location = 1) in float in_group;
layout(location = 0) out vec4 out_color;

void main()
{
    CLIP;

    // Discard fragments between vertices of different groups.
    float m = in_group;
    m = fract(m);
    if (m > EPS && m < 1 - EPS)
        discard;

    out_color = in_color;
}
