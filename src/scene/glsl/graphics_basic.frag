#version 450
#include "common.glsl"

layout(location = 0) in vec4 in_color;
layout(location = 0) out vec4 out_color;

void main()
{
    CLIP;

    out_color = in_color;
    // if (out_color.a < .01)
    //     discard;
}
