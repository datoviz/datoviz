#version 450
#include "common.glsl"

layout(std140, binding = USER_BINDING) uniform MonoglyphParams { float size; /* point size */ }
params;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec4 color;
layout(location = 2) in float group;

layout(location = 0) out vec4 out_color;
layout(location = 1) out float out_group;

void main()
{
    gl_Position = transform(pos);
    out_color = color;
    out_group = group;
    gl_PointSize = params.size;
}
