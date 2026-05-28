#version 450

#include "common.glsl"

layout(location = 0) in vec3 inPos;
layout(location = 1) in uint inId;

layout(location = 0) flat out uint fragId;

void main()
{
    gl_Position = transform(inPos);
    fragId = inId;
}
