#version 450
#include "common.glsl"

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec2 in_uv;

layout (location = 0) out vec4 out_color;


void main() {
    out_color = texture(color_texture, in_uv);
    out_color.a = 1;
}
