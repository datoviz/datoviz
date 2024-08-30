/*
* Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
* Licensed under the MIT license. See LICENSE file in the project root for details.
* SPDX-License-Identifier: MIT
*/

#version 450

layout(location = 0) in vec3 pos;
layout(location = 1) in vec4 color;

layout(location = 0) out vec4 out_color;

void main()
{
    vec2 xy = pos.xy / 7;
    xy.x += (6. / 7.) * (-1.0 + 2.0 * gl_InstanceIndex / 6.0);
    gl_Position = vec4(xy, 0, 1.0);
    out_color = color;
}
