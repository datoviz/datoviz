/*
* Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
* Licensed under the MIT license. See LICENSE file in the project root for details.
* SPDX-License-Identifier: MIT
*/

#version 450
#include "common.glsl"
#include "colormaps.glsl"


layout (location = 0) in vec2 pos;
layout (location = 1) in float depth;
layout (location = 2) in float cmap_val;
layout (location = 3) in float alpha;
layout (location = 4) in float size;

layout (location = 0) out vec4 out_color;

layout(std140, binding = USER_BINDING) uniform Params
{
    vec2 alpha_range;
    vec2 size_range;
    vec2 cmap_range;
    int cmap_id;
} params;



void main() {
    gl_Position = transform(vec3(pos, depth));

    // Marker color.
    float c = mix(params.cmap_range.x, params.cmap_range.y, cmap_val);
    out_color = colormap(params.cmap_id, c);
    out_color.a = mix(params.alpha_range.x, params.alpha_range.y, alpha);

    // Marker size.
    float out_size = mix(params.size_range.x, params.size_range.y, size);
    gl_PointSize = out_size;
}
