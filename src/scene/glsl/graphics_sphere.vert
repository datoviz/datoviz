/*
* Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
* Licensed under the MIT license. See LICENSE file in the project root for details.
* SPDX-License-Identifier: MIT
*/

#version 450
#include "common.glsl"

layout(binding = 2) uniform SphereParams
{
    vec4 light_pos;
    vec4 light_params;
}
params;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec4 color;
layout(location = 2) in float size;

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_pos;
layout(location = 2) out float out_radius;
layout(location = 3) out vec4 out_cam_pos;

void main()
{
    out_radius = size / 2.0;
    out_color = color;

    // Calculate position and eye-space position
    out_pos = mvp.model * vec4(pos, 1.0);
    out_cam_pos = inverse(mvp.view) * vec4(0, 0, 0, 1);

    // Project the position to clip space using the transform function
    gl_Position = transform(pos);

    // Set the point size to the diameter of the sphere and scale to window.
    gl_PointSize = size * viewport.size.y * mvp.proj[1][1] / gl_Position.w;
}
