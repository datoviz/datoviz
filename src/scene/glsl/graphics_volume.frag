/*
* Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
* Licensed under the MIT license. See LICENSE file in the project root for details.
* SPDX-License-Identifier: MIT
*/

#version 450
#include "colormaps.glsl"
#include "common.glsl"
#include "utils_volume.glsl"

// Constants.
#define STEP_SIZE 0.01
#define MAX_ITER  10 / STEP_SIZE

// Volume type specialization constant.
layout(constant_id = 0) const int VOLUME_TYPE = VOLUME_TYPE_SCALAR;

// Volume color specialization constant.
layout(constant_id = 1) const int VOLUME_COLOR = VOLUME_COLOR_DIRECT;

// Volume front to back or back to front.
layout(constant_id = 2) const int VOLUME_DIR = VOLUME_DIR_FRONT_BACK;

// Uniform variables.
layout(std140, binding = USER_BINDING) uniform Params
{
    vec4 box_size; /* size of the box containing the volume, in NDC */
    vec4 uvw0;     /* texture coordinates of the 2 corner points */
    vec4 uvw1;     /* texture coordinates of the 2 corner points */
    vec4 transfer;
}
params;

// Texture.
layout(binding = (USER_BINDING + 1)) uniform sampler3D tex_density; // 3D vol with vox R density

// Varying variables.
layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_ray;
layout(location = 0) out vec4 out_color;

// Functions
bool intersect_box(vec3 origin, vec3 dir, vec3 box_min, vec3 box_max, out float t0, out float t1)
{
    vec3 inv_r = 1.0 / dir;
    vec3 tbot = inv_r * (box_min - origin);
    vec3 ttop = inv_r * (box_max - origin);
    vec3 tmin = min(ttop, tbot);
    vec3 tmax = max(ttop, tbot);
    vec2 t = max(tmin.xx, tmin.yz);
    t0 = max(t.x, t.y);
    t = min(tmax.xx, tmax.yz);
    t1 = min(t.x, t.y);
    return t0 <= t1;
}

// Entry-point.
void main()
{
    CLIP;

    float t0, t1;
    mat4 mi = inverse(mvp.model);
    vec4 u_ = mi * vec4(normalize(in_ray), 1);
    vec3 u = u_.xyz / u_.w;
    vec4 o_ = mi * vec4(-mvp.view[3].xyz, 1);
    vec3 o = o_.xyz / o_.w;

    vec3 b0 = -params.box_size.xyz / 2;
    vec3 b1 = +params.box_size.xyz / 2;
    vec3 d = vec3(1) / (b1 - b0);
    intersect_box(o, u, b0, b1, t0, t1);

    if (t0 < 0 || t1 < 0)
        discard;

    vec3 ray_start = o + u * t0;
    vec3 ray_stop = o + u * t1;

    vec3 pos = ray_start;
    vec3 dl = -normalize(ray_start - ray_stop) * STEP_SIZE;

    // Direction: back to front or front to back.
    if (VOLUME_DIR == VOLUME_DIR_BACK_FRONT)
    {
        pos = ray_stop;
        dl = -dl;
    }

    float travel = distance(ray_start, ray_stop);
    float max_intensity = 0.0;
    vec3 uvw = vec3(0);

    vec3 rgbVoxel = vec3(0);
    vec3 rgbAcc = vec3(0);
    float intensity = 0;
    float alpha = 0;
    float alphaAcc = 0;
    vec4 fetched = vec4(0);
    ivec2 modes = ivec2(VOLUME_TYPE, VOLUME_COLOR);

    for (int i = 0; i < MAX_ITER && travel > 0.0; ++i, pos += dl, travel -= STEP_SIZE)
    {
        // Normalize 3D pos within cube in [0,1]^3
        uvw = (pos - b0) * d;

        // Now, normalize between uvw0 and uvw1.
        uvw = params.uvw0.xyz + uvw * (params.uvw1 - params.uvw0).xyz;

        // Fetch the color from the 3D texture.
        fetched = fetch_color(modes, tex_density, uvw, params.transfer.x);

        rgbVoxel = fetched.rgb;
        intensity = fetched.a;
        alpha = intensity;
        rgbAcc = (1 - alpha) * rgbAcc + alpha * rgbVoxel;
        alphaAcc += alpha;
        if (alphaAcc >= 1)
            break;
    }

    gl_FragDepth = pos.z;

    rgbAcc /= max(alphaAcc, 1e-6);
    out_color.rgb = rgbAcc.rgb;
    out_color.a = alphaAcc;
}
