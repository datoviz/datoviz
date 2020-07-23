#version 450
#include "common.glsl"

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec4 in_color;
layout (location = 2) in float in_area_idx;

layout (location = 0) out vec4 out_color;


void main() {
    if (fract(in_area_idx) > 0) discard;
    out_color = in_color;
}
