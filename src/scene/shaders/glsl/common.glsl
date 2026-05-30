/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* Shared transform pipeline for builtin scene vertex shaders.
 *
 * Bind group layout (set 0):
 *   binding 0: MVP { model, view, proj, time, flags }
 *   binding 1: Viewport { x, y, width, height }
 *
 * Single entry point: transform(pos) returns a Vulkan-correct gl_Position.
 *  - Applies the MVP chain (cglm-built, OpenGL-NDC convention).
 *  - Converts OpenGL NDC to Vulkan NDC (Y down, depth [0,1]).
 */

#ifndef DVZ_COMMON_GLSL
#define DVZ_COMMON_GLSL

layout(set = 0, binding = 0) uniform MVP {
    mat4 model;
    mat4 view;
    mat4 proj;
    float time;
    uint flags;
} mvp;

layout(set = 0, binding = 1) uniform Viewport {
    vec4 rect;
} viewport;

vec4 transform(vec3 pos)
{
    vec4 tr = mvp.proj * mvp.view * mvp.model * vec4(pos, 1.0);
    tr.y = -tr.y;
    tr.z = 0.5 * (tr.z + tr.w);
    return tr;
}

float transform_radius(float radius)
{
    float sx = length(mvp.model[0].xyz);
    float sy = length(mvp.model[1].xyz);
    float sz = length(mvp.model[2].xyz);
    return radius * max(max(sx, sy), sz);
}

#endif
