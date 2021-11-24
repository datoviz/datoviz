#version 450
#include "common.glsl"

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 uv;

layout (location = 0) out vec2 out_uv;

void main() {
    gl_Position = transform(pos);
    out_uv = uv;
}
