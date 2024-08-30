/*
* Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
* Licensed under the MIT license. See LICENSE file in the project root for details.
* SPDX-License-Identifier: MIT
*/

#version 450

layout (location = 0) in vec3 pos;
layout (location = 1) in vec4 color;

layout (location = 0) out vec4 out_color;
layout (location = 1) flat out ivec4 out_index;

void main() {
    gl_Position = vec4(pos, 1.0);
    out_color = color;
    out_index = ivec4(gl_VertexIndex, 0, 0, 0);
}
