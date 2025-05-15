/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

#version 450
#include "common.glsl"

layout(constant_id = 0) const int SIZE_PIXELS = 0;

//layout(binding = 2) uniform SphereParams
//{
//    vec4 light_pos;
//    vec4 light_params;
//}
//params;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec4 color;
layout(location = 2) in float size; // in NDC or in pixels

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_pos;
layout(location = 2) out float out_radius;
layout(location = 3) out vec4 out_cam_pos;

void main()
{
    float s = size;

    // If the size is specified in pixels
    if (SIZE_PIXELS != 0) {
        vec2 vp = get_inner_viewport();
        s *= 2.0 / vp.y;
    }

    // s should be the size in NDC.
    out_radius = s / 2.0;

    out_color = color;

    // Calculate world space fragment and camera positions.
    out_pos = mvp.model * vec4(pos, 1.0);
    out_cam_pos = inverse(mvp.view) * vec4(0, 0, 0, 1);

    // Project the position to clip space using the transform function
    gl_Position = transform(pos);

    // Set the point size to the diameter of the sphere and scale to window.
    gl_PointSize = s * viewport.size.y * mvp.proj[1][1] / gl_Position.w;
}
