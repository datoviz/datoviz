/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

#version 450
#include "colormaps.glsl"
#include "common.glsl"
#include "utils_volume.glsl"


// Volume type specialization constant.
layout(constant_id = 0) const int VOLUME_TYPE = VOLUME_TYPE_SCALAR;

// Volume color specialization constant.
layout(constant_id = 1) const int VOLUME_COLOR = VOLUME_COLOR_DIRECT;

// Volume front to back or back to front.
layout(constant_id = 2) const int VOLUME_DIR = VOLUME_DIR_FRONT_BACK;


// Uniform variables.
layout(std140, binding = USER_BINDING) uniform Params
{
    float alpha; //
}
params;

// Texture
layout(binding = (USER_BINDING + 1)) uniform sampler3D tex_density;


// Attributes.
layout(location = 0) in vec3 in_uvw;

// Varyings.
layout(location = 0) out vec4 out_color;


// Shader.
void main()
{
    CLIP;

    ivec2 modes = ivec2(VOLUME_TYPE, VOLUME_COLOR);

    vec4 color = fetch_color(modes, tex_density, in_uvw, vec4(1, 0, 0, 0));

    out_color = color;
    if (out_color.a < .01)
        discard;
    out_color.a = params.alpha;
}
