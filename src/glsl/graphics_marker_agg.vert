#version 450
#include "constants.glsl"
#include "common.glsl"

layout (location = 0) in vec3 pos;
layout (location = 1) in vec4 color;
layout (location = 2) in float size;
layout (location = 3) in uint marker;
layout (location = 4) in float angle;

layout (location = 0) out vec4 out_color;
layout (location = 1) out float out_size;
layout (location = 2) out float out_marker;
layout (location = 3) out float out_angle;

void main() {
    gl_Position = transform(pos);
    gl_PointSize = size;

    out_color = color;
    out_size = size;
    out_marker = marker;
    out_angle = angle * M_2PI;
}
