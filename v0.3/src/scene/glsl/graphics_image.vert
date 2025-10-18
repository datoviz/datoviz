/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

#version 450
#include "common.glsl"
#include "params_image.glsl"

layout(constant_id = 0) const int SIZE_NDC = 0;
layout(constant_id = 1) const int RESCALE = 0; // 1 = rescale_keep_ratio, 2 = rescale

// Attributes.
layout(location = 0) in vec3 pos;    // in NDC
layout(location = 1) in vec2 size;   // in pixels or NDC depending on SIZE_NDC
layout(location = 2) in vec2 anchor; // in relative coordinates
layout(location = 3) in vec2 uv;     // in texel coordinates
layout(location = 4) in vec4 facecolor; // rectangle facecolor in FILL mode (no texture)

// Varyings.
layout(location = 0) out vec2 out_uv;
layout(location = 1) out vec3 out_size; // w, h in pixels, zoom
layout(location = 2) out vec4 out_color;

// NOTE: offset of each of the 6 vertices making the 2 triangles of each image.
// This needs to match the texture coordinates generated in dvz_baker_quads().
vec2 ds[6] = {
    {-.5, +.5},  // top left
    {-.5, -.5},  // bottom left
    {+.5, -.5},  // bottom right

    {+.5, -.5},  // bottom right
    {+.5, +.5},  // top right
    {-.5, +.5},  // top left
};

void main()
{
    vec2 vp = get_inner_viewport();

    vec4 tr = transform(pos);

    // NOTE: we are here in Vulkan CDS with y reversed. For the image vertex displacement, we
    // prefer to do it in the OpenGL CDS with y up, so we temporarily go back to this CDS
    // and will revert to Vulkan CDS with y down at the end of the vertex shader.
    tr.y = -tr.y;

    // Keep aspect ratio?
    float k = 1.0;
    if (RESCALE == 1) {
        k = vp.x / float(vp.y);
    }


    // First, we determine the image size in NDC.
    vec2 s = size;

    // If the size is in pixels, we convert it into NDC.
    if (SIZE_NDC == 0) {
        s *= 2. / vp;
    }
    // If the size is in NDC, we need to rescale it to take the margins into account.
    else {

        s *= vp / viewport.size;
    }


    // Then, we take into account the optional rescaling in the image size.

    // Image rescaling: also affects the vertex displacement.
    vec2 zoom = vec2(1, 1);

    // Keep aspect ratio.
    if (RESCALE == 1)
    {
        zoom.x = total_zoom();
        zoom.y = zoom.x;
        if (SIZE_NDC != 0) {
            zoom.y *= k;
        }
    }

    // Do not keep aspect ratio.
    else if (RESCALE == 2)
    {
        vec3 az = axis_zoom();
        if ((TRANSFORM_FLAGS & DVZ_TRANSFORM_FIXED_X) == 0)
            zoom.x = az.x;
        if ((TRANSFORM_FLAGS & DVZ_TRANSFORM_FIXED_Y) == 0)
            zoom.y = az.y;
    }

    s *= zoom;


    // Finally, compute the quad vertex displacement depending on the anchor and the computed size.

    // The quad vertex index, between 0 and 5:
    // top left, bottom left, bottom right, bottom right, top right, top left.
    int idx = gl_VertexIndex % 6;

    // Normalized vertex displacement for quad triangulation.
    vec2 t = ds[idx];

    // Computing the final displacement in NDC.
    // anchor is in [-1, +1]
    vec2 a = anchor;

    // if anchor == [0, 0], the image is centered around its position
    vec2 d = s * (t - .5 * a);

    // Now, the vertex displacement d should be in NDC.
    tr.xy += d;


    // NOTE: we are here in Vulkan CDS with y reversed. For the image vertex displacement, we
    // prefer to do it in the OpenGL CDS with y up, so we temporarily go back to this CDS
    // and will revert to Vulkan CDS with y down at the end of the vertex shader.
    tr.y = -tr.y;
    gl_Position = tr;


    // Varyings.
    out_uv = uv;

    // The fragment shader expects the size in pixels, whereas "s" is in NDC here.
    out_size.xy = s * vp / 2.0;
    out_size.z = .5 * (zoom.x + zoom.y);

    out_color = facecolor;
}
