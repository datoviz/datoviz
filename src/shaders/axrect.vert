#version 450
#include "common.glsl"

layout (location = 0) in vec3 pos;  // NOTE: pos.z is span_axis (0 or 1)
layout (location = 1) in vec4 color;

layout (location = 0) out vec3 out_pos;
layout (location = 1) out vec4 out_color;

void main() {
    float span_axis = pos.z;
    vec3 pos_ = pos;
    pos_.z = 0;
    gl_Position = transform_pos(pos_);
    if (span_axis < .5) gl_Position.x = pos.x;
    else if (span_axis > .5) gl_Position.y = pos.y;
    out_color = color;
}
