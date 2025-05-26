/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

#version 450
#include "common.glsl"
#include "lighting.glsl"

// mvp --> slot 0
// viewport --> slot 1
#define SPHERE_SLOT_LIGHT 2
#define SPHERE_SLOT_MATERIAL 3
#define SPHERE_SLOT_TEX 4

layout(constant_id = 0) const int SPHERE_TEXTURED = 0; // 1 to enable
layout(constant_id = 1) const int SPHERE_LIGHTING = 0; // 1 to enable

layout(location = 0) in vec4 in_color;
layout(location = 1) in vec4 in_pos;
layout(location = 2) in float in_radius;
layout(location = 3) in vec4 in_cam_pos;

layout(location = 0) out vec4 out_color;


layout(std140, binding = SPHERE_SLOT_LIGHT) uniform u_light {
    Light light;
};


layout(std140, binding = SPHERE_SLOT_MATERIAL) uniform u_material {
    Material material;
};


layout(binding = SPHERE_SLOT_TEX) uniform sampler2D tex;



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

    if (SPHERE_LIGHTING > 0)
    {
        pos.y = -pos.y;
        out_color = lighting(pos, in_color, normal, in_cam_pos, light, material);
    }
    else
    {
        out_color = in_color * (0.2 + 0.8 * normal.z);
        out_color.a = 1.0;
    }
}
