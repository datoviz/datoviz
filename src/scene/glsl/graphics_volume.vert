/*
* Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
* Licensed under the MIT license. See LICENSE file in the project root for details.
* SPDX-License-Identifier: MIT
*/

#version 450
#include "common.glsl"

layout(location = 0) in vec3 pos;

layout(location = 0) out vec3 out_pos;
layout(location = 1) out vec3 out_ray;

void main()
{
    gl_Position = transform(pos);
    out_pos = (mvp.model * vec4(pos, 1.0)).xyz; // pos in world coordinates
    out_ray = out_pos + mvp.view[3].xyz;        // out_pos - view_pos (world coordinates)
}
