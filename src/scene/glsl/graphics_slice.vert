/*
* Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
* Licensed under the MIT license. See LICENSE file in the project root for details.
* SPDX-License-Identifier: MIT
*/

#version 450
#include "common.glsl"

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 uvw;

layout(location = 0) out vec3 out_uvw;

void main()
{
    gl_Position = transform(pos);
    out_uvw = uvw;
}
