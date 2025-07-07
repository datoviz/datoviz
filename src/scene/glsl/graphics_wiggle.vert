/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

#version 450
#include "common.glsl"

// Attributes.
layout(location = 0) in vec3 pos;    // in NDC
layout(location = 1) in vec2 uv;     // in texel coordinates

// Varyings.
layout(location = 0) out vec2 out_uv;
layout(location = 1) out float out_zoom;


void main()
{
    gl_Position = transform(pos);

    // Varyings.
    out_uv = uv;
    out_zoom = total_zoom();
}
