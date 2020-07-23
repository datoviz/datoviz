#version 450
#include "common.glsl"
#include "antialias.glsl"
#include "markers.glsl"

layout (binding = 1) uniform sampler2D texture_sampler;

layout (location = 0) in vec2 in_tex_coords;
layout (location = 1) in vec4 in_face_color;
layout (location = 2) in vec4 in_edge_color;
layout (location = 3) in vec2 in_size;
layout (location = 4) in float in_linewidth;

layout (location = 0) out vec4 out_color;

void main()
{
    vec2 P = in_tex_coords - vec2(0.5, 0.5);
    vec2 size = in_size * (VIEWPORT).zw;
    P = P * (size + 2 * in_linewidth + antialias);
    float d = max(abs(P.x) - size.x / 2.0, abs(P.y) - size.y / 2.0);
    out_color = texture(texture_sampler, in_tex_coords);
    out_color.a = 1;
    if (in_linewidth > 0) {
        vec4 color = vec4(1, 1, 1, 1);
        out_color *= outline(d, in_linewidth, in_edge_color, color);
    }
}
