#version 450

layout(set = 0, binding = 0) uniform sampler2D albedo;
layout(location = 0) in vec3 vertex_color;
layout(location = 1) in vec2 vertex_texcoord;
layout(location = 0) out vec4 out_color;

void main()
{
    out_color = texture(albedo, vertex_texcoord) * vec4(vertex_color, 1.0);
}
