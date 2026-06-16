#version 450

#include "color.glsl"

layout(location = 0) in vec4 fragColor;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = semanticColorToLinear(fragColor);
}
