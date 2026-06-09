#version 450

#include "common.glsl"
#include "item_state_style.glsl"

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec4 inColor;
layout(location = 7) in uint inItemState;

layout(location = 0) out vec4 fragColor;

void main()
{
    gl_Position = transform(inPos);
    fragColor = applyItemStateColor(inColor, inItemState);
}
