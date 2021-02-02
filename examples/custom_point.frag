#version 450
#include "../src/glsl/common.glsl"

layout (location = 0) in vec4 in_color;
layout (location = 0) out vec4 out_color;

void main()
{
    CLIP
    out_color = in_color;
}
