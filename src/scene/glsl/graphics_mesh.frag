/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

#version 450
#include "common.glsl"
#include "constants.glsl"
#include "params_mesh.glsl"

layout(constant_id = 0) const int MESH_TEXTURED = 0; // 1 to enable
layout(constant_id = 1) const int MESH_LIGHTING = 0; // 1 to enable
layout(constant_id = 2) const int MESH_CONTOUR = 0;  // 1 to enable
layout(constant_id = 3) const int MESH_ISOLINE = 0;  // 1 to enable

const float eps = .00001;
const float antialias_ = 2 * antialias;

// Varying variables.
layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec4 in_uvcolor;
layout(location = 3) in vec3 in_barycentric;
layout(location = 4) in vec3 in_d_left;
layout(location = 5) in vec3 in_d_right;
layout(location = 6) flat in ivec3 in_contour;
layout(location = 7) in float in_isoline;

layout(location = 0) out vec4 out_color;

layout(binding = (USER_BINDING + 1)) uniform sampler2D tex;

// Replacement for fwidth, using L2 norm of gradient instead of L1 norm.
float fwidth2(float p) { return sqrt(pow(abs(dFdx(p)), 2) + pow(abs(dFdy(p)), 2)); }
vec2 fwidth2(vec2 p) { return sqrt(pow(abs(dFdx(p)), vec2(2)) + pow(abs(dFdy(p)), vec2(2))); }
vec3 fwidth2(vec3 p) { return sqrt(pow(abs(dFdx(p)), vec3(2)) + pow(abs(dFdy(p)), vec3(2))); }

// Vertex corner between the left and right edge.
float one_corner(float d_left, float d_right, int orient, float linewidth)
{
    float scale = linewidth;
    vec2 d = vec2(d_left, d_right);
    vec2 deltas = fwidth2(d);                     // rate of change of the distances
    float a = d.x / deltas.x;                     // normalized distance to left edge
    float b = d.y / deltas.y;                     // normalized distance to right edge
    float c = orient > 0 ? max(a, b) : min(a, b); // take min or max of the distance
    // return smoothstep(scale, scale + 1, c);      // 0 on contour, 1 inside the polygon
    return c <= scale ? 0 : 1;
}

float corner(vec3 d_left, vec3 d_right, ivec3 contour, float linewidth)
{
    ivec3 corner = ((contour >> 1) & 1);
    ivec3 orient = ((contour >> 2) & 1);

    float res = 1;

    if (corner.x > 0)
    {
        res = min(res, one_corner(d_left.x, d_right.x, orient.x, linewidth));
    }
    if (corner.y > 0)
    {
        res = min(res, one_corner(d_left.y, d_right.y, orient.y, linewidth));
    }
    if (corner.z > 0)
    {
        res = min(res, one_corner(d_left.z, d_right.z, orient.z, linewidth));
    }
    return res;
}

vec2 edge(vec3 barycentric, vec3 deltas, ivec3 edge, float linewidth)
{
    // cf https://web.archive.org/web/20190220052115/http://codeflow.org/entries/2012/aug/02/
    // easy-wireframe-display-with-barycentric-coordinates/
    // cf https://catlikecoding.com/unity/tutorials/advanced-rendering/flat-and-wireframe-shading/

    float scale = linewidth;
    vec3 a = deltas * scale;
    vec3 b = deltas * (scale + antialias_);

    vec3 stepped = smoothstep(a, b, barycentric);
    float x = stepped.x;
    float y = stepped.y;
    float z = stepped.z;

    float res = 1;
    float alpha = 100000;

    if (edge.x > 0)
    {
        res = min(res, x);
        alpha = min(alpha, barycentric.x / deltas.x);
    }
    if (edge.y > 0)
    {
        res = min(res, y);
        alpha = min(alpha, barycentric.y / deltas.y);
    }
    if (edge.z > 0)
    {
        res = min(res, z);
        alpha = min(alpha, barycentric.z / deltas.z);
    }
    return vec2(res, alpha);
}

// Isolines.
// cf https://observablehq.com/@rreusser/locally-scaled-domain-coloring-part-1-contour-plots
float logContours(float f, float spacing, float width)
{
    float plotVar = log2(abs(f)) * spacing;
    float screenSpaceGradient = length(vec2(dFdx(f), dFdy(f))) / abs(f) * spacing;
    width *= 2;
    return smoothstep(
        width + antialias_, width, (0.5 - abs(fract(plotVar) - 0.5)) / screenSpaceGradient);
}

// cf https://observablehq.com/@rreusser/contour-plots-with-d3-regl-and-observable
float contourFunction(float parameter, float width, float feather)
{
    float w1 = width - feather * 0.5;
    float d = length(vec2(dFdx(parameter), dFdy(parameter)));
    float looped = 0.5 - abs(mod(parameter, 1.0) - 0.5);
    return smoothstep(d * (w1 + feather), d * w1, looped);
}



void main()
{
    CLIP;

    // // DEBUG
    // out_color = in_uvcolor;
    // out_color.rgb *= out_color.a;
    // return;

    // if (in_clip < -eps)
    //     discard;

    vec3 normal, light_dir, light_color, ambient, diffuse, view_dir, reflect_dir, specular, color;
    vec4 lpar;
    vec3 lpos;
    float diff, spec, view_facing;

    // Stroke parameters.
    float linewidth = params.stroke.a;
    vec3 stroke = params.stroke.rgb;
    vec3 pos_tr;

    normal = normalize(in_normal);
    out_color = vec4(0, 0, 0, 1);
    diffuse = vec3(0);
    specular = vec3(0);

    // Texture.
    if (MESH_TEXTURED > 0)
    {
        // in this case, in_uvcolor.xy is uv coordinates
        color = texture(tex, in_uvcolor.xy).xyz;
    }
    // Color.
    else
    {
        color = in_uvcolor.xyz; // rgb
    }

    // Lighting.
    if (MESH_LIGHTING > 0)
    {
        pos_tr = ((mvp.model * vec4(in_pos, 1.0))).xyz;
        view_dir = normalize(-mvp.view[3].xyz - pos_tr);

        for (int i = 0; i < 4; i++)
        {
            // Light position and params.
            lpar = params.light_params[i];
            if (length(lpar) == 0)
                continue;

            // Light direction.
            // light_dir = normalize(lpos - pos_tr);
            // TODO OPTIM: normalize on the CPU instead, in dvz_mesh_light_dir()
            light_dir = normalize(params.light_dir[i].xyz);

            // Color.
            light_color = params.light_color[i].xyz;
            ambient = light_color;

            // Diffuse component.
            view_facing = max(dot(normal, view_dir), 0.0);
            diff = max(dot(normal, -light_dir), 0.0);
            // HACK: normals on both faces
            // diff = max(diff, max(dot(light_dir, -normal), 0.0));
            diffuse = diff * view_facing * light_color;

            // Specular component.
            reflect_dir = reflect(light_dir, normal);
            spec = pow(max(dot(view_dir, reflect_dir), 0.0), lpar.w);
            specular = spec * view_facing * light_color;

            // Total color.
            out_color.xyz += (lpar.x * ambient + lpar.y * diffuse + lpar.z * specular) * color;
        }

        // by convention, alpha channel is in 4th component of this attribute
        out_color.a = in_uvcolor.a;
    }
    else
    {
        // NOTE: the 4th component of in_uvcolor is always the alpha channel, both in the
        // color case (rgba) or the uv tex case (uv*a).
        out_color = vec4(color, in_uvcolor.a);
    }

    // Stroke.
    if (MESH_CONTOUR > 0)
    {
        // Contour information.
        ivec3 bedge = (in_contour >> 0) & 1;

        // Barycentric coordinates scale.
        vec3 deltas = fwidth2(in_barycentric);

        // Edges.
        vec2 ea = vec2(1);
        if (bedge.x > 0 || bedge.y > 0 || bedge.z > 0)
            ea = edge(in_barycentric, deltas, bedge, linewidth);
        float e = ea.x;
        float alpha = ea.y;

        // Corners.
        float c = corner(in_d_left, in_d_right, in_contour, linewidth);

        // Final color.
        out_color.rgb = mix(stroke, out_color.rgb, min(e, c));

        // // Antialiasing.
        // if (c == 0 && e > 0)
        // {
        //     float min_bary = min(min(in_barycentric.x, in_barycentric.y), in_barycentric.z);
        //     float aa = fwidth2(min_bary);
        //     alpha = smoothstep(0.0, aa, min_bary);
        // }
        // out_color.a *= alpha;
    }

    // Isoline.
    if (MESH_ISOLINE > 0)
    {
        // Calculate the normalized distance to the nearest contour line
        float value = in_isoline; //(1 + in_pos.y)
        float isoline = logContours(value, params.isoline_count, linewidth);
        out_color.rgb = mix(out_color.rgb, stroke, isoline);
    }
}
