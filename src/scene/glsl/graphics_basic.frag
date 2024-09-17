/*
* Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
* Licensed under the MIT license. See LICENSE file in the project root for details.
* SPDX-License-Identifier: MIT
*/

#version 450
#include "common.glsl"

layout(location = 0) in vec4 in_color;
layout(location = 1) in float in_group;
layout(location = 0) out vec4 out_color;

void main()
{
    CLIP;

    // Discard fragments between vertices of different groups.
    if (mod(in_group, 1.0) > 0)
        discard;

    out_color = in_color;
    // if (out_color.a < .01)
    //     discard;
}
