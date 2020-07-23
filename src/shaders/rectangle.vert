#version 450
#include "common.glsl"

layout (location = 0) in vec3 center;
layout (location = 1) in vec2 size;
layout (location = 2) in vec4 face_color;
layout (location = 3) in vec4 edge_color;
layout (location = 4) in float linewidth;

layout (location = 0) out vec2 out_tex_coords;
layout (location = 1) out vec4 out_face_color;
layout (location = 2) out vec4 out_edge_color;
layout (location = 3) out vec2 out_size;
layout (location = 4) out float out_linewidth;

void main() {
    vec2 size_tr = size;
    size_tr.y = -size_tr.y;
    vec3 p0 = vec3(center.xy - size_tr / 2, center.z);
    vec3 p1 = vec3(center.xy + size_tr / 2, center.z);

    int index = gl_VertexIndex % 6;
    size_tr = transform_pos(p1).xy - transform_pos(p0).xy;

    // Rectangle triangulation.
    vec3 P = vec3(0, 0, .5 * (p0.z + p1.z));  // NOTE: take the half depth
    out_tex_coords = vec2(0, 0);
    switch (index) {
        case 0:
            P.xy = p0.xy;
            out_tex_coords = vec2(0, 0);
            break;
        case 1:
            P.xy = vec2(p1.x, p0.y);
            out_tex_coords = vec2(1, 0);
            break;
        case 2:
            P.xy = p1.xy;
            out_tex_coords = vec2(1, 1);
            break;
        case 3:
            P.xy = p1.xy;
            out_tex_coords = vec2(1, 1);
            break;
        case 4:
            P.xy = vec2(p0.x, p1.y);
            out_tex_coords = vec2(0, 1);
            break;
        case 5:
            P.xy = p0.xy;
            out_tex_coords = vec2(0, 0);
            break;
        default:
            break;
    }

    gl_Position = transform_pos(P);

    out_face_color = face_color;
    out_edge_color = edge_color;
    out_size = size_tr;
    out_linewidth = linewidth;
}
