#version 450
#include "common2.glsl"

layout (std140, binding = USER_BINDING) uniform Params {
    float point_size;
} params;

layout (location = 0) in vec3 pos;
layout (location = 1) in vec4 color;

layout (location = 0) out vec4 out_color;

void main() {
    gl_Position = transform(pos);
    out_color = color;
    gl_PointSize = params.point_size;
}
