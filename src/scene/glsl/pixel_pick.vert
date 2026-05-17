#version 450

#include "common.glsl"

layout(location = 0) in vec3 inPos;
layout(location = 2) in float inSize;
layout(location = 0) flat out uint fragId;

void main()
{
    gl_Position = transform(inPos);
    gl_PointSize = inSize;
    fragId = uint(gl_VertexIndex) + 1u;
}
