#version 450
#include "common.glsl"

layout (location = 0) in flat vec4 in_color;

layout (location = 0) out vec4 out_color;


void main() {
    out_color = in_color;
}
