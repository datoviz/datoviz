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

layout(location = 0) in vec4 in_color;
layout(location = 1) in vec4 in_pos;
layout(location = 2) in float in_radius;
layout(location = 3) in vec4 in_cam_pos;

layout(location = 0) out vec4 out_color;


const vec4 light_color = vec4(1.0);


vec4 lighting(vec4 frag_pos, vec4 frag_color, vec3 normal,
              vec4 cam_pos, vec4 light_pos, vec4 light_color, vec4 light_params)
{
    normal = normalize(normal);

    //vec4 material = light_params;
    vec3 light_dir = normalize(light_pos.xyz - frag_pos.xyz);
    light_dir.y *= -1.0;           // Correction for vulkan y direction.

    float ambient = light_params.x;
    ambient *= .75 + .25 * min(dot(normal, light_dir), 0.0);   //  Dark side gradient.

    float diffuse = light_params.y * max(dot(normal, light_dir), 0.0);

    vec3 view_dir = normalize(cam_pos.xyz - frag_pos.xyz);
    view_dir.y *= -1.0;

    vec3 halfAngle = normalize(view_dir + light_dir);
    float spec_term = pow(max(dot(normal, halfAngle), 0.0), light_params.w);
    float specular = light_params.z * spec_term;

    vec4 color = ambient * frag_color * light_color;
    color += (1.0 - color) * diffuse * frag_color * light_color;
    color += (1.0 - color) * specular * light_color;

    color.a = 1.0;
    return color;
}


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
    vec3 cam_dir = normalize(in_cam_pos.xyz - in_pos.xyz);
    vec4 pos = in_pos;
    pos.xyz += in_radius * cam_dir * normal.z;

    // Update depth buffer to match new position.
    float clip_depth = (mvp.proj * mvp.view * pos).w;
    gl_FragDepth = 1.0 - 1.0/(1.0 + clip_depth);

    // Get lighting.  (Reqires LightParams structure to be set.
    out_color = lighting(pos, in_color, normal, in_cam_pos,
                         params.light_pos, light_color, params.light_params);
}
