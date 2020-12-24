#version 450
#include "antialias.glsl"
#include "markers.glsl"
#include "common.glsl"

layout (binding = USER_BINDING) uniform MarkersParams {
    vec4 edge_color;
    float edge_width;
    int enable_depth;
} params;

layout(location = 0) in vec4 color;
layout(location = 1) in float size;
layout(location = 2) in float marker;
layout(location = 3) in float angle;

layout(location = 0) out vec4 out_color;


void main() {
    CLIP

    vec2 P = gl_PointCoord.xy - vec2(0.5, 0.5);
    mat2 rot = mat2(cos(angle), sin(angle), -sin(angle), cos(angle));
    P = rot * P;
    float distance = select_marker(P * (size + 2 * params.edge_width + antialias), size, marker);
    out_color = outline(distance, params.edge_width, params.edge_color, color);
}
