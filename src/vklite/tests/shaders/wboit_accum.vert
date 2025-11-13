#version 450

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec4 in_color;

layout(location = 0) out vec4 v_color;

void main()
{
    gl_Position = vec4(in_pos, 1.0);
    v_color = in_color;
}
