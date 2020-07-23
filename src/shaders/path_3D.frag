#version 450
#include "common.glsl"
#include "antialias.glsl"

layout (binding = 2) uniform PathParams {
    float linewidth;
} params;

layout (location = 0) in vec4 in_color;
layout (location = 1) in float in_transverse;

layout (location = 0) out vec4 out_color;


void main()
{
    vec4 color = in_color;
    // Minimal basic shading.
    out_color = mix(in_color, vec4(0, 0, 0, 1), asin(in_transverse));
}
