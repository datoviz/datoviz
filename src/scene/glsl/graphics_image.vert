#version 450
#include "common.glsl"

layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 uv;

layout(location = 0) out vec2 out_uv;

void main()
{
    gl_Position = transform(vec3(pos, 0));
    out_uv = uv;
}
