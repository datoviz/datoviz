#version 450

#include "common.glsl"
#include "selection_style.glsl"

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec4 inColor;
layout(location = 2) in float inSize;
layout(location = 5) in uint inItemState;

layout(location = 0) out vec4 fragColor;

void main()
{
    gl_Position = transform(inPos);
    gl_PointSize = applyItemStateScale(inSize, inItemState);
    fragColor = applyItemStateColor(inColor, inItemState);
}
