/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

#version 450
#include "common.glsl"
#include "lighting.glsl"

// TODO: Use binding as defined in lighting.glsl
//      This should include color and postion, and light specific paramters
//      that are needed to implement differently kind of lights.
//
layout(binding = 2) uniform SphereParams
{
    vec4 light_pos;
    vec4 light_params;
}
params;

layout(location = 0) in vec4 in_color;
layout(location = 1) in vec4 in_pos;
layout(location = 2) in float in_radius;
layout(location = 3) in vec4 in_cam_pos;
layout(location = 4) in vec4 in_flags;       // bvec4(use tex, ...)  others to be determined.

layout(location = 0) out vec4 out_color;


void main()
{
    // Calculate the normalized coordinates of the fragment within the point sprite
    vec2 coord = 2.0 * gl_PointCoord - 1.0;
    float dist_squared = dot(coord, coord);
    if (dist_squared > 1.0)
        discard;

    // Calculate the normal of the sphere at this fragment
    vec3 normal = vec3(coord, sqrt(1.0 - dist_squared));

    // Update position in world space.
    vec4 cam_dir = normalize(in_cam_pos - in_pos);
    vec4 pos = in_pos;
    pos += in_radius * cam_dir * normal.z;

    // Update depth buffer to match new position.
    float clip_depth = (mvp.proj * mvp.view * pos).w;
    gl_FragDepth = 1.0 - 1.0/(1.0 + clip_depth);

    // Temporary fix until new binding is created.
    // Todo: Create new binding and initializations for bindings.
    vec4 light_color = vec4(1.0);
    vec4 light_pos = params.light_pos;
    vec4 material = params.light_params;

    // Flip y direction in shader matches y direction in gl_Postition.
    light_pos.y *= -1;                      // Flip y axis for vulkan.

    // Get lighting.  (Reqires LightParams structure to be set.
    out_color = basic_lighting(pos, in_color, material, normal, in_cam_pos, light_pos, light_color);

}
