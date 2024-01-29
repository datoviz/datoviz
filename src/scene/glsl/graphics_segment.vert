#version 450
#include "common.glsl"
#include "constants.glsl"

layout(location = 0) in vec3 P0;
layout(location = 1) in vec3 P1;
layout(location = 2) in vec4 shift;
layout(location = 3) in vec4 color;
layout(location = 4) in float linewidth;
layout(location = 5) in int cap0;
layout(location = 6) in int cap1;
// layout(location = 7) in uint transform_flags;

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec2 out_texcoord;
layout(location = 2) out float out_length;
layout(location = 3) out float out_linewidth;
layout(location = 4) out float out_cap;

void main(void)
{
    out_color = color;
    out_linewidth = linewidth;

    int index = gl_VertexIndex % 4;

    vec4 P0_ = transform(P0, shift.xy); //, transform_flags);
    vec4 P1_ = transform(P1, shift.zw); //, transform_flags);

    // Viewport coordinates.
    mat4 ortho = get_ortho_matrix(viewport.size);
    mat4 ortho_inv = inverse(ortho);

    // if (TRANSFORM_FLAGS == 17)
    //     out_color.r = 0;

    vec4 p0 = ortho_inv * P0_;
    vec4 p1 = ortho_inv * P1_;

    // NOTE: we need to normalize by the homogeneous coordinates after converting into pixels.
    p0.xyz /= p0.w;
    p1.xyz /= p1.w;

    float z = p0.z;

    vec2 position = p0.xy;
    vec2 T = (p1 - p0).xy;
    out_length = length(T);
    float w = linewidth / 2.0 + 1.5 * antialias;
    T = w * normalize(T);

    if (index < 0.5)
    {
        position = vec2(p0.x - T.y - T.x, p0.y + T.x - T.y);
        out_texcoord = vec2(-w, +w);
        z = p0.z;
        out_cap = cap0;
    }
    else if (index < 1.5)
    {
        position = vec2(p0.x + T.y - T.x, p0.y - T.x - T.y);
        out_texcoord = vec2(-w, -w);
        z = p0.z;
        out_cap = cap0;
    }
    else if (index < 2.5)
    {
        position = vec2(p1.x + T.y + T.x, p1.y - T.x + T.y);
        out_texcoord = vec2(out_length + w, -w);
        z = p1.z;
        out_cap = cap1;
    }
    else
    {
        position = vec2(p1.x - T.y + T.x, p1.y + T.x + T.y);
        out_texcoord = vec2(out_length + w, +w);
        z = p1.z;
        out_cap = cap1;
    }

    gl_Position = ortho * vec4(position, z, 1.0);
}
