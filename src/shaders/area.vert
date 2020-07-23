#version 450
#include "common.glsl"

layout (location = 0) in vec3 pos;
layout (location = 1) in vec4 color;
layout (location = 2) in uint area_idx;

layout (location = 0) out vec3 out_pos;
layout (location = 1) out vec4 out_color;
layout (location = 2) out float out_area_idx;

void main() {
    gl_Position = transform_pos(pos);
    out_color = color;
    out_area_idx = float(area_idx);
}
