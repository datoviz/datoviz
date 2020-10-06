#version 450
#include "common.glsl"

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 tex_coords;

layout (location = 0) out vec3 out_tex_coords;

void main() {
    out_tex_coords = tex_coords;
    gl_Position = transform_pos(pos);
}
