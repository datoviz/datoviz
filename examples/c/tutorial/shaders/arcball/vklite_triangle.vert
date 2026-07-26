#version 450

layout(push_constant) uniform Transform
{
    mat4 mvp;
}
transform;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 0) out vec3 vertex_color;

void main()
{
    gl_Position = transform.mvp * vec4(position, 1.0);
    vertex_color = color;
}
