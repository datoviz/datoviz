#version 450

#include "common.glsl"

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec4 inColor;
layout(location = 2) in float inSize;
layout(location = 5) in uint inSelection;

layout(location = 0) out vec4 fragColor;

void main()
{
    float selected = inSelection != 0u ? 1.0 : 0.0;
    float dim = mix(0.25, 1.0, selected);
    gl_Position = transform(inPos);
    gl_PointSize = inSize;
    fragColor = vec4(inColor.rgb * dim, mix(inColor.a * 0.25, 1.0, selected));
}
