#version 450

#include "common.glsl"

layout(location = 0) in vec3 inPos;
layout(location = 2) in float inSize;
layout(location = 0) flat out uint fragId;
layout(location = 1) out float fragSize;

void main()
{
    gl_Position = transform(inPos);
    gl_PointSize = max(inSize + 4.0, 1.0);
    fragId = uint(gl_VertexIndex) + 1u;
    fragSize = inSize;
}
