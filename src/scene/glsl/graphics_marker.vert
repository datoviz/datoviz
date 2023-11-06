#version 450
#include "common.glsl"
#include "constants.glsl"

layout(location = 0) in vec3 pos;
layout(location = 1) in float size;
layout(location = 2) in float angle;
layout(location = 3) in vec4 color;

layout(location = 0) out vec4 out_color;
layout(location = 1) out float out_size;
layout(location = 2) out float out_angle;

void main()
{
    gl_Position = transform(pos);

    out_color = color;
    out_size = size;
    out_angle = angle;

    gl_PointSize = size * (abs(cos(angle)) + abs(sin(angle)));
}
