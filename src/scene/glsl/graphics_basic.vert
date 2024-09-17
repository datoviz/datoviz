/*
* Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
* Licensed under the MIT license. See LICENSE file in the project root for details.
* SPDX-License-Identifier: MIT
*/

#version 450
#include "common.glsl"

layout(std140, binding = USER_BINDING) uniform BasicParams { float size; /* point size */ }
params;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec4 color;
layout(location = 2) in float group;

layout(location = 0) out vec4 out_color;
layout(location = 1) out float out_group;

void main()
{
    gl_Position = transform(pos);
    out_color = color;
    out_group = group;
    gl_PointSize = params.size;
}
