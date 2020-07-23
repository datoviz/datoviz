#version 450

layout (binding = 1) uniform sampler3D texture_sampler;

layout (location = 0) in vec3 tex_coords;

layout (location = 0) out vec4 out_color;

void main()
{
    float value = texture(texture_sampler, tex_coords).r;
    out_color = vec4(value, value, value, 1.0);
}
