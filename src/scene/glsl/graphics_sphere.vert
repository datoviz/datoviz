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
layout(location = 2) in float radius;

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_pos;
layout(location = 2) out float out_radius;
layout(location = 3) out vec4 out_eye_pos;

void main()
{
    out_radius = radius;
    out_color = color;

    // Calculate position and eye-space position
    out_pos = mvp.model * vec4(pos, 1.0);
    out_eye_pos = mvp.view * out_pos;

    // Project the position to clip space using the transform function
    gl_Position = transform(pos);

    // Set the point size to the diameter of the sphere in pixels
    float distance_to_camera = length(out_eye_pos.xyz);
    gl_PointSize = (2.0 * radius) / distance_to_camera;
}
