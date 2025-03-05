/*
* Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
* Licensed under the MIT license. See LICENSE file in the project root for details.
* SPDX-License-Identifier: MIT
*/

#version 450
#include "common.glsl"

layout(binding = 2) uniform SphereParams
{
    vec3 light_pos;
    vec4 light_params;
}
params;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec4 color;
layout(location = 2) in float radius;

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_pos;
layout(location = 2) out float out_radius;
layout(location = 3) out vec4 out_world_pos;
layout(location = 4) out vec4 out_view_pos;
layout(location = 5) out vec3 out_light_pos;

void main()
{
    // Convert coordinates
    out_pos = vec4(dvz_to_vulkan(pos), 1.0);
    out_light_pos = dvz_to_vulkan(params.light_pos);

    out_world_pos = mvp.model * out_pos;
    out_view_pos = mvp.view * mvp.model * out_pos;
    out_radius = radius;
    out_color = color;

    // Project the position to clip space using the transform function
    gl_Position = transform(out_pos.xyz);

    // Set the point size to the diameter of the sphere in pixels
    float distance_to_camera = length(out_view_pos.xyz);
    gl_PointSize = (2.0 * radius) / distance_to_camera;
}
