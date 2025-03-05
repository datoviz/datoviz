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
    vec3 light_pos;
    vec4 light_params;
}
params;

layout(location = 0) in vec4 in_color;
layout(location = 1) in vec3 in_pos;
layout(location = 2) in float in_radius;
layout(location = 3) in vec4 in_world_pos;
layout(location = 4) in vec4 in_view_pos;
layout(location = 5) in vec3 in_light_pos;

layout(location = 0) out vec4 out_color;


const vec3 light_color = vec3(1.0);

#define EPSILON 0



vec4 render_lighting(vec4 view_pos, vec4 world_pos, vec3 normal,
                     vec4 color, vec4 material,
                     vec3 light_pos, vec3 light_color) {

    vec3 light_dir = normalize(light_pos - world_pos.xyz);
    vec3 view_dir = -normalize(view_pos.xyz);
    vec3 reflect_dir = reflect(-light_dir, normal);

    vec3 ambient = material.x * vec3(1.0);
    float diff = max(dot(normal, light_dir), 0.0);
    vec3 diffuse = material.y * diff * light_color;
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), material.w);
    vec3 specular = material.z * spec * light_color;
    vec3 final_color = (ambient + diffuse + specular) * color.rgb;

    return vec4(final_color, color.a);
}



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
    vec4 view_pos = in_view_pos;
    float d = length(view_pos * viewport.size.x);
    view_pos.z += in_radius/d * normal.z;
    vec4 clip_pos = mvp.proj * view_pos;
    gl_FragDepth = clip_pos.z/clip_pos.w;

    out_color = render_lighting(in_view_pos, in_world_pos, normal, in_color, params.light_params,
                                in_light_pos, light_color);


//    // Calculate the lighting
//    vec3 light_dir = normalize(in_light_pos.xyz - in_world_pos.xyz);
//    vec3 view_dir = normalize(-view_pos.xyz);
//    vec3 reflect_dir = reflect(-light_dir, normal);
//    vec3 ambient = params.light_params.x * vec3(1.0);
//    float diff = max(dot(normal, light_dir), 0.0);
//    vec3 diffuse = params.light_params.y * diff * light_color;
//    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), params.light_params.w);
//    vec3 specular = params.light_params.z * spec * light_color;
//    vec3 final_color = (ambient + diffuse + specular) * in_color.rgb + specular * spec;
//
//    // TODO: border antialias.
//    float alpha = in_color.a;
//    // if (dist_squared > 1.0)
//    //     alpha *= compute_distance(dist_squared - 1, 1.0).z;
//    out_color = vec4(final_color, alpha);
}
