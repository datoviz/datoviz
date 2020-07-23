#version 450
#include "common.glsl"

layout (location = 0) in vec3 pos;
layout (location = 1) in vec4 color;

layout (location = 0) out flat vec4 out_color;

void main() {
    gl_Position = transform_pos(pos);
    out_color = color;
}
