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

#ifndef DVZ_MVP_UNIFORM_GLSL
#define DVZ_MVP_UNIFORM_GLSL
layout(set = 0, binding = 0) uniform MVP {
    mat4 model;
    mat4 view;
    mat4 proj;
    float time;
    uint flags;
} mvp;
#endif

layout(set = 0, binding = 1) uniform Viewport {
    vec4 rect;
} viewport;

vec4 sceneClipToDeviceClip(vec4 sceneClip)
{
    sceneClip.y = -sceneClip.y;
    sceneClip.z = 0.5 * (sceneClip.z + sceneClip.w);
    return sceneClip;
}

vec4 transform(vec3 pos)
{
    vec4 sceneClip = mvp.proj * mvp.view * mvp.model * vec4(pos, 1.0);
    return sceneClipToDeviceClip(sceneClip);
}

float transform_radius(float radius)
{
    float sx = length(mvp.model[0].xyz);
    float sy = length(mvp.model[1].xyz);
    float sz = length(mvp.model[2].xyz);
    return radius * max(max(sx, sy), sz);
}

float positiveLinearViewDepth(float deviceDepth)
{
    vec4 view = inverse(mvp.proj) * vec4(0.0, 0.0, deviceDepth * 2.0 - 1.0, 1.0);
    if (abs(view.w) <= 1e-8)
        return 0.0;
    return max(-view.z / view.w, 0.0);
}

#endif
