/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

#version 450
#include "common.glsl"
#include "constants.glsl"

layout(location = 0) out vec3 out_world_pos;
layout(location = 1) out vec3 out_view_pos;

layout(std140, binding = 2) uniform GridParams
{
    vec4 color;         /* Line color. */
    float linewidth;    /* Line width. */
    float scale;        /* Grid scaling. */
    float elevation;    /* Grid elevation on the y axis. */
}
params;

const float GRID_HALF_EXTENT = 10000.0;

const vec3 COORDS[6] = vec3[](
    vec3(-1, 0, -1),
    vec3(+1, 0, -1),
    vec3(+1, 0, +1),
    vec3(-1, 0, -1),
    vec3(+1, 0, +1),
    vec3(-1, 0, +1)
);

void main()
{
    vec3 world = COORDS[gl_VertexIndex];
    world.y = params.elevation;
    world.x *= GRID_HALF_EXTENT;
    world.z *= GRID_HALF_EXTENT;
    out_world_pos = world;
    out_view_pos = (mvp.view * mvp.model * vec4(world, 1)).xyz;

    gl_Position = transform(world);
}
