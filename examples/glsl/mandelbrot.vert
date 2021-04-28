#version 450
#include "common.glsl"

layout (location = 0) in vec3 pos;

layout (location = 0) out vec2 out_coords;

void main() {
    // gl_Position = vec4(pos, 1.0);
    gl_Position = transform(pos);
    out_coords = pos.xy;
}
