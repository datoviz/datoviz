/*
* Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
* Licensed under the MIT license. See LICENSE file in the project root for details.
* SPDX-License-Identifier: MIT
*/

#version 450
#include "common.glsl"

layout(location = 0) in vec3 pos;
layout(location = 1) in vec4 color;

layout(location = 0) out vec4 out_color;

void main()
{
    gl_Position = transform(pos);
    out_color = color;

    // HACK: this is to use the same shader with the pixel visual.
    gl_PointSize = 1;
}