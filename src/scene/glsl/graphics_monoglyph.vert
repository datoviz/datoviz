/*
* Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
* Licensed under the MIT license. See LICENSE file in the project root for details.
* SPDX-License-Identifier: MIT
*/

#version 450
#include "common.glsl"
#include "params_monoglyph.glsl"

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 bytes_012;
layout(location = 2) in vec3 bytes_345;
layout(location = 3) in ivec2 offset;
layout(location = 4) in vec4 color;

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec3 out_bytes_012;
layout(location = 2) out vec3 out_bytes_345;

void main()
{
    // Offset.
    float dx = params.size * 3 * offset.y;
    float dy = -params.size * 5 * offset.x;
    vec2 trans = vec2(dx, dy);
    trans -= params.anchor * params.size;
    mat4 tra = get_translation_matrix(trans);

    // Transform.
    mat4 ortho = get_ortho_matrix();
    mat4 ortho_inv = inverse(ortho);
    vec4 tr = transform_mvp(pos);
    tr = transform_fixed(tr, pos);
    tr = ortho * tra * ortho_inv * tr;
    tr = transform_margins(tr);
    tr = to_vulkan(tr);
    // HACK: without this the z is negative and clipped
    tr.z = 0;
    gl_Position = tr;

    out_color = color;
    out_bytes_012 = bytes_012;
    out_bytes_345 = bytes_345;
    gl_PointSize = 8.0 * params.size;
}
