#version 450

layout(location = 0) in vec3 pos;
layout(location = 1) in vec4 color;

layout(location = 0) out vec4 out_color;

layout(constant_id = 0) const float RED = 1.0;
layout(constant_id = 1) const float GREEN = 0.0;
layout(constant_id = 2) const float BLUE = 0.0;

void main()
{
    gl_Position = vec4(pos, 1.0);
    out_color = vec4(RED, GREEN, BLUE, 1);
}
