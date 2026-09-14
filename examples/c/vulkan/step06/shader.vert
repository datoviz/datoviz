#version 450

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 0) out vec3 color;

void main()
{
    gl_Position = vec4(in_position, 0.0, 1.0);
    color = in_color;
}
