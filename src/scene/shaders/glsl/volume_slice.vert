#version 450

#include "common.glsl"

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inUVW;

layout(location = 0) out vec3 fragUVW;
layout(location = 1) out vec3 fragObj;

void main()
{
    gl_Position = transform(inPos);
    fragUVW = inUVW;
    fragObj = inPos;
}
