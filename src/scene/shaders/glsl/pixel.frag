#version 450

#include "color.glsl"
#include "surface_depth.glsl"

layout(location = 0) in vec4 fragColor;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = semanticColorToLinear(fragColor);
#ifdef DVZ_SURFACE_DEPTH_OUTPUT
    writeSurfaceDepthFromDevice();
#endif
}
