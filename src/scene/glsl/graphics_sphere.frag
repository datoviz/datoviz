/*
* Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
* Licensed under the MIT license. See LICENSE file in the project root for details.
* SPDX-License-Identifier: MIT
*/

#version 450
#include "antialias.glsl"
#include "common.glsl"

layout(binding = 2) uniform SphereParams
{
    vec4 light_pos;
    vec4 light_params;
}
params;

layout(location = 0) in vec4 in_color;
layout(location = 1) in vec4 in_pos;
layout(location = 2) in float in_radius;
layout(location = 3) in vec4 in_eye_pos;

layout(location = 0) out vec4 out_color;


const vec3 light_color = vec3(1.0);

#define EPSILON 0


void main()
{
    // Calculate the normalized coordinates of the fragment within the point sprite
    vec2 coord = 2.0 * gl_PointCoord - 1.0;
    float dist_squared = dot(coord, coord);

    if (dist_squared > 1.0 + EPSILON)
        discard;

    // Calculate the normal of the sphere at this fragment
    vec3 normal = vec3(coord, sqrt(1.0 - dist_squared));

    // Update depth
    vec4 view_pos = in_eye_pos;
    float d = length(view_pos * viewport.size.x);
    view_pos.xyz += (in_radius * normal)/d;
    gl_FragDepth = 1.0 - 1.0/(1.0 + length(view_pos.xyz));

    // Calculate the lighting
    vec3 light_pos = params.light_pos.xyz;

    vec3 light_dir = normalize(light_pos.xyz - in_pos.xyz);
    light_dir.y *= -1.0;  // Correction for vulkan y positions reversed.
    vec3 view_dir = normalize(-view_pos.xyz);
    vec3 reflect_dir = reflect(light_dir, normal);

    float spec = pow(max(dot(view_dir, -reflect_dir), 0.0), params.light_params.w);
    vec3 specular = params.light_params.z * spec * light_color;
    vec3 color = specular * smoothstep(0.0, 0.5, normal.z);  // Reduced at edges.

    float diff = max(dot(normal, light_dir), 0.0);
    vec3 diffuse = params.light_params.y * diff * light_color;
    color += diffuse * (1.0 - color) * in_color.xyz;

    float ambient = params.light_params.x;
    color += ambient * (1.0 - color) * in_color.xyz;

    // TODO: border antialias.
    float alpha = in_color.a;
    // if (dist_squared > 1.0)
    //     alpha *= compute_distance(dist_squared - 1, 1.0).z;
    out_color = vec4(color, alpha);
}