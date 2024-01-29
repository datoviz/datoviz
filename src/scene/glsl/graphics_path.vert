// Copyright (c) 2009-2016 Nicolas P. Rougier. All rights reserved.
// Distributed under the (new) BSD License.
// Modifications by Cyrille Rossant for Datoviz, 2021

#version 450
#include "common.glsl"

layout(std140, binding = USER_BINDING) uniform Params
{
    float linewidth;
    float miter_limit;
    int cap_type;
    int round_join;
}
params;

const float antialias = 1.0;

layout(location = 0) in vec3 p0_ndc;
layout(location = 1) in vec3 p1_ndc;
layout(location = 2) in vec3 p2_ndc;
layout(location = 3) in vec3 p3_ndc;
layout(location = 4) in vec4 color;

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec2 out_caps;
layout(location = 2) out float out_length;
layout(location = 3) out vec2 out_texcoord;
layout(location = 4) out vec2 out_bevel_distance;


float compute_u(vec2 p0, vec2 p1, vec2 p)
{
    // Projection p' of p such that p' = p0 + u*(p1-p0)
    // Then  u *= lenght(p1-p0)
    vec2 v = p1 - p0;
    float l = length(v);
    return ((p.x - p0.x) * v.x + (p.y - p0.y) * v.y) / l;
}

float line_distance(vec2 p0, vec2 p1, vec2 p)
{
    // Projection p' of p such that p' = p0 + u*(p1-p0)
    vec2 v = p1 - p0;
    float l2 = v.x * v.x + v.y * v.y;
    float u = ((p.x - p0.x) * v.x + (p.y - p0.y) * v.y) / l2;

    // h is the projection of p on (p0,p1)
    vec2 h = p0 + u * v;

    return length(p - h);
}

void main()
{
    // // DEBUG
    // gl_PointSize = 50.0;
    // gl_Position = vec4(p1_ndc, 1);
    // return;

    int index = gl_VertexIndex % 4;


    mat4 ortho = get_ortho_matrix();
    mat4 ortho_inv = inverse(ortho);

    // Screen coordinates.
    vec4 p0_ = ortho_inv * transform(p0_ndc);
    vec4 p1_ = ortho_inv * transform(p1_ndc);
    vec4 p2_ = ortho_inv * transform(p2_ndc);
    vec4 p3_ = ortho_inv * transform(p3_ndc);

    vec2 p0 = p0_.xy / p0_.w;
    vec2 p1 = p1_.xy / p1_.w;
    vec2 p2 = p2_.xy / p2_.w;
    vec2 p3 = p3_.xy / p3_.w;
    float z = p1_.z / p1_.w;

    out_color = color;

    float linewidth = params.linewidth;
    float miter_limit = params.miter_limit;

    // Determine the direction of each of the 3 segments (previous, current, next)
    vec2 v0 = normalize(p1 - p0);
    vec2 v1 = normalize(p2 - p1);
    vec2 v2 = normalize(p3 - p2);

    // Determine the normal of each of the 3 segments (previous, current, next)
    vec2 n0 = vec2(-v0.y, v0.x);
    vec2 n1 = vec2(-v1.y, v1.x);
    vec2 n2 = vec2(-v2.y, v2.x);

    // Determine miter lines by averaging the normals of the 2 segments
    vec2 miter_a = normalize(n0 + n1); // miter at start of current segment
    vec2 miter_b = normalize(n1 + n2); // miter at end of current segment

    // Determine the length of the miter by projecting it onto normal
    vec2 p, v;
    float d;
    float w = linewidth / 2.0 + 1.5 * antialias;

    float length_a = w / dot(miter_a, n1);
    float length_b = w / dot(miter_b, n1);

    float m = miter_limit * linewidth / 2.0;

    // Angle between prev and current segment (sign only)
    float d0 = +1.0;
    if ((v0.x * v1.y - v0.y * v1.x) > 0)
    {
        d0 = -1.0;
    }

    // Angle between current and next segment (sign only)
    float d1 = +1.0;
    if ((v1.x * v2.y - v1.y * v2.x) > 0)
    {
        d1 = -1.0;
    }


    if (index == 0)
    {
        out_length = length(p2 - p1);
        // Cap at start
        if (p0 == p1)
        {
            p = p1 - w * v1 + w * n1;
            out_texcoord = vec2(-w, +w);
            out_caps.x = out_texcoord.x;
            // Regular join
        }
        else
        {
            p = p1 + length_a * miter_a;
            out_texcoord = vec2(compute_u(p1, p2, p), +w);
            out_caps.x = 1.0;
        }
        if (p2 == p3)
            out_caps.y = out_texcoord.x;
        else
            out_caps.y = 1.0;
        gl_Position = ortho * vec4(p, z, 1.0);
        out_bevel_distance.x = +d0 * line_distance(p1 + d0 * n0 * w, p1 + d0 * n1 * w, p);
        out_bevel_distance.y = -line_distance(p2 + d1 * n1 * w, p2 + d1 * n2 * w, p);
    }


    if (index == 1)
    { // || index == 3) {
        out_length = length(p2 - p1);
        // Cap at start
        if (p0 == p1)
        {
            p = p1 - w * v1 - w * n1;
            out_texcoord = vec2(-w, -w);
            out_caps.x = out_texcoord.x;
            // Regular join
        }
        else
        {
            p = p1 - length_a * miter_a;
            out_texcoord = vec2(compute_u(p1, p2, p), -w);
            out_caps.x = 1.0;
        }
        if (p2 == p3)
            out_caps.y = out_texcoord.x;
        else
            out_caps.y = 1.0;
        gl_Position = ortho * vec4(p, z, 1.0);
        out_bevel_distance.x = -d0 * line_distance(p1 + d0 * n0 * w, p1 + d0 * n1 * w, p);
        out_bevel_distance.y = -line_distance(p2 + d1 * n1 * w, p2 + d1 * n2 * w, p);
    }


    if (index == 2)
    { // || index == 4) {
        out_length = length(p2 - p1);
        // Cap at end
        if (p2 == p3)
        {
            p = p2 + w * v1 + w * n1;
            out_texcoord = vec2(out_length + w, +w);
            out_caps.y = out_texcoord.x;
            // Regular join
        }
        else
        {
            p = p2 + length_b * miter_b;
            out_texcoord = vec2(compute_u(p1, p2, p), +w);
            out_caps.y = 1.0;
        }
        if (p0 == p1)
            out_caps.x = out_texcoord.x;
        else
            out_caps.x = 1.0;
        gl_Position = ortho * vec4(p, z, 1.0);
        out_bevel_distance.x = -line_distance(p1 + d0 * n0 * w, p1 + d0 * n1 * w, p);
        out_bevel_distance.y = +d1 * line_distance(p2 + d1 * n1 * w, p2 + d1 * n2 * w, p);
    }


    if (index == 3)
    {
        out_length = length(p2 - p1);
        // Cap at end
        if (p2 == p3)
        {
            p = p2 + w * v1 - w * n1;
            out_texcoord = vec2(out_length + w, -w);
            out_caps.y = out_texcoord.x;
            // Regular join
        }
        else
        {
            p = p2 - length_b * miter_b;
            out_texcoord = vec2(compute_u(p1, p2, p), -w);
            out_caps.y = 1.0;
        }
        if (p0 == p1)
            out_caps.x = out_texcoord.x;
        else
            out_caps.x = 1.0;
        gl_Position = ortho * vec4(p, z, 1.0);
        out_bevel_distance.x = -line_distance(p1 + d0 * n0 * w, p1 + d0 * n1 * w, p);
        out_bevel_distance.y = -d1 * line_distance(p2 + d1 * n1 * w, p2 + d1 * n2 * w, p);
    }
}
