#version 450
#include "../../src/shaders/common.glsl"

layout (location = 0) in vec3 pos;

layout (location = 0) out vec2 out_coords;

void main() {
    gl_Position = transform_pos(pos);
    out_coords = pos.xy;
}
